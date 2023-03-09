#ifndef __ORM_MODEL_HPP__
#define __ORM_MODEL_HPP__

#include <sstream>
#include "reflection.h"
#include "dataloader.h"
#include "mysql4cpp/connectionpool.h"
#include <algorithm>
#include <tuple>
#include <type_traits>
#include <mutex>

#define AND		"AND"
#define OR		"OR"
#define LIMIT	"LIMIT"

namespace orm {

    //定义查询/修改时候的限制条件，也就是where,or的内容
	class Constraint
	{
	public:
        //条件字符串(例如 id = ?)
		std::string condition;
        //连接词(and/or/limit)
		std::string conj;
        //绑定值(例如调用Where("id = ?", 1), condition 就是 "id = ?", conj 就是 "and", bindValues中保存了1)
        //因为值可能是任意类型，只能使用动态分配内存，转void*存储，事后再释放掉
		std::vector<std::pair<std::string, void*>> bindValues;

        inline Constraint(const std::string& cond, const std::string& _conj):
			condition(cond), conj(_conj){}
        inline Constraint()= default;
	};

    template<size_t N, typename tup>
    inline std::enable_if_t<N == std::tuple_size_v<tup>>
    updateColumn(std::stringstream& ss, Constraint& c, const tup& t) {}

    template<size_t N, typename tup>
    inline std::enable_if_t < 2 * N >= std::tuple_size_v<tup> && N < std::tuple_size_v<tup>>
    updateColumn(std::stringstream & ss, Constraint & c, const tup & t)
    {
        bindVal(c, std::get<N>(t));
        updateColumn<N + 1>(ss, c, t);
    }
    template<size_t N, typename tup>
    inline std::enable_if_t < (N > 0) && 2 * N < std::tuple_size_v<tup>>
    updateColumn(std::stringstream & ss, Constraint & c, const tup & t)
    {
        ss << ", " << std::get<N>(t) << " = ?";
        updateColumn<N + 1>(ss, c, t);
    }
    template<size_t N = 0, typename tup>
    inline std::enable_if_t <N == 0>
    updateColumn(std::stringstream& ss, Constraint& c, const tup& t)
    {
        ss << std::get<0>(t) << " = ?";
        updateColumn<1>(ss, c, t);
    }

    inline std::string getCppTypeName(std::string typeName)
    {
        size_t where = typeName.find_last_of("::");
        if(where == std::string::npos)
            return typeName;
        return typeName.substr(where + 1);
    }

    /********************************* Migration ***********************************/

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
    class Model;

    template<typename T>
    class Migrator
    {
    public:
        explicit Migrator(Model<T>* _model): model(_model)
        {
            ConnectionPool* pool = ConnectionPool::getInstance();
            conn = pool->getConn();
        }

        void addCommand(const Cmd& cmd) {cmds.push_back(cmd);}
        virtual void migrate() = 0;
        virtual ~Migrator()
        {
            conn.close();
            spdlog::info("Model migrated");
        }

    protected:
        SqlConn conn;
        Model<T>* model;
        std::vector<Cmd> cmds;
    };

    template<typename T>
    class MysqlMigrator: public Migrator<T>
    {
    public:
        explicit MysqlMigrator(Model<T> *_model): Migrator<T>(_model) {}

        void migrate()
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
    };

	template<typename T>
	class Model
	{
	public:
        Model(SqlConn&& _conn, std::string _db, bool mode): conn(std::move(_conn)), status(INIT)
        {
            {
                std::lock_guard<std::mutex> lg(mu);
                if(db.length() == 0)
                    db = _db;
            }
            conn.setAutocommit(mode);
            auto metas = orm::Base<T>::getMetadatas();
            for (auto it = metas.cbegin(); it != metas.cend(); it++)
            {
                if (it->first != "tablename")
                    metadata.push_back(FieldMeta(it->first, it->second));
                else
                    tableName = it->second.tag;
                sort(metadata.begin(), metadata.end());
            }
            for (int i = 0; i < metadata.size(); i++)
            {
                std::string col = metadata[i].getColumnName();
                columnToIndex[col] = i;
            }
            auto inds = orm::Base<T>::getIndices();
            for(const Index& i: inds)
                indices[i.name] = i;
            for(FieldMeta& meta: metadata)
                if(meta.hasIndex)
                {
                    meta.index.name = (meta.index.type == Index::INDEX_TYPE_PRI) ? "PRIMARY" : meta.getColumnName();
                    indices[meta.index.name] = meta.index;
                }
        }

