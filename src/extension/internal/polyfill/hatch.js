// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Use patterns to render a hatch paint server via this polyfill
 *//*
 * Copyright (C) 2019 Valentin Ionita
 * Distributed under GNU General Public License version 2 or later. See <http://fsf.org/>.
 */

(function () {
  // Name spaces -----------------------------------
  const svgNS = 'http://www.w3.org/2000/svg';
  const xlinkNS = 'http://www.w3.org/1999/xlink';
  const unitObjectBoundingBox = 'objectBoundingBox';
  const unitUserSpace = 'userSpaceOnUse';

  // Set multiple attributes to an element
  const setAttributes = (el, attrs) => {
    for (let key in attrs) {
      el.setAttribute(key, attrs[key]);
    }
  };

  // Copy attributes from the hatch with 'id' to the current element
  const setReference = (el, id) => {
    const attr = [
      'x', 'y', 'pitch', 'rotate',
      'hatchUnits', 'hatchContentUnits', 'transform'
    ];
    const template = document.getElementById(id.slice(1));

    if (template && template.nodeName === 'hatch') {
      attr.forEach(a => {
        let t = template.getAttribute(a);
        if (el.getAttribute(a) === null && t !== null) {
          el.setAttribute(a, t);
        }
      });

      if (el.children.length === 0) {
        Array.from(template.children).forEach(c => {
          el.appendChild(c.cloneNode(true));
        });
      }
    }
  };

  // Order pain-order of hatchpaths relative to their pitch
  const orderHatchPaths = (paths) => {
    const nodeArray = [];
    paths.forEach(p => nodeArray.push(p));

    return nodeArray.sort((a, b) =>
      // (pitch - a.offset) - (pitch - b.offset)
      Number(b.getAttribute('offset')) - Number(a.getAttribute('offset'))
    );
  };

  // Generate x-axis coordinates for the pattern paths
  const generatePositions = (width, diagonal, initial, distance) => {
    const offset = (diagonal - width) / 2;
    const leftDistance = initial + offset;
    const rightDistance = width + offset + distance;
    const units = Math.round(leftDistance / distance) + 1;
    let array = [];

    for (let i = initial - units * distance; i < rightDistance; i += distance) {
      array.push(i);
    }

    return array;
  };

  // Turn a path array into a tokenized version of it
  const parsePath = (data) => {
    let array = [];
    let i = 0;
    let len = data.length;
    let last = 0;

    /*
     * Last state (last) index map
     * 0 => ()
     * 1 => (x y)
     * 2 => (x)
     * 3 => (y)
     * 4 => (x1 y1 x2 y2 x y)
     * 5 => (x2 y2 x y)
     * 6 => (_ _ _ _ _ x y)
     * 7 => (_)
     */

    while (i < len) {
      switch (data[i].toUpperCase()) {
        case 'Z':
          array.push(data[i]);
          i += 1;
          last = 0;
          break;
        case 'M':
        case 'L':
        case 'T':
          array.push(data[i], new Point(Number(data[i + 1]), Number(data[i + 2])));
          i += 3;
          last = 1;
          break;
        case 'H':
          array.push(data[i], new Point(Number(data[i + 1]), null));
          i += 2;
          last = 2;
          break;
        case 'V':
          array.push(data[i], new Point(null, Number(data[i + 1])));
          i += 2;
          last = 3;
          break;
        case 'C':
          array.push(
            data[i], new Point(Number(data[i + 1]), Number(data[i + 2])),
            new Point(Number(data[i + 3]), Number(data[i + 4])),
            new Point(Number(data[i + 5]), Number(data[i + 6]))
          );
          i += 7;
          last = 4;
          break;
        case 'S':
        case 'Q':
          array.push(
            data[i], new Point(Number(data[i + 1]), Number(data[i + 2])),
            new Point(Number(data[i + 3]), Number(data[i + 4]))
          );
          i += 5;
          last = 5;
          break;
        case 'A':
          array.push(
            data[i], data[i + 1], data[i + 2], data[i + 3], data[i + 4],
            data[i + 5], new Point(Number(data[i + 6]), Number(data[i + 7]))
          );
          i += 8;
          last = 6;
          break;
        case 'B':
          array.push(data[i], data[i + 1]);
          i += 2;
          last = 7;
          break;
        default:
          switch (last) {
            case 1:
              array.push(new Point(Number(data[i]), Number(data[i + 1])));
              i += 2;
              break;
            case 2:
              array.push(new Point(Number(data[i]), null));
              i += 1;
              break;
            case 3:
              array.push(new Point(null, Number(data[i])));
              i += 1;
              break;
            case 4:
              array.push(
                new Point(Number(data[i]), Number(data[i + 1])),
                new Point(Number(data[i + 2]), Number(data[i + 3])),
                new Point(Number(data[i + 4]), Number(data[i + 5]))
              );
              i += 6;
              break;
            case 5:
              array.push(
                new Point(Number(data[i]), Number(data[i + 1])),
                new Point(Number(data[i + 2]), Number(data[i + 3]))
              );
              i += 4;
              break;
            case 6:
              array.push(
                data[i], data[i + 1], data[i + 2], data[i + 3], data[i + 4],
                new Point(Number(data[i + 5]), Number(data[i + 6]))
              );
              i += 7;
              break;
            default:
              array.push(data[i]);
              i += 1;
          }
      }
    }

    return array;
  };

  const getYDistance = (hatchpath) => {
    const path = document.createElementNS(svgNS, 'path');
    let d = hatchpath.getAttribute('d');

    if (d[0].toUpperCase() !== 'M') {
      d = `M 0,0 ${d}`;
    }

    path.setAttribute('d', d);

    return path.getPointAtLength(path.getTotalLength()).y -
      path.getPointAtLength(0).y;
  };

  // Point class --------------------------------------
  class Point {
    constructor (x, y) {
      this.x = x;
      this.y = y;
    }

    toString () {
      return `${this.x} ${this.y}`;
    }

    isPoint () {
      return true;
    }

    clone () {
      return new Point(this.x, this.y);
    }

    add (v) {
      return new Point(this.x + v.x, this.y + v.y);
    }

    distSquared (v) {
      let x = this.x - v.x;
      let y = this.y - v.y;
      return (x * x + y * y);
    }
  }

  // Start of document processing ---------------------
  const shapes = document.querySelectorAll('rect,circle,ellipse,path,text');

  shapes.forEach((shape, i) => {
    // Get id. If no id, create one.
    let shapeId = shape.getAttribute('id');
    if (!shapeId) {
      shapeId = 'hatch_shape_' + i;
      shape.setAttribute('id', shapeId);
    }

    const fill = shape.getAttribute('fill') || shape.style.fill;
    const fillURL = fill.match(/^url\(\s*"?\s*#([^\s"]+)"?\s*\)/);

    if (fillURL && fillURL[1]) {
      const hatch = document.getElementById(fillURL[1]);

      if (hatch && hatch.nodeName === 'hatch') {
        const href = hatch.getAttributeNS(xlinkNS, 'href');

        if (href !== null && href !== '') {
          setReference(hatch, href);
        }

        // Degenerate hatch, with no hatchpath children
        if (hatch.children.length === 0) {
          return;
        }

        const bbox = shape.getBBox();
        const hatchDiag = Math.ceil(Math.sqrt(
          bbox.width * bbox.width + bbox.height * bbox.height
        ));

        // Hatch variables
        const units = hatch.getAttribute('hatchUnits') || unitObjectBoundingBox;
        const contentUnits = hatch.getAttribute('hatchContentUnits') || unitUserSpace;
        const rotate = Number(hatch.getAttribute('rotate')) || 0;
        const transform = hatch.getAttribute('transform') ||
        hatch.getAttribute('hatchTransform') || '';
        const hatchpaths = orderHatchPaths(hatch.querySelectorAll('hatchpath,hatchPath'));
        const x = units === unitObjectBoundingBox
          ? (Number(hatch.getAttribute('x')) * bbox.width) || 0
          : Number(hatch.getAttribute('x')) || 0;
        const y = units === unitObjectBoundingBox
          ? (Number(hatch.getAttribute('y')) * bbox.width) || 0
          : Number(hatch.getAttribute('y')) || 0;
        let pitch = units === unitObjectBoundingBox
          ? (Number(hatch.getAttribute('pitch')) * bbox.width) || 0
          : Number(hatch.getAttribute('pitch')) || 0;

        if (contentUnits === unitObjectBoundingBox && bbox.height) {
          pitch /= bbox.height;
        }

        // A negative value is an error.
        // A value of zero disables rendering of the element
        if (pitch <= 0) {
          console.error('Non-positive pitch');
          return;
        }

        // Pattern variables
        const pattern = document.createElementNS(svgNS, 'pattern');
        const patternId = `${fillURL[1]}_pattern`;
        let patternWidth = bbox.width - bbox.width % pitch;
        let patternHeight = 0;

        const xPositions = generatePositions(patternWidth, hatchDiag, x, pitch);

        hatchpaths.forEach(hatchpath => {
          let offset = Number(hatchpath.getAttribute('offset')) || 0;
          offset = offset > pitch ? (offset % pitch) : offset;
          const currentXPositions = xPositions.map(p => p + offset);

          const path = document.createElementNS(svgNS, 'path');
          let d = '';

          for (let j = 0; j < hatchpath.attributes.length; ++j) {
            const attr = hatchpath.attributes.item(j);
            if (attr.name !== 'd') {
              path.setAttribute(attr.name, attr.value);
            }
          }

          if (hatchpath.getAttribute('d') === null) {
            d += currentXPositions.reduce(
              (acc, xPos) => `${acc}M ${xPos} ${y} V ${hatchDiag} `, ''
            );
            patternHeight = hatchDiag;
          } else {
            const hatchData = hatchpath.getAttribute('d');
            const data = parsePath(
              hatchData.match(/([+-]?(\d+(\.\d+)?))|[MmZzLlHhVvCcSsQqTtAaBb]/g)
            );
            const len = data.length;
            const startsWithM = data[0] === 'M';
            const relative = data[0].toLowerCase() === data[0];
            const point = new Point(0, 0);
            let yOffset = getYDistance(hatchpath);

            if (data[len - 1].y !== undefined && yOffset < data[len - 1].y) {
              yOffset = data[len - 1].y;
            }

            // The offset must be positive
            if (yOffset <= 0) {
              console.error('y offset is non-positive');
              return;
            }
            patternHeight = bbox.height - bbox.height % yOffset;

            const currentYPositions = generatePositions(
              patternHeight, hatchDiag, y, yOffset
            );

            currentXPositions.forEach(xPos => {
              point.x = xPos;

              if (!startsWithM && !relative) {
                d += `M ${xPos} 0`;
              }

              currentYPositions.forEach(yPos => {
                point.y = yPos;

                if (relative) {
                  // Path is relative, set the first point in each path render
                  d += `M ${xPos} ${yPos} ${hatchData}`;
                } else {
                  // Path is absolute, translate every point
                  d += data.map(e => e.isPoint && e.isPoint() ? e.add(point) : e)
                    .map(e => e.isPoint && e.isPoint() ? e.toString() : e)
                    .reduce((acc, e) => `${acc} ${e}`, '');
                }
              });
            });

            // The hatchpaths are infinite, so they have no fill
            path.style.fill = 'none';
          }

          path.setAttribute('d', d);
          pattern.appendChild(path);
        });

        setAttributes(pattern, {
          'id': patternId,
          'patternUnits': unitUserSpace,
          'patternContentUnits': contentUnits,
          'width': patternWidth,
          'height': patternHeight,
          'x': bbox.x,
          'y': bbox.y,
          'patternTransform': `rotate(${rotate} ${0} ${0}) ${transform}`
        });
        hatch.parentElement.insertBefore(pattern, hatch);

        shape.style.fill = `url(#${patternId})`;
        shape.setAttribute('fill', `url(#${patternId})`);
      }
    }
  });
})();
