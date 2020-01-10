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

#include <com/sun/star/i18n/TransliterationModules.hpp>

#include <unotools/textsearch.hxx>
#include <svx/srchitem.hxx>
#include <svx/editobj.hxx>

#include "table.hxx"
#include "collect.hxx"
#include "cell.hxx"
#include "document.hxx"
#include "stlpool.hxx"
#include "markdata.hxx"
#include "editutil.hxx"
#include "detfunc.hxx"
#include "postit.hxx"

//--------------------------------------------------------------------------


BOOL lcl_GetTextWithBreaks( const ScEditCell& rCell, ScDocument* pDoc, String& rVal )
{
	//	TRUE = more than 1 paragraph

	const EditTextObject* pData = NULL;
	rCell.GetData( pData );
	EditEngine& rEngine = pDoc->GetEditEngine();
	rEngine.SetText( *pData );
	rVal = rEngine.GetText( LINEEND_LF );
	return ( rEngine.GetParagraphCount() > 1 );
}

BOOL ScTable::SearchCell(const SvxSearchItem& rSearchItem, SCCOL nCol, SCROW nRow,
							const ScMarkData& rMark, String& rUndoStr, ScDocument* pUndoDoc)
{
	BOOL	bFound = FALSE;
	BOOL 	bDoSearch = TRUE;
	BOOL	bDoBack = rSearchItem.GetBackward();

	String	aString;
	ScBaseCell* pCell;
	if (rSearchItem.GetSelection())
		bDoSearch = rMark.IsCellMarked(nCol, nRow);
	if ( bDoSearch && ((pCell = aCol[nCol].GetCell( nRow )) != NULL) )
	{
		BOOL bMultiLine = FALSE;
		CellType eCellType = pCell->GetCellType();
		switch (rSearchItem.GetCellType())
		{
			case SVX_SEARCHIN_FORMULA:
			{
				if ( eCellType == CELLTYPE_FORMULA )
                    ((ScFormulaCell*)pCell)->GetFormula( aString,
                       formula::FormulaGrammar::GRAM_NATIVE_UI);
				else if ( eCellType == CELLTYPE_EDIT )
					bMultiLine = lcl_GetTextWithBreaks(
						*(const ScEditCell*)pCell, pDocument, aString );
				else
					aCol[nCol].GetInputString( nRow, aString );
			}
			break;
			case SVX_SEARCHIN_VALUE:
				if ( eCellType == CELLTYPE_EDIT )
					bMultiLine = lcl_GetTextWithBreaks(
						*(const ScEditCell*)pCell, pDocument, aString );
				else
					aCol[nCol].GetInputString( nRow, aString );
				break;
			case SVX_SEARCHIN_NOTE:
				{
                    if(const ScPostIt* pNote = pCell->GetNote())
				    {
                        aString = pNote->GetText();
                        bMultiLine = pNote->HasMultiLineText();
				    }
				}
				break;
			default:
				break;
		}
		xub_StrLen nStart = 0;
		xub_StrLen nEnd = aString.Len();
        ::com::sun::star::util::SearchResult aSearchResult;
		if (pSearchText)
		{
			if ( bDoBack )
			{
				xub_StrLen nTemp=nStart; nStart=nEnd; nEnd=nTemp;
				bFound = (BOOL)(pSearchText->SearchBkwrd(aString, &nStart, &nEnd, &aSearchResult));
				// change results to definition before 614:
				--nEnd;
			}
			else
			{
				bFound = (BOOL)(pSearchText->SearchFrwrd(aString, &nStart, &nEnd, &aSearchResult));
				// change results to definition before 614:
				--nEnd;
			}

			if (bFound && rSearchItem.GetWordOnly())
				bFound = (nStart == 0 && nEnd == aString.Len() - 1);
		}
		else
		{
			DBG_ERROR("pSearchText == NULL");
			return bFound;
		}

		BYTE cMatrixFlag = MM_NONE;
		if ( bFound &&
			( (rSearchItem.GetCommand() == SVX_SEARCHCMD_REPLACE)
			||(rSearchItem.GetCommand() == SVX_SEARCHCMD_REPLACE_ALL) ) &&
				// #60558# Matrix nicht zerreissen, nur Matrixformel ersetzen
				!( (eCellType == CELLTYPE_FORMULA &&
				((cMatrixFlag = ((ScFormulaCell*)pCell)->GetMatrixFlag()) == MM_REFERENCE))
				// kein UndoDoc => Matrix nicht wiederherstellbar => nicht ersetzen
				|| (cMatrixFlag != MM_NONE && !pUndoDoc) )
			)
		{
			if ( cMatrixFlag == MM_NONE && rSearchItem.GetCommand() == SVX_SEARCHCMD_REPLACE )
				rUndoStr = aString;
			else if (pUndoDoc)
			{
				ScAddress aAdr( nCol, nRow, nTab );
                ScBaseCell* pUndoCell = pCell->CloneWithoutNote( *pUndoDoc );
				pUndoDoc->PutCell( aAdr, pUndoCell);
			}
			BOOL bRepeat = !rSearchItem.GetWordOnly();
			do
			{
				//	wenn der gefundene Text leer ist, nicht weitersuchen,
				//	sonst wuerde man nie mehr aufhoeren (#35410#)
				if ( nEnd < nStart || nEnd == STRING_MAXLEN )
					bRepeat = FALSE;

				String sReplStr = rSearchItem.GetReplaceString();
				if (rSearchItem.GetRegExp())
				{
					String sFndStr = aString.Copy(nStart, nEnd-nStart+1);
                    pSearchText->ReplaceBackReferences( sReplStr, aString, aSearchResult );
					aString.Erase(nStart, nEnd-nStart+1);
					aString.Insert(sReplStr, nStart);
				}
				else
				{
					aString.Erase(nStart, nEnd - nStart + 1);
					aString.Insert(rSearchItem.GetReplaceString(), nStart);
				}

						//	Indizes anpassen
				if (bDoBack)
				{
					nEnd = nStart;
					nStart = 0;
				}
				else
				{
                    nStart = sal::static_int_cast<xub_StrLen>( nStart + sReplStr.Len() );
					nEnd = aString.Len();
				}

						//	weitersuchen ?
				if (bRepeat)
				{
					if ( rSearchItem.GetCommand() != SVX_SEARCHCMD_REPLACE_ALL || nStart >= nEnd )
						bRepeat = FALSE;
					else if (bDoBack)
					{
						xub_StrLen nTemp=nStart; nStart=nEnd; nEnd=nTemp;
						bRepeat = ((BOOL)(pSearchText->SearchBkwrd(aString, &nStart, &nEnd)));
						// change results to definition before 614:
						--nEnd;
					}
					else
					{
						bRepeat = ((BOOL)(pSearchText->SearchFrwrd(aString, &nStart, &nEnd)));
						// change results to definition before 614:
						--nEnd;
					}
				}
			}
			while (bRepeat);
			if (rSearchItem.GetCellType() == SVX_SEARCHIN_NOTE)
			{
                // NB: rich text format is lost.
                // This is also true of Cells.
                if( ScPostIt* pNote = pCell->GetNote() )
                    pNote->SetText( ScAddress( nCol, nRow, nTab ), aString );
			}
			else if ( cMatrixFlag != MM_NONE )
			{	// #60558# Matrix nicht zerreissen
				if ( aString.Len() > 2 )
				{	// {} raus, erst hier damit auch "{=" durch "{=..." ersetzt werden kann
					if ( aString.GetChar( aString.Len()-1 ) == '}' )
						aString.Erase( aString.Len()-1, 1 );
					if ( aString.GetChar(0) == '{' )
						aString.Erase( 0, 1 );
				}
				ScAddress aAdr( nCol, nRow, nTab );
				ScFormulaCell* pFCell = new ScFormulaCell( pDocument, aAdr,
					aString,formula::FormulaGrammar::GRAM_NATIVE_UI, cMatrixFlag );
                SCCOL nMatCols;
                SCROW nMatRows;
				((ScFormulaCell*)pCell)->GetMatColsRows( nMatCols, nMatRows );
				pFCell->SetMatColsRows( nMatCols, nMatRows );
				aCol[nCol].Insert( nRow, pFCell );
			}
			else if ( bMultiLine && aString.Search('\n') != STRING_NOTFOUND )
				PutCell( nCol, nRow, new ScEditCell( aString, pDocument ) );
			else
				aCol[nCol].SetString(nRow, nTab, aString);
			// pCell is invalid now (deleted)
		}
	}
	return bFound;
}

