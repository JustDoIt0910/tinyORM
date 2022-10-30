//
// Created by zr on 22-10-29.
//

#include "databasemetadata.h"
#include "sstream"

DatabaseMetadata::DatabaseMetadata(SqlConn&& _conn): conn(std::move(_conn)){}

DatabaseMetadata::DatabaseMetadata(DatabaseMetadata&& meta):conn(std::move(meta.conn)) {}

DatabaseMetadata& DatabaseMetadata::operator=(DatabaseMetadata && meta)
{
    conn = std::move(meta.conn);
    return *this;
}

ResultSet DatabaseMetadata::getTables(const std::string& schemaPattern, const std::string tablePattern)
{
    std::stringstream sql;
    sql << "SELECT * FROM TABLES WHERE TABLE_SCHEMA LIKE '"
    << schemaPattern << "' AND TABLE_NAME LIKE '" << tablePattern << "'";
    return conn.executeQuery(sql.str());
}

ResultSet DatabaseMetadata::getColumns(const std::string& schemaPattern, const std::string tablePattern, const std::string columnPattern)
{
    std::stringstream sql;
    sql << "SELECT * FROM COLUMNS WHERE TABLE_SCHEMA LIKE '"
        << schemaPattern << "' AND TABLE_NAME LIKE '" << tablePattern
        << "' AND COLUMN_NAME LIKE '" << columnPattern << "'";
    return conn.executeQuery(sql.str());
}