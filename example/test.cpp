#include <iostream>
#include "user.h"
#include "role.h"
#include "userRole.h"
#include "tinyorm/db.h"
#include "tinyorm/model.h"

int main()
{
    orm::DB db("localhost", 3306, "root", "20010910cheng", "orm");
    db.AutoMigrate(User(), Role(), UserRole());

//    User user;
//    user.name = "Durant";
//    user.age = 34;
//    user.createTime = Timestamp::now();
//    db.model<User>().Create(user);
//
//    std::vector<User> users = {User("Jordan", 59, Timestamp::now()),
//                               User("James", 38, Timestamp::now()),
//                               User("Curry", 32, Timestamp::now()),
//                               User("Irving", 30, Timestamp::now())};
//
//    db.model<User>().Create(users);
//
//    std::vector<Role> roles = {Role(1, "PG"), Role(2, "SG"),
//                               Role(3, "SF"), Role(4, "PF"), Role(5, "C")};
//    db.model<Role>().Create(roles);
//
//    std::vector<UserRole> urs = {UserRole(1, 2), UserRole(2, 2), UserRole(3, 3), UserRole(4, 1), UserRole(5, 1)};
//    db.model<UserRole>().Create(urs);

//    User user;
//
//    db.model<User>().Select("name", "create_time").Where("id = ?", 2).First(user);
//    printf("User{id = %d, name = %s, create_time = %s}\n", user.id, user.name.c_str(), user.createTime.toFormattedString().c_str());
//
//    std::vector<User> us = db.model<User>().Where("id = ?", 1).Or("name = ?", "Jordan").All();
//    for(User& u: us)
//        printf("User{id = %d, name = %s, create_time = %s}\n", u.id, u.name.c_str(), u.createTime.toFormattedString().c_str());
//
//    us = db.model<User>().Where("id < ?", 100).Page(1, 3).All();
//    for(User& u: us)
//        printf("User{id = %d, name = %s, create_time = %s}\n", u.id, u.name.c_str(), u.createTime.toFormattedString().c_str());

//    int rowAffected = db.model<User>().Where("id = ?", 1).Update("name", "team_id", "Kevin Durant", 1);
//    std::cout << rowAffected << " rows affected" << std::endl;

    std::string raw = "select u.name from "
                      "user u left join user_role ur on u.id = ur.user_id "
                      "left join role r on r.id = ur.role_id "
                      "where r.role_name = ?";
    std::vector<User> users = db.model<User>().Raw(raw, "SG").Fetch();
    for(User& u: users)
        printf("User{id = %d, name = %s, create_time = %s}\n", u.id, u.name.c_str(), u.createTime.toFormattedString().c_str());


return 0;
}
