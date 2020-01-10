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



// INCLUDE ---------------------------------------------------------

#include <comphelper/processfactory.hxx>
#include <comphelper/types.hxx>
#include <vcl/msgbox.hxx>
#include <tools/debug.hxx>
#include <svx/dataaccessdescriptor.hxx>
#include <sfx2/viewfrm.hxx>

#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/sdb/XCompletedExecution.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XRowSet.hpp>
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>


#include "dbdocfun.hxx"
#include "docsh.hxx"
#include "globstr.hrc"
#include "scerrors.hxx"
#include "dbcolect.hxx"
#include "markdata.hxx"
#include "undodat.hxx"
#include "progress.hxx"
#include "patattr.hxx"
#include "docpool.hxx"
#include "attrib.hxx"
#include "dbdocutl.hxx"
#include "editable.hxx"
#include "hints.hxx"

using namespace com::sun::star;

#define SC_SERVICE_ROWSET			"com.sun.star.sdb.RowSet"
#define SC_SERVICE_INTHANDLER		"com.sun.star.sdb.InteractionHandler"

//!	move to a header file?
#define SC_DBPROP_DATASOURCENAME	"DataSourceName"
#define SC_DBPROP_COMMAND			"Command"
#define SC_DBPROP_COMMANDTYPE		"CommandType"
#define SC_DBPROP_SELECTION			"Selection"
#define SC_DBPROP_CURSOR			"Cursor"

// static
void ScDBDocFunc::ShowInBeamer( const ScImportParam& rParam, SfxViewFrame* pFrame )
{
	//	called after opening the database beamer

	if ( !pFrame || !rParam.bImport )
		return;

	uno::Reference<frame::XFrame> xFrame = pFrame->GetFrame()->GetFrameInterface();
	uno::Reference<frame::XDispatchProvider> xDP(xFrame, uno::UNO_QUERY);

	uno::Reference<frame::XFrame> xBeamerFrame = xFrame->findFrame(
										rtl::OUString::createFromAscii("_beamer"),
										frame::FrameSearchFlag::CHILDREN);
	if (xBeamerFrame.is())
	{
		uno::Reference<frame::XController> xController = xBeamerFrame->getController();
		uno::Reference<view::XSelectionSupplier> xControllerSelection(xController, uno::UNO_QUERY);
		if (xControllerSelection.is())
		{
			sal_Int32 nType = rParam.bSql ? sdb::CommandType::COMMAND :
						( (rParam.nType == ScDbQuery) ? sdb::CommandType::QUERY :
														sdb::CommandType::TABLE );

            ::svx::ODataAccessDescriptor aSelection;
            aSelection.setDataSource(rtl::OUString( rParam.aDBName ));
            aSelection[svx::daCommand]      <<= rtl::OUString( rParam.aStatement );
            aSelection[svx::daCommandType]  <<= nType;

			xControllerSelection->select(uno::makeAny(aSelection.createPropertyValueSequence()));
		}
		else
		{
			DBG_ERROR("no selection supplier in the beamer!");
		}
	}
}

// -----------------------------------------------------------------

