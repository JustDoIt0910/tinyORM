#include "result.h"
#include "spdlog.h"

bool Result::isInt(enum_field_types mysqlType)
{
	return (mysqlType == MYSQL_TYPE_TINY ||
		mysqlType == MYSQL_TYPE_SHORT ||
		mysqlType == MYSQL_TYPE_INT24 ||
		mysqlType == MYSQL_TYPE_LONG);
}

bool Result::isLong(enum_field_types mysqlType)
{
	return (mysqlType == MYSQL_TYPE_LONGLONG);
}

bool Result::isFloat(enum_field_types mysqlType)
{
	return (mysqlType == MYSQL_TYPE_FLOAT);
}

bool Result::isDouble(enum_field_types mysqlType)
{
	return (mysqlType == MYSQL_TYPE_DOUBLE);
}

bool Result::isString(enum_field_types mysqlType)
{
	return (mysqlType == MYSQL_TYPE_STRING ||
		mysqlType == MYSQL_TYPE_VAR_STRING ||
		mysqlType == MYSQL_TYPE_VARCHAR ||
        mysqlType == MYSQL_TYPE_BLOB);
}

bool Result::isTime(enum_field_types mysqlType)
{
	return (mysqlType == MYSQL_TYPE_TIME ||
		mysqlType == MYSQL_TYPE_TIMESTAMP ||
		mysqlType == MYSQL_TYPE_DATE ||
		mysqlType == MYSQL_TYPE_YEAR ||
		mysqlType == MYSQL_TYPE_DATETIME);
}
