/*  text_reassemble.h
version 0.0.2 2012-12-07
Copyright 2012, Caltech and David Mathog

See text_reassemble.c for notes

*/

#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <ctype.h>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include <iconv.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define TEREMIN(A,B) (A < B ? A : B)
#define TEREMAX(A,B) (A > B ? A : B)

#ifndef M_PI
# define M_PI		3.14159265358979323846	/* pi */
#endif 
#define ALLOCINFO_CHUNK 32
#define ALLOCOUT_CHUNK  8192
#define TRPRINT trinfo_append_out

/* text alignment types */
#define ALILEFT    0x01
#define ALICENTER  0x02
#define ALIRIGHT   0x04
#define ALIHORI    0x07
#define ALITOP     0x08
#define ALIBASE    0x10
#define ALIBOT     0x20
#define ALIVERT    0x38

/* language direction types */
#define LDIR_LR    0x00
#define LDIR_RL    0x01
#define LDIR_TB    0x02

/* Flags */
#define TR_EMFBOT  0x01 /* use an approximation compatible with EMF file's "BOTTOM" text orientation, which is not the "bottom" for Freetype fonts  */

/* complex classification types
   TR_TEXT    simple text object
   TR_LINE    linear assembly of TR_TEXTS
   TR_PARA_UJ sequential assembly of TR_LINE into a paragraph, with unknown justification properties
   TR_PARA_LJ ..., left justified
   TR_PARA_CJ ..., center justified
   TR_PARA_RJ ..., right justified
*/
enum tr_classes {TR_TEXT,TR_LINE,TR_PARA_UJ,TR_PARA_LJ,TR_PARA_CJ,TR_PARA_RJ};

/* Fontinfo structure, values related to fonts */
typedef struct {
   FT_Face     face;        /* font face structures (FT_FACE is a pointer!)                         */
   uint8_t    *file;        /* pointers to font paths to files                                      */
   uint8_t    *fname;       /* pointers to font names                                               */
   FcPattern  *fpat;        /* must hang onto this or faces operations break                        */
   double      spcadv;      /* advance equal to a space, in points                                  */
   double      fsize;       /* face size in points                                                  */
} FNT_SPECS;

typedef struct {
   FT_Library  library;     /* Fontconfig handle                                                    */
   FNT_SPECS  *fonts;       /* Array of fontinfo structures                                         */
   int         space;       /* storage slots allocated                                              */
   int         used;        /* storage slots in use                                                 */
} FT_INFO;

typedef struct {
   uint8_t    *string;      /* UTF-8 text                                                           */
   double      ori;         /* Orientation, angle of characters with respect to baseline in degrees */
   double      fs;          /* font size of text                                                    */
   double      x;           /* x coord relative to tri x,y, in points                               */
   double      y;           /* y coord                                                              */
   double      boff;        /* Y LL corner - boff finds baseline                                    */
   double      vadvance;    /* Line spacing typically 1.25 or 1.2,  only set on the first text
                                 element in a complex                                               */
   uint32_t    color;       /* RGBA                                                                 */
   int         taln;        /* text alignment with respect to x,y                                   */
   int         ldir;        /* language diretion LDIR_*                                             */
   int         italics;     /* italics, as in FontConfig                                            */
   int         weight;      /* weight, as in FontConfig                                             */
   int         condensed;   /* condensed, as in FontConfig                                          */
   int         co;          /* condensed override, if set Font name included narrow                 */
   int         rt_tidx;     /* index of rectangle that contains it                                  */
   int         fi_idx;      /* index of the font it uses                                            */
} TCHUNK_SPECS;

/* Text Info/Position Info structure, values related to text properties and string geometry
   Coordinates here are INTERNAL, after offset/rotate using values in TR_INFO.
*/
typedef struct {
   TCHUNK_SPECS *chunks;     /* text chunks                                                         */
   int         space;        /* storage slots allocated                                             */
   int         used;         /* storage slots in use                                                */
} TP_INFO;

