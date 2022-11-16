# tinyorm

c++17 实现的简单ORM(目前仅支持mysql)

```sh
mkdir build
cd build
cmake ..
make
```

**API**

- 增

  - 单条插入

    ```c++
    void orm::Model<T>::Create(const T& obj)
    ```

  - 批量插入

    ```c++
    void orm::Model<T>::Create(std::vector<T>& objs)
    ```

- 删

  ```c++
  void orm::Model<T>::Delete()
      
  //一般要与条件连用
  db.Model<T>().Where("id = ?", 1).Delete();
  ```

- 改

  ```c++
  //Update函数参数前半部分是要更新的列名，后半部分是对应的值，列和值个数要相等
  //返回值是受影响行数
  int orm::Model<T>::Update(column1, column2, column3..., value1, value2, value3...)
      
  //一般与条件连用
  db.Model<T>().Where("id = ?", 1).Update("username", "password", "new_username", "new_password");
  ```

- 单表查询

  ```c++
  //Select()功能等同于sql语句中的SELECT
  orm::Model<T>& orm::Model<T>::Select(column1, column2, column3...)
  //First()获取第一个结果
  int orm::Model<T>::First(T& obj)
  //All()获取所有结果
  std::vector<T> orm::Model<T>::All()
      
      
  db.model<User>().Select("name", "create_time").Where("id = ?", 2).First(user);
  //使用Page(currentPage, pageSize)函数分页
  std::vector<User> users = db.model<User>().Where("id < ?", 100).Page(1, 3).All();
  ```

- 原生SQL执行

  用于执行较为复杂的联表查询等

  ```c++
  //Raw()用于执行原生sql
  orm::Model<T>& orm::Model<T>::Raw(const std::string& sql, arg1, arg2...)
  //Fetch()用于在执行查询的Raw()之后获取结果
  //Exec()用于执行非查询的raw sql
  std::vector<T> orm::Model<T>::Fetch()
  int orm::Model<T>::Exec()
  ```

- 自动迁移

  ```c++
  //参数传入需要自动迁移的类的空对象即可
  orm::DB::AutoMigrate(T1(), T2(), T3()..。)
  ```

  

**Example**

```c++
#pragma once
#include <string>
#include "tinyorm/reflection.hpp"
#include "tinyorm/mysql4cpp/timestamp.h"

//定义表结构
entity(User) {
        //指定表名(默认Table -> table, TableName -> table_name)
	tableName(user);

        //定义列(变量名, 数据类型, tags(name = 列名, type = 列数据类型, 补充信息))
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
	orm::DB db("localhost", 3306, "username", "password", "db");

	//自动迁移
	db.AutoMigrate(User(), Role(), UserRole());
	//Create
	//***************************************************************************************
	
	//User user;
	//user.name = "Durant";
	//user.age = 34;
	//user.createTime = Timestamp::now();
	//db.model<User>().Create(user);
	//
	//std::vector<User> users = {User("Jordan", 59, Timestamp::now()),
	//                           User("James", 38, Timestamp::now()),
	//                           User("Curry", 32, Timestamp::now()),
	//                           User("Irving", 30, Timestamp::now())};
	//
	//db.model<User>().Create(users);
	//
	//std::vector<Role> roles = {Role(1, "PG"), Role(2, "SG"),
	//                           Role(3, "SF"), Role(4, "PF"), Role(5, "C")};
	//db.model<Role>().Create(roles);
	//
	//std::vector<UserRole> urs = {UserRole(1, 2), UserRole(2, 2), UserRole(3, 3), UserRole(4, 1), UserRole(5, 1)};
	//db.model<UserRole>().Create(urs);
        
    	//Select
     	//******************************************************************************

	//User user;
	//
	//db.model<User>().Select("name", "create_time").Where("id = ?", 2).First(user);
	//printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
	//
	//std::vector<User> us = db.model<User>().Where("id = ?", 1).Or("name = ?", "Jordan").All();
	//for(User& u: us)
	//    printf("User{id = %d, name = %s, create_time = %s}\n", u.id, u.name.c_str(), u.createTime.toFormattedString().c_str());
	//
	//us = db.model<User>().Where("id < ?", 100).Page(1, 3).All();
	//for(User& u: us)
	//    printf("User{id = %d, name = %s, create_time = %s}\n", u.id, u.name.c_str(), u.createTime.toFormattedString().c_str());

	//Update
	//****************************************************************************
	
	//int rowAffected = db.model<User>().Where("id = ?", 1).Update("name", "team_id", "Kevin Durant", 1);
	//std::cout << rowAffected << " rows affected" << std::endl;

	//Raw sql
	//****************************************************************************

	std::string raw = "select u.name from "
                      "user u left join user_role ur on u.id = ur.user_id "
                      "left join role r on r.id = ur.role_id "
                      "where r.role_name = ?";
   	 std::vector<User> users = db.model<User>().Raw(raw, "SG").Fetch();
    	 for(User& u: users)
		printf("User{id = %d, name = %s, create_time = %s}\n", u.id, u.name.c_str(), u.createTime.toFormattedString().c_str());

	return 0;
}
```



- [x] select
- [x] update
- [x] create
- [x] delete
- [x] 分页
- [x] raw sql
- [x] 自动迁移
- [ ] 一级缓存
- [ ] 二级缓存
