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

#include <boost/shared_ptr.hpp>

#define SC_HTMLPARS_CXX
#include "scitems.hxx"
#include <svx/eeitem.hxx>

#include <svx/htmlcfg.hxx>
#include <svx/algitem.hxx>
#include <svx/colritem.hxx>
#include <svx/brshitem.hxx>
#include <svx/editeng.hxx>
#include <svx/fhgtitem.hxx>
#include <svx/fontitem.hxx>
#include <svx/impgrf.hxx>
#include <svx/postitem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/boxitem.hxx>
#include <sfx2/objsh.hxx>
#include <svtools/eitem.hxx>
#include <svtools/filter.hxx>
#include <svtools/parhtml.hxx>
#include <svtools/htmlkywd.hxx>
#include <svtools/htmltokn.h>
#include <sfx2/docfile.hxx>

#include <vcl/svapp.hxx>
#include <tools/urlobj.hxx>
#include <tools/tenccvt.hxx>

#include "htmlpars.hxx"
#include "global.hxx"
#include "document.hxx"
#include "rangelst.hxx"

#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>


using namespace ::com::sun::star;


SV_IMPL_VARARR_SORT( ScHTMLColOffset, ULONG );


// ============================================================================
// BASE class for HTML parser classes
// ============================================================================

ScHTMLParser::ScHTMLParser( EditEngine* pEditEngine, ScDocument* pDoc ) :
    ScEEParser( pEditEngine ),
    mpDoc( pDoc )
{
	SvxHtmlOptions* pHtmlOptions = SvxHtmlOptions::Get();
    for( sal_uInt16 nIndex = 0; nIndex < SC_HTML_FONTSIZES; ++nIndex )
        maFontHeights[ nIndex ] = pHtmlOptions->GetFontSize( nIndex ) * 20;
}

ScHTMLParser::~ScHTMLParser()
{
}


// ============================================================================

ScHTMLLayoutParser::ScHTMLLayoutParser( EditEngine* pEditP, const String& rBaseURL, const Size& aPageSizeP, ScDocument* pDocP ) :
        ScHTMLParser( pEditP, pDocP ),
        aPageSize( aPageSizeP ),
        aBaseURL( rBaseURL ),
        xLockedList( new ScRangeList ),
        pTables( NULL ),
        pColOffset( new ScHTMLColOffset ),
        pLocalColOffset( new ScHTMLColOffset ),
        nFirstTableCell(0),
        nTableLevel(0),
        nTable(0),
        nMaxTable(0),
        nColCntStart(0),
        nMaxCol(0),
        nTableWidth(0),
        nColOffset(0),
        nColOffsetStart(0),
        nMetaCnt(0),
        nOffsetTolerance( SC_HTML_OFFSET_TOLERANCE_SMALL ),
        bTabInTabCell( FALSE ),
        bFirstRow( TRUE ),
        bInCell( FALSE ),
        bInTitle( FALSE )
{
    MakeColNoRef( pLocalColOffset, 0, 0, 0, 0 );
    MakeColNoRef( pColOffset, 0, 0, 0, 0 );
}


ScHTMLLayoutParser::~ScHTMLLayoutParser()
{
    ScHTMLTableStackEntry* pS;
    while ( (pS = aTableStack.Pop()) != 0 )
    {
        if ( pList->GetPos( pS->pCellEntry ) == LIST_ENTRY_NOTFOUND )
            delete pS->pCellEntry;
        if ( pS->pLocalColOffset != pLocalColOffset )
            delete pS->pLocalColOffset;
        delete pS;
    }
    if ( pLocalColOffset )
        delete pLocalColOffset;
    if ( pColOffset )
        delete pColOffset;
    if ( pTables )
    {
        for ( Table* pT = (Table*) pTables->First(); pT; pT = (Table*) pTables->Next() )
            delete pT;
        delete pTables;
    }
}


ULONG ScHTMLLayoutParser::Read( SvStream& rStream, const String& rBaseURL )
{
    Link aOldLink = pEdit->GetImportHdl();
    pEdit->SetImportHdl( LINK( this, ScHTMLLayoutParser, HTMLImportHdl ) );

    SfxObjectShell* pObjSh = mpDoc->GetDocumentShell();
    BOOL bLoading = pObjSh && pObjSh->IsLoading();

    SvKeyValueIteratorRef xValues;
    SvKeyValueIterator* pAttributes = NULL;
    if ( bLoading )
        pAttributes = pObjSh->GetHeaderAttributes();
    else
    {
        //  When not loading, set up fake http headers to force the SfxHTMLParser to use UTF8
        //  (used when pasting from clipboard)

        const sal_Char* pCharSet = rtl_getBestMimeCharsetFromTextEncoding( RTL_TEXTENCODING_UTF8 );
        if( pCharSet )
        {
            String aContentType = String::CreateFromAscii( "text/html; charset=" );
            aContentType.AppendAscii( pCharSet );

            xValues = new SvKeyValueIterator;
            xValues->Append( SvKeyValue( String::CreateFromAscii( OOO_STRING_SVTOOLS_HTML_META_content_type ), aContentType ) );
            pAttributes = xValues;
        }
    }

    ULONG nErr = pEdit->Read( rStream, rBaseURL, EE_FORMAT_HTML, pAttributes );

    pEdit->SetImportHdl( aOldLink );
    // Spaltenbreiten erzeugen
    Adjust();
    OutputDevice* pDefaultDev = Application::GetDefaultDevice();
    USHORT nCount = pColOffset->Count();
    const ULONG* pOff = (const ULONG*) pColOffset->GetData();
    ULONG nOff = *pOff++;
    Size aSize;
    for ( USHORT j = 1; j < nCount; j++, pOff++ )
    {
        aSize.Width() = *pOff - nOff;
        aSize = pDefaultDev->PixelToLogic( aSize, MapMode( MAP_TWIP ) );
        pColWidths->Insert( j-1, (void*)aSize.Width() );
        nOff = *pOff;
    }
    return nErr;
}


const ScHTMLTable* ScHTMLLayoutParser::GetGlobalTable() const
{
    return 0;
}


void ScHTMLLayoutParser::NewActEntry( ScEEParseEntry* pE )
{
    ScEEParser::NewActEntry( pE );
    if ( pE )
    {
        if ( !pE->aSel.HasRange() )
        {   // komplett leer, nachfolgender Text landet im gleichen Absatz!
            pActEntry->aSel.nStartPara = pE->aSel.nEndPara;
            pActEntry->aSel.nStartPos = pE->aSel.nEndPos;
        }
    }
    pActEntry->aSel.nEndPara = pActEntry->aSel.nStartPara;
    pActEntry->aSel.nEndPos = pActEntry->aSel.nStartPos;
}


void ScHTMLLayoutParser::EntryEnd( ScEEParseEntry* pE, const ESelection& rSel )
{
    if ( rSel.nEndPara >= pE->aSel.nStartPara )
    {
        pE->aSel.nEndPara = rSel.nEndPara;
        pE->aSel.nEndPos = rSel.nEndPos;
    }
    else if ( rSel.nStartPara == pE->aSel.nStartPara - 1 && !pE->aSel.HasRange() )
    {   // kein Absatz angehaengt aber leer, nichts tun
    }
    else
    {
        DBG_ERRORFILE( "EntryEnd: EditEngine ESelection End < Start" );
    }
}


void ScHTMLLayoutParser::NextRow( ImportInfo* pInfo )
{
    if ( bInCell )
        CloseEntry( pInfo );
    if ( nRowMax < ++nRowCnt )
        nRowMax = nRowCnt;
    nColCnt = nColCntStart;
    nColOffset = nColOffsetStart;
    bFirstRow = FALSE;
}


BOOL ScHTMLLayoutParser::SeekOffset( ScHTMLColOffset* pOffset, USHORT nOffset,
        SCCOL* pCol, USHORT nOffsetTol )
{
    DBG_ASSERT( pOffset, "ScHTMLLayoutParser::SeekOffset - illegal call" );
    USHORT nPos;
    BOOL bFound = pOffset->Seek_Entry( nOffset, &nPos );
    *pCol = static_cast<SCCOL>(nPos);
    if ( bFound )
        return TRUE;
    USHORT nCount = pOffset->Count();
    if ( !nCount )
        return FALSE;
    // nPos ist Einfuegeposition, da liegt der Naechsthoehere (oder auch nicht)
    if ( nPos < nCount && (((*pOffset)[nPos] - nOffsetTol) <= nOffset) )
        return TRUE;
    // nicht kleiner als alles andere? dann mit Naechstniedrigerem vergleichen
    else if ( nPos && (((*pOffset)[nPos-1] + nOffsetTol) >= nOffset) )
    {
        (*pCol)--;
        return TRUE;
    }
    return FALSE;
}


void ScHTMLLayoutParser::MakeCol( ScHTMLColOffset* pOffset, USHORT& nOffset,
        USHORT& nWidth, USHORT nOffsetTol, USHORT nWidthTol )
{
    DBG_ASSERT( pOffset, "ScHTMLLayoutParser::MakeCol - illegal call" );
    SCCOL nPos;
    if ( SeekOffset( pOffset, nOffset, &nPos, nOffsetTol ) )
        nOffset = (USHORT)(*pOffset)[nPos];
    else
        pOffset->Insert( nOffset );
    if ( nWidth )
    {
        if ( SeekOffset( pOffset, nOffset + nWidth, &nPos, nWidthTol ) )
            nWidth = (USHORT)(*pOffset)[nPos] - nOffset;
        else
            pOffset->Insert( nOffset + nWidth );
    }
}


void ScHTMLLayoutParser::MakeColNoRef( ScHTMLColOffset* pOffset, USHORT nOffset,
        USHORT nWidth, USHORT nOffsetTol, USHORT nWidthTol )
{
    DBG_ASSERT( pOffset, "ScHTMLLayoutParser::MakeColNoRef - illegal call" );
    SCCOL nPos;
    if ( SeekOffset( pOffset, nOffset, &nPos, nOffsetTol ) )
        nOffset = (USHORT)(*pOffset)[nPos];
    else
        pOffset->Insert( nOffset );
    if ( nWidth )
    {
        if ( !SeekOffset( pOffset, nOffset + nWidth, &nPos, nWidthTol ) )
            pOffset->Insert( nOffset + nWidth );
    }
}


void ScHTMLLayoutParser::ModifyOffset( ScHTMLColOffset* pOffset, USHORT& nOldOffset,
            USHORT& nNewOffset, USHORT nOffsetTol )
{
    DBG_ASSERT( pOffset, "ScHTMLLayoutParser::ModifyOffset - illegal call" );
    SCCOL nPos;
    if ( !SeekOffset( pOffset, nOldOffset, &nPos, nOffsetTol ) )
    {
        if ( SeekOffset( pOffset, nNewOffset, &nPos, nOffsetTol ) )
            nNewOffset = (USHORT)(*pOffset)[nPos];
        else
            pOffset->Insert( nNewOffset );
        return ;
    }
    nOldOffset = (USHORT)(*pOffset)[nPos];
    SCCOL nPos2;
    if ( SeekOffset( pOffset, nNewOffset, &nPos2, nOffsetTol ) )
    {
        nNewOffset = (USHORT)(*pOffset)[nPos2];
        return ;
    }
    ULONG* pData = ((ULONG*) pOffset->GetData()) + nPos;        //! QAD
    long nDiff = nNewOffset - nOldOffset;
    if ( nDiff < 0 )
    {
        const ULONG* pStop = pOffset->GetData();
        do
        {
            *pData += nDiff;
        } while ( pStop < pData-- );
    }
    else
    {
        const ULONG* pStop = pOffset->GetData() + pOffset->Count();
        do
        {
            *pData += nDiff;
        } while ( ++pData < pStop );
    }
}


void ScHTMLLayoutParser::SkipLocked( ScEEParseEntry* pE, BOOL bJoin )
{
    if ( ValidCol(pE->nCol) )
    {   // wuerde sonst bei ScAddress falschen Wert erzeugen, evtl. Endlosschleife!
        BOOL bBadCol = FALSE;
        BOOL bAgain;
        ScRange aRange( pE->nCol, pE->nRow, 0,
            pE->nCol + pE->nColOverlap - 1, pE->nRow + pE->nRowOverlap - 1, 0 );
        do
        {
            bAgain = FALSE;
            for ( ScRange* pR = xLockedList->First(); pR; pR = xLockedList->Next() )
            {
                if ( pR->Intersects( aRange ) )
                {
                    pE->nCol = pR->aEnd.Col() + 1;
                    SCCOL nTmp = pE->nCol + pE->nColOverlap - 1;
                    if ( pE->nCol > MAXCOL || nTmp > MAXCOL )
                        bBadCol = TRUE;
                    else
                    {
                        bAgain = TRUE;
                        aRange.aStart.SetCol( pE->nCol );
                        aRange.aEnd.SetCol( nTmp );
                    }
                    break;
                }
            }
        } while ( bAgain );
        if ( bJoin && !bBadCol )
            xLockedList->Join( aRange );
    }
}


