/**
  @file text_reassemble.c

\verbatim
Method:
  1.  For all ordered text objects which are sequential and share the same esc.
  2.     For the first only pull x,y,esc and save, these define origin and rotation.
  3.     Save the text object.
  4.  Phase I: For all saved text objects construct lines.
  5.     Check for allowed overlaps on sequential saved text object bounding rectangles.
  6      If found merge second with first, check next one.
  7.     If not found, start a new complex (line).
  8.  Phase II;  for all lines construct paragraphs.
  9.     Check alignment and line spacing of preceding line with current line.
  10.    if alignment is the same, and line spacing is compatible merge current line into
            current paragraph.  Reaverage line spacing over all lines in paragraph.  Check next one.
  11.    If alignment does not match start a new paragraph.
      (Test program)
  12. Over all phase II paragraphs
  13. Over all phase I lines in each paragraph.
  14. Over all text objects in each line.
         Emit SVG corresponding to this construct to a file dump.svg.
         (Conversion to other file types would be modeled on this example.)
  15. Clean up.
      (General program)
      Like for the Test program, but final representation may not be SVG.
      Text object and bounding rectangle memory would all be released.  If another set of
      text will be processed then hang onto both Freetype and Fontconfig structures. If no
      other text will be processed here, then also release Freetype structures.  If the caller uses
      Fontconfig elsewhere then do not release it, otherwise, do so.

NOTE ON COORDINATES:  x is positive to the right, y is positive down.  So (0,0) is the upper left corner, and the
lower left corner of a rectangle has a LARGER Y coordinate than the upper left.  Ie,  LL=(10,10) UR=(30,5) is typical.
\endverbatim
*/

/*

Compilation of test program (with all debugging output, but not loop testing):
On Windows use:

   gcc -Wall -DWIN32 -DTEST -DDBG_TR_PARA -DDBG_TR_INPUT \
     -I. -I/c/progs/devlibs32/include -I/c/progs/devlibs32/include/freetype2\
     -o text_reassemble text_reassemble.c uemf_utf.c \
     -lfreetype6 -lfontconfig-1 -liconv -lm -L/c/progs/devlibs32/bin

On Linux use:

    gcc -Wall -DTEST -DTEST -DDBG_TR_PARA -DDBG_TR_INPUT  -I. -I/usr/include/freetype2 -o text_reassemble text_reassemble.c uemf_utf.c -lfreetype -lfontconfig -lm

Compilation of object file only (Windows):

    gcc -Wall -DWIN32 -c \
      -I. -I/c/progs/devlibs32/include -I/c/progs/devlibs32/include/freetype2\
      text_reassemble.c

Compilation of object file only (Linux):
    gcc -Wall -c -I. -I/usr/include/freetype2  text_reassemble.c 


Optional compiler switches for development:
  -DDBG_TR_PARA    draw bounding rectangles for paragraphs in SVG output
  -DDBG_TR_INPUT   draw input text and their bounding rectangles in SVG output
  -DTEST           build the test program
  -DDBG_LOOP       force the test program to cycle 5 times.  Useful for finding
                   memory leaks.  Ouput file is overwritten each time.


File:      text_reassemble.c
Version:   0.0.5
Date:      19-FEB-2013
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2013 David Mathog and California Institute of Technology (Caltech)
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "text_reassemble.h"
#include "uemf_utf.h"  /* For a couple of text functions.  Exact copy from libUEMF. */

/**
    \brief Find a (sub)string in a caseinvariant manner, used for locating "Narrow" in font name
    \return Returns -1 if no match, else returns the position (numbered from 0) of the first character of the match.
    \param string  Text to search
    \param sub     Text to find
*/
int TR_findcasesub(char *string, char *sub){
  int i,j;
  int match=0;
  for(i=0; string[i]; i++){
     for(match=1,j=0; sub[j] && string[i+j]; j++){
       if(toupper(sub[j]) != toupper(string[i+j])){
          match=0;
          break;
       }
     }
     if(match && !sub[j])break;  /* matched over the entire substring */
  }
  return((match ? i : -1));
}

/**
    \brief Get the advance for the 32 bit character
    
    \return Returns -1 on error, or advance in units of 1/64th of a Point.
    \param fsp          Pointer to FNT_SPECS struct.
    \param wc           Current character (32 bit int)
    \param pc           Previous character
    \param load_flags   Controls internal advance:
  FT_LOAD_NO_SCALE, internal advance is in 1/64th of a point. (kerning values are still scaled)
  FT_LOAD_TARGET_NORMAL internal advance is in 1/64th of a point.  The scale
     factor seems to be (Font Size in points)*(DPI)/(32.0 pnts)*(72 dpi). 
    \param kern_mode  FT_KERNING_DEFAULT, FT_KERNING_UNFITTED, or FT_KERNING_UNSCALED.  Set to match calling application.
    \param ymin  If the pointer is defined, the value is adjusted if ymin of wc character is less than the current value.
    \param ymax  If the pointer is defined, the value is adjusted if ymin of wc character is more than the current value. 
*/
int TR_getadvance(FNT_SPECS *fsp, uint32_t wc, uint32_t pc, int load_flags, int kern_mode, int *ymin, int *ymax){ 
   FT_Glyph   glyph;
   int        glyph_index;
   int        advance=-1;
   FT_BBox    bbox;
 
   glyph_index = FT_Get_Char_Index( fsp->face, wc);
   if (!FT_Load_Glyph( fsp->face, glyph_index,  load_flags )){
      if ( !FT_Get_Glyph( fsp->face->glyph, &glyph ) ) {
         advance = fsp->face->glyph->advance.x;
         FT_Glyph_Get_CBox( glyph, FT_GLYPH_BBOX_UNSCALED, &bbox );
         if(ymin && (bbox.yMin < *ymin))*ymin=bbox.yMin;
         if(ymax && (bbox.yMax > *ymax))*ymax=bbox.yMax;
         if(pc)advance +=  TR_getkern2(fsp, wc, pc, kern_mode);
         FT_Done_Glyph(glyph);
      }
   }
   return(advance);
}

/**
    \brief Get the kerning for a pair of 32 bit characters
    \return Returns 0 on error, or kerning value (which may be 0) for the pair in units of 1/64th of a point.
    \param fsp          Pointer to FNT_SPECS struct.
    \param wc           Current character (32 bit int)
    \param pc           Previous character
    \param kern_mode  FT_KERNING_DEFAULT, FT_KERNING_UNFITTED, or FT_KERNING_UNSCALED.  Set to match calling application.
*/
int TR_getkern2(FNT_SPECS *fsp, uint32_t wc, uint32_t pc, int kern_mode){ 
   int        this_glyph_index;
   int        prev_glyph_index;
   int        kern=0;
   FT_Vector  akerning;
 
   this_glyph_index = FT_Get_Char_Index( fsp->face, wc);
   prev_glyph_index = FT_Get_Char_Index( fsp->face, pc);
   if(!FT_Get_Kerning( fsp->face, 
       prev_glyph_index,
       this_glyph_index,
       kern_mode,
       &akerning )){
       kern = akerning.x; /* Is sign correct? */
   }
   return(kern);
}

/**
    \brief Get the kerning for a pair of 32 bit characters, where one is the last charcter in the previous text block, and the other is the first in the current text block.
    \return Returns 0 on error, or kerning value (which may be 0) for the pair in units of 1/64th of a point.
    \param fsp  Pointer to FNT_SPECS struct.
    \param tsp  current text object
    \param ptsp previous text object
    \param kern_mode  FT_KERNING_DEFAULT, FT_KERNING_UNFITTED, or FT_KERNING_UNSCALED.  Set to match calling application.
*/
int TR_kern_gap(FNT_SPECS *fsp, TCHUNK_SPECS *tsp, TCHUNK_SPECS *ptsp, int kern_mode){ 
   int          kern=0;
   uint32_t    *text32=NULL;
   uint32_t    *ptxt32=NULL;
   size_t       tlen,plen;
   while(ptsp && tsp){
       text32 = U_Utf8ToUtf32le((char *) tsp->string, 0, &tlen);
       if(!text32){  // LATIN1 encoded >128 are generally not valid UTF, so the first will fail
          text32 = U_Latin1ToUtf32le((char *) tsp->string,0, &tlen);
          if(!text32)break;
       }
       ptxt32 = U_Utf8ToUtf32le((char *) ptsp->string,0,&plen);
       if(!ptxt32){  // LATIN1 encoded >128 are generally not valid UTF, so the first will fail
          ptxt32 = U_Latin1ToUtf32le((char *) ptsp->string,0, &plen);
          if(!ptxt32)break;
       }
       kern = TR_getkern2(fsp, *text32, ptxt32[plen-1], kern_mode); 
       break;
   }
   if(text32)free(text32);
   if(ptxt32)free(ptxt32);
   return(kern);
}




/**
    \brief Find baseline on Y axis of a complex.
       If the complex is a TR_TEXT or TR_LINE find its baseline.
       If the complex is TR_PARA_[UCLR]J find the baseline of the last line.
       If there are multiple text elements in a TR_LINE, the baseline is that of the
         element that uses the largest font.   This will definitely give the wrong
         result if that line starts with a super or subscript that is full font size, but
         they are usually smaller.
    \return Returns 0 if it cannot determine a baseline, else returns the baseline Y coordinate.
    \param tri pointer to the TR_INFO structure holding all TR data
    \param src index of the current complex
    \param ymax  If the pointer is defined, the value is adjusted if ymax of current complex is more than the current value. 
    \param ymin  If the pointer is defined, the value is adjusted if ymin of current complex is less than the current value.
*/
double TR_baseline(TR_INFO *tri, int src, double *ymax, double *ymin){
   double       baseline=0;
   double       tmp;
   double       yheight;
   int          last;
   int          i;
   int          trec;
   FNT_SPECS   *fsp;
   static int   depth=0;
   CX_INFO     *cxi=tri->cxi;
   BR_INFO     *bri=tri->bri;
   TP_INFO     *tpi=tri->tpi;
   FT_INFO     *fti=tri->fti;
   last = cxi->cx[src].kids.used - 1;
   switch (cxi->cx[src].type){
      case TR_TEXT:
          trec = cxi->cx[src].kids.members[0];    /* for this complex type there is only ever one member */
          baseline = bri->rects[trec].yll - tpi->chunks[trec].boff;
          fsp = &(fti->fonts[tpi->chunks[trec].fi_idx]);
          yheight = fsp->face->bbox.yMax - fsp->face->bbox.yMin;
          if(ymax){
             tmp = tpi->chunks[trec].fs * ((double)fsp->face->bbox.yMax/yheight);
             if(*ymax <= tmp)*ymax = tmp;
          }
          else if(ymin){
             tmp = tpi->chunks[trec].fs * ((double)-fsp->face->bbox.yMin/yheight); /* yMin in face is negative */
             if(*ymin <= tmp)*ymin = tmp;
          }
          break;
      case TR_LINE:
          for(i=last;i>=0;i--){  /* here last is the count of */
             trec = cxi->cx[src].kids.members[i];
             fsp = &(fti->fonts[tpi->chunks[trec].fi_idx]);
             yheight = fsp->face->bbox.yMax - fsp->face->bbox.yMin;
             if(ymax){
                tmp = tpi->chunks[trec].fs * ((double)fsp->face->bbox.yMax/yheight);
                if(*ymax <= tmp){
                   *ymax = tmp;
                    baseline = bri->rects[trec].yll - tpi->chunks[trec].boff;
                }
             }
             else if(ymin){
                tmp = tpi->chunks[trec].fs * ((double)-fsp->face->bbox.yMin/yheight); /* yMin in face is negative */
                if(*ymin <= tmp){
                   *ymin = tmp;
                    baseline = bri->rects[trec].yll - tpi->chunks[trec].boff;
                }
             }
          }
          break;
      case TR_PARA_UJ:
      case TR_PARA_LJ:
      case TR_PARA_CJ:
      case TR_PARA_RJ:
          depth++;
          trec = cxi->cx[src].kids.members[last];
          baseline = TR_baseline(tri, trec, ymax, ymin);
          break;
   }
   return(baseline);
}

