#pragma once
#include "result.h"
#include "common.h"
#include "mysql.h"
#include <memory>
#include <vector>
#include <unordered_map>
using namespace std;

typedef void (*result_free)(MYSQL_RES* res);

class SqlResult: public Result
{
public:
	SqlResult(MYSQL_RES* res);

	int getInt(int columnIndex);
	int getInt(const string& columnName);

	long long getLong(int columnIndex);
	long long getLong(const string& columnName);

	float getFloat(int columnIndex);
	float getFloat(const string& columnName);

	double getDouble(int columnIndex);
	double getDouble(const string& columnName);

	string getString(int columnIndex);
	string getString(const string& columnName);

	Timestamp getTime(int columnIndex);
	Timestamp getTime(const string& columnName);

	bool hasNext();
	void next();

private:
	unique_ptr<MYSQL_RES, result_free> res_ptr;
	vector<FieldMeta> fields;
	unordered_map<string, int> nameToIndex;
	MYSQL_ROW row;
	unsigned long* lengths;

	unsigned int numFields;
	unsigned long long numRows;

	unsigned long long cur;

	bool isValidIndex(int index);
};

