#pragma once
#include "tinyorm/reflection.hpp"

entity(UserRole) {
	tableName(user_role);

	column(userId, int, tags(name = user_id, type = int(11)));
	column(roleId, int, tags(name = role_id, type = int(11)));

	add_unique(uni, user_id, role_id);

    UserRole(){}
    UserRole(int uid, int rid): userId(uid), roleId(rid){}
};