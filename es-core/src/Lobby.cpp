#include <iostream>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "Lobby.h"


static int make_socket_non_blocking ( int sfd )
{
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);

    if (flags == -1)
    {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);

    if (s == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

LobbyThread *LobbyThread::instance = NULL;


LobbyThread *LobbyThread::getInstance() {
    if (LobbyThread::instance == NULL) {
        LobbyThread::instance = new LobbyThread();
    }
    return LobbyThread::instance;
}

LobbyThread::LobbyThread() {
	mefd = epoll_create1(EPOLL_CLOEXEC);
	if (mefd < 0) {
		std::cerr << "epoll_create error: " << strerror(errno) << std::endl;
		exit(1);
	}

	mtfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (mtfd < 0) {
		std::cerr << "timerfd_create error: " << strerror(errno) << std::endl;
		exit(1);
	}

	struct epoll_event ev;
	memset(&ev, 0, sizeof(epoll_event));
	ev.events = EPOLLIN | EPOLLERR | EPOLLET;
	ev.data.fd = mtfd;
	epoll_ctl(mefd, EPOLL_CTL_ADD, mtfd, &ev);

	m_broadcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_broadcast_fd == -1) {
		std::cerr << "socket error: " << strerror(errno) << std::endl;
		exit(1);
	}

	int broadcast = 1;
	if((setsockopt(m_broadcast_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))) == -1) {
		std::cerr << "setsockopt SOL_SOCKET error: " << strerror(errno) << std::endl;
		exit(1);
	}
	make_socket_non_blocking(m_broadcast_fd);

	struct sockaddr_in srvaddr;        // Broadcast Server Address
	struct sockaddr_in dstaddr;        // Broadcast Destination Address

	memset( &srvaddr, 0, sizeof( srvaddr ) );
	memset( &dstaddr, 0, sizeof( dstaddr ) );

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(5601);
	srvaddr.sin_addr.s_addr = INADDR_ANY;

	if( bind(m_broadcast_fd, (struct sockaddr*) &srvaddr, sizeof(srvaddr)) == -1 ) {
		std::cerr << "bind error: " << strerror(errno) << std::endl;
		exit(1);
	}

	memset(&ev, 0, sizeof(epoll_event));
	ev.data.fd = m_broadcast_fd;
	ev.events = EPOLLIN | EPOLLET;
	epoll_ctl(mefd, EPOLL_CTL_ADD, m_broadcast_fd, &ev);

	mThreadHandle = new boost::thread(boost::bind(&LobbyThread::run, this));
}

LobbyThread::~LobbyThread() {
	// FIXME: Close mefd

	mThreadHandle->join();
}

void LobbyThread::startBroadcast(std::string gameHash) {
	m_gameHash = gameHash;

	struct itimerspec new_timeout;
	new_timeout.it_value.tv_sec = 1;
	new_timeout.it_value.tv_nsec = 0;
	new_timeout.it_interval.tv_sec = 1;
	new_timeout.it_interval.tv_nsec = 0;

	if (timerfd_settime(mtfd, 0, &new_timeout, NULL) != 0) {
		std::cerr << "timerfd_settime error: " << strerror(errno) << std::endl;
	}
}

void LobbyThread::stopBroadcast() {
	itimerspec new_timeout{{0}};
	if (timerfd_settime(mtfd, 0, &new_timeout, NULL) != 0) {
		std::cerr << "timerfd_settime error: " << strerror(errno) << std::endl;
	}
}

void LobbyThread::handleTimeout() {
	std::cerr << "TIME TO BROADCAST" << std::endl;

	struct sockaddr_in srvaddr;
	memset( &srvaddr, 0, sizeof( srvaddr ) );
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(5601);
	srvaddr.sin_addr.s_addr = INADDR_ANY;

	if (sendto(m_broadcast_fd, m_gameHash.c_str(), m_gameHash.length(), 0, ( struct sockaddr * )&srvaddr, sizeof(srvaddr)) < 0) {
		std::cerr << "sendto error: " << strerror(errno) << std::endl;
		return;
	}
}

void LobbyThread::handleIncomingBroadcast(std::string gameHash, std::string peer) {
		std::cerr << "SAW ANNOUNCEMENT FROM GAME" << gameHash << peer << std::endl;
}

void LobbyThread::run() {
	struct epoll_event events[128];
	int numEvents = 0;

	while (1) {
		numEvents = epoll_wait(mefd, events, 128, -1);
		if (numEvents < 0) {
			std::cerr << "An error occured: " << strerror(errno) << std::endl;
			return;
		}
		for (int i = 0; i < numEvents; ++i) {
			std::cerr << "An event occured: " << events[i].data.fd << std::endl;

			if (events[i].data.fd == mtfd) {
				char data[8];
				read(mtfd, &data, 8);
				lseek(mtfd, 0, SEEK_SET);
				this->handleTimeout();
			}

			if (events[i].data.fd == m_broadcast_fd) {
				char data[33];
				struct sockaddr peer;
				socklen_t peer_len = sizeof(peer);
				char peer_string[INET_ADDRSTRLEN];

				recvfrom(m_broadcast_fd, &data, sizeof(data), 0, &peer, &peer_len);
				inet_ntop(peer.sa_family, &(((struct sockaddr_in *)&peer)->sin_addr), peer_string, INET_ADDRSTRLEN);

				this->handleIncomingBroadcast(std::string(data), std::string(peer_string));
			}
		}
	}
}
