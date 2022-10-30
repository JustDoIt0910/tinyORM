#ifndef __ORM_MODEL_HPP__
#define __ORM_MODEL_HPP__

#include <sstream>
#include <iostream>
#include "reflection.hpp"
#include "mysql4cpp/sqlconn.h"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <type_traits>
#include "spdlog/spdlog.h"

#define AND		"AND"
#define OR		"OR"
#define LIMIT	"LIMIT"

namespace orm {

	class Constraint
	{
	public:
		std::string condition;
		std::string conj;
		std::vector<std::pair<std::string, void*>> bindValues;

		Constraint(const std::string& cond, const std::string& _conj): 
			condition(cond), conj(_conj){}
		Constraint(){}
	};

	template<typename T>
	class Model
	{
	public:
		Model(SqlConn&& conn, std::string _db, bool mode);
		bool Create(const T& t);

		int Update(const T& t);

		template<typename... Args>
		int Update(Args... args);

		int Delete();

		template<typename... Args>
		Model<T>& Select(Args... args);

		template<typename... Args>
		Model<T>& Where(const std::string& cond, Args... args);

		template<typename... Args>
		Model<T>& Or(const std::string& cond, Args... args);

		template<typename... Args>
		Model<T>& Raw(const std::string& sql, Args... args);

		Model<T>& Page(int pageNum, int pageSize);

		int First(T& t);
		std::vector<T> All();
		std::vector<T> Fetch();
		int Exec();

        void AutoMigrate();
		void setAutoCommit(bool mode);
		void Commit();
		void Rollback();
	private:
		std::string getClassName();
		Statement prepareQuery(int limit);
		void bindParams(Statement& stmt);
		void fillObject(T& t, ResultSet& rs, std::vector<std::string>& cols);
		void reset();

		std::vector<FieldMeta> metadata;
        std::vector<Index> indices;
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
        std::string db;

        static std::unordered_map<std::string, FieldMeta> fields_prev;
        static std::vector<Index> indices_prev;
	};

    template<typename T>
    std::unordered_map<std::string, FieldMeta> Model<T>::fields_prev;
    template<typename T>
    std::vector<Index> Model<T>::indices_prev;


	template<size_t N, typename tup>
	std::enable_if_t<N == std::tuple_size_v<tup>>
		updateColumn(std::stringstream& ss, Constraint& c, const tup& t) {}

	template<size_t N, typename tup>
	std::enable_if_t < 2 * N >= std::tuple_size_v<tup> && N < std::tuple_size_v<tup>>
		updateColumn(std::stringstream & ss, Constraint & c, const tup & t)
	{
		bindVal(c, std::get<N>(t));
		updateColumn<N + 1>(ss, c, t);
	}
	template<size_t N, typename tup>
	std::enable_if_t < (N > 0) && 2 * N < std::tuple_size_v<tup>>
		updateColumn(std::stringstream & ss, Constraint & c, const tup & t)
	{
		ss << ", " << std::get<N>(t) << " = ?";
		updateColumn<N + 1>(ss, c, t);
	}
	template<size_t N = 0, typename tup>
	std::enable_if_t <N == 0>
		updateColumn(std::stringstream& ss, Constraint& c, const tup& t)
	{
		ss << std::get<0>(t) << " = ?";
		updateColumn<1>(ss, c, t);
	}

