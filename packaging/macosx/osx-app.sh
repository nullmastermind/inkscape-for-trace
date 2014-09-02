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
#		 Liam P. White <inkscapebrony@gmail.com>
#		 ~suv <suv-sf@users.sourceforge.net>
# 
# Copyright (C) 2005 Kees Cook
# Copyright (C) 2005-2009 Michael Wybrow
# Copyright (C) 2007-2009 Jean-Olivier Irisson
# Copyright (C) 2014 Liam P. White
# Copyright (C) 2014 ~suv
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
add_python=true
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
#----------------------------------------------------------

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
	if [ -z "$python_dir" ]; then
		echo "Python modules will be copied from MacPorts tree."
	else
		if [ ! -e "$python_dir" ]; then
			echo "Python modules directory \""$python_dir"\" not found." >&2
			exit 1
		else
			if [ -e "$python_dir/i386" -o -e "$python_dir/ppc" ]; then
				echo "Outdated structure in custom python modules detected,"
				echo "not compatible with current packaging."
				exit 1
			else
				echo "Python modules will be copied from $python_dir."
			fi
		fi
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

# awk on Leopard fails in fixlib(), test earlier and require gawk if test fails
awk_test="$(echo "/lib" | awk -F/ '{for (i=1;i<NF;i++) sub($i,".."); sub($NF,"",$0); print $0}')"
if [ -z "$awk_test" ]; then
	if [ ! -x "$LIBPREFIX/bin/gawk" ]; then
		echo "awk provided by system is too old, please install gawk and try again" >&2
		exit 1
	else
		awk_cmd="$LIBPREFIX/bin/gawk"
	fi
else
	awk_cmd="awk"
fi
unset awk_test


# OS X version
#----------------------------------------------------------
OSXVERSION="$(/usr/bin/sw_vers | grep ProductVersion | cut -f2)"
OSXMINORVER="$(cut -d. -f 1,2 <<< $OSXVERSION)"
OSXMINORNO="$(cut -d. -f2 <<< $OSXVERSION)"
OSXPOINTNO="$(cut -d. -f3 <<< $OSXVERSION)"
ARCH="$(uname -a | awk '{print $NF;}')"


# Setup
#----------------------------------------------------------
# Handle some version specific details.
if [ "$OSXMINORNO" -le "4" ]; then
	echo "Note: Inkscape packaging requires Mac OS X 10.5 Leopard or later."
	exit 1
else # if [ "$OSXMINORNO" -ge "5" ]; then
	XCODEFLAGS="-configuration Deployment"
	SCRIPTEXECDIR="ScriptExec/build/Deployment/ScriptExec.app/Contents/MacOS"
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
pkgetc="$package/Contents/Resources/etc"
pkglib="$package/Contents/Resources/lib"
pkgshare="$package/Contents/Resources/share"
pkglocale="$package/Contents/Resources/share/locale"
#pkgpython="$package/Contents/Resources/python/site-packages/"
pkgresources="$package/Contents/Resources"

mkdir -p "$pkgexec"
mkdir -p "$pkgbin"
mkdir -p "$pkgetc"
mkdir -p "$pkglib"
mkdir -p "$pkgshare"
mkdir -p "$pkglocale"
#mkdir -p "$pkgpython"


# utility
#----------------------------------------------------------

if [ $verbose_mode ] ; then 
    cp_cmd="/bin/cp -v"
    ln_cmd="/bin/ln -sv"
    rsync_cmd="/usr/bin/rsync -av"
else
    cp_cmd="/bin/cp"
    ln_cmd="/bin/ln -s"
    rsync_cmd="/usr/bin/rsync -a"
