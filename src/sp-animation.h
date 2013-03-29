#ifndef __SP_ANIMATION_H__
#define __SP_ANIMATION_H__

/*
 * SVG <animate> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"



/* Animation base class */

#define SP_TYPE_ANIMATION (sp_animation_get_type ())
#define SP_ANIMATION(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_ANIMATION, SPAnimation))
#define SP_IS_ANIMATION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_ANIMATION))

class SPAnimation;
class SPAnimationClass;

class CAnimation;

class SPAnimation : public SPObject {
public:
	CAnimation* canimation;
};

struct SPAnimationClass {
	SPObjectClass parent_class;
};

class CAnimation : public CObject {
public:
	CAnimation(SPAnimation* animation);
	virtual ~CAnimation();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();

	virtual void onSet(unsigned int key, const gchar* value);

private:
	SPAnimation* spanimation;
};

GType sp_animation_get_type (void);

/* Interpolated animation base class */

#define SP_TYPE_IANIMATION (sp_ianimation_get_type ())
#define SP_IANIMATION(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_IANIMATION, SPIAnimation))
#define SP_IS_IANIMATION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_IANIMATION))

class SPIAnimation;
class SPIAnimationClass;

class CIAnimation;

class SPIAnimation : public SPAnimation {
public:
	CIAnimation* cianimation;
};

struct SPIAnimationClass {
	SPAnimationClass parent_class;
};

class CIAnimation : public CAnimation {
public:
	CIAnimation(SPIAnimation* animation);
	virtual ~CIAnimation();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();

	virtual void onSet(unsigned int key, const gchar* value);

private:
	SPIAnimation* spianimation;
};

GType sp_ianimation_get_type (void);

/* SVG <animate> */

#define SP_TYPE_ANIMATE (sp_animate_get_type ())
#define SP_ANIMATE(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_ANIMATE, SPAnimate))
#define SP_IS_ANIMATE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_ANIMATE))

class SPAnimate;
class SPAnimateClass;

class CAnimate;

class SPAnimate : public SPIAnimation {
public:
	CAnimate* canimate;
};

struct SPAnimateClass {
	SPIAnimationClass parent_class;
};

class CAnimate : public CIAnimation {
public:
	CAnimate(SPAnimate* animate);
	virtual ~CAnimate();

	virtual void onBuild(SPDocument* doc, Inkscape::XML::Node* repr);
	virtual void onRelease();

	virtual void onSet(unsigned int key, const gchar* value);

private:
	SPAnimate* spanimate;
};

GType sp_animate_get_type (void);



#endif
