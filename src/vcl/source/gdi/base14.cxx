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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_vcl.hxx"

#include "pdfwriter_impl.hxx"
#include <rtl/strbuf.hxx>

using namespace vcl;
using namespace rtl;

OString PDFWriterImpl::BuiltinFont::getNameObject() const
{
    OStringBuffer aBuf( 16 );
    aBuf.append( '/' );
    const char* pRun = m_pPSName;
    
    unsigned int nCopied = 0;
    while( *pRun )
    {
        if( *pRun >= 'A' && *pRun <= 'Z' )
            nCopied = 0;
        if( nCopied++ < 2 )
            aBuf.append( *pRun );
        pRun++;
    }
    return aBuf.makeStringAndClear();
}

const PDFWriterImpl::BuiltinFont PDFWriterImpl::m_aBuiltinFonts[ 14 ] = {
{ "Courier", // family name
  "Normal", // style
  "Courier", // PSName
  629, -157, // ascend, descend
  FAMILY_MODERN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_FIXED, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    600, 600, 600, 600, 600, 600, 600, 600, // 32 - 39
    600, 600, 600, 600, 600, 600, 600, 600, // 40 - 47
    600, 600, 600, 600, 600, 600, 600, 600, // 48 - 55
    600, 600, 600, 600, 600, 600, 600, 600, // 56 - 63
    600, 600, 600, 600, 600, 600, 600, 600, // 64 - 71
    600, 600, 600, 600, 600, 600, 600, 600, // 72 - 79
    600, 600, 600, 600, 600, 600, 600, 600, // 80 - 87
    600, 600, 600, 600, 600, 600, 600, 600, // 88 - 95
    600, 600, 600, 600, 600, 600, 600, 600, // 96 - 103
    600, 600, 600, 600, 600, 600, 600, 600, // 104 - 111
    600, 600, 600, 600, 600, 600, 600, 600, // 112 - 119
    600, 600, 600, 600, 600, 600, 600, 0, // 120 - 127
    600, 0, 600, 600, 600, 600, 600, 600, // 128 - 135
    600, 600, 600, 600, 600, 0, 600, 0, // 136 - 143
    0, 600, 600, 600, 600, 600, 600, 600, // 144 - 151
    600, 600, 600, 600, 600, 0, 600, 600, // 152 - 159
    600, 600, 600, 600, 600, 600, 600, 600, // 160 - 167
    600, 600, 600, 600, 600, 600, 600, 600, // 168 - 175
    600, 600, 600, 600, 600, 600, 600, 600, // 176 - 183
    600, 600, 600, 600, 600, 600, 600, 600, // 184 - 191
    600, 600, 600, 600, 600, 600, 600, 600, // 192 - 199
    600, 600, 600, 600, 600, 600, 600, 600, // 200 - 207
    600, 600, 600, 600, 600, 600, 600, 600, // 208 - 215
    600, 600, 600, 600, 600, 600, 600, 600, // 216 - 223
    600, 600, 600, 600, 600, 600, 600, 600, // 224 - 231
    600, 600, 600, 600, 600, 600, 600, 600, // 232 - 239
    600, 600, 600, 600, 600, 600, 600, 600, // 240 - 247
    600, 600, 600, 600, 600, 600, 600, 600 // 248 - 255
    }
},

