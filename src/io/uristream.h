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
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "object/uri.h"

#include "inkscapestream.h"


namespace Inkscape
{

class URI;

namespace IO
{

//#########################################################################
//# U R I    I N P U T    S T R E A M   /   R E A D E R
//#########################################################################

/**
 * This class is for receiving a stream of data from a resource
 * defined in a URI
 */
class UriInputStream : public InputStream
{

public:
    UriInputStream(FILE *source, Inkscape::URI &uri);

    UriInputStream(Inkscape::URI &source);

    ~UriInputStream() override;

    int available() override;

    void close() override;

    int get() override;

private:
    Inkscape::URI &uri;
    FILE *inf;           //for file: uris
    unsigned char *data; //for data: uris
    int dataPos;         //  current read position in data field
    int dataLen;         //  length of data buffer
    bool closed;
    int scheme;


}; // class UriInputStream




/**
 * This class is for receiving a stream of formatted data from a resource
 * defined in a URI
 */
class UriReader : public BasicReader
{

public:

    UriReader(Inkscape::URI &source);

    ~UriReader() override;

    int available() override;

    void close() override;

    char get() override;

private:

    UriInputStream *inputStream;

}; // class UriReader



//#########################################################################
//# U R I    O U T P U T    S T R E A M    /    W R I T E R
//#########################################################################

/**
 * This class is for sending a stream to a destination resource
 * defined in a URI
 *
 */
class UriOutputStream : public OutputStream
{

public:

    UriOutputStream(FILE *fp, Inkscape::URI &destination);

    UriOutputStream(Inkscape::URI &destination);

    ~UriOutputStream() override;

    void close() override;

    void flush() override;

    int put(char ch) override;

private:

    bool closed;
    bool ownsFile;

    FILE *outf;         //for file: uris
    Glib::ustring data; //for data: uris

    Inkscape::URI &uri;

    int scheme;

}; // class UriOutputStream





/**
 * This class is for sending a stream of formatted data to a resource
 * defined in a URI
 */
class UriWriter : public BasicWriter
{

public:

    UriWriter(Inkscape::URI &source);

    ~UriWriter() override;

    void close() override;

    void flush() override;

    void put(char ch) override;

private:

    UriOutputStream *outputStream;

}; // class UriReader






} // namespace IO
} // namespace Inkscape


#endif // SEEN_INKSCAPE_IO_URISTREAM_H
