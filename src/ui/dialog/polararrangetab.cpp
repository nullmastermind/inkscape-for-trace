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
	parametersTable.attach(centerLabel, 0, 1, 0, 1, Gtk::FILL);
	centerX.setDigits(2);
	//centerX.set_size_request(60, -1);
	centerX.setIncrements(0.2, 0);
	centerX.setRange(-10000, 10000);
	centerX.setValue(0, "px");
	centerY.setDigits(2);
	//centerY.set_size_request(120, -1);
	centerY.setIncrements(0.2, 0);
	centerY.setRange(-10000, 10000);
	centerY.setValue(0, "px");
	parametersTable.attach(centerX, 1, 2, 0, 1, Gtk::FILL);
	parametersTable.attach(centerY, 2, 3, 0, 1, Gtk::FILL);

	radiusLabel.set_text(_("Radius X/Y:"));
	parametersTable.attach(radiusLabel, 0, 1, 1, 2, Gtk::FILL);
	radiusX.setDigits(2);
	//radiusX.set_size_request(60, -1);
	radiusX.setIncrements(0.2, 0);
	radiusX.setRange(0.001, 10000);
	radiusX.setValue(100, "px");
	radiusY.setDigits(2);
	//radiusY.set_size_request(120, -1);
	radiusY.setIncrements(0.2, 0);
	radiusY.setRange(0.001, 10000);
	radiusY.setValue(100, "px");
	parametersTable.attach(radiusX, 1, 2, 1, 2, Gtk::FILL);
	parametersTable.attach(radiusY, 2, 3, 1, 2, Gtk::FILL);

	angleLabel.set_text(_("Angle X/Y:"));
	parametersTable.attach(angleLabel, 0, 1, 2, 3, Gtk::FILL);
	angleX.setDigits(2);
	//angleX.set_size_request(60, -1);
	angleX.setIncrements(0.2, 0);
	angleX.setRange(-10000, 10000);
	angleX.setValue(0, "°");
	angleY.setDigits(2);
	//angleY.set_size_request(120, -1);
	angleY.setIncrements(0.2, 0);
	angleY.setRange(-10000, 10000);
	angleY.setValue(180, "°");
	parametersTable.attach(angleX, 1, 2, 2, 3, Gtk::FILL);
	parametersTable.attach(angleY, 2, 3, 2, 3, Gtk::FILL);
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

void rotateAround(SPItem *item, Geom::Point center, Geom::Rotate const &rotation)
{
	Geom::Translate const s(center);
	Geom::Affine affine = Geom::Affine(s).inverse() * Geom::Affine(rotation) * Geom::Affine(s);

	// Save old center
    center = item->getCenter();

	item->set_i2d_affine(item->i2dt_affine() * affine);
	item->doWriteTransform(item->getRepr(), item->transform);

	if(item->isCenterSet())
	{
		item->setCenter(center * affine);
		item->updateRepr();
	}
}

float calcAngle(float arcBegin, float arcEnd, int count, int n)
{
	float arcLength = arcEnd - arcBegin;
	if(abs(abs(arcLength) - 2*M_PI) > 0.0001) count--; // If not a complete circle, put an object also at the extremes of the arc;

	float angle = n / (float)count;
	// Normalize for arcLength:
	angle = angle * arcLength;
	angle += arcBegin;

	return angle;
}

Geom::Point calcPoint(float cx, float cy, float rx, float ry, float angle)
{
	// Parameters for radius equation
	float a = ry * cos(angle);
	float b = rx * sin(angle);

	float radius = (rx * ry) / sqrtf((a*a) + (b*b));

	return Geom::Point(cos(angle) * radius + cx, sin(angle) * radius + cy);
}

Geom::Point getAnchorPoint(int anchor, SPItem *item)
{
	Geom::Point source;

	Geom::OptRect bbox = item->documentVisualBounds();

	switch(anchor)
	{
		case 0: // Top    - Left
		case 3: // Middle - Left
		case 6: // Bottom - Left
			source[0] = bbox->min()[Geom::X];
			break;
		case 1: // Top    - Middle
		case 4: // Middle - Middle
		case 7: // Bottom - Middle
			source[0] = (bbox->min()[Geom::X] + bbox->max()[Geom::X]) / 2.0f;
			break;
		case 2: // Top    - Right
		case 5: // Middle - Right
		case 8: // Bottom - Right
			source[0] = bbox->max()[Geom::X];
			break;
	};

	switch(anchor)
	{
		case 0: // Top    - Left
		case 1: // Top    - Middle
		case 2: // Top    - Right
			source[1] = bbox->min()[Geom::Y];
			break;
		case 3: // Middle - Left
		case 4: // Middle - Middle
		case 5: // Middle - Right
			source[1] = (bbox->min()[Geom::Y] + bbox->max()[Geom::Y]) / 2.0f;
			break;
		case 6: // Bottom - Left
		case 7: // Bottom - Middle
		case 8: // Bottom - Right
			source[1] = bbox->max()[Geom::Y];
			break;
	};

	// If using center
	if(anchor == 9)
		source = item->getCenter();
	else
	{
		source[1] -= item->document->getHeight();
		source[1] *= -1;
	}

	return source;
}

void moveToPoint(int anchor, SPItem *item, Geom::Point p)
{
	sp_item_move_rel(item, Geom::Translate(p - getAnchorPoint(anchor, item)));
}

void PolarArrangeTab::arrange()
{
	std::cout << "PolarArrangeTab::arrange()" << std::endl;
	Inkscape::Selection *selection = sp_desktop_selection(parent->getDesktop());
	const GSList *items, *tmp;
	tmp = items = selection->itemList();

	int count = 0;
	while(tmp)
	{
		tmp = tmp->next;
		++count;
	}

	// Read options from UI
	float cx = centerX.getValue("px");
	float cy = centerY.getValue("px");
	float rx = radiusX.getValue("px");
	float ry = radiusY.getValue("px");
	float arcBeg = angleX.getValue("rad");
	float arcEnd = angleY.getValue("rad");

	int anchor = 9;
	if(anchorBoundingBoxRadio.get_active())
	{
		anchor = anchorSelector.getHorizontalAlignment() +
				anchorSelector.getVerticalAlignment() * 3;
	}

	tmp = items;
	int i = 0;
	while(tmp)
	{
		SPItem *item = SP_ITEM(tmp->data);

		float angle = calcAngle(arcBeg, arcEnd, count, i);
		Geom::Point newLocation = calcPoint(cx, cy, rx, ry, angle);

		moveToPoint(anchor, item, newLocation);

		if(rotateObjectsCheckBox.get_active())
			rotateAround(item, newLocation, Geom::Rotate(angle));

		tmp = tmp->next;
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
