#ifndef __ORM_REFLECTION_HPP__
#define __ORM_REFLECTION_HPP__
#include <map>
#include <string>

namespace orm::detail {
#define tags(...) ,#__VA_ARGS__

#define STR(s) #s

	template<class T>
	class static_instance {
	public:
		static_instance() {
			ins;
		}
	private:
		static T ins;
	};
	template<class T>
	T static_instance<T>::ins;

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

	template<class T>
	class Base {
	public:
		using CT = T;
		static std::map<std::string, orm::detail::Metadata> getMetadatas()
		{
			return metadatas;
		}
	protected:
		static std::map<std::string, orm::detail::Metadata> metadatas;
	};
	template<class T>
	std::map<std::string, orm::detail::Metadata> Base<T>::metadatas;

	template<typename T, typename V = void>
	struct is_entity: std::false_type{};

	template<typename T>
	struct is_entity<T, std::void_t<typename orm::Base<T>::CT>>: std::true_type{};

#define column(name, type, ...) \
type name; \
class collector_##name :public orm::detail::static_instance<collector_##name> { \
public: \
	collector_##name() { \
		orm::Base<CT>::metadatas[#name] = orm::detail::Metadata(#type, offsetof(CT, name) __VA_ARGS__); \
	} \
};

#define entity(name) \
struct name :public orm::Base<name> 

#define tableName(name) \
	class collector_table_##name :public orm::detail::static_instance<collector_table_##name>{ \
public: \
	collector_table_##name() { \
		orm::Base<CT>::metadatas[STR(tablename)] = orm::detail::Metadata("", -1, #name); \
	} \
};

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