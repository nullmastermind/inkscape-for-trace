Inkscape 1.0beta2

Release hilights

Released on 2019-12-01

  • Better support for macOS
  • Rendering performance improvements
  • Various bugfixes

Known issues

  • Fontconfig errors on some configurations
  • Pango 1.44 has issues with vertical text


Inkscape 1.0beta

Release highlights

Released on 2019-09-07

  • Changes to extensions translation system and to .inx format
  • New trace dialog with basic autotrace support
  • Improved XML dialog
  • Hatches and pattern UI (GSOC 2019)

Known issues

  • Various warnings printed to console output
  • Performance issues when Objects dialog or XML dialog has been opened
  • Text toolbar and UX is subject to change, specially for setting line heights


Inkscape 1.0alpha2

Release highlights

Released on 2019-06-02

  • Performance improvements
  • "Native" MacOS look
  • Various fixes

Inkscape 1.0alpha

Release highlights

Released on 2019-01-16.

  • Themeing support
  • Origin in top left corner (optional)
  • Canvas rotation and mirroring
  • Better hidpi screen support
  • Control width of PowerStroke with pressure sensitive graphics tablet
  • Fillet/chamfer LPE
  • New PNG export options
  • Path operations and deselection of a large number of paths are much faster
    now

Important changes

For users

Custom Icon Sets

Icon sets do no longer consist of a single file that contains all icons, but of
single files for each icon. The directory structure must follow the standard
structure for gnome icons.

If you would like to create or convert your own icon set to the new format,
please see the 'hicolor' and 'Tango' icon theme folders in your Inkscape
installation 'share' directory for suitable examples.

As a side effect of a bug fix to the icon preview dialog (see below), custom UI
icon SVG files need to be updated to have their background color alpha channel
set to 0 so that they display correctly (see Bug #1661989).

Third-party extensions

Third-party extensions need to be updated to work with this version of
Inkscape.

For extension writers

[TBC - not final] Extensions have undergone some fundamental changes.
Inkscape's stock extensions have been moved to their own repository and were
updated for compatibility with Python 3. Internally, extensions have been
reorganized and many functions have been deprecated.

[Extension manager? How-to-guide for updating? New API elements?
Documentation?]

Instructions for updating old extensions are available at Updating your
Extension for 1.0

Also note the changed command line options.

For packagers and those who compile Inkscape

  • autotools builds have been dropped. Please use CMake for building Inkscape
    from now on. More info is available on our website.
  • libsoup dependency added: we use libsoup for making http requests without
    the need for dbus and gvfs.
  • Inkscape now uses a git submodule for the extensions directory. If you have
    cloned the repository and are not building from the release source code
    tarball, please note the updated build instructions
  • On Ubuntu 18.04, Gnome's fallback icon set (package
    'adwaita-icon-theme-full'), that is needed to display Inkscape's default
    icons completely, is no longer automatically installed. It has been added
    as a 'recommends' level dependency.
  • lib2geom: [insert up-to-date info here]

General: Application