BOOL ScTable::Search(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
						const ScMarkData& rMark, String& rUndoStr, ScDocument* pUndoDoc)
{
	BOOL bFound = FALSE;
	BOOL bAll =  (rSearchItem.GetCommand() == SVX_SEARCHCMD_FIND_ALL)
			   ||(rSearchItem.GetCommand() == SVX_SEARCHCMD_REPLACE_ALL);
	SCCOL nCol = rCol;
	SCROW nRow = rRow;
	SCCOL nLastCol;
	SCROW nLastRow;
	GetLastDataPos(nLastCol, nLastRow);
	if (!bAll && rSearchItem.GetBackward())
	{
		nCol = Min(nCol, (SCCOL)(nLastCol + 1));
		nRow = Min(nRow, (SCROW)(nLastRow + 1));
		if (rSearchItem.GetRowDirection())
		{
			nCol--;
			while (!bFound && ((SCsROW)nRow >= 0))
			{
				while (!bFound && ((SCsCOL)nCol >= 0))
				{
					bFound = SearchCell(rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc);
					if (!bFound)
					{
						BOOL bIsEmpty;
						do
						{
							nCol--;
							if ((SCsCOL)nCol >= 0)
								bIsEmpty = aCol[nCol].IsEmptyData();
							else
								bIsEmpty = TRUE;
						}
						while (((SCsCOL)nCol >= 0) && bIsEmpty);
					}
				}
				if (!bFound)
				{
					nCol = nLastCol;
					nRow--;
				}
			}
		}
		else
		{
			nRow--;
			while (!bFound && ((SCsCOL)nCol >= 0))
			{
				while (!bFound && ((SCsROW)nRow >= 0))
				{
					bFound = SearchCell(rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc);
					if (!bFound)
					{
						 if (!aCol[nCol].GetPrevDataPos(nRow))
							nRow = -1;
					}
				}
				if (!bFound)
				{
					BOOL bIsEmpty;
					nRow = nLastRow;
					do
					{
						nCol--;
						if ((SCsCOL)nCol >= 0)
							bIsEmpty = aCol[nCol].IsEmptyData();
						else
							bIsEmpty = TRUE;
					}
					while (((SCsCOL)nCol >= 0) && bIsEmpty);
				}
			}
		}
	}
	else
	{
		if (!bAll && rSearchItem.GetRowDirection())
		{
			nCol++;
			while (!bFound && (nRow <= nLastRow))
			{
				while (!bFound && (nCol <= nLastCol))
				{
					bFound = SearchCell(rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc);
					if (!bFound)
					{
						nCol++;
						while ((nCol <= nLastCol) && aCol[nCol].IsEmptyData()) nCol++;
					}
				}
				if (!bFound)
				{
					nCol = 0;
					nRow++;
				}
			}
		}
		else
		{
			nRow++;
			while (!bFound && (nCol <= nLastCol))
			{
				while (!bFound && (nRow <= nLastRow))
				{
					bFound = SearchCell(rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc);
					if (!bFound)
					{
						 if (!aCol[nCol].GetNextDataPos(nRow))
							nRow = MAXROW + 1;
					}
				}
				if (!bFound)
				{
					nRow = 0;
					nCol++;
					while ((nCol <= nLastCol) && aCol[nCol].IsEmptyData()) nCol++;
				}
			}
		}
	}
	if (bFound)
	{
		rCol = nCol;
		rRow = nRow;
	}
	return bFound;
}

