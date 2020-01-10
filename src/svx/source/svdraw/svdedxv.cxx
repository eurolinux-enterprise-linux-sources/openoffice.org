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
#include "precompiled_svx.hxx"
#include <svtools/accessibilityoptions.hxx>

#include <svx/svdedxv.hxx>
#include <svtools/solar.hrc>

//#include <tools/string.h>
#include <svtools/itemiter.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/hatch.hxx>
#include <svtools/whiter.hxx>
#include <svtools/style.hxx>
#include <editstat.hxx>
#include <tools/config.hxx>
#include <vcl/cursor.hxx>
#include <svx/unotext.hxx>

#include <svx/editobj.hxx>
#include <svx/outlobj.hxx>
#include <svx/scripttypeitem.hxx>
#include "svditext.hxx"
#include <svx/svdoutl.hxx>
#include <svx/sdtfchim.hxx>
#include <svx/svdotext.hxx>
#include <svx/svdundo.hxx>
#include "svditer.hxx"
#include "svx/svdpagv.hxx"
#include "svx/svdpage.hxx"
#include "svx/svdetc.hxx"   // fuer GetDraftFillColor
#include "svx/svdotable.hxx"
#include <svx/selectioncontroller.hxx>

#ifdef DBG_UTIL
#include <svdibrow.hxx>
#endif

#include <svx/svdoutl.hxx>
#include <svx/svddrgv.hxx>  // fuer SetSolidDragging()
#include "svdstr.hrc"   // Namen aus der Resource
#include "svdglob.hxx"  // StringCache
#include <svx/outliner.hxx>
#include <svx/adjitem.hxx>

// #98988#
#include <svtools/colorcfg.hxx>
#include <vcl/svapp.hxx> //add CHINA001 
#include <sdrpaintwindow.hxx>

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrObjEditView::ImpClearVars()
{
    bQuickTextEditMode=TRUE;
    bMacroMode=TRUE;
    pTextEditOutliner=NULL;
    pTextEditOutlinerView=NULL;
    pTextEditPV=NULL;
    pTextEditWin=NULL;
    pTextEditCursorMerker=NULL;
    pEditPara=NULL;
    bTextEditNewObj=FALSE;
    bMacroDown=FALSE;
    pMacroObj=NULL;
    pMacroPV=NULL;
    pMacroWin=NULL;
    nMacroTol=0;
    bTextEditDontDelete=FALSE;
    bTextEditOnlyOneView=FALSE;
}

SdrObjEditView::SdrObjEditView(SdrModel* pModel1, OutputDevice* pOut):
    SdrGlueEditView(pModel1,pOut)
{
    ImpClearVars();
}

