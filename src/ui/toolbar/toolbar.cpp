#include "toolbar.h"

#include <gtkmm/separatortoolitem.h>

#include "desktop.h"

#include "helper/action.h"

namespace Inkscape {
namespace UI {
namespace Toolbar {

/**
 * \brief Add a toolbutton that performs a given verb
 *
 * \param[in] verb_code The code for the verb (e.g., SP_VERB_EDIT_SELECT_ALL)
 */
void
Toolbar::add_toolbutton_for_verb(unsigned int verb_code)
{
    auto context = Inkscape::ActionContext(_desktop);
    auto button  = SPAction::create_toolbutton_for_verb(verb_code, context);
    add(*button);
}

/**
 * \brief Add a separator line to the toolbar
 *
 * \details This is just a convenience wrapper for the
 *          standard GtkMM functionality
 */
void
Toolbar::add_separator()
{
    add(* Gtk::manage(new Gtk::SeparatorToolItem()));
}

GtkWidget *
Toolbar::create(SPDesktop *desktop)
{
    auto toolbar = Gtk::manage(new Toolbar(desktop));
    return GTK_WIDGET(toolbar->gobj());
}
}
}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
