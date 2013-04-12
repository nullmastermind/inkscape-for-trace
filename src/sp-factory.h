#pragma once

#include <exception>
#include <map>
#include <string>

class SPObject;

namespace Inkscape {
	namespace XML {
		class Node;
	}
}

/**
 * Factory for creating objects from the SPObject tree.
 */
class SPFactory {
public:
	class TypeNotRegistered : public std::exception {
	public:
		TypeNotRegistered(const std::string& type);
		const char* what() const throw();

	private:
		const std::string type;
	};

	static SPFactory& instance();

	typedef SPObject* CreateFunction();

	bool registerObject(const std::string& id, CreateFunction* createFunction);
	SPObject* createObject(const std::string& id) const;

private:
	std::map<const std::string, CreateFunction*> objectMap;
};


#include "xml/node.h"

struct NodeTraits {
	static std::string getTypeString(const Inkscape::XML::Node& node) {
		std::string name;

		switch (node.type()) {
			case Inkscape::XML::TEXT_NODE:
				name = "string";
				break;

			case Inkscape::XML::ELEMENT_NODE: {
				gchar const* const sptype = node.attribute("sodipodi:type");

				if (sptype) {
					name = sptype;
				} else {
					name = node.name();
				}
				break;
			}
			default:
				name = "";
				break;
		}

		return name;
	}
};
