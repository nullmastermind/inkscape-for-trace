/**
 * @file
 * Color swatches dialog.
 */
/* Authors:
 *   Jon A. Cruz
 *   John Bintz
 *   Abhishek Sharma
 *   Theodore Janezcko
 *
 * Copyright (C) 2005 Jon A. Cruz
 * Copyright (C) 2008 John Bintz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <errno.h>
#include <map>
#include <algorithm>
#include <set>

#include "swatches.h"
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/widget.h>

#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <glibmm/timer.h>
#include <gdkmm/pixbuf.h>

#include "color-item.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-style.h"
#include "document.h"
#include "document-private.h"
#include "document-undo.h"
#include "extension/db.h"
#include "inkscape.h"
#include "inkscape.h"
#include "io/sys.h"
#include "io/resource.h"
#include "message-context.h"
#include "path-prefix.h"
#include "preferences.h"
#include "sp-item.h"
#include "sp-gradient.h"
#include "sp-gradient-vector.h"
#include "style.h"
#include "widgets/desktop-widget.h"
#include "widgets/gradient-vector.h"
#include "widgets/eek-preview.h"
#include "display/cairo-utils.h"
#include "sp-gradient-reference.h"
#include "dialog-manager.h"
#include "selection.h"
#include "verbs.h"
#include "gradient-chemistry.h"
#include "helper/action.h"
#include "xml/node-observer.h"
#include "xml/repr.h"
#include "sp-pattern.h"
#include "icon-size.h"
#include "widgets/icon.h"
#include "filedialog.h"
#include "sp-stop.h"
#include "svg/svg-color.h"
#include "sp-radial-gradient.h"
#include "color-rgba.h"
#include "ui/tools/tool-base.h"
#include "svg/css-ostringstream.h"
#include <queue>
#ifdef WIN32
#include <windows.h>
#endif

//lazy!
void sp_desktop_set_gradient(SPDesktop *desktop, SPGradient* gradient, bool fill);

namespace Inkscape {
namespace UI {
namespace Dialogs {

#define SWATCHES_FILE_NAME "swatches.svg"

static char* trim( char* str ) {
    char* ret = str;
    while ( *str && (*str == ' ' || *str == '\t') ) {
        str++;
    }
    ret = str;
    while ( *str ) {
        str++;
    }
    str--;
    while ( str > ret && (( *str == ' ' || *str == '\t' ) || *str == '\r' || *str == '\n') ) {
        *str-- = 0;
    }
    return ret;
}

static void skipWhitespace( char*& str ) {
    while ( *str == ' ' || *str == '\t' ) {
        str++;
    }
}

static bool parseNum( char*& str, int& val ) {
    val = 0;
    while ( '0' <= *str && *str <= '9' ) {
        val = val * 10 + (*str - '0');
        str++;
    }
    bool retval = !(*str == 0 || *str == ' ' || *str == '\t' || *str == '\r' || *str == '\n');
    return retval;
}

static char * SwatchFile;
static SPDocument * SwatchDocument;
static unsigned int page_suffix;

static void loadPalletFile()
{
    if (!SwatchDocument) {
        SwatchFile=g_build_filename(INKSCAPE_PALETTESDIR, _("swatches.svg"), NULL);
        SwatchDocument=SPDocument::createNewDoc (SwatchFile, TRUE);
        if (!SwatchDocument) {
            SwatchDocument = SPDocument::createNewDoc(NULL, TRUE, true);
            sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
        }
    }
}

static void addStop( Inkscape::XML::Node *parent, Glib::ustring const &color, gfloat opacity, gchar const *offset )
{
#ifdef SP_GR_VERBOSE
    g_message("addStop(%p, %s, %d, %s)", parent, color.c_str(), opacity, offset);
#endif
    Inkscape::XML::Node *stop = parent->document()->createElement("svg:stop");
    {
        gchar *tmp = g_strdup_printf( "stop-color:%s;stop-opacity:%f;", color.c_str(), opacity < 0.0 ? 0.0 : (opacity > 1.0 ? 1.0 : opacity) );
        stop->setAttribute( "style", tmp );
        g_free(tmp);
    }

    stop->setAttribute( "offset", offset );

    parent->appendChild(stop);
    Inkscape::GC::release(stop);
}

static SPGroup* importGPL(SPDocument* doc, const gchar* full)
{
    SPGroup* ret = NULL;
    if ( !Inkscape::IO::file_test( full, G_FILE_TEST_IS_DIR ) ) {
                        
        /*Load the pallet file here*/
        char block[1024];
        FILE *f = Inkscape::IO::fopen_utf8name( full, "r" );
        if ( f ) {
            char* result = fgets( block, sizeof(block), f );
            if ( result ) {
                if ( strncmp( "GIMP Palette", block, 12 ) == 0 ) {
                    bool inHeader = true;
                    bool hasErr = false;

                    Inkscape::XML::Node * page = doc->getReprDoc()->createElement("svg:g");
                    gchar *id=NULL;
                    do {
                        g_free(id);
                        id = g_strdup_printf("page%d", page_suffix++);
                    } while (doc->getObjectById(id));

                    page->setAttribute("id", id);

                    do {
                        result = fgets( block, sizeof(block), f );
                        block[sizeof(block) - 1] = 0;
                        if ( result ) {
                            if ( block[0] == '#' ) {
                                // ignore comment
                            } else {
                                char *ptr = block;
                                // very simple check for header versus entry
                                while ( *ptr == ' ' || *ptr == '\t' ) {
                                    ptr++;
                                }
                                if ( (*ptr == 0) || (*ptr == '\r') || (*ptr == '\n') ) {
                                    // blank line. skip it.
                                } else if ( '0' <= *ptr && *ptr <= '9' ) {
                                    // should be an entry link
                                    inHeader = false;
                                    ptr = block;
                                    Glib::ustring name("");
                                    skipWhitespace(ptr);
                                    if ( *ptr ) {
                                        int r = 0;
                                        int g = 0;
                                        int b = 0;
                                        hasErr = parseNum(ptr, r);
                                        if ( !hasErr ) {
                                            skipWhitespace(ptr);
                                            hasErr = parseNum(ptr, g);
                                        }
                                        if ( !hasErr ) {
                                            skipWhitespace(ptr);
                                            hasErr = parseNum(ptr, b);
                                        }
                                        if ( !hasErr && *ptr ) {
                                            char* n = trim(ptr);
                                            if (n != NULL) {
                                                name = g_dpgettext2(NULL, "Palette", n);
                                            }
                                        }
                                        if ( !hasErr ) {
                                            // Add the entry now
                                            
                                            Inkscape::XML::Node *grad = doc->getReprDoc()->createElement("svg:linearGradient");
                                            grad->setAttribute("inkscape:label", name.c_str());
                                            grad->setAttribute( "osb:paint", "solid", 0 );
                                            SPColor color((float)r / 255, (float)g / 255, (float)b / 255);
                                            addStop(grad, color.toString(), 1, "0");
                                            page->appendChild(grad);
                                            Inkscape::GC::release(grad);
                                        }
                                    } else {
                                        hasErr = true;
                                    }
                                } else {
                                    if ( !inHeader ) {
                                        // Hmmm... probably bad. Not quite the format we want?
                                        hasErr = true;
                                    } else {
                                        char* sep = strchr(result, ':');
                                        if ( sep ) {
                                            *sep = 0;
                                            char* val = trim(sep + 1);
                                            char* name = trim(result);
                                            if ( *name ) {
                                                if ( strcmp( "Name", name ) == 0 )
                                                {
                                                    page->setAttribute("inkscape:label", val);
                                                }
                                            } else {
                                                // error
                                                hasErr = true;
                                            }
                                        } else {
                                            // error
                                            hasErr = true;
                                        }
                                    }
                                }
                            }
                        }
                    } while ( result && !hasErr );
                    if ( !hasErr ) {
                        doc->getDefs()->appendChild(page);
                        Inkscape::GC::release(page);
                        SPObject* obj = doc->getObjectByRepr(page);
                        if (SP_IS_GROUP(obj)) {
                            ret = SP_GROUP(obj);
                        }
    #if ENABLE_MAGIC_COLORS
                        ColorItem::_wireMagicColors( onceMore );
    #endif // ENABLE_MAGIC_COLORS
                    } else {
                        delete page;
                    }
                }
            }

            fclose(f);
        }
        /* end loading the pallet file*/
    }
    return ret;
}

SwatchesPanel& SwatchesPanel::getInstance()
{
    return *new SwatchesPanel();
}

class SwatchesPanel::StopWatcher : public Inkscape::XML::NodeObserver {
public:
    StopWatcher(SwatchesPanel* pnl, SPStop* obj) :
        _pnl(pnl),
        _obj(obj),
        _repr(obj->getRepr())
    {
        _repr->addObserver(*this);
    }
        
    ~StopWatcher() {
        _repr->removeObserver(*this);
    }
    
    virtual void notifyChildAdded( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &/*child*/, Inkscape::XML::Node */*prev*/ ){}
    virtual void notifyChildRemoved( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &/*child*/, Inkscape::XML::Node */*prev*/ ){}
    virtual void notifyChildOrderChanged( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &/*child*/, Inkscape::XML::Node */*old_prev*/, Inkscape::XML::Node */*new_prev*/ ){}
    virtual void notifyContentChanged( Inkscape::XML::Node &/*node*/, Util::ptr_shared<char> /*old_content*/, Util::ptr_shared<char> /*new_content*/ ) {}
    
    virtual void notifyAttributeChanged( Inkscape::XML::Node &/*node*/, GQuark name, Util::ptr_shared<char> /*old_value*/, Util::ptr_shared<char> /*new_value*/ ) {
        if (_pnl && _obj) {
            _pnl->_defsChanged( );
        }
    }

    SwatchesPanel* _pnl;
    SPStop* _obj;
    Inkscape::XML::Node* _repr;
};

class SwatchesPanel::GradientWatcher : public Inkscape::XML::NodeObserver {
public:
    GradientWatcher(SwatchesPanel* pnl, SPGradient* obj) :
        _pnl(pnl),
        _obj(obj),
        _repr(obj->getRepr()),
        _labelAttr(g_quark_from_string("inkscape:label")),
        _swatchAttr(g_quark_from_string("osb:paint"))
    {
        _repr->addObserver(*this);
    }
        
    ~GradientWatcher() {
        _repr->removeObserver(*this);
    }
    
    virtual void notifyChildAdded( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &/*child*/, Inkscape::XML::Node */*prev*/ )
    {
        if ( _pnl && _obj && _obj->isSwatch()) {
            _pnl->_defsChanged( );
        }
    }
    virtual void notifyChildRemoved( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &/*child*/, Inkscape::XML::Node */*prev*/ )
    {
        if ( _pnl && _obj && _obj->isSwatch() ) {
            _pnl->_defsChanged( );
        }
    }
    virtual void notifyChildOrderChanged( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &/*child*/, Inkscape::XML::Node */*old_prev*/, Inkscape::XML::Node */*new_prev*/ )
    {
        if ( _pnl && _obj && _obj->isSwatch() ) {
            _pnl->_defsChanged( );
        }
    }
    virtual void notifyContentChanged( Inkscape::XML::Node &/*node*/, Util::ptr_shared<char> /*old_content*/, Util::ptr_shared<char> /*new_content*/ ) {}
    virtual void notifyAttributeChanged( Inkscape::XML::Node &/*node*/, GQuark name, Util::ptr_shared<char> /*old_value*/, Util::ptr_shared<char> /*new_value*/ ) {
        if (_pnl && _obj && ((_obj->isSwatch() && name == _labelAttr) || name == _swatchAttr)) {
            _pnl->_defsChanged( );
        }
    }

