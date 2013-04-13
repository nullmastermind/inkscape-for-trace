#include "sp-factory.h"

//#include <stdexcept>

//#include "sp-object.h"
//#include "xml/node.h"


//SPFactory::TypeNotRegistered::TypeNotRegistered(const std::string& type)
//	: std::exception(), type(type) {
//}
//
//const char* SPFactory::TypeNotRegistered::what() const throw() {
//	return type.c_str();
//}

//SPFactory& SPFactory::instance() {
//	static SPFactory factory;
//	return factory;
//}

//bool SPFactory::registerObject(const std::string& id, CreateFunction* createFunction) {
//	return this->objectMap.insert(std::make_pair(id, createFunction)).second;
//}

//SPObject* SPFactory::createObject(const std::string& id) const {
//	std::map<const std::string, CreateFunction*>::const_iterator it = this->objectMap.find(id);
//
//	if (it == this->objectMap.end()) {
//		throw TypeNotRegistered(id);
//	}
//
//	return (*it).second();
//}
