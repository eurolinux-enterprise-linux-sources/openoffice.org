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

#include <sfx2/app.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/sfxsids.hrc>
#include <svx/linkmgr.hxx>
#include <svtools/stritem.hxx>
#include <vcl/msgbox.hxx>

#include "arealink.hxx"

#include "tablink.hxx"
#include "document.hxx"
#include "docsh.hxx"
#include "rangenam.hxx"
#include "dbcolect.hxx"
#include "undoblk.hxx"
#include "globstr.hrc"
#include "markdata.hxx"
#include "hints.hxx"
#include "filter.hxx"
//CHINA001 #include "linkarea.hxx"			// dialog

#include "attrib.hxx"			// raus, wenn ResetAttrib am Dokument
#include "patattr.hxx"			// raus, wenn ResetAttrib am Dokument
#include "docpool.hxx"			// raus, wenn ResetAttrib am Dokument

#include "sc.hrc" //CHINA001
#include "scabstdlg.hxx" //CHINA001
#include "clipparam.hxx"

struct AreaLink_Impl
{
    ScDocShell* m_pDocSh;
    AbstractScLinkedAreaDlg* m_pDialog;

    AreaLink_Impl() : m_pDocSh( NULL ), m_pDialog( NULL ) {}
};

TYPEINIT1(ScAreaLink,::sfx2::SvBaseLink);

//------------------------------------------------------------------------

ScAreaLink::ScAreaLink( SfxObjectShell* pShell, const String& rFile,
						const String& rFilter, const String& rOpt,
						const String& rArea, const ScRange& rDest,
						ULONG nRefresh ) :
	::sfx2::SvBaseLink(sfx2::LINKUPDATE_ONCALL,FORMAT_FILE),
	ScRefreshTimer	( nRefresh ),
    pImpl           ( new AreaLink_Impl() ),
	aFileName		(rFile),
	aFilterName		(rFilter),
	aOptions		(rOpt),
	aSourceArea		(rArea),
	aDestArea		(rDest),
	bAddUndo		(TRUE),
	bInCreate		(FALSE),
	bDoInsert		(TRUE)
{
	DBG_ASSERT(pShell->ISA(ScDocShell), "ScAreaLink mit falscher ObjectShell");
    pImpl->m_pDocSh = static_cast< ScDocShell* >( pShell );
	SetRefreshHandler( LINK( this, ScAreaLink, RefreshHdl ) );
    SetRefreshControl( pImpl->m_pDocSh->GetDocument()->GetRefreshTimerControlAddress() );
}

__EXPORT ScAreaLink::~ScAreaLink()
{
	StopRefreshTimer();
    delete pImpl;
}

void __EXPORT ScAreaLink::Edit(Window* pParent, const Link& /* rEndEditHdl */ )
{
	//	use own dialog instead of SvBaseLink::Edit...
	//	DefModalDialogParent setzen, weil evtl. aus der DocShell beim ConvertFrom
	//	ein Optionen-Dialog kommt...

	ScAbstractDialogFactory* pFact = ScAbstractDialogFactory::Create();
	DBG_ASSERT(pFact, "ScAbstractFactory create fail!");//CHINA001

	AbstractScLinkedAreaDlg* pDlg = pFact->CreateScLinkedAreaDlg( pParent, RID_SCDLG_LINKAREA);
	DBG_ASSERT(pDlg, "Dialog create fail!");//CHINA001
	pDlg->InitFromOldLink( aFileName, aFilterName, aOptions, aSourceArea, GetRefreshDelay() );
    pImpl->m_pDialog = pDlg;
    pDlg->StartExecuteModal( LINK( this, ScAreaLink, AreaEndEditHdl ) );
}

void __EXPORT ScAreaLink::DataChanged( const String&,
									   const ::com::sun::star::uno::Any& )
{
	//	bei bInCreate nichts tun, damit Update gerufen werden kann, um den Status im
	//	LinkManager zu setzen, ohne die Daten im Dokument zu aendern

	if (bInCreate)
		return;

    SvxLinkManager* pLinkManager=pImpl->m_pDocSh->GetDocument()->GetLinkManager();
	if (pLinkManager!=NULL)
	{
		String aFile;
		String aFilter;
		String aArea;
		pLinkManager->GetDisplayNames( this,0,&aFile,&aArea,&aFilter);

		//	the file dialog returns the filter name with the application prefix
		//	-> remove prefix
		ScDocumentLoader::RemoveAppPrefix( aFilter );

		// #81155# dialog doesn't set area, so keep old one
		if ( !aArea.Len() )
		{
			aArea = aSourceArea;

			// adjust in dialog:
            String aNewLinkName;
            sfx2::MakeLnkName( aNewLinkName, NULL, aFile, aArea, &aFilter );
            SetName( aNewLinkName );
		}

		Refresh( aFile, aFilter, aArea, GetRefreshDelay() );
	}
}

