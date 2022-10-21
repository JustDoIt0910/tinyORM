#pragma once
#include "result.h"
#include "statement.h"

class PreparedResult: public Result
{
public:
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

	PreparedResult(Statement* _stmt, int _count);
	bool hasNext();
	void next();

private:
	Statement* stmt;
	int count;
	int cur;
};

