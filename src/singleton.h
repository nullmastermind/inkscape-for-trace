#pragma once

/**
 * A simple singleton implementation.
 */
template<class T>
struct Singleton {
	static T& instance() {
		static T inst;
		return inst;
	}
};