fi


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
rsync -av "$binary_dir/../share/$binary_name"/* "$pkgshare/$binary_name"
cp "$plist" "$package/Contents/Info.plist"
rsync -av "$binary_dir/../share/locale"/* "$pkglocale"

# Copy GTK shared mime information
mkdir -p "$pkgresources/share"
cp -rp "$LIBPREFIX/share/mime" "$pkgshare/"

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

# Add python modules if requested
if [ ${add_python} = "true" ]; then
	function install_py_modules ()
	{
		# lxml
		$cp_cmd -RL "$packages_path/lxml" "$pkgpython"
		# numpy
		$cp_cmd -RL "$packages_path/nose" "$pkgpython"
		$cp_cmd -RL "$packages_path/numpy" "$pkgpython"
		# UniConvertor
		$cp_cmd -RL "$packages_path/PIL" "$pkgpython"
		if [ "$PYTHON_VER" == "2.5" ]; then
			$cp_cmd -RL "$packages_path/_imaging.so" "$pkgpython"
			$cp_cmd -RL "$packages_path/_imagingcms.so" "$pkgpython"
			$cp_cmd -RL "$packages_path/_imagingft.so" "$pkgpython"
			$cp_cmd -RL "$packages_path/_imagingmath.so" "$pkgpython"
		fi
		$cp_cmd -RL "$packages_path/sk1libs" "$pkgpython"
		$cp_cmd -RL "$packages_path/uniconvertor" "$pkgpython"
		# cleanup python modules
		find "$pkgpython" -name *.pyc -print0 | xargs -0 rm -f
		find "$pkgpython" -name *.pyo -print0 | xargs -0 rm -f

		# TODO: test whether to remove hard-coded paths from *.la files or to exclude them altogether
		for la_file in $(find "$pkgpython" -name *.la); do
			sed -e "s,libdir=\'.*\',libdir=\'\',g" -i "" "$la_file"
		done
	}

	if [ $OSXMINORNO -eq "5" ]; then
		PYTHON_VERSIONS="2.5 2.6 2.7"
	elif [ $OSXMINORNO -eq "6" ]; then
		PYTHON_VERSIONS="2.6 2.7"
	else # if [ $OSXMINORNO -ge "7" ]; then
		PYTHON_VERSIONS="2.7"
	fi
	if [ -z "$python_dir" ]; then
		for PYTHON_VER in $PYTHON_VERSIONS; do
			python_dir="$(${LIBPREFIX}/bin/python${PYTHON_VER}-config --prefix)"
			packages_path="${python_dir}/lib/python${PYTHON_VER}/site-packages"
			pkgpython="${pkglib}/python${PYTHON_VER}/site-packages"
			mkdir -p $pkgpython
			install_py_modules
		done
	else
		# copy custom python site-packages. 
		# They need to be organized in a hierarchical set of directories by python major+minor version:
		#   - ${python_dir}/python2.5/site-packages/lxml
		#   - ${python_dir}/python2.5/site-packages/nose
		#   - ${python_dir}/python2.5/site-packages/numpy
		#   - ${python_dir}/python2.6/site-packages/lxml
		#   - ...
		cp -rvf "$python_dir"/* "$pkglib"
	fi
fi
sed -e "s,__build_arch__,$ARCH,g" -i "" $pkgbin/inkscape

# PkgInfo must match bundle type and creator code from Info.plist
echo "APPLInks" > $package/Contents/PkgInfo

# Pull in extra requirements for Pango and GTK
mkdir -p $pkgetc/pango

# Need to adjust path and quote in case of spaces in path.
sed -e "s,$LIBPREFIX,\"\${CWD},g" -e 's,\.so ,.so" ,g' $LIBPREFIX/etc/pango/pango.modules > $pkgetc/pango/pango.modules
cat > $pkgetc/pango/pangorc <<END_PANGO
[Pango]
ModuleFiles=\${INK_CACHE_DIR}/pango.modules
END_PANGO

# We use a modified fonts.conf file so only need the dtd
mkdir -p $pkgshare/xml/fontconfig
cp $LIBPREFIX/share/xml/fontconfig/fonts.dtd $pkgshare/xml/fontconfig
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

gdk_pixbuf_version=`pkg-config --variable=gdk_pixbuf_binary_version gdk-pixbuf-2.0`
mkdir -p $pkglib/gdk-pixbuf-2.0/$gdk_pixbuf_version/loaders
cp $LIBPREFIX/lib/gdk-pixbuf-2.0/$gdk_pixbuf_version/loaders/*.so $pkglib/gdk-pixbuf-2.0/$gdk_pixbuf_version/loaders/

sed -e "s,__gtk_version__,$gtk_version,g" -i "" $pkgbin/inkscape
sed -e "s,__gdk_pixbuf_version__,$gdk_pixbuf_version,g" -i "" $pkgbin/inkscape
sed -e "s,$LIBPREFIX,\${CWD},g" $LIBPREFIX/lib/gtk-2.0/$gtk_version/immodules.cache > $pkglib/gtk-2.0/$gtk_version/immodules.cache
sed -e "s,$LIBPREFIX,\${CWD},g" $LIBPREFIX/lib/gdk-pixbuf-2.0/$gdk_pixbuf_version/loaders.cache > $pkglib/gdk-pixbuf-2.0/$gdk_pixbuf_version/loaders.cache

# ImageMagick version
IMAGEMAGICKVER="$(pkg-config --modversion ImageMagick)"
IMAGEMAGICKVER_MAJOR="$(cut -d. -f1 <<< "$IMAGEMAGICKVER")"

# ImageMagick data
# include *.la files for main libs too
for item in "$LIBPREFIX/lib/libMagick"*.la; do
    cp "$item" "$pkglib/"
done
# ImageMagick modules
cp -r "$LIBPREFIX/lib/ImageMagick-$IMAGEMAGICKVER" "$pkglib/"
cp -r "$LIBPREFIX/etc/ImageMagick-$IMAGEMAGICKVER_MAJOR" "$pkgetc/"
cp -r "$LIBPREFIX/share/ImageMagick-$IMAGEMAGICKVER_MAJOR" "$pkgshare/"
# REQUIRED: remove hard-coded paths from *.la files
for la_file in "$pkglib/libMagick"*.la; do
    sed -e "s,$LIBPREFIX/lib,,g" -i "" "$la_file"
done
for la_file in "$pkglib/ImageMagick-$IMAGEMAGICKVER/modules-Q16/coders"/*.la; do
    sed -e "s,$LIBPREFIX/lib/ImageMagick-$IMAGEMAGICKVER/modules-Q16/coders,,g" -i "" "$la_file"
done
for la_file in "$pkglib/ImageMagick-$IMAGEMAGICKVER/modules-Q16/filters"/*.la; do
    sed -e "s,$LIBPREFIX/lib/ImageMagick-$IMAGEMAGICKVER/modules-Q16/filters,,g" -i "" "$la_file"
done
sed -e "s,IMAGEMAGICKVER,$IMAGEMAGICKVER,g" -i "" $pkgbin/inkscape
sed -e "s,IMAGEMAGICKVER_MAJOR,$IMAGEMAGICKVER_MAJOR,g" -i "" $pkgbin/inkscape

# Copy aspell dictionary files:
cp -r "$LIBPREFIX/share/aspell" "$pkgresources/share/"

# Copy Poppler data:
cp -r "$LIBPREFIX/share/poppler" "$pkgshare"

# Copy all linked libraries into the bundle
#----------------------------------------------------------
# get list of *.so modules from python modules
python_libs=""
for PYTHON_VER in "2.5" "2.6" "2.7"; do
	python_libs="$python_libs $(find "${pkglib}/python${PYTHON_VER}" -name *.so -or -name *.dylib)"
done

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
                loader_to_res="$(echo $filePath | $awk_cmd -F/ '{for (i=1;i<NF;i++) sub($i,".."); sub($NF,"",$0); print $0}')"
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
                lib_prefix_levels="$(echo $lib | $awk_cmd -F/ '{for (i=NF;i>0;i--) if($i=="lib") j=i; print j}')"
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