    SwatchesPanel* _pnl;
    SPGradient* _obj;
    Inkscape::XML::Node* _repr;
    GQuark _labelAttr;
    GQuark _swatchAttr;
};

class SwatchesPanel::SwatchWatcher : public Inkscape::XML::NodeObserver {
public:
    SwatchWatcher(SwatchesPanel* pnl, SPObject* obj, bool builtIn) :
        _pnl(pnl),
        _obj(obj),
        _builtIn(builtIn),
        _repr(obj->getRepr()),
        _labelAttr(g_quark_from_string("inkscape:label")),
        _swatchAttr(g_quark_from_string("osb:paint"))
    {
        _repr->addObserver(*this);
    }
        
    ~SwatchWatcher() {
        _repr->removeObserver(*this);
    }
    
    virtual void notifyChildAdded( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &child, Inkscape::XML::Node */*prev*/ )
    {
        if ( _pnl && _obj) {
            SPObject *childobj = _builtIn ? SwatchDocument->getObjectByRepr(&child) : (_pnl->_currentDocument ? _pnl->_currentDocument->getObjectByRepr(&child) : 0);
            if (childobj && ((SP_IS_GRADIENT(childobj) && SP_GRADIENT(childobj)->hasStops()) || SP_IS_GROUP(childobj))) {
                if (_builtIn) {
                    _pnl->_swatchesChanged( );
                } else {
                    _pnl->_defsChanged( );
                }
            }
        }
    }
    virtual void notifyChildRemoved( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &child, Inkscape::XML::Node */*prev*/ )
    {
        if ( _pnl && _obj) {
            if (_builtIn) {
                _pnl->_swatchesChanged( );
            } else {
                _pnl->_defsChanged( );
            }
        }
    }
    virtual void notifyChildOrderChanged( Inkscape::XML::Node &/*node*/, Inkscape::XML::Node &child, Inkscape::XML::Node */*old_prev*/, Inkscape::XML::Node */*new_prev*/ )
    {
        if ( _pnl && _obj) {
            SPObject *childobj = _builtIn ? SwatchDocument->getObjectByRepr(&child) : (_pnl->_currentDocument ? _pnl->_currentDocument->getObjectByRepr(&child) : 0);
            if (childobj && ((SP_IS_GRADIENT(childobj) && SP_GRADIENT(childobj)->hasStops()) || SP_IS_GROUP(childobj))) {
                if (_builtIn) {
                    _pnl->_swatchesChanged( );
                } else {
                    _pnl->_defsChanged( );
                }
            }
        }
    }
    virtual void notifyContentChanged( Inkscape::XML::Node &/*node*/, Util::ptr_shared<char> /*old_content*/, Util::ptr_shared<char> /*new_content*/ ) {}
    virtual void notifyAttributeChanged( Inkscape::XML::Node &/*node*/, GQuark name, Util::ptr_shared<char> /*old_value*/, Util::ptr_shared<char> /*new_value*/ ) {
        if (_pnl && _obj && name == _labelAttr) {
            if (_builtIn) {
                _pnl->_swatchesChanged( );
            } else {
                _pnl->_defsChanged( );
            }
        }
    }

    SwatchesPanel* _pnl;
    SPObject* _obj;
    bool _builtIn;
    Inkscape::XML::Node* _repr;
    GQuark _labelAttr;
    GQuark _swatchAttr;
};

class SwatchesPanel::ModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:

    ModelColumns()
    {
        add(_colObject);
        add(_colLabel);
    }
    virtual ~ModelColumns() {}

    Gtk::TreeModelColumn<SPObject*> _colObject;
    Gtk::TreeModelColumn<Glib::ustring> _colLabel;
};

class SwatchesPanel::ModelColumnsDoc : public Gtk::TreeModel::ColumnRecord
{
public:

    ModelColumnsDoc()
    {
        add(_colObject);
        add(_colLabel);
        add(_colPixbuf);
    }
    virtual ~ModelColumnsDoc() {}

    Gtk::TreeModelColumn<SPObject*> _colObject;
    Gtk::TreeModelColumn<Glib::ustring> _colLabel;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > _colPixbuf;
};

static void StripChildGroups(Inkscape::XML::Node * node, SPObject* addTo)
{
    for (Inkscape::XML::Node * it = node->firstChild(); it != NULL;) {
        if (!strcmp(it->name(), "svg:g")) {
            Inkscape::XML::Node * todel = it;
            it = it->next();
            node->removeChild(todel);
        } else {
            it = it->next();
        }
    }
    addTo->appendChildRepr(node);
}

static void BubbleChildGroups(Inkscape::XML::Node * node, SPObject* addTo)
{
    std::queue<Inkscape::XML::Node *> groups;
    for (Inkscape::XML::Node * it = node->firstChild(); it != NULL; ) {
        if (!strcmp(it->name(), "svg:g")) {
            groups.push(it->duplicate(addTo->document->getReprDoc()));
            Inkscape::XML::Node * todel = it;
            it = it->next();
            node->removeChild(todel);
        } else {
            it = it->next();
        }
    }
    addTo->appendChildRepr(node);
    while (!groups.empty()) {
        Inkscape::XML::Node * it = groups.front();
        groups.pop();
        BubbleChildGroups(it, addTo);
        Inkscape::GC::release(it);
    }
}

void SwatchesPanel::_addSwatchButtonClicked(SPGroup* swatch, bool recurse)
{
    if (_currentDocument) {
        if (swatch && SP_IS_GROUP(swatch) ) {
            Inkscape::XML::Node * copy = swatch->getRepr()->duplicate(_currentDocument->getReprDoc());
            if (recurse) {
                BubbleChildGroups(copy, _currentDocument->getDefs());
            } else {
                StripChildGroups(copy, _currentDocument->getDefs());
            }
            Inkscape::GC::release(copy);
            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add swatches to document"));
        }
    }
}

