// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * TODO: insert short description here
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#include <3rdparty/libcroco/cr-parser.h>
#include "xml/node-event-vector.h"
#include "xml/repr.h"
#include "document.h"
#include "sp-style-elem.h"
#include "sp-root.h"
#include "attributes.h"
#include "style.h"

// For external style sheets
#include "io/resource.h"
#include <iostream>
#include <fstream>

// For font-rule
#include "libnrtype/FontFactory.h"

SPStyleElem::SPStyleElem() : SPObject() {
    media_set_all(this->media);
    this->is_css = false;
    this->style_sheet = nullptr;
}

SPStyleElem::~SPStyleElem() = default;

void SPStyleElem::set(SPAttr key, const gchar* value) {
    switch (key) {
        case SPAttr::TYPE: {
            if (!value) {
                /* TODO: `type' attribute is required.  Give error message as per
                   http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing. */
                is_css = false;
            } else {
                /* fixme: determine what whitespace is allowed.  Will probably need to ask on SVG
                  list; though the relevant RFC may give info on its lexer. */
                is_css = ( g_ascii_strncasecmp(value, "text/css", 8) == 0
                           && ( value[8] == '\0' ||
                                value[8] == ';'    ) );
            }
            break;
        }

#if 0 /* unfinished */
        case SPAttr::MEDIA: {
            parse_media(style_elem, value);
            break;
        }
#endif

        /* title is ignored. */
        default: {
            SPObject::set(key, value);
            break;
        }
    }
}

/**
 * Callback for changing the content of a <style> child text node
 */
static void content_changed_cb(Inkscape::XML::Node *, gchar const *, gchar const *, void *const data)
{
    SPObject *obj = reinterpret_cast<SPObject *>(data);
    g_assert(data != nullptr);
    obj->read_content();
    obj->document->getRoot()->emitModified(SP_OBJECT_MODIFIED_CASCADE);
}

static Inkscape::XML::NodeEventVector const textNodeEventVector = {
    nullptr,            // child_added
    nullptr,            // child_removed
    nullptr,            // attr_changed
    content_changed_cb, // content_changed
    nullptr,            // order_changed
};

/**
 * Callback for adding a text child to a <style> node
 */
static void child_add_cb(Inkscape::XML::Node *, Inkscape::XML::Node *child, Inkscape::XML::Node *, void *const data)
{
    SPObject *obj = reinterpret_cast<SPObject *>(data);
    g_assert(data != nullptr);

    if (child->type() == Inkscape::XML::NodeType::TEXT_NODE) {
        child->addListener(&textNodeEventVector, obj);
    }

    obj->read_content();
}

/**
 * Callback for removing a (text) child from a <style> node
 */
static void child_rm_cb(Inkscape::XML::Node *, Inkscape::XML::Node *child, Inkscape::XML::Node *, void *const data)
{
    SPObject *obj = reinterpret_cast<SPObject *>(data);

    child->removeListenerByData(obj);

    obj->read_content();
}

/**
 * Callback for rearranging text nodes inside a <style> node
 */
static void
child_order_changed_cb(Inkscape::XML::Node *, Inkscape::XML::Node *,
                       Inkscape::XML::Node *, Inkscape::XML::Node *,
                       void *const data)
{
    SPObject *obj = reinterpret_cast<SPObject *>(data);
    g_assert(data != nullptr);
    obj->read_content();
}

Inkscape::XML::Node* SPStyleElem::write(Inkscape::XML::Document* xml_doc, Inkscape::XML::Node* repr, guint flags) {
    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = xml_doc->createElement("svg:style");
    }

    if (flags & SP_OBJECT_WRITE_BUILD) {
        g_warning("nyi: Forming <style> content for SP_OBJECT_WRITE_BUILD.");
        /* fixme: Consider having the CRStyleSheet be a member of SPStyleElem, and then
           pretty-print to a string s, then repr->addChild(xml_doc->createTextNode(s), NULL). */
    }
    if (is_css) {
        repr->setAttribute("type", "text/css");
    }
    /* todo: media */

    SPObject::write(xml_doc, repr, flags);

    return repr;
}


