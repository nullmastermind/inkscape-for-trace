// SPDX-License-Identifier: GPL-2.0-or-later
// Use Canvas to render a mesh gradient, passing the rendering to an image via a data stream.
// Copyright: Tavmjong Bah 2018
// Contributor: Valentin Ionita 2019
// Distributed under GNU General Public License version 2 or later. See <http://fsf.org/>.

(function () {
  // Name spaces -----------------------------------
  const svgNS = 'http://www.w3.org/2000/svg';
  const xlinkNS = 'http://www.w3.org/1999/xlink';
  const xhtmlNS = 'http://www.w3.org/1999/xhtml';
  /*
   * Maximum threshold for Bezier step size
   * Larger values leave holes, smaller take longer to render.
   */
  const maxBezierStep = 2.0;

  // Test if mesh gradients are supported.
  if (document.createElementNS(svgNS, 'meshgradient').x) {
    return;
  }

  /*
   * Utility functions -----------------------------
   */
  // Split Bezier using de Casteljau's method.
  const splitBezier = (p0, p1, p2, p3) => {
    let tmp = new Point((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5);
    let p01 = new Point((p0.x + p1.x) * 0.5, (p0.y + p1.y) * 0.5);
    let p12 = new Point((p2.x + p3.x) * 0.5, (p2.y + p3.y) * 0.5);
    let p02 = new Point((tmp.x + p01.x) * 0.5, (tmp.y + p01.y) * 0.5);
    let p11 = new Point((tmp.x + p12.x) * 0.5, (tmp.y + p12.y) * 0.5);
    let p03 = new Point((p02.x + p11.x) * 0.5, (p02.y + p11.y) * 0.5);

    return ([
      [p0, p01, p02, p03],
      [p03, p11, p12, p3]
    ]);
  };

  // See Cairo: cairo-mesh-pattern-rasterizer.c
  const bezierStepsSquared = (points) => {
    let tmp0 = points[0].distSquared(points[1]);
    let tmp1 = points[2].distSquared(points[3]);
    let tmp2 = points[0].distSquared(points[2]) * 0.25;
    let tmp3 = points[1].distSquared(points[3]) * 0.25;

    let max1 = tmp0 > tmp1 ? tmp0 : tmp1;

    let max2 = tmp2 > tmp3 ? tmp2 : tmp3;

    let max = max1 > max2 ? max1 : max2;

    return max * 18;
  };

  // Euclidean distance
  const distance = (p0, p1) => Math.sqrt(p0.distSquared(p1));

  // Weighted average to find Bezier points for linear sides.
  const wAvg = (p0, p1) => p0.scale(2.0 / 3.0).add(p1.scale(1.0 / 3.0));

  // Browsers return a string rather than a transform list for gradientTransform!
  const parseTransform = (t) => {
    let affine = new Affine();
    let trans, scale, radian, tan, skewx, skewy, rotate;
    let transforms = t.match(/(\w+\(\s*[^)]+\))+/g);

    transforms.forEach((i) => {
      let c = i.match(/[\w.-]+/g);
      let type = c.shift();

      switch (type) {
        case 'translate':
          if (c.length === 2) {
            trans = new Affine(1, 0, 0, 1, c[0], c[1]);
          } else {
            console.error('mesh.js: translate does not have 2 arguments!');
            trans = new Affine(1, 0, 0, 1, 0, 0);
          }
          affine = affine.append(trans);
          break;

        case 'scale':
          if (c.length === 1) {
            scale = new Affine(c[0], 0, 0, c[0], 0, 0);
          } else if (c.length === 2) {
            scale = new Affine(c[0], 0, 0, c[1], 0, 0);
          } else {
            console.error('mesh.js: scale does not have 1 or 2 arguments!');
            scale = new Affine(1, 0, 0, 1, 0, 0);
          }
          affine = affine.append(scale);
          break;

        case 'rotate':
          if (c.length === 3) {
            trans = new Affine(1, 0, 0, 1, c[1], c[2]);
            affine = affine.append(trans);
          }
          if (c[0]) {
            radian = c[0] * Math.PI / 180.0;
            let cos = Math.cos(radian);
            let sin = Math.sin(radian);
            if (Math.abs(cos) < 1e-16) { // I hate rounding errors...
              cos = 0;
            }
            if (Math.abs(sin) < 1e-16) { // I hate rounding errors...
              sin = 0;
            }
            rotate = new Affine(cos, sin, -sin, cos, 0, 0);
            affine = affine.append(rotate);
          } else {
            console.error('math.js: No argument to rotate transform!');
          }
          if (c.length === 3) {
            trans = new Affine(1, 0, 0, 1, -c[1], -c[2]);
            affine = affine.append(trans);
          }
          break;

        case 'skewX':
          if (c[0]) {
            radian = c[0] * Math.PI / 180.0;
            tan = Math.tan(radian);
            skewx = new Affine(1, 0, tan, 1, 0, 0);
            affine = affine.append(skewx);
          } else {
            console.error('math.js: No argument to skewX transform!');
          }
          break;

        case 'skewY':
          if (c[0]) {
            radian = c[0] * Math.PI / 180.0;
            tan = Math.tan(radian);
            skewy = new Affine(1, tan, 0, 1, 0, 0);
            affine = affine.append(skewy);
          } else {
            console.error('math.js: No argument to skewY transform!');
          }
          break;

        case 'matrix':
          if (c.length === 6) {
            affine = affine.append(new Affine(...c));
          } else {
            console.error('math.js: Incorrect number of arguments for matrix!');
          }
          break;

        default:
          console.error('mesh.js: Unhandled transform type: ' + type);
          break;
      }
    });

    return affine;
  };

  const parsePoints = (s) => {
    let points = [];
    let values = s.split(/[ ,]+/);
    for (let i = 0, imax = values.length - 1; i < imax; i += 2) {
      points.push(new Point(parseFloat(values[i]), parseFloat(values[i + 1])));
    }
    return points;
  };

  // Set multiple attributes to an element
  const setAttributes = (el, attrs) => {
    for (let key in attrs) {
      el.setAttribute(key, attrs[key]);
    }
  };

  // Find the slope of point p_k by the values in p_k-1 and p_k+1
  const finiteDifferences = (c0, c1, c2, d01, d12) => {
    let slope = [0, 0, 0, 0];
    let slow, shigh;

    for (let k = 0; k < 3; ++k) {
      if ((c1[k] < c0[k] && c1[k] < c2[k]) || (c0[k] < c1[k] && c2[k] < c1[k])) {
        slope[k] = 0;
      } else {
        slope[k] = 0.5 * ((c1[k] - c0[k]) / d01 + (c2[k] - c1[k]) / d12);
        slow = Math.abs(3.0 * (c1[k] - c0[k]) / d01);
        shigh = Math.abs(3.0 * (c2[k] - c1[k]) / d12);

        if (slope[k] > slow) {
          slope[k] = slow;
        } else if (slope[k] > shigh) {
          slope[k] = shigh;
        }
      }
    }

    return slope;
  };

  // Coefficient matrix used for solving linear system
  const A = [
    [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    [-3, 3, 0, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    [2, -2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0, -3, 3, 0, 0, -2, -1, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 1, 1, 0, 0],
    [-3, 0, 3, 0, 0, 0, 0, 0, -2, 0, -1, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, -3, 0, 3, 0, 0, 0, 0, 0, -2, 0, -1, 0],
    [9, -9, -9, 9, 6, 3, -6, -3, 6, -6, 3, -3, 4, 2, 2, 1],
    [-6, 6, 6, -6, -3, -3, 3, 3, -4, 4, -2, 2, -2, -2, -1, -1],
    [2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 2, 0, -2, 0, 0, 0, 0, 0, 1, 0, 1, 0],
    [-6, 6, 6, -6, -4, -2, 4, 2, -3, 3, -3, 3, -2, -1, -2, -1],
    [4, -4, -4, 4, 2, 2, -2, -2, 2, -2, 2, -2, 1, 1, 1, 1]
  ];

  // Solve the linear system for bicubic interpolation
  const solveLinearSystem = (v) => {
    let alpha = [];

    for (let i = 0; i < 16; ++i) {
      alpha[i] = 0;
      for (let j = 0; j < 16; ++j) {
        alpha[i] += A[i][j] * v[j];
      }
    }

    return alpha;
  };

  // Evaluate the interpolation parameters at (y, x)
  const evaluateSolution = (alpha, x, y) => {
    const xx = x * x;
    const yy = y * y;
    const xxx = x * x * x;
    const yyy = y * y * y;

    let result =
      alpha[0] +
      alpha[1] * x +
      alpha[2] * xx +
      alpha[3] * xxx +
      alpha[4] * y +
      alpha[5] * y * x +
      alpha[6] * y * xx +
      alpha[7] * y * xxx +
      alpha[8] * yy +
      alpha[9] * yy * x +
      alpha[10] * yy * xx +
      alpha[11] * yy * xxx +
      alpha[12] * yyy +
      alpha[13] * yyy * x +
      alpha[14] * yyy * xx +
      alpha[15] * yyy * xxx;

    return result;
  };

  // Split a patch into 8x8 smaller patches
  const splitPatch = (patch) => {
    let yPatches = [];
    let xPatches = [];
    let patches = [];

    // Horizontal splitting
    for (let i = 0; i < 4; ++i) {
      yPatches[i] = [];
      yPatches[i][0] = splitBezier(
        patch[0][i], patch[1][i],
        patch[2][i], patch[3][i]
      );

      yPatches[i][1] = [];
      yPatches[i][1].push(...splitBezier(...yPatches[i][0][0]));
      yPatches[i][1].push(...splitBezier(...yPatches[i][0][1]));

      yPatches[i][2] = [];
      yPatches[i][2].push(...splitBezier(...yPatches[i][1][0]));
      yPatches[i][2].push(...splitBezier(...yPatches[i][1][1]));
      yPatches[i][2].push(...splitBezier(...yPatches[i][1][2]));
      yPatches[i][2].push(...splitBezier(...yPatches[i][1][3]));
    }

    // Vertical splitting
    for (let i = 0; i < 8; ++i) {
      xPatches[i] = [];

      for (let j = 0; j < 4; ++j) {
        xPatches[i][j] = [];
        xPatches[i][j][0] = splitBezier(
          yPatches[0][2][i][j], yPatches[1][2][i][j],
          yPatches[2][2][i][j], yPatches[3][2][i][j]
        );

        xPatches[i][j][1] = [];
        xPatches[i][j][1].push(...splitBezier(...xPatches[i][j][0][0]));
        xPatches[i][j][1].push(...splitBezier(...xPatches[i][j][0][1]));

        xPatches[i][j][2] = [];
        xPatches[i][j][2].push(...splitBezier(...xPatches[i][j][1][0]));
        xPatches[i][j][2].push(...splitBezier(...xPatches[i][j][1][1]));
        xPatches[i][j][2].push(...splitBezier(...xPatches[i][j][1][2]));
        xPatches[i][j][2].push(...splitBezier(...xPatches[i][j][1][3]));
      }
    }

    for (let i = 0; i < 8; ++i) {
      patches[i] = [];

      for (let j = 0; j < 8; ++j) {
        patches[i][j] = [];

        patches[i][j][0] = xPatches[i][0][2][j];
        patches[i][j][1] = xPatches[i][1][2][j];
        patches[i][j][2] = xPatches[i][2][2][j];
        patches[i][j][3] = xPatches[i][3][2][j];
      }
    }

    return patches;
  };

  // Point class -----------------------------------
  class Point {
    constructor (x, y) {
      this.x = x || 0;
      this.y = y || 0;
    }

    toString () {
      return `(x=${this.x}, y=${this.y})`;
    }

    clone () {
      return new Point(this.x, this.y);
    }

    add (v) {
      return new Point(this.x + v.x, this.y + v.y);
    }

    scale (v) {
      if (v.x === undefined) {
        return new Point(this.x * v, this.y * v);
      }
      return new Point(this.x * v.x, this.y * v.y);
    }

    distSquared (v) {
      let x = this.x - v.x;
      let y = this.y - v.y;
      return (x * x + y * y);
    }

    // Transform by affine
    transform (affine) {
      let x = this.x * affine.a + this.y * affine.c + affine.e;
      let y = this.x * affine.b + this.y * affine.d + affine.f;
      return new Point(x, y);
    }
  }

  /*
   * Affine class -------------------------------------
   *
   * As defined in the SVG spec
   * | a  c  e |
   * | b  d  f |
   * | 0  0  1 |
   *
   */

  class Affine {
    constructor (a, b, c, d, e, f) {
      if (a === undefined) {
        this.a = 1;
        this.b = 0;
        this.c = 0;
        this.d = 1;
        this.e = 0;
        this.f = 0;
      } else {
        this.a = a;
        this.b = b;
        this.c = c;
        this.d = d;
        this.e = e;
        this.f = f;
      }
    }

    toString () {
      return `affine: ${this.a} ${this.c} ${this.e} \n\
       ${this.b} ${this.d} ${this.f}`;
    }

    append (v) {
      if (!(v instanceof Affine)) {
        console.error(`mesh.js: argument to Affine.append is not affine!`);
      }
      let a = this.a * v.a + this.c * v.b;
      let b = this.b * v.a + this.d * v.b;
      let c = this.a * v.c + this.c * v.d;
      let d = this.b * v.c + this.d * v.d;
      let e = this.a * v.e + this.c * v.f + this.e;
      let f = this.b * v.e + this.d * v.f + this.f;
      return new Affine(a, b, c, d, e, f);
    }
  }

  // Curve class --------------------------------------
  class Curve {
    constructor (nodes, colors) {
      this.nodes = nodes; // 4 Bezier points
      this.colors = colors; // 2 x 4 colors (two ends x R+G+B+A)
    }

    /*
     * Paint a Bezier curve
     * w is canvas.width
     * h is canvas.height
     */
    paintCurve (v, w) {
      // If inside, see if we need to split
      if (bezierStepsSquared(this.nodes) > maxBezierStep) {
        const beziers = splitBezier(...this.nodes);
        // ([start][end])
        let colors0 = [[], []];
        let colors1 = [[], []];

        /*
         * Linear horizontal interpolation of the middle value for every
         * patch exceeding thereshold
         */
        for (let i = 0; i < 4; ++i) {
          colors0[0][i] = this.colors[0][i];
          colors0[1][i] = (this.colors[0][i] + this.colors[1][i]) / 2;
          colors1[0][i] = colors0[1][i];
          colors1[1][i] = this.colors[1][i];
        }
        let curve0 = new Curve(beziers[0], colors0);
        let curve1 = new Curve(beziers[1], colors1);
        curve0.paintCurve(v, w);
        curve1.paintCurve(v, w);
      } else {
        // Directly write data
        let x = Math.round(this.nodes[0].x);
        if (x >= 0 && x < w) {
          let index = (~~this.nodes[0].y * w + x) * 4;
          v[index] = Math.round(this.colors[0][0]);
          v[index + 1] = Math.round(this.colors[0][1]);
          v[index + 2] = Math.round(this.colors[0][2]);
          v[index + 3] = Math.round(this.colors[0][3]); // Alpha
        }
      }
    }
  }

  // Patch class -------------------------------------
  class Patch {
    constructor (nodes, colors) {
      this.nodes = nodes; // 4x4 array of points
      this.colors = colors; // 2x2x4 colors (four corners x R+G+B+A)
    }

    // Split patch horizontally into two patches.
    split () {
      let nodes0 = [[], [], [], []];
      let nodes1 = [[], [], [], []];
      let colors0 = [
        [[], []],
        [[], []]
      ];
      let colors1 = [
        [[], []],
        [[], []]
      ];

      for (let i = 0; i < 4; ++i) {
        const beziers = splitBezier(
          this.nodes[0][i], this.nodes[1][i],
          this.nodes[2][i], this.nodes[3][i]
        );

        nodes0[0][i] = beziers[0][0];
        nodes0[1][i] = beziers[0][1];
        nodes0[2][i] = beziers[0][2];
        nodes0[3][i] = beziers[0][3];
        nodes1[0][i] = beziers[1][0];
        nodes1[1][i] = beziers[1][1];
        nodes1[2][i] = beziers[1][2];
        nodes1[3][i] = beziers[1][3];
      }

      /*
       * Linear vertical interpolation of the middle value for every
       * patch exceeding thereshold
       */
      for (let i = 0; i < 4; ++i) {
        colors0[0][0][i] = this.colors[0][0][i];
        colors0[0][1][i] = this.colors[0][1][i];
        colors0[1][0][i] = (this.colors[0][0][i] + this.colors[1][0][i]) / 2;
        colors0[1][1][i] = (this.colors[0][1][i] + this.colors[1][1][i]) / 2;
        colors1[0][0][i] = colors0[1][0][i];
        colors1[0][1][i] = colors0[1][1][i];
        colors1[1][0][i] = this.colors[1][0][i];
        colors1[1][1][i] = this.colors[1][1][i];
      }

      return ([new Patch(nodes0, colors0), new Patch(nodes1, colors1)]);
    }

    paint (v, w) {
      // Check if we need to split
      let larger = false;
      let step;
      for (let i = 0; i < 4; ++i) {
        step = bezierStepsSquared([
          this.nodes[0][i], this.nodes[1][i],
          this.nodes[2][i], this.nodes[3][i]
        ]);

        if (step > maxBezierStep) {
          larger = true;
          break;
        }
      }

      if (larger) {
        let patches = this.split();
        patches[0].paint(v, w);
        patches[1].paint(v, w);
      } else {
        /*
         * Paint a Bezier curve using just the top of the patch. If
         * the patch is thin enough this should work. We leave this
         * function here in case we want to do something more fancy.
         */
        let curve = new Curve([...this.nodes[0]], [...this.colors[0]]);
        curve.paintCurve(v, w);
      }
    }
  }

  // Mesh class ---------------------------------------
  class Mesh {
    constructor (mesh) {
      this.readMesh(mesh);
      this.type = mesh.getAttribute('type') || 'bilinear';
    }

    // Function to parse an SVG mesh and set the nodes (points) and colors
    readMesh (mesh) {
      let nodes = [[]];
      let colors = [[]];

      let x = Number(mesh.getAttribute('x'));
      let y = Number(mesh.getAttribute('y'));
      nodes[0][0] = new Point(x, y);

      let rows = mesh.children;
      for (let i = 0, imax = rows.length; i < imax; ++i) {
        // Need to validate if meshrow...
        nodes[3 * i + 1] = []; // Need three extra rows for each meshrow.
        nodes[3 * i + 2] = [];
        nodes[3 * i + 3] = [];
        colors[i + 1] = []; // Need one more row than number of meshrows.

        let patches = rows[i].children;
        for (let j = 0, jmax = patches.length; j < jmax; ++j) {
          let stops = patches[j].children;
          for (let k = 0, kmax = stops.length; k < kmax; ++k) {
            let l = k;
            if (i !== 0) {
              ++l; // There is no top if row isn't first row.
            }
            let path = stops[k].getAttribute('path');
            let parts;

            let type = 'l'; // We need to still find mid-points even if no path.
            if (path != null) {
              parts = path.match(/\s*([lLcC])\s*(.*)/);
              type = parts[1];
            }
            let stopNodes = parsePoints(parts[2]);

            switch (type) {
              case 'l':
                if (l === 0) { // Top
                  nodes[3 * i][3 * j + 3] = stopNodes[0].add(nodes[3 * i][3 * j]);
                  nodes[3 * i][3 * j + 1] = wAvg(nodes[3 * i][3 * j], nodes[3 * i][3 * j + 3]);
                  nodes[3 * i][3 * j + 2] = wAvg(nodes[3 * i][3 * j + 3], nodes[3 * i][3 * j]);
                } else if (l === 1) { // Right
                  nodes[3 * i + 3][3 * j + 3] = stopNodes[0].add(nodes[3 * i][3 * j + 3]);
                  nodes[3 * i + 1][3 * j + 3] = wAvg(nodes[3 * i][3 * j + 3], nodes[3 * i + 3][3 * j + 3]);
                  nodes[3 * i + 2][3 * j + 3] = wAvg(nodes[3 * i + 3][3 * j + 3], nodes[3 * i][3 * j + 3]);
                } else if (l === 2) { // Bottom
                  if (j === 0) {
                    nodes[3 * i + 3][3 * j + 0] = stopNodes[0].add(nodes[3 * i + 3][3 * j + 3]);
                  }
                  nodes[3 * i + 3][3 * j + 1] = wAvg(nodes[3 * i + 3][3 * j], nodes[3 * i + 3][3 * j + 3]);
                  nodes[3 * i + 3][3 * j + 2] = wAvg(nodes[3 * i + 3][3 * j + 3], nodes[3 * i + 3][3 * j]);
                } else { // Left
                  nodes[3 * i + 1][3 * j] = wAvg(nodes[3 * i][3 * j], nodes[3 * i + 3][3 * j]);
                  nodes[3 * i + 2][3 * j] = wAvg(nodes[3 * i + 3][3 * j], nodes[3 * i][3 * j]);
                }
                break;
              case 'L':
                if (l === 0) { // Top
                  nodes[3 * i][3 * j + 3] = stopNodes[0];
                  nodes[3 * i][3 * j + 1] = wAvg(nodes[3 * i][3 * j], nodes[3 * i][3 * j + 3]);
                  nodes[3 * i][3 * j + 2] = wAvg(nodes[3 * i][3 * j + 3], nodes[3 * i][3 * j]);
                } else if (l === 1) { // Right
                  nodes[3 * i + 3][3 * j + 3] = stopNodes[0];
                  nodes[3 * i + 1][3 * j + 3] = wAvg(nodes[3 * i][3 * j + 3], nodes[3 * i + 3][3 * j + 3]);
                  nodes[3 * i + 2][3 * j + 3] = wAvg(nodes[3 * i + 3][3 * j + 3], nodes[3 * i][3 * j + 3]);
                } else if (l === 2) { // Bottom
                  if (j === 0) {
                    nodes[3 * i + 3][3 * j + 0] = stopNodes[0];
                  }
                  nodes[3 * i + 3][3 * j + 1] = wAvg(nodes[3 * i + 3][3 * j], nodes[3 * i + 3][3 * j + 3]);
                  nodes[3 * i + 3][3 * j + 2] = wAvg(nodes[3 * i + 3][3 * j + 3], nodes[3 * i + 3][3 * j]);
                } else { // Left
                  nodes[3 * i + 1][3 * j] = wAvg(nodes[3 * i][3 * j], nodes[3 * i + 3][3 * j]);
                  nodes[3 * i + 2][3 * j] = wAvg(nodes[3 * i + 3][3 * j], nodes[3 * i][3 * j]);
                }
                break;
              case 'c':
                if (l === 0) { // Top
                  nodes[3 * i][3 * j + 1] = stopNodes[0].add(nodes[3 * i][3 * j]);
                  nodes[3 * i][3 * j + 2] = stopNodes[1].add(nodes[3 * i][3 * j]);
                  nodes[3 * i][3 * j + 3] = stopNodes[2].add(nodes[3 * i][3 * j]);
                } else if (l === 1) { // Right
                  nodes[3 * i + 1][3 * j + 3] = stopNodes[0].add(nodes[3 * i][3 * j + 3]);
                  nodes[3 * i + 2][3 * j + 3] = stopNodes[1].add(nodes[3 * i][3 * j + 3]);
                  nodes[3 * i + 3][3 * j + 3] = stopNodes[2].add(nodes[3 * i][3 * j + 3]);
                } else if (l === 2) { // Bottom
                  nodes[3 * i + 3][3 * j + 2] = stopNodes[0].add(nodes[3 * i + 3][3 * j + 3]);
                  nodes[3 * i + 3][3 * j + 1] = stopNodes[1].add(nodes[3 * i + 3][3 * j + 3]);
                  if (j === 0) {
                    nodes[3 * i + 3][3 * j + 0] = stopNodes[2].add(nodes[3 * i + 3][3 * j + 3]);
                  }
                } else { // Left
                  nodes[3 * i + 2][3 * j] = stopNodes[0].add(nodes[3 * i + 3][3 * j]);
                  nodes[3 * i + 1][3 * j] = stopNodes[1].add(nodes[3 * i + 3][3 * j]);
                }
                break;
              case 'C':
                if (l === 0) { // Top
                  nodes[3 * i][3 * j + 1] = stopNodes[0];
                  nodes[3 * i][3 * j + 2] = stopNodes[1];
                  nodes[3 * i][3 * j + 3] = stopNodes[2];
                } else if (l === 1) { // Right
                  nodes[3 * i + 1][3 * j + 3] = stopNodes[0];
                  nodes[3 * i + 2][3 * j + 3] = stopNodes[1];
                  nodes[3 * i + 3][3 * j + 3] = stopNodes[2];
                } else if (l === 2) { // Bottom
                  nodes[3 * i + 3][3 * j + 2] = stopNodes[0];
                  nodes[3 * i + 3][3 * j + 1] = stopNodes[1];
                  if (j === 0) {
                    nodes[3 * i + 3][3 * j + 0] = stopNodes[2];
                  }
                } else { // Left
                  nodes[3 * i + 2][3 * j] = stopNodes[0];
                  nodes[3 * i + 1][3 * j] = stopNodes[1];
                }
                break;
              default:
                console.error('mesh.js: ' + type + ' invalid path type.');
            }

            if ((i === 0 && j === 0) || k > 0) {
              let colorRaw = window.getComputedStyle(stops[k]).stopColor
                .match(/^rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/i);
              let alphaRaw = window.getComputedStyle(stops[k]).stopOpacity;
              let alpha = 255;
              if (alphaRaw) {
                alpha = Math.floor(alphaRaw * 255);
              }

              if (colorRaw) {
                if (l === 0) { // upper left corner
                  colors[i][j] = [];
                  colors[i][j][0] = Math.floor(colorRaw[1]);
                  colors[i][j][1] = Math.floor(colorRaw[2]);
                  colors[i][j][2] = Math.floor(colorRaw[3]);
                  colors[i][j][3] = alpha; // Alpha
                } else if (l === 1) { // upper right corner
                  colors[i][j + 1] = [];
                  colors[i][j + 1][0] = Math.floor(colorRaw[1]);
                  colors[i][j + 1][1] = Math.floor(colorRaw[2]);
                  colors[i][j + 1][2] = Math.floor(colorRaw[3]);
                  colors[i][j + 1][3] = alpha; // Alpha
                } else if (l === 2) { // lower right corner
                  colors[i + 1][j + 1] = [];
                  colors[i + 1][j + 1][0] = Math.floor(colorRaw[1]);
                  colors[i + 1][j + 1][1] = Math.floor(colorRaw[2]);
                  colors[i + 1][j + 1][2] = Math.floor(colorRaw[3]);
                  colors[i + 1][j + 1][3] = alpha; // Alpha
                } else if (l === 3) { // lower left corner
                  colors[i + 1][j] = [];
                  colors[i + 1][j][0] = Math.floor(colorRaw[1]);
                  colors[i + 1][j][1] = Math.floor(colorRaw[2]);
                  colors[i + 1][j][2] = Math.floor(colorRaw[3]);
                  colors[i + 1][j][3] = alpha; // Alpha
                }
              }
            }
          }

          // SVG doesn't use tensor points but we need them for rendering.
          nodes[3 * i + 1][3 * j + 1] = new Point();
          nodes[3 * i + 1][3 * j + 2] = new Point();
          nodes[3 * i + 2][3 * j + 1] = new Point();
          nodes[3 * i + 2][3 * j + 2] = new Point();

          nodes[3 * i + 1][3 * j + 1].x =
            (-4.0 * nodes[3 * i][3 * j].x +
              6.0 * (nodes[3 * i][3 * j + 1].x + nodes[3 * i + 1][3 * j].x) +
              -2.0 * (nodes[3 * i][3 * j + 3].x + nodes[3 * i + 3][3 * j].x) +
              3.0 * (nodes[3 * i + 3][3 * j + 1].x + nodes[3 * i + 1][3 * j + 3].x) +
              -1.0 * nodes[3 * i + 3][3 * j + 3].x) / 9.0;
          nodes[3 * i + 1][3 * j + 2].x =
            (-4.0 * nodes[3 * i][3 * j + 3].x +
              6.0 * (nodes[3 * i][3 * j + 2].x + nodes[3 * i + 1][3 * j + 3].x) +
              -2.0 * (nodes[3 * i][3 * j].x + nodes[3 * i + 3][3 * j + 3].x) +
              3.0 * (nodes[3 * i + 3][3 * j + 2].x + nodes[3 * i + 1][3 * j].x) +
              -1.0 * nodes[3 * i + 3][3 * j].x) / 9.0;
          nodes[3 * i + 2][3 * j + 1].x =
            (-4.0 * nodes[3 * i + 3][3 * j].x +
              6.0 * (nodes[3 * i + 3][3 * j + 1].x + nodes[3 * i + 2][3 * j].x) +
              -2.0 * (nodes[3 * i + 3][3 * j + 3].x + nodes[3 * i][3 * j].x) +
              3.0 * (nodes[3 * i][3 * j + 1].x + nodes[3 * i + 2][3 * j + 3].x) +
              -1.0 * nodes[3 * i][3 * j + 3].x) / 9.0;
          nodes[3 * i + 2][3 * j + 2].x =
            (-4.0 * nodes[3 * i + 3][3 * j + 3].x +
              6.0 * (nodes[3 * i + 3][3 * j + 2].x + nodes[3 * i + 2][3 * j + 3].x) +
              -2.0 * (nodes[3 * i + 3][3 * j].x + nodes[3 * i][3 * j + 3].x) +
              3.0 * (nodes[3 * i][3 * j + 2].x + nodes[3 * i + 2][3 * j].x) +
              -1.0 * nodes[3 * i][3 * j].x) / 9.0;

          nodes[3 * i + 1][3 * j + 1].y =
            (-4.0 * nodes[3 * i][3 * j].y +
              6.0 * (nodes[3 * i][3 * j + 1].y + nodes[3 * i + 1][3 * j].y) +
              -2.0 * (nodes[3 * i][3 * j + 3].y + nodes[3 * i + 3][3 * j].y) +
              3.0 * (nodes[3 * i + 3][3 * j + 1].y + nodes[3 * i + 1][3 * j + 3].y) +
              -1.0 * nodes[3 * i + 3][3 * j + 3].y) / 9.0;
          nodes[3 * i + 1][3 * j + 2].y =
            (-4.0 * nodes[3 * i][3 * j + 3].y +
              6.0 * (nodes[3 * i][3 * j + 2].y + nodes[3 * i + 1][3 * j + 3].y) +
              -2.0 * (nodes[3 * i][3 * j].y + nodes[3 * i + 3][3 * j + 3].y) +
              3.0 * (nodes[3 * i + 3][3 * j + 2].y + nodes[3 * i + 1][3 * j].y) +
              -1.0 * nodes[3 * i + 3][3 * j].y) / 9.0;
          nodes[3 * i + 2][3 * j + 1].y =
            (-4.0 * nodes[3 * i + 3][3 * j].y +
              6.0 * (nodes[3 * i + 3][3 * j + 1].y + nodes[3 * i + 2][3 * j].y) +
              -2.0 * (nodes[3 * i + 3][3 * j + 3].y + nodes[3 * i][3 * j].y) +
              3.0 * (nodes[3 * i][3 * j + 1].y + nodes[3 * i + 2][3 * j + 3].y) +
              -1.0 * nodes[3 * i][3 * j + 3].y) / 9.0;
          nodes[3 * i + 2][3 * j + 2].y =
            (-4.0 * nodes[3 * i + 3][3 * j + 3].y +
              6.0 * (nodes[3 * i + 3][3 * j + 2].y + nodes[3 * i + 2][3 * j + 3].y) +
              -2.0 * (nodes[3 * i + 3][3 * j].y + nodes[3 * i][3 * j + 3].y) +
              3.0 * (nodes[3 * i][3 * j + 2].y + nodes[3 * i + 2][3 * j].y) +
              -1.0 * nodes[3 * i][3 * j].y) / 9.0;
        }
      }

      this.nodes = nodes; // (m*3+1) x (n*3+1) points
      this.colors = colors; // (m+1) x (n+1) x 4  colors (R+G+B+A)
    }

    // Extracts out each patch and then paints it
    paintMesh (v, w) {
      let imax = (this.nodes.length - 1) / 3;
      let jmax = (this.nodes[0].length - 1) / 3;

      if (this.type === 'bilinear' || imax < 2 || jmax < 2) {
        let patch;

        for (let i = 0; i < imax; ++i) {
          for (let j = 0; j < jmax; ++j) {
            let sliceNodes = [];
            for (let k = i * 3, kmax = (i * 3) + 4; k < kmax; ++k) {
              sliceNodes.push(this.nodes[k].slice(j * 3, (j * 3) + 4));
            }

            let sliceColors = [];
            sliceColors.push(this.colors[i].slice(j, j + 2));
            sliceColors.push(this.colors[i + 1].slice(j, j + 2));

            patch = new Patch(sliceNodes, sliceColors);
            patch.paint(v, w);
          }
        }
      } else {
        // Reference:
        // https://en.wikipedia.org/wiki/Bicubic_interpolation#Computation
        let d01, d12, patch, sliceNodes, nodes, f, alpha;
        const ilast = imax;
        const jlast = jmax;
        imax++;
        jmax++;

        /*
         * d = the interpolation data
         * d[i][j] = a node record (Point, color_array, color_dx, color_dy)
         * d[i][j][0] : Point
         * d[i][j][1] : [RGBA]
         * d[i][j][2] = dx [RGBA]
         * d[i][j][3] = dy [RGBA]
         * d[i][j][][k] : color channel k
         */
        let d = new Array(imax);

        // Setting the node and the colors
        for (let i = 0; i < imax; ++i) {
          d[i] = new Array(jmax);
          for (let j = 0; j < jmax; ++j) {
            d[i][j] = [];
            d[i][j][0] = this.nodes[3 * i][3 * j];
            d[i][j][1] = this.colors[i][j];
          }
        }

        // Calculate the inner derivatives
        for (let i = 0; i < imax; ++i) {
          for (let j = 0; j < jmax; ++j) {
            // dx
            if (i !== 0 && i !== ilast) {
              d01 = distance(d[i - 1][j][0], d[i][j][0]);
              d12 = distance(d[i + 1][j][0], d[i][j][0]);
              d[i][j][2] = finiteDifferences(d[i - 1][j][1], d[i][j][1],
                d[i + 1][j][1], d01, d12);
            }

            // dy
            if (j !== 0 && j !== jlast) {
              d01 = distance(d[i][j - 1][0], d[i][j][0]);
              d12 = distance(d[i][j + 1][0], d[i][j][0]);
              d[i][j][3] = finiteDifferences(d[i][j - 1][1], d[i][j][1],
                d[i][j + 1][1], d01, d12);
            }

            // dxy is, by standard, set to 0
          }
        }

        /*
         * Calculate the exterior derivatives
         * We fit the exterior derivatives onto parabolas generated by
         * the point and the interior derivatives.
         */
        for (let j = 0; j < jmax; ++j) {
          d[0][j][2] = [];
          d[ilast][j][2] = [];

          for (let k = 0; k < 4; ++k) {
            d01 = distance(d[1][j][0], d[0][j][0]);
            d12 = distance(d[ilast][j][0], d[ilast - 1][j][0]);

            if (d01 > 0) {
              d[0][j][2][k] = 2.0 * (d[1][j][1][k] - d[0][j][1][k]) / d01 -
                d[1][j][2][k];
            } else {
              console.log(`0 was 0! (j: ${j}, k: ${k})`);
              d[0][j][2][k] = 0;
            }

            if (d12 > 0) {
              d[ilast][j][2][k] = 2.0 * (d[ilast][j][1][k] - d[ilast - 1][j][1][k]) /
                d12 - d[ilast - 1][j][2][k];
            } else {
              console.log(`last was 0! (j: ${j}, k: ${k})`);
              d[ilast][j][2][k] = 0;
            }
          }
        }

        for (let i = 0; i < imax; ++i) {
          d[i][0][3] = [];
          d[i][jlast][3] = [];

          for (let k = 0; k < 4; ++k) {
            d01 = distance(d[i][1][0], d[i][0][0]);
            d12 = distance(d[i][jlast][0], d[i][jlast - 1][0]);

            if (d01 > 0) {
              d[i][0][3][k] = 2.0 * (d[i][1][1][k] - d[i][0][1][k]) / d01 -
                d[i][1][3][k];
            } else {
              console.log(`0 was 0! (i: ${i}, k: ${k})`);
              d[i][0][3][k] = 0;
            }

            if (d12 > 0) {
              d[i][jlast][3][k] = 2.0 * (d[i][jlast][1][k] - d[i][jlast - 1][1][k]) /
                d12 - d[i][jlast - 1][3][k];
            } else {
              console.log(`last was 0! (i: ${i}, k: ${k})`);
              d[i][jlast][3][k] = 0;
            }
          }
        }

        // Fill patches
        for (let i = 0; i < ilast; ++i) {
          for (let j = 0; j < jlast; ++j) {
            let dLeft = distance(d[i][j][0], d[i + 1][j][0]);
            let dRight = distance(d[i][j + 1][0], d[i + 1][j + 1][0]);
            let dTop = distance(d[i][j][0], d[i][j + 1][0]);
            let dBottom = distance(d[i + 1][j][0], d[i + 1][j + 1][0]);
            let r = [[], [], [], []];

            for (let k = 0; k < 4; ++k) {
              f = [];

              f[0] = d[i][j][1][k];
              f[1] = d[i + 1][j][1][k];
              f[2] = d[i][j + 1][1][k];
              f[3] = d[i + 1][j + 1][1][k];
              f[4] = d[i][j][2][k] * dLeft;
              f[5] = d[i + 1][j][2][k] * dLeft;
              f[6] = d[i][j + 1][2][k] * dRight;
              f[7] = d[i + 1][j + 1][2][k] * dRight;
              f[8] = d[i][j][3][k] * dTop;
              f[9] = d[i + 1][j][3][k] * dBottom;
              f[10] = d[i][j + 1][3][k] * dTop;
              f[11] = d[i + 1][j + 1][3][k] * dBottom;
              f[12] = 0; // dxy
              f[13] = 0; // dxy
              f[14] = 0; // dxy
              f[15] = 0; // dxy

              // get alpha values
              alpha = solveLinearSystem(f);

              for (let l = 0; l < 9; ++l) {
                r[k][l] = [];

                for (let m = 0; m < 9; ++m) {
                  // evaluation
                  r[k][l][m] = evaluateSolution(alpha, l / 8, m / 8);

                  if (r[k][l][m] > 255) {
                    r[k][l][m] = 255;
                  } else if (r[k][l][m] < 0.0) {
                    r[k][l][m] = 0.0;
                  }
                }
              }
            }

            // split the bezier patch into 8x8 patches
            sliceNodes = [];
            for (let k = i * 3, kmax = (i * 3) + 4; k < kmax; ++k) {
              sliceNodes.push(this.nodes[k].slice(j * 3, (j * 3) + 4));
            }

            nodes = splitPatch(sliceNodes);

            // Create patches and paint the bilinearliy
            for (let l = 0; l < 8; ++l) {
              for (let m = 0; m < 8; ++m) {
                patch = new Patch(
                  nodes[l][m],
                  [[
                    [r[0][l][m], r[1][l][m], r[2][l][m], r[3][l][m]],
                    [r[0][l][m + 1], r[1][l][m + 1], r[2][l][m + 1], r[3][l][m + 1]]
                  ], [
                    [r[0][l + 1][m], r[1][l + 1][m], r[2][l + 1][m], r[3][l + 1][m]],
                    [r[0][l + 1][m + 1], r[1][l + 1][m + 1], r[2][l + 1][m + 1], r[3][l + 1][m + 1]]
                  ]]
                );

                patch.paint(v, w);
              }
            }
          }
        }
      }
    }

    // Transforms mesh into coordinate space of canvas (t is either Point or Affine).
    transform (t) {
      if (t instanceof Point) {
        for (let i = 0, imax = this.nodes.length; i < imax; ++i) {
          for (let j = 0, jmax = this.nodes[0].length; j < jmax; ++j) {
            this.nodes[i][j] = this.nodes[i][j].add(t);
          }
        }
      } else if (t instanceof Affine) {
        for (let i = 0, imax = this.nodes.length; i < imax; ++i) {
          for (let j = 0, jmax = this.nodes[0].length; j < jmax; ++j) {
            this.nodes[i][j] = this.nodes[i][j].transform(t);
          }
        }
      }
    }

    // Scale mesh into coordinate space of canvas (t is a Point).
    scale (t) {
      for (let i = 0, imax = this.nodes.length; i < imax; ++i) {
        for (let j = 0, jmax = this.nodes[0].length; j < jmax; ++j) {
          this.nodes[i][j] = this.nodes[i][j].scale(t);
        }
      }
    }
  }

  // Start of document processing ---------------------
  const shapes = document.querySelectorAll('rect,circle,ellipse,path,text');

  shapes.forEach((shape, i) => {
    // Get id. If no id, create one.
    let shapeId = shape.getAttribute('id');
    if (!shapeId) {
      shapeId = 'patchjs_shape' + i;
      shape.setAttribute('id', shapeId);
    }

    const fillURL = shape.style.fill.match(/^url\(\s*"?\s*#([^\s"]+)"?\s*\)/);
    const strokeURL = shape.style.stroke.match(/^url\(\s*"?\s*#([^\s"]+)"?\s*\)/);

    if (fillURL && fillURL[1]) {
      const mesh = document.getElementById(fillURL[1]);

      if (mesh && mesh.nodeName === 'meshgradient') {
        const bbox = shape.getBBox();

        // Create temporary canvas
        let myCanvas = document.createElementNS(xhtmlNS, 'canvas');
        setAttributes(myCanvas, {
          'width': bbox.width,
          'height': bbox.height
        });

        const myContext = myCanvas.getContext('2d');
        let myCanvasImage = myContext.createImageData(bbox.width, bbox.height);

        // Draw a mesh
        const myMesh = new Mesh(mesh);

        // Adjust for bounding box if necessary.
        if (mesh.getAttribute('gradientUnits') === 'objectBoundingBox') {
          myMesh.scale(new Point(bbox.width, bbox.height));
        }

        // Apply gradient transform.
        const gradientTransform = mesh.getAttribute('gradientTransform');
        if (gradientTransform != null) {
          myMesh.transform(parseTransform(gradientTransform));
        }

        // Position to Canvas coordinate.
        if (mesh.getAttribute('gradientUnits') === 'userSpaceOnUse') {
          myMesh.transform(new Point(-bbox.x, -bbox.y));
        }

        // Paint
        myMesh.paintMesh(myCanvasImage.data, myCanvas.width);
        myContext.putImageData(myCanvasImage, 0, 0);

        // Create image element of correct size
        const myImage = document.createElementNS(svgNS, 'image');
        setAttributes(myImage, {
          'width': bbox.width,
          'height': bbox.height,
          'x': bbox.x,
          'y': bbox.y
        });

        // Set image to data url
        let myPNG = myCanvas.toDataURL();
        myImage.setAttributeNS(xlinkNS, 'xlink:href', myPNG);

        // Insert image into document
        shape.parentNode.insertBefore(myImage, shape);
        shape.style.fill = 'none';

        // Create clip referencing shape and insert into document
        const use = document.createElementNS(svgNS, 'use');
        use.setAttributeNS(xlinkNS, 'xlink:href', '#' + shapeId);

        const clipId = 'patchjs_clip' + i;
        const clip = document.createElementNS(svgNS, 'clipPath');
        clip.setAttribute('id', clipId);
        clip.appendChild(use);
        shape.parentElement.insertBefore(clip, shape);
        myImage.setAttribute('clip-path', 'url(#' + clipId + ')');

        // Force the Garbage Collector to free the space
        myCanvasImage = null;
        myCanvas = null;
        myPNG = null;
      }
    }

    if (strokeURL && strokeURL[1]) {
      const mesh = document.getElementById(strokeURL[1]);

      if (mesh && mesh.nodeName === 'meshgradient') {
        const strokeWidth = parseFloat(shape.style.strokeWidth.slice(0, -2));
        const strokeMiterlimit = parseFloat(shape.style.strokeMiterlimit) ||
          parseFloat(shape.getAttribute('stroke-miterlimit')) || 1;
        const phase = strokeWidth * strokeMiterlimit;

        const bbox = shape.getBBox();
        const boxWidth = Math.trunc(bbox.width + phase);
        const boxHeight = Math.trunc(bbox.height + phase);
        const boxX = Math.trunc(bbox.x - phase / 2);
        const boxY = Math.trunc(bbox.y - phase / 2);

        // Create temporary canvas
        let myCanvas = document.createElementNS(xhtmlNS, 'canvas');
        setAttributes(myCanvas, {
          'width': boxWidth,
          'height': boxHeight
        });

        const myContext = myCanvas.getContext('2d');
        let myCanvasImage = myContext.createImageData(boxWidth, boxHeight);

        // Draw a mesh
        const myMesh = new Mesh(mesh);

        // Adjust for bounding box if necessary.
        if (mesh.getAttribute('gradientUnits') === 'objectBoundingBox') {
          myMesh.scale(new Point(boxWidth, boxHeight));
        }

        // Apply gradient transform.
        const gradientTransform = mesh.getAttribute('gradientTransform');
        if (gradientTransform != null) {
          myMesh.transform(parseTransform(gradientTransform));
        }

        // Position to Canvas coordinate.
        if (mesh.getAttribute('gradientUnits') === 'userSpaceOnUse') {
          myMesh.transform(new Point(-boxX, -boxY));
        }

        // Paint
        myMesh.paintMesh(myCanvasImage.data, myCanvas.width);
        myContext.putImageData(myCanvasImage, 0, 0);

        // Create image element of correct size
        const myImage = document.createElementNS(svgNS, 'image');
        setAttributes(myImage, {
          'width': boxWidth,
          'height': boxHeight,
          'x': 0,
          'y': 0
        });

        // Set image to data url
        let myPNG = myCanvas.toDataURL();
        myImage.setAttributeNS(xlinkNS, 'xlink:href', myPNG);

        // Create pattern to hold the stroke image
        const patternId = 'pattern_clip' + i;
        const myPattern = document.createElementNS(svgNS, 'pattern');
        setAttributes(myPattern, {
          'id': patternId,
          'patternUnits': 'userSpaceOnUse',
          'width': boxWidth,
          'height': boxHeight,
          'x': boxX,
          'y': boxY
        });
        myPattern.appendChild(myImage);

        // Insert image into document
        mesh.parentNode.appendChild(myPattern);
        shape.style.stroke = 'url(#' + patternId + ')';

        // Force the Garbage Collector to free the space
        myCanvasImage = null;
        myCanvas = null;
        myPNG = null;
      }
    }
  });
})();
