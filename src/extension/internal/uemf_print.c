/**
  @file uemf_print.c Functions for printing EMF records
*/

/*
File:      uemf_print.c
Version:   0.0.9
Date:      19-OCT-2012
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2012 David Mathog and California Institute of Technology (Caltech)
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "uemf.h"

/** 
    \brief Print some number of hex bytes
    \param buf pointer to the first byte
    \param num number of bytes
*/
void hexbytes_print(uint8_t *buf,unsigned int num){
   for(; num; num--,buf++){
      printf("%2.2X",*buf);
   }
}

/* **********************************************************************************************
   These functions print standard objects used in the EMR records.
   The low level ones do not append EOL.
*********************************************************************************************** */



/** 
    \brief Print a U_COLORREF object.
    \param color  U_COLORREF object    
*/
void colorref_print(
      U_COLORREF color
   ){
   printf("{%u,%u,%u} ",color.Red,color.Green,color.Blue);
}


/**
    \brief Print a U_RGBQUAD object.
    \param color  U_RGBQUAD object    
*/
void rgbquad_print(
      U_RGBQUAD color
   ){
   printf("{%u,%u,%u,%u} ",color.Blue,color.Green,color.Red,color.Reserved);
}

/**
    \brief Print rect and rectl objects from Upper Left and Lower Right corner points.
    \param rect U_RECTL object
*/
void rectl_print(
      U_RECTL rect
    ){
    printf("{%d,%d,%d,%d} ",rect.left,rect.top,rect.right,rect.bottom);
}

/**
    \brief Print a U_SIZEL object.
    \param sz U_SizeL object
*/
void sizel_print(
       U_SIZEL sz
    ){
    printf("{%d,%d} ",sz.cx ,sz.cy);
} 

/**
    \brief Print a U_POINTL object
    \param pt U_POINTL object
*/
void pointl_print(
       U_POINTL pt
    ){
    printf("{%d,%d} ",pt.x ,pt.y);
} 

/**
    \brief Print a U_POINT16 object
    \param pt U_POINT16 object
*/
void point16_print(
       U_POINT16 pt
    ){
    printf("{%d,%d} ",pt.x ,pt.y);
} 

/**
    \brief Print a U_LCS_GAMMA object
    \param lg U_LCS_GAMMA object
*/
void lcs_gamma_print(
      U_LCS_GAMMA lg
   ){
   uint8_t tmp;
   tmp = lg.ignoreHi; printf("ignoreHi:%u ",tmp);
   tmp = lg.intPart ; printf("intPart :%u ",tmp);
   tmp = lg.fracPart; printf("fracPart:%u ",tmp);
   tmp = lg.ignoreLo; printf("ignoreLo:%u ",tmp);
}

/**
    \brief Print a U_LCS_GAMMARGB object
    \param lgr U_LCS_GAMMARGB object
*/
void lcs_gammargb_print(
      U_LCS_GAMMARGB lgr
   ){
   printf("lcsGammaRed:");   lcs_gamma_print(lgr.lcsGammaRed  );
   printf("lcsGammaGreen:"); lcs_gamma_print(lgr.lcsGammaGreen);
   printf("lcsGammaBlue:");  lcs_gamma_print(lgr.lcsGammaBlue );
}

/**
    \brief Print a U_TRIVERTEX object.
    \param tv U_TRIVERTEX object.
*/
void trivertex_print(
      U_TRIVERTEX tv
   ){
   printf("{{%d,%d},{%u,%u,%u,%u}} ",tv.x,tv.y,tv.Red,tv.Green,tv.Blue,tv.Alpha);
}

/**
    \brief Print a U_GRADIENT3 object.
    \param tv U_GRADIENT3 object.
*/
void gradient3_print(
      U_GRADIENT3 g3
   ){
   printf("{%u,%u,%u} ",g3.Vertex1,g3.Vertex2,g3.Vertex3);
}

/**
    \brief Print a U_GRADIENT4 object.
    \param tv U_GRADIENT4 object.
*/
void gradient4_print(
      U_GRADIENT4 g4
   ){
   printf("{%u,%u} ",g4.UpperLeft,g4.LowerRight);
}

/**
    \brief Print a U_LOGBRUSH object.
    \param lb U_LOGBRUSH object.
*/
void logbrush_print(
      U_LOGBRUSH lb  
   ){
    printf("lbStyle:0x%8.8X ",  lb.lbStyle);
    printf("lbColor:");         colorref_print(lb.lbColor);
    printf("lbHatch:0x%8.8X ",  lb.lbHatch);
}

/**
    \brief Print a U_XFORM object.
    \param xform U_XFORM object
*/
void xform_print(
      U_XFORM xform
   ){
   printf("{%f,%f.%f,%f,%f,%f} ",xform.eM11,xform.eM12,xform.eM21,xform.eM22,xform.eDx,xform.eDy);
}

/**
  \brief Print a U_CIEXYZ object
  \param ciexyz U_CIEXYZ object
*/
void ciexyz_print(
      U_CIEXYZ ciexyz
   ){
   printf("{%d,%d.%d} ",ciexyz.ciexyzX,ciexyz.ciexyzY,ciexyz.ciexyzZ);
    
}

/**
  \brief Print a U_CIEXYZTRIPLE object
  \param cie3 U_CIEXYZTRIPLE object
*/
void ciexyztriple_print(
      U_CIEXYZTRIPLE cie3
   ){
   printf("{Red:");     ciexyz_print(cie3.ciexyzRed  );
   printf(", Green:");  ciexyz_print(cie3.ciexyzGreen);
   printf(", Blue:");   ciexyz_print(cie3.ciexyzBlue );
   printf("} ");
}
/**
    \brief Print a U_LOGCOLORSPACEA object.
    \param lcsa     U_LOGCOLORSPACEA object
*/
void logcolorspacea_print(
      U_LOGCOLORSPACEA lcsa
   ){
   printf("lcsSignature:%u ",lcsa.lcsSignature);
   printf("lcsVersion:%u ",  lcsa.lcsVersion  );
   printf("lcsSize:%u ",     lcsa.lcsSize     );
   printf("lcsCSType:%d ",    lcsa.lcsCSType   );
   printf("lcsIntent:%d ",    lcsa.lcsIntent   );
   printf("lcsEndpoints:");   ciexyztriple_print(lcsa.lcsEndpoints);
   printf("lcsGammaRGB: ");   lcs_gammargb_print(lcsa.lcsGammaRGB );
   printf("filename:%s ",     lcsa.lcsFilename );
}

/**

    \brief Print a U_LOGCOLORSPACEW object.
    \param lcsa U_LOGCOLORSPACEW object                                               
*/
void logcolorspacew_print(
      U_LOGCOLORSPACEW lcsa
   ){
   char *string;
   printf("lcsSignature:%d ",lcsa.lcsSignature);
   printf("lcsVersion:%d ",  lcsa.lcsVersion  );
   printf("lcsSize:%d ",     lcsa.lcsSize     );
   printf("lcsCSType:%d ",   lcsa.lcsCSType   );
   printf("lcsIntent:%d ",   lcsa.lcsIntent   );
   printf("lcsEndpoints:");   ciexyztriple_print(lcsa.lcsEndpoints);
   printf("lcsGammaRGB: ");   lcs_gammargb_print(lcsa.lcsGammaRGB );
   string = U_Utf16leToUtf8(lcsa.lcsFilename, U_MAX_PATH, NULL);
   printf("filename:%s ",   string );
   free(string);
}

/**
    \brief Print a U_PANOSE object.
    \param panose U_PANOSE object
*/
void panose_print(
      U_PANOSE panose
    ){
    printf("bFamilyType:%u ",     panose.bFamilyType     );
    printf("bSerifStyle:%u ",     panose.bSerifStyle     );
    printf("bWeight:%u ",         panose.bWeight         );
    printf("bProportion:%u ",     panose.bProportion     );
    printf("bContrast:%u ",       panose.bContrast       );
    printf("bStrokeVariation:%u ",panose.bStrokeVariation);
    printf("bArmStyle:%u ",       panose.bArmStyle       );
    printf("bLetterform:%u ",     panose.bLetterform     );
    printf("bMidline:%u ",        panose.bMidline        );
    printf("bXHeight:%u ",        panose.bXHeight        );
}

/**
    \brief Print a U_LOGFONT object.
    \param lf   U_LOGFONT object
*/
void logfont_print(
       U_LOGFONT lf
   ){
   char *string;
   printf("lfHeight:%d ",            lf.lfHeight        );
   printf("lfWidth:%d ",             lf.lfWidth         );
   printf("lfEscapement:%d ",        lf.lfEscapement    );
   printf("lfOrientation:%d ",       lf.lfOrientation   );
   printf("lfWeight:%d ",            lf.lfWeight        );
   printf("lfItalic:0x%2.2X ",         lf.lfItalic        );
   printf("lfUnderline:0x%2.2X ",      lf.lfUnderline     );
   printf("lfStrikeOut:0x%2.2X ",      lf.lfStrikeOut     );
   printf("lfCharSet:0x%2.2X ",        lf.lfCharSet       );
   printf("lfOutPrecision:0x%2.2X ",   lf.lfOutPrecision  );
   printf("lfClipPrecision:0x%2.2X ",  lf.lfClipPrecision );
   printf("lfQuality:0x%2.2X ",        lf.lfQuality       );
   printf("lfPitchAndFamily:0x%2.2X ", lf.lfPitchAndFamily);
     string = U_Utf16leToUtf8(lf.lfFaceName, U_LF_FACESIZE, NULL);
   printf("lfFaceName:%s ",   string );
   free(string);
}

/**
    \brief Print a U_LOGFONT_PANOSE object.
    \return U_LOGFONT_PANOSE object
*/
void logfont_panose_print(
      U_LOGFONT_PANOSE lfp
   ){    
   char *string;
   printf("elfLogFont:");       logfont_print(lfp.elfLogFont);
     string = U_Utf16leToUtf8(lfp.elfFullName, U_LF_FULLFACESIZE, NULL);
   printf("elfFullName:%s ",    string );
   free(string);
     string = U_Utf16leToUtf8(lfp.elfStyle, U_LF_FACESIZE, NULL);
   printf("elfStyle:%s ",       string );
   free(string);
   printf("elfVersion:%u "      ,lfp.elfVersion  );
   printf("elfStyleSize:%u "    ,lfp.elfStyleSize);
   printf("elfMatch:%u "        ,lfp.elfMatch    );
   printf("elfReserved:%u "     ,lfp.elfReserved );
   printf("elfVendorId:");      hexbytes_print((uint8_t *)lfp.elfVendorId,U_ELF_VENDOR_SIZE); printf(" ");
   printf("elfCulture:%u "      ,lfp.elfCulture  );
   printf("elfPanose:");        panose_print(lfp.elfPanose);
}

/**
    \brief Print a U_BITMAPINFOHEADER object.
    \param Bmi U_BITMAPINFOHEADER object 
*/
void bitmapinfoheader_print(
      U_BITMAPINFOHEADER Bmi
   ){
   printf("biSize:%u "            ,Bmi.biSize         );
   printf("biWidth:%d "            ,Bmi.biWidth        );
   printf("biHeight:%d "           ,Bmi.biHeight       );
   printf("biPlanes:%u "          ,Bmi.biPlanes       );
   printf("biBitCount:%u "        ,Bmi.biBitCount     );
   printf("biCompression:%u "     ,Bmi.biCompression  );
   printf("biSizeImage:%u "       ,Bmi.biSizeImage    );
   printf("biXPelsPerMeter:%d "    ,Bmi.biXPelsPerMeter);
   printf("biYPelsPerMeter:%d "    ,Bmi.biYPelsPerMeter);
   printf("biClrUsed:%u "         ,Bmi.biClrUsed      );
   printf("biClrImportant:%u "    ,Bmi.biClrImportant );
}


/**
    \brief Print a Pointer to a U_BITMAPINFO object.
    \param Bmi Pointer to a U_BITMAPINFO object
*/
void bitmapinfo_print(
      PU_BITMAPINFO Bmi
   ){
   int i;
   PU_RGBQUAD BmiColors;
   PU_BITMAPINFOHEADER BmiHeader = &(Bmi->bmiHeader);
   printf("BmiHeader: ");  bitmapinfoheader_print(*BmiHeader);
   if(BmiHeader->biClrUsed){
     BmiColors = (PU_RGBQUAD) ((char *)Bmi + sizeof(U_BITMAPINFOHEADER));
     for(i=0; i<BmiHeader->biClrUsed; i++){
        printf("%d:",i); rgbquad_print(BmiColors[i]);
     }
   }
}

/**
    \brief Print a U_BLEND object.
    \param blend a U_BLEND object
*/
void blend_print(
      U_BLEND blend
   ){
   printf("Operation:%u " ,blend.Operation);
   printf("Flags:%u "     ,blend.Flags    );
   printf("Global:%u "    ,blend.Global   );
   printf("Op:%u "        ,blend.Op       );
}

