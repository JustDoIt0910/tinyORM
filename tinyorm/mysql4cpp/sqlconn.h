#pragma once
#include <memory>
#include "mysql.h"
#include <string>
#include <iostream>
#include <mutex>
#include "statement.h"
using namespace std;

typedef void (*CloseFunc)(MYSQL*);
class ConnectionPool;

class DatabaseMetadata;

class SqlConn
{
public:
	SqlConn();
	SqlConn(MYSQL* mysql, ConnectionPool* pool);
	SqlConn(SqlConn&& conn);
	SqlConn& operator=(SqlConn&& conn);
	SqlConn& operator=(const SqlConn&) = delete;

	MYSQL* get();
	int getId();
	bool isOpen();
	void setError(const string& err);
	void setInternalError();
	string getError();
	Statement prepareStatement(const string& sql);
	ResultSet executeQuery(const string& sql);
	unsigned long long executeUpdate(const string& sql);
	bool setAutocommit(bool);
	bool commit();
	bool rollback();
	bool testConn(const string& testSql, int expect);
	void close();

    DatabaseMetadata getMetadata();

private:
	unique_ptr<MYSQL, CloseFunc> ptr;
	string error;
	int id;
	ConnectionPool* pool;

	static mutex mu;
	static int globalId;
};

