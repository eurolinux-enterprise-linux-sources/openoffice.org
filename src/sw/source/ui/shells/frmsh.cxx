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
#include "precompiled_sw.hxx"


#include <hintids.hxx>
#include <svtools/whiter.hxx>
#include <svtools/imapobj.hxx>
#include <svx/srchitem.hxx>
#include <svtools/imap.hxx>
#include <sfx2/viewfrm.hxx>
#include <basic/sbstar.hxx>
#include <svtools/rectitem.hxx>
#include <svtools/ptitem.hxx>
#include <svtools/stritem.hxx>
#include <svx/colritem.hxx>
#include <svx/bolnitem.hxx>
#include <svx/boxitem.hxx>
#include <svx/protitem.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/request.hxx>
#include <sfx2/objface.hxx>
#include <svx/hlnkitem.hxx>
// --> OD 2009-07-07 #i73249#
#include <svx/svdview.hxx>
#include <vcl/msgbox.hxx>
// <--


#include <fmturl.hxx>
#include <fmtclds.hxx>
#include <fmtcnct.hxx>
#include <swmodule.hxx>
#include <wrtsh.hxx>
#include <wview.hxx>
#include <frmatr.hxx>
#include <uitool.hxx>
#include <frmfmt.hxx>
#include <frmsh.hxx>
#include <frmmgr.hxx>
#include <frmdlg.hxx>
#include <swevent.hxx>
#include <usrpref.hxx>
#include <edtwin.hxx>
#include <swdtflvr.hxx>
#include <swwait.hxx>
#include <docstat.hxx>
#include <IDocumentStatistics.hxx>

#include <comphelper/processfactory.hxx>
#include <com/sun/star/ui/dialogs/XExecutableDialog.hpp>

#include <helpid.h>
#include <cmdid.h>
#include <globals.hrc>
#include <popup.hrc>
#include <shells.hrc>
#include "swabstdlg.hxx"
#include "misc.hrc"
// --> OD 2009-07-14 #i73249#
#include <svx/dialogs.hrc>
// <--

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

// Prototypen ------------------------------------------------------------

void lcl_FrmGetMaxLineWidth(const SvxBorderLine* pBorderLine, SvxBorderLine& rBorderLine);
const SwFrmFmt* lcl_GetFrmFmtByName(SwWrtShell& rSh, const String& rName)
{
    sal_uInt16 nCount = rSh.GetFlyCount(FLYCNTTYPE_FRM);
    for( sal_uInt16 i = 0; i < nCount; i++)
    {
        const SwFrmFmt* pFmt = rSh.GetFlyNum(i, FLYCNTTYPE_FRM);
		if(pFmt->GetName() == rName)
			return pFmt;
    }
    return 0;
}

#define SwFrameShell
#include <sfx2/msg.hxx>
#include "swslots.hxx"

SFX_IMPL_INTERFACE(SwFrameShell, SwBaseShell, SW_RES(STR_SHELLNAME_FRAME))
{
	SFX_POPUPMENU_REGISTRATION(SW_RES(MN_FRM_POPUPMENU));
	SFX_OBJECTBAR_REGISTRATION(SFX_OBJECTBAR_OBJECT, SW_RES(RID_FRAME_TOOLBOX));
}



