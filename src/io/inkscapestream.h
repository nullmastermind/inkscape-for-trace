// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INKSCAPE_IO_INKSCAPESTREAM_H
#define SEEN_INKSCAPE_IO_INKSCAPESTREAM_H
/*
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *
 * Copyright (C) 2004 Inkscape.org
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <cstdio>
#include <glibmm/ustring.h>

namespace Inkscape
{
namespace IO
{

class StreamException : public std::exception
{
public:

    StreamException(const char *theReason) noexcept
        { reason = theReason; }
    StreamException(Glib::ustring &theReason) noexcept
        { reason = theReason; }
    ~StreamException() noexcept override
        = default;
    char const *what() const noexcept override
        { return reason.c_str(); }
        
private:
    Glib::ustring reason;

};

//#########################################################################
//# I N P U T    S T R E A M
//#########################################################################

/**
 * This interface is the base of all input stream classes.  Users who wish
 * to make an InputStream that is part of a chain should inherit from
 * BasicInputStream.  Inherit from this class to make a source endpoint,
 * such as a URI or buffer.
 *
 */
class InputStream
{

public:

    /**
     * Constructor.
     */
    InputStream() = default;

    /**
     * Destructor
     */
    virtual ~InputStream() = default;

    /**
     * Return the number of bytes that are currently available
     * to be read
     */
    virtual int available() = 0;
    
    /**
     * Do whatever it takes to 'close' this input stream
     * The most likely implementation of this method will be
     * for endpoints that use a resource for their data.
     */
    virtual void close() = 0;
    
    /**
     * Read one byte from this input stream.  This is a blocking
     * call.  If no data is currently available, this call will
     * not return until it exists.  If the user does not want
     * their code to block,  then the usual solution is:
     *     if (available() > 0)
     *         myChar = get();
     * This call returns -1 on end-of-file.
     */
    virtual int get() = 0;
    
}; // class InputStream




/**
 * This is the class that most users should inherit, to provide
 * their own streams.
 *
 */
class BasicInputStream : public InputStream
{

public:

    BasicInputStream(InputStream &sourceStream);
    
    ~BasicInputStream() override = default;
    
    int available() override;
    
    void close() override;
    
    int get() override;
    
protected:

    bool closed;

    InputStream &source;
    
private:


}; // class BasicInputStream



/**
 * Convenience class for reading from standard input
 */
class StdInputStream : public InputStream
{
public:

    int available() override
        { return 0; }
    
    void close() override
        { /* do nothing */ }
    
    int get() override
        {  return getchar(); }

};






//#########################################################################
//# O U T P U T    S T R E A M
//#########################################################################

/**
 * This interface is the base of all input stream classes.  Users who wish
 * to make an OutputStream that is part of a chain should inherit from
 * BasicOutputStream.  Inherit from this class to make a destination endpoint,
 * such as a URI or buffer.
 */
class OutputStream
{

public:

    /**
     * Constructor.
     */
    OutputStream() = default;

    /**
     * Destructor
     */
    virtual ~OutputStream() = default;

    /**
     * This call should
     *  1.  flush itself
     *  2.  close itself
     *  3.  close the destination stream
     */
    virtual void close() = 0;
    
    /**
     * This call should push any pending data it might have to
     * the destination stream.  It should NOT call flush() on
     * the destination stream.
     */
    virtual void flush() = 0;
    
    /**
     * Send one byte to the destination stream.
     */
    virtual int put(char ch) = 0;


}; // class OutputStream


/**
 * This is the class that most users should inherit, to provide
 * their own output streams.
 */
class BasicOutputStream : public OutputStream
{

public:

    BasicOutputStream(OutputStream &destinationStream);
    
    ~BasicOutputStream() override = default;

    void close() override;
    
    void flush() override;
    
    int put(char ch) override;

protected:

    bool closed;

    OutputStream &destination;


}; // class BasicOutputStream



/**
 * Convenience class for writing to standard output
 */
class StdOutputStream : public OutputStream
{
public:

    void close() override
        { }
    
    void flush() override
        { }
    
    int put(char ch) override
        {return  putchar(ch); }

};




//#########################################################################
//# R E A D E R
//#########################################################################


