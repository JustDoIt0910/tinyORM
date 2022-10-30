#include "statement.h"
#include "preparedresult.h"
#include <string.h>
#include <assert.h>

void stmtClose(MYSQL_STMT* stmt)
{
	if (stmt)
		mysql_stmt_close(stmt);
}

Statement::Statement(MYSQL_STMT* stmt, const string& sql):
	stmt_ptr(unique_ptr<MYSQL_STMT, StmtCloseFunc>(stmt, stmtClose)), paramCount(0)
{
	if (mysql_stmt_prepare(stmt_ptr.get(), sql.c_str(), sql.length()))
	{
		error = string(mysql_stmt_error(stmt_ptr.get()));
		stmt_ptr.reset(nullptr);
		return;
	}
	MYSQL_RES* res = mysql_stmt_result_metadata(stmt_ptr.get());
	numField = res ? mysql_num_fields(res) : 0;
	MYSQL_FIELD* fs = nullptr;
	if(res)
		fs = mysql_fetch_fields(res);
	for (int i = 0; i < numField; i++)
	{
		fields.emplace_back(FieldMeta(fs[i]));
		nameToIndex[fields[i].name] = i;
		makeResultBind(fields[i]);
	}
	if(res)
		mysql_free_result(res);
	paramCount = mysql_stmt_param_count(stmt_ptr.get());
	paramBind.resize(paramCount);
}

Statement::Statement(Statement&& stmt) :
	stmt_ptr(stmt.stmt_ptr.release(), stmtClose),
	paramCount(stmt.paramCount),
	numField(stmt.numField)
{
	swap(fields, stmt.fields);
	swap(nameToIndex, stmt.nameToIndex);
	swap(resBind, stmt.resBind);
	swap(paramBind, stmt.paramBind);
}

Statement& Statement::operator=(Statement&& stmt)
{
	stmt_ptr.reset(stmt.stmt_ptr.release());
	swap(fields, stmt.fields);
	swap(nameToIndex, stmt.nameToIndex);
	swap(resBind, stmt.resBind);
	swap(paramBind, stmt.paramBind);
	paramCount = stmt.paramCount;
	numField = stmt.numField;
	return *this;
}

Statement::~Statement()
{
	vector<MYSQL_BIND>::iterator it = resBind.begin();
	for (; it != resBind.end(); it++)
	{
		if(it->buffer)
			free(it->buffer);
		if (it->length)
			free(it->length);
		if (it->is_null)
			free(it->is_null);
		if (it->error)
			free(it->error);
	}
	it = paramBind.begin();
	for (; it != paramBind.end(); it++)
	{
		if (it->buffer)
			free(it->buffer);
		if (it->length)
			free(it->length);
	}
}

MYSQL_STMT* Statement::get()
{
	return stmt_ptr.get();
}

string Statement::getError()
{
	return error;
}

MYSQL_BIND newBind(enum_field_types type, int bufsize)
{
	MYSQL_BIND bind;
	memset(&bind, 0, sizeof(bind));
	bind.buffer_type = type;
	bind.buffer = malloc(bufsize);
	bind.length = (unsigned long*)malloc(sizeof(unsigned long));
	bind.buffer_length = bufsize;
	bind.is_null = (my_bool*)malloc(sizeof(my_bool));
	bind.error = (my_bool*)malloc(sizeof(my_bool));
	return bind;
}

MYSQL_BIND newParam(enum_field_types type, const void* value, int bufsize, int valueLen)
{
	assert(bufsize >= valueLen);
	MYSQL_BIND bind;
	memset(&bind, 0, sizeof(bind));
	bind.buffer_type = type;
	bind.buffer = malloc(bufsize);
	memcpy(bind.buffer, value, valueLen);
	bind.length = (unsigned long*)malloc(sizeof(unsigned long));
	*bind.length = valueLen;
	bind.buffer_length = bufsize;
	return bind;
}

void Statement::makeResultBind(const FieldMeta& field)
{
	switch (field.type)
	{
	case MYSQL_TYPE_TINY:
		resBind.push_back(newBind(MYSQL_TYPE_TINY, BUFSIZE_TINYINT));
		break;
	case MYSQL_TYPE_SHORT:
		resBind.push_back(newBind(MYSQL_TYPE_SHORT, BUFSIZE_SMALLINT));
		break;
	case MYSQL_TYPE_INT24:
		resBind.push_back(newBind(MYSQL_TYPE_INT24, BUFSIZE_MEDIUMINT));
		break;
	case MYSQL_TYPE_LONG:
		resBind.push_back(newBind(MYSQL_TYPE_LONG, BUFSIZE_INT));
		break;
	case MYSQL_TYPE_LONGLONG:
		resBind.push_back(newBind(MYSQL_TYPE_LONGLONG, BUFSIZE_BIGINT));
		break;
	case MYSQL_TYPE_FLOAT:
		resBind.push_back(newBind(MYSQL_TYPE_FLOAT, BUFSIZE_FLOAT));
		break;
	case MYSQL_TYPE_DOUBLE:
		resBind.push_back(newBind(MYSQL_TYPE_DOUBLE, BUFSIZE_DOUBLE));
		break;
	case MYSQL_TYPE_VARCHAR:
	case MYSQL_TYPE_BLOB:
	case MYSQL_TYPE_TINY_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_STRING:
	case MYSQL_TYPE_VAR_STRING:
		resBind.push_back(newBind(field.type, field.length));
		break;
	case MYSQL_TYPE_TIMESTAMP:
	case MYSQL_TYPE_TIME:
	case MYSQL_TYPE_DATE:
	case MYSQL_TYPE_DATETIME:
	case MYSQL_TYPE_YEAR:
		resBind.push_back(newBind(field.type, BUFSIZE_TIME));
		break;
	}
}

