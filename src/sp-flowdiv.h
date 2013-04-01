#ifndef __SP_ITEM_FLOWDIV_H__
#define __SP_ITEM_FLOWDIV_H__

/*
 */

#include "sp-object.h"
#include "sp-item.h"

#define SP_TYPE_FLOWDIV            (sp_flowdiv_get_type ())
#define SP_FLOWDIV(obj) ((SPFlowdiv*)obj)
#define SP_IS_FLOWDIV(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowdiv)))

#define SP_TYPE_FLOWTSPAN            (sp_flowtspan_get_type ())
#define SP_FLOWTSPAN(obj) ((SPFlowtspan*)obj)
#define SP_IS_FLOWTSPAN(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowtspan)))

#define SP_TYPE_FLOWPARA            (sp_flowpara_get_type ())
#define SP_FLOWPARA(obj) ((SPFlowpara*)obj)
#define SP_IS_FLOWPARA(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowpara)))

#define SP_TYPE_FLOWLINE            (sp_flowline_get_type ())
#define SP_FLOWLINE(obj) ((SPFlowline*)obj)
#define SP_IS_FLOWLINE(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowline)))

#define SP_TYPE_FLOWREGIONBREAK            (sp_flowregionbreak_get_type ())
#define SP_FLOWREGIONBREAK(obj) ((SPFlowregionbreak*)obj)
#define SP_IS_FLOWREGIONBREAK(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowregionbreak)))

class CFlowdiv;
class CFlowtspan;
class CFlowpara;
class CFlowline;
class CFlowregionbreak;

// these 3 are derivatives of SPItem to get the automatic style handling
class SPFlowdiv : public SPItem {
public:
	CFlowdiv* cflowdiv;
};

class SPFlowtspan : public SPItem {
public:
	CFlowtspan* cflowtspan;
};

class SPFlowpara : public SPItem {
public:
	CFlowpara* cflowpara;
};

// these do not need any style
class SPFlowline : public SPObject {
public:
	CFlowline* cflowline;
};

class SPFlowregionbreak : public SPObject {
public:
	CFlowregionbreak* cflowregionbreak;
};

struct SPFlowdivClass {
	SPItemClass parent_class;
};

struct SPFlowtspanClass {
	SPItemClass parent_class;
};

struct SPFlowparaClass {
	SPItemClass parent_class;
};

struct SPFlowlineClass {
	SPObjectClass parent_class;
};

struct SPFlowregionbreakClass {
	SPObjectClass parent_class;
};


class CFlowdiv : public CItem {
public:
	CFlowdiv(SPFlowdiv* flowdiv);
	virtual ~CFlowdiv();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void release();
	virtual void update(SPCtx* ctx, guint flags);
	virtual void modified(unsigned int flags);

	virtual void set(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowdiv* spflowdiv;
};

class CFlowtspan : public CItem {
public:
	CFlowtspan(SPFlowtspan* flowtspan);
	virtual ~CFlowtspan();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void release();
	virtual void update(SPCtx* ctx, guint flags);
	virtual void modified(unsigned int flags);

	virtual void set(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowtspan* spflowtspan;
};

class CFlowpara : public CItem {
public:
	CFlowpara(SPFlowpara* flowpara);
	virtual ~CFlowpara();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void release();
	virtual void update(SPCtx* ctx, guint flags);
	virtual void modified(unsigned int flags);

	virtual void set(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowpara* spflowpara;
};

class CFlowline : public CObject {
public:
	CFlowline(SPFlowline* flowline);
	virtual ~CFlowline();

	virtual void release();
	virtual void modified(unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowline* spflowline;
};

class CFlowregionbreak : public CObject {
public:
	CFlowregionbreak(SPFlowregionbreak* flowregionbreak);
	virtual ~CFlowregionbreak();

	virtual void release();
	virtual void modified(unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowregionbreak* spflowregionbreak;
};


GType sp_flowdiv_get_type (void);
GType sp_flowtspan_get_type (void);
GType sp_flowpara_get_type (void);
GType sp_flowline_get_type (void);
GType sp_flowregionbreak_get_type (void);

#endif