void ScHTMLLayoutParser::Adjust()
{
    for ( ScRange* pR = xLockedList->First(); pR; pR = xLockedList->Next() )
        delete pR;
    xLockedList->Clear();
    ScHTMLAdjustStack aStack;
    ScHTMLAdjustStackEntry* pS;
    USHORT nTab = 0;
    SCCOL nLastCol = SCCOL_MAX;
    SCROW nNextRow = 0;
    SCROW nCurRow = 0;
    USHORT nPageWidth = (USHORT) aPageSize.Width();
    Table* pTab = NULL;
    for ( ScEEParseEntry* pE = pList->First(); pE; pE = pList->Next() )
    {
        if ( pE->nTab < nTab )
        {   // Table beendet
            if ( (pS = aStack.Pop()) != 0 )
            {
                nLastCol = pS->nLastCol;
                nNextRow = pS->nNextRow;
                nCurRow = pS->nCurRow;
            }
            delete pS;
            nTab = pE->nTab;
            pTab = (pTables ? (Table*) pTables->Get( nTab ) : NULL);

        }
        SCROW nRow = pE->nRow;
        if ( pE->nCol <= nLastCol )
        {   // naechste Zeile
            if ( pE->nRow < nNextRow )
                pE->nRow = nCurRow = nNextRow;
            else
                nCurRow = nNextRow = pE->nRow;
            SCROW nR;
            if ( pTab && ((nR = (SCROW)(ULONG)pTab->Get( nCurRow )) != 0) )
                nNextRow += nR;
            else
                nNextRow++;
        }
        else
            pE->nRow = nCurRow;
        nLastCol = pE->nCol;    // eingelesene Col
        if ( pE->nTab > nTab )
        {   // neue Table
            aStack.Push( new ScHTMLAdjustStackEntry(
                nLastCol, nNextRow, nCurRow ) );
            nTab = pE->nTab;
            pTab = (pTables ? (Table*) pTables->Get( nTab ) : NULL);
            // neuer Zeilenabstand
            SCROW nR;
            if ( pTab && ((nR = (SCROW)(ULONG)pTab->Get( nCurRow )) != 0) )
                nNextRow = nCurRow + nR;
            else
                nNextRow = nCurRow + 1;
        }
        if ( nTab == 0 )
            pE->nWidth = nPageWidth;
        else
        {   // echte Table, keine Absaetze auf der Wiese
            if ( pTab )
            {
                SCROW nRowSpan = pE->nRowOverlap;
                for ( SCROW j=0; j < nRowSpan; j++ )
                {   // aus merged Zeilen resultierendes RowSpan
                    SCROW nRows = (SCROW)(ULONG)pTab->Get( nRow+j );
                    if ( nRows > 1 )
                    {
                        pE->nRowOverlap += nRows - 1;
                        if ( j == 0 )
                        {   // merged Zeilen verschieben die naechste Zeile
                            SCROW nTmp = nCurRow + nRows;
                            if ( nNextRow < nTmp )
                                nNextRow = nTmp;
                        }
                    }
                }
            }
        }
        // echte Col
        SeekOffset( pColOffset, pE->nOffset, &pE->nCol, nOffsetTolerance );
        SCCOL nColBeforeSkip = pE->nCol;
        SkipLocked( pE, FALSE );
        if ( pE->nCol != nColBeforeSkip )
        {
            SCCOL nCount = (SCCOL)pColOffset->Count();
            if ( nCount <= pE->nCol )
            {
                pE->nOffset = (USHORT) (*pColOffset)[nCount-1];
                MakeCol( pColOffset, pE->nOffset, pE->nWidth, nOffsetTolerance, nOffsetTolerance );
            }
            else
            {
                pE->nOffset = (USHORT) (*pColOffset)[pE->nCol];
            }
        }
        SCCOL nPos;
        if ( pE->nWidth && SeekOffset( pColOffset, pE->nOffset + pE->nWidth, &nPos, nOffsetTolerance ) )
            pE->nColOverlap = (nPos > pE->nCol ? nPos - pE->nCol : 1);
        else
        {
//2do: das muss nicht korrekt sein, ist aber..
            pE->nColOverlap = 1;
        }
        xLockedList->Join( ScRange( pE->nCol, pE->nRow, 0,
            pE->nCol + pE->nColOverlap - 1, pE->nRow + pE->nRowOverlap - 1, 0 ) );
        // MaxDimensions mitfuehren
        SCCOL nColTmp = pE->nCol + pE->nColOverlap;
        if ( nColMax < nColTmp )
            nColMax = nColTmp;
        SCROW nRowTmp = pE->nRow + pE->nRowOverlap;
        if ( nRowMax < nRowTmp )
            nRowMax = nRowTmp;
    }
    while ( (pS = aStack.Pop()) != 0 )
        delete pS;
}


USHORT ScHTMLLayoutParser::GetWidth( ScEEParseEntry* pE )
{
    if ( pE->nWidth )
        return pE->nWidth;
    sal_Int32 nTmp = ::std::min( static_cast<sal_Int32>( pE->nCol -
                nColCntStart + pE->nColOverlap),
            static_cast<sal_Int32>( pLocalColOffset->Count() - 1));
    SCCOL nPos = (nTmp < 0 ? 0 : static_cast<SCCOL>(nTmp));
    USHORT nOff2 = (USHORT) (*pLocalColOffset)[nPos];
    if ( pE->nOffset < nOff2 )
        return nOff2 - pE->nOffset;
    return 0;
}


void ScHTMLLayoutParser::SetWidths()
{
    ScEEParseEntry* pE;
    SCCOL nCol;
    if ( !nTableWidth )
        nTableWidth = (USHORT) aPageSize.Width();
    SCCOL nColsPerRow = nMaxCol - nColCntStart;
    if ( nColsPerRow <= 0 )
        nColsPerRow = 1;
    if ( pLocalColOffset->Count() <= 2 )
    {   // nur PageSize, es gab keine Width-Angabe
        USHORT nWidth = nTableWidth / static_cast<USHORT>(nColsPerRow);
        USHORT nOff = nColOffsetStart;
        pLocalColOffset->Remove( (USHORT)0, pLocalColOffset->Count() );
        for ( nCol = 0; nCol <= nColsPerRow; ++nCol, nOff = nOff + nWidth )
        {
            MakeColNoRef( pLocalColOffset, nOff, 0, 0, 0 );
        }
        nTableWidth = (USHORT)((*pLocalColOffset)[pLocalColOffset->Count() -1 ] - (*pLocalColOffset)[0]);
        pE = pList->Seek( nFirstTableCell );
        while ( pE )
        {
            if ( pE->nTab == nTable )
            {
                pE->nOffset = (USHORT) (*pLocalColOffset)[pE->nCol - nColCntStart];
                pE->nWidth = 0;     // to be recalculated later
            }
            pE = pList->Next();
        }
    }
    else
    {   // einige mit einige ohne Width
        pE = pList->Seek( nFirstTableCell );
        // #36350# wieso eigentlich kein pE ?!?
        if ( pE )
        {
            USHORT* pOffsets = new USHORT[ nColsPerRow+1 ];
            memset( pOffsets, 0, (nColsPerRow+1) * sizeof(USHORT) );
            USHORT* pWidths = new USHORT[ nColsPerRow ];
            memset( pWidths, 0, nColsPerRow * sizeof(USHORT) );
            pOffsets[0] = nColOffsetStart;
            while ( pE )
            {
                if ( pE->nTab == nTable && pE->nWidth )
                {
                    nCol = pE->nCol - nColCntStart;
                    if ( nCol < nColsPerRow )
                    {
                        if ( pE->nColOverlap == 1 )
                        {
                            if ( pWidths[nCol] < pE->nWidth )
                                pWidths[nCol] = pE->nWidth;
                        }
                        else
                        {   // try to find a single undefined width
                            USHORT nTotal = 0;
                            BOOL bFound = FALSE;
                            SCCOL nHere = 0;
                            SCCOL nStop = Min( static_cast<SCCOL>(nCol + pE->nColOverlap), nColsPerRow );
                            for ( ; nCol < nStop; nCol++ )
                            {
                                if ( pWidths[nCol] )
                                    nTotal = nTotal + pWidths[nCol];
                                else
                                {
                                    if ( bFound )
                                    {
                                        bFound = FALSE;
                                        break;  // for
                                    }
                                    bFound = TRUE;
                                    nHere = nCol;
                                }
                            }
                            if ( bFound && pE->nWidth > nTotal )
                                pWidths[nHere] = pE->nWidth - nTotal;
                        }
                    }
                }
                pE = pList->Next();
            }
            USHORT nWidths = 0;
            USHORT nUnknown = 0;
            for ( nCol = 0; nCol < nColsPerRow; nCol++ )
            {
                if ( pWidths[nCol] )
                    nWidths = nWidths + pWidths[nCol];
                else
                    nUnknown++;
            }
            if ( nUnknown )
            {
                USHORT nW = ((nWidths < nTableWidth) ?
                    ((nTableWidth - nWidths) / nUnknown) :
                    (nTableWidth / nUnknown));
                for ( nCol = 0; nCol < nColsPerRow; nCol++ )
                {
                    if ( !pWidths[nCol] )
                        pWidths[nCol] = nW;
                }
            }
            for ( nCol = 1; nCol <= nColsPerRow; nCol++ )
            {
                pOffsets[nCol] = pOffsets[nCol-1] + pWidths[nCol-1];
            }
            pLocalColOffset->Remove( (USHORT)0, pLocalColOffset->Count() );
            for ( nCol = 0; nCol <= nColsPerRow; nCol++ )
            {
                MakeColNoRef( pLocalColOffset, pOffsets[nCol], 0, 0, 0 );
            }
            nTableWidth = pOffsets[nColsPerRow] - pOffsets[0];

            pE = pList->Seek( nFirstTableCell );
            while ( pE )
            {
                if ( pE->nTab == nTable )
                {
                    nCol = pE->nCol - nColCntStart;
                    DBG_ASSERT( nCol < nColsPerRow, "ScHTMLLayoutParser::SetWidths: column overflow" );
                    if ( nCol < nColsPerRow )
                    {
                        pE->nOffset = pOffsets[nCol];
                        nCol = nCol + pE->nColOverlap;
                        if ( nCol > nColsPerRow )
                            nCol = nColsPerRow;
                        pE->nWidth = pOffsets[nCol] - pE->nOffset;
                    }
                }
                pE = pList->Next();
            }

            delete [] pWidths;
            delete [] pOffsets;
        }
    }
    if ( pLocalColOffset->Count() )
    {
        USHORT nMax = (USHORT) (*pLocalColOffset)[pLocalColOffset->Count() - 1];
        if ( aPageSize.Width() < nMax )
            aPageSize.Width() = nMax;
    }
    pE = pList->Seek( nFirstTableCell );
    while ( pE )
    {
        if ( pE->nTab == nTable )
        {
            if ( !pE->nWidth )
            {
                pE->nWidth = GetWidth( pE );
                DBG_ASSERT( pE->nWidth, "SetWidths: pE->nWidth == 0" );
            }
            MakeCol( pColOffset, pE->nOffset, pE->nWidth, nOffsetTolerance, nOffsetTolerance );
        }
        pE = pList->Next();
    }
}


void ScHTMLLayoutParser::Colonize( ScEEParseEntry* pE )
{
    if ( pE->nCol == SCCOL_MAX )
        pE->nCol = nColCnt;
    if ( pE->nRow == SCROW_MAX )
        pE->nRow = nRowCnt;
    SCCOL nCol = pE->nCol;
    SkipLocked( pE );       // Spaltenverdraengung nach rechts

    if ( nCol < pE->nCol )
    {   // verdraengt
        nCol = pE->nCol - nColCntStart;
        SCCOL nCount = static_cast<SCCOL>(pLocalColOffset->Count());
        if ( nCol < nCount )
            nColOffset = (USHORT) (*pLocalColOffset)[nCol];
        else
            nColOffset = (USHORT) (*pLocalColOffset)[nCount - 1];
    }
    pE->nOffset = nColOffset;
    USHORT nWidth = GetWidth( pE );
    MakeCol( pLocalColOffset, pE->nOffset, nWidth, nOffsetTolerance, nOffsetTolerance );
    if ( pE->nWidth )
        pE->nWidth = nWidth;
    nColOffset = pE->nOffset + nWidth;
    if ( nTableWidth < nColOffset - nColOffsetStart )
        nTableWidth = nColOffset - nColOffsetStart;
}


void ScHTMLLayoutParser::CloseEntry( ImportInfo* pInfo )
{
    bInCell = FALSE;
    if ( bTabInTabCell )
    {   // in TableOff vom Stack geholt
        bTabInTabCell = FALSE;
        if ( pList->GetPos( pActEntry ) == LIST_ENTRY_NOTFOUND )
            delete pActEntry;
        NewActEntry( pList->Last() );   // neuer freifliegender pActEntry
        return ;
    }
    if ( pActEntry->nTab == 0 )
        pActEntry->nWidth = (USHORT) aPageSize.Width();
    Colonize( pActEntry );
    nColCnt = pActEntry->nCol + pActEntry->nColOverlap;
    if ( nMaxCol < nColCnt )
        nMaxCol = nColCnt;          // TableStack MaxCol
    if ( nColMax < nColCnt )
        nColMax = nColCnt;      // globales MaxCol fuer ScEEParser GetDimensions!
    EntryEnd( pActEntry, pInfo->aSelection );
    ESelection& rSel = pActEntry->aSel;
    while ( rSel.nStartPara < rSel.nEndPara
            && pEdit->GetTextLen( rSel.nStartPara ) == 0 )
    {   // vorgehaengte Leerabsaetze strippen
        rSel.nStartPara++;
    }
    while ( rSel.nEndPos == 0 && rSel.nEndPara > rSel.nStartPara )
    {   // angehaengte Leerabsaetze strippen
        rSel.nEndPara--;
        rSel.nEndPos = pEdit->GetTextLen( rSel.nEndPara );
    }
    if ( rSel.nStartPara > rSel.nEndPara )
    {   // gibt GPF in CreateTextObject
        DBG_ERRORFILE( "CloseEntry: EditEngine ESelection Start > End" );
        rSel.nEndPara = rSel.nStartPara;
    }
    if ( rSel.HasRange() )
        pActEntry->aItemSet.Put( SfxBoolItem( ATTR_LINEBREAK, TRUE ) );
    pList->Insert( pActEntry, LIST_APPEND );
    NewActEntry( pActEntry );   // neuer freifliegender pActEntry
}


