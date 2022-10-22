#include <iostream>
#include "tinyorm/mysql4cpp/common.h"
#include "tinyorm/reflection.hpp"
#include "user.h"
#include "tinyorm/model.hpp"
#include "tinyorm/db.hpp"

int main()
{
	orm::DB db("localhost", 3306, "root", "20010910cheng", "orm");

	// User u;
	// u.name = "Kobe";
	// u.createTime = Timestamp::now();
	// db.model<User>().Create(u);

	// User user;
	// db.model<User>().Select("name", "create_time").Where("id = ?", 2).First(user);
	// printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
	
	// std::vector<User> users = db.model<User>().Where("id = ?", 1).Or("name = ?", "Kobe").All();
	// for (User& user : users)
	// 	printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());

	// std::vector<User> users = db.model<User>().Where("id < ?", 100).Page(1, 2).All();
	// for (User& user : users)
	// 	printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());

	// int rowAffected = db.model<User>().Where("id = ?", 1).Update("name", "Jordan");
	// std::cout << rowAffected << " rows affected" << std::endl;

	std::string sql = "select u.name from                                   \
                     user u left join user_role ur on u.id = ur.user_id   \
                     left join role r on r.id = ur.role_id                \
                     where r.role_name = ?";
	std::vector<User> users = db.model<User>().Raw(sql, "pg").Fetch();
  for (User& user : users)
      printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());

	return 0;
}