void SwatchesPanel::_importButtonClicked(bool addToDoc, bool addToBI)
{
    if (addToDoc || addToBI) {
            //# Get the current directory for finding files
        static Glib::ustring open_path;
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();

        if(open_path.empty())
        {
            Glib::ustring attr = prefs->getString("/dialogs/open/path");
            if (!attr.empty()) open_path = attr;
        }

        //# Test if the open_path directory exists
        if (!Inkscape::IO::file_test(open_path.c_str(),
                  (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
            open_path = "";

    #ifdef WIN32
        //# If no open path, default to our win32 documents folder
        if (open_path.empty())
        {
            // The path to the My Documents folder is read from the
            // value "HKEY_CURRENT_USER\Software\Windows\CurrentVersion\Explorer\Shell Folders\Personal"
            HKEY key = NULL;
            if(RegOpenKeyExA(HKEY_CURRENT_USER,
                "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
            {
                WCHAR utf16path[_MAX_PATH];
                DWORD value_type;
                DWORD data_size = sizeof(utf16path);
                if(RegQueryValueExW(key, L"Personal", NULL, &value_type,
                    (BYTE*)utf16path, &data_size) == ERROR_SUCCESS)
                {
                    g_assert(value_type == REG_SZ);
                    gchar *utf8path = g_utf16_to_utf8(
                        (const gunichar2*)utf16path, -1, NULL, NULL, NULL);
                    if(utf8path)
                    {
                        open_path = Glib::ustring(utf8path);
                        g_free(utf8path);
                    }
                }
            }
        }
    #endif

        //# If no open path, default to our home directory
        if (open_path.empty())
        {
            open_path = g_get_home_dir();
            open_path.append(G_DIR_SEPARATOR_S);
        }

        Gtk::Window * parent = SP_ACTIVE_DESKTOP->getToplevel();
        //# Create a dialog
        Inkscape::UI::Dialog::FileOpenDialog *openDialogInstance =
                  Inkscape::UI::Dialog::FileOpenDialog::create(
                     *parent, open_path,
                     Inkscape::UI::Dialog::SWATCH_TYPES,
                     _("Select file to open"));

        //# Show the dialog
        bool const success = openDialogInstance->show();

        //# Save the folder the user selected for later
        open_path = openDialogInstance->getCurrentDirectory();

        if (!success)
        {
            delete openDialogInstance;
            return;
        }

        //# User selected something.  Get name and type
        Glib::ustring fileName = openDialogInstance->getFilename();

        //# We no longer need the file dialog object - delete it
        delete openDialogInstance;
        openDialogInstance = NULL;


        if (!fileName.empty())
        {
            Glib::ustring newFileName = Glib::filename_to_utf8(fileName);

            if ( newFileName.size() > 0)
                fileName = newFileName;
            else
                g_warning( "ERROR CONVERTING OPEN FILENAME TO UTF-8" );

            open_path = Glib::path_get_dirname (fileName);
            open_path.append(G_DIR_SEPARATOR_S);
            prefs->setString("/dialogs/open/path", open_path);

            SPDocument* importdoc = SPDocument::createNewDoc(fileName.c_str(), true);
            Inkscape::XML::Node * page = NULL;
            if (importdoc && importdoc->getDefs()) {
                if (addToBI) {
                    page = SwatchDocument->getReprDoc()->createElement("svg:g");
                    gchar *id=NULL;
                    do {
                        g_free(id);
                        id = g_strdup_printf("page%d", page_suffix++);
                    } while (SwatchDocument->getObjectById(id));

                    page->setAttribute("id", id);
                    gchar* name = g_path_get_basename(importdoc->getName());
                    page->setAttribute("inkscape:label", name);
                    g_free(name);
                }

                for (SPObject* it = importdoc->getDefs()->firstChild(); it != NULL; it = it->next) {
                    if (SP_IS_GROUP(it)) {
                        if (page) {
                            Inkscape::XML::Node * copy = it->getRepr()->duplicate(SwatchDocument->getReprDoc());
                            page->appendChild(copy);
                        }
                        if (addToDoc) {
                            _addSwatchButtonClicked(SP_GROUP(it), false);
                        }
                    }
                }
                if (page) {
                    SwatchDocument->getDefs()->appendChildRepr(page);
                    sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
                }

                importdoc->doUnref();
            } else {
                SPGroup* g = importGPL(addToBI ? SwatchDocument : _currentDocument, fileName.c_str());
                if (addToBI) {
                    if (addToDoc) {
                        _addSwatchButtonClicked(g, false);
                    }
                    sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
                } else {
                    DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add swatches to document"));
                }
            }
        }
    }
}

void SwatchesPanel::SetSelectedFill(SPGradient* swatch)
{
    if (_currentDesktop && _currentDocument) {
        SPItem* it = _currentDesktop->selection->singleItem();
        if (it) {
            if (it->style->fill.isSet()) {
                if (it->style->fill.isPaintserver()) {
                    SPPaintServer * server = it->style->getFillPaintServer();
                    if (SP_IS_GRADIENT(server)) {
                        SPGradient *grad = SP_GRADIENT(server)->getVector();
                        Inkscape::XML::Node * repr = grad->getRepr();
                        Inkscape::XML::Node * drepr = swatch->getRepr();
                        if (repr != drepr && grad != swatch) {
                            while (SPStop* olds = swatch->getFirstStop()) {
                                olds->deleteObject();
                            }
                            for (SPStop* news = grad->getVector()->getFirstStop(); news != NULL; news = news->getNextStop()) {
                                Inkscape::XML::Node* clone = news->getRepr()->duplicate(_currentDocument->getReprDoc());
                                swatch->appendChildRepr(clone);
                                Inkscape::GC::release(clone);
                            }
                            swatch->setSwatch();
                            if (!_noLink.get_active()) {
                                sp_item_set_gradient(it, sp_gradient_ensure_vector_normalized(swatch), SP_IS_RADIALGRADIENT(swatch) ? SP_GRADIENT_TYPE_RADIAL : SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_FILL);
                            }
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Set Gradient Swatch"));
                        }
                    }
                } else if (it->style->fill.isColor()) {
                    while (SPStop* olds = swatch->getFirstStop()) {
                        olds->deleteObject();
                    }
                    addStop(swatch->getRepr(), it->style->fill.value.color.toString(), SP_SCALE24_TO_FLOAT(it->style->fill_opacity.value), "0");
                    swatch->setSwatch();
                    if (!_noLink.get_active()) {
                        SPGradient* normalized = sp_gradient_ensure_vector_normalized(swatch);
                        sp_item_set_gradient(it, normalized, SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_FILL);
                    }
                    DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Set Color Swatch"));
                }
            }
        }
    }
}

void SwatchesPanel::SetSelectedStroke(SPGradient* swatch)
{
    if (_currentDesktop && _currentDocument) {
        SPItem* it = _currentDesktop->selection->singleItem();
        if (it) {
            if (it->style->stroke.isSet()) {
                if (it->style->stroke.isPaintserver()) {
                    SPPaintServer * server = it->style->getStrokePaintServer();
                    if (SP_IS_GRADIENT(server)) {
                        SPGradient *grad = SP_GRADIENT(server)->getVector();
                        Inkscape::XML::Node * repr = grad->getRepr();
                        Inkscape::XML::Node * drepr = swatch->getRepr();
                        if (repr != drepr && grad != swatch) {
                            while (SPStop* olds = swatch->getFirstStop()) {
                                olds->deleteObject();
                            }
                            for (SPStop* news = grad->getVector()->getFirstStop(); news != NULL; news = news->getNextStop()) {
                                Inkscape::XML::Node* clone = news->getRepr()->duplicate(_currentDocument->getReprDoc());
                                swatch->appendChildRepr(clone);
                                Inkscape::GC::release(clone);
                            }
                            swatch->setSwatch();
                            if (!_noLink.get_active()) {
                                sp_item_set_gradient(it, sp_gradient_ensure_vector_normalized(swatch), SP_IS_RADIALGRADIENT(swatch) ? SP_GRADIENT_TYPE_RADIAL : SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_STROKE);
                            }
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Set Gradient Swatch"));
                        }
                    }
                } else if (it->style->stroke.isColor()) {
                    while (SPStop* olds = swatch->getFirstStop()) {
                        olds->deleteObject();
                    }
                    addStop(swatch->getRepr(), it->style->stroke.value.color.toString(), SP_SCALE24_TO_FLOAT(it->style->stroke_opacity.value), "0");
                    swatch->setSwatch();
                    if (!_noLink.get_active()) {
                        SPGradient* normalized = sp_gradient_ensure_vector_normalized(swatch);
                        sp_item_set_gradient(it, normalized, SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_STROKE);
                    }
                    DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Set Color Swatch"));
                }
            }
        }
    }
}

void SwatchesPanel::MoveSwatchDown(SPGradient* swatch)
{
    if (_currentDocument && swatch) {
        SPObject* next = swatch->next;
        while (next && (!SP_IS_GRADIENT(next) || !(SP_GRADIENT(next)->isSwatch()))) {
            next = next->next;
        }
        if (next) {
            Inkscape::XML::Node* repr = swatch->getRepr();
            repr->parent()->changeOrder(repr, next->getRepr());
            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Move Swatch Right"));
        }
    }
}

void SwatchesPanel::MoveSwatchUp(SPGradient* swatch)
{
    if (_currentDocument && swatch) {
        SPObject* g = NULL;
        SPObject* next = swatch->parent->firstChild();
        while (next && next != swatch) {
            if (SP_IS_GRADIENT(next) && SP_GRADIENT(next)->isSwatch()) {
                g = next;
            }
            next = next->next;
        }
        if (g) {
            g = g->getPrev();
            Inkscape::XML::Node* repr = swatch->getRepr();
            repr->parent()->changeOrder(repr, g ? g->getRepr() : NULL);
            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Move Swatch Left"));
        }
    }
}

void SwatchesPanel::RemoveSwatch(SPGradient* swatch)
{
    if (_currentDocument) {
        if (swatch->isReferenced()) {
            Inkscape::XML::Node * repr = swatch->getRepr();
            repr->parent()->removeChild(repr);
            _currentDocument->getDefs()->getRepr()->appendChild(repr);
            SP_GRADIENT(_currentDocument->getObjectByRepr(repr))->setSwatch(false);
        } else {
            swatch->deleteObject(false);
        }
    }
}

void SwatchesPanel::_setSelectionSwatch(SPGradient* swatch, bool isStroke)
{
    if (_currentDocument) {
        if (swatch) {
            if (_noLink.get_active()) {
                if (swatch->isSolid()) {
                    ColorRGBA rgba(swatch->getFirstStop()->getEffectiveColor().toRGBA32(swatch->getFirstStop()->opacity));
                    sp_desktop_set_color(_currentDesktop, rgba, false, !isStroke);
                } else {
                    SPCSSAttr *css = sp_repr_css_attr_new();
                    sp_repr_css_set_property(css, isStroke ? "stroke-opacity" : "fill-opacity", "1.0");
                    
                    Inkscape::XML::Node * clone = swatch->getRepr()->duplicate(_currentDocument->getReprDoc());
                    _currentDocument->getDefs()->appendChildRepr(clone);
                    SPGradient* grad = SP_GRADIENT(_currentDocument->getObjectByRepr(clone));
                    Inkscape::GC::release(clone);
                    grad->setSwatch(false);
                    SPGradient* normalized = sp_gradient_ensure_vector_normalized(grad);
//                    for (GSList const * it = _currentDesktop->selection->itemList(); it != NULL; it = it->next) {
//                        sp_desktop_apply_css_recursive(SP_ITEM(it->data), css, true);
//                        sp_item_set_gradient(SP_ITEM(it->data), normalized, SP_IS_RADIALGRADIENT(normalized) ? SP_GRADIENT_TYPE_RADIAL : SP_GRADIENT_TYPE_LINEAR, isStroke ? Inkscape::FOR_STROKE : Inkscape::FOR_FILL);
//                    }
                    sp_desktop_set_style(_currentDesktop, css);
                    sp_desktop_set_gradient(_currentDesktop, normalized, !isStroke);
                    sp_repr_css_attr_unref (css);
                }
            } else {
                SPCSSAttr *css = sp_repr_css_attr_new();
                sp_repr_css_set_property(css, isStroke ? "stroke-opacity" : "fill-opacity", "1.0");

                SPGradient* normalized = sp_gradient_ensure_vector_normalized(swatch);
//                for (GSList const * it = _currentDesktop->selection->itemList(); it != NULL; it = it->next) {
//                    sp_desktop_apply_css_recursive(SP_ITEM(it->data), css, true);
//                    sp_item_set_gradient(SP_ITEM(it->data), normalized, SP_IS_RADIALGRADIENT(normalized) ? SP_GRADIENT_TYPE_RADIAL : SP_GRADIENT_TYPE_LINEAR, isStroke ? Inkscape::FOR_STROKE : Inkscape::FOR_FILL);
//                }
                sp_desktop_set_style(_currentDesktop, css);
                sp_desktop_set_gradient(_currentDesktop, normalized, !isStroke);
                sp_repr_css_attr_unref (css);
            }
        } else {
            SPCSSAttr *css = sp_repr_css_attr_new ();
            sp_repr_css_set_property (css, isStroke ? "stroke" : "fill", "none");
            sp_desktop_set_style(_currentDesktop, css);
        }
        if (isStroke) {
            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Set item stroke swatch"));
        } else {
            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Set item fill swatch"));
        }
    }
}

void SwatchesPanel::_swatchClicked(GdkEventButton* event, SPGradient* swatch)
{
    if (_currentDesktop) {
        if (event->button == 3) {
            Gtk::Menu * menu = Gtk::manage(new Gtk::Menu());
            
            Gtk::MenuItem* mi;
            
            Glib::ustring us = Glib::ustring::compose("<b>%1</b>", swatch ? (swatch->label() ? swatch->label() : swatch->getId()) : _("[None]"));
            mi = Gtk::manage(new Gtk::MenuItem(swatch ? (swatch->label() ? swatch->label() : swatch->getId()) : _("[None]")));
            Gtk::Label* namelbl = dynamic_cast<Gtk::Label*>(mi->get_child());
            if (namelbl) {
                namelbl->set_markup(us);
            }
            mi->show();
            mi->set_sensitive(false);
            menu->append(*mi);
            
            Gtk::SeparatorMenuItem* sep;
            sep = manage(new Gtk::SeparatorMenuItem());
            sep->show();
            menu->append(*sep);
                
            mi = Gtk::manage(new Gtk::MenuItem(_("Set Fill")));
            mi->signal_activate().connect_notify(sigc::bind<SPGradient*, bool>(sigc::mem_fun(*this, &SwatchesPanel::_setSelectionSwatch), swatch, false));
            mi->show();
            mi->set_sensitive(_currentDesktop && !_currentDesktop->selection->isEmpty());
            menu->append(*mi);

            mi = Gtk::manage(new Gtk::MenuItem(_("Set Stroke")));
            mi->signal_activate().connect_notify(sigc::bind<SPGradient*, bool>(sigc::mem_fun(*this, &SwatchesPanel::_setSelectionSwatch), swatch, true));
            mi->show();
            mi->set_sensitive(_currentDesktop && !_currentDesktop->selection->isEmpty());
            menu->append(*mi);
            
            if (swatch) {
                sep = manage(new Gtk::SeparatorMenuItem());
                sep->show();
                menu->append(*sep);

                mi = Gtk::manage(new Gtk::MenuItem(_("Set to Selected Fill")));
                mi->signal_activate().connect_notify(sigc::bind<SPGradient*>(sigc::mem_fun(*this, &SwatchesPanel::SetSelectedFill), swatch));
                mi->show();
                mi->set_sensitive(_currentDesktop && _currentDesktop->selection->singleItem());
                menu->append(*mi);

                mi = Gtk::manage(new Gtk::MenuItem(_("Set to Selected Stroke")));
                mi->signal_activate().connect_notify(sigc::bind<SPGradient*>(sigc::mem_fun(*this, &SwatchesPanel::SetSelectedStroke), swatch));
                mi->show();
                mi->set_sensitive(_currentDesktop && _currentDesktop->selection->singleItem());
                menu->append(*mi);


                sep = manage(new Gtk::SeparatorMenuItem());
                sep->show();
                menu->append(*sep);

                mi = Gtk::manage(new Gtk::MenuItem(_("Move Left")));
                mi->signal_activate().connect_notify(sigc::bind<SPGradient*>(sigc::mem_fun(*this, &SwatchesPanel::MoveSwatchUp), swatch));
                mi->show();
                menu->append(*mi);

                mi = Gtk::manage(new Gtk::MenuItem(_("Move Right")));
                mi->signal_activate().connect_notify(sigc::bind<SPGradient*>(sigc::mem_fun(*this, &SwatchesPanel::MoveSwatchDown), swatch));
                mi->show();
                menu->append(*mi);

                sep = manage(new Gtk::SeparatorMenuItem());
                sep->show();
                menu->append(*sep);

                mi = Gtk::manage(new Gtk::MenuItem(_("Remove")));
                mi->signal_activate().connect_notify(sigc::bind<SPGradient*>(sigc::mem_fun(*this, &SwatchesPanel::RemoveSwatch), swatch));
                mi->show();
                menu->append(*mi);
            }

            menu->popup(event->button, event->time);
        } else if (!_currentDesktop->selection->isEmpty()) {
            _setSelectionSwatch(swatch, event->state & GDK_SHIFT_MASK);
        }
    }
}

void SwatchesPanel::MoveDown(SPGroup* page)
{
    if (_currentDocument && page) {
        SPObject* next = page->next;
        while (next && !SP_IS_GROUP(next)) {
            next = next->next;
        }
        if (next) {
            Inkscape::XML::Node* repr = page->getRepr();
            repr->parent()->changeOrder(repr, next->getRepr());
            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Move Swatch Down"));
        }
    }
}

void SwatchesPanel::MoveUp(SPGroup* page)
{
    if (_currentDocument && page) {
        SPObject* g = NULL;
        SPObject* next = page->parent->firstChild();
        while (next && next != page) {
            if (SP_IS_GROUP(next)) {
                g = next;
            }
            next = next->next;
        }
        if (g) {
            g = g->getPrev();
            Inkscape::XML::Node* repr = page->getRepr();
            repr->parent()->changeOrder(repr, g ? g->getRepr() : NULL);
            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Move Swatch Up"));
        }
    }
}

void SwatchesPanel::Remove(SPGroup* page)
{
    if (_currentDocument && page) {
        std::vector<Inkscape::XML::Node*> toMove;
        for(SPObject* obj = page->firstChild(); obj != NULL; obj = obj->next) {
            if (SP_IS_GRADIENT(obj)) {
                SPGradient* grad = SP_GRADIENT(obj);
                if (grad->isReferenced()) {
                    toMove.push_back(grad->getRepr());
                }
            }
        }
        while (!toMove.empty()) {
            Inkscape::XML::Node * repr = toMove.back();
            toMove.pop_back();
            repr->parent()->removeChild(repr);
            _currentDocument->getDefs()->getRepr()->appendChild(repr);
            SP_GRADIENT(_currentDocument->getObjectByRepr(repr))->setSwatch(false);
        }
        page->deleteObject(false);
        DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Remove Swatch Group"));
    }
}

void SwatchesPanel::AddSelectedFill(SPGroup* page)
{
    if (_currentDesktop && _currentDocument) {
        SPItem* it = _currentDesktop->selection->singleItem();
        if (it) {
            if (it->style->fill.isSet()) {
                if (it->style->fill.isPaintserver()) {
                    SPPaintServer * server = it->style->getFillPaintServer();
                    if (SP_IS_GRADIENT(server)) {
                        SPGradient *grad = SP_GRADIENT(server)->getVector();
                        if (_noLink.get_active()) {
                            Inkscape::XML::Node * clone = grad->getRepr()->duplicate(_currentDocument->getReprDoc());
                            clone->setAttribute( "osb:paint", "gradient", 0 );
                            if (page) {
                                page->appendChildRepr(clone);
                            } else {
                                _currentDocument->getDefs()->appendChildRepr(clone);
                            }
                            Inkscape::GC::release(clone);
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Gradient Swatch"));                            
                        } else if (grad->isSwatch()) {
                            Inkscape::XML::Node * repr = grad->getRepr();
                            Inkscape::XML::Node * clone = repr->duplicate(_currentDocument->getReprDoc());
                            if (page) {
                                page->appendChildRepr(clone);
                            } else {
                                _currentDocument->getDefs()->appendChildRepr(clone);
                            }
                            SPGradient *newgrad = SP_GRADIENT(_currentDocument->getObjectByRepr(clone));
                            
                            sp_item_set_gradient(it, sp_gradient_ensure_vector_normalized(newgrad), SP_IS_RADIALGRADIENT(newgrad) ? SP_GRADIENT_TYPE_RADIAL : SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_FILL);
                            Inkscape::GC::release(clone);
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Gradient Swatch"));                            
                        } else {
                            Inkscape::XML::Node * repr = grad->getRepr();
                            Inkscape::XML::Node * drepr = page ? page->getRepr() : _currentDocument->getDefs()->getRepr();
                            if (repr->parent() != drepr) {
                                repr->parent()->removeChild(repr);
                                drepr->appendChild(repr);
                            }
                            SP_GRADIENT(_currentDocument->getObjectByRepr(repr))->setSwatch();
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Gradient Swatch"));
                        }
                    }
                } else if (it->style->fill.isColor()) {
                    Inkscape::XML::Node *grad = _currentDocument->getReprDoc()->createElement("svg:linearGradient");
                    grad->setAttribute( "osb:paint", "solid", 0 );
                    addStop(grad, it->style->fill.value.color.toString(), SP_SCALE24_TO_FLOAT(it->style->fill_opacity.value), "0");
                    if (page) {
                        page->appendChild(grad);
                    } else {
                        _currentDocument->getDefs()->appendChild(grad);
                    }
                    SPGradient* normalized = sp_gradient_ensure_vector_normalized(SP_GRADIENT(_currentDocument->getObjectByRepr(grad)));
                    Inkscape::GC::release(grad);
                    if (!_noLink.get_active()) {
                        sp_item_set_gradient(it, normalized, SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_FILL);
                    }
                    DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Color Swatch"));
                }
            }
        }
    }
}

void SwatchesPanel::AddSelectedStroke(SPGroup* page)
{
    if (_currentDesktop && _currentDocument) {
        SPItem* it = _currentDesktop->selection->singleItem();
        if (it) {
            if (it->style->stroke.isSet()) {
                if (it->style->stroke.isPaintserver()) {
                    SPPaintServer * server = it->style->getStrokePaintServer();
                    if (SP_IS_GRADIENT(server)) {
                        SPGradient *grad = SP_GRADIENT(server)->getVector();
                        if (_noLink.get_active()) {
                            Inkscape::XML::Node * clone = grad->getRepr()->duplicate(_currentDocument->getReprDoc());
                            clone->setAttribute( "osb:paint", "gradient", 0 );
                            if (page) {
                                page->appendChildRepr(clone);
                            } else {
                                _currentDocument->getDefs()->appendChildRepr(clone);
                            }
                            Inkscape::GC::release(clone);
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Gradient Swatch"));                            
                        } else if (grad->isSwatch()) {
                            Inkscape::XML::Node * repr = grad->getRepr();
                            Inkscape::XML::Node * clone = repr->duplicate(_currentDocument->getReprDoc());
                            if (page) {
                                page->appendChildRepr(clone);
                            } else {
                                _currentDocument->getDefs()->appendChildRepr(clone);
                            }
                            SPGradient *newgrad = SP_GRADIENT(_currentDocument->getObjectByRepr(clone));
                            
                            sp_item_set_gradient(it, sp_gradient_ensure_vector_normalized(newgrad), SP_IS_RADIALGRADIENT(newgrad) ? SP_GRADIENT_TYPE_RADIAL : SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_STROKE);
                            Inkscape::GC::release(clone);
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Gradient Swatch"));                            
                        } else {
                            Inkscape::XML::Node * repr = grad->getRepr();
                            Inkscape::XML::Node * drepr = page ? page->getRepr() : _currentDocument->getDefs()->getRepr();
                            if (repr->parent() != drepr) {
                                repr->parent()->removeChild(repr);
                                drepr->appendChild(repr);
                            }
                            SP_GRADIENT(_currentDocument->getObjectByRepr(repr))->setSwatch();
                            DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Gradient Swatch"));
                        }
                    }
                } else if (it->style->stroke.isColor()) {
                    Inkscape::XML::Node *grad = _currentDocument->getReprDoc()->createElement("svg:linearGradient");
                    grad->setAttribute( "osb:paint", "solid", 0 );
                    addStop(grad, it->style->stroke.value.color.toString(), SP_SCALE24_TO_FLOAT(it->style->stroke_opacity.value), "0");
                    if (page) {
                        page->appendChild(grad);
                    } else {
                        _currentDocument->getDefs()->appendChild(grad);
                    }
                    SPGradient* normalized = sp_gradient_ensure_vector_normalized(SP_GRADIENT(_currentDocument->getObjectByRepr(grad)));
                    Inkscape::GC::release(grad);
                    if (!_noLink.get_active()) {
                        sp_item_set_gradient(it, normalized, SP_GRADIENT_TYPE_LINEAR, Inkscape::FOR_STROKE);
                    }
                    DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Add Color Swatch"));
                }
            }
        }
    }
}

void SwatchesPanel::_addBIButtonClicked(GdkEventButton* event)
{
    if (popUpImportMenu) {
        popUpImportMenu->popup(event->button, event->time);
    }
}

void SwatchesPanel::NewGroupBI()
{
    if (_currentDocument) {
        Inkscape::XML::Node * page = SwatchDocument->getReprDoc()->createElement("svg:g");
        gchar *id=NULL;
        do {
            g_free(id);
            id = g_strdup_printf("page%d", page_suffix++);
        } while (SwatchDocument->getObjectById(id));

        page->setAttribute("id", id);

        SwatchDocument->getDefs()->appendChild(page);
        sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
    }
}

void SwatchesPanel::NewGroup()
{
    if (_currentDocument) {
        Inkscape::XML::Node * page = _currentDocument->getReprDoc()->createElement("svg:g");
        gchar *id=NULL;
        do {
            g_free(id);
            id = g_strdup_printf("page%d", page_suffix++);
        } while (_currentDocument->getObjectById(id));

        page->setAttribute("id", id);

        _currentDocument->getDefs()->appendChild(page);
        DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("New Swatch Group"));
    }
}

void SwatchesPanel::SaveAs()
{
    if (_currentDocument) {
        Inkscape::XML::Node * page = _currentDocument->getReprDoc()->createElement("svg:g");
        gchar *id=NULL;
        do {
            g_free(id);
            id = g_strdup_printf("page%d", page_suffix++);
        } while (_currentDocument->getObjectById(id));

        page->setAttribute("id", id);

        std::vector<Inkscape::XML::Node*> toMove;
        for (SPObject *it = _currentDocument->getDefs()->firstChild(); it != NULL; it = it->next) {
            if (SP_IS_GRADIENT(it)) {
                SPGradient* grad = SP_GRADIENT(it);
                if (grad->isSwatch()) {
                    toMove.push_back(grad->getRepr());
                }
            }
        }
        while (!toMove.empty()) {
            Inkscape::XML::Node* repr = toMove.back();
            toMove.pop_back();
            repr->parent()->removeChild(repr);
            page->appendChild(repr);
        }
        _currentDocument->getDefs()->appendChild(page);
        DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Save Swatch Group"));
    }
}

void SwatchesPanel::Duplicate(SPGroup* oldpage)
{
    if (_currentDocument) {
        Inkscape::XML::Node * page = _currentDocument->getReprDoc()->createElement("svg:g");
        gchar *id=NULL;
        do {
            g_free(id);
            id = g_strdup_printf("page%d", page_suffix++);
        } while (_currentDocument->getObjectById(id));

        page->setAttribute("id", id);
        if (oldpage->label()) page->setAttribute("inkscape:label", oldpage->label());

        for (SPObject *it = oldpage->firstChild(); it != NULL; it = it->next) {
            if (SP_IS_GRADIENT(it)) {
                SPGradient* grad = SP_GRADIENT(it);
                if (grad->isSwatch()) {
                    Inkscape::XML::Node * copy = grad->getRepr()->duplicate(_currentDocument->getReprDoc());
                    page->appendChild(copy);
                    Inkscape::GC::release(copy);
                }
            }
        }
        _currentDocument->getDefs()->appendChild(page);
        DocumentUndo::done(_currentDocument, SP_VERB_DIALOG_SWATCHES, _("Duplicate Swatch Group"));
    }
}

void SwatchesPanel::_lblClick(GdkEventButton* event, SPGroup* page)
{
    Gtk::Menu * menu = Gtk::manage(new Gtk::Menu());
    
    Gtk::MenuItem* mi;
    Glib::ustring us = Glib::ustring::compose("<b>%1</b>", page ? (page->label() ? page->label() : page->getId()) : _("[Base]"));
    mi = Gtk::manage(new Gtk::MenuItem(page ? (page->label() ? page->label() : page->getId()) : _("[Base]")));
    Gtk::Label* namelbl = dynamic_cast<Gtk::Label*>(mi->get_child());
    if (namelbl) {
        namelbl->set_markup(us);
    }
    mi->show();
    mi->set_sensitive(false);
    menu->append(*mi);
            
    Gtk::SeparatorMenuItem* sep;
    sep = manage(new Gtk::SeparatorMenuItem());
    sep->show();
    menu->append(*sep);
    
    mi = Gtk::manage(new Gtk::MenuItem(_("Add Selected Fill")));
    mi->signal_activate().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::AddSelectedFill), page));
    mi->show();
    mi->set_sensitive(_currentDesktop && _currentDesktop->selection->singleItem());
    menu->append(*mi);
    
    mi = Gtk::manage(new Gtk::MenuItem(_("Add Selected Stroke")));
    mi->signal_activate().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::AddSelectedStroke), page));
    mi->show();
    mi->set_sensitive(_currentDesktop && _currentDesktop->selection->singleItem());
    menu->append(*mi);

    sep = manage(new Gtk::SeparatorMenuItem());
    sep->show();
    menu->append(*sep);
    
    if (page) {

        mi = Gtk::manage(new Gtk::MenuItem(_("Move Up")));
        mi->signal_activate().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::MoveUp), page));
        mi->show();
        menu->append(*mi);

        mi = Gtk::manage(new Gtk::MenuItem(_("Move Down")));
        mi->signal_activate().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::MoveDown), page));
        mi->show();
        menu->append(*mi);

        sep = manage(new Gtk::SeparatorMenuItem());
        sep->show();
        menu->append(*sep);
        
        mi = Gtk::manage(new Gtk::MenuItem(_("Duplicate")));
        mi->signal_activate().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::Duplicate), page));
        mi->show();
        menu->append(*mi);
        
        sep = manage(new Gtk::SeparatorMenuItem());
        sep->show();
        menu->append(*sep);

        mi = Gtk::manage(new Gtk::MenuItem(_("Remove")));
        mi->signal_activate().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::Remove), page));
        mi->show();
        menu->append(*mi);
    } else {
        mi = Gtk::manage(new Gtk::MenuItem(_("Create New Group")));
        mi->signal_activate().connect_notify(sigc::mem_fun(*this, &SwatchesPanel::NewGroup));
        mi->show();
        menu->append(*mi);

        mi = Gtk::manage(new Gtk::MenuItem(_("Save As New Group")));
        mi->signal_activate().connect_notify(sigc::mem_fun(*this, &SwatchesPanel::SaveAs));
        mi->show();
        menu->append(*mi);
    }
    
    menu->popup(event->button, event->time);
}

