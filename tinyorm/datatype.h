//
// Created by zr on 22-10-29.
//

#ifndef __ORM_DATATYPE_HPP__
#define __ORM_DATATYPE_HPP__

#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <unordered_set>
#include "mysql4cpp/common.h"
#include "naming.h"

namespace orm::detail {
    //通过column宏提取出的原始元数据
    struct Metadata {
        //变量类型
        std::string type;
        //偏移地址
        int offset;
        //列的详细描述，包含列名，数据库中类型，是否非空，默认值，额外信息等
        std::string tag;

        Metadata(const std::string& _type, int _offset, const std::string& _tag) :
                type(_type), offset(_offset), tag(_tag){}

        Metadata(const std::string& _type, int _offset): Metadata(_type, _offset, ""){}
        Metadata(){}
    };
}

namespace orm {

    //描述索引信息
    struct Index
    {
        enum IndexType
        {
            //唯一索引
            INDEX_TYPE_UNI,
            //普通索引
            INDEX_TYPE_MUL,
            //主键
            INDEX_TYPE_PRI
        } type;

        //索引名
        std::string name;

        //索引列
        std::vector<std::string> columns;

        Index(const std::string& _name, IndexType _type): name(_name),type(_type) {}
        Index() {}
        Index(Index&& index): type(index.type)
        {
            name = std::move(index.name);
            columns.swap(index.columns);
        }
        Index(const Index& index): type(index.type), name(index.name){columns = index.columns;}
        Index& operator=(Index&& index)
        {
            type = index.type;
            name = std::move(index.name);
            columns.swap(index.columns);
            return *this;
        }
        Index& operator=(const Index& index)
        {
            type = index.type;
            name = index.name;
            columns = index.columns;
            return *this;
        }
        bool operator==(const Index& index)
        {
            if(name != index.name || type != index.type)
                return false;
            std::unordered_set<std::string> s;
            for(std::string& col: columns)
                s.insert(col);
            for(const std::string& col: index.columns)
                s.insert(col);
            if(s.size() != columns.size() || s.size() != index.columns.size())
                return false;
            return true;
        }
    };

    //描述详细的列信息, 由Metadata处理而来
    class FieldMeta
    {
    public:
        //变量名
        std::string fieldName;
        //变量类型
        std::string cppType;

        //数据库表中的列类型
        std::optional<std::string> columnType;

        //数据库表中的列名
        std::optional<std::string> columnName;

        //附加元信息
        std::string extra;

        //是否为主键
        bool isPk;
        bool autoInc;
        //该列是否具有索引
        bool hasIndex;
        bool notNull;
        std::string _default;
        Index index;

        //和Metadata中offset意义相同
        int offset;
        //在插入数据的时候是否忽略该列，比如设置了自增的主键
        bool ignore;

        FieldMeta(): isPk(false), autoInc(false), hasIndex(false), notNull(false), ignore(false){}

        FieldMeta(const std::string& _fieldName, const orm::detail::Metadata& meta):
            fieldName(_fieldName), cppType(meta.type), offset(meta.offset),
            isPk(false), ignore(false), autoInc(false), hasIndex(false), notNull(false)
        {
            //构造FieldMeta的过程主要就是处理和分离Metadata的tag中包含的信息
            std::vector<std::string> tags = Util::split(Util::strip(meta.tag), ",");
            auto it = tags.cbegin();
            for (; it != tags.cend(); it++)
            {
                size_t eq_pos = it->find_first_of("=");
                if (eq_pos != it->npos)
                {
                    std::string k = Util::strip(it->substr(0, eq_pos));
                    Util::toLowerCase(k);
                    if (k == "name")
                        columnName = Util::strip(it->substr(eq_pos + 1));
                    else if(k == "type")
                        columnType = Util::strip(it->substr(eq_pos + 1));
                }
                else
                {
                    bool checkNotNull = false;
                    bool checkDefault = false;
                    std::vector<std::string> exts = Util::split(Util::strip(*it), " ");
                    for (std::string ext : exts)
                    {
                        if(checkDefault)
                        {
                            _default = ext;
                            continue;
                        }
                        Util::toLowerCase(ext);
                        if(checkNotNull && ext == "null")
                        {
                            notNull = true;
                            checkNotNull = false;
                            continue;
                        }
                        if (ext == "pk" || ext == "primary_key" || ext == "primary key")
                        {
                            isPk = true;
                            hasIndex = true;
                            index.type = Index::INDEX_TYPE_PRI;
                            index.columns.push_back(getColumnName());
                        }
                        else if (ext == "auto_increment" || ext == "auto increment")
                            autoInc = true;
                        else if (ext == "ignore")
                            ignore = true;
                        else if(ext == "index")
                        {
                            hasIndex = true;
                            index.type = Index::INDEX_TYPE_MUL;
                            index.columns.push_back(getColumnName());
                        }
                        else if(ext == "unique")
                        {
                            hasIndex = true;
                            index.type = Index::INDEX_TYPE_UNI;
                            index.columns.push_back(getColumnName());
                        }
                        else if(ext == "not")
                            checkNotNull = true;
                        else if(ext == "default")
                            checkDefault = true;
                        else
                        {
                            if(extra.length() > 0)
                                extra.append(" ");
                            extra += ext;
                        }
                    }
                }
            }
            ignore = (isPk && autoInc) || ignore;
            notNull = isPk || notNull;
        }

        FieldMeta(FieldMeta&& meta) :
                offset(meta.offset), isPk(meta.isPk), ignore(meta.ignore),
                autoInc(meta.autoInc), notNull(meta.notNull), hasIndex(meta.hasIndex),
                fieldName(std::move(meta.fieldName)), cppType(std::move(meta.cppType)),
                _default(std::move(meta._default)), extra(std::move(meta.extra))
        {
            std::swap(columnName, meta.columnName);
            std::swap(columnType, meta.columnType);
            index = std::move(meta.index);
        }

        FieldMeta& operator=(FieldMeta&& meta) noexcept
        {
            offset = meta.offset; isPk = meta.isPk; ignore = meta.ignore;
            notNull = meta.notNull; hasIndex = meta.hasIndex; autoInc = meta.autoInc;
            fieldName = std::move(meta.fieldName);
            cppType = std::move(meta.cppType);
            _default = std::move(meta._default);
            extra = std::move(meta.extra);
            std::swap(columnName, meta.columnName);
            std::swap(columnType, meta.columnType);
            index = std::move(meta.index);
            return *this;
        }

        FieldMeta& operator=(const FieldMeta& meta) = default;

        bool operator<(const FieldMeta& meta) const
        {
            return offset < meta.offset;
        }

        bool operator==(const FieldMeta &meta) const
        {
            if(getColumnName() != meta.getColumnName() ||
               notNull != meta.notNull ||
               extra != meta.extra ||
               !isSame(_default, meta._default))
                return false;
            return isSameDbType(getColumnType(), meta.getColumnType());
        }

        //如果没有显式地定义列名，那么就根据变量名生成一个默认的列名
        [[nodiscard]] std::string getColumnName() const
        {
            return columnName.value_or(cppToDb(fieldName));
        }

        //如果没有显式定义列类型，使用rules.json中定义的默认类型
        [[nodiscard]] std::string getColumnType() const
        {
            return columnType.value_or(cppTypeToDbType(cppType));
        }
    };
}

#endif
