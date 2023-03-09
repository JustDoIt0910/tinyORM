//
// Created by zr on 22-11-2.
//

#ifndef __ORM_DATALOADER_HPP__
#define __ORM_DATALOADER_HPP__

#include "datatype.h"
#include "mysql4cpp/sqlconn.h"
#include "mysql4cpp/databasemetadata.h"
#include <unordered_map>
#include <fstream>
#include "bytearray.h"

namespace orm {
#define uchar unsigned char

#define HEADER_MARKER   0xFC
#define COLUMN_MARKER   0xFD
#define INDEX_MARKER    0xFE

    void read_combined(std::ifstream& ifs, std::string& p1, std::string& p2, char len, const std::string& sp);

    class DataLoader
    {
    public:
        using FieldVec = std::vector<FieldMeta>;
        using Indices = std::unordered_map<std::string, Index>;
        using NameToIndex = std::unordered_map<std::string, int>;

//        Header, ColumnField, IndexField都是保存在数据文件中的结构
//        每个model(对应每张表)有自己的Header, ColumnField, IndexField(如果有索引的话)
//        例如有model1,2,3需要自动迁移，那么生成的数据文件结构：
//        |Header(model1)|ColumnField1|ColumnField2|ColumnField3|IndexField1|IndexField2|
//        |Header(model2)|ColumnField1|ColumnField2|ColumnField3|IndexField1|
//        |Header(model3)|ColumnField1|ColumnField2|

        struct Header
        {
            uchar marker;
            //model_len是当前model的数据长度(不算Header), 该字段用于搜索对应model数据的时候跳过其他model
            short model_len;
            char header_len;
            std::string _className;
            std::string _tableName;

            Header(){}

            Header(const std::string cName, const std::string tName):
                    marker(HEADER_MARKER), _className(cName), _tableName(tName), model_len(0){
                header_len = sizeof(short) + 2 + _className.length() + _tableName.length() + 1;
            }

            ByteArray toByteArray()
            {
                ByteArray arr(header_len + 1);
                arr.fill(marker, 1);
                arr.fill((const char*)&model_len, 2);
                arr.fill(header_len, 1);
                arr.fill(_className.length() + _tableName.length() + 1, 1);
                arr.fill(_tableName.c_str(), _tableName.length());
                arr.fill("|", 1);
                arr.fill(_className.c_str(), _className.length());
                return arr;
            }
        };

        struct ColumnField
        {
            uchar marker;
            std::string columnId;
            std::string dataType;
            char flags;
            std::string columnDefault;
            std::string columnExtra;

            short len;

            ColumnField(const FieldMeta& meta): marker(COLUMN_MARKER)
            {
                columnId = meta.getColumnName() + "|" + meta.fieldName;
                dataType = meta.getColumnType() + "|" + meta.cppType;
                flags = (meta.isPk * 8 + meta.autoInc * 4 + meta.hasIndex * 2 + meta.notNull) & 0x0F;
                columnDefault = meta._default;
                columnExtra = meta.extra;
                len = 2 + columnId.length() +
                        1 + dataType.length() +
                        2 + columnDefault.length() +
                        1 + columnExtra.length();
            }

            ByteArray toByteArray()
            {
                ByteArray arr(len);
                arr.fill(marker, 1);
                arr.fill(columnId.length(), 1);
                arr.fill(columnId.c_str(), columnId.length());
                arr.fill(dataType.length(), 1);
                arr.fill(dataType.c_str(), dataType.length());
                arr.fill(flags, 1);
                arr.fill(columnDefault.length(), 1);
                arr.fill(columnDefault.c_str(), columnDefault.length());
                arr.fill(columnExtra.length(), 1);
                arr.fill(columnExtra.c_str(), columnExtra.length());
                return arr;
            }
        };

        struct IndexField
        {
            uchar marker;
            char type;
            std::string indexName;
            std::string indexColumns;

            char len;

            IndexField(const Index& index):
                marker(INDEX_MARKER), type(index.type), indexName(index.name)
            {
                for(int i = 0; i < index.columns.size(); i++)
                {
                    if(i > 0)
                        indexColumns.append("|");
                    indexColumns.append(index.columns[i]);
                }
                len = 2 + 1 + indexName.length() + 1 + indexColumns.length();
            }

