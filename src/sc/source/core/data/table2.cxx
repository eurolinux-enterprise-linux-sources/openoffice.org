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

// INCLUDE ---------------------------------------------------------------

#include "scitems.hxx"
#include <svx/boxitem.hxx>
#include <tools/urlobj.hxx>
#include <svtools/poolcach.hxx>
#include <unotools/charclass.hxx>
#include <math.h>
#include <svtools/PasswordHelper.hxx>
#include <unotools/transliterationwrapper.hxx>

#include "patattr.hxx"
#include "docpool.hxx"
#include "cell.hxx"
#include "document.hxx"
#include "drwlayer.hxx"
#include "olinetab.hxx"
#include "rechead.hxx"
#include "stlpool.hxx"
#include "attarray.hxx"		// Iterator
#include "markdata.hxx"
#include "progress.hxx"
#include "dociter.hxx"
#include "conditio.hxx"
#include "chartlis.hxx"
#include "fillinfo.hxx"
#include "bcaslot.hxx"
#include "postit.hxx"
#include "globstr.hrc"

#include <math.h>

// STATIC DATA -----------------------------------------------------------


BOOL ScTable::SetOutlineTable( const ScOutlineTable* pNewOutline )
{
	USHORT nOldSizeX = 0;
	USHORT nOldSizeY = 0;
	USHORT nNewSizeX = 0;
	USHORT nNewSizeY = 0;

	if (pOutlineTable)
	{
		nOldSizeX = pOutlineTable->GetColArray()->GetDepth();
		nOldSizeY = pOutlineTable->GetRowArray()->GetDepth();
		delete pOutlineTable;
	}

	if (pNewOutline)
	{
		pOutlineTable = new ScOutlineTable( *pNewOutline );
		nNewSizeX = pOutlineTable->GetColArray()->GetDepth();
		nNewSizeY = pOutlineTable->GetRowArray()->GetDepth();
	}
	else
		pOutlineTable = NULL;

	return ( nNewSizeX != nOldSizeX || nNewSizeY != nOldSizeY );		// Groesse geaendert ?
}


void ScTable::StartOutlineTable()
{
	if (!pOutlineTable)
		pOutlineTable = new ScOutlineTable;
}


BOOL ScTable::TestInsertRow( SCCOL nStartCol, SCCOL nEndCol, SCSIZE nSize )
{
	BOOL bTest = TRUE;

	if ( nStartCol==0 && nEndCol==MAXCOL && pOutlineTable )
		bTest = pOutlineTable->TestInsertRow(nSize);

	for (SCCOL i=nStartCol; (i<=nEndCol) && bTest; i++)
		bTest = aCol[i].TestInsertRow( nSize );

	return bTest;
}


void ScTable::InsertRow( SCCOL nStartCol, SCCOL nEndCol, SCROW nStartRow, SCSIZE nSize )
{
	IncRecalcLevel();
    InitializeNoteCaptions();
	if (nStartCol==0 && nEndCol==MAXCOL)
	{
		if (pRowHeight && pRowFlags)
		{
            pRowHeight->Insert( nStartRow, nSize);
            BYTE nNewFlags = pRowFlags->Insert( nStartRow, nSize);
            // only copy manual size flag, clear all others
            if (nNewFlags && (nNewFlags != CR_MANUALSIZE))
                pRowFlags->SetValue( nStartRow, nStartRow + nSize - 1,
                        nNewFlags & CR_MANUALSIZE);
		}
		if (pOutlineTable)
			pOutlineTable->InsertRow( nStartRow, nSize );
	}

	for (SCCOL j=nStartCol; j<=nEndCol; j++)
		aCol[j].InsertRow( nStartRow, nSize );
	DecRecalcLevel( false );
}


void ScTable::DeleteRow( SCCOL nStartCol, SCCOL nEndCol, SCROW nStartRow, SCSIZE nSize,
							BOOL* pUndoOutline )
{
	IncRecalcLevel();
    InitializeNoteCaptions();
	if (nStartCol==0 && nEndCol==MAXCOL)
	{
		if (pRowHeight && pRowFlags)
        {
            pRowHeight->Remove( nStartRow, nSize);
            pRowFlags->Remove( nStartRow, nSize);
        }
		if (pOutlineTable)
			if (pOutlineTable->DeleteRow( nStartRow, nSize ))
				if (pUndoOutline)
					*pUndoOutline = TRUE;
	}

    {   // scope for bulk broadcast
        ScBulkBroadcast aBulkBroadcast( pDocument->GetBASM());
        for (SCCOL j=nStartCol; j<=nEndCol; j++)
            aCol[j].DeleteRow( nStartRow, nSize );
    }
	DecRecalcLevel();
}


BOOL ScTable::TestInsertCol( SCROW nStartRow, SCROW nEndRow, SCSIZE nSize )
{
	BOOL bTest = TRUE;

	if ( nStartRow==0 && nEndRow==MAXROW && pOutlineTable )
		bTest = pOutlineTable->TestInsertCol(nSize);

	if ( nSize > static_cast<SCSIZE>(MAXCOL) )
		bTest = FALSE;

	for (SCCOL i=MAXCOL; (i+static_cast<SCCOL>(nSize)>MAXCOL) && bTest; i--)
		bTest = aCol[i].TestInsertCol(nStartRow, nEndRow);

	return bTest;
}


void ScTable::InsertCol( SCCOL nStartCol, SCROW nStartRow, SCROW nEndRow, SCSIZE nSize )
{
	IncRecalcLevel();
    InitializeNoteCaptions();
	if (nStartRow==0 && nEndRow==MAXROW)
	{
		if (pColWidth && pColFlags)
        {
            memmove( &pColWidth[nStartCol+nSize], &pColWidth[nStartCol],
                    (MAXCOL - nStartCol + 1 - nSize) * sizeof(pColWidth[0]) );
            memmove( &pColFlags[nStartCol+nSize], &pColFlags[nStartCol],
                    (MAXCOL - nStartCol + 1 - nSize) * sizeof(pColFlags[0]) );
        }
		if (pOutlineTable)
			pOutlineTable->InsertCol( nStartCol, nSize );
	}


	if ((nStartRow == 0) && (nEndRow == MAXROW))
	{
		for (SCSIZE i=0; i < nSize; i++)
			for (SCCOL nCol = MAXCOL; nCol > nStartCol; nCol--)
				aCol[nCol].SwapCol(aCol[nCol-1]);
	}
	else
	{
		for (SCSIZE i=0; static_cast<SCCOL>(i+nSize)+nStartCol <= MAXCOL; i++)
			aCol[MAXCOL - nSize - i].MoveTo(nStartRow, nEndRow, aCol[MAXCOL - i]);
	}

	if (nStartCol>0)						// copy old attributes
	{
		USHORT nWhichArray[2];
		nWhichArray[0] = ATTR_MERGE;
		nWhichArray[1] = 0;

		for (SCSIZE i=0; i<nSize; i++)
		{
			aCol[nStartCol-1].CopyToColumn( nStartRow, nEndRow, IDF_ATTRIB,
												FALSE, aCol[nStartCol+i] );
			aCol[nStartCol+i].RemoveFlags( nStartRow, nEndRow,
												SC_MF_HOR | SC_MF_VER | SC_MF_AUTO );
			aCol[nStartCol+i].ClearItems( nStartRow, nEndRow, nWhichArray );
		}
	}
	DecRecalcLevel();
}


void ScTable::DeleteCol( SCCOL nStartCol, SCROW nStartRow, SCROW nEndRow, SCSIZE nSize,
							BOOL* pUndoOutline )
{
	IncRecalcLevel();
    InitializeNoteCaptions();
	if (nStartRow==0 && nEndRow==MAXROW)
	{
		if (pColWidth && pColFlags)
        {
            memmove( &pColWidth[nStartCol], &pColWidth[nStartCol+nSize],
                    (MAXCOL - nStartCol + 1 - nSize) * sizeof(pColWidth[0]) );
            memmove( &pColFlags[nStartCol], &pColFlags[nStartCol+nSize],
                    (MAXCOL - nStartCol + 1 - nSize) * sizeof(pColFlags[0]) );
        }
		if (pOutlineTable)
			if (pOutlineTable->DeleteCol( nStartCol, nSize ))
				if (pUndoOutline)
					*pUndoOutline = TRUE;
	}


    {   // scope for bulk broadcast
        ScBulkBroadcast aBulkBroadcast( pDocument->GetBASM());
        for (SCSIZE i = 0; i < nSize; i++)
            aCol[nStartCol + i].DeleteArea(nStartRow, nEndRow, IDF_ALL);
    }

	if ((nStartRow == 0) && (nEndRow == MAXROW))
	{
		for (SCSIZE i=0; i < nSize; i++)
			for (SCCOL nCol = nStartCol; nCol < MAXCOL; nCol++)
				aCol[nCol].SwapCol(aCol[nCol+1]);
	}
	else
	{
		for (SCSIZE i=0; static_cast<SCCOL>(i+nSize)+nStartCol <= MAXCOL; i++)
			aCol[nStartCol + nSize + i].MoveTo(nStartRow, nEndRow, aCol[nStartCol + i]);
	}
	DecRecalcLevel();
}


void ScTable::DeleteArea(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, USHORT nDelFlag)
{
	if (nCol2 > MAXCOL) nCol2 = MAXCOL;
	if (nRow2 > MAXROW) nRow2 = MAXROW;
	if (ValidColRow(nCol1, nRow1) && ValidColRow(nCol2, nRow2))
	{
//		IncRecalcLevel();

        {   // scope for bulk broadcast
            ScBulkBroadcast aBulkBroadcast( pDocument->GetBASM());
            for (SCCOL i = nCol1; i <= nCol2; i++)
                aCol[i].DeleteArea(nRow1, nRow2, nDelFlag);
        }

			//
			// Zellschutz auf geschuetzter Tabelle nicht setzen
			//

		if ( IsProtected() && (nDelFlag & IDF_ATTRIB) )
		{
			ScPatternAttr aPattern(pDocument->GetPool());
			aPattern.GetItemSet().Put( ScProtectionAttr( FALSE ) );
			ApplyPatternArea( nCol1, nRow1, nCol2, nRow2, aPattern );
		}

//		DecRecalcLevel();
	}
}


void ScTable::DeleteSelection( USHORT nDelFlag, const ScMarkData& rMark )
{
    {   // scope for bulk broadcast
        ScBulkBroadcast aBulkBroadcast( pDocument->GetBASM());
        for (SCCOL i=0; i<=MAXCOL; i++)
            aCol[i].DeleteSelection( nDelFlag, rMark );
    }

		//
		// Zellschutz auf geschuetzter Tabelle nicht setzen
		//

	if ( IsProtected() && (nDelFlag & IDF_ATTRIB) )
	{
		ScDocumentPool* pPool = pDocument->GetPool();
		SfxItemSet aSet( *pPool, ATTR_PATTERN_START, ATTR_PATTERN_END );
		aSet.Put( ScProtectionAttr( FALSE ) );
		SfxItemPoolCache aCache( pPool, &aSet );
		ApplySelectionCache( &aCache, rMark );
	}
}


//	pTable = Clipboard
void ScTable::CopyToClip(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
                        ScTable* pTable, BOOL bKeepScenarioFlags, BOOL bCloneNoteCaptions)
{
	if (ValidColRow(nCol1, nRow1) && ValidColRow(nCol2, nRow2))
	{
		//	Inhalte kopieren
		SCCOL i;

		for ( i = nCol1; i <= nCol2; i++)
            aCol[i].CopyToClip(nRow1, nRow2, pTable->aCol[i], bKeepScenarioFlags, bCloneNoteCaptions);

		//	copy widths/heights, and only "hidden", "filtered" and "manual" flags
		//	also for all preceding columns/rows, to have valid positions for drawing objects

		if (pColFlags && pTable->pColFlags && pColWidth && pTable->pColWidth)
			for (i=0; i<=nCol2; i++)
			{
				pTable->pColFlags[i] = pColFlags[i] & CR_HIDDEN;
				pTable->pColWidth[i] = pColWidth[i];
			}

		if (pRowFlags && pTable->pRowFlags && pRowHeight && pTable->pRowHeight)
        {
            pTable->pRowFlags->CopyFromAnded( *pRowFlags, 0, nRow2,
                    (CR_HIDDEN | CR_FILTERED | CR_MANUALSIZE));
            pTable->pRowHeight->CopyFrom( *pRowHeight, 0, nRow2);
        }


		//	ggf. Formeln durch Werte ersetzen

		if ( IsProtected() )
			for (i = nCol1; i <= nCol2; i++)
				pTable->aCol[i].RemoveProtected(nRow1, nRow2);
	}
}

void ScTable::CopyToClip(const ScRangeList& rRanges, ScTable* pTable, 
                         bool bKeepScenarioFlags, bool bCloneNoteCaptions)
{
    ScRangeList aRanges(rRanges);
    for (ScRangePtr p = aRanges.First(); p; p = aRanges.Next())
    {
        CopyToClip(p->aStart.Col(), p->aStart.Row(), p->aEnd.Col(), p->aEnd.Row(), 
                   pTable, bKeepScenarioFlags, bCloneNoteCaptions);
    }
}

