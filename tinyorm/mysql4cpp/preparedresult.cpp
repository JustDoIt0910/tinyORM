#include "preparedresult.h"

PreparedResult::PreparedResult(Statement* _stmt, int _count):
    stmt(_stmt), count(_count), cur(1) {}

bool PreparedResult::hasNext()
{
    return cur <= count;
}

void PreparedResult::next()
{
    mysql_stmt_fetch(stmt->get());
    cur++;
}

int PreparedResult::getInt(int columnIndex)
{
    int* data = (int*)(stmt->getRes(columnIndex - 1).data);
    return data ? *data : 0;
}

int PreparedResult::getInt(const string& columnName)
{
    int* data = (int*)(stmt->getRes(columnName).data);
    return data ? *data : 0;
}

long long PreparedResult::getLong(int columnIndex)
{
    long long* data = (long long*)(stmt->getRes(columnIndex - 1).data);
    return data ? *data : 0;
}

long long PreparedResult::getLong(const string& columnName)
{
    long long* data = (long long*)(stmt->getRes(columnName).data);
    return data ? *data : 0;
}

float PreparedResult::getFloat(int columnIndex)
{
    float* data = (float*)(stmt->getRes(columnIndex - 1).data);
    return data ? *data : 0.0f;
}

float PreparedResult::getFloat(const string& columnName)
{
    float* data = (float*)(stmt->getRes(columnName).data);
    return data ? *data : 0.0f;
}

double PreparedResult::getDouble(int columnIndex)
{
    double* data = (double*)(stmt->getRes(columnIndex - 1).data);
    return data ? *data : 0.0f;
}

double PreparedResult::getDouble(const string& columnName)
{
    double* data = (double*)(stmt->getRes(columnName).data);
    return data ? *data : 0.0f;
}

string PreparedResult::getString(int columnIndex)
{
    Res res = stmt->getRes(columnIndex - 1);
    char* data = (char*)(res.data);
    return data ? string(data, res.len) : string();
}

string PreparedResult::getString(const string& columnName)
{
    Res res = stmt->getRes(columnName);
    char* data = (char*)(res.data);
    return data ? string(data, res.len) : string();
}

Timestamp PreparedResult::getTime(int columnIndex)
{
    Res res = stmt->getRes(columnIndex - 1);
    MYSQL_TIME* mytime = (MYSQL_TIME*)(res.data);
    if (!mytime)
        return Timestamp();
    return Timestamp(mytime);
}

Timestamp PreparedResult::getTime(const string& columnName)
{
    Res res = stmt->getRes(columnName);
    MYSQL_TIME* mytime = (MYSQL_TIME*)(res.data);
    if (!mytime)
        return Timestamp();
    return Timestamp(mytime);
}