#include "xlate_i.h"
#include "parseAFM.h"
#include "isotab.c"

static struct {
    char *font;
    char *cname;
    int mask;
} MaskToFont[N_FONTS] = {
    "Times-Roman",	   "Times_Roman",	   LO_FONT_NORMAL,
    "Times-Bold",	   "Times_Bold",	   LO_FONT_BOLD,
    "Times-Italic",	   "Times_Italic",	   LO_FONT_ITALIC,
    "Times-BoldItalic",	   "Times_BoldItalic",	   LO_FONT_BOLD|LO_FONT_ITALIC,

    "Courier",		   "Courier",		   LO_FONT_FIXED|LO_FONT_NORMAL,
    "Courier-Bold",	   "Courier_Bold",	   LO_FONT_FIXED|LO_FONT_BOLD,
    "Courier-Oblique",	   "Courier_Oblique",	   LO_FONT_FIXED|LO_FONT_ITALIC,
    "Courier-BoldOblique", "Courier_BoldOblique", LO_FONT_FIXED|LO_FONT_BOLD|LO_FONT_ITALIC
};

void
main(void)
{
    int i;

    printf("#include \"xlate_i.h\"\n/* -- automatically generated file, do not edit*/\n");
    for (i = 0; i < N_FONTS; i++) {
	FILE *fp;
	FontInfo*	f;
	int enc;
	CharMetricInfo *iso_cmi[256];

	assert(i == MaskToFont[i].mask);
	fp = fopen(MaskToFont[i].font, "r");
	if (fp != NULL) {
	    assert(MaskToFont[i].mask >= 0 && MaskToFont[i].mask < N_FONTS);
	    switch (parseFile(fp, &f, P_GM))
	    {
		case parseError:
		case earlyEOF:
		case storageProblem:
		default:
		    abort();
		case ok:
		    break;
	    }
	    fclose(fp);
	    /*
	    ** Now build the ISO encoding indexed pointers to the char metrics
	    */
	    for (enc = 0; enc < 256; enc++)
	    {
		int c;
		CharMetricInfo *tmp;


		iso_cmi[enc] = NULL;
		if (*isotab[enc] != '\0')
		{
		    c = f->numOfChars;
		    tmp = f->cmi;
		    while (c-- > 0) {
			if (strcmp(isotab[enc], tmp->name) == 0) {
			    iso_cmi[enc] = tmp;
			    break;
			}
			tmp++;
		    }
		}
	    }
	    /*
	    ** Now write out the info we want in compile-able form
	    */
	    printf("PS_FontInfo PSFE_%s = {\n",MaskToFont[i].cname);
	    printf(" \"%s\",", f->gfi->fontName);
	    printf(" { %d, %d, %d, %d }, %d, %d,\n",
		f->gfi->fontBBox.llx, f->gfi->fontBBox.lly,
		f->gfi->fontBBox.urx, f->gfi->fontBBox.ury,
	        f->gfi->underlinePosition,
	        f->gfi->underlineThickness
	    );
	    printf(" {\n");
	    for (enc = 0; enc < 256; enc++)
		if (iso_cmi[enc] != NULL)
		    printf("   { %d, %d, { %d, %d, %d, %d } },\n",
			iso_cmi[enc]->wx,
			iso_cmi[enc]->wy,
			iso_cmi[enc]->charBBox.llx,
			iso_cmi[enc]->charBBox.lly,
			iso_cmi[enc]->charBBox.urx,
			iso_cmi[enc]->charBBox.ury);
		else
		    printf("    { 0, 0, { 0, 0, 0, 0 } },\n");
	    printf("  }\n};\n");

	}
    }
    printf("PS_FontInfo *PSFE_MaskToFI[%d] = {\n", N_FONTS);
    for (i = 0; i < N_FONTS; i++)
	printf(" &PSFE_%s,\n", MaskToFont[i].cname);
    printf("};\n");
    exit(0);
}