/**
    \brief Print a pointer to a U_EXTLOGPEN object.
    \param elp   PU_EXTLOGPEN object
*/
void extlogpen_print(
      PU_EXTLOGPEN elp
   ){
   int i;
   U_STYLEENTRY *elpStyleEntry;
   printf("elpPenStyle:0x%8.8X "   ,elp->elpPenStyle  );
   printf("elpWidth:%u "           ,elp->elpWidth     );
   printf("elpBrushStyle:0x%8.8X " ,elp->elpBrushStyle);
   printf("elpColor");              colorref_print(elp->elpColor);
   printf("elpHatch:%d "           ,elp->elpHatch     );
   printf("elpNumEntries:%u "      ,elp->elpNumEntries);
   if(elp->elpNumEntries){
     printf("elpStyleEntry:");
     elpStyleEntry = (uint32_t *) elp->elpStyleEntry;
     for(i=0;i<elp->elpNumEntries;i++){
        printf("%d:%u ",i,elpStyleEntry[i]);
     }
   }
}

/**
    \brief Print a U_LOGPEN object.
    \param lp  U_LOGPEN object
    
*/
void logpen_print(
      U_LOGPEN lp
   ){
   printf("lopnStyle:%u "    ,lp.lopnStyle  );
   printf("lopnWidth:");      pointl_print(  lp.lopnWidth  );
   printf("lopnColor:");      colorref_print(lp.lopnColor );
} 

/**
    \brief Print a U_LOGPLTNTRY object.
    \param lpny Ignore U_LOGPLTNTRY object.
*/
void logpltntry_print(
      U_LOGPLTNTRY lpny
   ){
   printf("peReserved:%u " ,lpny.peReserved );
   printf("peRed:%u "      ,lpny.peRed      );
   printf("peGreen:%u "    ,lpny.peGreen    );
   printf("peBlue:%u "     ,lpny.peBlue     );
}

/**
    \brief Print a pointer to a U_LOGPALETTE object.
    \param lp  Pointer to a U_LOGPALETTE object.
*/
void logpalette_print(
      PU_LOGPALETTE lp
   ){
   int            i;
   PU_LOGPLTNTRY palPalEntry;
   printf("palVersion:%u ",    lp->palVersion );
   printf("palNumEntries:%u ", lp->palNumEntries );
   if(lp->palNumEntries){
     palPalEntry = (PU_LOGPLTNTRY) &(lp->palPalEntry);
     for(i=0;i<lp->palNumEntries;i++){
        printf("%d:",i); logpltntry_print(palPalEntry[i]);
     }
   }
}

/**
    \brief Print a U_RGNDATAHEADER object.
    \param rdh  U_RGNDATAHEADER object
*/
void rgndataheader_print(
      U_RGNDATAHEADER rdh
   ){
   printf("dwSize:%u ",   rdh.dwSize   );
   printf("iType:%u ",    rdh.iType    );
   printf("nCount:%u ",   rdh.nCount   );
   printf("nRgnSize:%u ", rdh.nRgnSize );
   printf("rclBounds:");  rectl_print(rdh.rclBounds  );
}

/**
    \brief Print a pointer to a U_RGNDATA object.
    \param rgd  pointer to a U_RGNDATA object.
*/
void rgndata_print(
      PU_RGNDATA rd
   ){
   int i;
   PU_RECTL rects;
   printf("rdh:");     rgndataheader_print(rd->rdh );
   if(rd->rdh.nCount){
     rects = (PU_RECTL) &(rd->Buffer);
     for(i=0;i<rd->rdh.nCount;i++){
        printf("%d:",i); rectl_print(rects[i]);
     }
   }
}

/**
    \brief Print a U_COLORADJUSTMENT object.
    \param ca U_COLORADJUSTMENT object.        
*/
void coloradjustment_print(
      U_COLORADJUSTMENT ca
   ){
   printf("caSize:%u "            ,ca.caSize           );
   printf("caFlags:%u "           ,ca.caFlags          );
   printf("caIlluminantIndex:%u " ,ca.caIlluminantIndex);
   printf("caRedGamma:%u "        ,ca.caRedGamma       );
   printf("caGreenGamma:%u "      ,ca.caGreenGamma     );
   printf("caBlueGamma:%u "       ,ca.caBlueGamma      );
   printf("caReferenceBlack:%u "  ,ca.caReferenceBlack );
   printf("caReferenceWhite:%u "  ,ca.caReferenceWhite );
   printf("caContrast:%d "         ,ca.caContrast       );
   printf("caBrightness:%d "       ,ca.caBrightness     );
   printf("caColorfulness:%d "     ,ca.caColorfulness   );
   printf("caRedGreenTint:%d "     ,ca.caRedGreenTint   );
}

/**
    \brief Print a U_PIXELFORMATDESCRIPTOR object.
    \return U_PIXELFORMATDESCRIPTOR object
    \param dwFlags         PFD_dwFlags Enumeration
    \param iPixelType      PFD_iPixelType Enumeration
    \param cColorBits      RGBA: total bits per pixel
    \param cRedBits        Red   bits per pixel
    \param cRedShift       Red   shift to data bits
    \param cGreenBits      Green bits per pixel
    \param cGreenShift     Green shift to data bits
    \param cBlueBits       Blue  bits per pixel
    \param cBlueShift      Blue  shift to data bits
    \param cAlphaBits      Alpha bits per pixel
    \param cAlphaShift     Alpha shift to data bits
    \param cAccumBits      Accumulator buffer, total bitplanes
    \param cAccumRedBits   Red   accumulator buffer bitplanes
    \param cAccumGreenBits Green accumulator buffer bitplanes
    \param cAccumBlueBits  Blue  accumulator buffer bitplanes
    \param cAccumAlphaBits Alpha accumulator buffer bitplanes
    \param cDepthBits      Depth of Z-buffer
    \param cStencilBits    Depth of stencil buffer
    \param cAuxBuffers     Depth of auxilliary buffers (not supported)
    \param iLayerType      PFD_iLayerType Enumeration, may be ignored
    \param bReserved       Bits 0:3/4:7 are number of Overlay/Underlay planes 
    \param dwLayerMask     may be ignored
    \param dwVisibleMask   color or index of underlay plane
    \param dwDamageMask    may be ignored
*/
void pixelformatdescriptor_print(
      U_PIXELFORMATDESCRIPTOR pfd
   ){
   printf("nSize:%u "           ,pfd.nSize           );
   printf("nVersion:%u "        ,pfd.nVersion        );
   printf("dwFlags:%u "         ,pfd.dwFlags         );
   printf("iPixelType:%u "      ,pfd.iPixelType      );
   printf("cColorBits:%u "      ,pfd.cColorBits      );
   printf("cRedBits:%u "        ,pfd.cRedBits        );
   printf("cRedShift:%u "       ,pfd.cRedShift       );
   printf("cGreenBits:%u "      ,pfd.cGreenBits      );
   printf("cGreenShift:%u "     ,pfd.cGreenShift     );
   printf("cBlueBits:%u "       ,pfd.cBlueBits       );
   printf("cBlueShift:%u "      ,pfd.cBlueShift      );
   printf("cAlphaBits:%u "      ,pfd.cAlphaBits      );
   printf("cAlphaShift:%u "     ,pfd.cAlphaShift     );
   printf("cAccumBits:%u "      ,pfd.cAccumBits      );
   printf("cAccumRedBits:%u "   ,pfd.cAccumRedBits   );
   printf("cAccumGreenBits:%u " ,pfd.cAccumGreenBits );
   printf("cAccumBlueBits:%u "  ,pfd.cAccumBlueBits  );
   printf("cAccumAlphaBits:%u " ,pfd.cAccumAlphaBits );
   printf("cDepthBits:%u "      ,pfd.cDepthBits      );
   printf("cStencilBits:%u "    ,pfd.cStencilBits    );
   printf("cAuxBuffers:%u "     ,pfd.cAuxBuffers     );
   printf("iLayerType:%u "      ,pfd.iLayerType      );
   printf("bReserved:%u "       ,pfd.bReserved       );
   printf("dwLayerMask:%u "     ,pfd.dwLayerMask     );
   printf("dwVisibleMask:%u "   ,pfd.dwVisibleMask   );
   printf("dwDamageMask:%u "    ,pfd.dwDamageMask    );
}

/**
    \brief Print a Pointer to a U_EMRTEXT record
    \param emt      Pointer to a U_EMRTEXT record 
    \param record   Pointer to the start of the record which contains this U_ERMTEXT
    \param type     0 for 8 bit character, anything else for 16 
*/
void emrtext_print(
      char *emt,
      char *record,
      int   type
   ){
   int i,off;
   char *string;
   PU_EMRTEXT pemt = (PU_EMRTEXT) emt;
   // constant part
   printf("ptlReference:");   pointl_print(pemt->ptlReference);
   printf("nChars:%u "       ,pemt->nChars      );
   printf("offString:%u "    ,pemt->offString   );
   if(pemt->offString){
      if(!type){
         printf("string8:<%s> ",record + pemt->offString);
      }
      else {
         string = U_Utf16leToUtf8((uint16_t *)(record + pemt->offString), pemt->nChars, NULL);
         printf("string16:<%s> ",string);
         free(string);
      }
   }
   printf("fOptions:%u "     ,pemt->fOptions    );
   off = sizeof(U_EMRTEXT);
   if(!(pemt->fOptions & U_ETO_NO_RECT)){
      printf("rcl");   rectl_print( *((U_RECTL *)(emt+off)) );
      off += sizeof(U_RECTL);
   }
   printf("offDx:%u "        , *((U_OFFDX *)(emt+off))   ); off = *(U_OFFDX *)(emt+off);
   printf("Dx:");
   for(i=0; i<pemt->nChars; i++, off+=sizeof(uint32_t)){
      printf("%d:", *((uint32_t *)(record+off))  );
   }
}




// hide these from Doxygen
//! @cond
/* **********************************************************************************************
These functions contain shared code used by various U_EMR*_print functions.  These should NEVER be called
by end user code and to further that end prototypes are NOT provided and they are hidden from Doxygen.


   These are (mostly) ordered by U_EMR_* index number.
   
   void core5_print(char *name, char *contents, int recnum, size_t off)
   
   The exceptions:
   void core3_print(char *name, char *label, char *contents, int recnum, size_t off)
   void core7_print(char *name, char *field1, char *field2, char *contents, int recnum, size_t off)
   void core8_print(char *name, char *contents, int recnum, size_t off, int type)
   
   
*********************************************************************************************** */

// all core*_print call this, U_EMRSETMARGN_print and some others all it directly
// numbered as core5 to be consistent with uemf.c, but must appear before the others as there is no prototype
void core5_print(char *name, char *contents, int recnum, size_t off){
   PU_ENHMETARECORD lpEMFR = (PU_ENHMETARECORD)(contents + off);
   printf("%-30srecord:%5d type:%3d offset:%8d size:%8d\n",name,recnum,lpEMFR->iType,(int) off,lpEMFR->nSize);
}

// Functions with the same form starting with U_EMRPOLYBEZIER_print
void core1_print(char *name, char *contents, int recnum, size_t off){
   int i;
   PU_EMRPOLYLINETO pEmr = (PU_EMRPOLYLINETO) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);    printf("\n");
   printf("   cptl:           %d\n",pEmr->cptl        );
   printf("   Points:         ");
   for(i=0;i<pEmr->cptl; i++){
      printf("[%d]:",i); pointl_print(pEmr->aptl[i]);
   }
   printf("\n");
}

// Functions with the same form starting with U_EMRPOLYPOLYLINE_print
void core2_print(char *name, char *contents, int recnum, size_t off){
   int i;
   PU_EMRPOLYPOLYGON pEmr = (PU_EMRPOLYPOLYGON) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);    printf("\n");
   printf("   nPolys:         %d\n",pEmr->nPolys        );
   printf("   cptl:           %d\n",pEmr->cptl          );
   printf("   Counts:         ");
   for(i=0;i<pEmr->nPolys; i++){
      printf(" [%d]:%d ",i,pEmr->aPolyCounts[i] );
   }
   printf("\n");
   PU_POINTL paptl = (PU_POINTL)((char *)pEmr->aPolyCounts + sizeof(uint32_t)* pEmr->nPolys);
   printf("   Points:         ");
   for(i=0;i<pEmr->cptl; i++){
      printf(" [%d]:",i); pointl_print(paptl[i]);
   }
   printf("\n");
}


// Functions with the same form starting with U_EMRSETMAPMODE_print
void core3_print(char *name, char *label, char *contents, int recnum, size_t off){
   PU_EMRSETMAPMODE pEmr   = (PU_EMRSETMAPMODE)(contents + off);
   core5_print(name, contents, recnum, off);
   if(!strcmp(label,"crColor:")){
      printf("   %-15s ",label); colorref_print(*(U_COLORREF *)&(pEmr->iMode)); printf("\n");
   }
   else if(!strcmp(label,"iMode:")){
      printf("   %-15s 0x%8.8X\n",label,pEmr->iMode     );
   }
   else {
      printf("   %-15s %d\n",label,pEmr->iMode        );
   }
} 