BOOL ScDBDocFunc::DoImportUno( const ScAddress& rPos,
								const uno::Sequence<beans::PropertyValue>& aArgs )
{
	BOOL bDone = FALSE;

	ScImportParam aImParam;
	aImParam.nCol1 = aImParam.nCol2 = rPos.Col();
	aImParam.nRow1 = aImParam.nRow2 = rPos.Row();
	aImParam.bImport = TRUE;

	uno::Reference<sdbc::XResultSet> xResSet;
	uno::Sequence<uno::Any> aSelection;

	rtl::OUString aStrVal;
	const beans::PropertyValue* pPropArray = aArgs.getConstArray();
	long nPropCount = aArgs.getLength();
	long i;
	for (i = 0; i < nPropCount; i++)
	{
		const beans::PropertyValue& rProp = pPropArray[i];
		String aPropName = rProp.Name;

		if ( aPropName.EqualsAscii( SC_DBPROP_DATASOURCENAME ))
		{
			if ( rProp.Value >>= aStrVal )
				aImParam.aDBName = aStrVal;
		}
		else if ( aPropName.EqualsAscii( SC_DBPROP_COMMAND ))
		{
			if ( rProp.Value >>= aStrVal )
				aImParam.aStatement = aStrVal;
		}
		else if ( aPropName.EqualsAscii( SC_DBPROP_COMMANDTYPE ))
		{
			sal_Int32 nType = 0;
			if ( rProp.Value >>= nType )
			{
				aImParam.bSql = ( nType == sdb::CommandType::COMMAND );
                aImParam.nType = sal::static_int_cast<BYTE>( ( nType == sdb::CommandType::QUERY ) ? ScDbQuery : ScDbTable );
				// nType is ignored if bSql is set
			}
		}
		else if ( aPropName.EqualsAscii( SC_DBPROP_SELECTION ))
		{
			rProp.Value >>= aSelection;
		}
		else if ( aPropName.EqualsAscii( SC_DBPROP_CURSOR ))
		{
			rProp.Value >>= xResSet;
		}
	}

	SbaSelectionList aList;
	long nSelLen = aSelection.getLength();
	for (i = 0; i < nSelLen; i++)
	{
		sal_Int32 nEntry = 0;
		if ( aSelection[i] >>= nEntry )
			aList.Insert( (void*)nEntry, LIST_APPEND );
	}

	BOOL bAddrInsert = FALSE;		//!???
	if ( bAddrInsert )
	{
		bDone = DoImport( rPos.Tab(), aImParam, xResSet, &aList, TRUE, bAddrInsert );
	}
	else
	{
		//	create database range
		//!	merge this with SID_SBA_IMPORT execute in docsh4.cxx

		ScDBData* pDBData = rDocShell.GetDBData( ScRange(rPos), SC_DB_IMPORT, FALSE );
		DBG_ASSERT(pDBData, "can't create DB data");
		String sTarget = pDBData->GetName();

		//!	change UpdateImport to use only one of rTableName, rStatement

		String aTableName, aStatement;
		if ( aImParam.bSql )
			aStatement = aImParam.aStatement;
		else
			aTableName = aImParam.aStatement;

        UpdateImport( sTarget, aImParam.aDBName, aTableName, aStatement,
                aImParam.bNative, aImParam.nType, xResSet, &aList );
		bDone = TRUE;
	}

	return bDone;
}

// -----------------------------------------------------------------

