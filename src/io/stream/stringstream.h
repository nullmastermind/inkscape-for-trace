// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef __INKSCAPE_IO_STRINGSTREAM_H__
#define __INKSCAPE_IO_STRINGSTREAM_H__

#include <glibmm/ustring.h>

#include "inkscapestream.h"


namespace Inkscape
{
namespace IO
{


//#########################################################################
//# S T R I N G    I N P U T    S T R E A M
//#########################################################################

/**
 * This class is for reading character from a Glib::ustring
 *
 */
class StringInputStream : public InputStream
{

public:

    StringInputStream(Glib::ustring &sourceString);
    
    ~StringInputStream() override;
    
    int available() override;
    
    void close() override;
    
    int get() override;
    
private:

    Glib::ustring &buffer;

    long position;

}; // class StringInputStream




//#########################################################################
//# S T R I N G   O U T P U T    S T R E A M
//#########################################################################

/**
 * This class is for sending a stream to a Glib::ustring
 *
 */
class StringOutputStream : public OutputStream
{

public:

    StringOutputStream();
    
    ~StringOutputStream() override;
    
    void close() override;
    
    void flush() override;
    
    int put(char ch) override;

    virtual Glib::ustring &getString()
        { return buffer; }

    virtual void clear()
        { buffer = ""; }

private:

    Glib::ustring buffer;


}; // class StringOutputStream







} // namespace IO
} // namespace Inkscape



#endif /* __INKSCAPE_IO_STRINGSTREAM_H__ */
