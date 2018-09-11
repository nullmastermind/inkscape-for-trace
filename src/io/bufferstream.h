// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file
 * Phoebe DOM Implementation.
 *
 * This is a C++ approximation of the W3C DOM model, which follows
 * fairly closely the specifications in the various .idl files, copies of
 * which are provided for reference.  Most important is this one:
 *
 * http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/idl-definitions.html
 *//*
 * Authors:
 * see git history
 *   Bob Jamison
 *
 * Copyright (C) 2018 Authors
 * Released under GNU LGPL v2.1+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_BUFFERSTREAM_H
#define SEEN_BUFFERSTREAM_H


#include <vector>
#include "inkscapestream.h"


namespace Inkscape
{
namespace IO
{

//#########################################################################
//# S T R I N G    I N P U T    S T R E A M
//#########################################################################

/**
 * This class is for reading character from a DOMString
 *
 */
class BufferInputStream : public InputStream
{

public:

    BufferInputStream(const std::vector<unsigned char> &sourceBuffer);
    ~BufferInputStream() override;
    int available() override;
    void close() override;
    int get() override;

private:
    const std::vector<unsigned char> &buffer;
    long position;
    bool closed;

}; // class BufferInputStream




//#########################################################################
//# B U F F E R     O U T P U T    S T R E A M
//#########################################################################

/**
 * This class is for sending a stream to a character buffer
 *
 */
class BufferOutputStream : public OutputStream
{

public:

    BufferOutputStream();
    ~BufferOutputStream() override;
    void close() override;
    void flush() override;
    int put(char ch) override;
    virtual std::vector<unsigned char> &getBuffer()
        { return buffer; }

    virtual void clear()
        { buffer.clear(); }

private:
    std::vector<unsigned char> buffer;
    bool closed;

}; // class BufferOutputStream



}  //namespace IO
}  //namespace Inkscape



#endif // SEEN_BUFFERSTREAM_H