IMPL_LINK( ScHTMLLayoutParser, HTMLImportHdl, ImportInfo*, pInfo )
{
#if defined(erDEBUG) //|| 1
    static ESelection aDebugSel;
    static String aDebugStr;
    static SvFileStream* pDebugStrm = NULL;
    static ULONG nDebugStrmPos = 0;
    static ULONG nDebugCount = 0;
    static ULONG nDebugCountAll = 0;
    static const sal_Char* sDebugState[15] = {
                    "RTFIMP_START", "RTFIMP_END",
                    "RTFIMP_NEXTTOKEN", "RTFIMP_UNKNOWNATTR",
                    "RTFIMP_SETATTR",
                    "RTFIMP_INSERTTEXT",
                    "RTFIMP_INSERTPARA",
                    "HTMLIMP_START", "HTMLIMP_END",
                    "HTMLIMP_NEXTTOKEN", "HTMLIMP_UNKNOWNATTR",
                    "HTMLIMP_SETATTR",
                    "HTMLIMP_INSERTTEXT",
                    "HTMLIMP_INSERTPARA", "HTMLIMP_INSERTFIELD"
    };

    nDebugCountAll++;
    if ( pInfo->eState != HTMLIMP_NEXTTOKEN     // not too much
      || pInfo->nToken == HTML_TABLE_ON
      || pInfo->nToken == HTML_TABLE_OFF
      || pInfo->nToken == HTML_TABLEROW_ON
      || pInfo->nToken == HTML_TABLEROW_OFF
      || pInfo->nToken == HTML_TABLEHEADER_ON
      || pInfo->nToken == HTML_TABLEHEADER_OFF
      || pInfo->nToken == HTML_TABLEDATA_ON
      || pInfo->nToken == HTML_TABLEDATA_OFF
      || !aDebugSel.IsEqual( pInfo->aSelection )
      || pInfo->aText.Len() || aDebugStr != pInfo->aText
        )
    {
        aDebugSel = pInfo->aSelection;
        aDebugStr = pInfo->aText;
        nDebugCount++;
        if ( !pDebugStrm )
        {
            pDebugStrm = new SvFileStream( "d:\\erdbghtm.log",
                STREAM_WRITE | STREAM_TRUNC );
        }
        else
        {
            pDebugStrm->ReOpen();
            pDebugStrm->Seek( nDebugStrmPos );
        }
        SvFileStream& rS = *pDebugStrm;
        rS.WriteNumber( nDebugCountAll ); rS << ".: ";
        rS.WriteNumber( nDebugCount ); rS << ". State: ";
        rS.WriteNumber( (USHORT) pInfo->eState );
        rS << ' ' << sDebugState[pInfo->eState] << endl;
        rS << "SPar,SPos EPar,EPos: ";
        rS.WriteNumber( aDebugSel.nStartPara ); rS << ',';
        rS.WriteNumber( aDebugSel.nStartPos ); rS << ' ';
        rS.WriteNumber( aDebugSel.nEndPara ); rS << ',';
        rS.WriteNumber( aDebugSel.nEndPos ); rS << endl;
        if ( aDebugStr.Len() )
        {
            rS << "Text: \"" << aDebugStr << '\"' << endl;
        }
        else
        {
            rS << "Text:" << endl;
        }
        rS << "Token: "; rS.WriteNumber( pInfo->nToken );
        switch ( pInfo->nToken )
        {
            case HTML_TABLE_ON:
                rS << " HTML_TABLE_ON";
                break;
            case HTML_TABLE_OFF:
                rS << " HTML_TABLE_OFF";
                break;
            case HTML_TABLEROW_ON:
                rS << " HTML_TABLEROW_ON";
                break;
            case HTML_TABLEROW_OFF:
                rS << " HTML_TABLEROW_OFF";
                break;
            case HTML_TABLEHEADER_ON:
                rS << " HTML_TABLEHEADER_ON";
                break;
            case HTML_TABLEHEADER_OFF:
                rS << " HTML_TABLEHEADER_OFF";
                break;
            case HTML_TABLEDATA_ON:
                rS << " HTML_TABLEDATA_ON";
                break;
            case HTML_TABLEDATA_OFF:
                rS << " HTML_TABLEDATA_OFF";
                break;
        }
        rS << " Value: "; rS.WriteNumber( pInfo->nTokenValue );
        rS << endl << endl;
        nDebugStrmPos = pDebugStrm->Tell();
        pDebugStrm->Close();
    }
#endif
    switch ( pInfo->eState )
    {
        case HTMLIMP_NEXTTOKEN:
            ProcToken( pInfo );
            break;
        case HTMLIMP_UNKNOWNATTR:
            ProcToken( pInfo );
            break;
        case HTMLIMP_START:
            break;
        case HTMLIMP_END:
            if ( pInfo->aSelection.nEndPos )
            {
                // If text remains: create paragraph, without calling CloseEntry().
                if( bInCell )   // #108269# ...but only in opened table cells.
                {
                    bInCell = FALSE;
                    NextRow( pInfo );
                    bInCell = TRUE;
                }
                CloseEntry( pInfo );
            }
            while ( nTableLevel > 0 )
                TableOff( pInfo );      // close tables, if </TABLE> missing
            break;
        case HTMLIMP_SETATTR:
            break;
        case HTMLIMP_INSERTTEXT:
            break;
        case HTMLIMP_INSERTPARA:
            if ( nTableLevel < 1 )
            {
                CloseEntry( pInfo );
                NextRow( pInfo );
            }
            break;
        case HTMLIMP_INSERTFIELD:
            break;
        default:
            DBG_ERRORFILE("HTMLImportHdl: unknown ImportInfo.eState");
    }
    return 0;
}


// Groesster Gemeinsamer Teiler nach Euklid (Kettendivision)
// Sonderfall: 0 und irgendwas geben 1
SCROW lcl_GGT( SCROW a, SCROW b )
{
    if ( !a || !b )
        return 1;
    do
    {
        if ( a > b )
            a -= SCROW(a / b) * b;
        else
            b -= SCROW(b / a) * a;
    } while ( a && b );
    return ((a != 0) ? a : b);
}


// Kleinstes Gemeinsames Vielfaches: a * b / GGT(a,b)
SCROW lcl_KGV( SCROW a, SCROW b )
{
    if ( a > b )    // Ueberlauf unwahrscheinlicher machen
        return (a / lcl_GGT(a,b)) * b;
    else
        return (b / lcl_GGT(a,b)) * a;
}


void ScHTMLLayoutParser::TableDataOn( ImportInfo* pInfo )
{
    if ( bInCell )
        CloseEntry( pInfo );
    if ( !nTableLevel )
    {
        DBG_ERROR( "Dummbatz-Dok! <TH> oder <TD> ohne vorheriges <TABLE>" );
        TableOn( pInfo );
    }
    bInCell = TRUE;
    BOOL bHorJustifyCenterTH = (pInfo->nToken == HTML_TABLEHEADER_ON);
    const HTMLOptions* pOptions = ((HTMLParser*)pInfo->pParser)->GetOptions();
    USHORT nArrLen = pOptions->Count();
    for ( USHORT i = 0; i < nArrLen; i++ )
    {
        const HTMLOption* pOption = (*pOptions)[i];
        switch( pOption->GetToken() )
        {
            case HTML_O_COLSPAN:
            {
                pActEntry->nColOverlap = ( SCCOL ) pOption->GetString().ToInt32();
            }
            break;
            case HTML_O_ROWSPAN:
            {
                pActEntry->nRowOverlap = ( SCROW ) pOption->GetString().ToInt32();
            }
            break;
            case HTML_O_ALIGN:
            {
                bHorJustifyCenterTH = FALSE;
                SvxCellHorJustify eVal;
                const String& rOptVal = pOption->GetString();
                if ( rOptVal.CompareIgnoreCaseToAscii( OOO_STRING_SVTOOLS_HTML_AL_right ) == COMPARE_EQUAL )
                    eVal = SVX_HOR_JUSTIFY_RIGHT;
                else if ( rOptVal.CompareIgnoreCaseToAscii( OOO_STRING_SVTOOLS_HTML_AL_center ) == COMPARE_EQUAL )
                    eVal = SVX_HOR_JUSTIFY_CENTER;
                else if ( rOptVal.CompareIgnoreCaseToAscii( OOO_STRING_SVTOOLS_HTML_AL_left ) == COMPARE_EQUAL )
                    eVal = SVX_HOR_JUSTIFY_LEFT;
                else
                    eVal = SVX_HOR_JUSTIFY_STANDARD;
                if ( eVal != SVX_HOR_JUSTIFY_STANDARD )
                    pActEntry->aItemSet.Put( SvxHorJustifyItem( eVal, ATTR_HOR_JUSTIFY) );
            }
            break;
            case HTML_O_VALIGN:
            {
                SvxCellVerJustify eVal;
                const String& rOptVal = pOption->GetString();
                if ( rOptVal.CompareIgnoreCaseToAscii( OOO_STRING_SVTOOLS_HTML_VA_top ) == COMPARE_EQUAL )
                    eVal = SVX_VER_JUSTIFY_TOP;
                else if ( rOptVal.CompareIgnoreCaseToAscii( OOO_STRING_SVTOOLS_HTML_VA_middle ) == COMPARE_EQUAL )
                    eVal = SVX_VER_JUSTIFY_CENTER;
                else if ( rOptVal.CompareIgnoreCaseToAscii( OOO_STRING_SVTOOLS_HTML_VA_bottom ) == COMPARE_EQUAL )
                    eVal = SVX_VER_JUSTIFY_BOTTOM;
                else
                    eVal = SVX_VER_JUSTIFY_STANDARD;
                pActEntry->aItemSet.Put( SvxVerJustifyItem( eVal, ATTR_VER_JUSTIFY) );
            }
            break;
            case HTML_O_WIDTH:
            {
                pActEntry->nWidth = GetWidthPixel( pOption );
            }
            break;
            case HTML_O_BGCOLOR:
            {
                Color aColor;
                pOption->GetColor( aColor );
                pActEntry->aItemSet.Put(
                    SvxBrushItem( aColor, ATTR_BACKGROUND ) );
            }
            break;
            case HTML_O_SDVAL:
            {
                pActEntry->pValStr = new String( pOption->GetString() );
            }
            break;
            case HTML_O_SDNUM:
            {
                pActEntry->pNumStr = new String( pOption->GetString() );
            }
            break;
        }
    }
    pActEntry->nCol = nColCnt;
    pActEntry->nRow = nRowCnt;
    pActEntry->nTab = nTable;

    if ( bHorJustifyCenterTH )
        pActEntry->aItemSet.Put(
            SvxHorJustifyItem( SVX_HOR_JUSTIFY_CENTER, ATTR_HOR_JUSTIFY) );
}


void ScHTMLLayoutParser::TableRowOn( ImportInfo* pInfo )
{
    if ( nColCnt > nColCntStart )
        NextRow( pInfo );       // das optionale TableRowOff war nicht
    nColOffset = nColOffsetStart;
}


void ScHTMLLayoutParser::TableRowOff( ImportInfo* pInfo )
{
    NextRow( pInfo );
}


void ScHTMLLayoutParser::TableDataOff( ImportInfo* pInfo )
{
    if ( bInCell )
        CloseEntry( pInfo );    // aber nur wenn's auch eine war
}


void ScHTMLLayoutParser::TableOn( ImportInfo* pInfo )
{
    String aTabName;
    bool bBorderOn = false;

    if ( ++nTableLevel > 1 )
    {   // Table in Table
        USHORT nTmpColOffset = nColOffset;  // wird in Colonize noch angepasst
        Colonize( pActEntry );
        aTableStack.Push( new ScHTMLTableStackEntry(
            pActEntry, xLockedList, pLocalColOffset, nFirstTableCell,
            nColCnt, nRowCnt, nColCntStart, nMaxCol, nTable,
            nTableWidth, nColOffset, nColOffsetStart,
            bFirstRow ) );
        USHORT nLastWidth = nTableWidth;
        nTableWidth = GetWidth( pActEntry );
        if ( nTableWidth == nLastWidth && nMaxCol - nColCntStart > 1 )
        {   // es muss mehr als einen geben, also kann dieser nicht alles sein
            nTableWidth = nLastWidth / static_cast<USHORT>((nMaxCol - nColCntStart));
        }
        nLastWidth = nTableWidth;
        if ( pInfo->nToken == HTML_TABLE_ON )
        {   // es kann auch TD oder TH sein, wenn es vorher kein TABLE gab
            const HTMLOptions* pOptions = ((HTMLParser*)pInfo->pParser)->GetOptions();
            USHORT nArrLen = pOptions->Count();
            for ( USHORT i = 0; i < nArrLen; i++ )
            {
                const HTMLOption* pOption = (*pOptions)[i];
                switch( pOption->GetToken() )
                {
                    case HTML_O_WIDTH:
                    {   // Prozent: von Dokumentbreite bzw. aeusserer Zelle
                        nTableWidth = GetWidthPixel( pOption );
                    }
                    break;
                    case HTML_O_BORDER:
                        bBorderOn = ((pOption->GetString().Len() == 0) || (pOption->GetNumber() != 0));
                    break;
                    case HTML_O_ID:
                        aTabName.Assign( pOption->GetString() );
                    break;
                }
            }
        }
        bInCell = FALSE;
        if ( bTabInTabCell && !(nTableWidth < nLastWidth) )
        {   // mehrere Tabellen in einer Zelle, untereinander
            bTabInTabCell = FALSE;
            NextRow( pInfo );
        }
        else
        {   // in dieser Zelle geht's los, oder nebeneinander
            bTabInTabCell = FALSE;
            nColCntStart = nColCnt;
            nColOffset = nTmpColOffset;
            nColOffsetStart = nColOffset;
        }

        ScEEParseEntry* pE = pList->Last();
        NewActEntry( pE );      // neuer freifliegender pActEntry
        xLockedList = new ScRangeList;
    }
    else
    {   // einfache Table auf Dokumentebene
        EntryEnd( pActEntry, pInfo->aSelection );
        if ( pActEntry->aSel.HasRange() )
        {   // noch fliegender Text
            CloseEntry( pInfo );
            NextRow( pInfo );
        }
        aTableStack.Push( new ScHTMLTableStackEntry(
            pActEntry, xLockedList, pLocalColOffset, nFirstTableCell,
            nColCnt, nRowCnt, nColCntStart, nMaxCol, nTable,
            nTableWidth, nColOffset, nColOffsetStart,
            bFirstRow ) );
        // As soon as we have multiple tables we need to be tolerant with the offsets.
        if (nMaxTable > 0)
            nOffsetTolerance = SC_HTML_OFFSET_TOLERANCE_LARGE;
        nTableWidth = 0;
        if ( pInfo->nToken == HTML_TABLE_ON )
        {   // es kann auch TD oder TH sein, wenn es vorher kein TABLE gab
            const HTMLOptions* pOptions = ((HTMLParser*)pInfo->pParser)->GetOptions();
            USHORT nArrLen = pOptions->Count();
            for ( USHORT i = 0; i < nArrLen; i++ )
            {
                const HTMLOption* pOption = (*pOptions)[i];
                switch( pOption->GetToken() )
                {
                    case HTML_O_WIDTH:
                    {   // Prozent: von Dokumentbreite bzw. aeusserer Zelle
                        nTableWidth = GetWidthPixel( pOption );
                    }
                    break;
                    case HTML_O_BORDER:
                        bBorderOn = ((pOption->GetString().Len() == 0) || (pOption->GetNumber() != 0));
                    break;
                    case HTML_O_ID:
                        aTabName.Assign( pOption->GetString() );
                    break;
                }
            }
        }
    }
    nTable = ++nMaxTable;
    bFirstRow = TRUE;
    nFirstTableCell = pList->Count();

    pLocalColOffset = new ScHTMLColOffset;
    MakeColNoRef( pLocalColOffset, nColOffsetStart, 0, 0, 0 );
}