BOOL ScTable::SearchAll(const SvxSearchItem& rSearchItem, ScMarkData& rMark,
						String& rUndoStr, ScDocument* pUndoDoc)
{
	BOOL bFound = TRUE;
	SCCOL nCol = 0;
	SCROW nRow = -1;

	ScMarkData aNewMark( rMark );	// Tabellen-Markierungen kopieren
	aNewMark.ResetMark();
	do
	{
		bFound = Search(rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc);
		if (bFound)
			aNewMark.SetMultiMarkArea( ScRange( nCol, nRow, nTab ) );
	}
	while (bFound);

	rMark = aNewMark;		//	Markierung kopieren
							//!	pro Tabelle

	return (aNewMark.IsMultiMarked());
}

BOOL ScTable::Replace(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
						const ScMarkData& rMark, String& rUndoStr, ScDocument* pUndoDoc)
{
	BOOL bFound = FALSE;
	SCCOL nCol = rCol;
	SCROW nRow = rRow;
	if (rSearchItem.GetBackward())
	{
		if (rSearchItem.GetRowDirection())
			nCol += 1;
		else
			nRow += 1;
	}
	else
	{
		if (rSearchItem.GetRowDirection())
			nCol -= 1;
		else
			nRow -= 1;
	}
	bFound = Search(rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc);
	if (bFound)
	{
		rCol = nCol;
		rRow = nRow;
	}
	return bFound;
}

