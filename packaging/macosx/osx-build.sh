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
#	Liam P. White <inkscapebrony@gmail.com>
#	~suv <suv-sf@users.sourceforge.net>
# with information from
#	Kees Cook
#	Michael Wybrow
#
# Copyright (C) 2006-2014
# Released under GNU GPL, read the file 'COPYING' for more information
#

############################################################

# User modifiable parameters
#----------------------------------------------------------
# Configure flags
CONFFLAGS="--enable-osxapp"
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
  \033[1m$0 u a c b -p ~ i -s -py ~/python_modules/ p d\033[0m
    update an bzr checkout, prepare configure script, configure,
    build and install Inkscape in the user home directory (~). 	
    Then package Inkscape without debugging information,
    with python packages from ~/python_modules/ and prepare 
    a dmg for distribution."
}

# Parameters
#----------------------------------------------------------
# Paths
HERE="$(pwd)"
SRCROOT="$(cd ../.. && pwd)"	# we are currently in packaging/macosx

# Defaults
if [ -z "$BUILDPREFIX" ]; then
	BUILDPREFIX="$SRCROOT/build-osxapp/"
fi
if [ -z "$INSTALLPREFIX" ]; then
	INSTALLPREFIX="$SRCROOT/inst-osxapp/"
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

# OS X version
# ----------------------------------------------------------
OSXVERSION="$(/usr/bin/sw_vers | grep ProductVersion | cut -f2)"
OSXMINORVER="$(cut -d. -f 1,2 <<< $OSXVERSION)"
OSXMINORNO="$(cut -d. -f2 <<< $OSXVERSION)"
OSXPOINTNO="$(cut -d. -f3 <<< $OSXVERSION)"
ARCH="$(uname -a | awk '{print $NF;}')"

# Set environment variables
# ----------------------------------------------------------
export LIBPREFIX

# Specific environment variables
#  automake seach path
export CPATH="$LIBPREFIX/include"
#  configure search path
export CPPFLAGS="$CPPFLAGS -I$LIBPREFIX/include"
export LDFLAGS="$LDFLAGS -L$LIBPREFIX/lib"
#  compiler arguments
export CFLAGS="$CFLAGS -pipe -Os"

# Use system compiler and compiler flags which are known to work:
if [ "$OSXMINORNO" -le "4" ]; then
	echo "Note: Inkscape packaging requires Mac OS X 10.5 Leopard or later."
	exit 1
elif [ "$OSXMINORNO" -eq "5" ]; then
	## Apple's GCC 4.2.1 on Leopard
	TARGETNAME="LEOPARD"
	TARGETVERSION="10.5"
	export CC="/usr/bin/gcc-4.2"
	export CXX="/usr/bin/g++-4.2"
	export CLAGS="$CFLAGS -arch $ARCH"
	export CXXFLAGS="$CFLAGS"
	CONFFLAGS="--disable-openmp $CONFFLAGS"
elif [ "$OSXMINORNO" -eq "6" ]; then
	## Apple's LLVM-GCC 4.2.1 on Snow Leopard
	TARGETNAME="SNOW LEOPARD"
	TARGETVERSION="10.6"
	export CC="/usr/bin/llvm-gcc-4.2"
	export CXX="/usr/bin/llvm-g++-4.2"
	export CLAGS="$CFLAGS -arch $ARCH"
	export CXXFLAGS="$CFLAGS"
	CONFFLAGS="--disable-openmp $CONFFLAGS"
elif [ "$OSXMINORNO" -eq "7" ]; then
	## Apple's clang on Lion and later
	TARGETNAME="LION"
	TARGETVERSION="10.7"
	export CC="/usr/bin/clang"
	export CXX="/usr/bin/clang++"
	export CLAGS="$CFLAGS -arch $ARCH"
	export CXXFLAGS="$CFLAGS -Wno-mismatched-tags -Wno-cast-align" #-stdlib=libstdc++ -std=c++11
elif [ "$OSXMINORNO" -eq "8" ]; then
	## Apple's clang on Mountain Lion
	TARGETNAME="MOUTAIN LION"
	TARGETVERSION="10.8"
	export CC="/usr/bin/clang"
	export CXX="/usr/bin/clang++"
	export CLAGS="$CFLAGS -arch $ARCH"
	export CXXFLAGS="$CFLAGS -Wno-mismatched-tags -Wno-cast-align -std=c++11 -stdlib=libstdc++"
elif [ "$OSXMINORNO" -eq "9" ]; then
	## Apple's clang on Mavericks
	TARGETNAME="MAVERICKS"
	TARGETVERSION="10.9"
	export CC="/usr/bin/clang"
	export CXX="/usr/bin/clang++"
	export CLAGS="$CFLAGS -arch $ARCH"
	export CXXFLAGS="$CLAGS -Wno-mismatched-tags -Wno-cast-align -std=c++11 -stdlib=libc++"
elif [ "$OSXMINORNO" -eq "10" ]; then
	## Apple's clang on Yosemite
	TARGETNAME="YOSEMITE"
	TARGETVERSION="10.10"
	export CC="/usr/bin/clang"
	export CXX="/usr/bin/clang++"
	export CLAGS="$CFLAGS -arch $ARCH"
	export CXXFLAGS="$CLAGS -Wno-mismatched-tags -Wno-cast-align -std=c++11 -stdlib=libc++"
	echo "Note: Detected version of OS X: $TARGETNAME $OSXVERSION"
	echo "      Inkscape packaging has not been tested on ${TARGETNAME}."
else # if [ "$OSXMINORNO" -ge "11" ]; then
	## Apple's clang after Yosemite?
	TARGETNAME="UNKNOWN"
	TARGETVERSION="10.XX"
	export CC="/usr/bin/clang"
	export CXX="/usr/bin/clang++"
	export CLAGS="$CFLAGS -arch $ARCH"
	export CXXFLAGS="$CLAGS -Wno-mismatched-tags -Wno-cast-align -std=c++11 -stdlib=libc++"
	echo "Note: Detected version of OS X: $TARGETNAME $OSXVERSION"
	echo "      Inkscape packaging has not been tested on this unknown version of OS X (${OSXVERSION})."
fi

# Utility functions
# ----------------------------------------------------------
function getinkscapeinfo () {

	osxapp_domain="$BUILDPREFIX/Info"
	INKVERSION="$(defaults read $osxapp_domain CFBundleVersion)"
	[ $? -ne 0 ] && INKVERSION="devel"
	REVISION="$(bzr revno)"
	[ $? -ne 0 ] && REVISION="" || REVISION="-r$REVISION"

	gtk_target=`pkg-config --variable=target gtk+-2.0 2>/dev/null`

	TARGETARCH="$ARCH"
	NEWNAME="Inkscape-$INKVERSION$REVISION-$gtk_target-$TARGETVERSION-$TARGETARCH"
	DMGFILE="$NEWNAME.dmg"
	INFOFILE="$NEWNAME-info.txt"

}

# Actions
# ----------------------------------------------------------
if [[ "$BZRUPDATE" == "t" ]]
then
	cd $SRCROOT
	if [ -z "$(bzr info | grep "checkout")" ]; then
		echo "repo is unbound (branch)"
		update_cmd="bzr pull"
	else
		echo "repo is bound (checkout)"
		update_cmd="bzr update"
	fi
	$update_cmd
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nBZR update failed"
		exit $status
	fi
	cd $HERE
fi

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
	ALLCONFFLAGS="$CONFFLAGS --prefix=$INSTALLPREFIX --enable-localinstall"
	cd $SRCROOT
	if [ ! -d $BUILDPREFIX ]
	then
		mkdir $BUILDPREFIX || exit 1
	fi
	cd $BUILDPREFIX
	if [ ! -f $SRCROOT/configure ]
	then
		echo "Configure script not found in $SRCROOT. Run '$0 autogen' first"
		exit 1
	fi
	$SRCROOT/configure $ALLCONFFLAGS
	status=$?
	if [[ $status -ne 0 ]]; then
		echo -e "\nConfigure failed"
		exit $status
	fi
	cd $HERE
fi

if [[ "$BUILD" == "t" ]]
then
	cd $BUILDPREFIX || exit 1
	touch "$SRCROOT/src/main.cpp" "$SRCROOT/src/ui/dialog/aboutbox.cpp"
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
	cd $BUILDPREFIX || exit 1
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
	if [ ! -e $BUILDPREFIX/Info.plist ]
	then
		echo "The file \"$BUILDPREFIX/Info.plist\" could not be found, please re-run configure."
		exit 1
	fi
	
	# Set python command line option (if PYTHON_MODULES location is not empty, then add the python call to the command line, otherwise, stay empty)
	if [[ "$PYTHON_MODULES" != "" ]]; then
		PYTHON_MODULES="-py $PYTHON_MODULES"
		# TODO: fix this: it does not allow for spaces in the PATH under this form and cannot be quoted
	fi

	# Create app bundle
	./osx-app.sh $STRIP -b $INSTALLPREFIX/bin/inkscape -p $BUILDPREFIX/Info.plist $PYTHON_MODULES
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
	getinkscapeinfo
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
	GCC           `$CXX --version | head -1`
	GTK+ backend  $gtk_target
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
