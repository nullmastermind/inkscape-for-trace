#ifndef __SP_ITEM_FLOWDIV_H__
#define __SP_ITEM_FLOWDIV_H__

/*
 */

#include "sp-object.h"
#include "sp-item.h"

#define SP_TYPE_FLOWDIV            (sp_flowdiv_get_type ())
#define SP_FLOWDIV(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FLOWDIV, SPFlowdiv))
#define SP_FLOWDIV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FLOWDIV, SPFlowdivClass))
#define SP_IS_FLOWDIV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FLOWDIV))
#define SP_IS_FLOWDIV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FLOWDIV))

#define SP_TYPE_FLOWTSPAN            (sp_flowtspan_get_type ())
#define SP_FLOWTSPAN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FLOWTSPAN, SPFlowtspan))
#define SP_FLOWTSPAN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FLOWTSPAN, SPFlowtspanClass))
#define SP_IS_FLOWTSPAN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FLOWTSPAN))
#define SP_IS_FLOWTSPAN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FLOWTSPAN))

#define SP_TYPE_FLOWPARA            (sp_flowpara_get_type ())
#define SP_FLOWPARA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FLOWPARA, SPFlowpara))
#define SP_FLOWPARA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FLOWPARA, SPFlowparaClass))
#define SP_IS_FLOWPARA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FLOWPARA))
#define SP_IS_FLOWPARA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FLOWPARA))

#define SP_TYPE_FLOWLINE            (sp_flowline_get_type ())
#define SP_FLOWLINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FLOWLINE, SPFlowline))
#define SP_FLOWLINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FLOWLINE, SPFlowlineClass))
#define SP_IS_FLOWLINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FLOWLINE))
#define SP_IS_FLOWLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FLOWLINE))

#define SP_TYPE_FLOWREGIONBREAK            (sp_flowregionbreak_get_type ())
#define SP_FLOWREGIONBREAK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_FLOWREGIONBREAK, SPFlowregionbreak))
#define SP_FLOWREGIONBREAK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_FLOWREGIONBREAK, SPFlowregionbreakClass))
#define SP_IS_FLOWREGIONBREAK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_FLOWREGIONBREAK))
#define SP_IS_FLOWREGIONBREAK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_FLOWREGIONBREAK))

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

	virtual void onBuild(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void onRelease();
	virtual void onUpdate(SPCtx* ctx, guint flags);
	virtual void onModified(unsigned int flags);

	virtual void onSet(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowdiv* spflowdiv;
};

class CFlowtspan : public CItem {
public:
	CFlowtspan(SPFlowtspan* flowtspan);
	virtual ~CFlowtspan();

	virtual void onBuild(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void onRelease();
	virtual void onUpdate(SPCtx* ctx, guint flags);
	virtual void onModified(unsigned int flags);

	virtual void onSet(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowtspan* spflowtspan;
};

class CFlowpara : public CItem {
public:
	CFlowpara(SPFlowpara* flowpara);
	virtual ~CFlowpara();

	virtual void onBuild(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void onRelease();
	virtual void onUpdate(SPCtx* ctx, guint flags);
	virtual void onModified(unsigned int flags);

	virtual void onSet(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowpara* spflowpara;
};

class CFlowline : public CObject {
public:
	CFlowline(SPFlowline* flowline);
	virtual ~CFlowline();

	virtual void onRelease();
	virtual void onModified(unsigned int flags);

	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowline* spflowline;
};

class CFlowregionbreak : public CObject {
public:
	CFlowregionbreak(SPFlowregionbreak* flowregionbreak);
	virtual ~CFlowregionbreak();

	virtual void onRelease();
	virtual void onModified(unsigned int flags);

	virtual Inkscape::XML::Node* onWrite(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);

protected:
	SPFlowregionbreak* spflowregionbreak;
};


GType sp_flowdiv_get_type (void);
GType sp_flowtspan_get_type (void);
GType sp_flowpara_get_type (void);
GType sp_flowline_get_type (void);
GType sp_flowregionbreak_get_type (void);

#endif
