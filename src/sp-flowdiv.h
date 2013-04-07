#ifndef __SP_ITEM_FLOWDIV_H__
#define __SP_ITEM_FLOWDIV_H__

/*
 */

#include "sp-object.h"
#include "sp-item.h"

#define SP_FLOWDIV(obj) ((SPFlowdiv*)obj)
#define SP_IS_FLOWDIV(obj) (dynamic_cast<const SPFlowdiv*>((SPObject*)obj))

#define SP_FLOWTSPAN(obj) ((SPFlowtspan*)obj)
#define SP_IS_FLOWTSPAN(obj) (dynamic_cast<const SPFlowtspan*>((SPObject*)obj))

#define SP_FLOWPARA(obj) ((SPFlowpara*)obj)
#define SP_IS_FLOWPARA(obj) (dynamic_cast<const SPFlowpara*>((SPObject*)obj))

#define SP_FLOWLINE(obj) ((SPFlowline*)obj)
#define SP_IS_FLOWLINE(obj) (dynamic_cast<const SPFlowline*>((SPObject*)obj))

#define SP_FLOWREGIONBREAK(obj) ((SPFlowregionbreak*)obj)
#define SP_IS_FLOWREGIONBREAK(obj) (dynamic_cast<const SPFlowregionbreak*>((SPObject*)obj))

// these 3 are derivatives of SPItem to get the automatic style handling
class SPFlowdiv : public SPItem {
public:
	SPFlowdiv();
	virtual ~SPFlowdiv();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void release();
	virtual void update(SPCtx* ctx, guint flags);
	virtual void modified(unsigned int flags);

	virtual void set(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
};

class SPFlowtspan : public SPItem {
public:
	SPFlowtspan();
	virtual ~SPFlowtspan();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void release();
	virtual void update(SPCtx* ctx, guint flags);
	virtual void modified(unsigned int flags);

	virtual void set(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
};

class SPFlowpara : public SPItem {
public:
	SPFlowpara();
	virtual ~SPFlowpara();

	virtual void build(SPDocument *document, Inkscape::XML::Node *repr);
	virtual void release();
	virtual void update(SPCtx* ctx, guint flags);
	virtual void modified(unsigned int flags);

	virtual void set(unsigned int key, gchar const* value);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
};

// these do not need any style
class SPFlowline : public SPObject {
public:
	SPFlowline();
	virtual ~SPFlowline();

	virtual void release();
	virtual void modified(unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
};

class SPFlowregionbreak : public SPObject {
public:
	SPFlowregionbreak();
	virtual ~SPFlowregionbreak();

	virtual void release();
	virtual void modified(unsigned int flags);

	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
};

#endif
