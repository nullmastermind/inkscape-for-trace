// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape - an ambitious vector drawing program
 *
 * Authors:
 * Tavmjong Bah
 *
 * (C) 2018 Tavmjong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "inkscape-application.h"


int main(int argc, char *argv[])
{

    if (gtk_init_check(NULL, NULL))
        return (ConcreteInkscapeApplication<Gtk::Application>::get_instance()).run(argc, argv);
    else
        return (ConcreteInkscapeApplication<Gio::Application>::get_instance()).run(argc, argv);
}

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
