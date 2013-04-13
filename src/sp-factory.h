#pragma once

#include <exception>
#include <map>
#include <string>

/**
 * A simple singleton implementation.
 */
template<class T>
struct Singleton {
	static T& instance() {
		static T i;
		return i;
	}
};


namespace FactoryExceptions {
	class TypeNotRegistered : public std::exception {
	public:
		TypeNotRegistered(const std::string& typeString) : std::exception(), typeString(typeString) {
		}

		virtual ~TypeNotRegistered() throw() {
		}

		const char* what() const throw() {
			return typeString.c_str();
		}

	private:
		const std::string typeString;
	};
}

/**
 * A Factory for creating objects which can be identified by strings.
 */
template<class BaseObject>
class Factory {
public:
	typedef BaseObject* CreateFunction();

	bool registerObject(const std::string& id, CreateFunction* createFunction) {
		return this->objectMap.insert(std::make_pair(id, createFunction)).second;
	}

	BaseObject* createObject(const std::string& id) const throw(FactoryExceptions::TypeNotRegistered) {
		typename std::map<const std::string, CreateFunction*>::const_iterator it = this->objectMap.find(id);

		if (it == this->objectMap.end()) {
			throw FactoryExceptions::TypeNotRegistered(id);
		}

		return it->second();
	}

private:
	std::map<const std::string, CreateFunction*> objectMap;
};


class SPObject;
typedef Singleton< Factory<SPObject> > SPFactory;

class SPEventContext;
typedef Singleton< Factory<SPEventContext> > ToolFactory;



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