{ "Courier", // family name
  "Italic", // style
  "Courier-Oblique", // PSName
  629, -157, // ascend, descend
  FAMILY_MODERN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_FIXED, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NORMAL, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    600, 600, 600, 600, 600, 600, 600, 600, // 32 - 39
    600, 600, 600, 600, 600, 600, 600, 600, // 40 - 47
    600, 600, 600, 600, 600, 600, 600, 600, // 48 - 55
    600, 600, 600, 600, 600, 600, 600, 600, // 56 - 63
    600, 600, 600, 600, 600, 600, 600, 600, // 64 - 71
    600, 600, 600, 600, 600, 600, 600, 600, // 72 - 79
    600, 600, 600, 600, 600, 600, 600, 600, // 80 - 87
    600, 600, 600, 600, 600, 600, 600, 600, // 88 - 95
    600, 600, 600, 600, 600, 600, 600, 600, // 96 - 103
    600, 600, 600, 600, 600, 600, 600, 600, // 104 - 111
    600, 600, 600, 600, 600, 600, 600, 600, // 112 - 119
    600, 600, 600, 600, 600, 600, 600, 0, // 120 - 127
    600, 0, 600, 600, 600, 600, 600, 600, // 128 - 135
    600, 600, 600, 600, 600, 0, 600, 0, // 136 - 143
    0, 600, 600, 600, 600, 600, 600, 600, // 144 - 151
    600, 600, 600, 600, 600, 0, 600, 600, // 152 - 159
    600, 600, 600, 600, 600, 600, 600, 600, // 160 - 167
    600, 600, 600, 600, 600, 600, 600, 600, // 168 - 175
    600, 600, 600, 600, 600, 600, 600, 600, // 176 - 183
    600, 600, 600, 600, 600, 600, 600, 600, // 184 - 191
    600, 600, 600, 600, 600, 600, 600, 600, // 192 - 199
    600, 600, 600, 600, 600, 600, 600, 600, // 200 - 207
    600, 600, 600, 600, 600, 600, 600, 600, // 208 - 215
    600, 600, 600, 600, 600, 600, 600, 600, // 216 - 223
    600, 600, 600, 600, 600, 600, 600, 600, // 224 - 231
    600, 600, 600, 600, 600, 600, 600, 600, // 232 - 239
    600, 600, 600, 600, 600, 600, 600, 600, // 240 - 247
    600, 600, 600, 600, 600, 600, 600, 600 // 248 - 255
    }
},

{ "Courier", // family name
  "Bold", // style
  "Courier-Bold", // PSName
  629, -157, // ascend, descend
  FAMILY_MODERN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_FIXED, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_BOLD, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    600, 600, 600, 600, 600, 600, 600, 600, // 32 - 39
    600, 600, 600, 600, 600, 600, 600, 600, // 40 - 47
    600, 600, 600, 600, 600, 600, 600, 600, // 48 - 55
    600, 600, 600, 600, 600, 600, 600, 600, // 56 - 63
    600, 600, 600, 600, 600, 600, 600, 600, // 64 - 71
    600, 600, 600, 600, 600, 600, 600, 600, // 72 - 79
    600, 600, 600, 600, 600, 600, 600, 600, // 80 - 87
    600, 600, 600, 600, 600, 600, 600, 600, // 88 - 95
    600, 600, 600, 600, 600, 600, 600, 600, // 96 - 103
    600, 600, 600, 600, 600, 600, 600, 600, // 104 - 111
    600, 600, 600, 600, 600, 600, 600, 600, // 112 - 119
    600, 600, 600, 600, 600, 600, 600, 0, // 120 - 127
    600, 0, 600, 600, 600, 600, 600, 600, // 128 - 135
    600, 600, 600, 600, 600, 0, 600, 0, // 136 - 143
    0, 600, 600, 600, 600, 600, 600, 600, // 144 - 151
    600, 600, 600, 600, 600, 0, 600, 600, // 152 - 159
    600, 600, 600, 600, 600, 600, 600, 600, // 160 - 167
    600, 600, 600, 600, 600, 600, 600, 600, // 168 - 175
    600, 600, 600, 600, 600, 600, 600, 600, // 176 - 183
    600, 600, 600, 600, 600, 600, 600, 600, // 184 - 191
    600, 600, 600, 600, 600, 600, 600, 600, // 192 - 199
    600, 600, 600, 600, 600, 600, 600, 600, // 200 - 207
    600, 600, 600, 600, 600, 600, 600, 600, // 208 - 215
    600, 600, 600, 600, 600, 600, 600, 600, // 216 - 223
    600, 600, 600, 600, 600, 600, 600, 600, // 224 - 231
    600, 600, 600, 600, 600, 600, 600, 600, // 232 - 239
    600, 600, 600, 600, 600, 600, 600, 600, // 240 - 247
    600, 600, 600, 600, 600, 600, 600, 600 // 248 - 255
    }
},