/**
    \brief Check or set vertical advance on the growing complex relative to the current complex.
        Vadvance is a multiplicative factor like 1.25.
        The distance between successive baselines is vadvance * max(font_size), where the maximum
        is over all text elements in src.
        The growing complex is always the last one in the CX_INFO section of the TR_INFO structure.
        If an existing vadvance does not match the one which would be required to fit the next complex
          to add to the growing one, it terminates a growing complex.  (Ie, starts a new paragraph.)
    Find baseline on Y axis of a complex.
       If the complex is a TR_TEXT or TR_LINE find its baseline.  
       If the complex is TR_PARA+* find the baseline of the last line.
       If there are multiple text elements in a TR_LINE, the baseline is that of the
         element that uses the largest font.   This will definitely give the wrong
         result if that line starts with a super or subscript that is full font size, but
         they are usually smaller.
    \return Returns 0 on success, !0 on failure.
    \param tri pointer to the TR_INFO structure holding all TR data
    \param src index of the current complex, to be added to the growing complex.
      This lets the value of "src - lines"  determine the weight to give to each new vadvance value
      as it is merged into the running weighted average.  This improves the accuracy of the vertical advance,
      since there can be some noise introduced when lines have different maximum font sizes.
    \param lines index of the first text block that was added to the growing complex.
*/
int TR_check_set_vadvance(TR_INFO *tri, int src, int lines){
   int         status = 0;
   CX_INFO    *cxi    = tri->cxi;
   TP_INFO    *tpi    = tri->tpi;
   double      ymax = 0.0;
   double      ymin = 0.0;
   double      prevbase;
   double      thisbase;
   double      weight;
   int         trec;
   double      newV;
   int         dst;
   
   dst = cxi->used-1; /* complex being grown */
    
   prevbase = TR_baseline(tri, dst, NULL, &ymin);
   thisbase = TR_baseline(tri, src, &ymax, NULL);
   newV     = (thisbase - prevbase)/(ymax + ymin);
   trec     = cxi->cx[dst].kids.members[0];   /* complex whose first text record holds vadvance for this complex */
   trec     = cxi->cx[trec].kids.members[0];  /* text record that halds vadvance for this complex                */
   if(tpi->chunks[trec].vadvance){
      /* already set on the first text (only place it is stored.)
         See if the line to be added is compatible.
         All text fields in a complex have the same advance, so just set/check the first one.
         vadvance must be within 1% or do not add a new line */
      if(fabs(1.0 - (tpi->chunks[trec].vadvance/newV) > 0.01)){
          status = 1;
      }
      else {  /* recalculate the weighted vadvance */
          weight = (1.0 / (double) (src - lines));
          tpi->chunks[trec].vadvance = tpi->chunks[trec].vadvance*(1.0-weight) + newV*weight;
     }
   }
   else { /* only happens when src = lines + 1*/
      tpi->chunks[trec].vadvance = newV;
   }
   return(status);
}


/**
    \brief Initialize an FT_INFO structure.  Sets up a freetype library to use in this context.
    \returns a pointer to the FT_INFO structure created, or NULL on error.
*/
FT_INFO *ftinfo_init(void){
   FT_INFO *fti = NULL;
   if(FcInit()){
      fti = (FT_INFO *)calloc(1,sizeof(FT_INFO));
      if(fti){
         if(!FT_Init_FreeType( &(fti->library))){
            fti->space=0;
            fti->used=0;

            if(ftinfo_make_insertable(fti)){
               FT_Done_FreeType(fti->library);
               free(fti);
               fti=NULL;
            }
         }
         else {
            free(fti);
            fti=NULL;
         }
      }
      if(!fti)FcFini();
   }
   return(fti);
}

/**
    \brief Make an FT_INFO structure insertable. Adds storage as needed.
    \param fti pointer to the FT_INFO structure
    \returns 0 on success, !0 on error.
*/
int ftinfo_make_insertable(FT_INFO *fti){
   int status=0;
   if(!fti)return(2);
   if(fti->used < fti->space){
     /* already insertable */
   }
   else {
      fti->space += ALLOCINFO_CHUNK;
      if((fti->fonts = (FNT_SPECS *) realloc(fti->fonts, fti->space * sizeof(FNT_SPECS) ))){
         memset(&fti->fonts[fti->used],0,(fti->space - fti->used)*sizeof(FNT_SPECS));
      }
      else {
         status=1;
      }
   }
   return(status);
}


/**
    \brief Insert a copy of a FNT_SPECS structure into the FT_INFO structure.
    \param fti pointer to the FT_INFO structure.
    \param fsp pointer to the FNT_SPECS structure.
    \returns 0 on success, !0 on error.
*/
int ftinfo_insert(FT_INFO *fti, FNT_SPECS *fsp){
   int status=1;
   if(!fti)return(2);
   if(!fsp)return(3);
   if(!(status = ftinfo_make_insertable(fti))){
      memcpy(&(fti->fonts[fti->used]),fsp,sizeof(FNT_SPECS));
      fti->used++;
   }
   return(status);
}



/**
    \brief  Release an FT_INFO structure. Release all associated memory.
        Use like:  fi_ptr = ftinfo_release(fi_ptr)
    \param fti pointer to the FT_INFO structure.
    \returns NULL.
*/
FT_INFO *ftinfo_release(FT_INFO *fti){
   int i;
   if(fti){
      for(i=0;i<fti->used;i++){
         FT_Done_Face(fti->fonts[i].face);        /* release memory for face controlled by FreeType */
         free(fti->fonts[i].file);                /* release memory holding copies of paths         */
         free(fti->fonts[i].fname);               /* release memory holding copies of font names    */
         FcPatternDestroy(fti->fonts[i].fpat);    /* release memory for FontConfit fpats     */
      }
      free(fti->fonts);
      FT_Done_FreeType(fti->library);        /* release all other FreeType memory */
      free(fti);
      FcFini();                              /* shut down FontConfig, release memory, patterns must have already been released or boom! */
   }
   return NULL;
}

/**
    \brief  Clear an FT_INFO structure. Release all Freetype memory but does not release Fontconfig.
        This would be called in preference to ftinfo_release() if some other part of the program needed
        to continue using Fontconfig.
        Use like:  fi_ptr = ftinfo_clear(fi_ptr)
    \param fti pointer to the FT_INFO structure.
    \returns NULL.
*/
FT_INFO *ftinfo_clear(FT_INFO *fti){
   int i;
   if(fti){
      for(i=0;i<fti->used;i++){
         FT_Done_Face(fti->fonts[i].face);        /* release memory for face controlled by FreeType */
         free(fti->fonts[i].file);                /* release memory holding copies of paths         */
         free(fti->fonts[i].fname);               /* release memory holding copies of font names    */
         FcPatternDestroy(fti->fonts[i].fpat);    /* release memory for FontConfit fpats     */
      }
      free(fti->fonts);
      FT_Done_FreeType(fti->library);        /* release all other FreeType memory */
      free(fti);
   }
   return NULL;
}


/**
    \brief Make a CHILD_SPECS structure insertable. Adds storage as needed.
    \param csp pointer to the CHILD_SPECS structure
    \returns 0 on success, !0 on error.
*/
int csp_make_insertable(CHILD_SPECS *csp){
   int status=0;
   if(!csp)return(2);
   if(csp->used < csp->space){
     /* already insertable */
   }
   else {
      csp->space += ALLOCINFO_CHUNK;
      if((csp->members = (int *) realloc(csp->members, csp->space * sizeof(int) ))){
         memset(&csp->members[csp->used],0,(csp->space - csp->used)*sizeof(int));
      }
      else {
         status=1;
      }
   }
   return(status);
}

/**
    \brief Add a member to a CHILD_SPECS structure. (Member is an index for either a text object or a complex.)
    \param dst pointer to the CHILD_SPECS structure.
    \param src index of the member.
    \returns 0 on success, !0 on error.
*/
int csp_insert(CHILD_SPECS *dst, int src){
   int status=1;
   if(!dst)return(2);
   if(!(status=csp_make_insertable(dst))){
      dst->members[dst->used]=src;
      dst->used++;
   }
   return(status);
}

/**
    \brief Append all the members of one CHILD_SPECS structure to another CHILD_SPECS structure. 
       Member is an index for either a text object or a complex.
       The donor is not modified.
    \param dst pointer to the recipient CHILD_SPECS structure.
    \param src pointer to the donor CHILD_SPECS structure.
    \returns 0 on success, !0 on error.
*/
int csp_merge(CHILD_SPECS *dst, CHILD_SPECS *src){
   int i;
   int status=1;
   if(!dst)return(2);
   if(!src)return(3);
   for(i=0;i<src->used;i++){
      status = csp_insert(dst, src->members[i]);
      if(status)break;
   }
   return(status);
}

/**
    \brief  Release a CHILD_SPECS structure. Release all associated memory.
    \param csp pointer to the CHILD_SPECS structure.
    \returns NULL.
*/
void csp_release(CHILD_SPECS *csp){
   if(csp){
      free(csp->members);
      csp->space = 0;
      csp->used  = 0;
   }
}


/**
    \brief Initialize an CX_INFO structure.  Holds complexes (multiple text objects in known positions and order.)
    \returns a pointer to the CX_INFO structure created, or NULL on error.
*/
CX_INFO *cxinfo_init(void){
   CX_INFO *cxi = NULL;
   cxi = (CX_INFO *)calloc(1,sizeof(CX_INFO));
   if(cxi){
      if(cxinfo_make_insertable(cxi)){
         free(cxi);
         cxi=NULL;
      }
   }
   return(cxi);
}

/**
    \brief Make a CX_INFO structure insertable. Adds storage as needed.
    \returns 0 on success, !0 on error.
    \param cxi pointer to the CX_INFO structure
*/
int cxinfo_make_insertable(CX_INFO *cxi){
   int status=0;
   if(cxi->used < cxi->space){
     /* already insertable */
   }
   else {
      cxi->space += ALLOCINFO_CHUNK;
      if((cxi->cx = (CX_SPECS *) realloc(cxi->cx, cxi->space * sizeof(CX_SPECS) ))){
         memset(&cxi->cx[cxi->used],0,(cxi->space - cxi->used)*sizeof(CX_SPECS));
      }
      else {
         status=1;
      }
   }
   return(status);
}

/**
    \brief Insert a complex into the CX_INFO structure. (Insert may be either TR_TEXT or TR_LINE.)
    \returns 0 on success, !0 on error.
    \param cxi pointer to the CX_INFO structure (complexes).
    \param src index of the the complex to insert.
    \param src_rt_tidx index of the bounding rectangle
    \param type TR_TEXT (index is for tpi->chunks[]) or TR_LINE (index is for cxi->kids[])
*/
int cxinfo_insert(CX_INFO *cxi, int src, int src_rt_tidx, enum tr_classes type){
   int status=1;
   if(!cxi)return(2);
   if(!(status=cxinfo_make_insertable(cxi))){
      cxi->cx[cxi->used].rt_cidx = src_rt_tidx;
      cxi->cx[cxi->used].type   = type;
      status = csp_insert(&(cxi->cx[cxi->used].kids), src);
      cxi->used++;
   }
   return(status);
}

/**
    \brief Append a complex to the CX_INFO structure and give it a type.
    \param cxi pointer to the CX_INFO structure (complexes).
    \param src index of the complex to append.
    \param type TR_LINE (src is an index for tpi->chunks[]) or TR_PARA (src is an index for cxi->kids[]).
    \returns 0 on success, !0 on error.
*/
int cxinfo_append(CX_INFO *cxi, int src, enum tr_classes type){
   int status=1;
   if(!cxi)return(2);
   if(!(status=cxinfo_make_insertable(cxi))){
      cxi->cx[cxi->used-1].type   = type;
      status = csp_insert(&(cxi->cx[cxi->used-1].kids), src);
   }
   return(status);
}


/**
    \brief Merge a complex dst with N members (N>=1) by adding a second complex src, and change the type.
    \param cxi pointer to the CX_INFO structure (complexes).
    \param dst index of the complex to expand.
    \param src index of the donor complex (which is not modified).
    \param type TR_LINE (src is an index for tpi->chunks[]) or TR_PARA (src is an index for cxi->kids[]).
    \returns 0 on success, !0 on error.
*/
int cxinfo_merge(CX_INFO *cxi, int dst, int src, enum tr_classes type){
   int status =1;
   if(!cxi)return(2);
   if(dst < 0 || dst >= cxi->used)return(3);   
   if(src < 0)return(4);   
   cxi->cx[dst].type = type; 
   status = csp_merge(&(cxi->cx[dst].kids), &(cxi->cx[src].kids));
   return(status);
}

