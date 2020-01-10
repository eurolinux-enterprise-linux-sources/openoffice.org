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

#include <com/sun/star/embed/XEmbeddedObject.hpp>
#include <com/sun/star/embed/XClassifiedObject.hpp>
#include <com/sun/star/chart2/data/XDataReceiver.hpp>

// INCLUDE ---------------------------------------------------------------

#include "scitems.hxx"
#include <svx/eeitem.hxx>

#include <sot/exchange.hxx>
#include <svx/akrnitem.hxx>
#include <svx/fontitem.hxx>
#include <svx/forbiddencharacterstable.hxx>
#include <svx/langitem.hxx>
#include <svx/svdetc.hxx>
#include <svx/svditer.hxx>
#include <svx/svdocapt.hxx>
#include <svx/svdograf.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdundo.hxx>
#include <svx/xtable.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/printer.hxx>
#include <svtools/saveopt.hxx>
#include <svtools/pathoptions.hxx>

#include "document.hxx"
#include "docoptio.hxx"
#include "table.hxx"
#include "drwlayer.hxx"
#include "markdata.hxx"
#include "patattr.hxx"
#include "rechead.hxx"
#include "poolhelp.hxx"
#include "docpool.hxx"
#include "chartarr.hxx"
#include "detfunc.hxx"		// for UpdateAllComments
#include "editutil.hxx"
#include "postit.hxx"

using namespace ::com::sun::star;

// -----------------------------------------------------------------------


SfxBroadcaster* ScDocument::GetDrawBroadcaster()
{
	return pDrawLayer;
}

void ScDocument::BeginDrawUndo()
{
	if (pDrawLayer)
		pDrawLayer->BeginCalcUndo();
}

XColorTable* ScDocument::GetColorTable()
{
	if (pDrawLayer)
		return pDrawLayer->GetColorTable();
	else
	{
		if (!pColorTable)
		{
			SvtPathOptions aPathOpt;
			pColorTable = new XColorTable( aPathOpt.GetPalettePath() );
		}

		return pColorTable;
	}
}

BOOL lcl_AdjustRanges( ScRangeList& rRanges, SCTAB nSource, SCTAB nDest, SCTAB nTabCount )
{
	//!	if multiple sheets are copied, update references into the other copied sheets?

	BOOL bChanged = FALSE;

	ULONG nCount = rRanges.Count();
	for (ULONG i=0; i<nCount; i++)
	{
		ScRange* pRange = rRanges.GetObject(i);
		if ( pRange->aStart.Tab() == nSource && pRange->aEnd.Tab() == nSource )
		{
			pRange->aStart.SetTab( nDest );
			pRange->aEnd.SetTab( nDest );
			bChanged = TRUE;
		}
		if ( pRange->aStart.Tab() >= nTabCount )
		{
			pRange->aStart.SetTab( nTabCount > 0 ? ( nTabCount - 1 ) : 0 );
			bChanged = TRUE;
		}
		if ( pRange->aEnd.Tab() >= nTabCount )
		{
			pRange->aEnd.SetTab( nTabCount > 0 ? ( nTabCount - 1 ) : 0 );
			bChanged = TRUE;
		}
	}

	return bChanged;
}