BOOL ScTable::ReplaceAll(const SvxSearchItem& rSearchItem, ScMarkData& rMark,
							String& rUndoStr, ScDocument* pUndoDoc)
{
	BOOL bOldDouble = ScColumn::bDoubleAlloc;		// sollte immer FALSE sein?
	DBG_ASSERT(!bOldDouble,"bDoubleAlloc ???");
	ScColumn::bDoubleAlloc = TRUE;					// fuer Undo-Doc

	BOOL bFound = TRUE;
	SCCOL nCol = 0;
	SCROW nRow = -1;

	ScMarkData aNewMark( rMark );	// Tabellen-Markierungen kopieren
	aNewMark.ResetMark();
	do
	{
		bFound = Search(rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc);
		if (bFound)
			aNewMark.SetMultiMarkArea( ScRange( nCol, nRow, nTab ) );
	}
	while (bFound);

	ScColumn::bDoubleAlloc = bOldDouble;

	rMark = aNewMark;		//	Markierung kopieren
							//!	pro Tabelle

	return (aNewMark.IsMultiMarked());
}

BOOL ScTable::SearchStyle(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
							ScMarkData& rMark)
{
	const ScStyleSheet* pSearchStyle = (const ScStyleSheet*)
										pDocument->GetStyleSheetPool()->Find(
										rSearchItem.GetSearchString(), SFX_STYLE_FAMILY_PARA );

	SCsCOL nCol = rCol;
	SCsROW nRow = rRow;
	BOOL bFound = FALSE;

	BOOL bSelect = rSearchItem.GetSelection();
	BOOL bRows = rSearchItem.GetRowDirection();
	BOOL bBack = rSearchItem.GetBackward();
	short nAdd = bBack ? -1 : 1;

	if (bRows)										// zeilenweise
	{
		nRow += nAdd;
		do
		{
			SCsROW nNextRow = aCol[nCol].SearchStyle( nRow, pSearchStyle, bBack, bSelect, rMark );
			if (!ValidRow(nNextRow))
			{
				nRow = bBack ? MAXROW : 0;
                nCol = sal::static_int_cast<SCsCOL>( nCol + nAdd );
			}
			else
			{
				nRow = nNextRow;
				bFound = TRUE;
			}
		}
		while (!bFound && ValidCol(nCol));
	}
	else											// spaltenweise
	{
		SCsROW nNextRows[MAXCOLCOUNT];
		SCsCOL i;
		for (i=0; i<=MAXCOL; i++)
		{
			SCsROW nSRow = nRow;
			if (bBack)	{ if (i>=nCol) --nSRow; }
			else		{ if (i<=nCol) ++nSRow;	}
			nNextRows[i] = aCol[i].SearchStyle( nSRow, pSearchStyle, bBack, bSelect, rMark );
		}
		if (bBack)							// rueckwaerts
		{
			nRow = -1;
			for (i=MAXCOL; i>=0; i--)
				if (nNextRows[i]>nRow)
				{
					nCol = i;
					nRow = nNextRows[i];
					bFound = TRUE;
				}
		}
		else								// vorwaerts
		{
			nRow = MAXROW+1;
			for (i=0; i<=MAXCOL; i++)
				if (nNextRows[i]<nRow)
				{
					nCol = i;
					nRow = nNextRows[i];
					bFound = TRUE;
				}
		}
	}

	if (bFound)
	{
		rCol = (SCCOL) nCol;
		rRow = (SCROW) nRow;
	}
	return bFound;
}

