# Dialog System

Author: vanntile

The dialog system used for dialog-type widgets is made out of the classes
defined in the following header files (each explained later on):

- dialog-container.h
- dialog-multipaned.h
- dialog-notebook.h
- dialog-window.h
- dialog-base.h

This is a Gtk::Notebook based dialog manager with a GIMP style multipane.
Dialogs can live only inside DialogNotebooks, as pages in the inner
Gtk::Notebook, with their own tabs. A DialogNotebook itself is one of the
children of a DialogMultipaned. There can be several levels of DialogMultipaned,
but the top parent is a DialogContainer, which manages the existance of such
dialogs.

DialogMultipaned is a paned-type container which supports resizing the children
by dragging the separator "handle" widgets. More than this, it supports the
addition of new children (in DialogNotebook) by drag-and-dropping them at the
extremeties of a multipane, where you can find dropzones (left and right for
horizontal ones, or top and bottom for vertical ones).

Dialogs can also live independently in their own DialogWindow. In this floating
state, they track the last active window, while in the attached (docked) state,
they track the InkscapeWindow they are in. You can drag tabs from DialogNotebook
to move a DialogBase (or child class instance) between windows. In Wayland,
if you drag the tab to an invalid position, it will create automatically a
DialogWindow to live in.

DialogContainers are instantiated with a horizontal DialogMultipaned.

## Initialisation

The initial DialogContainer is created inside a DesktopWidget, then inserting
toolbars and an empty vertical DialogMultipaned where new dialogs will be added.

## Adding a new dialog

In order to add a new dialog to a window, you have to call the method
`new_dialog(unsigned int)` on the container inside the desktop of the window.
The value is the code of the corresponding `DialogVerb` from in `verbs.cpp` and
similar methods for containers that have a different type of verb.