void ScTable::CopyFromClip(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
							SCsCOL nDx, SCsROW nDy, USHORT nInsFlag,
							BOOL bAsLink, BOOL bSkipAttrForEmpty, ScTable* pTable)
{
	SCCOL i;

	if (nCol2 > MAXCOL) nCol2 = MAXCOL;
	if (nRow2 > MAXROW) nRow2 = MAXROW;
	if (ValidColRow(nCol1, nRow1) && ValidColRow(nCol2, nRow2))
	{
		IncRecalcLevel();
		for ( i = nCol1; i <= nCol2; i++)
			aCol[i].CopyFromClip(nRow1, nRow2, nDy, nInsFlag, bAsLink, bSkipAttrForEmpty, pTable->aCol[i - nDx]);

		if ((nInsFlag & IDF_ATTRIB) != 0)
		{
			if (nRow1==0 && nRow2==MAXROW && pColWidth && pTable->pColWidth)
				for (i=nCol1; i<=nCol2; i++)
					pColWidth[i] = pTable->pColWidth[i-nDx];

			if (nCol1==0 && nCol2==MAXCOL && pRowHeight && pTable->pRowHeight &&
											 pRowFlags && pTable->pRowFlags)
            {
                pRowHeight->CopyFrom( *pTable->pRowHeight, nRow1, nRow2, -nDy);
                // Must copy CR_MANUALSIZE bit too, otherwise pRowHeight doesn't make sense
				for (SCROW j=nRow1; j<=nRow2; j++)
				{
					if ( pTable->pRowFlags->GetValue(j-nDy) & CR_MANUALSIZE )
						pRowFlags->OrValue( j, CR_MANUALSIZE);
					else
                        pRowFlags->AndValue( j, sal::static_int_cast<BYTE>(~CR_MANUALSIZE));
				}
            }

				//
				// Zellschutz auf geschuetzter Tabelle nicht setzen
				//

			if ( IsProtected() && (nInsFlag & IDF_ATTRIB) )
			{
				ScPatternAttr aPattern(pDocument->GetPool());
				aPattern.GetItemSet().Put( ScProtectionAttr( FALSE ) );
				ApplyPatternArea( nCol1, nRow1, nCol2, nRow2, aPattern );
			}
		}
		DecRecalcLevel();
	}
}


void ScTable::MixData( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
							USHORT nFunction, BOOL bSkipEmpty, ScTable* pSrcTab )
{
	for (SCCOL i=nCol1; i<=nCol2; i++)
		aCol[i].MixData( nRow1, nRow2, nFunction, bSkipEmpty, pSrcTab->aCol[i] );
}


//	Markierung von diesem Dokument
void ScTable::MixMarked( const ScMarkData& rMark, USHORT nFunction,
						BOOL bSkipEmpty, ScTable* pSrcTab )
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].MixMarked( rMark, nFunction, bSkipEmpty, pSrcTab->aCol[i] );
}


void ScTable::TransposeClip( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
								ScTable* pTransClip, USHORT nFlags, BOOL bAsLink )
{
	BOOL bWasCut = pDocument->IsCutMode();

	ScDocument* pDestDoc = pTransClip->pDocument;

	for (SCCOL nCol=nCol1; nCol<=nCol2; nCol++)
	{
		SCROW nRow;
		ScBaseCell* pCell;

		if ( bAsLink && nFlags == IDF_ALL )
		{
			//	#68989# with IDF_ALL, also create links (formulas) for empty cells

			for ( nRow=nRow1; nRow<=nRow2; nRow++ )
			{
				//	create simple formula, as in ScColumn::CreateRefCell

				ScAddress aDestPos( static_cast<SCCOL>(nRow-nRow1), static_cast<SCROW>(nCol-nCol1), pTransClip->nTab );
				ScSingleRefData aRef;
				aRef.nCol = nCol;
				aRef.nRow = nRow;
				aRef.nTab = nTab;
				aRef.InitFlags();							// -> all absolute
				aRef.SetFlag3D(TRUE);
				aRef.CalcRelFromAbs( aDestPos );
				ScTokenArray aArr;
				aArr.AddSingleReference( aRef );

				ScBaseCell* pNew = new ScFormulaCell( pDestDoc, aDestPos, &aArr );
				pTransClip->PutCell( static_cast<SCCOL>(nRow-nRow1), static_cast<SCROW>(nCol-nCol1), pNew );
			}
		}
		else
		{
			ScColumnIterator aIter( &aCol[nCol], nRow1, nRow2 );
			while (aIter.Next( nRow, pCell ))
			{
                ScAddress aDestPos( static_cast<SCCOL>(nRow-nRow1), static_cast<SCROW>(nCol-nCol1), pTransClip->nTab );
				ScBaseCell* pNew;
				if ( bAsLink )					// Referenz erzeugen ?
				{
					pNew = aCol[nCol].CreateRefCell( pDestDoc, aDestPos, aIter.GetIndex(), nFlags );
				}
				else							// kopieren
				{
                    ScAddress aOwnPos( nCol, nRow, nTab );
					if (pCell->GetCellType() == CELLTYPE_FORMULA)
					{
                        pNew = pCell->CloneWithNote( aOwnPos, *pDestDoc, aDestPos, SC_CLONECELL_STARTLISTENING );

						//	Referenzen drehen
						//	bei Cut werden Referenzen spaeter per UpdateTranspose angepasst

						if (!bWasCut)
							((ScFormulaCell*)pNew)->TransposeReference();
					}
					else
                    {
                        pNew = pCell->CloneWithNote( aOwnPos, *pDestDoc, aDestPos );
                    }
				}
				pTransClip->PutCell( static_cast<SCCOL>(nRow-nRow1), static_cast<SCROW>(nCol-nCol1), pNew );
			}
		}

		//	Attribute

		SCROW nAttrRow1;
		SCROW nAttrRow2;
		const ScPatternAttr* pPattern;
		ScAttrIterator* pAttrIter = aCol[nCol].CreateAttrIterator( nRow1, nRow2 );
		while ( (pPattern = pAttrIter->Next( nAttrRow1, nAttrRow2 )) != 0 )
		{
			if ( !IsDefaultItem( pPattern ) )
			{
				const SfxItemSet& rSet = pPattern->GetItemSet();
				if ( rSet.GetItemState( ATTR_MERGE, FALSE ) == SFX_ITEM_DEFAULT &&
					 rSet.GetItemState( ATTR_MERGE_FLAG, FALSE ) == SFX_ITEM_DEFAULT &&
					 rSet.GetItemState( ATTR_BORDER, FALSE ) == SFX_ITEM_DEFAULT )
				{
					// no borders or merge items involved - use pattern as-is
					for (nRow = nAttrRow1; nRow<=nAttrRow2; nRow++)
						pTransClip->SetPattern( static_cast<SCCOL>(nRow-nRow1), static_cast<SCROW>(nCol-nCol1), *pPattern, TRUE );
				}
				else
				{
					// transpose borders and merge values, remove merge flags (refreshed after pasting)
					ScPatternAttr aNewPattern( *pPattern );
					SfxItemSet& rNewSet = aNewPattern.GetItemSet();

					const SvxBoxItem& rOldBox = (const SvxBoxItem&)rSet.Get(ATTR_BORDER);
					if ( rOldBox.GetTop() || rOldBox.GetBottom() || rOldBox.GetLeft() || rOldBox.GetRight() )
					{
						SvxBoxItem aNew( ATTR_BORDER );
						aNew.SetLine( rOldBox.GetLine( BOX_LINE_TOP ), BOX_LINE_LEFT );
						aNew.SetLine( rOldBox.GetLine( BOX_LINE_LEFT ), BOX_LINE_TOP );
						aNew.SetLine( rOldBox.GetLine( BOX_LINE_BOTTOM ), BOX_LINE_RIGHT );
						aNew.SetLine( rOldBox.GetLine( BOX_LINE_RIGHT ), BOX_LINE_BOTTOM );
						aNew.SetDistance( rOldBox.GetDistance( BOX_LINE_TOP ), BOX_LINE_LEFT );
						aNew.SetDistance( rOldBox.GetDistance( BOX_LINE_LEFT ), BOX_LINE_TOP );
						aNew.SetDistance( rOldBox.GetDistance( BOX_LINE_BOTTOM ), BOX_LINE_RIGHT );
						aNew.SetDistance( rOldBox.GetDistance( BOX_LINE_RIGHT ), BOX_LINE_BOTTOM );
						rNewSet.Put( aNew );
					}

					const ScMergeAttr& rOldMerge = (const ScMergeAttr&)rSet.Get(ATTR_MERGE);
					if (rOldMerge.IsMerged())
                        rNewSet.Put( ScMergeAttr( Min(
                                        static_cast<SCsCOL>(rOldMerge.GetRowMerge()),
                                        static_cast<SCsCOL>(MAXCOL+1 - (nAttrRow2-nRow1))),
                                    Min(
                                        static_cast<SCsROW>(rOldMerge.GetColMerge()),
                                        static_cast<SCsROW>(MAXROW+1 - (nCol-nCol1)))));
					const ScMergeFlagAttr& rOldFlag = (const ScMergeFlagAttr&)rSet.Get(ATTR_MERGE_FLAG);
					if (rOldFlag.IsOverlapped())
					{
						INT16 nNewFlags = rOldFlag.GetValue() & ~( SC_MF_HOR | SC_MF_VER );
						if ( nNewFlags )
							rNewSet.Put( ScMergeFlagAttr( nNewFlags ) );
						else
							rNewSet.ClearItem( ATTR_MERGE_FLAG );
					}

					for (nRow = nAttrRow1; nRow<=nAttrRow2; nRow++)
                        pTransClip->SetPattern( static_cast<SCCOL>(nRow-nRow1),
                                static_cast<SCROW>(nCol-nCol1), aNewPattern, TRUE);
				}
			}
		}

		delete pAttrIter;
	}
}


void ScTable::StartAllListeners()
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].StartAllListeners();
}


void ScTable::StartNeededListeners()
{
	for (SCCOL i=0; i<=MAXCOL; i++)
        aCol[i].StartNeededListeners();
}


void ScTable::BroadcastInArea( SCCOL nCol1, SCROW nRow1,
		SCCOL nCol2, SCROW nRow2 )
{
	if (nCol2 > MAXCOL) nCol2 = MAXCOL;
	if (nRow2 > MAXROW) nRow2 = MAXROW;
	if (ValidColRow(nCol1, nRow1) && ValidColRow(nCol2, nRow2))
		for (SCCOL i = nCol1; i <= nCol2; i++)
			aCol[i].BroadcastInArea( nRow1, nRow2 );
}


void ScTable::StartListeningInArea( SCCOL nCol1, SCROW nRow1,
		SCCOL nCol2, SCROW nRow2 )
{
	if (nCol2 > MAXCOL) nCol2 = MAXCOL;
	if (nRow2 > MAXROW) nRow2 = MAXROW;
	if (ValidColRow(nCol1, nRow1) && ValidColRow(nCol2, nRow2))
		for (SCCOL i = nCol1; i <= nCol2; i++)
			aCol[i].StartListeningInArea( nRow1, nRow2 );
}


void ScTable::CopyToTable(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
							USHORT nFlags, BOOL bMarked, ScTable* pDestTab,
							const ScMarkData* pMarkData,
							BOOL bAsLink, BOOL bColRowFlags)
{
	if (ValidColRow(nCol1, nRow1) && ValidColRow(nCol2, nRow2))
	{
		if (nFlags)
			for (SCCOL i = nCol1; i <= nCol2; i++)
				aCol[i].CopyToColumn(nRow1, nRow2, nFlags, bMarked,
								pDestTab->aCol[i], pMarkData, bAsLink);

		if (bColRowFlags)		// Spaltenbreiten/Zeilenhoehen/Flags
		{
			//	Charts muessen beim Ein-/Ausblenden angepasst werden
			ScChartListenerCollection* pCharts = pDestTab->pDocument->GetChartListenerCollection();

			BOOL bWidth  = (nRow1==0 && nRow2==MAXROW && pColWidth && pDestTab->pColWidth);
			BOOL bHeight = (nCol1==0 && nCol2==MAXCOL && pRowHeight && pDestTab->pRowHeight);

			if (bWidth||bHeight)
			{
				pDestTab->IncRecalcLevel();

				if (bWidth)
					for (SCCOL i=nCol1; i<=nCol2; i++)
					{
						BOOL bChange = pCharts &&
							( pDestTab->pColFlags[i] & CR_HIDDEN ) != ( pColFlags[i] & CR_HIDDEN );
						pDestTab->pColWidth[i] = pColWidth[i];
						pDestTab->pColFlags[i] = pColFlags[i];
						//!	Aenderungen zusammenfassen?
						if (bChange)
							pCharts->SetRangeDirty(ScRange( i, 0, nTab, i, MAXROW, nTab ));
					}

				if (bHeight)
				{
					pDestTab->pRowHeight->CopyFrom( *pRowHeight, nRow1, nRow2);
					for (SCROW i=nRow1; i<=nRow2; i++)
					{
						// TODO: might need some performance improvement, block
						// operations instead of single GetValue()/SetValue() calls.
						BYTE nThisRowFlags = pRowFlags->GetValue(i);
						BOOL bChange = pCharts &&
							( pDestTab->pRowFlags->GetValue(i) & CR_HIDDEN ) != ( nThisRowFlags & CR_HIDDEN );
						pDestTab->pRowFlags->SetValue( i, nThisRowFlags );
						//!	Aenderungen zusammenfassen?
						if (bChange)
							pCharts->SetRangeDirty(ScRange( 0, i, nTab, MAXCOL, i, nTab ));
					}
				}
				pDestTab->DecRecalcLevel();
			}
			pDestTab->SetOutlineTable( pOutlineTable );		// auch nur wenn bColRowFlags
		}
	}
}