void SwatchesPanel::_defsChanged()
{
    if (_storeDoc) {
        _storeDoc->clear();
    }
    
    while (!docWatchers.empty()) {
        Inkscape::XML::NodeObserver* w = docWatchers.back();
        docWatchers.pop_back();
        delete w;
    }
    
    std::vector<Gtk::Widget *> tableChildren = _insideTable.get_children();
    for (std::vector<Gtk::Widget *>::iterator c = tableChildren.begin(); c != tableChildren.end(); ++c) {
        _insideTable.remove(**c);
    }
    
    if (_currentDocument) {
        Gtk::EventBox* eb;
        Gtk::Label* lbl;
        if (_showlabels) {
            eb = Gtk::manage(new Gtk::EventBox());
            eb->add_events(Gdk::BUTTON_PRESS_MASK);
            eb->signal_button_press_event().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::_lblClick), NULL));

            lbl = Gtk::manage(new Gtk::Label(_("[Base]")));
            eb->add(*lbl);
            _insideTable.attach( *eb, 0, 1, 0, 1, Gtk::FILL/*|Gtk::EXPAND*/, Gtk::FILL/*|Gtk::EXPAND*/ , 5, 0);
        }

        ColorItem* item = Gtk::manage(new ColorItem(NULL, _("[None]"), _currentDesktop));
        item->signal_button_press_event().connect_notify(sigc::bind<SPGradient *>(sigc::mem_fun(*this, &SwatchesPanel::_swatchClicked), NULL));
        item->set_tooltip_text(_("[None]"));
        _insideTable.attach( *item, _showlabels ? 1 : 0, _showlabels ? 2 : 1, 0, 1, Gtk::FILL/*|Gtk::EXPAND*/, Gtk::FILL/*|Gtk::EXPAND*/ );
        
        unsigned int i = 1;
        for (SPObject *it = _currentDocument->getDefs()->firstChild(); it != NULL; it = it->next) {
            if (SP_IS_GRADIENT(it)) {
                SPGradient* grad = SP_GRADIENT(it);
                if (grad->hasStops()) {
                
                    GradientWatcher* w = new GradientWatcher(this, grad);
                    docWatchers.push_back(w);

                    if (grad->isSwatch()) {

                        for (SPStop* s = grad->getFirstStop(); s != NULL; s = s->getNextStop()) {
                            StopWatcher* sw = new StopWatcher(this, s);
                            docWatchers.push_back(sw);
                        }

                        if (_storeDoc) {
                            Gtk::TreeModel::iterator iter = _storeDoc->append();
                            Gtk::TreeModel::Row row = *iter;
                            row[_modelDoc->_colObject] = grad;
                            row[_modelDoc->_colLabel] = grad->label() ? grad->label() : grad->getId();
                            GdkPixbuf* pixb = sp_gradient_to_pixbuf (grad, 64, 18);
                            row[_modelDoc->_colPixbuf] = Glib::wrap(pixb);
                        }

                        item = Gtk::manage(new ColorItem(grad, it->label() ? it->label() : it->getId(), _currentDesktop));
                        item->signal_button_press_event().connect_notify(sigc::bind<SPGradient *>(sigc::mem_fun(*this, &SwatchesPanel::_swatchClicked), grad));
                        item->set_tooltip_text(it->label() ? it->label() : it->getId());
                        if (_showlabels) {
                            _insideTable.attach( *item, 1 + (i % 20), 2 + (i % 20), i / 20, i / 20 + 1, Gtk::FILL/*|Gtk::EXPAND*/, Gtk::FILL/*|Gtk::EXPAND*/ );
                        } else {
                            _insideTable.attach( *item, i, i + 1, 0, 1, Gtk::FILL/*|Gtk::EXPAND*/, Gtk::FILL/*|Gtk::EXPAND*/ );
                        }

                        i++;
                    }
                }
            }
        }
        
        for (SPObject *it = _currentDocument->getDefs()->firstChild(); it != NULL; it = it->next) {
            if (SP_IS_GROUP(it)) {
                SwatchWatcher* w = new SwatchWatcher(this, it, false);
                docWatchers.push_back(w);
                
                if (_showlabels) {
                    i += 20 - (i % 20);
                    eb = Gtk::manage(new Gtk::EventBox());
                    eb->add_events(Gdk::BUTTON_PRESS_MASK);
                    eb->signal_button_press_event().connect_notify(sigc::bind<SPGroup*>(sigc::mem_fun(*this, &SwatchesPanel::_lblClick), SP_GROUP(it)));
                    lbl = Gtk::manage(new Gtk::Label(it->label() ? it->label() : it->getId()));
                    eb->add(*lbl);
                    _insideTable.attach( *eb, 0, 1, i / 20, i / 20 + 1, Gtk::FILL/*|Gtk::EXPAND*/, Gtk::FILL/*|Gtk::EXPAND*/ , 5, 0);
                }
                
                Gtk::TreeModel::iterator rowiter;
                
                if (_storeDoc) {
                    rowiter = _storeDoc->append();
                    Gtk::TreeModel::Row row = *rowiter;
                    row[_modelDoc->_colObject] = it;
                    row[_modelDoc->_colLabel] = it->label() ? it->label() : it->getId();
                }
                
                for (SPObject *cit = it->firstChild(); cit != NULL; cit = cit->next) {
                    if (SP_IS_GRADIENT(cit)) {
                        SPGradient* grad = SP_GRADIENT(cit);
                        
                        if (grad->hasStops()) {
                        
                            GradientWatcher* w = new GradientWatcher(this, grad);
                            docWatchers.push_back(w);

                            if (grad->isSwatch()) {

                                for (SPStop* s = grad->getFirstStop(); s != NULL; s = s->getNextStop()) {
                                    StopWatcher* sw = new StopWatcher(this, s);
                                    docWatchers.push_back(sw);
                                }

                                if (_storeDoc && rowiter) {
                                    Gtk::TreeModel::iterator iter = _storeDoc->append(rowiter->children());
                                    Gtk::TreeModel::Row row = *iter;
                                    row[_modelDoc->_colObject] = grad;
                                    row[_modelDoc->_colLabel] = grad->label() ? grad->label() : grad->getId();
                                    GdkPixbuf* pixb = sp_gradient_to_pixbuf (grad, 64, 18);
                                    row[_modelDoc->_colPixbuf] = Glib::wrap(pixb);

                                    _editDoc.expand_to_path(_storeDoc->get_path(iter));
                                }

                                item = Gtk::manage(new ColorItem(grad, cit->label() ? cit->label() : cit->getId(), _currentDesktop));
                                item->signal_button_press_event().connect_notify(sigc::bind<SPGradient *>(sigc::mem_fun(*this, &SwatchesPanel::_swatchClicked), grad));
                                item->set_tooltip_text(cit->label() ? cit->label() : cit->getId());
                                if (_showlabels) {
                                    _insideTable.attach( *item, 1 + (i % 20), 2 + (i % 20), i / 20, i / 20 + 1, Gtk::FILL/*|Gtk::EXPAND*/, Gtk::FILL/*|Gtk::EXPAND*/ );
                                } else {
                                    _insideTable.attach( *item, i, i + 1, 0, 1, Gtk::FILL/*|Gtk::EXPAND*/, Gtk::FILL/*|Gtk::EXPAND*/ );
                                }
                                i++;
                            }
                        }
                    }
                }
            }
        }
    }
    //_scroller.resize(1,1);
    _insideTable.resize(1,1);
    _scroller.show_all_children();
    _scroller.queue_draw();

}

