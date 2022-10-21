#pragma once
#include <string>
#include "tinyorm/reflection.hpp"
#include "tinyorm/mysql4cpp/timestamp.h"

entity(User) {

	tableName(user);

	column(id, int, tags(name = id, type = integer, pk auto_increment));
	column(name, std::string, tags(name = name, type = varchar(50), default ""));
	column(createTime, Timestamp, tags(name = create_time, type = timestamp, default NOW()))
};