void ScHTMLLayoutParser::TableOff( ImportInfo* pInfo )
{
    if ( bInCell )
        CloseEntry( pInfo );
    if ( nColCnt > nColCntStart )
        TableRowOff( pInfo );      // das optionale TableRowOff war nicht
    if ( !nTableLevel )
    {
        DBG_ERROR( "Dummbatz-Dok! </TABLE> ohne oeffnendes <TABLE>" );
        return ;
    }
    if ( --nTableLevel > 0 )
    {   // Table in Table beendet
        ScHTMLTableStackEntry* pS = aTableStack.Pop();
        if ( pS )
        {
            ScEEParseEntry* pE = pS->pCellEntry;
            SCROW nRows = nRowCnt - pS->nRowCnt;
            if ( nRows > 1 )
            {   // Groesse der Tabelle an dieser Position eintragen
                SCROW nRow = pS->nRowCnt;
                USHORT nTab = pS->nTable;
                if ( !pTables )
                    pTables = new Table;
                // Hoehen der aeusseren Table
                Table* pTab1 = (Table*) pTables->Get( nTab );
                if ( !pTab1 )
                {
                    pTab1 = new Table;
                    pTables->Insert( nTab, pTab1 );
                }
                SCROW nRowSpan = pE->nRowOverlap;
                SCROW nRowKGV;
                SCROW nRowsPerRow1;    // aeussere Table
                SCROW nRowsPerRow2;    // innere Table
                if ( nRowSpan > 1 )
                {   // KGV auf das sich aussere und innere Zeilen
                    // abbilden lassen
                    nRowKGV = lcl_KGV( nRowSpan, nRows );
                    nRowsPerRow1 = nRowKGV / nRowSpan;
                    nRowsPerRow2 = nRowKGV / nRows;
                }
                else
                {
                    nRowKGV = nRowsPerRow1 = nRows;
                    nRowsPerRow2 = 1;
                }
                Table* pTab2 = NULL;
                if ( nRowsPerRow2 > 1 )
                {   // Hoehen der inneren Table
                    pTab2 = new Table;
                    pTables->Insert( nTable, pTab2 );
                }
                // void* Data-Entry der Table-Class fuer das
                // Hoehen-Mapping missbrauchen
                if ( nRowKGV > 1 )
                {
                    if ( nRowsPerRow1 > 1 )
                    {   // aussen
                        for ( SCROW j=0; j < nRowSpan; j++ )
                        {
                            ULONG nRowKey = nRow + j;
                            SCROW nR = (SCROW)(ULONG)pTab1->Get( nRowKey );
                            if ( !nR )
                                pTab1->Insert( nRowKey, (void*) nRowsPerRow1 );
                            else if ( nRowsPerRow1 > nR )
                                pTab1->Replace( nRowKey, (void*) nRowsPerRow1 );
                                //2do: wie geht das noch besser?
                            else if ( nRowsPerRow1 < nR && nRowSpan == 1
                              && nTable == nMaxTable )
                            {   // Platz uebrig, evtl. besser mergen
                                SCROW nAdd = nRowsPerRow1 - (nR % nRowsPerRow1);
                                nR += nAdd;
                                if ( (nR % nRows) == 0 )
                                {   // nur wenn abbildbar
                                    SCROW nR2 = (SCROW)(ULONG)pTab1->Get( nRowKey+1 );
                                    if ( nR2 > nAdd )
                                    {   // nur wenn wirklich Platz
                                        pTab1->Replace( nRowKey, (void*) nR );
                                        pTab1->Replace( nRowKey+1, (void*) (nR2 - nAdd) );
                                        nRowsPerRow2 = nR / nRows;
                                    }
                                }
                            }
                        }
                    }
                    if ( nRowsPerRow2 > 1 )
                    {   // innen
                        if ( !pTab2 )
                        {   // nRowsPerRow2 kann erhoeht worden sein
                            pTab2 = new Table;
                            pTables->Insert( nTable, pTab2 );
                        }
                        for ( SCROW j=0; j < nRows; j++ )
                        {
                            ULONG nRowKey = nRow + j;
                            SCROW nR = (SCROW)(ULONG)pTab2->Get( nRowKey );
                            if ( !nR )
                                pTab2->Insert( nRowKey, (void*) nRowsPerRow2 );
                            else if ( nRowsPerRow2 > nR )
                                pTab2->Replace( nRowKey, (void*) nRowsPerRow2 );
                        }
                    }
                }
            }

            SetWidths();

            if ( !pE->nWidth )
                pE->nWidth = nTableWidth;
            else if ( pE->nWidth < nTableWidth )
            {
                USHORT nOldOffset = pE->nOffset + pE->nWidth;
                USHORT nNewOffset = pE->nOffset + nTableWidth;
                ModifyOffset( pS->pLocalColOffset, nOldOffset, nNewOffset, nOffsetTolerance );
                USHORT nTmp = nNewOffset - pE->nOffset - pE->nWidth;
                pE->nWidth = nNewOffset - pE->nOffset;
                pS->nTableWidth = pS->nTableWidth + nTmp;
                if ( pS->nColOffset >= nOldOffset )
                    pS->nColOffset = pS->nColOffset + nTmp;
            }

            nColCnt = pE->nCol + pE->nColOverlap;
            nRowCnt = pS->nRowCnt;
            nColCntStart = pS->nColCntStart;
            nMaxCol = pS->nMaxCol;
            nTable = pS->nTable;
            nTableWidth = pS->nTableWidth;
            nFirstTableCell = pS->nFirstTableCell;
            nColOffset = pS->nColOffset;
            nColOffsetStart = pS->nColOffsetStart;
            bFirstRow = pS->bFirstRow;
            xLockedList = pS->xLockedList;
            if ( pLocalColOffset )
                delete pLocalColOffset;
            pLocalColOffset = pS->pLocalColOffset;
            delete pActEntry;
            // pActEntry bleibt erstmal erhalten falls da noch 'ne Table in
            // der gleichen Zelle aufgemacht werden sollte (in HTML ist ja
            // alles moeglich..) und wird in CloseEntry deleted
            pActEntry = pE;
            delete pS;
        }
        bTabInTabCell = TRUE;
        bInCell = TRUE;
    }
    else
    {   // einfache Table beendet
        SetWidths();
        ScHTMLTableStackEntry* pS = aTableStack.Pop();
        nMaxCol = 0;
        nTable = 0;
        if ( pS )
        {
            if ( pLocalColOffset )
                delete pLocalColOffset;
            pLocalColOffset = pS->pLocalColOffset;
            delete pS;
        }
    }
}


void ScHTMLLayoutParser::Image( ImportInfo* pInfo )
{
    if ( !pActEntry->pImageList )
        pActEntry->pImageList = new ScHTMLImageList;
    ScHTMLImageList* pIL = pActEntry->pImageList;
    ScHTMLImage* pImage = new ScHTMLImage;
    pIL->Insert( pImage, LIST_APPEND );
    const HTMLOptions* pOptions = ((HTMLParser*)pInfo->pParser)->GetOptions();
    USHORT nArrLen = pOptions->Count();
    for ( USHORT i = 0; i < nArrLen; i++ )
    {
        const HTMLOption* pOption = (*pOptions)[i];
        switch( pOption->GetToken() )
        {
            case HTML_O_SRC:
            {
                pImage->aURL = INetURLObject::GetAbsURL( aBaseURL, pOption->GetString() );
            }
            break;
            case HTML_O_ALT:
            {
                if ( !pActEntry->bHasGraphic )
                {   // ALT text only if not any image loaded
                    if ( pActEntry->aAltText.Len() )
                        pActEntry->aAltText.AppendAscii( "; " );
                    pActEntry->aAltText += pOption->GetString();
                }
            }
            break;
            case HTML_O_WIDTH:
            {
                pImage->aSize.Width() = (long)pOption->GetNumber();
            }
            break;
            case HTML_O_HEIGHT:
            {
                pImage->aSize.Height() = (long)pOption->GetNumber();
            }
            break;
            case HTML_O_HSPACE:
            {
                pImage->aSpace.X() = (long)pOption->GetNumber();
            }
            break;
            case HTML_O_VSPACE:
            {
                pImage->aSpace.Y() = (long)pOption->GetNumber();
            }
            break;
        }
    }
    if ( !pImage->aURL.Len() )
    {
        DBG_ERRORFILE( "Image: Grafik ohne URL ?!?" );
        return ;
    }

    USHORT nFormat;
    Graphic* pGraphic = new Graphic;
    GraphicFilter* pFilter = ::GetGrfFilter();
    if ( GRFILTER_OK != ::LoadGraphic( pImage->aURL, pImage->aFilterName,
            *pGraphic, pFilter, &nFormat ) )
    {
        delete pGraphic;
        return ;        // dumm gelaufen
    }
    if ( !pActEntry->bHasGraphic )
    {   // discard any ALT text in this cell if we have any image
        pActEntry->bHasGraphic = TRUE;
        pActEntry->aAltText.Erase();
    }
    pImage->aFilterName = pFilter->GetImportFormatName( nFormat );
    pImage->pGraphic = pGraphic;
    if ( !(pImage->aSize.Width() && pImage->aSize.Height()) )
    {
        OutputDevice* pDefaultDev = Application::GetDefaultDevice();
        pImage->aSize = pDefaultDev->LogicToPixel( pGraphic->GetPrefSize(),
            pGraphic->GetPrefMapMode() );
    }
    if ( pIL->Count() > 0 )
    {
        long nWidth = 0;
        for ( ScHTMLImage* pI = pIL->First(); pI; pI = pIL->Next() )
        {
            if ( pI->nDir & nHorizontal )
                nWidth += pI->aSize.Width() + 2 * pI->aSpace.X();
            else
                nWidth = 0;
        }
        if ( pActEntry->nWidth
          && (nWidth + pImage->aSize.Width() + 2 * pImage->aSpace.X()
                >= pActEntry->nWidth) )
            pIL->Last()->nDir = nVertical;
    }
}


void ScHTMLLayoutParser::ColOn( ImportInfo* pInfo )
{
    const HTMLOptions* pOptions = ((HTMLParser*)pInfo->pParser)->GetOptions();
    USHORT nArrLen = pOptions->Count();
    for ( USHORT i = 0; i < nArrLen; i++ )
    {
        const HTMLOption* pOption = (*pOptions)[i];
        switch( pOption->GetToken() )
        {
            case HTML_O_WIDTH:
            {
                USHORT nVal = GetWidthPixel( pOption );
                MakeCol( pLocalColOffset, nColOffset, nVal, 0, 0 );
                nColOffset = nColOffset + nVal;
            }
            break;
        }
    }
}


USHORT ScHTMLLayoutParser::GetWidthPixel( const HTMLOption* pOption )
{
    const String& rOptVal = pOption->GetString();
    if ( rOptVal.Search('%') != STRING_NOTFOUND )
    {   // Prozent
        USHORT nW = (nTableWidth ? nTableWidth : (USHORT) aPageSize.Width());
        return (USHORT)((pOption->GetNumber() * nW) / 100);
    }
    else
    {
        if ( rOptVal.Search('*') != STRING_NOTFOUND )
        {   // relativ zu was?!?
//2do: ColArray aller relativen Werte sammeln und dann MakeCol
            return 0;
        }
        else
            return (USHORT)pOption->GetNumber();    // Pixel
    }
}


void ScHTMLLayoutParser::AnchorOn( ImportInfo* pInfo )
{
    const HTMLOptions* pOptions = ((HTMLParser*)pInfo->pParser)->GetOptions();
    USHORT nArrLen = pOptions->Count();
    for ( USHORT i = 0; i < nArrLen; i++ )
    {
        const HTMLOption* pOption = (*pOptions)[i];
        switch( pOption->GetToken() )
        {
            case HTML_O_NAME:
            {
                pActEntry->pName = new String( pOption->GetString() );
            }
            break;
        }
    }
}


BOOL ScHTMLLayoutParser::IsAtBeginningOfText( ImportInfo* pInfo )
{
    ESelection& rSel = pActEntry->aSel;
    return rSel.nStartPara == rSel.nEndPara &&
        rSel.nStartPara <= pInfo->aSelection.nEndPara &&
        pEdit->GetTextLen( rSel.nStartPara ) == 0;
}


void ScHTMLLayoutParser::FontOn( ImportInfo* pInfo )
{
    if ( IsAtBeginningOfText( pInfo ) )
    {   // nur am Anfang des Textes, gilt dann fuer gesamte Zelle
        const HTMLOptions* pOptions = ((HTMLParser*)pInfo->pParser)->GetOptions();
        USHORT nArrLen = pOptions->Count();
        for ( USHORT i = 0; i < nArrLen; i++ )
        {
            const HTMLOption* pOption = (*pOptions)[i];
            switch( pOption->GetToken() )
            {
                case HTML_O_FACE :
                {
                    const String& rFace = pOption->GetString();
                    String aFontName;
                    xub_StrLen nPos = 0;
                    while( nPos != STRING_NOTFOUND )
                    {   // Fontliste, VCL: Semikolon als Separator, HTML: Komma
                        String aFName = rFace.GetToken( 0, ',', nPos );
                        aFName.EraseTrailingChars().EraseLeadingChars();
                        if( aFontName.Len() )
                            aFontName += ';';
                        aFontName += aFName;
                    }
                    if ( aFontName.Len() )
                        pActEntry->aItemSet.Put( SvxFontItem( FAMILY_DONTKNOW,
                            aFontName, EMPTY_STRING, PITCH_DONTKNOW,
                            RTL_TEXTENCODING_DONTKNOW, ATTR_FONT ) );
                }
                break;
                case HTML_O_SIZE :
                {
                    USHORT nSize = (USHORT) pOption->GetNumber();
                    if ( nSize == 0 )
                        nSize = 1;
                    else if ( nSize > SC_HTML_FONTSIZES )
                        nSize = SC_HTML_FONTSIZES;
                    pActEntry->aItemSet.Put( SvxFontHeightItem(
                        maFontHeights[nSize-1], 100, ATTR_FONT_HEIGHT ) );
                }
                break;
                case HTML_O_COLOR :
                {
                    Color aColor;
                    pOption->GetColor( aColor );
                    pActEntry->aItemSet.Put( SvxColorItem( aColor, ATTR_FONT_COLOR ) );
                }
                break;
            }
        }
    }
}


