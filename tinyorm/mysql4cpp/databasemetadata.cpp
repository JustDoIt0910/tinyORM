//
// Created by zr on 22-10-29.
//

#include "databasemetadata.h"
#include "sstream"
#include "spdlog/spdlog.h"

DatabaseMetadata::DatabaseMetadata(SqlConn&& _conn): conn(std::move(_conn)){}

DatabaseMetadata::DatabaseMetadata(DatabaseMetadata&& meta):conn(std::move(meta.conn)) {}

DatabaseMetadata& DatabaseMetadata::operator=(DatabaseMetadata && meta)
{
    conn = std::move(meta.conn);
    return *this;
}

ResultSet DatabaseMetadata::getTables(const std::string& schemaPattern, const std::string tablePattern,
                                      const std::vector<std::string>& select = {})
{
    std::stringstream sql;
    sql << "SELECT ";
    if(select.size() == 0)
        sql << "*";
    else
    {
        for(int i = 0; i < select.size(); i++)
        {
            if(i > 0)
                sql << ", ";
            sql << select[i];
        }
    }
    sql << " FROM TABLES WHERE TABLE_SCHEMA LIKE '"
    << schemaPattern << "' AND TABLE_NAME LIKE '" << tablePattern << "'";
    //spdlog::info(sql.str());
    return conn.executeQuery(sql.str());
}

ResultSet DatabaseMetadata::getColumns(const std::string& schemaPattern, const std::string tablePattern,
                                       const std::string columnPattern, const std::vector<std::string>& select = {})
{
    std::stringstream sql;
    sql << "SELECT ";
    if(select.size() == 0)
        sql << "*";
    else
    {
        for(int i = 0; i < select.size(); i++)
        {
            if(i > 0)
                sql << ", ";
            sql << select[i];
        }
    }
    sql <<" FROM COLUMNS WHERE TABLE_SCHEMA LIKE '"
        << schemaPattern << "' AND TABLE_NAME LIKE '" << tablePattern
        << "' AND COLUMN_NAME LIKE '" << columnPattern << "'";
    //spdlog::info(sql.str());
    return conn.executeQuery(sql.str());
}

ResultSet DatabaseMetadata::getIndices(const std::string& schemaPattern, const std::string tablePattern,
                                      const std::vector<std::string>& select = {})
{
    std::stringstream sql;
    sql << "SELECT ";
    if(select.size() == 0)
        sql << "*";
    else
    {
        for(int i = 0; i < select.size(); i++)
        {
            if(i > 0)
                sql << ", ";
            sql << select[i];
        }
    }
    sql << " FROM STATISTICS WHERE TABLE_SCHEMA LIKE '"
        << schemaPattern << "' AND TABLE_NAME LIKE '" << tablePattern << "'";
    //spdlog::info(sql.str());
    return conn.executeQuery(sql.str());
}