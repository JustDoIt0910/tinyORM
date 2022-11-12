#ifndef __ORM_DB_HPP__
#define __ORM_DB_HPP__
#include <string>
#include "model.hpp"
#include "dataloader.hpp"
#include "mysql4cpp/connectionpool.h"
#include <vector>
#include <fstream>

namespace orm {
	class DB
	{
	public:
		DB(const std::string& _host, const int _port,
			const std::string& _username, const std::string& _password,
			const std::string& _db);
        DB(const DB& _db);
        DB& operator=(const DB& _db);

		template<typename T>
		Model<T> model();

        SqlConn getConn();

		void setAutoCommit(bool mode);

        template<typename... Args>
        void AutoMigrate(Args&&... args);

        template<typename T>
        void doMigrate();

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
		autoCommit = true;
		pool = ConnectionPool::getInstance();
		pool->configure(host, username, password, port, db, 10, 1000, 5);
	}

    DB::DB(const DB& _db): pool(_db.pool), host(_db.host), port(_db.port),
        username(_db.username), password(_db.password), db(_db.db), autoCommit(_db.autoCommit){}

    DB& DB::operator=(const orm::DB &_db)
    {
        pool = _db.pool; host = _db.host; port = _db.port;
        username = _db.username; password = _db.password; db = _db.db;
        autoCommit = _db.autoCommit;
        return *this;
    }

	template<typename T>
	Model<T> DB::model()
	{
		return Model<T>(pool->getConn(), db, autoCommit);
	}

	void DB::setAutoCommit(bool mode)
	{
		autoCommit = mode;
	}

    SqlConn DB::getConn()
    {
        return pool->getConn();
    }

    template<typename... Args>
    void DB::AutoMigrate(Args&&... args)
    {
        //先对比新老元数据，迁移改动
        ((doMigrate<std::decay_t<Args>>()), ...);
        //清空数据文件
        std::ofstream ofs(DataLoader::dataFilePath(db), std::ios::binary | std::ios::trunc);
        ofs.close();
        //将最新的model元信息写入文件
        ((Model<std::decay_t<Args>>::saveModel()), ...);
    }

    template<typename T>
    void DB::doMigrate()
    {
        Model<T> md = model<T>();
        md.AutoMigrate();
    }
}


#endif