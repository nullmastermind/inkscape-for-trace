/** \file
 * Desktop widget implementation.
 */
/* Authors:
 *   Jon A. Cruz <jon@joncruz.org>
 *
 * Copyright (C) 2010 Jon A. Cruz
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "widgets/desktop-widget.h"

#include "uxmanager.h"
#include "desktop.h"
#include "util/ege-tags.h"
#include "widgets/toolbox.h"

using std::vector;

class TrackItem
{
public:
    TrackItem() : 
        destroyConn(),
        boxes()
    {}

    sigc::connection destroyConn;
    std::vector<GtkWidget*> boxes;
};

static vector<SPDesktop*> desktops;
static vector<SPDesktopWidget*> dtws;
static std::map<SPDesktop*, TrackItem> trackedBoxes;


namespace {

void desktopDestructHandler(SPDesktop *desktop)
{
    std::map<SPDesktop*, TrackItem>::iterator it = trackedBoxes.find(desktop);
    if (it != trackedBoxes.end())
    {
        trackedBoxes.erase(it);
    }
}


// TODO unify this later:
static Glib::ustring getLayoutPrefPath( Inkscape::UI::View::View *view )
{
    Glib::ustring prefPath;

    if (reinterpret_cast<SPDesktop*>(view)->is_focusMode()) {
        prefPath = "/focus/";
    } else if (reinterpret_cast<SPDesktop*>(view)->is_fullscreen()) {
        prefPath = "/fullscreen/";
    } else {
        prefPath = "/window/";
    }

    return prefPath;
}

}

namespace Inkscape {
namespace UI {

UXManager* instance = 0;

class UXManagerImpl : public UXManager
{
public:
    UXManagerImpl();
    virtual ~UXManagerImpl();

    virtual void addTrack( SPDesktopWidget* dtw );
    virtual void delTrack( SPDesktopWidget* dtw );

    virtual void connectToDesktop( vector<GtkWidget *> const & toolboxes, SPDesktop *desktop );

    virtual gint getDefaultTask( SPDesktop *desktop );
    virtual void setTask(SPDesktop* dt, gint val);

    virtual bool isWidescreen() const;

private:
    bool _widescreen;
};

UXManager* UXManager::getInstance()
{
    if (!instance) {
        instance = new UXManagerImpl();
    }
    return instance;
}


UXManager::UXManager()
{
}

UXManager::~UXManager()
{
}

UXManagerImpl::UXManagerImpl() :
    _widescreen(false)
{
    ege::TagSet tags;
    tags.setLang("en");

    tags.addTag(ege::Tag("General"));
    tags.addTag(ege::Tag("Icons"));

    Glib::RefPtr<Gdk::Screen> defaultScreen = Gdk::Screen::get_default();
    if (defaultScreen) {
        int width = defaultScreen->get_width();
        int height = defaultScreen->get_height();
        gdouble aspect = static_cast<gdouble>(width) / static_cast<gdouble>(height);
        if (aspect > 1.65) {
            _widescreen = true;
        }
    }
}

UXManagerImpl::~UXManagerImpl()
{
}

bool UXManagerImpl::isWidescreen() const
{
    return _widescreen;
}

gint UXManagerImpl::getDefaultTask( SPDesktop *desktop )
{
    gint taskNum = isWidescreen() ? 2 : 0;

    Glib::ustring prefPath = getLayoutPrefPath( desktop );
    taskNum = Inkscape::Preferences::get()->getInt( prefPath + "task/taskset", taskNum );
    taskNum = (taskNum < 0) ? 0 : (taskNum > 2) ? 2 : taskNum;

    return taskNum;
}

void UXManagerImpl::setTask(SPDesktop* dt, gint val)
{
    for (vector<SPDesktopWidget*>::iterator it = dtws.begin(); it != dtws.end(); ++it) {
        SPDesktopWidget* dtw = *it;

        gboolean notDone = Inkscape::Preferences::get()->getBool("/options/workarounds/dynamicnotdone", false);

        if (dtw->desktop == dt) {
            int taskNum = val;
            switch (val) {
                default:
                case 0:
                    dtw->setToolboxPosition("ToolToolbar", GTK_POS_LEFT);
                    dtw->setToolboxPosition("CommandsToolbar", GTK_POS_TOP);
                    if (notDone) {
                        dtw->setToolboxPosition("AuxToolbar", GTK_POS_TOP);
                    }
                    dtw->setToolboxPosition("SnapToolbar", GTK_POS_RIGHT);
                    taskNum = val; // in case it was out of range
                    break;
                case 1:
                    dtw->setToolboxPosition("ToolToolbar", GTK_POS_LEFT);
                    dtw->setToolboxPosition("CommandsToolbar", GTK_POS_TOP);
                    if (notDone) {
                        dtw->setToolboxPosition("AuxToolbar", GTK_POS_TOP);
                    }
                    dtw->setToolboxPosition("SnapToolbar", GTK_POS_TOP);
                    break;
                case 2:
                    dtw->setToolboxPosition("ToolToolbar", GTK_POS_LEFT);
                    dtw->setToolboxPosition("CommandsToolbar", GTK_POS_RIGHT);
                    dtw->setToolboxPosition("SnapToolbar", GTK_POS_RIGHT);
                    if (notDone) {
                        dtw->setToolboxPosition("AuxToolbar", GTK_POS_RIGHT);
                    }
            }
            Glib::ustring prefPath = getLayoutPrefPath( dtw->desktop );
            Inkscape::Preferences::get()->setInt( prefPath + "task/taskset", taskNum );
        }
    }
}


void UXManagerImpl::addTrack( SPDesktopWidget* dtw )
{
    if (std::find(dtws.begin(), dtws.end(), dtw) == dtws.end()) {
        dtws.push_back(dtw);
    }
}

void UXManagerImpl::delTrack( SPDesktopWidget* dtw )
{
    vector<SPDesktopWidget*>::iterator iter = std::find(dtws.begin(), dtws.end(), dtw);
    if (iter != dtws.end()) {
        dtws.erase(iter);
    }
}

void UXManagerImpl::connectToDesktop( vector<GtkWidget *> const & toolboxes, SPDesktop *desktop )
{
    if (!desktop)
    {
        return;
    }
    TrackItem &tracker = trackedBoxes[desktop];
    vector<GtkWidget*>& tracked = tracker.boxes;
    tracker.destroyConn = desktop->connectDestroy(&desktopDestructHandler);

    for (vector<GtkWidget*>::const_iterator it = toolboxes.begin(); it != toolboxes.end(); ++it ) {
        GtkWidget* toolbox = *it;

        ToolboxFactory::setToolboxDesktop( toolbox, desktop );
        if (find(tracked.begin(), tracked.end(), toolbox) == tracked.end()) {
            tracked.push_back(toolbox);
        }
    }

    if (std::find(desktops.begin(), desktops.end(), desktop) == desktops.end()) {
        desktops.push_back(desktop);
    }

    gint taskNum = getDefaultTask( desktop );

    // note: this will change once more options are in the task set support:
    Inkscape::UI::UXManager::getInstance()->setTask( desktop, taskNum );
}


} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