{ "Courier", // family name
  "Bold Italic", // style
  "Courier-BoldOblique", // PSName
  629, -157, // ascend, descend
  FAMILY_MODERN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_FIXED, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_BOLD, // weight type
  ITALIC_NORMAL, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    600, 600, 600, 600, 600, 600, 600, 600, // 32 - 39
    600, 600, 600, 600, 600, 600, 600, 600, // 40 - 47
    600, 600, 600, 600, 600, 600, 600, 600, // 48 - 55
    600, 600, 600, 600, 600, 600, 600, 600, // 56 - 63
    600, 600, 600, 600, 600, 600, 600, 600, // 64 - 71
    600, 600, 600, 600, 600, 600, 600, 600, // 72 - 79
    600, 600, 600, 600, 600, 600, 600, 600, // 80 - 87
    600, 600, 600, 600, 600, 600, 600, 600, // 88 - 95
    600, 600, 600, 600, 600, 600, 600, 600, // 96 - 103
    600, 600, 600, 600, 600, 600, 600, 600, // 104 - 111
    600, 600, 600, 600, 600, 600, 600, 600, // 112 - 119
    600, 600, 600, 600, 600, 600, 600, 0, // 120 - 127
    600, 0, 600, 600, 600, 600, 600, 600, // 128 - 135
    600, 600, 600, 600, 600, 0, 600, 0, // 136 - 143
    0, 600, 600, 600, 600, 600, 600, 600, // 144 - 151
    600, 600, 600, 600, 600, 0, 600, 600, // 152 - 159
    600, 600, 600, 600, 600, 600, 600, 600, // 160 - 167
    600, 600, 600, 600, 600, 600, 600, 600, // 168 - 175
    600, 600, 600, 600, 600, 600, 600, 600, // 176 - 183
    600, 600, 600, 600, 600, 600, 600, 600, // 184 - 191
    600, 600, 600, 600, 600, 600, 600, 600, // 192 - 199
    600, 600, 600, 600, 600, 600, 600, 600, // 200 - 207
    600, 600, 600, 600, 600, 600, 600, 600, // 208 - 215
    600, 600, 600, 600, 600, 600, 600, 600, // 216 - 223
    600, 600, 600, 600, 600, 600, 600, 600, // 224 - 231
    600, 600, 600, 600, 600, 600, 600, 600, // 232 - 239
    600, 600, 600, 600, 600, 600, 600, 600, // 240 - 247
    600, 600, 600, 600, 600, 600, 600, 600 // 248 - 255
    }
},

{ "Helvetica", // family name
  "Normal", // style
  "Helvetica", // PSName
  718, -207, // ascend, descend
  FAMILY_SWISS, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    278, 278, 355, 556, 556, 889, 667, 191, // 32 - 39
    333, 333, 389, 584, 278, 333, 278, 278, // 40 - 47
    556, 556, 556, 556, 556, 556, 556, 556, // 48 - 55
    556, 556, 278, 278, 584, 584, 584, 556, // 56 - 63
    1015, 667, 667, 722, 722, 667, 611, 778, // 64 - 71
    722, 278, 500, 667, 556, 833, 722, 778, // 72 - 79
    667, 778, 722, 667, 611, 722, 667, 944, // 80 - 87
    667, 667, 611, 278, 278, 278, 469, 556, // 88 - 95
    333, 556, 556, 500, 556, 556, 278, 556, // 96 - 103
    556, 222, 222, 500, 222, 833, 556, 556, // 104 - 111
    556, 556, 333, 500, 278, 556, 500, 722, // 112 - 119
    500, 500, 500, 334, 260, 334, 584, 0, // 120 - 127
    556, 0, 222, 556, 333, 1000, 556, 556, // 128 - 135
    333, 1000, 667, 333, 1000, 0, 500, 0, // 136 - 143
    0, 222, 222, 333, 333, 350, 556, 1000, // 144 - 151
    333, 1000, 500, 333, 944, 0, 500, 667, // 152 - 159
    278, 333, 556, 556, 556, 556, 260, 556, // 160 - 167
    333, 737, 370, 556, 584, 333, 737, 333, // 168 - 175
    400, 584, 333, 333, 333, 556, 537, 278, // 176 - 183
    333, 333, 365, 556, 834, 834, 834, 611, // 184 - 191
    667, 667, 667, 667, 667, 667, 1000, 722, // 192 - 199
    667, 667, 667, 667, 278, 278, 278, 278, // 200 - 207
    722, 722, 778, 778, 778, 778, 778, 584, // 208 - 215
    778, 722, 722, 722, 722, 667, 667, 611, // 216 - 223
    556, 556, 556, 556, 556, 556, 889, 500, // 224 - 231
    556, 556, 556, 556, 278, 278, 278, 278, // 232 - 239
    556, 556, 556, 556, 556, 556, 556, 584, // 240 - 247
    611, 556, 556, 556, 556, 500, 556, 500 // 248 - 255
    }
},

