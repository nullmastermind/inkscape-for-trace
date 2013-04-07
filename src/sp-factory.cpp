#include "sp-factory.h"

#include <stdexcept>

#include "sp-object.h"
#include "xml/node.h"


SPFactory::TypeNotRegistered::TypeNotRegistered(const std::string& type)
	: std::exception(), type(type) {
}

const char* SPFactory::TypeNotRegistered::what() const noexcept {
	return type.c_str();
}

SPFactory& SPFactory::instance() {
	static SPFactory factory;
	return factory;
}

bool SPFactory::registerObject(const std::string& id, std::function<SPObject* ()> createFunction) {
	return this->objectMap.insert(std::make_pair(id, createFunction)).second;

	// replace when gcc supports this
	//return this->objectMap.emplace(id, createFunction).second;
}

SPObject* SPFactory::createObject(const Inkscape::XML::Node& id) const {
	std::string name;

	switch (id.type()) {
		case Inkscape::XML::TEXT_NODE:
			name = "string";
			break;

		case Inkscape::XML::ELEMENT_NODE: {
			gchar const* const sptype = id.attribute("sodipodi:type");

			if (sptype) {
				name = sptype;
			} else {
				name = id.name();
			}
			break;
		}
		default:
			break;
	}

	try {
		std::function<SPObject* ()> createFunction = this->objectMap.at(name);
		return createFunction();
	} catch (const std::out_of_range& ex) {
		std::throw_with_nested(TypeNotRegistered(name));
	}
}
