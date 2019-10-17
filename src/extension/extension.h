// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef INK_EXTENSION_H
#define INK_EXTENSION_H

/** \file
 * Frontend to certain, possibly pluggable, actions.
 */

/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2005 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <fstream>
#include <ostream>
#include <vector>

#include <glib.h>
#include <sigc++/signal.h>

namespace Glib {
    class ustring;
}

namespace Gtk {
	class Grid;
	class VBox;
	class Widget;
}

/** The key that is used to identify that the I/O should be autodetected */
#define SP_MODULE_KEY_AUTODETECT "autodetect"
/** This is the key for the SVG input module */
#define SP_MODULE_KEY_INPUT_SVG "org.inkscape.input.svg"
#define SP_MODULE_KEY_INPUT_SVGZ "org.inkscape.input.svgz"
/** Specifies the input module that should be used if none are selected */
#define SP_MODULE_KEY_INPUT_DEFAULT SP_MODULE_KEY_AUTODETECT
/** The key for outputting standard W3C SVG */
#define SP_MODULE_KEY_OUTPUT_SVG "org.inkscape.output.svg.plain"
#define SP_MODULE_KEY_OUTPUT_SVGZ "org.inkscape.output.svgz.plain"
/** This is an output file that has SVG data with the Sodipodi namespace extensions */
#define SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE "org.inkscape.output.svg.inkscape"
#define SP_MODULE_KEY_OUTPUT_SVGZ_INKSCAPE "org.inkscape.output.svgz.inkscape"
/** Which output module should be used? */
#define SP_MODULE_KEY_OUTPUT_DEFAULT SP_MODULE_KEY_AUTODETECT

/** Defines the key for Postscript printing */
#define SP_MODULE_KEY_PRINT_PS    "org.inkscape.print.ps"
#define SP_MODULE_KEY_PRINT_CAIRO_PS    "org.inkscape.print.ps.cairo"
#define SP_MODULE_KEY_PRINT_CAIRO_EPS    "org.inkscape.print.eps.cairo"
/** Defines the key for PDF printing */
#define SP_MODULE_KEY_PRINT_PDF    "org.inkscape.print.pdf"
#define SP_MODULE_KEY_PRINT_CAIRO_PDF    "org.inkscape.print.pdf.cairo"
/** Defines the key for LaTeX printing */
#define SP_MODULE_KEY_PRINT_LATEX    "org.inkscape.print.latex"
/** Defines the key for printing with GNOME Print */
#define SP_MODULE_KEY_PRINT_GNOME "org.inkscape.print.gnome"

/** Mime type for SVG */
#define MIME_SVG "image/svg+xml"

/** Name of the extension error file */
#define EXTENSION_ERROR_LOG_FILENAME  "extension-errors.log"


#define INKSCAPE_EXTENSION_URI   "http://www.inkscape.org/namespace/inkscape/extension"
#define INKSCAPE_EXTENSION_NS_NC "extension"
#define INKSCAPE_EXTENSION_NS    "extension:"

class SPDocument;

namespace Inkscape {

namespace XML {
class Node;
}

namespace Extension {

class ExecutionEnv;
class Dependency;
class ExpirationTimer;
class ExpirationTimer;
class InxParameter;
class InxWidget;

namespace Implementation
{
class Implementation;
}


/** The object that is the basis for the Extension system.  This object
    contains all of the information that all Extension have.  The
    individual items are detailed within. This is the interface that
    those who want to _use_ the extensions system should use.  This
    is most likely to be those who are inside the Inkscape program. */
class Extension {
public:
    /** An enumeration to identify if the Extension has been loaded or not. */
    enum state_t {
        STATE_LOADED,      /**< The extension has been loaded successfully */
        STATE_UNLOADED,    /**< The extension has not been loaded */
        STATE_DEACTIVATED  /**< The extension is missing something which makes it unusable */
    };

private:
    gchar     *_id = nullptr;                  /**< The unique identifier for the Extension */
    gchar     *_name = nullptr;                /**< A user friendly name for the Extension */
    state_t    _state = STATE_UNLOADED;        /**< Which state the Extension is currently in */
    std::vector<Dependency *>  _deps;          /**< Dependencies for this extension */
    static FILE *error_file;                   /**< This is the place where errors get reported */
    bool _gui;
    std::string _error_reason;                 /**< Short, textual explanation for the latest error */

protected:
    Inkscape::XML::Node *repr;                 /**< The XML description of the Extension */
    Implementation::Implementation * imp;      /**< An object that holds all the functions for making this work */
    ExecutionEnv * execution_env;              /**< Execution environment of the extension
                                                 *  (currently only used by Effects) */
    std::string _base_directory;               /**< Directory containing the .inx file,
                                                 *  relative paths in the extension should usually be relative to it */
    ExpirationTimer * timer = nullptr;         /**< Timeout to unload after a given time */
    bool _translation_enabled = true;          /**< Attempt translation of strings provided by the extension? */

private:
    const char *_translationdomain = nullptr;  /**< Domainname of gettext textdomain that should
                                                 *  be used for translation of the extension's strings */
    std::string _gettext_catalog_dir;          /**< Directory containing the gettext catalog for _translationdomain */

