/*
 * Author:
 *   mderezynski <mderezynski@users.sourceforge.net>
 *
 * Copyright (C) 2006 Author
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <glib-object.h>
#include <sigc++/sigc++.h>

#include <map>
#include <stddef.h>
#include <string>
#include <exception>

namespace Inkscape {

/**
 * @class ConnectionPool 
 *  an auxilliary class to manage sigc::connections as if the referred object
 *  were a GObject; in that way, the class that holds a ConnectionPool does not
 *  need to also hold several sigc::connection objects
 */
class ConnectionPool {
public:
    typedef std::map<std::string, sigc::connection*> ConnectionMap;

    struct NameExistsException : public std::exception {
        virtual const char* what() const throw() { return "Inkscape::ConnectionPool: name exists"; }
    };
    struct NameDoesNotExistException : public std::exception {
        virtual const char* what() const throw() { return "Inkscape::ConnectionPool: name doesn't exist"; }
    };

    void add_connection(std::string name, sigc::connection* connection) {
        if (_map.find(name) != _map.end()) {
            throw NameExistsException();
        }
        _map.insert(std::make_pair(name, connection)); 
    }

    void del_connection(std::string name) {
        ConnectionMap::iterator iter = _map.find(name); 
        if (iter == _map.end()) {
            throw NameDoesNotExistException();
        }
        sigc::connection* connection = (*iter).second;
        connection->disconnect();
        delete connection;
    }


    static Inkscape::ConnectionPool* new_connection_pool(std::string name) {
        return new ConnectionPool(name); 
    }

    static void del_connection_pool(ConnectionPool* pool) {
        delete pool;
    }

    static void connect_destroy(GObject *obj, ConnectionPool *pool) {
        g_object_connect (obj, "swapped-signal::destroy", G_CALLBACK(del_connection_pool), pool, NULL);
    }

    operator std::string() {
        return _name;
    }

private:
    ConnectionPool(std::string name) : _name(name) {}
    virtual ~ConnectionPool() {
        for (ConnectionMap::iterator iter = _map.begin(), end = _map.end(); iter != end; ++iter) {
            sigc::connection* connection = (*iter).second;
            connection->disconnect();
            delete connection;
        }
    }

    ConnectionMap _map;
    std::string _name;
};
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
