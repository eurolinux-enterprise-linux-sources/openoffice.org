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
#include "precompiled_sd.hxx"
#include <com/sun/star/ui/dialogs/XExecutableDialog.hpp>
#include <comphelper/processfactory.hxx>
#include <svx/svdlayer.hxx>
#ifndef _SVXIDS_HXX
#include <svx/svxids.hrc>
#endif

#include <svx/hyprlink.hxx>
#include <sfx2/viewfrm.hxx>
#include <svx/hlnkitem.hxx>
#include <tools/urlobj.hxx>
#include <svx/eeitem.hxx>
#ifndef _FLDITEM_HXX
#include <svx/flditem.hxx>
#endif
#include <vcl/msgbox.hxx>
#include <sfx2/request.hxx>
#include <sfx2/dispatch.hxx>
#include <svx/svdorect.hxx>
#include <sfx2/docfile.hxx>
#include <basic/sbstar.hxx>
#include <basic/sberrors.hxx>
#include <svx/fmshell.hxx>
#include <svx/svxdlg.hxx>
#include <svx/dialogs.hrc>
#include <svtools/useroptions.hxx>

#include "app.hrc"
#include "strings.hrc"
#include "res_bmp.hrc"
#include "glob.hrc"
#include "Outliner.hxx"
#include "Window.hxx"
#include "app.hxx"
#include "sdattr.hxx"
#include "drawdoc.hxx"
#include "DrawDocShell.hxx"
#include "sdresid.hxx"
#include "sdpage.hxx"
#include "DrawViewShell.hxx"
#include "drawview.hxx"
#include "unmodpg.hxx"
#include "undolayer.hxx"
#include "ViewShellBase.hxx"
#include "FormShellManager.hxx"
#include "LayerTabBar.hxx"
#include "sdabstdlg.hxx"
#include "SlideSorterViewShell.hxx"
#include "SlideSorter.hxx"
#include "controller/SlideSorterController.hxx"

#define	RET_DELETE	100