/**
    \brief Dump the contents of the TR_INFO structure to stdout. For debugging purposes,not used in production code.
    \param tri pointer to the TR_INFO structure.
*/
void cxinfo_dump(TR_INFO *tri){
   int i,j,k;
   CX_INFO *cxi = tri->cxi;
   BR_INFO *bri = tri->bri;
   TP_INFO *tpi = tri->tpi;
   BRECT_SPECS *bsp;
   CX_SPECS    *csp;
   if(cxi){
      printf("cxi  space:  %d\n",cxi->space);
      printf("cxi  used:   %d\n",cxi->used);
      printf("cxi  phase1: %d\n",cxi->phase1);
      printf("cxi  lines:  %d\n",cxi->lines);
      printf("cxi  paras:  %d\n",cxi->paras);

      for(i=0;i<cxi->used;i++){
         csp = &(cxi->cx[i]);
         bsp = &(bri->rects[csp->rt_cidx]);
         printf("cxi  cx[%d] type:%d rt_tidx:%d kids_used:%d kids:space:%d\n",i, csp->type, csp->rt_cidx, csp->kids.used, csp->kids.space);
         printf("cxi  cx[%d] br (LL,UR) (%lf,%lf),(%lf,%lf)\n",i,bsp->xll,bsp->yll,bsp->xur,bsp->yur);
         for(j=0;j<csp->kids.used;j++){
            k = csp->kids.members[j];
            bsp = &(bri->rects[k]);
            if(csp->type == TR_TEXT || csp->type == TR_LINE){
                printf("cxi  cx[%d] member:%d tp_idx:%d rt_tidx:%d br (LL,UR) (%8.3lf,%8.3lf),(%8.3lf,%8.3lf) text:<%s>\n",i, j, k, tpi->chunks[k].rt_tidx, bsp->xll,bsp->yll,bsp->xur,bsp->yur, tpi->chunks[k].string);
            }
            else { /* TR_PARA_* */
                printf("cxi  cx[%d] member:%d cx_idx:%d\n",i, j, k);
            }
         }
      }
   }
   return;
}

/**
    \brief  Release a CX_INFO structure. Release all associated memory.
      use like:  cxi = cxiinfo_release(cxi);
    \param cxi pointer to the CX_INFO structure.
    \returns NULL.
*/
CX_INFO *cxinfo_release(CX_INFO *cxi){
   int i;
   if(cxi){
      for(i=0;i<cxi->used;i++){ csp_release(&cxi->cx[i].kids); }
      free(cxi->cx);
      free(cxi);                             /* release the overall cxinfo structure         */
   }
   return NULL;
}


/**
    \brief Initialize an TP_INFO structure.  Holds text objects from which complexes are built.
    \returns a pointer to the TP_INFO structure created, or NULL on error.
*/
TP_INFO *tpinfo_init(void){
   TP_INFO *tpi = NULL;
   tpi = (TP_INFO *)calloc(1,sizeof(TP_INFO));
   if(tpi){
      if(tpinfo_make_insertable(tpi)){
         free(tpi);
         tpi=NULL;
      }
   }
   return(tpi);
}


/**
    \brief Make a TP_INFO structure insertable. Adds storage as needed.
    \returns 0 on success, !0 on error.
    \param tpi pointer to the TP_INFO structure
*/
int tpinfo_make_insertable(TP_INFO *tpi){
   int status=0;
   if(tpi->used >= tpi->space){
      tpi->space += ALLOCINFO_CHUNK;
      if((tpi->chunks = (TCHUNK_SPECS *) realloc(tpi->chunks, tpi->space * sizeof(TCHUNK_SPECS) ))){
         memset(&tpi->chunks[tpi->used],0,(tpi->space - tpi->used)*sizeof(TCHUNK_SPECS));
      }
      else {
         status=1;
      }
   }
   return(status);
}

/**
    \brief Insert a copy of a  TCHUNK_SPECS structure into a TP_INFO structure. (Insert a text object.)
    \returns 0 on success, !0 on error.
    \param tpi pointer to the TP_INFO structure
    \param tsp pointer to the TCHUNK_SPECS structure
*/
int tpinfo_insert(TP_INFO *tpi, TCHUNK_SPECS *tsp){
   int status=1;
   if(!tpi)return(2);
   if(!tsp)return(3);
   if(!(status = tpinfo_make_insertable(tpi))){
      memcpy(&(tpi->chunks[tpi->used]),tsp,sizeof(TCHUNK_SPECS));
      if(tsp->co)tpi->chunks[tpi->used].condensed = 75;      /* Narrow was set in the font name */
      tpi->used++;
   }
   return(status);
}

/**
    \brief  Release a TP_INFO structure. Release all associated memory.
      use like:  tpi = tpinfo_release(tpi);
    \returns NULL.
    \param tpi pointer to the TP_INFO structure.
*/
TP_INFO *tpinfo_release(TP_INFO *tpi){
   int i;
   if(tpi){
      for(i=0;i<tpi->used;i++){
         free(tpi->chunks[i].string); }
      free(tpi->chunks);                     /* release the array                            */
      free(tpi);                             /* release the overall tpinfo structure         */
   }
   return NULL;
}

/**
    \brief Initialize an BR_INFO structure.  Holds bounding rectangles, for both text objects and complexes.
    \returns a pointer to the BR_INFO structure created, or NULL on error.
*/
BR_INFO *brinfo_init(void){
   BR_INFO *bri = NULL;
   bri = (BR_INFO *)calloc(1,sizeof(BR_INFO));
   if(bri){
      if(brinfo_make_insertable(bri)){
         free(bri);
         bri=NULL;
      }
   }
   return(bri);
}

/**
    \brief Make a BR_INFO structure insertable. Adds storage as needed.
    \returns 0 on success, !0 on error.
    \param bri pointer to the BR_INFO structure
*/
int brinfo_make_insertable(BR_INFO *bri){
   int status=0;
   if(!bri)return(2);
   if(bri->used >= bri->space){
      bri->space += ALLOCINFO_CHUNK;
      if(!(bri->rects = (BRECT_SPECS *) realloc(bri->rects, bri->space * sizeof(BRECT_SPECS) ))){ status = 1; }
   }
   return(status);
}

/**
    \brief Insert a copy of a  BRECT_SPEC structure into a BR_INFO structure. (Insert a bounding rectangle.)
    \returns 0 on success, !0 on error.
    \param bri pointer to the BR_INFO structure
    \param element pointer to the BRECT_SPECS structure
*/
int brinfo_insert(BR_INFO *bri, BRECT_SPECS *element){
   int status=1;
   if(!bri)return(2);
   if(!(status=brinfo_make_insertable(bri))){
      memcpy(&(bri->rects[bri->used]),element,sizeof(BRECT_SPECS));
      bri->used++;
   }
   return(status);
}

/**
    \brief Merge BRECT_SPEC element src into/with BRECT_SPEC element dst. src is unchanged.  (Merge two bounding rectangles.)
    \returns 0 on success, !0 on error.
    \param bri pointer to the BR_INFO structure
    \param dst index of the destination bounding rectangle.
    \param src index of the source bounding rectangle.
*/
int brinfo_merge(BR_INFO *bri, int dst, int src){
   if(!bri)return(1);
   if(dst<0 || dst>= bri->used)return(2);
   if(src<0 || src>= bri->used)return(3);
   bri->rects[dst].xll = TEREMIN(bri->rects[dst].xll, bri->rects[src].xll);
   bri->rects[dst].yll = TEREMAX(bri->rects[dst].yll, bri->rects[src].yll); /* MAX because Y is positive DOWN */
   bri->rects[dst].xur = TEREMAX(bri->rects[dst].xur, bri->rects[src].xur);
   bri->rects[dst].yur = TEREMIN(bri->rects[dst].yur, bri->rects[src].yur); /* MIN because Y is positive DOWN */
/*
printf("bri_Merge into rect:%d (LL,UR) dst:(%lf,%lf),(%lf,%lf) src:(%lf,%lf),(%lf,%lf)\n",dst,
(bri->rects[dst].xll),
(bri->rects[dst].yll),
(bri->rects[dst].xur),
(bri->rects[dst].yur),
(bri->rects[src].xll),
(bri->rects[src].yll),
(bri->rects[src].xur),
(bri->rects[src].yur));
*/
   return(0);
}

/**
    \brief Check for an allowable overlap of two bounding rectangles.
       Allowable overlap is any area overlap of src and dst bounding rectangles, after
         they have been expanded (padded) by allowed edge expansions.  (For instance, if
         missing spaces must be accounted for.)
       The method works backwards: look for all reasons they might not overlap, 
       if none are found, then the rectangles do overlap.
       An overlap here does not count just a line or a point - area must be involved.
    \returns 0 on success (overlap detected), 1 on no overlap, anything else is an error.
    \param bri pointer to the BR_INFO structure
    \param dst index of the destination bounding rectangle.
    \param src index of the source bounding rectangle.
    \param rp_dst Pointer to edge padding values for dst.
    \param rp_src Pointer to edge padding values for src.
*/
int brinfo_overlap(BR_INFO *bri, int dst, int src, RT_PAD *rp_dst, RT_PAD *rp_src){
   int status;
   BRECT_SPECS *br_dst;
   BRECT_SPECS *br_src;
   if(!bri || !rp_dst || !rp_src)return(2);
   if(dst<0 || dst>= bri->used)return(3);
   if(src<0 || src>= bri->used)return(4);
   br_dst=&bri->rects[dst];
   br_src=&bri->rects[src];
   if(   /* Test all conditions that exclude overlap, if any are true, then no overlap */
      ((br_dst->xur + rp_dst->right) < (br_src->xll - rp_src->left)  ) ||  /* dst fully to the left */
      ((br_dst->xll - rp_dst->left)  > (br_src->xur + rp_src->right) ) ||  /* dst fully to the right */
      ((br_dst->yur - rp_dst->up)    > (br_src->yll + rp_src->down)  ) ||  /* dst fully below (Y is positive DOWN) */
      ((br_dst->yll + rp_dst->down)  < (br_src->yur - rp_src->up)    )     /* dst fully above (Y is positive DOWN) */
      ){
      status = 1;  
   }
   else {   
      /* overlap not excluded, so it must occur.  
         Only accept overlaps that are mostly at one end or the other, not mostly top or bottom.
         If the following condition is true then there is no more than a tiny bit of horizontal overlap of src 
         within dist, which suggests that the two pieces of text may be considered part of one line. 
         (For a vertical alphabet the same method could be used for up/down.) */
      if(
         (br_src->xll >= br_dst->xur - rp_dst->right) ||  /*  src overlaps just a little on the right (L->R language) */
         (br_src->xur <= br_dst->xll + rp_dst->left)      /*  src overlaps just a little on the left  (R->L language) */
      ){
         status = 0;
      }
      else {  /* Too much overlap, reject the overlap */
         status = 1;
      }
   }
/*
printf("Overlap status:%d\nOverlap trects (LL,UR) dst:(%lf,%lf),(%lf,%lf) src:(%lf,%lf),(%lf,%lf)\n",
status,
(br_dst->xll - rp_dst->left ),
(br_dst->yll - rp_dst->down ),
(br_dst->xur + rp_dst->right),
(br_dst->yur + rp_dst->up   ),
(br_src->xll - rp_src->left ),
(br_src->yll - rp_src->down ),
(br_src->xur + rp_src->right),
(br_src->yur + rp_src->up   ));
printf("Overlap brects (LL,UR) dst:(%lf,%lf),(%lf,%lf) src:(%lf,%lf),(%lf,%lf)\n",
(br_dst->xll),
(br_dst->yll),
(br_dst->xur),
(br_dst->yur),
(br_src->xll),
(br_src->yll),
(br_src->xur),
(br_src->yur));
printf("Overlap rprect (LL,UR) dst:(%lf,%lf),(%lf,%lf) src:(%lf,%lf),(%lf,%lf)\n",
(rp_dst->left),
(rp_dst->down),
(rp_dst->right),
(rp_dst->up),
(rp_src->left),
(rp_src->down),
(rp_src->right),
(rp_src->up));
*/
   return(status);
}