ResultSet Statement::executeQuery()
{
	if(paramCount > 0)
		if (mysql_stmt_bind_param(stmt_ptr.get(), &paramBind[0]))
		{
			error = string(mysql_stmt_error(stmt_ptr.get()));
			return ResultSet(nullptr, false);
		}
	if (mysql_stmt_execute(stmt_ptr.get()))
	{
		error = string(mysql_stmt_error(stmt_ptr.get()));
		return ResultSet(nullptr, false);
	}
	if (mysql_stmt_bind_result(stmt_ptr.get(), &resBind[0]))
	{
		error = string(mysql_stmt_error(stmt_ptr.get()));
		return ResultSet(nullptr, false);
	}
	if (mysql_stmt_store_result(stmt_ptr.get()))
	{
		error = string(mysql_stmt_error(stmt_ptr.get()));
		return ResultSet(nullptr, false);
	}
	int cnt = mysql_stmt_num_rows(stmt_ptr.get());
	return ResultSet(new PreparedResult(this, cnt), true);
}

unsigned long long Statement::executeUpdate()
{
	if (paramCount > 0)
		if (mysql_stmt_bind_param(stmt_ptr.get(), &paramBind[0]))
		{
			error = string(mysql_stmt_error(stmt_ptr.get()));
			return 0;
		}
	if (mysql_stmt_execute(stmt_ptr.get()))
	{
		error = string(mysql_stmt_error(stmt_ptr.get()));
		return 0;
	}
	return mysql_stmt_affected_rows(stmt_ptr.get());
}

int Statement::getParamCount()
{
	return paramCount;
}

vector<std::string> Statement::fieldNames()
{
	vector<string> names;
	for (FieldMeta& f : fields)
		names.push_back(f.name);
	return names;
}

Res Statement::getRes(int index)
{
	if (index < 0 || index >= numField)
		return Res();
	int len = resBind[index].length ? *(resBind[index].length) : 0;
	return Res(resBind[index].buffer, len);
}

Res Statement::getRes(const string& columnName)
{
	unordered_map<string, int>::iterator it = nameToIndex.find(columnName);
	int index = it != nameToIndex.end() ? it->second : -1;
	if (index == -1)
		return Res(nullptr, 0);
	return getRes(index);
}

bool Statement::isValidParamIndex(int index)
{
	return index > 0 && index <= paramCount;
}

void Statement::setInt(int columnIndex, int value)
{
	if (isValidParamIndex(columnIndex))
		paramBind[columnIndex - 1] = newParam(MYSQL_TYPE_LONG,
			&value, sizeof(int), sizeof(int));
}

void Statement::setFloat(int columnIndex, float value)
{
	if (isValidParamIndex(columnIndex))
		paramBind[columnIndex - 1] = newParam(MYSQL_TYPE_FLOAT,
			&value, sizeof(float), sizeof(float));
}

void Statement::setDouble(int columnIndex, double value)
{
	if (isValidParamIndex(columnIndex))
		paramBind[columnIndex - 1] = newParam(MYSQL_TYPE_DOUBLE,
			&value, sizeof(double), sizeof(double));
}

void Statement::setString(int columnIndex, const string& value)
{
	if (isValidParamIndex(columnIndex))
		paramBind[columnIndex - 1] = newParam(MYSQL_TYPE_STRING, 
			value.c_str(), value.length() + 1, value.length());
}

void Statement::setTime(int columnIndex, Timestamp value, enum_field_types type)
{
	if (!isValidParamIndex(columnIndex))
		return;
	MYSQL_TIME mytime;
	switch (type)
	{
	case MYSQL_TYPE_DATETIME:
	case MYSQL_TYPE_TIMESTAMP:
		mytime = value.toMysqlTime(MYSQL_TIMESTAMP_DATETIME);
		break;

	case MYSQL_TYPE_DATE:
	case MYSQL_TYPE_YEAR:
		mytime = value.toMysqlTime(MYSQL_TIMESTAMP_DATE);
		break;

	case MYSQL_TYPE_TIME:
		mytime = value.toMysqlTime(MYSQL_TIMESTAMP_TIME);
		break;

	default: break;
	}
	paramBind[columnIndex - 1] = newParam(
		type, &mytime, 
		sizeof(mytime), sizeof(mytime));
}