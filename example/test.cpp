#include <iostream>
#include "tinyorm/mysql4cpp/common.h"
#include "tinyorm/reflection.hpp"
#include "user.h"
#include "tinyorm/model.hpp"
#include "tinyorm/db.hpp"

int main()
{
	orm::DB db("localhost", 3306, "root", "20010910cheng", "test");

	
	
	std::vector<User> users = db.model<User>().Where("id < ?", 100).Page(2, 5).All();
	for (User& user : users)
		printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());

	return 0;
}