/** Returns the concatenation of the content of the text children of the specified object. */
static Glib::ustring
concat_children(Inkscape::XML::Node const &repr)
{
    Glib::ustring ret;
    // effic: Initialising ret to a reasonable starting size could speed things up.
    for (Inkscape::XML::Node const *rch = repr.firstChild(); rch != nullptr; rch = rch->next()) {
        if ( rch->type() == Inkscape::XML::NodeType::TEXT_NODE ) {
            ret += rch->content();
        }
    }
    return ret;
}



/* Callbacks for SAC-style libcroco parser. */

enum StmtType { NO_STMT, FONT_FACE_STMT, NORMAL_RULESET_STMT };

/**
 * Helper class which owns the parser and tracks the current statement
 */
struct ParseTmp
{
private:
    static constexpr unsigned ParseTmp_magic = 0x23474397; // from /dev/urandom
    unsigned const magic = ParseTmp_magic;

public:
    CRParser *const parser;
    CRStyleSheet *const stylesheet;
    SPDocument *const document; // Need file location for '@import'

    // Current statement
    StmtType stmtType = NO_STMT;
    CRStatement *currStmt = nullptr;

    ParseTmp() = delete;
    ParseTmp(ParseTmp const &) = delete;
    ParseTmp(CRStyleSheet *const stylesheet, SPDocument *const document);
    ~ParseTmp() { cr_parser_destroy(parser); }

    /**
     * @pre a_handler->app_data points to a ParseTmp instance
     */
    static ParseTmp *cast(CRDocHandler *a_handler)
    {
        assert(a_handler && a_handler->app_data);
        auto self = static_cast<ParseTmp *>(a_handler->app_data);
        assert(self->magic == ParseTmp_magic);
        return self;
    }
};

static void
import_style_cb (CRDocHandler *a_handler,
                 GList *a_media_list,
                 CRString *a_uri,
                 CRString *a_uri_default_ns,
                 CRParsingLocation *a_location)
{
    /* a_uri_default_ns is set to NULL and is unused by libcroco */

    // Get document
    g_return_if_fail(a_handler && a_uri);
    auto &parse_tmp = *ParseTmp::cast(a_handler);

    SPDocument* document = parse_tmp.document;
    if (!document) {
        std::cerr << "import_style_cb: No document!" << std::endl;
        return;
    }
    if (!document->getDocumentURI()) {
        std::cerr << "import_style_cb: Document URI is NULL" << std::endl;
        return;
    }

    // Get file
    auto import_file =
        Inkscape::IO::Resource::get_filename (document->getDocumentURI(), a_uri->stryng->str);

    // Parse file
    CRStyleSheet *stylesheet = cr_stylesheet_new (nullptr);
    ParseTmp parse_new(stylesheet, document);
    CRStatus const parse_status =
        cr_parser_parse_file(parse_new.parser, reinterpret_cast<const guchar *>(import_file.c_str()), CR_UTF_8);
    if (parse_status == CR_OK) {
        g_assert(parse_tmp.stylesheet);
        g_assert(parse_tmp.stylesheet != stylesheet);
        stylesheet->origin = ORIGIN_AUTHOR;
        // Append import statement
        CRStatement *ruleset =
            cr_statement_new_at_import_rule(parse_tmp.stylesheet, cr_string_dup(a_uri), nullptr, stylesheet);
        parse_tmp.stylesheet->statements = cr_statement_append(parse_tmp.stylesheet->statements, ruleset);
    } else {
        std::cerr << "import_style_cb: Could not parse: " << import_file << std::endl;
        cr_stylesheet_destroy (stylesheet);
    }
};

