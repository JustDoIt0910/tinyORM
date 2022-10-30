#include "sqlconn.h"
#include "sqlresult.h"
#include "connectionpool.h"
#include "databasemetadata.h"

int SqlConn::globalId = 1;
mutex SqlConn::mu;

void close(MYSQL* sql)
{
	if (sql)
		mysql_close(sql);
}

SqlConn::SqlConn(): ptr(unique_ptr<MYSQL, CloseFunc>(nullptr, ::close)), pool(nullptr){}

//SqlConn::SqlConn(const SqlConn& conn): ptr(unique_ptr<MYSQL, CloseFunc>(nullptr, ::close)), pool(nullptr){}

SqlConn::SqlConn(MYSQL* mysql, ConnectionPool* _pool):
	ptr(unique_ptr<MYSQL, CloseFunc>(mysql, ::close)), pool(_pool)
{
	lock_guard<mutex> lg(mu);
	id = globalId++;
}

SqlConn::SqlConn(SqlConn&& conn) :
	ptr(conn.ptr.release(), ::close),
	error(conn.error),
	id(conn.id)
{
	pool = conn.pool;
	conn.pool = nullptr;
}

SqlConn& SqlConn::operator=(SqlConn&& conn)
{
	if (this == &conn)
		return *this;
    this->id = conn.id;
	this->ptr.reset(conn.ptr.release());
	this->error = conn.error;
	this->pool = conn.pool;
	conn.pool = nullptr;
	return *this;
}

MYSQL* SqlConn::get()
{
	return ptr.get();
}

int SqlConn::getId()
{
	return id;
}

bool SqlConn::isOpen()
{
	return ptr.get() != NULL;
}

void SqlConn::setInternalError()
{
	const char* err = mysql_error(ptr.get());
	error = string(err);
}

void SqlConn::setError(const string& err)
{
	error = err;
}

string SqlConn::getError()
{
	return error;
}

Statement SqlConn::prepareStatement(const string& sql)
{
	return Statement(mysql_stmt_init(ptr.get()), sql);
}

ResultSet SqlConn::executeQuery(const string& sql)
{
	mysql_real_query(ptr.get(), sql.c_str(), sql.length());
	MYSQL_RES* res = mysql_store_result(ptr.get());
	if (res == nullptr)
		return ResultSet(nullptr, false);
	return ResultSet(new SqlResult(res), true);
}

unsigned long long SqlConn::executeUpdate(const string& sql)
{
	mysql_real_query(ptr.get(), sql.c_str(), sql.length());
	return mysql_affected_rows(ptr.get());
}

bool SqlConn::setAutocommit(bool mode)
{
	if (mysql_autocommit(ptr.get(), mode ? 1 : 0))
		return false;
	return true;
}

bool SqlConn::commit()
{
	if (mysql_commit(ptr.get()))
		return false;
	return true;
}

bool SqlConn::rollback()
{
	if (mysql_rollback(ptr.get()))
		return false;
	return true;
}

bool SqlConn::testConn(const string& testSql, int expect)
{
	try {
		mysql_real_query(ptr.get(), testSql.c_str(), testSql.length());
		MYSQL_RES* res = mysql_store_result(ptr.get());
		int val = atoi(res->data->data->data[0]);
		return val == expect;
	}
	catch (exception& e) {
		return false;
	}
}

void SqlConn::close()
{
	if (pool)
		pool->releaseConn(this);
}

DatabaseMetadata SqlConn::getMetadata()
{
    MYSQL* mysql = new MYSQL;
    mysql_init(mysql);
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "gbk");
    if (!mysql_real_connect(mysql, pool->getHost().c_str(),
                            pool->getUsername().c_str(), pool->getPassword().c_str(),
                            "information_schema", pool->getPort(), NULL, 0))
    {
        delete mysql;
        return DatabaseMetadata(SqlConn(nullptr, nullptr));
    }
    return DatabaseMetadata(SqlConn(mysql, nullptr));
}