void ScDocument::TransferDrawPage(ScDocument* pSrcDoc, SCTAB nSrcPos, SCTAB nDestPos)
{
	if (pDrawLayer && pSrcDoc->pDrawLayer)
	{
		SdrPage* pOldPage = pSrcDoc->pDrawLayer->GetPage(static_cast<sal_uInt16>(nSrcPos));
		SdrPage* pNewPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nDestPos));

		if (pOldPage && pNewPage)
		{
			SdrObjListIter aIter( *pOldPage, IM_FLAT );
			SdrObject* pOldObject = aIter.Next();
			while (pOldObject)
			{
				// #116235#
				SdrObject* pNewObject = pOldObject->Clone();
				// SdrObject* pNewObject = pOldObject->Clone( pNewPage, pDrawLayer );
				pNewObject->SetModel(pDrawLayer);
				pNewObject->SetPage(pNewPage);

				pNewObject->NbcMove(Size(0,0));
				pNewPage->InsertObject( pNewObject );

				if (pDrawLayer->IsRecording())
					pDrawLayer->AddCalcUndo( new SdrUndoInsertObj( *pNewObject ) );

				//	#71726# if it's a chart, make sure the data references are valid
				//	(this must be after InsertObject!)

				if ( pNewObject->GetObjIdentifier() == OBJ_OLE2 )
				{
					uno::Reference< embed::XEmbeddedObject > xIPObj = ((SdrOle2Obj*)pNewObject)->GetObjRef();
					uno::Reference< embed::XClassifiedObject > xClassified( xIPObj, uno::UNO_QUERY );
					SvGlobalName aObjectClassName;
					if ( xClassified.is() )
				    {
						try {
							aObjectClassName = SvGlobalName( xClassified->getClassID() );
						} catch( uno::Exception& )
						{
							// TODO: handle error?
						}
					}

					if ( xIPObj.is() && SotExchange::IsChart( aObjectClassName ) )
					{
                        String aChartName = ((SdrOle2Obj*)pNewObject)->GetPersistName();

                        uno::Reference< chart2::XChartDocument > xChartDoc( GetChartByName( aChartName ) );
                        uno::Reference< chart2::data::XDataReceiver > xReceiver( xChartDoc, uno::UNO_QUERY );
                        if( xChartDoc.is() && xReceiver.is() )
                        {
                            if( !xChartDoc->hasInternalDataProvider() )
                            {
                                ::std::vector< ScRangeList > aRangesVector;
                                GetChartRanges( aChartName, aRangesVector, pSrcDoc );

                                ::std::vector< ScRangeList >::iterator aIt( aRangesVector.begin() );
                                for( ; aIt!=aRangesVector.end(); aIt++ )
                                {
                                    ScRangeList& rScRangeList( *aIt );
                                    lcl_AdjustRanges( rScRangeList, nSrcPos, nDestPos, GetTableCount() );
                                }
                                SetChartRanges( aChartName, aRangesVector );
                            }
                        }
					}
				}

				pOldObject = aIter.Next();
			}
		}
	}
}

void ScDocument::InitDrawLayer( SfxObjectShell* pDocShell )
{
	if (pDocShell && !pShell)
		pShell = pDocShell;

//	DBG_ASSERT(pShell,"InitDrawLayer ohne Shell");

	if (!pDrawLayer)
	{
		String aName;
		if ( pShell && !pShell->IsLoading() )		// #88438# don't call GetTitle while loading
			aName = pShell->GetTitle();
		pDrawLayer = new ScDrawLayer( this, aName );
		if (GetLinkManager())
			pDrawLayer->SetLinkManager( pLinkManager );

		//	Drawing pages are accessed by table number, so they must also be present
		//	for preceding table numbers, even if the tables aren't allocated
		//	(important for clipboard documents).

		SCTAB nDrawPages = 0;
		SCTAB nTab;
		for (nTab=0; nTab<=MAXTAB; nTab++)
			if (pTab[nTab])
				nDrawPages = nTab + 1;			// needed number of pages

		for (nTab=0; nTab<nDrawPages; nTab++)
		{
			pDrawLayer->ScAddPage( nTab );		// always add page, with or without the table
			if (pTab[nTab])
			{
                String aTabName;
                pTab[nTab]->GetName(aTabName);
                pDrawLayer->ScRenamePage( nTab, aTabName );

                pTab[nTab]->SetDrawPageSize(false,false);     // #54782# set the right size immediately
#if 0
				ULONG nx = (ULONG) ((double) (MAXCOL+1) * STD_COL_WIDTH			  * HMM_PER_TWIPS );
				ULONG ny = (ULONG) ((double) (MAXROW+1) * ScGlobal::nStdRowHeight * HMM_PER_TWIPS );
				pDrawLayer->SetPageSize( nTab, Size( nx, ny ) );
#endif
			}
		}

		pDrawLayer->SetDefaultTabulator( GetDocOptions().GetTabDistance() );

		UpdateDrawPrinter();
        UpdateDrawDefaults();
		UpdateDrawLanguages();
		if (bImportingXML)
			pDrawLayer->EnableAdjust(FALSE);

		pDrawLayer->SetForbiddenCharsTable( xForbiddenCharacters );
		pDrawLayer->SetCharCompressType( GetAsianCompression() );
		pDrawLayer->SetKernAsianPunctuation( GetAsianKerning() );
	}
}

