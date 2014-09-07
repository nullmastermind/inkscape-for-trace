Quick instructions:
===================

1) install MacPorts from source into $MP_PREFIX (e.g. "/opt/local-x11")
   <https://www.macports.org/install.php>

2) add MacPorts to your PATH environement variable, for example:

$ export PATH="$MP_PREFIX/bin:$MP_PREFIX/sbin:$PATH"

3) add 'ports/' subdirectory as local portfile repository:

$ sed -e '/^rsync:/i\'$'\n'"file://$(pwd)/ports" -i "" foo.conf

4) index the new local portfile repository: 

$ (cd ports && portindex)

5) install required dependencies: 

$ sudo port install inkscape-packaging

6) compile inkscape, create app bundle and DMG: 

$ LIBPREFIX="$MP_PREFIX" ./osx-build.sh a c b -j 5 i p -s d

7) upload the DMG.