Gtk::MenuItem* SwatchesPanel::_addSwatchGroup(SPGroup* group, Gtk::TreeModel::Row* parentRow)
{
    Gtk::MenuItem* mi = manage(new Gtk::MenuItem(group->label() ? group->label() : group->getId(),1));
    Gtk::Menu* m = NULL;
    
    Gtk::TreeModel::Row* r;
    if (_store) {
        Gtk::TreeModel::iterator iter = parentRow ? _store->append(parentRow->children()) : _store->append();
        Gtk::TreeModel::Row row = *iter;
        row[_model->_colObject] = group;
        row[_model->_colLabel] = group->label() ? group->label() : group->getId();

        if (SP_IS_GROUP(group->parent) && SP_GROUP(group->parent)->expanded()) {
            _editBI.expand_to_path(_store->get_path(iter));
        }
        r = &row;
    } else {
        r = NULL;
    }
    
    bool hasswatches = false;
    for (SPObject * obj = group->firstChild(); obj != NULL; obj = obj->next)
    {
        if (SP_IS_GROUP(obj)) {
            if (!m) {
                m = manage(new Gtk::Menu());
                m->show_all();
                mi->set_submenu(*m);
                
                Gtk::MenuItem* basemi = manage(new Gtk::MenuItem(_("[All]")));
                basemi->show();
                Gtk::Label* namelbl = dynamic_cast<Gtk::Label*>(basemi->get_child());
                if (namelbl) {
                    namelbl->set_markup(_("<b>[All]</b>"));
                }
                basemi->signal_activate().connect(sigc::bind<SPGroup*, bool>(sigc::mem_fun(*this, &SwatchesPanel::_addSwatchButtonClicked), group, true));
                m->append(*basemi);
                
                Gtk::SeparatorMenuItem* sep = manage(new Gtk::SeparatorMenuItem());
                sep->show();
                m->append(*sep);
            }
            SwatchWatcher* w = new SwatchWatcher(this, obj, true);
            rootWatchers.push_back(w);
            m->append(*_addSwatchGroup(SP_GROUP(obj), r));
        } else if (SP_IS_GRADIENT(obj)) {
            hasswatches = true;
        }
    }
    if (!m) {
        mi->signal_activate().connect(sigc::bind<SPGroup*, bool>(sigc::mem_fun(*this, &SwatchesPanel::_addSwatchButtonClicked), group, false));
    } else if (hasswatches) {
        Glib::ustring us = Glib::ustring::compose("<b>%1</b>", group->label() ? group->label() : group->getId());
        Gtk::MenuItem* basemi = manage(new Gtk::MenuItem(group->label() ? group->label() : group->getId()));
        basemi->show();
        Gtk::Label* namelbl = dynamic_cast<Gtk::Label*>(basemi->get_child());
        if (namelbl) {
            namelbl->set_markup(us);
        }
        basemi->signal_activate().connect(sigc::bind<SPGroup*, bool>(sigc::mem_fun(*this, &SwatchesPanel::_addSwatchButtonClicked), group, false));
        m->prepend(*basemi);
    }
    mi->show();
    return mi;
}

