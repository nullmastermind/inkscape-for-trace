#!/bin/bash
#
# USAGE
# osx-app [-s] [-l /path/to/libraries] -py /path/to/python/modules [-l /path/to/libraries] -b /path/to/bin/inkscape -p /path/to/Info.plist
#
# This script attempts to build an Inkscape.app package for OS X, resolving
# dynamic libraries, etc.
# 
# If the '-s' option is given, then the libraries and executable are stripped.
# 
# The Info.plist file can be found in the base inkscape directory once
# configure has been run.
#
# AUTHORS
#		 Kees Cook <kees@outflux.net>
#		 Michael Wybrow <mjwybrow@users.sourceforge.net>
#		 Jean-Olivier Irisson <jo.irisson@gmail.com>
#                Liam P. White <inkscapebrony@gmail.com>
# 
# Copyright (C) 2005 Kees Cook
# Copyright (C) 2005-2009 Michael Wybrow
# Copyright (C) 2007-2009 Jean-Olivier Irisson
# Copyright (C) 2014 Liam P. White
#
# Released under GNU GPL, read the file 'COPYING' for more information
#
# Thanks to GNUnet's "build_app" script for help with library dep resolution.
#		https://gnunet.org/svn/GNUnet/contrib/OSX/build_app
# 
# NB:
# When packaging Inkscape for OS X, configure should be run with the 
# "--enable-osxapp" option which sets the correct paths for support
# files inside the app bundle.
# 

# Defaults
strip=false
add_python=false
python_dir=""

# If LIBPREFIX is not already set (by osx-build.sh for example) set it to blank (one should use the command line argument to set it correctly)
if [ -z $LIBPREFIX ]; then
	LIBPREFIX=""
fi