/**
    \brief Try to deduce justification of a paragraph from the bounding rectangles for two successive lines.
    \returns one of TR_PARA_ UJ (unknown justified), LJ, CJ, or RJ (left, center, or right justified).
    \param bri pointer to the BR_INFO structure
    \param dst index of the destination bounding rectangle.
    \param src index of the source bounding rectangle.
    \param slop allowed error in edge alignment.
    \param type Preexisting justification for dst, if any.  Justification of dst and src must match this or
       TR_PARA_UJ is returned even if dst and src have some (other) alignment.
*/
enum tr_classes brinfo_pp_alignment(BR_INFO *bri, int dst, int src, double slop, enum tr_classes type){
   enum tr_classes newtype;
   BRECT_SPECS *br_dst = & bri->rects[dst];
   BRECT_SPECS *br_src = & bri->rects[src];
   if((br_dst->yur >= br_src->yur) || (br_dst->yll >= br_src->yll)){ /* Y is positive DOWN */
      /* lines in the wrong vertical order, no paragraph possible (Y is positive down) */
      newtype = TR_PARA_UJ;
   }
   else if(fabs(br_dst->xll - br_src->xll) < slop){
      /* LJ (might also be CJ but LJ takes precedence) */
      newtype = TR_PARA_LJ;
   } 
   else if(fabs(br_dst->xur - br_src->xur) < slop){
      /* RJ */
      newtype = TR_PARA_RJ;
   }
   else if(fabs( (br_dst->xur + br_dst->xll)/2.0 - (br_src->xur + br_src->xll)/2.0 ) < slop){
      /* CJ */
      newtype = TR_PARA_CJ;
   }
   else {
      /* not aligned */
      newtype = TR_PARA_UJ;
   }
   /*  within a paragraph type can change from unknown to known, but not from one known type to another*/
   if((type != TR_PARA_UJ) && (newtype != type)){
      newtype = TR_PARA_UJ;
   }
/*
printf("pp_align newtype:%d brects (LL,UR) dst:(%lf,%lf),(%lf,%lf) src:(%lf,%lf),(%lf,%lf)\n",
newtype,
(br_dst->xll),
(br_dst->yll),
(br_dst->xur),
(br_dst->yur),
(br_src->xll),
(br_src->yll),
(br_src->xur),
(br_src->yur));
*/
   return(newtype);
}

/**
    \brief  Release a BR_INFO structure. Release all associated memory.
      use like:  bri = brinfo_release(bri);
    \param bri pointer to the BR_INFO structure.
    \returns NULL.
*/
BR_INFO *brinfo_release(BR_INFO *bri){
   if(bri){
      free(bri->rects);
      free(bri);                             /* release the overall brinfo structure         */
   }
   return NULL;
}



/**
    \brief Initialize an TR_INFO structure.  Holds all data for text reassembly.
    \returns a pointer to the TR_INFO structure created, or NULL on error.
*/
TR_INFO *trinfo_init(TR_INFO *tri){
   if(tri)return(tri);                    /* tri is already set, double initialization is not allowed */
   if(!(tri = (TR_INFO *)calloc(1,sizeof(TR_INFO))) ||
      !(tri->fti = ftinfo_init()) ||
      !(tri->tpi = tpinfo_init()) ||
      !(tri->bri = brinfo_init()) ||
      !(tri->cxi = cxinfo_init())
      ){   tri = trinfo_release(tri);  }
   tri->use_kern   = 1;
   tri->load_flags = FT_LOAD_NO_SCALE;
   tri->kern_mode  = FT_KERNING_UNSCALED;
   tri->out        = NULL;                 /* This will allocate as needed, it might not ever be needed. */
   tri->outspace   = 0;
   tri->outused    = 0;
   return(tri);
}

/**
    \brief  Release a TR_INFO structure completely.
      Release all associated memory, including FontConfig.
      See also trinfo_clear() and trinfo_release_except_FC().
      use like:  tri = trinfo_release(tri);
    \param tri pointer to the TR_INFO structure.
    \returns NULL.
*/
TR_INFO *trinfo_release(TR_INFO *tri){
   if(tri){
      if(tri->bri)tri->bri=brinfo_release(tri->bri);
      if(tri->tpi)tri->tpi=tpinfo_release(tri->tpi);
      if(tri->fti)tri->fti=ftinfo_release(tri->fti);
      if(tri->cxi)tri->cxi=cxinfo_release(tri->cxi);
      if(tri->out){ free(tri->out); tri->out=NULL; };
      free(tri);
   }
   return(NULL);
}

/**
    \brief  Release a TR_INFO structure mostly. 
      Release all associated memory EXCEPT Fontconfig.
      Fontconfig may still be needed elsewhere in a program and there is no way to figure that out here.
      See also trinfo_clear() and trinfo_release().
      use like:  tri = trinfo_release_except_FC(tri);
    \param tri pointer to the TR_INFO structure.
    \returns NULL.
*/
TR_INFO *trinfo_release_except_FC(TR_INFO *tri){
   if(tri){
      if(tri->bri)tri->bri=brinfo_release(tri->bri);
      if(tri->tpi)tri->tpi=tpinfo_release(tri->tpi);
      if(tri->fti)tri->fti=ftinfo_clear(tri->fti);
      if(tri->cxi)tri->cxi=cxinfo_release(tri->cxi);
      if(tri->out){ free(tri->out); tri->out=NULL; };
      free(tri);
   }
   return(NULL);
}

/**
    \brief  Clear a TR_INFO structure.
    Releases text and rectangle information, but retains font information, both
      Freetype information and Fontconfig information.
      See also trinfo_release() and trinfo_release_except_FC().
      Use like: tri = trinfo_clear(tri);
    \param tri pointer to the TR_INFO structure.
    \returns NULL.
*/
TR_INFO *trinfo_clear(TR_INFO *tri){
   if(tri){
      tri->dirty      = 0;    /* set these back to their defaults  */
      tri->esc        = 0.0;
      /* Do NOT modify use_kern, load_flags, or kern_mode */

      if(tri->bri)tri->bri=brinfo_release(tri->bri);
      if(tri->tpi)tri->tpi=tpinfo_release(tri->tpi);
      if(tri->cxi)tri->cxi=cxinfo_release(tri->cxi);
      if(tri->out){ 
         free(tri->out);
         tri->out      = NULL;
         tri->outused  = 0;
         tri->outspace = 0;
      };
      if(!(tri->tpi = tpinfo_init()) ||     /* re-init the pieces just released */
         !(tri->bri = brinfo_init()) ||
         !(tri->cxi = cxinfo_init())
         ){
            tri = trinfo_release(tri);      /* something horrible happened, clean out tri and return NULL */
         }
   }
   return(tri);
}

/**
    \brief  Load a (new) font by name into a TR_INFO structure.
    \returns index of font.    If the font was already loaded return the existing index.
    \param tri pointer to the TR_INFO structure.
    \param fontname UTF-8 font name
    \param tsp pointer to the TCHUNK_SPECS structure - used to construct a font query string.
*/

int trinfo_load_fontname(TR_INFO *tri, uint8_t *fontname, TCHUNK_SPECS *tsp){
   FcPattern *pattern, *fpat;
   FcResult   result = FcResultMatch;
   char      *filename;
   double     fd;
   int        i;
   FT_INFO   *fti;
   char       buffer[512]; /* big enough */
   FNT_SPECS *fsp;
   
   if(!tri || !(tri->fti))return(1);
   
   fti = tri->fti; 
   /* construct a font name */
   sprintf(buffer,"%s:slant=%d:weight=%d:size=%lf:width=%d",fontname,tsp->italics,tsp->weight,tsp->fs,(tsp->co ? 75 : tsp->condensed));

   for(i=0;i<fti->used;i++){
     if(0==strcmp((char *) fti->fonts[i].fname,buffer)){
       tsp->fi_idx=i;
       return(0);
     }
   }

   ftinfo_make_insertable(fti);
   tsp->fi_idx = fti->used;

   if((pattern = FcNameParse((const FcChar8 *)buffer)) == NULL)return(2);
   if(!FcConfigSubstitute(NULL, pattern, FcMatchPattern))return(3);
   FcDefaultSubstitute(pattern);
   if((fpat = FcFontMatch(NULL, pattern, &result)) == NULL || result != FcResultMatch)return(4);
   if(FcPatternGetString(  fpat, FC_FILE,   0, (FcChar8 **)&filename)    != FcResultMatch)return(5);
   if(FcPatternGetDouble(  fpat, FC_SIZE,   0,  &fd)                     != FcResultMatch)return(7);
   
   /* copy these into memory for external use */
   fsp        = &(fti->fonts[fti->used]);
   fsp->file  = (uint8_t *) U_strdup((char *) filename);
   fsp->fname = (uint8_t *) U_strdup((char *) buffer);
   fsp->fpat  = fpat;
   fsp->fsize = fd;

   /* release FC's own memory related to this call that does not need to be kept around so that face will work */
   FcPatternDestroy(pattern);

   /* get the face */
   if(FT_New_Face( fti->library, (const char *) fsp->file, 0, &(fsp->face) )){ return(8); }

   if(FT_Set_Char_Size( 
      fsp->face,      /* handle to face object             */ 
      0,              /* char_width in 1/64th of points    */
      fd*64,          /* char_height in 1/64th of points   */ 
      72,             /* horizontal device resolution, DPI */ 
      72)             /* vebrical   device resolution, DPI */ 
      ){ return(9); }
             
   fti->used++;
 
/*
   char *fs;
   int fb;
   if(FcPatternGetBool(    fpat, FC_OUTLINE,     0, &fb)== FcResultMatch){  printf("outline:     %d\n",fb);fflush(stdout); }
   if(FcPatternGetBool(    fpat, FC_SCALABLE,    0, &fb)== FcResultMatch){  printf("scalable:    %d\n",fb);fflush(stdout); }
   if(FcPatternGetDouble(  fpat, FC_DPI,         0, &fd)== FcResultMatch){  printf("DPI:         %lf\n",fd);fflush(stdout); }
   if(FcPatternGetInteger( fpat, FC_FONTVERSION, 0, &fb)== FcResultMatch){  printf("fontversion: %d\n",fb);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_FULLNAME    ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("FULLNAME    :    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_FAMILY      ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("FAMILY      :    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_STYLE       ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("STYLE       :    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_FOUNDRY     ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("FOUNDRY     :    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_FAMILYLANG  ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("FAMILYLANG  :    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_STYLELANG   ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("STYLELANG   :    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_FULLNAMELANG,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("FULLNAMELANG:    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_CAPABILITY  ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("CAPABILITY  :    %s\n",fs);fflush(stdout); }
   if(FcPatternGetString(  fpat, FC_FONTFORMAT  ,     0,  (FcChar8 **)&fs)== FcResultMatch){  printf("FONTFORMAT  :    %s\n",fs);fflush(stdout); }
*/
   
   return(0);
}


/**
    \brief  Set the quantization error value for a TR_INFO structure.
      If coordinates have passed through an integer form limits
      in accuracy may have been imposed.  For instance, if the X coordinate of a point in such a file
      is 1000, and the conversion factor from those coordinates to points is .04, then eq is .04.  This
      just says that single coordinates are only good to within .04, and two coordinates may differ by as much
      as .08, just due to quantization error. So if some calculation shows a difference of
     .02 it may be interpreted as this sort of error and set to 0.0.
    \returns 0 on success, !0 on error.
    \param tri pointer to TR_INFO structure
    \param qe  quantization error.
*/
int trinfo_load_qe(TR_INFO *tri, double qe){
   if(!tri)return(1);
   if(qe<0.0)return(2);
   tri->qe=qe;
   return(0);
}