static void
start_selector_cb(CRDocHandler *a_handler,
                  CRSelector *a_sel_list)
{
    g_return_if_fail(a_handler && a_sel_list);
    auto &parse_tmp = *ParseTmp::cast(a_handler);

    if ( (parse_tmp.currStmt != nullptr)
         || (parse_tmp.stmtType != NO_STMT) ) {
        g_warning("Expecting currStmt==NULL and stmtType==0 (NO_STMT) at start of ruleset, but found currStmt=%p, stmtType=%u",
                  static_cast<void *>(parse_tmp.currStmt), unsigned(parse_tmp.stmtType));
        // fixme: Check whether we need to unref currStmt if non-NULL.
    }
    CRStatement *ruleset = cr_statement_new_ruleset(parse_tmp.stylesheet, a_sel_list, nullptr, nullptr);
    g_return_if_fail(ruleset && ruleset->type == RULESET_STMT);
    parse_tmp.stmtType = NORMAL_RULESET_STMT;
    parse_tmp.currStmt = ruleset;
}

static void
end_selector_cb(CRDocHandler *a_handler,
                CRSelector *a_sel_list)
{
    g_return_if_fail(a_handler && a_sel_list);
    auto &parse_tmp = *ParseTmp::cast(a_handler);

    CRStatement *const ruleset = parse_tmp.currStmt;
    if (parse_tmp.stmtType == NORMAL_RULESET_STMT
        && ruleset
        && ruleset->type == RULESET_STMT
        && ruleset->kind.ruleset->sel_list == a_sel_list)
    {
        parse_tmp.stylesheet->statements = cr_statement_append(parse_tmp.stylesheet->statements,
                                                               ruleset);
    } else {
        g_warning("Found stmtType=%u, stmt=%p, stmt.type=%u, ruleset.sel_list=%p, a_sel_list=%p.",
                  unsigned(parse_tmp.stmtType),
                  ruleset,
                  unsigned(ruleset->type),
                  ruleset->kind.ruleset->sel_list,
                  a_sel_list);
    }
    parse_tmp.currStmt = nullptr;
    parse_tmp.stmtType = NO_STMT;
}

static void
start_font_face_cb(CRDocHandler *a_handler,
                   CRParsingLocation *)
{
    auto &parse_tmp = *ParseTmp::cast(a_handler);

    if (parse_tmp.stmtType != NO_STMT || parse_tmp.currStmt != nullptr) {
        g_warning("Expecting currStmt==NULL and stmtType==0 (NO_STMT) at start of @font-face, but found currStmt=%p, stmtType=%u",
                  static_cast<void *>(parse_tmp.currStmt), unsigned(parse_tmp.stmtType));
        // fixme: Check whether we need to unref currStmt if non-NULL.
    }
    CRStatement *font_face_rule = cr_statement_new_at_font_face_rule (parse_tmp.stylesheet, nullptr);
    g_return_if_fail(font_face_rule && font_face_rule->type == AT_FONT_FACE_RULE_STMT);
    parse_tmp.stmtType = FONT_FACE_STMT;
    parse_tmp.currStmt = font_face_rule;
}

static void
end_font_face_cb(CRDocHandler *a_handler)
{
    auto &parse_tmp = *ParseTmp::cast(a_handler);

    CRStatement *const font_face_rule = parse_tmp.currStmt;
    if (parse_tmp.stmtType == FONT_FACE_STMT
        && font_face_rule
        && font_face_rule->type == AT_FONT_FACE_RULE_STMT)
    {
        parse_tmp.stylesheet->statements = cr_statement_append(parse_tmp.stylesheet->statements,
                                                               font_face_rule);
    } else {
        g_warning("Found stmtType=%u, stmt=%p, stmt.type=%u.",
                  unsigned(parse_tmp.stmtType),
                  font_face_rule,
                  unsigned(font_face_rule->type));
    }

    std::cout << "end_font_face_cb: font face rule limited support." << std::endl;
    cr_declaration_dump (font_face_rule->kind.font_face_rule->decl_list, stdout, 2, TRUE);
    printf ("\n");

    // Get document
    SPDocument* document = parse_tmp.document;
    if (!document) {
        std::cerr << "end_font_face_cb: No document!" << std::endl;
        return;
    }
    if (!document->getDocumentURI()) {
        std::cerr << "end_font_face_cb: Document URI is NULL" << std::endl;
        return;
    }

    // Add ttf or otf fonts.
    CRDeclaration const *cur = nullptr;
    for (cur = font_face_rule->kind.font_face_rule->decl_list; cur; cur = cur->next) {
        if (cur->property &&
            cur->property->stryng &&
            cur->property->stryng->str &&
            strcmp(cur->property->stryng->str, "src") == 0 ) {

            if (cur->value &&
                cur->value->content.str &&
                cur->value->content.str->stryng &&
                cur->value->content.str->stryng->str) {

                Glib::ustring value = cur->value->content.str->stryng->str;

                if (value.rfind("ttf") == (value.length() - 3) ||
                    value.rfind("otf") == (value.length() - 3)) {

                    // Get file
                    Glib::ustring ttf_file =
                        Inkscape::IO::Resource::get_filename (document->getDocumentURI(), value);

                    if (!ttf_file.empty()) {
                        font_factory *factory = font_factory::Default();
                        factory->AddFontFile( ttf_file.c_str() );
                        std::cout << "end_font_face_cb: Added font: " << ttf_file << std::endl;

                        // FIX ME: Need to refresh font list.
                    } else {
                        std::cout << "end_font_face_cb: Failed to add: " << value << std::endl;
                    }
                }
            }
        }
    }

    parse_tmp.currStmt = nullptr;
    parse_tmp.stmtType = NO_STMT;

}

