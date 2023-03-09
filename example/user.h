#pragma once
#include <string>
#include "tinyorm/reflection.h"
#include "tinyorm/mysql4cpp/timestamp.h"

entity(User) {

	tableName(user);

	column(id, int, tags(name = ID, type = integer, pk auto_increment));
	column(name, std::string, tags(name = name, type = varchar(50), default ""));
    column(age, int, tags(name = age, type = integer, not null));
    column(teamId, int, tags(name = team_id, type = integer));
	column(createTime, Timestamp, tags(name = create_time, type = timestamp, default NOW()));

    add_unique(uni_name, name);
    add_index(index_age, age);

    User(const std::string& _name, int _age, Timestamp ts): name(_name), age(_age), createTime(ts){}
    User(){}
};
