#include "sp-factory.h"

#include <stdexcept>

#include "sp-object.h"
#include "xml/node.h"

SPFactory& SPFactory::instance() {
	static SPFactory factory;
	return factory;
}

bool SPFactory::registerObject(std::string id, CreateObjectMethod* createFunction) {
	return this->objectMap.insert(std::make_pair(id, createFunction)).second;
}

SPObject* SPFactory::createObject(const Inkscape::XML::Node& id) const {
	std::map<std::string, CreateObjectMethod*>::const_iterator entry;

	if (id.type() == Inkscape::XML::TEXT_NODE) {
		entry = this->objectMap.find("string");
	} else if (id.type() == Inkscape::XML::ELEMENT_NODE) {
		gchar const* const type_name = id.attribute("sodipodi:type");

		if (type_name) {
			entry = this->objectMap.find(type_name);
		} else {
			entry = this->objectMap.find(id.name());
		}
	} else {
		entry = this->objectMap.end();
	}

	if (entry == this->objectMap.end()) {
		g_warning("Factory: Type \"%s\" not registered!", id.name());

		return 0;
	}

	return (entry->second)();

//	std::map<std::string, CreateObjectMethod*>::const_iterator entry = this->objectMap.find(id);
//
//	if (entry == objectMap.end()) {
//		g_warning("Factory: Type \"%s\" not registered!", id.c_str());
//
//		SPObject* o = new SPObject();
//		return o;
//	}
//
//	return (entry->second)();
}

/*
 *     if ( repr->type() == Inkscape::XML::TEXT_NODE ) {
        return SP_TYPE_STRING;
    } else if ( repr->type() == Inkscape::XML::ELEMENT_NODE ) {
        gchar const * const type_name = repr->attribute("sodipodi:type");
        return ( type_name
                 ? name_to_gtype(SODIPODI_TYPE, type_name)
                 : name_to_gtype(REPR_NAME, repr->name()) );
    } else {
        return 0;
    }
 */