void SwatchesPanel::_swatchesChanged()
{
    while (!rootWatchers.empty()) {
        Inkscape::XML::NodeObserver* w = rootWatchers.back();
        rootWatchers.pop_back();
        delete w;
    }

    std::vector<Gtk::Widget *> menuChildren = popUpMenu->get_children();
    for (std::vector<Gtk::Widget *>::iterator c = menuChildren.begin(); c != menuChildren.end(); ++c) {
        popUpMenu->remove(**c);
    }
    
    if (_store) {
        _store->clear();
    }
    
    Gtk::MenuItem* mi;
    
    for (SPObject * obj = SwatchDocument->getDefs()->firstChild(); obj != NULL; obj = obj->next)
    {
        if (SP_IS_GROUP(obj)) {
            SwatchWatcher* w = new SwatchWatcher(this, obj, true);
            rootWatchers.push_back(w);
            popUpMenu->append(*_addSwatchGroup(SP_GROUP(obj), NULL));
            
        }
    }
    Gtk::SeparatorMenuItem* sep = manage(new Gtk::SeparatorMenuItem());
    sep->show();
    popUpMenu->append(*sep);

    mi = manage(new Gtk::MenuItem(_("New Swatch Group..."),1));
    mi->signal_activate().connect(sigc::mem_fun(*this, &SwatchesPanel::NewGroup));
    mi->show();
    popUpMenu->append(*mi);
    
    mi = manage(new Gtk::MenuItem(_("Add Swatch File..."),1));
    mi->signal_activate().connect(sigc::bind<bool, bool>(sigc::mem_fun(*this, &SwatchesPanel::_importButtonClicked), true, false));
    mi->show();
    popUpMenu->append(*mi);
    
    mi = manage(new Gtk::MenuItem(_("Import Swatch File..."),1));
    mi->signal_activate().connect(sigc::bind<bool, bool>(sigc::mem_fun(*this, &SwatchesPanel::_importButtonClicked), true, true));
    mi->show();
    popUpMenu->append(*mi);
}

void SwatchesPanel::_styleButton( Gtk::Button& btn, SPDesktop *desktop, unsigned int code, char const* iconName, char const* fallback )
{
    bool set = false;

    if ( iconName ) {
        GtkWidget *child = sp_icon_new( Inkscape::ICON_SIZE_SMALL_TOOLBAR, iconName );
        gtk_widget_show( child );
        btn.add( *manage(Glib::wrap(child)) );
        btn.set_relief(Gtk::RELIEF_NONE);
        set = true;
    }

    if ( desktop ) {
        Verb *verb = Verb::get( code );
        if ( verb ) {
            SPAction *action = verb->get_action(desktop);
            if ( !set && action && action->image ) {
                GtkWidget *child = sp_icon_new( Inkscape::ICON_SIZE_SMALL_TOOLBAR, action->image );
                gtk_widget_show( child );
                btn.add( *manage(Glib::wrap(child)) );
                set = true;
            }
        }
    }
    
    btn.set_tooltip_text (fallback);

    if ( !set ) {
        btn.set_label( fallback );
    }
}

void SwatchesPanel::_addButtonClicked(GdkEventButton* event) {
    if ( (event->type == GDK_BUTTON_PRESS) && (event->button == 3 || event->button == 1) ) {
        if (popUpMenu) {
            popUpMenu->popup(event->button, event->time);
        }
    }
}

void SwatchesPanel::NoLinkToggled() {
    static Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring nolink = Glib::ustring::compose("%1/nolink", _prefs_path);
    prefs->setBool(nolink, _noLink.get_active());
}

bool SwatchesPanel::_handleButtonEvent(GdkEventButton *event)
{
    static unsigned doubleclick = 0;
    if ( (event->type == GDK_2BUTTON_PRESS) && (event->button == 1) ) {
        doubleclick = 1;
    }

    if ( event->type == GDK_BUTTON_RELEASE && doubleclick) {
        doubleclick = 0;
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col = 0;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if ( _editBI.get_path_at_pos( x, y, path, col, x2, y2 ) && col == _name_columnBI) {
            // Double click on the Layer name, enable editing
            _text_rendererBI->property_editable() = true;
            _editBI.set_cursor (path, *_name_columnBI, true);
            grab_focus();
        }
    }
   
    return false;
}

bool SwatchesPanel::_handleKeyEvent(GdkEventKey *event)
{
    switch (Inkscape::UI::Tools::get_group0_keyval(event)) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_F2: {
            Gtk::TreeModel::iterator iter = _editBI.get_selection()->get_selected();
            if (iter && !_text_rendererBI->property_editable()) {
                Gtk::TreeRow row = *iter;
                SPObject * obj = row[_model->_colObject];
                if (obj) {
                    Gtk::TreeModel::Path *path = new Gtk::TreeModel::Path(iter);
                    // Edit the layer label
                    _text_rendererBI->property_editable() = true;
                    _editBI.set_cursor(*path, *_name_columnBI, true);
                    grab_focus();
                    return true;
                }
            }
        }
        case GDK_KEY_Delete: {
            
            Gtk::TreeModel::iterator iter = _editBI.get_selection()->get_selected();
            if (iter) {
                Gtk::TreeRow row = *iter;
                SPObject * obj = row[_model->_colObject];
                if (obj) {
                    obj->deleteObject(false);
                    sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
                }
            }
            return true;
        }
        break;
    }
    return false;
}

void SwatchesPanel::_handleEdited(const Glib::ustring& path, const Glib::ustring& new_text)
{
    Gtk::TreeModel::iterator iter = _editBI.get_model()->get_iter(path);
    Gtk::TreeModel::Row row = *iter;

    _renameObject(row, new_text);
    _text_rendererBI->property_editable() = false;
}

void SwatchesPanel::_handleEditingCancelled()
{
    _text_rendererBI->property_editable() = false;
}

void SwatchesPanel::_renameObject(Gtk::TreeModel::Row row, const Glib::ustring& name)
{
    if ( row && SwatchDocument) {
        SPObject* obj = row[_model->_colObject];
        if ( obj ) {
            gchar const* oldLabel = obj->label();
            if ( !name.empty() && (!oldLabel || name != oldLabel) ) {
                obj->setLabel(name.c_str());
                sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
            }
        }
    }
}

void SwatchesPanel::_setCollapsed(SPGroup * group)
{
    group->setExpanded(false);
    group->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
    for (SPObject *iter = group->children; iter != NULL; iter = iter->next)
    {
        if (SP_IS_GROUP(iter)) _setCollapsed(SP_GROUP(iter));
    }
}

void SwatchesPanel::_setExpanded(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& /*path*/, bool isexpanded)
{
    Gtk::TreeModel::Row row = *iter;

    SPObject* obj = row[_model->_colObject];
    if (obj && SP_IS_GROUP(obj))
    {
        if (isexpanded)
        {
            SP_GROUP(obj)->setExpanded(isexpanded);
            obj->updateRepr(SP_OBJECT_WRITE_NO_CHILDREN | SP_OBJECT_WRITE_EXT);
        }
        else
        {
            _setCollapsed(SP_GROUP(obj));
        }
    }
    sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
}

void SwatchesPanel::_deleteButtonClicked()
{
    Gtk::TreeModel::iterator iter = _editBI.get_selection()->get_selected();
    if (iter) {
        Gtk::TreeModel::Row row = *iter;
        SPObject* obj = row[_model->_colObject];
        if (obj) {
            obj->deleteObject(false);
            sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
        }
    }
}

bool SwatchesPanel::_handleButtonEventDoc(GdkEventButton* event)
{
    static unsigned doubleclick = 0;

    if ( (event->type == GDK_BUTTON_PRESS) && (event->button == 3) ) {
        // TODO - fix to a better is-popup function
        Gtk::TreeModel::Path path;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        if ( _editDoc.get_path_at_pos( x, y, path ) ) {
            Gtk::TreeModel::Children::iterator iter = _editDoc.get_model()->get_iter(path);
            Gtk::TreeModel::Row row = *iter;

            SPObject* obj = row[_modelDoc->_colObject];
            if (SP_IS_GRADIENT(obj)) {
                _swatchClicked(event, SP_GRADIENT(obj));
            } else if (SP_IS_GROUP(obj)) {
                _lblClick(event, SP_GROUP(obj));
            } else {
                _lblClick(event, NULL);
            }
        } else {
            _lblClick(event, NULL);
        }
    }
    
    if ( (event->type == GDK_2BUTTON_PRESS) && (event->button == 1) ) {
        doubleclick = 1;
    }

    if ( event->type == GDK_BUTTON_RELEASE && doubleclick) {
        doubleclick = 0;
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col = 0;
        int x = static_cast<int>(event->x);
        int y = static_cast<int>(event->y);
        int x2 = 0;
        int y2 = 0;
        if ( _editDoc.get_path_at_pos( x, y, path, col, x2, y2 ) && col == _name_columnDoc) {
            // Double click on the Layer name, enable editing
            _text_rendererDoc->property_editable() = true;
            _editDoc.set_cursor (path, *_name_columnDoc, true);
            grab_focus();
        }
    }
   
    return false;
}

