//
// Created by zr on 22-11-10.
//

#include "cmdline/cmdline.h"
#include "../dataloader.h"
#include "../db.h"

#include <vector>
#include <fstream>
#include <string>
#include <unordered_set>

static std::string dbName;
static std::string dir;

bool gen(const std::string& table, SqlConn& conn)
{
    orm::DataLoader::FieldVec fields;
    orm::DataLoader::Indices indices;
    orm::DataLoader::NameToIndex nti;
    orm::DataLoader::loadFromDB(fields, indices, nti, dbName, table, conn);
    if(fields.size() == 0)
        return false;
    std::ofstream ofs(dir + "/" + orm::dbToCpp(table) + ".h");
    ofs << "#pragma once\n";
    ofs << "#include \"tinyorm/reflection.h\"\n";
    std::unordered_set<std::string> s;
    for(const orm::FieldMeta& f: fields)
        s.insert(f.cppType);
    if(s.count("std::string") > 0)
        ofs << "#include <string>\n";
    if(s.count("Timestamp") > 0)
        ofs << "#include \"tinyorm/mysql4cpp/timestamp.h\"\n";
    ofs << "\nentity(" << orm::dbToCpp(table, true) << ") {\n";
    ofs << "\ttableName(" << table << ");\n\n";
    for(const orm::FieldMeta& f: fields)
    {
        ofs << "\tcolumn(";
        ofs << orm::dbToCpp(f.columnName.value()) << ", " << f.cppType << ", "
        << "tags(name = " << f.columnName.value() << ", type = " << f.columnType.value();
        if(f.isPk)
            ofs << ", pk";
        if(f.autoInc)
            ofs << " auto_increment";
        if(f.notNull)
            ofs << ", not null";
        if(f._default.length() > 0)
            ofs << ", default " << f._default;
        if(f.extra.length() > 0)
            ofs << ", " << f.extra;
        ofs << "));\n";
    }
    if(indices.size() > 0)
        ofs << "\n";
    for(auto it = indices.cbegin(); it != indices.cend(); it++)
    {
        if(it->second.type == orm::Index::INDEX_TYPE_PRI)
            continue;
        ofs << "\t";
        if(it->second.type == orm::Index::INDEX_TYPE_MUL)
            ofs << "add_index(" << it->second.name;
        else if(it->second.type == orm::Index::INDEX_TYPE_UNI)
            ofs << "add_unique(" << it->second.name;
        for(const std::string& col: it->second.columns)
            ofs << ", " << col;
        ofs << ");\n";
    }
    ofs << "};";
    ofs.close();
    return true;
}

int main(int argc, char* argv[])
{
    // create a parser
    cmdline::parser parser;
    parser.add<std::string>("dir", 'd', "generating directory", false, ".");
    parser.add<std::string>("username", 'u', "username of database");
    parser.add<std::string>("password", 'p', "password of database");
    parser.add<std::string>("db", 's', "database name");
    parser.add<std::string>("table", 't', "tables for generating", false, "");
    parser.parse_check(argc, argv);
    dbName = parser.get<std::string>("db");
    dir = parser.get<std::string>("dir");
    std::string username = parser.get<std::string>("username");
    std::string password = parser.get<std::string>("password");
    std::vector<std::string> tbs = Util::split(parser.get<std::string>("table"), ",");
    orm::DB db("localhost", 3306, username, password, dbName);
    SqlConn conn = db.getConn();
    for(const std::string& t: tbs)
    {
        if(gen(t, conn))
            std::cout << "generate model for table " << t << " --- success" << std::endl;
        else
            std::cout << "generate model for table " << t << " --- failed" << std::endl;
    }
    conn.close();
    std::cout << "done" << std::endl;
    return 0;
}