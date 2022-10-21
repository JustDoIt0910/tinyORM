#pragma once
#include <string>
#include "mysql.h"
#include <vector>

#define BUFSIZE_TINYINT			1
#define BUFSIZE_SMALLINT		2
#define BUFSIZE_MEDIUMINT		3
#define BUFSIZE_INT				4
#define BUFSIZE_BIGINT			8
#define BUFSIZE_FLOAT			4
#define BUFSIZE_DOUBLE			8
#define BUFSIZE_TIME			sizeof(MYSQL_TIME)

struct FieldMeta
{
	std::string name;
	unsigned long length;
	enum enum_field_types type;

	FieldMeta(const MYSQL_FIELD& field)
	{
		name = std::string(field.name, field.name_length);
		length = field.length;
		type = field.type;
	}
};

class Util
{
public:
	static std::string strip(const std::string& str);
	static std::vector<std::string> split(const std::string& str, const std::string& sep);
	static void toLowerCase(std::string& str);
	static void toUpperCase(std::string& str);
};