void ScTable::UndoToTable(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
							USHORT nFlags, BOOL bMarked, ScTable* pDestTab,
							const ScMarkData* pMarkData)
{
	if (ValidColRow(nCol1, nRow1) && ValidColRow(nCol2, nRow2))
	{
		BOOL bWidth  = (nRow1==0 && nRow2==MAXROW && pColWidth && pDestTab->pColWidth);
		BOOL bHeight = (nCol1==0 && nCol2==MAXCOL && pRowHeight && pDestTab->pRowHeight);

		if (bWidth||bHeight)
			IncRecalcLevel();

		for ( SCCOL i = 0; i <= MAXCOL; i++)
		{
			if ( i >= nCol1 && i <= nCol2 )
				aCol[i].UndoToColumn(nRow1, nRow2, nFlags, bMarked, pDestTab->aCol[i],
										pMarkData);
			else
				aCol[i].CopyToColumn(0, MAXROW, IDF_FORMULA, FALSE, pDestTab->aCol[i]);
		}

		if (bWidth||bHeight)
		{
			if (bWidth)
				for (SCCOL i=nCol1; i<=nCol2; i++)
					pDestTab->pColWidth[i] = pColWidth[i];
			if (bHeight)
                pDestTab->pRowHeight->CopyFrom( *pRowHeight, nRow1, nRow2);
			DecRecalcLevel();
		}
	}
}


void ScTable::CopyUpdated( const ScTable* pPosTab, ScTable* pDestTab ) const
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].CopyUpdated( pPosTab->aCol[i], pDestTab->aCol[i] );
}

void ScTable::CopyScenarioTo( ScTable* pDestTab ) const
{
	DBG_ASSERT( bScenario, "bScenario == FALSE" );

	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].CopyScenarioTo( pDestTab->aCol[i] );
}

void ScTable::CopyScenarioFrom( const ScTable* pSrcTab )
{
	DBG_ASSERT( bScenario, "bScenario == FALSE" );

	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].CopyScenarioFrom( pSrcTab->aCol[i] );
}

void ScTable::MarkScenarioIn( ScMarkData& rDestMark, USHORT nNeededBits ) const
{
	DBG_ASSERT( bScenario, "bScenario == FALSE" );

	if ( ( nScenarioFlags & nNeededBits ) != nNeededBits )	// alle Bits gesetzt?
		return;

	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].MarkScenarioIn( rDestMark );
}

BOOL ScTable::HasScenarioRange( const ScRange& rRange ) const
{
	DBG_ASSERT( bScenario, "bScenario == FALSE" );

//	ScMarkData aMark;
//	MarkScenarioIn( aMark, 0 );				//! Bits als Parameter von HasScenarioRange?
//	return aMark.IsAllMarked( rRange );

	ScRange aTabRange = rRange;
	aTabRange.aStart.SetTab( nTab );
	aTabRange.aEnd.SetTab( nTab );

	const ScRangeList* pList = GetScenarioRanges();
//	return ( pList && pList->Find( aTabRange ) );

	if (pList)
	{
		ULONG nCount = pList->Count();
		for ( ULONG j = 0; j < nCount; j++ )
		{
			ScRange* pR = pList->GetObject( j );
			if ( pR->Intersects( aTabRange ) )
				return TRUE;
		}
	}

	return FALSE;
}

void ScTable::InvalidateScenarioRanges()
{
	delete pScenarioRanges;
	pScenarioRanges = NULL;
}

const ScRangeList* ScTable::GetScenarioRanges() const
{
	DBG_ASSERT( bScenario, "bScenario == FALSE" );

	if (!pScenarioRanges)
	{
		((ScTable*)this)->pScenarioRanges = new ScRangeList;
		ScMarkData aMark;
		MarkScenarioIn( aMark, 0 );		// immer
		aMark.FillRangeListWithMarks( pScenarioRanges, FALSE );
	}
	return pScenarioRanges;
}

BOOL ScTable::TestCopyScenarioTo( const ScTable* pDestTab ) const
{
	DBG_ASSERT( bScenario, "bScenario == FALSE" );

	if (!pDestTab->IsProtected())
		return TRUE;

	BOOL bOk = TRUE;
	for (SCCOL i=0; i<=MAXCOL && bOk; i++)
		bOk = aCol[i].TestCopyScenarioTo( pDestTab->aCol[i] );
	return bOk;
}

void ScTable::PutCell( SCCOL nCol, SCROW nRow, ScBaseCell* pCell )
{
	if (ValidColRow(nCol,nRow))
	{
		if (pCell)
			aCol[nCol].Insert( nRow, pCell );
		else
			aCol[nCol].Delete( nRow );
	}
}


void ScTable::PutCell( SCCOL nCol, SCROW nRow, ULONG nFormatIndex, ScBaseCell* pCell )
{
	if (ValidColRow(nCol,nRow))
	{
		if (pCell)
			aCol[nCol].Insert( nRow, nFormatIndex, pCell );
		else
			aCol[nCol].Delete( nRow );
	}
}


void ScTable::PutCell( const ScAddress& rPos, ScBaseCell* pCell )
{
	if (pCell)
		aCol[rPos.Col()].Insert( rPos.Row(), pCell );
	else
		aCol[rPos.Col()].Delete( rPos.Row() );
}


//UNUSED2009-05 void ScTable::PutCell( const ScAddress& rPos, ULONG nFormatIndex, ScBaseCell* pCell )
//UNUSED2009-05 {
//UNUSED2009-05     if (pCell)
//UNUSED2009-05         aCol[rPos.Col()].Insert( rPos.Row(), nFormatIndex, pCell );
//UNUSED2009-05     else
//UNUSED2009-05         aCol[rPos.Col()].Delete( rPos.Row() );
//UNUSED2009-05 }


BOOL ScTable::SetString( SCCOL nCol, SCROW nRow, SCTAB nTabP, const String& rString )
{
	if (ValidColRow(nCol,nRow))
        return aCol[nCol].SetString( nRow, nTabP, rString );
	else
		return FALSE;
}


void ScTable::SetValue( SCCOL nCol, SCROW nRow, const double& rVal )
{
	if (ValidColRow(nCol, nRow))
		aCol[nCol].SetValue( nRow, rVal );
}


void ScTable::GetString( SCCOL nCol, SCROW nRow, String& rString )
{
	if (ValidColRow(nCol,nRow))
		aCol[nCol].GetString( nRow, rString );
	else
		rString.Erase();
}


void ScTable::GetInputString( SCCOL nCol, SCROW nRow, String& rString )
{
	if (ValidColRow(nCol,nRow))
		aCol[nCol].GetInputString( nRow, rString );
	else
		rString.Erase();
}


double ScTable::GetValue( SCCOL nCol, SCROW nRow )
{
	if (ValidColRow( nCol, nRow ))
		return aCol[nCol].GetValue( nRow );
	return 0.0;
}


void ScTable::GetFormula( SCCOL nCol, SCROW nRow, String& rFormula,
						  BOOL bAsciiExport )
{
	if (ValidColRow(nCol,nRow))
		aCol[nCol].GetFormula( nRow, rFormula, bAsciiExport );
	else
		rFormula.Erase();
}


ScPostIt* ScTable::GetNote( SCCOL nCol, SCROW nRow )
{
	return ValidColRow( nCol, nRow ) ? aCol[ nCol ].GetNote( nRow ) : 0;
}


void ScTable::TakeNote( SCCOL nCol, SCROW nRow, ScPostIt*& rpNote )
{
	if( ValidColRow( nCol, nRow ) )
    {
        aCol[ nCol ].TakeNote( nRow, rpNote );
        if( rpNote && rpNote->GetNoteData().mxInitData.get() )
        {
            if( !mxUninitNotes.get() )
                mxUninitNotes.reset( new ScAddress2DVec );
            mxUninitNotes->push_back( ScAddress2D( nCol, nRow ) );
        }
    }
    else
        DELETEZ( rpNote );
}


ScPostIt* ScTable::ReleaseNote( SCCOL nCol, SCROW nRow )
{
	return ValidColRow( nCol, nRow ) ? aCol[ nCol ].ReleaseNote( nRow ) : 0;
}


void ScTable::DeleteNote( SCCOL nCol, SCROW nRow )
{
	if( ValidColRow( nCol, nRow ) )
        aCol[ nCol ].DeleteNote( nRow );
}


void ScTable::InitializeNoteCaptions( bool bForced )
{
    if( mxUninitNotes.get() && (bForced || pDocument->IsUndoEnabled()) )
    {
        for( ScAddress2DVec::iterator aIt = mxUninitNotes->begin(), aEnd = mxUninitNotes->end(); aIt != aEnd; ++aIt )
            if( ScPostIt* pNote = GetNote( aIt->first, aIt->second ) )
                pNote->GetOrCreateCaption( ScAddress( aIt->first, aIt->second, nTab ) );
        mxUninitNotes.reset();
    }
}

CellType ScTable::GetCellType( SCCOL nCol, SCROW nRow ) const
{
	if (ValidColRow( nCol, nRow ))
		return aCol[nCol].GetCellType( nRow );
	return CELLTYPE_NONE;
}


ScBaseCell* ScTable::GetCell( SCCOL nCol, SCROW nRow ) const
{
	if (ValidColRow( nCol, nRow ))
		return aCol[nCol].GetCell( nRow );

	DBG_ERROR("GetCell ausserhalb");
	return NULL;
}


void ScTable::GetLastDataPos(SCCOL& rCol, SCROW& rRow) const
{
	rCol = MAXCOL;
	rRow = 0;
	while (aCol[rCol].IsEmptyData() && (rCol > 0))
		rCol--;
	SCCOL nCol = rCol;
	while ((SCsCOL)nCol >= 0)
	{
		rRow = Max(rRow, aCol[nCol].GetLastDataPos());
		nCol--;
	}
}


BOOL ScTable::HasData( SCCOL nCol, SCROW nRow )
{
	if (ValidColRow(nCol,nRow))
		return aCol[nCol].HasDataAt( nRow );
	else
		return FALSE;
}


BOOL ScTable::HasStringData( SCCOL nCol, SCROW nRow )
{
	if (ValidColRow(nCol,nRow))
		return aCol[nCol].HasStringData( nRow );
	else
		return FALSE;
}


BOOL ScTable::HasValueData( SCCOL nCol, SCROW nRow )
{
	if (ValidColRow(nCol,nRow))
		return aCol[nCol].HasValueData( nRow );
	else
		return FALSE;
}


BOOL ScTable::HasStringCells( SCCOL nStartCol, SCROW nStartRow,
								SCCOL nEndCol, SCROW nEndRow ) const
{
	if ( ValidCol(nEndCol) )
		for ( SCCOL nCol=nStartCol; nCol<=nEndCol; nCol++ )
			if (aCol[nCol].HasStringCells(nStartRow, nEndRow))
				return TRUE;

	return FALSE;
}


//UNUSED2008-05  USHORT ScTable::GetErrCode( SCCOL nCol, SCROW nRow ) const
//UNUSED2008-05  {
//UNUSED2008-05      if (ValidColRow( nCol, nRow ))
//UNUSED2008-05          return aCol[nCol].GetErrCode( nRow );
//UNUSED2008-05      return 0;
//UNUSED2008-05  }


void ScTable::SetDirtyVar()
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].SetDirtyVar();
}