// Functions taking a single U_RECT or U_RECTL, starting with U_EMRELLIPSE_print, also U_EMRFILLPATH_print, 
void core4_print(char *name, char *contents, int recnum, size_t off){
   PU_EMRELLIPSE pEmr      = (PU_EMRELLIPSE)(   contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBox:         ");  rectl_print(pEmr->rclBox);  printf("\n");
} 

// Functions with the same form starting with U_EMRPOLYBEZIER16_print
void core6_print(char *name, char *contents, int recnum, size_t off){
   int i;
   PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);    printf("\n");
   printf("   cpts:           %d\n",pEmr->cpts        );
   printf("   Points:         ");
   PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
   for(i=0; i<pEmr->cpts; i++){
      printf(" [%d]:",i);  point16_print(papts[i]);
   }
   printf("\n");
} 


// Records with the same form starting with U_EMRSETWINDOWEXTEX_print
// CAREFUL, in the _set equivalents all functions with two uint32_t values are mapped here, and member names differ, consequently
//   print routines must supply the names of the two arguments.  These cannot be null.  If the second one is 
//   empty the values are printed as a pair {x,y}, otherwise each is printed with its own label on a separate line.
void core7_print(char *name, char *field1, char *field2, char *contents, int recnum, size_t off){
   PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR) (contents + off);
   core5_print(name, contents, recnum, off);
   if(*field2){
      printf("   %-15s %d\n",field1,pEmr->pair.x);
      printf("   %-15s %d\n",field2,pEmr->pair.y);
   }
   else {
      printf("   %-15s {%d,%d}\n",field1,pEmr->pair.x,pEmr->pair.y);
   } 
}

// For U_EMREXTTEXTOUTA and U_EMREXTTEXTOUTW, type=0 for the first one
void core8_print(char *name, char *contents, int recnum, size_t off, int type){
   PU_EMREXTTEXTOUTA pEmr = (PU_EMREXTTEXTOUTA) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   iGraphicsMode:  %u\n",pEmr->iGraphicsMode );
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);                              printf("\n");
   printf("   exScale:        %f\n",pEmr->exScale        );
   printf("   eyScale:        %f\n",pEmr->eyScale        );
   printf("   emrtext:        ");
      emrtext_print(contents + off + sizeof(U_EMREXTTEXTOUTA) - sizeof(U_EMRTEXT),contents + off,type);
      printf("\n");
} 

// Functions that take a rect and a pair of points, starting with U_EMRARC_print
void core9_print(char *name, char *contents, int recnum, size_t off){
   PU_EMRARC pEmr = (PU_EMRARC) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBox:         ");    rectl_print(pEmr->rclBox);    printf("\n");
   printf("   ptlStart:       ");  pointl_print(pEmr->ptlStart);   printf("\n");
   printf("   ptlEnd:         ");    pointl_print(pEmr->ptlEnd);   printf("\n");
}

// Functions with the same form starting with U_EMRPOLYPOLYLINE16_print
void core10_print(char *name, char *contents, int recnum, size_t off){
   int i;
   PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);    printf("\n");
   printf("   nPolys:         %d\n",pEmr->nPolys        );
   printf("   cpts:           %d\n",pEmr->cpts          );
   printf("   Counts:         ");
   for(i=0;i<pEmr->nPolys; i++){
      printf(" [%d]:%d ",i,pEmr->aPolyCounts[i] );
   }
   printf("\n");
   printf("   Points:         ");
   PU_POINT16 papts = (PU_POINT16)((char *)pEmr->aPolyCounts + sizeof(uint32_t)* pEmr->nPolys);
   for(i=0; i<pEmr->cpts; i++){
      printf(" [%d]:",i);  point16_print(papts[i]);
   }
   printf("\n");

} 

// Functions with the same form starting with  U_EMRINVERTRGN_print and U_EMRPAINTRGN_print,
void core11_print(char *name, char *contents, int recnum, size_t off){
   int i,roff;
   PU_EMRINVERTRGN pEmr = (PU_EMRINVERTRGN) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);    printf("\n");
   printf("   cbRgnData:      %d\n",pEmr->cbRgnData);
   // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
   roff=0;
   i=1; 
   char *prd = (char *) &(pEmr->RgnData);
   while(roff + 28 < pEmr->emr.nSize){ // up to the end of the record
      printf("   RegionData:%d",i);
      rgndata_print((PU_RGNDATA) (prd + roff));
      roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
      printf("\n");
   }
} 


// common code for U_EMRCREATEMONOBRUSH_print and U_EMRCREATEDIBPATTERNBRUSHPT_print,
void core12_print(char *name, char *contents, int recnum, size_t off){
   PU_EMRCREATEMONOBRUSH pEmr = (PU_EMRCREATEMONOBRUSH) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   ihBrush:      %u\n",pEmr->ihBrush );
   printf("   iUsage :      %u\n",pEmr->iUsage  );
   printf("   offBmi :      %u\n",pEmr->offBmi  );
   printf("   cbBmi  :      %u\n",pEmr->cbBmi   );
   if(pEmr->cbBmi){
      printf("      bitmap:");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmi));
      printf("\n");
   }
   printf("   offBits:      %u\n",pEmr->offBits );
   printf("   cbBits :      %u\n",pEmr->cbBits  );
}

// common code for U_EMRALPHABLEND_print and U_EMRTRANSPARENTBLT_print,
void core13_print(char *name, char *contents, int recnum, size_t off){
   PU_EMRALPHABLEND pEmr = (PU_EMRALPHABLEND) (contents + off);
   core5_print(name, contents, recnum, off);
   printf("   rclBounds:      ");    rectl_print( pEmr->rclBounds);       printf("\n");
   printf("   Dest:           ");    pointl_print(pEmr->Dest);            printf("\n");
   printf("   cDest:          ");    pointl_print(pEmr->cDest);           printf("\n");
   printf("   Blend:          ");    blend_print(pEmr->Blend);            printf("\n");
   printf("   Src:            ");    pointl_print(pEmr->Src);             printf("\n");
   printf("   xformSrc:       ");    xform_print( pEmr->xformSrc);        printf("\n");
   printf("   crBkColorSrc:   ");    colorref_print( pEmr->crBkColorSrc); printf("\n");
   printf("   iUsageSrc:      %u\n",pEmr->iUsageSrc   );
   printf("   offBmiSrc:      %u\n",pEmr->offBmiSrc   );
   printf("   cbBmiSrc:       %u\n",pEmr->cbBmiSrc    );
   if(pEmr->cbBmiSrc){
      printf("      bitmap:");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiSrc));
      printf("\n");
   }
   printf("   offBitsSrc:     %u\n",pEmr->offBitsSrc  );
   printf("   cbBitsSrc:      %u\n",pEmr->cbBitsSrc   );
}
//! @endcond

/* **********************************************************************************************
These are the core EMR functions, each creates a particular type of record.  
All return these records via a char* pointer, which is NULL if the call failed.  
They are listed in order by the corresponding U_EMR_* index number.  
*********************************************************************************************** */

/**
    \brief Print a pointer to a U_EMR_whatever record which has not been implemented.
    \param name       name of this type of record
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRNOTIMPLEMENTED_print(char *name, char *contents, int recnum, size_t off){
   PU_ENHMETARECORD lpEMFR = (PU_ENHMETARECORD)(contents + off);
   printf("%-30srecord:%5d type:%3d offset:%8d size:%8d\n",name,recnum,lpEMFR->iType,(int) off,lpEMFR->nSize);
   printf("   Not Implemented!\n");
}

// U_EMRHEADER                1
/**
    \brief Print a pointer to a U_EMR_HEADER record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRHEADER_print(char *contents, int recnum, size_t off){
   char *string;
   int  p1len;
   core5_print("U_EMRHEADER", contents, recnum, off);

   PU_EMRHEADER pEmr = (PU_EMRHEADER)(contents+off);
   printf("   rclBounds:      ");          rectl_print( pEmr->rclBounds);   printf("\n");
   printf("   rclFrame:       ");          rectl_print( pEmr->rclFrame);    printf("\n");
   printf("   dSignature:     0x%8.8X\n",  pEmr->dSignature    );
   printf("   nVersion:       0x%8.8X\n",  pEmr->nVersion      );
   printf("   nBytes:         %d\n",       pEmr->nBytes        );
   printf("   nRecords:       %d\n",       pEmr->nRecords      );
   printf("   nHandles:       %d\n",       pEmr->nHandles      );
   printf("   sReserved:      %d\n",       pEmr->sReserved     );
   printf("   nDescription:   %d\n",       pEmr->nDescription  );
   printf("   offDescription: %d\n",       pEmr->offDescription);
   if(pEmr->offDescription){
      string = U_Utf16leToUtf8((uint16_t *)((char *) pEmr + pEmr->offDescription), pEmr->nDescription, NULL);
      printf("      Desc. A:  %s\n",string);
      free(string);
      p1len = 2 + 2*wchar16len((uint16_t *)((char *) pEmr + pEmr->offDescription));
      string = U_Utf16leToUtf8((uint16_t *)((char *) pEmr + pEmr->offDescription + p1len), pEmr->nDescription, NULL);
      printf("      Desc. B:  %s\n",string);
      free(string);
   }
   printf("   nPalEntries:    %d\n",       pEmr->nPalEntries   );
   printf("   szlDevice:      {%d,%d} \n", pEmr->szlDevice.cx,pEmr->szlDevice.cy);
   printf("   szlMillimeters: {%d,%d} \n", pEmr->szlMillimeters.cx,pEmr->szlMillimeters.cy);
   if((pEmr->nDescription && (pEmr->offDescription >= 100)) || 
      (!pEmr->offDescription && pEmr->emr.nSize >= 100)
     ){
      printf("   cbPixelFormat:  %d\n",       pEmr->cbPixelFormat );
      printf("   offPixelFormat: %d\n",       pEmr->offPixelFormat);
      if(pEmr->cbPixelFormat){
         printf("      PFD:");
         pixelformatdescriptor_print( *(PU_PIXELFORMATDESCRIPTOR) (contents + off + pEmr->offPixelFormat));
         printf("\n");
      }
      printf("   bOpenGL:        %d\n",pEmr->bOpenGL       );
      if((pEmr->nDescription    && (pEmr->offDescription >= 108)) || 
              (pEmr->cbPixelFormat   && (pEmr->offPixelFormat >=108)) ||
              (!pEmr->offDescription && !pEmr->cbPixelFormat && pEmr->emr.nSize >= 108)
             ){
         printf("   szlMicrometers: {%d,%d} \n", pEmr->szlMicrometers.cx,pEmr->szlMicrometers.cy);
     }
   }
}

// U_EMRPOLYBEZIER                       2
/**
    \brief Print a pointer to a U_EMR_POLYBEZIER record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYBEZIER_print(char *contents, int recnum, size_t off){
   core1_print("U_EMRPOLYBEZIER", contents, recnum, off);
} 

// U_EMRPOLYGON                          3
/**
    \brief Print a pointer to a U_EMR_POLYGON record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
 */
void U_EMRPOLYGON_print(char *contents, int recnum, size_t off){
   core1_print("U_EMRPOLYGON", contents, recnum, off);
} 


