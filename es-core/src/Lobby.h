#pragma once

#include <boost/thread.hpp>

class LobbyThread {
public:
	LobbyThread();
	~LobbyThread();

	void startBroadcast(std::string gameHash);
	void stopBroadcast();

  static LobbyThread *getInstance();
private:
  static LobbyThread *instance;

	boost::thread *mThreadHandle;
	std::string m_gameHash;
	int mefd;
	int mtfd;
	int m_broadcast_fd;
	void handleTimeout();
	void handleIncomingBroadcast(std::string gameHash, std::string peer);
	void run();
};