BOOL ScDBDocFunc::DoImport( SCTAB nTab, const ScImportParam& rParam,
        const uno::Reference< sdbc::XResultSet >& xResultSet,
        const SbaSelectionList* pSelection, BOOL bRecord, BOOL bAddrInsert )
{
	ScDocument* pDoc = rDocShell.GetDocument();

	if (bRecord && !pDoc->IsUndoEnabled())
		bRecord = FALSE;

	ScDBData* pDBData = 0;
	if ( !bAddrInsert )
	{
		pDBData = pDoc->GetDBAtArea( nTab, rParam.nCol1, rParam.nRow1,
											rParam.nCol2, rParam.nRow2 );
		if (!pDBData)
		{
			DBG_ERROR( "DoImport: no DBData" );
			return FALSE;
		}
	}

	Window* pWaitWin = rDocShell.GetActiveDialogParent();
	if (pWaitWin)
		pWaitWin->EnterWait();
	ScDocShellModificator aModificator( rDocShell );

	BOOL bSuccess = FALSE;
	BOOL bApi = FALSE;						//! pass as argument
	BOOL bTruncated = FALSE;				// for warning
	USHORT nErrStringId = 0;
	String aErrorMessage;

	SCCOL nCol = rParam.nCol1;
	SCROW nRow = rParam.nRow1;
	SCCOL nEndCol = nCol;					// end of resulting database area
	SCROW nEndRow = nRow;
	long i;

	BOOL bDoSelection = FALSE;
	BOOL bRealSelection = FALSE;			// TRUE if not everything is selected
	ULONG nListPos = 0;
	ULONG nRowsRead = 0;
	ULONG nListCount = 0;

	//	-1 is special
	if ( pSelection && pSelection->Count() && (long)pSelection->GetObject(0) != -1L )
	{
		bDoSelection = TRUE;
		nListCount = pSelection->Count();
	}

	// ImportDoc - also used for Redo
	ScDocument* pImportDoc = new ScDocument( SCDOCMODE_UNDO );
	pImportDoc->InitUndo( pDoc, nTab, nTab );
	ScColumn::bDoubleAlloc = TRUE;

	//
	//	get data from database into import document
	//

	try
	{
		//	progress bar
		//	only text (title is still needed, for the cancel button)
		ScProgress aProgress( &rDocShell, ScGlobal::GetRscString(STR_UNDO_IMPORTDATA), 0 );
		USHORT nInserted = 0;

        uno::Reference<sdbc::XRowSet> xRowSet = uno::Reference<sdbc::XRowSet>(
                xResultSet, uno::UNO_QUERY );
        sal_Bool bDispose = sal_False;
        if ( !xRowSet.is() )
        {
            bDispose = sal_True;
            xRowSet = uno::Reference<sdbc::XRowSet>(
                    comphelper::getProcessServiceFactory()->createInstance(
                        rtl::OUString::createFromAscii( SC_SERVICE_ROWSET ) ),
                    uno::UNO_QUERY);
            uno::Reference<beans::XPropertySet> xRowProp( xRowSet, uno::UNO_QUERY );
            DBG_ASSERT( xRowProp.is(), "can't get RowSet" );
            if ( xRowProp.is() )
            {
                //
                //	set source parameters
                //

                sal_Int32 nType = rParam.bSql ? sdb::CommandType::COMMAND :
                            ( (rParam.nType == ScDbQuery) ? sdb::CommandType::QUERY :
                                                            sdb::CommandType::TABLE );
                uno::Any aAny;

                aAny <<= rtl::OUString( rParam.aDBName );
                xRowProp->setPropertyValue(
                            rtl::OUString::createFromAscii(SC_DBPROP_DATASOURCENAME), aAny );

                aAny <<= rtl::OUString( rParam.aStatement );
                xRowProp->setPropertyValue(
                            rtl::OUString::createFromAscii(SC_DBPROP_COMMAND), aAny );

                aAny <<= nType;
                xRowProp->setPropertyValue(
                            rtl::OUString::createFromAscii(SC_DBPROP_COMMANDTYPE), aAny );

                uno::Reference<sdb::XCompletedExecution> xExecute( xRowSet, uno::UNO_QUERY );
                if ( xExecute.is() )
                {
                    uno::Reference<task::XInteractionHandler> xHandler(
                            comphelper::getProcessServiceFactory()->createInstance(
                                rtl::OUString::createFromAscii( SC_SERVICE_INTHANDLER ) ),
                            uno::UNO_QUERY);
                    xExecute->executeWithCompletion( xHandler );
                }
                else
                    xRowSet->execute();
            }
        }
        if ( xRowSet.is() )
        {
			//
			//	get column descriptions
			//

			long nColCount = 0;
			uno::Reference<sdbc::XResultSetMetaData> xMeta;
			uno::Reference<sdbc::XResultSetMetaDataSupplier> xMetaSupp( xRowSet, uno::UNO_QUERY );
			if ( xMetaSupp.is() )
				xMeta = xMetaSupp->getMetaData();
			if ( xMeta.is() )
				nColCount = xMeta->getColumnCount();	// this is the number of real columns

			if ( rParam.nCol1 + nColCount - 1 > MAXCOL )
			{
				nColCount = 0;
				//!	error message
			}

			uno::Reference<sdbc::XRow> xRow( xRowSet, uno::UNO_QUERY );
			if ( nColCount > 0 && xRow.is() )
			{
				nEndCol = (SCCOL)( rParam.nCol1 + nColCount - 1 );

				uno::Sequence<sal_Int32> aColTypes( nColCount );	// column types
				uno::Sequence<sal_Bool> aColCurr( nColCount );		// currency flag is not in types
				sal_Int32* pTypeArr = aColTypes.getArray();
				sal_Bool* pCurrArr = aColCurr.getArray();
				for (i=0; i<nColCount; i++)
				{
					pTypeArr[i] = xMeta->getColumnType( i+1 );
					pCurrArr[i] = xMeta->isCurrency( i+1 );
				}

				if ( !bAddrInsert )					// read column names
				{
					nCol = rParam.nCol1;
					for (i=0; i<nColCount; i++)
					{
						pImportDoc->SetString( nCol, nRow, nTab,
												xMeta->getColumnLabel( i+1 ) );
						++nCol;
					}
					++nRow;
				}

				BOOL bEnd = FALSE;
				if ( !bDoSelection )
					xRowSet->beforeFirst();
				while ( !bEnd )
				{
					//	skip rows that are not selected
					if ( !bDoSelection )
					{
                        if ( (bEnd = !xRowSet->next()) == FALSE )
							++nRowsRead;
					}
					else
					{
						if (nListPos < nListCount)
						{
							ULONG nNextRow = (ULONG) pSelection->GetObject(nListPos);
							if ( nRowsRead+1 < nNextRow )
								bRealSelection = TRUE;
							bEnd = !xRowSet->absolute(nRowsRead = nNextRow);
							++nListPos;
						}
						else
						{
							bRealSelection = xRowSet->next();
							bEnd = TRUE; // more data available but not used
						}
					}

					if ( !bEnd )
					{
						if ( ValidRow(nRow) )
						{
							nCol = rParam.nCol1;
							for (i=0; i<nColCount; i++)
							{
								ScDatabaseDocUtil::PutData( pImportDoc, nCol, nRow, nTab,
												xRow, i+1, pTypeArr[i], pCurrArr[i] );
								++nCol;
							}
							nEndRow = nRow;
							++nRow;

							//	progress bar

							++nInserted;
							if (!(nInserted & 15))
							{
								String aPict = ScGlobal::GetRscString( STR_PROGRESS_IMPORT );
								String aText = aPict.GetToken(0,'#');
								aText += String::CreateFromInt32( nInserted );
								aText += aPict.GetToken(1,'#');

								if (!aProgress.SetStateText( 0, aText ))	// stopped by user?
								{
									bEnd = TRUE;
									bSuccess = FALSE;
									nErrStringId = STR_DATABASE_ABORTED;
								}
							}
						}
						else		// past the end of the spreadsheet
						{
							bEnd = TRUE;			// don't continue
							bTruncated = TRUE;		// warning flag
						}
					}
				}

				bSuccess = TRUE;
			}

            if ( bDispose )
                ::comphelper::disposeComponent( xRowSet );
		}
	}
	catch ( sdbc::SQLException& rError )
	{
		aErrorMessage = rError.Message;
	}
	catch ( uno::Exception& )
	{
		DBG_ERROR("Unexpected exception in database");
	}

	ScColumn::bDoubleAlloc = FALSE;
	pImportDoc->DoColResize( nTab, rParam.nCol1,nEndCol, 0 );

	//
	//	test for cell protection
	//

	BOOL bKeepFormat = !bAddrInsert && pDBData->IsKeepFmt();
	BOOL bMoveCells = !bAddrInsert && pDBData->IsDoSize();
	SCCOL nFormulaCols = 0;	// columns to be filled with formulas
	if (bMoveCells && nEndCol == rParam.nCol2)
	{
		//	if column count changes, formulas would become invalid anyway
		//	-> only set nFormulaCols for unchanged column count

		SCCOL nTestCol = rParam.nCol2 + 1;		// right of the data
		SCROW nTestRow = rParam.nRow1 + 1;		// below the title row
		while ( nTestCol <= MAXCOL &&
				pDoc->GetCellType(ScAddress( nTestCol, nTestRow, nTab )) == CELLTYPE_FORMULA )
			++nTestCol, ++nFormulaCols;
	}

	if (bSuccess)
	{
		//	old and new range editable?
		ScEditableTester aTester;
		aTester.TestBlock( pDoc, nTab, rParam.nCol1,rParam.nRow1,rParam.nCol2,rParam.nRow2 );
		aTester.TestBlock( pDoc, nTab, rParam.nCol1,rParam.nRow1,nEndCol,nEndRow );
		if ( !aTester.IsEditable() )
		{
			nErrStringId = aTester.GetMessageId();
			bSuccess = FALSE;
		}
		else if ( pDoc->GetChangeTrack() != NULL )
		{
			nErrStringId = STR_PROTECTIONERR;
			bSuccess = FALSE;
		}
	}

	if ( bSuccess && bMoveCells )
	{
		ScRange aOld( rParam.nCol1, rParam.nRow1, nTab,
						rParam.nCol2+nFormulaCols, rParam.nRow2, nTab );
		ScRange aNew( rParam.nCol1, rParam.nRow1, nTab,
						nEndCol+nFormulaCols, nEndRow, nTab );
		if (!pDoc->CanFitBlock( aOld, aNew ))
		{
			nErrStringId = STR_MSSG_DOSUBTOTALS_2;		// can't insert cells
			bSuccess = FALSE;
		}
	}

	//
	//	copy data from import doc into real document
	//

	if ( bSuccess )
	{
		if (bKeepFormat)
		{
			//	keep formatting of title and first data row from the document
			//	CopyToDocument also copies styles, Apply... needs separate calls

			SCCOL nMinEndCol = Min( rParam.nCol2, nEndCol );	// not too much
            nMinEndCol = sal::static_int_cast<SCCOL>( nMinEndCol + nFormulaCols );  // only if column count unchanged
			pImportDoc->DeleteAreaTab( 0,0, MAXCOL,MAXROW, nTab, IDF_ATTRIB );
			pDoc->CopyToDocument( rParam.nCol1, rParam.nRow1, nTab,
									nMinEndCol, rParam.nRow1, nTab,
									IDF_ATTRIB, FALSE, pImportDoc );

			SCROW nDataStartRow = rParam.nRow1+1;
			for (SCCOL nCopyCol=rParam.nCol1; nCopyCol<=nMinEndCol; nCopyCol++)
			{
				const ScPatternAttr* pSrcPattern = pDoc->GetPattern(
													nCopyCol, nDataStartRow, nTab );
				pImportDoc->ApplyPatternAreaTab( nCopyCol, nDataStartRow, nCopyCol, nEndRow,
													nTab, *pSrcPattern );
				const ScStyleSheet* pStyle = pSrcPattern->GetStyleSheet();
				if (pStyle)
					pImportDoc->ApplyStyleAreaTab( nCopyCol, nDataStartRow, nCopyCol, nEndRow,
													nTab, *pStyle );
			}
		}

		//	don't set cell protection attribute if table is protected
		if (pDoc->IsTabProtected(nTab))
		{
			ScPatternAttr aPattern(pImportDoc->GetPool());
			aPattern.GetItemSet().Put( ScProtectionAttr( FALSE,FALSE,FALSE,FALSE ) );
			pImportDoc->ApplyPatternAreaTab( 0,0,MAXCOL,MAXROW, nTab, aPattern );
		}

		//
		//	copy old data for undo
		//

		SCCOL nUndoEndCol = Max( nEndCol, rParam.nCol2 );		// rParam = old end
		SCROW nUndoEndRow = Max( nEndRow, rParam.nRow2 );

		ScDocument* pUndoDoc = NULL;
		ScDBData* pUndoDBData = NULL;
		if ( bRecord )
		{
			pUndoDoc = new ScDocument( SCDOCMODE_UNDO );
			pUndoDoc->InitUndo( pDoc, nTab, nTab );

			if ( !bAddrInsert )
				pUndoDBData = new ScDBData( *pDBData );
		}

		ScMarkData aNewMark;
		aNewMark.SelectOneTable( nTab );

		if (bRecord)
		{
            // do not touch notes (ScUndoImportData does not support drawing undo)
            sal_uInt16 nCopyFlags = IDF_ALL & ~IDF_NOTE;

			//	nFormulaCols is set only if column count is unchanged
			pDoc->CopyToDocument( rParam.nCol1, rParam.nRow1, nTab,
									nEndCol+nFormulaCols, nEndRow, nTab,
                                    nCopyFlags, FALSE, pUndoDoc );
			if ( rParam.nCol2 > nEndCol )
				pDoc->CopyToDocument( nEndCol+1, rParam.nRow1, nTab,
										nUndoEndCol, nUndoEndRow, nTab,
                                        nCopyFlags, FALSE, pUndoDoc );
			if ( rParam.nRow2 > nEndRow )
				pDoc->CopyToDocument( rParam.nCol1, nEndRow+1, nTab,
										nUndoEndCol+nFormulaCols, nUndoEndRow, nTab,
                                        nCopyFlags, FALSE, pUndoDoc );
		}

		//
		//	move new data
		//

		if (bMoveCells)
		{
			//	clear only the range without the formulas,
			//	so the formula title and first row are preserved

			ScRange aDelRange( rParam.nCol1, rParam.nRow1, nTab,
								rParam.nCol2, rParam.nRow2, nTab );
            pDoc->DeleteAreaTab( aDelRange, IDF_ALL & ~IDF_NOTE );  // ohne die Formeln

			ScRange aOld( rParam.nCol1, rParam.nRow1, nTab,
							rParam.nCol2+nFormulaCols, rParam.nRow2, nTab );
			ScRange aNew( rParam.nCol1, rParam.nRow1, nTab,
							nEndCol+nFormulaCols, nEndRow, nTab );
			pDoc->FitBlock( aOld, aNew, FALSE );		// Formeln nicht loeschen
		}
		else if ( nEndCol < rParam.nCol2 )		// DeleteArea calls PutInOrder
			pDoc->DeleteArea( nEndCol+1, rParam.nRow1, rParam.nCol2, rParam.nRow2,
                                aNewMark, IDF_CONTENTS & ~IDF_NOTE );

		//	CopyToDocument doesn't remove contents
        pDoc->DeleteAreaTab( rParam.nCol1, rParam.nRow1, nEndCol, nEndRow, nTab, IDF_CONTENTS & ~IDF_NOTE );

		//	#41216# remove each column from ImportDoc after copying to reduce memory usage
		BOOL bOldAutoCalc = pDoc->GetAutoCalc();
		pDoc->SetAutoCalc( FALSE );				// outside of the loop
		for (SCCOL nCopyCol = rParam.nCol1; nCopyCol <= nEndCol; nCopyCol++)
		{
			pImportDoc->CopyToDocument( nCopyCol, rParam.nRow1, nTab, nCopyCol, nEndRow, nTab,
										IDF_ALL, FALSE, pDoc );
			pImportDoc->DeleteAreaTab( nCopyCol, rParam.nRow1, nCopyCol, nEndRow, nTab, IDF_CONTENTS );
			pImportDoc->DoColResize( nTab, nCopyCol, nCopyCol, 0 );
		}
		pDoc->SetAutoCalc( bOldAutoCalc );

		if (nFormulaCols > 0)				// copy formulas
		{
			if (bKeepFormat)			// formats for formulas
				pImportDoc->CopyToDocument( nEndCol+1, rParam.nRow1, nTab,
											nEndCol+nFormulaCols, nEndRow, nTab,
											IDF_ATTRIB, FALSE, pDoc );
			// fill formulas
			ScMarkData aMark;
			aMark.SelectOneTable(nTab);
			pDoc->Fill( nEndCol+1, rParam.nRow1+1, nEndCol+nFormulaCols, rParam.nRow1+1,
							aMark, nEndRow-rParam.nRow1-1, FILL_TO_BOTTOM, FILL_SIMPLE );
		}

		//	if new range is smaller, clear old contents

		if (!bMoveCells)		// move has happened above
		{
			if ( rParam.nCol2 > nEndCol )
				pDoc->DeleteArea( nEndCol+1, rParam.nRow1, rParam.nCol2, rParam.nRow2,
									aNewMark, IDF_CONTENTS );
			if ( rParam.nRow2 > nEndRow )
				pDoc->DeleteArea( rParam.nCol1, nEndRow+1, rParam.nCol2, rParam.nRow2,
									aNewMark, IDF_CONTENTS );
		}

		if( !bAddrInsert )		// update database range
		{
			pDBData->SetImportParam( rParam );
			pDBData->SetHeader( TRUE );
			pDBData->SetByRow( TRUE );
			pDBData->SetArea( nTab, rParam.nCol1,rParam.nRow1, nEndCol,nEndRow );
			pDBData->SetImportSelection( bRealSelection );
			pDoc->CompileDBFormula();
		}

		if (bRecord)
		{
			ScDocument* pRedoDoc = pImportDoc;
			pImportDoc = NULL;

			if (nFormulaCols > 0)					// include filled formulas for redo
				pDoc->CopyToDocument( rParam.nCol1, rParam.nRow1, nTab,
										nEndCol+nFormulaCols, nEndRow, nTab,
                                        IDF_ALL & ~IDF_NOTE, FALSE, pRedoDoc );

			ScDBData* pRedoDBData = pDBData ? new ScDBData( *pDBData ) : NULL;

			rDocShell.GetUndoManager()->AddUndoAction(
				new ScUndoImportData( &rDocShell, nTab,
										rParam, nUndoEndCol, nUndoEndRow,
										nFormulaCols,
										pUndoDoc, pRedoDoc, pUndoDBData, pRedoDBData ) );
		}

		pDoc->SetDirty();
		rDocShell.PostPaint( 0,0,nTab, MAXCOL,MAXROW,nTab, PAINT_GRID );
		aModificator.SetDocumentModified();

	    ScDBRangeRefreshedHint aHint( rParam );
	    pDoc->BroadcastUno( aHint );

		if (pWaitWin)
			pWaitWin->LeaveWait();

		if ( bTruncated && !bApi )			// show warning
			ErrorHandler::HandleError(SCWARN_IMPORT_RANGE_OVERFLOW);
	}
	else if ( !bApi )
	{
		if (pWaitWin)
			pWaitWin->LeaveWait();

		if (!aErrorMessage.Len())
		{
			if (!nErrStringId)
				nErrStringId = STR_MSSG_IMPORTDATA_0;
			aErrorMessage = ScGlobal::GetRscString( nErrStringId );
		}
		InfoBox aInfoBox( rDocShell.GetActiveDialogParent(), aErrorMessage );
		aInfoBox.Execute();
	}

	delete pImportDoc;

	return bSuccess;
}




