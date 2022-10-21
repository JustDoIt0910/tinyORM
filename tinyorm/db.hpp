#ifndef __ORM_DB_HPP__
#define __ORM_DB_HPP__
#include <string>
#include "model.hpp"
#include "mysql4cpp/connectionpool.h"
#include <vector>

namespace orm {
	class DB
	{
	public:
		DB(const std::string& _host, const int _port,
			const std::string& _username, const std::string& _password,
			const std::string& _db);

		template<typename T>
		Model<T> model();

		void setAutoCommit(bool mode);

	private:
		ConnectionPool* pool;
		std::string host;
		int port;
		std::string username;
		std::string password;
		std::string db;
		bool autoCommit;
	};

	DB::DB(const std::string& _host, const int _port,
		const std::string& _username, const std::string& _password,
		const std::string& _db) : host(_host), port(_port), username(_username), password(_password), db(_db)
	{
		autoCommit = false;
		pool = ConnectionPool::getInstance();
		pool->configure(host, username, password, port, db, 10, 1000, 5);
	}

	template<typename T>
	Model<T> DB::model()
	{
		return Model<T>(pool->getConn(), autoCommit);
	}

	void DB::setAutoCommit(bool mode)
	{
		autoCommit = mode;
	}
}


#endif