/**
 * This interface and its descendants are for unicode character-oriented input
 *
 */
class Reader
{

public:

    /**
     * Constructor.
     */
    Reader() = default;

    /**
     * Destructor
     */
    virtual ~Reader() = default;


    virtual int available() = 0;
    
    virtual void close() = 0;
    
    virtual char get() = 0;
    
    virtual Glib::ustring readLine() = 0;
    
    virtual Glib::ustring readWord() = 0;
    
    /* Input formatting */
    virtual const Reader& readBool (bool& val ) = 0;
    virtual const Reader& operator>> (bool& val ) = 0;
        
    virtual const Reader& readShort (short &val) = 0;
    virtual const Reader& operator>> (short &val) = 0;
        
    virtual const Reader& readUnsignedShort (unsigned short &val) = 0;
    virtual const Reader& operator>> (unsigned short &val) = 0;
        
    virtual const Reader& readInt (int &val) = 0;
    virtual const Reader& operator>> (int &val) = 0;
        
    virtual const Reader& readUnsignedInt (unsigned int &val) = 0;
    virtual const Reader& operator>> (unsigned int &val) = 0;
        
    virtual const Reader& readLong (long &val) = 0;
    virtual const Reader& operator>> (long &val) = 0;
        
    virtual const Reader& readUnsignedLong (unsigned long &val) = 0;
    virtual const Reader& operator>> (unsigned long &val) = 0;
        
    virtual const Reader& readFloat (float &val) = 0;
    virtual const Reader& operator>> (float &val) = 0;
        
    virtual const Reader& readDouble (double &val) = 0;
    virtual const Reader& operator>> (double &val) = 0;

}; // interface Reader



/**
 * This class and its descendants are for unicode character-oriented input
 *
 */
class BasicReader : public Reader
{

public:

    BasicReader(Reader &sourceStream);
    
    ~BasicReader() override = default;

    int available() override;
    
    void close() override;
    
    char get() override;
    
    Glib::ustring readLine() override;
    
    Glib::ustring readWord() override;
    
    /* Input formatting */
    const Reader& readBool (bool& val ) override;
    const Reader& operator>> (bool& val ) override
        { return readBool(val); }
        
    const Reader& readShort (short &val) override;
    const Reader& operator>> (short &val) override
        { return readShort(val); }
        
    const Reader& readUnsignedShort (unsigned short &val) override;
    const Reader& operator>> (unsigned short &val) override
        { return readUnsignedShort(val); }
        
    const Reader& readInt (int &val) override;
    const Reader& operator>> (int &val) override
        { return readInt(val); }
        
    const Reader& readUnsignedInt (unsigned int &val) override;
    const Reader& operator>> (unsigned int &val) override
        { return readUnsignedInt(val); }
        
    const Reader& readLong (long &val) override;
    const Reader& operator>> (long &val) override
        { return readLong(val); }
        
    const Reader& readUnsignedLong (unsigned long &val) override;
    const Reader& operator>> (unsigned long &val) override
        { return readUnsignedLong(val); }
        
    const Reader& readFloat (float &val) override;
    const Reader& operator>> (float &val) override
        { return readFloat(val); }
        
    const Reader& readDouble (double &val) override;
    const Reader& operator>> (double &val) override
        { return readDouble(val); }
 

protected:

    Reader *source;

    BasicReader()
        { source = nullptr; }

private:

}; // class BasicReader



/**
 * Class for placing a Reader on an open InputStream
 *
 */
class InputStreamReader : public BasicReader
{
public:

    InputStreamReader(InputStream &inputStreamSource);
    
    /*Overload these 3 for your implementation*/
    int available() override;
    
    void close() override;
    
    char get() override;


private:

    InputStream &inputStream;


};

/**
 * Convenience class for reading formatted from standard input
 *
 */
class StdReader : public BasicReader
{
public:

    StdReader();

    ~StdReader() override;
    
    /*Overload these 3 for your implementation*/
    int available() override;
    
    void close() override;
    
    char get() override;


private:

    InputStream *inputStream;


};





//#########################################################################
//# W R I T E R
//#########################################################################

/**
 * This interface and its descendants are for unicode character-oriented output
 *
 */