void ScDocument::UpdateDrawLanguages()
{
	if (pDrawLayer)
	{
		SfxItemPool& rDrawPool = pDrawLayer->GetItemPool();
		rDrawPool.SetPoolDefaultItem( SvxLanguageItem( eLanguage, EE_CHAR_LANGUAGE ) );
		rDrawPool.SetPoolDefaultItem( SvxLanguageItem( eCjkLanguage, EE_CHAR_LANGUAGE_CJK ) );
		rDrawPool.SetPoolDefaultItem( SvxLanguageItem( eCtlLanguage, EE_CHAR_LANGUAGE_CTL ) );
	}
}

void ScDocument::UpdateDrawDefaults()
{
    // drawing layer defaults that are set for new documents (if InitNew was called)

    if ( pDrawLayer && bSetDrawDefaults )
    {
        SfxItemPool& rDrawPool = pDrawLayer->GetItemPool();
        rDrawPool.SetPoolDefaultItem( SvxAutoKernItem( TRUE, EE_CHAR_PAIRKERNING ) );
    }
}

void ScDocument::UpdateDrawPrinter()
{
	if (pDrawLayer)
	{
		// use the printer even if IsValid is false
		// Application::GetDefaultDevice causes trouble with changing MapModes

//		OutputDevice* pRefDev = GetPrinter();
//		pRefDev->SetMapMode( MAP_100TH_MM );
		pDrawLayer->SetRefDevice(GetRefDevice());
	}
}

sal_Bool ScDocument::IsChart( const SdrObject* pObject )
{
	// #109985#
	// IsChart() implementation moved to svx drawinglayer
	if(pObject && OBJ_OLE2 == pObject->GetObjIdentifier())
	{
		return ((SdrOle2Obj*)pObject)->IsChart();
	}

	return sal_False;
}

IMPL_LINK_INLINE_START( ScDocument, GetUserDefinedColor, USHORT *, pColorIndex )
{
	return (long) &((GetColorTable()->GetColor(*pColorIndex))->GetColor());
}
IMPL_LINK_INLINE_END( ScDocument, GetUserDefinedColor, USHORT *, pColorIndex )

void ScDocument::DeleteDrawLayer()
{
	delete pDrawLayer;
}

void ScDocument::DeleteColorTable()
{
	delete pColorTable;
}

BOOL ScDocument::DrawGetPrintArea( ScRange& rRange, BOOL bSetHor, BOOL bSetVer ) const
{
	return pDrawLayer->GetPrintArea( rRange, bSetHor, bSetVer );
}

void ScDocument::DrawMovePage( USHORT nOldPos, USHORT nNewPos )
{
	pDrawLayer->ScMovePage(nOldPos,nNewPos);
}

void ScDocument::DrawCopyPage( USHORT nOldPos, USHORT nNewPos )
{
	// angelegt wird die Page schon im ScTable ctor
	pDrawLayer->ScCopyPage( nOldPos, nNewPos, FALSE );
}