void ScTable::SetDirty()
{
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].SetDirty();
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScTable::SetDirty( const ScRange& rRange )
{
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	SCCOL nCol2 = rRange.aEnd.Col();
	for (SCCOL i=rRange.aStart.Col(); i<=nCol2; i++)
		aCol[i].SetDirty( rRange );
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScTable::SetTableOpDirty( const ScRange& rRange )
{
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// no multiple recalculation
	SCCOL nCol2 = rRange.aEnd.Col();
	for (SCCOL i=rRange.aStart.Col(); i<=nCol2; i++)
		aCol[i].SetTableOpDirty( rRange );
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScTable::SetDirtyAfterLoad()
{
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].SetDirtyAfterLoad();
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScTable::SetRelNameDirty()
{
	BOOL bOldAutoCalc = pDocument->GetAutoCalc();
	pDocument->SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].SetRelNameDirty();
	pDocument->SetAutoCalc( bOldAutoCalc );
}


void ScTable::CalcAll()
{
	for (SCCOL i=0; i<=MAXCOL; i++) aCol[i].CalcAll();
}


void ScTable::CompileAll()
{
	for (SCCOL i=0; i <= MAXCOL; i++) aCol[i].CompileAll();
}


void ScTable::CompileXML( ScProgress& rProgress )
{
	for (SCCOL i=0; i <= MAXCOL; i++)
	{
		aCol[i].CompileXML( rProgress );
	}
}

void ScTable::CalcAfterLoad()
{
	for (SCCOL i=0; i <= MAXCOL; i++) aCol[i].CalcAfterLoad();
}


bool ScTable::MarkUsedExternalReferences()
{
    bool bAllMarked = false;
    for (SCCOL i=0; i <= MAXCOL && !bAllMarked; ++i)
    {
        bAllMarked = aCol[i].MarkUsedExternalReferences();
    }
    return bAllMarked;
}


void ScTable::ResetChanged( const ScRange& rRange )
{
	SCCOL nStartCol = rRange.aStart.Col();
	SCROW nStartRow = rRange.aStart.Row();
	SCCOL nEndCol = rRange.aEnd.Col();
	SCROW nEndRow = rRange.aEnd.Row();

	for (SCCOL nCol=nStartCol; nCol<=nEndCol; nCol++)
		aCol[nCol].ResetChanged(nStartRow, nEndRow);
}

//	Attribute

const SfxPoolItem* ScTable::GetAttr( SCCOL nCol, SCROW nRow, USHORT nWhich ) const
{
	if (ValidColRow(nCol,nRow))
		return aCol[nCol].GetAttr( nRow, nWhich );
	else
		return NULL;
}


ULONG ScTable::GetNumberFormat( SCCOL nCol, SCROW nRow ) const
{
	if (ValidColRow(nCol,nRow))
		return aCol[nCol].GetNumberFormat( nRow );
	else
		return 0;
}


const ScPatternAttr* ScTable::GetPattern( SCCOL nCol, SCROW nRow ) const
{
	if (ValidColRow(nCol,nRow))
		return aCol[nCol].GetPattern( nRow );
	else
    {
        DBG_ERROR("wrong column or row");
        return pDocument->GetDefPattern();      // for safety
    }
}


const ScPatternAttr* ScTable::GetMostUsedPattern( SCCOL nCol, SCROW nStartRow, SCROW nEndRow ) const
{
    if ( ValidColRow( nCol, nStartRow ) && ValidRow( nEndRow ) && (nStartRow <= nEndRow) )
        return aCol[nCol].GetMostUsedPattern( nStartRow, nEndRow );
    else
        return NULL;
}


BOOL ScTable::HasAttrib( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, USHORT nMask ) const
{
	BOOL bFound=FALSE;
	for (SCCOL i=nCol1; i<=nCol2 && !bFound; i++)
		bFound |= aCol[i].HasAttrib( nRow1, nRow2, nMask );
	return bFound;
}


//UNUSED2009-05 BOOL ScTable::HasLines( const ScRange& rRange, Rectangle& rSizes ) const
//UNUSED2009-05 {
//UNUSED2009-05     SCCOL nCol1 = rRange.aStart.Col();
//UNUSED2009-05     SCROW nRow1 = rRange.aStart.Row();
//UNUSED2009-05     SCCOL nCol2 = rRange.aEnd.Col();
//UNUSED2009-05     SCROW nRow2 = rRange.aEnd.Row();
//UNUSED2009-05     PutInOrder( nCol1, nCol2 );
//UNUSED2009-05     PutInOrder( nRow1, nRow2 );
//UNUSED2009-05 
//UNUSED2009-05     BOOL bFound = FALSE;
//UNUSED2009-05     for (SCCOL i=nCol1; i<=nCol2; i++)
//UNUSED2009-05         if (aCol[i].HasLines( nRow1, nRow2, rSizes, (i==nCol1), (i==nCol2) ))
//UNUSED2009-05             bFound = TRUE;
//UNUSED2009-05 
//UNUSED2009-05     return bFound;
//UNUSED2009-05 }


BOOL ScTable::HasAttribSelection( const ScMarkData& rMark, USHORT nMask ) const
{
	BOOL bFound=FALSE;
	for (SCCOL i=0; i<=MAXCOL && !bFound; i++)
		bFound |= aCol[i].HasAttribSelection( rMark, nMask );
	return bFound;
}


BOOL ScTable::ExtendMerge( SCCOL nStartCol, SCROW nStartRow,
						   SCCOL& rEndCol, SCROW& rEndRow,
						   BOOL bRefresh, BOOL bAttrs )
{
    if (!(ValidCol(nStartCol) && ValidCol(rEndCol)))
    {
        DBG_ERRORFILE("ScTable::ExtendMerge: invalid column number");
        return FALSE;
    }
	BOOL bFound=FALSE;
	SCCOL nOldEndX = rEndCol;
	SCROW nOldEndY = rEndRow;
	for (SCCOL i=nStartCol; i<=nOldEndX; i++)
		bFound |= aCol[i].ExtendMerge( i, nStartRow, nOldEndY, rEndCol, rEndRow, bRefresh, bAttrs );
	return bFound;
}


BOOL ScTable::IsBlockEmpty( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, bool bIgnoreNotes ) const
{
    if (!(ValidCol(nCol1) && ValidCol(nCol2)))
    {
        DBG_ERRORFILE("ScTable::IsBlockEmpty: invalid column number");
        return FALSE;
    }
	BOOL bEmpty = TRUE;
	for (SCCOL i=nCol1; i<=nCol2 && bEmpty; i++)
		bEmpty = aCol[i].IsEmptyBlock( nRow1, nRow2, bIgnoreNotes );
	return bEmpty;
}

SCSIZE ScTable::FillMaxRot( RowInfo* pRowInfo, SCSIZE nArrCount, SCCOL nX1, SCCOL nX2,
							SCCOL nCol, SCROW nAttrRow1, SCROW nAttrRow2, SCSIZE nArrY,
							const ScPatternAttr* pPattern, const SfxItemSet* pCondSet ) const
{
	//	Rueckgabe = neues nArrY

	BYTE nRotDir = pPattern->GetRotateDir( pCondSet );
	if ( nRotDir != SC_ROTDIR_NONE )
	{
		BOOL bHit = TRUE;
		if ( nCol+1 < nX1 )								// column to the left
			bHit = ( nRotDir != SC_ROTDIR_LEFT );
		else if ( nCol > nX2+1 )						// column to the right
			bHit = ( nRotDir != SC_ROTDIR_RIGHT );		// SC_ROTDIR_STANDARD may now also be extended to the left

		if ( bHit )
		{
			double nFactor = 0.0;
			if ( nCol > nX2+1 )
			{
				long nRotVal = ((const SfxInt32Item&) pPattern->
						GetItem( ATTR_ROTATE_VALUE, pCondSet )).GetValue();
				double nRealOrient = nRotVal * F_PI18000;	// 1/100 Grad
				double nCos = cos( nRealOrient );
				double nSin = sin( nRealOrient );
				//!	begrenzen !!!
				//!	zusaetzlich Faktor fuer unterschiedliche PPT X/Y !!!

				//	bei SC_ROTDIR_LEFT kommt immer ein negativer Wert heraus,
				//	wenn der Modus beruecksichtigt wird
				nFactor = -fabs( nCos / nSin );
			}

			for ( SCROW nRow = nAttrRow1; nRow <= nAttrRow2; nRow++ )
			{
				if ( !(pRowFlags->GetValue(nRow) & CR_HIDDEN) )
				{
					BOOL bHitOne = TRUE;
					if ( nCol > nX2+1 )
					{
						// reicht die gedrehte Zelle bis in den sichtbaren Bereich?

						SCCOL nTouchedCol = nCol;
						long nWidth = (long) ( pRowHeight->GetValue(nRow) * nFactor );
						DBG_ASSERT(nWidth <= 0, "Richtung falsch");
						while ( nWidth < 0 && nTouchedCol > 0 )
						{
							--nTouchedCol;
							nWidth += GetColWidth( nTouchedCol );
						}
						if ( nTouchedCol > nX2 )
							bHitOne = FALSE;
					}

					if (bHitOne)
					{
						while ( nArrY<nArrCount && pRowInfo[nArrY].nRowNo < nRow )
							++nArrY;
						if ( nArrY<nArrCount && pRowInfo[nArrY].nRowNo == nRow )
							pRowInfo[nArrY].nRotMaxCol = nCol;
					}
				}
			}
		}
	}

	return nArrY;
}

void ScTable::FindMaxRotCol( RowInfo* pRowInfo, SCSIZE nArrCount, SCCOL nX1, SCCOL nX2 ) const
{
	if ( !pColWidth || !pRowHeight || !pColFlags || !pRowFlags )
	{
		DBG_ERROR( "Spalten-/Zeileninfo fehlt" );
		return;
	}

	//	nRotMaxCol ist auf SC_ROTMAX_NONE initialisiert, nRowNo ist schon gesetzt

	SCROW nY1 = pRowInfo[0].nRowNo;
	SCROW nY2 = pRowInfo[nArrCount-1].nRowNo;

	for (SCCOL nCol=0; nCol<=MAXCOL; nCol++)
	{
		if ( !(pColFlags[nCol] & CR_HIDDEN) )
		{
			SCSIZE nArrY = 0;
			ScDocAttrIterator aIter( pDocument, nTab, nCol, nY1, nCol, nY2 );
			SCCOL nAttrCol;
            SCROW nAttrRow1, nAttrRow2;
			const ScPatternAttr* pPattern = aIter.GetNext( nAttrCol, nAttrRow1, nAttrRow2 );
			while ( pPattern )
			{
				const SfxPoolItem* pCondItem;
				if ( pPattern->GetItemSet().GetItemState( ATTR_CONDITIONAL, TRUE, &pCondItem )
						== SFX_ITEM_SET )
				{
					//	alle Formate durchgehen, damit die Zellen nicht einzeln
					//	angeschaut werden muessen

					ULONG nIndex = ((const SfxUInt32Item*)pCondItem)->GetValue();
					ScConditionalFormatList* pList = pDocument->GetCondFormList();
					ScStyleSheetPool* pStylePool = pDocument->GetStyleSheetPool();
					if (pList && pStylePool && nIndex)
					{
						const ScConditionalFormat* pFormat = pList->GetFormat(nIndex);
						if ( pFormat )
						{
							USHORT nEntryCount = pFormat->Count();
							for (USHORT nEntry=0; nEntry<nEntryCount; nEntry++)
							{
                                String aStyleName = pFormat->GetEntry(nEntry)->GetStyle();
                                if (aStyleName.Len())
								{
									SfxStyleSheetBase* pStyleSheet =
                                            pStylePool->Find( aStyleName, SFX_STYLE_FAMILY_PARA );
									if ( pStyleSheet )
									{
										FillMaxRot( pRowInfo, nArrCount, nX1, nX2,
													nCol, nAttrRow1, nAttrRow2,
													nArrY, pPattern, &pStyleSheet->GetItemSet() );
										//	nArrY nicht veraendern
									}
								}
							}
						}
					}
				}

				nArrY = FillMaxRot( pRowInfo, nArrCount, nX1, nX2,
									nCol, nAttrRow1, nAttrRow2,
									nArrY, pPattern, NULL );

				pPattern = aIter.GetNext( nAttrCol, nAttrRow1, nAttrRow2 );
			}
		}
	}
}

BOOL ScTable::HasBlockMatrixFragment( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 ) const
{
	// nix:0, mitte:1, unten:2, links:4, oben:8, rechts:16, offen:32
	USHORT nEdges;

	if ( nCol1 == nCol2 )
	{	// linke und rechte Spalte
		const USHORT n = 4 | 16;
		nEdges = aCol[nCol1].GetBlockMatrixEdges( nRow1, nRow2, n );
		// nicht (4 und 16) oder 1 oder 32
		if ( nEdges && (((nEdges & n) != n) || (nEdges & 33)) )
			return TRUE;		// linke oder rechte Kante fehlt oder offen
	}
	else
	{	// linke Spalte
		nEdges = aCol[nCol1].GetBlockMatrixEdges( nRow1, nRow2, 4 );
		// nicht 4 oder 1 oder 32
		if ( nEdges && (((nEdges & 4) != 4) || (nEdges & 33)) )
			return TRUE;		// linke Kante fehlt oder offen
		// rechte Spalte
		nEdges = aCol[nCol2].GetBlockMatrixEdges( nRow1, nRow2, 16 );
		// nicht 16 oder 1 oder 32
		if ( nEdges && (((nEdges & 16) != 16) || (nEdges & 33)) )
			return TRUE;		// rechte Kante fehlt oder offen
	}

	if ( nRow1 == nRow2 )
	{	// obere und untere Zeile
		BOOL bOpen = FALSE;
		const USHORT n = 2 | 8;
		for ( SCCOL i=nCol1; i<=nCol2; i++)
		{
			nEdges = aCol[i].GetBlockMatrixEdges( nRow1, nRow1, n );
			if ( nEdges )
			{
				if ( (nEdges & n) != n )
					return TRUE;		// obere oder untere Kante fehlt
				if ( nEdges & 4 )
					bOpen = TRUE;		// linke Kante oeffnet, weitersehen
				else if ( !bOpen )
					return TRUE;		// es gibt was, was nicht geoeffnet wurde
				if ( nEdges & 16 )
					bOpen = FALSE;		// rechte Kante schliesst
			}
		}
		if ( bOpen )
			return TRUE;				// es geht noch weiter
	}
	else
	{
		USHORT j, n;
        SCROW nR;
		// erst obere Zeile, dann untere Zeile
		for ( j=0, nR=nRow1, n=8; j<2; j++, nR=nRow2, n=2 )
		{
			BOOL bOpen = FALSE;
			for ( SCCOL i=nCol1; i<=nCol2; i++)
			{
				nEdges = aCol[i].GetBlockMatrixEdges( nR, nR, n );
				if ( nEdges )
				{
					// in oberere Zeile keine obere Kante bzw.
					// in unterer Zeile keine untere Kante
					if ( (nEdges & n) != n )
						return TRUE;
					if ( nEdges & 4 )
						bOpen = TRUE;		// linke Kante oeffnet, weitersehen
					else if ( !bOpen )
						return TRUE;		// es gibt was, was nicht geoeffnet wurde
					if ( nEdges & 16 )
						bOpen = FALSE;		// rechte Kante schliesst
				}
			}
			if ( bOpen )
				return TRUE;				// es geht noch weiter
		}
	}
	return FALSE;
}


BOOL ScTable::HasSelectionMatrixFragment( const ScMarkData& rMark ) const
{
	BOOL bFound=FALSE;
	for (SCCOL i=0; i<=MAXCOL && !bFound; i++)
		bFound |= aCol[i].HasSelectionMatrixFragment(rMark);
	return bFound;
}


BOOL ScTable::IsBlockEditable( SCCOL nCol1, SCROW nRow1, SCCOL nCol2,
			SCROW nRow2, BOOL* pOnlyNotBecauseOfMatrix /* = NULL */ ) const
{
    if ( !ValidColRow( nCol2, nRow2 ) )
    {
        DBG_ERRORFILE("IsBlockEditable: invalid column or row");
        if (pOnlyNotBecauseOfMatrix)
            *pOnlyNotBecauseOfMatrix = FALSE;
        return FALSE;
    }

	BOOL bIsEditable = TRUE;
	if ( nLockCount )
		bIsEditable = FALSE;
    else if ( IsProtected() && !pDocument->IsScenario(nTab) )
    {
        if((bIsEditable = !HasAttrib( nCol1, nRow1, nCol2, nRow2, HASATTR_PROTECTED )) != FALSE)
        {
            // If Sheet is protected and cells are not protected then
            // check the active scenario protect flag if this range is
            // on the active scenario range. Note the 'copy back' must also
            // be set to apply protection.
            USHORT nScenTab = nTab+1;
            while(pDocument->IsScenario(nScenTab))
            {
                ScRange aEditRange(nCol1, nRow1, nScenTab, nCol2, nRow2, nScenTab);
                if(pDocument->IsActiveScenario(nScenTab) && pDocument->HasScenarioRange(nScenTab, aEditRange))
                {
                    USHORT nFlags;
                    pDocument->GetScenarioFlags(nScenTab,nFlags);
                    bIsEditable = !((nFlags & SC_SCENARIO_PROTECT) && (nFlags & SC_SCENARIO_TWOWAY));
                    break;
                }
                nScenTab++;
            }
        }
    }
    else if (pDocument->IsScenario(nTab))
    {
        // Determine if the preceding sheet is protected
        SCTAB nActualTab = nTab;
        do
        {
            nActualTab--;
        }
        while(pDocument->IsScenario(nActualTab));

        if(pDocument->IsTabProtected(nActualTab))
        {
            ScRange aEditRange(nCol1, nRow1, nTab, nCol2, nRow2, nTab);
            if(pDocument->HasScenarioRange(nTab, aEditRange))
            {
                USHORT nFlags;
                pDocument->GetScenarioFlags(nTab,nFlags);
                bIsEditable = !(nFlags & SC_SCENARIO_PROTECT);
            }
        }
    }
	if ( bIsEditable )
	{
		if ( HasBlockMatrixFragment( nCol1, nRow1, nCol2, nRow2 ) )
		{
			bIsEditable = FALSE;
			if ( pOnlyNotBecauseOfMatrix )
				*pOnlyNotBecauseOfMatrix = TRUE;
		}
		else if ( pOnlyNotBecauseOfMatrix )
			*pOnlyNotBecauseOfMatrix = FALSE;
	}
	else if ( pOnlyNotBecauseOfMatrix )
		*pOnlyNotBecauseOfMatrix = FALSE;
	return bIsEditable;
}


BOOL ScTable::IsSelectionEditable( const ScMarkData& rMark,
			BOOL* pOnlyNotBecauseOfMatrix /* = NULL */ ) const
{
	BOOL bIsEditable = TRUE;
	if ( nLockCount )
		bIsEditable = FALSE;
    else if ( IsProtected() && !pDocument->IsScenario(nTab) )
    {
        if((bIsEditable = !HasAttribSelection( rMark, HASATTR_PROTECTED )) != FALSE)
        {
            // If Sheet is protected and cells are not protected then
            // check the active scenario protect flag if this area is
            // in the active scenario range.
            ScRangeList aRanges;
            rMark.FillRangeListWithMarks( &aRanges, FALSE );
            ULONG nRangeCount = aRanges.Count();
            SCTAB nScenTab = nTab+1;
            while(pDocument->IsScenario(nScenTab) && bIsEditable)
            {
                if(pDocument->IsActiveScenario(nScenTab))
                {
                    for (ULONG i=0; i<nRangeCount && bIsEditable; i++)
                    {
                        ScRange aRange = *aRanges.GetObject(i);
                        if(pDocument->HasScenarioRange(nScenTab, aRange))
                        {
                            USHORT nFlags;
                            pDocument->GetScenarioFlags(nScenTab,nFlags);
                            bIsEditable = !((nFlags & SC_SCENARIO_PROTECT) && (nFlags & SC_SCENARIO_TWOWAY));
                        }
                    }
                }
                nScenTab++;
            }
        }
    }
    else if (pDocument->IsScenario(nTab))
    {
        // Determine if the preceding sheet is protected
        SCTAB nActualTab = nTab;
        do
        {
            nActualTab--;
        }
        while(pDocument->IsScenario(nActualTab));

        if(pDocument->IsTabProtected(nActualTab))
        {
            ScRangeList aRanges;
            rMark.FillRangeListWithMarks( &aRanges, FALSE );
            ULONG nRangeCount = aRanges.Count();
            for (ULONG i=0; i<nRangeCount && bIsEditable; i++)
            {
                ScRange aRange = *aRanges.GetObject(i);
                if(pDocument->HasScenarioRange(nTab, aRange))
                {
                    USHORT nFlags;
                    pDocument->GetScenarioFlags(nTab,nFlags);
                    bIsEditable = !(nFlags & SC_SCENARIO_PROTECT);
                }
            }
        }
    }
	if ( bIsEditable )
	{
		if ( HasSelectionMatrixFragment( rMark ) )
		{
			bIsEditable = FALSE;
			if ( pOnlyNotBecauseOfMatrix )
				*pOnlyNotBecauseOfMatrix = TRUE;
		}
		else if ( pOnlyNotBecauseOfMatrix )
			*pOnlyNotBecauseOfMatrix = FALSE;
	}
	else if ( pOnlyNotBecauseOfMatrix )
		*pOnlyNotBecauseOfMatrix = FALSE;
	return bIsEditable;
}



void ScTable::LockTable()
{
	++nLockCount;
}


void ScTable::UnlockTable()
{
	if (nLockCount)
		--nLockCount;
	else
	{
		DBG_ERROR("UnlockTable ohne LockTable");
	}
}


void ScTable::MergeSelectionPattern( ScMergePatternState& rState, const ScMarkData& rMark, BOOL bDeep ) const
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].MergeSelectionPattern( rState, rMark, bDeep );
}