/**
    \brief  Set Freetype parameters and kerning mode (if any) in a TRI_INFO structure.
    \returns 0 on success, !0 on error.
    \param tri pointer to a TR_INFO structure
    \param use_kern  0 if kerning is to be employed, !0 otherwise.
    \param load_flags   Controls internal advance:
  FT_LOAD_NO_SCALE, internal advance is in 1/64th of a point. (kerning values are still scaled)
  FT_LOAD_TARGET_NORMAL internal advance is in 1/64th of a point.  The scale
     factor seems to be (Font Size in points)*(DPI)/(32.0 pnts)*(72 dpi). 
    \param kern_mode FT_KERNING_DEFAULT, FT_KERNING_UNFITTED, or FT_KERNING_UNSCALED.  Set to match calling application.
*/
int trinfo_load_ft_opts(TR_INFO *tri, int use_kern, int load_flags, int kern_mode){
   if(!tri)return(1);
   tri->use_kern   = use_kern;
   tri->load_flags = load_flags;
   tri->kern_mode  = kern_mode;
   return(0);
}

/**
    \brief  Append text to a TR_INFO struct's output buffer, expanding it if necessary.
    \returns 0 on success, !0 on error.
    \param tri pointer to a TR_INFO structure
    \param src  Pointer to a text string.
*/
int trinfo_append_out(TR_INFO *tri, char *src){
   size_t slen;
   if(!src)return(-1);
   slen = strlen(src);
   if(tri->outused + (int) slen + 1 < tri->outspace){
     /* already insertable */
   }
   else {
      tri->outspace += TEREMAX(ALLOCOUT_CHUNK,slen+1);
      if(!(tri->out = realloc(tri->out, tri->outspace )))return(-1);
   }
   memcpy(tri->out + tri->outused, src, slen+1); /* copy the terminator                       */
   tri->outused += slen;                         /* do not count the terminator in the length */
   return(0);
}


/**
    \brief  Load a text object into a TR_INFO struct.
    \returns 0 on success, !0 on error. -1 means that the escapement is different from the objects already loaded.
    \param tri pointer to a TR_INFO structure
    \param tsp pointer to a TCHUNK_SPECS structure (text object to load)
    \param escapement angle in degrees of the text object.
    \param flags special processing flags:
       TR_EMFBOT calculate Y coordinates of ALIBOT object compatible with EMF files TA_BOTTOM alignment.
*/
int trinfo_load_textrec(TR_INFO *tri, TCHUNK_SPECS *tsp, double escapement, int flags){
   
   int          status;
   double       x,y,xe;
   double       asc,dsc;     /* these are the ascender/descender for the actual text */
   int          ymin,ymax;
   double       fasc,fdsc;   /* these are the ascender/descender for the font as a whole (text independent) */
   TP_INFO     *tpi;
   FT_INFO     *fti;
   BR_INFO     *bri;
   int          current,idx,taln;
   uint32_t     prev;
   uint32_t    *text32,*tptr;
   FNT_SPECS   *fsp;
   BRECT_SPECS  bsp;
   
   /* check incoming parameters */
   if(!tri)return(1);
   if(!tsp)return(2);
   if(!tsp->string)return(3);
   fti  = tri->fti;
   tpi  = tri->tpi;
   bri  = tri->bri;
   idx  = tsp->fi_idx;
   taln = tsp->taln;
   if(idx <0 || idx >= tri->fti->used)return(4);

   if(!tri->dirty){
      tri->x     = tsp->x;
      tri->y     = tsp->y;
      tri->esc   = escapement;
      tri->dirty = 1;
   }
   else {
      if(tri->esc != escapement)return(-1);
   }


   tpinfo_insert(tpi,tsp);
   current=tpi->used-1;
   ymin  =  64000;
   ymax  = -64000;

   /* The geometry model has origin Y at the top of screen, positive Y is down, maximum positive
   Y is at the bottom of the screen.  That makes "top" (by positive Y) actually the bottom
   (as viewed on the screen.) */

   escapement  *= 2.0 * M_PI / 360.0;                                      /* degrees to radians */
   x = tpi->chunks[current].x - tri->x;                                    /* convert to internal orientation */
   y = tpi->chunks[current].y - tri->y;
   tpi->chunks[current].x = x * cos(escapement) - y * sin(escapement);     /* coordinate transformation */
   tpi->chunks[current].y = x * sin(escapement) + y * cos(escapement);

   fsp = &(fti->fonts[idx]);
/*  Careful!  face bbox does NOT scale with FT_Set_Char_Size 
printf("Face idx:%d bbox: xMax/Min:%ld,%ld yMax/Min:%ld,%ld UpEM:%d asc/des:%d,%d height:%d size:%lf\n",
                 idx, fsp->face->bbox.xMax,fsp->face->bbox.xMin,fsp->face->bbox.yMax,fsp->face->bbox.yMin,
                 fsp->face->units_per_EM,fsp->face->ascender,fsp->face->descender,fsp->face->height,fsp->fsize);
*/

   text32 = U_Utf8ToUtf32le((char *) tsp->string,0,NULL);
   if(!text32){  // LATIN1 encoded >128 are generally not valid UTF, so the first will fail
      text32 = U_Latin1ToUtf32le((char *) tsp->string,0,NULL);
      if(!text32)return(5);
   }
   fsp->spcadv = 0.0;
   /* baseline advance is independent of character orientation */
   for(xe=0.0, prev=0, tptr=text32; *tptr; tptr++){
      status = TR_getadvance(fsp, *tptr, (tri->use_kern ? prev: 0), tri->load_flags, tri->kern_mode, &ymin, &ymax);
      if(status>=0){
         xe += ((double) status)/64.0;
         if(*tptr==' ')fsp->spcadv = ((double) status)/64.0;
      }
      else { return(6); }
      prev=*tptr;
   }
   asc = ((double)  (ymax))/64.0;
   dsc = ((double)  (ymin))/64.0;  /* This is negative */
/*  This did not work very well because the ascender/descender went well beyond the actual characters, causing
    overlaps on lines that did not actually overlap (vertically).
   asc = ((double) (fsp->face->ascender) )/64.0;
   dsc = ((double) (fsp->face->descender))/64.0;
*/

   free(text32);
   
   /* get the advance on a space if it has not already been set */
   if(fsp->spcadv==0.0){
      status = TR_getadvance(fsp,' ',0, tri->load_flags, tri->kern_mode, NULL, NULL);
      if(status>=0){ fsp->spcadv = ((double) status)/64.0; }
      else {         return(7);           }
   }

   /* find the font ascender descender (general one, not specific for current text) */
   fasc = ((double) (fsp->face->ascender) )/64.0;
   fdsc = ((double) (fsp->face->descender))/64.0;
      
   if(tri->load_flags & FT_LOAD_NO_SCALE){ 
      xe           *= tsp->fs/32.0;
      fsp->spcadv  *= tsp->fs/32.0;
   }

   /* now place the rectangle using ALN information */
   if(      taln & ALIHORI & ALILEFT  ){
      bsp.xll = tpi->chunks[current].x;
      bsp.xur = tpi->chunks[current].x + xe;
   }
   else if( taln & ALIHORI & ALICENTER){
      bsp.xll = tpi->chunks[current].x - xe/2.0;
      bsp.xur = tpi->chunks[current].x + xe/2.0;
   }
   else{ /* taln & ALIHORI & ALIRIGHT */
      bsp.xll = tpi->chunks[current].x - xe;
      bsp.xur = tpi->chunks[current].x;
   }

   if(tri->load_flags & FT_LOAD_NO_SCALE){ 
      asc  *= tsp->fs/32.0;
      dsc  *= tsp->fs/32.0;
      fasc *= tsp->fs/32.0;
      fdsc *= tsp->fs/32.0;
   }
   

   /* From this point forward y is on the baseline, so need to correct it in chunks.  The asc/dsc are the general
      ones for the font, else the text content will muck around with the baseline in BAD ways. */
   if(      taln & ALIVERT & ALITOP  ){  tpi->chunks[current].y += fasc;           }
   else if( taln & ALIVERT & ALIBASE){                                             } /* no correction required */
   else{ /* taln & ALIVERT & ALIBOT */
       if(flags & TR_EMFBOT){            tpi->chunks[current].y -= 0.35 * tsp->fs; } /* compatible with EMF implementations */
       else {                            tpi->chunks[current].y += fdsc;           }
   } 
   tpi->chunks[current].boff = -dsc;

   /* since y is always on the baseline, the lower left and upper right are easy.  These use asc/dsc for the particular text,
      so that the bounding box will fit it tightly. */
   bsp.yll = tpi->chunks[current].y - dsc;
   bsp.yur = tpi->chunks[current].y - asc;
   brinfo_insert(bri,&bsp);
   tpi->chunks[current].rt_tidx = bri->used - 1; /* index of rectangle that contains it */

  return(0);  
}

/**
    \brief  Fontweight conversion.  Fontconfig units to SVG units.
         Anything not recognized becomes "normal" == 400.  
         There is no interpolation because a value that mapped to 775, for instance, most
         likely would not display properly because it is intermediate between 700 and 800, and
         only those need be supported in SVG viewers.
    \returns SVG font weight
    \param weight Fontconfig font weight.
*/
int TR_weight_FC_to_SVG(int weight){
  int ret=400;
  if(     weight ==      0){ ret = 100; }
  else if(weight ==     40){ ret = 200; }
  else if(weight ==     50){ ret = 300; }
  else if(weight ==     80){ ret = 400; }
  else if(weight ==    100){ ret = 500; }
  else if(weight ==    180){ ret = 600; }
  else if(weight ==    200){ ret = 700; }
  else if(weight ==    205){ ret = 800; }
  else if(weight ==    210){ ret = 900; }
  else                     { ret = 400; }
  return(ret);
}

/**
    \brief  Set the padding that will be added to bounding rectangles before checking for overlaps in brinfo_overlap().
    \returns void
    \param rt_pad pointer to an RT_PAD structure.
    \param up     padding for the top    of a bounding rectangle.
    \param down   padding for the bottom of a bounding rectangle.
    \param left   padding for the left   of a bounding rectangle.
    \param right  padding for the right  of a bounding rectangle.
*/
void TR_rt_pad_set(RT_PAD *rt_pad, double up, double down, double left, double right){
   rt_pad->up    = up;  
   rt_pad->down  = down; 
   rt_pad->left  = left; 
   rt_pad->right = right;
}            

