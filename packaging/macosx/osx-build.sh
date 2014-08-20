#!/bin/bash
#
#  Inkscape compilation and packaging script for Mac OS X
#
# Please see
#  http://wiki.inkscape.org/wiki/index.php?title=CompilingMacOsX
# for more complete information
#
# Authors:
#	Jean-Olivier Irisson <jo.irisson@gmail.com>
#       Liam P. White <inkscapebrony@gmail.com>
# with information from
#	Kees Cook
#	Michael Wybrow
#
# Copyright (C) 2006-2010
# Released under GNU GPL, read the file 'COPYING' for more information
#

############################################################

# User modifiable parameters
#----------------------------------------------------------
# Configure flags
CONFFLAGS="--enable-osxapp --enable-localinstall"
# Libraries prefix (Warning: NO trailing slash)
if [ -z "$LIBPREFIX" ]; then
	LIBPREFIX="/opt/local-x11"
fi

############################################################

# Help message
#----------------------------------------------------------
help()
{

echo -e "
Compilation script for Inkscape on Mac OS X.

\033[1mUSAGE\033[0m
  $0 [options] action[s]

\033[1mACTIONS & OPTIONS\033[0m
  \033[1mh,help\033[0m	
    display this help message
  \033[1mu,up,update\033[0m
    update an existing checkout from bzr (run bzr pull)
  \033[1ma,auto,autogen\033[0m
    prepare configure script (run autogen.sh). This is only necessary
    for a fresh bzr checkout or after make distclean.
  \033[1mc,conf,configure\033[0m
    configure the build (run configure). Edit your configuration
    options in $0
    \033[1m-p,--prefix\033[0m	specify install prefix (configure step only)
  \033[1mb,build\033[0m
    build Inkscape (run make)
    \033[1m-j,--jobs\033[0m   Set the number of parallel execution for make
  \033[1mi,install\033[0m
    install the build products locally, inside the source
    directory (run make install)
  \033[1mp,pack,package\033[0m
    package Inkscape in a double clickable .app bundle 
    \033[1m-s,--strip\033[0m	remove debugging information in Inkscape package
    \033[1m-py,--with-python\033[0m	specify python modules path for inclusion into the app bundle
  \033[1md,dist,distrib\033[0m
    store Inkscape.app in a disk image (dmg) for distribution

\033[1mEXAMPLES\033[0m
  \033[1m$0 conf build install\033[0m
    configure, build and install a dowloaded version of Inkscape in the default
    directory, keeping debugging information.	
  \033[1m$0 u a c b -p ~ i -s -py ~/site-packages/ p d\033[0m
    update an bzr checkout, prepare configure script, configure,
    build and install Inkscape in the user home directory (~). 	
    Then package Inkscape without debugging information,
    with python packages from ~/site-packages/ and prepare 
    a dmg for distribution."
}

# Parameters
#----------------------------------------------------------
# Paths
HERE=`pwd`
SRCROOT=$HERE/../..		# we are currently in packaging/macosx

# Defaults
if [ "$INSTALLPREFIX" = "" ]
then
	INSTALLPREFIX=$SRCROOT/inst-osxapp/
fi
BZRUPDATE="f"
AUTOGEN="f"
CONFIGURE="f"
BUILD="f"
NJOBS=1
INSTALL="f"
PACKAGE="f"
DISTRIB="f"
UNIVERSAL="f"

STRIP=""
PYTHON_MODULES=""

# Parse command line options
#----------------------------------------------------------
while [ "$1" != "" ]
do
	case $1 in
	h|help)
		help 
		exit 1 ;;
	all)            
		BZRUPDATE="t"
		CONFIGURE="t"
		BUILD="t" 
		INSTALL="t"
		PACKAGE="t"
		DISTRIB="t" ;;
	u|up|update)
		BZRUPDATE="t" ;;
	a|auto|autogen)
		AUTOGEN="t" ;;
	c|conf|configure)
		CONFIGURE="t" ;;
	b|build)
		BUILD="t" ;;
	-j|--jobs)
		NJOBS=$2
		shift 1 ;;
	i|install)
		INSTALL="t" ;;
	p|pack|package)
		PACKAGE="t" ;;
	d|dist|distrib)
		DISTRIB="t" ;;
	-p|--prefix)
	  	INSTALLPREFIX=$2
	  	shift 1 ;;
	-s|--strip)
	     	STRIP="-s" ;;
	-py|--with-python)
		PYTHON_MODULES="$2"
		shift 1 ;;
	*)
		echo "Invalid command line option: $1" 
		exit 2 ;;
	esac
	shift 1
done

# OSXMINORVER=`/usr/bin/sw_vers | grep ProductVersion | cut -d'	' -f2 | cut -f1-2 -d.`

# Set environment variables
# ----------------------------------------------------------
export LIBPREFIX

# Specific environment variables
#  automake seach path
export CPATH="$LIBPREFIX/include"
#  configure search path
export CPPFLAGS="-I$LIBPREFIX/include"
export LDFLAGS="-L$LIBPREFIX/lib"
#  compiler arguments
export CFLAGS="-pipe -Os"
export CXXFLAGS="$CFLAGS -Wno-cast-align"

# Actions
# ----------------------------------------------------------
if [[ "$BZRUPDATE" == "t" ]]
then
	cd $SRCROOT
	bzr pull
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nBZR update failed"
		exit $status
	fi
	cd $HERE
fi

# Fetch some information
REVISION=$(bzr revno)
ARCH=$(arch)
TARGETVERSION=$(/usr/bin/sw_vers | fgrep ProductVersion | tr -d \  | cut -d: -f2)
TARGETARCH=$ARCH

NEWNAME="Inkscape-r$REVISION-$TARGETVERSION-$TARGETARCH"
DMGFILE="$NEWNAME.dmg"
INFOFILE="$NEWNAME-info.txt"

if [[ "$AUTOGEN" == "t" ]]
then
	cd $SRCROOT
	export NOCONFIGURE=true && ./autogen.sh
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nautogen failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$CONFIGURE" == "t" ]]
then
	ALLCONFFLAGS="$CONFFLAGS --prefix=$INSTALLPREFIX"
	cd $SRCROOT
	if [ ! -f configure ]
	then
		echo "Configure script not found in $SRCROOT. Run '$0 autogen' first"
		exit 1
	fi
        mkdir -p build-osxapp; cd build-osxapp
	../configure $ALLCONFFLAGS
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nConfigure failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$BUILD" == "t" ]]
then
	cd $SRCROOT/build-osxapp
	make -j $NJOBS
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nBuild failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$INSTALL" == "t" ]] 
then
	cd $SRCROOT/build-osxapp
	make install
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nInstall failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$PACKAGE" == "t" ]]
then
	
	# Test the existence of required files
	if [ ! -e $INSTALLPREFIX/bin/inkscape ]
	then
		echo "The inkscape executable \"$INSTALLPREFIX/bin/inkscape\" cound not be found."
		exit 1
	fi
	if [ ! -e $SRCROOT/build-osxapp/Info.plist ]
	then
		echo "The file \"$SRCROOT/build-osxapp/Info.plist\" could not be found, please re-run configure."
		exit 1
	fi
	
	# Set python command line option (if PYTHON_MODULES location is not empty, then add the python call to the command line, otherwise, stay empty)
	if [[ "$PYTHON_MODULES" != "" ]]; then
		PYTHON_MODULES="-py $PYTHON_MODULES"
		# TODO: fix this: it does not allow for spaces in the PATH under this form and cannot be quoted
	fi

	# Create app bundle
	./osx-app.sh $STRIP -b $INSTALLPREFIX/bin/inkscape -p $SRCROOT/build-osxapp/Info.plist $PYTHON_MODULES
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nApplication bundle creation failed"
		exit $status
	fi
fi

function checkversion {
	DEPVER=`pkg-config --modversion $1 2>/dev/null`
	if [[ "$?" == "1" ]]; then
		DEPVER="Not included"
	fi
	echo "$DEPVER"
}


if [[ "$DISTRIB" == "t" ]]
then
	# Create dmg bundle
	./osx-dmg.sh -p "Inkscape.app"
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nDisk image creation failed"
		exit $status
	fi

	mv Inkscape.dmg $DMGFILE
	
	# Prepare information file
	echo "Build information on `date` for `whoami`:
	For OS X Ver  $TARGETNAME ($TARGETVERSION)
	Architecture  $TARGETARCH
Build system information:
	OS X Version  $OSXVERSION
	Architecture  $ARCH
	MacPorts Ver  `port version | cut -f2 -d \ `
	GCC           `$CXX --version | grep GCC`
Included dependency versions:
	GTK           `checkversion gtk+-2.0`
	GTKmm         `checkversion gtkmm-2.4`
	Cairo         `checkversion cairo`
	Cairomm       `checkversion cairomm-1.0`
	CairoPDF      `checkversion cairo-pdf`
	Fontconfig    `checkversion fontconfig`
	Pango         `checkversion pango`
	LibXML2       `checkversion libxml-2.0`
	LibXSLT       `checkversion libxslt`
	LibSigC++     `checkversion sigc++-2.0`
	LibPNG        `checkversion libpng`
	GSL           `checkversion gsl`
	ImageMagick   `checkversion ImageMagick`
	Poppler       `checkversion poppler-cairo`
	LittleCMS     `checkversion lcms`
	GnomeVFS      `checkversion gnome-vfs-2.0`
	LibWPG        `checkversion libwpg-0.2`
Configure options:
	$CONFFLAGS" > $INFOFILE
	if [[ "$STRIP" == "t" ]]; then
		echo "Debug info
	no" >> $INFOFILE
	else
		echo "Debug info
	yes" >> $INFOFILE
	fi	
fi

if [[ "$PACKAGE" == "t" || "$DISTRIB" == "t" ]]; then
	# open a Finder window here to admire what we just produced
	open .
fi

exit 0
