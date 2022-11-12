#include "sqlresult.h"

void free_func(MYSQL_RES* res)
{
	mysql_free_result(res);
}

SqlResult::SqlResult(MYSQL_RES* res) :
	res_ptr(unique_ptr<MYSQL_RES, result_free>(res, free_func)),
	cur(1)
{
	if (res_ptr.get() == nullptr)
		return;
	numFields = mysql_num_fields(res_ptr.get());
	numRows = mysql_num_rows(res_ptr.get());
	MYSQL_FIELD* fs = mysql_fetch_fields(res_ptr.get());
	for (unsigned int i = 0; i < numFields; i++)
	{
		fields.push_back(FieldMeta(fs[i]));
		nameToIndex[fields[i].name] = i;
	}
}

bool SqlResult::hasNext()
{
	return cur <= numRows;
}

void SqlResult::next()
{
	row = mysql_fetch_row(res_ptr.get());
	lengths = mysql_fetch_lengths(res_ptr.get());
	cur++;
}

bool SqlResult::isValidIndex(int index)
{
	return index >= 0 && index < numFields;
}

int SqlResult::getInt(int columnIndex)
{
	if (!isValidIndex(columnIndex - 1) || !isInt(fields[columnIndex - 1].type))
		return 0;
	return atoi(row[columnIndex - 1]);
}

int SqlResult::getInt(const string& columnName)
{
	unordered_map<string, int>::iterator it = nameToIndex.find(columnName);
	int index = it != nameToIndex.end() ? it->second + 1 : -1;
	return getInt(index);
}

long long SqlResult::getLong(int columnIndex)
{
	if (!isValidIndex(columnIndex - 1) || !isLong(fields[columnIndex - 1].type))
		return 0;
	return atol(row[columnIndex - 1]);
}

long long SqlResult::getLong(const string& columnName)
{
	unordered_map<string, int>::iterator it = nameToIndex.find(columnName);
	int index = it != nameToIndex.end() ? it->second + 1: -1;
	return getLong(index);
}

float SqlResult::getFloat(int columnIndex)
{
	if (!isValidIndex(columnIndex - 1) || !isFloat(fields[columnIndex - 1].type))
		return 0.0f;
	return (float)atof(row[columnIndex - 1]);
}

float SqlResult::getFloat(const string& columnName)
{
	unordered_map<string, int>::iterator it = nameToIndex.find(columnName);
	int index = it != nameToIndex.end() ? it->second + 1: -1;
	return getFloat(index);
}

double SqlResult::getDouble(int columnIndex)
{
	if (!isValidIndex(columnIndex - 1) || !isDouble(fields[columnIndex - 1].type))
		return 0.0f;
	return atof(row[columnIndex - 1]);
}

double SqlResult::getDouble(const string& columnName)
{
	unordered_map<string, int>::iterator it = nameToIndex.find(columnName);
	int index = it != nameToIndex.end() ? it->second + 1: -1;
	return getDouble(index);
}

string SqlResult::getString(int columnIndex)
{
	if (!isValidIndex(columnIndex - 1) || !isString(fields[columnIndex - 1].type))
		return string();
    if(row[columnIndex - 1] == nullptr)
        return string();
	return string(row[columnIndex - 1]);
}

string SqlResult::getString(const string& columnName)
{
	unordered_map<string, int>::iterator it = nameToIndex.find(columnName);
	int index = it != nameToIndex.end() ? it->second + 1: -1;
	return getString(index);
}

Timestamp SqlResult::getTime(int columnIndex)
{
	if (!isValidIndex(columnIndex - 1) || !isTime(fields[columnIndex - 1].type))
		return Timestamp();
	string time(row[columnIndex - 1]);
	return Timestamp(time);
}

Timestamp SqlResult::getTime(const string& columnName)
{
	unordered_map<string, int>::iterator it = nameToIndex.find(columnName);
	int index = it != nameToIndex.end() ? it->second + 1 : -1;
	return getTime(index);
}