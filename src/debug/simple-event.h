// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Inkscape::Debug::SimpleEvent - trivial implementation of Debug::Event
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2005 MenTaLguY
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_DEBUG_SIMPLE_EVENT_H
#define SEEN_INKSCAPE_DEBUG_SIMPLE_EVENT_H

#include <cstdarg>
#include <memory>
#include <string>
#include <vector>
#include <glib.h> // g_assert()

#include "debug/event.h"

namespace Inkscape {

namespace Debug {

template <Event::Category C=Event::OTHER>
class SimpleEvent : public Event {
public:
    explicit SimpleEvent(char const *name) : _name(name) {}

    // default copy
    // default assign

    static Category category() { return C; }

    char const *name() const override { return _name; }
    unsigned propertyCount() const override { return _properties.size(); }
    PropertyPair property(unsigned property) const override {
        return _properties[property];
    }

    void generateChildEvents() const override {}

protected:
    void _addProperty(char const *name, std::shared_ptr<std::string>&& value) {
        _properties.emplace_back(name, std::move(value));
    }
    void _addProperty(char const *name, char const *value) {
        _addProperty(name, std::make_shared<std::string>(value));
    }
    void _addProperty(char const *name, long value) {
        _addFormattedProperty(name, "%ld", value);
    }

private:
    char const *_name;
    std::vector<PropertyPair> _properties;

    void _addFormattedProperty(char const *name, char const *format, ...)
    {
        va_list args;
        va_start(args, format);
        gchar *value=g_strdup_vprintf(format, args);
        g_assert(value != nullptr);
        va_end(args);
        _addProperty(name, value);
        g_free(value);
    }
};

}

}

#endif
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