        ~Model() { conn.close();}

        void Create(const T& t)
        {
            std::stringstream sql;
            sql << "INSERT INTO " << getTableName() << "(";
            bool isFirst = true;
            int cnt = 0;
            for (FieldMeta& field : metadata)
            {
                if (field.ignore)
                    continue;
                if (!isFirst)
                    sql << ", ";
                isFirst = false;
                sql << field.getColumnName();
                cnt++;
            }
            sql << ") VALUES(";
            for (int i = 0; i < cnt; i++)
            {
                sql << "?";
                if (i < cnt - 1)
                    sql << ", ";
            }
            sql << ")";
            spdlog::info(sql.str());
            Statement stmt = conn.prepareStatement(sql.str());
            cnt = 1;
            for (int i = 0; i < metadata.size(); i++)
            {
                if (metadata[i].ignore)
                    continue;
                if (metadata[i].cppType == ORM_TYPE_INT)
                    stmt.setInt(cnt, getfield(int, t, i));
                else if (getCppTypeName(metadata[i].cppType) == ORM_TYPE_STRING)
                    stmt.setString(cnt, getfield(std::string, t, i));
                else if (getCppTypeName(metadata[i].cppType) == ORM_TYPE_TIMESTAMP)
                    stmt.setTime(cnt, getfield(Timestamp, t, i), MYSQL_TYPE_TIMESTAMP);
                cnt++;
            }
            stmt.executeUpdate();
            reset();
        }

        void Create(const std::vector<T>& v)
        {
            std::stringstream sql;
            sql << "INSERT INTO " << getTableName() << "(";
            bool isFirst = true;
            int cnt = 0;
            for (FieldMeta& field : metadata)
            {
                if (field.ignore)
                    continue;
                if (!isFirst)
                    sql << ", ";
                isFirst = false;
                sql << field.getColumnName();
                cnt++;
            }
            sql << ") VALUES";
            for(int j = 0; j < v.size(); j++)
            {
                if(j > 0)
                    sql << ", ";
                sql << "(";
                for (int i = 0; i < cnt; i++)
                {
                    sql << "?";
                    if (i < cnt - 1)
                        sql << ", ";
                }
                sql << ")";
            }
            spdlog::info(sql.str());
            Statement stmt = conn.prepareStatement(sql.str());
            cnt = 1;
            for(const T& t: v)
            {
                for (int i = 0; i < metadata.size(); i++)
                {
                    if (metadata[i].ignore)
                        continue;
                    if (metadata[i].cppType == ORM_TYPE_INT)
                        stmt.setInt(cnt, getfield(int, t, i));
                    else if (getCppTypeName(metadata[i].cppType) == ORM_TYPE_STRING)
                        stmt.setString(cnt, getfield(std::string, t, i));
                    else if (getCppTypeName(metadata[i].cppType) == ORM_TYPE_TIMESTAMP)
                        stmt.setTime(cnt, getfield(Timestamp, t, i), MYSQL_TYPE_TIMESTAMP);
                    cnt++;
                }
            }
            stmt.executeUpdate();
            reset();
        }

        int Update(const T& t)
        {
            return 0;
        }

        template<typename... Args>
        int Update(Args... args)
        {
            std::stringstream sql;
            Constraint binds;
            sql << "UPDATE " << getTableName() << " SET ";
            updateColumn(sql, binds, std::make_tuple(args...));
            constraints.insert(constraints.begin(), binds);
            if (constraints.size() > 1)
            {
                sql << " WHERE ";
                for (int i = 1; i < constraints.size(); i++)
                {
                    if (i > 1)
                        sql << " " << constraints[i].conj << " ";
                    sql << constraints[i].condition;
                }
            }
            spdlog::info(sql.str());
            Statement stmt = conn.prepareStatement(sql.str());
            bindParams(stmt);
            int rowAffect = stmt.executeUpdate();
            reset();
            return rowAffect;
        }