// U_EMRPOLYLINE              4
/**
    \brief Print a pointer to a U_EMR_POLYLINE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYLINE_print(char *contents, int recnum, size_t off){
   core1_print("U_EMRPOLYLINE", contents, recnum, off);
} 

// U_EMRPOLYBEZIERTO          5
/**
    \brief Print a pointer to a U_EMR_POLYBEZIERTO record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYBEZIERTO_print(char *contents, int recnum, size_t off){
   core1_print("U_EMRPOLYBEZIERTO", contents, recnum, off);
} 

// U_EMRPOLYLINETO            6
/**
    \brief Print a pointer to a U_EMR_POLYLINETO record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYLINETO_print(char *contents, int recnum, size_t off){
   core1_print("U_EMRPOLYLINETO", contents, recnum, off);
} 

// U_EMRPOLYPOLYLINE          7
/**
    \brief Print a pointer to a U_EMR_POLYPOLYLINE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYPOLYLINE_print(char *contents, int recnum, size_t off){
   core2_print("U_EMRPOLYPOLYLINE", contents, recnum, off);
} 

// U_EMRPOLYPOLYGON           8
/**
    \brief Print a pointer to a U_EMR_POLYPOLYGON record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYPOLYGON_print(char *contents, int recnum, size_t off){
   core2_print("U_EMRPOLYPOLYGON", contents, recnum, off);
} 

// U_EMRSETWINDOWEXTEX        9
/**
    \brief Print a pointer to a U_EMR_SETWINDOWEXTEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETWINDOWEXTEX_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRSETWINDOWEXTEX", "szlExtent:","",contents, recnum, off);
} 

// U_EMRSETWINDOWORGEX       10
/**
    \brief Print a pointer to a U_EMR_SETWINDOWORGEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETWINDOWORGEX_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRSETWINDOWORGEX", "ptlOrigin:","",contents, recnum, off);
} 

// U_EMRSETVIEWPORTEXTEX     11
/**
    \brief Print a pointer to a U_EMR_SETVIEWPORTEXTEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETVIEWPORTEXTEX_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRSETVIEWPORTEXTEX", "szlExtent:","",contents, recnum, off);
} 

// U_EMRSETVIEWPORTORGEX     12
/**
    \brief Print a pointer to a U_EMR_SETVIEWPORTORGEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETVIEWPORTORGEX_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRSETVIEWPORTORGEX", "ptlOrigin:","",contents, recnum, off);
} 

// U_EMRSETBRUSHORGEX        13
/**
    \brief Print a pointer to a U_EMR_SETBRUSHORGEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETBRUSHORGEX_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRSETBRUSHORGEX", "ptlOrigin:","",contents, recnum, off);
} 

// U_EMREOF                  14
/**
    \brief Print a pointer to a U_EMR_EOF record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREOF_print(char *contents, int recnum, size_t off){
   core5_print("U_EMREOF", contents, recnum, off);

   PU_EMREOF pEmr = (PU_EMREOF)(contents+off);
   printf("   cbPalEntries:   %u\n",      pEmr->cbPalEntries );
   printf("   offPalEntries:  %u\n",      pEmr->offPalEntries);
   if(pEmr->cbPalEntries){
     printf("      PE:");
     logpalette_print( (PU_LOGPALETTE)(contents + off + pEmr->offPalEntries));
     printf("\n");
   }
} 


// U_EMRSETPIXELV            15
/**
    \brief Print a pointer to a U_EMR_SETPIXELV record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETPIXELV_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRSETPIXELV", contents, recnum, off);
   PU_EMRSETPIXELV pEmr = (PU_EMRSETPIXELV)(contents+off);
   printf("   ptlPixel:       ");  pointl_print(  pEmr->ptlPixel);  printf("\n");
   printf("   crColor:        ");  colorref_print(pEmr->crColor);   printf("\n");
} 


// U_EMRSETMAPPERFLAGS       16
/**
    \brief Print a pointer to a U_EMR_SETMAPPERFLAGS record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETMAPPERFLAGS_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRSETMAPPERFLAGS", contents, recnum, off);
   PU_EMRSETMAPPERFLAGS pEmr = (PU_EMRSETMAPPERFLAGS)(contents+off);
   printf("   dwFlags:        %u\n",pEmr->dwFlags);
} 


// U_EMRSETMAPMODE           17
/**
    \brief Print a pointer to a U_EMR_SETMAPMODE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETMAPMODE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETMAPMODE", "iMode:", contents, recnum, off);
}

// U_EMRSETBKMODE            18
/**
    \brief Print a pointer to a U_EMR_SETBKMODE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETBKMODE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETBKMODE", "iMode:", contents, recnum, off);
}

// U_EMRSETPOLYFILLMODE      19
/**
    \brief Print a pointer to a U_EMR_SETPOLYFILLMODE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETPOLYFILLMODE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETPOLYFILLMODE", "iMode:", contents, recnum, off);
}

// U_EMRSETROP2              20
/**
    \brief Print a pointer to a U_EMR_SETROP2 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETROP2_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETROP2", "dwRop:", contents, recnum, off);
}

// U_EMRSETSTRETCHBLTMODE    21
/**
    \brief Print a pointer to a U_EMR_SETSTRETCHBLTMODE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETSTRETCHBLTMODE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETSTRETCHBLTMODE", "iMode:", contents, recnum, off);
}

// U_EMRSETTEXTALIGN         22
/**
    \brief Print a pointer to a U_EMR_SETTEXTALIGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETTEXTALIGN_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETTEXTALIGN", "iMode:", contents, recnum, off);
}

// U_EMRSETCOLORADJUSTMENT   23
/**
    \brief Print a pointer to a U_EMR_SETCOLORADJUSTMENT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETCOLORADJUSTMENT_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRSETCOLORADJUSTMENT", contents, recnum, off);
   PU_EMRSETCOLORADJUSTMENT pEmr = (PU_EMRSETCOLORADJUSTMENT)(contents+off);
   printf("   ColorAdjustment:");
   coloradjustment_print(pEmr->ColorAdjustment);
   printf("\n");
}

// U_EMRSETTEXTCOLOR         24
/**
    \brief Print a pointer to a U_EMR_SETTEXTCOLOR record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETTEXTCOLOR_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETTEXTCOLOR", "crColor:", contents, recnum, off);
}

// U_EMRSETBKCOLOR           25
/**
    \brief Print a pointer to a U_EMR_SETBKCOLOR record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETBKCOLOR_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETBKCOLOR", "crColor:", contents, recnum, off);
}

// U_EMROFFSETCLIPRGN        26
/**
    \brief Print a pointer to a U_EMR_OFFSETCLIPRGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMROFFSETCLIPRGN_print(char *contents, int recnum, size_t off){
   core7_print("U_EMROFFSETCLIPRGN", "ptl:","",contents, recnum, off);
} 

// U_EMRMOVETOEX             27
/**
    \brief Print a pointer to a U_EMR_MOVETOEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRMOVETOEX_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRMOVETOEX", "ptl:","",contents, recnum, off);
} 

// U_EMRSETMETARGN           28
/**
    \brief Print a pointer to a U_EMR_SETMETARGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETMETARGN_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRSETMETARGN", contents, recnum, off);
}

// U_EMREXCLUDECLIPRECT      29
/**
    \brief Print a pointer to a U_EMR_EXCLUDECLIPRECT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREXCLUDECLIPRECT_print(char *contents, int recnum, size_t off){
   core4_print("U_EMREXCLUDECLIPRECT", contents, recnum, off);
}

// U_EMRINTERSECTCLIPRECT    30
/**
    \brief Print a pointer to a U_EMR_INTERSECTCLIPRECT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRINTERSECTCLIPRECT_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRINTERSECTCLIPRECT", contents, recnum, off);
}

// U_EMRSCALEVIEWPORTEXTEX   31
/**
    \brief Print a pointer to a U_EMR_SCALEVIEWPORTEXTEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSCALEVIEWPORTEXTEX_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRSCALEVIEWPORTEXTEX", contents, recnum, off);
}


// U_EMRSCALEWINDOWEXTEX     32
/**
    \brief Print a pointer to a U_EMR_SCALEWINDOWEXTEX record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSCALEWINDOWEXTEX_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRSCALEWINDOWEXTEX", contents, recnum, off);
}

// U_EMRSAVEDC               33
/**
    \brief Print a pointer to a U_EMR_SAVEDC record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSAVEDC_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRSAVEDC", contents, recnum, off);
}

// U_EMRRESTOREDC            34
/**
    \brief Print a pointer to a U_EMR_RESTOREDC record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRRESTOREDC_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRRESTOREDC", "iRelative:", contents, recnum, off);
}

// U_EMRSETWORLDTRANSFORM    35
/**
    \brief Print a pointer to a U_EMR_SETWORLDTRANSFORM record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETWORLDTRANSFORM_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRSETWORLDTRANSFORM", contents, recnum, off);
   PU_EMRSETWORLDTRANSFORM pEmr = (PU_EMRSETWORLDTRANSFORM)(contents+off);
   printf("   xform:");
   xform_print(pEmr->xform);
   printf("\n");
} 

// U_EMRMODIFYWORLDTRANSFORM 36
/**
    \brief Print a pointer to a U_EMR_MODIFYWORLDTRANSFORM record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRMODIFYWORLDTRANSFORM_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRMODIFYWORLDTRANSFORM", contents, recnum, off);
   PU_EMRMODIFYWORLDTRANSFORM pEmr = (PU_EMRMODIFYWORLDTRANSFORM)(contents+off);
   printf("   xform:");
   xform_print(pEmr->xform);
   printf("\n");
   printf("   iMode:          %u\n",      pEmr->iMode );
} 

// U_EMRSELECTOBJECT         37
/**
    \brief Print a pointer to a U_EMR_SELECTOBJECT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSELECTOBJECT_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRSELECTOBJECT", contents, recnum, off);
   PU_EMRSELECTOBJECT pEmr = (PU_EMRSELECTOBJECT)(contents+off);
   if(pEmr->ihObject & U_STOCK_OBJECT){
     printf("   StockObject:    0x%8.8X\n",  pEmr->ihObject );
   }
   else {
     printf("   ihObject:       %u\n",     pEmr->ihObject );
   }
} 

// U_EMRCREATEPEN            38
/**
    \brief Print a pointer to a U_EMR_CREATEPEN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCREATEPEN_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRCREATEPEN", contents, recnum, off);
   PU_EMRCREATEPEN pEmr = (PU_EMRCREATEPEN)(contents+off);
   printf("   ihPen:          %u\n",      pEmr->ihPen );
   printf("   lopn:           ");    logpen_print(pEmr->lopn);  printf("\n");
} 

// U_EMRCREATEBRUSHINDIRECT  39
/**
    \brief Print a pointer to a U_EMR_CREATEBRUSHINDIRECT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCREATEBRUSHINDIRECT_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRCREATEBRUSHINDIRECT", contents, recnum, off);
   PU_EMRCREATEBRUSHINDIRECT pEmr = (PU_EMRCREATEBRUSHINDIRECT)(contents+off);
   printf("   ihBrush:        %u\n",      pEmr->ihBrush );
   printf("   lb:             ");         logbrush_print(pEmr->lb);  printf("\n");
} 

// U_EMRDELETEOBJECT         40
/**
    \brief Print a pointer to a U_EMR_DELETEOBJECT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRDELETEOBJECT_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRDELETEOBJECT", contents, recnum, off);
   PU_EMRDELETEOBJECT pEmr = (PU_EMRDELETEOBJECT)(contents+off);
   printf("   ihObject:       %u\n",      pEmr->ihObject );
} 

// U_EMRANGLEARC             41
/**
    \brief Print a pointer to a U_EMR_ANGLEARC record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRANGLEARC_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRANGLEARC", contents, recnum, off);
   PU_EMRANGLEARC pEmr = (PU_EMRANGLEARC)(contents+off);
   printf("   ptlCenter:      "), pointl_print(pEmr->ptlCenter ); printf("\n");
   printf("   nRadius:        %u\n",      pEmr->nRadius );
   printf("   eStartAngle:    %f\n",       pEmr->eStartAngle );
   printf("   eSweepAngle:    %f\n",       pEmr->eSweepAngle );
} 

// U_EMRELLIPSE              42
/**
    \brief Print a pointer to a U_EMR_ELLIPSE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRELLIPSE_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRELLIPSE", contents, recnum, off);
}

// U_EMRRECTANGLE            43
/**
    \brief Print a pointer to a U_EMR_RECTANGLE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRRECTANGLE_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRRECTANGLE", contents, recnum, off);
}

// U_EMRROUNDRECT            44
/**
    \brief Print a pointer to a U_EMR_ROUNDRECT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRROUNDRECT_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRROUNDRECT", contents, recnum, off);
   PU_EMRROUNDRECT pEmr = (PU_EMRROUNDRECT)(contents+off);
   printf("   rclBox:         "), rectl_print(pEmr->rclBox );     printf("\n");
   printf("   szlCorner:      "), sizel_print(pEmr->szlCorner );  printf("\n");
}

// U_EMRARC                  45
/**
    \brief Print a pointer to a U_EMR_ARC record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRARC_print(char *contents, int recnum, size_t off){
   core9_print("U_EMRARC", contents, recnum, off);
}

// U_EMRCHORD                46
/**
    \brief Print a pointer to a U_EMR_CHORD record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCHORD_print(char *contents, int recnum, size_t off){
   core9_print("U_EMRCHORD", contents, recnum, off);
}

// U_EMRPIE                  47
/**
    \brief Print a pointer to a U_EMR_PIE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPIE_print(char *contents, int recnum, size_t off){
   core9_print("U_EMRPIE", contents, recnum, off);
}

// U_EMRSELECTPALETTE        48
/**
    \brief Print a pointer to a U_EMR_SELECTPALETTE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSELECTPALETTE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSELECTPALETTE", "ihPal:", contents, recnum, off);
}

// U_EMRCREATEPALETTE        49
/**
    \brief Print a pointer to a U_EMR_CREATEPALETTE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCREATEPALETTE_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRCREATEPALETTE", contents, recnum, off);
   PU_EMRCREATEPALETTE pEmr = (PU_EMRCREATEPALETTE)(contents+off);
   printf("   ihPal:          %u\n",pEmr->ihPal);
   printf("   lgpl:           "), logpalette_print( (PU_LOGPALETTE)&(pEmr->lgpl) );  printf("\n");
}

// U_EMRSETPALETTEENTRIES    50
/**
    \brief Print a pointer to a U_EMR_SETPALETTEENTRIES record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETPALETTEENTRIES_print(char *contents, int recnum, size_t off){
   int i;
   core5_print("U_EMRSETPALETTEENTRIES", contents, recnum, off);
   PU_EMRSETPALETTEENTRIES pEmr = (PU_EMRSETPALETTEENTRIES)(contents+off);
   printf("   ihPal:          %u\n",pEmr->ihPal);
   printf("   iStart:         %u\n",pEmr->iStart);
   printf("   cEntries:       %u\n",pEmr->cEntries);
   if(pEmr->cEntries){
      printf("      PLTEntries:");
      PU_LOGPLTNTRY aPalEntries = (PU_LOGPLTNTRY) &(pEmr->aPalEntries);
      for(i=0; i<pEmr->cEntries; i++){
         printf("%d:",i); logpltntry_print(aPalEntries[i]);
      }
      printf("\n");
   }
}

// U_EMRRESIZEPALETTE        51
/**
    \brief Print a pointer to a U_EMR_RESIZEPALETTE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRRESIZEPALETTE_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRRESIZEPALETTE", "ihPal:","cEntries",contents, recnum, off);
} 

// U_EMRREALIZEPALETTE       52
/**
    \brief Print a pointer to a U_EMR_REALIZEPALETTE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRREALIZEPALETTE_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRREALIZEPALETTE", contents, recnum, off);
}

// U_EMREXTFLOODFILL         53
/**
    \brief Print a pointer to a U_EMR_EXTFLOODFILL record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREXTFLOODFILL_print(char *contents, int recnum, size_t off){
   core5_print("U_EMREXTFLOODFILL", contents, recnum, off);
   PU_EMREXTFLOODFILL pEmr = (PU_EMREXTFLOODFILL)(contents+off);
   printf("   ptlStart:       ");   pointl_print(pEmr->ptlStart);    printf("\n");
   printf("   crColor:        ");   colorref_print(pEmr->crColor);   printf("\n");
   printf("   iMode:          %u\n",pEmr->iMode);
}

// U_EMRLINETO               54
/**
    \brief Print a pointer to a U_EMR_LINETO record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRLINETO_print(char *contents, int recnum, size_t off){
   core7_print("U_EMRLINETO", "ptl:","",contents, recnum, off);
} 

// U_EMRARCTO                55
/**
    \brief Print a pointer to a U_EMR_ARCTO record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRARCTO_print(char *contents, int recnum, size_t off){
   core9_print("U_EMRARCTO", contents, recnum, off);
}

// U_EMRPOLYDRAW             56
/**
    \brief Print a pointer to a U_EMR_POLYDRAW record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYDRAW_print(char *contents, int recnum, size_t off){
   int i;
   core5_print("U_EMRPOLYDRAW", contents, recnum, off);
   PU_EMRPOLYDRAW pEmr = (PU_EMRPOLYDRAW)(contents+off);
   printf("   rclBounds:      ");          rectl_print( pEmr->rclBounds);   printf("\n");
   printf("   cptl:           %d\n",pEmr->cptl        );
   printf("   Points:         ");
   for(i=0;i<pEmr->cptl; i++){
      printf(" [%d]:",i);
      pointl_print(pEmr->aptl[i]);
   }
   printf("\n");
   printf("   Types:          ");
   for(i=0;i<pEmr->cptl; i++){
      printf(" [%d]:%u ",i,pEmr->abTypes[i]);
   }
   printf("\n");
}

// U_EMRSETARCDIRECTION      57
/**
    \brief Print a pointer to a U_EMR_SETARCDIRECTION record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETARCDIRECTION_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETARCDIRECTION","arcDirection:", contents, recnum, off);
}

// U_EMRSETMITERLIMIT        58
/**
    \brief Print a pointer to a U_EMR_SETMITERLIMIT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETMITERLIMIT_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETMITERLIMIT", "eMiterLimit:", contents, recnum, off);
}


// U_EMRBEGINPATH            59
/**
    \brief Print a pointer to a U_EMR_BEGINPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRBEGINPATH_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRBEGINPATH", contents, recnum, off);
}

// U_EMRENDPATH              60
/**
    \brief Print a pointer to a U_EMR_ENDPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRENDPATH_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRENDPATH", contents, recnum, off);
}

// U_EMRCLOSEFIGURE          61
/**
    \brief Print a pointer to a U_EMR_CLOSEFIGURE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCLOSEFIGURE_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRCLOSEFIGURE", contents, recnum, off);
}

// U_EMRFILLPATH             62
/**
    \brief Print a pointer to a U_EMR_FILLPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRFILLPATH_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRFILLPATH", contents, recnum, off);
}

// U_EMRSTROKEANDFILLPATH    63
/**
    \brief Print a pointer to a U_EMR_STROKEANDFILLPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSTROKEANDFILLPATH_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRSTROKEANDFILLPATH", contents, recnum, off);
}

// U_EMRSTROKEPATH           64
/**
    \brief Print a pointer to a U_EMR_STROKEPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSTROKEPATH_print(char *contents, int recnum, size_t off){
   core4_print("U_EMRSTROKEPATH", contents, recnum, off);
}

// U_EMRFLATTENPATH          65
/**
    \brief Print a pointer to a U_EMR_FLATTENPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRFLATTENPATH_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRFLATTENPATH", contents, recnum, off);
}

// U_EMRWIDENPATH            66
/**
    \brief Print a pointer to a U_EMR_WIDENPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRWIDENPATH_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRWIDENPATH", contents, recnum, off);
}

// U_EMRSELECTCLIPPATH       67
/**
    \brief Print a pointer to a U_EMR_SELECTCLIPPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSELECTCLIPPATH_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSELECTCLIPPATH", "iMode:", contents, recnum, off);
}

// U_EMRABORTPATH            68
/**
    \brief Print a pointer to a U_EMR_ABORTPATH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRABORTPATH_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRABORTPATH", contents, recnum, off);
}

// U_EMRUNDEF69                       69
#define U_EMRUNDEF69_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF69",A,B,C)

// U_EMRCOMMENT              70  Comment (any binary data, interpretation is program specific)
/**
    \brief Print a pointer to a U_EMR_COMMENT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCOMMENT_print(char *contents, int recnum, size_t off){
   char *string;
   char *src;
   uint32_t cIdent,cbData;
   core5_print("U_EMRCOMMENT", contents, recnum, off);
   PU_EMRCOMMENT pEmr = (PU_EMRCOMMENT)(contents+off);

   /* There are several different types of comments */

   cbData = pEmr->cbData;
   printf("   cbData:         %d\n",cbData        );
   src = (char *)&(pEmr->Data);  // default
   if(cbData >= 4){
      cIdent = *(uint32_t *)&(pEmr->Data);
      if(     cIdent == U_EMR_COMMENT_PUBLIC       ){
         printf("   cIdent:  Public\n");
         PU_EMRCOMMENT_PUBLIC pEmrp = (PU_EMRCOMMENT_PUBLIC) pEmr;
         printf("   pcIdent:        %8.8x\n",pEmrp->pcIdent);
         src = (char *)&(pEmrp->Data);
         cbData -= 8;
      }
      else if(cIdent == U_EMR_COMMENT_SPOOL        ){
         printf("   cIdent:  Spool\n");
         PU_EMRCOMMENT_SPOOL pEmrs = (PU_EMRCOMMENT_SPOOL) pEmr;
         printf("   esrIdent:       %8.8x\n",pEmrs->esrIdent);
         src = (char *)&(pEmrs->Data);
         cbData -= 8;
      }
      else if(cIdent == U_EMR_COMMENT_EMFPLUSRECORD){
         printf("   cIdent:  EMF+\n");
         PU_EMRCOMMENT_EMFPLUS pEmrpl = (PU_EMRCOMMENT_EMFPLUS) pEmr;
         src = (char *)&(pEmrpl->Data);
         cbData -= 4;
      }
      else {
         printf("   cIdent:         not (Public or Spool or EMF+)\n");
      }
   }
   if(cbData){ // The data may not be printable, but try it just in case
      string = malloc(cbData + 1);
      (void)strncpy(string, src, cbData);
      string[cbData] = '\0'; // it might not be terminated - it might not even be text!
      printf("   Data:           <%s>\n",string);
      free(string);
   }

} 

