#pragma once

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
	static SPFactory& instance();

	typedef SPObject* CreateObjectMethod();
	bool registerObject(std::string id, CreateObjectMethod* createFunction);

	SPObject* createObject(const Inkscape::XML::Node& id) const;

private:
	std::map<std::string, CreateObjectMethod*> objectMap;
};
