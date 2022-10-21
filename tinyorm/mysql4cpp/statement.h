#pragma once
#include "mysql.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "resultset.h"
#include "common.h"
using namespace std;


typedef void (*StmtCloseFunc)(MYSQL_STMT* stmt);

struct Res
{
	void* data;
	unsigned long len;

	Res(void* _data, unsigned int _len): data(_data), len(_len){}
	Res() : data(nullptr), len(0) {};
};

class Statement
{
public:
	Statement(MYSQL_STMT* stmt, const string& sql);
	Statement(Statement&) = delete;
	Statement& operator=(const Statement&) = delete;
	Statement& operator=(Statement&&);
	Statement(Statement&& o);
	~Statement();
	ResultSet executeQuery();
	unsigned long long executeUpdate();
	int getParamCount();
	vector<string> fieldNames();
	void setInt(int columnIndex, int value);
	void setFloat(int columnIndex, float value);
	void setDouble(int columnIndex, double value);
	void setString(int columnIndex, const string& value);
	void setTime(int columnIndex, Timestamp value, enum_field_types type);
	Res getRes(int index);
	Res getRes(const string& columnName);
	MYSQL_STMT* get();
	string getError();

private:
	unique_ptr<MYSQL_STMT, StmtCloseFunc> stmt_ptr;
	string error;
	vector<FieldMeta> fields;
	unsigned int numField;
	unordered_map<string, int> nameToIndex;
	vector<MYSQL_BIND> resBind;
	vector<MYSQL_BIND> paramBind;
	int paramCount;

	void makeResultBind(const FieldMeta& field);
	bool isValidParamIndex(int index);
};