void SwFrameShell::Execute(SfxRequest &rReq)
{
	//Erstmal die, die keinen FrmMgr benoetigen.
	SwWrtShell &rSh = GetShell();
	BOOL bMore = FALSE;
	const SfxItemSet* pArgs = rReq.GetArgs();
	const SfxPoolItem* pItem;
	USHORT nSlot = rReq.GetSlot();

	switch ( nSlot )
	{
		case FN_FRAME_TO_ANCHOR:
			if ( rSh.IsFrmSelected() )
			{
				rSh.GotoFlyAnchor();
				rSh.EnterStdMode();
				rSh.CallChgLnk();
			}
			break;
		case SID_FRAME_TO_TOP:
			rSh.SelectionToTop();
			break;

		case SID_FRAME_TO_BOTTOM:
			rSh.SelectionToBottom();
			break;

		case FN_FRAME_UP:
			rSh.SelectionToTop( FALSE );
			break;

		case FN_FRAME_DOWN:
			rSh.SelectionToBottom( FALSE );
			break;
		case FN_INSERT_FRAME:
			if (!pArgs)
			{
				// Rahmen existiert bereits, Rahmendialog zur Bearbeitung oeffnen
				SfxUInt16Item aDefPage(FN_FORMAT_FRAME_DLG, TP_COLUMN);
				rSh.GetView().GetViewFrame()->GetDispatcher()->Execute( FN_FORMAT_FRAME_DLG,
								SFX_CALLMODE_SYNCHRON|SFX_CALLMODE_RECORD,
								&aDefPage, 0L );

			}
			else
			{
				// Rahmen existiert bereits, nur Spaltenanzahl wird geaendert
				USHORT nCols = 1;
				if(pArgs->GetItemState(SID_ATTR_COLUMNS, FALSE, &pItem) == SFX_ITEM_SET)
					nCols = ((SfxUInt16Item *)pItem)->GetValue();

				SfxItemSet aSet(GetPool(),RES_COL,RES_COL);
				rSh.GetFlyFrmAttr( aSet );
				SwFmtCol aCol((const SwFmtCol&)aSet.Get(RES_COL));
				// GutterWidth wird nicht immer uebergeben, daher erst besorgen (siehe view2: Execute auf diesen Slot)
				USHORT nGutterWidth = aCol.GetGutterWidth();
				if(!nCols )
					nCols++;
				aCol.Init(nCols, nGutterWidth, aCol.GetWishWidth());
				aSet.Put(aCol);
				// Vorlagen-AutoUpdate
				SwFrmFmt* pFmt = rSh.GetCurFrmFmt();
				if(pFmt && pFmt->IsAutoUpdateFmt())
				{
					rSh.AutoUpdateFrame(pFmt, aSet);
				}
				else
				{
					rSh.StartAllAction();
					rSh.SetFlyFrmAttr( aSet );
					rSh.SetModified();
					rSh.EndAllAction();
				}

			}
			return;

		case SID_HYPERLINK_SETLINK:
		{
			if(pArgs && SFX_ITEM_SET == pArgs->GetItemState(SID_HYPERLINK_SETLINK, FALSE, &pItem))
			{
				const SvxHyperlinkItem& rHLinkItem = *(const SvxHyperlinkItem *)pItem;
				const String& rURL = rHLinkItem.GetURL();
				const String& rTarget = rHLinkItem.GetTargetFrame();

				SfxItemSet aSet( rSh.GetAttrPool(), RES_URL, RES_URL );
				rSh.GetFlyFrmAttr( aSet );
				SwFmtURL aURL( (SwFmtURL&)aSet.Get( RES_URL ) );

				String sOldName(rHLinkItem.GetName());
				String sFlyName(rSh.GetFlyName());
				if (sOldName.ToUpperAscii() != sFlyName.ToUpperAscii())
				{
					String sName(sOldName);
					USHORT i = 1;
					while (rSh.FindFlyByName(sName))
					{
						sName = sOldName;
						sName += '_';
						sName += String::CreateFromInt32(i++);
					}
					rSh.SetFlyName(sName);
				}
				aURL.SetURL( rURL, FALSE );
				aURL.SetTargetFrameName(rTarget);

				aSet.Put( aURL );
				rSh.SetFlyFrmAttr( aSet );
			}
		}
		break;

		case FN_FRAME_CHAIN:
			rSh.GetView().GetEditWin().SetChainMode( !rSh.GetView().GetEditWin().IsChainMode() );
			break;

		case FN_FRAME_UNCHAIN:
			rSh.Unchain( (SwFrmFmt&)*rSh.GetFlyFrmFmt() );
			GetView().GetViewFrame()->GetBindings().Invalidate(FN_FRAME_CHAIN);
			break;
		case FN_FORMAT_FOOTNOTE_DLG:
		{
            SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            DBG_ASSERT(pFact, "SwAbstractDialogFactory fail!");

            VclAbstractDialog* pDlg = pFact->CreateSwFootNoteOptionDlg( GetView().GetWindow(), GetView().GetWrtShell(), DLG_DOC_FOOTNOTE );
            DBG_ASSERT(pDlg, "Dialogdiet fail!");
			pDlg->Execute();
			delete pDlg;
			break;
		}
		case FN_NUMBERING_OUTLINE_DLG:
		{
			SfxItemSet aTmp(GetPool(), FN_PARAM_1, FN_PARAM_1);
			SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
            DBG_ASSERT(pFact, "Dialogdiet fail!");
            SfxAbstractTabDialog* pDlg = pFact->CreateSwTabDialog( DLG_TAB_OUTLINE,
														GetView().GetWindow(), &aTmp, GetView().GetWrtShell());
            DBG_ASSERT(pDlg, "Dialogdiet fail!");
			pDlg->Execute();
			delete pDlg;
			rReq.Done();
			break;
		}
		case SID_OPEN_XML_FILTERSETTINGS:
		{
			try
			{
				uno::Reference < ui::dialogs::XExecutableDialog > xDialog(::comphelper::getProcessServiceFactory()->createInstance(rtl::OUString::createFromAscii("com.sun.star.comp.ui.XSLTFilterDialog")), uno::UNO_QUERY);
				if( xDialog.is() )
				{
					xDialog->execute();
				}
			}
			catch( uno::Exception& )
			{
			}
			rReq.Ignore ();
		}
		break;
		case FN_WORDCOUNT_DIALOG:
		{
			SwDocStat aCurr;
			SwDocStat aDocStat( rSh.getIDocumentStatistics()->GetDocStat() );
			{
				SwWait aWait( *GetView().GetDocShell(), TRUE );
				rSh.StartAction();
				rSh.CountWords( aCurr );
				rSh.UpdateDocStat( aDocStat );
				rSh.EndAction();
			}

			SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
			DBG_ASSERT(pFact, "Dialogdiet fail!");
			AbstractSwWordCountDialog* pDialog = pFact->CreateSwWordCountDialog( GetView().GetWindow() );
			pDialog->SetValues(aCurr, aDocStat );
			pDialog->Execute();
			delete pDialog;
		}
		break;
		default: bMore = TRUE;
	}

	if ( !bMore )
	{
		return;
	}

	SwFlyFrmAttrMgr aMgr( FALSE, &rSh, FRMMGR_TYPE_NONE );
	BOOL bUpdateMgr = TRUE;
	BOOL bCopyToFmt = FALSE;
	switch ( nSlot )
	{
        case SID_OBJECT_ALIGN_MIDDLE:
        case FN_FRAME_ALIGN_VERT_CENTER:
            aMgr.SetVertOrientation( text::VertOrientation::CENTER );
			break;
        case SID_OBJECT_ALIGN_DOWN :
		case FN_FRAME_ALIGN_VERT_BOTTOM:
            aMgr.SetVertOrientation( text::VertOrientation::BOTTOM );
			break;
        case SID_OBJECT_ALIGN_UP :
		case FN_FRAME_ALIGN_VERT_TOP:
            aMgr.SetVertOrientation( text::VertOrientation::TOP );
			break;

		case FN_FRAME_ALIGN_VERT_CHAR_CENTER:
            aMgr.SetVertOrientation( text::VertOrientation::CHAR_CENTER );
			break;

		case FN_FRAME_ALIGN_VERT_CHAR_BOTTOM:
            aMgr.SetVertOrientation( text::VertOrientation::CHAR_BOTTOM );
			break;

		case FN_FRAME_ALIGN_VERT_CHAR_TOP:
            aMgr.SetVertOrientation( text::VertOrientation::CHAR_TOP );
			break;

		case FN_FRAME_ALIGN_VERT_ROW_CENTER:
            aMgr.SetVertOrientation( text::VertOrientation::LINE_CENTER );
			break;

		case FN_FRAME_ALIGN_VERT_ROW_BOTTOM:
            aMgr.SetVertOrientation( text::VertOrientation::LINE_BOTTOM );
			break;

		case FN_FRAME_ALIGN_VERT_ROW_TOP:
            aMgr.SetVertOrientation( text::VertOrientation::LINE_TOP );
			break;
        case SID_OBJECT_ALIGN_CENTER :
		case FN_FRAME_ALIGN_HORZ_CENTER:
            aMgr.SetHorzOrientation( text::HoriOrientation::CENTER );
			break;
        case SID_OBJECT_ALIGN_RIGHT:
		case FN_FRAME_ALIGN_HORZ_RIGHT:
            aMgr.SetHorzOrientation( text::HoriOrientation::RIGHT );
			break;
        case SID_OBJECT_ALIGN_LEFT:
		case FN_FRAME_ALIGN_HORZ_LEFT:
            aMgr.SetHorzOrientation( text::HoriOrientation::LEFT );
			break;

		case FN_SET_FRM_POSITION:
		{
			aMgr.SetAbsPos(((SfxPointItem &)pArgs->Get
								(FN_SET_FRM_POSITION)).GetValue());
		}
		break;
		case SID_ATTR_BRUSH:
		{
			if(pArgs)
			{
				aMgr.SetAttrSet( *pArgs );
				bCopyToFmt = TRUE;
			}
		}
		break;
		case SID_ATTR_ULSPACE:
		case SID_ATTR_LRSPACE:
		{
			if(pArgs && SFX_ITEM_SET == pArgs->GetItemState(GetPool().GetWhich(nSlot), FALSE, &pItem))
			{
				aMgr.SetAttrSet( *pArgs );
				if(SID_ATTR_ULSPACE == nSlot && SID_ATTR_ULSPACE == nSlot)
					bCopyToFmt = TRUE;
			}
		}
		break;
		case FN_FORMAT_FRAME_DLG:
		{
			const int nSel = rSh.GetSelectionType();
			if (nSel & nsSelectionType::SEL_GRF)
			{
				rSh.GetView().GetViewFrame()->GetDispatcher()->Execute(FN_FORMAT_GRAFIC_DLG);
				bUpdateMgr = FALSE;
			}
			else
			{
				SfxItemSet aSet(GetPool(), 	RES_FRMATR_BEGIN, 		RES_FRMATR_END-1,
											SID_ATTR_BORDER_INNER, 	SID_ATTR_BORDER_INNER,
											FN_GET_PRINT_AREA, 		FN_GET_PRINT_AREA,
											SID_ATTR_PAGE_SIZE, 	SID_ATTR_PAGE_SIZE,
											SID_ATTR_BRUSH, 		SID_ATTR_BRUSH,
											SID_ATTR_LRSPACE,		SID_ATTR_ULSPACE,
											FN_SURROUND, 			FN_HORI_ORIENT,
											FN_SET_FRM_NAME, 		FN_SET_FRM_NAME,
											FN_KEEP_ASPECT_RATIO, 	FN_KEEP_ASPECT_RATIO,
											SID_DOCFRAME, 			SID_DOCFRAME,
											SID_HTML_MODE, 			SID_HTML_MODE,
											FN_SET_FRM_ALT_NAME, 	FN_SET_FRM_ALT_NAME,
                                            FN_PARAM_CHAIN_PREVIOUS, FN_PARAM_CHAIN_NEXT,
                                            0);

				const SwViewOption* pVOpt = rSh.GetViewOptions();
				if(nSel & nsSelectionType::SEL_OLE)
				{
					aSet.Put(SfxBoolItem(FN_KEEP_ASPECT_RATIO,
						pVOpt->IsKeepRatio()));
				}
				aSet.Put(SfxUInt16Item(SID_HTML_MODE, ::GetHtmlMode(GetView().GetDocShell())));
				aSet.Put(SfxStringItem(FN_SET_FRM_NAME, rSh.GetFlyName()));
                if( nSel & nsSelectionType::SEL_OLE )
                {
                    // --> OD 2009-07-13 #i73249#
//                    aSet.Put(SfxStringItem(FN_SET_FRM_ALT_NAME, rSh.GetAlternateText()));
                    aSet.Put( SfxStringItem( FN_SET_FRM_ALT_NAME, rSh.GetObjTitle() ) );
                    // <--
                }

				const SwRect &rPg = rSh.GetAnyCurRect(RECT_PAGE);
				SwFmtFrmSize aFrmSize(ATT_VAR_SIZE, rPg.Width(), rPg.Height());
				aFrmSize.SetWhich(GetPool().GetWhich(SID_ATTR_PAGE_SIZE));
				aSet.Put(aFrmSize);

				const SwRect &rPr = rSh.GetAnyCurRect(RECT_PAGE_PRT);
				SwFmtFrmSize aPrtSize(ATT_VAR_SIZE, rPr.Width(), rPr.Height());
				aPrtSize.SetWhich(GetPool().GetWhich(FN_GET_PRINT_AREA));
				aSet.Put(aPrtSize);

				aSet.Put(aMgr.GetAttrSet());
				aSet.SetParent( aMgr.GetAttrSet().GetParent() );

				// Bei %-Werten Groesse initialisieren
				SwFmtFrmSize& rSize = (SwFmtFrmSize&)aSet.Get(RES_FRM_SIZE);
				if (rSize.GetWidthPercent() && rSize.GetWidthPercent() != 0xff)
					rSize.SetWidth(rSh.GetAnyCurRect(RECT_FLY_EMBEDDED).Width());
				if (rSize.GetHeightPercent() && rSize.GetHeightPercent() != 0xff)
					rSize.SetHeight(rSh.GetAnyCurRect(RECT_FLY_EMBEDDED).Height());

				UINT16 nDefPage = 0;
				if(pArgs && pArgs->GetItemState(FN_FORMAT_FRAME_DLG, FALSE, &pItem) == SFX_ITEM_SET)
					nDefPage = ((SfxUInt16Item *)pItem)->GetValue();

				aSet.Put(SfxFrameItem( SID_DOCFRAME, GetView().GetViewFrame()->GetTopFrame()));
                FieldUnit eMetric = ::GetDfltMetric(0 != PTR_CAST(SwWebView, &GetView()));
                SW_MOD()->PutItem(SfxUInt16Item(SID_ATTR_METRIC, static_cast< UINT16 >(eMetric) ));
				SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
                DBG_ASSERT(pFact, "Dialogdiet fail!");
                SfxAbstractTabDialog* pDlg = pFact->CreateFrmTabDialog( DLG_FRM_STD,
														GetView().GetViewFrame(),
														GetView().GetWindow(),
														aSet, FALSE,
														nSel & nsSelectionType::SEL_GRF ? DLG_FRM_GRF :
														nSel & nsSelectionType::SEL_OLE ? DLG_FRM_OLE :
																						DLG_FRM_STD,
														FALSE,
														nDefPage);
                DBG_ASSERT(pDlg, "Dialogdiet fail!");

				if ( pDlg->Execute() )
				{
					const SfxItemSet* pOutSet = pDlg->GetOutputItemSet();
					if(pOutSet)
					{
                        rReq.Done(*pOutSet);
                        if(nSel & nsSelectionType::SEL_OLE &&
						SFX_ITEM_SET == pOutSet->GetItemState(FN_KEEP_ASPECT_RATIO, TRUE, &pItem))
						{
							SwViewOption aUsrPref( *pVOpt );
							aUsrPref.SetKeepRatio(((const SfxBoolItem*)pItem)->GetValue());
							SW_MOD()->ApplyUsrPref(aUsrPref, &GetView());
						}
						if (SFX_ITEM_SET == pOutSet->GetItemState(FN_SET_FRM_ALT_NAME, TRUE, &pItem))
						{
                            // --> OD 2009-07-13 #i73249#
//                            rSh.SetAlternateText(((const SfxStringItem*)pItem)->GetValue());
                            rSh.SetObjTitle(((const SfxStringItem*)pItem)->GetValue());
                            // <--
						}
						// Vorlagen-AutoUpdate
						SwFrmFmt* pFmt = rSh.GetCurFrmFmt();
						if(pFmt && pFmt->IsAutoUpdateFmt())
						{
							rSh.AutoUpdateFrame(pFmt, *pOutSet);
							// alles, dass das Format nicht kann, muss hart
							// gesetzt werden
							if(SFX_ITEM_SET == pOutSet->GetItemState(FN_SET_FRM_NAME, FALSE, &pItem))
								rSh.SetFlyName(((SfxStringItem*)pItem)->GetValue());
							SfxItemSet aShellSet(GetPool(), RES_FRM_SIZE,	RES_FRM_SIZE,
															RES_SURROUND, 	RES_SURROUND,
															RES_ANCHOR,		RES_ANCHOR,
															RES_VERT_ORIENT,RES_HORI_ORIENT,
															0);
							aShellSet.Put(*pOutSet);
							aMgr.SetAttrSet(aShellSet);
							if(SFX_ITEM_SET == pOutSet->GetItemState(FN_SET_FRM_NAME, FALSE, &pItem))
								rSh.SetFlyName(((SfxStringItem*)pItem)->GetValue());
						}
						else
							aMgr.SetAttrSet( *pOutSet );

                        const SwFrmFmt* pCurrFlyFmt = rSh.GetFlyFrmFmt();
                        if(SFX_ITEM_SET ==
                           pOutSet->GetItemState(FN_PARAM_CHAIN_PREVIOUS,
                                                 FALSE, &pItem))
                        {
                            rSh.HideChainMarker();

                            String sPrevName =
                                ((const SfxStringItem*)pItem)->GetValue();
                            const SwFmtChain &rChain = pCurrFlyFmt->GetChain();
                            //needs cast - no non-const method available
                            SwFlyFrmFmt* pFlyFmt =
                                (SwFlyFrmFmt*)rChain.GetPrev();
                            if(pFlyFmt)
                            {
                                if (pFlyFmt->GetName() != sPrevName)
                                {
                                    rSh.Unchain(*pFlyFmt);
                                }
                                else
                                    sPrevName.Erase();
                            }

                            if(sPrevName.Len())
                            {
                                //needs cast - no non-const method available
                                SwFrmFmt* pPrevFmt = (SwFrmFmt*)
                                    lcl_GetFrmFmtByName(rSh, sPrevName);
                                DBG_ASSERT(pPrevFmt, "No frame found!");
                                if(pPrevFmt)
                                {
                                    rSh.Chain(*pPrevFmt, *pCurrFlyFmt);
                                }
                            }
                            rSh.SetChainMarker();
                        }
                        if(SFX_ITEM_SET ==
                           pOutSet->GetItemState(FN_PARAM_CHAIN_NEXT, FALSE,
                                                 &pItem))
                        {
                            rSh.HideChainMarker();
                            String sNextName =
                                ((const SfxStringItem*)pItem)->GetValue();
                            const SwFmtChain &rChain = pCurrFlyFmt->GetChain();
                            //needs cast - no non-const method available
                            SwFlyFrmFmt* pFlyFmt =
                                (SwFlyFrmFmt*)rChain.GetNext();
                            if(pFlyFmt)
                            {
                                if (pFlyFmt->GetName() != sNextName)
                                {
                                    rSh.Unchain(*((SwFlyFrmFmt*) pCurrFlyFmt));
                                }
                                else
                                    sNextName.Erase();
                            }

                            if(sNextName.Len())
                            {
                                //needs cast - no non-const method available
                                SwFrmFmt* pNextFmt = (SwFrmFmt*)
                                    lcl_GetFrmFmtByName(rSh, sNextName);
                                DBG_ASSERT(pNextFmt, "No frame found!");
                                if(pNextFmt)
                                {
                                    rSh.Chain(*(SwFrmFmt*)
                                              pCurrFlyFmt, *pNextFmt);
                                }
                            }
                            rSh.SetChainMarker();
                        }
					}
				}
				else
					bUpdateMgr = FALSE;
				delete pDlg;
			}
		}
		break;
		case FN_FRAME_MIRROR_ON_EVEN_PAGES:
		{
			SwFmtHoriOrient aHori(aMgr.GetHoriOrient());
			BOOL bMirror = !aHori.IsPosToggle();
			aHori.SetPosToggle(bMirror);
			SfxItemSet aSet(GetPool(), RES_HORI_ORIENT, RES_HORI_ORIENT);
			aSet.Put(aHori);
			aMgr.SetAttrSet(aSet);
			bCopyToFmt = TRUE;
			rReq.SetReturnValue(SfxBoolItem(nSlot, bMirror));
		}
		break;
        // --> OD 2009-07-14 #i73249#
        case FN_TITLE_DESCRIPTION_SHAPE:
        {
            bUpdateMgr = FALSE;
            SdrView* pSdrView = rSh.GetDrawViewWithValidMarkList();
            if ( pSdrView &&
                 pSdrView->GetMarkedObjectCount() == 1 )
            {
                String aDescription(rSh.GetObjDescription());
                String aTitle(rSh.GetObjTitle());

                SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
                OSL_ENSURE(pFact, "Dialogdiet fail!");
                AbstractSvxObjectTitleDescDialog* pDlg =
                    pFact->CreateSvxObjectTitleDescDialog( NULL,
                                                           aTitle,
                                                           aDescription,
                                                           RID_SVXDLG_OBJECT_TITLE_DESC );
                OSL_ENSURE(pDlg, "Dialogdiet fail!");

                if ( pDlg->Execute() == RET_OK )
                {
                    pDlg->GetDescription(aDescription);
                    pDlg->GetTitle(aTitle);

                    rSh.SetObjDescription(aDescription);
                    rSh.SetObjTitle(aTitle);
                }

                delete pDlg;
            }
        }
        break;
        // <--
		default:
			ASSERT( !this, "falscher Dispatcher" );
			return;
	}
	// Vorlagen-AutoUpdate
	SwFrmFmt* pFmt = rSh.GetCurFrmFmt();
	if ( bUpdateMgr )
	{
		if(bCopyToFmt && pFmt && pFmt->IsAutoUpdateFmt())
		{
			rSh.AutoUpdateFrame(pFmt, aMgr.GetAttrSet());
		}
		else
			aMgr.UpdateFlyFrm();
	}

}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void SwFrameShell::GetState(SfxItemSet& rSet)
{
	SwWrtShell &rSh = GetShell();
	BOOL bHtmlMode = 0 != ::GetHtmlMode(rSh.GetView().GetDocShell());
	if (rSh.IsFrmSelected())
	{
		SfxItemSet aSet( rSh.GetAttrPool(),
							RES_LR_SPACE, RES_UL_SPACE,
							RES_PROTECT, RES_HORI_ORIENT,
							RES_OPAQUE, RES_OPAQUE,
							RES_PRINT, RES_OPAQUE,
							0 );
		rSh.GetFlyFrmAttr( aSet );

		BOOL bProtect = rSh.IsSelObjProtected(FLYPROTECT_POS);
		BOOL bParentCntProt = rSh.IsSelObjProtected( FLYPROTECT_CONTENT|FLYPROTECT_PARENT ) != 0;

		bProtect |= bParentCntProt;

		const USHORT eFrmType = rSh.GetFrmType(0,TRUE);
		SwFlyFrmAttrMgr aMgr( FALSE, &rSh, FRMMGR_TYPE_NONE );

		SfxWhichIter aIter( rSet );
		USHORT nWhich = aIter.FirstWhich();
		while ( nWhich )
		{
			switch ( nWhich )
			{
				case RES_FRM_SIZE:
				{
					SwFmtFrmSize aSz(aMgr.GetFrmSize());
					rSet.Put(aSz);
				}
				break;
				case RES_VERT_ORIENT:
				case RES_HORI_ORIENT:
				case SID_ATTR_ULSPACE:
				case SID_ATTR_LRSPACE:
				case RES_LR_SPACE:
				case RES_UL_SPACE:
				case RES_PROTECT:
				case RES_OPAQUE:
				case RES_PRINT:
				case RES_SURROUND:
				{
					rSet.Put(aSet.Get(GetPool().GetWhich(nWhich), TRUE ));
				}
				break;
                case SID_OBJECT_ALIGN_LEFT   :
                case SID_OBJECT_ALIGN_CENTER :
                case SID_OBJECT_ALIGN_RIGHT  :
				case FN_FRAME_ALIGN_HORZ_CENTER:
				case FN_FRAME_ALIGN_HORZ_RIGHT:
				case FN_FRAME_ALIGN_HORZ_LEFT:
					if ( (eFrmType & FRMTYPE_FLY_INCNT) ||
							bProtect ||
                            (nWhich == FN_FRAME_ALIGN_HORZ_CENTER  || nWhich == SID_OBJECT_ALIGN_CENTER)&& bHtmlMode )
						rSet.DisableItem( nWhich );
				break;
				case FN_FRAME_ALIGN_VERT_ROW_TOP:
				case FN_FRAME_ALIGN_VERT_ROW_CENTER:
				case FN_FRAME_ALIGN_VERT_ROW_BOTTOM:
				case FN_FRAME_ALIGN_VERT_CHAR_TOP:
				case FN_FRAME_ALIGN_VERT_CHAR_CENTER:
				case FN_FRAME_ALIGN_VERT_CHAR_BOTTOM:
					if ( !(eFrmType & FRMTYPE_FLY_INCNT) || bProtect
							|| bHtmlMode && FN_FRAME_ALIGN_VERT_CHAR_BOTTOM == nWhich )
						rSet.DisableItem( nWhich );
				break;

                case SID_OBJECT_ALIGN_UP     :
                case SID_OBJECT_ALIGN_MIDDLE :
                case SID_OBJECT_ALIGN_DOWN :

                case FN_FRAME_ALIGN_VERT_TOP:
				case FN_FRAME_ALIGN_VERT_CENTER:
				case FN_FRAME_ALIGN_VERT_BOTTOM:
					if ( bProtect || bHtmlMode && eFrmType & FRMTYPE_FLY_ATCNT)
						rSet.DisableItem( nWhich );
					else
					{
						USHORT nId = 0;
						if (eFrmType & FRMTYPE_FLY_INCNT)
						{
							switch (nWhich)
							{
                                case SID_OBJECT_ALIGN_UP     :
                                case FN_FRAME_ALIGN_VERT_TOP:
									nId = STR_TOP_BASE;	break;
                                case SID_OBJECT_ALIGN_MIDDLE :
                                case FN_FRAME_ALIGN_VERT_CENTER:
									nId = STR_CENTER_BASE;	break;
                                case SID_OBJECT_ALIGN_DOWN :
                                case FN_FRAME_ALIGN_VERT_BOTTOM:
									if(!bHtmlMode)
										nId = STR_BOTTOM_BASE;
									else
										rSet.DisableItem( nWhich );
								break;
							}
						}
						else
						{
                            if (nWhich != FN_FRAME_ALIGN_VERT_TOP &&
                                    nWhich != SID_OBJECT_ALIGN_UP )
							{
								if (aMgr.GetAnchor() == FLY_AT_FLY)
								{
									const SwFrmFmt* pFmt = rSh.IsFlyInFly();
									if (pFmt)
									{
										const SwFmtFrmSize& rFrmSz = pFmt->GetFrmSize();
                                        if (rFrmSz.GetHeightSizeType() != ATT_FIX_SIZE)
										{
											rSet.DisableItem( nWhich );
											break;
										}
									}
								}
							}
							switch (nWhich)
							{
                                case SID_OBJECT_ALIGN_UP :
                                case FN_FRAME_ALIGN_VERT_TOP:
									nId = STR_TOP; break;
                                case SID_OBJECT_ALIGN_MIDDLE:
                                case FN_FRAME_ALIGN_VERT_CENTER:
									nId = STR_CENTER_VERT; break;
                                case SID_OBJECT_ALIGN_DOWN:
                                case FN_FRAME_ALIGN_VERT_BOTTOM:
									nId = STR_BOTTOM; break;
							}
						}
						if ( nId )
							rSet.Put( SfxStringItem( nWhich, SW_RES(nId) ));
					}
				break;
				case SID_HYPERLINK_GETLINK:
				{
					String sURL;
					SvxHyperlinkItem aHLinkItem;
					const SfxPoolItem* pItem;

                    SfxItemSet aURLSet(GetPool(), RES_URL, RES_URL);
                    rSh.GetFlyFrmAttr( aURLSet );

                    if(SFX_ITEM_SET == aURLSet.GetItemState(RES_URL, TRUE, &pItem))
					{
						const SwFmtURL* pFmtURL = (const SwFmtURL*)pItem;
						aHLinkItem.SetURL(pFmtURL->GetURL());
						aHLinkItem.SetTargetFrame(pFmtURL->GetTargetFrameName());
						aHLinkItem.SetName(rSh.GetFlyName());
					}

					aHLinkItem.SetInsertMode((SvxLinkInsertMode)(aHLinkItem.GetInsertMode() |
						(bHtmlMode ? HLINK_HTMLMODE : 0)));

					rSet.Put(aHLinkItem);
				}
				break;

				case FN_FRAME_CHAIN:
				{
					const int nSel = rSh.GetSelectionType();
					if (nSel & nsSelectionType::SEL_GRF || nSel & nsSelectionType::SEL_OLE)
						rSet.DisableItem( FN_FRAME_CHAIN );
					else
					{
						const SwFrmFmt *pFmt = rSh.GetFlyFrmFmt();
						if ( bParentCntProt || rSh.GetView().GetEditWin().GetApplyTemplate() ||
						 	!pFmt || pFmt->GetChain().GetNext() )
						{
							rSet.DisableItem( FN_FRAME_CHAIN );
						}
						else
						{
							BOOL bChainMode = rSh.GetView().GetEditWin().IsChainMode();
							rSet.Put( SfxBoolItem( FN_FRAME_CHAIN, bChainMode ) );
						}
					}
				}
				break;
				case FN_FRAME_UNCHAIN:
				{
					const int nSel = rSh.GetSelectionType();
					if (nSel & nsSelectionType::SEL_GRF || nSel & nsSelectionType::SEL_OLE)
						rSet.DisableItem( FN_FRAME_UNCHAIN );
					else
					{
						const SwFrmFmt *pFmt = rSh.GetFlyFrmFmt();
						if ( bParentCntProt || rSh.GetView().GetEditWin().GetApplyTemplate() ||
						 	!pFmt || !pFmt->GetChain().GetNext() )
						{
							rSet.DisableItem( FN_FRAME_UNCHAIN );
						}
					}
				}
				break;
				case SID_FRAME_TO_TOP:
				case SID_FRAME_TO_BOTTOM:
				case FN_FRAME_UP:
				case FN_FRAME_DOWN:
                    if ( bParentCntProt )
                        rSet.DisableItem( nWhich );
                break;
                case FN_FORMAT_FRAME_DLG:
                {
                    const int nSel = rSh.GetSelectionType();
                    if ( bParentCntProt || nSel & nsSelectionType::SEL_GRF)
						rSet.DisableItem( nWhich );
                }
                break;
                // --> OD 2009-07-07 #i73249#
                case FN_TITLE_DESCRIPTION_SHAPE:
                {
                    SwWrtShell &rWrtSh = GetShell();
                    SdrView* pSdrView = rWrtSh.GetDrawViewWithValidMarkList();
                    if ( !pSdrView ||
                         pSdrView->GetMarkedObjectCount() != 1 )
                    {
                        rSet.DisableItem( nWhich );
                    }

                }
                break;
                // <--
				default:
					/* do nothing */;
					break;
			}
			nWhich = aIter.NextWhich();
		}
	}
}

