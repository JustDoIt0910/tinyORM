#include "database.h"

const string Database::localhost = "localhost";
const string Database::defaultUser = "root";

Database::Database(const string& _db, const string& pwd, const string& _host,
	const string& _user, const string& _charset) :
	db(_db), password(pwd), host(_host), username(_user), charset(_charset)
{
	pool.reset(ConnectionPool::getInstance());
	pool->configure(host, username, password, port, db, 20, 1000, 5);
}

SqlConn Database::getConn()
{
	return pool->getConn();
}