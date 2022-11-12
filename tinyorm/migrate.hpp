//
// Created by zr on 22-10-27.
//

#ifndef __ORM_MIGRATE_HPP__
#define __ORM_MIGRATE_HPP__

#include <vector>
#include "model.hpp"
#include "dataloader.hpp"
#include "mysql4cpp/connectionpool.h"
#include "mysql4cpp/sqlconn.h"
#include "mysql4cpp/databasemetadata.h"
#include "spdlog/spdlog.h"
#include <sstream>

namespace orm {
    struct Cmd
    {
        enum Action
        {
            MIGACTION_CREATE_TABLE,
            MIGACTION_ADD_COLUMN,
            MIGACTION_CHANGE_COLUMN,
            MIGACTION_ADD_INDEX,
            MIGACTION_CHANGE_TABLENAME,
            MIGACTION_DROP_INDEX
        } action;
        int priority;
        std::string targetName;
        std::string prevTargetName;

        Cmd(const std::string& tar, Action ac, const std::string& prevTar = ""):
                targetName(tar), action(ac), prevTargetName(prevTar)
        {
            switch(action)
            {
                case MIGACTION_CREATE_TABLE:
                case MIGACTION_CHANGE_TABLENAME:
                    priority = 1;
                    break;
                case MIGACTION_ADD_COLUMN:
                case MIGACTION_CHANGE_COLUMN:
                    priority = 2;
                    break;
                case MIGACTION_DROP_INDEX:
                    priority = 3;
                    break;
                case MIGACTION_ADD_INDEX:
                    priority = 4;
            }
        }

        bool operator<(const Cmd& cmd)
        {
            return priority < cmd.priority;
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

    protected:
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
        sort(this->cmds.begin(), this->cmds.end());
        std::stringstream sql;
        for(const Cmd& cmd: this->cmds)
        {
            switch (cmd.action)
            {
                //修改表名命令
                case Cmd::MIGACTION_CHANGE_TABLENAME:
                    sql << "ALTER TABLE " << cmd.prevTargetName << " RENAME AS " << cmd.targetName;
                    spdlog::info(sql.str());
                    break;
                //建表命令
                case Cmd::MIGACTION_CREATE_TABLE:
                    sql << "CREATE TABLE " << this->model->getTableName() << "(";
                    for(int i = 0; i < this->model->getAllMetadata().size(); i++)
                    {
                        if(i > 0)
                            sql << ", ";
                        sql << this->model->getAllMetadata()[i].getColumnName() << " "
                        << this->model->getAllMetadata()[i].getColumnType();
                        if(this->model->getAllMetadata()[i].isPk)
                        {
                            sql << " PRIMARY KEY";
                            if(this->model->getAllMetadata()[i].autoInc)
                                sql << " AUTO_INCREMENT";
                        }
                        else if(this->model->getAllMetadata()[i].notNull)
                            sql << " NOT NULL";
                        if(this->model->getAllMetadata()[i]._default.length() > 0)
                            sql << " DEFAULT " << this->model->getAllMetadata()[i]._default;
                    }
                    for(auto it = this->model->getIndices().cbegin(); it != this->model->getIndices().cend(); it++)
                    {
                        if(it->first == "PRIMARY")
                            continue;
                        sql << ", ";
                        if(it->second.type == Index::INDEX_TYPE_MUL)
                            sql << "INDEX ";
                        else
                            sql << "UNIQUE ";
                        sql << it->second.name << "(";
                        for(int c = 0; c < it->second.columns.size(); c++)
                        {
                            if(c > 0)
                                sql << ", ";
                            sql << it->second.columns[c];
                        }
                        sql << ")";
                    }
                    sql << ")";
                    spdlog::info(sql.str());
                    break;
                //修改列或者新建列
                case Cmd::MIGACTION_CHANGE_COLUMN:
                case Cmd::MIGACTION_ADD_COLUMN:
                    if(cmd.action == Cmd::MIGACTION_CHANGE_COLUMN)
                        sql << "ALTER TABLE " << this->model->getTableName() << " CHANGE "
                        << ((cmd.prevTargetName.length() == 0) ? cmd.targetName : cmd.prevTargetName) << " ";
                    else
                        sql << "ALTER TABLE " << this->model->getTableName() << " ADD COLUMN ";

                    sql<< cmd.targetName << " " << this->model->getMetadata(cmd.targetName).getColumnType();
                    if(this->model->getMetadata(cmd.targetName).notNull)
                        sql << " NOT NULL ";
                    if(this->model->getMetadata(cmd.targetName)._default.length() > 0)
                        sql << " DEFAULT " << this->model->getMetadata(cmd.targetName)._default;
                    sql << this->model->getMetadata(cmd.targetName).extra;
                    if(this->model->getMetadata(cmd.targetName).autoInc)
                        sql << " AUTO_INCREMENT ";
                    spdlog::info(sql.str());
                    break;
                //删除索引命令
                case Cmd::MIGACTION_DROP_INDEX:
                    if(cmd.targetName == "PRIMARY")
                        sql << "ALTER TABLE " << this->model->getTableName() << " DROP PRIMARY KEY";
                    else
                        sql << "ALTER TABLE " << this->model->getTableName() << " DROP INDEX " << cmd.targetName;
                    spdlog::info(sql.str());
                    break;
                //新建索引命令
                case Cmd::MIGACTION_ADD_INDEX:
                    sql << "ALTER TABLE " << this->model->getTableName() << " ADD ";
                    if(this->model->getIndex(cmd.targetName).type == Index::INDEX_TYPE_MUL)
                        sql << "INDEX " << cmd.targetName << "(";
                    else if(this->model->getIndex(cmd.targetName).type == Index::INDEX_TYPE_UNI)
                        sql << "UNIQUE " << cmd.targetName << "(";
                    else
                        sql << "PRIMARY KEY ";
                    for(int i = 0; i < this->model->getIndex(cmd.targetName).columns.size(); i++)
                    {
                        if(i > 0)
                            sql << ", ";
                        sql << this->model->getIndex(cmd.targetName).columns[i];
                    }
                    sql << ")";
                    spdlog::info(sql.str());
                    break;
            }
            this->conn.executeUpdate(sql.str());
            //清空sql. 准备生成下一个语句
            std::stringstream().swap(sql);
        }
    }