/*--------------------------------------------------------------------
	Beschreibung:	Ctor fuer FrameShell
 --------------------------------------------------------------------*/


SwFrameShell::SwFrameShell(SwView &_rView) :
    SwBaseShell( _rView )
{
	SetName(String::CreateFromAscii("Frame"));
	SetHelpId(SW_FRAMESHELL);

	/* #96392# Use this to announce it is the frame shell who creates the
       selection. */
    SwTransferable::CreateSelection( _rView.GetWrtShell(), (ViewShell *) this );
}

SwFrameShell::~SwFrameShell()
{
	/* #96392# Only clear the selection if it was this frame shell who created
       it. */
	SwTransferable::ClearSelection( GetShell(), (ViewShell *) this );
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/



void SwFrameShell::ExecFrameStyle(SfxRequest& rReq)
{
	SwWrtShell &rSh = GetShell();
	BOOL bDefault = FALSE;
	if (!rSh.IsFrmSelected())
		return;

	// Erst Default-BoxItem aus Pool holen. Wenn ungleich normalem Boxitem,
	// dann ist es bereits geaendert worden (neues ist kein Default).
	const SvxBoxItem* pPoolBoxItem = (const SvxBoxItem*)::GetDfltAttr(RES_BOX);

	const SfxItemSet *pArgs = rReq.GetArgs();
	SfxItemSet aFrameSet(rSh.GetAttrPool(), RES_BOX, RES_BOX);

	rSh.GetFlyFrmAttr( aFrameSet );
	const SvxBoxItem& rBoxItem = (const SvxBoxItem&)aFrameSet.Get(RES_BOX);

	if (pPoolBoxItem == &rBoxItem)
		bDefault = TRUE;

	SvxBoxItem aBoxItem(rBoxItem);

	SvxBorderLine aBorderLine;
	const SfxPoolItem *pItem = 0;

	if(pArgs)    //irgendein Controller kann auch mal nichts liefern #48169#
	{
		switch (rReq.GetSlot())
		{
			case SID_ATTR_BORDER:
			{
				if (pArgs->GetItemState(RES_BOX, TRUE, &pItem) == SFX_ITEM_SET)
				{
					SvxBoxItem aNewBox(*((SvxBoxItem *)pItem));
					const SvxBorderLine* pBorderLine;

					if ((pBorderLine = aBoxItem.GetTop()) != NULL)
						lcl_FrmGetMaxLineWidth(pBorderLine, aBorderLine);
					if ((pBorderLine = aBoxItem.GetBottom()) != NULL)
						lcl_FrmGetMaxLineWidth(pBorderLine, aBorderLine);
					if ((pBorderLine = aBoxItem.GetLeft()) != NULL)
						lcl_FrmGetMaxLineWidth(pBorderLine, aBorderLine);
					if ((pBorderLine = aBoxItem.GetRight()) != NULL)
						lcl_FrmGetMaxLineWidth(pBorderLine, aBorderLine);

					if(aBorderLine.GetOutWidth() == 0)
					{
						aBorderLine.SetInWidth(0);
						aBorderLine.SetOutWidth(DEF_LINE_WIDTH_0);
						aBorderLine.SetDistance(0);
					}
					//Distance nur setzen, wenn der Request vom Controller kommt

					if(!StarBASIC::IsRunning())
					{
						aNewBox.SetDistance( rBoxItem.GetDistance() );
					}

					aBoxItem = aNewBox;
					SvxBorderLine aDestBorderLine;

					if ((pBorderLine = aBoxItem.GetTop()) != NULL)
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_TOP);
					if ((pBorderLine = aBoxItem.GetBottom()) != NULL)
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_BOTTOM);
					if ((pBorderLine = aBoxItem.GetLeft()) != NULL)
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_LEFT);
					if ((pBorderLine = aBoxItem.GetRight()) != NULL)
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_RIGHT);
				}
			}
			break;

			case SID_FRAME_LINESTYLE:
			{
				if (pArgs->GetItemState(SID_FRAME_LINESTYLE, FALSE, &pItem) == SFX_ITEM_SET)
				{
					const SvxLineItem* pLineItem =
							(const SvxLineItem*)pItem;

					if ( pLineItem->GetLine() )
					{
						aBorderLine = *(pLineItem->GetLine());

						if (!aBoxItem.GetTop() && !aBoxItem.GetBottom() &&
							!aBoxItem.GetLeft() && !aBoxItem.GetRight())
						{
							aBoxItem.SetLine(&aBorderLine, BOX_LINE_TOP);
							aBoxItem.SetLine(&aBorderLine, BOX_LINE_BOTTOM);
							aBoxItem.SetLine(&aBorderLine, BOX_LINE_LEFT);
							aBoxItem.SetLine(&aBorderLine, BOX_LINE_RIGHT);
						}
						else
						{
							if( aBoxItem.GetTop() )
							{
								aBorderLine.SetColor( aBoxItem.GetTop()->GetColor() );
								aBoxItem.SetLine(&aBorderLine, BOX_LINE_TOP);
							}
							if( aBoxItem.GetBottom() )
							{
								aBorderLine.SetColor( aBoxItem.GetBottom()->GetColor());
								aBoxItem.SetLine(&aBorderLine, BOX_LINE_BOTTOM);
							}
							if( aBoxItem.GetLeft() )
							{
								aBorderLine.SetColor( aBoxItem.GetLeft()->GetColor());
								aBoxItem.SetLine(&aBorderLine, BOX_LINE_LEFT);
							}
							if( aBoxItem.GetRight() )
							{
								aBorderLine.SetColor(aBoxItem.GetRight()->GetColor());
								aBoxItem.SetLine(&aBorderLine, BOX_LINE_RIGHT);
							}
						}
					}
					else
					{
						aBoxItem.SetLine(0, BOX_LINE_TOP);
						aBoxItem.SetLine(0, BOX_LINE_BOTTOM);
						aBoxItem.SetLine(0, BOX_LINE_LEFT);
						aBoxItem.SetLine(0, BOX_LINE_RIGHT);
					}
				}
			}
			break;

			case SID_FRAME_LINECOLOR:
			{
				if (pArgs->GetItemState(SID_FRAME_LINECOLOR, FALSE, &pItem) == SFX_ITEM_SET)
				{
					const Color& rNewColor = ((const SvxColorItem*)pItem)->GetValue();

					if (!aBoxItem.GetTop() && !aBoxItem.GetBottom() &&
						!aBoxItem.GetLeft() && !aBoxItem.GetRight())
					{
						aBorderLine.SetColor( rNewColor );
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_TOP);
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_BOTTOM);
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_LEFT);
						aBoxItem.SetLine(&aBorderLine, BOX_LINE_RIGHT);
					}
					else
					{
						if ( aBoxItem.GetTop() )
							((SvxBorderLine*)aBoxItem.GetTop())->SetColor( rNewColor );
						if ( aBoxItem.GetBottom() )
							((SvxBorderLine*)aBoxItem.GetBottom())->SetColor( rNewColor );
						if ( aBoxItem.GetLeft() )
							((SvxBorderLine*)aBoxItem.GetLeft())->SetColor( rNewColor );
						if ( aBoxItem.GetRight() )
							((SvxBorderLine*)aBoxItem.GetRight())->SetColor( rNewColor );
					}
				}
			}
			break;
		}
	}
	if (bDefault && (aBoxItem.GetTop() || aBoxItem.GetBottom() ||
		aBoxItem.GetLeft() || aBoxItem.GetRight()))
	{
		aBoxItem.SetDistance(MIN_BORDER_DIST);
	}
	aFrameSet.Put( aBoxItem );
	// Vorlagen-AutoUpdate
	SwFrmFmt* pFmt = rSh.GetCurFrmFmt();
	if(pFmt && pFmt->IsAutoUpdateFmt())
	{
		rSh.AutoUpdateFrame(pFmt, aFrameSet);
	}
	else
		rSh.SetFlyFrmAttr( aFrameSet );

}



