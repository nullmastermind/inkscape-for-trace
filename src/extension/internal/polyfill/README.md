# Gradient Mesh JavaScript polyfill

This directory contains JavaScript "Polyfills" to support rendering of SVG 2
features that are not well supported by browsers, but appeared in the 2016
[specification](https://www.w3.org/TR/2016/CR-SVG2-20160915/pservers.html#MeshGradients)

The included files are:
 - `mesh.js` mesh gradients supporting bicubic meshes and mesh on strokes.
 - `mesh_compressed.include` mesh.js minified and wrapped as a C++11 raw string literal.

## Details
The coding standard used is [semistandard](https://github.com/Flet/semistandard),
a more permissive (allows endrow semicolons) over the famous, open-source
[standardjs](https://standardjs.com/).

The minifier used for the compressed version is [JavaScript minifier](https://javascript-minifier.com/).