SdrObjEditView::~SdrObjEditView()
{
	pTextEditWin = NULL;            // Damit es in SdrEndTextEdit kein ShowCursor gibt
	if (IsTextEdit()) SdrEndTextEdit();
    if (pTextEditOutliner!=NULL) {
        delete pTextEditOutliner;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SdrObjEditView::IsAction() const
{
    return IsMacroObj() || SdrGlueEditView::IsAction();
}

void SdrObjEditView::MovAction(const Point& rPnt)
{
    if (IsMacroObj()) MovMacroObj(rPnt);
    SdrGlueEditView::MovAction(rPnt);
}

void SdrObjEditView::EndAction()
{
    if (IsMacroObj()) EndMacroObj();
    SdrGlueEditView::EndAction();
}

void SdrObjEditView::BckAction()
{
    BrkMacroObj();
    SdrGlueEditView::BckAction();
}

void SdrObjEditView::BrkAction()
{
    BrkMacroObj();
    SdrGlueEditView::BrkAction();
}

void SdrObjEditView::TakeActionRect(Rectangle& rRect) const
{
    if (IsMacroObj()) {
        rRect=pMacroObj->GetCurrentBoundRect();
    } else {
        SdrGlueEditView::TakeActionRect(rRect);
    }
}

void __EXPORT SdrObjEditView::Notify(SfxBroadcaster& rBC, const SfxHint& rHint)
{
    SdrGlueEditView::Notify(rBC,rHint);
    // Printerwechsel waerend des Editierens
    SdrHint* pSdrHint=PTR_CAST(SdrHint,&rHint);
    if (pSdrHint!=NULL && pTextEditOutliner!=NULL) {
        SdrHintKind eKind=pSdrHint->GetKind();
        if (eKind==HINT_REFDEVICECHG) {
            pTextEditOutliner->SetRefDevice(pMod->GetRefDevice());
        }
        if (eKind==HINT_DEFAULTTABCHG) {
            pTextEditOutliner->SetDefTab(pMod->GetDefaultTabulator());
        }
        if (eKind==HINT_DEFFONTHGTCHG) {
            // ...
        }
        if (eKind==HINT_MODELSAVED) { // #43095#
            pTextEditOutliner->ClearModifyFlag();
        }
    }
}

void SdrObjEditView::ModelHasChanged()
{
    SdrGlueEditView::ModelHasChanged();
    if (mxTextEditObj.is() && !mxTextEditObj->IsInserted()) SdrEndTextEdit(); // Objekt geloescht
    // TextEditObj geaendert?
    if (IsTextEdit()) {
        SdrTextObj* pTextObj=dynamic_cast<SdrTextObj*>( mxTextEditObj.get() );
        if (pTextObj!=NULL) {
            ULONG nOutlViewAnz=pTextEditOutliner->GetViewCount();
            BOOL bAreaChg=FALSE;
            BOOL bAnchorChg=FALSE;
            BOOL bColorChg=FALSE;
            bool bContourFrame=pTextObj->IsContourTextFrame();
            EVAnchorMode eNewAnchor(ANCHOR_VCENTER_HCENTER);
            Rectangle aOldArea(aMinTextEditArea);
            aOldArea.Union(aTextEditArea);
            Color aNewColor;
            { // Area Checken
                Size aPaperMin1;
                Size aPaperMax1;
                Rectangle aEditArea1;
                Rectangle aMinArea1;
                pTextObj->TakeTextEditArea(&aPaperMin1,&aPaperMax1,&aEditArea1,&aMinArea1);

				// #108784#
				Point aPvOfs(pTextObj->GetTextEditOffset());

                aEditArea1.Move(aPvOfs.X(),aPvOfs.Y());
                aMinArea1.Move(aPvOfs.X(),aPvOfs.Y());
                Rectangle aNewArea(aMinArea1);
                aNewArea.Union(aEditArea1);
                if (aNewArea!=aOldArea || aEditArea1!=aTextEditArea || aMinArea1!=aMinTextEditArea ||
                    pTextEditOutliner->GetMinAutoPaperSize()!=aPaperMin1 || pTextEditOutliner->GetMaxAutoPaperSize()!=aPaperMax1) {
                    aTextEditArea=aEditArea1;
                    aMinTextEditArea=aMinArea1;
                    pTextEditOutliner->SetUpdateMode(FALSE);
                    pTextEditOutliner->SetMinAutoPaperSize(aPaperMin1);
                    pTextEditOutliner->SetMaxAutoPaperSize(aPaperMax1);
                    pTextEditOutliner->SetPaperSize(Size(0,0)); // Damit der Outliner neu formatiert
                    if (!bContourFrame) {
                        pTextEditOutliner->ClearPolygon();
                        ULONG nStat=pTextEditOutliner->GetControlWord();
                        nStat|=EE_CNTRL_AUTOPAGESIZE;
                        pTextEditOutliner->SetControlWord(nStat);
                    } else {
                        ULONG nStat=pTextEditOutliner->GetControlWord();
                        nStat&=~EE_CNTRL_AUTOPAGESIZE;
                        pTextEditOutliner->SetControlWord(nStat);
                        Rectangle aAnchorRect;
                        pTextObj->TakeTextAnchorRect(aAnchorRect);
                        pTextObj->ImpSetContourPolygon(*pTextEditOutliner,aAnchorRect, TRUE);
                    }
                    for (ULONG nOV=0; nOV<nOutlViewAnz; nOV++) {
                        OutlinerView* pOLV=pTextEditOutliner->GetView(nOV);
                        ULONG nStat0=pOLV->GetControlWord();
                        ULONG nStat=nStat0;
                        // AutoViewSize nur wenn nicht KontourFrame.
                        if (!bContourFrame) nStat|=EV_CNTRL_AUTOSIZE;
                        else nStat&=~EV_CNTRL_AUTOSIZE;
                        if (nStat!=nStat0) pOLV->SetControlWord(nStat);
                    }
                    pTextEditOutliner->SetUpdateMode(TRUE);
                    bAreaChg=TRUE;
                }
            }
            if (pTextEditOutlinerView!=NULL) { // Fuellfarbe und Anker checken
                EVAnchorMode eOldAnchor=pTextEditOutlinerView->GetAnchorMode();
                eNewAnchor=(EVAnchorMode)pTextObj->GetOutlinerViewAnchorMode();
                bAnchorChg=eOldAnchor!=eNewAnchor;
                Color aOldColor(pTextEditOutlinerView->GetBackgroundColor());
                aNewColor = GetTextEditBackgroundColor(*this);
                bColorChg=aOldColor!=aNewColor;
            }
			// #104082# refresh always when it's a contour frame. That
			// refresh is necessary since it triggers the repaint
			// which makes the Handles visible. Changes at TakeTextRect()
			// seem to have resulted in a case where no refresh is executed.
			// Before that, a refresh must have been always executed
			// (else this error would have happend earlier), thus i
			// even think here a refresh should be done always.
			// Since follow-up problems cannot even be guessed I only
			// add this one more case to the if below.
			// BTW: It's VERY bad style that here, inside ModelHasChanged()
			// the outliner is again massively changed for the text object
			// in text edit mode. Normally, all necessary data should be
			// set at SdrBeginTextEdit(). Some changes and value assigns in
			// SdrBeginTextEdit() are completely useless since they are set here
			// again on ModelHasChanged().
            if (bContourFrame || bAreaChg || bAnchorChg || bColorChg)
			{
                for (ULONG nOV=0; nOV<nOutlViewAnz; nOV++)
				{
                    OutlinerView* pOLV=pTextEditOutliner->GetView(nOV);
                    { // Alten OutlinerView-Bereich invalidieren
                        Window* pWin=pOLV->GetWindow();
                        Rectangle aTmpRect(aOldArea);
                        USHORT nPixSiz=pOLV->GetInvalidateMore()+1;
                        Size aMore(pWin->PixelToLogic(Size(nPixSiz,nPixSiz)));
                        aTmpRect.Left()-=aMore.Width();
                        aTmpRect.Right()+=aMore.Width();
                        aTmpRect.Top()-=aMore.Height();
                        aTmpRect.Bottom()+=aMore.Height();
                        InvalidateOneWin(*pWin,aTmpRect);
                    }
                    if (bAnchorChg)
						pOLV->SetAnchorMode(eNewAnchor);
                    if (bColorChg)
						pOLV->SetBackgroundColor( aNewColor );

					pOLV->SetOutputArea(aTextEditArea); // weil sonst scheinbar nicht richtig umgeankert wird
                    ImpInvalidateOutlinerView(*pOLV);
                }
                pTextEditOutlinerView->ShowCursor();
            }
        }
        ImpMakeTextCursorAreaVisible();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@@@@@ @@@@@ @@   @@ @@@@@@  @@@@@ @@@@@  @@ @@@@@@
//    @@   @@    @@@ @@@   @@    @@    @@  @@ @@   @@
//    @@   @@     @@@@@    @@    @@    @@  @@ @@   @@
//    @@   @@@@    @@@     @@    @@@@  @@  @@ @@   @@
//    @@   @@     @@@@@    @@    @@    @@  @@ @@   @@
//    @@   @@    @@@ @@@   @@    @@    @@  @@ @@   @@
//    @@   @@@@@ @@   @@   @@    @@@@@ @@@@@  @@   @@
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrObjEditView::ImpPaintOutlinerView(OutlinerView& rOutlView, const Rectangle& rRect) const
{
    Window* pWin = rOutlView.GetWindow();

	if(pWin)
	{
		const SdrTextObj* pText = PTR_CAST(SdrTextObj,GetTextEditObject());
		bool bTextFrame(pText && pText->IsTextFrame());
		bool bFitToSize(0 != (pTextEditOutliner->GetControlWord() & EE_CNTRL_STRETCHING));
		bool bModifyMerk(pTextEditOutliner->IsModified()); // #43095#
		Rectangle aBlankRect(rOutlView.GetOutputArea());
		aBlankRect.Union(aMinTextEditArea);
		Rectangle aPixRect(pWin->LogicToPixel(aBlankRect));
		aBlankRect.Intersection(rRect);
		rOutlView.GetOutliner()->SetUpdateMode(TRUE); // Bugfix #22596#
		rOutlView.Paint(aBlankRect);

		if(!bModifyMerk) 
		{
			// #43095#
			pTextEditOutliner->ClearModifyFlag(); 
		}

		if(bTextFrame && !bFitToSize) 
		{
			aPixRect.Left()--;
			aPixRect.Top()--;
			aPixRect.Right()++;
			aPixRect.Bottom()++;
			sal_uInt16 nPixSiz(rOutlView.GetInvalidateMore() - 1);
			
			{ 
				// xPixRect Begrenzen, wegen Treiberproblem bei zu weit hinausragenden Pixelkoordinaten
				Size aMaxXY(pWin->GetOutputSizePixel());
				long a(2 * nPixSiz);
				long nMaxX(aMaxXY.Width() + a);
				long nMaxY(aMaxXY.Height() + a);

				if (aPixRect.Left  ()<-a) aPixRect.Left()=-a;
				if (aPixRect.Top   ()<-a) aPixRect.Top ()=-a;
				if (aPixRect.Right ()>nMaxX) aPixRect.Right ()=nMaxX;
				if (aPixRect.Bottom()>nMaxY) aPixRect.Bottom()=nMaxY;
			}

			Rectangle aOuterPix(aPixRect);
			aOuterPix.Left()-=nPixSiz;
			aOuterPix.Top()-=nPixSiz;
			aOuterPix.Right()+=nPixSiz;
			aOuterPix.Bottom()+=nPixSiz;

			bool bMerk(pWin->IsMapModeEnabled());
			pWin->EnableMapMode(FALSE);
			PolyPolygon aPolyPoly( 2 );

			svtools::ColorConfig aColorConfig;
			Color aHatchCol( aColorConfig.GetColorValue( svtools::FONTCOLOR ).nColor );
			const Hatch aHatch( HATCH_SINGLE, aHatchCol, 3, 450 );

			aPolyPoly.Insert( aOuterPix );
			aPolyPoly.Insert( aPixRect );
			pWin->DrawHatch( aPolyPoly, aHatch );

			pWin->EnableMapMode(bMerk);
		}
		
		rOutlView.ShowCursor();
	}
}

void SdrObjEditView::ImpInvalidateOutlinerView(OutlinerView& rOutlView) const
{
    Window* pWin = rOutlView.GetWindow();

	if(pWin)
	{
		const SdrTextObj* pText = PTR_CAST(SdrTextObj,GetTextEditObject());
		bool bTextFrame(pText && pText->IsTextFrame());
		bool bFitToSize(0 != (pTextEditOutliner->GetControlWord() & EE_CNTRL_STRETCHING));

		if(bTextFrame && !bFitToSize) 
		{
			Rectangle aBlankRect(rOutlView.GetOutputArea());
			aBlankRect.Union(aMinTextEditArea);
			Rectangle aPixRect(pWin->LogicToPixel(aBlankRect));
			sal_uInt16 nPixSiz(rOutlView.GetInvalidateMore() - 1);

			aPixRect.Left()--;
			aPixRect.Top()--;
			aPixRect.Right()++;
			aPixRect.Bottom()++;
			
			{ 
				// xPixRect Begrenzen, wegen Treiberproblem bei zu weit hinausragenden Pixelkoordinaten
				Size aMaxXY(pWin->GetOutputSizePixel());
				long a(2 * nPixSiz);
				long nMaxX(aMaxXY.Width() + a);
				long nMaxY(aMaxXY.Height() + a);

				if (aPixRect.Left  ()<-a) aPixRect.Left()=-a;
				if (aPixRect.Top   ()<-a) aPixRect.Top ()=-a;
				if (aPixRect.Right ()>nMaxX) aPixRect.Right ()=nMaxX;
				if (aPixRect.Bottom()>nMaxY) aPixRect.Bottom()=nMaxY;
			}

			Rectangle aOuterPix(aPixRect);
			aOuterPix.Left()-=nPixSiz;
			aOuterPix.Top()-=nPixSiz;
			aOuterPix.Right()+=nPixSiz;
			aOuterPix.Bottom()+=nPixSiz;

			bool bMerk(pWin->IsMapModeEnabled());
			pWin->EnableMapMode(FALSE);
			pWin->Invalidate(aOuterPix);
			pWin->EnableMapMode(bMerk);
		}
	}
}

OutlinerView* SdrObjEditView::ImpMakeOutlinerView(Window* pWin, BOOL /*bNoPaint*/, OutlinerView* pGivenView) const
{
    // Hintergrund
    Color aBackground(GetTextEditBackgroundColor(*this));
    SdrTextObj* pText = dynamic_cast< SdrTextObj * >( mxTextEditObj.get() );
    BOOL bTextFrame=pText!=NULL && pText->IsTextFrame();
    BOOL bContourFrame=pText!=NULL && pText->IsContourTextFrame();
    // OutlinerView erzeugen
    OutlinerView* pOutlView=pGivenView;
    pTextEditOutliner->SetUpdateMode(FALSE);
    if (pOutlView==NULL) pOutlView=new OutlinerView(pTextEditOutliner,pWin);
    else pOutlView->SetWindow(pWin);
    // Scrollen verbieten
    ULONG nStat=pOutlView->GetControlWord();
    nStat&=~EV_CNTRL_AUTOSCROLL;
    // AutoViewSize nur wenn nicht KontourFrame.
    if (!bContourFrame) nStat|=EV_CNTRL_AUTOSIZE;
    if (bTextFrame) {
        USHORT nPixSiz=aHdl.GetHdlSize()*2+1;
        nStat|=EV_CNTRL_INVONEMORE;
        pOutlView->SetInvalidateMore(nPixSiz);
    }
    pOutlView->SetControlWord(nStat);
    pOutlView->SetBackgroundColor( aBackground );
    if (pText!=NULL)
	{
        pOutlView->SetAnchorMode((EVAnchorMode)(pText->GetOutlinerViewAnchorMode()));
		pTextEditOutliner->SetFixedCellHeight(((const SdrTextFixedCellHeightItem&)pText->GetMergedItem(SDRATTR_TEXT_USEFIXEDCELLHEIGHT)).GetValue());
    }
    pOutlView->SetOutputArea(aTextEditArea);
    pTextEditOutliner->SetUpdateMode(TRUE);
    ImpInvalidateOutlinerView(*pOutlView);
    return pOutlView;
}

BOOL SdrObjEditView::IsTextEditFrame() const
{
    SdrTextObj* pText = dynamic_cast< SdrTextObj* >( mxTextEditObj.get() );
    return pText!=NULL && pText->IsTextFrame();
}

IMPL_LINK(SdrObjEditView,ImpOutlinerStatusEventHdl,EditStatus*,pEditStat)
{
    if(pTextEditOutliner )
	{
	    SdrTextObj* pTextObj = dynamic_cast< SdrTextObj * >( mxTextEditObj.get() );
		if( pTextObj )
		{
			pTextObj->onEditOutlinerStatusEvent( pEditStat );
		}
	}
	return 0;
}

IMPL_LINK(SdrObjEditView,ImpOutlinerCalcFieldValueHdl,EditFieldInfo*,pFI)
{
    bool bOk=false;
    String& rStr=pFI->GetRepresentation();
    rStr.Erase();
    SdrTextObj* pTextObj = dynamic_cast< SdrTextObj* >( mxTextEditObj.get() );
    if (pTextObj!=NULL) {
        Color* pTxtCol=NULL;
        Color* pFldCol=NULL;
        bOk=pTextObj->CalcFieldValue(pFI->GetField(),pFI->GetPara(),pFI->GetPos(),TRUE,pTxtCol,pFldCol,rStr);
        if (bOk) {
            if (pTxtCol!=NULL) {
                pFI->SetTxtColor(*pTxtCol);
                delete pTxtCol;
            }
            if (pFldCol!=NULL) {
                pFI->SetFldColor(*pFldCol);
                delete pFldCol;
            } else {
                pFI->SetFldColor(Color(COL_LIGHTGRAY)); // kann spaeter (357) raus
            }
        }
    }
    Outliner& rDrawOutl=pMod->GetDrawOutliner(pTextObj);
    Link aDrawOutlLink=rDrawOutl.GetCalcFieldValueHdl();
    if (!bOk && aDrawOutlLink.IsSet()) {
        aDrawOutlLink.Call(pFI);
        bOk = (BOOL)rStr.Len();
    }
    if (!bOk && aOldCalcFieldValueLink.IsSet()) {
        return aOldCalcFieldValueLink.Call(pFI);
    }
    return 0;
}

sal_Bool SdrObjEditView::SdrBeginTextEdit(
	SdrObject* pObj, SdrPageView* pPV, Window* pWin, 
	sal_Bool bIsNewObj,	SdrOutliner* pGivenOutliner, 
	OutlinerView* pGivenOutlinerView,
	sal_Bool bDontDeleteOutliner, sal_Bool bOnlyOneView, 
	sal_Bool bGrabFocus)
{
    SdrEndTextEdit();

	if( dynamic_cast< SdrTextObj* >( pObj ) == 0 )
		return FALSE; // currently only possible with text objects

    if(bGrabFocus && pWin)
	{
		// attetion, this call may cause an EndTextEdit() call to this view
		pWin->GrabFocus(); // to force the cursor into the edit view
	}

    bTextEditDontDelete=bDontDeleteOutliner && pGivenOutliner!=NULL;
    bTextEditOnlyOneView=bOnlyOneView;
    bTextEditNewObj=bIsNewObj;
    const sal_uInt32 nWinAnz(PaintWindowCount());
    sal_uInt32 i;
    sal_Bool bBrk(sal_False);
    // Abbruch, wenn kein Objekt angegeben.
    
	if(!pObj) 
	{
		bBrk = sal_True;
	}

    if(!bBrk && !pWin) 
	{
        for(i = 0L; i < nWinAnz && !pWin; i++) 
		{
			SdrPaintWindow* pPaintWindow = GetPaintWindow(i);

			if(OUTDEV_WINDOW == pPaintWindow->GetOutputDevice().GetOutDevType())
			{
				pWin = (Window*)(&pPaintWindow->GetOutputDevice());
			}
        }

		// Abbruch, wenn kein Window da.
        if(!pWin) 
		{
			bBrk = sal_True;
		}
    }

	if(!bBrk && !pPV) 
	{
        pPV = GetSdrPageView();

		// Abbruch, wenn keine PageView zu dem Objekt vorhanden.
        if(!pPV) 
		{
			bBrk = sal_True;
		}
    }

	if(pObj && pPV) 
	{
        // Kein TextEdit an Objekten im gesperrten Layer
        if(pPV->GetLockedLayers().IsSet(pObj->GetLayer())) 
		{
            bBrk = sal_True;
        }
    }

    if(pTextEditOutliner) 
	{
        DBG_ERROR("SdrObjEditView::SdrBeginTextEdit() da stand noch ein alter Outliner rum");
        delete pTextEditOutliner;
        pTextEditOutliner = 0L;
    }

    if(!bBrk) 
	{
        pTextEditWin=pWin;
        pTextEditPV=pPV;
        mxTextEditObj.reset( pObj );
        pTextEditOutliner=pGivenOutliner;
        if (pTextEditOutliner==NULL)
			pTextEditOutliner = SdrMakeOutliner( OUTLINERMODE_TEXTOBJECT, mxTextEditObj->GetModel() );

		{
			SvtAccessibilityOptions aOptions;
			pTextEditOutliner->ForceAutoColor( aOptions.GetIsAutomaticFontColor() );
		}

        BOOL bEmpty = mxTextEditObj->GetOutlinerParaObject()==NULL;

        aOldCalcFieldValueLink=pTextEditOutliner->GetCalcFieldValueHdl();
        // Der FieldHdl muss von SdrBeginTextEdit gesetzt sein, da dor ein UpdateFields gerufen wird.
        pTextEditOutliner->SetCalcFieldValueHdl(LINK(this,SdrObjEditView,ImpOutlinerCalcFieldValueHdl));
        pTextEditOutliner->SetBeginPasteOrDropHdl(LINK(this,SdrObjEditView,BeginPasteOrDropHdl));
        pTextEditOutliner->SetEndPasteOrDropHdl(LINK(this,SdrObjEditView, EndPasteOrDropHdl));

		// It is just necessary to make the visualized page known. Set it.
		pTextEditOutliner->setVisualizedPage(pPV ? pPV->GetPage() : 0);

		pTextEditOutliner->SetTextObjNoInit( dynamic_cast< SdrTextObj* >( mxTextEditObj.get() ) );

        if(mxTextEditObj->BegTextEdit(*pTextEditOutliner))
		{
			SdrTextObj* pTextObj = dynamic_cast< SdrTextObj* >( mxTextEditObj.get() );
			DBG_ASSERT( pTextObj, "svx::SdrObjEditView::BegTextEdit(), no text object?" );
			if( !pTextObj )
				return FALSE;

			// #111096# Switch off evtl. running TextAnimation
			pTextObj->SetTextAnimationAllowed(sal_False);

            // alten Cursor merken
            if (pTextEditOutliner->GetViewCount()!=0)
			{
                OutlinerView* pTmpOLV=pTextEditOutliner->RemoveView(ULONG(0));
                if(pTmpOLV!=NULL && pTmpOLV!=pGivenOutlinerView)
					delete pTmpOLV;
            }

            // EditArea ueberTakeTextEditArea bestimmen
			// Das koennte eigentlich entfallen, da TakeTextRect() die Berechnung der aTextEditArea vornimmt
			// Die aMinTextEditArea muss jedoch wohl auch erfolgen (darum bleibt es voerst drinnen)
            pTextObj->TakeTextEditArea(NULL,NULL,&aTextEditArea,&aMinTextEditArea);

			Rectangle aTextRect;
    		Rectangle aAnchorRect;
    		pTextObj->TakeTextRect(*pTextEditOutliner, aTextRect, TRUE,
				&aAnchorRect /* #97097# Give TRUE here, not FALSE */);

    		if ( !pTextObj->IsContourTextFrame() )
			{
				// FitToSize erstmal nicht mit ContourFrame
        		SdrFitToSizeType eFit = pTextObj->GetFitToSize();
        		if (eFit==SDRTEXTFIT_PROPORTIONAL || eFit==SDRTEXTFIT_ALLLINES)
        			aTextRect = aAnchorRect;
			}

			aTextEditArea = aTextRect;

			// #108784#
			Point aPvOfs(pTextObj->GetTextEditOffset());

			aTextEditArea.Move(aPvOfs.X(),aPvOfs.Y());
            aMinTextEditArea.Move(aPvOfs.X(),aPvOfs.Y());
            pTextEditCursorMerker=pWin->GetCursor();

	        aHdl.SetMoveOutside(TRUE);

			// #i72757#
			// Since IsMarkHdlWhenTextEdit() is ignored, it is necessary
			// to call AdjustMarkHdl() always.
			AdjustMarkHdl();

            pTextEditOutlinerView=ImpMakeOutlinerView(pWin,!bEmpty,pGivenOutlinerView);

			// check if this view is already inserted
			ULONG i2,nCount = pTextEditOutliner->GetViewCount();
			for( i2 = 0; i2 < nCount; i2++ )
			{
				if( pTextEditOutliner->GetView(i2) == pTextEditOutlinerView )
					break;
			}

			if( i2 == nCount )
				pTextEditOutliner->InsertView(pTextEditOutlinerView,0);

	        aHdl.SetMoveOutside(FALSE);
	        aHdl.SetMoveOutside(TRUE);
			//OLMRefreshAllIAOManagers();

            // alle Wins als OutlinerView beim Outliner anmelden
            if(!bOnlyOneView) 
			{
                for(i = 0L; i < nWinAnz; i++) 
				{
					SdrPaintWindow* pPaintWindow = GetPaintWindow(i);
					OutputDevice& rOutDev = pPaintWindow->GetOutputDevice();

					if(&rOutDev != pWin && OUTDEV_WINDOW == rOutDev.GetOutDevType())
					{
                        OutlinerView* pOutlView = ImpMakeOutlinerView((Window*)(&rOutDev), !bEmpty, 0L);
                        pTextEditOutliner->InsertView(pOutlView, (sal_uInt16)i);
                    }
                }
            }

			pTextEditOutlinerView->ShowCursor();
            pTextEditOutliner->SetStatusEventHdl(LINK(this,SdrObjEditView,ImpOutlinerStatusEventHdl));
#ifdef DBG_UTIL
            if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
            pTextEditOutliner->ClearModifyFlag();

			// #71519#, #91453#
			if(pWin)
			{
				sal_Bool bExtraInvalidate(sal_False);

				// #71519#
				if(!bExtraInvalidate)
				{
        			SdrFitToSizeType eFit = pTextObj->GetFitToSize();
					if(eFit == SDRTEXTFIT_PROPORTIONAL || eFit == SDRTEXTFIT_ALLLINES)
						bExtraInvalidate = sal_True;
				}

				if(bExtraInvalidate)
				{
					pWin->Invalidate(aTextEditArea);
				}
			}

            // send HINT_BEGEDIT #99840#
            if( GetModel() )
            {
                SdrHint aHint(*pTextObj);
                aHint.SetKind(HINT_BEGEDIT);
                GetModel()->Broadcast(aHint);
            }

			pTextEditOutliner->setVisualizedPage(0);

			if( mxSelectionController.is() )
				mxSelectionController->onSelectionHasChanged();

            return sal_True; // Gut gelaufen, TextEdit laeuft nun
        } 
		else 
		{
            bBrk = sal_True;
            pTextEditOutliner->SetCalcFieldValueHdl(aOldCalcFieldValueLink);
            pTextEditOutliner->SetBeginPasteOrDropHdl(Link());
            pTextEditOutliner->SetEndPasteOrDropHdl(Link());

        }
    }
    if (pTextEditOutliner != NULL)
	{
		pTextEditOutliner->setVisualizedPage(0);
	}

    // wenn hier angekommen, dann ist irgendwas schief gelaufen
    if(!bDontDeleteOutliner)
	{
        if(pGivenOutliner!=NULL)
		{
			delete pGivenOutliner;
			pTextEditOutliner = NULL;
		}
		if(pGivenOutlinerView!=NULL)
		{
			delete pGivenOutlinerView;
			pGivenOutlinerView = NULL;
		}
    }
    if( pTextEditOutliner!=NULL )
	{
		delete pTextEditOutliner;
	}

    pTextEditOutliner=NULL;
    pTextEditOutlinerView=NULL;
    mxTextEditObj.reset(0);
    pTextEditPV=NULL;
    pTextEditWin=NULL;
    //HMHif (bMarkHdlWhenTextEdit) {
    //HMH    HideMarkHdl();
    //HMH}
    aHdl.SetMoveOutside(FALSE);
    //HMHShowMarkHdl();

	return sal_False;
}

SdrEndTextEditKind SdrObjEditView::SdrEndTextEdit(sal_Bool bDontDeleteReally)
{
    SdrEndTextEditKind eRet=SDRENDTEXTEDIT_UNCHANGED;
    SdrTextObj* pTEObj = dynamic_cast< SdrTextObj* >( mxTextEditObj.get() );
    Window*       pTEWin         =pTextEditWin;
    SdrOutliner*  pTEOutliner    =pTextEditOutliner;
    OutlinerView* pTEOutlinerView=pTextEditOutlinerView;
    Cursor*       pTECursorMerker=pTextEditCursorMerker;

    // send HINT_ENDEDIT #99840#
    if( GetModel() && mxTextEditObj.is() )
    {
        SdrHint aHint(*mxTextEditObj.get());
        aHint.SetKind(HINT_ENDEDIT);
        GetModel()->Broadcast(aHint);
    }

    mxTextEditObj.reset(0);
    pTextEditPV=NULL;
    pTextEditWin=NULL;
    pTextEditOutliner=NULL;
    pTextEditOutlinerView=NULL;
    pTextEditCursorMerker=NULL;
    aTextEditArea=Rectangle();

    if (pTEOutliner!=NULL)
	{
        BOOL bModified=pTEOutliner->IsModified();
        if (pTEOutlinerView!=NULL)
		{
            pTEOutlinerView->HideCursor();
        }
        if (pTEObj!=NULL)
		{
            pTEOutliner->CompleteOnlineSpelling();

			SdrUndoObjSetText* pTxtUndo = 0;
			
			if( bModified )
			{
				sal_Int32 nText;
				for( nText = 0; nText < pTEObj->getTextCount(); ++nText )
					if( pTEObj->getText( nText ) == pTEObj->getActiveText() )
						break;

				pTxtUndo = dynamic_cast< SdrUndoObjSetText* >( GetModel()->GetSdrUndoFactory().CreateUndoObjectSetText(*pTEObj, nText ) );
			}
			DBG_ASSERT( !bModified || pTxtUndo, "svx::SdrObjEditView::EndTextEdit(), could not create undo action!" );
            // Den alten CalcFieldValue-Handler wieder setzen
            // Muss vor Obj::EndTextEdit() geschehen, da dort ein UpdateFields() gemacht wird.
            pTEOutliner->SetCalcFieldValueHdl(aOldCalcFieldValueLink);
            pTEOutliner->SetBeginPasteOrDropHdl(Link());
            pTEOutliner->SetEndPasteOrDropHdl(Link());

			const bool bUndo = IsUndoEnabled();
			if( bUndo )
			{
				XubString aObjName;
		        pTEObj->TakeObjNameSingul(aObjName);
			    BegUndo(ImpGetResStr(STR_UndoObjSetText),aObjName);
			}

            pTEObj->EndTextEdit(*pTEOutliner);

			if( (pTEObj->GetRotateAngle() != 0) || (pTEObj && pTEObj->ISA(SdrTextObj) && ((SdrTextObj*)pTEObj)->IsFontwork())  )
			{
				// obviously a repaint
				pTEObj->ActionChanged();
			}

            if (pTxtUndo!=NULL)
			{
                pTxtUndo->AfterSetText();
                if (!pTxtUndo->IsDifferent())
				{
					delete pTxtUndo;
					pTxtUndo=NULL;
				}
            }
            // Loeschung des gesamten TextObj checken
            SdrUndoAction* pDelUndo=NULL;
            BOOL bDelObj=FALSE;
            SdrTextObj* pTextObj=PTR_CAST(SdrTextObj,pTEObj);
            if (pTextObj!=NULL && bTextEditNewObj)
			{
                bDelObj=pTextObj->IsTextFrame() &&
                        !pTextObj->HasText() &&
                        !pTextObj->IsEmptyPresObj() &&
                        !pTextObj->HasFill() &&
                        !pTextObj->HasLine();

                if(pTEObj->IsInserted() && bDelObj && pTextObj->GetObjInventor()==SdrInventor && !bDontDeleteReally)
				{
                    SdrObjKind eIdent=(SdrObjKind)pTextObj->GetObjIdentifier();
                    if(eIdent==OBJ_TEXT || eIdent==OBJ_TEXTEXT)
					{
                        pDelUndo= GetModel()->GetSdrUndoFactory().CreateUndoDeleteObject(*pTEObj);
                    }
                }
            }
            if (pTxtUndo!=NULL)
			{ 
				if( bUndo )
					AddUndo(pTxtUndo);
				eRet=SDRENDTEXTEDIT_CHANGED;
			}
            if (pDelUndo!=NULL)
			{
				if( bUndo )
				{
					AddUndo(pDelUndo);
				}
				else
				{
					delete pDelUndo;
				}
                eRet=SDRENDTEXTEDIT_DELETED;
                DBG_ASSERT(pTEObj->GetObjList()!=NULL,"SdrObjEditView::SdrEndTextEdit(): Fatal: Editiertes Objekt hat keine ObjList!");
                if (pTEObj->GetObjList()!=NULL)
				{
                    pTEObj->GetObjList()->RemoveObject(pTEObj->GetOrdNum());
                    CheckMarked(); // und gleich die Maekierung entfernen...
                }
            }
			else if (bDelObj)
			{ // Fuer den Writer: Loeschen muss die App nachholen.
                eRet=SDRENDTEXTEDIT_SHOULDBEDELETED;
            }

			if( bUndo )
				EndUndo(); // EndUndo hinter Remove, falls der UndoStack gleich weggehaun' wird

			// #111096#
			// Switch on evtl. TextAnimation again after TextEdit
			if(pTEObj->ISA(SdrTextObj))
			{
				((SdrTextObj*)pTEObj)->SetTextAnimationAllowed(sal_True);
			}

			// #i72757#
			// Since IsMarkHdlWhenTextEdit() is ignored, it is necessary
			// to call AdjustMarkHdl() always.
			AdjustMarkHdl();
        }
        // alle OutlinerViews loeschen
        for (ULONG i=pTEOutliner->GetViewCount(); i>0;)
		{
            i--;
            OutlinerView* pOLV=pTEOutliner->GetView(i);
            USHORT nMorePix=pOLV->GetInvalidateMore() + 10; // solaris aw033 test #i#
            Window* pWin=pOLV->GetWindow();
            Rectangle aRect(pOLV->GetOutputArea());
            pTEOutliner->RemoveView(i);
            if (!bTextEditDontDelete || i!=0)
			{
                // die nullte gehoert mir u.U. nicht.
                delete pOLV;
            }
            aRect.Union(aTextEditArea);
            aRect.Union(aMinTextEditArea);
            aRect=pWin->LogicToPixel(aRect);
            aRect.Left()-=nMorePix;
            aRect.Top()-=nMorePix;
            aRect.Right()+=nMorePix;
            aRect.Bottom()+=nMorePix;
            aRect=pWin->PixelToLogic(aRect);
            InvalidateOneWin(*pWin,aRect);
//			pWin->Invalidate(INVALIDATE_UPDATE);

//			pWin->Update();
//			pWin->Flush();
			pWin->SetFillColor();
			pWin->SetLineColor(COL_BLACK);
			pWin->DrawPixel(aRect.TopLeft());
			pWin->DrawPixel(aRect.TopRight());
			pWin->DrawPixel(aRect.BottomLeft());
			pWin->DrawPixel(aRect.BottomRight());
			//pWin->DrawRect(aRect);
        }
        // und auch den Outliner selbst
        if (!bTextEditDontDelete) delete pTEOutliner;
        else pTEOutliner->Clear();
        if (pTEWin!=NULL) {
            pTEWin->SetCursor(pTECursorMerker);
        }
//HMH        if (bMarkHdlWhenTextEdit) {
//HMH            HideMarkHdl();
//HMH        }
        aHdl.SetMoveOutside(FALSE);
        if (eRet!=SDRENDTEXTEDIT_UNCHANGED) 
//HMH		{
//HMH            ShowMarkHdl(); // Handles kommen ansonsten via Broadcast
//HMH        } 
//HMH		else 
		{
			GetMarkedObjectListWriteAccess().SetNameDirty();
		}
#ifdef DBG_UTIL
        if (pItemBrowser) 
		{
			GetMarkedObjectListWriteAccess().SetNameDirty();
			pItemBrowser->SetDirty();
		}
#endif
    }

	// #108784#
	if(	pTEObj &&
		pTEObj->GetModel() &&
		!pTEObj->GetModel()->isLocked() &&
		pTEObj->GetBroadcaster())
	{
		SdrHint aHint(HINT_ENDEDIT);
		aHint.SetObject(pTEObj);
		((SfxBroadcaster*)pTEObj->GetBroadcaster())->Broadcast(aHint);
	}

	return eRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// info about TextEdit. Default is sal_False.
bool SdrObjEditView::IsTextEdit() const 
{ 
	return mxTextEditObj.is(); 
}

// info about TextEditPageView. Default is 0L.
SdrPageView* SdrObjEditView::GetTextEditPageView() const 
{ 
	return pTextEditPV; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////

OutlinerView* SdrObjEditView::ImpFindOutlinerView(Window* pWin) const
{
    if (pWin==NULL) return NULL;
    if (pTextEditOutliner==NULL) return NULL;
    OutlinerView* pNewView=NULL;
    ULONG nWinAnz=pTextEditOutliner->GetViewCount();
    for (ULONG i=0; i<nWinAnz && pNewView==NULL; i++) {
        OutlinerView* pView=pTextEditOutliner->GetView(i);
        if (pView->GetWindow()==pWin) pNewView=pView;
    }
    return pNewView;
}

void SdrObjEditView::SetTextEditWin(Window* pWin)
{
    if(mxTextEditObj.is() && pWin!=NULL && pWin!=pTextEditWin)
	{
        OutlinerView* pNewView=ImpFindOutlinerView(pWin);
        if (pNewView!=NULL && pNewView!=pTextEditOutlinerView)
		{
            if (pTextEditOutlinerView!=NULL)
			{
                pTextEditOutlinerView->HideCursor();
            }
            pTextEditOutlinerView=pNewView;
            pTextEditWin=pWin;
            pWin->GrabFocus(); // Damit der Cursor hier auch blinkt
            pNewView->ShowCursor();
            ImpMakeTextCursorAreaVisible();
        }
    }
}

BOOL SdrObjEditView::IsTextEditHit(const Point& rHit, short nTol) const
{
    BOOL bOk=FALSE;
    if(mxTextEditObj.is())
	{
        nTol=ImpGetHitTolLogic(nTol,NULL);
        // nur drittel Toleranz hier, damit die Handles
        // noch vernuenftig getroffen werden koennen
        nTol=nTol/3;
        nTol=0; // Joe am 6.3.1997: Keine Hittoleranz mehr hier
        if (!bOk)
		{
            Rectangle aEditArea;
            OutlinerView* pOLV=pTextEditOutliner->GetView(0);
            if (pOLV!=NULL)
			{
                aEditArea.Union(pOLV->GetOutputArea());
            }
            aEditArea.Left()-=nTol;
            aEditArea.Top()-=nTol;
            aEditArea.Right()+=nTol;
            aEditArea.Bottom()+=nTol;
            bOk=aEditArea.IsInside(rHit);
            if (bOk)
			{ // Nun noch checken, ob auch wirklich Buchstaben getroffen wurden
                Point aPnt(rHit); aPnt-=aEditArea.TopLeft();
				long nHitTol = 2000;
				OutputDevice* pRef = pTextEditOutliner->GetRefDevice();
				if( pRef )
					nHitTol = pRef->LogicToLogic( nHitTol, MAP_100TH_MM, pRef->GetMapMode().GetMapUnit() );

                bOk = pTextEditOutliner->IsTextPos( aPnt, (sal_uInt16)nHitTol );
            }
        }
    }
    return bOk;
}

BOOL SdrObjEditView::IsTextEditFrameHit(const Point& rHit) const
{
    BOOL bOk=FALSE;
    if(mxTextEditObj.is())
	{
        SdrTextObj* pText= dynamic_cast<SdrTextObj*>(mxTextEditObj.get());
        OutlinerView* pOLV=pTextEditOutliner->GetView(0);
		if( pOLV )
		{
        	Window* pWin=pOLV->GetWindow();
        	if (pText!=NULL && pText->IsTextFrame() && pOLV!=NULL && pWin!=NULL) {
            	USHORT nPixSiz=pOLV->GetInvalidateMore();
            	Rectangle aEditArea(aMinTextEditArea);
            	aEditArea.Union(pOLV->GetOutputArea());
            	if (!aEditArea.IsInside(rHit)) {
                	Size aSiz(pWin->PixelToLogic(Size(nPixSiz,nPixSiz)));
                	aEditArea.Left()-=aSiz.Width();
                	aEditArea.Top()-=aSiz.Height();
                	aEditArea.Right()+=aSiz.Width();
                	aEditArea.Bottom()+=aSiz.Height();
                	bOk=aEditArea.IsInside(rHit);
				}
            }
        }
    }
    return bOk;
}

void SdrObjEditView::AddTextEditOfs(MouseEvent& rMEvt) const
{
    if(mxTextEditObj.is())
	{
        Point aPvOfs;
		SdrTextObj* pTextObj = dynamic_cast< SdrTextObj* >( mxTextEditObj.get() );

		if( pTextObj )
		{
			// #108784#
			aPvOfs += pTextObj->GetTextEditOffset();
		}

		Point aObjOfs(mxTextEditObj->GetLogicRect().TopLeft());
        (Point&)(rMEvt.GetPosPixel())+=aPvOfs+aObjOfs;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SdrObjEditView::KeyInput(const KeyEvent& rKEvt, Window* pWin)
{
    if(pTextEditOutlinerView)
	{
#ifdef DBG_UTIL
        if(rKEvt.GetKeyCode().GetCode() == KEY_RETURN && pTextEditOutliner->GetParagraphCount() == 1)
		{
            ByteString aLine(
				pTextEditOutliner->GetText(pTextEditOutliner->GetParagraph( 0 ), 1),
				gsl_getSystemTextEncoding());
            aLine = aLine.ToUpperAscii();

            if(aLine == "HELLO JOE, PLEASE SHOW THE ITEMBROWSER")
				ShowItemBrowser();
        }
#endif
		if (pTextEditOutlinerView->PostKeyEvent(rKEvt))
		{
			if( pMod && !pMod->IsChanged() )
			{
				if( pTextEditOutliner && pTextEditOutliner->IsModified() )
					pMod->SetChanged( sal_True );
			}

            if (pWin!=NULL && pWin!=pTextEditWin) SetTextEditWin(pWin);
#ifdef DBG_UTIL
            if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
            ImpMakeTextCursorAreaVisible();
            return TRUE;
        }
    }
    return SdrGlueEditView::KeyInput(rKEvt,pWin);
}

BOOL SdrObjEditView::MouseButtonDown(const MouseEvent& rMEvt, Window* pWin)
{
    if (pTextEditOutlinerView!=NULL) {
        BOOL bPostIt=pTextEditOutliner->IsInSelectionMode();
        if (!bPostIt) {
            Point aPt(rMEvt.GetPosPixel());
            if (pWin!=NULL) aPt=pWin->PixelToLogic(aPt);
            else if (pTextEditWin!=NULL) aPt=pTextEditWin->PixelToLogic(aPt);
            bPostIt=IsTextEditHit(aPt,nHitTolLog);
        }
        if (bPostIt) {
            Point aPixPos(rMEvt.GetPosPixel());
            Rectangle aR(pWin->LogicToPixel(pTextEditOutlinerView->GetOutputArea()));
            if (aPixPos.X()<aR.Left  ()) aPixPos.X()=aR.Left  ();
            if (aPixPos.X()>aR.Right ()) aPixPos.X()=aR.Right ();
            if (aPixPos.Y()<aR.Top   ()) aPixPos.Y()=aR.Top   ();
            if (aPixPos.Y()>aR.Bottom()) aPixPos.Y()=aR.Bottom();
            MouseEvent aMEvt(aPixPos,rMEvt.GetClicks(),rMEvt.GetMode(),
                             rMEvt.GetButtons(),rMEvt.GetModifier());
            if (pTextEditOutlinerView->MouseButtonDown(aMEvt)) {
                if (pWin!=NULL && pWin!=pTextEditWin) SetTextEditWin(pWin);
#ifdef DBG_UTIL
                if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
                ImpMakeTextCursorAreaVisible();
                return TRUE;
            }
        }
    }
    return SdrGlueEditView::MouseButtonDown(rMEvt,pWin);
}

BOOL SdrObjEditView::MouseButtonUp(const MouseEvent& rMEvt, Window* pWin)
{
    if (pTextEditOutlinerView!=NULL) {
        BOOL bPostIt=pTextEditOutliner->IsInSelectionMode();
        if (!bPostIt) {
            Point aPt(rMEvt.GetPosPixel());
            if (pWin!=NULL) aPt=pWin->PixelToLogic(aPt);
            else if (pTextEditWin!=NULL) aPt=pTextEditWin->PixelToLogic(aPt);
            bPostIt=IsTextEditHit(aPt,nHitTolLog);
        }
        if (bPostIt) {
            Point aPixPos(rMEvt.GetPosPixel());
            Rectangle aR(pWin->LogicToPixel(pTextEditOutlinerView->GetOutputArea()));
            if (aPixPos.X()<aR.Left  ()) aPixPos.X()=aR.Left  ();
            if (aPixPos.X()>aR.Right ()) aPixPos.X()=aR.Right ();
            if (aPixPos.Y()<aR.Top   ()) aPixPos.Y()=aR.Top   ();
            if (aPixPos.Y()>aR.Bottom()) aPixPos.Y()=aR.Bottom();
            MouseEvent aMEvt(aPixPos,rMEvt.GetClicks(),rMEvt.GetMode(),
                             rMEvt.GetButtons(),rMEvt.GetModifier());
            if (pTextEditOutlinerView->MouseButtonUp(aMEvt)) {
#ifdef DBG_UTIL
                if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
                ImpMakeTextCursorAreaVisible();
                return TRUE;
            }
        }
    }
    return SdrGlueEditView::MouseButtonUp(rMEvt,pWin);
}

BOOL SdrObjEditView::MouseMove(const MouseEvent& rMEvt, Window* pWin)
{
    if (pTextEditOutlinerView!=NULL) {
        BOOL bSelMode=pTextEditOutliner->IsInSelectionMode();
        BOOL bPostIt=bSelMode;
        if (!bPostIt) {
            Point aPt(rMEvt.GetPosPixel());
            if (pWin!=NULL) aPt=pWin->PixelToLogic(aPt);
            else if (pTextEditWin!=NULL) aPt=pTextEditWin->PixelToLogic(aPt);
            bPostIt=IsTextEditHit(aPt,nHitTolLog);
        }
        if (bPostIt) {
            Point aPixPos(rMEvt.GetPosPixel());
            Rectangle aR(pWin->LogicToPixel(pTextEditOutlinerView->GetOutputArea()));
            if (aPixPos.X()<aR.Left  ()) aPixPos.X()=aR.Left  ();
            if (aPixPos.X()>aR.Right ()) aPixPos.X()=aR.Right ();
            if (aPixPos.Y()<aR.Top   ()) aPixPos.Y()=aR.Top   ();
            if (aPixPos.Y()>aR.Bottom()) aPixPos.Y()=aR.Bottom();
            MouseEvent aMEvt(aPixPos,rMEvt.GetClicks(),rMEvt.GetMode(),
                             rMEvt.GetButtons(),rMEvt.GetModifier());
            if (pTextEditOutlinerView->MouseMove(aMEvt) && bSelMode) {
#ifdef DBG_UTIL
                if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
                ImpMakeTextCursorAreaVisible();
                return TRUE;
            }
        }
    }
    return SdrGlueEditView::MouseMove(rMEvt,pWin);
}

BOOL SdrObjEditView::Command(const CommandEvent& rCEvt, Window* pWin)
{
    // solange bis die OutlinerView einen BOOL zurueckliefert
    // bekommt sie nur COMMAND_STARTDRAG
    if (pTextEditOutlinerView!=NULL)
	{
		if (rCEvt.GetCommand()==COMMAND_STARTDRAG) {
	        BOOL bPostIt=pTextEditOutliner->IsInSelectionMode() || !rCEvt.IsMouseEvent();
    	    if (!bPostIt && rCEvt.IsMouseEvent()) {
        	    Point aPt(rCEvt.GetMousePosPixel());
            	if (pWin!=NULL) aPt=pWin->PixelToLogic(aPt);
	            else if (pTextEditWin!=NULL) aPt=pTextEditWin->PixelToLogic(aPt);
    	        bPostIt=IsTextEditHit(aPt,nHitTolLog);
        	}
	        if (bPostIt) {
    	        Point aPixPos(rCEvt.GetMousePosPixel());
        	    if (rCEvt.IsMouseEvent()) {
            	    Rectangle aR(pWin->LogicToPixel(pTextEditOutlinerView->GetOutputArea()));
			        if (aPixPos.X()<aR.Left  ()) aPixPos.X()=aR.Left  ();
            	    if (aPixPos.X()>aR.Right ()) aPixPos.X()=aR.Right ();
	                if (aPixPos.Y()<aR.Top   ()) aPixPos.Y()=aR.Top   ();
    	            if (aPixPos.Y()>aR.Bottom()) aPixPos.Y()=aR.Bottom();
        	    }
	            CommandEvent aCEvt(aPixPos,rCEvt.GetCommand(),rCEvt.IsMouseEvent());
    	        // Command ist an der OutlinerView leider void
        	    pTextEditOutlinerView->Command(aCEvt);
            	if (pWin!=NULL && pWin!=pTextEditWin) SetTextEditWin(pWin);
#ifdef DBG_UTIL
	            if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
    	        ImpMakeTextCursorAreaVisible();
        	    return TRUE;
        	}
		}
		else // if (rCEvt.GetCommand() == COMMAND_VOICE )
		{
			pTextEditOutlinerView->Command(rCEvt);
			return TRUE;
		}
	}
	return SdrGlueEditView::Command(rCEvt,pWin);
}

BOOL SdrObjEditView::Cut(ULONG nFormat)
{
    if (pTextEditOutliner!=NULL) {
        pTextEditOutlinerView->Cut();
#ifdef DBG_UTIL
        if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
        ImpMakeTextCursorAreaVisible();
        return TRUE;
    } else {
        return SdrGlueEditView::Cut(nFormat);
    }
}

BOOL SdrObjEditView::Yank(ULONG nFormat)
{
    if (pTextEditOutliner!=NULL) {
        pTextEditOutlinerView->Copy();
        return TRUE;
    } else {
        return SdrGlueEditView::Yank(nFormat);
    }
}

BOOL SdrObjEditView::Paste(Window* pWin, ULONG nFormat)
{
    if (pTextEditOutliner!=NULL) {
        if (pWin!=NULL) {
            OutlinerView* pNewView=ImpFindOutlinerView(pWin);
            if (pNewView!=NULL) {
                pNewView->Paste();
            }
        } else {
            pTextEditOutlinerView->Paste();
        }
#ifdef DBG_UTIL
        if (pItemBrowser!=NULL) pItemBrowser->SetDirty();
#endif
        ImpMakeTextCursorAreaVisible();
        return TRUE;
    } else {
        return SdrGlueEditView::Paste(pWin,nFormat);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SdrObjEditView::ImpIsTextEditAllSelected() const
{
    BOOL bRet=FALSE;
    if (pTextEditOutliner!=NULL && pTextEditOutlinerView!=NULL)
	{
		if(SdrTextObj::HasTextImpl( pTextEditOutliner ) )
		{
			const sal_uInt32 nParaAnz=pTextEditOutliner->GetParagraphCount();
	        Paragraph* pLastPara=pTextEditOutliner->GetParagraph( nParaAnz > 1 ? nParaAnz - 1 : 0 );

			ESelection aESel(pTextEditOutlinerView->GetSelection());
            if (aESel.nStartPara==0 && aESel.nStartPos==0 && aESel.nEndPara==USHORT(nParaAnz-1))
			{
                XubString aStr(pTextEditOutliner->GetText(pLastPara));

				if(aStr.Len() == aESel.nEndPos)
					bRet = TRUE;
            }
            // und nun auch noch fuer den Fall, das rueckwaerts selektiert wurde
            if (!bRet && aESel.nEndPara==0 && aESel.nEndPos==0 && aESel.nStartPara==USHORT(nParaAnz-1))
			{
                XubString aStr(pTextEditOutliner->GetText(pLastPara));

                if(aStr.Len() == aESel.nStartPos)
					bRet = TRUE;
            }
        }
		else
		{
            bRet=TRUE;
        }
    }
    return bRet;
}

void SdrObjEditView::ImpMakeTextCursorAreaVisible()
{
    if (pTextEditOutlinerView!=NULL && pTextEditWin!=NULL) {
        Cursor* pCsr=pTextEditWin->GetCursor();
        if (pCsr!=NULL) {
            Size aSiz(pCsr->GetSize());
            if (aSiz.Width()!=0 && aSiz.Height()!=0) { // #38450#
                MakeVisible(Rectangle(pCsr->GetPos(),aSiz),*pTextEditWin);
            }
        }
    }
}

USHORT SdrObjEditView::GetScriptType() const
{
	USHORT nScriptType = 0;

    if( IsTextEdit() )
	{
		if( mxTextEditObj->GetOutlinerParaObject() )
			nScriptType = mxTextEditObj->GetOutlinerParaObject()->GetTextObject().GetScriptType();

		if( pTextEditOutlinerView )
			nScriptType = pTextEditOutlinerView->GetSelectedScriptType();
	}
	else
	{
		sal_uInt32 nMarkCount( GetMarkedObjectCount() );

		for( sal_uInt32 i = 0; i < nMarkCount; i++ )
		{
			OutlinerParaObject* pParaObj = GetMarkedObjectByIndex( i )->GetOutlinerParaObject();

			if( pParaObj )
			{
				nScriptType |= pParaObj->GetTextObject().GetScriptType();
			}
		}
	}

	if( nScriptType == 0 )
		nScriptType = SCRIPTTYPE_LATIN;

	return nScriptType;
}

/* new interface src537 */
BOOL SdrObjEditView::GetAttributes(SfxItemSet& rTargetSet, BOOL bOnlyHardAttr) const
{
	if( mxSelectionController.is() )
		if( mxSelectionController->GetAttributes( rTargetSet, bOnlyHardAttr ) )
			return TRUE;

    if(IsTextEdit())
	{
        DBG_ASSERT(pTextEditOutlinerView!=NULL,"SdrObjEditView::GetAttributes(): pTextEditOutlinerView=NULL");
        DBG_ASSERT(pTextEditOutliner!=NULL,"SdrObjEditView::GetAttributes(): pTextEditOutliner=NULL");

		// #92389# take care of bOnlyHardAttr(!)
		if(!bOnlyHardAttr && mxTextEditObj->GetStyleSheet())
			rTargetSet.Put(mxTextEditObj->GetStyleSheet()->GetItemSet());

		// add object attributes
		rTargetSet.Put( mxTextEditObj->GetMergedItemSet() );

		if( mxTextEditObj->GetOutlinerParaObject() )
			rTargetSet.Put( SvxScriptTypeItem( mxTextEditObj->GetOutlinerParaObject()->GetTextObject().GetScriptType() ) );

		if(pTextEditOutlinerView)
		{
			// FALSE= InvalidItems nicht al Default, sondern als "Loecher" betrachten
            rTargetSet.Put(pTextEditOutlinerView->GetAttribs(), FALSE);
			rTargetSet.Put( SvxScriptTypeItem( pTextEditOutlinerView->GetSelectedScriptType() ), FALSE );
        }

		if(GetMarkedObjectCount()==1 && GetMarkedObjectByIndex(0)==mxTextEditObj.get())
		{
			MergeNotPersistAttrFromMarked(rTargetSet, bOnlyHardAttr);
		}

		return TRUE;
	}
	else
	{
		return SdrGlueEditView::GetAttributes(rTargetSet, bOnlyHardAttr);
	}
}

BOOL SdrObjEditView::SetAttributes(const SfxItemSet& rSet, BOOL bReplaceAll)
{
    BOOL bRet=FALSE;
    BOOL bTextEdit=pTextEditOutlinerView!=NULL && mxTextEditObj.is();
    BOOL bAllTextSelected=ImpIsTextEditAllSelected();
    SfxItemSet* pModifiedSet=NULL;
    const SfxItemSet* pSet=&rSet;
    //const SvxAdjustItem* pParaJust=NULL;

    if (!bTextEdit)
	{
        // Kein TextEdit aktiv -> alle Items ans Zeichenobjekt
		if( mxSelectionController.is() )
			bRet=mxSelectionController->SetAttributes(*pSet,bReplaceAll );

		if( !bRet )
		{
		    bRet=SdrGlueEditView::SetAttributes(*pSet,bReplaceAll);
		}
    }
	else
	{
#ifdef DBG_UTIL
        {
            BOOL bHasEEFeatureItems=FALSE;
            SfxItemIter aIter(rSet);
            const SfxPoolItem* pItem=aIter.FirstItem();
            while (!bHasEEFeatureItems && pItem!=NULL)
			{
                if (!IsInvalidItem(pItem))
				{
                    USHORT nW=pItem->Which();
                    if (nW>=EE_FEATURE_START && nW<=EE_FEATURE_END)
						bHasEEFeatureItems=TRUE;
                }

                pItem=aIter.NextItem();
            }

            if(bHasEEFeatureItems)
			{
				String aMessage;
				aMessage.AppendAscii("SdrObjEditView::SetAttributes(): Das setzen von EE_FEATURE-Items an der SdrView macht keinen Sinn! Es fuehrt nur zu Overhead und nicht mehr lesbaren Dokumenten.");
                InfoBox(NULL, aMessage).Execute();
            }
        }
#endif

        BOOL bOnlyEEItems;
        BOOL bNoEEItems=!SearchOutlinerItems(*pSet,bReplaceAll,&bOnlyEEItems);
        // alles selektiert? -> Attrs auch an den Rahmen
        // und falls keine EEItems, dann Attrs nur an den Rahmen
        if (bAllTextSelected || bNoEEItems)
		{
			if( mxSelectionController.is() )
				bRet=mxSelectionController->SetAttributes(*pSet,bReplaceAll );

			if( !bRet )
			{
				const bool bUndo = IsUndoEnabled();

				if( bUndo )
				{
					String aStr;
					ImpTakeDescriptionStr(STR_EditSetAttributes,aStr);
					BegUndo(aStr);
					AddUndo(GetModel()->GetSdrUndoFactory().CreateUndoGeoObject(*mxTextEditObj.get()));

					// #i43537#
					// If this is a text object also rescue the OutlinerParaObject since
					// applying attributes to the object may change text layout when
					// multiple portions exist with multiple formats. If a OutlinerParaObject
					// really exists and needs to be rescued is evaluated in the undo
					// implementation itself.
					bool bRescueText = dynamic_cast< SdrTextObj* >(mxTextEditObj.get());

					AddUndo(GetModel()->GetSdrUndoFactory().CreateUndoAttrObject(*mxTextEditObj.get(),false,!bNoEEItems || bRescueText));
					EndUndo();
				}

				mxTextEditObj->SetMergedItemSetAndBroadcast(*pSet, bReplaceAll);

				FlushComeBackTimer(); // Damit ModeHasChanged sofort kommt
				bRet=TRUE;
			}
        }
		else if (!bOnlyEEItems)
		{ 
			// sonst Set ggf. splitten
            // Es wird nun ein ItemSet aSet gemacht, in den die EE_Items von
            // *pSet nicht enhalten ist (ansonsten ist es eine Kopie).
            USHORT* pNewWhichTable=RemoveWhichRange(pSet->GetRanges(),EE_ITEMS_START,EE_ITEMS_END);
            SfxItemSet aSet(pMod->GetItemPool(),pNewWhichTable);
            /*90353*/ delete[] pNewWhichTable;
            SfxWhichIter aIter(aSet);
            USHORT nWhich=aIter.FirstWhich();
            while (nWhich!=0)
			{
                const SfxPoolItem* pItem;
                SfxItemState eState=pSet->GetItemState(nWhich,FALSE,&pItem);
                if (eState==SFX_ITEM_SET) aSet.Put(*pItem);
                nWhich=aIter.NextWhich();
            }

			
			if( mxSelectionController.is() )
				bRet=mxSelectionController->SetAttributes(aSet,bReplaceAll );

			if( !bRet )
			{
				if( IsUndoEnabled() )
				{
					String aStr;
					ImpTakeDescriptionStr(STR_EditSetAttributes,aStr);
					BegUndo(aStr);
					AddUndo(GetModel()->GetSdrUndoFactory().CreateUndoGeoObject(*mxTextEditObj.get()));
					AddUndo(GetModel()->GetSdrUndoFactory().CreateUndoAttrObject(*mxTextEditObj.get(),false,false));
					EndUndo();
				}

				mxTextEditObj->SetMergedItemSetAndBroadcast(aSet, bReplaceAll);

				if (GetMarkedObjectCount()==1 && GetMarkedObjectByIndex(0)==mxTextEditObj.get())
				{
					SetNotPersistAttrToMarked(aSet,bReplaceAll);
				}
			}
			FlushComeBackTimer();
            bRet=TRUE;
        }
        if(!bNoEEItems)
		{
            // und nun die Attribute auch noch an die EditEngine
            if (bReplaceAll) {
                // Am Outliner kann man leider nur alle Attribute platthauen
                pTextEditOutlinerView->RemoveAttribs( TRUE );
            }
            pTextEditOutlinerView->SetAttribs(rSet);

#ifdef DBG_UTIL
            if (pItemBrowser!=NULL)
				pItemBrowser->SetDirty();
#endif

            ImpMakeTextCursorAreaVisible();
        }
        bRet=TRUE;
    }
    if (pModifiedSet!=NULL)
		delete pModifiedSet;
    return bRet;
}

SfxStyleSheet* SdrObjEditView::GetStyleSheet() const
{
	SfxStyleSheet* pSheet = 0;

	if( mxSelectionController.is() )
	{
		if( mxSelectionController->GetStyleSheet( pSheet ) )
			return pSheet;
	}

    if ( pTextEditOutlinerView )
	{
		pSheet = pTextEditOutlinerView->GetStyleSheet();
    }
	else
	{
		pSheet = SdrGlueEditView::GetStyleSheet();
    }
	return pSheet;
}

BOOL SdrObjEditView::SetStyleSheet(SfxStyleSheet* pStyleSheet, BOOL bDontRemoveHardAttr)
{
	if( mxSelectionController.is() )
	{
		if( mxSelectionController->SetStyleSheet( pStyleSheet, bDontRemoveHardAttr ) )
			return TRUE;
	}

	// if we are currently in edit mode we must also set the stylesheet
	// on all paragraphs in the Outliner for the edit view
	// #92191#
	if( NULL != pTextEditOutlinerView )
	{
		Outliner* pOutliner = pTextEditOutlinerView->GetOutliner();

		const ULONG nParaCount = pOutliner->GetParagraphCount();
		ULONG nPara;
		for( nPara = 0; nPara < nParaCount; nPara++ )
		{
			pOutliner->SetStyleSheet( nPara, pStyleSheet );
		}
	}

	return SdrGlueEditView::SetStyleSheet(pStyleSheet,bDontRemoveHardAttr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrObjEditView::AddWindowToPaintView(OutputDevice* pNewWin)
{
    SdrGlueEditView::AddWindowToPaintView(pNewWin);
    
	if(mxTextEditObj.is() && !bTextEditOnlyOneView && pNewWin->GetOutDevType()==OUTDEV_WINDOW) 
	{
        OutlinerView* pOutlView=ImpMakeOutlinerView((Window*)pNewWin,FALSE,NULL);
        pTextEditOutliner->InsertView(pOutlView);
    }
}

void SdrObjEditView::DeleteWindowFromPaintView(OutputDevice* pOldWin)
{
    SdrGlueEditView::DeleteWindowFromPaintView(pOldWin);
    
	if(mxTextEditObj.is() && !bTextEditOnlyOneView && pOldWin->GetOutDevType()==OUTDEV_WINDOW) 
	{
        for (ULONG i=pTextEditOutliner->GetViewCount(); i>0;) {
            i--;
            OutlinerView* pOLV=pTextEditOutliner->GetView(i);
            if (pOLV && pOLV->GetWindow()==(Window*)pOldWin) {
                delete pTextEditOutliner->RemoveView(i);
            }
        }
    }
}

BOOL SdrObjEditView::IsTextEditInSelectionMode() const
{
    return pTextEditOutliner!=NULL && pTextEditOutliner->IsInSelectionMode();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@   @@  @@@@   @@@@  @@@@@   @@@@   @@   @@  @@@@  @@@@@  @@@@@
//  @@@ @@@ @@  @@ @@  @@ @@  @@ @@  @@  @@@ @@@ @@  @@ @@  @@ @@
//  @@@@@@@ @@  @@ @@     @@  @@ @@  @@  @@@@@@@ @@  @@ @@  @@ @@
//  @@@@@@@ @@@@@@ @@     @@@@@  @@  @@  @@@@@@@ @@  @@ @@  @@ @@@@
//  @@ @ @@ @@  @@ @@     @@  @@ @@  @@  @@ @ @@ @@  @@ @@  @@ @@
//  @@   @@ @@  @@ @@  @@ @@  @@ @@  @@  @@   @@ @@  @@ @@  @@ @@
//  @@   @@ @@  @@  @@@@  @@  @@  @@@@   @@   @@  @@@@  @@@@@  @@@@@
//
////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL SdrObjEditView::BegMacroObj(const Point& rPnt, short nTol, SdrObject* pObj, SdrPageView* pPV, Window* pWin)
{
    BOOL bRet=FALSE;
    BrkMacroObj();
    if (pObj!=NULL && pPV!=NULL && pWin!=NULL && pObj->HasMacro()) {
        nTol=ImpGetHitTolLogic(nTol,NULL);
        pMacroObj=pObj;
        pMacroPV=pPV;
        pMacroWin=pWin;
        bMacroDown=FALSE;
        nMacroTol=USHORT(nTol);
        aMacroDownPos=rPnt;
        MovMacroObj(rPnt);
    }
    return bRet;
}

void SdrObjEditView::ImpMacroUp(const Point& rUpPos)
{
    if (pMacroObj!=NULL && bMacroDown) 
    {
        SdrObjMacroHitRec aHitRec;
        aHitRec.aPos=rUpPos;
        aHitRec.aDownPos=aMacroDownPos;
        aHitRec.nTol=nMacroTol;
        aHitRec.pVisiLayer=&pMacroPV->GetVisibleLayers();
        aHitRec.pPageView=pMacroPV;
        aHitRec.pOut=pMacroWin;
        pMacroObj->PaintMacro(*pMacroWin,Rectangle(),aHitRec);
        bMacroDown=FALSE;
    }
}

void SdrObjEditView::ImpMacroDown(const Point& rDownPos)
{
    if (pMacroObj!=NULL && !bMacroDown) 
    {
        SdrObjMacroHitRec aHitRec;
        aHitRec.aPos=rDownPos;
        aHitRec.aDownPos=aMacroDownPos;
        aHitRec.nTol=nMacroTol;
        aHitRec.pVisiLayer=&pMacroPV->GetVisibleLayers();
        aHitRec.pPageView=pMacroPV;
        aHitRec.bDown=TRUE;
        aHitRec.pOut=pMacroWin;
        pMacroObj->PaintMacro(*pMacroWin,Rectangle(),aHitRec);
        bMacroDown=TRUE;
    }
}

void SdrObjEditView::MovMacroObj(const Point& rPnt)
{
    if (pMacroObj!=NULL) {
        SdrObjMacroHitRec aHitRec;
        aHitRec.aPos=rPnt;
        aHitRec.aDownPos=aMacroDownPos;
        aHitRec.nTol=nMacroTol;
        aHitRec.pVisiLayer=&pMacroPV->GetVisibleLayers();
        aHitRec.pPageView=pMacroPV;
        aHitRec.bDown=bMacroDown;
        aHitRec.pOut=pMacroWin;
        BOOL bDown=pMacroObj->IsMacroHit(aHitRec);
        if (bDown) ImpMacroDown(rPnt);
        else ImpMacroUp(rPnt);
    }
}

void SdrObjEditView::BrkMacroObj()
{
    if (pMacroObj!=NULL) {
        ImpMacroUp(aMacroDownPos);
        pMacroObj=NULL;
        pMacroPV=NULL;
        pMacroWin=NULL;
    }
}

BOOL SdrObjEditView::EndMacroObj()
{
    if (pMacroObj!=NULL && bMacroDown) {
        ImpMacroUp(aMacroDownPos);
        SdrObjMacroHitRec aHitRec;
        aHitRec.aPos=aMacroDownPos;
        aHitRec.aDownPos=aMacroDownPos;
        aHitRec.nTol=nMacroTol;
        aHitRec.pVisiLayer=&pMacroPV->GetVisibleLayers();
        aHitRec.pPageView=pMacroPV;
        aHitRec.bDown=TRUE;
        aHitRec.pOut=pMacroWin;
        bool bRet=pMacroObj->DoMacro(aHitRec);
        pMacroObj=NULL;
        pMacroPV=NULL;
        pMacroWin=NULL;
        return bRet;
    } else {
        BrkMacroObj();
        return FALSE;
    }
}

/** fills the given any with a XTextCursor for the current text selection.
	Leaves the any untouched if there currently is no text selected */
void SdrObjEditView::getTextSelection( ::com::sun::star::uno::Any& rSelection )
{
	if( IsTextEdit() )
	{
		OutlinerView* pOutlinerView = GetTextEditOutlinerView();
		if( pOutlinerView && pOutlinerView->HasSelection() )
		{
			SdrObject* pObj = GetTextEditObject();

			if( pObj )
			{
				::com::sun::star::uno::Reference< ::com::sun::star::text::XText > xText( pObj->getUnoShape(), ::com::sun::star::uno::UNO_QUERY );
				if( xText.is() )
				{
					SvxUnoTextBase* pRange = SvxUnoTextBase::getImplementation( xText );
					if( pRange )
					{
						rSelection <<= pRange->createTextCursorBySelection( pOutlinerView->GetSelection() );
					}
				}
			}
		}
	}
}

namespace sdr { namespace table {
extern rtl::Reference< sdr::SelectionController > CreateTableController( SdrObjEditView* pView, const SdrObject* pObj, const rtl::Reference< sdr::SelectionController >& xRefController );
} }

/* check if we have a single selection and that single object likes
	to handle the mouse and keyboard events itself
	
	@todo: the selection controller should be queried from the 
	object specific view contact. Currently this method only
	works for tables.
*/
void SdrObjEditView::MarkListHasChanged()
{
	SdrGlueEditView::MarkListHasChanged();

	if( mxSelectionController.is() )
	{
		mxLastSelectionController = mxSelectionController;
		mxSelectionController->onSelectionHasChanged();
	}

	mxSelectionController.clear();

	const SdrMarkList& rMarkList=GetMarkedObjectList();
	if( rMarkList.GetMarkCount() == 1 )
	{
		const SdrObject* pObj= rMarkList.GetMark(0)->GetMarkedSdrObj();
		// check for table
		if( pObj && (pObj->GetObjInventor() == SdrInventor ) && (pObj->GetObjIdentifier() == OBJ_TABLE) )
		{
			mxSelectionController = sdr::table::CreateTableController( this, pObj, mxLastSelectionController );
			if( mxSelectionController.is() )
			{
				mxLastSelectionController.clear();
				mxSelectionController->onSelectionHasChanged();
			}
		}
	}
}

IMPL_LINK( SdrObjEditView, EndPasteOrDropHdl, PasteOrDropInfos*, pInfos )
{
    OnEndPasteOrDrop( pInfos );
    return 0;
}

IMPL_LINK( SdrObjEditView, BeginPasteOrDropHdl, PasteOrDropInfos*, pInfos )
{
    OnBeginPasteOrDrop( pInfos );
    return 0;
}

void SdrObjEditView::OnBeginPasteOrDrop( PasteOrDropInfos* )
{
    // applications can derive from these virtual methods to do something before a drop or paste operation
}

void SdrObjEditView::OnEndPasteOrDrop( PasteOrDropInfos* )
{
    // applications can derive from these virtual methods to do something before a drop or paste operation
}

