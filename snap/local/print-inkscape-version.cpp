// SPDX-License-Identifier: GPL-2.0-or-later
// Use the Inkscape compiled version identifier and print it out with
// the Snap restrictions on versions. Only a few characters allowed
// and a limit of 32 characters.
#include "inkscape-version.h"

#include <iostream>
#include <cctype>

int main (int argc, char ** argv) {
	std::string outstr;

	for (const auto& c : std::string{Inkscape::version_string}) {
		if (outstr.length() == 32) {
			break;
		}

		if (std::isalpha(c) || std::isdigit(c)) {
			outstr.append(1, c);
			continue;
		}

		if (c == '(' || c == ')' || c == ',') {
			continue;
		}

		if (c == '.' || c == '-') {
			outstr.append(1, c);
			continue;
		}

		outstr.append(1, '-');
	}

	while (outstr.length() > 0 && !std::isalpha(outstr.back()) && !std::isdigit(outstr.back())) {
		outstr.pop_back();
	}

	std::cout << outstr << std::endl;

	return 0;
}