        int Delete()
        {
            std::stringstream sql;
            sql << "DELETE FROM " << getTableName();
            if (constraints.size() > 0)
            {
                sql << " WHERE ";
                for (int i = 0; i < constraints.size(); i++)
                {
                    if (i > 1)
                        sql << " " << constraints[i].conj << " ";
                    sql << constraints[i].condition;
                }
            }
            spdlog::info(sql.str());
            Statement stmt = conn.prepareStatement(sql.str());
            bindParams(stmt);
            int rowAffect = stmt.executeUpdate();
            reset();
            return rowAffect;
        }

        template<typename... Args>
        Model<T>& Select(Args... args)
        {
            ((selectedColumns.push_back(args)), ...);
            return *this;
        }

        template<typename... Args>
        Model<T>& Where(const std::string& cond, Args... args)
        {
            Constraint c(cond, AND);
            ((bindVal(c, args)), ...);
            constraints.push_back(c);
            return *this;
        }

        template<typename... Args>
        Model<T>& Or(const std::string& cond, Args... args)
        {
            Constraint c(cond, OR);
            ((bindVal(c, args)), ...);
            constraints.push_back(c);
            return *this;
        }

        template<typename... Args>
        Model<T>& Raw(const std::string& _sql, Args... args)
        {
            std::string sql = _sql;
            Util::toLowerCase(sql);
            sql = Util::strip(sql);
            if (sql.compare(0, 6, "select") == 0)
                status = QUERY;
            else
                status = UPDATE;
            rawSql = std::move(sql);
            Constraint c;
            ((bindVal(c, args)), ...);
            constraints.clear();
            constraints.push_back(c);
            return *this;
        }

        Model<T>& Page(int pageNum, int pageSize)
        {
            if (pageSize <= 0 || pageNum <= 0)
                return *this;
            std::stringstream sql;
            sql << " LIMIT " << pageSize << " OFFSET " << (pageNum - 1) * pageSize;
            constraints.push_back(Constraint(sql.str(), LIMIT));
            return *this;
        }

        int First(T& t)
        {
            Statement stmt = prepareQuery(1);
            std::vector<std::string> fieldNames = stmt.fieldNames();
            ResultSet rs = stmt.executeQuery();
            if (!rs.isValid() || !rs.hasNext())
                return 0;
            rs.next();
            fillObject(t, rs, fieldNames);
            reset();
            return 1;
        }

        std::vector<T> All()
        {
            std::vector<T> res;
            Statement stmt = prepareQuery(-1);
            std::vector<std::string> fieldNames = stmt.fieldNames();
            ResultSet rs = stmt.executeQuery();
            if (!rs.isValid())
                return res;
            while (rs.hasNext())
            {
                rs.next();
                T t;
                fillObject(t, rs, fieldNames);
                res.push_back(t);
            }
            reset();
            return res;
        }

        int Exec()
        {
            if (status == INIT)
            {
                spdlog::error("Exec() must be called after Raw()");
                return 0;
            }
            if (status != UPDATE)
            {
                spdlog::error("Current operation is a query, use Fetch()");
                return 0;
            }
            Statement stmt = conn.prepareStatement(rawSql);
            bindParams(stmt);
            int rowAffect = stmt.executeUpdate();
            conn.close();
            return rowAffect;
        }

        std::vector<T> Fetch()
        {
            std::vector<T> res;
            if (status == INIT)
            {
                spdlog::error("Fetch() must be called after Raw()");
                return res;
            }
            if (status != QUERY)
            {
                spdlog::error("Current operation is not query, use Exec()");
                return res;
            }
            Statement stmt = conn.prepareStatement(rawSql);
            bindParams(stmt);
            ResultSet rs = stmt.executeQuery();
            std::vector<std::string> fieldNames = stmt.fieldNames();
            if (!rs.isValid())
                return res;
            while (rs.hasNext())
            {
                rs.next();
                T t;
                fillObject(t, rs, fieldNames);
                res.push_back(t);
            }
            reset();
            return res;
        }