void ScDocument::DeleteObjectsInArea( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2,
						const ScMarkData& rMark )
{
	if (!pDrawLayer)
		return;

	SCTAB nTabCount = GetTableCount();
	for (SCTAB nTab=0; nTab<=nTabCount; nTab++)
		if (pTab[nTab] && rMark.GetTableSelect(nTab))
			pDrawLayer->DeleteObjectsInArea( nTab, nCol1, nRow1, nCol2, nRow2 );
}

void ScDocument::DeleteObjectsInSelection( const ScMarkData& rMark )
{
	if (!pDrawLayer)
		return;

	pDrawLayer->DeleteObjectsInSelection( rMark );
}

BOOL ScDocument::HasOLEObjectsInArea( const ScRange& rRange, const ScMarkData* pTabMark )
{
	//	pTabMark is used only for selected tables. If pTabMark is 0, all tables of rRange are used.

	if (!pDrawLayer)
		return FALSE;

	SCTAB nStartTab = 0;
	SCTAB nEndTab = MAXTAB;
	if ( !pTabMark )
	{
		nStartTab = rRange.aStart.Tab();
		nEndTab = rRange.aEnd.Tab();
	}

	for (SCTAB nTab = nStartTab; nTab <= nEndTab; nTab++)
	{
		if ( !pTabMark || pTabMark->GetTableSelect(nTab) )
		{
			Rectangle aMMRect = GetMMRect( rRange.aStart.Col(), rRange.aStart.Row(),
											rRange.aEnd.Col(), rRange.aEnd.Row(), nTab );

			SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
			DBG_ASSERT(pPage,"Page ?");
			if (pPage)
			{
				SdrObjListIter aIter( *pPage, IM_FLAT );
				SdrObject* pObject = aIter.Next();
				while (pObject)
				{
					if ( pObject->GetObjIdentifier() == OBJ_OLE2 &&
							aMMRect.IsInside( pObject->GetCurrentBoundRect() ) )
						return TRUE;

					pObject = aIter.Next();
				}
			}
		}
	}

	return FALSE;
}


void ScDocument::StartAnimations( SCTAB nTab, Window* pWin )
{
	if (!pDrawLayer)
		return;
	SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
	DBG_ASSERT(pPage,"Page ?");
	if (!pPage)
		return;

	SdrObjListIter aIter( *pPage, IM_FLAT );
	SdrObject* pObject = aIter.Next();
	while (pObject)
	{
		if (pObject->ISA(SdrGrafObj))
		{
			SdrGrafObj* pGrafObj = (SdrGrafObj*)pObject;
			if ( pGrafObj->IsAnimated() )
			{
				const Rectangle& rRect = pGrafObj->GetCurrentBoundRect();
				pGrafObj->StartAnimation( pWin, rRect.TopLeft(), rRect.GetSize() );
			}
		}
		pObject = aIter.Next();
	}
}

