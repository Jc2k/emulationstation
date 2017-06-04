#pragma once

#include <time.h>

#include <boost/thread.hpp>


struct Session {
public:
	std::string peer;
	std::string gameHash;
	timespec lastSeen;
};

typedef std::function<void(Session *session)> PlayerStartedPlayingFunction;
typedef std::function<void(Session *session)> PlayerStoppedPlayingFunction;


class LobbyThread {
public:
	LobbyThread();
	~LobbyThread();

	void startBroadcast(std::string gameHash);
	void stopBroadcast();

	void subscribeStartedPlaying(PlayerStartedPlayingFunction callback);
	void subscribeStoppedPlaying(PlayerStoppedPlayingFunction callback);

  static LobbyThread *getInstance();
private:
  static LobbyThread *instance;


	std::map<std::string, Session*> mActiveSessions;
	std::vector<PlayerStartedPlayingFunction> mStartedPlayingCallbacks;
	std::vector<PlayerStoppedPlayingFunction> mStoppedPlayingCallbacks;

	boost::thread *mThreadHandle;
	std::string m_gameHash;
	int mefd;
	int mexpireFd;
	int mtfd;
	int m_broadcast_fd;

	void expireSessions();
	void handleTimeout();
	void handleIncomingBroadcast(std::string gameHash, std::string peer);
	void run();
};