void ScTable::MergePatternArea( ScMergePatternState& rState, SCCOL nCol1, SCROW nRow1,
													SCCOL nCol2, SCROW nRow2, BOOL bDeep ) const
{
	for (SCCOL i=nCol1; i<=nCol2; i++)
		aCol[i].MergePatternArea( rState, nRow1, nRow2, bDeep );
}


void ScTable::MergeBlockFrame( SvxBoxItem* pLineOuter, SvxBoxInfoItem* pLineInner, ScLineFlags& rFlags,
					SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow ) const
{
	if (ValidColRow(nStartCol, nStartRow) && ValidColRow(nEndCol, nEndRow))
	{
		PutInOrder(nStartCol, nEndCol);
		PutInOrder(nStartRow, nEndRow);
		for (SCCOL i=nStartCol; i<=nEndCol; i++)
			aCol[i].MergeBlockFrame( pLineOuter, pLineInner, rFlags,
									nStartRow, nEndRow, (i==nStartCol), nEndCol-i );
	}
}


void ScTable::ApplyBlockFrame( const SvxBoxItem* pLineOuter, const SvxBoxInfoItem* pLineInner,
					SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow )
{
	if (ValidColRow(nStartCol, nStartRow) && ValidColRow(nEndCol, nEndRow))
	{
		PutInOrder(nStartCol, nEndCol);
		PutInOrder(nStartRow, nEndRow);
		for (SCCOL i=nStartCol; i<=nEndCol; i++)
			aCol[i].ApplyBlockFrame( pLineOuter, pLineInner,
									nStartRow, nEndRow, (i==nStartCol), nEndCol-i );
	}
}


void ScTable::ApplyPattern( SCCOL nCol, SCROW nRow, const ScPatternAttr& rAttr )
{
	if (ValidColRow(nCol,nRow))
		aCol[nCol].ApplyPattern( nRow, rAttr );
}


void ScTable::ApplyPatternArea( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									 const ScPatternAttr& rAttr )
{
	if (ValidColRow(nStartCol, nStartRow) && ValidColRow(nEndCol, nEndRow))
	{
		PutInOrder(nStartCol, nEndCol);
		PutInOrder(nStartRow, nEndRow);
		for (SCCOL i = nStartCol; i <= nEndCol; i++)
			aCol[i].ApplyPatternArea(nStartRow, nEndRow, rAttr);
	}
}

void ScTable::ApplyPatternIfNumberformatIncompatible( const ScRange& rRange,
		const ScPatternAttr& rPattern, short nNewType )
{
	SCCOL nEndCol = rRange.aEnd.Col();
	for ( SCCOL nCol = rRange.aStart.Col(); nCol <= nEndCol; nCol++ )
	{
		aCol[nCol].ApplyPatternIfNumberformatIncompatible( rRange, rPattern, nNewType );
	}
}



void ScTable::ApplyStyle( SCCOL nCol, SCROW nRow, const ScStyleSheet& rStyle )
{
	if (ValidColRow(nCol,nRow))
		aCol[nCol].ApplyStyle( nRow, rStyle );
}


void ScTable::ApplyStyleArea( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow, const ScStyleSheet& rStyle )
{
	if (ValidColRow(nStartCol, nStartRow) && ValidColRow(nEndCol, nEndRow))
	{
		PutInOrder(nStartCol, nEndCol);
		PutInOrder(nStartRow, nEndRow);
		for (SCCOL i = nStartCol; i <= nEndCol; i++)
			aCol[i].ApplyStyleArea(nStartRow, nEndRow, rStyle);
	}
}


void ScTable::ApplySelectionStyle(const ScStyleSheet& rStyle, const ScMarkData& rMark)
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].ApplySelectionStyle( rStyle, rMark );
}


void ScTable::ApplySelectionLineStyle( const ScMarkData& rMark,
							const SvxBorderLine* pLine, BOOL bColorOnly )
{
	if ( bColorOnly && !pLine )
		return;

	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].ApplySelectionLineStyle( rMark, pLine, bColorOnly );
}


const ScStyleSheet* ScTable::GetStyle( SCCOL nCol, SCROW nRow ) const
{
	if (ValidColRow(nCol, nRow))
		return aCol[nCol].GetStyle(nRow);
	else
		return NULL;
}