//UNUSED2008-05  void ScDocument::RefreshNoteFlags()
//UNUSED2008-05  {
//UNUSED2008-05      if (!pDrawLayer)
//UNUSED2008-05          return;
//UNUSED2008-05
//UNUSED2008-05      BOOL bAnyIntObj = FALSE;
//UNUSED2008-05      SCTAB nTab;
//UNUSED2008-05      ScPostIt aNote(this);
//UNUSED2008-05      for (nTab=0; nTab<=MAXTAB && pTab[nTab]; nTab++)
//UNUSED2008-05      {
//UNUSED2008-05          SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
//UNUSED2008-05          DBG_ASSERT(pPage,"Page ?");
//UNUSED2008-05          if (pPage)
//UNUSED2008-05          {
//UNUSED2008-05              SdrObjListIter aIter( *pPage, IM_FLAT );
//UNUSED2008-05              SdrObject* pObject = aIter.Next();
//UNUSED2008-05              while (pObject)
//UNUSED2008-05              {
//UNUSED2008-05                  if ( pObject->GetLayer() == SC_LAYER_INTERN )
//UNUSED2008-05                  {
//UNUSED2008-05                      bAnyIntObj = TRUE;  // for all internal objects, including detective
//UNUSED2008-05
//UNUSED2008-05                      if ( pObject->ISA( SdrCaptionObj ) )
//UNUSED2008-05                      {
//UNUSED2008-05                          ScDrawObjData* pData = ScDrawLayer::GetObjData( pObject );
//UNUSED2008-05                          if ( pData )
//UNUSED2008-05                          {
//UNUSED2008-05                              if ( GetNote( pData->aStt.Col(), pData->aStt.Row(), nTab, aNote))
//UNUSED2008-05                                  if ( !aNote.IsShown() )
//UNUSED2008-05                                  {
//UNUSED2008-05                                      aNote.SetShown(TRUE);
//UNUSED2008-05                                      SetNote( pData->aStt.Col(), pData->aStt.Row(), nTab, aNote);
//UNUSED2008-05                                  }
//UNUSED2008-05                          }
//UNUSED2008-05                      }
//UNUSED2008-05                  }
//UNUSED2008-05                  pObject = aIter.Next();
//UNUSED2008-05              }
//UNUSED2008-05          }
//UNUSED2008-05      }
//UNUSED2008-05
//UNUSED2008-05      if (bAnyIntObj)
//UNUSED2008-05      {
//UNUSED2008-05          //  update attributes for all note objects and the colors of detective objects
//UNUSED2008-05          //  (we don't know with which settings the file was created)
//UNUSED2008-05
//UNUSED2008-05          ScDetectiveFunc aFunc( this, 0 );
//UNUSED2008-05          aFunc.UpdateAllComments();
//UNUSED2008-05          aFunc.UpdateAllArrowColors();
//UNUSED2008-05      }
//UNUSED2008-05  }

BOOL ScDocument::HasBackgroundDraw( SCTAB nTab, const Rectangle& rMMRect )
{
	//	Gibt es Objekte auf dem Hintergrund-Layer, die (teilweise) von rMMRect
	//	betroffen sind?
	//	(fuer Drawing-Optimierung, vor dem Hintergrund braucht dann nicht geloescht
	//	 zu werden)

	if (!pDrawLayer)
		return FALSE;
	SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
	DBG_ASSERT(pPage,"Page ?");
	if (!pPage)
		return FALSE;

	BOOL bFound = FALSE;

	SdrObjListIter aIter( *pPage, IM_FLAT );
	SdrObject* pObject = aIter.Next();
	while (pObject && !bFound)
	{
		if ( pObject->GetLayer() == SC_LAYER_BACK && pObject->GetCurrentBoundRect().IsOver( rMMRect ) )
			bFound = TRUE;
		pObject = aIter.Next();
	}

	return bFound;
}

BOOL ScDocument::HasAnyDraw( SCTAB nTab, const Rectangle& rMMRect )
{
	//	Gibt es ueberhaupt Objekte, die (teilweise) von rMMRect
	//	betroffen sind?
	//	(um leere Seiten beim Drucken zu erkennen)

	if (!pDrawLayer)
		return FALSE;
	SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
	DBG_ASSERT(pPage,"Page ?");
	if (!pPage)
		return FALSE;

	BOOL bFound = FALSE;

	SdrObjListIter aIter( *pPage, IM_FLAT );
	SdrObject* pObject = aIter.Next();
	while (pObject && !bFound)
	{
		if ( pObject->GetCurrentBoundRect().IsOver( rMMRect ) )
			bFound = TRUE;
		pObject = aIter.Next();
	}

	return bFound;
}

void ScDocument::EnsureGraphicNames()
{
	if (pDrawLayer)
		pDrawLayer->EnsureGraphicNames();
}

