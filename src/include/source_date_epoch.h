// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Functions to parse the "SOURCE_DATE_EPOCH" environment variable for reproducible build hacks, see
 *     https://reproducible-builds.org/docs/source-date-epoch/
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_SOURCE_DATE_EPOCH
#define SEEN_SOURCE_DATE_EPOCH

#include <sstream>
#include <iostream>
#include <cstdlib>
#include <ctime>

namespace ReproducibleBuilds {

/** parse current time from SOURCE_DATE_EPOCH environment variable
 *
 *  \return current time (or zero if SOURCE_DATE_EPOCH unset)
 */
time_t now()
{
    time_t now = 0;

    char *source_date_epoch = std::getenv("SOURCE_DATE_EPOCH");
    if (source_date_epoch) {
        std::istringstream iss(source_date_epoch);
        iss >> now;
        if (iss.fail() || !iss.eof()) {
            std::cerr << "Error: Cannot parse SOURCE_DATE_EPOCH as integer\n";
            exit(27);
        }
    }

    return now;
}

/** like ReproducibleBuilds::now() but returns a ISO 8601 formatted string
 *
 *  \return current time as ISO 8601 formatted string (or empty string if SOURCE_DATE_EPOCH unset)
 */
Glib::ustring now_iso_8601()
{
    Glib::ustring now_formatted;

    time_t now = ReproducibleBuilds::now();
    if (now) {
        const tm *now_struct;
        char buffer[25];

        now_struct = gmtime(&now);
        if (strftime(buffer, 25, "%Y-%m-%dT%H:%M:%S", now_struct)) {
            now_formatted = buffer;
        }
    }

    return now_formatted;
}

} // namespace ReproducibleBuilds

#endif // SEEN_SOURCE_DATE_EPOCH
