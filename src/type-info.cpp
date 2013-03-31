#include "type-info.h"


TypeInfo::TypeInfo(const std::type_info& type_info) : type_info(&type_info) {
}

TypeInfo::TypeInfo(const TypeInfo& tinfo) : type_info(tinfo.type_info) {
}

TypeInfo& TypeInfo::operator=(const TypeInfo& rhs) {
	this->type_info = rhs.type_info;
	return *this;
}

bool TypeInfo::before(const TypeInfo& tinfo) const {
	return this->type_info->before(*tinfo.type_info);
}

const char* TypeInfo::name() const {
	return this->type_info->name();
}

const std::type_info& TypeInfo::get() const {
	return *this->type_info;
}

bool operator==(const TypeInfo& lhs, const TypeInfo& rhs) {
	return lhs.get() == rhs.get();
}

bool operator!=(const TypeInfo& lhs, const TypeInfo& rhs) {
	return !(lhs == rhs);
}

bool operator<(const TypeInfo& lhs, const TypeInfo& rhs) {
	return lhs.before(rhs);
}

bool operator<=(const TypeInfo& lhs, const TypeInfo& rhs) {
	return !(lhs > rhs);
}

bool operator>(const TypeInfo& lhs, const TypeInfo& rhs) {
	return rhs < lhs;
}

bool operator>=(const TypeInfo& lhs, const TypeInfo& rhs) {
	return !(lhs < rhs);
}