            ByteArray toByteArray()
            {
                ByteArray arr(len);
                arr.fill((const char*)this, 2);
                arr.fill(indexName.length(), 1);
                arr.fill(indexName.c_str(), indexName.length());
                arr.fill(indexColumns.length(), 1);
                arr.fill(indexColumns.c_str(), indexColumns.length());
                return arr;
            }
        };

        //从information_schema中加载表的元信息，在自动迁移时要用到
        static void loadFromDB(FieldVec& fields, Indices& indices, NameToIndex& nameToIndex,
                                           const std::string& db, const std::string& tableName,
                                           SqlConn &conn)
        {
            DatabaseMetadata meta = conn.getMetadata();
            ResultSet rs = meta.getColumns(db, tableName, "%%",
                                           {"COLUMN_NAME", "COLUMN_TYPE", "IS_NULLABLE", "COLUMN_DEFAULT", "EXTRA"});
            if(!rs.hasNext())
                return;
            int index = 0;
            while(rs.hasNext())
            {
                rs.next();
                FieldMeta field;
                field.columnName = rs.getString("COLUMN_NAME");
                field.fieldName = dbToCpp(field.columnName.value());
                field.columnType = rs.getString("COLUMN_TYPE");
                field.cppType = dbTypeToCppType(field.columnType.value());
                field.notNull = (rs.getString("IS_NULLABLE") == "NO");
                field._default = rs.getString("COLUMN_DEFAULT");
                field.extra = rs.getString("EXTRA");
                if(field.extra == "auto_increment")
                {
                    field.autoInc = true;
                    field.extra = "";
                }
                nameToIndex[field.columnName.value()] = index++;
                fields.push_back(std::move(field));
            }
            rs = meta.getIndices(db, tableName,
                                 {"INDEX_NAME", "NON_UNIQUE", "COLUMN_NAME"});
            while(rs.hasNext())
            {
                rs.next();
                std::string name = rs.getString("INDEX_NAME");
                auto it = indices.find(name);
                if(it != indices.end())
                {
                    it->second.columns.push_back(rs.getString("COLUMN_NAME"));
                    continue;
                }
                Index ind;
                ind.name = name;
                ind.columns.push_back(rs.getString("COLUMN_NAME"));
                if(name == "PRIMARY")
                {
                    ind.type = Index::INDEX_TYPE_PRI;
                    fields[nameToIndex[ind.columns[0]]].isPk = true;
                }
                else
                    ind.type = (rs.getLong("NON_UNIQUE") == 1) ? Index::INDEX_TYPE_MUL : Index::INDEX_TYPE_UNI;
                indices[name] = ind;
            }
        }

        //将元信息保存到数据文件中，对于当前数据库，会在models文件夹下生成一个名为model_库名.dat的二进制文件
        //自动迁移的实现就是将当前model与从数据文件中加载的上一版本信息对比。
        static void saveModel(std::vector<FieldMeta>& fields,
                                          std::unordered_map<std::string, Index> &indices, const std::string &db,
                                          Header& header)
        {
            struct stat st{};
            if(::stat("models", &st) < 0 || !S_ISDIR(st.st_mode))
                mkdir("models", S_IRWXU);
            std::ofstream ofs("models/model_" + db + ".dat", std::ios::binary | std::ios::app);
            std::vector<ByteArray> cols;
            for(FieldMeta& col: fields)
            {
                ColumnField columnField(col);
                cols.push_back(columnField.toByteArray());
                header.model_len += columnField.len;
            }
            std::vector<ByteArray> inds;
            for(auto it = indices.cbegin(); it != indices.cend(); it++)
            {
                IndexField indexField(it->second);
                inds.push_back(indexField.toByteArray());
                header.model_len += indexField.len;
            }
            ByteArray headerData = header.toByteArray();
            ofs.write(headerData.value(), headerData.length());
            for(ByteArray& arr: cols)
                ofs.write(arr.value(), arr.length());
            for(ByteArray& arr: inds)
                ofs.write(arr.value(), arr.length());
            ofs.close();
        }

