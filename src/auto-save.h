// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Auto-save
 *
 * Copyright (C) 2020 Tavmjong Bah
 *
 * Re-write of code formerly in inkscape.cpp and originally written by Jon Cruz and others.
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 *
 */

#ifndef INKSCAPE_AUTOSAVE_H
#define INKSCAPE_AUTOSAVE_H

class InkscapeApplication;

namespace Inkscape {

class AutoSave {
private:
    AutoSave() = default;

public:
    AutoSave(const AutoSave &) = delete;
    AutoSave &operator=(const AutoSave &) = delete;
    AutoSave(AutoSave &&) = delete;
    AutoSave &operator=(AutoSave &&) = delete;

    static AutoSave &getInstance()
    {
        static AutoSave theInstance;
        return theInstance;
    }

    static void restart();
    void init(InkscapeApplication *app);
    void start(); // Includes restarting.
    bool save();

private:
    InkscapeApplication* _app = nullptr;
};

} // namespace Inkscape

#endif // INKSCAPE_AUTOSAVE_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