/**
    \brief  Convert from analyzed complexes to SVG format.
    \returns void
    \param tri pointer to a TR_INFO struct which will be analyzed.  Result is stored in its "out" buffer.
*/
void TR_layout_2_svg(TR_INFO *tri){
   double        x = tri->x;
   double        y = tri->y;
   double        dx,dy;
   double        lastx    = 0.0;
   double        lasty    = 0.0;
   double        qsp;
   double        esc;
   double        recenter;  /* horizontal offset to set things up correctly for CJ and RJ text, is 0 for LJ*/
   double        lineheight=1.25;
   int           cutat;
   FT_INFO      *fti=tri->fti;      /* Font info storage                   */
   TP_INFO      *tpi=tri->tpi;      /* Text Info/Position Info storage     */
   BR_INFO      *bri=tri->bri;      /* bounding Rectangle Info storage     */
   CX_INFO      *cxi=tri->cxi;      /* Complexes deduced for this text     */
   TCHUNK_SPECS *tsp;               /* current text object                 */
   TCHUNK_SPECS *ptsp;              /* previous text object in the same line as current text object, if any */
   FNT_SPECS    *fsp;
   CX_SPECS     *csp;
   int           i,j,k,jdx,kdx;
   int           status;
   char          obuf[1024];        /* big enough for style and so forth   */

#if defined(DBG_TR_PARA) || defined(DBG_TR_INPUT)  /* enable debugging code, writes extra information into SVG */
   char          stransform[128];
   double        newx,newy;

    /* put rectangles down for each text string - debugging!!!  This will not work properly for any Narrow fonts */
   for(i=cxi->phase1; i<cxi->used;i++){                   /* over all complex members from phase2 == TR_PARA_* complexes */
      csp = &(cxi->cx[i]);
      esc = tri->esc;
      esc  *= 2.0 * M_PI / 360.0;                         /* degrees to radians and change direction of rotation */
      for(j=0; j<csp->kids.used; j++){                    /* over all members of these complexes, which are phase1 complexes  */
         jdx = csp->kids.members[j];                      /* index of phase1 complex (all are TR_TEXT or TR_LINE)             */
         for(k=0; k<cxi->cx[jdx].kids.used; k++){         /* over all members of the phase1 complex     */
            kdx = cxi->cx[jdx].kids.members[k];           /* index for text objects in tpi              */
            tsp = &tpi->chunks[kdx];
            if(!j && !k){
               sprintf(stransform,"transform=\"matrix(%lf,%lf,%lf,%lf,%lf,%lf)\"\n",cos(esc),-sin(esc),sin(esc),cos(esc),
                 1.25*x,1.25*y);
               lastx = bri->rects[tsp->rt_tidx].xll;
               lasty = bri->rects[tsp->rt_tidx].yll - tsp->boff;
#ifdef DBG_TR_PARA
               TRPRINT(tri, "<rect\n");
               TRPRINT(tri, "style=\"color:#0000FF;color-interpolation:sRGB;color-interpolation-filters:linearRGB;fill:none;stroke:#000000;stroke-width:0.30000001;stroke-miterlimit:4;stroke-opacity:1;stroke-dasharray:none;marker:none;visibility:visible;display:inline;overflow:visible;enable-background:accumulate;clip-rule:nonzero\"\n");
                  sprintf(obuf,"width=\"%lf\"\n", 1.25*(bri->rects[csp->rt_cidx].xur - bri->rects[csp->rt_cidx].xll));
               TRPRINT(tri, obuf);
                  sprintf(obuf,"height=\"%lf\"\n",1.25*(bri->rects[csp->rt_cidx].yll - bri->rects[csp->rt_cidx].yur));
               TRPRINT(tri, obuf);
                  sprintf(obuf,"x=\"%lf\" y=\"%lf\"\n",1.25*(bri->rects[csp->rt_cidx].xll),1.25*(bri->rects[csp->rt_cidx].yur));
               TRPRINT(tri, obuf);
               TRPRINT(tri, stransform);
               TRPRINT(tri, "/>\n");
#endif  /* DBG_TR_PARA */
            }
#ifdef DBG_TR_INPUT  /* debugging code, this section writes the original text objects */
            newx = 1.25*(bri->rects[tsp->rt_tidx].xll);
            newy = 1.25*(bri->rects[tsp->rt_tidx].yur);
            TRPRINT(tri, "<rect\n");
            TRPRINT(tri, "style=\"color:#000000;color-interpolation:sRGB;color-interpolation-filters:linearRGB;fill:none;stroke:#000000;stroke-width:0.30000001;stroke-miterlimit:4;stroke-opacity:1;stroke-dasharray:none;marker:none;visibility:visible;display:inline;overflow:visible;enable-background:accumulate;clip-rule:nonzero\"\n");
               sprintf(obuf,"width=\"%lf\"\n", 1.25*(bri->rects[tsp->rt_tidx].xur - bri->rects[tsp->rt_tidx].xll));
            TRPRINT(tri, obuf);
               sprintf(obuf,"height=\"%lf\"\n",1.25*(bri->rects[tsp->rt_tidx].yll - bri->rects[tsp->rt_tidx].yur));
            TRPRINT(tri, obuf);
               sprintf(obuf,"x=\"%lf\" y=\"%lf\"\n",newx,newy);
            TRPRINT(tri, obuf);
            TRPRINT(tri, stransform);
            TRPRINT(tri, "/>\n");

            newy = 1.25*(bri->rects[tsp->rt_tidx].yll - tsp->boff);
               sprintf(obuf,"<text x=\"%lf\" y=\"%lf\"\n",newx, newy );
            TRPRINT(tri, obuf);
               sprintf(obuf,"xml:space=\"preserve\"\n");
            TRPRINT(tri, obuf);
            TRPRINT(tri, stransform);
            TRPRINT(tri, "style=\"fill:#FF0000;");    
               sprintf(obuf,"font-size:%lfpx;",tsp->fs*1.25);  /*IMPORTANT, if the FS is given in pt it looks like crap in browsers.  As if px != 1.25 pt, maybe 96 dpi not 90?*/
            TRPRINT(tri, obuf);
               sprintf(obuf,"font-style:%s;",(tsp->italics ? "italic" : "normal"));
            TRPRINT(tri, obuf);
            TRPRINT(tri, "font-variant:normal;");
               sprintf(obuf,"font-weight:%d;",TR_weight_FC_to_SVG(tsp->weight));
            TRPRINT(tri, obuf);
               sprintf(obuf,"font-stretch:%s;",(tsp->condensed==100 ? "Normal" : "Condensed"));
            TRPRINT(tri, obuf);
            cutat=strcspn((char *)fti->fonts[tsp->fi_idx].fname,":");
            fti->fonts[tsp->fi_idx].fname[cutat]='\0';
               sprintf(obuf,"font-family:%s;",fti->fonts[tsp->fi_idx].fname);
            TRPRINT(tri, obuf);
               sprintf(obuf,"\n\">%s</text>\n",tsp->string);    
            TRPRINT(tri, obuf);
#endif /* DBG_TR_INPUT debugging code, original text objects */
         }
      }
   }
#endif /* DBG_TR_PARA and/or DBG_TR_INPUT  */


   tsp=tpi->chunks;
   /* over all complex members from phase2.  Paragraphs == TR_PARA_*  */
   for(i=cxi->phase1; i<cxi->used;i++){
      csp = &(cxi->cx[i]);
      esc = tri->esc;
      esc  *= 2.0 * M_PI / 360.0;              /* degrees to radians and change direction of rotation */
      
      /* over all members of the present Paragraph.  Each of these is a line and a phase 1 complex.
      It may be either TR_TEXT or TR_LINE */
      for(j=0; j<csp->kids.used; j++){
         if(j){
             sprintf(obuf,"</tspan>");
             TRPRINT(tri, obuf);
         }
         jdx = csp->kids.members[j];           /* index of phase1 complex (all are TR_TEXT or TR_LINE)             */
         lastx = bri->rects[jdx].xur;
         lasty = bri->rects[jdx].yll - tsp->boff;
         recenter = 0;                         /* mostly to quiet a compiler warning, should always be set below */
         
         /* over all members of the present Line.  These are the original text objects which were reassembled.
         There will be one for TR_TEXT, more than one for TR_LINE */
         for(ptsp = NULL, k=0; k<cxi->cx[jdx].kids.used; k++){
            if(k){
               ptsp=tsp;                                      /* previous text object in this line  */
               fsp = &(fti->fonts[tpi->chunks[kdx].fi_idx]);  /* font spec for previous text object */
            }
            kdx = cxi->cx[jdx].kids.members[k];       /* index for text objects in tpi              */
            tsp = &tpi->chunks[kdx];
            if(!k){
               switch(csp->type){                     /* set up the alignment, if there is one */
                  case TR_TEXT:
                  case TR_LINE:
                     /* these should never occur, this section quiets a compiler warning */
                     break;
                  case TR_PARA_UJ:
                     recenter=0.0;
                     break;                     
                  case TR_PARA_LJ:
                     recenter=0.0;
                     break;
                  case TR_PARA_CJ:
                     recenter=(bri->rects[cxi->cx[jdx].rt_cidx].xur - bri->rects[cxi->cx[jdx].rt_cidx].xll)/2.0;
                     break;
                  case TR_PARA_RJ:
                     recenter=bri->rects[cxi->cx[jdx].rt_cidx].xur - bri->rects[cxi->cx[jdx].rt_cidx].xll;
                     break;
               }
               if(!j){
                   TRPRINT(tri, "<text\n");
                   TRPRINT(tri, "xml:space=\"preserve\"\n");
                   TRPRINT(tri, "style=\"");
                      sprintf(obuf,"font-size:%lfpx;",tsp->fs*1.25);  /*IMPORTANT, if the FS is given in pt it looks like crap in browsers.  As if px != 1.25 pt, maybe 96 dpi not 90?*/
                   TRPRINT(tri, obuf);
                      sprintf(obuf,"font-style:%s;",(tsp->italics ? "italic" : "normal"));
                   TRPRINT(tri, obuf);
                   TRPRINT(tri, "font-variant:normal;");
                      sprintf(obuf,"font-weight:%d;",TR_weight_FC_to_SVG(tsp->weight));
                   TRPRINT(tri, obuf);
                      sprintf(obuf,"font-stretch:%s;",(tsp->condensed==100 ? "Normal" : "Condensed"));
                   TRPRINT(tri, obuf);
                   if(tsp->vadvance){ lineheight = tsp->vadvance *100.0; }
                   else {             lineheight = 125.0;                }
                      sprintf(obuf,"line-height:%lf%%;",lineheight);
                   TRPRINT(tri, obuf);
                   TRPRINT(tri, "letter-spacing:0px;");
                   TRPRINT(tri, "word-spacing:0px;");
                   TRPRINT(tri, "fill:#000000;");
                   TRPRINT(tri, "fill-opacity:1;");
                   TRPRINT(tri, "stroke:none;");
                   cutat=strcspn((char *)fti->fonts[tsp->fi_idx].fname,":");
                   fti->fonts[tsp->fi_idx].fname[cutat]='\0';
                      sprintf(obuf,"font-family:%s;",fti->fonts[tsp->fi_idx].fname);
                   TRPRINT(tri, obuf);
                   switch(csp->type){                     /* set up the alignment, if there is one */
                      case TR_TEXT:
                      case TR_LINE:
                         /* these should never occur, this section quiets a compiler warning */
                         break;
                      case TR_PARA_UJ:
                         *obuf='\0';
                         break;
                      case TR_PARA_LJ:
                         sprintf(obuf,"text-align:start;text-anchor:start;");
                         break;
                      case TR_PARA_CJ:
                         sprintf(obuf,"text-align:center;text-anchor:middle;");
                         break;
                      case TR_PARA_RJ:
                         sprintf(obuf,"text-align:end;text-anchor:end;");
                         break;
                   }
                   TRPRINT(tri, obuf);
                   TRPRINT(tri, "\"\n");  /* End of style specification */
                      sprintf(obuf,"transform=\"matrix(%lf,%lf,%lf,%lf,%lf,%lf)\"\n",cos(esc),-sin(esc),sin(esc),cos(esc),1.25*x,1.25*y);
                   TRPRINT(tri, obuf);
                      sprintf(obuf,"x=\"%lf\" y=\"%lf\"\n>",1.25*(bri->rects[kdx].xll + recenter),1.25*(bri->rects[kdx].yll - tsp->boff));
                   TRPRINT(tri, obuf);
               }
                  sprintf(obuf,"<tspan sodipodi:role=\"line\"\nx=\"%lf\" y=\"%lf\"\n>",
                 1.25*(bri->rects[kdx].xll + recenter),1.25*(bri->rects[kdx].yll - tsp->boff));
               TRPRINT(tri, obuf);
            }
            TRPRINT(tri, "<tspan\n");
            dx = 1.25*(bri->rects[tsp->rt_tidx].xll - lastx);
            dy = 1.25*(bri->rects[tsp->rt_tidx].yll - tsp->boff - lasty);
            
            /* Have to also take into account kerning between the last letter of the preceding rectangle
            and the first letter of the current one. Assume font values are from leading retangle's font. */
            if(ptsp && tri->use_kern){
               status = TR_kern_gap(fsp, tsp, ptsp, tri->kern_mode);
               if(status){
                  dx += (tri->load_flags & FT_LOAD_NO_SCALE ? tsp->fs/32.0: 1.0) * ((double) status)/64.0;
               }
            }

            /* Sometimes a font substitution was absolutely terrible, for instance, for Arial Narrow on (most) Linux systems,
               The resulting advance may be much too large so that it overruns the next text chunk.  Since overlapping text on
               the same line is almost never encountered, this may be used to detect the bad substitution so that a more appropriate
               offset can be used.  
               Detect this situation as a negative dx < 1/2 a space character's width while |dy| < an entire space width. */
            qsp = 1.25 * 0.25 * fti->fonts[tsp->fi_idx].spcadv;
            if((dy <=qsp && dy >= -qsp) && dx < -2*qsp){  dx=0.0; }
            if(k==0){ sprintf(obuf,"dx=\"%lf\" dy=\"%lf\" ",0.0, 0.0); }
            else {    sprintf(obuf,"dx=\"%lf\" dy=\"%lf\" ",dx, dy); }
            TRPRINT(tri, obuf);
               sprintf(obuf,"style=\"fill:#%2.2X%2.2X%2.2X;",tsp->color.Red,tsp->color.Green,tsp->color.Blue);    
            TRPRINT(tri, obuf);
               sprintf(obuf,"font-size:%lfpx;",tsp->fs*1.25);  /*IMPORTANT, if the FS is given in pt it looks like crap in browsers.  As if px != 1.25 pt, maybe 96 dpi not 90?*/
            TRPRINT(tri, obuf);
               sprintf(obuf,"font-style:%s;",(tsp->italics ? "italic" : "normal"));
            TRPRINT(tri, obuf);
            TRPRINT(tri, "font-variant:normal;");
               sprintf(obuf,"font-weight:%d;",TR_weight_FC_to_SVG(tsp->weight));
            TRPRINT(tri, obuf);
               sprintf(obuf,"font-stretch:%s;",(tsp->condensed==100 ? "Normal" : "Condensed"));
            TRPRINT(tri, obuf);
            cutat=strcspn((char *)fti->fonts[tsp->fi_idx].fname,":");
            fti->fonts[tsp->fi_idx].fname[cutat]='\0';
               sprintf(obuf,"font-family:%s;\"",fti->fonts[tsp->fi_idx].fname);
            TRPRINT(tri, obuf);
            TRPRINT(tri, "\n>");    
            TRPRINT(tri, (char *) tsp->string);    
            TRPRINT(tri, "</tspan>");    
            lastx=bri->rects[tsp->rt_tidx].xur;
            lasty=bri->rects[tsp->rt_tidx].yll - tsp->boff;
         }  /* end of k loop */
      }     /* end of j loop */
      TRPRINT(tri,"</tspan></text>\n");
   }        /* end of i loop */
}

