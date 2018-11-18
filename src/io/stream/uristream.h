// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_IO_URISTREAM_H
#define SEEN_INKSCAPE_IO_URISTREAM_H
/**
 * @file
 * This should be the only way that we provide sources/sinks
 * to any input/output stream.
 */
/*
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *
 * Copyright (C) 2004 Inkscape.org
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "object/uri.h"

#include "inkscapestream.h"


namespace Inkscape
{

class URI;

namespace IO
{

//#########################################################################
//# F I L E    I N P U T    S T R E A M
//#########################################################################

/**
 * This class is for receiving a stream of data from a file
 */
class FileInputStream : public InputStream
{

public:
    FileInputStream(FILE *source);

    ~FileInputStream() override;

    int available() override;

    void close() override;

    int get() override;

private:
    FILE *inf;           //for file: uris

}; // class FileInputStream




//#########################################################################
//# F I L E    O U T P U T    S T R E A M
//#########################################################################

/**
 * This class is for sending a stream to a destination file
 */
class FileOutputStream : public OutputStream
{

public:

    FileOutputStream(FILE *fp);

    ~FileOutputStream() override;

    void close() override;

    void flush() override;

    int put(char ch) override;

private:

    bool ownsFile;

    FILE *outf;         //for file: uris

}; // class FileOutputStream





} // namespace IO
} // namespace Inkscape


#endif // SEEN_INKSCAPE_IO_URISTREAM_H

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