const ScStyleSheet* ScTable::GetSelectionStyle( const ScMarkData& rMark, BOOL& rFound ) const
{
	rFound = FALSE;

	BOOL	bEqual = TRUE;
	BOOL	bColFound;

	const ScStyleSheet* pStyle = NULL;
	const ScStyleSheet* pNewStyle;

	for (SCCOL i=0; i<=MAXCOL && bEqual; i++)
		if (rMark.HasMultiMarks(i))
		{
			pNewStyle = aCol[i].GetSelectionStyle( rMark, bColFound );
			if (bColFound)
			{
				rFound = TRUE;
				if ( !pNewStyle || ( pStyle && pNewStyle != pStyle ) )
					bEqual = FALSE;												// unterschiedliche
				pStyle = pNewStyle;
			}
		}

	return bEqual ? pStyle : NULL;
}


const ScStyleSheet*	ScTable::GetAreaStyle( BOOL& rFound, SCCOL nCol1, SCROW nRow1,
													SCCOL nCol2, SCROW nRow2 ) const
{
	rFound = FALSE;

	BOOL	bEqual = TRUE;
	BOOL	bColFound;

	const ScStyleSheet* pStyle = NULL;
	const ScStyleSheet* pNewStyle;

	for (SCCOL i=nCol1; i<=nCol2 && bEqual; i++)
	{
		pNewStyle = aCol[i].GetAreaStyle(bColFound, nRow1, nRow2);
		if (bColFound)
		{
			rFound = TRUE;
			if ( !pNewStyle || ( pStyle && pNewStyle != pStyle ) )
				bEqual = FALSE;												// unterschiedliche
			pStyle = pNewStyle;
		}
	}

	return bEqual ? pStyle : NULL;
}


BOOL ScTable::IsStyleSheetUsed( const ScStyleSheet& rStyle, BOOL bGatherAllStyles ) const
{
	BOOL bIsUsed = FALSE;

	for ( SCCOL i=0; i<=MAXCOL; i++ )
    {
		if ( aCol[i].IsStyleSheetUsed( rStyle, bGatherAllStyles ) )
        {
            if ( !bGatherAllStyles )
                return TRUE;
            bIsUsed = TRUE;
        }
    }

	return bIsUsed;
}


void ScTable::StyleSheetChanged( const SfxStyleSheetBase* pStyleSheet, BOOL bRemoved,
								OutputDevice* pDev,
								double nPPTX, double nPPTY,
								const Fraction& rZoomX, const Fraction& rZoomY )
{
	BOOL* pUsed = new BOOL[MAXROWCOUNT];
    memset( pUsed, 0, sizeof(BOOL) * (MAXROWCOUNT) );

	SCCOL nCol;
	for (nCol=0; nCol<=MAXCOL; nCol++)
		aCol[nCol].FindStyleSheet( pStyleSheet, pUsed, bRemoved );

	BOOL bFound = FALSE;
	SCROW nStart = 0, nEnd = 0;
	for (SCROW i=0; i<=MAXROW; i++)
	{
		if (pUsed[i])
		{
			if (!bFound)
			{
				nStart = i;
				bFound = TRUE;
			}
			nEnd = i;
		}
		else if (bFound)
		{
			SetOptimalHeight( nStart, nEnd, 0, pDev, nPPTX, nPPTY, rZoomX, rZoomY, FALSE );
			bFound = FALSE;
		}
	}
	if (bFound)
		SetOptimalHeight( nStart, nEnd, 0, pDev, nPPTX, nPPTY, rZoomX, rZoomY, FALSE );

	delete[] pUsed;
}


BOOL ScTable::ApplyFlags( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									INT16 nFlags )
{
	BOOL bChanged = FALSE;
	if (ValidColRow(nStartCol, nStartRow) && ValidColRow(nEndCol, nEndRow))
		for (SCCOL i = nStartCol; i <= nEndCol; i++)
			bChanged |= aCol[i].ApplyFlags(nStartRow, nEndRow, nFlags);
	return bChanged;
}


BOOL ScTable::RemoveFlags( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									INT16 nFlags )
{
	BOOL bChanged = FALSE;
	if (ValidColRow(nStartCol, nStartRow) && ValidColRow(nEndCol, nEndRow))
		for (SCCOL i = nStartCol; i <= nEndCol; i++)
			bChanged |= aCol[i].RemoveFlags(nStartRow, nEndRow, nFlags);
	return bChanged;
}


void ScTable::SetPattern( SCCOL nCol, SCROW nRow, const ScPatternAttr& rAttr, BOOL bPutToPool )
{
	if (ValidColRow(nCol,nRow))
		aCol[nCol].SetPattern( nRow, rAttr, bPutToPool );
}


void ScTable::ApplyAttr( SCCOL nCol, SCROW nRow, const SfxPoolItem& rAttr )
{
	if (ValidColRow(nCol,nRow))
		aCol[nCol].ApplyAttr( nRow, rAttr );
}


void ScTable::ApplySelectionCache( SfxItemPoolCache* pCache, const ScMarkData& rMark )
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].ApplySelectionCache( pCache, rMark );
}


void ScTable::ChangeSelectionIndent( BOOL bIncrement, const ScMarkData& rMark )
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].ChangeSelectionIndent( bIncrement, rMark );
}


void ScTable::ClearSelectionItems( const USHORT* pWhich, const ScMarkData& rMark )
{
	for (SCCOL i=0; i<=MAXCOL; i++)
		aCol[i].ClearSelectionItems( pWhich, rMark );
}


//	Spaltenbreiten / Zeilenhoehen

void ScTable::SetColWidth( SCCOL nCol, USHORT nNewWidth )
{
	if (VALIDCOL(nCol) && pColWidth)
	{
		if (!nNewWidth)
		{
//			DBG_ERROR("Spaltenbreite 0 in SetColWidth");
			nNewWidth = STD_COL_WIDTH;
		}

		if ( nNewWidth != pColWidth[nCol] )
		{
			IncRecalcLevel();
            InitializeNoteCaptions();
			ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
			if (pDrawLayer)
				pDrawLayer->WidthChanged( nTab, nCol, ((long) nNewWidth) - (long) pColWidth[nCol] );
			pColWidth[nCol] = nNewWidth;
			DecRecalcLevel();
		}
	}
	else
	{
		DBG_ERROR("Falsche Spaltennummer oder keine Breiten");
	}
}


void ScTable::SetRowHeight( SCROW nRow, USHORT nNewHeight )
{
	if (VALIDROW(nRow) && pRowHeight)
	{
		if (!nNewHeight)
		{
			DBG_ERROR("Zeilenhoehe 0 in SetRowHeight");
			nNewHeight = ScGlobal::nStdRowHeight;
		}

        USHORT nOldHeight = pRowHeight->GetValue(nRow);
		if ( nNewHeight != nOldHeight )
		{
			IncRecalcLevel();
            InitializeNoteCaptions();
			ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
			if (pDrawLayer)
				pDrawLayer->HeightChanged( nTab, nRow, ((long) nNewHeight) - (long) nOldHeight );
			pRowHeight->SetValue( nRow, nNewHeight);
			DecRecalcLevel();
		}
	}
	else
	{
		DBG_ERROR("Falsche Zeilennummer oder keine Hoehen");
	}
}


BOOL ScTable::SetRowHeightRange( SCROW nStartRow, SCROW nEndRow, USHORT nNewHeight,
                                    double /* nPPTX */, double nPPTY )
{
	BOOL bChanged = FALSE;
	if (VALIDROW(nStartRow) && VALIDROW(nEndRow) && pRowHeight)
	{
		IncRecalcLevel();
        InitializeNoteCaptions();
		if (!nNewHeight)
		{
			DBG_ERROR("Zeilenhoehe 0 in SetRowHeight");
			nNewHeight = ScGlobal::nStdRowHeight;
		}

		long nNewPix = (long) ( nNewHeight * nPPTY );

        BOOL bSingle = FALSE;   // TRUE = process every row for its own
		ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
		if (pDrawLayer)
			if (pDrawLayer->HasObjectsInRows( nTab, nStartRow, nEndRow ))
				bSingle = TRUE;

		if (bSingle)
		{
            size_t nIndex;
            SCROW nRegionEndRow;
            USHORT nOldHeight = pRowHeight->GetValue( nStartRow, nIndex, nRegionEndRow);
            if (nNewHeight == nOldHeight && nEndRow <= nRegionEndRow)
                bSingle = FALSE;    // no difference in this range
		}
		if (bSingle)
		{
			if (nEndRow-nStartRow < 20)
            {
                // Whether new pixel size will differ from old pixel size in any row.
                ScCompressedArrayIterator< SCROW, USHORT> aIter( *pRowHeight,
                        nStartRow, nEndRow);
                do
                {
                    if (*aIter != nNewHeight)
                        bChanged = (nNewPix != (long) (*aIter * nPPTY));
                } while (!bChanged && aIter.NextRange());

                /*  #i94028# #i94991# If drawing objects are involved, each row
                    has to be changed for its own, because each call to
                    ScDrawLayer::HeightChanged expects correct row heights
                    above passed row in the document. Cannot use array iterator
                    because array changes in every cycle. */
                if( pDrawLayer )
                {
                    for( SCROW nRow = nStartRow; nRow <= nEndRow ; ++nRow )
                    {
                        pDrawLayer->HeightChanged( nTab, nRow,
                            ((long) nNewHeight) - ((long) pRowHeight->GetValue( nRow )));
                        pRowHeight->SetValue( nRow, nNewHeight );
                    }
                }
                else
                    pRowHeight->SetValue( nStartRow, nEndRow, nNewHeight);
            }
			else
			{
				SCROW nMid = (nStartRow+nEndRow) / 2;
				if (SetRowHeightRange( nStartRow, nMid, nNewHeight, 1.0, 1.0 ))
					bChanged = TRUE;
				if (SetRowHeightRange( nMid+1, nEndRow, nNewHeight, 1.0, 1.0 ))
					bChanged = TRUE;
			}
		}
		else
		{
			if (pDrawLayer)
			{
                unsigned long nOldHeights = pRowHeight->SumValues( nStartRow, nEndRow);
                // FIXME: should we test for overflows?
				long nHeightDif = (long) (unsigned long) nNewHeight *
                    (nEndRow - nStartRow + 1) - nOldHeights;
				pDrawLayer->HeightChanged( nTab, nEndRow, nHeightDif );
			}
            // Whether new pixel size will differ from old pixel size in any row.
            ScCompressedArrayIterator< SCROW, USHORT> aIter( *pRowHeight,
                    nStartRow, nEndRow);
            do
            {
                if (*aIter != nNewHeight)
                    bChanged = (nNewPix != (long) (*aIter * nPPTY));
            } while (!bChanged && aIter.NextRange());
            pRowHeight->SetValue( nStartRow, nEndRow, nNewHeight);
		}
		DecRecalcLevel();
	}
	else
	{
		DBG_ERROR("Falsche Zeilennummer oder keine Hoehen");
	}

	return bChanged;
}


void ScTable::SetManualHeight( SCROW nStartRow, SCROW nEndRow, BOOL bManual )
{
	if (VALIDROW(nStartRow) && VALIDROW(nEndRow) && pRowFlags)
	{
		if (bManual)
            pRowFlags->OrValue( nStartRow, nEndRow, CR_MANUALSIZE);
		else
            pRowFlags->AndValue( nStartRow, nEndRow, sal::static_int_cast<BYTE>(~CR_MANUALSIZE));
	}
	else
	{
		DBG_ERROR("Falsche Zeilennummer oder keine Zeilenflags");
	}
}


USHORT ScTable::GetColWidth( SCCOL nCol ) const
{
	DBG_ASSERT(VALIDCOL(nCol),"Falsche Spaltennummer");

	if (VALIDCOL(nCol) && pColFlags && pColWidth)
	{
		if ( pColFlags[nCol] & CR_HIDDEN )
			return 0;
		else
			return pColWidth[nCol];
	}
	else
		return (USHORT) STD_COL_WIDTH;
}


USHORT ScTable::GetOriginalWidth( SCCOL nCol ) const		// immer die eingestellte
{
	DBG_ASSERT(VALIDCOL(nCol),"Falsche Spaltennummer");

	if (VALIDCOL(nCol) && pColWidth)
		return pColWidth[nCol];
	else
		return (USHORT) STD_COL_WIDTH;
}


