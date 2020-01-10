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
#include "precompiled_sc.hxx"




#include "expbase.hxx"
#include "document.hxx"
#include "editutil.hxx"


//------------------------------------------------------------------

#if defined(UNX)
const sal_Char __FAR_DATA ScExportBase::sNewLine = '\012';
#else
const sal_Char __FAR_DATA ScExportBase::sNewLine[] = "\015\012";
#endif


ScExportBase::ScExportBase( SvStream& rStrmP, ScDocument* pDocP,
				const ScRange& rRangeP )
			:
			rStrm( rStrmP ),
			aRange( rRangeP ),
			pDoc( pDocP ),
			pFormatter( pDocP->GetFormatTable() ),
			pEditEngine( NULL )
{
}


ScExportBase::~ScExportBase()
{
	delete pEditEngine;
}


BOOL ScExportBase::GetDataArea( SCTAB nTab, SCCOL& nStartCol,
			SCROW& nStartRow, SCCOL& nEndCol, SCROW& nEndRow ) const
{
	pDoc->GetDataStart( nTab, nStartCol, nStartRow );
	pDoc->GetPrintArea( nTab, nEndCol, nEndRow, TRUE );
	return TrimDataArea( nTab, nStartCol, nStartRow, nEndCol, nEndRow );
}


BOOL ScExportBase::TrimDataArea( SCTAB nTab, SCCOL& nStartCol,
			SCROW& nStartRow, SCCOL& nEndCol, SCROW& nEndRow ) const
{
	while ( nStartCol <= nEndCol &&
			pDoc->GetColFlags( nStartCol, nTab ) & CR_HIDDEN )
		++nStartCol;
	while ( nStartCol <= nEndCol &&
			pDoc->GetColFlags( nEndCol, nTab ) & CR_HIDDEN )
		--nEndCol;
    nStartRow = pDoc->GetRowFlagsArray( nTab).GetFirstForCondition( nStartRow,
            nEndRow, CR_HIDDEN, 0);
    nEndRow = pDoc->GetRowFlagsArray( nTab).GetLastForCondition( nStartRow,
            nEndRow, CR_HIDDEN, 0);
    return nStartCol <= nEndCol && nStartRow <= nEndRow && nEndRow !=
        ::std::numeric_limits<SCROW>::max();
}


BOOL ScExportBase::IsEmptyTable( SCTAB nTab ) const
{
	if ( !pDoc->HasTable( nTab ) || !pDoc->IsVisible( nTab ) )
		return TRUE;
	SCCOL nStartCol, nEndCol;
	SCROW nStartRow, nEndRow;
	return !GetDataArea( nTab, nStartCol, nStartRow, nEndCol, nEndRow );
}


ScFieldEditEngine& ScExportBase::GetEditEngine() const
{
	if ( !pEditEngine )
		((ScExportBase*)this)->pEditEngine = new ScFieldEditEngine( pDoc->GetEditPool() );
	return *pEditEngine;
}