static void
property_cb(CRDocHandler *const a_handler,
            CRString *const a_name,
            CRTerm *const a_value, gboolean const a_important)
{
    // std::cout << "property_cb: Entrance: " << a_name->stryng->str << ": " << cr_term_to_string(a_value) << std::endl;
    g_return_if_fail(a_handler && a_name);
    auto &parse_tmp = *ParseTmp::cast(a_handler);

    CRStatement *const ruleset = parse_tmp.currStmt;
    g_return_if_fail(ruleset);

    CRDeclaration *const decl = cr_declaration_new (ruleset, cr_string_dup(a_name), a_value);
    g_return_if_fail(decl);
    decl->important = a_important;

    switch (parse_tmp.stmtType) {

        case NORMAL_RULESET_STMT: {
            g_return_if_fail (ruleset->type == RULESET_STMT);
            CRStatus const append_status = cr_statement_ruleset_append_decl (ruleset, decl);
            g_return_if_fail (append_status == CR_OK);
            break;
        }
        case FONT_FACE_STMT: {
            g_return_if_fail (ruleset->type == AT_FONT_FACE_RULE_STMT);
            CRDeclaration *new_decls = cr_declaration_append (ruleset->kind.font_face_rule->decl_list, decl);
            g_return_if_fail (new_decls);
            ruleset->kind.font_face_rule->decl_list = new_decls;
            break;
        }
        default:
            g_warning ("property_cb: Unhandled stmtType: %u", parse_tmp.stmtType);
            return;
    }
}

ParseTmp::ParseTmp(CRStyleSheet *const stylesheet, SPDocument *const document)
    : parser(cr_parser_new(nullptr))
    , stylesheet(stylesheet)
    , document(document)
{
    CRDocHandler *sac_handler = cr_doc_handler_new();
    sac_handler->app_data = this;
    sac_handler->import_style = import_style_cb;
    sac_handler->start_selector = start_selector_cb;
    sac_handler->end_selector = end_selector_cb;
    sac_handler->start_font_face = start_font_face_cb;
    sac_handler->end_font_face = end_font_face_cb;
    sac_handler->property = property_cb;
    cr_parser_set_sac_handler(parser, sac_handler);
    cr_doc_handler_unref(sac_handler);
}

void update_style_recursively( SPObject *object ) {
    if (object) {
        // std::cout << "update_style_recursively: "
        //           << (object->getId()?object->getId():"null") << std::endl;
        if (object->style) {
            object->style->readFromObject( object );
        }
        for (auto& child : object->children) {
            update_style_recursively( &child );
        }
    }
}

/**
 * Get the list of styles.
 * Currently only used for testing.
 */
std::vector<std::unique_ptr<SPStyle>> SPStyleElem::get_styles() const
{
    std::vector<std::unique_ptr<SPStyle>> styles;

    if (style_sheet) {
        auto count = cr_stylesheet_nr_rules(style_sheet);
        for (int x = 0; x < count; ++x) {
            CRStatement *statement = cr_stylesheet_statement_get_from_list(style_sheet, x);
            styles.emplace_back(new SPStyle(document));
            styles.back()->mergeStatement(statement);
        }
    }

    return styles;
}

