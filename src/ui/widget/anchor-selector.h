/*
 * anchor-selector.h
 *
 *  Created on: Mar 22, 2012
 *      Author: denis
 */

#ifndef ANCHOR_SELECTOR_H_
#define ANCHOR_SELECTOR_H_

#include <gtkmm.h>

class AnchorSelector : public Gtk::Alignment
{
private:
	Gtk::ToggleButton  _buttons[9];
	int                _selection;
	Gtk::Table         _container;

	void setupButton(const Glib::ustring &icon, Gtk::ToggleButton &button);
	void btn_activated(int index);
	void btn_tl_activated();
	void btn_t_activated();
	void btn_tr_activated();
	void btn_l_activated();
	void btn_c_activated();
	void btn_r_activated();
	void btn_bl_activated();
	void btn_b_activated();
	void btn_br_activated();

public:
	AnchorSelector();
	virtual ~AnchorSelector();
};

#endif /* ANCHOR_SELECTOR_H_ */