    template<typename T>
    void Model<T>::AutoMigrate()
    {
        MysqlMigrator<T> migrator(this);
        DataLoader::loadFromFile(db, getClassName(), tablenamePrev, fieldsPrev, indicesPrev, columnToIndexPrev);
        spdlog::info("Model {} migrating...", getClassName());

        //如果数据文件中查找不到model元信息，尝试从数据库中查找这张表的元数据，如果仍然查找不到，说明表不存在，需要建表
        if(tablenamePrev.length() == 0)
        {
            //先尝试用自定义表名称查找，如果找到了，说明数据库中存在这张表，无需建表
            DataLoader::loadFromDB(fieldsPrev, indicesPrev, columnToIndexPrev, db, getTableName(), conn);
            //查不到换用默认表名称查找，如果找到了，说明数据库中存在这张表，无需建表
            if(fieldsPrev.size() == 0)
            {
                DataLoader::loadFromDB(fieldsPrev, indicesPrev,columnToIndexPrev, db, getDefaultTableName(), conn);
                //还查不到说明没有建过这张表，传一个建表命令给migrator
                if(fieldsPrev.size() == 0)
                    migrator.addCommand(Cmd("", Cmd::MIGACTION_CREATE_TABLE));
                    //使用默认表名称查到了元数据，而使用自定义表名没有查到，说明数据库中存在这张表，但是新model修改了表名称
                else
                    migrator.addCommand(Cmd(getTableName(), Cmd::MIGACTION_CHANGE_TABLENAME, getDefaultTableName()));
            }
        }
        if(fieldsPrev.size() > 0)
        {
            if(tablenamePrev.length() != 0 && tablenamePrev != getTableName())
                migrator.addCommand(Cmd(getTableName(), Cmd::MIGACTION_CHANGE_TABLENAME, getDefaultTableName()));
            //逐列对比有无更改
            for(FieldMeta& field: metadata)
            {
                //如果使用列名称查找不到
                if(columnToIndexPrev.find(field.getColumnName()) == columnToIndexPrev.end())
                {
                    //使用变量名查找
                    auto it = fieldsPrev.cbegin();
                    for(; it != fieldsPrev.cend(); it++)
                        if(it->fieldName == field.fieldName)
                            break;
                    //仍然查找不到，说明该列是新增的
                    if(it == fieldsPrev.cend())
                    {
                        migrator.addCommand(Cmd(field.getColumnName(), Cmd::MIGACTION_ADD_COLUMN));
                        continue;
                    }
                        //如果查到了，说明修改了列名称
                    else
                    {
                        migrator.addCommand(Cmd(field.getColumnName(), Cmd::MIGACTION_CHANGE_COLUMN,
                                                it->getColumnName()));
                        continue;
                    }
                }
                else
                {
                    if(field == fieldsPrev[columnToIndexPrev[field.getColumnName()]])
                        continue;
                    //新旧出现不同，触发一个change action
                    migrator.addCommand(Cmd(field.getColumnName(), Cmd::MIGACTION_CHANGE_COLUMN));
                }
            }
            //对比索引信息
            for(auto it = indicesPrev.begin(); it != indicesPrev.end(); it++)
            {
                //原有索引查找不到，触发drop index action
                if(indices.find(it->first) == indices.end())
                    migrator.addCommand(Cmd(it->first, Cmd::MIGACTION_DROP_INDEX));
                    //索引信息发生变化，先删除再新建
                else if(!(it->second == indices.find(it->first)->second))
                {
                    migrator.addCommand(Cmd(it->first, Cmd::MIGACTION_DROP_INDEX));
                    migrator.addCommand(Cmd(it->first, Cmd::MIGACTION_ADD_INDEX));
                }
            }
            //检查是否有新增的索引
            for(auto it = indices.begin(); it != indices.end(); it++)
            {
                if(indicesPrev.find(it->first) == indicesPrev.end())
                    migrator.addCommand(Cmd(it->first, Cmd::MIGACTION_ADD_INDEX));
            }
        }
        //迁移
        migrator.migrate();
        //将上一版本的原信息替换成新的，为saveModel作准备
        tablenamePrev = getTableName();
        metadata.swap(fieldsPrev);
        indices.swap(indicesPrev);
    }
}

#endif