/* Bounding Rectangle(s) structure
   Coordinates here are INTERNAL, after offset/rotate using values in TR_INFO.
*/
typedef struct {
   double      xll;           /* x rectangle lower left corner                                      */
   double      yll;           /* y "                                                                */
   double      xur;           /* x upper right corner                                               */
   double      yur;           /* y "                                                                */
   double      xbearing;      /* x bearing of the leftmost character                                */
} BRECT_SPECS;

typedef struct {
   BRECT_SPECS *rects;         /* bounding rectangles                                               */
   int         space;          /* storage slots allocated                                           */
   int         used;           /* storage slots in use                                              */
} BR_INFO;

typedef struct {
   int        *members;        /* array of immediate children (for TR_PARA_* these are indicies 
                                   for TR_TEXT or TR_LINE complexes also in cxi. For TR_TEXT
                                   and TR_LINE these are indices to the actual text in tpi.)        */
   int         space;          /* storage slots allocated                                           */
   int         used;           /* storage slots in use                                              */
} CHILD_SPECS;

/* Complex info structure, values related to the assembly of complex text from smaller pieces */
typedef struct {
   int               rt_cidx;  /* index of rectangle that contains all members                      */
   enum tr_classes   type;     /* classification of the complex                                     */
   CHILD_SPECS       kids;     /* immediate child nodes of this complex, for type TR_TEXT the
                                  idx refers to the tpi data. otherwise, cxi data                   */
} CX_SPECS;

typedef struct {
   CX_SPECS   *cx;             /* complexes                                                         */
   int         space;          /* storage slots allocated                                           */
   int         used;           /* storage slots in use                                              */
   int         phase1;         /* Number of complexes (lines + text fragments) entered in phase 1   */
   int         lines;          /* Number of lines in phase 1                                        */
   int         paras;          /* Number of complexes (paras) entered in phase 2                    */
} CX_INFO;

/* Text reassemble, overall structure */
typedef struct {
   FT_INFO    *fti;            /* Font info storage                                                 */
   TP_INFO    *tpi;            /* Text Info/Position Info storage                                   */
   BR_INFO    *bri;            /* Bounding Rectangle Info storage                                   */
   CX_INFO    *cxi;            /* Complex Info storage                                              */
   uint8_t    *out;            /* buffer to hold formatted output                                   */
   double      qe;             /* quantization error in points.                                     */
   double      esc;            /* escapement angle in DEGREES                                       */
   double      x;              /* coordinates of first text, in points                              */
   double      y;
   int         dirty;          /* 1 if text records are loaded                                      */ 
   int         use_kern;       /* 1 if kerning is used, 0 if not                                    */
   int         load_flags;     /* FT_LOAD_NO_SCALE or FT_LOAD_TARGET_NORMAL                         */
   int         kern_mode;      /* FT_KERNING_DEFAULT, FT_KERNING_UNFITTED, or FT_KERNING_UNSCALED   */
   int         outspace;       /* storage in output buffer  allocated                               */
   int         outused;        /* storage in output buffer in use                                   */
} TR_INFO;

/* padding added to rectangles before overlap test */
typedef struct {
   double up;                  /* to top                                                            */
   double down;                /* to bottom                                                         */
   double left;                /* to left                                                           */
   double right;               /* to right                                                          */
} RT_PAD;

/* iconv() has a funny cast on some older systems, on most recent ones
   it is just char **.  This tries to work around the issue.  If you build this
   on another funky system this code may need to be modified, or define ICONV_CAST
   on the compile line(but it may be tricky).
*/
#ifdef SOL8
#define ICONV_CAST (const char **)
#endif  //SOL8
#if !defined(ICONV_CAST)
#define ICONV_CAST (char **)
#endif  //ICONV_CAST

