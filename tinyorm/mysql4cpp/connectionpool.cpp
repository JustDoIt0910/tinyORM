#include "connectionpool.h"
#include "spdlog.h"
#include <chrono>

ConnectionPool* ConnectionPool::instance;
mutex ConnectionPool::instanceMu;

void ConnectionPool::configure(const string& _host, const string& _username, const string& _password, int _port, 
	const string& _db, int _poolSize, int _maxWait, int _maxRetry, bool _testOnBorrow, const string& _testSql)
{
	host = _host;
	username = _username;
	password = _password;
	port = _port;
	db = _db;
	poolSize = _poolSize;
	maxWait = _maxWait;
	maxRetry = _maxRetry;
	waitInterval = maxWait / maxRetry;
	testOnBorrow = _testOnBorrow;
	testSql = _testSql;
}

ConnectionPool* ConnectionPool::getInstance()
{
	if (!instance)
	{
		lock_guard<mutex> lg(instanceMu);
		if (!instance)
			instance = new ConnectionPool();
	}
	return instance;
}

SqlConn ConnectionPool::getConn()
{
	unique_lock<mutex> lk(mu);
	int total = idleConnections.size() + usedConnections.size();
	for (int i = 0; i < maxRetry && idleConnections.size() == 0 && total == poolSize; i++)
	{
		cond.wait_for(lk, chrono::milliseconds(waitInterval));
		total = idleConnections.size() + usedConnections.size();
	}
	if (idleConnections.size() > 0)
	{
		int connId = *idleConnections.begin();
		SqlConn conn = move(connections.at(connId));
		idleConnections.erase(connId);
		connections.erase(connId);
		usedConnections.insert(connId);
		spdlog::info("reuse idle connection#{0}, total {1}", connId, idleConnections.size() + usedConnections.size());
		return conn;
	}
	else if (total < poolSize)
	{
		SqlConn conn = newConnection();
		if (!conn.isOpen())
			return conn;
		if (testOnBorrow && !conn.testConn(testSql, 1))
		{
			conn.setError("sql test fail");
			spdlog::error("fail to create new connection(sql test fail)");
			return conn;
		}
		usedConnections.insert(conn.getId());
		spdlog::info("create new connection#{0}, total {1}", conn.getId(), idleConnections.size() + usedConnections.size());
		return conn;
	}
	else
	{
		spdlog::error("fail to create new connection");
		SqlConn invalid(nullptr, nullptr);
		invalid.setError("failed to create new connection");
		return invalid;
	}
}

void ConnectionPool::releaseConn(SqlConn* conn)
{
	unique_lock<mutex> lk(mu);
	if (usedConnections.find(conn->getId()) == usedConnections.end())
		return;
	connections.insert(make_pair<int, SqlConn>(conn->getId(), move(*conn)));
	usedConnections.erase(conn->getId());
	idleConnections.insert(conn->getId());
	spdlog::info("return connection#{0} total {1}", conn->getId(), idleConnections.size() + usedConnections.size());
}

SqlConn ConnectionPool::newConnection()
{
	MYSQL* mysql = new MYSQL;
	mysql_init(mysql);
	mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "gbk");
	if (!mysql_real_connect(mysql, host.c_str(),
		username.c_str(), password.c_str(),
		db.c_str(), port, NULL, 0))
	{
		delete mysql;
		SqlConn conn(nullptr, nullptr);
		conn.setInternalError();
		return conn;
	}
	return SqlConn(mysql, this);
}
