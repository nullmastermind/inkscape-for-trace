// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2005-2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "prefdialog.h"

#include <gtkmm/checkbutton.h>
#include <gtkmm/separator.h>
#include <glibmm/i18n.h>

#include "ui/dialog-events.h"
#include "xml/repr.h"

// Used to get SP_ACTIVE_DESKTOP
#include "inkscape.h"
#include "document.h"
#include "document-undo.h"

#include "extension/effect.h"
#include "extension/execution-env.h"
#include "extension/implementation/implementation.h"

#include "parameter.h"


namespace Inkscape {
namespace Extension {


/** \brief  Creates a new preference dialog for extension preferences
    \param  name      Name of the Extension whose dialog this is (should already be translated)
    \param  controls  The extension specific widgets in the dialog

    This function initializes the dialog with the name of the extension
    in the title.  It adds a few buttons and sets up handlers for
    them.  It also places the passed-in widgets into the dialog.
*/
PrefDialog::PrefDialog (Glib::ustring name, Gtk::Widget * controls, Effect * effect) :
    Gtk::Dialog(name, true),
    _name(name),
    _button_ok(nullptr),
    _button_cancel(nullptr),
    _button_preview(nullptr),
    _param_preview(nullptr),
    _effect(effect),
    _exEnv(nullptr)
{
    this->set_default_size(0,0);  // we want the window to be as small as possible instead of clobbering up space

    Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox());
    if (controls == nullptr) {
        if (_effect == nullptr) {
            std::cout << "AH!!!  No controls and no effect!!!" << std::endl;
            return;
        }
        controls = _effect->get_imp()->prefs_effect(_effect, SP_ACTIVE_DESKTOP, &_signal_param_change, nullptr);
        _signal_param_change.connect(sigc::mem_fun(this, &PrefDialog::param_change));
    }

    hbox->pack_start(*controls, true, true, 0);
    hbox->show();

    this->get_content_area()->pack_start(*hbox, true, true, 0);

    _button_cancel = add_button(_effect == nullptr ? _("_Cancel") : _("_Close"), Gtk::RESPONSE_CANCEL);
    _button_ok     = add_button(_effect == nullptr ? _("_OK")     : _("_Apply"), Gtk::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);
    _button_ok->grab_focus();

    if (_effect != nullptr && !_effect->no_live_preview) {
        if (_param_preview == nullptr) {
            XML::Document * doc = sp_repr_read_mem(live_param_xml, strlen(live_param_xml), nullptr);
            if (doc == nullptr) {
                std::cout << "Error encountered loading live parameter XML !!!" << std::endl;
                return;
            }
            _param_preview = InxParameter::make(doc->root(), _effect);
        }

        auto sep = Gtk::manage(new Gtk::Separator());
        sep->show();

        this->get_content_area()->pack_start(*sep, false, false, InxWidget::GUI_BOX_SPACING);

        hbox = Gtk::manage(new Gtk::HBox());
        hbox->set_border_width(InxWidget::GUI_BOX_MARGIN);
        _button_preview = _param_preview->get_widget(&_signal_preview);
        _button_preview->show();
        hbox->pack_start(*_button_preview, true, true, 0);
        hbox->show();

        this->get_content_area()->pack_start(*hbox, false, false, 0);

        Gtk::Box *hbox = dynamic_cast<Gtk::Box *>(_button_preview);
        if (hbox != nullptr) {
            _checkbox_preview = dynamic_cast<Gtk::CheckButton *>(hbox->get_children().front());
        }

        preview_toggle();
        _signal_preview.connect(sigc::mem_fun(this, &PrefDialog::preview_toggle));
    }

    // Set window modality for effects that don't use live preview
    if (_effect != nullptr && _effect->no_live_preview) {
        set_modal(false);
    }

    GtkWidget *dlg = GTK_WIDGET(gobj());
    sp_transientize(dlg);

    return;
}

PrefDialog::~PrefDialog ( )
{
    if (_param_preview != nullptr) {
        delete _param_preview;
        _param_preview = nullptr;
    }

    if (_exEnv != nullptr) {
        _exEnv->cancel();
        delete _exEnv;
        _exEnv = nullptr;
        _effect->set_execution_env(_exEnv);
    }

    if (_effect != nullptr) {
        _effect->set_pref_dialog(nullptr);
    }

    return;
}

void
PrefDialog::preview_toggle () {
    SPDocument *document = SP_ACTIVE_DOCUMENT;
    bool modified = document->isModifiedSinceSave();
    if(_param_preview->get_bool()) {
        if (_exEnv == nullptr) {
            set_modal(true);
            _exEnv = new ExecutionEnv(_effect, SP_ACTIVE_DESKTOP, nullptr, false, false);
            _effect->set_execution_env(_exEnv);
            _exEnv->run();
        }
    } else {
        set_modal(false);
        if (_exEnv != nullptr) {
            _exEnv->cancel();
            _exEnv->undo();
            _exEnv->reselect();
            delete _exEnv;
            _exEnv = nullptr;
            _effect->set_execution_env(_exEnv);
        }
    }
    document->setModifiedSinceSave(modified);
}

void
PrefDialog::param_change () {
    if (_exEnv != nullptr) {
        if (!_effect->loaded()) {
            _effect->set_state(Extension::STATE_LOADED);
        }
        _timersig.disconnect();
        _timersig = Glib::signal_timeout().connect(sigc::mem_fun(this, &PrefDialog::param_timer_expire),
                                                   250, /* ms */
                                                   Glib::PRIORITY_DEFAULT_IDLE);
    }

    return;
}

bool
PrefDialog::param_timer_expire () {
    if (_exEnv != nullptr) {
        _exEnv->cancel();
        _exEnv->undo();
        _exEnv->reselect();
        _exEnv->run();
    }

    return false;
}

void
PrefDialog::on_response (int signal) {
    if (signal == Gtk::RESPONSE_OK) {
        if (_exEnv == nullptr) {
			if (_effect != nullptr) {
				_effect->effect(SP_ACTIVE_DESKTOP);
			} else {
				// Shutdown run()
				return;
			}
        } else {
            if (_exEnv->wait()) {
                _exEnv->commit();
            } else {
                _exEnv->undo();
                _exEnv->reselect();
            }
            delete _exEnv;
            _exEnv = nullptr;
            _effect->set_execution_env(_exEnv);
        }
    }

    if (_param_preview != nullptr) {
        _checkbox_preview->set_active(false);
    }

    if ((signal == Gtk::RESPONSE_CANCEL || signal == Gtk::RESPONSE_DELETE_EVENT) && _effect != nullptr) {
        delete this;
    }
    return;
}

#include "extension/internal/clear-n_.h"

const char * PrefDialog::live_param_xml = "<param name=\"__live_effect__\" type=\"bool\" gui-text=\"" N_("Live preview") "\" gui-description=\"" N_("Is the effect previewed live on canvas?") "\">false</param>";

}; }; /* namespace Inkscape, Extension */

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