Several small performance improvements in various areas add up to make Inkscape
run more smoothly than before (e.g. MR #448).

General User Interface

The user interface has been changed to using a more recent version of GTK+, the
widget toolkit that Inkscape uses to draw the user interface on the screen.
This new version brings a lot of improvements, especially for users of hidpi
screens. Updating Inkscape for using it has been a large effort that has been
anticipated eagerly for a long time, and was a focus of the Boston Hackfest.

Window position / size

Improvements and fixes to the code for handling/restoring window size and
position [1] . The window manager handles most of the job now which should make
it much more robust. If you still encounter problems with this, please report
those to our bug tracker.

HiDPI

Icons

[Please fill in]


Y Axis Inversion

During the Inkscape Hackfest in Kiel, a large change by Thomas Holder was
integrated into the Inkscape codebase. It allows you to optionally set the
origin of your document to the top left corner of the page. This finally makes
the coordinates that you can see in the interface match the ones that are saved
in the SVG data (unit conversions/transformations may be required), and makes
working in Inkscape more comfortable for people who are used to this more
standard behavior.


  • Option in the preferences' 'Interface' section

    Option in the preferences' 'Interface' section

  • Effect of the option (left upper page corner at 0/0)

    Effect of the option (left upper page corner at 0/0)

Canvas

Canvas Rotation

With Ctrl+Shift+Scroll wheel the drawing area can be rotated and viewed from
different angles. In the bottom right corner of the Window, the viewing angle
can be entered manually. Right-click to select between a set of preset values.
Keyboard shortcuts for clockwise/counter-clockwise/no rotation can be set in
the preferences.


Canvas rotation.gif

Canvas Mirroring

The canvas can now be flipped, to ensure that the drawing does not lean to one
side, and looks good either way. The vertical/horizontal flipping is available
from the menu View > Canvas orientation > Flip horizontally / Flip vertically.
Keyboard shortcuts for flipping the canvas can be set in the preferences (Edit
> Preferences > Interface > Keyboard shortcuts).

Flip canvas 300px.gif

Pinch-to-zoom

On supported hardware (trackpad, touchpad, multi-touch screen), the canvas can
be zoomed with the two-finger pinch gesture.

Visible Hairlines Display Mode

This new display mode is available under the "View->Display mode" menu. It
ensures that all lines are visible, regardless of zoom level, while still
drawing everything else normally.

This is especially useful for some CNC machines like laser cutters and vinyl
cutters which use hairlines to denote cut lines.

Visible hairlines.gif

Paths

Changed behavior of Stroke to Path

The 'Stroke to Path' command now not only converts the stroke of a shape to
path, but effectively splits it into its components.

In the case of applying it to a path that only has a stroke, the behavior is
unchanged.

For paths that don't only have a stroke, but also a fill and/or markers, the
result will be a group consisting of:

  • Stroke outline turned to path
  • Fill (if there was one)
  • A group of all markers (if applicable; one group per marker, consisting of
    its outline and its fill turned into a path)

Stroke to path.gif

Unlinking Clones for Path Operations

Clones and Symbols are now automatically unlinked, before a Boolean operation
(union, difference, etc.), or one of the Path operations 'Combine', 'Break
apart', or 'Stroke to Path' is performed.

A setting in the preferences at Behavior → Clones → Unlink Clones allows to
disable the automatic unlinking.

Tools

Calligraphy Tool

A new option to add dots has been added to the tool. Click in place without
moving the mouse to create a dot, Shift+Click to create a larger dot [needs
documentation in keyboard shortcut list].

Circle Tool

The circle tool can now also create closed ("filleted") circle shapes (closed
arcs) with the click of a button.

  • Circle tool shapes in this release

    Circle tool shapes in this release

Eraser

New option to erase as clip, which allows to non-destructively erase (parts of)
all kinds of elements, including raster images and clones.

Erase with clip.gif


Measurement Tool

Hovering over a path with the tool now displays the length, height, width and
position of the path. If you hover over a group, it will show the width, height
and position of the group. Holding Shift switches to showing info about the
constituents of the group.

The tool also has a new option to only measure selected objects when using
click-and-drag.

  • New info text from measurement tool (for a group)

    New info text from measurement tool (for a group)

  • Measurement tool info text for a single path (in a group with Shift)

    Measurement tool info text for a single path (in a group with Shift)

  • Measuring only the selected object (the ice cap)

    Measuring only the selected object (the ice cap)

Pencil Tool

PowerPencil

Pressure sensitivity can now be enabled for the Pencil tool. This feature makes
use of the PowerStroke Live Path Effect (LPE).

New settings for the tool are available for tweaking the behavior of the
PowerStroke LPE when it is being created with the Pencil tool (and a graphics
tablet/stylus):

  • Use pressure input (in the tool controls bar): activates the PowerStroke
    feature, if a pressure sensitive device is available.
  • Min/Max (in the tool controls bar): determines the minimal and maximal
    stroke width (0 to 100%). This does not change the number of available
    pressure levels, but spreads them out in the available line width interval.
  • Additionally, the PowerStroke LPE itself has been improved, to better work
    when used in this new way, see the section about LPE updates.
  • Pressure change for new knot (in the global Inkscape preferences, Edit >
    Preferences > Tools > Pencil): adds a PowerStroke Knot when the stylus
    pressure changes by this percentage.

[needs video/gif]

Clipping / Masking

Clip paths and masks now have an inverse mode in the menu, using the PowerClip
and PowerMask LPEs.

Inverse clip 400.gif

Live Path Effects

Live Path Effects received a major overhaul, with lots of improvements and new
features. The main changes are:

  • Set default parameters: default values for any LPE can be set in the
    respective LPE's dialog, when it is applied to an object

(Note: we have the 'multiple desktop preferences' problem here: If you have
multiple Inkscape windows open, the last one will determine what will be saved
to the preferences file, as preferences changes are only saved when Inkscape is
closed, and the settings are only loaded from file when a new window is opened.
)

  • Clip and Mask: improved handling
  • Fix multiple LPE BBox: a problem with the size of the bounding box when
    applying multiple LPEs to an object has been fixed
  • Knots on shapes: show edit knots in LPE shapes
  • Switch knots: change the handles to the correct LPE handles when one
    selects an LPE in the list of active LPEs for the selected object.


In addition to this, the LPE list now features an icon for each LPE (TBC).

  • Set default values for Mirror LPE

    Set default values for Mirror LPE


Boolean Operations LPE

[The Boolean Operations LPE finally makes non-destructive boolean operations
available in Inkscape. It works by adding the LPE to a path, then linking a
copied path to it by clicking on the 'link to path' button. That way, two
[more?] paths can be combined to a single shape, and both are still editable.
Available options:

  • union
  • symmetric difference
  • intersection
  • division
  • difference
  • cut outside
  • cut inside
  • cut

] functionality incomplete currently, does not hide linked operand, see https:/
/gitlab.com/inkscape/inkscape/-/merge_requests/20#note_100799480

  • Boolean Operations LPE

    Boolean Operations LPE

BSPline and Spiro

Improvements in Pen/Pencil mode. With "Alt", you can move the previous node.

'Clone Original' Improvements

This path effect now allows various objects instead of only paths and is even
more powerful.

Demo Video

Demo Video

Dash Stroke LPE

This new LPE creates uniformly dashed paths, optionally by subdividing the
path's segments, or including dashes that are symmetrically wrapped around
corners.

  • Rectangles with dash stroke LPE with various settings

    Rectangles with dash stroke LPE with various settings

Demo Video

Ellipse from Points

This new LPE creates an optimally fitted ellipse from a path's nodes.

In contrast to the already existing LPE "Ellipse by 5 points" this LPE is more
flexible (since, depending on the number of points available, it can fit both
circles and ellipses) and has more features. Especially technical illustrators
can benefit from these features.

See LPE:_Ellipse_from_Points for a documentation.

  • Ellipse (5 nodes), circle (3 nodes), circle segment (3 nodes), isometric
    circle (3 nodes), isometric circle with frame (3 nodes)

    Ellipse (5 nodes), circle (3 nodes), circle segment (3 nodes), isometric
    circle (3 nodes), isometric circle with frame (3 nodes)

Embroidery Stitch LPE

This new LPE can add nodes to your paths and create jump stitches, to create
data that can be converted for use with a stitching machine. To view the
stitches that you added, activate the 'Show stitches' checkbox, and, if
necessary, adjust the 'Show stitch gap' value, so you can see the single
stitches.

There are various options for calculating the order of the stitching, for
connecting the parts of the drawing and 3 different stitch patterns available.
It is suggested to play around with these until you like the result.

For exporting your data, you can, for example, use the HPGL file format and go
from there.

  • Inkscape Logo with Embroidery LPE (stitches made visible)

    Inkscape Logo with Embroidery LPE (stitches made visible)

  • Options: left: methods to order subpaths, right: methods to connect end
    points of subpaths

    Available options: left: methods to order subpaths, right: methods to
    connect end points of subpaths

Fill Between Many / Fill Between Strokes LPE

New options added:

  • Fuse coincident points: [describe]
  • Join subpaths: [describe]
  • Close: [describe]
  • LPEs on linked: [describe] (Fill Between Many only)

Fillet/Chamfer LPE

This new LPE adds fillet and chamfer to paths. Also adds a new internal class
that allows to handle extra info per node, the LPE itself is an example of use
the new classes.

Demo video

  • Chamfer by LPE

    Chamfer with LPE

  • Chamfer with 2 steps

    Chamfer with 2 steps

  • Inverse Chamfer with 2 steps

    Inverse Chamfer with 2 steps

  • Fillet with LPE

    Fillet with LPE

  • Inverse fillet with LPE

    Inverse fillet with LPE

Knot LPE

New options added:

  • Inverse: use the stroke width of the other path as basis for calculating
    the gap length
  • Add stroke width: make the gap wider by adding the width of the stroke to
    the value for the gap length
  • Add bottom (other?) stroke width: make the gap wider by adding the width of
    the bottom (other?) stroke to the value for the gap length
  • Crossing signs: [not final]

Measure Segments LPE

This new path effect adds DIN and custom style measuring lines to "straight"
segments in a path.

Demo video

  • Measure Segments LPE

    Measure Segments LPE

Mirror Symmetry and Rotate Copies LPE

  • Split feature: This new feature allows custom styles for each part of the
    resulting drawing without unlinking the LPE. Demo Video
  • The LPE display now updates accordingly when there are objects added or
    removed.

  • Separate styles for rotated copies

    Separate styles for rotated copies

Power Clip and Power Mask LPE

This new LPE adds options to clips and masks.


PowerStroke LPE Improvements

  • Width scale setting added: adjust the overall width of the stroke after it
    has been drawn.
  • Closed paths: PowerStroke now works much better on closed paths.

Import / Export

Linking and embedding SVG files

On import of an SVG file, there is now a dialog that asks if the user would
like to link to the SVG file, to embed it (base64 encoded) into an <img> tag,
or if the objects in the SVG file should be imported into the document (which
was how Inkscape handled importing SVG files previously).

[ TBC: The dpi value for displaying embedded SVG files can be set in the import
dialog.]

This makes importing SVG files work (almost) the same as importing raster
images.


The 'Embed' and 'Extract' options in the context menu for linked SVG files work
the same as they do for raster images. The 'Edit externally' option will open
the linked SVG file with Inkscape per default. This setting can be changed in
the preferences' 'Imported Images' section.

The displaying of the dialog can be disabled by checking the 'Don't ask me
again' option.

Linked and embedded SVG images are displayed as their raster representations.

The resolution used for displaying them [TBC: can be set per image? can be set
in the xxx dialog for the selected image] is the default image import
resolution set in the preferences' 'Imported Images' section. A change in this
option will take effect upon closing and reopening the file, and will affect
all linked SVG images in the file.

Export PNG images

The export dialog has received several new options which are available when you
expand the 'Advanced' section.

  • Enable interlacing (ADAM7): when loading images, they will be displayed
    faster
  • Bit depth: set the number of bits that code for the color of a pixel,
    supports grayscale and up to 16bit
  • Compression type: choose strength of lossless compression
  • pHYs dpi: force-set a dpi value for the image
  • Antialiasing: choose type of antialiasing or disable it


  • PNG export options

    PNG export options

  • PNG bit depth options

    PNG bit depth options

  • PNG compression options

    PNG compression options

  • PNG antialiasing options

    PNG antialiasing options

Extensions

Extension development

  • All INX Parameters now have the common attribute indent="n" where n
    specifies the level of indentation in the extension UI.
  • Add appearance="url" for INX Parameters of type "description". You can now
    add clickable links to your extension UI.

Plot extension

The new option 'Convert objects to paths' will take care of converting
everything to a path non-destructively before the data is sent to the plotter.
[gives wrong error message, maybe not working? https://gitlab.com/inkscape/
inkscape/-/commit/dd3b6aa099175e2244e1e04dde45bf21a966425e#note_100908512]

Palettes

The Munsell palette has been added to Inkscape's set of stock palettes.

  • Munsell palette

    Munsell palette

Templates

  • The Desktop template has new options for 4k, 5k and 8k screens.
  • A new template for an A4 3-fold roll flyer was added.

  • New template options for 'Desktop' template

    New template options for 'Desktop' template

  • New A4 3-fold roll flyer template

    New A4 3-fold roll flyer template

SVG and CSS

  • Dashes: Inkscape can now load and display files with dashes and/or
    dashoffsets defined in other units than the unitless user unit (e.g. %, mm)
    correctly. There is no user interface for editing these values currently,
    except for the XML editor. Values for the dasharray that are entered in
    other units (except for %) will be converted to user units when the new
    values are set.

  • [Please fill in]

Dialogs

Document Properties

  • When resizing the page, the page margin fields can now be locked, so the
    same value will be used for all margins, but only needs to be entered once.
  • The guides panel now has controls to lock or unlock all guides, create
    guides around the page, and delete all guides. These actions also appear on
    the Edit menu, making it possible to assign custom keyboard shortcuts.
  • Grids can now be aligned to the corners, edge midpoints, or centre of the
    page with a button click in the grids panel.

  • Lock to set same margins for page resizing

    Lock to set same margins for page resizing

  • Document properties: Toggle guide lock for document, create page border
    guides, remove all guides

    Document properties: Toggle guide lock for document, create page border
    guides, remove all guides

  • Grid alignment options in document properties

    Grid alignment options in document properties

Preferences

  • The Bitmaps subsection has been renamed to Imported Images, as it now
    applies to both imported (embedded or linked) raster images as well as to
    imported (embedded or linked) SVG images (i.e. to everything in <img>
    tags).
  • The System subsection lists more relevant folders and offers buttons to
    open those folders with the system's file browser. This makes it easier to
    find the correct folder, e.g. for resetting the preferences or for adding
    an extension or a new icon set.
  • The System subsection now has a button for quickly resetting all Inkscape
    preferences.
  • An option for scaling a stroke's dash pattern when scaling the stroke width
    has been added and can be found at Behaviour → Dashes. It is activated by
    default.
  • Autosave is now enabled by default. The default directory has changed (the
    path is displayed in Edit > Preferences > Input/Output > Autosave: Autosave
    directory).

  • Important folders can be opened from the preferences

    Important folders can be opened from the preferences

Symbols

  • The Symbols dialog can now handle a lot of symbols without delay on
    startup, and also allows searching. Symbols and symbol sets now displayed
    in alphabetical order.


  • Symbol sets ordered alphabetically

    Symbol sets ordered alphabetically

  • Searching for symbols

    Searching for symbols

Filter Editor

  • The filter primitives now also have a symbolic icon (one whose color can be
    changed).

Customization

Customize all files in the share folder

All files in /share can be over-ridden by placing files in the user's
configuration folder (e.g. ~/.config/inkscape). Configurable contents now
includes extensions, filters, fonts, gradients, icons, keyboard shortcuts,
preset markers, palettes, patterns, about screen, symbol sets, templates,
tutorials and some user interface configuration files. Only the file
'units.xml' cannot be overridden.

Fonts

  • Inkscape can now load fonts that are not installed on the system. By
    default Inkscape will load additional fonts from Inkscape's share folder (/
    share/inkscape/fonts) and the user's configuration folder (~/.config/
    inkscape/fonts). Custom folders can be set in preferences (see Tools → Text
    → Additional font directories).

  • Set custom font folders

    Set custom font folders