void __EXPORT ScAreaLink::Closed()
{
	// Verknuepfung loeschen: Undo

    ScDocument* pDoc = pImpl->m_pDocSh->GetDocument();
	BOOL bUndo (pDoc->IsUndoEnabled());
	if (bAddUndo && bUndo)
	{
        pImpl->m_pDocSh->GetUndoManager()->AddUndoAction( new ScUndoRemoveAreaLink( pImpl->m_pDocSh,
														aFileName, aFilterName, aOptions,
														aSourceArea, aDestArea, GetRefreshDelay() ) );

		bAddUndo = FALSE;	// nur einmal
	}

    SCTAB nDestTab = aDestArea.aStart.Tab();
    if (pDoc->IsStreamValid(nDestTab))
        pDoc->SetStreamValid(nDestTab, FALSE);

	SvBaseLink::Closed();
}

void ScAreaLink::SetDestArea(const ScRange& rNew)
{
	aDestArea = rNew;			// fuer Undo
}

void ScAreaLink::SetSource(const String& rDoc, const String& rFlt, const String& rOpt,
								const String& rArea)
{
	aFileName	= rDoc;
	aFilterName	= rFlt;
	aOptions	= rOpt;
	aSourceArea	= rArea;

	//	also update link name for dialog
    String aNewLinkName;
    sfx2::MakeLnkName( aNewLinkName, NULL, aFileName, aSourceArea, &aFilterName );
    SetName( aNewLinkName );
}

BOOL ScAreaLink::IsEqual( const String& rFile, const String& rFilter, const String& rOpt,
							const String& rSource, const ScRange& rDest ) const
{
	return aFileName == rFile && aFilterName == rFilter && aOptions == rOpt &&
			aSourceArea == rSource && aDestArea.aStart == rDest.aStart;
}

// find a range with name >rAreaName< in >pSrcDoc<, return it in >rRange<
BOOL ScAreaLink::FindExtRange( ScRange& rRange, ScDocument* pSrcDoc, const String& rAreaName )
{
	BOOL bFound = FALSE;
	ScRangeName* pNames = pSrcDoc->GetRangeName();
	USHORT nPos;
	if (pNames)			// benannte Bereiche
	{
		if (pNames->SearchName( rAreaName, nPos ))
			if ( (*pNames)[nPos]->IsValidReference( rRange ) )
				bFound = TRUE;
	}
	if (!bFound)		// Datenbankbereiche
	{
		ScDBCollection*	pDBColl = pSrcDoc->GetDBCollection();
		if (pDBColl)
			if (pDBColl->SearchName( rAreaName, nPos ))
			{
                SCTAB nTab;
                SCCOL nCol1, nCol2;
                SCROW nRow1, nRow2;
                (*pDBColl)[nPos]->GetArea(nTab,nCol1,nRow1,nCol2,nRow2);
				rRange = ScRange( nCol1,nRow1,nTab, nCol2,nRow2,nTab );
				bFound = TRUE;
			}
	}
	if (!bFound)		// direct reference (range or cell)
	{
        ScAddress::Details aDetails(pSrcDoc->GetAddressConvention(), 0, 0);
		if ( rRange.ParseAny( rAreaName, pSrcDoc, aDetails ) & SCA_VALID )
			bFound = TRUE;
	}
	return bFound;
}

//	ausfuehren:

