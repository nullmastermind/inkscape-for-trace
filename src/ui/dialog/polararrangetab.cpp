/**
 * @brief Arranges Objects into a Circle/Ellipse
 */
/* Authors:
 *   Declara Denis
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <2geom/transforms.h>
#include <glibmm/i18n.h>

#include "ui/dialog/polararrangetab.h"
#include "ui/dialog/tile.h"

#include "verbs.h"
#include "preferences.h"
#include "inkscape.h"
#include "desktop-handles.h"
#include "selection.h"
#include "document.h"
#include "document-undo.h"
#include "sp-item.h"
#include "widgets/icon.h"
#include "desktop.h"
#include "sp-item-transform.h"

namespace Inkscape {
namespace UI {
namespace Dialog {

PolarArrangeTab::PolarArrangeTab(ArrangeDialog *parent_)
	: parent(parent_),
	  parametersTable(3, 3, false),
	  centerY("", "Y coordinate of the center", UNIT_TYPE_LINEAR),
	  centerX("", "X coordinate of the center", centerY),
	  radiusY("", "Y coordinate of the radius", UNIT_TYPE_LINEAR),
	  radiusX("", "X coordinate of the radius", radiusY),
	  angleY("", "Starting angle", UNIT_TYPE_RADIAL),
	  angleX("", "End angle", angleY)
{
	anchorPointLabel.set_text(C_("Polar arrange tab", "Anchor point:"));
	anchorPointLabel.set_alignment(Gtk::ALIGN_START);
	pack_start(anchorPointLabel, false, false);

	anchorBoundingBoxRadio.set_label(C_("Polar arrange tab", "Object's bounding box:"));
	anchorRadioGroup = anchorBoundingBoxRadio.get_group();
	anchorBoundingBoxRadio.signal_toggled().connect(sigc::mem_fun(*this, &PolarArrangeTab::on_anchor_radio_changed));
	pack_start(anchorBoundingBoxRadio, false, false);

	pack_start(anchorSelector, false, false);

	anchorObjectPivotRadio.set_label(C_("Polar arrange tab", "Object's rotational center"));
	anchorObjectPivotRadio.set_group(anchorRadioGroup);
	anchorObjectPivotRadio.signal_toggled().connect(sigc::mem_fun(*this, &PolarArrangeTab::on_anchor_radio_changed));
	pack_start(anchorObjectPivotRadio, false, false);

	arrangeOnLabel.set_text(C_("Polar arrange tab", "Arrange on:"));
	arrangeOnLabel.set_alignment(Gtk::ALIGN_START);
	pack_start(arrangeOnLabel, false, false);

	arrangeOnCircleRadio.set_label(C_("Polar arrange tab", "Last selected circle/ellipse/arc"));
	arrangeRadioGroup = arrangeOnCircleRadio.get_group();
	arrangeOnCircleRadio.signal_toggled().connect(sigc::mem_fun(*this, &PolarArrangeTab::on_arrange_radio_changed));
	pack_start(arrangeOnCircleRadio, false, false);

	arrangeOnParametersRadio.set_label(C_("Polar arrange tab", "Parameterized:"));
	arrangeOnParametersRadio.set_group(arrangeRadioGroup);
	arrangeOnParametersRadio.signal_toggled().connect(sigc::mem_fun(*this, &PolarArrangeTab::on_arrange_radio_changed));
	pack_start(arrangeOnParametersRadio, false, false);

	//FIXME: Objects in grid do not line up properly!
	centerLabel.set_text(_("Center X/Y:"));
	parametersTable.attach(centerLabel, 0, 1, 0, 1);
	centerX.setDigits(2);
	centerX.set_size_request(60, -1);
	centerX.setIncrements(0.2, 0);
	centerX.setRange(-10000, 10000);
	centerX.setValue(0, "px");
	centerY.setDigits(2);
	centerY.set_size_request(120, -1);
	centerY.setIncrements(0.2, 0);
	centerY.setRange(-10000, 10000);
	centerY.setValue(0, "px");
	parametersTable.attach(centerX, 1, 2, 0, 1);
	parametersTable.attach(centerY, 2, 3, 0, 1);

	radiusLabel.set_text(_("Radius X/Y:"));
	parametersTable.attach(radiusLabel, 0, 1, 1, 2);
	radiusX.setDigits(2);
	radiusX.set_size_request(60, -1);
	radiusX.setIncrements(0.2, 0);
	radiusX.setRange(-10000, 10000);
	radiusX.setValue(0, "px");
	radiusY.setDigits(2);
	radiusY.set_size_request(120, -1);
	radiusY.setIncrements(0.2, 0);
	radiusY.setRange(-10000, 10000);
	radiusY.setValue(0, "px");
	parametersTable.attach(radiusX, 1, 2, 1, 2);
	parametersTable.attach(radiusY, 2, 3, 1, 2);

	angleLabel.set_text(_("Center X/Y:"));
	parametersTable.attach(angleLabel, 0, 1, 2, 3);
	angleX.setDigits(2);
	angleX.set_size_request(60, -1);
	angleX.setIncrements(0.2, 0);
	angleX.setRange(-10000, 10000);
	angleX.setValue(0, "°");
	angleY.setDigits(2);
	angleY.set_size_request(120, -1);
	angleY.setIncrements(0.2, 0);
	angleY.setRange(-10000, 10000);
	angleY.setValue(0, "°");
	parametersTable.attach(angleX, 1, 2, 2, 3);
	parametersTable.attach(angleY, 2, 3, 2, 3);
	pack_start(parametersTable, false, false);

	rotateObjectsCheckBox.set_label(_("Rotate objects"));
	rotateObjectsCheckBox.set_active(true);
	pack_start(rotateObjectsCheckBox, false, false);

	centerX.set_sensitive(false);
	centerY.set_sensitive(false);
	angleX.set_sensitive(false);
	angleY.set_sensitive(false);
	radiusX.set_sensitive(false);
	radiusY.set_sensitive(false);
}

void PolarArrangeTab::arrange()
{
	std::cout << "PolarArrangeTab::arrange()" << std::endl;
	Inkscape::Selection *selection = sp_desktop_selection(parent->getDesktop());
	const GSList *items = selection->itemList();
	int i = 0;
	while(items)
	{
		SPItem *item = SP_ITEM(items->data);

		float centerx = 1000;
		float centery = 2000;

		float radiusx = 1000;
		float radiusy = 2000;

		float objectx = - item->documentVisualBounds()->min()[Geom::X];
		float objecty =   item->documentVisualBounds()->min()[Geom::Y];

		float angle = M_PI / 36 * i;

		float r = (radiusx * radiusy) /
				sqrtf(powf(radiusy * cos(angle), 2) + powf(radiusx * sin(angle), 2));
		float calcx = cos(angle) * r;
		float calcy = sin(angle) * r;

        sp_item_move_rel(item, Geom::Translate(objectx + calcx, objecty + calcy));
        sp_item_rotate_rel(item, Geom::Rotate(angle));

		//item->set_i2d_affine(item->i2dt_affine() * toOrigin * rotation);
        //item->doWriteTransform(item->getRepr(), item->transform,  NULL);

		items = items->next;
		++i;
	}
}

void PolarArrangeTab::updateSelection()
{
}

void PolarArrangeTab::on_arrange_radio_changed()
{
	bool arrangeParametric = !arrangeOnCircleRadio.get_active();

	centerX.set_sensitive(arrangeParametric);
	centerY.set_sensitive(arrangeParametric);

	angleX.set_sensitive(arrangeParametric);
	angleY.set_sensitive(arrangeParametric);

	radiusX.set_sensitive(arrangeParametric);
	radiusY.set_sensitive(arrangeParametric);
}

void PolarArrangeTab::on_anchor_radio_changed()
{
	bool anchorBoundingBox = anchorBoundingBoxRadio.get_active();

	anchorSelector.set_sensitive(anchorBoundingBox);
}

} //namespace Dialog
} //namespace UI
} //namespace Inkscape

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