namespace sd {

/*************************************************************************
|*
|* SfxRequests fuer temporaere Funktionen
|*
\************************************************************************/

void DrawViewShell::FuTemp02(SfxRequest& rReq)
{
	USHORT nSId = rReq.GetSlot();
	switch( nSId )
	{
		case SID_INSERTLAYER:
		{
			if ( mpDrawView->IsTextEdit() )
			{
				mpDrawView->SdrEndTextEdit();
			}

			SdrLayerAdmin& rLayerAdmin = GetDoc()->GetLayerAdmin();
			USHORT nLayerCnt = rLayerAdmin.GetLayerCount();
			USHORT nLayer = nLayerCnt - 2 + 1;
			String aLayerName ( SdResId(STR_LAYER) ), aLayerTitle, aLayerDesc;
			aLayerName += String::CreateFromInt32( (sal_Int32)nLayer );
			BOOL bIsVisible = FALSE;
			BOOL bIsLocked = FALSE;
			BOOL bIsPrintable = FALSE;

			const SfxItemSet* pArgs = rReq.GetArgs();

			if (! pArgs)
			{
				SfxItemSet aNewAttr( GetDoc()->GetPool(), ATTR_LAYER_START, ATTR_LAYER_END );

				aNewAttr.Put( SdAttrLayerName( aLayerName ) );
				aNewAttr.Put( SdAttrLayerTitle() );
				aNewAttr.Put( SdAttrLayerDesc() );
				aNewAttr.Put( SdAttrLayerVisible() );
				aNewAttr.Put( SdAttrLayerPrintable() );
				aNewAttr.Put( SdAttrLayerLocked() );
				aNewAttr.Put( SdAttrLayerThisPage() );

				SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
				AbstractSdInsertLayerDlg* pDlg = pFact ? pFact->CreateSdInsertLayerDlg(NULL, aNewAttr, true, String( SdResId( STR_INSERTLAYER ) ) ) : 0;
				if( pDlg )
				{
					pDlg->SetHelpId( SID_INSERTLAYER );

					// Ueberpruefung auf schon vorhandene Namen
					BOOL bLoop = TRUE;
					while( bLoop && pDlg->Execute() == RET_OK )
					{
						pDlg->GetAttr( aNewAttr );
						aLayerName	 = ((SdAttrLayerName &) aNewAttr.Get (ATTR_LAYER_NAME)).GetValue ();

						if( rLayerAdmin.GetLayer( aLayerName, FALSE )
							|| aLayerName.Len()==0 )
						{
							// Name ist schon vorhanden
							WarningBox aWarningBox (
								GetParentWindow(),
								WinBits( WB_OK ),
								String(SdResId( STR_WARN_NAME_DUPLICATE)));
							aWarningBox.Execute();
						}
						else
							bLoop = FALSE;
					}
					if( bLoop ) // wurde abgebrochen
					{
						delete pDlg;

						Cancel();
						rReq.Ignore ();
						break;
					}
					else
					{
						//pDlg->GetAttr( aNewAttr );
						//aLayerName	 = ((SdAttrLayerName &) aNewAttr.Get (ATTR_LAYER_NAME)).GetValue ();
						aLayerTitle  = ((SdAttrLayerTitle &) aNewAttr.Get (ATTR_LAYER_TITLE)).GetValue ();
						aLayerDesc   = ((SdAttrLayerDesc &) aNewAttr.Get (ATTR_LAYER_DESC)).GetValue ();
						bIsVisible	 = ((SdAttrLayerVisible &) aNewAttr.Get (ATTR_LAYER_VISIBLE)).GetValue ();
						bIsLocked	 = ((SdAttrLayerLocked &) aNewAttr.Get (ATTR_LAYER_LOCKED)).GetValue () ;
						bIsPrintable = ((SdAttrLayerPrintable &) aNewAttr.Get (ATTR_LAYER_PRINTABLE)).GetValue () ;

						delete pDlg;
					}
				}
			}
			else if (pArgs->Count () != 4)
				 {
					 StarBASIC::FatalError (SbERR_WRONG_ARGS);
					 Cancel();
					 rReq.Ignore ();
					 break;
				 }
				 else
				 {
					 SFX_REQUEST_ARG (rReq, pLayerName, SfxStringItem, ID_VAL_LAYERNAME, FALSE);
					 SFX_REQUEST_ARG (rReq, pIsVisible, SfxBoolItem, ID_VAL_ISVISIBLE, FALSE);
					 SFX_REQUEST_ARG (rReq, pIsLocked, SfxBoolItem, ID_VAL_ISLOCKED, FALSE);
					 SFX_REQUEST_ARG (rReq, pIsPrintable, SfxBoolItem, ID_VAL_ISPRINTABLE, FALSE);

					 aLayerName   = pLayerName->GetValue ();
					 bIsVisible   = pIsVisible->GetValue ();
					 bIsLocked	  = pIsLocked->GetValue ();
					 bIsPrintable = pIsPrintable->GetValue ();
				 }

			String aPrevLayer = mpDrawView->GetActiveLayer();
			String aName;
			SdrLayer* pLayer;
			USHORT nPrevLayer = 0;
			nLayerCnt = rLayerAdmin.GetLayerCount();

			for ( nLayer = 0; nLayer < nLayerCnt; nLayer++ )
			{
				pLayer = rLayerAdmin.GetLayer(nLayer);
				aName = pLayer->GetName();

				if ( aPrevLayer == aName )
				{
//                    nPrevLayer = nLayer;
					nPrevLayer = Max(nLayer, (USHORT) 4);
				}
			}

			mpDrawView->InsertNewLayer(aLayerName, nPrevLayer + 1);
			pLayer = rLayerAdmin.GetLayer(aLayerName, FALSE);
			if( pLayer )
			{
				pLayer->SetTitle( aLayerTitle );
				pLayer->SetDescription( aLayerDesc );
			}

			mpDrawView->SetLayerVisible( aLayerName, bIsVisible );
			mpDrawView->SetLayerLocked( aLayerName, bIsLocked);
			mpDrawView->SetLayerPrintable(aLayerName, bIsPrintable);

			mpDrawView->SetActiveLayer(aLayerName);

			ResetActualLayer();

			GetDoc()->SetChanged(TRUE);

			GetViewFrame()->GetDispatcher()->Execute(SID_SWITCHLAYER,
					SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD);

			Cancel();
			rReq.Done ();
		}
		break;

		case SID_MODIFYLAYER:
		{
			if ( mpDrawView->IsTextEdit() )
			{
				mpDrawView->SdrEndTextEdit();
			}

			SdrLayerAdmin& rLayerAdmin = GetDoc()->GetLayerAdmin();
			USHORT nCurPage = GetLayerTabControl()->GetCurPageId();
			String aLayerName( GetLayerTabControl()->GetPageText(nCurPage) );
			SdrLayer* pLayer = rLayerAdmin.GetLayer(aLayerName, FALSE);

			String aLayerTitle( pLayer->GetTitle() );
			String aLayerDesc( pLayer->GetDescription() );
			
			String aOldLayerName( aLayerName );
			String aOldLayerTitle( aLayerTitle );
			String aOldLayerDesc( aLayerDesc );

			BOOL bIsVisible, bIsLocked, bIsPrintable;
			BOOL bOldIsVisible = bIsVisible = mpDrawView->IsLayerVisible(aLayerName);
			BOOL bOldIsLocked = bIsLocked = mpDrawView->IsLayerLocked(aLayerName);
			BOOL bOldIsPrintable = bIsPrintable = mpDrawView->IsLayerPrintable(aLayerName);


			const SfxItemSet* pArgs = rReq.GetArgs();
			// darf der Layer geloescht werden ?
			bool bDelete = true;

			String aLayoutLayer ( SdResId(STR_LAYER_LAYOUT) );
			String aControlsLayer ( SdResId(STR_LAYER_CONTROLS) );
			String aMeasureLinesLayer ( SdResId(STR_LAYER_MEASURELINES) );
			String aBackgroundLayer( SdResId(STR_LAYER_BCKGRND) );
			String aBackgroundObjLayer( SdResId(STR_LAYER_BCKGRNDOBJ) );

			if( aLayerName == aLayoutLayer       || aLayerName == aControlsLayer ||
				aLayerName == aMeasureLinesLayer ||
				aLayerName == aBackgroundLayer   || aLayerName == aBackgroundObjLayer )
			{
				bDelete = false;
			}

			if (! pArgs)
			{
				SfxItemSet aNewAttr( GetDoc()->GetPool(), ATTR_LAYER_START, ATTR_LAYER_END );

				aNewAttr.Put( SdAttrLayerName( aLayerName ) );
				aNewAttr.Put( SdAttrLayerTitle( aLayerTitle ) );
				aNewAttr.Put( SdAttrLayerDesc( aLayerDesc ) );
				aNewAttr.Put( SdAttrLayerVisible( bIsVisible ) );
				aNewAttr.Put( SdAttrLayerLocked( bIsLocked ) );
				aNewAttr.Put( SdAttrLayerPrintable( bIsPrintable ) );
				aNewAttr.Put( SdAttrLayerThisPage() );

				SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
				AbstractSdInsertLayerDlg* pDlg = pFact ? pFact->CreateSdInsertLayerDlg(NULL, aNewAttr, bDelete, String( SdResId( STR_MODIFYLAYER ) ) ) : 0;
				if( pDlg )
				{
					pDlg->SetHelpId( SID_MODIFYLAYER );

					// Ueberpruefung auf schon vorhandene Namen
					BOOL	bLoop = TRUE;
					USHORT	nRet = 0;
					while( bLoop && ( (nRet = pDlg->Execute()) == RET_OK ) )
					{
						pDlg->GetAttr( aNewAttr );
						aLayerName	 = ((SdAttrLayerName &) aNewAttr.Get (ATTR_LAYER_NAME)).GetValue ();

						if( (rLayerAdmin.GetLayer( aLayerName, FALSE ) &&
							 aLayerName != aOldLayerName) || aLayerName.Len()==0 )
						{
							// Name ist schon vorhanden
							WarningBox aWarningBox (
								GetParentWindow(),
								WinBits( WB_OK ),
								String( SdResId( STR_WARN_NAME_DUPLICATE)));
							aWarningBox.Execute();
						}
						else
							bLoop = FALSE;
					}
					switch (nRet)
					{
						case RET_OK :
							aLayerTitle  = ((SdAttrLayerTitle &) aNewAttr.Get (ATTR_LAYER_TITLE)).GetValue ();
							aLayerDesc   = ((SdAttrLayerDesc &) aNewAttr.Get (ATTR_LAYER_DESC)).GetValue ();
							bIsVisible	 = ((const SdAttrLayerVisible &) aNewAttr.Get (ATTR_LAYER_VISIBLE)).GetValue ();
							bIsLocked	 = ((const SdAttrLayerLocked &) aNewAttr.Get (ATTR_LAYER_LOCKED)).GetValue ();
							bIsPrintable = ((const SdAttrLayerLocked &) aNewAttr.Get (ATTR_LAYER_PRINTABLE)).GetValue ();

							delete pDlg;
							break;

						default :
							delete pDlg;
							rReq.Ignore ();
							Cancel ();
							return;
					}
				}
			}
			else if (pArgs->Count () == 4)
			{
				SFX_REQUEST_ARG (rReq, pLayerName, SfxStringItem, ID_VAL_LAYERNAME, FALSE);
				SFX_REQUEST_ARG (rReq, pIsVisible, SfxBoolItem, ID_VAL_ISVISIBLE, FALSE);
				SFX_REQUEST_ARG (rReq, pIsLocked, SfxBoolItem, ID_VAL_ISLOCKED, FALSE);
				SFX_REQUEST_ARG (rReq, pIsPrintable, SfxBoolItem, ID_VAL_ISPRINTABLE, FALSE);

				aLayerName   = pLayerName->GetValue ();
				bIsVisible   = pIsVisible->GetValue ();
				bIsLocked	 = pIsLocked->GetValue ();
				bIsPrintable = pIsPrintable->GetValue ();
			}
			else
			{
				StarBASIC::FatalError (SbERR_WRONG_ARGS);
				Cancel ();
				rReq.Ignore ();
				break;
			}

			SfxUndoManager* pManager = GetDoc()->GetDocSh()->GetUndoManager();
			SdLayerModifyUndoAction* pAction = new SdLayerModifyUndoAction(
				GetDoc(),
				pLayer,
				// old values
				aOldLayerName,
				aOldLayerTitle,
				aOldLayerDesc,
				bOldIsVisible,
				bOldIsLocked,
				bOldIsPrintable,
				// new values
				aLayerName,
				aLayerTitle,
				aLayerDesc,
				bIsVisible,
				bIsLocked,
				bIsPrintable
				);
			pManager->AddUndoAction( pAction );

			ModifyLayer( pLayer, aLayerName, aLayerTitle, aLayerDesc, bIsVisible, bIsLocked, bIsPrintable );

			Cancel();
			rReq.Done ();
		}
		break;

		case SID_RENAMELAYER:
		{
			if ( mpDrawView->IsTextEdit() )
			{
				mpDrawView->SdrEndTextEdit();
			}

			GetLayerTabControl()->StartEditMode(
                GetLayerTabControl()->GetCurPageId() );

			Cancel();
			rReq.Ignore ();
		}
		break;

		case SID_EDIT_HYPERLINK :
		{
			GetViewFrame()->GetDispatcher()->Execute( SID_HYPERLINK_DIALOG );

			Cancel();
			rReq.Done ();
		}
		break;

        case SID_OPEN_HYPERLINK:
        {
            OutlinerView* pOutView = mpDrawView->GetTextEditOutlinerView();
            if ( pOutView )
            {
                const SvxFieldItem* pFieldItem = pOutView->GetFieldAtSelection();
                if ( pFieldItem )
                {
                    const SvxFieldData* pField = pFieldItem->GetField();
                    if( pField && pField->ISA( SvxURLField ) )
                    {
                        const SvxURLField* pURLField = static_cast< const SvxURLField* >( pField );

						SfxStringItem aUrl( SID_FILE_NAME, pURLField->GetURL() );
						SfxStringItem aTarget( SID_TARGETNAME, pURLField->GetTargetFrame() );

						String aReferName;
						SfxViewFrame* pFrame = GetViewFrame();
						SfxMedium* pMed = pFrame->GetObjectShell()->GetMedium();
						if (pMed)
							aReferName = pMed->GetName();

						SfxFrameItem aFrm( SID_DOCFRAME, pFrame );
						SfxStringItem aReferer( SID_REFERER, aReferName );

						SfxBoolItem aNewView( SID_OPEN_NEW_VIEW, FALSE );
						SfxBoolItem aBrowsing( SID_BROWSE, TRUE );

						SfxViewFrame* pViewFrm = SfxViewFrame::Current();
						if (pViewFrm)
							pViewFrm->GetDispatcher()->Execute( SID_OPENDOC,
														SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD,
														&aUrl, &aTarget,
														&aFrm, &aReferer,
														&aNewView, &aBrowsing,
														0L );
                    }
                }
            }
			Cancel();
			rReq.Done ();
        }
        break;

		case SID_HYPERLINK_SETLINK:
		{
			const SfxItemSet* pReqArgs = rReq.GetArgs();

			if (pReqArgs)
			{
				SvxHyperlinkItem* pHLItem =
                (SvxHyperlinkItem*) &pReqArgs->Get(SID_HYPERLINK_SETLINK);

				if (pHLItem->GetInsertMode() == HLINK_FIELD)
				{
					InsertURLField(pHLItem->GetURL(), pHLItem->GetName(),
								   pHLItem->GetTargetFrame(), NULL);
				}
				else if (pHLItem->GetInsertMode() == HLINK_BUTTON)
				{
					InsertURLButton(pHLItem->GetURL(), pHLItem->GetName(),
									pHLItem->GetTargetFrame(), NULL);
				}
				else if (pHLItem->GetInsertMode() == HLINK_DEFAULT)
				{
					OutlinerView* pOlView = mpDrawView->GetTextEditOutlinerView();

					if (pOlView)
					{
						InsertURLField(pHLItem->GetURL(), pHLItem->GetName(),
									   pHLItem->GetTargetFrame(), NULL);
					}
					else
					{
						InsertURLButton(pHLItem->GetURL(), pHLItem->GetName(),
										pHLItem->GetTargetFrame(), NULL);
					}
				}
			}

			Cancel();
			rReq.Ignore ();
		}
		break;

		case SID_INSERT_FLD_DATE_FIX:
		case SID_INSERT_FLD_DATE_VAR:
		case SID_INSERT_FLD_TIME_FIX:
		case SID_INSERT_FLD_TIME_VAR:
		case SID_INSERT_FLD_AUTHOR:
		case SID_INSERT_FLD_PAGE:
		case SID_INSERT_FLD_PAGES:
		case SID_INSERT_FLD_FILE:
		{
			USHORT nMul = 1;
			SvxFieldItem* pFieldItem = 0;

			switch( nSId )
			{
				case SID_INSERT_FLD_DATE_FIX:
					pFieldItem = new SvxFieldItem(
                        SvxDateField( Date(), SVXDATETYPE_FIX ), EE_FEATURE_FIELD );
				break;

				case SID_INSERT_FLD_DATE_VAR:
                    pFieldItem = new SvxFieldItem( SvxDateField(), EE_FEATURE_FIELD );
				break;

				case SID_INSERT_FLD_TIME_FIX:
					pFieldItem = new SvxFieldItem(
                        SvxExtTimeField( Time(), SVXTIMETYPE_FIX ), EE_FEATURE_FIELD );
				break;

				case SID_INSERT_FLD_TIME_VAR:
                    pFieldItem = new SvxFieldItem( SvxExtTimeField(), EE_FEATURE_FIELD );
				break;

				case SID_INSERT_FLD_AUTHOR:
				{
                    SvtUserOptions aUserOptions;
                    pFieldItem = new SvxFieldItem(
                            SvxAuthorField(
                                aUserOptions.GetFirstName(), aUserOptions.GetLastName(), aUserOptions.GetID() ), EE_FEATURE_FIELD );
				}
				break;

				case SID_INSERT_FLD_PAGE:
				{
                    pFieldItem = new SvxFieldItem( SvxPageField(), EE_FEATURE_FIELD );
					nMul = 3;
				}
				break;

                case SID_INSERT_FLD_PAGES:
				{
                    pFieldItem = new SvxFieldItem( SvxPagesField(), EE_FEATURE_FIELD );
					nMul = 3;
				}
				break;

				case SID_INSERT_FLD_FILE:
				{
					String aName;
					if( GetDocSh()->HasName() )
						aName = GetDocSh()->GetMedium()->GetName();
					//else
					//	aName = GetDocSh()->GetName();
                    pFieldItem = new SvxFieldItem( SvxExtFileField( aName ), EE_FEATURE_FIELD );
				}
				break;
			}

			OutlinerView* pOLV = mpDrawView->GetTextEditOutlinerView();

			if( pOLV )
			{
				const SvxFieldItem* pOldFldItem = pOLV->GetFieldAtSelection();

				if( pOldFldItem && ( pOldFldItem->GetField()->ISA( SvxURLField ) ||
									pOldFldItem->GetField()->ISA( SvxDateField ) ||
									pOldFldItem->GetField()->ISA( SvxTimeField ) ||
									pOldFldItem->GetField()->ISA( SvxExtTimeField ) ||
									pOldFldItem->GetField()->ISA( SvxExtFileField ) ||
									pOldFldItem->GetField()->ISA( SvxAuthorField ) ||
									pOldFldItem->GetField()->ISA( SvxPageField ) ) )
				{
					// Feld selektieren, so dass es beim Insert geloescht wird
					ESelection aSel = pOLV->GetSelection();
					if( aSel.nStartPos == aSel.nEndPos )
						aSel.nEndPos++;
					pOLV->SetSelection( aSel );
				}

				if( pFieldItem )
					pOLV->InsertField( *pFieldItem );
			}
			else
			{
				Outliner* pOutl = GetDoc()->GetInternalOutliner();
				pOutl->Init( OUTLINERMODE_TEXTOBJECT );
				USHORT nOutlMode = pOutl->GetMode();
				pOutl->SetStyleSheet( 0, NULL );
				pOutl->QuickInsertField( *pFieldItem, ESelection() );
				OutlinerParaObject* pOutlParaObject = pOutl->CreateParaObject();

				SdrRectObj* pRectObj = new SdrRectObj( OBJ_TEXT );
				pRectObj->SetMergedItem(SdrTextAutoGrowWidthItem(TRUE));

				pOutl->UpdateFields();
				pOutl->SetUpdateMode( TRUE );
				Size aSize( pOutl->CalcTextSize() );
				aSize.Width() *= nMul;
				pOutl->SetUpdateMode( FALSE );

				Point aPos;
				Rectangle aRect( aPos, GetActiveWindow()->GetOutputSizePixel() );
				aPos = aRect.Center();
				aPos = GetActiveWindow()->PixelToLogic(aPos);
				aPos.X() -= aSize.Width() / 2;
				aPos.Y() -= aSize.Height() / 2;

				Rectangle aLogicRect(aPos, aSize);
				pRectObj->SetLogicRect(aLogicRect);
				pRectObj->SetOutlinerParaObject( pOutlParaObject );
				mpDrawView->InsertObjectAtView(pRectObj, *mpDrawView->GetSdrPageView());
				pOutl->Init( nOutlMode );
			}

			delete pFieldItem;

			Cancel();
			rReq.Ignore ();
		}
		break;

		case SID_MODIFY_FIELD:
		{
			OutlinerView* pOLV = mpDrawView->GetTextEditOutlinerView();

			if( pOLV )
			{
				const SvxFieldItem* pFldItem = pOLV->GetFieldAtSelection();

				if( pFldItem && (pFldItem->GetField()->ISA( SvxDateField ) ||
								 pFldItem->GetField()->ISA( SvxAuthorField ) ||
								 pFldItem->GetField()->ISA( SvxExtFileField ) ||
								 pFldItem->GetField()->ISA( SvxExtTimeField ) ) )
				{
					// Dialog...
					SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
					AbstractSdModifyFieldDlg* pDlg = pFact ? pFact->CreateSdModifyFieldDlg(GetActiveWindow(), pFldItem->GetField(), pOLV->GetAttribs() ) : 0;
					if( pDlg && pDlg->Execute() == RET_OK )
					{
						// #108538#
						// To make a correct SetAttribs() call at the utlinerView
						// it is necessary to split the actions here
						SvxFieldData* pField = pDlg->GetField();
						ESelection aSel = pOLV->GetSelection();
						sal_Bool bSelectionWasModified(sal_False);

						if( pField )
						{
                            SvxFieldItem aFieldItem( *pField, EE_FEATURE_FIELD );

							if( aSel.nStartPos == aSel.nEndPos )
							{
								bSelectionWasModified = sal_True;
								aSel.nEndPos++;
								pOLV->SetSelection( aSel );
							}

							pOLV->InsertField( aFieldItem );

							// #108538# select again for eventual SetAttribs call
							pOLV->SetSelection( aSel );
						}

						SfxItemSet aSet( pDlg->GetItemSet() );

						if( aSet.Count() )
						{
							pOLV->SetAttribs( aSet );

							::Outliner* pOutliner = pOLV->GetOutliner();
							if( pOutliner )
								pOutliner->UpdateFields();
						}

						if(pField)
						{
							// #108538# restore selection to original
							if(bSelectionWasModified)
							{
								aSel.nEndPos--;
								pOLV->SetSelection( aSel );
							}

							delete pField;
						}
					}
					delete pDlg;
				}
			}

			Cancel();
			rReq.Ignore ();
		}
		break;

        case SID_OPEN_XML_FILTERSETTINGS:
        {
			try
			{
				com::sun::star::uno::Reference < ::com::sun::star::ui::dialogs::XExecutableDialog > xDialog(::comphelper::getProcessServiceFactory()->createInstance(rtl::OUString::createFromAscii("com.sun.star.comp.ui.XSLTFilterDialog")), com::sun::star::uno::UNO_QUERY);
				if( xDialog.is() )
				{
					xDialog->execute();
				}
			}
			catch( ::com::sun::star::uno::RuntimeException& )
			{
			}

			Cancel();
			rReq.Ignore ();
		}
		break;

		default:
		{
			// switch Anweisung wegen CLOOKS aufgeteilt. Alle case-Anweisungen die
			// eine Fu???? -Funktion aufrufen, sind in die Methode FuTemp03 (drviewsb)
			// gewandert.
			FuTemp03(rReq);
		}
		break;
	};
};

bool DrawViewShell::RenameSlide( USHORT nPageId, const String & rName  )
{
    BOOL   bOutDummy;
    if( GetDoc()->GetPageByName( rName, bOutDummy ) != SDRPAGE_NOTFOUND )
        return false;

    SdPage* pPageToRename = NULL;
    PageKind ePageKind = GetPageKind();

    if( GetEditMode() == EM_PAGE )
    {
        pPageToRename = GetDoc()->GetSdPage( nPageId - 1, ePageKind );

        // Undo
        SdPage* pUndoPage = pPageToRename;
        SdrLayerAdmin &  rLayerAdmin = GetDoc()->GetLayerAdmin();
        BYTE nBackground = rLayerAdmin.GetLayerID( String( SdResId( STR_LAYER_BCKGRND )), FALSE );
        BYTE nBgObj = rLayerAdmin.GetLayerID( String( SdResId( STR_LAYER_BCKGRNDOBJ )), FALSE );
        SetOfByte aVisibleLayers = mpActualPage->TRG_GetMasterPageVisibleLayers();

        // (#67720#)
        SfxUndoManager* pManager = GetDoc()->GetDocSh()->GetUndoManager();
        ModifyPageUndoAction* pAction = new ModifyPageUndoAction(
            pManager, GetDoc(), pUndoPage, rName, pUndoPage->GetAutoLayout(),
            aVisibleLayers.IsSet( nBackground ),
            aVisibleLayers.IsSet( nBgObj ));
        pManager->AddUndoAction( pAction );

        // rename
        pPageToRename->SetName( rName );

        if( ePageKind == PK_STANDARD )
        {
            // also rename notes-page
            SdPage* pNotesPage = GetDoc()->GetSdPage( nPageId - 1, PK_NOTES );
            pNotesPage->SetName( rName );
        }
    }
    else
    {
        // rename MasterPage -> rename LayoutTemplate
        pPageToRename = GetDoc()->GetMasterSdPage( nPageId - 1, ePageKind );
        GetDoc()->RenameLayoutTemplate( pPageToRename->GetLayoutName(), rName );
    }

    bool bSuccess = ( FALSE != rName.Equals( pPageToRename->GetName()));

    if( bSuccess )
    {
        // user edited page names may be changed by the page so update control
        maTabControl.SetPageText( nPageId, rName );

        // set document to modified state
        GetDoc()->SetChanged( TRUE );

        // inform navigator about change
		SfxBoolItem aItem( SID_NAVIGATOR_INIT, TRUE );
		GetViewFrame()->GetDispatcher()->Execute(
			SID_NAVIGATOR_INIT, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD, &aItem, 0L );

        // Tell the slide sorter about the name change (necessary for
        // accessibility.)
        slidesorter::SlideSorterViewShell* pSlideSorterViewShell
            = slidesorter::SlideSorterViewShell::GetSlideSorter(GetViewShellBase());
        if (pSlideSorterViewShell != NULL)
        {
            pSlideSorterViewShell->GetSlideSorter().GetController().PageNameHasChanged(
                nPageId-1, rName);
        }
    }

    return bSuccess;
}




IMPL_LINK( DrawViewShell, RenameSlideHdl, AbstractSvxNameDialog*, pDialog )
{
    if( ! pDialog )
        return 0;

    String aNewName;
    pDialog->GetName( aNewName );

    SdPage* pCurrentPage = GetDoc()->GetSdPage( maTabControl.GetCurPageId() - 1, GetPageKind() );

    return pCurrentPage && ( aNewName.Equals( pCurrentPage->GetName() ) || GetDocSh()->IsNewPageNameValid( aNewName ) );
}




void DrawViewShell::ModifyLayer (
    SdrLayer* pLayer,
    const String& rLayerName,
    const String& rLayerTitle,
    const String& rLayerDesc,
    bool bIsVisible,
    bool bIsLocked,
    bool bIsPrintable)
{
	if( pLayer )
	{
		const USHORT nPageCount = GetLayerTabControl()->GetPageCount();
		USHORT nCurPage = 0;
		USHORT nPos;
		for( nPos = 0; nPos < nPageCount; nPos++ )
		{
			USHORT nId = GetLayerTabControl()->GetPageId( nPos );
			if( pLayer->GetName() == GetLayerTabControl()->GetPageText( nId ) )
			{
				nCurPage = nId;
				break;
			}
		}

		pLayer->SetName( rLayerName );
		pLayer->SetTitle( rLayerTitle );
		pLayer->SetDescription( rLayerDesc );
		mpDrawView->SetLayerVisible( rLayerName, bIsVisible );
		mpDrawView->SetLayerLocked( rLayerName, bIsLocked);
		mpDrawView->SetLayerPrintable(rLayerName, bIsPrintable);

		GetDoc()->SetChanged(TRUE);

		GetLayerTabControl()->SetPageText(nCurPage, rLayerName);

		TabBarPageBits nBits = 0;

		if (!bIsVisible)
		{
			// Unsichtbare Layer werden anders dargestellt
			nBits = TPB_SPECIAL;
		}

		GetLayerTabControl()->SetPageBits(nCurPage, nBits);

		GetViewFrame()->GetDispatcher()->Execute(
            SID_SWITCHLAYER,
            SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD);

        // Call Invalidate at the form shell.
        FmFormShell* pFormShell = GetViewShellBase().GetFormShellManager()->GetFormShell();
        if (pFormShell != NULL)
            pFormShell->Invalidate();
	}
}

} // end of namespace sd