/**
 * Remove `style_sheet` from the document style cascade and destroy it.
 * @post style_sheet is NULL
 */
static void clear_style_sheet(SPStyleElem &self)
{
    if (!self.style_sheet) {
        return;
    }

    auto *next = self.style_sheet->next;
    auto *cascade = self.document->getStyleCascade();
    auto *topsheet = cr_cascade_get_sheet(cascade, ORIGIN_AUTHOR);

    cr_stylesheet_unlink(self.style_sheet);

    if (topsheet == self.style_sheet) {
        // will unref style_sheet
        cr_cascade_set_sheet(cascade, next, ORIGIN_AUTHOR);
    } else if (!topsheet) {
        cr_stylesheet_unref(self.style_sheet);
    }

    self.style_sheet = nullptr;
}

void SPStyleElem::read_content() {
    // TODO On modification (observer callbacks), clearing and re-appending to
    // the cascade can change the positon of a stylesheet relative to other
    // sheets in the document. We need a better way to update a style sheet
    // which preserves the position.
    clear_style_sheet(*this);

    // First, create the style-sheet object and track it in this
    // element so that it can be edited. It'll be combined with
    // the document's style sheet later.
    style_sheet = cr_stylesheet_new (nullptr);

    ParseTmp parse_tmp(style_sheet, document);

    //XML Tree being used directly here while it shouldn't be.
    Glib::ustring const text = concat_children(*getRepr());
    if (!(text.find_first_not_of(" \t\r\n") != std::string::npos)) {
        return;
    }
    CRStatus const parse_status =
        cr_parser_parse_buf(parse_tmp.parser, reinterpret_cast<const guchar *>(text.c_str()), text.bytes(), CR_UTF_8);

    if (parse_status == CR_OK) {
        auto *cascade = document->getStyleCascade();
        auto *topsheet = cr_cascade_get_sheet(cascade, ORIGIN_AUTHOR);
        if (!topsheet) {
            // if the style is the first style sheet that we've seen, set the document's
            // first style sheet to this style and create a cascade object with it.
            cr_cascade_set_sheet(cascade, style_sheet, ORIGIN_AUTHOR);
        } else {
            // If not the first, then chain up this style_sheet
            cr_stylesheet_append_stylesheet(topsheet, style_sheet);
        }
    } else {
        cr_stylesheet_destroy (style_sheet);
        style_sheet = nullptr;
        if (parse_status != CR_PARSING_ERROR) {
            g_printerr("parsing error code=%u\n", unsigned(parse_status));
        }
    }

    // If style sheet has changed, we need to cascade the entire object tree, top down
    // Get root, read style, loop through children
    update_style_recursively( (SPObject *)document->getRoot() );
    // cr_stylesheet_dump (document->getStyleSheet(), stdout);
}

static Inkscape::XML::NodeEventVector const nodeEventVector = {
    child_add_cb,           // child_added
    child_rm_cb,            // child_removed
    nullptr,                // attr_changed
    nullptr,                // content_changed
    child_order_changed_cb, // order_changed
};

void SPStyleElem::build(SPDocument *document, Inkscape::XML::Node *repr) {
    read_content();

    readAttr(SPAttr::TYPE);
#if 0
    readAttr( "media" );
#endif

    repr->addListener(&nodeEventVector, this);
    for (Inkscape::XML::Node *child = repr->firstChild(); child != nullptr; child = child->next()) {
        if (child->type() == Inkscape::XML::NodeType::TEXT_NODE) {
            child->addListener(&textNodeEventVector, this);
        }
    }

    SPObject::build(document, repr);
}

void SPStyleElem::release() {
    getRepr()->removeListenerByData(this);
    for (Inkscape::XML::Node *child = getRepr()->firstChild(); child != nullptr; child = child->next()) {
        child->removeListenerByData(this);
    }

    clear_style_sheet(*this);

    SPObject::release();
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
