#include "sp-factory.h"

#include <stdexcept>

#include "sp-object.h"

SPFactory& SPFactory::instance() {
	static SPFactory factory;
	return factory;
}

bool SPFactory::registerObject(std::string id, CreateObjectMethod* createFunction) {
	return this->objectMap.insert(std::make_pair(id, createFunction)).second;
}

SPObject* SPFactory::createObject(std::string id) const {
	std::map<std::string, CreateObjectMethod*>::const_iterator entry = this->objectMap.find(id);

	if (entry == objectMap.end()) {
		//g_warning("Factory: Type \"%s\" not registered!", id.c_str());
//		SPObject* o = new SPObject();
//
//		return o;
		return 0;
	}

	return (entry->second)();
}