{ "Helvetica", // family name
  "Italic", // style
  "Helvetica-Oblique", // PSName
  718, -207, // ascend, descend
  FAMILY_SWISS, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NORMAL, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    278, 278, 355, 556, 556, 889, 667, 191, // 32 - 39
    333, 333, 389, 584, 278, 333, 278, 278, // 40 - 47
    556, 556, 556, 556, 556, 556, 556, 556, // 48 - 55
    556, 556, 278, 278, 584, 584, 584, 556, // 56 - 63
    1015, 667, 667, 722, 722, 667, 611, 778, // 64 - 71
    722, 278, 500, 667, 556, 833, 722, 778, // 72 - 79
    667, 778, 722, 667, 611, 722, 667, 944, // 80 - 87
    667, 667, 611, 278, 278, 278, 469, 556, // 88 - 95
    333, 556, 556, 500, 556, 556, 278, 556, // 96 - 103
    556, 222, 222, 500, 222, 833, 556, 556, // 104 - 111
    556, 556, 333, 500, 278, 556, 500, 722, // 112 - 119
    500, 500, 500, 334, 260, 334, 584, 0, // 120 - 127
    556, 0, 222, 556, 333, 1000, 556, 556, // 128 - 135
    333, 1000, 667, 333, 1000, 0, 500, 0, // 136 - 143
    0, 222, 222, 333, 333, 350, 556, 1000, // 144 - 151
    333, 1000, 500, 333, 944, 0, 500, 667, // 152 - 159
    278, 333, 556, 556, 556, 556, 260, 556, // 160 - 167
    333, 737, 370, 556, 584, 333, 737, 333, // 168 - 175
    400, 584, 333, 333, 333, 556, 537, 278, // 176 - 183
    333, 333, 365, 556, 834, 834, 834, 611, // 184 - 191
    667, 667, 667, 667, 667, 667, 1000, 722, // 192 - 199
    667, 667, 667, 667, 278, 278, 278, 278, // 200 - 207
    722, 722, 778, 778, 778, 778, 778, 584, // 208 - 215
    778, 722, 722, 722, 722, 667, 667, 611, // 216 - 223
    556, 556, 556, 556, 556, 556, 889, 500, // 224 - 231
    556, 556, 556, 556, 278, 278, 278, 278, // 232 - 239
    556, 556, 556, 556, 556, 556, 556, 584, // 240 - 247
    611, 556, 556, 556, 556, 500, 556, 500 // 248 - 255
    }
},

{ "Helvetica", // family name
  "Bold", // style
  "Helvetica-Bold", // PSName
  718, -207, // ascend, descend
  FAMILY_SWISS, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_BOLD, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    278, 333, 474, 556, 556, 889, 722, 238, // 32 - 39
    333, 333, 389, 584, 278, 333, 278, 278, // 40 - 47
    556, 556, 556, 556, 556, 556, 556, 556, // 48 - 55
    556, 556, 333, 333, 584, 584, 584, 611, // 56 - 63
    975, 722, 722, 722, 722, 667, 611, 778, // 64 - 71
    722, 278, 556, 722, 611, 833, 722, 778, // 72 - 79
    667, 778, 722, 667, 611, 722, 667, 944, // 80 - 87
    667, 667, 611, 333, 278, 333, 584, 556, // 88 - 95
    333, 556, 611, 556, 611, 556, 333, 611, // 96 - 103
    611, 278, 278, 556, 278, 889, 611, 611, // 104 - 111
    611, 611, 389, 556, 333, 611, 556, 778, // 112 - 119
    556, 556, 500, 389, 280, 389, 584, 0, // 120 - 127
    556, 0, 278, 556, 500, 1000, 556, 556, // 128 - 135
    333, 1000, 667, 333, 1000, 0, 500, 0, // 136 - 143
    0, 278, 278, 500, 500, 350, 556, 1000, // 144 - 151
    333, 1000, 556, 333, 944, 0, 500, 667, // 152 - 159
    278, 333, 556, 556, 556, 556, 280, 556, // 160 - 167
    333, 737, 370, 556, 584, 333, 737, 333, // 168 - 175
    400, 584, 333, 333, 333, 611, 556, 278, // 176 - 183
    333, 333, 365, 556, 834, 834, 834, 611, // 184 - 191
    722, 722, 722, 722, 722, 722, 1000, 722, // 192 - 199
    667, 667, 667, 667, 278, 278, 278, 278, // 200 - 207
    722, 722, 778, 778, 778, 778, 778, 584, // 208 - 215
    778, 722, 722, 722, 722, 667, 667, 611, // 216 - 223
    556, 556, 556, 556, 556, 556, 889, 556, // 224 - 231
    556, 556, 556, 556, 278, 278, 278, 278, // 232 - 239
    611, 611, 611, 611, 611, 611, 611, 584, // 240 - 247
    611, 611, 611, 611, 611, 556, 611, 556 // 248 - 255
    }
},

