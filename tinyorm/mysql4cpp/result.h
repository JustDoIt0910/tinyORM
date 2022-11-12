#pragma once
#include <string>
#include "mysql.h"
#include "timestamp.h"
using namespace std;

class Result
{
public:
    virtual ~Result() {}
	virtual int getInt(int columnIndex) = 0;
	virtual int getInt(const string& columnName) = 0;

	virtual long long getLong(int columnIndex) = 0;
	virtual long long getLong(const string& columnName) = 0;

	virtual float getFloat(int columnIndex) = 0;
	virtual float getFloat(const string& columnName) = 0;

	virtual double getDouble(int columnIndex) = 0;
	virtual double getDouble(const string& columnName) = 0;

	virtual string getString(int columnIndex) = 0;
	virtual string getString(const string& columnName) = 0;

	virtual Timestamp getTime(int columnIndex) = 0;
	virtual Timestamp getTime(const string& columnName) = 0;

	virtual bool hasNext() = 0;
	virtual void next() = 0;

protected:
	bool isInt(enum_field_types mysqlType);
	bool isLong(enum_field_types mysqlType);
	bool isFloat(enum_field_types mysqlType);
	bool isDouble (enum_field_types mysqlType);
	bool isString(enum_field_types mysqlType);
	bool isTime(enum_field_types mysqlType);
};