void lcl_FrmGetMaxLineWidth(const SvxBorderLine* pBorderLine, SvxBorderLine& rBorderLine)
{
	if(pBorderLine->GetInWidth() > rBorderLine.GetInWidth())
		rBorderLine.SetInWidth(pBorderLine->GetInWidth());

	if(pBorderLine->GetOutWidth() > rBorderLine.GetOutWidth())
		rBorderLine.SetOutWidth(pBorderLine->GetOutWidth());

	if(pBorderLine->GetDistance() > rBorderLine.GetDistance())
		rBorderLine.SetDistance(pBorderLine->GetDistance());

	rBorderLine.SetColor(pBorderLine->GetColor());
}



void SwFrameShell::GetLineStyleState(SfxItemSet &rSet)
{
	SwWrtShell &rSh = GetShell();
	BOOL bParentCntProt = rSh.IsSelObjProtected( FLYPROTECT_CONTENT|FLYPROTECT_PARENT ) != 0;

	if (bParentCntProt)
	{
		if (rSh.IsFrmSelected())
			rSet.DisableItem( SID_FRAME_LINECOLOR );

		rSet.DisableItem( SID_ATTR_BORDER );
		rSet.DisableItem( SID_FRAME_LINESTYLE );
	}
	else
	{
		if (rSh.IsFrmSelected())
		{
			SfxItemSet aFrameSet( rSh.GetAttrPool(), RES_BOX, RES_BOX );

			rSh.GetFlyFrmAttr(aFrameSet);

			const SvxBorderLine* pLine = ((const SvxBoxItem&)aFrameSet.Get(RES_BOX)).GetTop();
			rSet.Put(SvxColorItem(pLine ? pLine->GetColor() : Color(), SID_FRAME_LINECOLOR));
		}
	}
}

void  SwFrameShell::StateInsert(SfxItemSet &rSet)
{
	const int nSel = GetShell().GetSelectionType();

	if ((nSel & nsSelectionType::SEL_GRF) || (nSel & nsSelectionType::SEL_OLE))
		rSet.DisableItem(FN_INSERT_FRAME);
}