    void lookup_translation_catalog();

public:
    Extension(Inkscape::XML::Node *in_repr, Implementation::Implementation *in_imp, std::string *base_directory);
    virtual ~Extension();

    void          set_state    (state_t in_state);
    state_t       get_state    ();
    bool          loaded       ();
    virtual bool  check        ();
    Inkscape::XML::Node *      get_repr     ();
    gchar *       get_id       () const;
    gchar *       get_name     () const;
    void          deactivate   ();
    bool          deactivated  ();
    void          printFailure (Glib::ustring reason);
    std::string const &getErrorReason() { return _error_reason; };
    Implementation::Implementation * get_imp () { return imp; };
    void          set_execution_env (ExecutionEnv * env) { execution_env = env; };
    ExecutionEnv *get_execution_env () { return execution_env; };
    std::string   get_base_directory() const { return _base_directory; };
    void          set_base_directory(std::string base_directory) { _base_directory = base_directory; };
    std::string   get_dependency_location(const char *name);
    const char   *get_translation(const char* msgid, const char *msgctxt=nullptr);
    void          set_environment();

/* Parameter Stuff */
private:
    std::vector<InxWidget *> _widgets; /**< A list of widgets for this extension. */

public:
    /** \brief  A function to get the number of visible parameters of the extension.
        \return The number of visible parameters. */
    unsigned int widget_visible_count ( );

public:
    /** An error class for when a parameter is looked for that just
     * simply doesn't exist */
    class param_not_exist {};

    /** no valid ID found while parsing XML representation */
    class extension_no_id{};

    /** no valid name found while parsing XML representation */
    class extension_no_name{};

    /** extension is not compatible with the current system and should not be loaded */
    class extension_not_compatible{};

    /** An error class for when a filename already exists, but the user
     * doesn't want to overwrite it */
    class no_overwrite {};

private:
    void             make_param       (Inkscape::XML::Node * paramrepr);

    /**
     * Looks up the parameter with the specified name.
     *
     * Searches the list of parameters attached to this extension,
     * looking for a parameter with a matching name.
     *
     * This function can throw a 'param_not_exist' exception if the
     * name is not found.
     *
     * @param  name Name of the parameter to search for.
     * @return Parameter with matching name.
     */
     InxParameter *get_param(const gchar *name);

     InxParameter const *get_param(const gchar *name) const;

public:
    bool        get_param_bool          (const gchar *name) const;
    int         get_param_int           (const gchar *name) const;
    float       get_param_float         (const gchar *name) const;
    const char *get_param_string        (const gchar *name) const;
    const char *get_param_optiongroup   (const gchar *name) const;
    guint32     get_param_color         (const gchar *name) const;

    bool get_param_optiongroup_contains (const gchar *name, const char *value) const;

    bool        set_param_bool          (const gchar *name, const bool  value);
    int         set_param_int           (const gchar *name, const int   value);
    float       set_param_float         (const gchar *name, const float value);
    const char *set_param_string        (const gchar *name, const char *value);
    const char *set_param_optiongroup   (const gchar *name, const char *value);
    guint32     set_param_color         (const gchar *name, const guint32 color);


    /* Error file handling */
public:
    static void      error_file_open  ();
    static void      error_file_close ();
    static void      error_file_write (Glib::ustring text);

public:
    Gtk::Widget *autogui (SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal = nullptr);
    void paramListString(std::list <std::string> &retlist);
    void set_gui(bool s) { _gui = s; }
    bool get_gui() { return _gui; }

    /* Extension editor dialog stuff */
public:
    Gtk::VBox *get_info_widget();
    Gtk::VBox *get_params_widget();
protected:
    inline static void add_val(Glib::ustring labelstr, Glib::ustring valuestr, Gtk::Grid * table, int * row);
};



/*

This is a prototype for how collections should work.  Whoever gets
around to implementing this gets to decide what a 'folder' and an
'item' really is.  That is the joy of implementing it, eh?

class Collection : public Extension {

public:
    folder  get_root (void);
    int     get_count (folder);
    thumbnail get_thumbnail(item);
    item[]  get_items(folder);
    folder[]  get_folders(folder);
    metadata get_metadata(item);
    image   get_image(item);

};
*/

}  /* namespace Extension */
}  /* namespace Inkscape */

#endif // INK_EXTENSION_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
