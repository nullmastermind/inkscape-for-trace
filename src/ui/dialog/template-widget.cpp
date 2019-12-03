// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief New From Template - templates widget - implementation
 */
/* Authors:
 *   Jan Darowski <jan.darowski@gmail.com>, supervised by Krzysztof Kosiński
 *
 * Copyright (C) 2013 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "template-widget.h"

#include <glibmm/miscutils.h>
#include <gtkmm/messagedialog.h>

#include "desktop.h"
#include "document.h"
#include "document-undo.h"
#include "file.h"
#include "inkscape.h"

#include "extension/implementation/implementation.h"

#include "object/sp-namedview.h"

namespace Inkscape {
namespace UI {


TemplateWidget::TemplateWidget()
    : _more_info_button(_("More info"))
    , _short_description_label(" ")
    , _template_name_label(_("no template selected"))
    , _effect_prefs(nullptr)
{
    pack_start(_template_name_label, Gtk::PACK_SHRINK, 10);
    pack_start(_preview_box, Gtk::PACK_SHRINK, 0);

    _preview_box.pack_start(_preview_image, Gtk::PACK_EXPAND_PADDING, 15);
    _preview_box.pack_start(_preview_render, Gtk::PACK_EXPAND_PADDING, 10);

    _short_description_label.set_line_wrap(true);

    _more_info_button.set_halign(Gtk::ALIGN_END);
    _more_info_button.set_valign(Gtk::ALIGN_CENTER);
    pack_end(_more_info_button, Gtk::PACK_SHRINK);

    pack_end(_short_description_label, Gtk::PACK_SHRINK, 5);

    _more_info_button.signal_clicked().connect(
    sigc::mem_fun(*this, &TemplateWidget::_displayTemplateDetails));
    _more_info_button.set_sensitive(false);
}


void TemplateWidget::create()
{
    if (_current_template.display_name == "")
        return;

    if (_current_template.is_procedural){
        SPDesktop *desktop = SP_ACTIVE_DESKTOP;
        SPDesktop *desc = sp_file_new_default();
        _current_template.tpl_effect->effect(desc);
        DocumentUndo::clearUndo(desc->getDocument());
        desc->getDocument()->setModifiedSinceSave(false);

	// Apply cx,cy etc. from document
	sp_namedview_window_from_document( desc );

        if (desktop)
            desktop->clearWaitingCursor();
    }
    else {
        sp_file_new(_current_template.path);
    }
}


void TemplateWidget::display(TemplateLoadTab::TemplateData data)
{
    clear();
    _current_template = data;

    _template_name_label.set_text(_current_template.display_name);
    _short_description_label.set_text(_current_template.short_description);

    if (data.preview_name != ""){
        std::string imagePath = Glib::build_filename(Glib::path_get_dirname(_current_template.path),  _current_template.preview_name);
        _preview_image.set(imagePath);
        _preview_image.show();
    }
    else if (!data.is_procedural){
        Glib::ustring gPath = data.path.c_str();
        _preview_render.showImage(gPath);
        _preview_render.show();
    }

    if (data.is_procedural){
        _effect_prefs = data.tpl_effect->get_imp()->prefs_effect(data.tpl_effect, SP_ACTIVE_DESKTOP, nullptr, nullptr);
        pack_start(*_effect_prefs);
    }
    _more_info_button.set_sensitive(true);
}

void TemplateWidget::clear()
{
    _template_name_label.set_text("");
    _short_description_label.set_text("");
    _preview_render.hide();
    _preview_image.hide();
    if (_effect_prefs != nullptr){
        remove (*_effect_prefs);
        _effect_prefs = nullptr;
    }
    _more_info_button.set_sensitive(false);
}

void TemplateWidget::_displayTemplateDetails()
{
    Glib::ustring message = _current_template.display_name + "\n\n";

    if (!_current_template.author.empty()) {
        message += _("Author");
        message += ": ";
        message += _current_template.author + " " + _current_template.creation_date + "\n\n";
    }

    if (!_current_template.keywords.empty()){
        message += _("Keywords");
        message += ":";
        for (const auto & keyword : _current_template.keywords) {
            message += " ";
            message += keyword;
        }
        message += "\n\n";
    }

    if (!_current_template.path.empty()) {
        message += _("Path");
        message += ": ";
        message += _current_template.path;
        message += "\n\n";
    }

    Gtk::MessageDialog dl(message, false, Gtk::MESSAGE_OTHER);
    dl.run();
}

}
}
