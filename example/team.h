#pragma once
#include "tinyorm/reflection.hpp"
#include <string>

entity(Team) {
	tableName(team);

	column(id, int, tags(name = id, type = int(11), pk auto_increment, not null));
	column(name, std::string, tags(name = name, type = varchar(100), not null));

	add_unique(uni_name, name);
};