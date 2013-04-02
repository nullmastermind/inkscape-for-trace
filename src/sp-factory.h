#pragma once

#include <functional>
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

	bool registerObject(std::string id, std::function<SPObject*()> createFunction);

	SPObject* createObject(const Inkscape::XML::Node& id) const;

private:
	std::map<std::string, std::function<SPObject*()>> objectMap;
};
