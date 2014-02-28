
#ifndef SEEN_URI_TEST_H
#define SEEN_URI_TEST_H
/*
 * Test uri.h
 *
 * Written to aid with refactoring the uri handling to support fullPath
 * and data URIs and also cover code which wasn't before tested.
 *
 * Copyright 2014 (c) BasisTech Boston
 *
 */
#include <cxxtest/TestSuite.h>

#include "uri.h"

using Inkscape::URI;

class URITest : public CxxTest::TestSuite
{
public:
    void stringTest( std::string result, std::string expected )
    {
        if ( !result.empty() && !expected.empty() ) {
            TS_ASSERT_EQUALS( result, expected );
        } else if ( result.empty() && !expected.empty() ) {
            TS_FAIL( std::string("Expected (") + expected + "), found null" );
        } else if ( !result.empty() && expected.empty() ) {
            TS_FAIL( std::string("Expected null, found (") + result + ")" );
        }
    }

    std::string ValueOrEmpty(const char* s) {
        return s == NULL ? std::string() : s;
    }

    void testToString()
    {
        // Using std::strings is a statement that URI should move to std::string
        // instead of using c_strings. Please don't turn these to c strings.
        const std::string cases[][2] = {
            { "foo",            "foo" },
            { "#foo",           "#foo" },
            { "blah.svg#h",     "blah.svg#h" },
            { "data:data",      "data:data" },
            { "data:head,data", "data:head,data" },
        };

        for ( size_t i = 0; i < G_N_ELEMENTS(cases); i++ ) {
            stringTest( URI(cases[i][0].c_str()).toString(), cases[i][1] );
        }
    }
    void testDataUri()
    {
        URI test_uri;
        std::string cases[][5] = {
            { "data:HAIL-DATUM",           "HAIL-DATUM", "test/plain", "US-ASCII", "" },
            { "data:image/png,HAIL-DATUM", "HAIL-DATUM", "image/png", "US-ASCII",  "" },
        };

        for ( size_t i = 0; i < G_N_ELEMENTS(cases); i++ ) {
            test_uri = URI( cases[i][0].c_str() );
            stringText( test_uri.getData(),      cases[i][1] );
            stringText( test_uri.getMimeType(),  cases[i][2] );
            stringText( test_uri.getCharset(),   cases[i][3] );
            vectorText( test_uri.getDataBytes(), cases[i][4] );
            // We will assert urls aren't data in the url test
            TS_ASSERT_TRUE( test_uri.isData() );
        }
    }
    void testPath()
    {
        const std::string cases[][2] = {
            { "foo.svg",     "foo.svg" },
            { "foo.svg#bar", "foo.svg" },
            { "#bar",        NULL },
            { "data:data",   NULL },
        };

        for ( size_t i = 0; i < G_N_ELEMENTS(cases); i++ ) {
            stringTest( ValueOrEmpty(URI(cases[i][0].c_str()).getPath()), cases[i][1] );
        }
    }
    void testFullPath() {
        std::ofstream fhl("/tmp/cxxtest-uri.svg", std::ofstream::out);
        stringTest( URI("cxxtest-uri.svg").getFullPath("/tmp"), std::string("/tmp/cxxtest-uri.svg") );
        //stringTest( URI("cxxtest-uri.svg").getFullPath("/usr/../tmp"), std::string("/tmp/cxxtest-uri.svg") );
    }
    void testUrls() {
        URI test_uri;
        char const* cases[][3] = {
            { "file:///tmp/a.svg",         "/tmp/a.svg", "file" },
            { "http://i.org/s.svg",        "",           "http" },
        };

        for ( size_t i = 0; i < G_N_ELEMENTS(cases); i++ ) {
            test_uri = URI( cases[i][0].c_str() );
            stringText( test_uri.getPath(),       cases[i][1] );
            stringText( test_uri.getTransport(),  cases[i][2] );
            // We will assert urls are data in the data uri test
            TS_ASSERT_FALSE( test_uri.isData() );
        }

    }

};

#endif // SEEN_URI_TEST_H

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