/**
    \brief  Attempt to figure out the original organization, in lines and paragraphs, of the text objects.
   The method is:
   1. Generate complexes from the text objects (strings) by overlaps (optionally allowing up to two spaces to be
      added) to produce larger rectangles. Complexes that are more or less sequential and have 2 or more text objects
      are TR_LINEs, therwise they are TR_TEXT.
   2. Group sequential complexes (TR_LINE or TR_TEXT) into TR_PARA_UJ (paragraphs,by smooth progression in vertical
      position down page).
   3. Analyze the paragraphs to classify them as Left/Center/Right justified (possibly with indentation.)  If 
      they do not fall into any of these categories break that one back down into TR_LINE/TR_TEXT.
   4. Return the number of complex text objects.
    \returns Number of complexes.  (>=1, <= number of text objects.) <0 is an error.
    \param tri    pointer to the TR_INFO structure holding the data, which will also hold the results.
*/
int TR_layout_analyze(TR_INFO *tri){
   int             i,j;
   int             ok;
   int             cxidx;
   int             src_rt;
   int             dst_rt;
   TP_INFO        *tpi;
   BR_INFO        *bri;
   CX_INFO        *cxi;
   FT_INFO        *fti;
   BRECT_SPECS     bsp;
   RT_PAD          rt_pad_i;
   RT_PAD          rt_pad_j;
   double          ratio;
   enum tr_classes type;

   if(!tri)return(-1);
   if(!tri->cxi)return(-2);
   if(!tri->tpi)return(-3);
   if(!tri->bri)return(-4);
   if(!tri->fti)return(-5);
   tpi=tri->tpi;
   cxi=tri->cxi;
   bri=tri->bri;
   fti=tri->fti;
   cxi->lines  = 0;
   cxi->paras  = 0;
   cxi->phase1 = 0;

   /* Phase 1.  Working sequentially, insert text.  Initially as TR_TEXT and then try to extend to TR_LINE by checking
      overlaps.  When done the complexes will contain a mix of TR_LINE and TR_TEXT. */
   
   for(i=0; i<tpi->used; i++){
      memcpy(&bsp,&(bri->rects[tpi->chunks[i].rt_tidx]),sizeof(BRECT_SPECS));  /* Must make a copy as next call may reallocate rects! */
      (void) brinfo_insert(bri,&bsp);
      dst_rt = bri->used-1;
      (void) cxinfo_insert(cxi, i, dst_rt, TR_TEXT);
      cxidx = cxi->used-1;
      /* for the leading text: pad with no leading and two trailing spaces */
      TR_rt_pad_set(&rt_pad_i,tri->qe, tri->qe, 0.0, tri->qe + 2.0 * fti->fonts[tpi->chunks[i].fi_idx].spcadv);

      for(j=i+1; j<tpi->used; j++){
         /* Reject font size changes of greater than 50%, these are almost certainly not continuous text.  These happen
            in math formulas, for instance, where a sum or integral is much larger than the other symbols. */
         ratio = (double)(tpi->chunks[j].fs)/(double)(tpi->chunks[i].fs);
         if(ratio >2.0 || ratio <0.5)break;

         /* for the trailing text: pad with one leading and trailing spaces (so it should work L->R and R->L */
         TR_rt_pad_set(&rt_pad_j,tri->qe, tri->qe, fti->fonts[tpi->chunks[j].fi_idx].spcadv, fti->fonts[tpi->chunks[j].fi_idx].spcadv);
         src_rt = tpi->chunks[j].rt_tidx;
         if(!brinfo_overlap(bri,
                            dst_rt,                   /* index into bri for dst */
                            src_rt,                   /* index into bri for src */
                            &rt_pad_i,&rt_pad_j)){
             (void) cxinfo_append(cxi,j,TR_LINE);
             (void) brinfo_merge(bri,dst_rt,src_rt);
             /* for the leading text: pad with two leading and trailing spaces (so it should work L->R and R->L */
             TR_rt_pad_set(&rt_pad_i, tri->qe, tri->qe,
                tri->qe + 2.0 * fti->fonts[tpi->chunks[j].fi_idx].spcadv, tri->qe + 2.0 * fti->fonts[tpi->chunks[j].fi_idx].spcadv);
         }
         else { /* either alignment ge*/
             break;
         }
      }
      i=j-1;  /* start up after the last merged entry (there may not be any) */
      if(cxi->cx[cxidx].type == TR_LINE)cxi->lines++;
   }
   cxi->phase1 = cxi->used;  /* total complexes defined in this phase, all TR_LINE or TR_TEXT */

   /*  Phase 2, try to group sequential lines.  There may be "lines" that are still TR_TEXT, as in:
   
      ... this is a sentence that wraps by one
      word.
      
    And some paragrahs might be single word lines (+ = bullet in the following)
    
      +verbs
      +nouns
      +adjectives
      
    Everything starts out as TR_PARA_UJ and if the next one can be lined up, the type changes to
    an aligned paragraph and complexes are appended to the existing one.
   */

   for(i=0; i < cxi->phase1; i++){ 
      type    = TR_PARA_UJ;    /* any paragraph alignment will be acceptable  */
      memcpy(&bsp,&(bri->rects[cxi->cx[i].rt_cidx]),sizeof(BRECT_SPECS));  /* Must make a copy as next call may reallocate rects! */
      (void) brinfo_insert(bri,&bsp);
      dst_rt = bri->used-1;
      (void) cxinfo_insert(cxi, i, dst_rt, type);
      cxi->paras++;
      ok = 1;
      for(j=i+1; ok && (j < cxi->phase1); j++){
         type = brinfo_pp_alignment(bri, cxi->cx[i].rt_cidx, cxi->cx[j].rt_cidx, 3*tri->qe, type);
         switch (type){
            case TR_PARA_UJ:  /* paragraph type was set and j line does not fit, or no paragraph alignment matched */
               ok = 0;  /* force exit from j loop */
               j--;     /* this will increment at loop bottom */
               break;
            case TR_PARA_LJ:
            case TR_PARA_CJ:
            case TR_PARA_RJ:
               /* two successive lines have been identified (possible following others already in the paragraph */
               if(TR_check_set_vadvance(tri,j,i)){  /* check for compatibility with vadvance if set, set it if it isn't. */
                  ok = 0;  /* force exit from j loop */
                  j--;     /* this will increment at loop bottom */
               }
               else {
                  src_rt = cxi->cx[j].rt_cidx;
                  (void) cxinfo_append(cxi, j, type);
                  (void) brinfo_merge(bri, dst_rt, src_rt);
                }
              break;
            default:
               return(-6);  /* programming error */
         }         
      }
      if(j>=cxi->phase1)break;
      i=j-1;
   }
      
/* When debugging  
  cxinfo_dump(tri);
*/

   return(cxi->used);
}


/* no doxygen documentation below this point, these pieces are for the text program, not the library. */

#if TEST
#define MAXLINE 2048  /* big enough for testing */
enum OP_TYPES {OPCOM,OPOOPS,OPFONT,OPESC,OPORI,OPXY,OPFS,OPTEXT,OPALN,OPLDIR,OPMUL,OPITA,OPWGT,OPCND,OPCLR,OPFLAGS,OPEMIT,OPDONE};

int parseit(char *buffer,char **data){
   int pre;
   pre = strcspn(buffer,":");
   if(!pre)return(OPOOPS);
   *data=&buffer[pre+1];
   buffer[pre]='\0';
   if(*buffer=='#'            )return(OPCOM );
   if(0==strcmp("FONT",buffer))return(OPFONT);
   if(0==strcmp("ESC" ,buffer))return(OPESC );
   if(0==strcmp("ORI", buffer))return(OPORI );
   if(0==strcmp("XY",  buffer))return(OPXY  );
   if(0==strcmp("FS",  buffer))return(OPFS  );
   if(0==strcmp("TEXT",buffer))return(OPTEXT);
   if(0==strcmp("ALN", buffer))return(OPALN );
   if(0==strcmp("LDIR",buffer))return(OPLDIR);
   if(0==strcmp("MUL", buffer))return(OPMUL );
   if(0==strcmp("ITA", buffer))return(OPITA );
   if(0==strcmp("WGT", buffer))return(OPWGT );
   if(0==strcmp("CND", buffer))return(OPCND );
   if(0==strcmp("CLR", buffer))return(OPCLR );
   if(0==strcmp("FLAG",buffer))return(OPFLAGS);
   if(0==strcmp("EMIT",buffer))return(OPEMIT);
   if(0==strcmp("DONE",buffer))return(OPDONE);
   return(OPOOPS);
}

void boom(char *string,int lineno){
   fprintf(stderr,"Fatal error at line %d %s\n",lineno,string);
   exit(EXIT_FAILURE);
}