void SwatchesPanel::_deleteButtonClickedDoc()
{
    Gtk::TreeModel::iterator iter = _editDoc.get_selection()->get_selected();
    if (iter) {
        Gtk::TreeRow row = *iter;
        SPObject * obj = row[_modelDoc->_colObject];
        if (SP_IS_GRADIENT(obj)) {
            RemoveSwatch(SP_GRADIENT(obj));
        } else if (SP_IS_GROUP(obj)) {
            Remove(SP_GROUP(obj));
        }
    }
}

bool SwatchesPanel::_handleKeyEventDoc(GdkEventKey *event)
{
    switch (Inkscape::UI::Tools::get_group0_keyval(event)) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_F2: {
            Gtk::TreeModel::iterator iter = _editDoc.get_selection()->get_selected();
            if (iter && !_text_rendererDoc->property_editable()) {
                Gtk::TreeRow row = *iter;
                SPObject * obj = row[_modelDoc->_colObject];
                if (obj) {
                    Gtk::TreeModel::Path *path = new Gtk::TreeModel::Path(iter);
                    // Edit the layer label
                    _text_rendererDoc->property_editable() = true;
                    _editDoc.set_cursor(*path, *_name_columnDoc, true);
                    grab_focus();
                    return true;
                }
            }
        }
        case GDK_KEY_Delete: {
            
            _deleteButtonClickedDoc();
            return true;
        }
        break;
    }
    return false;
}

void SwatchesPanel::_renameObjectDoc(Gtk::TreeModel::Row row, const Glib::ustring& name)
{
    if ( row && SwatchDocument) {
        SPObject* obj = row[_modelDoc->_colObject];
        if ( obj ) {
            gchar const* oldLabel = obj->label();
            if ( !name.empty() && (!oldLabel || name != oldLabel) ) {
                obj->setLabel(name.c_str());
            }
        }
    }
}

void SwatchesPanel::_handleEditedDoc(const Glib::ustring& path, const Glib::ustring& new_text)
{
    Gtk::TreeModel::iterator iter = _editDoc.get_model()->get_iter(path);
    Gtk::TreeModel::Row row = *iter;

    _renameObjectDoc(row, new_text);
    _text_rendererDoc->property_editable() = false;
}

void SwatchesPanel::_handleEditingCancelledDoc()
{
    _text_rendererDoc->property_editable() = false;
}

/*
 * Drap and drop within the tree
 * Save the drag source and drop target SPObjects and if its a drag between layers or into (sublayer) a layer
 */
bool SwatchesPanel::_handleDragDrop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time)
{
    int cell_x = 0, cell_y = 0;
    Gtk::TreeModel::Path target_path;
    Gtk::TreeView::Column *target_column;
    
    bool _dnd_top = false;
    _dnd_into = false;
    _dnd_target = SwatchDocument->getDefs()->lastChild();
    Gtk::TreeModel::iterator itersel = _editBI.get_selection()->get_selected();
    if (!itersel) {
        return true;
    }
    Gtk::TreeModel::Row rowsel = *itersel;
    _dnd_source = rowsel[_model->_colObject];

    if (!_dnd_source) {
        return true;
    }
    
    if (_editBI.get_path_at_pos (x, y, target_path, target_column, cell_x, cell_y)) {
        // Are we before, inside or after the drop layer
        Gdk::Rectangle rect;
        _editBI.get_background_area (target_path, *target_column, rect);
        int cell_height = rect.get_height();
        _dnd_into = (cell_y > (int)(cell_height * 1/3) && cell_y <= (int)(cell_height * 2/3));
        if (cell_y < (int)(cell_height * 1/3)) {
            Gtk::TreeModel::Path next_path = target_path;
            if (next_path.prev()) {
                target_path = next_path;
            } else {
                // Dragging to the "end"
                Gtk::TreeModel::Path up_path = target_path;
                up_path.up();
                if (_store->iter_is_valid(_store->get_iter(up_path))) {
                    // Drop into parent
                    target_path = up_path;
                    _dnd_into = true;
                } else {
                    _dnd_top = true;
                    // Drop into the top level
                    _dnd_target = NULL;
                }
            }
        }
        if (!_dnd_top) {
            Gtk::TreeModel::iterator iter = _store->get_iter(target_path);
            if (_store->iter_is_valid(iter)) {
                Gtk::TreeModel::Row row = *iter;
                SPObject *obj = row[_model->_colObject];
                if (obj) {
                    _dnd_target = obj;
                }
            }
        }
    }
    
    if (_dnd_source != _dnd_target) {

        Inkscape::XML::Node *target_ref = _dnd_target ? _dnd_target->getRepr() : NULL;
        Inkscape::XML::Node *our_ref = _dnd_source->getRepr();

        if (target_ref != our_ref) {
            if (!target_ref) {
                target_ref = SwatchDocument->getDefs()->getRepr();
                if (target_ref != our_ref->parent()) {
                    our_ref->parent()->removeChild(our_ref);
                    target_ref->addChild(our_ref, NULL);
                } else {
                    our_ref->parent()->changeOrder(our_ref, NULL);
                }
            } else if (_dnd_into) {
                // Move this inside of the target at the end
                our_ref->parent()->removeChild(our_ref);
                target_ref->addChild(our_ref, NULL);
            } else if (target_ref->parent() != our_ref->parent()) {
                // Change in parent, need to remove and add
                our_ref->parent()->removeChild(our_ref);
                target_ref->parent()->addChild(our_ref, target_ref);
            } else {
                // Same parent, just move
                our_ref->parent()->changeOrder(our_ref, target_ref);
            }
        }
        sp_repr_save_file(SwatchDocument->getReprDoc(), SwatchFile, SP_SVG_NS_URI);
    }

    return true;
}

/*
 * Drap and drop within the tree
 * Save the drag source and drop target SPObjects and if its a drag between layers or into (sublayer) a layer
 */
bool SwatchesPanel::_handleDragDropDoc(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time)
{
    int cell_x = 0, cell_y = 0;
    Gtk::TreeModel::Path target_path;
    Gtk::TreeView::Column *target_column;
    
    bool _dnd_top = false;
    _dnd_into = false;
    _dnd_target = _currentDocument->getDefs()->lastChild();
    Gtk::TreeModel::iterator itersel = _editDoc.get_selection()->get_selected();
    if (!itersel) {
        return true;
    }
    Gtk::TreeModel::Row rowsel = *itersel;
    _dnd_source = rowsel[_modelDoc->_colObject];

    if (!_dnd_source) {
        return true;
    }
    
    if (_editDoc.get_path_at_pos (x, y, target_path, target_column, cell_x, cell_y)) {
        // Are we before, inside or after the drop layer
        Gdk::Rectangle rect;
        _editDoc.get_background_area (target_path, *target_column, rect);
        int cell_height = rect.get_height();
        _dnd_into = (cell_y > (int)(cell_height * 1/3) && cell_y <= (int)(cell_height * 2/3));
        if (cell_y < (int)(cell_height * 1/3)) {
            Gtk::TreeModel::Path next_path = target_path;
            if (next_path.prev()) {
                target_path = next_path;
            } else {
                // Dragging to the "end"
                Gtk::TreeModel::Path up_path = target_path;
                up_path.up();
                if (_storeDoc->iter_is_valid(_storeDoc->get_iter(up_path))) {
                    // Drop into parent
                    target_path = up_path;
                    _dnd_into = true;
                } else {
                    _dnd_top = true;
                    // Drop into the top level
                    _dnd_target = NULL;
                }
            }
        }
        if (!_dnd_top) {
            Gtk::TreeModel::iterator iter = _storeDoc->get_iter(target_path);
            if (_storeDoc->iter_is_valid(iter)) {
                Gtk::TreeModel::Row row = *iter;
                SPObject *obj = row[_modelDoc->_colObject];
                if (obj) {
                    _dnd_target = obj;
                    if (SP_IS_GRADIENT(obj)) {
                        _dnd_into = false;
                        if (SP_IS_GROUP(_dnd_source)) {
                            _dnd_target = obj->parent;
                        }
                    } else if (SP_IS_GROUP(_dnd_source)) {
                        _dnd_into = false;
                    }
                }
            }
        }
    }
    
    if (_dnd_source != _dnd_target) {

        Inkscape::XML::Node *target_ref = _dnd_target ? _dnd_target->getRepr() : NULL;
        Inkscape::XML::Node *our_ref = _dnd_source->getRepr();

        if (target_ref != our_ref) {
            if (!target_ref) {
                target_ref = _currentDocument->getDefs()->getRepr();
                if (target_ref != our_ref->parent()) {
                    our_ref->parent()->removeChild(our_ref);
                    target_ref->addChild(our_ref, NULL);
                } else {
                    our_ref->parent()->changeOrder(our_ref, NULL);
                }
            } else if (_dnd_into) {
                // Move this inside of the target at the end
                our_ref->parent()->removeChild(our_ref);
                target_ref->addChild(our_ref, NULL);
            } else if (target_ref->parent() != our_ref->parent()) {
                // Change in parent, need to remove and add
                our_ref->parent()->removeChild(our_ref);
                target_ref->parent()->addChild(our_ref, target_ref);
            } else {
                // Same parent, just move
                our_ref->parent()->changeOrder(our_ref, target_ref);
            }
        }
        DocumentUndo::done( _currentDocument , SP_VERB_DIALOG_SWATCHES,
                                                _("Moved Swatches"));
    }

    return true;
}


/**
 * Constructor
 */
