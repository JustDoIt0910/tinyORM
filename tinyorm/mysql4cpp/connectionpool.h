#pragma once
#include "sqlconn.h"
#include <mutex>
#include <unordered_set>
#include <condition_variable>
using namespace std;

class ConnectionPool
{
public:
	SqlConn getConn();
	void releaseConn(SqlConn* conn);
	void configure(const string& host, const string& _username, const string& _password,
		int _port, const string& _db, int _poolSize,
		int _maxWait, int _maxRetry,
		bool _testOnBorrow = true, const string& _testSql = "select 1");
	static ConnectionPool* getInstance();

private:
	SqlConn newConnection();
	static ConnectionPool* instance;
	static mutex instanceMu;
	mutex mu;
	condition_variable cond;
	unordered_set<int> idleConnections;
	unordered_set<int> usedConnections;
	unordered_map<int, SqlConn> connections;
	int maxWait;
	int waitInterval;
	int maxRetry;
	int poolSize;
	bool testOnBorrow;
	string testSql;
	string host;
	string username;
	string password;
	int port;
	string db;
};

