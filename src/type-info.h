#pragma once

#include <typeinfo>

/**
 * A wrapper around typeinfo. Inspired by Andrei Alexandrescu's "Modern C++ Design".
 * Used as a temporary replacement for glib's type-checking system as long as SPObject
 * must not be polymorphic / new objects are instantiated by g_object_new.
 */
class TypeInfo {
public:
	TypeInfo(const std::type_info& type_info);
	TypeInfo(const TypeInfo& tinfo);

	TypeInfo& operator=(const TypeInfo& tinfo);

	bool before(const TypeInfo& tinfo) const;
	const char* name() const;

	const std::type_info& get() const;

private:
	const std::type_info* type_info;
};

bool operator==(const TypeInfo& lhs, const TypeInfo& rhs);
bool operator!=(const TypeInfo& lhs, const TypeInfo& rhs);
bool operator<(const TypeInfo& lhs, const TypeInfo& rhs);
bool operator<=(const TypeInfo& lhs, const TypeInfo& rhs);
bool operator>(const TypeInfo& lhs, const TypeInfo& rhs);
bool operator>=(const TypeInfo& lhs, const TypeInfo& rhs);