Keyboard shortcuts

  • Allow to use "Super", "Hyper" and "Meta" modifier keys
  • Improve shortcut handling code. This should fix a lot of issues and allow
    to use a lot of shortcuts which were inaccessible before, especially on
    non-English keyboard layouts.

User interface customization

  • Inkscape is starting to use glade files for its dialogs so they can be
    reconfigured by users. Only one is currently supported (filter editor).
  • The contents of the menus can be configured by customizing the menus.xml
    file.
  • Toolbar contents for the command bar (toolbar-commands.ui), the snap bar
    (toolbar-snap.ui), the tool controls bars for each tool
    (toolbar-select.ui), the toolbox (tool-toolbar.ui) is now configurable.
  • The file keybindings.rc allows you to... (TODO: do what? What does it do in
    comparison to keys.xml? Seems to not work at all... seems to be ancient.
    Can be deleted?)
  • The interface colors and some more UI styles can be customized in style.css
    (very raw themeing support).

Theme selection

In 'Edit > Preferences > User Interface > Theme', users can set a custom GTK3
theme for Inkscape. If the theme comes with a dark variant, activating the 'Use
dark theme' checkbox will result in the dark variant being used. The new theme
will be applied immediately.

New theme folders can be added to the directory indicated in Edit > Preferences
> System : User themes. A large selection of (more or less current) GTK3 themes
is available for download at gnome-look.org