        void setAutoCommit(bool mode) {conn.setAutocommit(mode);}

        void Commit() {conn.commit();}

        void Rollback() {conn.rollback();}

        std::string getTableName() {return tableName.value_or(cppToDb(getClassName()));}

        std::string getDefaultTableName() {return cppToDb(getClassName());}

        FieldMeta& getMetadata(const std::string& name) {return metadata[columnToIndex[name]];}

        Index& getIndex(const std::string& name){return indices[name];}

        std::vector<FieldMeta>& getAllMetadata(){return metadata;}

        std::unordered_map<std::string, Index>& getIndices(){return indices;}

        static void saveModel()
        {
            DataLoader::Header header(getClassName(), tablenamePrev);
            DataLoader::saveModel(fieldsPrev, indicesPrev, db, header);
        }

        void AutoMigrate()
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

	private:
        static std::string getClassName()
        {
            std::string fullName = typeid(T).name();
            size_t space = fullName.find_last_of(" ");
            if(space != fullName.npos)
                fullName = fullName.substr(space + 1);
            int i = 0;
            while('0' <= fullName.at(i) && fullName.at(i) <= '9')
                i++;
            return fullName.substr(i);
        }

        Statement prepareQuery(int limit)
        {
            std::stringstream sql;
            sql << "SELECT ";
            if (selectedColumns.empty())
                sql << "*";
            for (int i = 0; i < selectedColumns.size(); i++)
            {
                sql << selectedColumns[i];
                if (i < selectedColumns.size() - 1)
                    sql << ", ";
            }
            sql << " FROM " << getTableName();
            if (!constraints.empty())
            {
                sql << " WHERE ";
                for (int i = 0; i < constraints.size(); i++)
                {
                    bool isLimit = (constraints[i].conj == LIMIT);
                    if (i > 0 && !isLimit)
                        sql << " " << constraints[i].conj << " ";
                    if (!isLimit)
                        sql << "(";
                    sql << constraints[i].condition;
                    if (!isLimit)
                        sql << ")";
                }
            }
            if (limit > 0)
                sql << " LIMIT " << limit;
            spdlog::info(sql.str());
            Statement stmt = conn.prepareStatement(sql.str());
            bindParams(stmt);
            return stmt;
        }

        void bindParams(Statement& stmt)
        {
            int index = 1;
            for (int i = 0; i < constraints.size(); i++)
            {
                for (auto& bind : constraints[i].bindValues)
                {
                    if (bind.first == ORM_TYPE_INT)
                        stmt.setInt(index++, *(int*)bind.second);
                    else if (bind.first == ORM_TYPE_FLOAT)
                        stmt.setFloat(index++, *(float*)bind.second);
                    else if (getCppTypeName(bind.first) == ORM_TYPE_STRING)
                        stmt.setString(index++, *(std::string*)bind.second);
                    else if (bind.first == ORM_TYPE_CONST_STR)
                        stmt.setString(index++, std::string((char*)bind.second));
                    else if (getCppTypeName(bind.first) == ORM_TYPE_TIMESTAMP)
                        stmt.setTime(index++, *(Timestamp*)bind.second, MYSQL_TYPE_TIMESTAMP);
                }
            }
        }

        void fillObject(T& t, ResultSet& rs, std::vector<std::string>& cols)
        {
            for (std::string& col : cols)
            {
                auto it = columnToIndex.find(col);
                if (it == columnToIndex.end())
                    continue;
                int index = columnToIndex[col];
                std::string type = metadata[index].cppType;
                if (type == ORM_TYPE_INT)
                    setfield(int, t, index, rs.getInt(col));
                else if (getCppTypeName(type) == ORM_TYPE_STRING)
                    setfield(std::string, t, index, rs.getString(col));
                else if (getCppTypeName(type) == ORM_TYPE_TIMESTAMP)
                    setfield(Timestamp, t, index, rs.getTime(col));
            }
        }