{ "Helvetica", // family name
  "Bold Italic", // style
  "Helvetica-BoldOblique", // PSName
  718, -207, // ascend, descend
  FAMILY_SWISS, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_BOLD, // weight type
  ITALIC_NORMAL, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    278, 333, 474, 556, 556, 889, 722, 238, // 32 - 39
    333, 333, 389, 584, 278, 333, 278, 278, // 40 - 47
    556, 556, 556, 556, 556, 556, 556, 556, // 48 - 55
    556, 556, 333, 333, 584, 584, 584, 611, // 56 - 63
    975, 722, 722, 722, 722, 667, 611, 778, // 64 - 71
    722, 278, 556, 722, 611, 833, 722, 778, // 72 - 79
    667, 778, 722, 667, 611, 722, 667, 944, // 80 - 87
    667, 667, 611, 333, 278, 333, 584, 556, // 88 - 95
    333, 556, 611, 556, 611, 556, 333, 611, // 96 - 103
    611, 278, 278, 556, 278, 889, 611, 611, // 104 - 111
    611, 611, 389, 556, 333, 611, 556, 778, // 112 - 119
    556, 556, 500, 389, 280, 389, 584, 0, // 120 - 127
    556, 0, 278, 556, 500, 1000, 556, 556, // 128 - 135
    333, 1000, 667, 333, 1000, 0, 500, 0, // 136 - 143
    0, 278, 278, 500, 500, 350, 556, 1000, // 144 - 151
    333, 1000, 556, 333, 944, 0, 500, 667, // 152 - 159
    278, 333, 556, 556, 556, 556, 280, 556, // 160 - 167
    333, 737, 370, 556, 584, 333, 737, 333, // 168 - 175
    400, 584, 333, 333, 333, 611, 556, 278, // 176 - 183
    333, 333, 365, 556, 834, 834, 834, 611, // 184 - 191
    722, 722, 722, 722, 722, 722, 1000, 722, // 192 - 199
    667, 667, 667, 667, 278, 278, 278, 278, // 200 - 207
    722, 722, 778, 778, 778, 778, 778, 584, // 208 - 215
    778, 722, 722, 722, 722, 667, 667, 611, // 216 - 223
    556, 556, 556, 556, 556, 556, 889, 556, // 224 - 231
    556, 556, 556, 556, 278, 278, 278, 278, // 232 - 239
    611, 611, 611, 611, 611, 611, 611, 584, // 240 - 247
    611, 611, 611, 611, 611, 556, 611, 556 // 248 - 255
    }
},

{ "Times", // family name
  "Normal", // style
  "Times-Roman", // PSName
  683, -217, // ascend, descend
  FAMILY_ROMAN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    250, 333, 408, 500, 500, 833, 778, 180, // 32 - 39
    333, 333, 500, 564, 250, 333, 250, 278, // 40 - 47
    500, 500, 500, 500, 500, 500, 500, 500, // 48 - 55
    500, 500, 278, 278, 564, 564, 564, 444, // 56 - 63
    921, 722, 667, 667, 722, 611, 556, 722, // 64 - 71
    722, 333, 389, 722, 611, 889, 722, 722, // 72 - 79
    556, 722, 667, 556, 611, 722, 722, 944, // 80 - 87
    722, 722, 611, 333, 278, 333, 469, 500, // 88 - 95
    333, 444, 500, 444, 500, 444, 333, 500, // 96 - 103
    500, 278, 278, 500, 278, 778, 500, 500, // 104 - 111
    500, 500, 333, 389, 278, 500, 500, 722, // 112 - 119
    500, 500, 444, 480, 200, 480, 541, 0, // 120 - 127
    500, 0, 333, 500, 444, 1000, 500, 500, // 128 - 135
    333, 1000, 556, 333, 889, 0, 444, 0, // 136 - 143
    0, 333, 333, 444, 444, 350, 500, 1000, // 144 - 151
    333, 980, 389, 333, 722, 0, 444, 722, // 152 - 159
    250, 333, 500, 500, 500, 500, 200, 500, // 160 - 167
    333, 760, 276, 500, 564, 333, 760, 333, // 168 - 175
    400, 564, 300, 300, 333, 500, 453, 250, // 176 - 183
    333, 300, 310, 500, 750, 750, 750, 444, // 184 - 191
    722, 722, 722, 722, 722, 722, 889, 667, // 192 - 199
    611, 611, 611, 611, 333, 333, 333, 333, // 200 - 207
    722, 722, 722, 722, 722, 722, 722, 564, // 208 - 215
    722, 722, 722, 722, 722, 722, 556, 500, // 216 - 223
    444, 444, 444, 444, 444, 444, 667, 444, // 224 - 231
    444, 444, 444, 444, 278, 278, 278, 278, // 232 - 239
    500, 500, 500, 500, 500, 500, 500, 564, // 240 - 247
    500, 500, 500, 500, 500, 500, 500, 500 // 248 - 255
    }
},