SdrObject* ScDocument::GetObjectAtPoint( SCTAB nTab, const Point& rPos )
{
	//	fuer Drag&Drop auf Zeichenobjekt

	SdrObject* pFound = NULL;
	if (pDrawLayer && pTab[nTab])
	{
		SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
		DBG_ASSERT(pPage,"Page ?");
		if (pPage)
		{
			SdrObjListIter aIter( *pPage, IM_FLAT );
			SdrObject* pObject = aIter.Next();
			while (pObject)
			{
				if ( pObject->GetCurrentBoundRect().IsInside(rPos) )
				{
					//	Intern interessiert gar nicht
					//	Objekt vom Back-Layer nur, wenn kein Objekt von anderem Layer getroffen

					SdrLayerID nLayer = pObject->GetLayer();
                    if ( (nLayer != SC_LAYER_INTERN) && (nLayer != SC_LAYER_HIDDEN) )
					{
						if ( nLayer != SC_LAYER_BACK ||
								!pFound || pFound->GetLayer() == SC_LAYER_BACK )
						{
							pFound = pObject;
						}
					}
				}
				//	weitersuchen -> letztes (oberstes) getroffenes Objekt nehmen

				pObject = aIter.Next();
			}
		}
	}
	return pFound;
}

BOOL ScDocument::IsPrintEmpty( SCTAB nTab, SCCOL nStartCol, SCROW nStartRow,
								SCCOL nEndCol, SCROW nEndRow, BOOL bLeftIsEmpty,
								ScRange* pLastRange, Rectangle* pLastMM ) const
{
	if (!IsBlockEmpty( nTab, nStartCol, nStartRow, nEndCol, nEndRow ))
		return FALSE;

	ScDocument* pThis = (ScDocument*)this;	//! GetMMRect / HasAnyDraw etc. const !!!

	Rectangle aMMRect;
	if ( pLastRange && pLastMM && nTab == pLastRange->aStart.Tab() &&
			nStartRow == pLastRange->aStart.Row() && nEndRow == pLastRange->aEnd.Row() )
	{
		//	keep vertical part of aMMRect, only update horizontal position
		aMMRect = *pLastMM;

		long nLeft = 0;
		SCCOL i;
		for (i=0; i<nStartCol; i++)
			nLeft += GetColWidth(i,nTab);
		long nRight = nLeft;
		for (i=nStartCol; i<=nEndCol; i++)
			nRight += GetColWidth(i,nTab);

		aMMRect.Left()  = (long)(nLeft  * HMM_PER_TWIPS);
		aMMRect.Right() = (long)(nRight * HMM_PER_TWIPS);
	}
	else
		aMMRect = pThis->GetMMRect( nStartCol, nStartRow, nEndCol, nEndRow, nTab );

	if ( pLastRange && pLastMM )
	{
		*pLastRange = ScRange( nStartCol, nStartRow, nTab, nEndCol, nEndRow, nTab );
		*pLastMM = aMMRect;
	}

	if ( pThis->HasAnyDraw( nTab, aMMRect ))
		return FALSE;

	if ( nStartCol > 0 && !bLeftIsEmpty )
	{
		//	aehnlich wie in ScPrintFunc::AdjustPrintArea
		//!	ExtendPrintArea erst ab Start-Spalte des Druckbereichs

		SCCOL nExtendCol = nStartCol - 1;
		SCROW nTmpRow = nEndRow;

		pThis->ExtendMerge( 0,nStartRow, nExtendCol,nTmpRow, nTab,
							FALSE, TRUE );		// kein Refresh, incl. Attrs

		OutputDevice* pDev = pThis->GetPrinter();
		pDev->SetMapMode( MAP_PIXEL );				// wichtig fuer GetNeededSize
		pThis->ExtendPrintArea( pDev, nTab, 0, nStartRow, nExtendCol, nEndRow );
		if ( nExtendCol >= nStartCol )
			return FALSE;
	}

	return TRUE;
}