        void reset()
        {
            for (Constraint& c : constraints)
            {
                for (auto& bind : c.bindValues)
                {
                    if (bind.second == nullptr)
                        continue;
                    if (bind.first == ORM_TYPE_INT)
                        delete (int*)bind.second;
                    else if (bind.first == ORM_TYPE_FLOAT)
                        delete (float*)bind.second;
                    else if (bind.first == ORM_TYPE_DOUBLE)
                        delete (double*)bind.second;
                    else if (getCppTypeName(bind.first) == ORM_TYPE_STRING)
                        delete (std::string*)bind.second;
                    else if (getCppTypeName(bind.first) == ORM_TYPE_TIMESTAMP)
                        delete (Timestamp*)bind.second;
                    else if (bind.first == ORM_TYPE_CONST_STR)
                        delete[](char*)bind.second;
                }
            }
            std::vector<Constraint>().swap(constraints);
            std::vector<std::string>().swap(selectedColumns);
        }

		std::vector<FieldMeta> metadata;
        std::unordered_map<std::string, Index> indices;
		std::unordered_map<std::string, int> columnToIndex;
		std::optional<std::string> tableName;
		std::vector<std::string> selectedColumns;
		std::vector<Constraint> constraints;
		SqlConn conn;
		std::string rawSql;
		enum Status {
			INIT,
			QUERY,
			UPDATE
		} status;
        static std::mutex mu;
        static std::string db;

        static std::vector<FieldMeta> fieldsPrev;
        static std::unordered_map<std::string, int> columnToIndexPrev;
        static std::unordered_map<std::string, Index> indicesPrev;
        static std::string tablenamePrev;
	};

    template<typename T>
    inline std::mutex Model<T>::mu;
    template<typename T>
    inline std::string Model<T>::db;
    template<typename T>
    inline std::vector<FieldMeta> Model<T>::fieldsPrev;
    template<typename T>
    inline std::unordered_map<std::string, int> Model<T>::columnToIndexPrev;
    template<typename T>
    inline std::unordered_map<std::string, Index> Model<T>::indicesPrev;
    template<typename T>
    inline std::string Model<T>::tablenamePrev;


    inline void bindVal(Constraint& c, int val)
	{
		auto bind = std::make_pair(ORM_TYPE_INT, (void*)(new int));
		*(int*)bind.second = val;
		c.bindValues.push_back(bind);
	}

    inline void bindVal(Constraint& c, float val)
	{
		auto bind = std::make_pair(ORM_TYPE_FLOAT, (void*)(new float));
		*(float*)bind.second = val;
		c.bindValues.push_back(bind);
	}

    inline void bindVal(Constraint& c, double val)
	{
		auto bind = std::make_pair(ORM_TYPE_DOUBLE, (void*)(new double));
		*(double*)bind.second = val;
		c.bindValues.push_back(bind);
	}

    inline void bindVal(Constraint& c, long long val)
	{
		auto bind = std::make_pair(ORM_TYPE_LONG_LONG, (void*)(new long long));
		*(long long*)bind.second = val;
		c.bindValues.push_back(bind);
	}

    inline void bindVal(Constraint& c, const std::string& val)
	{
		auto bind = std::make_pair(ORM_TYPE_STRING, (void*)(new std::string));
		*(std::string*)bind.second = val;
		c.bindValues.push_back(bind);
	}

    inline void bindVal(Constraint& c, const char* val)
	{
		size_t len = strlen(val);
		auto bind = std::make_pair(ORM_TYPE_CONST_STR, (void*)(new char[len + 1]));
		memcpy(bind.second, (const void*)val, len + 1);
		c.bindValues.push_back(bind);
	}

    inline void bindVal(Constraint& c, Timestamp val)
	{
		auto bind = std::make_pair(ORM_TYPE_TIMESTAMP, (void*)(new Timestamp));
		*(Timestamp*)bind.second = val;
		c.bindValues.push_back(bind);
	}
}
#endif