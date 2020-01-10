/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

#ifndef _VCL_VCLENUM_HXX
#define _VCL_VCLENUM_HXX

#include <sal/types.h>
#include <tools/solar.h>

#ifndef ENUM_TIMEFIELDFORMAT_DECLARED
#define ENUM_TIMEFIELDFORMAT_DECLARED

// By changes you must also change: rsc/vclrsc.hxx
enum TimeFieldFormat {TIMEF_NONE, TIMEF_SEC, TIMEF_100TH_SEC, TIMEF_SEC_CS, TimeFieldFormat_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_EXTTIMEFIELDFORMAT_DECLARED
#define ENUM_EXTTIMEFIELDFORMAT_DECLARED

enum ExtTimeFieldFormat { EXTTIMEF_24H_SHORT, EXTTIMEF_24H_LONG,
						  EXTTIMEF_12H_SHORT, EXTTIMEF_12H_LONG,
						  EXTTIMEF_DURATION_SHORT, EXTTIMEF_DURATION_LONG };

#endif

// ------------------------------------------------------------

#ifndef ENUM_EXTDATEFIELDFORMAT_DECLARED
#define ENUM_EXTDATEFIELDFORMAT_DECLARED

enum ExtDateFieldFormat { XTDATEF_SYSTEM_SHORT, XTDATEF_SYSTEM_SHORT_YY, XTDATEF_SYSTEM_SHORT_YYYY,
						  XTDATEF_SYSTEM_LONG,
						  XTDATEF_SHORT_DDMMYY, XTDATEF_SHORT_MMDDYY, XTDATEF_SHORT_YYMMDD,
						  XTDATEF_SHORT_DDMMYYYY, XTDATEF_SHORT_MMDDYYYY, XTDATEF_SHORT_YYYYMMDD,
						  XTDATEF_SHORT_YYMMDD_DIN5008, XTDATEF_SHORT_YYYYMMDD_DIN5008, ExtDateFieldFormat_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

// to avoid conflicts with enum's declared otherwise
#define GRADIENT_LINEAR 			GradientStyle_LINEAR
#define GRADIENT_AXIAL				GradientStyle_AXIAL
#define GRADIENT_RADIAL 			GradientStyle_RADIAL
#define GRADIENT_ELLIPTICAL 		GradientStyle_ELLIPTICAL
#define GRADIENT_SQUARE 			GradientStyle_SQUARE
#define GRADIENT_RECT				GradientStyle_RECT
#define GRADIENT_FORCE_EQUAL_SIZE	GradientStyle_FORCE_EQUAL_SIZE

#ifndef ENUM_GRADIENTSTYLE_DECLARED
#define ENUM_GRADIENTSTYLE_DECLARED

enum GradientStyle
{
	GRADIENT_LINEAR = 0,
	GRADIENT_AXIAL = 1,
	GRADIENT_RADIAL = 2,
	GRADIENT_ELLIPTICAL = 3,
	GRADIENT_SQUARE = 4,
	GRADIENT_RECT = 5,
	GradientStyle_FORCE_EQUAL_SIZE = SAL_MAX_ENUM
};

#endif

// ------------------------------------------------------------

// to avoid conflicts with enum's declared otherwise
#define HATCH_SINGLE			HatchStyle_SINGLE
#define HATCH_DOUBLE			HatchStyle_DOUBLE
#define HATCH_TRIPLE			HatchStyle_TRIPLE
#define HATCH_FORCE_EQUAL_SIZE	HatchStyle_FORCE_EQUAL_SIZE

#ifndef ENUM_HATCHSTYLE_DECLARED
#define ENUM_HATCHSTYLE_DECLARED

enum HatchStyle
{
	HATCH_SINGLE = 0,
	HATCH_DOUBLE = 1,
	HATCH_TRIPLE = 2,
	HatchStyle_FORCE_EQUAL_SIZE = SAL_MAX_ENUM
};

#endif

// ------------------------------------------------------------

// to avoid conflicts with enum's declared otherwise
#define LINE_NONE				LineStyle_NONE
#define LINE_SOLID				LineStyle_SOLID
#define LINE_DASH				LineStyle_DASH
#define LINE_FORCE_EQUAL_SIZE	LineStyle_FORCE_EQUAL_SIZE

#ifndef ENUM_LINESTYLE_DECLARED
#define ENUM_LINESTYLE_DECLARED

enum LineStyle
{
	LINE_NONE = 0,
	LINE_SOLID = 1,
	LINE_DASH = 2,
	LineStyle_FORCE_EQUAL_SIZE = SAL_MAX_ENUM
};

#endif

// ------------------------------------------------------------

#ifndef ENUM_RASTEROP_DECLARED
#define ENUM_RASTEROP_DECLARED

enum RasterOp { ROP_OVERPAINT, ROP_XOR, ROP_0, ROP_1, ROP_INVERT };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTFAMILY_DECLARED
#define ENUM_FONTFAMILY_DECLARED

enum FontFamily { FAMILY_DONTKNOW, FAMILY_DECORATIVE, FAMILY_MODERN,
				  FAMILY_ROMAN, FAMILY_SCRIPT, FAMILY_SWISS, FAMILY_SYSTEM, FontFamily_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTPITCH_DECLARED
#define ENUM_FONTPITCH_DECLARED

enum FontPitch { PITCH_DONTKNOW, PITCH_FIXED, PITCH_VARIABLE, FontPitch_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_TEXTALIGN_DECLARED
#define ENUM_TEXTALIGN_DECLARED

enum TextAlign { ALIGN_TOP, ALIGN_BASELINE, ALIGN_BOTTOM, TextAlign_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTWEIGHT_DECLARED
#define ENUM_FONTWEIGHT_DECLARED

enum FontWeight { WEIGHT_DONTKNOW, WEIGHT_THIN, WEIGHT_ULTRALIGHT,
				  WEIGHT_LIGHT, WEIGHT_SEMILIGHT, WEIGHT_NORMAL,
				  WEIGHT_MEDIUM, WEIGHT_SEMIBOLD, WEIGHT_BOLD,
				  WEIGHT_ULTRABOLD, WEIGHT_BLACK, FontWeight_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTWIDTH_DECLARED
#define ENUM_FONTWIDTH_DECLARED

enum FontWidth { WIDTH_DONTKNOW, WIDTH_ULTRA_CONDENSED, WIDTH_EXTRA_CONDENSED,
				 WIDTH_CONDENSED, WIDTH_SEMI_CONDENSED, WIDTH_NORMAL,
				 WIDTH_SEMI_EXPANDED, WIDTH_EXPANDED, WIDTH_EXTRA_EXPANDED,
				 WIDTH_ULTRA_EXPANDED,
				 FontWidth_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTITALIC_DECLARED
#define ENUM_FONTITALIC_DECLARED

enum FontItalic { ITALIC_NONE, ITALIC_OBLIQUE, ITALIC_NORMAL, ITALIC_DONTKNOW, FontItalic_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTUNDERLINE_DECLARED
#define ENUM_FONTUNDERLINE_DECLARED

enum FontUnderline { UNDERLINE_NONE, UNDERLINE_SINGLE, UNDERLINE_DOUBLE,
					 UNDERLINE_DOTTED, UNDERLINE_DONTKNOW,
					 UNDERLINE_DASH, UNDERLINE_LONGDASH,
					 UNDERLINE_DASHDOT, UNDERLINE_DASHDOTDOT,
					 UNDERLINE_SMALLWAVE,
					 UNDERLINE_WAVE, UNDERLINE_DOUBLEWAVE,
					 UNDERLINE_BOLD, UNDERLINE_BOLDDOTTED,
					 UNDERLINE_BOLDDASH, UNDERLINE_BOLDLONGDASH,
					 UNDERLINE_BOLDDASHDOT, UNDERLINE_BOLDDASHDOTDOT,
					 UNDERLINE_BOLDWAVE,
					 FontUnderline_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTSTRIKEOUT_DECLARED
#define ENUM_FONTSTRIKEOUT_DECLARED

enum FontStrikeout { STRIKEOUT_NONE, STRIKEOUT_SINGLE, STRIKEOUT_DOUBLE,
					 STRIKEOUT_DONTKNOW, STRIKEOUT_BOLD,
					 STRIKEOUT_SLASH, STRIKEOUT_X,
					 FontStrikeout_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTEMPHASISMARK_DECLARED
#define ENUM_FONTEMPHASISMARK_DECLARED

typedef USHORT FontEmphasisMark;
#define EMPHASISMARK_NONE			((FontEmphasisMark)0x0000)
#define EMPHASISMARK_DOT			((FontEmphasisMark)0x0001)
#define EMPHASISMARK_CIRCLE 		((FontEmphasisMark)0x0002)
#define EMPHASISMARK_DISC			((FontEmphasisMark)0x0003)
#define EMPHASISMARK_ACCENT 		((FontEmphasisMark)0x0004)
#define EMPHASISMARK_STYLE			((FontEmphasisMark)0x00FF)
#define EMPHASISMARK_POS_ABOVE		((FontEmphasisMark)0x1000)
#define EMPHASISMARK_POS_BELOW		((FontEmphasisMark)0x2000)

// Only for kompability
#define EMPHASISMARK_DOTS_ABOVE 	(EMPHASISMARK_DOT | EMPHASISMARK_POS_ABOVE)
#define EMPHASISMARK_DOTS_BELOW 	(EMPHASISMARK_DOT | EMPHASISMARK_POS_BELOW)
#define EMPHASISMARK_SIDE_DOTS		(EMPHASISMARK_ACCENT | EMPHASISMARK_POS_ABOVE)
#define EMPHASISMARK_CIRCLE_ABOVE	(EMPHASISMARK_CIRCLE | EMPHASISMARK_POS_ABOVE)

#endif

// ------------------------------------------------------------

#ifndef ENUM_FONTTYPE_DECLARED
#define ENUM_FONTTYPE_DECLARED

enum FontType { TYPE_DONTKNOW, TYPE_RASTER, TYPE_VECTOR, TYPE_SCALABLE,
				FontType_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

#ifndef ENUM_FONTEMBEDDEDBITMAP_DECLARED
#define ENUM_FONTEMBEDDEDBITMAP_DECLARED

enum FontEmbeddedBitmap { EMBEDDEDBITMAP_DONTKNOW, EMBEDDEDBITMAP_FALSE, EMBEDDEDBITMAP_TRUE };

#endif

#ifndef ENUM_FONTANTIALIAS_DECLARED
#define ENUM_FONTANTIALIAS_DECLARED

enum FontAntiAlias { ANTIALIAS_DONTKNOW, ANTIALIAS_FALSE, ANTIALIAS_TRUE };

#endif

// ------------------------------------------------------------

#ifndef ENUM_KEYFUNCTYPE_DECLARED
#define ENUM_KEYFUNCTYPE_DECLARED

enum KeyFuncType { KEYFUNC_DONTKNOW, KEYFUNC_NEW, KEYFUNC_OPEN, KEYFUNC_SAVE,
				   KEYFUNC_SAVEAS, KEYFUNC_PRINT, KEYFUNC_CLOSE, KEYFUNC_QUIT,
				   KEYFUNC_CUT, KEYFUNC_COPY, KEYFUNC_PASTE, KEYFUNC_UNDO,
				   KEYFUNC_REDO, KEYFUNC_DELETE, KEYFUNC_REPEAT, KEYFUNC_FIND,
				   KEYFUNC_FINDBACKWARD, KEYFUNC_PROPERTIES, KEYFUNC_FRONT,
				   KeyFuncType_FORCE_EQUAL_SIZE=SAL_MAX_ENUM };

#endif

typedef sal_uInt32 sal_UCS4;	// TODO: this should be moved to rtl

#ifndef ENUM_OUTDEVSUPPORT_DECLARED
#define ENUM_OUTDEVSUPPORT_DECLARED

enum OutDevSupportType { OutDevSupport_TransparentRect, OutDevSupport_B2DClip, OutDevSupport_B2DDraw };

#endif

#endif	// _VCL_VCLENUM_HXX
