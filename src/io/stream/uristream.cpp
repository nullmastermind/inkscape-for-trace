// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Our base String stream classes.  We implement these to
 * be based on Glib::ustring
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *
 * Copyright (C) 2004 Inkscape.org
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */


#include "uristream.h"
#include "io/sys.h"
#include <string>
#include <cstring>


namespace Inkscape
{
namespace IO
{

/*
 * URI scheme types
 */
#define SCHEME_NONE 0
#define SCHEME_FILE 1
#define SCHEME_DATA 2

/*
 * A temporary modification of Jon Cruz's portable fopen().
 * Simplified a bit, since we will always use binary
*/

#define FILE_READ  1
#define FILE_WRITE 2

static FILE *fopen_utf8name( char const *utf8name, int mode )
{
    FILE *fp = nullptr;
    if (!utf8name)
        {
        return nullptr;
        }
    if (mode!=FILE_READ && mode!=FILE_WRITE)
        {
        return nullptr;
        }

#ifndef _WIN32
    gchar *filename = g_filename_from_utf8( utf8name, -1, nullptr, nullptr, nullptr );
    if ( filename ) {
        if (mode == FILE_READ)
            fp = std::fopen(filename, "rb");
        else
            fp = std::fopen(filename, "wb");
        g_free(filename);
    }
#else
    {
        gunichar2 *wideName = g_utf8_to_utf16( utf8name, -1, NULL, NULL, NULL );
        if ( wideName )  {
            if (mode == FILE_READ)
                fp = _wfopen( (wchar_t*)wideName, L"rb" );
            else
                fp = _wfopen( (wchar_t*)wideName, L"wb" );
            g_free( wideName );
        } else {
            gchar *safe = Inkscape::IO::sanitizeString(utf8name);
            g_message("Unable to convert filename from UTF-8 to UTF-16 [%s]", safe);
            g_free(safe);
        }
    }
#endif

    return fp;
}



//#########################################################################
//# F I L E    I N P U T    S T R E A M
//#########################################################################


/**
 *
 */
FileInputStream::FileInputStream(FILE *source)
    : inf(source)
{
    if (!inf) {
        Glib::ustring err = "FileInputStream passed NULL";
        throw StreamException(err);
    }
}

/**
 *
 */
FileInputStream::~FileInputStream()
{
    close();
}

/**
 * Returns the number of bytes that can be read (or skipped over) from
 * this input stream without blocking by the next caller of a method for
 * this input stream.
 */
int FileInputStream::available()
{
    return 0;
}


/**
 *  Closes this input stream and releases any system resources
 *  associated with the stream.
 */
void FileInputStream::close()
{
            if (!inf)
                return;
            fflush(inf);
            fclose(inf);
            inf=nullptr;
}

/**
 * Reads the next byte of data from the input stream.  -1 if EOF
 */
int FileInputStream::get()
{
    int retVal = -1;
                if (!inf || feof(inf))
                {
                    retVal = -1;
                }
                else
                {
                    retVal = fgetc(inf);
                }

    return retVal;
}




//#########################################################################
//#  F I L E    O U T P U T    S T R E A M
//#########################################################################

FileOutputStream::FileOutputStream(FILE *fp)
    : ownsFile(false)
    , outf(fp)
{
    if (!outf) {
        Glib::ustring err = "FileOutputStream given null file ";
        throw StreamException(err);
    }
}

/**
 *
 */
FileOutputStream::~FileOutputStream()
{
    close();
}

/**
 * Closes this output stream and releases any system resources
 * associated with this stream.
 */
void FileOutputStream::close()
{
            if (!outf)
                return;
            fflush(outf);
            if ( ownsFile )
                fclose(outf);
            outf=nullptr;
}

/**
 *  Flushes this output stream and forces any buffered output
 *  bytes to be written out.
 */
void FileOutputStream::flush()
{
            if (!outf)
                return;
            fflush(outf);
}

/**
 * Writes the specified byte to this output stream.
 */
int FileOutputStream::put(char ch)
{
    unsigned char uch;

            if (!outf)
                return -1;
            uch = (unsigned char)(ch & 0xff);
            if (fputc(uch, outf) == EOF) {
                Glib::ustring err = "ERROR writing to file ";
                throw StreamException(err);
            }

    return 1;
}





} // namespace IO
} // namespace Inkscape


//#########################################################################
//# E N D    O F    F I L E
//#########################################################################

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
