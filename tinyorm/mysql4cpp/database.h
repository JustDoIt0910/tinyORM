#pragma once
#include <string>
#include "mysql.h"
#include "sqlconn.h"
#include "connectionpool.h"
#include "databasemetadata.h"
#include <memory>
using namespace std;


class Database
{
public:
	const static string localhost;
	const static string defaultUser;
	const static int port = 3306;

	Database(const string& _db, const string& pwd, 
		const string& _host = localhost, 
		const string& _user = defaultUser,
		const string& _charset = "gbk");

	Database() = delete;
	Database(Database&) = delete;
	Database(Database&&) = delete;

	SqlConn getConn();

private:
	string host;
	string username;
	string password;
	string db;
	string charset;
	unique_ptr<ConnectionPool> pool;
};

