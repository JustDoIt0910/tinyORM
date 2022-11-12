//
// Created by zr on 22-10-29.
//
#pragma once

#include "resultset.h"
#include "sqlconn.h"

class DatabaseMetadata {
public:
    DatabaseMetadata(SqlConn&& conn);
    DatabaseMetadata(DatabaseMetadata&) = delete;
    DatabaseMetadata& operator=(const DatabaseMetadata&) = delete;
    DatabaseMetadata(DatabaseMetadata&&);
    DatabaseMetadata& operator=(DatabaseMetadata&&);
    ResultSet getTables(const std::string& schemaPattern, const std::string tablePattern,
                        const std::vector<std::string>& select);
    ResultSet getColumns(const std::string& schemaPattern, const std::string tablePattern,
                         const std::string columnPattern, const std::vector<std::string>& select);
    ResultSet getIndices(const std::string& schemaPattern, const std::string tablePattern,
                      const std::vector<std::string>& select);

private:
    SqlConn conn;
};