void ScDocument::Clear( sal_Bool bFromDestructor )
{
	for (SCTAB i=0; i<=MAXTAB; i++)
		if (pTab[i])
		{
			delete pTab[i];
			pTab[i]=NULL;
		}
	delete pSelectionAttr;
	pSelectionAttr = NULL;

	if (pDrawLayer)
	{
		// #116168#
		//pDrawLayer->Clear();
		pDrawLayer->ClearModel( bFromDestructor );
	}
}

BOOL ScDocument::HasControl( SCTAB nTab, const Rectangle& rMMRect )
{
	BOOL bFound = FALSE;

	if (pDrawLayer)
	{
		SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
		DBG_ASSERT(pPage,"Page ?");
		if (pPage)
		{
			SdrObjListIter aIter( *pPage, IM_DEEPNOGROUPS );
			SdrObject* pObject = aIter.Next();
			while (pObject && !bFound)
			{
				if (pObject->ISA(SdrUnoObj))
				{
					Rectangle aObjRect = pObject->GetLogicRect();
					if ( aObjRect.IsOver( rMMRect ) )
						bFound = TRUE;
				}

				pObject = aIter.Next();
			}
		}
	}

	return bFound;
}

void ScDocument::InvalidateControls( Window* pWin, SCTAB nTab, const Rectangle& rMMRect )
{
	if (pDrawLayer)
	{
		SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
		DBG_ASSERT(pPage,"Page ?");
		if (pPage)
		{
			SdrObjListIter aIter( *pPage, IM_DEEPNOGROUPS );
			SdrObject* pObject = aIter.Next();
			while (pObject)
			{
				if (pObject->ISA(SdrUnoObj))
				{
					Rectangle aObjRect = pObject->GetLogicRect();
					if ( aObjRect.IsOver( rMMRect ) )
					{
						//	Uno-Controls zeichnen sich immer komplett, ohne Ruecksicht
						//	auf ClippingRegions. Darum muss das ganze Objekt neu gepainted
						//	werden, damit die Selektion auf der Tabelle nicht uebermalt wird.

						//pWin->Invalidate( aObjRect.GetIntersection( rMMRect ) );
						pWin->Invalidate( aObjRect );
					}
				}

				pObject = aIter.Next();
			}
		}
	}
}

BOOL ScDocument::HasDetectiveObjects(SCTAB nTab) const
{
	//	looks for detective objects, annotations don't count
	//	(used to adjust scale so detective objects hit their cells better)

	BOOL bFound = FALSE;

	if (pDrawLayer)
	{
		SdrPage* pPage = pDrawLayer->GetPage(static_cast<sal_uInt16>(nTab));
		DBG_ASSERT(pPage,"Page ?");
		if (pPage)
		{
			SdrObjListIter aIter( *pPage, IM_DEEPNOGROUPS );
			SdrObject* pObject = aIter.Next();
			while (pObject && !bFound)
			{
				// anything on the internal layer except captions (annotations)
                if ( (pObject->GetLayer() == SC_LAYER_INTERN) && !ScDrawLayer::IsNoteCaption( pObject ) )
					bFound = TRUE;

				pObject = aIter.Next();
			}
		}
	}

	return bFound;
}