# Help message
#----------------------------------------------------------
help()
{
echo -e "
Create an app bundle for OS X

\033[1mUSAGE\033[0m
	$0 [-s] [-l /path/to/libraries] -py /path/to/python/modules -b /path/to/bin/inkscape -p /path/to/Info.plist

\033[1mOPTIONS\033[0m
	\033[1m-h,--help\033[0m 
		display this help message
	\033[1m-s\033[0m
		strip the libraries and executables from debugging symbols
	\033[1m-py,--with-python\033[0m
		add python modules (numpy, lxml) from given directory
		inside the app bundle
	\033[1m-l,--libraries\033[0m
		specify the path to the librairies Inkscape depends on
		(typically /sw or /opt/local)
	\033[1m-b,--binary\033[0m
		specify the path to Inkscape's binary. By default it is in
		Build/bin/ at the base of the source code directory
	\033[1m-p,--plist\033[0m
		specify the path to Info.plist. Info.plist can be found
		in the base directory of the source code once configure
		has been run

\033[1mEXAMPLE\033[0m
	$0 -s -py ~/python-modules -l /opt/local -b ../../Build/bin/inkscape -p ../../Info.plist
"
}


# Parse command line arguments
#----------------------------------------------------------
while [ "$1" != "" ]
do
	case $1 in
		-py|--with-python)
			add_python=true
			python_dir="$2"
			shift 1 ;;
		-s)
			strip=true ;;
		-l|--libraries)
			LIBPREFIX="$2"
			shift 1 ;;
		-b|--binary)
			binary="$2"
			shift 1 ;;
		-p|--plist)
			plist="$2"
			shift 1 ;;
		-h|--help)
			help
			exit 0 ;;
		*)
			echo "Invalid command line option: $1" 
			exit 2 ;;
	esac
	shift 1
done

echo -e "\n\033[1mCREATE INKSCAPE APP BUNDLE\033[0m\n"

# Safety tests

if [ "x$binary" == "x" ]; then
	echo "Inkscape binary path not specified." >&2
	exit 1
fi

if [ ! -x "$binary" ]; then
	echo "Inkscape executable not not found at $binary." >&2
	exit 1
fi

if [ "x$plist" == "x" ]; then
	echo "Info.plist file not specified." >&2
	exit 1
fi

if [ ! -f "$plist" ]; then
	echo "Info.plist file not found at $plist." >&2
	exit 1
fi

if [ ${add_python} = "true" ]; then
	if [ "x$python_dir" == "x" ]; then
		echo "Python modules directory not specified." >&2
		exit 1
	fi
fi

if [ ! -e "$LIBPREFIX" ]; then
	echo "Cannot find the directory containing the libraires: $LIBPREFIX" >&2
	exit 1
fi

if ! pkg-config --exists gtk-engines-2; then
	echo "Missing gtk-engines2 -- please install gtk-engines2 and try again." >&2
	exit 1
fi

if ! pkg-config --exists gnome-vfs-2.0; then
	echo "Missing gnome-vfs2 -- please install gnome-vfs2 and try again." >&2
	exit 1
fi

if ! pkg-config --exists poppler; then
	echo "Missing poppler -- please install poppler and try again." >&2
	exit 1
fi

if ! pkg-config --modversion ImageMagick >/dev/null 2>&1; then
	echo "Missing ImageMagick -- please install ImageMagick and try again." >&2
	exit 1
fi

if [ ! -e "$LIBPREFIX/lib/aspell-0.60/en.dat" ]; then
	echo "Missing aspell en dictionary -- please install at least 'aspell-dict-en', but" >&2
	echo "preferably more dictionaries ('aspell-dict-*') and try again." >&2
	exit 1
fi


# Handle some version specific details.
VERSION=`/usr/bin/sw_vers | grep ProductVersion | cut -f2 -d'.'`
if [ "$VERSION" -ge "4" ]; then
	# We're on Tiger (10.4) or later.
	# XCode behaves a little differently in Tiger and later.
	XCODEFLAGS="-configuration Deployment"
	SCRIPTEXECDIR="ScriptExec/build/Deployment/ScriptExec.app/Contents/MacOS"
	EXTRALIBS=""
else
	# Panther (10.3) or earlier.
	XCODEFLAGS="-buildstyle Deployment"
	SCRIPTEXECDIR="ScriptExec/build/ScriptExec.app/Contents/MacOS"
	EXTRALIBS=""
fi


# Package always has the same name. Version information is stored in
# the Info.plist file which is filled in by the configure script.
package="Inkscape.app"

# Remove a previously existing package if necessary
if [ -d $package ]; then
	echo "Removing previous Inkscape.app"
	rm -rf $package
fi


# Set the 'macosx' directory, usually the current directory.
resdir=`pwd`

# Custom resources used to generate resources during app bundle creation.
if [ -z "$custom_res" ] ; then
    custom_res="${resdir}/Resources-extras"
fi


# Prepare Package
#----------------------------------------------------------
pkgexec="$package/Contents/MacOS"
pkgbin="$package/Contents/Resources/bin"
pkgshare="$package/Contents/Resources/share"
pkglib="$package/Contents/Resources/lib"
pkglocale="$package/Contents/Resources/locale"
pkgpython="$package/Contents/Resources/python/site-packages/"
pkgresources="$package/Contents/Resources"

mkdir -p "$pkgexec"
mkdir -p "$pkgbin"
mkdir -p "$pkglib"
mkdir -p "$pkgshare"
mkdir -p "$pkglocale"
mkdir -p "$pkgpython"

# Build and add the launcher
#----------------------------------------------------------
(
	# Build fails if CC happens to be set (to anything other than CompileC)
	unset CC
	
	cd "$resdir/ScriptExec"
	echo -e "\033[1mBuilding launcher...\033[0m\n"
	xcodebuild $XCODEFLAGS clean build
)
cp "$resdir/$SCRIPTEXECDIR/ScriptExec" "$pkgexec/Inkscape"


# Copy all files into the bundle
#----------------------------------------------------------
echo -e "\n\033[1mFilling app bundle...\033[0m\n"

binary_name=`basename "$binary"`
binary_dir=`dirname "$binary"`

# Inkscape's binary
binpath="$pkgbin/inkscape-bin"
cp -v "$binary" "$binpath"
# TODO Add a "$verbose" variable and command line switch, which sets wether these commands are verbose or not

# Share files
rsync -av "$binary_dir/../share/$binary_name"/* "$pkgresources/"
cp "$plist" "$package/Contents/Info.plist"
rsync -av "$binary_dir/../share/locale"/* "$pkgresources/locale"

# Copy GTK shared mime information
mkdir -p "$pkgresources/share"
cp -rp "$LIBPREFIX/share/mime" "$pkgresources/share/"

# Copy GTK hicolor icon theme index file
mkdir -p "$pkgresources/share/icons/hicolor"
cp "$LIBPREFIX/share/icons/hicolor/index.theme"  "$pkgresources/share/icons/hicolor"

# GTK+ stock icons with legacy icon mapping
echo "Creating GtkStock icon theme ..."
stock_src="${custom_res}/src/icons/stock-icons" \
    ./create-stock-icon-theme.sh "${pkgshare}/icons/GtkStock"
gtk-update-icon-cache --index-only "${pkgshare}/icons/GtkStock"

# GTK+ themes
cp -RP "$LIBPREFIX/share/gtk-engines" "$pkgshare/"
for item in Adwaita Clearlooks HighContrast Industrial Raleigh Redmond ThinIce; do
    mkdir -p "$pkgshare/themes/$item"
    cp -RP "$LIBPREFIX/share/themes/$item/gtk-2.0" "$pkgshare/themes/$item/"
done

# Icons and the rest of the script framework
rsync -av --exclude ".svn" "$resdir"/Resources/* "$pkgresources/"

# Update the ImageMagick path in startup script.
IMAGEMAGICKVER=`pkg-config --modversion ImageMagick`
sed -e "s,IMAGEMAGICKVER,$IMAGEMAGICKVER,g" -i "" $pkgbin/inkscape

# Add python modules if requested
if [ ${add_python} = "true" ]; then
	# copy python site-packages. They need to be organized in a hierarchical set of directories, by architecture and python major+minor version, e.g. i386/2.3/ for Ptyhon 2.3 on Intel
	cp -rvf "$python_dir"/* "$pkgpython"
fi

# PkgInfo must match bundle type and creator code from Info.plist
echo "APPLInks" > $package/Contents/PkgInfo

# Pull in extra requirements for Pango and GTK
pkgetc="$package/Contents/Resources/etc"
mkdir -p $pkgetc/pango

# Need to adjust path and quote in case of spaces in path.
sed -e "s,$LIBPREFIX,\"\${CWD},g" -e 's,\.so ,.so" ,g' $LIBPREFIX/etc/pango/pango.modules > $pkgetc/pango/pango.modules
cat > $pkgetc/pango/pangorc <<END_PANGO
[Pango]
ModuleFiles=\${HOME}/Library/Application Support/org.inkscape.Inkscape/0.91/pango.modules
END_PANGO

mkdir -p $pkgetc/fonts
cp -r $LIBPREFIX/etc/fonts/conf.d $pkgetc/fonts/
mkdir -p $pkgshare/fontconfig
cp -r $LIBPREFIX/share/fontconfig/conf.avail $pkgshare/fontconfig/
(cd $pkgetc/fonts/conf.d && ln -s ../../../share/fontconfig/conf.avail/10-autohint.conf)
(cd $pkgetc/fonts/conf.d && ln -s ../../../share/fontconfig/conf.avail/70-no-bitmaps.conf)


for item in gnome-vfs-mime-magic gnome-vfs-2.0
do
	cp -r $LIBPREFIX/etc/$item $pkgetc/
done

pango_version=`pkg-config --variable=pango_module_version pango`
mkdir -p $pkglib/pango/$pango_version/modules
cp $LIBPREFIX/lib/pango/$pango_version/modules/*.so $pkglib/pango/$pango_version/modules/

gtk_version=`pkg-config --variable=gtk_binary_version gtk+-2.0`
mkdir -p $pkglib/gtk-2.0/$gtk_version/{engines,immodules,printbackends}
cp -r $LIBPREFIX/lib/gtk-2.0/$gtk_version/* $pkglib/gtk-2.0/$gtk_version/

mkdir -p $pkglib/gnome-vfs-2.0/modules
cp $LIBPREFIX/lib/gnome-vfs-2.0/modules/*.so $pkglib/gnome-vfs-2.0/modules/

mkdir -p $pkglib/gdk-pixbuf-2.0/$gtk_version/loaders
cp $LIBPREFIX/lib/gdk-pixbuf-2.0/$gtk_version/loaders/*.so $pkglib/gdk-pixbuf-2.0/$gtk_version/loaders/

mkdir -p "$pkgetc/gtk-2.0/"
sed -e "s,$LIBPREFIX,\${CWD},g" $LIBPREFIX/lib/gtk-2.0/$gtk_version/immodules.cache > $pkgetc/gtk-2.0/gtk.immodules
sed -e "s,$LIBPREFIX,\${CWD},g" $LIBPREFIX/lib/gdk-pixbuf-2.0/$gtk_version/loaders.cache > $pkgetc/gtk-2.0/gdk-pixbuf.loaders

cp -r "$LIBPREFIX/lib/ImageMagick-$IMAGEMAGICKVER" "$pkglib/"
cp -r "$LIBPREFIX/share/ImageMagick-6" "$pkgresources/share/"

# Copy aspell dictionary files:
cp -r "$LIBPREFIX/share/aspell" "$pkgresources/share/"

# Copy all linked libraries into the bundle
#----------------------------------------------------------
# get list of *.so modules from python modules
python_libs="$(find $pkgpython -name *.so -or -name *.dylib)"

# get list of included binary executables
extra_bin=$(find $pkgbin -exec file {} \; | grep executable | grep -v text | cut -d: -f1)

# Find out libs we need from MacPorts, Fink, or from a custom install
# (i.e. $LIBPREFIX), then loop until no changes.
a=1
nfiles=0
endl=true
while $endl; do
    echo -e "\033[1mLooking for dependencies.\033[0m Round" $a
    libs="$(otool -L \
        $pkglib/gtk-2.0/$gtk_version/{engines,immodules,printbackends}/*.{dylib,so} \
        $pkglib/gdk-pixbuf-2.0/$gtk_version/loaders/*.so \
        $pkglib/pango/$pango_version/modules/*.so \
        $pkglib/gnome-vfs-2.0/modules/*.so \
        $pkglib/gio/modules/*.so \
        $pkglib/ImageMagick-$IMAGEMAGICK_VER/modules-Q16/{filters,coders}/*.so \
        $pkglib/*.{dylib,so} \
        $pkgbin/*.so \
	$python_libs \
        $extra_bin \
        2>/dev/null | fgrep compatibility | cut -d\( -f1 | grep $LIBPREFIX | sort | uniq)"
    cp -f $libs "$pkglib"
    let "a+=1"	
    nnfiles="$(ls "$pkglib" | wc -l)"
    if [ $nnfiles = $nfiles ]; then
        endl=false
    else
        nfiles=$nnfiles
    fi
done

# Some libraries don't seem to have write permission, fix this.
chmod -R u+w "$package/Contents/Resources/lib"

# Rewrite id and paths of linked libraries
#----------------------------------------------------------
# extract level for relative path to libs
echo -e "\n\033[1mRewriting library paths ...\033[0m\n"

LIBPREFIX_levels="$(echo "$LIBPREFIX"|awk -F/ '{print NF+1}')"

fixlib () {
    if [ ! -d "$1" ]; then
        fileLibs="$(otool -L $1 | fgrep compatibility | cut -d\( -f1)"
        filePath="$(echo "$2" | sed 's/.*Resources//')"
        fileType="$3"
        unset to_id
        case $fileType in
            lib)
                # TODO: verfiy correct/expected install name for relocated libs
                to_id="$package/Contents/Resources$filePath/$1"
                loader_to_res="$(echo $filePath | gawk -F/ '{for (i=1;i<NF;i++) sub($i,".."); sub($NF,"",$0); print $0}')"
                ;;
            bin)
                loader_to_res="../"
                ;;
            exec)
                loader_to_res="../Resources/"
                ;;
            *)
                echo "Skipping loader_to_res for $1"
                ;;
        esac
        [ $to_id ] && install_name_tool -id "$to_id" "$1"
        for lib in $fileLibs; do
            first="$(echo $lib | cut -d/ -f1-3)"
            if [ $first != /usr/lib -a $first != /usr/X11 -a $first != /opt/X11 -a $first != /System/Library ]; then
                lib_prefix_levels="$(echo $lib | awk -F/ '{for (i=NF;i>0;i--) if($i=="lib") j=i; print j}')"
                res_to_lib="$(echo $lib | cut -d/ -f$lib_prefix_levels-)"
                unset to_path
                case $fileType in
                    lib)
                        to_path="@loader_path/$loader_to_res$res_to_lib"
                        ;;
                    bin)
                        to_path="@executable_path/$loader_to_res$res_to_lib"
                        ;;
                    exec)
                        to_path="@executable_path/$loader_to_res$res_to_lib"
                        ;;
                    *)
                        echo "Skipping to_path for $lib in $1"
                        ;;
                esac
                [ $to_path ] && install_name_tool -change "$lib" "$to_path" "$1"
            fi
        done
    fi
}

rewritelibpaths () {
    echo "Rewriting dylib paths for included binaries:"
    for file in $extra_bin; do
        echo -n "Rewriting dylib paths for $file ... "
        (cd "$(dirname $file)" ; fixlib "$(basename $file)" "$(dirname $file)" "bin")
        echo "done"
    done
    echo "Rewriting dylib paths for included libraries:"
    for file in $(find $package \( -name '*.so' -or -name '*.dylib' \) -and -not -ipath '*.dSYM*'); do
        echo -n "Rewriting dylib paths for $file ... "
        (cd "$(dirname $file)" ; fixlib "$(basename $file)" "$(dirname $file)" "lib")
        echo "done"
    done
}

rewritelibpaths

exit 0
