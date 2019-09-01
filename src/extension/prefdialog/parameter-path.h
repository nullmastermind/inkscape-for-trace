// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Path parameter for extensions
 *//*
 * Authors:
 *   Patrick Storz <eduard.braun2@gmx.de>
 *
 * Copyright (C) 2019 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INK_EXTENSION_PARAM_PATH_H_SEEN
#define INK_EXTENSION_PARAM_PATH_H_SEEN

#include "parameter.h"


namespace Inkscape {
namespace Extension {

class ParamPathEntry;

class ParamPath : public InxParameter {
public:
    ParamPath(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);

    /** \brief  Returns \c _value, with a \i const to protect it. */
    const std::string& get() const { return _value; }
    const std::string& set(const std::string in);

    Gtk::Widget *get_widget(sigc::signal<void> *changeSignal) override;

    std::string value_to_string() const override;

private:
    enum Mode {
        FILE, FOLDER, FILE_NEW, FOLDER_NEW
    };

    /** \brief  Internal value. */
    std::string _value;

    /** selection mode for the file chooser: files or folders? */
    Mode _mode = FILE;

    /** selection mode for the file chooser: multiple items? */
    bool _select_multiple = false;

    /** filetypes that should be selectable in file chooser */
    std::vector<std::string> _filetypes;

    /** pointer to the parameters text entry
      * keep this around, so we can update the value accordingly in \a on_button_clicked() */
    ParamPathEntry *_entry;

    void on_button_clicked();
};


}  // namespace Extension
}  // namespace Inkscape

#endif /* INK_EXTENSION_PARAM_PATH_H_SEEN */

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