void ScDocument::UpdateFontCharSet()
{
	//	In alten Versionen (bis incl. 4.0 ohne SP) wurden beim Austausch zwischen
	//	Systemen die CharSets in den Font-Attributen nicht angepasst.
	//	Das muss fuer Dokumente bis incl SP2 nun nachgeholt werden:
	//	Alles, was nicht SYMBOL ist, wird auf den System-CharSet umgesetzt.
	//	Bei neuen Dokumenten (Version SC_FONTCHARSET) sollte der CharSet stimmen.

	BOOL bUpdateOld = ( nSrcVer < SC_FONTCHARSET );

	CharSet eSysSet = gsl_getSystemTextEncoding();
	if ( eSrcSet != eSysSet || bUpdateOld )
	{
		USHORT nCount,i;
		SvxFontItem* pItem;

		ScDocumentPool* pPool = xPoolHelper->GetDocPool();
		nCount = pPool->GetItemCount(ATTR_FONT);
		for (i=0; i<nCount; i++)
		{
			pItem = (SvxFontItem*)pPool->GetItem(ATTR_FONT, i);
			if ( pItem && ( pItem->GetCharSet() == eSrcSet ||
							( bUpdateOld && pItem->GetCharSet() != RTL_TEXTENCODING_SYMBOL ) ) )
				pItem->GetCharSet() = eSysSet;
		}

		if ( pDrawLayer )
		{
			SfxItemPool& rDrawPool = pDrawLayer->GetItemPool();
			nCount = rDrawPool.GetItemCount(EE_CHAR_FONTINFO);
			for (i=0; i<nCount; i++)
			{
				pItem = (SvxFontItem*)rDrawPool.GetItem(EE_CHAR_FONTINFO, i);
				if ( pItem && ( pItem->GetCharSet() == eSrcSet ||
								( bUpdateOld && pItem->GetCharSet() != RTL_TEXTENCODING_SYMBOL ) ) )
					pItem->GetCharSet() = eSysSet;
			}
		}
	}
}

void ScDocument::SetImportingXML( BOOL bVal )
{
	bImportingXML = bVal;
	if (pDrawLayer)
		pDrawLayer->EnableAdjust(!bImportingXML);

    if ( !bVal )
    {
        // #i57869# after loading, do the real RTL mirroring for the sheets that have the LoadingRTL flag set

        for ( SCTAB nTab=0; nTab<=MAXTAB && pTab[nTab]; nTab++ )
            if ( pTab[nTab]->IsLoadingRTL() )
            {
                pTab[nTab]->SetLoadingRTL( FALSE );
                SetLayoutRTL( nTab, TRUE );             // includes mirroring; bImportingXML must be cleared first
            }
    }
}

void ScDocument::SetXMLFromWrapper( BOOL bVal )
{
    bXMLFromWrapper = bVal;
}

vos::ORef<SvxForbiddenCharactersTable> ScDocument::GetForbiddenCharacters()
{
	return xForbiddenCharacters;
}

void ScDocument::SetForbiddenCharacters( const vos::ORef<SvxForbiddenCharactersTable> xNew )
{
	xForbiddenCharacters = xNew;
	if ( pEditEngine )
		pEditEngine->SetForbiddenCharsTable( xForbiddenCharacters );
	if ( pDrawLayer )
		pDrawLayer->SetForbiddenCharsTable( xForbiddenCharacters );
}

BOOL ScDocument::IsValidAsianCompression() const
{
	return ( nAsianCompression != SC_ASIANCOMPRESSION_INVALID );
}

BYTE ScDocument::GetAsianCompression() const
{
	if ( nAsianCompression == SC_ASIANCOMPRESSION_INVALID )
		return 0;
	else
		return nAsianCompression;
}

void ScDocument::SetAsianCompression(BYTE nNew)
{
	nAsianCompression = nNew;
	if ( pEditEngine )
		pEditEngine->SetAsianCompressionMode( nAsianCompression );
	if ( pDrawLayer )
		pDrawLayer->SetCharCompressType( nAsianCompression );
}

BOOL ScDocument::IsValidAsianKerning() const
{
	return ( nAsianKerning != SC_ASIANKERNING_INVALID );
}

BOOL ScDocument::GetAsianKerning() const
{
	if ( nAsianKerning == SC_ASIANKERNING_INVALID )
		return FALSE;
	else
		return (BOOL)nAsianKerning;
}

void ScDocument::SetAsianKerning(BOOL bNew)
{
	nAsianKerning = (BYTE)bNew;
	if ( pEditEngine )
		pEditEngine->SetKernAsianPunctuation( (BOOL)nAsianKerning );
	if ( pDrawLayer )
		pDrawLayer->SetKernAsianPunctuation( (BOOL)nAsianKerning );
}

