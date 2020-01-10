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

#ifndef _TTBASIC_HXX
#define _TTBASIC_HXX

#include <basic/sbstar.hxx>
#include <basic/mybasic.hxx>

class ErrorEntry;

#define SBXID_TTBASIC	0x5454		// TTBasic: TT

#define SBXCR_TEST2   0x54534554L   // TEST

class TTBasic : public MyBasic
{
public:
	SBX_DECL_PERSIST_NODATA(SBXCR_TEST2,SBXID_TTBASIC,1);
	TYPEINFO();
	TTBasic();
   ~TTBasic();
	BOOL Compile( SbModule* );
	static MyBasic* CreateMyBasic();

	// nicht mit #ifdefs klammern, da diese Headerdatei f�r testtool und basic
	// gleicherma�en verwendet wird.
	DECL_LINK( CErrorImpl, ErrorEntry* );
//	SbxObject *pTestObject;			// f�r das Testtool; ansonsten NULL

	void LoadIniFile();
	SbTextType GetSymbolType( const String &Symbol, BOOL bWasTTControl );	// Besimmt den erweiterten Symboltyp f�r das Syntaxhighlighting
	virtual const String GetSpechialErrorText();
	virtual void ReportRuntimeError( AppBasEd *pEditWin );
	virtual void DebugFindNoErrors( BOOL bDebugFindNoErrors );
};

SV_DECL_IMPL_REF(TTBasic)

#endif