BOOL ScAreaLink::Refresh( const String& rNewFile, const String& rNewFilter,
							const String& rNewArea, ULONG nNewRefresh )
{
	//	Dokument laden - wie TabLink

	if (!rNewFile.Len() || !rNewFilter.Len())
		return FALSE;

    String aNewUrl( ScGlobal::GetAbsDocName( rNewFile, pImpl->m_pDocSh ) );
	BOOL bNewUrlName = (aNewUrl != aFileName);

    const SfxFilter* pFilter = pImpl->m_pDocSh->GetFactory().GetFilterContainer()->GetFilter4FilterName(rNewFilter);
	if (!pFilter)
		return FALSE;

    ScDocument* pDoc = pImpl->m_pDocSh->GetDocument();

	BOOL bUndo (pDoc->IsUndoEnabled());
	pDoc->SetInLinkUpdate( TRUE );

	//	wenn neuer Filter ausgewaehlt wurde, Optionen vergessen
	if ( rNewFilter != aFilterName )
		aOptions.Erase();

	//	ItemSet immer anlegen, damit die DocShell die Optionen setzen kann
	SfxItemSet* pSet = new SfxAllItemSet( SFX_APP()->GetPool() );
	if ( aOptions.Len() )
		pSet->Put( SfxStringItem( SID_FILE_FILTEROPTIONS, aOptions ) );

	SfxMedium* pMed = new SfxMedium(aNewUrl, STREAM_STD_READ, FALSE, pFilter);

	ScDocShell* pSrcShell = new ScDocShell(SFX_CREATE_MODE_INTERNAL);
//REMOVE		SvEmbeddedObjectRef aRef = pSrcShell;
	SfxObjectShellRef aRef = pSrcShell;
	pSrcShell->DoLoad(pMed);

	ScDocument* pSrcDoc = pSrcShell->GetDocument();

	// Optionen koennten gesetzt worden sein
	String aNewOpt = ScDocumentLoader::GetOptions(*pMed);
	if (!aNewOpt.Len())
		aNewOpt = aOptions;

	// correct source range name list for web query import
	String aTempArea;

	if( rNewFilter == ScDocShell::GetWebQueryFilterName() )
		aTempArea = ScFormatFilter::Get().GetHTMLRangeNameList( pSrcDoc, rNewArea );
	else
		aTempArea = rNewArea;

	// find total size of source area
	SCCOL nWidth = 0;
	SCROW nHeight = 0;
	xub_StrLen nTokenCnt = aTempArea.GetTokenCount( ';' );
	xub_StrLen nStringIx = 0;
	xub_StrLen nToken;

	for( nToken = 0; nToken < nTokenCnt; nToken++ )
	{
		String aToken( aTempArea.GetToken( 0, ';', nStringIx ) );
		ScRange aTokenRange;
		if( FindExtRange( aTokenRange, pSrcDoc, aToken ) )
		{
			// columns: find maximum
			nWidth = Max( nWidth, (SCCOL)(aTokenRange.aEnd.Col() - aTokenRange.aStart.Col() + 1) );
			// rows: add row range + 1 empty row
			nHeight += aTokenRange.aEnd.Row() - aTokenRange.aStart.Row() + 2;
		}
	}
	// remove the last empty row
	if( nHeight > 0 )
		nHeight--;

	//	alte Daten loeschen / neue kopieren

	ScAddress aDestPos = aDestArea.aStart;
	SCTAB nDestTab = aDestPos.Tab();
	ScRange aOldRange = aDestArea;
	ScRange aNewRange = aDestArea;			// alter Bereich, wenn Datei nicht gefunden o.ae.
	if (nWidth > 0 && nHeight > 0)
	{
		aNewRange.aEnd.SetCol( aNewRange.aStart.Col() + nWidth - 1 );
		aNewRange.aEnd.SetRow( aNewRange.aStart.Row() + nHeight - 1 );
	}

    //! check CanFitBlock only if bDoInsert is set?
    BOOL bCanDo = ValidColRow( aNewRange.aEnd.Col(), aNewRange.aEnd.Row() ) &&
                  pDoc->CanFitBlock( aOldRange, aNewRange );
	if (bCanDo)
	{
        ScDocShellModificator aModificator( *pImpl->m_pDocSh );

		SCCOL nOldEndX = aOldRange.aEnd.Col();
		SCROW nOldEndY = aOldRange.aEnd.Row();
		SCCOL nNewEndX = aNewRange.aEnd.Col();
		SCROW nNewEndY = aNewRange.aEnd.Row();
		ScRange aMaxRange( aDestPos,
					ScAddress(Max(nOldEndX,nNewEndX), Max(nOldEndY,nNewEndY), nDestTab) );

		//	Undo initialisieren

		ScDocument* pUndoDoc = NULL;
		ScDocument* pRedoDoc = NULL;
		if ( bAddUndo && bUndo )
		{
			pUndoDoc = new ScDocument( SCDOCMODE_UNDO );
			if ( bDoInsert )
			{
				if ( nNewEndX != nOldEndX || nNewEndY != nOldEndY )				// Bereich veraendert?
				{
					pUndoDoc->InitUndo( pDoc, 0, pDoc->GetTableCount()-1 );
					pDoc->CopyToDocument( 0,0,0,MAXCOL,MAXROW,MAXTAB,
											IDF_FORMULA, FALSE, pUndoDoc );		// alle Formeln
				}
				else
					pUndoDoc->InitUndo( pDoc, nDestTab, nDestTab );				// nur Zieltabelle
                pDoc->CopyToDocument( aOldRange, IDF_ALL & ~IDF_NOTE, FALSE, pUndoDoc );
			}
			else		// ohne Einfuegen
			{
				pUndoDoc->InitUndo( pDoc, nDestTab, nDestTab );				// nur Zieltabelle
                pDoc->CopyToDocument( aMaxRange, IDF_ALL & ~IDF_NOTE, FALSE, pUndoDoc );
			}
		}

		//	Zellen einfuegen / loeschen
		//	DeleteAreaTab loescht auch MERGE_FLAG Attribute

		if (bDoInsert)
			pDoc->FitBlock( aOldRange, aNewRange );			// incl. loeschen
		else
            pDoc->DeleteAreaTab( aMaxRange, IDF_ALL & ~IDF_NOTE );

		//	Daten kopieren

		if (nWidth > 0 && nHeight > 0)
		{
			ScDocument aClipDoc( SCDOCMODE_CLIP );
			ScRange aNewTokenRange( aNewRange.aStart );
			nStringIx = 0;
			for( nToken = 0; nToken < nTokenCnt; nToken++ )
			{
				String aToken( aTempArea.GetToken( 0, ';', nStringIx ) );
				ScRange aTokenRange;
				if( FindExtRange( aTokenRange, pSrcDoc, aToken ) )
				{
					SCTAB nSrcTab = aTokenRange.aStart.Tab();
					ScMarkData aSourceMark;
					aSourceMark.SelectOneTable( nSrcTab );		// selektieren fuer CopyToClip
					aSourceMark.SetMarkArea( aTokenRange );

                    ScClipParam aClipParam(aTokenRange, false);
                    pSrcDoc->CopyToClip(aClipParam, &aClipDoc, &aSourceMark);

					if ( aClipDoc.HasAttrib( 0,0,nSrcTab, MAXCOL,MAXROW,nSrcTab,
											HASATTR_MERGED | HASATTR_OVERLAPPED ) )
					{
						//!	ResetAttrib am Dokument !!!

						ScPatternAttr aPattern( pSrcDoc->GetPool() );
						aPattern.GetItemSet().Put( ScMergeAttr() );				// Defaults
						aPattern.GetItemSet().Put( ScMergeFlagAttr() );
						aClipDoc.ApplyPatternAreaTab( 0,0, MAXCOL,MAXROW, nSrcTab, aPattern );
					}

					aNewTokenRange.aEnd.SetCol( aNewTokenRange.aStart.Col() + (aTokenRange.aEnd.Col() - aTokenRange.aStart.Col()) );
					aNewTokenRange.aEnd.SetRow( aNewTokenRange.aStart.Row() + (aTokenRange.aEnd.Row() - aTokenRange.aStart.Row()) );
					ScMarkData aDestMark;
					aDestMark.SelectOneTable( nDestTab );
					aDestMark.SetMarkArea( aNewTokenRange );
					pDoc->CopyFromClip( aNewTokenRange, aDestMark, IDF_ALL, NULL, &aClipDoc, FALSE );
					aNewTokenRange.aStart.SetRow( aNewTokenRange.aEnd.Row() + 2 );
				}
			}
		}
		else
		{
			String aErr = ScGlobal::GetRscString(STR_LINKERROR);
			pDoc->SetString( aDestPos.Col(), aDestPos.Row(), aDestPos.Tab(), aErr );
		}

		//	Undo eintragen

		if ( bAddUndo && bUndo)
		{
			pRedoDoc = new ScDocument( SCDOCMODE_UNDO );
			pRedoDoc->InitUndo( pDoc, nDestTab, nDestTab );
            pDoc->CopyToDocument( aNewRange, IDF_ALL & ~IDF_NOTE, FALSE, pRedoDoc );

            pImpl->m_pDocSh->GetUndoManager()->AddUndoAction(
                new ScUndoUpdateAreaLink( pImpl->m_pDocSh,
											aFileName, aFilterName, aOptions,
											aSourceArea, aOldRange, GetRefreshDelay(),
											aNewUrl, rNewFilter, aNewOpt,
											rNewArea, aNewRange, nNewRefresh,
											pUndoDoc, pRedoDoc, bDoInsert ) );
		}

		//	neue Einstellungen merken

		if ( bNewUrlName )
			aFileName = aNewUrl;
		if ( rNewFilter != aFilterName )
			aFilterName = rNewFilter;
		if ( rNewArea != aSourceArea )
			aSourceArea = rNewArea;
		if ( aNewOpt != aOptions )
			aOptions = aNewOpt;

		if ( aNewRange != aDestArea )
			aDestArea = aNewRange;

		if ( nNewRefresh != GetRefreshDelay() )
			SetRefreshDelay( nNewRefresh );

		SCCOL nPaintEndX = Max( aOldRange.aEnd.Col(), aNewRange.aEnd.Col() );
		SCROW nPaintEndY = Max( aOldRange.aEnd.Row(), aNewRange.aEnd.Row() );

		if ( aOldRange.aEnd.Col() != aNewRange.aEnd.Col() )
			nPaintEndX = MAXCOL;
		if ( aOldRange.aEnd.Row() != aNewRange.aEnd.Row() )
			nPaintEndY = MAXROW;

        if ( !pImpl->m_pDocSh->AdjustRowHeight( aDestPos.Row(), nPaintEndY, nDestTab ) )
            pImpl->m_pDocSh->PostPaint( aDestPos.Col(),aDestPos.Row(),nDestTab,
									nPaintEndX,nPaintEndY,nDestTab, PAINT_GRID );
		aModificator.SetDocumentModified();
	}
	else
	{
		//	CanFitBlock FALSE -> Probleme mit zusammengefassten Zellen
		//						 oder Tabellengrenze erreicht!
		//!	Zellschutz ???

		//!	Link-Dialog muss Default-Parent setzen
		//	"kann keine Zeilen einfuegen"
		InfoBox aBox( Application::GetDefDialogParent(),
						ScGlobal::GetRscString( STR_MSSG_DOSUBTOTALS_2 ) );
		aBox.Execute();
	}

	//	aufraeumen

	aRef->DoClose();

	pDoc->SetInLinkUpdate( FALSE );

	if (bCanDo)
	{
		//	notify Uno objects (for XRefreshListener)
		//!	also notify Uno objects if file name was changed!
		ScLinkRefreshedHint aHint;
		aHint.SetAreaLink( aDestPos );
		pDoc->BroadcastUno( aHint );
	}

	return bCanDo;
}


IMPL_LINK( ScAreaLink, RefreshHdl, ScAreaLink*, EMPTYARG )
{
    long nRes = Refresh( aFileName, aFilterName, aSourceArea,
        GetRefreshDelay() ) != 0;
    return nRes;
}

IMPL_LINK( ScAreaLink, AreaEndEditHdl, void*, EMPTYARG )
{
    //  #i76514# can't use link argument to access the dialog,
    //  because it's the ScLinkedAreaDlg, not AbstractScLinkedAreaDlg

    if ( pImpl->m_pDialog && pImpl->m_pDialog->GetResult() == RET_OK )
    {
        aOptions = pImpl->m_pDialog->GetOptions();
        Refresh( pImpl->m_pDialog->GetURL(), pImpl->m_pDialog->GetFilter(),
                 pImpl->m_pDialog->GetSource(), pImpl->m_pDialog->GetRefresh() );

        //  copy source data from members (set in Refresh) into link name for dialog
        String aNewLinkName;
        sfx2::MakeLnkName( aNewLinkName, NULL, aFileName, aSourceArea, &aFilterName );
        SetName( aNewLinkName );
    }
    pImpl->m_pDialog = NULL;    // dialog is deleted with parent

    return 0;
}

