//
// Created by zr on 22-11-7.
//

#pragma once
#include "tinyorm/reflection.h"

entity(Role) {
    tableName(role);

    column(id, int, tags(name = id, type = integer, pk auto_increment));
    column(RoleKey, int, tags(name = role_key, type = integer, not null));
    column(RoleName, std::string, tags(name = role_name, type = varchar(50), unique));

    Role(){}
    Role(int key, const std::string& name): RoleKey(key), RoleName(name){}
};
