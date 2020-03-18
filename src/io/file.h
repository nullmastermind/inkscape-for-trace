// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * File operations (independent of GUI)
 *
 * Copyright (C) 2018 Tavmjong Bah
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INK_FILE_IO_H
#define INK_FILE_IO_H

#include <string>

namespace Gio {
class File;
}

namespace Glib {
class ustring;

template <class T>
class RefPtr;
}

class SPDocument;

SPDocument* ink_file_new(const std::string &Template = nullptr);
SPDocument* ink_file_open(const Glib::ustring &data);
SPDocument* ink_file_open(const Glib::RefPtr<Gio::File>& file, bool *cancelled = nullptr);

// To do:
// ink_file_save()
// ink_file_export()
// ink_file_import()



#endif // INK_FILE_IO_H