void ScHTMLLayoutParser::ProcToken( ImportInfo* pInfo )
{
    BOOL bSetLastToken = TRUE;
    switch ( pInfo->nToken )
    {
        case HTML_META:
        {
            HTMLParser* pParser = (HTMLParser*) pInfo->pParser;
            uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                mpDoc->GetDocumentShell()->GetModel(), uno::UNO_QUERY_THROW);
            pParser->ParseMetaOptions(
                xDPS->getDocumentProperties(),
                mpDoc->GetDocumentShell()->GetHeaderAttributes() );
        }
        break;
        case HTML_TITLE_ON:
        {
            bInTitle = TRUE;
            aString.Erase();
        }
        break;
        case HTML_TITLE_OFF:
        {
            if ( bInTitle && aString.Len() )
            {
                // Leerzeichen von Zeilenumbruechen raus
                aString.EraseLeadingChars();
                aString.EraseTrailingChars();
                uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                    mpDoc->GetDocumentShell()->GetModel(),
                    uno::UNO_QUERY_THROW);
                xDPS->getDocumentProperties()->setTitle(aString);
            }
            bInTitle = FALSE;
        }
        break;
        case HTML_TABLE_ON:
        {
            TableOn( pInfo );
        }
        break;
        case HTML_COL_ON:
        {
            ColOn( pInfo );
        }
        break;
        case HTML_TABLEHEADER_ON:       // oeffnet Zelle
        {
            if ( bInCell )
                CloseEntry( pInfo );
            // bInCell nicht TRUE setzen, das macht TableDataOn
            pActEntry->aItemSet.Put(
                SvxWeightItem( WEIGHT_BOLD, ATTR_FONT_WEIGHT) );
        }   // fall thru
        case HTML_TABLEDATA_ON:         // oeffnet Zelle
        {
            TableDataOn( pInfo );
        }
        break;
        case HTML_TABLEHEADER_OFF:
        case HTML_TABLEDATA_OFF:        // schliesst Zelle
        {
            TableDataOff( pInfo );
        }
        break;
        case HTML_TABLEROW_ON:          // vor erster Zelle in Row
        {
            TableRowOn( pInfo );
        }
        break;
        case HTML_TABLEROW_OFF:         // nach letzter Zelle in Row
        {
            TableRowOff( pInfo );
        }
        break;
        case HTML_TABLE_OFF:
        {
            TableOff( pInfo );
        }
        break;
        case HTML_IMAGE:
        {
            Image( pInfo );
        }
        break;
        case HTML_PARABREAK_OFF:
        {   // nach einem Image geht es vertikal weiter
            if ( pActEntry->pImageList && pActEntry->pImageList->Count() > 0 )
                pActEntry->pImageList->Last()->nDir = nVertical;
        }
        break;
        case HTML_ANCHOR_ON:
        {
            AnchorOn( pInfo );
        }
        break;
        case HTML_FONT_ON :
        {
            FontOn( pInfo );
        }
        break;
        case HTML_BIGPRINT_ON :
        {
//2do: aktuelle Fontgroesse merken und einen groesser
            if ( IsAtBeginningOfText( pInfo ) )
                pActEntry->aItemSet.Put( SvxFontHeightItem(
                    maFontHeights[3], 100, ATTR_FONT_HEIGHT ) );
        }
        break;
        case HTML_SMALLPRINT_ON :
        {
//2do: aktuelle Fontgroesse merken und einen kleiner
            if ( IsAtBeginningOfText( pInfo ) )
                pActEntry->aItemSet.Put( SvxFontHeightItem(
                    maFontHeights[0], 100, ATTR_FONT_HEIGHT ) );
        }
        break;
        case HTML_BOLD_ON :
        case HTML_STRONG_ON :
        {
            if ( IsAtBeginningOfText( pInfo ) )
                pActEntry->aItemSet.Put( SvxWeightItem( WEIGHT_BOLD,
                    ATTR_FONT_WEIGHT ) );
        }
        break;
        case HTML_ITALIC_ON :
        case HTML_EMPHASIS_ON :
        case HTML_ADDRESS_ON :
        case HTML_BLOCKQUOTE_ON :
        case HTML_BLOCKQUOTE30_ON :
        case HTML_CITIATION_ON :
        case HTML_VARIABLE_ON :
        {
            if ( IsAtBeginningOfText( pInfo ) )
                pActEntry->aItemSet.Put( SvxPostureItem( ITALIC_NORMAL,
                    ATTR_FONT_POSTURE ) );
        }
        break;
        case HTML_DEFINSTANCE_ON :
        {
            if ( IsAtBeginningOfText( pInfo ) )
            {
                pActEntry->aItemSet.Put( SvxWeightItem( WEIGHT_BOLD,
                    ATTR_FONT_WEIGHT ) );
                pActEntry->aItemSet.Put( SvxPostureItem( ITALIC_NORMAL,
                    ATTR_FONT_POSTURE ) );
            }
        }
        break;
        case HTML_UNDERLINE_ON :
        {
            if ( IsAtBeginningOfText( pInfo ) )
                pActEntry->aItemSet.Put( SvxUnderlineItem( UNDERLINE_SINGLE,
                    ATTR_FONT_UNDERLINE ) );
        }
        break;
        case HTML_TEXTTOKEN:
        {
            if ( bInTitle )
                aString += pInfo->aText;
        }
        break;
        default:
        {   // nLastToken nicht setzen!
            bSetLastToken = FALSE;
        }
    }
    if ( bSetLastToken )
        nLastToken = pInfo->nToken;
}



// ============================================================================
// HTML DATA QUERY PARSER
// ============================================================================

template< typename Type >
inline Type getLimitedValue( const Type& rValue, const Type& rMin, const Type& rMax )
{ return ::std::max( ::std::min( rValue, rMax ), rMin ); }

// ============================================================================

/** Iterates through all HTML tag options of the passed ImportInfo struct. */
class ScHTMLOptionIterator
{
private:
    const HTMLOptions*  mpOptions;      /// The options array.
    const HTMLOption*   mpCurrOption;   /// Current option.
    sal_uInt16          mnCount;        /// Size of the options array.
    sal_uInt16          mnIndex;        /// Next option to return.

public:
    explicit            ScHTMLOptionIterator( const ImportInfo& rInfo );

    inline bool         is() const { return mnIndex < mnCount; }
    inline const HTMLOption* operator->() const { return mpCurrOption; }
    inline const HTMLOption& operator*() const { return *mpCurrOption; }
    ScHTMLOptionIterator& operator++();
};

// ----------------------------------------------------------------------------

ScHTMLOptionIterator::ScHTMLOptionIterator( const ImportInfo& rInfo ) :
    mpOptions( 0 ),
    mpCurrOption( 0 ),
    mnCount( 0 ),
    mnIndex( 0 )
{
    const HTMLParser* pParser = static_cast< const HTMLParser* >( rInfo.pParser );
    if( pParser )
        mpOptions = pParser->GetOptions();
    if( mpOptions )
        mnCount = mpOptions->Count();
    if( mnCount )
        mpCurrOption = mpOptions->GetObject( 0 );
}

ScHTMLOptionIterator& ScHTMLOptionIterator::operator++()
{
    if( mnIndex < mnCount ) ++mnIndex;
    mpCurrOption = (mnIndex < mnCount) ? mpOptions->GetObject( mnIndex ) : 0;
    return *this;
}

// ============================================================================

ScHTMLEntry::ScHTMLEntry( const SfxItemSet& rItemSet, ScHTMLTableId nTableId ) :
    ScEEParseEntry( rItemSet ),
    mbImportAlways( false )
{
    nTab = nTableId;
    bEntirePara = false;
}

bool ScHTMLEntry::HasContents() const
{
     return mbImportAlways || aSel.HasRange() || aAltText.Len() || IsTable();
}

void ScHTMLEntry::AdjustStart( const ImportInfo& rInfo )
{
    // set start position
    aSel.nStartPara = rInfo.aSelection.nStartPara;
    aSel.nStartPos = rInfo.aSelection.nStartPos;
    // adjust end position
    if( (aSel.nEndPara < aSel.nStartPara) || ((aSel.nEndPara == aSel.nStartPara) && (aSel.nEndPos < aSel.nStartPos)) )
    {
        aSel.nEndPara = aSel.nStartPara;
        aSel.nEndPos = aSel.nStartPos;
    }
}

void ScHTMLEntry::AdjustEnd( const ImportInfo& rInfo )
{
    DBG_ASSERT( (aSel.nEndPara < rInfo.aSelection.nEndPara) ||
                ((aSel.nEndPara == rInfo.aSelection.nEndPara) && (aSel.nEndPos <= rInfo.aSelection.nEndPos)),
                "ScHTMLQueryParser::AdjustEntryEnd - invalid end position" );
    // set end position
    aSel.nEndPara = rInfo.aSelection.nEndPara;
    aSel.nEndPos = rInfo.aSelection.nEndPos;
}

void ScHTMLEntry::Strip( const EditEngine& rEditEngine )
{
    // strip leading empty paragraphs
    while( (aSel.nStartPara < aSel.nEndPara) && (rEditEngine.GetTextLen( aSel.nStartPara ) <= aSel.nStartPos) )
    {
        ++aSel.nStartPara;
        aSel.nStartPos = 0;
    }
    // strip trailing empty paragraphs
    while( (aSel.nStartPara < aSel.nEndPara) && (aSel.nEndPos == 0) )
    {
        --aSel.nEndPara;
        aSel.nEndPos = rEditEngine.GetTextLen( aSel.nEndPara );
    }
}

// ============================================================================

/** A map of ScHTMLTable objects.

    Organizes the tables with a unique table key. Stores nested tables inside
    the parent table and forms in this way a tree structure of tables. An
    instance of this class ownes the contained table objects and deletes them
    on destruction.
 */
class ScHTMLTableMap
{
private:
    typedef ::boost::shared_ptr< ScHTMLTable >          ScHTMLTablePtr;
    typedef ::std::map< ScHTMLTableId, ScHTMLTablePtr > ScHTMLTableStdMap;

public:
    typedef ScHTMLTableStdMap::iterator             iterator;
    typedef ScHTMLTableStdMap::const_iterator       const_iterator;

private:
    ScHTMLTable&        mrParentTable;      /// Reference to parent table.
    ScHTMLTableStdMap   maTables;           /// Container for all table objects.
    mutable ScHTMLTable* mpCurrTable;       /// Current table, used for fast search.

public:
    explicit            ScHTMLTableMap( ScHTMLTable& rParentTable );
    virtual             ~ScHTMLTableMap();

    inline iterator     begin() { return maTables.begin(); }
    inline const_iterator begin() const { return maTables.begin(); }
    inline iterator     end() { return maTables.end(); }
    inline const_iterator end() const { return maTables.end(); }
    inline bool         empty() const { return maTables.empty(); }

    /** Returns the specified table.
        @param nTableId  Unique identifier of the table.
        @param bDeep  true = searches deep in all nested table; false = only in this container. */
    ScHTMLTable*        FindTable( ScHTMLTableId nTableId, bool bDeep = true ) const;

    /** Inserts a new table into the container. This container owns the created table.
        @param bPreFormText  true = New table is based on preformatted text (<pre> tag). */
    ScHTMLTable*        CreateTable( const ImportInfo& rInfo, bool bPreFormText );

private:
    /** Sets a working table with its index for search optimization. */
    inline void         SetCurrTable( ScHTMLTable* pTable ) const
                            { if( pTable ) mpCurrTable = pTable; }
};

// ----------------------------------------------------------------------------

ScHTMLTableMap::ScHTMLTableMap( ScHTMLTable& rParentTable ) :
    mrParentTable( rParentTable )
{
}

ScHTMLTableMap::~ScHTMLTableMap()
{
}

ScHTMLTable* ScHTMLTableMap::FindTable( ScHTMLTableId nTableId, bool bDeep ) const
{
    ScHTMLTable* pResult = 0;
    if( mpCurrTable && (nTableId == mpCurrTable->GetTableId()) )
        pResult = mpCurrTable;              // cached table
    else
    {
        const_iterator aFind = maTables.find( nTableId );
        if( aFind != maTables.end() )
            pResult = aFind->second.get();  // table from this container
    }

    // not found -> search deep in nested tables
    if( !pResult && bDeep )
        for( const_iterator aIter = begin(), aEnd = end(); !pResult && (aIter != aEnd); ++aIter )
            pResult = aIter->second->FindNestedTable( nTableId );

    SetCurrTable( pResult );
    return pResult;
}

ScHTMLTable* ScHTMLTableMap::CreateTable( const ImportInfo& rInfo, bool bPreFormText )
{
    ScHTMLTable* pTable = new ScHTMLTable( mrParentTable, rInfo, bPreFormText );
    maTables[ pTable->GetTableId() ].reset( pTable );
    SetCurrTable( pTable );
    return pTable;
}

// ----------------------------------------------------------------------------

/** Simplified forward iterator for convenience.

    Before the iterator can be dereferenced, it must be tested with the is()
    method. The iterator may be invalid directly after construction (e.g. empty
    container).
 */
class ScHTMLTableIterator
{
public:
    /** Constructs the iterator for the passed table map.
        @param pTableMap  Pointer to the table map (is allowed to be NULL). */
    explicit            ScHTMLTableIterator( const ScHTMLTableMap* pTableMap );

    inline bool         is() const { return maIter != maEnd; }
    inline ScHTMLTable* operator->() { return maIter->second.get(); }
    inline ScHTMLTable& operator*() { return *maIter->second; }
    inline ScHTMLTableIterator& operator++() { ++maIter; return *this; }

private:
    ScHTMLTableMap::const_iterator maIter;
    ScHTMLTableMap::const_iterator maEnd;
};

ScHTMLTableIterator::ScHTMLTableIterator( const ScHTMLTableMap* pTableMap )
{
    if( pTableMap )
    {
        maIter = pTableMap->begin();
        maEnd = pTableMap->end();
    }
}

// ============================================================================

ScHTMLTableAutoId::ScHTMLTableAutoId( ScHTMLTableId& rnUnusedId ) :
    mnTableId( rnUnusedId ),
    mrnUnusedId( rnUnusedId )
{
    ++mrnUnusedId;
}

// ----------------------------------------------------------------------------

ScHTMLTable::ScHTMLTable( ScHTMLTable& rParentTable, const ImportInfo& rInfo, bool bPreFormText ) :
    mpParentTable( &rParentTable ),
    maTableId( rParentTable.maTableId.mrnUnusedId ),
    maTableItemSet( rParentTable.GetCurrItemSet() ),
    mrEditEngine( rParentTable.mrEditEngine ),
    mrEEParseList( rParentTable.mrEEParseList ),
    mpCurrEntryList( 0 ),
    maSize( 1, 1 ),
    mbBorderOn( false ),
    mbPreFormText( bPreFormText ),
    mbRowOn( false ),
    mbDataOn( false ),
    mbPushEmptyLine( false )
{
    if( mbPreFormText )
    {
        ImplRowOn();
        ImplDataOn( ScHTMLSize( 1, 1 ) );
    }
    else
    {
        ProcessFormatOptions( maTableItemSet, rInfo );
        for( ScHTMLOptionIterator aIter( rInfo ); aIter.is(); ++aIter )
        {
            switch( aIter->GetToken() )
            {
                case HTML_O_BORDER:
                    mbBorderOn = ((aIter->GetString().Len() == 0) || (aIter->GetNumber() != 0));
                break;
                case HTML_O_ID:
                    maTableName = aIter->GetString();
                break;
            }
        }
    }

    CreateNewEntry( rInfo );
}

ScHTMLTable::ScHTMLTable( SfxItemPool& rPool, EditEngine& rEditEngine, ScEEParseList& rEEParseList, ScHTMLTableId& rnUnusedId ) :
    mpParentTable( 0 ),
    maTableId( rnUnusedId ),
    maTableItemSet( rPool ),
    mrEditEngine( rEditEngine ),
    mrEEParseList( rEEParseList ),
    mpCurrEntryList( 0 ),
    maSize( 1, 1 ),
    mbBorderOn( false ),
    mbPreFormText( false ),
    mbRowOn( false ),
    mbDataOn( false ),
    mbPushEmptyLine( false )
{
    // open the first "cell" of the document
    ImplRowOn();
    ImplDataOn( ScHTMLSize( 1, 1 ) );
    mxCurrEntry = CreateEntry();
}

ScHTMLTable::~ScHTMLTable()
{
}

const SfxItemSet& ScHTMLTable::GetCurrItemSet() const
{
    // first try cell item set, then row item set, then table item set
    return mxDataItemSet.get() ? *mxDataItemSet : (mxRowItemSet.get() ? *mxRowItemSet : maTableItemSet);
}

ScHTMLSize ScHTMLTable::GetSpan( const ScHTMLPos& rCellPos ) const
{
    ScHTMLSize aSpan( 1, 1 );
    ScRange* pRange = 0;
    if( ((pRange = maVMergedCells.Find( rCellPos.MakeAddr() )) != 0) || ((pRange = maHMergedCells.Find( rCellPos.MakeAddr() )) != 0) )
        aSpan.Set( pRange->aEnd.Col() - pRange->aStart.Col() + 1, pRange->aEnd.Row() - pRange->aStart.Row() + 1 );
    return aSpan;
}

