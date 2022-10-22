# tinyorm
c++17 实现的简单ORM(仅支持mysql)

```sh
mkdir build
cd build
cmake ..
make
```

**Usage**
```c++
#pragma once
#include <string>
#include "tinyorm/reflection.hpp"
#include "tinyorm/mysql4cpp/timestamp.h"

//定义表结构
entity(User) {
        //指定表名(默认为类名)
	tableName(user);

        //定义列(变量名, 数据类型, tags(补充信息))
	column(id, int, tags(name = id, type = integer, pk auto_increment));
	column(name, std::string, tags(name = name, type = varchar(50), default ""));
	column(createTime, Timestamp, tags(name = create_time, type = timestamp, default NOW()))
};
```

```c++
#include <iostream>
#include "tinyorm/mysql4cpp/common.h"
#include "tinyorm/reflection.hpp"
#include "user.h"
#include "tinyorm/model.hpp"
#include "tinyorm/db.hpp"

int main()
{
	orm::DB db("localhost", 3306, "root", "20010910cheng", "orm");

    //Create
    ***************************************************************************************
	// User u;
	// u.name = "Kobe";
	// u.createTime = Timestamp::now();
	// db.model<User>().Create(u);
        
     //Select
    ***************************************************************************************

	// User user;
	// db.model<User>().Select("name", "create_time").Where("id = ?", 2).First(user);
	// printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
	
	// std::vector<User> users = db.model<User>().Where("id = ?", 1).Or("name = ?", "Kobe").All();
	// for (User& user : users)
	// 	printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());

	// std::vector<User> users = db.model<User>().Where("id < ?", 100).Page(1, 2).All();
	// for (User& user : users)
	// 	printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
        
        
     //Update
    ***************************************************************************************

	// int rowAffected = db.model<User>().Where("id = ?", 1).Update("name", "Jordan");
	// std::cout << rowAffected << " rows affected" << std::endl;
        
        
    //Raw sql
    ***************************************************************************************

	std::string sql = "select u.name from                                   \
                     user u left join user_role ur on u.id = ur.user_id   \
                     left join role r on r.id = ur.role_id                \
                     where r.role_name = ?";
	std::vector<User> users = db.model<User>().Raw(sql, "pg").Fetch();
  for (User& user : users)
      printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());

	return 0;
}
```



- ~~select~~
- ~~update~~
- ~~create~~
- delete
- ~~分页~~
- ~~raw sql~~
- 自动迁移