SwatchesPanel::SwatchesPanel(gchar const* prefsPath, bool showLabels) :
    Inkscape::UI::Widget::Panel("", prefsPath, SP_VERB_DIALOG_SWATCHES, ""),
    _scroller(),
    _insideTable(1, 1),
    _insideV(),
    _insideH(),
    _buttonsRow(),
    _outsideV(),
    _noLink(_("Don't Link")),
    _showlabels(showLabels),
    _store(0),
    _scrollerBI(),
    _editBI(),
    _buttonsRowBI(),
    _editBIV(),
    _storeDoc(0),
    _scrollerDoc(),
    _editDoc(),
    _buttonsRowDoc(),
    _editDocV(),
    _notebook(),
    rootWatcher(0),
    docWatcher(0),
    _currentDesktop(0),
    _currentDocument(0)
{
    _insideH.pack_start(_insideTable, Gtk::PACK_SHRINK);
    if (_showlabels) {
        _insideV.pack_start(_insideH, Gtk::PACK_SHRINK);
        _scroller.add(_insideV);
    } else {
        _scroller.property_height_request() = 20;
        _scroller.add(_insideH);
    }
        
    popUpMenu = manage( new Gtk::Menu() );
    popUpMenu->show_all_children();

    Gtk::Button* btn = manage( new Gtk::Button() );
    _styleButton( *btn, SP_ACTIVE_DESKTOP, SP_VERB_LAYER_NEW, GTK_STOCK_ADD, _("Add Swatch") );
    btn->signal_button_press_event().connect_notify( sigc::mem_fun(*this, &SwatchesPanel::_addButtonClicked) );
    _buttonsRow.pack_start(*btn, Gtk::PACK_SHRINK);

//    btn = manage( new Gtk::Button() );
//    _styleButton( *btn, SP_ACTIVE_DESKTOP, SP_VERB_LAYER_NEW, GTK_STOCK_DISCONNECT, C_("Unlink", "New") );
//    _buttonsRow.pack_end(*btn, Gtk::PACK_SHRINK);

    static Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    Glib::ustring nolink = Glib::ustring::compose("%1/nolink", prefsPath);
    _noLink.set_tooltip_text(_("Don't link swatches"));
    _noLink.set_active(prefs->getBool(nolink, false));
    _noLink.signal_toggled().connect_notify(sigc::mem_fun(*this, &SwatchesPanel::NoLinkToggled));
    _buttonsRow.pack_end(_noLink, Gtk::PACK_SHRINK);

    _scroller.set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
    _scroller.set_shadow_type(Gtk::SHADOW_NONE);
    
    if (_showlabels) {
        _outsideV.pack_end(_buttonsRow, Gtk::PACK_SHRINK);
        _outsideV.pack_end(_scroller, Gtk::PACK_EXPAND_WIDGET);
        
        _notebook.append_page(_outsideV, "Use");
        
        {
            ModelColumnsDoc *zoop = new ModelColumnsDoc();
            _modelDoc = zoop;

            _storeDoc = Gtk::TreeStore::create( *zoop );

            _editDoc.set_model( _storeDoc );
            _editDoc.set_headers_visible(false);
            _editDoc.set_reorderable(true);
            _editDoc.enable_model_drag_dest (Gdk::ACTION_MOVE);

            Gtk::Button* btn = manage( new Gtk::Button() );
            _styleButton( *btn, SP_ACTIVE_DESKTOP, SP_VERB_LAYER_NEW, GTK_STOCK_ADD, _("Import Swatch") );
            btn->signal_button_press_event().connect_notify( sigc::mem_fun(*this, &SwatchesPanel::_addButtonClicked));
            _buttonsRowDoc.pack_start(*btn, Gtk::PACK_SHRINK);

            btn = manage( new Gtk::Button() );
            _styleButton( *btn, SP_ACTIVE_DESKTOP, SP_VERB_LAYER_DELETE, GTK_STOCK_DELETE, _("Delete Swatch") );
            btn->signal_button_press_event().connect_notify( sigc::hide(sigc::mem_fun(*this, &SwatchesPanel::_deleteButtonClickedDoc)));
            _buttonsRowDoc.pack_start(*btn, Gtk::PACK_SHRINK);

            _pixbuf_rendererDoc = manage(new Gtk::CellRendererPixbuf());
            int pixbufColNum = _editDoc.append_column("Pixbuf", *_pixbuf_rendererDoc) - 1;
            _pixbuf_columnDoc = _editDoc.get_column(pixbufColNum);
            _pixbuf_columnDoc->add_attribute(_pixbuf_rendererDoc->property_pixbuf(), _modelDoc->_colPixbuf);
            
            _text_rendererDoc = manage(new Gtk::CellRendererText());
            int nameColNum = _editDoc.append_column("Name", *_text_rendererDoc) - 1;
            _name_columnDoc = _editDoc.get_column(nameColNum);
            _name_columnDoc->add_attribute(_text_rendererDoc->property_text(), _modelDoc->_colLabel);

            _text_rendererDoc->signal_edited().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleEditedDoc) );
            _text_rendererDoc->signal_editing_canceled().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleEditingCancelledDoc) );

            _editDoc.set_expander_column( *_editDoc.get_column(nameColNum) );

            _editDoc.signal_button_press_event().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleButtonEventDoc), false );
            _editDoc.signal_button_release_event().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleButtonEventDoc), false );
            _editDoc.signal_key_press_event().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleKeyEventDoc), false );

            _editDoc.signal_drag_drop().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleDragDropDoc), false);

            _scrollerDoc.set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
            _scrollerDoc.set_shadow_type(Gtk::SHADOW_IN);
            _scrollerDoc.add(_editDoc);

            _editDocV.pack_start(_scrollerDoc, Gtk::PACK_EXPAND_WIDGET);
            _editDocV.pack_end(_buttonsRowDoc, Gtk::PACK_SHRINK);

            _notebook.append_page(_editDocV, "Edit");
        }
        
        {
            popUpImportMenu = Gtk::manage(new Gtk::Menu());
            
            Gtk::MenuItem* mi = Gtk::manage(new Gtk::MenuItem(_("New Swatch Group...")));
            mi->signal_activate().connect_notify(sigc::mem_fun(*this, &SwatchesPanel::NewGroupBI));
            mi->show();
            popUpImportMenu->append(*mi);
            
            mi = Gtk::manage(new Gtk::MenuItem(_("Import Swatch File...")));
            mi->signal_activate().connect_notify(sigc::bind<bool, bool>(sigc::mem_fun(*this, &SwatchesPanel::_importButtonClicked), false, true));
            mi->show();
            popUpImportMenu->append(*mi);
            
            ModelColumns *zoop = new ModelColumns();
            _model = zoop;

            _store = Gtk::TreeStore::create( *zoop );

            _editBI.set_model( _store );
            _editBI.set_headers_visible(false);
            _editBI.set_reorderable(true);
            _editBI.enable_model_drag_dest (Gdk::ACTION_MOVE);

            Gtk::Button* btn = manage( new Gtk::Button() );
            _styleButton( *btn, SP_ACTIVE_DESKTOP, SP_VERB_LAYER_NEW, GTK_STOCK_ADD, _("Import Swatch") );
            btn->signal_button_press_event().connect_notify(sigc::mem_fun(*this, &SwatchesPanel::_addBIButtonClicked));
            _buttonsRowBI.pack_start(*btn, Gtk::PACK_SHRINK);

            btn = manage( new Gtk::Button() );
            _styleButton( *btn, SP_ACTIVE_DESKTOP, SP_VERB_LAYER_DELETE, GTK_STOCK_DELETE, _("Delete Swatch") );
            btn->signal_button_press_event().connect_notify( sigc::hide(sigc::mem_fun(*this, &SwatchesPanel::_deleteButtonClicked)));
            _buttonsRowBI.pack_start(*btn, Gtk::PACK_SHRINK);

            _text_rendererBI = manage(new Gtk::CellRendererText());
            int nameColNum = _editBI.append_column("Name", *_text_rendererBI) - 1;
            _name_columnBI = _editBI.get_column(nameColNum);
            _name_columnBI->add_attribute(_text_rendererBI->property_text(), _model->_colLabel);

            _text_rendererBI->signal_edited().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleEdited) );
            _text_rendererBI->signal_editing_canceled().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleEditingCancelled) );

            _editBI.set_expander_column( *_editBI.get_column(nameColNum) );

            _editBI.signal_button_press_event().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleButtonEvent), false );
            _editBI.signal_button_release_event().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleButtonEvent), false );
            _editBI.signal_key_press_event().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleKeyEvent), false );

            _editBI.signal_drag_drop().connect( sigc::mem_fun(*this, &SwatchesPanel::_handleDragDrop), false);
            _editBI.signal_row_collapsed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &SwatchesPanel::_setExpanded), false));
            _editBI.signal_row_expanded().connect( sigc::bind<bool>(sigc::mem_fun(*this, &SwatchesPanel::_setExpanded), true));

            _scrollerBI.set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
            _scrollerBI.set_shadow_type(Gtk::SHADOW_IN);
            _scrollerBI.add(_editBI);

            _editBIV.pack_start(_scrollerBI, Gtk::PACK_EXPAND_WIDGET);
            _editBIV.pack_end(_buttonsRowBI, Gtk::PACK_SHRINK);

            _notebook.append_page(_editBIV, "Edit Built-In");
        }
        
        _getContents()->pack_end(_notebook, Gtk::PACK_EXPAND_WIDGET);
    } else {
        //_outsideV.pack_end(_scroller, Gtk::PACK_SHRINK);
        _buttonsRow.pack_end(_scroller, Gtk::PACK_EXPAND_WIDGET);
        //_getContents()->pack_end(_outsideV, Gtk::PACK_SHRINK);
        _getContents()->pack_end(_buttonsRow, Gtk::PACK_SHRINK);
    }
    
    loadPalletFile();
    
    rootWatcher = new SwatchWatcher(this, SwatchDocument->getDefs(), true);
    SwatchDocument->getDefs()->getRepr()->addObserver(*rootWatcher);
    _swatchesChanged();
    _insideTable.resize(1,1);
}

SwatchesPanel::~SwatchesPanel()
{
    if (rootWatcher) {
        rootWatcher->_repr->removeObserver(*rootWatcher);
        delete rootWatcher;
    }
    
    if (docWatcher) {
        docWatcher->_repr->removeObserver(*docWatcher);
        delete docWatcher;
    }

    _documentConnection.disconnect();
    _selChanged.disconnect();
}

void SwatchesPanel::setDesktop( SPDesktop* desktop )
{
    Inkscape::UI::Widget::Panel::setDesktop(desktop);
    if ( desktop != _currentDesktop ) {
        if ( _currentDesktop ) {
            _documentConnection.disconnect();
            _selChanged.disconnect();
        }

        _currentDesktop = desktop;

        if ( desktop ) {
            //_currentDesktop->selection->connectChanged(sigc::hide(sigc::mem_fun(*this, &SwatchesPanel::_updateFromSelection)));

            _documentConnection = desktop->connectDocumentReplaced(sigc::mem_fun(*this, &SwatchesPanel::_setDocument));

            _setDocument( desktop, desktop->doc() );
        } else {
            _setDocument(0, 0);
        }
    }
}

void SwatchesPanel::_setDocument( SPDesktop* desktop, SPDocument *document )
{
    if ( document != _currentDocument ) {
        if (docWatcher) {
            docWatcher->_repr->removeObserver(*docWatcher);
            delete docWatcher;
            docWatcher = NULL;
        }
        _currentDocument = document;
        
        if (_currentDocument) {
            docWatcher = new SwatchWatcher(this, _currentDocument->getDefs(), false);
            _currentDocument->getDefs()->getRepr()->addObserver(*docWatcher);
        }
        _defsChanged();
    }
}

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape

//really lazy!
void sp_desktop_set_gradient(SPDesktop *desktop, SPGradient* gradient, bool fill)
{
    
    bool intercepted = false;
    
    if (gradient->isSolid()) {
        ColorRGBA color(gradient->getFirstStop()->getEffectiveColor().toRGBA32(gradient->getFirstStop()->opacity));
        guint32 rgba = SP_RGBA32_F_COMPOSE(color[0], color[1], color[2], color[3]);
        gchar b[64];
        sp_svg_write_color(b, sizeof(b), rgba);
        SPCSSAttr *css = sp_repr_css_attr_new();
        if (fill) {
            sp_repr_css_set_property(css, "fill", b);
            Inkscape::CSSOStringStream osalpha;
            osalpha << color[3];
            sp_repr_css_set_property(css, "fill-opacity", osalpha.str().c_str());
        } else {
            sp_repr_css_set_property(css, "stroke", b);
            Inkscape::CSSOStringStream osalpha;
            osalpha << color[3];
            sp_repr_css_set_property(css, "stroke-opacity", osalpha.str().c_str());
        }
        intercepted = desktop->_set_style_signal.emit(css);
    }
    
    if (!intercepted) {
        for (const GSList *it = desktop->selection->itemList(); it != NULL; it = it->next) {
            sp_item_set_gradient(SP_ITEM(it->data), gradient, SP_IS_RADIALGRADIENT(gradient) ? SP_GRADIENT_TYPE_RADIAL : SP_GRADIENT_TYPE_LINEAR, !fill ? Inkscape::FOR_STROKE : Inkscape::FOR_FILL);
        }
    }
}

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