        //从数据文件中加载出对应model的列，索引信息
        static void loadFromFile(const std::string& db, const std::string& className, std::string& tableName,
                                             FieldVec& fields, Indices& indices, NameToIndex& nameToIndex)
        {
            std::ifstream ifs("models/model_" + db + ".dat", ios::binary);
            if(!ifs.is_open())
                return;
            PARSING_STATE state = PARSE_MARKER;
            uchar marker;
            Header header; FieldMeta fieldMeta; Index index;
            bool found = false;
            int columnIndex = 0;
            while(state != PARSE_FAILED && state != PARSE_SUCCESS)
            {
                switch(state)
                {
                    case PARSE_MARKER:
                        if(parseMarker(ifs, &marker))
                            state = PARSE_FIELD;
                        else
                            state = PARSE_SUCCESS;
                        break;
                    case PARSE_FIELD:
                        switch(marker)
                        {
                            case HEADER_MARKER:
                                if(found)
                                {
                                    state = PARSE_SUCCESS;
                                    break;
                                }
                                parseHeader(ifs, header);
                                if(header._className != className)
                                    ifs.seekg(header.model_len, std::ios_base::cur);
                                else
                                {
                                    tableName = header._tableName;
                                    found = true;
                                }
                                state = PARSE_MARKER;
                                break;
                            case COLUMN_MARKER:
                                parseColumnField(ifs, fieldMeta);
                                nameToIndex[fieldMeta.columnName.value()] = columnIndex++;
                                fields.push_back(std::move(fieldMeta));
                                state = PARSE_MARKER;
                                break;
                            case INDEX_MARKER:
                                parseIndexField(ifs, index);
                                indices[index.name] = index;
                                state = PARSE_MARKER;
                                break;
                            default:
                                state = PARSE_FAILED;
                                break;
                        }
                        break;
                }
            }
            ifs.close();
        }

    private:
        //数据文件解析FSM的状态
        enum PARSING_STATE
        {
            PARSE_MARKER,
            PARSE_FIELD,
            PARSE_SUCCESS,
            PARSE_FAILED
        };

        static bool parseMarker(std::ifstream &ifs, uchar* marker)
        {
            ifs.read((char*)marker, 1);
            if(ifs.gcount() != 1)
                return false;
            return true;
        }

        static void parseHeader(std::ifstream &ifs, orm::DataLoader::Header &header)
        {
            ifs.read((char*)&header.model_len, 2);
            ifs.read((char*)&header.header_len, 1);
            char len;
            ifs.read(&len, 1);
            read_combined(ifs, header._tableName, header._className, len, "|");
        }

        static void parseColumnField(std::ifstream &ifs, FieldMeta& fieldMeta)
        {
            char len;
            ifs.read(&len, 1);
            std::string columName;
            read_combined(ifs, columName, fieldMeta.fieldName, len, "|");
            fieldMeta.columnName = columName;
            ifs.read(&len, 1);
            std::string columnType;
            read_combined(ifs, columnType, fieldMeta.cppType, len, "|");
            fieldMeta.columnType = columnType;
            char flags;
            ifs.read(&flags, 1);
            fieldMeta.isPk = (flags >> 3) & 1;
            fieldMeta.autoInc = (flags >> 2) & 1;
            fieldMeta.hasIndex = (flags >> 1) & 1;
            fieldMeta.notNull = flags & 1;
            ifs.read(&len, 1);
            ByteArray arr;
            if(len > 0)
            {
                arr = ByteArray(ifs, len);
                fieldMeta._default = std::string(arr.value(), arr.length());
            }
            ifs.read(&len, 1);
            if(len > 0)
            {
                arr = ByteArray(ifs, len);
                fieldMeta.extra = std::string(arr.value(), arr.length());
            }
        }

        static void parseIndexField(std::ifstream &ifs, orm::Index &index)
        {
            char type;
            ifs.read(&type, 1);
            index.type = (Index::IndexType)type;
            char len;
            ifs.read(&len, 1);
            ByteArray arr(ifs, len);
            index.name = std::string(arr.value(), arr.length());
            ifs.read(&len, 1);
            arr = ByteArray(ifs, len);
            std::string columns(arr.value(), arr.length());
            index.columns = Util::split(columns, "|");
        }
    };

    inline void read_combined(std::ifstream& ifs, std::string& p1, std::string& p2, char len, const std::string& sp)
    {
        ByteArray arr(ifs, len);
        std::string s(arr.value(), arr.length());
        std::vector<std::string> v = Util::split(s, sp);
        p1 = v[0];
        p2= v[1];
    }
}

#endif
