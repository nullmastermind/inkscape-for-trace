// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Abstraction for different style widget operands.
 */
/*
 * Copyright (C) 2007 MenTaLguY <mental@rydia.net>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INKSCAPE_UI_WIDGET_STYLE_SUBJECT_H
#define SEEN_INKSCAPE_UI_WIDGET_STYLE_SUBJECT_H

#include <optional>
#include <2geom/rect.h>
#include <cstddef>
#include <sigc++/sigc++.h>

#include "object/sp-item.h"
#include "object/sp-tag.h"
#include "object/sp-tag-use.h"
#include "object/sp-tag-use-reference.h"

class SPDesktop;
class SPObject;
class SPCSSAttr;
class SPStyle;

namespace Inkscape {
class Selection;
}

namespace Inkscape {
namespace UI {
namespace Widget {

class StyleSubject {
public:
    class Selection;
    class CurrentLayer;


    StyleSubject();
    virtual ~StyleSubject();

    void setDesktop(SPDesktop *desktop);
    SPDesktop *getDesktop() const { return _desktop; }

    virtual Geom::OptRect getBounds(SPItem::BBoxType type) = 0;
    virtual int queryStyle(SPStyle *query, int property) = 0;
    virtual void setCSS(SPCSSAttr *css) = 0;
    virtual std::vector<SPObject*> list(){return std::vector<SPObject*>();};

    sigc::connection connectChanged(sigc::signal<void>::slot_type slot) {
        return _changed_signal.connect(slot);
    }

protected:
    virtual void _afterDesktopSwitch(SPDesktop */*desktop*/) {}
    void _emitChanged() { _changed_signal.emit(); }
    void _emitModified(Inkscape::Selection* selection, guint flags) {
        // Do not say this object has styles unless it's style has been modified
        if (flags & (SP_OBJECT_STYLE_MODIFIED_FLAG)) {
            _emitChanged();
        }
    }

private:
    sigc::signal<void> _changed_signal;
    SPDesktop *_desktop;
};

class StyleSubject::Selection : public StyleSubject {
public:
    Selection();
    ~Selection() override;

    Geom::OptRect getBounds(SPItem::BBoxType type) override;
    int queryStyle(SPStyle *query, int property) override;
    void setCSS(SPCSSAttr *css) override;
    std::vector<SPObject*> list() override;

protected:
    void _afterDesktopSwitch(SPDesktop *desktop) override;

private:
    Inkscape::Selection *_getSelection() const;

    sigc::connection _sel_changed;
    sigc::connection _subsel_changed;
    sigc::connection _sel_modified;
};

class StyleSubject::CurrentLayer : public StyleSubject {
public:
    CurrentLayer();
    ~CurrentLayer() override;

    Geom::OptRect getBounds(SPItem::BBoxType type) override;
    int queryStyle(SPStyle *query, int property) override;
    void setCSS(SPCSSAttr *css) override;
    std::vector<SPObject*> list() override;

protected:
    void _afterDesktopSwitch(SPDesktop *desktop) override;

private:
    SPObject *_getLayer() const;
    void _setLayer(SPObject *layer);
    SPObject *_getLayerSList() const;

    sigc::connection _layer_switched;
    sigc::connection _layer_release;
    sigc::connection _layer_modified;
    mutable SPObject* _element;
};

}
}
}

#endif // SEEN_INKSCAPE_UI_WIDGET_STYLE_SUBJECT_H

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