//!		einzelnes Pattern fuer Undo zurueckgeben

BOOL ScTable::ReplaceStyle(const SvxSearchItem& rSearchItem, SCCOL& rCol, SCROW& rRow,
						   ScMarkData& rMark, BOOL bIsUndo)
{
	BOOL bRet;
	if (bIsUndo)
		bRet = TRUE;
	else
		bRet = SearchStyle(rSearchItem, rCol, rRow, rMark);
	if (bRet)
	{
		const ScStyleSheet* pReplaceStyle = (const ScStyleSheet*)
										pDocument->GetStyleSheetPool()->Find(
										rSearchItem.GetReplaceString(), SFX_STYLE_FAMILY_PARA );

		if (pReplaceStyle)
			ApplyStyle( rCol, rRow, *pReplaceStyle );
		else
		{
			DBG_ERROR("pReplaceStyle==0");
		}
	}

	return bRet;
}

BOOL ScTable::SearchAllStyle(const SvxSearchItem& rSearchItem, ScMarkData& rMark)
{
	const ScStyleSheet* pSearchStyle = (const ScStyleSheet*)
										pDocument->GetStyleSheetPool()->Find(
										rSearchItem.GetSearchString(), SFX_STYLE_FAMILY_PARA );
	BOOL bSelect = rSearchItem.GetSelection();
	BOOL bBack = rSearchItem.GetBackward();

	ScMarkData aNewMark( rMark );	// Tabellen-Markierungen kopieren
	aNewMark.ResetMark();
	for (SCCOL i=0; i<=MAXCOL; i++)
	{
		BOOL bFound = TRUE;
		SCsROW nRow = 0;
		SCsROW nEndRow;
		while (bFound && nRow <= MAXROW)
		{
			bFound = aCol[i].SearchStyleRange( nRow, nEndRow, pSearchStyle, bBack, bSelect, rMark );
			if (bFound)
			{
				if (nEndRow<nRow)
				{
					SCsROW nTemp = nRow;
					nRow = nEndRow;
					nEndRow = nTemp;
				}
				aNewMark.SetMultiMarkArea( ScRange( i,nRow,nTab, i,nEndRow,nTab ) );
				nRow = nEndRow + 1;
			}
		}
	}

	rMark = aNewMark;		//	Markierung kopieren
							//!	pro Tabelle

	return (aNewMark.IsMultiMarked());
}

