/**
  @file uemf_print.h Functions for printing records from EMF files.
*/

/*
File:      uemf_print.h
Version:   0.0.3
Date:      24-JUL-2012
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2012 David Mathog and California Institute of Technology (Caltech)
*/

#ifndef _UEMF_PRINT_
#define _UEMF_PRINT_

#ifdef __cplusplus
extern "C" {
#endif

// prototypes
void U_EMRNOTIMPLEMENTED_print(char *name, char *contents, int recnum, int off);
void U_EMRHEADER_print(char *contents, int recnum, int off);
void U_EMRPOLYBEZIER_print(char *contents, int recnum, int off);
void U_EMRPOLYGON_print(char *contents, int recnum, int off);
void U_EMRPOLYLINE_print(char *contents, int recnum, int off);
void U_EMRPOLYBEZIERTO_print(char *contents, int recnum, int off);
void U_EMRPOLYLINETO_print(char *contents, int recnum, int off);
void U_EMRPOLYPOLYLINE_print(char *contents, int recnum, int off);
void U_EMRPOLYPOLYGON_print(char *contents, int recnum, int off);
void U_EMRSETWINDOWEXTEX_print(char *contents, int recnum, int off);
void U_EMRSETWINDOWORGEX_print(char *contents, int recnum, int off);
void U_EMRSETVIEWPORTEXTEX_print(char *contents, int recnum, int off);
void U_EMRSETVIEWPORTORGEX_print(char *contents, int recnum, int off);
void U_EMRSETBRUSHORGEX_print(char *contents, int recnum, int off);
void U_EMREOF_print(char *contents, int recnum, int off);
void U_EMRSETPIXELV_print(char *contents, int recnum, int off);
void U_EMRSETMAPPERFLAGS_print(char *contents, int recnum, int off);
void U_EMRSETMAPMODE_print(char *contents, int recnum, int off);
void U_EMRSETBKMODE_print(char *contents, int recnum, int off);
void U_EMRSETPOLYFILLMODE_print(char *contents, int recnum, int off);
void U_EMRSETROP2_print(char *contents, int recnum, int off);
void U_EMRSETSTRETCHBLTMODE_print(char *contents, int recnum, int off);
void U_EMRSETTEXTALIGN_print(char *contents, int recnum, int off);
void U_EMRSETCOLORADJUSTMENT_print(char *contents, int recnum, int off);
void U_EMRSETTEXTCOLOR_print(char *contents, int recnum, int off);
void U_EMRSETBKCOLOR_print(char *contents, int recnum, int off);
void U_EMROFFSETCLIPRGN_print(char *contents, int recnum, int off);
void U_EMRMOVETOEX_print(char *contents, int recnum, int off);
void U_EMRSETMETARGN_print(char *contents, int recnum, int off);
void U_EMREXCLUDECLIPRECT_print(char *contents, int recnum, int off);
void U_EMRINTERSECTCLIPRECT_print(char *contents, int recnum, int off);
void U_EMRSCALEVIEWPORTEXTEX_print(char *contents, int recnum, int off);
void U_EMRSCALEWINDOWEXTEX_print(char *contents, int recnum, int off);
void U_EMRSAVEDC_print(char *contents, int recnum, int off);
void U_EMRRESTOREDC_print(char *contents, int recnum, int off);
void U_EMRSETWORLDTRANSFORM_print(char *contents, int recnum, int off);
void U_EMRMODIFYWORLDTRANSFORM_print(char *contents, int recnum, int off);
void U_EMRSELECTOBJECT_print(char *contents, int recnum, int off);
void U_EMRCREATEPEN_print(char *contents, int recnum, int off);
void U_EMRCREATEBRUSHINDIRECT_print(char *contents, int recnum, int off);
void U_EMRDELETEOBJECT_print(char *contents, int recnum, int off);
void U_EMRANGLEARC_print(char *contents, int recnum, int off);
void U_EMRELLIPSE_print(char *contents, int recnum, int off);
void U_EMRRECTANGLE_print(char *contents, int recnum, int off);
void U_EMRROUNDRECT_print(char *contents, int recnum, int off);
void U_EMRARC_print(char *contents, int recnum, int off);
void U_EMRCHORD_print(char *contents, int recnum, int off);
void U_EMRPIE_print(char *contents, int recnum, int off);
void U_EMRSELECTPALETTE_print(char *contents, int recnum, int off);
void U_EMRCREATEPALETTE_print(char *contents, int recnum, int off);
void U_EMRSETPALETTEENTRIES_print(char *contents, int recnum, int off);
void U_EMRRESIZEPALETTE_print(char *contents, int recnum, int off);
void U_EMRREALIZEPALETTE_print(char *contents, int recnum, int off);
void U_EMREXTFLOODFILL_print(char *contents, int recnum, int off);
void U_EMRLINETO_print(char *contents, int recnum, int off);
void U_EMRARCTO_print(char *contents, int recnum, int off);
void U_EMRPOLYDRAW_print(char *contents, int recnum, int off);
void U_EMRSETARCDIRECTION_print(char *contents, int recnum, int off);
void U_EMRSETMITERLIMIT_print(char *contents, int recnum, int off);
void U_EMRBEGINPATH_print(char *contents, int recnum, int off);
void U_EMRENDPATH_print(char *contents, int recnum, int off);
void U_EMRCLOSEFIGURE_print(char *contents, int recnum, int off);
void U_EMRFILLPATH_print(char *contents, int recnum, int off);
void U_EMRSTROKEANDFILLPATH_print(char *contents, int recnum, int off);
void U_EMRSTROKEPATH_print(char *contents, int recnum, int off);
void U_EMRFLATTENPATH_print(char *contents, int recnum, int off);
void U_EMRWIDENPATH_print(char *contents, int recnum, int off);
void U_EMRSELECTCLIPPATH_print(char *contents, int recnum, int off);
void U_EMRABORTPATH_print(char *contents, int recnum, int off);
void U_EMRCOMMENT_print(char *contents, int recnum, int off);
void U_EMRFILLRGN_print(char *contents, int recnum, int off);
void U_EMRFRAMERGN_print(char *contents, int recnum, int off);
void U_EMRINVERTRGN_print(char *contents, int recnum, int off);
void U_EMRPAINTRGN_print(char *contents, int recnum, int off);
void U_EMREXTSELECTCLIPRGN_print(char *contents, int recnum, int off);
void U_EMRBITBLT_print(char *contents, int recnum, int off);
void U_EMRSTRETCHBLT_print(char *contents, int recnum, int off);
void U_EMRMASKBLT_print(char *contents, int recnum, int off);
void U_EMRPLGBLT_print(char *contents, int recnum, int off);
void U_EMRSETDIBITSTODEVICE_print(char *contents, int recnum, int off);
void U_EMRSTRETCHDIBITS_print(char *contents, int recnum, int off);
void U_EMREXTCREATEFONTINDIRECTW_print(char *contents, int recnum, int off);
void U_EMREXTTEXTOUTA_print(char *contents, int recnum, int off);
void U_EMREXTTEXTOUTW_print(char *contents, int recnum, int off);
void U_EMRPOLYBEZIER16_print(char *contents, int recnum, int off);
void U_EMRPOLYGON16_print(char *contents, int recnum, int off);
void U_EMRPOLYLINE16_print(char *contents, int recnum, int off);
void U_EMRPOLYBEZIERTO16_print(char *contents, int recnum, int off);
void U_EMRPOLYLINETO16_print(char *contents, int recnum, int off);
void U_EMRPOLYPOLYLINE16_print(char *contents, int recnum, int off);
void U_EMRPOLYPOLYGON16_print(char *contents, int recnum, int off);
void U_EMRPOLYDRAW16_print(char *contents, int recnum, int off);
void U_EMRCREATEMONOBRUSH_print(char *contents, int recnum, int off);
void U_EMRCREATEDIBPATTERNBRUSHPT_print(char *contents, int recnum, int off);
void U_EMREXTCREATEPEN_print(char *contents, int recnum, int off);
void U_EMRSETICMMODE_print(char *contents, int recnum, int off);
void U_EMRCREATECOLORSPACE_print(char *contents, int recnum, int off);
void U_EMRSETCOLORSPACE_print(char *contents, int recnum, int off);
void U_EMRDELETECOLORSPACE_print(char *contents, int recnum, int off);
void U_EMRPIXELFORMAT_print(char *contents, int recnum, int off);
void U_EMRSMALLTEXTOUT_print(char *contents, int recnum, int off);
void U_EMRALPHABLEND_print(char *contents, int recnum, int off);
void U_EMRSETLAYOUT_print(char *contents, int recnum, int off);
void U_EMRTRANSPARENTBLT_print(char *contents, int recnum, int off);
void U_EMRGRADIENTFILL_print(char *contents, int recnum, int off);
void U_EMRCREATECOLORSPACEW_print(char *contents, int recnum, int off);
int  U_emf_onerec_print(char *contents, int recnum, int off);

#ifdef __cplusplus
}
#endif

#endif /* _UEMF_PRINT_ */
