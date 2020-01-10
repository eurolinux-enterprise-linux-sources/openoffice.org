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
#ifndef _TOOLS_TENCCVT_HXX
#define _TOOLS_TENCCVT_HXX

#include <rtl/textenc.h>
#ifndef _TOOLS_SOLAR_H
#include <tools/solar.h>
#endif
#include "tools/toolsdllapi.h"

// ----------------------------------------
// - Functions for handling Import/Export -
// ----------------------------------------

// return an encoding which has more defined Characters as the given
// encoding, but have the same definition for the defined characters
// e.g.: windows-1252 for iso-8859-1 or windows-1254 for iso-8859-9
TOOLS_DLLPUBLIC rtl_TextEncoding GetExtendedCompatibilityTextEncoding( rtl_TextEncoding eEncoding );

// return an encoding which has more defined Characters as the given
// encoding. The encodings could be different.
// e.g.: windows-1251 for iso-8859-5
TOOLS_DLLPUBLIC rtl_TextEncoding GetExtendedTextEncoding( rtl_TextEncoding eEncoding );

// if the given encoding is an multi-byte encoding (which allows more than
// one byte per char, e.g. UTF-8 or Shift-JIS), a one-byte encoding
// is returned (normally windows-1252).
TOOLS_DLLPUBLIC rtl_TextEncoding GetOneByteTextEncoding( rtl_TextEncoding eEncoding );

TOOLS_DLLPUBLIC rtl_TextEncoding GetSOLoadTextEncoding( rtl_TextEncoding eEncoding, USHORT nVersion = SOFFICE_FILEFORMAT_50 );
TOOLS_DLLPUBLIC rtl_TextEncoding GetSOStoreTextEncoding( rtl_TextEncoding eEncoding, USHORT nVersion = SOFFICE_FILEFORMAT_50 );

#endif  // _TOOLS_TENCCVT_HXX
