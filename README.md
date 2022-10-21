# tinyorm
c++17 实现的简单ORM(仅支持mysql)
目前只在windows下测试过

```sh
cd build
cmake ../
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
int main()
{
	orm::DB db("localhost", 3306, "your_username", "your_password", "db_name");
	
  //select
  std::vector<User> users = db.model<User>().Where("id < ?", 100).Page(2, 5).All();
  for (User& user : users)
      printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
    
  User user;
  db.model<User>().Select("name", "create_time").Where("id = ?", 10).First(user);
  printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
  
  std::vector<User> users = db.model<User>().Where("name = ?", "james").Or("name = ?", "jack").All();
  for (User& user : users)
      printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
    
  //create
  User u;
  u.name = "Kobe";
  u.createTime = Timestamp::now();
  db.model<User>().Create(u);
  
  //update
  int rowAffected = db.model<User>().Where("id = ?", 1).Update("name", "Tom");
  
  //raw sql
  std::string sql = "select u.name from                                   \
                     user u left join user_role ur on u.id = ur.user_id   \
                     left join role r on r.id = ur.role_id                \
                     where r.role_name = ?";
  std::vector<User> users = db.model<User>().Raw(sql, "student").Fetch();
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
