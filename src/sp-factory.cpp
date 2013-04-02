#include "sp-factory.h"

#include <stdexcept>

#include "sp-object.h"
#include "xml/node.h"

SPFactory& SPFactory::instance() {
	static SPFactory factory;
	return factory;
}

bool SPFactory::registerObject(std::string id, std::function<SPObject*()> createFunction) {
	return this->objectMap.insert(std::make_pair(id, createFunction)).second;
}

SPObject* SPFactory::createObject(const Inkscape::XML::Node& id) const {
	std::map<std::string, std::function<SPObject*()>>::const_iterator entry;

	switch (id.type()) {
		case Inkscape::XML::TEXT_NODE:
			entry = this->objectMap.find("string");
			break;

		case Inkscape::XML::ELEMENT_NODE: {
			gchar const* const type_name = id.attribute("sodipodi:type");

			if (type_name) {
				entry = this->objectMap.find(type_name);
			} else {
				entry = this->objectMap.find(id.name());
			}

			break;
		}
		default:
			entry = this->objectMap.end();
	}

	if (entry == this->objectMap.end()) {
		g_warning("Factory: Type \"%s\" not registered!", id.name());

		return 0;
	}

	return (entry->second)();
}
