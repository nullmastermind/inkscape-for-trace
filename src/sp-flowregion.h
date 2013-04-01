#ifndef __SP_ITEM_FLOWREGION_H__
#define __SP_ITEM_FLOWREGION_H__

/*
 */

#include "sp-item.h"

#define SP_TYPE_FLOWREGION            (sp_flowregion_get_type ())
#define SP_FLOWREGION(obj) ((SPFlowregion*)obj)
#define SP_IS_FLOWREGION(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowregion)))

#define SP_TYPE_FLOWREGIONEXCLUDE            (sp_flowregionexclude_get_type ())
#define SP_FLOWREGIONEXCLUDE(obj) ((SPFlowregionExclude*)obj)
#define SP_IS_FLOWREGIONEXCLUDE(obj) (obj != NULL && static_cast<const SPObject*>(obj)->typeHierarchy.count(typeid(SPFlowregionExclude)))

class Path;
class Shape;
class flow_dest;
class FloatLigne;
class CFlowregion;
class CFlowregionExclude;

class SPFlowregion : public SPItem {
public:
	CFlowregion* cflowregion;

	std::vector<Shape*>     computed;
	
	void             UpdateComputed(void);
};

struct SPFlowregionClass {
	SPItemClass parent_class;
};

class CFlowregion : public CItem {
public:
	CFlowregion(SPFlowregion* flowregion);
	virtual ~CFlowregion();

	virtual void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
	virtual void remove_child(Inkscape::XML::Node *child);
	virtual void update(SPCtx *ctx, unsigned int flags);
	virtual void modified(guint flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual gchar *description();

protected:
	SPFlowregion* spflowregion;
};

GType sp_flowregion_get_type (void);

class SPFlowregionExclude : public SPItem {
public:
	CFlowregionExclude* cflowregionexclude;

	Shape            *computed;
	
	void             UpdateComputed(void);
};

struct SPFlowregionExcludeClass {
	SPItemClass parent_class;
};

class CFlowregionExclude : public CItem {
public:
	CFlowregionExclude(SPFlowregionExclude* flowregionexclude);
	virtual ~CFlowregionExclude();

	virtual void child_added(Inkscape::XML::Node* child, Inkscape::XML::Node* ref);
	virtual void remove_child(Inkscape::XML::Node *child);
	virtual void update(SPCtx *ctx, unsigned int flags);
	virtual void modified(guint flags);
	virtual Inkscape::XML::Node* write(Inkscape::XML::Document *xml_doc, Inkscape::XML::Node *repr, guint flags);
	virtual gchar *description();

protected:
	SPFlowregionExclude* spflowregionexclude;
};

GType sp_flowregionexclude_get_type (void);

#endif
