#pragma once

#include <map>
#include <string>

class SPObject;

/**
 * Factory for creating objects from the SPObject tree.
 */
class SPFactory {
public:
	static SPFactory& instance();

	typedef SPObject* CreateObjectMethod();
	bool registerObject(std::string id, CreateObjectMethod* createFunction);

	SPObject* createObject(std::string id) const;

private:
	std::map<std::string, CreateObjectMethod*> objectMap;
};