ScHTMLTable* ScHTMLTable::FindNestedTable( ScHTMLTableId nTableId ) const
{
    return mxNestedTables.get() ? mxNestedTables->FindTable( nTableId, true ) : 0;
}

void ScHTMLTable::PutItem( const SfxPoolItem& rItem )
{
    DBG_ASSERT( mxCurrEntry.get(), "ScHTMLTable::PutItem - no current entry" );
    if( mxCurrEntry.get() && mxCurrEntry->IsEmpty() )
        mxCurrEntry->GetItemSet().Put( rItem );
}

void ScHTMLTable::PutText( const ImportInfo& rInfo )
{
    DBG_ASSERT( mxCurrEntry.get(), "ScHTMLTable::PutText - no current entry" );
    if( mxCurrEntry.get() )
    {
        if( !mxCurrEntry->HasContents() && IsSpaceCharInfo( rInfo ) )
            mxCurrEntry->AdjustStart( rInfo );
        else
            mxCurrEntry->AdjustEnd( rInfo );
    }
}

void ScHTMLTable::InsertPara( const ImportInfo& rInfo )
{
    if( mxCurrEntry.get() && mbDataOn && !IsEmptyCell() )
        mxCurrEntry->SetImportAlways();
    PushEntry( rInfo );
    CreateNewEntry( rInfo );
    InsertLeadingEmptyLine();
}

void ScHTMLTable::BreakOn()
{
    // empty line, if <br> is at start of cell
    mbPushEmptyLine = !mbPreFormText && mbDataOn && IsEmptyCell();
}

void ScHTMLTable::HeadingOn()
{
    // call directly, InsertPara() has not been called before
    InsertLeadingEmptyLine();
}

void ScHTMLTable::InsertLeadingEmptyLine()
{
    // empty line, if <p>, </p>, <h?>, or </h*> are not at start of cell
    mbPushEmptyLine = !mbPreFormText && mbDataOn && !IsEmptyCell();
}

void ScHTMLTable::AnchorOn()
{
    DBG_ASSERT( mxCurrEntry.get(), "ScHTMLTable::AnchorOn - no current entry" );
    // don't skip entries with single hyperlinks
    if( mxCurrEntry.get() )
        mxCurrEntry->SetImportAlways();
}

ScHTMLTable* ScHTMLTable::TableOn( const ImportInfo& rInfo )
{
    PushEntry( rInfo );
    return InsertNestedTable( rInfo, false );
}

ScHTMLTable* ScHTMLTable::TableOff( const ImportInfo& rInfo )
{
    return mbPreFormText ? this : CloseTable( rInfo );
}

ScHTMLTable* ScHTMLTable::PreOn( const ImportInfo& rInfo )
{
    PushEntry( rInfo );
    return InsertNestedTable( rInfo, true );
}

ScHTMLTable* ScHTMLTable::PreOff( const ImportInfo& rInfo )
{
    return mbPreFormText ? CloseTable( rInfo ) : this;
}

void ScHTMLTable::RowOn( const ImportInfo& rInfo )
{
    PushEntry( rInfo, true );
    if( mpParentTable && !mbPreFormText )   // no rows allowed in global and preformatted tables
    {
        ImplRowOn();
        ProcessFormatOptions( *mxRowItemSet, rInfo );
    }
    CreateNewEntry( rInfo );
}

void ScHTMLTable::RowOff( const ImportInfo& rInfo )
{
    PushEntry( rInfo, true );
    if( mpParentTable && !mbPreFormText )   // no rows allowed in global and preformatted tables
        ImplRowOff();
    CreateNewEntry( rInfo );
}

void ScHTMLTable::DataOn( const ImportInfo& rInfo )
{
    PushEntry( rInfo, true );
    if( mpParentTable && !mbPreFormText )   // no cells allowed in global and preformatted tables
    {
        // read needed options from the <td> tag
        ScHTMLSize aSpanSize( 1, 1 );
        ::std::auto_ptr< String > pValStr, pNumStr;
        for( ScHTMLOptionIterator aIter( rInfo ); aIter.is(); ++aIter )
        {
            switch( aIter->GetToken() )
            {
                case HTML_O_COLSPAN:
                    aSpanSize.mnCols = static_cast< SCCOL >( getLimitedValue< sal_Int32 >( aIter->GetString().ToInt32(), 1, 256 ) );
                break;
                case HTML_O_ROWSPAN:
                    aSpanSize.mnRows = static_cast< SCROW >( getLimitedValue< sal_Int32 >( aIter->GetString().ToInt32(), 1, 256 ) );
                break;
                case HTML_O_SDVAL:
                    pValStr.reset( new String( aIter->GetString() ) );
                break;
                case HTML_O_SDNUM:
                    pNumStr.reset( new String( aIter->GetString() ) );
                break;
            }
        }

        ImplDataOn( aSpanSize );
        ProcessFormatOptions( *mxDataItemSet, rInfo );
        CreateNewEntry( rInfo );
        mxCurrEntry->pValStr = pValStr.release();
        mxCurrEntry->pNumStr = pNumStr.release();
    }
    else
        CreateNewEntry( rInfo );
}

void ScHTMLTable::DataOff( const ImportInfo& rInfo )
{
    PushEntry( rInfo, true );
    if( mpParentTable && !mbPreFormText )   // no cells allowed in global and preformatted tables
        ImplDataOff();
    CreateNewEntry( rInfo );
}

void ScHTMLTable::BodyOn( const ImportInfo& rInfo )
{
    bool bPushed = PushEntry( rInfo );
    if( !mpParentTable )
    {
        // #108269# do not start new row, if nothing (no title) precedes the body.
        if( bPushed || !mbRowOn )
            ImplRowOn();
        if( bPushed || !mbDataOn )
            ImplDataOn( ScHTMLSize( 1, 1 ) );
        ProcessFormatOptions( *mxDataItemSet, rInfo );
    }
    CreateNewEntry( rInfo );
}

void ScHTMLTable::BodyOff( const ImportInfo& rInfo )
{
    PushEntry( rInfo );
    if( !mpParentTable )
    {
        ImplDataOff();
        ImplRowOff();
    }
    CreateNewEntry( rInfo );
}

ScHTMLTable* ScHTMLTable::CloseTable( const ImportInfo& rInfo )
{
    if( mpParentTable )     // not allowed to close global table
    {
        PushEntry( rInfo, mbDataOn );
        ImplDataOff();
        ImplRowOff();
        mpParentTable->PushTableEntry( GetTableId() );
        mpParentTable->CreateNewEntry( rInfo );
        if( mbPreFormText ) // enclose preformatted table with empty lines in parent table
            mpParentTable->InsertLeadingEmptyLine();
        return mpParentTable;
    }
    return this;
}

SCCOLROW ScHTMLTable::GetDocSize( ScHTMLOrient eOrient, SCCOLROW nCellPos ) const
{
    const ScSizeVec& rSizes = maCumSizes[ eOrient ];
    size_t nIndex = static_cast< size_t >( nCellPos );
    if( nIndex >= rSizes.size() ) return 0;
    return (nIndex == 0) ? rSizes.front() : (rSizes[ nIndex ] - rSizes[ nIndex - 1 ]);
}

SCCOLROW ScHTMLTable::GetDocSize( ScHTMLOrient eOrient, SCCOLROW nCellBegin, SCCOLROW nCellEnd ) const
{
    const ScSizeVec& rSizes = maCumSizes[ eOrient ];
    size_t nBeginIdx = static_cast< size_t >( ::std::max< SCCOLROW >( nCellBegin, 0 ) );
    size_t nEndIdx = static_cast< size_t >( ::std::min< SCCOLROW >( nCellEnd, static_cast< SCCOLROW >( rSizes.size() ) ) );
    if (nBeginIdx >= nEndIdx ) return 0;
    return rSizes[ nEndIdx - 1 ] - ((nBeginIdx == 0) ? 0 : rSizes[ nBeginIdx - 1 ]);
}

SCCOLROW ScHTMLTable::GetDocSize( ScHTMLOrient eOrient ) const
{
    const ScSizeVec& rSizes = maCumSizes[ eOrient ];
    return rSizes.empty() ? 0 : rSizes.back();
}

ScHTMLSize ScHTMLTable::GetDocSize( const ScHTMLPos& rCellPos ) const
{
    ScHTMLSize aCellSpan = GetSpan( rCellPos );
    return ScHTMLSize(
        static_cast< SCCOL >( GetDocSize( tdCol, rCellPos.mnCol, rCellPos.mnCol + aCellSpan.mnCols ) ),
        static_cast< SCROW >( GetDocSize( tdRow, rCellPos.mnRow, rCellPos.mnRow + aCellSpan.mnRows ) ) );
}

SCCOLROW ScHTMLTable::GetDocPos( ScHTMLOrient eOrient, SCCOLROW nCellPos ) const
{
    return maDocBasePos.Get( eOrient ) + GetDocSize( eOrient, 0, nCellPos );
}

ScHTMLPos ScHTMLTable::GetDocPos( const ScHTMLPos& rCellPos ) const
{
    return ScHTMLPos(
        static_cast< SCCOL >( GetDocPos( tdCol, rCellPos.mnCol ) ),
        static_cast< SCROW >( GetDocPos( tdRow, rCellPos.mnRow ) ) );
}

void ScHTMLTable::GetDocRange( ScRange& rRange ) const
{
    rRange.aStart = rRange.aEnd = maDocBasePos.MakeAddr();
    rRange.aEnd.Move( static_cast< SCsCOL >( GetDocSize( tdCol ) ) - 1, static_cast< SCsROW >( GetDocSize( tdRow ) ) - 1, 0 );
}

void ScHTMLTable::ApplyCellBorders( ScDocument* pDoc, const ScAddress& rFirstPos ) const
{
    DBG_ASSERT( pDoc, "ScHTMLTable::ApplyCellBorders - no document" );
    if( pDoc && mbBorderOn )
    {
        const SCCOL nLastCol = maSize.mnCols - 1;
        const SCROW nLastRow = maSize.mnRows - 1;
        const sal_uInt16 nOuterLine = DEF_LINE_WIDTH_2;
        const sal_uInt16 nInnerLine = DEF_LINE_WIDTH_0;
        SvxBorderLine aOuterLine, aInnerLine;
        aOuterLine.SetColor( Color( COL_BLACK ) );
        aOuterLine.SetOutWidth( nOuterLine );
        aInnerLine.SetColor( Color( COL_BLACK ) );
        aInnerLine.SetOutWidth( nInnerLine );
        SvxBoxItem aBorderItem( ATTR_BORDER );

        for( SCCOL nCol = 0; nCol <= nLastCol; ++nCol )
        {
            SvxBorderLine* pLeftLine = (nCol == 0) ? &aOuterLine : &aInnerLine;
            SvxBorderLine* pRightLine = (nCol == nLastCol) ? &aOuterLine : &aInnerLine;
            SCCOL nCellCol1 = static_cast< SCCOL >( GetDocPos( tdCol, nCol ) ) + rFirstPos.Col();
            SCCOL nCellCol2 = nCellCol1 + static_cast< SCCOL >( GetDocSize( tdCol, nCol ) ) - 1;
            for( SCROW nRow = 0; nRow <= nLastRow; ++nRow )
            {
                SvxBorderLine* pTopLine = (nRow == 0) ? &aOuterLine : &aInnerLine;
                SvxBorderLine* pBottomLine = (nRow == nLastRow) ? &aOuterLine : &aInnerLine;
                SCROW nCellRow1 = GetDocPos( tdRow, nRow ) + rFirstPos.Row();
                SCROW nCellRow2 = nCellRow1 + GetDocSize( tdRow, nRow ) - 1;
                for( SCCOL nCellCol = nCellCol1; nCellCol <= nCellCol2; ++nCellCol )
                {
                    aBorderItem.SetLine( (nCellCol == nCellCol1) ? pLeftLine : 0, BOX_LINE_LEFT );
                    aBorderItem.SetLine( (nCellCol == nCellCol2) ? pRightLine : 0, BOX_LINE_RIGHT );
                    for( SCROW nCellRow = nCellRow1; nCellRow <= nCellRow2; ++nCellRow )
                    {
                        aBorderItem.SetLine( (nCellRow == nCellRow1) ? pTopLine : 0, BOX_LINE_TOP );
                        aBorderItem.SetLine( (nCellRow == nCellRow2) ? pBottomLine : 0, BOX_LINE_BOTTOM );
                        pDoc->ApplyAttr( nCellCol, nCellRow, rFirstPos.Tab(), aBorderItem );
                    }
                }
            }
        }
    }

    for( ScHTMLTableIterator aIter( mxNestedTables.get() ); aIter.is(); ++aIter )
        aIter->ApplyCellBorders( pDoc, rFirstPos );
}

// ----------------------------------------------------------------------------

bool ScHTMLTable::IsEmptyCell() const
{
    return mpCurrEntryList && mpCurrEntryList->empty();
}

bool ScHTMLTable::IsSpaceCharInfo( const ImportInfo& rInfo )
{
    return (rInfo.nToken == HTML_TEXTTOKEN) && (rInfo.aText.Len() == 1) && (rInfo.aText.GetChar( 0 ) == ' ');
}

ScHTMLTable::ScHTMLEntryPtr ScHTMLTable::CreateEntry() const
{
    return ScHTMLEntryPtr( new ScHTMLEntry( GetCurrItemSet() ) );
}

void ScHTMLTable::CreateNewEntry( const ImportInfo& rInfo )
{
    DBG_ASSERT( !mxCurrEntry.get(), "ScHTMLTable::CreateNewEntry - old entry still present" );
    mxCurrEntry = CreateEntry();
    mxCurrEntry->aSel = rInfo.aSelection;
}

void ScHTMLTable::ImplPushEntryToList( ScHTMLEntryList& rEntryList, ScHTMLEntryPtr& rxEntry )
{
    // HTML entry list does not own the entries
    rEntryList.push_back( rxEntry.get() );
    // mrEEParseList (reference to member of ScEEParser) owns the entries
    mrEEParseList.Insert( rxEntry.release(), LIST_APPEND );
}

bool ScHTMLTable::PushEntry( ScHTMLEntryPtr& rxEntry )
{
    bool bPushed = false;
    if( rxEntry.get() && rxEntry->HasContents() )
    {
        if( mpCurrEntryList )
        {
            if( mbPushEmptyLine )
            {
                ScHTMLEntryPtr xEmptyEntry = CreateEntry();
                ImplPushEntryToList( *mpCurrEntryList, xEmptyEntry );
                mbPushEmptyLine = false;
            }
            ImplPushEntryToList( *mpCurrEntryList, rxEntry );
            bPushed = true;
        }
        else if( mpParentTable )
        {
            bPushed = mpParentTable->PushEntry( rxEntry );
        }
        else
        {
            DBG_ERRORFILE( "ScHTMLTable::PushEntry - cannot push entry, no parent found" );
        }
    }
    return bPushed;
}

