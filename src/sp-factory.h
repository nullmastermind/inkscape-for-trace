#pragma once

#include <exception>
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
	class TypeNotRegistered : public std::exception {
	public:
		TypeNotRegistered(const std::string& type);
		const char* what() const noexcept;

	private:
		const std::string type;
	};

	static SPFactory& instance();

	bool registerObject(const std::string& id, std::function<SPObject* ()> createFunction);
	SPObject* createObject(const Inkscape::XML::Node& id) const;

private:
	std::map<const std::string, std::function<SPObject* ()>> objectMap;
};