USHORT ScTable::GetCommonWidth( SCCOL nEndCol ) const
{
	//	get the width that is used in the largest continuous column range (up to nEndCol)

	if ( !ValidCol(nEndCol) )
	{
		DBG_ERROR("wrong column");
		nEndCol = MAXCOL;
	}

	USHORT nMaxWidth = 0;
	USHORT nMaxCount = 0;
	USHORT nRangeStart = 0;
	while ( nRangeStart <= nEndCol )
	{
		//	skip hidden columns
		while ( nRangeStart <= nEndCol && (pColFlags[nRangeStart] & CR_HIDDEN) )
			++nRangeStart;
		if ( nRangeStart <= nEndCol )
		{
			USHORT nThisCount = 0;
			USHORT nThisWidth = pColWidth[nRangeStart];
			USHORT nRangeEnd = nRangeStart;
			while ( nRangeEnd <= nEndCol && pColWidth[nRangeEnd] == nThisWidth )
			{
				++nThisCount;
				++nRangeEnd;

				//	skip hidden columns
				while ( nRangeEnd <= nEndCol && (pColFlags[nRangeEnd] & CR_HIDDEN) )
					++nRangeEnd;
			}

			if ( nThisCount > nMaxCount )
			{
				nMaxCount = nThisCount;
				nMaxWidth = nThisWidth;
			}

			nRangeStart = nRangeEnd;		// next range
		}
	}

	return nMaxWidth;
}


USHORT ScTable::GetRowHeight( SCROW nRow ) const
{
	DBG_ASSERT(VALIDROW(nRow),"Falsche Zeilennummer");

	if (VALIDROW(nRow) && pRowFlags && pRowHeight)
	{
		if ( pRowFlags->GetValue(nRow) & CR_HIDDEN )
			return 0;
		else
			return pRowHeight->GetValue(nRow);
	}
	else
		return (USHORT) ScGlobal::nStdRowHeight;
}


ULONG ScTable::GetRowHeight( SCROW nStartRow, SCROW nEndRow ) const
{
	DBG_ASSERT(VALIDROW(nStartRow) && VALIDROW(nEndRow),"Falsche Zeilennummer");

	if (VALIDROW(nStartRow) && VALIDROW(nEndRow) && pRowFlags && pRowHeight)
	{
        return pRowFlags->SumCoupledArrayForCondition( nStartRow, nEndRow,
                CR_HIDDEN, 0, *pRowHeight);
	}
	else
		return (ULONG) ((nEndRow - nStartRow + 1) * ScGlobal::nStdRowHeight);
}


ULONG ScTable::GetScaledRowHeight( SCROW nStartRow, SCROW nEndRow, double fScale ) const
{
	DBG_ASSERT(VALIDROW(nStartRow) && VALIDROW(nEndRow),"Falsche Zeilennummer");

	if (VALIDROW(nStartRow) && VALIDROW(nEndRow) && pRowFlags && pRowHeight)
	{
        return pRowFlags->SumScaledCoupledArrayForCondition( nStartRow,
                nEndRow, CR_HIDDEN, 0, *pRowHeight, fScale);
	}
	else
		return (ULONG) ((nEndRow - nStartRow + 1) * ScGlobal::nStdRowHeight * fScale);
}


USHORT ScTable::GetOriginalHeight( SCROW nRow ) const		// non-0 even if hidden
{
	DBG_ASSERT(VALIDROW(nRow),"wrong row number");

	if (VALIDROW(nRow) && pRowHeight)
		return pRowHeight->GetValue(nRow);
	else
		return (USHORT) ScGlobal::nStdRowHeight;
}


//	Spalten-/Zeilen-Flags


SCROW ScTable::GetHiddenRowCount( SCROW nRow ) const
{
	SCROW nEndRow = nRow;
	if ( pRowFlags )
	{
        nEndRow = pRowFlags->GetBitStateEnd( nRow, CR_HIDDEN, CR_HIDDEN);
        if (ValidRow(nEndRow))
            ++nEndRow;
        else
            nEndRow = nRow;
	}
	return nEndRow - nRow;
}


//!		ShowRows / DBShowRows zusammenfassen

void ScTable::ShowCol(SCCOL nCol, BOOL bShow)
{
	if (VALIDCOL(nCol) && pColFlags)
	{
		BOOL bWasVis = ( pColFlags[nCol] & CR_HIDDEN ) == 0;
		if (bWasVis != bShow)
		{
			IncRecalcLevel();
            InitializeNoteCaptions();
			ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
			if (pDrawLayer)
			{
				if (bShow)
					pDrawLayer->WidthChanged( nTab, nCol, (long) pColWidth[nCol] );
				else
					pDrawLayer->WidthChanged( nTab, nCol, -(long) pColWidth[nCol] );
			}

			if (bShow)
				pColFlags[nCol] &= ~CR_HIDDEN;
			else
				pColFlags[nCol] |= CR_HIDDEN;
			DecRecalcLevel();

			ScChartListenerCollection* pCharts = pDocument->GetChartListenerCollection();
			if ( pCharts )
				pCharts->SetRangeDirty(ScRange( nCol, 0, nTab, nCol, MAXROW, nTab ));
		}
	}
	else
	{
		DBG_ERROR("Falsche Spaltennummer oder keine Flags");
	}
}


void ScTable::ShowRow(SCROW nRow, BOOL bShow)
{
	if (VALIDROW(nRow) && pRowFlags)
	{
        BYTE nFlags = pRowFlags->GetValue(nRow);
		BOOL bWasVis = ( nFlags & CR_HIDDEN ) == 0;
		if (bWasVis != bShow)
		{
			IncRecalcLevel();
            InitializeNoteCaptions();
			ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
			if (pDrawLayer)
			{
				if (bShow)
					pDrawLayer->HeightChanged( nTab, nRow, (long) pRowHeight->GetValue(nRow) );
				else
					pDrawLayer->HeightChanged( nTab, nRow, -(long) pRowHeight->GetValue(nRow) );
			}

			if (bShow)
				pRowFlags->SetValue( nRow, nFlags & ~(CR_HIDDEN | CR_FILTERED));
			else
				pRowFlags->SetValue( nRow, nFlags | CR_HIDDEN);
			DecRecalcLevel();

			ScChartListenerCollection* pCharts = pDocument->GetChartListenerCollection();
			if ( pCharts )
				pCharts->SetRangeDirty(ScRange( 0, nRow, nTab, MAXCOL, nRow, nTab ));
		}
	}
	else
	{
		DBG_ERROR("Falsche Zeilennummer oder keine Flags");
	}
}


void ScTable::DBShowRow(SCROW nRow, BOOL bShow)
{
	if (VALIDROW(nRow) && pRowFlags)
	{
        BYTE nFlags = pRowFlags->GetValue(nRow);
		BOOL bWasVis = ( nFlags & CR_HIDDEN ) == 0;
		IncRecalcLevel();
        InitializeNoteCaptions();
		if (bWasVis != bShow)
		{
			ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
			if (pDrawLayer)
			{
				if (bShow)
					pDrawLayer->HeightChanged( nTab, nRow, (long) pRowHeight->GetValue(nRow) );
				else
					pDrawLayer->HeightChanged( nTab, nRow, -(long) pRowHeight->GetValue(nRow) );
			}
		}

		//	Filter-Flag immer setzen, auch wenn Hidden unveraendert
		if (bShow)
			pRowFlags->SetValue( nRow, nFlags & ~(CR_HIDDEN | CR_FILTERED));
		else
			pRowFlags->SetValue( nRow, nFlags | (CR_HIDDEN | CR_FILTERED));
		DecRecalcLevel();

		if (bWasVis != bShow)
		{
			ScChartListenerCollection* pCharts = pDocument->GetChartListenerCollection();
			if ( pCharts )
				pCharts->SetRangeDirty(ScRange( 0, nRow, nTab, MAXCOL, nRow, nTab ));

			if (pOutlineTable)
				UpdateOutlineRow( nRow, nRow, bShow );
		}
	}
	else
	{
		DBG_ERROR("Falsche Zeilennummer oder keine Flags");
	}
}


void ScTable::DBShowRows(SCROW nRow1, SCROW nRow2, BOOL bShow)
{
	SCROW nStartRow = nRow1;
	IncRecalcLevel();
    InitializeNoteCaptions();
	while (nStartRow <= nRow2)
	{
		BYTE nOldFlag = pRowFlags->GetValue(nStartRow) & CR_HIDDEN;
        SCROW nEndRow = pRowFlags->GetBitStateEnd( nStartRow, CR_HIDDEN, nOldFlag);
        if (nEndRow > nRow2)
            nEndRow = nRow2;

		BOOL bWasVis = ( nOldFlag == 0 );
		BOOL bChanged = ( bWasVis != bShow );
		if ( bChanged )
		{
			ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
			if (pDrawLayer)
			{
				long nHeight = (long) pRowHeight->SumValues( nStartRow, nEndRow);
				if (bShow)
					pDrawLayer->HeightChanged( nTab, nStartRow, nHeight );
				else
					pDrawLayer->HeightChanged( nTab, nStartRow, -nHeight );
			}
		}

		if (bShow)
            pRowFlags->AndValue( nStartRow, nEndRow, sal::static_int_cast<BYTE>(~(CR_HIDDEN | CR_FILTERED)) );
		else
            pRowFlags->OrValue( nStartRow, nEndRow, (CR_HIDDEN | CR_FILTERED));

		if ( bChanged )
		{
			ScChartListenerCollection* pCharts = pDocument->GetChartListenerCollection();
			if ( pCharts )
				pCharts->SetRangeDirty(ScRange( 0, nStartRow, nTab, MAXCOL, nEndRow, nTab ));
		}

		nStartRow = nEndRow + 1;
	}

	//	#i12341# For Show/Hide rows, the outlines are updated separately from the outside.
	//	For filtering, the changes aren't visible to the caller, so UpdateOutlineRow has
	//	to be done here.
	if (pOutlineTable)
		UpdateOutlineRow( nRow1, nRow2, bShow );

	DecRecalcLevel();
}


void ScTable::ShowRows(SCROW nRow1, SCROW nRow2, BOOL bShow)
{
	SCROW nStartRow = nRow1;
	IncRecalcLevel();
    InitializeNoteCaptions();
	while (nStartRow <= nRow2)
	{
		BYTE nOldFlag = pRowFlags->GetValue(nStartRow) & CR_HIDDEN;
        SCROW nEndRow = pRowFlags->GetBitStateEnd( nStartRow, CR_HIDDEN, nOldFlag);
        if (nEndRow > nRow2)
            nEndRow = nRow2;

		BOOL bWasVis = ( nOldFlag == 0 );
		BOOL bChanged = ( bWasVis != bShow );
		if ( bChanged )
		{
			ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
			if (pDrawLayer)
			{
				long nHeight = (long) pRowHeight->SumValues( nStartRow, nEndRow);
				if (bShow)
					pDrawLayer->HeightChanged( nTab, nStartRow, nHeight );
				else
					pDrawLayer->HeightChanged( nTab, nStartRow, -nHeight );
			}
		}

		if (bShow)
            pRowFlags->AndValue( nStartRow, nEndRow, sal::static_int_cast<BYTE>(~(CR_HIDDEN | CR_FILTERED)) );
		else
            pRowFlags->OrValue( nStartRow, nEndRow, CR_HIDDEN);

		if ( bChanged )
		{
			ScChartListenerCollection* pCharts = pDocument->GetChartListenerCollection();
			if ( pCharts )
				pCharts->SetRangeDirty(ScRange( 0, nStartRow, nTab, MAXCOL, nEndRow, nTab ));
		}

		nStartRow = nEndRow + 1;
	}
	DecRecalcLevel();
}


BOOL ScTable::IsFiltered(SCROW nRow) const
{
	if (VALIDROW(nRow) && pRowFlags)
		return ( pRowFlags->GetValue(nRow) & CR_FILTERED ) != 0;

	DBG_ERROR("Falsche Zeilennummer oder keine Flags");
	return FALSE;
}


void ScTable::SetColFlags( SCCOL nCol, BYTE nNewFlags )
{
	if (VALIDCOL(nCol) && pColFlags)
		pColFlags[nCol] = nNewFlags;
	else
	{
		DBG_ERROR("Falsche Spaltennummer oder keine Flags");
	}
}


void ScTable::SetRowFlags( SCROW nRow, BYTE nNewFlags )
{
	if (VALIDROW(nRow) && pRowFlags)
		pRowFlags->SetValue( nRow, nNewFlags);
	else
	{
		DBG_ERROR("Falsche Zeilennummer oder keine Flags");
	}
}


void ScTable::SetRowFlags( SCROW nStartRow, SCROW nEndRow, BYTE nNewFlags )
{
	if (VALIDROW(nStartRow) && VALIDROW(nEndRow) && pRowFlags)
		pRowFlags->SetValue( nStartRow, nEndRow, nNewFlags);
	else
	{
		DBG_ERROR("Falsche Zeilennummer(n) oder keine Flags");
	}
}


BYTE ScTable::GetColFlags( SCCOL nCol ) const
{
	if (VALIDCOL(nCol) && pColFlags)
		return pColFlags[nCol];
	else
		return 0;
}


BYTE ScTable::GetRowFlags( SCROW nRow ) const
{
	if (VALIDROW(nRow) && pRowFlags)
		return pRowFlags->GetValue(nRow);
	else
		return 0;
}


SCROW ScTable::GetLastFlaggedRow() const
{
	if ( !pRowFlags )
		return 0;

    SCROW nLastFound = pRowFlags->GetLastAnyBitAccess( 0, sal::static_int_cast<BYTE>(~CR_PAGEBREAK) );
	return ValidRow(nLastFound) ? nLastFound : 0;
}