bool ScHTMLTable::PushEntry( const ImportInfo& rInfo, bool bLastInCell )
{
    DBG_ASSERT( mxCurrEntry.get(), "ScHTMLTable::PushEntry - no current entry" );
    bool bPushed = false;
    if( mxCurrEntry.get() )
    {
        mxCurrEntry->AdjustEnd( rInfo );
        mxCurrEntry->Strip( mrEditEngine );

        // import entry always, if it is the last in cell, and cell is still empty
        if( bLastInCell && IsEmptyCell() )
        {
            mxCurrEntry->SetImportAlways();
            // don't insert empty lines before single empty entries
            if( mxCurrEntry->IsEmpty() )
                mbPushEmptyLine = false;
        }

        bPushed = PushEntry( mxCurrEntry );
        mxCurrEntry.reset();
    }
    return bPushed;
}

bool ScHTMLTable::PushTableEntry( ScHTMLTableId nTableId )
{
    DBG_ASSERT( nTableId != SC_HTML_GLOBAL_TABLE, "ScHTMLTable::PushTableEntry - cannot push global table" );
    bool bPushed = false;
    if( nTableId != SC_HTML_GLOBAL_TABLE )
    {
        ScHTMLEntryPtr xEntry( new ScHTMLEntry( maTableItemSet, nTableId ) );
        bPushed = PushEntry( xEntry );
    }
    return bPushed;
}

ScHTMLTable* ScHTMLTable::GetExistingTable( ScHTMLTableId nTableId ) const
{
    ScHTMLTable* pTable = ((nTableId != SC_HTML_GLOBAL_TABLE) && mxNestedTables.get()) ?
        mxNestedTables->FindTable( nTableId, false ) : 0;
    DBG_ASSERT( pTable || (nTableId == SC_HTML_GLOBAL_TABLE), "ScHTMLTable::GetExistingTable - table not found" );
    return pTable;
}

ScHTMLTable* ScHTMLTable::InsertNestedTable( const ImportInfo& rInfo, bool bPreFormText )
{
    if( !mxNestedTables.get() )
        mxNestedTables.reset( new ScHTMLTableMap( *this ) );
    if( bPreFormText )      // enclose new preformatted table with empty lines
        InsertLeadingEmptyLine();
    return mxNestedTables->CreateTable( rInfo, bPreFormText );
}

void ScHTMLTable::InsertNewCell( const ScHTMLSize& rSpanSize )
{
    ScRange* pRange;

    // find an unused cell
    while( (pRange = maVMergedCells.Find( maCurrCell.MakeAddr() )) != 0 )
        maCurrCell.mnCol = pRange->aEnd.Col() + 1;
    mpCurrEntryList = &maEntryMap[ maCurrCell ];

    // try to find collisions, shrink existing ranges
    SCCOL nColEnd = maCurrCell.mnCol + rSpanSize.mnCols;
    for( ScAddress aAddr( maCurrCell.MakeAddr() ); aAddr.Col() < nColEnd; aAddr.IncCol() )
        if( (pRange = maVMergedCells.Find( aAddr )) != 0 )
            pRange->aEnd.SetRow( maCurrCell.mnRow - 1 );

    // insert the new range into the cell lists
    ScRange aNewRange( maCurrCell.MakeAddr() );
    aNewRange.aEnd.Move( rSpanSize.mnCols - 1, rSpanSize.mnRows - 1, 0 );
    if( rSpanSize.mnCols > 1 )
    {
        maVMergedCells.Append( aNewRange );
    }
    else
    {
        if( rSpanSize.mnRows > 1 )
            maHMergedCells.Append( aNewRange );
        maUsedCells.Join( aNewRange );
    }

    // adjust table size
    maSize.mnCols = ::std::max< SCCOL >( maSize.mnCols, aNewRange.aEnd.Col() + 1 );
    maSize.mnRows = ::std::max< SCROW >( maSize.mnRows, aNewRange.aEnd.Row() + 1 );
}

void ScHTMLTable::ImplRowOn()
{
    if( mbRowOn )
        ImplRowOff();
    mxRowItemSet.reset( new SfxItemSet( maTableItemSet ) );
    maCurrCell.mnCol = 0;
    mbRowOn = true;
    mbDataOn = false;
}

void ScHTMLTable::ImplRowOff()
{
    if( mbDataOn )
        ImplDataOff();
    if( mbRowOn )
    {
        mxRowItemSet.reset();
        ++maCurrCell.mnRow;
        mbRowOn = mbDataOn = false;
    }
}

void ScHTMLTable::ImplDataOn( const ScHTMLSize& rSpanSize )
{
    if( mbDataOn )
        ImplDataOff();
    if( !mbRowOn )
        ImplRowOn();
    mxDataItemSet.reset( new SfxItemSet( *mxRowItemSet ) );
    InsertNewCell( rSpanSize );
    mbDataOn = true;
    mbPushEmptyLine = false;
}

void ScHTMLTable::ImplDataOff()
{
    if( mbDataOn )
    {
        mxDataItemSet.reset();
        ++maCurrCell.mnCol;
        mpCurrEntryList = 0;
        mbDataOn = false;
    }
}

void ScHTMLTable::ProcessFormatOptions( SfxItemSet& rItemSet, const ImportInfo& rInfo )
{
    // special handling for table header cells
    if( rInfo.nToken == HTML_TABLEHEADER_ON )
    {
        rItemSet.Put( SvxWeightItem( WEIGHT_BOLD, ATTR_FONT_WEIGHT ) );
        rItemSet.Put( SvxHorJustifyItem( SVX_HOR_JUSTIFY_CENTER, ATTR_HOR_JUSTIFY ) );
    }

    for( ScHTMLOptionIterator aIter( rInfo ); aIter.is(); ++aIter )
    {
        switch( aIter->GetToken() )
        {
            case HTML_O_ALIGN:
            {
                SvxCellHorJustify eVal = SVX_HOR_JUSTIFY_STANDARD;
                const String& rOptVal = aIter->GetString();
                if( rOptVal.EqualsIgnoreCaseAscii( OOO_STRING_SVTOOLS_HTML_AL_right ) )
                    eVal = SVX_HOR_JUSTIFY_RIGHT;
                else if( rOptVal.EqualsIgnoreCaseAscii( OOO_STRING_SVTOOLS_HTML_AL_center ) )
                    eVal = SVX_HOR_JUSTIFY_CENTER;
                else if( rOptVal.EqualsIgnoreCaseAscii( OOO_STRING_SVTOOLS_HTML_AL_left ) )
                    eVal = SVX_HOR_JUSTIFY_LEFT;
                if( eVal != SVX_HOR_JUSTIFY_STANDARD )
                    rItemSet.Put( SvxHorJustifyItem( eVal, ATTR_HOR_JUSTIFY ) );
            }
            break;

            case HTML_O_VALIGN:
            {
                SvxCellVerJustify eVal = SVX_VER_JUSTIFY_STANDARD;
                const String& rOptVal = aIter->GetString();
                if( rOptVal.EqualsIgnoreCaseAscii( OOO_STRING_SVTOOLS_HTML_VA_top ) )
                    eVal = SVX_VER_JUSTIFY_TOP;
                else if( rOptVal.EqualsIgnoreCaseAscii( OOO_STRING_SVTOOLS_HTML_VA_middle ) )
                    eVal = SVX_VER_JUSTIFY_CENTER;
                else if( rOptVal.EqualsIgnoreCaseAscii( OOO_STRING_SVTOOLS_HTML_VA_bottom ) )
                    eVal = SVX_VER_JUSTIFY_BOTTOM;
                if( eVal != SVX_VER_JUSTIFY_STANDARD )
                    rItemSet.Put( SvxVerJustifyItem( eVal, ATTR_VER_JUSTIFY ) );
            }
            break;

            case HTML_O_BGCOLOR:
            {
                Color aColor;
                aIter->GetColor( aColor );
                rItemSet.Put( SvxBrushItem( aColor, ATTR_BACKGROUND ) );
            }
            break;
        }
    }
}

void ScHTMLTable::SetDocSize( ScHTMLOrient eOrient, SCCOLROW nCellPos, SCCOLROW nSize )
{
    DBG_ASSERT( nCellPos >= 0, "ScHTMLTable::SetDocSize - unexpected negative position" );
    ScSizeVec& rSizes = maCumSizes[ eOrient ];
    size_t nIndex = static_cast< size_t >( nCellPos );
    // expand with height/width == 1
    while( nIndex >= rSizes.size() )
        rSizes.push_back( rSizes.empty() ? 1 : (rSizes.back() + 1) );
    // update size of passed position and all following
    SCsCOLROW nDiff = nSize - ((nIndex == 0) ? rSizes.front() : (rSizes[ nIndex ] - rSizes[ nIndex - 1 ]));
    if( nDiff != 0 )
        for( ScSizeVec::iterator aIt = rSizes.begin() + nIndex, aEnd = rSizes.end(); aIt != aEnd; ++aIt )
            *aIt += nDiff;
}

void ScHTMLTable::CalcNeededDocSize(
        ScHTMLOrient eOrient, SCCOLROW nCellPos, SCCOLROW nCellSpan, SCCOLROW nRealDocSize )
{
    SCCOLROW nDiffSize = 0;
    // in merged columns/rows: reduce needed size by size of leading columns
    while( nCellSpan > 1 )
    {
        nDiffSize += GetDocSize( eOrient, nCellPos );
        --nCellSpan;
        ++nCellPos;
    }
    // set remaining needed size to last column/row
    nRealDocSize -= ::std::min< SCCOLROW >( nRealDocSize - 1, nDiffSize );
    SetDocSize( eOrient, nCellPos, nRealDocSize );
}

// ----------------------------------------------------------------------------

void ScHTMLTable::FillEmptyCells()
{
    for( ScHTMLTableIterator aIter( mxNestedTables.get() ); aIter.is(); ++aIter )
        aIter->FillEmptyCells();

    for( const ScRange* pRange = maVMergedCells.First(); pRange; pRange = maVMergedCells.Next() )
        maUsedCells.Join( *pRange );

    for( ScAddress aAddr; aAddr.Row() < maSize.mnRows; aAddr.IncRow() )
    {
        for( aAddr.SetCol( 0 ); aAddr.Col() < maSize.mnCols; aAddr.IncCol() )
        {
            if( !maUsedCells.Find( aAddr ) )
            {
                // create a range for the lock list (used to calc. cell span)
                ScRange aRange( aAddr );
                do
                {
                    aRange.aEnd.IncCol();
                }
                while( (aRange.aEnd.Col() < maSize.mnCols) && !maUsedCells.Find( aRange.aEnd ) );
                aRange.aEnd.IncCol( -1 );
                maUsedCells.Join( aRange );

                // insert a dummy entry
                ScHTMLEntryPtr xEntry = CreateEntry();
                ImplPushEntryToList( maEntryMap[ ScHTMLPos( aAddr ) ], xEntry );
            }
        }
    }
}

void ScHTMLTable::RecalcDocSize()
{
    // recalc table sizes recursively from inner to outer
    for( ScHTMLTableIterator aIter( mxNestedTables.get() ); aIter.is(); ++aIter )
        aIter->RecalcDocSize();

    /*  Two passes: first calculates the sizes of single columns/rows, then
        the sizes of spanned columns/rows. This allows to fill nested tables
        into merged cells optimally. */
    static const sal_uInt16 PASS_SINGLE = 0;
    static const sal_uInt16 PASS_SPANNED = 1;
    for( sal_uInt16 nPass = PASS_SINGLE; nPass <= PASS_SPANNED; ++nPass )
    {
        // iterate through every table cell
        ScHTMLEntryMap::const_iterator aMapIterEnd = maEntryMap.end();
        for( ScHTMLEntryMap::const_iterator aMapIter = maEntryMap.begin(); aMapIter != aMapIterEnd; ++aMapIter )
        {
            const ScHTMLPos& rCellPos = aMapIter->first;
            ScHTMLSize aCellSpan = GetSpan( rCellPos );

            const ScHTMLEntryList& rEntryList = aMapIter->second;
            ScHTMLEntryList::const_iterator aListIter;
            ScHTMLEntryList::const_iterator aListIterEnd = rEntryList.end();

            // process the dimension of the current cell in this pass?
            // (pass is single and span is 1) or (pass is not single and span is not 1)
            bool bProcessColWidth = ((nPass == PASS_SINGLE) == (aCellSpan.mnCols == 1));
            bool bProcessRowHeight = ((nPass == PASS_SINGLE) == (aCellSpan.mnRows == 1));
            if( bProcessColWidth || bProcessRowHeight )
            {
                ScHTMLSize aDocSize( 1, 0 );    // resulting size of the cell in document

                // expand the cell size for each cell parse entry
                for( aListIter = rEntryList.begin(); aListIter != aListIterEnd; ++aListIter )
                {
                    ScHTMLTable* pTable = GetExistingTable( (*aListIter)->GetTableId() );
                    // find entry with maximum width
                    if( bProcessColWidth && pTable )
                        aDocSize.mnCols = ::std::max( aDocSize.mnCols, static_cast< SCCOL >( pTable->GetDocSize( tdCol ) ) );
                    // add up height of each entry
                    if( bProcessRowHeight )
                        aDocSize.mnRows += pTable ? pTable->GetDocSize( tdRow ) : 1;
                }
                if( !aDocSize.mnRows )
                    aDocSize.mnRows = 1;

                if( bProcessColWidth )
                    CalcNeededDocSize( tdCol, rCellPos.mnCol, aCellSpan.mnCols, aDocSize.mnCols );
                if( bProcessRowHeight )
                    CalcNeededDocSize( tdRow, rCellPos.mnRow, aCellSpan.mnRows, aDocSize.mnRows );
            }
        }
    }
}