{ "Times", // family name
  "Italic", // style
  "Times-Italic", // PSName
  683, -217, // ascend, descend
  FAMILY_ROMAN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NORMAL, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    250, 333, 420, 500, 500, 833, 778, 214, // 32 - 39
    333, 333, 500, 675, 250, 333, 250, 278, // 40 - 47
    500, 500, 500, 500, 500, 500, 500, 500, // 48 - 55
    500, 500, 333, 333, 675, 675, 675, 500, // 56 - 63
    920, 611, 611, 667, 722, 611, 611, 722, // 64 - 71
    722, 333, 444, 667, 556, 833, 667, 722, // 72 - 79
    611, 722, 611, 500, 556, 722, 611, 833, // 80 - 87
    611, 556, 556, 389, 278, 389, 422, 500, // 88 - 95
    333, 500, 500, 444, 500, 444, 278, 500, // 96 - 103
    500, 278, 278, 444, 278, 722, 500, 500, // 104 - 111
    500, 500, 389, 389, 278, 500, 444, 667, // 112 - 119
    444, 444, 389, 400, 275, 400, 541, 0, // 120 - 127
    500, 0, 333, 500, 556, 889, 500, 500, // 128 - 135
    333, 1000, 500, 333, 944, 0, 389, 0, // 136 - 143
    0, 333, 333, 556, 556, 350, 500, 889, // 144 - 151
    333, 980, 389, 333, 667, 0, 389, 556, // 152 - 159
    250, 389, 500, 500, 500, 500, 275, 500, // 160 - 167
    333, 760, 276, 500, 675, 333, 760, 333, // 168 - 175
    400, 675, 300, 300, 333, 500, 523, 250, // 176 - 183
    333, 300, 310, 500, 750, 750, 750, 500, // 184 - 191
    611, 611, 611, 611, 611, 611, 889, 667, // 192 - 199
    611, 611, 611, 611, 333, 333, 333, 333, // 200 - 207
    722, 667, 722, 722, 722, 722, 722, 675, // 208 - 215
    722, 722, 722, 722, 722, 556, 611, 500, // 216 - 223
    500, 500, 500, 500, 500, 500, 667, 444, // 224 - 231
    444, 444, 444, 444, 278, 278, 278, 278, // 232 - 239
    500, 500, 500, 500, 500, 500, 500, 675, // 240 - 247
    500, 500, 500, 500, 500, 444, 500, 444 // 248 - 255
    }
},

{ "Times", // family name
  "Bold", // style
  "Times-Bold", // PSName
  683, -217, // ascend, descend
  FAMILY_ROMAN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_BOLD, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    250, 333, 555, 500, 500, 1000, 833, 278, // 32 - 39
    333, 333, 500, 570, 250, 333, 250, 278, // 40 - 47
    500, 500, 500, 500, 500, 500, 500, 500, // 48 - 55
    500, 500, 333, 333, 570, 570, 570, 500, // 56 - 63
    930, 722, 667, 722, 722, 667, 611, 778, // 64 - 71
    778, 389, 500, 778, 667, 944, 722, 778, // 72 - 79
    611, 778, 722, 556, 667, 722, 722, 1000, // 80 - 87
    722, 722, 667, 333, 278, 333, 581, 500, // 88 - 95
    333, 500, 556, 444, 556, 444, 333, 500, // 96 - 103
    556, 278, 333, 556, 278, 833, 556, 500, // 104 - 111
    556, 556, 444, 389, 333, 556, 500, 722, // 112 - 119
    500, 500, 444, 394, 220, 394, 520, 0, // 120 - 127
    500, 0, 333, 500, 500, 1000, 500, 500, // 128 - 135
    333, 1000, 556, 333, 1000, 0, 444, 0, // 136 - 143
    0, 333, 333, 500, 500, 350, 500, 1000, // 144 - 151
    333, 1000, 389, 333, 722, 0, 444, 722, // 152 - 159
    250, 333, 500, 500, 500, 500, 220, 500, // 160 - 167
    333, 747, 300, 500, 570, 333, 747, 333, // 168 - 175
    400, 570, 300, 300, 333, 556, 540, 250, // 176 - 183
    333, 300, 330, 500, 750, 750, 750, 500, // 184 - 191
    722, 722, 722, 722, 722, 722, 1000, 722, // 192 - 199
    667, 667, 667, 667, 389, 389, 389, 389, // 200 - 207
    722, 722, 778, 778, 778, 778, 778, 570, // 208 - 215
    778, 722, 722, 722, 722, 722, 611, 556, // 216 - 223
    500, 500, 500, 500, 500, 500, 722, 444, // 224 - 231
    444, 444, 444, 444, 278, 278, 278, 278, // 232 - 239
    500, 556, 500, 500, 500, 500, 500, 570, // 240 - 247
    500, 556, 556, 556, 556, 500, 556, 500 // 248 - 255
    }
},