// U_EMRFILLRGN              71
/**
    \brief Print a pointer to a U_EMR_FILLRGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRFILLRGN_print(char *contents, int recnum, size_t off){
   int i,roff;
   core5_print("U_EMRFILLRGN", contents, recnum, off);
   PU_EMRFILLRGN pEmr = (PU_EMRFILLRGN)(contents+off);
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);    printf("\n");
   printf("   cbRgnData:      %u\n",pEmr->cbRgnData);
   printf("   ihBrush:        %u\n",pEmr->ihBrush);
   // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
   roff=0;
   i=1; 
   char *prd = (char *) &(pEmr->RgnData);
   while(roff + 28 < pEmr->emr.nSize){ // up to the end of the record
      printf("   RegionData[%d]: ",i);   rgndata_print((PU_RGNDATA) (prd + roff));  printf("\n");
      roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
   }
} 

// U_EMRFRAMERGN             72
/**
    \brief Print a pointer to a U_EMR_FRAMERGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRFRAMERGN_print(char *contents, int recnum, size_t off){
   int i,roff;
   core5_print("U_EMRFRAMERGN", contents, recnum, off);
   PU_EMRFRAMERGN pEmr = (PU_EMRFRAMERGN)(contents+off);
   printf("   rclBounds:      ");    rectl_print(pEmr->rclBounds);    printf("\n");
   printf("   cbRgnData:      %u\n",pEmr->cbRgnData);
   printf("   ihBrush:        %u\n",pEmr->ihBrush);
   printf("   szlStroke:      "), sizel_print(pEmr->szlStroke );      printf("\n");
   // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
   roff=0;
   i=1; 
   char *prd = (char *) &(pEmr->RgnData);
   while(roff + 28 < pEmr->emr.nSize){ // up to the end of the record
      printf("   RegionData[%d]: ",i);   rgndata_print((PU_RGNDATA) (prd + roff));  printf("\n");
      roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
   }
} 

// U_EMRINVERTRGN            73
/**
    \brief Print a pointer to a U_EMR_INVERTRGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRINVERTRGN_print(char *contents, int recnum, size_t off){
   core11_print("U_EMRINVERTRGN", contents, recnum, off);
}

// U_EMRPAINTRGN             74
/**
    \brief Print a pointer to a U_EMR_PAINTRGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPAINTRGN_print(char *contents, int recnum, size_t off){
   core11_print("U_EMRPAINTRGN", contents, recnum, off);
}

// U_EMREXTSELECTCLIPRGN     75
/**
    \brief Print a pointer to a U_EMR_EXTSELECTCLIPRGN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREXTSELECTCLIPRGN_print(char *contents, int recnum, size_t off){
   int i,roff;
   PU_EMREXTSELECTCLIPRGN pEmr = (PU_EMREXTSELECTCLIPRGN) (contents + off);
   core5_print("U_EMREXTSELECTCLIPRGN", contents, recnum, off);
   printf("   cbRgnData:      %u\n",pEmr->cbRgnData);
   printf("   iMode:          %u\n",pEmr->iMode);
   // This one is a pain since each RGNDATA may be a different size, so it isn't possible to index through them.
   char *prd = (char *) &(pEmr->RgnData);
   i=roff=0;
   while(roff + 16 < pEmr->emr.nSize){ // stop at end of the record
      printf("   RegionData[%d]: ",i++); rgndata_print((PU_RGNDATA) (prd + roff)); printf("\n");
      roff += (((PU_RGNDATA)prd)->rdh.dwSize + ((PU_RGNDATA)prd)->rdh.nRgnSize - 16);
   }
} 

// U_EMRBITBLT               76
/**
    \brief Print a pointer to a U_EMR_BITBLT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRBITBLT_print(char *contents, int recnum, size_t off){
   PU_EMRBITBLT pEmr = (PU_EMRBITBLT) (contents + off);
   core5_print("U_EMRBITBLT", contents, recnum, off);
   printf("   rclBounds:      ");     rectl_print( pEmr->rclBounds);       printf("\n");
   printf("   Dest:           ");     pointl_print(pEmr->Dest);            printf("\n");
   printf("   cDest:          ");     pointl_print(pEmr->cDest);           printf("\n");
   printf("   dwRop :         %u\n", pEmr->dwRop   );
   printf("   Src:            ");     pointl_print(pEmr->Src);             printf("\n");
   printf("   xformSrc:       ");     xform_print( pEmr->xformSrc);        printf("\n");
   printf("   crBkColorSrc:   ");     colorref_print( pEmr->crBkColorSrc); printf("\n");
   printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
   printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
   printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
   if(pEmr->cbBmiSrc){
      printf("      bitmap:      ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiSrc));
      printf("\n");
   }
   printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
   printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
}

// U_EMRSTRETCHBLT           77
/**
    \brief Print a pointer to a U_EMR_STRETCHBLT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSTRETCHBLT_print(char *contents, int recnum, size_t off){
   PU_EMRSTRETCHBLT pEmr = (PU_EMRSTRETCHBLT) (contents + off);
   core5_print("U_EMRSTRETCHBLT", contents, recnum, off);
   printf("   rclBounds:      ");     rectl_print( pEmr->rclBounds);       printf("\n");
   printf("   Dest:           ");     pointl_print(pEmr->Dest);            printf("\n");
   printf("   cDest:          ");     pointl_print(pEmr->cDest);           printf("\n");
   printf("   dwRop :         %u\n", pEmr->dwRop   );
   printf("   Src:            ");     pointl_print(pEmr->Src);             printf("\n");
   printf("   xformSrc:       ");     xform_print( pEmr->xformSrc);        printf("\n");
   printf("   crBkColorSrc:   ");     colorref_print( pEmr->crBkColorSrc); printf("\n");
   printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
   printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
   printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
   if(pEmr->cbBmiSrc){
      printf("      bitmap:      ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiSrc));
      printf("\n");
   }
   printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
   printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
   printf("   cSrc:           ");     pointl_print(pEmr->cSrc);            printf("\n");
}

// U_EMRMASKBLT              78
/**
    \brief Print a pointer to a U_EMR_MASKBLT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRMASKBLT_print(char *contents, int recnum, size_t off){
   PU_EMRMASKBLT pEmr = (PU_EMRMASKBLT) (contents + off);
   core5_print("U_EMRMASKBLT", contents, recnum, off);
   printf("   rclBounds:      ");     rectl_print( pEmr->rclBounds);       printf("\n");
   printf("   Dest:           ");     pointl_print(pEmr->Dest);            printf("\n");
   printf("   cDest:          ");     pointl_print(pEmr->cDest);           printf("\n");
   printf("   dwRop :         %u\n",  pEmr->dwRop   );
   printf("   Src:            ");     pointl_print(pEmr->Src);             printf("\n");
   printf("   xformSrc:       ");     xform_print( pEmr->xformSrc);        printf("\n");
   printf("   crBkColorSrc:   ");     colorref_print( pEmr->crBkColorSrc); printf("\n");
   printf("   iUsageSrc:      %u\n",  pEmr->iUsageSrc   );
   printf("   offBmiSrc:      %u\n",  pEmr->offBmiSrc   );
   printf("   cbBmiSrc:       %u\n",  pEmr->cbBmiSrc    );
   if(pEmr->cbBmiSrc){
      printf("      Src bitmap:  ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiSrc));
      printf("\n");
   }
   printf("   offBitsSrc:     %u\n",  pEmr->offBitsSrc   );
   printf("   cbBitsSrc:      %u\n",  pEmr->cbBitsSrc    );
   printf("   Mask:           ");     pointl_print(pEmr->Mask);            printf("\n");
   printf("   iUsageMask:     %u\n",  pEmr->iUsageMask   );
   printf("   offBmiMask:     %u\n",  pEmr->offBmiMask   );
   printf("   cbBmiMask:      %u\n",  pEmr->cbBmiMask    );
   if(pEmr->cbBmiMask){
      printf("      Mask bitmap: ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiMask));
      printf("\n");
   }
   printf("   offBitsMask:    %u\n",  pEmr->offBitsMask   );
   printf("   cbBitsMask:     %u\n",  pEmr->cbBitsMask    );
}

// U_EMRPLGBLT               79
/**
    \brief Print a pointer to a U_EMR_PLGBLT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPLGBLT_print(char *contents, int recnum, size_t off){
   PU_EMRPLGBLT pEmr = (PU_EMRPLGBLT) (contents + off);
   core5_print("U_EMRPLGBLT", contents, recnum, off);
   printf("   rclBounds:      ");     rectl_print( pEmr->rclBounds);       printf("\n");
   printf("   aptlDst(UL):    ");     pointl_print(pEmr->aptlDst[0]);      printf("\n");
   printf("   aptlDst(UR):    ");     pointl_print(pEmr->aptlDst[1]);      printf("\n");
   printf("   aptlDst(LL):    ");     pointl_print(pEmr->aptlDst[2]);      printf("\n");
   printf("   Src:            ");     pointl_print(pEmr->Src);             printf("\n");
   printf("   cSrc:           ");     pointl_print(pEmr->cSrc);            printf("\n");
   printf("   xformSrc:       ");     xform_print( pEmr->xformSrc);        printf("\n");
   printf("   crBkColorSrc:   ");     colorref_print( pEmr->crBkColorSrc); printf("\n");
   printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
   printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
   printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
   if(pEmr->cbBmiSrc){
      printf("      Src bitmap:  ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiSrc));
      printf("\n");
   }
   printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
   printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
   printf("   Mask:           ");    pointl_print(pEmr->Mask);            printf("\n");
   printf("   iUsageMsk:      %u\n", pEmr->iUsageMask   );
   printf("   offBmiMask:     %u\n", pEmr->offBmiMask   );
   printf("   cbBmiMask:      %u\n", pEmr->cbBmiMask    );
   if(pEmr->cbBmiMask){
      printf("      Mask bitmap: ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiMask));
      printf("\n");
   }
   printf("   offBitsMask:    %u\n", pEmr->offBitsMask   );
   printf("   cbBitsMask:     %u\n", pEmr->cbBitsMask    );
}

// U_EMRSETDIBITSTODEVICE    80
/**
    \brief Print a pointer to a U_EMRSETDIBITSTODEVICE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETDIBITSTODEVICE_print(char *contents, int recnum, size_t off){
   PU_EMRSETDIBITSTODEVICE pEmr = (PU_EMRSETDIBITSTODEVICE) (contents + off);
   core5_print("U_EMRSETDIBITSTODEVICE", contents, recnum, off);
   printf("   rclBounds:      ");     rectl_print( pEmr->rclBounds);       printf("\n");
   printf("   Dest:           ");     pointl_print(pEmr->Dest);            printf("\n");
   printf("   Src:            ");     pointl_print(pEmr->Src);             printf("\n");
   printf("   cSrc:           ");     pointl_print(pEmr->cSrc);            printf("\n");
   printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
   printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
   if(pEmr->cbBmiSrc){
      printf("      Src bitmap:  ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiSrc));
      printf("\n");
   }
   printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
   printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
   printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
   printf("   iStartScan:     %u\n", pEmr->iStartScan    );
   printf("   cScans :        %u\n", pEmr->cScans        );
}

// U_EMRSTRETCHDIBITS        81
/**
    \brief Print a pointer to a U_EMR_STRETCHDIBITS record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSTRETCHDIBITS_print(char *contents, int recnum, size_t off){
   PU_EMRSTRETCHDIBITS pEmr = (PU_EMRSTRETCHDIBITS) (contents + off);
   core5_print("U_EMRSTRETCHDIBITS", contents, recnum, off);
   printf("   rclBounds:      ");     rectl_print( pEmr->rclBounds);       printf("\n");
   printf("   Dest:           ");     pointl_print(pEmr->Dest);            printf("\n");
   printf("   Src:            ");     pointl_print(pEmr->Src);             printf("\n");
   printf("   cSrc:           ");     pointl_print(pEmr->cSrc);            printf("\n");
   printf("   offBmiSrc:      %u\n", pEmr->offBmiSrc   );
   printf("   cbBmiSrc:       %u\n", pEmr->cbBmiSrc    );
   if(pEmr->cbBmiSrc){
      printf("      Src bitmap:  ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmiSrc));
      printf("\n");
   }
   printf("   offBitsSrc:     %u\n", pEmr->offBitsSrc   );
   printf("   cbBitsSrc:      %u\n", pEmr->cbBitsSrc    );
   printf("   iUsageSrc:      %u\n", pEmr->iUsageSrc   );
   printf("   dwRop :         %u\n", pEmr->dwRop   );
   printf("   cDest:          ");     pointl_print(pEmr->cDest);           printf("\n");
}

// U_EMREXTCREATEFONTINDIRECTW_print    82
/**
    \brief Print a pointer to a U_EMR_EXTCREATEFONTINDIRECTW record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREXTCREATEFONTINDIRECTW_print(char *contents, int recnum, size_t off){
   PU_EMREXTCREATEFONTINDIRECTW pEmr = (PU_EMREXTCREATEFONTINDIRECTW) (contents + off);
   core5_print("U_EMREXTCREATEFONTINDIRECTW", contents, recnum, off);
   printf("   ihFont:         %u\n",pEmr->ihFont );
   printf("   Font:           ");
   if(pEmr->emr.nSize == sizeof(U_EMREXTCREATEFONTINDIRECTW)){ // holds logfont_panose
      logfont_panose_print(pEmr->elfw);
   }
   else { // holds logfont
      logfont_print( *(PU_LOGFONT) &(pEmr->elfw));
   }
   printf("\n");
}

// U_EMREXTTEXTOUTA          83
/**
    \brief Print a pointer to a U_EMR_EXTTEXTOUTA record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREXTTEXTOUTA_print(char *contents, int recnum, size_t off){
   core8_print("U_EMREXTTEXTOUTA", contents, recnum, off, 0);
}

// U_EMREXTTEXTOUTW          84
/**
    \brief Print a pointer to a U_EMR_EXTTEXTOUTW record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREXTTEXTOUTW_print(char *contents, int recnum, size_t off){
   core8_print("U_EMREXTTEXTOUTW", contents, recnum, off, 1);
}

// U_EMRPOLYBEZIER16         85
/**
    \brief Print a pointer to a U_EMR_POLYBEZIER16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYBEZIER16_print(char *contents, int recnum, size_t off){
   core6_print("U_EMRPOLYBEZIER16", contents, recnum, off);
}

// U_EMRPOLYGON16            86
/**
    \brief Print a pointer to a U_EMR_POLYGON16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYGON16_print(char *contents, int recnum, size_t off){
   core6_print("U_EMRPOLYGON16", contents, recnum, off);
}

// U_EMRPOLYLINE16           87
/**
    \brief Print a pointer to a U_EMR_POLYLINE16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYLINE16_print(char *contents, int recnum, size_t off){
   core6_print("U_EMRPOLYLINE16", contents, recnum, off);
}

// U_EMRPOLYBEZIERTO16       88
/**
    \brief Print a pointer to a U_EMR_POLYBEZIERTO16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYBEZIERTO16_print(char *contents, int recnum, size_t off){
   core6_print("U_EMRPOLYBEZIERTO16", contents, recnum, off);
}

// U_EMRPOLYLINETO16         89
/**
    \brief Print a pointer to a U_EMR_POLYLINETO16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYLINETO16_print(char *contents, int recnum, size_t off){
   core6_print("U_EMRPOLYLINETO16", contents, recnum, off);
}

// U_EMRPOLYPOLYLINE16       90
/**
    \brief Print a pointer to a U_EMR_POLYPOLYLINE16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYPOLYLINE16_print(char *contents, int recnum, size_t off){
   core10_print("U_EMRPOLYPOLYLINE16", contents, recnum, off);
}

// U_EMRPOLYPOLYGON16        91
/**
    \brief Print a pointer to a U_EMR_POLYPOLYGON16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYPOLYGON16_print(char *contents, int recnum, size_t off){
   core10_print("U_EMRPOLYPOLYGON16", contents, recnum, off);
}


// U_EMRPOLYDRAW16           92
/**
    \brief Print a pointer to a U_EMR_POLYDRAW16 record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPOLYDRAW16_print(char *contents, int recnum, size_t off){
   int i;
   core5_print("U_EMRPOLYDRAW16", contents, recnum, off);
   PU_EMRPOLYDRAW16 pEmr = (PU_EMRPOLYDRAW16)(contents+off);
   printf("   rclBounds:      ");          rectl_print( pEmr->rclBounds);   printf("\n");
   printf("   cpts:           %d\n",pEmr->cpts        );
   printf("   Points:         ");
   for(i=0;i<pEmr->cpts; i++){
      printf(" [%d]:",i);
      point16_print(pEmr->apts[i]);
   }
   printf("\n");
   printf("   Types:          ");
   for(i=0;i<pEmr->cpts; i++){
      printf(" [%d]:%u ",i,pEmr->abTypes[i]);
   }
   printf("\n");
}

// U_EMRCREATEMONOBRUSH      93
/**
    \brief Print a pointer to a U_EMR_CREATEMONOBRUSH record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCREATEMONOBRUSH_print(char *contents, int recnum, size_t off){
   core12_print("U_EMRCREATEMONOBRUSH", contents, recnum, off);
}

// U_EMRCREATEDIBPATTERNBRUSHPT_print   94
/**
    \brief Print a pointer to a U_EMR_CREATEDIBPATTERNBRUSHPT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCREATEDIBPATTERNBRUSHPT_print(char *contents, int recnum, size_t off){
   core12_print("U_EMRCREATEDIBPATTERNBRUSHPT", contents, recnum, off);
}


// U_EMREXTCREATEPEN         95
/**
    \brief Print a pointer to a U_EMR_EXTCREATEPEN record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMREXTCREATEPEN_print(char *contents, int recnum, size_t off){
   core5_print("U_EMREXTCREATEPEN", contents, recnum, off);
   PU_EMREXTCREATEPEN pEmr = (PU_EMREXTCREATEPEN)(contents+off);
   printf("   ihPen:          %u\n", pEmr->ihPen );
   printf("   offBmi:         %u\n", pEmr->offBmi   );
   printf("   cbBmi:          %u\n", pEmr->cbBmi    );
   if(pEmr->cbBmi){
      printf("      bitmap:      ");
      bitmapinfo_print((PU_BITMAPINFO)(contents + off + pEmr->offBmi));
      printf("\n");
   }
   printf("   offBits:        %u\n", pEmr->offBits   );
   printf("   cbBits:         %u\n", pEmr->cbBits    );
   printf("   elp:            ");     extlogpen_print((PU_EXTLOGPEN) &(pEmr->elp));  printf("\n");
} 

// U_EMRPOLYTEXTOUTA         96 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTA_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRPOLYTEXTOUTA",A,B,C)
// U_EMRPOLYTEXTOUTW         97 NOT IMPLEMENTED, denigrated after Windows NT
#define U_EMRPOLYTEXTOUTW_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRPOLYTEXTOUTW",A,B,C)

// U_EMRSETICMMODE           98
/**
    \brief Print a pointer to a U_EMR_SETICMMODE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETICMMODE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETICMMODE", "iMode:", contents, recnum, off);
}

// U_EMRCREATECOLORSPACE     99
/**
    \brief Print a pointer to a U_EMR_CREATECOLORSPACE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCREATECOLORSPACE_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRCREATECOLORSPACE", contents, recnum, off);
   PU_EMRCREATECOLORSPACE pEmr = (PU_EMRCREATECOLORSPACE)(contents+off);
   printf("   ihCS:           %u\n", pEmr->ihCS    );
   printf("   ColorSpace:     "); logcolorspacea_print(pEmr->lcs);  printf("\n");
}

// U_EMRSETCOLORSPACE       100
/**
    \brief Print a pointer to a U_EMR_SETCOLORSPACE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETCOLORSPACE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETCOLORSPACE", "ihCS:", contents, recnum, off);
}

// U_EMRDELETECOLORSPACE    101
/**
    \brief Print a pointer to a U_EMR_DELETECOLORSPACE record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRDELETECOLORSPACE_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRDELETECOLORSPACE", "ihCS:", contents, recnum, off);
}

// U_EMRGLSRECORD           102  Not implemented
#define U_EMRGLSRECORD_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRGLSRECORD",A,B,C)
// U_EMRGLSBOUNDEDRECORD    103  Not implemented
#define U_EMRGLSBOUNDEDRECORD_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRGLSBOUNDEDRECORD",A,B,C)

// U_EMRPIXELFORMAT         104
/**
    \brief Print a pointer to a U_EMR_PIXELFORMAT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRPIXELFORMAT_print(char *contents, int recnum, size_t off){
   core5_print("U_EMRPIXELFORMAT", contents, recnum, off);
   PU_EMRPIXELFORMAT pEmr = (PU_EMRPIXELFORMAT)(contents+off);
   printf("   Pfd:            ");  pixelformatdescriptor_print(pEmr->pfd);  printf("\n");
}

// U_EMRDRAWESCAPE          105  Not implemented
#define U_EMRDRAWESCAPE_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRDRAWESCAPE",A,B,C)
// U_EMREXTESCAPE           106  Not implemented
#define U_EMREXTESCAPE_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMREXTESCAPE",A,B,C)
// U_EMRUNDEF107            107  Not implemented
#define U_EMRUNDEF107_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF107",A,B,C)

// U_EMRSMALLTEXTOUT        108
/**
    \brief Print a pointer to a U_EMR_SMALLTEXTOUT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSMALLTEXTOUT_print(char *contents, int recnum, size_t off){
   int roff;
   char *string;
   core5_print("U_EMRSMALLTEXTOUT", contents, recnum, off);
   PU_EMRSMALLTEXTOUT pEmr = (PU_EMRSMALLTEXTOUT)(contents+off);
   printf("   Dest:           ");         pointl_print(pEmr->Dest);            printf("\n");
   printf("   cChars:         %u\n",      pEmr->cChars          );
   printf("   fuOptions:      0x%8.8X\n", pEmr->fuOptions       );
   printf("   iGraphicsMode:  0x%8.8X\n", pEmr->iGraphicsMode   );
   printf("   exScale:        %f\n",      pEmr->exScale         );
   printf("   eyScale:        %f\n",      pEmr->eyScale         );
   roff = sizeof(U_EMRSMALLTEXTOUT);  //offset to the start of the variable fields
   if(!(pEmr->fuOptions & U_ETO_NO_RECT)){
      printf("   rclBounds:      ");      rectl_print( *(PU_RECTL) (contents + off + roff));       printf("\n");
      roff += sizeof(U_RECTL);
   }
   if(pEmr->fuOptions & U_ETO_SMALL_CHARS){
      printf("   Text8:          <%s>\n",contents+off+roff);
   }
   else {
      string = U_Utf16leToUtf8((uint16_t *)(contents+off+roff), pEmr->cChars, NULL);
      printf("   Text16:         <%s>\n",contents+off+roff);
      free(string);
  }
}

// U_EMRFORCEUFIMAPPING     109  Not implemented
#define U_EMRFORCEUFIMAPPING_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRFORCEUFIMAPPING",A,B,C)
// U_EMRNAMEDESCAPE         110  Not implemented
#define U_EMRNAMEDESCAPE_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRNAMEDESCAPE",A,B,C)
// U_EMRCOLORCORRECTPALETTE 111  Not implemented
#define U_EMRCOLORCORRECTPALETTE_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRCOLORCORRECTPALETTE",A,B,C)
// U_EMRSETICMPROFILEA      112  Not implemented
#define U_EMRSETICMPROFILEA_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRSETICMPROFILEA",A,B,C)
// U_EMRSETICMPROFILEW      113  Not implemented
#define U_EMRSETICMPROFILEW_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRSETICMPROFILEW",A,B,C)

// U_EMRALPHABLEND          114
/**
    \brief Print a pointer to a U_EMR_ALPHABLEND record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRALPHABLEND_print(char *contents, int recnum, size_t off){
   core13_print("U_EMRALPHABLEND", contents, recnum, off);
}

// U_EMRSETLAYOUT           115
/**
    \brief Print a pointer to a U_EMR_SETLAYOUT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRSETLAYOUT_print(char *contents, int recnum, size_t off){
   core3_print("U_EMRSETLAYOUT", "iMode:", contents, recnum, off);
}

// U_EMRTRANSPARENTBLT      116
/**
    \brief Print a pointer to a U_EMR_TRANSPARENTBLT record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRTRANSPARENTBLT_print(char *contents, int recnum, size_t off){
   core13_print("U_EMRTRANSPARENTBLT", contents, recnum, off);
}

// U_EMRUNDEF117            117  Not implemented
#define U_EMRUNDEF117_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRUNDEF117",A,B,C)
// U_EMRGRADIENTFILL        118
/**
    \brief Print a pointer to a U_EMR_GRADIENTFILL record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRGRADIENTFILL_print(char *contents, int recnum, size_t off){
   int i;
   core5_print("U_EMRGRADIENTFILL", contents, recnum, off);
   PU_EMRGRADIENTFILL pEmr = (PU_EMRGRADIENTFILL)(contents+off);
   printf("   rclBounds:      ");      rectl_print( pEmr->rclBounds);   printf("\n");
   printf("   nTriVert:       %u\n",   pEmr->nTriVert   );
   printf("   nGradObj:       %u\n",   pEmr->nGradObj   );
   printf("   ulMode:         %u\n",   pEmr->ulMode     );
   off += sizeof(U_EMRGRADIENTFILL);
   if(pEmr->nTriVert){
      printf("   TriVert:        ");
      for(i=0; i<pEmr->nTriVert; i++, off+=sizeof(U_TRIVERTEX)){
         trivertex_print(*(PU_TRIVERTEX)(contents + off));
      }
      printf("\n");
   }
   if(pEmr->nGradObj){
      printf("   GradObj:        ");
      if(     pEmr->ulMode == U_GRADIENT_FILL_TRIANGLE){
         for(i=0; i<pEmr->nGradObj; i++, off+=sizeof(U_GRADIENT3)){
            gradient3_print(*(PU_GRADIENT3)(contents + off));
         }
      }
      else if(pEmr->ulMode == U_GRADIENT_FILL_RECT_H || 
              pEmr->ulMode == U_GRADIENT_FILL_RECT_V){
         for(i=0; i<pEmr->nGradObj; i++, off+=sizeof(U_GRADIENT4)){
            gradient4_print(*(PU_GRADIENT4)(contents + off));
         }
      }
      else { printf("invalid ulMode value!"); }
      printf("\n");
   }
}

// U_EMRSETLINKEDUFIS       119  Not implemented
#define U_EMRSETLINKEDUFIS_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRSETLINKEDUFIS",A,B,C)
// U_EMRSETTEXTJUSTIFICATION120  Not implemented (denigrated)
#define U_EMRSETTEXTJUSTIFICATION_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRSETTEXTJUSTIFICATION",A,B,C)
// U_EMRCOLORMATCHTOTARGETW 121  Not implemented  
#define U_EMRCOLORMATCHTOTARGETW_print(A,B,C) U_EMRNOTIMPLEMENTED_print("U_EMRCOLORMATCHTOTARGETW",A,B,C)

// U_EMRCREATECOLORSPACEW   122
/**
    \brief Print a pointer to a U_EMR_CREATECOLORSPACEW record.
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
void U_EMRCREATECOLORSPACEW_print(char *contents, int recnum, size_t off){
   int i;
   core5_print("U_EMRCREATECOLORSPACEW", contents, recnum, off);
   PU_EMRCREATECOLORSPACEW pEmr = (PU_EMRCREATECOLORSPACEW)(contents+off);
   printf("   ihCS:           %u\n", pEmr->ihCS     );
   printf("   ColorSpace:     "); logcolorspacew_print(pEmr->lcs);  printf("\n");
   printf("   dwFlags:        %u\n", pEmr->dwFlags  );
   printf("   cbData:         %u\n", pEmr->cbData   );
   printf("   Data:           ");
   if(pEmr->dwFlags & 1){
     for(i=0; i<pEmr->cbData; i++){
        printf("[%d]:%2.2X ",i,pEmr->Data[i]);
     }
   }
   printf("\n");
}

/**
    \brief Print any record in an emf
    \returns 1 for a normal record, 0 for EMREOF
    \param contents   pointer to a buffer holding all EMR records
    \param recnum     number of this record in contents
    \param off        offset to this record in contents
*/
int U_emf_onerec_print(char *contents, int recnum, size_t off){
    PU_ENHMETARECORD  lpEMFR  = (PU_ENHMETARECORD)(contents + off);
    int regular=1;
    switch (lpEMFR->iType)
    {
        case U_EMR_HEADER:                  U_EMRHEADER_print(contents, recnum, off);                  break;
        case U_EMR_POLYBEZIER:              U_EMRPOLYBEZIER_print(contents, recnum, off);              break;
        case U_EMR_POLYGON:                 U_EMRPOLYGON_print(contents, recnum, off);                 break;
        case U_EMR_POLYLINE:                U_EMRPOLYLINE_print(contents, recnum, off);                break;
        case U_EMR_POLYBEZIERTO:            U_EMRPOLYBEZIERTO_print(contents, recnum, off);            break;
        case U_EMR_POLYLINETO:              U_EMRPOLYLINETO_print(contents, recnum, off);              break;
        case U_EMR_POLYPOLYLINE:            U_EMRPOLYPOLYLINE_print(contents, recnum, off);            break;
        case U_EMR_POLYPOLYGON:             U_EMRPOLYPOLYGON_print(contents, recnum, off);             break;
        case U_EMR_SETWINDOWEXTEX:          U_EMRSETWINDOWEXTEX_print(contents, recnum, off);          break;
        case U_EMR_SETWINDOWORGEX:          U_EMRSETWINDOWORGEX_print(contents, recnum, off);          break;
        case U_EMR_SETVIEWPORTEXTEX:        U_EMRSETVIEWPORTEXTEX_print(contents, recnum, off);        break;
        case U_EMR_SETVIEWPORTORGEX:        U_EMRSETVIEWPORTORGEX_print(contents, recnum, off);        break;
        case U_EMR_SETBRUSHORGEX:           U_EMRSETBRUSHORGEX_print(contents, recnum, off);           break;
        case U_EMR_EOF:                     U_EMREOF_print(contents, recnum, off);          regular=0; break;
        case U_EMR_SETPIXELV:               U_EMRSETPIXELV_print(contents, recnum, off);               break;
        case U_EMR_SETMAPPERFLAGS:          U_EMRSETMAPPERFLAGS_print(contents, recnum, off);          break;
        case U_EMR_SETMAPMODE:              U_EMRSETMAPMODE_print(contents, recnum, off);              break;
        case U_EMR_SETBKMODE:               U_EMRSETBKMODE_print(contents, recnum, off);               break;
        case U_EMR_SETPOLYFILLMODE:         U_EMRSETPOLYFILLMODE_print(contents, recnum, off);         break;
        case U_EMR_SETROP2:                 U_EMRSETROP2_print(contents, recnum, off);                 break;
        case U_EMR_SETSTRETCHBLTMODE:       U_EMRSETSTRETCHBLTMODE_print(contents, recnum, off);       break;
        case U_EMR_SETTEXTALIGN:            U_EMRSETTEXTALIGN_print(contents, recnum, off);            break;
        case U_EMR_SETCOLORADJUSTMENT:      U_EMRSETCOLORADJUSTMENT_print(contents, recnum, off);      break;
        case U_EMR_SETTEXTCOLOR:            U_EMRSETTEXTCOLOR_print(contents, recnum, off);            break;
        case U_EMR_SETBKCOLOR:              U_EMRSETBKCOLOR_print(contents, recnum, off);              break;
        case U_EMR_OFFSETCLIPRGN:           U_EMROFFSETCLIPRGN_print(contents, recnum, off);           break;
        case U_EMR_MOVETOEX:                U_EMRMOVETOEX_print(contents, recnum, off);                break;
        case U_EMR_SETMETARGN:              U_EMRSETMETARGN_print(contents, recnum, off);              break;
        case U_EMR_EXCLUDECLIPRECT:         U_EMREXCLUDECLIPRECT_print(contents, recnum, off);         break;
        case U_EMR_INTERSECTCLIPRECT:       U_EMRINTERSECTCLIPRECT_print(contents, recnum, off);       break;
        case U_EMR_SCALEVIEWPORTEXTEX:      U_EMRSCALEVIEWPORTEXTEX_print(contents, recnum, off);      break;
        case U_EMR_SCALEWINDOWEXTEX:        U_EMRSCALEWINDOWEXTEX_print(contents, recnum, off);        break;
        case U_EMR_SAVEDC:                  U_EMRSAVEDC_print(contents, recnum, off);                  break;
        case U_EMR_RESTOREDC:               U_EMRRESTOREDC_print(contents, recnum, off);               break;
        case U_EMR_SETWORLDTRANSFORM:       U_EMRSETWORLDTRANSFORM_print(contents, recnum, off);       break;
        case U_EMR_MODIFYWORLDTRANSFORM:    U_EMRMODIFYWORLDTRANSFORM_print(contents, recnum, off);    break;
        case U_EMR_SELECTOBJECT:            U_EMRSELECTOBJECT_print(contents, recnum, off);            break;
        case U_EMR_CREATEPEN:               U_EMRCREATEPEN_print(contents, recnum, off);               break;
        case U_EMR_CREATEBRUSHINDIRECT:     U_EMRCREATEBRUSHINDIRECT_print(contents, recnum, off);     break;
        case U_EMR_DELETEOBJECT:            U_EMRDELETEOBJECT_print(contents, recnum, off);            break;
        case U_EMR_ANGLEARC:                U_EMRANGLEARC_print(contents, recnum, off);                break;
        case U_EMR_ELLIPSE:                 U_EMRELLIPSE_print(contents, recnum, off);                 break;
        case U_EMR_RECTANGLE:               U_EMRRECTANGLE_print(contents, recnum, off);               break;
        case U_EMR_ROUNDRECT:               U_EMRROUNDRECT_print(contents, recnum, off);               break;
        case U_EMR_ARC:                     U_EMRARC_print(contents, recnum, off);                     break;
        case U_EMR_CHORD:                   U_EMRCHORD_print(contents, recnum, off);                   break;
        case U_EMR_PIE:                     U_EMRPIE_print(contents, recnum, off);                     break;
        case U_EMR_SELECTPALETTE:           U_EMRSELECTPALETTE_print(contents, recnum, off);           break;
        case U_EMR_CREATEPALETTE:           U_EMRCREATEPALETTE_print(contents, recnum, off);           break;
        case U_EMR_SETPALETTEENTRIES:       U_EMRSETPALETTEENTRIES_print(contents, recnum, off);       break;
        case U_EMR_RESIZEPALETTE:           U_EMRRESIZEPALETTE_print(contents, recnum, off);           break;
        case U_EMR_REALIZEPALETTE:          U_EMRREALIZEPALETTE_print(contents, recnum, off);          break;
        case U_EMR_EXTFLOODFILL:            U_EMREXTFLOODFILL_print(contents, recnum, off);            break;
        case U_EMR_LINETO:                  U_EMRLINETO_print(contents, recnum, off);                  break;
        case U_EMR_ARCTO:                   U_EMRARCTO_print(contents, recnum, off);                   break;
        case U_EMR_POLYDRAW:                U_EMRPOLYDRAW_print(contents, recnum, off);                break;
        case U_EMR_SETARCDIRECTION:         U_EMRSETARCDIRECTION_print(contents, recnum, off);         break;
        case U_EMR_SETMITERLIMIT:           U_EMRSETMITERLIMIT_print(contents, recnum, off);           break;
        case U_EMR_BEGINPATH:               U_EMRBEGINPATH_print(contents, recnum, off);               break;
        case U_EMR_ENDPATH:                 U_EMRENDPATH_print(contents, recnum, off);                 break;
        case U_EMR_CLOSEFIGURE:             U_EMRCLOSEFIGURE_print(contents, recnum, off);             break;
        case U_EMR_FILLPATH:                U_EMRFILLPATH_print(contents, recnum, off);                break;
        case U_EMR_STROKEANDFILLPATH:       U_EMRSTROKEANDFILLPATH_print(contents, recnum, off);       break;
        case U_EMR_STROKEPATH:              U_EMRSTROKEPATH_print(contents, recnum, off);              break;
        case U_EMR_FLATTENPATH:             U_EMRFLATTENPATH_print(contents, recnum, off);             break;
        case U_EMR_WIDENPATH:               U_EMRWIDENPATH_print(contents, recnum, off);               break;
        case U_EMR_SELECTCLIPPATH:          U_EMRSELECTCLIPPATH_print(contents, recnum, off);          break;
        case U_EMR_ABORTPATH:               U_EMRABORTPATH_print(contents, recnum, off);               break;
        case U_EMR_UNDEF69:                 U_EMRUNDEF69_print(contents, recnum, off);                 break;
        case U_EMR_COMMENT:                 U_EMRCOMMENT_print(contents, recnum, off);                 break;
        case U_EMR_FILLRGN:                 U_EMRFILLRGN_print(contents, recnum, off);                 break;
        case U_EMR_FRAMERGN:                U_EMRFRAMERGN_print(contents, recnum, off);                break;
        case U_EMR_INVERTRGN:               U_EMRINVERTRGN_print(contents, recnum, off);               break;
        case U_EMR_PAINTRGN:                U_EMRPAINTRGN_print(contents, recnum, off);                break;
        case U_EMR_EXTSELECTCLIPRGN:        U_EMREXTSELECTCLIPRGN_print(contents, recnum, off);        break;
        case U_EMR_BITBLT:                  U_EMRBITBLT_print(contents, recnum, off);                  break;
        case U_EMR_STRETCHBLT:              U_EMRSTRETCHBLT_print(contents, recnum, off);              break;
        case U_EMR_MASKBLT:                 U_EMRMASKBLT_print(contents, recnum, off);                 break;
        case U_EMR_PLGBLT:                  U_EMRPLGBLT_print(contents, recnum, off);                  break;
        case U_EMR_SETDIBITSTODEVICE:       U_EMRSETDIBITSTODEVICE_print(contents, recnum, off);       break;
        case U_EMR_STRETCHDIBITS:           U_EMRSTRETCHDIBITS_print(contents, recnum, off);           break;
        case U_EMR_EXTCREATEFONTINDIRECTW:  U_EMREXTCREATEFONTINDIRECTW_print(contents, recnum, off);  break;
        case U_EMR_EXTTEXTOUTA:             U_EMREXTTEXTOUTA_print(contents, recnum, off);             break;
        case U_EMR_EXTTEXTOUTW:             U_EMREXTTEXTOUTW_print(contents, recnum, off);             break;
        case U_EMR_POLYBEZIER16:            U_EMRPOLYBEZIER16_print(contents, recnum, off);            break;
        case U_EMR_POLYGON16:               U_EMRPOLYGON16_print(contents, recnum, off);               break;
        case U_EMR_POLYLINE16:              U_EMRPOLYLINE16_print(contents, recnum, off);              break;
        case U_EMR_POLYBEZIERTO16:          U_EMRPOLYBEZIERTO16_print(contents, recnum, off);          break;
        case U_EMR_POLYLINETO16:            U_EMRPOLYLINETO16_print(contents, recnum, off);            break;
        case U_EMR_POLYPOLYLINE16:          U_EMRPOLYPOLYLINE16_print(contents, recnum, off);          break;
        case U_EMR_POLYPOLYGON16:           U_EMRPOLYPOLYGON16_print(contents, recnum, off);           break;
        case U_EMR_POLYDRAW16:              U_EMRPOLYDRAW16_print(contents, recnum, off);              break;
        case U_EMR_CREATEMONOBRUSH:         U_EMRCREATEMONOBRUSH_print(contents, recnum, off);         break;
        case U_EMR_CREATEDIBPATTERNBRUSHPT: U_EMRCREATEDIBPATTERNBRUSHPT_print(contents, recnum, off); break;
        case U_EMR_EXTCREATEPEN:            U_EMREXTCREATEPEN_print(contents, recnum, off);            break;
        case U_EMR_POLYTEXTOUTA:            U_EMRPOLYTEXTOUTA_print(contents, recnum, off);            break;
        case U_EMR_POLYTEXTOUTW:            U_EMRPOLYTEXTOUTW_print(contents, recnum, off);            break;
        case U_EMR_SETICMMODE:              U_EMRSETICMMODE_print(contents, recnum, off);              break;
        case U_EMR_CREATECOLORSPACE:        U_EMRCREATECOLORSPACE_print(contents, recnum, off);        break;
        case U_EMR_SETCOLORSPACE:           U_EMRSETCOLORSPACE_print(contents, recnum, off);           break;
        case U_EMR_DELETECOLORSPACE:        U_EMRDELETECOLORSPACE_print(contents, recnum, off);        break;
        case U_EMR_GLSRECORD:               U_EMRGLSRECORD_print(contents, recnum, off);               break;
        case U_EMR_GLSBOUNDEDRECORD:        U_EMRGLSBOUNDEDRECORD_print(contents, recnum, off);        break;
        case U_EMR_PIXELFORMAT:             U_EMRPIXELFORMAT_print(contents, recnum, off);             break;
        case U_EMR_DRAWESCAPE:              U_EMRDRAWESCAPE_print(contents, recnum, off);              break;
        case U_EMR_EXTESCAPE:               U_EMREXTESCAPE_print(contents, recnum, off);               break;
        case U_EMR_UNDEF107:                U_EMRUNDEF107_print(contents, recnum, off);                break;
        case U_EMR_SMALLTEXTOUT:            U_EMRSMALLTEXTOUT_print(contents, recnum, off);            break;
        case U_EMR_FORCEUFIMAPPING:         U_EMRFORCEUFIMAPPING_print(contents, recnum, off);         break;
        case U_EMR_NAMEDESCAPE:             U_EMRNAMEDESCAPE_print(contents, recnum, off);             break;
        case U_EMR_COLORCORRECTPALETTE:     U_EMRCOLORCORRECTPALETTE_print(contents, recnum, off);     break;
        case U_EMR_SETICMPROFILEA:          U_EMRSETICMPROFILEA_print(contents, recnum, off);          break;
        case U_EMR_SETICMPROFILEW:          U_EMRSETICMPROFILEW_print(contents, recnum, off);          break;
        case U_EMR_ALPHABLEND:              U_EMRALPHABLEND_print(contents, recnum, off);              break;
        case U_EMR_SETLAYOUT:               U_EMRSETLAYOUT_print(contents, recnum, off);               break;
        case U_EMR_TRANSPARENTBLT:          U_EMRTRANSPARENTBLT_print(contents, recnum, off);          break;
        case U_EMR_UNDEF117:                U_EMRUNDEF117_print(contents, recnum, off);                break;
        case U_EMR_GRADIENTFILL:            U_EMRGRADIENTFILL_print(contents, recnum, off);            break;
        case U_EMR_SETLINKEDUFIS:           U_EMRSETLINKEDUFIS_print(contents, recnum, off);           break;
        case U_EMR_SETTEXTJUSTIFICATION:    U_EMRSETTEXTJUSTIFICATION_print(contents, recnum, off);    break;
        case U_EMR_COLORMATCHTOTARGETW:     U_EMRCOLORMATCHTOTARGETW_print(contents, recnum, off);     break;
        case U_EMR_CREATECOLORSPACEW:       U_EMRCREATECOLORSPACEW_print(contents, recnum, off);       break;
        default:                            U_EMRNOTIMPLEMENTED_print("?",contents, recnum, off);      break;
    }  //end of switch
    return(regular);
}


#ifdef __cplusplus
}
#endif
