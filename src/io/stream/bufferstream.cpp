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

/**
 * This class provided buffered endpoints for input and output.
 */

#include "bufferstream.h"

namespace Inkscape
{
namespace IO
{

//#########################################################################
//# B U F F E R    I N P U T    S T R E A M
//#########################################################################
/**
 *
 */
BufferInputStream::BufferInputStream(
           const std::vector<unsigned char> &sourceBuffer)
           : buffer(sourceBuffer)
{
    position = 0;
    closed = false;
}

/**
 *
 */
BufferInputStream::~BufferInputStream()
= default;

/**
 * Returns the number of bytes that can be read (or skipped over) from
 * this input stream without blocking by the next caller of a method for
 * this input stream.
 */
int BufferInputStream::available()
{
    if (closed)
        return -1;
    return buffer.size() - position;
}


/**
 *  Closes this input stream and releases any system resources
 *  associated with the stream.
 */
void BufferInputStream::close()
{
    closed = true;
}

/**
 * Reads the next byte of data from the input stream.  -1 if EOF
 */
int BufferInputStream::get()
{
    if (closed)
        return -1;
    if (position >= (int)buffer.size())
        return -1;
    int ch = (int) buffer[position++];
    return ch;
}




//#########################################################################
//# B U F F E R    O U T P U T    S T R E A M
//#########################################################################

/**
 *
 */
BufferOutputStream::BufferOutputStream()
{
    closed = false;
}

/**
 *
 */
BufferOutputStream::~BufferOutputStream()
= default;

/**
 * Closes this output stream and releases any system resources
 * associated with this stream.
 */
void BufferOutputStream::close()
{
    closed = true;
}

/**
 *  Flushes this output stream and forces any buffered output
 *  bytes to be written out.
 */
void BufferOutputStream::flush()
{
    //nothing to do
}

/**
 * Writes the specified byte to this output stream.
 */
int BufferOutputStream::put(char ch)
{
    if (closed)
        return -1;
    buffer.push_back(ch);
    return 1;
}




}  //namespace IO
}  //namespace Inkscape

//#########################################################################
//# E N D    O F    F I L E
//#########################################################################