{ "Times", // family name
  "Bold Italic", // style
  "Times-BoldItalic", // PSName
  683, -217, // ascend, descend
  FAMILY_ROMAN, // family style
  RTL_TEXTENCODING_MS_1252, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_BOLD, // weight type
  ITALIC_NORMAL, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    250, 389, 555, 500, 500, 833, 778, 278, // 32 - 39
    333, 333, 500, 570, 250, 333, 250, 278, // 40 - 47
    500, 500, 500, 500, 500, 500, 500, 500, // 48 - 55
    500, 500, 333, 333, 570, 570, 570, 500, // 56 - 63
    832, 667, 667, 667, 722, 667, 667, 722, // 64 - 71
    778, 389, 500, 667, 611, 889, 722, 722, // 72 - 79
    611, 722, 667, 556, 611, 722, 667, 889, // 80 - 87
    667, 611, 611, 333, 278, 333, 570, 500, // 88 - 95
    333, 500, 500, 444, 500, 444, 333, 500, // 96 - 103
    556, 278, 278, 500, 278, 778, 556, 500, // 104 - 111
    500, 500, 389, 389, 278, 556, 444, 667, // 112 - 119
    500, 444, 389, 348, 220, 348, 570, 0, // 120 - 127
    500, 0, 333, 500, 500, 1000, 500, 500, // 128 - 135
    333, 1000, 556, 333, 944, 0, 389, 0, // 136 - 143
    0, 333, 333, 500, 500, 350, 500, 1000, // 144 - 151
    333, 1000, 389, 333, 722, 0, 389, 611, // 152 - 159
    250, 389, 500, 500, 500, 500, 220, 500, // 160 - 167
    333, 747, 266, 500, 606, 333, 747, 333, // 168 - 175
    400, 570, 300, 300, 333, 576, 500, 250, // 176 - 183
    333, 300, 300, 500, 750, 750, 750, 500, // 184 - 191
    667, 667, 667, 667, 667, 667, 944, 667, // 192 - 199
    667, 667, 667, 667, 389, 389, 389, 389, // 200 - 207
    722, 722, 722, 722, 722, 722, 722, 570, // 208 - 215
    722, 722, 722, 722, 722, 611, 611, 500, // 216 - 223
    500, 500, 500, 500, 500, 500, 722, 444, // 224 - 231
    444, 444, 444, 444, 278, 278, 278, 278, // 232 - 239
    500, 556, 500, 500, 500, 500, 500, 570, // 240 - 247
    500, 556, 556, 556, 556, 444, 500, 444 // 248 - 255
    }
},