	template<typename T>
	Model<T>::Model(SqlConn&& _conn, std::string _db, bool mode): conn(std::move(_conn)), status(INIT), db(_db)
	{
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
			std::string col = metadata[i].columnName.value_or(metadata[i].fieldName);
			columnToIndex[col] = i;
		}
        auto inds = orm::Base<T>::getIndices();
        indices.swap(inds);
	}

	template<typename T>
	bool Model<T>::Create(const T& t)
	{
		std::stringstream sql;
		sql << "INSERT INTO " << tableName.value_or(getClassName()) << "(";
		bool isFirst = true;
		int cnt = 0;
		for (FieldMeta& field : metadata)
		{
			if (field.ignore)
				continue;
			if (!isFirst)
				sql << " ,";
			isFirst = false;
			sql << field.columnName.value_or(field.fieldName);
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
			else if (metadata[i].cppType == ORM_TYPE_STRING ||
				metadata[i].cppType == ORM_TYPE_STD_STRING)
				stmt.setString(cnt, getfield(std::string, t, i));
			else if (metadata[i].cppType == ORM_TYPE_TIMESTAMP)
				stmt.setTime(cnt, getfield(Timestamp, t, i), MYSQL_TYPE_TIMESTAMP);
			cnt++;
		}
		stmt.executeUpdate();
		reset();
		return true;
	}

	template<typename T>
	template<typename... Args>
	Model<T>& Model<T>::Select(Args... args)
	{
		((selectedColumns.push_back(args)), ...);
		return *this;
	}

	template<typename T>
	template<typename... Args>
	Model<T>& Model<T>::Where(const std::string& cond, Args... args)
	{
		Constraint c(cond, AND);
		((bindVal(c, args)), ...);
		constraints.push_back(c);
		return *this;
	}

	template<typename T>
	template<typename... Args>
	Model<T>& Model<T>::Or(const std::string& cond, Args... args)
	{
		Constraint c(cond, OR);
		((bindVal(c, args)), ...);
		constraints.push_back(c);
		return *this;
	}

	template<typename T>
	template<typename... Args>
	int Model<T>::Update(Args... args)
	{
		std::stringstream sql;
		Constraint binds;
		sql << "UPDATE " << tableName.value_or(getClassName()) << " SET ";
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
		Statement stmt = conn.prepareStatement(sql.str());
		bindParams(stmt);
		int rowAffect = stmt.executeUpdate();
		reset();
		return rowAffect;
	}

	template<typename T>
	int Model<T>::Update(const T& t)
	{
		return 0;
	}

	template<typename T>
	int Model<T>::Delete()
	{
		std::stringstream sql;
		sql << "DELETE FROM " << tableName.value_or(getClassName());
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

	template<typename T>
	Model<T>& Model<T>::Page(int pageNum, int pageSize)
	{
		if (pageSize <= 0 || pageNum <= 0)
			return *this;
		std::stringstream sql;
		sql << " LIMIT " << pageSize << " OFFSET " << (pageNum - 1) * pageSize;
		constraints.push_back(Constraint(sql.str(), LIMIT));
		return *this;
	}

	template<typename T>
	template<typename... Args>
	Model<T>& Model<T>::Raw(const std::string& _sql, Args... args)
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

	template<typename T>
	int Model<T>::Exec()
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

	template<typename T>
	std::vector<T> Model<T>::Fetch()
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

	void bindVal(Constraint& c, int val)
	{
		auto bind = std::make_pair(ORM_TYPE_INT, (void*)(new int));
		*(int*)bind.second = val;
		c.bindValues.push_back(bind);
	}

	void bindVal(Constraint& c, float val)
	{
		auto bind = std::make_pair(ORM_TYPE_FLOAT, (void*)(new float));
		*(float*)bind.second = val;
		c.bindValues.push_back(bind);
	}

	void bindVal(Constraint& c, double val)
	{
		auto bind = std::make_pair(ORM_TYPE_DOUBLE, (void*)(new double));
		*(double*)bind.second = val;
		c.bindValues.push_back(bind);
	}

	void bindVal(Constraint& c, long long val)
	{
		auto bind = std::make_pair(ORM_TYPE_LONG_LONG, (void*)(new long long));
		*(long long*)bind.second = val;
		c.bindValues.push_back(bind);
	}

	void bindVal(Constraint& c, const std::string& val)
	{
		auto bind = std::make_pair(ORM_TYPE_STRING, (void*)(new std::string));
		*(std::string*)bind.second = val;
		c.bindValues.push_back(bind);
	}

	void bindVal(Constraint& c, const char* val)
	{
		size_t len = strlen(val);
		auto bind = std::make_pair(ORM_TYPE_CONST_STR, (void*)(new char[len + 1]));
		memcpy(bind.second, (const void*)val, len + 1);
		c.bindValues.push_back(bind);
	}

	void bindVal(Constraint& c, Timestamp val)
	{
		auto bind = std::make_pair(ORM_TYPE_TIMESTAMP, (void*)(new Timestamp));
		*(Timestamp*)bind.second = val;
		c.bindValues.push_back(bind);
	}

	template<typename T>
	int Model<T>::First(T& t)
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

	template<typename T>
	std::vector<T> Model<T>::All()
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

	template<typename T>
	Statement Model<T>::prepareQuery(int limit)
	{
		std::stringstream sql;
		sql << "SELECT ";
		if (selectedColumns.size() == 0)
			sql << "*";
		for (int i = 0; i < selectedColumns.size(); i++)
		{
			sql << selectedColumns[i];
			if (i < selectedColumns.size() - 1)
				sql << ", ";
		}
		sql << " FROM " << tableName.value_or(getClassName());
		if (constraints.size() > 0)
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

	template<typename T>
	void Model<T>::bindParams(Statement& stmt)
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
				else if (bind.first == ORM_TYPE_STRING || bind.first == ORM_TYPE_STD_STRING)
					stmt.setString(index++, *(std::string*)bind.second);
				else if (bind.first == ORM_TYPE_CONST_STR)
					stmt.setString(index++, std::string((char*)bind.second));
				else if (bind.first == ORM_TYPE_TIMESTAMP)
					stmt.setTime(index++, *(Timestamp*)bind.second, MYSQL_TYPE_TIMESTAMP);
			}
		}
	}

	template<typename T>
	void Model<T>::fillObject(T& t, ResultSet& rs, std::vector<std::string>& cols)
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
			else if (type == ORM_TYPE_STRING ||
				type == ORM_TYPE_STD_STRING)
				setfield(std::string, t, index, rs.getString(col));
			else if (type == ORM_TYPE_TIMESTAMP)
				setfield(Timestamp, t, index, rs.getTime(col));
		}
	}

	template<typename T>
	void Model<T>::reset()
	{
		conn.close();
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
				else if (bind.first == ORM_TYPE_STRING || bind.first == ORM_TYPE_STD_STRING)
					delete (std::string*)bind.second;
				else if (bind.first == ORM_TYPE_TIMESTAMP)
					delete (Timestamp*)bind.second;
				else if (bind.first == ORM_TYPE_CONST_STR)
					delete[](char*)bind.second;
			}
		}
		std::vector<Constraint>().swap(constraints);
		std::vector<FieldMeta>().swap(metadata);
		std::vector<std::string>().swap(selectedColumns);
		auto it = columnToIndex.begin();
		while (it != columnToIndex.end())
			it = columnToIndex.erase(it);
	}

	template<typename T>
	std::string Model<T>::getClassName()
	{
		std::string fullName = typeid(T).name();
		size_t space = fullName.find_last_of(" ");
		return fullName.substr(space + 1);
	}

	template<typename T>
	void Model<T>::setAutoCommit(bool mode)
	{
		conn.setAutocommit(mode);
	}

	template<typename T>
	void Model<T>::Commit()
	{
		conn.commit();
	}

	template<typename T>
	void Model<T>::Rollback()
	{
		conn.rollback();
	}
}
#endif