Icon set selection

In 'Edit > Preferences > User Interface > Theme', the icon set to use can be
selected. By default, Inkscape comes with 'hicolor' and 'Tango' icons. In
addition to this, it offers to use the system icons.

Inkscape also comes with a default symbolic icon set as part of the hicolor
icon set. These icons can be colorized in a custom color.

Changes to the icon set take effect when Inkscape is restarted, or when the
entire user interface is reloaded by clicking on the 'Reload icons' button.
This rebuilds all Inkscape windows. Rebuild takes a couple of seconds, during
which the Inkscape interface will be invisible.


  • Light theme and Tango icon set

    Light theme with Tango icon set

  • Light theme and hicolor icon set

    Light theme with hicolor icon set

  • Dark theme and symbolic icon set

    Dark theme with symbolic icon set

  • Dark theme with custom colored symbolic icon set

    Dark theme with symbolic icon set and a custom icon color

Saving the current file as template

A new entry for saving the current file as a template has been added to the
'File' menu. You need to specify a name for it, and optionally, you can add the
template's author, a description and some keywords. A checkbox allows you to
set the new template as the default template.

  • Save current file as a template

    Save current file as a template

Custom page sizes in Document Properties

Inkscape now creates a CSV file (comma separated values) called "pages.csv". It
is located in your Inkscape user preferences folder, next to your
'preferences.xml' file. This file contains the default page sizes that you can
choose from in the 'Page' tab of the 'Document properties' dialog. You can edit
the pages.csv file to remove the page sizes you won't use, or to add new ones.