{ "Symbol", // family name
  "Normal", // style
  "Symbol", // PSName
  1010, -293, // ascend, descend
  FAMILY_DONTKNOW, // family style
  RTL_TEXTENCODING_SYMBOL, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    250, 333, 713, 500, 549, 833, 778, 439, // 32 - 39
    333, 333, 500, 549, 250, 549, 250, 278, // 40 - 47
    500, 500, 500, 500, 500, 500, 500, 500, // 48 - 55
    500, 500, 278, 278, 549, 549, 549, 444, // 56 - 63
    549, 722, 667, 722, 612, 611, 763, 603, // 64 - 71
    722, 333, 631, 722, 686, 889, 722, 722, // 72 - 79
    768, 741, 556, 592, 611, 690, 439, 768, // 80 - 87
    645, 795, 611, 333, 863, 333, 658, 500, // 88 - 95
    500, 631, 549, 549, 494, 439, 521, 411, // 96 - 103
    603, 329, 603, 549, 549, 576, 521, 549, // 104 - 111
    549, 521, 549, 603, 439, 576, 713, 686, // 112 - 119
    493, 686, 494, 480, 200, 480, 549, 0, // 120 - 127
    0, 0, 0, 0, 0, 0, 0, 0, // 128 - 135
    0, 0, 0, 0, 0, 0, 0, 0, // 136 - 143
    0, 0, 0, 0, 0, 0, 0, 0, // 144 - 151
    0, 0, 0, 0, 0, 0, 0, 0, // 152 - 159
    750, 620, 247, 549, 167, 713, 500, 753, // 160 - 167
    753, 753, 753, 1042, 987, 603, 987, 603, // 168 - 175
    400, 549, 411, 549, 549, 713, 494, 460, // 176 - 183
    549, 549, 549, 549, 1000, 603, 1000, 658, // 184 - 191
    823, 686, 795, 987, 768, 768, 823, 768, // 192 - 199
    768, 713, 713, 713, 713, 713, 713, 713, // 200 - 207
    768, 713, 790, 790, 890, 823, 549, 250, // 208 - 215
    713, 603, 603, 1042, 987, 603, 987, 603, // 216 - 223
    494, 329, 790, 790, 786, 713, 384, 384, // 224 - 231
    384, 384, 384, 384, 494, 494, 494, 494, // 232 - 239
    0, 329, 274, 686, 686, 686, 384, 384, // 240 - 247
    384, 384, 384, 384, 494, 494, 494, 0 // 248 - 255
    }
},

{ "ZapfDingbats", // family name
  "Normal", // style
  "ZapfDingbats", // PSName
  820, -143, // ascend, descend
  FAMILY_DONTKNOW, // family style
  RTL_TEXTENCODING_ADOBE_DINGBATS, // charset
  PITCH_VARIABLE, // pitch
  WIDTH_NORMAL, // width type
  WEIGHT_NORMAL, // weight type
  ITALIC_NONE, // italic type
  { 0, 0, 0, 0, 0, 0, 0, 0, // 0 - 7
    0, 0, 0, 0, 0, 0, 0, 0, // 8 - 15
    0, 0, 0, 0, 0, 0, 0, 0, // 16 - 23
    0, 0, 0, 0, 0, 0, 0, 0, // 24 - 31
    278, 974, 961, 974, 980, 719, 789, 790, // 32 - 39
    791, 690, 960, 939, 549, 855, 911, 933, // 40 - 47
    911, 945, 974, 755, 846, 762, 761, 571, // 48 - 55
    677, 763, 760, 759, 754, 494, 552, 537, // 56 - 63
    577, 692, 786, 788, 788, 790, 793, 794, // 64 - 71
    816, 823, 789, 841, 823, 833, 816, 831, // 72 - 79
    923, 744, 723, 749, 790, 792, 695, 776, // 80 - 87
    768, 792, 759, 707, 708, 682, 701, 826, // 88 - 95
    815, 789, 789, 707, 687, 696, 689, 786, // 96 - 103
    787, 713, 791, 785, 791, 873, 761, 762, // 104 - 111
    762, 759, 759, 892, 892, 788, 784, 438, // 112 - 119
    138, 277, 415, 392, 392, 668, 668, 0, // 120 - 127
    390, 390, 317, 317, 276, 276, 509, 509, // 128 - 135
    410, 410, 234, 234, 334, 334, 0, 0, // 136 - 143
    0, 0, 0, 0, 0, 0, 0, 0, // 144 - 151
    0, 0, 0, 0, 0, 0, 0, 0, // 152 - 159
    0, 732, 544, 544, 910, 667, 760, 760, // 160 - 167
    776, 595, 694, 626, 788, 788, 788, 788, // 168 - 175
    788, 788, 788, 788, 788, 788, 788, 788, // 176 - 183
    788, 788, 788, 788, 788, 788, 788, 788, // 184 - 191
    788, 788, 788, 788, 788, 788, 788, 788, // 192 - 199
    788, 788, 788, 788, 788, 788, 788, 788, // 200 - 207
    788, 788, 788, 788, 894, 838, 1016, 458, // 208 - 215
    748, 924, 748, 918, 927, 928, 928, 834, // 216 - 223
    873, 828, 924, 924, 917, 930, 931, 463, // 224 - 231
    883, 836, 836, 867, 867, 696, 696, 874, // 232 - 239
    0, 874, 760, 946, 771, 865, 771, 888, // 240 - 247
    967, 888, 831, 873, 927, 970, 918, 0 // 248 - 255
    }
}

};