BOOL ScTable::ReplaceAllStyle(const SvxSearchItem& rSearchItem, ScMarkData& rMark,
								ScDocument* pUndoDoc)
{
	BOOL bRet = SearchAllStyle(rSearchItem, rMark);
	if (bRet)
	{
		const ScStyleSheet* pReplaceStyle = (const ScStyleSheet*)
										pDocument->GetStyleSheetPool()->Find(
										rSearchItem.GetReplaceString(), SFX_STYLE_FAMILY_PARA );

		if (pReplaceStyle)
		{
			if (pUndoDoc)
				pDocument->CopyToDocument( 0,0,nTab, MAXCOL,MAXROW,nTab,
											IDF_ATTRIB, TRUE, pUndoDoc, &rMark );
			ApplySelectionStyle( *pReplaceStyle, rMark );
		}
		else
		{
			DBG_ERROR("pReplaceStyle==0");
		}
	}

	return bRet;
}

BOOL ScTable::SearchAndReplace(const SvxSearchItem& rSearchItem,
								SCCOL& rCol, SCROW& rRow, ScMarkData& rMark,
								String& rUndoStr, ScDocument* pUndoDoc)
{
	USHORT nCommand = rSearchItem.GetCommand();
	BOOL bFound = FALSE;
	if ( ValidColRow(rCol, rRow) ||
		 ((nCommand == SVX_SEARCHCMD_FIND || nCommand == SVX_SEARCHCMD_REPLACE) &&
		   (((rCol == MAXCOLCOUNT || rCol == -1) && VALIDROW(rRow)) ||
			((rRow == MAXROWCOUNT || rRow == -1) && VALIDCOL(rCol))
		   )
		 )
	   )
	{
		BOOL bStyles = rSearchItem.GetPattern();
		if (bStyles)
		{
			if (nCommand == SVX_SEARCHCMD_FIND)
				bFound = SearchStyle(rSearchItem, rCol, rRow, rMark);
			else if (nCommand == SVX_SEARCHCMD_REPLACE)
				bFound = ReplaceStyle(rSearchItem, rCol, rRow, rMark, FALSE);
			else if (nCommand == SVX_SEARCHCMD_FIND_ALL)
				bFound = SearchAllStyle(rSearchItem, rMark);
			else if (nCommand == SVX_SEARCHCMD_REPLACE_ALL)
				bFound = ReplaceAllStyle(rSearchItem, rMark, pUndoDoc);
		}
		else
		{
			//	SearchParam no longer needed - SearchOptions contains all settings
			com::sun::star::util::SearchOptions aSearchOptions = rSearchItem.GetSearchOptions();
			aSearchOptions.Locale = *ScGlobal::GetLocale();

			//	#107259# reflect UseAsianOptions flag in SearchOptions
			//	(use only ignore case and width if asian options are disabled).
			//	This is also done in SvxSearchDialog CommandHdl, but not in API object.
			if ( !rSearchItem.IsUseAsianOptions() )
				aSearchOptions.transliterateFlags &=
					( com::sun::star::i18n::TransliterationModules_IGNORE_CASE |
					  com::sun::star::i18n::TransliterationModules_IGNORE_WIDTH );

			pSearchText = new utl::TextSearch( aSearchOptions );

			if (nCommand == SVX_SEARCHCMD_FIND)
				bFound = Search(rSearchItem, rCol, rRow, rMark, rUndoStr, pUndoDoc);
			else if (nCommand == SVX_SEARCHCMD_FIND_ALL)
				bFound = SearchAll(rSearchItem, rMark, rUndoStr, pUndoDoc);
			else if (nCommand == SVX_SEARCHCMD_REPLACE)
				bFound = Replace(rSearchItem, rCol, rRow, rMark, rUndoStr, pUndoDoc);
			else if (nCommand == SVX_SEARCHCMD_REPLACE_ALL)
				bFound = ReplaceAll(rSearchItem, rMark, rUndoStr, pUndoDoc);

			delete pSearchText;
			pSearchText = NULL;
		}
	}
	return bFound;
}






