#ifndef __ORM_REFLECTION_HPP__
#define __ORM_REFLECTION_HPP__
#include <map>
#include <string>
#include "datatype.hpp"

namespace orm::detail {
    //这些宏是用来获取宏可变参数个数的，最多支持100个参数(实际用不到这么多)
#define RSEQ_N() \
         99,98,97,96,95,94,93,92,91,90, \
         89,88,87,86,85,84,83,82,81,80, \
         79,78,77,76,75,74,73,72,71,70, \
         69,68,67,66,65,64,63,62,61,60, \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define ARG_N(\
         _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
         _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
         _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
         _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
         _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
         _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
         _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, \
         _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, \
         _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
         _91, _92, _93, _94, _95, _96, _97, _98, _99, N, ...) N
#define GET_ARG_COUNT_INNER(...)    ARG_N(__VA_ARGS__)
    //GET_ARG_COUNT获取可变参数个数
#define GET_ARG_COUNT(...)          GET_ARG_COUNT_INNER(__VA_ARGS__, RSEQ_N())

#define STR_LIST_1(arg)      #arg
#define STR_LIST_2(arg, ...) #arg, STR_LIST_1(__VA_ARGS__)
#define STR_LIST_3(arg, ...) #arg, STR_LIST_2(__VA_ARGS__)
#define STR_LIST_4(arg, ...) #arg, STR_LIST_3(__VA_ARGS__)
#define STR_LIST_5(arg, ...) #arg, STR_LIST_4(__VA_ARGS__)
#define STR_LIST_7(arg, ...) #arg, STR_LIST_5(__VA_ARGS__)
#define STR_LIST_8(arg, ...) #arg, STR_LIST_7(__VA_ARGS__)
#define STR_LIST_9(arg, ...) #arg, STR_LIST_8(__VA_ARGS__)
#define STR_LIST_10(arg, ...) #arg, STR_LIST_9(__VA_ARGS__)
#define STR_LIST_11(arg, ...) #arg, STR_LIST_10(__VA_ARGS__)
#define STR_LIST_12(arg, ...) #arg, STR_LIST_11(__VA_ARGS__)
#define STR_LIST_13(arg, ...) #arg, STR_LIST_12(__VA_ARGS__)
#define STR_LIST_14(arg, ...) #arg, STR_LIST_13(__VA_ARGS__)
#define STR_LIST_15(arg, ...) #arg, STR_LIST_14(__VA_ARGS__)
#define STR_LIST_16(arg, ...) #arg, STR_LIST_15(__VA_ARGS__)
#define STR_LIST_17(arg, ...) #arg, STR_LIST_16(__VA_ARGS__)
#define STR_LIST_18(arg, ...) #arg, STR_LIST_17(__VA_ARGS__)
#define STR_LIST_19(arg, ...) #arg, STR_LIST_18(__VA_ARGS__)
#define STR_LIST_20(arg, ...) #arg, STR_LIST_19(__VA_ARGS__)

#define MACRO_CONCAT(a, b) a##_##b
#define MAKE_STR_LIST(N, ...) MACRO_CONCAT(STR_LIST, N)(__VA_ARGS__)
//MAKE_INITIAL_LIST(a, b, c) -> {"a", "b", "c"}
//用来初始化vector
#define MAKE_INITIAL_LIST(...) { MAKE_STR_LIST(GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__) }

#define tags(...) ,#__VA_ARGS__

#define STR(s) #s

	template<typename T>
	class static_instance {
	public:
		static_instance() {
            //ins是模板成员，必须显式地"用"一下才会被初始化，否则仅仅被声明，不会被初始化
			ins;
		}
	private:
		static T ins;
	};
	template<typename T>
	//利用静态成员变量的dynamic initialize过程，使得collector_xxx的构造函数被调用
	T static_instance<T>::ins;
}

namespace orm {

	template<typename T>
	class Base {
	public:
		using CT = T;
		static std::map<std::string, orm::detail::Metadata> getMetadatas()
		{
			return metadatas;
		}
        static std::vector<Index> getIndices()
        {
            return indices;
        }
	protected:
        //metadata, indices 分别保存列和索引的元信息
		static std::map<std::string, orm::detail::Metadata> metadatas;
        static std::vector<Index> indices;
	};
	template<typename T>
	std::map<std::string, orm::detail::Metadata> Base<T>::metadatas;
    template<typename T>
    std::vector<orm::Index> Base<T>::indices;

//column宏定义一个内部类collector_xxx，并继承static_instance，利用static_instance的静态成员初始化的过程，使得
//collector_xxx的构造函数被调用，达到将元数据注册到Base中的目的
#define column(name, type, ...) \
type name; \
class collector_##name :public orm::detail::static_instance<collector_##name> { \
public: \
	collector_##name() { \
		orm::Base<CT>::metadatas[#name] = orm::detail::Metadata(#type, offsetof(CT, name) __VA_ARGS__); \
	} \
}

#define entity(name) \
struct name :public orm::Base<name> 

#define tableName(name) \
	class collector_table_##name :public orm::detail::static_instance<collector_table_##name>{ \
public: \
	collector_table_##name() { \
		orm::Base<CT>::metadatas[STR(tablename)] = orm::detail::Metadata("", -1, #name); \
	} \
}

//和column宏原理相同，只不过索引元数据是注册到Base的indices中
#define add_index(name, ...) \
    class collector_index_##name : public orm::detail::static_instance<collector_index_##name>{ \
    public: \
        collector_index_##name() { \
            orm::Index ind(#name, orm::Index::INDEX_TYPE_MUL); \
            ind.columns = {#__VA_ARGS__}; \
            orm::Base<CT>::indices.push_back(ind);\
        }\
    }

#define add_unique(name, ...) \
    class collector_unique_##name : public orm::detail::static_instance<collector_unique_##name>{ \
    public: \
        collector_unique_##name() { \
            orm::Index ind(#name, orm::Index::INDEX_TYPE_UNI); \
            ind.columns = MAKE_INITIAL_LIST(__VA_ARGS__); \
            orm::Base<CT>::indices.push_back(ind);\
        }\
    }

#define primary_key(col) \
    class collector_primary_key : public orm::detail::static_instance<collector_primary_key>{ \
    public: \
        collector_primary_key() { \
            orm::Index ind("", orm::Index::INDEX_TYPE_PRI); \
            ind.columns.push_back(#col); \
            orm::Base<CT>::indices.push_back(ind);\
        }\
    }

#define getfield(type, obj, fieldNum) *((type*)((char*)&obj + metadata[fieldNum].offset))
#define setfield(type, obj, fieldNum, val) *((type*)((char*)&obj + metadata[fieldNum].offset)) = val

#define ORM_TYPE_INT			"int"
#define ORM_TYPE_LONG_LONG		"long long"
#define ORM_TYPE_FLOAT			"float"
#define ORM_TYPE_DOUBLE			"double"
#define ORM_TYPE_STRING			"string"
#define ORM_TYPE_STD_STRING		"std::string"
#define ORM_TYPE_TIMESTAMP		"Timestamp"
#define ORM_TYPE_CONST_STR		"const char*"
}

#endif