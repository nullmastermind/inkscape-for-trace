// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2017 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include "xml/repr.h"
#include "inkscape.h"
#include "document.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    g_type_init();
    Inkscape::GC::init();
    if ( !Inkscape::Application::exists() )
        Inkscape::Application::create(false);
    //void* a= sp_repr_read_mem((const char*)data, size, 0);
    auto doc = std::unique_ptr<SPDocument>(SPDocument::createNewDocFromMem((const char *)data, size, 0));
    return 0;
}
