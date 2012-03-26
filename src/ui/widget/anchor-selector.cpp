/*
 * anchor-selector.cpp
 *
 *  Created on: Mar 22, 2012
 *      Author: denis
 */

#include <iostream>
#include "widgets/icon.h"
#include "ui/icon-names.h"
#include "ui/widget/anchor-selector.h"

void AnchorSelector::setupButton(const Glib::ustring& icon, Gtk::ToggleButton& button) {
	Gtk::Widget*  buttonIcon = Gtk::manage(sp_icon_get_icon(icon, Inkscape::ICON_SIZE_LARGE_TOOLBAR));
	buttonIcon->show();

	button.set_relief(Gtk::RELIEF_NONE);
	button.show();
	button.add(*buttonIcon);
	button.set_can_focus(false);
}

AnchorSelector::AnchorSelector()
	: Gtk::Alignment(0.5, 0, 0, 0),
	  _container(3, 3, true)
{
	setupButton(INKSCAPE_ICON("boundingbox_top_left"),     _buttons[0]);
	setupButton(INKSCAPE_ICON("boundingbox_top"),          _buttons[1]);
	setupButton(INKSCAPE_ICON("boundingbox_top_right"),    _buttons[2]);
	setupButton(INKSCAPE_ICON("boundingbox_left"),         _buttons[3]);
	setupButton(INKSCAPE_ICON("boundingbox_center"),       _buttons[4]);
	setupButton(INKSCAPE_ICON("boundingbox_right"),        _buttons[5]);
	setupButton(INKSCAPE_ICON("boundingbox_bottom_left"),  _buttons[6]);
	setupButton(INKSCAPE_ICON("boundingbox_bottom"),       _buttons[7]);
	setupButton(INKSCAPE_ICON("boundingbox_bottom_right"), _buttons[8]);

	_buttons[0].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_tl_activated));
	_buttons[1].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_t_activated));
	_buttons[2].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_tr_activated));
	_buttons[3].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_l_activated));
	_buttons[4].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_c_activated));
	_buttons[5].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_r_activated));
	_buttons[6].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_bl_activated));
	_buttons[7].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_b_activated));
	_buttons[8].signal_clicked().connect(sigc::mem_fun(*this, &AnchorSelector::btn_br_activated));

	for(int i = 0; i < 9; ++i) {
		_container.attach(_buttons[i], i % 3, i % 3+1, i / 3, i / 3+1, Gtk::FILL, Gtk::FILL);
	}
	_selection = 4;
	_buttons[4].set_active();

	this->add(_container);
}

AnchorSelector::~AnchorSelector()
{
	// TODO Auto-generated destructor stub
}

void AnchorSelector::btn_activated(int index)
{
	if(_buttons[index].get_active())
	{
		std::cout << "btn_activated(" << index << ", old=" << _selection << ");" << std::endl;
		if(index != _selection)
		{
			_buttons[_selection].set_active(false);
			_buttons[index].set_active();
			_selection = index;
		} else {
			_buttons[index].set_active();
		}
	}
}

void AnchorSelector::btn_tl_activated()
{
	btn_activated(0);
}

void AnchorSelector::btn_t_activated()
{
	btn_activated(1);
}

void AnchorSelector::btn_tr_activated()
{
	btn_activated(2);
}

void AnchorSelector::btn_l_activated()
{
	btn_activated(3);
}

void AnchorSelector::btn_c_activated()
{
	btn_activated(4);
}

void AnchorSelector::btn_r_activated()
{
	btn_activated(5);
}

void AnchorSelector::btn_bl_activated()
{
	btn_activated(6);
}

void AnchorSelector::btn_b_activated()
{
	btn_activated(7);
}

void AnchorSelector::btn_br_activated()
{
	btn_activated(8);
}