Inkview

Inkview was considerably improved and got some new features:

  • Support folders as input (will load all SVG files from the specified
    folder)
    The -r or --recursive option will even allow to search subfolders
    recursively.
  • Implement -t or --timer option which allows to set a time after which the
    next file will be automatically loaded.
  • Add -s or --scale option to set a factor by which to scale the displayed
    image.
  • Add -f or --fullscreen option to launch Inkview in fullscreen mode
  • Many smaller fixes and improvements


Command Line

The Inkscape command line has undergone a large overhaul. The most important
changes are:

  • verbs/actions .......
  • order of commands .......
  • parallel exports ....
  • shell mode(s)....
  • ........

  • Probably not in release: xverbs (command line commands that take
    parameters, e.g. for saving the selection under a specified filename as svg
    file) (mailing list thread)
  • New verb allows to swap fill and stroke from command line:
    EditSwapFillStroke (a keyboard shortcut can now be assigned to it) (bug
    675690)
  • Files can now also be saved as Inkscape SVG without calling the GUI save
    dialog (new command: --export-inkscape-svg)
  • Inkscape can now import a specific page of a PDF file from the command
    line, for batch processing (new option: --pdf-page N) - does this still
    work after Tav's changes?

Translations [as of 2019-01-08]

Translations were updated for:

  • Basque
  • British English
  • Catalan
  • Croatian
  • Czech
  • French
  • German
  • Hungarian
  • Icelandic
  • Italian
  • Latvian
  • Romanian
  • Russian
  • Slovak
  • Spanish
  • Ukrainian
  • Swedish

The installer was translated to:

  • Korean

Notable Bugfixes

  • Symbols: Visio Stencils loaded from .vss files now use their actual name
    instead of a placeholder derived from the symbol file's name (bug 1676144)
  • Shapes on Pen and Pencil tools now retain color and width (bug:1707899).
  • Text and Font dialog: The font selection no longer jumps to the top of the
    list when clicking Apply.
  • Docked dialogs now open on their own when the corresponding functionality
    is called from a menu or button [TBC: Bug: if minimized, this requires a
    second click]
  • The icon preview dialog now correctly shows the page background (Bug #
    1537497).
  • Improved UI performance of handling large paths and selections:
      □ Accelerated path break-apart and Boolean operations by disabling
        intermittent canvas rendering during these operations.
      □ Accelerated "deselect" speed by improving internal data structure
        algorithms.

For an exhaustive list of bugs that have been fixed, please see the milestones
page for Inkscape 1.0.

Known Issues

[Please fill in]



For information on prior releases, please see:

    http://wiki.inkscape.org/wiki/index.php/Inkscape