void init_as_svg(TR_INFO *tri){
   TRPRINT(tri,"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
   TRPRINT(tri,"<!-- Created with Inkscape (http://www.inkscape.org/) -->\n");
   TRPRINT(tri,"\n");
   TRPRINT(tri,"<svg\n");
   TRPRINT(tri,"   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n");
   TRPRINT(tri,"   xmlns:cc=\"http://creativecommons.org/ns#\"\n");
   TRPRINT(tri,"   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n");
   TRPRINT(tri,"   xmlns:svg=\"http://www.w3.org/2000/svg\"\n");
   TRPRINT(tri,"   xmlns=\"http://www.w3.org/2000/svg\"\n");
   TRPRINT(tri,"   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n");
   TRPRINT(tri,"   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n");
   TRPRINT(tri,"   width=\"900\"\n");
   TRPRINT(tri,"   height=\"675\"\n");
   TRPRINT(tri,"   id=\"svg4122\"\n");
   TRPRINT(tri,"   version=\"1.1\"\n");
   TRPRINT(tri,"   inkscape:version=\"0.48+devel r11679 custom\"\n");
   TRPRINT(tri,"   sodipodi:docname=\"simplest_text.svg\">\n");
   TRPRINT(tri,"  <defs\n");
   TRPRINT(tri,"     id=\"defs4124\" />\n");
   TRPRINT(tri,"  <sodipodi:namedview\n");
   TRPRINT(tri,"     id=\"base\"\n");
   TRPRINT(tri,"     pagecolor=\"#ffffff\"\n");
   TRPRINT(tri,"     bordercolor=\"#666666\"\n");
   TRPRINT(tri,"     borderopacity=\"1.0\"\n");
   TRPRINT(tri,"     inkscape:pageopacity=\"0.0\"\n");
   TRPRINT(tri,"     inkscape:pageshadow=\"2\"\n");
   TRPRINT(tri,"     inkscape:zoom=\"0.98994949\"\n");
   TRPRINT(tri,"     inkscape:cx=\"309.88761\"\n");
   TRPRINT(tri,"     inkscape:cy=\"482.63995\"\n");
   TRPRINT(tri,"     inkscape:document-units=\"px\"\n");
   TRPRINT(tri,"     inkscape:current-layer=\"layer1\"\n");
   TRPRINT(tri,"     showgrid=\"false\"\n");
   TRPRINT(tri,"     width=\"0px\"\n");
   TRPRINT(tri,"     height=\"0px\"\n");
   TRPRINT(tri,"     fit-margin-top=\"0\"\n");
   TRPRINT(tri,"     fit-margin-left=\"0\"\n");
   TRPRINT(tri,"     fit-margin-right=\"0\"\n");
   TRPRINT(tri,"     fit-margin-bottom=\"0\"\n");
   TRPRINT(tri,"     units=\"in\"\n");
   TRPRINT(tri,"     inkscape:window-width=\"1200\"\n");
   TRPRINT(tri,"     inkscape:window-height=\"675\"\n");
   TRPRINT(tri,"     inkscape:window-x=\"26\"\n");
   TRPRINT(tri,"     inkscape:window-y=\"51\"\n");
   TRPRINT(tri,"     inkscape:window-maximized=\"0\" />\n");
   TRPRINT(tri,"  <metadata\n");
   TRPRINT(tri,"     id=\"metadata4127\">\n");
   TRPRINT(tri,"    <rdf:RDF>\n");
   TRPRINT(tri,"      <cc:Work\n");
   TRPRINT(tri,"         rdf:about=\"\">\n");
   TRPRINT(tri,"        <dc:format>image/svg+xml</dc:format>\n");
   TRPRINT(tri,"        <dc:type\n");
   TRPRINT(tri,"           rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\" />\n");
   TRPRINT(tri,"        <dc:title></dc:title>\n");
   TRPRINT(tri,"      </cc:Work>\n");
   TRPRINT(tri,"    </rdf:RDF>\n");
   TRPRINT(tri,"  </metadata>\n");
   TRPRINT(tri,"  <g\n");
   TRPRINT(tri,"     inkscape:label=\"Layer 1\"\n");
   TRPRINT(tri,"     inkscape:groupmode=\"layer\"\n");
   TRPRINT(tri,"     id=\"layer1\"\n");
   TRPRINT(tri,"     transform=\"translate(0,0)\">\n");
   TRPRINT(tri,"\n");
}


void flush_as_svg(TR_INFO *tri, FILE *fp){
   fwrite(tri->out,tri->outused,1,fp);
}

FILE *close_as_svg(TR_INFO *tri, FILE *fp){
   TRPRINT(tri, "  </g>\n");
   TRPRINT(tri, "</svg>\n");
   flush_as_svg(tri,fp);
   fclose(fp);
   return(NULL);
}


int main(int argc, char *argv[]){
   char         *data;
   char          inbuf[MAXLINE];
   FILE         *fpi         = NULL;
   FILE         *fpo         = NULL;
   int           op;
   double        fact        = 1.0;  /* input units to points */
   double        escapement  = 0.0;  /* degrees */
   int           lineno      = 0;
   int           ok          = 1;
   int           status;
   TCHUNK_SPECS  tsp;
   TR_INFO      *tri=NULL;
   int           flags=0;
   char         *infile;
   uint32_t      utmp32;

   infile=malloc(strlen(argv[1])+1);
   strcpy(infile,argv[1]);

   if(argc < 2 || !(fpi = fopen(infile,"r"))){
      printf("Usage:  text_reassemble input_file\n");
      printf("  Test program reads an input file containing lines like:\n");
      printf("    FONT:(font for next text)\n");
      printf("    ESC:(escapement angle degrees of text line, up from X axis)\n");
      printf("    ORI:(angle degrees of character orientation, up from X axis)\n");
      printf("    FS:(font size, units)\n");
      printf("    XY:(x,y)   X 0 is at left, N is at right, Y 0 is at top, N is at bottom, as page is viewed.\n");
      printf("    TEXT:(UTF8 text)\n");
      printf("    ALN:combination of {LCR}{BLT} = Text is placed on {X,Y} at Left/Center/Right of text, at Bottom,baseLine,Top of text.\n");
      printf("    LDIR:{LR|RL|TB)  Left to Right, Right to Left, and Top to Bottom \n");
      printf("    MUL:(float, multiplicative factor to convert FS,XY units to points).\n");
      printf("    ITA:(Italics, 0=normal, 100=italics, 110=oblique).\n");
      printf("    WGT:(Weight, 0-215: 80=normal, 200=bold, 215=ultrablack, 0=thin)).\n");
      printf("    CND:(Condensed 50-200: 100=normal, 50=ultracondensed, 75=condensed, 200=expanded).\n");
      printf("    CLR:(RGB color, as 6 HEX digits, like: FF0000 (red) or 0000FF (blue)) \n");
      printf("    FLAG: Special processing options.  1 EMF compatible text alignment.\n");
      printf("    EMIT:(Process everything up to this point, then start clean for remaining input).\n");
      printf("    DONE:(no more input, process it).\n");
      printf("    # comment\n");
      printf("\n");
      printf("    The output is a summary of how the pieces are to be assembled into complex text.\n");
      printf("\n");
      printf("    egrep pattern:  '^LOAD:|^FONT:|^ESC:|^ORI:|^FS:|^XY:|^TEXT:|^ALN:|^LDIR:|^MUL:|^ITA:|^WGT:|^CND:|^CLR:|^FLAG:|^EMIT:^DONE:'\n");
      exit(EXIT_FAILURE);
   }

   tri = trinfo_init(tri); /* If it loops the trinfo_clear at the end will reset tri to the proper state, do NOT call trinfo_init twice! */

#ifdef DBG_LOOP
   int ldx;
   for(ldx=0;ldx<5;ldx++){
     if(fpi)fclose(fpi);
     fpi = fopen(infile,"r");
#endif
   tsp.string     = NULL;
   tsp.ori        = 0.0;  /* degrees */
   tsp.fs         = 12.0; /* font size */
   tsp.x          = 0.0;
   tsp.y          = 0.0;
   tsp.boff       = 0.0;  /* offset to baseline from LL corner of bounding rectangle, changes with fs and taln*/
   tsp.vadvance   = 0.0;  /* meaningful only when a complex contains two or more lines */
   tsp.taln       = ALILEFT + ALIBASE;
   tsp.ldir       = LDIR_LR;
   tsp.color.Red       = 0;    /* RGBA Black */
   tsp.color.Green     = 0;    /* RGBA Black */
   tsp.color.Blue      = 0;    /* RGBA Black */
   tsp.color.Reserved  = 0;    /* unused */
   tsp.italics    = 0;
   tsp.weight     = 80;
   tsp.condensed  = 100;
   tsp.co         = 0;
   tsp.fi_idx     = -1;  /* set to an invalid */
   /* no need to set rt_tidx */

   if(!tri){
      fprintf(stderr,"Fatal error, could not initialize data structures\n");
      exit(EXIT_FAILURE);
   }
   (void) trinfo_load_ft_opts(tri, 1,
      FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING  | FT_LOAD_NO_BITMAP, 
      FT_KERNING_UNSCALED);
   
   fpo=fopen("dump.svg","wb");
   init_as_svg(tri);

   while(ok){
      lineno++;
      if(!fgets(inbuf,MAXLINE,fpi))boom("Unexpected end of file - no DONE:",lineno);
      inbuf[strlen(inbuf)-1]='\0'; /* step on the EOL character */ 
      op = parseit(inbuf,&data);
      switch(op){
         case OPCOM:  /* ignore comments*/
            break;
         case OPFONT:
            /* If the font name includes "Narrow" condensed may not have been set */
            if(0<= TR_findcasesub(data, "Narrow")){
               tsp.co=1;
            }
            else {
               tsp.co=0;
            }
            if(trinfo_load_fontname(tri, (uint8_t *) data, &tsp))boom("Font load failed",lineno);
            break;
         case OPESC:
            if(1 != sscanf(data,"%lf",&escapement))boom("Invalid ESC:",lineno);
            break;
         case OPORI:
            if(1 != sscanf(data,"%lf",&tsp.ori))boom("Invalid ORI:",lineno);
            break;
         case OPFS:
            if(1 != sscanf(data,"%lf",&tsp.fs) || tsp.fs <= 0.0)boom("Invalid FS:",lineno);
            tsp.fs *= fact;
            break;
         case OPXY:
            if(2 != sscanf(data,"%lf,%lf",&tsp.x,&tsp.y) )boom("Invalid XY:",lineno);
            tsp.x *= fact;
            tsp.y *= fact;
            break;
         case OPTEXT:
            tsp.string = (uint8_t *) U_strdup(data);
            /* FreeType parameters match inkscape*/
            status = trinfo_load_textrec(tri, &tsp, escapement,flags);
            if(status==-1){ // change of escapement, emit what we have and reset 
               TR_layout_analyze(tri);
               TR_layout_2_svg(tri);
               flush_as_svg(tri, fpo);
               tri = trinfo_clear(tri);
               if(trinfo_load_textrec(tri, &tsp, escapement,flags)){ boom("Text load failed",lineno); }
            }
            else if(status){ boom("Text load failed",lineno); }
            break;
         case OPALN:
            tsp.taln=0;
            switch (*data++){
              case 'L': tsp.taln |= ALILEFT;   break;
              case 'C': tsp.taln |= ALICENTER; break;
              case 'R': tsp.taln |= ALIRIGHT;  break;
              default: boom("Invalid ALN:",lineno);
            }
            switch (*data++){
              case 'T': tsp.taln |= ALITOP;    break;
              case 'L': tsp.taln |= ALIBASE;   break;
              case 'B': tsp.taln |= ALIBOT;    break;
              default: boom("Invalid ALN:",lineno);
            }
            break;
         case OPLDIR:
            tsp.ldir=0;
            if(0==strcmp("LR",data)){ tsp.ldir=LDIR_LR;  break;}
            if(0==strcmp("RL",data)){ tsp.ldir=LDIR_RL;  break;}
            if(0==strcmp("TB",data)){ tsp.ldir=LDIR_TB;  break;}
            boom("Invalid LDIR:",lineno);
            break;
         case OPMUL:
            if(1 != sscanf(data,"%lf",&fact)     || fact <= 0.0)boom("Invalid MUL:",lineno);
            (void) trinfo_load_qe(tri,fact);
            break;
         case OPITA:
            if(1 != sscanf(data,"%d",&tsp.italics)   || tsp.italics < 0 || tsp.italics>110)boom("Invalid ITA:",lineno);
            break;
         case OPWGT:
            if(1 != sscanf(data,"%d",&tsp.weight)    || tsp.weight < 0  || tsp.weight > 215)boom("Invalid WGT:",lineno);
            break;
         case OPCND:
            if(1 != sscanf(data,"%d",&tsp.condensed) || tsp.condensed < 50 || tsp.condensed > 200)boom("Invalid CND:",lineno);
            break;
         case OPCLR:
            if(1 != sscanf(data,"%x",&utmp32) )boom("Invalid CLR:",lineno);
            tsp.color.Red      = (utmp32 >> 4) & 0xFF;
            tsp.color.Green    = (utmp32 >> 2) & 0xFF;
            tsp.color.Blue     = (utmp32 >> 0) & 0xFF;
            tsp.color.Reserved = 0;
            break;
         case OPFLAGS:
            if(1 != sscanf(data,"%d",&flags) )boom("Invalid FLAG:",lineno);
            break;
         case OPEMIT:
            TR_layout_analyze(tri);
            TR_layout_2_svg(tri);
            flush_as_svg(tri, fpo);
            tri = trinfo_clear(tri);
            break;
         case OPDONE:
            TR_layout_analyze(tri);
            TR_layout_2_svg(tri);
            flush_as_svg(tri, fpo);
            tri = trinfo_clear(tri);
            ok = 0;
            break;
         case OPOOPS:
         default:
            boom("Input line cannot be parsed",lineno);
            break;
      }
      
   }

   if(fpo){
      fpo=close_as_svg(tri, fpo); 
   }


#ifdef DBG_LOOP
   tri = trinfo_clear(tri);
   ok  = 1;
   }
#endif /* DBG_LOOP */

   fclose(fpi);
   tri = trinfo_release(tri);
   free(infile);

   exit(EXIT_SUCCESS);
}
#endif /* TEST */

#ifdef __cplusplus
}
#endif
