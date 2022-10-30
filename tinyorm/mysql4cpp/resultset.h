#pragma once
#include <string>
#include <memory>
#include "timestamp.h"
#include "result.h"
using namespace std;

class ResultSet
{
public:
	ResultSet(Result* res, bool _valid): _isValid(_valid) { result = unique_ptr<Result>(res); }
	ResultSet(const ResultSet& o) = delete;
    ResultSet& operator=(const ResultSet& o) = delete;
	ResultSet(ResultSet&& o): result(o.result.release()), _isValid(o._isValid){}
    ResultSet& operator=(ResultSet&& o)
    {
        result = std::move(o.result);
        _isValid = o._isValid;
        return *this;
    }

	int getInt(int columnIndex) { return result->getInt(columnIndex); }
	int getInt(const string& columnName) { return result->getInt(columnName); }

	long long getLong(int columnIndex) { return result->getLong(columnIndex); }
	long long getLong(const string& columnName) { return result->getLong(columnName); }

	float getFloat(int columnIndex) { return result->getFloat(columnIndex); }
	float getFloat(const string& columnName) { return result->getFloat(columnName); }

	double getDouble(int columnIndex) { return result->getDouble(columnIndex); }
	double getDouble(const string& columnName) { return result->getDouble(columnName); }

	string getString(int columnIndex) { return result->getString(columnIndex); }
	string getString(const string& columnName) { return result->getString(columnName); }

	Timestamp getTime(int columnIndex) { return result->getTime(columnIndex); }
	Timestamp getTime(const string& columnName) { return result->getTime(columnName); }

	bool hasNext() { return result->hasNext(); }
	void next() { return result->next(); }
	bool isValid() { return _isValid; }

private:
	unique_ptr<Result> result;
	bool _isValid;
};

