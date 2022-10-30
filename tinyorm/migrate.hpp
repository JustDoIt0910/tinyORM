//
// Created by zr on 22-10-27.
//

#ifndef TINYORM_EXAMPLE_MIGRATE_HPP
#define TINYORM_EXAMPLE_MIGRATE_HPP

#include <vector>
#include "model.hpp"
#include "mysql4cpp/connectionpool.h"
#include "mysql4cpp/sqlconn.h"
#include "mysql4cpp/databasemetadata.h"
#include "spdlog/spdlog.h"

namespace orm {
    struct Cmd
    {
        enum Action
        {
            MIGACTION_CREATE_TABLE,
            MIGACTION_ADD_COLUMN,
            MIGACTION_MODIFY_COLUMN,
            MIGACTION_CHANGE_COLUMN,
            MIGACTION_ADD_INDEX,
            MIGACTION_CHANGE_TABLENAME,
            MIGACTION_ADD_NOTNULL,
            MIGACTION_SET_DEFAULT
        } action;
        int priority;
        std::string columnName;

        Cmd(const std::string& col, Action ac): columnName(col), action(ac)
        {
            switch(action)
            {
                case MIGACTION_CREATE_TABLE:
                case MIGACTION_ADD_COLUMN:
                case MIGACTION_MODIFY_COLUMN:
                case MIGACTION_CHANGE_COLUMN:
                case MIGACTION_CHANGE_TABLENAME:
                    priority = 1;
                case MIGACTION_ADD_INDEX:
                case MIGACTION_ADD_NOTNULL:
                case MIGACTION_SET_DEFAULT:
                    priority = 2;
            }
        }
    };

    template<typename T>
    class Migrator
    {
    public:
        Migrator(Model<T>* _model);
        void addCommand(const Cmd& cmd);
        virtual void migrate() = 0;
        virtual ~Migrator();

    private:
        SqlConn conn;
        Model<T>* model;
        std::vector<Cmd> cmds;
    };

    template<typename T>
    Migrator<T>::Migrator(Model<T>* _model): model(_model)
    {
        ConnectionPool* pool = ConnectionPool::getInstance();
        conn = pool->getConn();
    }

    template<typename T>
    void Migrator<T>::addCommand(const Cmd& cmd)
    {
        cmds.push_back(cmd);
    }

    template<typename T>
    Migrator<T>::~Migrator()
    {
        conn.close();
        spdlog::info("Model migrated");
    }

    template<typename T>
    class MysqlMigrator: public Migrator<T>
    {
    public:
        MysqlMigrator(Model<T>* _model);
        void migrate();
    };

    template<typename T>
    MysqlMigrator<T>::MysqlMigrator(Model<T> *_model): Migrator<T>(_model) {}

    template<typename T>
    void MysqlMigrator<T>::migrate()
    {

    }

    template<typename T>
    void Model<T>::AutoMigrate()
    {
        MysqlMigrator<T> migrator(this);
        DatabaseMetadata dbMeta = conn.getMetadata();
        ResultSet rs = dbMeta.getTables(db, tableName.value_or(getClassName()));
        if(!rs.hasNext())
            migrator.addCommand(Cmd("", Cmd::MIGACTION_CREATE_TABLE));
        else if(fields_prev.size() > 0)
        {
            
        }
        spdlog::info("Model migrating...");
        migrator.migrate();
        for(FieldMeta& meta: metadata)
            fields_prev[meta.columnName.value_or(meta.fieldName)] = meta;
        for(Index& i: indices)
            indices_prev.push_back(i);
    }
}

#endif //TINYORM_EXAMPLE_MIGRATE_HPP