void ScHTMLTable::RecalcDocPos( const ScHTMLPos& rBasePos )
{
    maDocBasePos = rBasePos;
    // after the previous assignment it is allowed to call GetDocPos() methods

    // iterate through every table cell
    ScHTMLEntryMap::iterator aMapIterEnd = maEntryMap.end();
    for( ScHTMLEntryMap::iterator aMapIter = maEntryMap.begin(); aMapIter != aMapIterEnd; ++aMapIter )
    {
        // fixed doc position of the entire cell (first entry)
        const ScHTMLPos aCellDocPos( GetDocPos( aMapIter->first ) );
        // fixed doc size of the entire cell
        const ScHTMLSize aCellDocSize( GetDocSize( aMapIter->first ) );

        // running doc position for single entries
        ScHTMLPos aEntryDocPos( aCellDocPos );

        ScHTMLEntryList& rEntryList = aMapIter->second;
        ScHTMLEntry* pEntry = 0;
        ScHTMLEntryList::iterator aListIterEnd = rEntryList.end();
        for( ScHTMLEntryList::iterator aListIter = rEntryList.begin(); aListIter != aListIterEnd; ++aListIter )
        {
            pEntry = *aListIter;
            if( ScHTMLTable* pTable = GetExistingTable( pEntry->GetTableId() ) )
            {
                pTable->RecalcDocPos( aEntryDocPos );   // recalc nested table
                pEntry->nCol = SCCOL_MAX;
                pEntry->nRow = SCROW_MAX;
                SCROW nTableRows = static_cast< SCROW >( pTable->GetDocSize( tdRow ) );

                // use this entry to pad empty space right of table
                if( mpParentTable )     // ... but not in global table
                {
                    SCCOL nStartCol = aEntryDocPos.mnCol + static_cast< SCCOL >( pTable->GetDocSize( tdCol ) );
                    SCCOL nNextCol = aEntryDocPos.mnCol + aCellDocSize.mnCols;
                    if( nStartCol < nNextCol )
                    {
                        pEntry->nCol = nStartCol;
                        pEntry->nRow = aEntryDocPos.mnRow;
                        pEntry->nColOverlap = nNextCol - nStartCol;
                        pEntry->nRowOverlap = nTableRows;
                    }
                }
                aEntryDocPos.mnRow += nTableRows;
            }
            else
            {
                pEntry->nCol = aEntryDocPos.mnCol;
                pEntry->nRow = aEntryDocPos.mnRow;
                if( mpParentTable )    // do not merge in global table
                    pEntry->nColOverlap = aCellDocSize.mnCols;
                ++aEntryDocPos.mnRow;
            }
        }

        // pEntry points now to last entry.
        if( pEntry )
        {
            if( (pEntry == rEntryList.front()) && (pEntry->GetTableId() == SC_HTML_NO_TABLE) )
            {
                // pEntry is the only entry in this cell - merge rows of cell with single non-table entry.
                pEntry->nRowOverlap = aCellDocSize.mnRows;
            }
            else
            {
                // #111667# fill up incomplete entry lists
                SCROW nFirstUnusedRow = aCellDocPos.mnRow + aCellDocSize.mnRows;
                while( aEntryDocPos.mnRow < nFirstUnusedRow )
                {
                    ScHTMLEntryPtr xDummyEntry( new ScHTMLEntry( pEntry->GetItemSet() ) );
                    xDummyEntry->nCol = aEntryDocPos.mnCol;
                    xDummyEntry->nRow = aEntryDocPos.mnRow;
                    xDummyEntry->nColOverlap = aCellDocSize.mnCols;
                    ImplPushEntryToList( rEntryList, xDummyEntry );
                    ++aEntryDocPos.mnRow;
                }
            }
        }
    }
}

// ============================================================================

ScHTMLGlobalTable::ScHTMLGlobalTable( SfxItemPool& rPool, EditEngine& rEditEngine, ScEEParseList& rEEParseList, ScHTMLTableId& rnUnusedId ) :
    ScHTMLTable( rPool, rEditEngine, rEEParseList, rnUnusedId )
{
}

ScHTMLGlobalTable::~ScHTMLGlobalTable()
{
}

void ScHTMLGlobalTable::Recalc()
{
    // Fills up empty cells with a dummy entry. */
    FillEmptyCells();
    // recalc table sizes of all nested tables and this table
    RecalcDocSize();
    // recalc document positions of all entries in this table and in nested tables
    RecalcDocPos( GetDocPos() );
}

// ============================================================================

ScHTMLQueryParser::ScHTMLQueryParser( EditEngine* pEditEngine, ScDocument* pDoc ) :
    ScHTMLParser( pEditEngine, pDoc ),
    mnUnusedId( SC_HTML_GLOBAL_TABLE ),
    mbTitleOn( false )
{
    mxGlobTable.reset( new ScHTMLGlobalTable( *pPool, *pEdit, *pList, mnUnusedId ) );
    mpCurrTable = mxGlobTable.get();
}

ScHTMLQueryParser::~ScHTMLQueryParser()
{
}

ULONG ScHTMLQueryParser::Read( SvStream& rStrm, const String& rBaseURL  )
{
    SvKeyValueIteratorRef xValues;
    SvKeyValueIterator* pAttributes = 0;

    SfxObjectShell* pObjSh = mpDoc->GetDocumentShell();
    if( pObjSh && pObjSh->IsLoading() )
    {
        pAttributes = pObjSh->GetHeaderAttributes();
    }
    else
    {
        /*  When not loading, set up fake HTTP headers to force the SfxHTMLParser
            to use UTF8 (used when pasting from clipboard) */
        const sal_Char* pCharSet = rtl_getBestMimeCharsetFromTextEncoding( RTL_TEXTENCODING_UTF8 );
        if( pCharSet )
        {
            String aContentType = String::CreateFromAscii( "text/html; charset=" );
            aContentType.AppendAscii( pCharSet );

            xValues = new SvKeyValueIterator;
            xValues->Append( SvKeyValue( String::CreateFromAscii( OOO_STRING_SVTOOLS_HTML_META_content_type ), aContentType ) );
            pAttributes = xValues;
        }
    }

    Link aOldLink = pEdit->GetImportHdl();
    pEdit->SetImportHdl( LINK( this, ScHTMLQueryParser, HTMLImportHdl ) );
    ULONG nErr = pEdit->Read( rStrm, rBaseURL, EE_FORMAT_HTML, pAttributes );
    pEdit->SetImportHdl( aOldLink );

    mxGlobTable->Recalc();
    nColMax = static_cast< SCCOL >( mxGlobTable->GetDocSize( tdCol ) - 1 );
    nRowMax = static_cast< SCROW >( mxGlobTable->GetDocSize( tdRow ) - 1 );

    return nErr;
}

const ScHTMLTable* ScHTMLQueryParser::GetGlobalTable() const
{
    return mxGlobTable.get();
}

void ScHTMLQueryParser::ProcessToken( const ImportInfo& rInfo )
{
    switch( rInfo.nToken )
    {
// --- meta data ---
        case HTML_META:             MetaOn( rInfo );                break;  // <meta>

// --- title handling ---
        case HTML_TITLE_ON:         TitleOn( rInfo );               break;  // <title>
        case HTML_TITLE_OFF:        TitleOff( rInfo );              break;  // </title>

// --- body handling ---
        case HTML_BODY_ON:          mpCurrTable->BodyOn( rInfo );   break;  // <body>
        case HTML_BODY_OFF:         mpCurrTable->BodyOff( rInfo );  break;  // </body>

// --- insert text ---
        case HTML_TEXTTOKEN:        InsertText( rInfo );            break;  // any text
        case HTML_LINEBREAK:        mpCurrTable->BreakOn();         break;  // <br>
        case HTML_HEAD1_ON:                                                 // <h1>
        case HTML_HEAD2_ON:                                                 // <h2>
        case HTML_HEAD3_ON:                                                 // <h3>
        case HTML_HEAD4_ON:                                                 // <h4>
        case HTML_HEAD5_ON:                                                 // <h5>
        case HTML_HEAD6_ON:                                                 // <h6>
        case HTML_PARABREAK_ON:     mpCurrTable->HeadingOn();       break;  // <p>

// --- misc. contents ---
        case HTML_ANCHOR_ON:        mpCurrTable->AnchorOn();        break;  // <a>

// --- table handling ---
        case HTML_TABLE_ON:         TableOn( rInfo );               break;  // <table>
        case HTML_TABLE_OFF:        TableOff( rInfo );              break;  // </table>
        case HTML_TABLEROW_ON:      mpCurrTable->RowOn( rInfo );    break;  // <tr>
        case HTML_TABLEROW_OFF:     mpCurrTable->RowOff( rInfo );   break;  // </tr>
        case HTML_TABLEHEADER_ON:                                           // <th>
        case HTML_TABLEDATA_ON:     mpCurrTable->DataOn( rInfo );   break;  // <td>
        case HTML_TABLEHEADER_OFF:                                          // </th>
        case HTML_TABLEDATA_OFF:    mpCurrTable->DataOff( rInfo );  break;  // </td>
        case HTML_PREFORMTXT_ON:    PreOn( rInfo );                 break;  // <pre>
        case HTML_PREFORMTXT_OFF:   PreOff( rInfo );                break;  // </pre>

// --- formatting ---
        case HTML_FONT_ON:          FontOn( rInfo );                break;  // <font>

        case HTML_BIGPRINT_ON:      // <big>
            //! TODO: store current font size, use following size
            mpCurrTable->PutItem( SvxFontHeightItem( maFontHeights[ 3 ], 100, ATTR_FONT_HEIGHT ) );
        break;
        case HTML_SMALLPRINT_ON:    // <small>
            //! TODO: store current font size, use preceding size
            mpCurrTable->PutItem( SvxFontHeightItem( maFontHeights[ 0 ], 100, ATTR_FONT_HEIGHT ) );
        break;

        case HTML_BOLD_ON:          // <b>
        case HTML_STRONG_ON:        // <strong>
            mpCurrTable->PutItem( SvxWeightItem( WEIGHT_BOLD, ATTR_FONT_WEIGHT ) );
        break;

        case HTML_ITALIC_ON:        // <i>
        case HTML_EMPHASIS_ON:      // <em>
        case HTML_ADDRESS_ON:       // <address>
        case HTML_BLOCKQUOTE_ON:    // <blockquote>
        case HTML_BLOCKQUOTE30_ON:  // <bq>
        case HTML_CITIATION_ON:     // <cite>
        case HTML_VARIABLE_ON:      // <var>
            mpCurrTable->PutItem( SvxPostureItem( ITALIC_NORMAL, ATTR_FONT_POSTURE ) );
        break;

        case HTML_DEFINSTANCE_ON:   // <dfn>
            mpCurrTable->PutItem( SvxWeightItem( WEIGHT_BOLD, ATTR_FONT_WEIGHT ) );
            mpCurrTable->PutItem( SvxPostureItem( ITALIC_NORMAL, ATTR_FONT_POSTURE ) );
        break;

        case HTML_UNDERLINE_ON:     // <u>
            mpCurrTable->PutItem( SvxUnderlineItem( UNDERLINE_SINGLE, ATTR_FONT_UNDERLINE ) );
        break;
    }
}

void ScHTMLQueryParser::InsertText( const ImportInfo& rInfo )
{
    mpCurrTable->PutText( rInfo );
    if( mbTitleOn )
        maTitle.Append( rInfo.aText );
}

void ScHTMLQueryParser::FontOn( const ImportInfo& rInfo )
{
    for( ScHTMLOptionIterator aIter( rInfo ); aIter.is(); ++aIter )
    {
        switch( aIter->GetToken() )
        {
            case HTML_O_FACE :
            {
                const String& rFace = aIter->GetString();
                String aFontName;
                xub_StrLen nPos = 0;
                while( nPos != STRING_NOTFOUND )
                {
                    // font list separator: VCL = ';' HTML = ','
                    String aFName = rFace.GetToken( 0, ',', nPos );
                    aFName.EraseLeadingAndTrailingChars();
                    ScGlobal::AddToken( aFontName, aFName, ';' );
                }
                if ( aFontName.Len() )
                    mpCurrTable->PutItem( SvxFontItem( FAMILY_DONTKNOW,
                        aFontName, EMPTY_STRING, PITCH_DONTKNOW,
                        RTL_TEXTENCODING_DONTKNOW, ATTR_FONT ) );
            }
            break;
            case HTML_O_SIZE :
            {
                sal_uInt32 nSize = getLimitedValue< sal_uInt32 >( aIter->GetNumber(), 1, SC_HTML_FONTSIZES );
                mpCurrTable->PutItem( SvxFontHeightItem( maFontHeights[ nSize - 1 ], 100, ATTR_FONT_HEIGHT ) );
            }
            break;
            case HTML_O_COLOR :
            {
                Color aColor;
                aIter->GetColor( aColor );
                mpCurrTable->PutItem( SvxColorItem( aColor, ATTR_FONT_COLOR ) );
            }
            break;
        }
    }
}

void ScHTMLQueryParser::MetaOn( const ImportInfo& rInfo )
{
    if( mpDoc->GetDocumentShell() )
    {
        HTMLParser* pParser = static_cast< HTMLParser* >( rInfo.pParser );

        uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
            mpDoc->GetDocumentShell()->GetModel(), uno::UNO_QUERY_THROW);
        pParser->ParseMetaOptions(
            xDPS->getDocumentProperties(),
            mpDoc->GetDocumentShell()->GetHeaderAttributes() );
    }
}

void ScHTMLQueryParser::TitleOn( const ImportInfo& /*rInfo*/ )
{
    mbTitleOn = true;
    maTitle.Erase();
}

void ScHTMLQueryParser::TitleOff( const ImportInfo& rInfo )
{
    if( mbTitleOn )
    {
        maTitle.EraseLeadingAndTrailingChars();
        if( maTitle.Len() && mpDoc->GetDocumentShell() ) {
            uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                mpDoc->GetDocumentShell()->GetModel(), uno::UNO_QUERY_THROW);

            xDPS->getDocumentProperties()->setTitle(maTitle);
        }
        InsertText( rInfo );
        mbTitleOn = false;
    }
}

void ScHTMLQueryParser::TableOn( const ImportInfo& rInfo )
{
    mpCurrTable = mpCurrTable->TableOn( rInfo );
}

void ScHTMLQueryParser::TableOff( const ImportInfo& rInfo )
{
    mpCurrTable = mpCurrTable->TableOff( rInfo );
}

void ScHTMLQueryParser::PreOn( const ImportInfo& rInfo )
{
    mpCurrTable = mpCurrTable->PreOn( rInfo );
}

void ScHTMLQueryParser::PreOff( const ImportInfo& rInfo )
{
    mpCurrTable = mpCurrTable->PreOff( rInfo );
}

void ScHTMLQueryParser::CloseTable( const ImportInfo& rInfo )
{
    mpCurrTable = mpCurrTable->CloseTable( rInfo );
}

// ----------------------------------------------------------------------------

IMPL_LINK( ScHTMLQueryParser, HTMLImportHdl, const ImportInfo*, pInfo )
{
    switch( pInfo->eState )
    {
        case HTMLIMP_START:
        break;

        case HTMLIMP_NEXTTOKEN:
        case HTMLIMP_UNKNOWNATTR:
            ProcessToken( *pInfo );
        break;

        case HTMLIMP_INSERTPARA:
            mpCurrTable->InsertPara( *pInfo );
        break;

        case HTMLIMP_SETATTR:
        case HTMLIMP_INSERTTEXT:
        case HTMLIMP_INSERTFIELD:
        break;

        case HTMLIMP_END:
            while( mpCurrTable->GetTableId() != SC_HTML_GLOBAL_TABLE )
                CloseTable( *pInfo );
        break;

        default:
            DBG_ERRORFILE( "ScHTMLQueryParser::HTMLImportHdl - unknown ImportInfo::eState" );
    }
    return 0;
}

// ============================================================================