/* Prototypes */
int           TR_findcasesub(char *string, char *sub);
int           TR_getadvance(FNT_SPECS *fsp, uint32_t wc, uint32_t pc, int load_flags, int kern_mode, int *ymin, int *ymax);
int           TR_getkern2(FNT_SPECS *fsp, uint32_t wc, uint32_t pc, int kern_mode);
int           TR_kern_gap(FNT_SPECS *fsp, TCHUNK_SPECS *tsp, TCHUNK_SPECS *ptsp, int kern_mode);
void          TR_rt_pad_set(RT_PAD *rt_pad, double up, double down, double left, double right);
double        TR_baseline(TR_INFO *tri, int src, double *AscMax, double *DscMax);
int           TR_check_set_vadvance(TR_INFO *tri, int src, int lines);
int           TR_layout_analyze(TR_INFO *tri);
void          TR_layout_2_svg(TR_INFO *tri);
int           TR_weight_FC_to_SVG(int weight);

FT_INFO      *ftinfo_init(void);
int           ftinfo_make_insertable(FT_INFO *fti);
int           ftinfo_insert(FT_INFO *fti, FNT_SPECS *fsp);
FT_INFO      *ftinfo_release(FT_INFO *fti);
FT_INFO      *ftinfo_clear(FT_INFO *fti);

int           csp_make_insertable(CHILD_SPECS *csp);
int           csp_insert(CHILD_SPECS *csp, int src);
int           csp_merge(CHILD_SPECS *dst, CHILD_SPECS *src);
void          csp_release(CHILD_SPECS *csp);
#define       csp_clear csp_release  /* since the CHILD_SPECS area itself is not deleted, clear == reset */

CX_INFO      *cxinfo_init(void);
int           cxinfo_make_insertable(CX_INFO *cxi);
int           cxinfo_insert(CX_INFO *cxi, int src, int src_rt_idx, enum tr_classes type);
int           cxinfo_append(CX_INFO *cxi, int src, enum tr_classes type);
int           cxinfo_merge(CX_INFO *cxi,  int dst, int src, enum tr_classes type);
CX_INFO      *cxinfo_release(CX_INFO *cxi);
void          cxinfo_dump(TR_INFO *tri);

TP_INFO      *tpinfo_init(void);
int           tpinfo_make_insertable(TP_INFO *tpi);
int           tpinfo_insert(TP_INFO *tpi, TCHUNK_SPECS *tsp);
TP_INFO      *tpinfo_release(TP_INFO *tpi);

BR_INFO      *brinfo_init(void);
int           brinfo_make_insertable(BR_INFO *bri);
int           brinfo_insert(BR_INFO *bri, BRECT_SPECS *element);
int           brinfo_merge(BR_INFO *bri, int dst, int src);
enum tr_classes 
              brinfo_pp_alignment(BR_INFO *bri, int dst, int src, double slop, enum tr_classes type);
int           brinfo_overlap(BR_INFO *bri, int dst, int src, RT_PAD *rp_dst, RT_PAD *rp_src);
BR_INFO      *brinfo_release(BR_INFO *bri);

TR_INFO      *trinfo_init(TR_INFO *tri);
TR_INFO      *trinfo_release(TR_INFO *tri);
TR_INFO      *trinfo_release_except_FC(TR_INFO *tri);
TR_INFO      *trinfo_clear(TR_INFO *tri);
int           trinfo_load_fontname(TR_INFO *tri, uint8_t *fontname, TCHUNK_SPECS *tsp);
int           trinfo_load_qe(TR_INFO *tri, double qe);
int           trinfo_load_ft_opts(TR_INFO *tri, int use_kern, int load_flags, int kern_mode);
int           trinfo_load_textrec(TR_INFO *tri, TCHUNK_SPECS *tsp, double escapement, int flags);  
int           trinfo_append_out(TR_INFO *tri, char *src);

#ifdef __cplusplus
}
#endif
