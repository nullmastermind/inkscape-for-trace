// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * A widget for controlling object compositing (filter, opacity, etc.)
 *
 * Authors:
 *   Bryce W. Harrington <bryce@bryceharrington.org>
 *   Gustav Broberg <broberg@kth.se>
 *   Niko Kiirala <niko@kiirala.com>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004--2008 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "ui/widget/object-composite-settings.h"

#include "desktop.h"

#include "desktop-style.h"
#include "document.h"
#include "document-undo.h"
#include "filter-chemistry.h"
#include "inkscape.h"
#include "style.h"
#include "svg/css-ostringstream.h"
#include "verbs.h"
#include "display/sp-canvas.h"
#include "ui/widget/style-subject.h"

namespace Inkscape {
namespace UI {
namespace Widget {

ObjectCompositeSettings::ObjectCompositeSettings(unsigned int verb_code, char const *history_prefix, int flags)
: _verb_code(verb_code),
  _blend_tag(Glib::ustring(history_prefix) + ":blend"),
  _blur_tag(Glib::ustring(history_prefix) + ":blur"),
  _opacity_tag(Glib::ustring(history_prefix) + ":opacity"),
  _filter_modifier(flags),
  _blocked(false)
{
    set_name( "ObjectCompositeSettings");

    // Filter Effects
    pack_start(_filter_modifier, false, false, 2);

    _filter_modifier.signal_blend_changed().connect(sigc::mem_fun(*this, &ObjectCompositeSettings::_blendBlurValueChanged));
    _filter_modifier.signal_blur_changed().connect(sigc::mem_fun(*this, &ObjectCompositeSettings::_blendBlurValueChanged));
    _filter_modifier.signal_opacity_changed().connect(sigc::mem_fun(*this, &ObjectCompositeSettings::_opacityValueChanged));

    show_all_children();
}

ObjectCompositeSettings::~ObjectCompositeSettings() {
    setSubject(nullptr);
}

void ObjectCompositeSettings::setSubject(StyleSubject *subject) {
    _subject_changed.disconnect();
    if (subject) {
        _subject = subject;
        _subject_changed = _subject->connectChanged(sigc::mem_fun(*this, &ObjectCompositeSettings::_subjectChanged));
        _subject->setDesktop(SP_ACTIVE_DESKTOP);
    }
}

// We get away with sharing one callback for blend and blur as this is used by
//  * the Layers dialog where only one layer can be selected at a time,
//  * the Fill and Stroke dialog where only blur is used.
// If both blend and blur are used in a dialog where more than one object can
// be selected then this should be split into separate functions for blend and
// blur (like in the Objects dialog).
void
ObjectCompositeSettings::_blendBlurValueChanged()
{
    if (!_subject) {
        return;
    }

    SPDesktop *desktop = _subject->getDesktop();
    if (!desktop) {
        return;
    }
    SPDocument *document = desktop->getDocument();

    if (_blocked)
        return;
    _blocked = true;

    // FIXME: fix for GTK breakage, see comment in SelectedStyle::on_opacity_changed; here it results in crash 1580903
    //sp_canvas_force_full_redraw_after_interruptions(desktop->getCanvas(), 0);

    Geom::OptRect bbox = _subject->getBounds(SPItem::GEOMETRIC_BBOX);
    double radius;
    if (bbox) {
        double perimeter = bbox->dimensions()[Geom::X] + bbox->dimensions()[Geom::Y];   // fixme: this is only half the perimeter, is that correct?
        radius = _filter_modifier.get_blur_value() * perimeter / 400;
    } else {
        radius = 0;
    }

    const Glib::ustring blendmode = _filter_modifier.get_blend_mode();

    //apply created filter to every selected item
    std::vector<SPObject*> sel = _subject->list();
    for (std::vector<SPObject*>::const_iterator i = sel.begin() ; i != sel.end() ; ++i ) {
        if (!SP_IS_ITEM(*i)) {
            continue;
        }

        SPItem * item = SP_ITEM(*i);
        SPStyle *style = item->style;
        g_assert(style != nullptr);

        SPCSSAttr *css = sp_repr_css_attr_new();

        if (blendmode == "normal") {
            sp_repr_css_unset_property(css, "mix-blend-mode");
        } else {
            sp_repr_css_set_property(css, "mix-blend-mode", blendmode.c_str());
        }

        _subject->setCSS(css);

        sp_repr_css_attr_unref(css);

        if (radius == 0 && item->style->filter.set
            && filter_is_single_gaussian_blur(SP_FILTER(item->style->getFilter()))) {
            remove_filter(item, false);
        }
        else if (radius != 0) {
            SPFilter *filter = modify_filter_gaussian_blur_from_item(document, item, radius);
            sp_style_set_property_url(item, "filter", filter, false);
        }

        //request update
        item->requestDisplayUpdate(( SP_OBJECT_MODIFIED_FLAG |
                                     SP_OBJECT_STYLE_MODIFIED_FLAG ));
    }

    DocumentUndo::maybeDone(document, _blur_tag.c_str(), _verb_code,
                            _("Change blur/blend filter"));

    // resume interruptibility
    //sp_canvas_end_forced_full_redraws(desktop->getCanvas());

    _blocked = false;
}

void
ObjectCompositeSettings::_opacityValueChanged()
{
    if (!_subject) {
        return;
    }

    SPDesktop *desktop = _subject->getDesktop();
    if (!desktop) {
        return;
    }

    if (_blocked)
        return;
    _blocked = true;

    SPCSSAttr *css = sp_repr_css_attr_new ();

    Inkscape::CSSOStringStream os;
    os << CLAMP (_filter_modifier.get_opacity_value() / 100, 0.0, 1.0);
    sp_repr_css_set_property (css, "opacity", os.str().c_str());

    _subject->setCSS(css);

    sp_repr_css_attr_unref (css);

    DocumentUndo::maybeDone(desktop->getDocument(), _opacity_tag.c_str(), _verb_code,
                            _("Change opacity"));

    // resume interruptibility
    //sp_canvas_end_forced_full_redraws(desktop->getCanvas());

    _blocked = false;
}

void
ObjectCompositeSettings::_subjectChanged() {
    if (!_subject) {
        return;
    }

    SPDesktop *desktop = _subject->getDesktop();
    if (!desktop) {
        return;
    }

    if (_blocked)
        return;
    _blocked = true;
    SPStyle query(desktop->getDocument());
    int result = _subject->queryStyle(&query, QUERY_STYLE_PROPERTY_MASTEROPACITY);

    switch (result) {
        case QUERY_STYLE_NOTHING:
            break;
        case QUERY_STYLE_SINGLE:
        case QUERY_STYLE_MULTIPLE_AVERAGED: // TODO: treat this slightly differently
        case QUERY_STYLE_MULTIPLE_SAME:
            _filter_modifier.set_opacity_value(100 * SP_SCALE24_TO_FLOAT(query.opacity.value));
            break;
    }

    //query now for current filter mode and average blurring of selection
    const int blend_result = _subject->queryStyle(&query, QUERY_STYLE_PROPERTY_BLEND);
    switch(blend_result) {
        case QUERY_STYLE_NOTHING:
            break;
        case QUERY_STYLE_SINGLE:
        case QUERY_STYLE_MULTIPLE_SAME:
            _filter_modifier.set_blend_mode(query.mix_blend_mode.value); // here dont work mix_blend_mode.set
            break;
        case QUERY_STYLE_MULTIPLE_DIFFERENT:
            // TODO: set text
            break;
    }

    int blur_result = _subject->queryStyle(&query, QUERY_STYLE_PROPERTY_BLUR);
    switch (blur_result) {
        case QUERY_STYLE_NOTHING: // no blurring
            _filter_modifier.set_blur_value(0);
            break;
        case QUERY_STYLE_SINGLE:
        case QUERY_STYLE_MULTIPLE_AVERAGED:
        case QUERY_STYLE_MULTIPLE_SAME:
            Geom::OptRect bbox = _subject->getBounds(SPItem::GEOMETRIC_BBOX);
            if (bbox) {
                double perimeter =
                    bbox->dimensions()[Geom::X] +
                    bbox->dimensions()[Geom::Y]; // fixme: this is only half the perimeter, is that correct?
                // update blur widget value
                float radius = query.filter_gaussianBlur_deviation.value;
                float percent = radius * 400 / perimeter; // so that for a square, 100% == half side
                _filter_modifier.set_blur_value(percent);
            }
            break;
    }

    // If we have nothing selected, disable dialog.
    if (result       == QUERY_STYLE_NOTHING &&
        blend_result == QUERY_STYLE_NOTHING ) {
        _filter_modifier.set_sensitive( false );
    } else {
        _filter_modifier.set_sensitive( true );
    }

    _blocked = false;
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
