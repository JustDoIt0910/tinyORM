//
// Created by zr on 22-11-9.
//

#ifndef __ORM_NAMING_HPP__
#define __ORM_NAMING_HPP__
#include <string>
#include <ctype.h>
#include "mysql4cpp/common.h"
#include "conf.h"
#include <vector>
#include <exception>

namespace orm {
    //变量名称转数据库中的名称，驼峰变下划线
    inline std::string cppToDb(const std::string& name)
    {
        std::string res;
        if(name.length() == 0)
            return res;
        bool flag = true;
        int l = 0;
        for(int i = 0; i < name.length(); i++)
        {
            if(!isupper(name.at(i)))
                flag = false;
            if((!flag && isupper(name.at(i))))
            {
                flag = true;
                std::string sub = name.substr(l, i - l);
                Util::toLowerCase(sub);
                if(res.length() > 0)
                    res.append("_");
                res.append(sub);
                l = i;
            }
        }
        std::string sub = name.substr(l);
        Util::toLowerCase(sub);
        if(res.length() > 0)
            res.append("_");
        res.append(sub);
        return res;
    }

    inline std::string capitalize(const std::string& str)
    {
        std::string res;
        if(str.length() == 0)
            return res;
        res = str;
        if(islower(res.at(0)))
            res.replace(0, 1, std::string(1, res.at(0) - 32));
        return res;
    }

    //数据库中名称转变量名/类名, 下划线转驼峰，cap_ini指明首字母是否大写
    inline std::string dbToCpp(const std::string& name, bool cap_ini = false)
    {
        std::string res;
        if(name.length() == 0)
            return res;
        std::vector<std::string> v = Util::split(name, "_");
        for(int i = 0; i < v.size(); i++)
        {
            if(cap_ini || i > 0)
                res.append(capitalize(v[i]));
            else
                res.append(v[i]);
        }
        return res;
    }

    //判断两个字符串是否代表相同的数据库数据类型(比如integer, int, int(11))，规则定义在rules.json中
    inline bool isSameDbType(const std::string& type1, const std::string& type2)
    {
        Config* conf = Config::getInstance();
        std::vector<std::string> types;
        try {
            types = (*conf)["mysql"]["same_types"][type1.substr(0, 3)].
                    get<std::vector<std::string>>();
        }
        catch(std::exception& e)
        {
            return type1 == type2;
        }
        if(std::find(types.begin(), types.end(), type1) != types.end())
            return std::find(types.begin(), types.end(), type2) != types.end();
        return type1 == type2;
    }

    inline bool isSame(const std::string& s1, const std::string& s2)
    {
        std::string _s1 = s1; std::string _s2 = s2;
        Util::toLowerCase(_s1);
        Util::toLowerCase(_s2);
        if(s1 == s2)
            return true;
        if(_s1 == "now()")
            return _s2 == "current_timestamp";
        if(_s2 == "now()")
            return _s1 == "current_timestamp";
    }

    //c++中的变量类型转数据库中类型，规则定义在rules.json中
    inline std::string cppTypeToDbType(const std::string& cppType)
    {
        Config* conf = Config::getInstance();
        try {
            return (*conf)["mysql"]["default_types"][cppType].get<std::string>();
        }
        catch(std::exception& e){
            return cppType;
        }
    }

    //数据库中类型转c++中的变量类型，规则定义在rules.json中
    inline std::string dbTypeToCppType(const std::string& dbType)
    {
        std::string type = dbType;
        int i = dbType.find_first_of("(");
        if(i != dbType.npos)
            type = dbType.substr(0, i);
        Config* conf = Config::getInstance();
        try {
            return (*conf)["mysql"]["db_cpp_types"][type].get<std::string>();
        }
        catch(std::exception& e){
            return "";
        }
    }
}

#endif
