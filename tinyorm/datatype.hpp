//
// Created by zr on 22-10-29.
//

#ifndef TINYORM_EXAMPLE_DATATYPE_HPP
#define TINYORM_EXAMPLE_DATATYPE_HPP

#include <string>
#include <vector>
#include <utility>
#include <optional>
#include "mysql4cpp/common.h"

namespace orm::detail {
    struct Metadata {
        std::string type;
        int offset;
        std::string tag;

        Metadata(const std::string& _type, int _offset, const std::string& _tag) :
                type(_type), offset(_offset), tag(_tag){}

        Metadata(const std::string& _type, int _offset): Metadata(_type, _offset, ""){}
        Metadata(){}
    };
}

namespace orm {

    struct Index
    {
        enum IndexType
        {
            INDEX_TYPE_UNI,
            INDEX_TYPE_MUL,
            INDEX_TYPE_PRI
        } type;

        std::vector<std::string> columns;

        Index(IndexType _type): type(_type) {}
        Index() {}
    };

    class FieldMeta
    {
    public:
        //变量名
        std::string fieldName;
        std::string cppType;

        //数据库表中的列类型
        std::optional<std::string> columnType;

        //数据库表中的列名
        std::optional<std::string> columnName;

        //附加元信息
        std::vector<std::string> extra;

        bool isPk;
        bool autoInc;
        bool hasIndex;
        bool notNull;
        std::string _default;
        Index index;

        int offset;
        bool ignore;

        FieldMeta() {};
        FieldMeta(const std::string&, const orm::detail::Metadata&);
        FieldMeta(FieldMeta&& meta);

        FieldMeta& operator=(FieldMeta&& meta);
        FieldMeta& operator=(const FieldMeta& meta);
        bool operator<(const FieldMeta& meta) const;
    };

    FieldMeta::FieldMeta(const std::string& _fieldName, const orm::detail::Metadata& meta):
            fieldName(_fieldName), cppType(meta.type), offset(meta.offset),
            isPk(false), ignore(false), autoInc(false)
    {
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
                        isPk = true;
                    else if (ext == "auto_increment" || ext == "auto increment")
                        autoInc = true;
                    else if (ext == "ignore")
                        ignore = true;
                    else if(ext == "index")
                    {
                        hasIndex = true;
                        index.type = Index::INDEX_TYPE_MUL;
                        index.columns.push_back(columnName.value_or(fieldName));
                    }
                    else if(ext == "unique")
                    {
                        hasIndex = true;
                        index.type = Index::INDEX_TYPE_UNI;
                        index.columns.push_back(columnName.value_or(fieldName));
                    }
                    else if(ext == "not")
                        checkNotNull = true;
                    else if(ext == "default")
                        checkDefault = true;
                    else
                        extra.push_back(ext);
                }
            }
        }
        ignore = (isPk && autoInc) || ignore;
    }

    FieldMeta::FieldMeta(FieldMeta&& meta) :
            offset(meta.offset), isPk(meta.isPk), ignore(meta.ignore),
            autoInc(meta.autoInc), notNull(meta.notNull), hasIndex(meta.hasIndex),
            fieldName(std::move(meta.fieldName)), cppType(std::move(meta.cppType)), _default(std::move(meta._default))
    {
        std::swap(columnName, meta.columnName);
        std::swap(columnType, meta.columnType);
        extra.swap(meta.extra);
    }

    FieldMeta& FieldMeta::operator=(FieldMeta&& meta)
    {
        offset = meta.offset; isPk = meta.isPk; ignore = meta.ignore;
        notNull = meta.notNull; hasIndex = meta.hasIndex; autoInc = meta.autoInc;
        fieldName = std::move(meta.fieldName);
        cppType = std::move(meta.cppType);
        _default = std::move(meta._default);
        std::swap(columnName, meta.columnName);
        std::swap(columnType, meta.columnType);
        extra.swap(meta.extra);
        return *this;
    }

    FieldMeta& FieldMeta::operator=(const FieldMeta& meta)
    {
        offset = meta.offset; isPk = meta.isPk; ignore = meta.ignore;
        notNull = meta.notNull; hasIndex = meta.hasIndex; autoInc = meta.autoInc;
        fieldName = meta.fieldName; cppType = meta.cppType; _default = meta._default;
        columnName = meta.columnName;
        columnType = meta.columnType;
        extra = meta.extra;
        return *this;
    }

    bool FieldMeta::operator<(const FieldMeta& meta) const
    {
        return offset < meta.offset;
    }
}

#endif //TINYORM_EXAMPLE_DATATYPE_HPP