SCCOL ScTable::GetLastChangedCol() const
{
	if ( !pColFlags )
		return 0;

	SCCOL nLastFound = 0;
	for (SCCOL nCol = 1; nCol <= MAXCOL; nCol++)
		if ((pColFlags[nCol] & ~CR_PAGEBREAK) || (pColWidth[nCol] != STD_COL_WIDTH))
			nLastFound = nCol;

	return nLastFound;
}


SCROW ScTable::GetLastChangedRow() const
{
	if ( !pRowFlags )
		return 0;

    SCROW nLastFlags = pRowFlags->GetLastAnyBitAccess( 0, sal::static_int_cast<BYTE>(~CR_PAGEBREAK) );
    if (!ValidRow(nLastFlags))
        nLastFlags = 0;

    SCROW nLastHeight = pRowHeight->GetLastUnequalAccess( 0, ScGlobal::nStdRowHeight);
    if (!ValidRow(nLastHeight))
        nLastHeight = 0;

	return std::max( nLastFlags, nLastHeight);
}


BOOL ScTable::UpdateOutlineCol( SCCOL nStartCol, SCCOL nEndCol, BOOL bShow )
{
	if (pOutlineTable && pColFlags)
    {
        ScBitMaskCompressedArray< SCCOLROW, BYTE> aArray( MAXCOL, pColFlags, MAXCOLCOUNT);
		return pOutlineTable->GetColArray()->ManualAction( nStartCol, nEndCol, bShow, aArray );
    }
	else
		return FALSE;
}


BOOL ScTable::UpdateOutlineRow( SCROW nStartRow, SCROW nEndRow, BOOL bShow )
{
	if (pOutlineTable && pRowFlags)
		return pOutlineTable->GetRowArray()->ManualAction( nStartRow, nEndRow, bShow, *pRowFlags );
	else
		return FALSE;
}


void ScTable::ExtendHidden( SCCOL& rX1, SCROW& rY1, SCCOL& rX2, SCROW& rY2 )
{
	if (pColFlags)
	{
		while ( rX1>0 ? (pColFlags[rX1-1] & CR_HIDDEN) : FALSE )
			--rX1;
		while ( rX2<MAXCOL ? (pColFlags[rX2+1] & CR_HIDDEN) : FALSE )
			++rX2;
	}
	if (pRowFlags)
	{
        if (rY1 > 0)
        {
            SCROW nStartRow = pRowFlags->GetBitStateStart( rY1-1, CR_HIDDEN, CR_HIDDEN);
            if (ValidRow(nStartRow))
                rY1 = nStartRow;
        }
        if (rY2 < MAXROW)
        {
            SCROW nEndRow = pRowFlags->GetBitStateEnd( rY2+1, CR_HIDDEN, CR_HIDDEN);
            if (ValidRow(nEndRow))
                rY2 = nEndRow;
        }
	}
}


void ScTable::StripHidden( SCCOL& rX1, SCROW& rY1, SCCOL& rX2, SCROW& rY2 )
{
	if (pColFlags)
	{
		while ( rX2>rX1 && (pColFlags[rX2] & CR_HIDDEN) )
			--rX2;
		while ( rX2>rX1 && (pColFlags[rX1] & CR_HIDDEN) )
			++rX1;
	}
	if (pRowFlags)
	{
        if (rY1 < rY2)
        {
            SCROW nStartRow = pRowFlags->GetBitStateStart( rY2, CR_HIDDEN, CR_HIDDEN);
            if (ValidRow(nStartRow) && nStartRow >= rY1)
                rY2 = nStartRow;
        }
        if (rY1 < rY2)
        {
            SCROW nEndRow = pRowFlags->GetBitStateEnd( rY1, CR_HIDDEN, CR_HIDDEN);
            if (ValidRow(nEndRow) && nEndRow <= rY2)
                rY1 = nEndRow;
        }
	}
}


//	Auto-Outline

template< typename T >
short DiffSign( T a, T b )
{
	return (a<b) ? -1 :
			(a>b) ? 1 : 0;
}


void ScTable::DoAutoOutline( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow )
{
	BOOL bSizeChanged = FALSE;
	BOOL bMissed	  = FALSE;

	SCCOL nCol;
	SCROW nRow;
	SCROW i;
	BOOL bFound;
	ScOutlineArray* pArray;
	ScBaseCell* pCell;
	ScRange aRef;
/*	ScPatternAttr aBoldPattern( pDocument->GetPool() );				//!	spezielle Format-Vorlage
	aBoldPattern.GetItemSet().Put( SvxWeightItem( WEIGHT_BOLD ) );
*/

	StartOutlineTable();

							// Zeilen

	SCROW	nCount = nEndRow-nStartRow+1;
	BOOL*	pUsed = new BOOL[nCount];
	for (i=0; i<nCount; i++)
		pUsed[i] = FALSE;
	for (nCol=nStartCol; nCol<=nEndCol; nCol++)
		if (!aCol[nCol].IsEmptyData())
			aCol[nCol].FindUsed( nStartRow, nEndRow, pUsed );

	pArray = pOutlineTable->GetRowArray();
	for (nRow=nStartRow; nRow<=nEndRow; nRow++)
		if (pUsed[nRow-nStartRow])
		{
			bFound = FALSE;
			for (nCol=nStartCol; nCol<=nEndCol && !bFound; nCol++)
				if (!aCol[nCol].IsEmptyData())
				{
					pCell = aCol[nCol].GetCell( nRow );
					if (pCell)
						if ( pCell->GetCellType() == CELLTYPE_FORMULA )
							if (((ScFormulaCell*)pCell)->HasRefListExpressibleAsOneReference( aRef ))
								if ( aRef.aStart.Col() == nCol && aRef.aEnd.Col() == nCol &&
									 aRef.aStart.Tab() == nTab && aRef.aEnd.Tab() == nTab &&
									 DiffSign( aRef.aStart.Row(), nRow ) ==
										DiffSign( aRef.aEnd.Row(), nRow ) )
								{
									if (pArray->Insert( aRef.aStart.Row(), aRef.aEnd.Row(), bSizeChanged ))
									{
//										ApplyPatternArea( nStartCol, nRow, nEndCol, nRow, aBoldPattern );
										bFound = TRUE;
									}
									else
										bMissed = TRUE;
								}
				}
		}

	delete[] pUsed;

							// Spalten

	pArray = pOutlineTable->GetColArray();
	for (nCol=nStartCol; nCol<=nEndCol; nCol++)
	{
		if (!aCol[nCol].IsEmptyData())
		{
			bFound = FALSE;
			ScColumnIterator aIter( &aCol[nCol], nStartRow, nEndRow );
			while ( aIter.Next( nRow, pCell ) && !bFound )
			{
				if ( pCell->GetCellType() == CELLTYPE_FORMULA )
					if (((ScFormulaCell*)pCell)->HasRefListExpressibleAsOneReference( aRef ))
						if ( aRef.aStart.Row() == nRow && aRef.aEnd.Row() == nRow &&
							 aRef.aStart.Tab() == nTab && aRef.aEnd.Tab() == nTab &&
							 DiffSign( aRef.aStart.Col(), nCol ) ==
								DiffSign( aRef.aEnd.Col(), nCol ) )
						{
							if (pArray->Insert( aRef.aStart.Col(), aRef.aEnd.Col(), bSizeChanged ))
							{
//								ApplyPatternArea( nCol, nStartRow, nCol, nEndRow, aBoldPattern );
								bFound = TRUE;
							}
							else
								bMissed = TRUE;
						}
			}
		}
	}
}

									//	CopyData - fuer Query in anderen Bereich

void ScTable::CopyData( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
							SCCOL nDestCol, SCROW nDestRow, SCTAB nDestTab )
{
	//!		wenn fuer mehrere Zeilen benutzt, nach Spalten optimieren!

	ScAddress aSrc( nStartCol, nStartRow, nTab );
	ScAddress aDest( nDestCol, nDestRow, nDestTab );
	ScRange aRange( aSrc, aDest );
	BOOL bThisTab = ( nDestTab == nTab );
	SCROW nDestY = nDestRow;
	for (SCROW nRow=nStartRow; nRow<=nEndRow; nRow++)
	{
		aSrc.SetRow( nRow );
		aDest.SetRow( nDestY );
		SCCOL nDestX = nDestCol;
		for (SCCOL nCol=nStartCol; nCol<=nEndCol; nCol++)
		{
			aSrc.SetCol( nCol );
			aDest.SetCol( nDestX );
			ScBaseCell* pCell = GetCell( nCol, nRow );
			if (pCell)
			{
                pCell = pCell->CloneWithoutNote( *pDocument );
				if (pCell->GetCellType() == CELLTYPE_FORMULA)
				{
					((ScFormulaCell*)pCell)->UpdateReference( URM_COPY, aRange,
									((SCsCOL) nDestCol) - ((SCsCOL) nStartCol),
									((SCsROW) nDestRow) - ((SCsROW) nStartRow),
									((SCsTAB) nDestTab) - ((SCsTAB) nTab) );
					((ScFormulaCell*)pCell)->aPos = aDest;
				}
			}
			if (bThisTab)
			{
				PutCell( nDestX, nDestY, pCell );
				SetPattern( nDestX, nDestY, *GetPattern( nCol, nRow ), TRUE );
			}
			else
			{
				pDocument->PutCell( aDest, pCell );
				pDocument->SetPattern( aDest, *GetPattern( nCol, nRow ), TRUE );
			}

			++nDestX;
		}
		++nDestY;
	}
}


BOOL ScTable::RefVisible(ScFormulaCell* pCell)
{
	ScRange aRef;

	if (pCell->HasOneReference(aRef))
	{
		if (aRef.aStart.Col()==aRef.aEnd.Col() && aRef.aStart.Tab()==aRef.aEnd.Tab() && pRowFlags)
		{
            // while ((value & CR_FILTERED) == CR_FILTERED)
            // most times will be faster than
            // while ((value & CR_FILTERED) == 0)
            SCROW nEndRow = pRowFlags->GetBitStateEnd( aRef.aStart.Row(),
                    CR_FILTERED, CR_FILTERED);
            if (!ValidRow(nEndRow) || nEndRow < aRef.aEnd.Row())
                return TRUE;    // at least partly visible
            return FALSE;       // completely unvisible
		}
	}

	return TRUE;						// irgendwie anders
}


void ScTable::GetUpperCellString(SCCOL nCol, SCROW nRow, String& rStr)
{
	GetInputString(nCol, nRow, rStr);
	rStr.EraseTrailingChars();
	rStr.EraseLeadingChars();
	ScGlobal::pCharClass->toUpper(rStr);
}


// Berechnen der Groesse der Tabelle und setzen der Groesse an der DrawPage

void ScTable::SetDrawPageSize(bool bResetStreamValid, bool bUpdateNoteCaptionPos)
{
	ScDrawLayer* pDrawLayer = pDocument->GetDrawLayer();
	if( pDrawLayer )
	{
        long x = GetColOffset( MAXCOL + 1 );
        long y = GetRowOffset( MAXROW + 1 );
        x = (long) ((double) x * HMM_PER_TWIPS);
        y = (long) ((double) y * HMM_PER_TWIPS);

		if ( IsLayoutRTL() )		// IsNegativePage
			x = -x;

		pDrawLayer->SetPageSize( static_cast<sal_uInt16>(nTab), Size( x, y ), bUpdateNoteCaptionPos );
	}

    // #i102616# actions that modify the draw page size count as sheet modification
    // (exception: InitDrawLayer)
    if (bResetStreamValid && IsStreamValid())
        SetStreamValid(FALSE);
}


ULONG ScTable::GetRowOffset( SCROW nRow ) const
{
	ULONG n = 0;
	if ( pRowFlags && pRowHeight )
	{
        if (nRow == 0)
            return 0;
        else if (nRow == 1)
            return GetRowHeight(0);

        n = pRowFlags->SumCoupledArrayForCondition( 0, nRow-1, CR_HIDDEN, 0,
                *pRowHeight);
#ifdef DBG_UTIL
        if (n == ::std::numeric_limits<unsigned long>::max())
            DBG_ERRORFILE("ScTable::GetRowOffset: row heights overflow");
#endif
	}
	else
	{
		DBG_ERROR("GetRowOffset: Daten fehlen");
	}
	return n;
}


ULONG ScTable::GetColOffset( SCCOL nCol ) const
{
	ULONG n = 0;
	if ( pColFlags && pColWidth )
	{
		SCCOL i;
		BYTE* pFlags = pColFlags;
		USHORT* pWidth = pColWidth;
		for( i = 0; i < nCol; i++, pFlags++, pWidth++ )
			if( !( *pFlags & CR_HIDDEN ) )
				n += *pWidth;
	}
	else
	{
		DBG_ERROR("GetColumnOffset: Daten fehlen");
	}
	return n;
}

