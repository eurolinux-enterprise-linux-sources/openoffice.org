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

#ifndef _EDITDBG_HXX
#define _EDITDBG_HXX

#include <svtools/solar.hrc>
#include <tools/string.hxx>
#include <stdio.h>

class EditEngine;
class ParaPortion;
class EditUndoList;
class TextPortionList;
class SfxItemSet;
class SfxItemPool;
class SfxPoolItem;

ByteString 	DbgOutItem( const SfxItemPool& rPool, const SfxPoolItem& rItem );
void 		DbgOutItemSet( FILE* fp, const SfxItemSet& rSet, BOOL bSearchInParent, BOOL bShowALL );

class SVX_DLLPUBLIC EditDbg
{
public:
	static void			ShowEditEngineData( EditEngine* pEditEngine, BOOL bInfoBox = TRUE );
	static void			ShowPortionData( ParaPortion* pPortion );
	static ByteString	GetPortionInfo( ParaPortion* pPPortion );
	static ByteString	GetTextPortionInfo( TextPortionList& rPortions );
	static ByteString	GetUndoDebStr( EditUndoList* pUndoList );
};


#endif // _EDITDBG_HXX
