#ifndef __ORM_DB_HPP__
#define __ORM_DB_HPP__
#include <string>
#include <fstream>
#include "mysql4cpp/connectionpool.h"

namespace orm {

    template<typename T>
    class Model;

	class DB
	{
	public:
        DB(const std::string& _host, const int _port,
                      const std::string& _username, const std::string& _password,
                      const std::string& _db) : host(_host), port(_port), username(_username), password(_password), db(_db)
        {
            autoCommit = true;
            pool = ConnectionPool::getInstance();
            pool->configure(host, username, password, port, db, 10, 1000, 5);
        }

        DB(const DB& _db) = default;

        DB& operator=(const orm::DB &_db) = default;

        ~DB() {delete pool;}

        template<typename T>
        Model<T> model() {return Model<T>(pool->getConn(), db, autoCommit);}

        void setAutoCommit(bool mode) {autoCommit = mode;}

        SqlConn getConn() {return pool->getConn();}

        template<typename... Args>
        void AutoMigrate(Args&&... args)
        {
            //先对比新老元数据，迁移改动
            ((doMigrate<std::decay_t<Args>>()), ...);
            //清空数据文件
            std::ofstream ofs("models/model_" + db + ".dat", std::ios::binary | std::ios::trunc);
            ofs.close();
            //将最新的model元信息写入文件
            ((Model<std::decay_t<Args>>::saveModel()), ...);
        }

        template<typename T>
        void doMigrate()
        {
            Model<T> md = model<T>();
            md.AutoMigrate();
        }

	private:
		ConnectionPool* pool;
		std::string host;
		int port;
		std::string username;
		std::string password;
		std::string db;
		bool autoCommit;
	};
}


#endif