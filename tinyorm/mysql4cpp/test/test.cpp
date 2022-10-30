//
// Created by zr on 22-10-25.
//

#include "database.h"
#include "sqlconn.h"
#include <string>
#include <iostream>

int main()
{
    Database db("orm", "20010910cheng");
    SqlConn conn = db.getConn();
    DatabaseMetadata metadata = conn.getMetadata();
    ResultSet rs = metadata.getTables("orm", "user");
    if(rs.isValid())
    {
        while(rs.hasNext())
        {
            rs.next();
            std::cout << rs.getString("TABLE_NAME")
            << std::endl;
        }
    }
    return 0;
}