class Writer
{

public:

    /**
     * Constructor.
     */
    Writer() = default;

    /**
     * Destructor
     */
    virtual ~Writer() = default;

    virtual void close() = 0;
    
    virtual void flush() = 0;
    
    virtual void put(char ch) = 0;
    
    /* Formatted output */
    virtual Writer& printf(char const *fmt, ...) G_GNUC_PRINTF(2,3) = 0;

    virtual Writer& writeChar(char val) = 0;

    virtual Writer& writeUString(const Glib::ustring &val) = 0;

    virtual Writer& writeStdString(const std::string &val) = 0;

    virtual Writer& writeString(const char *str) = 0;

    virtual Writer& writeBool (bool val ) = 0;

    virtual Writer& writeShort (short val ) = 0;

    virtual Writer& writeUnsignedShort (unsigned short val ) = 0;

    virtual Writer& writeInt (int val ) = 0;

    virtual Writer& writeUnsignedInt (unsigned int val ) = 0;

    virtual Writer& writeLong (long val ) = 0;

    virtual Writer& writeUnsignedLong (unsigned long val ) = 0;

    virtual Writer& writeFloat (float val ) = 0;

    virtual Writer& writeDouble (double val ) = 0;

 

}; // interface Writer


/**
 * This class and its descendants are for unicode character-oriented output
 *
 */
class BasicWriter : public Writer
{

public:

    BasicWriter(Writer &destinationWriter);

    ~BasicWriter() override = default;

    /*Overload these 3 for your implementation*/
    void close() override;
    
    void flush() override;
    
    void put(char ch) override;
    
    
    
    /* Formatted output */
    Writer &printf(char const *fmt, ...) override G_GNUC_PRINTF(2,3);

    Writer& writeChar(char val) override;

    Writer& writeUString(const Glib::ustring &val) override;

    Writer& writeStdString(const std::string &val) override;

    Writer& writeString(const char *str) override;

    Writer& writeBool (bool val ) override;

    Writer& writeShort (short val ) override;

    Writer& writeUnsignedShort (unsigned short val ) override;

    Writer& writeInt (int val ) override;

    Writer& writeUnsignedInt (unsigned int val ) override;

    Writer& writeLong (long val ) override;

    Writer& writeUnsignedLong (unsigned long val ) override;

    Writer& writeFloat (float val ) override;

    Writer& writeDouble (double val ) override;

 
protected:

    Writer *destination;

    BasicWriter()
        { destination = nullptr; }
    
private:

}; // class BasicWriter



Writer& operator<< (Writer &writer, char val);

Writer& operator<< (Writer &writer, Glib::ustring &val);

Writer& operator<< (Writer &writer, std::string &val);

Writer& operator<< (Writer &writer, char const *val);

Writer& operator<< (Writer &writer, bool val);

Writer& operator<< (Writer &writer, short val);

Writer& operator<< (Writer &writer, unsigned short val);

Writer& operator<< (Writer &writer, int val);

Writer& operator<< (Writer &writer, unsigned int val);

Writer& operator<< (Writer &writer, long val);

Writer& operator<< (Writer &writer, unsigned long val);

Writer& operator<< (Writer &writer, float val);

Writer& operator<< (Writer &writer, double val);




/**
 * Class for placing a Writer on an open OutputStream
 *
 */
class OutputStreamWriter : public BasicWriter
{
public:

    OutputStreamWriter(OutputStream &outputStreamDest);
    
    /*Overload these 3 for your implementation*/
    void close() override;
    
    void flush() override;
    
    void put(char ch) override;


private:

    OutputStream &outputStream;


};


/**
 * Convenience class for writing to standard output
 */
class StdWriter : public BasicWriter
{
public:
    StdWriter();

    ~StdWriter() override;


    void close() override;

    
    void flush() override;

    
    void put(char ch) override;


private:

    OutputStream *outputStream;

};

//#########################################################################
//# U T I L I T Y
//#########################################################################

void pipeStream(InputStream &source, OutputStream &dest);



} // namespace IO
} // namespace Inkscape


#endif // SEEN_INKSCAPE_IO_INKSCAPESTREAM_H
