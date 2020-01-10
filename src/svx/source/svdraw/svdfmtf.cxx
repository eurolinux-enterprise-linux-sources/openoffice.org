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

#include "svdfmtf.hxx"
#include <svx/editdata.hxx>
#include <math.h>
#include <svx/xpoly.hxx>
#include <vcl/svapp.hxx>
#include <svx/eeitem.hxx>
#include <svx/fhgtitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/postitem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/crsditem.hxx>
#include <svx/shdditem.hxx>
#include <svx/xlnclit.hxx>
#include <svx/xlnwtit.hxx>
#include <svx/xflclit.hxx>

#include <svx/xgrad.hxx>
#include <svx/xflgrit.hxx>
#include <fontitem.hxx>
#include <svx/akrnitem.hxx>
#include <svx/wrlmitem.hxx>
#include <svx/cntritem.hxx>
#include <svx/colritem.hxx>
#include <vcl/metric.hxx>

#include <svx/charscaleitem.hxx>

#include <svx/xflhtit.hxx>
#include <svx/svdattr.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdobj.hxx>
#include "svditext.hxx"
#include <svx/svdotext.hxx>
#include <svx/svdorect.hxx>
#include <svx/svdocirc.hxx>
#include <svx/svdograf.hxx>
#include <svx/svdopath.hxx>
#include <svx/svdetc.hxx>
#include <svtools/itemset.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <vcl/salbtype.hxx>		// FRound

// #i73407#
#include <basegfx/matrix/b2dhommatrix.hxx>

////////////////////////////////////////////////////////////////////////////////////////////////////

ImpSdrGDIMetaFileImport::ImpSdrGDIMetaFileImport(SdrModel& rModel):
	nMapScalingOfs(0),
	pLineAttr(NULL),pFillAttr(NULL),pTextAttr(NULL),
	pPage(NULL),pModel(NULL),nLayer(0),
	nLineWidth(0),
	bFntDirty(TRUE),
	bLastObjWasPolyWithoutLine(FALSE),bNoLine(FALSE),bNoFill(FALSE),bLastObjWasLine(FALSE)
{
	aVD.EnableOutput(FALSE);
	aOldLineColor.SetRed( aVD.GetLineColor().GetRed() + 1 ); // invalidate old line color
	pLineAttr=new SfxItemSet(rModel.GetItemPool(),XATTR_LINE_FIRST,XATTR_LINE_LAST);
	pFillAttr=new SfxItemSet(rModel.GetItemPool(),XATTR_FILL_FIRST,XATTR_FILL_LAST);
	pTextAttr=new SfxItemSet(rModel.GetItemPool(),EE_ITEMS_START,EE_ITEMS_END);
	pModel=&rModel;
}

ImpSdrGDIMetaFileImport::~ImpSdrGDIMetaFileImport()
{
	delete pLineAttr;
	delete pFillAttr;
	delete pTextAttr;
}

ULONG ImpSdrGDIMetaFileImport::DoImport(const GDIMetaFile& rMtf,
	SdrObjList& rOL,
	ULONG nInsPos,
	SvdProgressInfo *pProgrInfo)
{
	pPage = rOL.GetPage();
	GDIMetaFile* pTmpMtf=NULL;
	GDIMetaFile* pMtf = (GDIMetaFile*) &rMtf;
	ULONG nActionAnz=pMtf->GetActionCount();
	sal_Bool bError = sal_False;


	// setup some global scale parameter
	// fScaleX, fScaleY, aScaleX, aScaleY, bMov, bSize
    fScaleX = fScaleY = 1.0;
	Size  aMtfSize( pMtf->GetPrefSize() );
	if ( aMtfSize.Width() & aMtfSize.Height() && ( aScaleRect.IsEmpty() == sal_False ) )
    {
		aOfs = aScaleRect.TopLeft();
        if ( aMtfSize.Width() != ( aScaleRect.GetWidth() - 1 ) )
            fScaleX = (double)( aScaleRect.GetWidth() - 1 ) / (double)aMtfSize.Width();
        if ( aMtfSize.Height() != ( aScaleRect.GetHeight() - 1 ) )
            fScaleY = (double)( aScaleRect.GetHeight() - 1 ) / (double)aMtfSize.Height();
    }

	bMov = aOfs.X()!=0 || aOfs.Y()!=0;
    bSize = sal_False;

	aScaleX = Fraction( 1, 1 );
	aScaleY = Fraction( 1, 1 );
    if ( aMtfSize.Width() != ( aScaleRect.GetWidth() - 1 ) )
    {
        aScaleX = Fraction( aScaleRect.GetWidth() - 1, aMtfSize.Width() );
        bSize = sal_True;
    }
    if ( aMtfSize.Height() != ( aScaleRect.GetHeight() - 1 ) )
    {
        aScaleY = Fraction( aScaleRect.GetHeight() - 1, aMtfSize.Height() );
        bSize = sal_True;
    }

	if(65000 < nActionAnz)
	{
		nActionAnz = 65000;
		bError = TRUE;
	}

	if(pProgrInfo)
		pProgrInfo->SetActionCount(nActionAnz);

	ULONG nActionsToReport = 0;

	for( MetaAction* pAct = pMtf->FirstAction(); pAct; pAct = pMtf->NextAction() )
	{
		switch (pAct->GetType())
		{
			case META_PIXEL_ACTION          : DoAction((MetaPixelAction          &)*pAct); break;
			case META_POINT_ACTION          : DoAction((MetaPointAction          &)*pAct); break;
			case META_LINE_ACTION           : DoAction((MetaLineAction           &)*pAct); break;
			case META_RECT_ACTION           : DoAction((MetaRectAction           &)*pAct); break;
			case META_ROUNDRECT_ACTION      : DoAction((MetaRoundRectAction      &)*pAct); break;
			case META_ELLIPSE_ACTION        : DoAction((MetaEllipseAction        &)*pAct); break;
			case META_ARC_ACTION            : DoAction((MetaArcAction            &)*pAct); break;
			case META_PIE_ACTION            : DoAction((MetaPieAction            &)*pAct); break;
			case META_CHORD_ACTION          : DoAction((MetaChordAction          &)*pAct); break;
			case META_POLYLINE_ACTION	    : DoAction((MetaPolyLineAction		 &)*pAct); break;
			case META_POLYGON_ACTION        : DoAction((MetaPolygonAction        &)*pAct); break;
			case META_POLYPOLYGON_ACTION    : DoAction((MetaPolyPolygonAction    &)*pAct); break;
			case META_TEXT_ACTION           : DoAction((MetaTextAction           &)*pAct); break;
			case META_TEXTARRAY_ACTION      : DoAction((MetaTextArrayAction      &)*pAct); break;
			case META_STRETCHTEXT_ACTION    : DoAction((MetaStretchTextAction    &)*pAct); break;
			case META_BMP_ACTION			: DoAction((MetaBmpAction			 &)*pAct); break;
			case META_BMPSCALE_ACTION		: DoAction((MetaBmpScaleAction		 &)*pAct); break;
			case META_BMPEX_ACTION			: DoAction((MetaBmpExAction			 &)*pAct); break;
			case META_BMPEXSCALE_ACTION		: DoAction((MetaBmpExScaleAction	 &)*pAct); break;
			case META_LINECOLOR_ACTION      : DoAction((MetaLineColorAction      &)*pAct); break;
			case META_FILLCOLOR_ACTION      : DoAction((MetaFillColorAction      &)*pAct); break;
			case META_TEXTCOLOR_ACTION      : DoAction((MetaTextColorAction      &)*pAct); break;
			case META_TEXTFILLCOLOR_ACTION  : DoAction((MetaTextFillColorAction  &)*pAct); break;
			case META_FONT_ACTION           : DoAction((MetaFontAction           &)*pAct); break;
			case META_TEXTALIGN_ACTION		: DoAction((MetaTextAlignAction		 &)*pAct); break;
			case META_MAPMODE_ACTION        : DoAction((MetaMapModeAction        &)*pAct); break;
			case META_CLIPREGION_ACTION     : DoAction((MetaClipRegionAction     &)*pAct); break;
			case META_MOVECLIPREGION_ACTION : DoAction((MetaMoveClipRegionAction &)*pAct); break;
			case META_ISECTRECTCLIPREGION_ACTION: DoAction((MetaISectRectClipRegionAction&)*pAct); break;
			case META_ISECTREGIONCLIPREGION_ACTION: DoAction((MetaISectRegionClipRegionAction&)*pAct); break;
			case META_RASTEROP_ACTION       : DoAction((MetaRasterOpAction       &)*pAct); break;
			case META_PUSH_ACTION           : DoAction((MetaPushAction           &)*pAct); break;
			case META_POP_ACTION            : DoAction((MetaPopAction            &)*pAct); break;
			case META_HATCH_ACTION			: DoAction((MetaHatchAction          &)*pAct); break;
			case META_COMMENT_ACTION		: DoAction((MetaCommentAction        &)*pAct, pMtf); break;

		}

		if(pProgrInfo != NULL)
		{
		  nActionsToReport++;
		  if(nActionsToReport >= 16) // Alle 16 Action updaten
		  {
			if(!pProgrInfo->ReportActions(nActionsToReport))
			  break;
			nActionsToReport = 0;
		  }
		}
	}

	if(pProgrInfo != NULL)
	{
		pProgrInfo->ReportActions(nActionsToReport);
		nActionsToReport = 0;
	}

	// MapMode-Scaling  vornehmen
	MapScaling();
	// Objekte in vorgegebenes Rechteck hineinskalieren
	ULONG nAnz=aTmpList.GetObjCount();

	// Beim berechnen der Fortschrittsanzeige wird GetActionCount()*3 benutzt.
	// Da in aTmpList allerdings weniger eintraege als GetActionCount()
	// existieren koennen, muessen hier die zuviel vermuteten Actionen wieder
	// hinzugefuegt werden.
	nActionsToReport = (pMtf->GetActionCount() - nAnz)*2;


	// Alle noch nicht gemeldeten Rescales melden
	if(pProgrInfo)
	{
		pProgrInfo->ReportRescales(nActionsToReport);
		pProgrInfo->SetInsertCount(nAnz);
	}
	nActionsToReport = 0;

	// alle in aTmpList zwischengespeicherten Objekte nun in rOL ab der Position nInsPos einfuegen
	if (nInsPos>rOL.GetObjCount()) nInsPos=rOL.GetObjCount();
	SdrInsertReason aReason(SDRREASON_VIEWCALL);
	for (ULONG i=0; i<nAnz; i++)
	{
		 SdrObject* pObj=aTmpList.GetObj(i);
		 rOL.NbcInsertObject(pObj,nInsPos,&aReason);
		 nInsPos++;

		if(pProgrInfo != NULL)
		{
			nActionsToReport++;
			if(nActionsToReport >= 32) // Alle 32 Action updaten
			{
				pProgrInfo->ReportInserts(nActionsToReport);
				nActionsToReport = 0;
			}
		}
	}
	if (pTmpMtf!=NULL) delete pTmpMtf;

	// ein letztesmal alle verbliebennen Inserts reporten
	if(pProgrInfo != NULL)
	{
		pProgrInfo->ReportInserts(nActionsToReport);
		if(bError)
			pProgrInfo->ReportError();
	}

	return aTmpList.GetObjCount();
}

void ImpSdrGDIMetaFileImport::SetAttributes(SdrObject* pObj, FASTBOOL bForceTextAttr)
{
	bNoLine = FALSE; bNoFill = FALSE;
	FASTBOOL bLine=TRUE && !bForceTextAttr;
	FASTBOOL bFill=pObj==NULL || pObj->IsClosedObj() && !bForceTextAttr;
	FASTBOOL bText=bForceTextAttr || (pObj!=NULL && pObj->GetOutlinerParaObject()!=NULL);

	if ( bLine )
	{
		if ( nLineWidth )
			pLineAttr->Put( XLineWidthItem( nLineWidth ) );
		aOldLineColor = aVD.GetLineColor();
		if( aVD.IsLineColor() )
		{
			pLineAttr->Put(XLineStyleItem(XLINE_SOLID));
			pLineAttr->Put(XLineColorItem(String(), aVD.GetLineColor()));
		}
		else
			pLineAttr->Put(XLineStyleItem(XLINE_NONE));
	}
	else
		bNoLine = TRUE;

	if ( bFill )
	{
		if( aVD.IsFillColor() )
		{
			pFillAttr->Put(XFillStyleItem(XFILL_SOLID));
			pFillAttr->Put(XFillColorItem(String(), aVD.GetFillColor()));
		}
		else
			pFillAttr->Put(XFillStyleItem(XFILL_NONE));
	}
	else
		bNoFill = TRUE;

	if ( bText && bFntDirty )
	{
		Font aFnt(aVD.GetFont());
		pTextAttr->Put( SvxFontItem( aFnt.GetFamily(), aFnt.GetName(), aFnt.GetStyleName(),
							aFnt.GetPitch(), aFnt.GetCharSet(), EE_CHAR_FONTINFO ) );
		pTextAttr->Put( SvxFontItem( aFnt.GetFamily(), aFnt.GetName(), aFnt.GetStyleName(),
							aFnt.GetPitch(), aFnt.GetCharSet(), EE_CHAR_FONTINFO_CJK ) );
		pTextAttr->Put( SvxFontItem( aFnt.GetFamily(), aFnt.GetName(), aFnt.GetStyleName(),
							aFnt.GetPitch(), aFnt.GetCharSet(), EE_CHAR_FONTINFO_CTL ) );
        pTextAttr->Put(SvxPostureItem(aFnt.GetItalic(), EE_CHAR_ITALIC));
        pTextAttr->Put(SvxWeightItem(aFnt.GetWeight(), EE_CHAR_WEIGHT));
		sal_uInt32 nHeight = FRound(aFnt.GetSize().Height() * fScaleY);
		pTextAttr->Put( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT ) );
		pTextAttr->Put( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT_CJK ) );
		pTextAttr->Put( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT_CTL ) );
        pTextAttr->Put(SvxCharScaleWidthItem(100, EE_CHAR_FONTWIDTH));
        pTextAttr->Put(SvxUnderlineItem(aFnt.GetUnderline(), EE_CHAR_UNDERLINE));
        pTextAttr->Put(SvxOverlineItem(aFnt.GetOverline(), EE_CHAR_OVERLINE));
        pTextAttr->Put(SvxCrossedOutItem(aFnt.GetStrikeout(), EE_CHAR_STRIKEOUT));
        pTextAttr->Put(SvxShadowedItem(aFnt.IsShadow(), EE_CHAR_SHADOW));
        pTextAttr->Put(SvxAutoKernItem(aFnt.IsKerning(), EE_CHAR_KERNING));
        pTextAttr->Put(SvxWordLineModeItem(aFnt.IsWordLineMode(), EE_CHAR_WLM));
        pTextAttr->Put(SvxContourItem(aFnt.IsOutline(), EE_CHAR_OUTLINE));
        pTextAttr->Put(SvxColorItem(aFnt.GetColor(), EE_CHAR_COLOR));
		//... svxfont textitem svditext
		bFntDirty=FALSE;
	}
	if (pObj!=NULL)
	{
		pObj->SetLayer(nLayer);
		if (bLine) pObj->SetMergedItemSet(*pLineAttr);
		if (bFill) pObj->SetMergedItemSet(*pFillAttr);
		if (bText)
		{
			pObj->SetMergedItemSet(*pTextAttr);
			pObj->SetMergedItem( SdrTextHorzAdjustItem( SDRTEXTHORZADJUST_LEFT ) );
		}
	}
}

void ImpSdrGDIMetaFileImport::InsertObj( SdrObject* pObj, sal_Bool bScale )
{
	if ( bScale && !aScaleRect.IsEmpty() )
    {
		if ( bSize )
			pObj->NbcResize( Point(), aScaleX, aScaleY );
		if ( bMov )
			pObj->NbcMove( Size( aOfs.X(), aOfs.Y() ) );
	}

	aTmpList.InsertObject( pObj );
	if ( HAS_BASE( SdrPathObj, pObj ) )
	{
		FASTBOOL bClosed=pObj->IsClosedObj();
		bLastObjWasPolyWithoutLine=bNoLine && bClosed;
		bLastObjWasLine=!bClosed;
	}
	else
	{
		bLastObjWasPolyWithoutLine = FALSE;
		bLastObjWasLine = FALSE;
	}
}

/**************************************************************************************************/

void ImpSdrGDIMetaFileImport::DoAction(MetaPixelAction& /*rAct*/)
{
}

void ImpSdrGDIMetaFileImport::DoAction(MetaPointAction& /*rAct*/)
{
}

void ImpSdrGDIMetaFileImport::DoAction(MetaLineAction& rAct)
{
	// #i73407# reformulation to use new B2DPolygon classes
	const basegfx::B2DPoint aStart(rAct.GetStartPoint().X(), rAct.GetStartPoint().Y());
	const basegfx::B2DPoint aEnd(rAct.GetEndPoint().X(), rAct.GetEndPoint().Y());

	if(!aStart.equal(aEnd))
	{
		basegfx::B2DPolygon aLine;
		basegfx::B2DHomMatrix aTransform;

		aLine.append(aStart);
		aLine.append(aEnd);
		aTransform.scale(fScaleX, fScaleY);
		aTransform.translate(aOfs.X(), aOfs.Y());
		aLine.transform(aTransform);

		const LineInfo& rLineInfo = rAct.GetLineInfo();
		const sal_Int32 nNewLineWidth(rLineInfo.GetWidth());
		bool bCreateLineObject(true);

		if(bLastObjWasLine && (nNewLineWidth == nLineWidth) && CheckLastLineMerge(aLine))
		{
			bCreateLineObject = false;
		}

		nLineWidth = nNewLineWidth;

		if(bCreateLineObject)
		{
			SdrPathObj* pPath = new SdrPathObj(OBJ_LINE, basegfx::B2DPolyPolygon(aLine));
			SetAttributes(pPath);
			InsertObj(pPath, false);
		}
	}
}

void ImpSdrGDIMetaFileImport::DoAction(MetaRectAction& rAct)
{
	SdrRectObj* pRect=new SdrRectObj(rAct.GetRect());
	SetAttributes(pRect);
	InsertObj(pRect);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaRoundRectAction& rAct)
{
	SdrRectObj* pRect=new SdrRectObj(rAct.GetRect());
	SetAttributes(pRect);
	long nRad=(rAct.GetHorzRound()+rAct.GetVertRound())/2;
	if (nRad!=0) {
		SfxItemSet aSet(*pLineAttr->GetPool(),SDRATTR_ECKENRADIUS,SDRATTR_ECKENRADIUS);
		aSet.Put(SdrEckenradiusItem(nRad));
		pRect->SetMergedItemSet(aSet);
	}
	InsertObj(pRect);
}

/**************************************************************************************************/

void ImpSdrGDIMetaFileImport::DoAction(MetaEllipseAction& rAct)
{
	SdrCircObj* pCirc=new SdrCircObj(OBJ_CIRC,rAct.GetRect());
	SetAttributes(pCirc);
	InsertObj(pCirc);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaArcAction& rAct)
{
	Point aCenter(rAct.GetRect().Center());
	long nStart=GetAngle(rAct.GetStartPoint()-aCenter);
	long nEnd=GetAngle(rAct.GetEndPoint()-aCenter);
	SdrCircObj* pCirc=new SdrCircObj(OBJ_CARC,rAct.GetRect(),nStart,nEnd);
	SetAttributes(pCirc);
	InsertObj(pCirc);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaPieAction& rAct)
{
	Point aCenter(rAct.GetRect().Center());
	long nStart=GetAngle(rAct.GetStartPoint()-aCenter);
	long nEnd=GetAngle(rAct.GetEndPoint()-aCenter);
	SdrCircObj* pCirc=new SdrCircObj(OBJ_SECT,rAct.GetRect(),nStart,nEnd);
	SetAttributes(pCirc);
	InsertObj(pCirc);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaChordAction& rAct)
{
	Point aCenter(rAct.GetRect().Center());
	long nStart=GetAngle(rAct.GetStartPoint()-aCenter);
	long nEnd=GetAngle(rAct.GetEndPoint()-aCenter);
	SdrCircObj* pCirc=new SdrCircObj(OBJ_CCUT,rAct.GetRect(),nStart,nEnd);
	SetAttributes(pCirc);
	InsertObj(pCirc);
}

/**************************************************************************************************/

bool ImpSdrGDIMetaFileImport::CheckLastLineMerge(const basegfx::B2DPolygon& rSrcPoly)
{
	// #i102706# Do not merge closed polygons
	if(rSrcPoly.isClosed())
	{
		return false;
	}

	// #i73407# reformulation to use new B2DPolygon classes
	if(bLastObjWasLine && (aOldLineColor == aVD.GetLineColor()) && rSrcPoly.count())
	{
		SdrObject* pTmpObj = aTmpList.GetObj(aTmpList.GetObjCount() - 1);
		SdrPathObj* pLastPoly = PTR_CAST(SdrPathObj, pTmpObj);

		if(pLastPoly)
		{
			if(1L == pLastPoly->GetPathPoly().count())
			{
				bool bOk(false);
				basegfx::B2DPolygon aDstPoly(pLastPoly->GetPathPoly().getB2DPolygon(0L));

				// #i102706# Do not merge closed polygons
				if(aDstPoly.isClosed())
				{
					return false;
				}
				
				if(aDstPoly.count())
				{
					const sal_uInt32 nMaxDstPnt(aDstPoly.count() - 1L);
					const sal_uInt32 nMaxSrcPnt(rSrcPoly.count() - 1L);

					if(aDstPoly.getB2DPoint(nMaxDstPnt) == rSrcPoly.getB2DPoint(0L))
					{
						aDstPoly.append(rSrcPoly, 1L, rSrcPoly.count() - 1L);
						bOk = true;
					}
					else if(aDstPoly.getB2DPoint(0L) == rSrcPoly.getB2DPoint(nMaxSrcPnt))
					{
						basegfx::B2DPolygon aNew(rSrcPoly);
						aNew.append(aDstPoly, 1L, aDstPoly.count() - 1L);
						aDstPoly = aNew;
						bOk = true;
					}
					else if(aDstPoly.getB2DPoint(0L) == rSrcPoly.getB2DPoint(0L))
					{
						aDstPoly.flip();
						aDstPoly.append(rSrcPoly, 1L, rSrcPoly.count() - 1L);
						bOk = true;
					}
					else if(aDstPoly.getB2DPoint(nMaxDstPnt) == rSrcPoly.getB2DPoint(nMaxSrcPnt))
					{
						basegfx::B2DPolygon aNew(rSrcPoly);
						aNew.flip();
						aDstPoly.append(aNew, 1L, aNew.count() - 1L);
						bOk = true;
					}
				}

				if(bOk)
				{
					pLastPoly->NbcSetPathPoly(basegfx::B2DPolyPolygon(aDstPoly));
				}

				return bOk;
			}
		}
	}

	return false;
}

bool ImpSdrGDIMetaFileImport::CheckLastPolyLineAndFillMerge(const basegfx::B2DPolyPolygon & rPolyPolygon)
{
	// #i73407# reformulation to use new B2DPolygon classes
	if(bLastObjWasPolyWithoutLine)
	{
		SdrObject* pTmpObj = aTmpList.GetObj(aTmpList.GetObjCount() - 1);
		SdrPathObj* pLastPoly = PTR_CAST(SdrPathObj, pTmpObj);

		if(pLastPoly)
		{
			if(pLastPoly->GetPathPoly() == rPolyPolygon)
			{
				SetAttributes(NULL);

				if(!bNoLine && bNoFill)
				{
					pLastPoly->SetMergedItemSet(*pLineAttr);

					return true;
				}
			}
		}
	}

	return false;
}


void ImpSdrGDIMetaFileImport::DoAction( MetaPolyLineAction& rAct )
{
	// #i73407# reformulation to use new B2DPolygon classes
	basegfx::B2DPolygon aSource(rAct.GetPolygon().getB2DPolygon());

	if(aSource.count())
	{
		basegfx::B2DHomMatrix aTransform;

		aTransform.scale(fScaleX, fScaleY);
		aTransform.translate(aOfs.X(), aOfs.Y());
		aSource.transform(aTransform);
	}

	const LineInfo& rLineInfo = rAct.GetLineInfo();
	const sal_Int32 nNewLineWidth(rLineInfo.GetWidth());
	bool bCreateLineObject(true);

	if(bLastObjWasLine && (nNewLineWidth == nLineWidth) && CheckLastLineMerge(aSource))
	{
		bCreateLineObject = false;
	}
	else if(bLastObjWasPolyWithoutLine && CheckLastPolyLineAndFillMerge(basegfx::B2DPolyPolygon(aSource)))
	{
		bCreateLineObject = false;
	}

	nLineWidth = nNewLineWidth;

	if(bCreateLineObject)
	{
		SdrPathObj* pPath = new SdrPathObj(OBJ_PLIN, basegfx::B2DPolyPolygon(aSource));
		SetAttributes(pPath);
		InsertObj(pPath, false);
	}
}

void ImpSdrGDIMetaFileImport::DoAction( MetaPolygonAction& rAct )
{
	// #i73407# reformulation to use new B2DPolygon classes
	basegfx::B2DPolygon aSource(rAct.GetPolygon().getB2DPolygon());

	if(aSource.count())
	{
		basegfx::B2DHomMatrix aTransform;

		aTransform.scale(fScaleX, fScaleY);
		aTransform.translate(aOfs.X(), aOfs.Y());
		aSource.transform(aTransform);

		if(!bLastObjWasPolyWithoutLine || !CheckLastPolyLineAndFillMerge(basegfx::B2DPolyPolygon(aSource)))
		{
			// #i73407# make sure polygon is closed, it's a filled primitive
			aSource.setClosed(true);

			SdrPathObj* pPath = new SdrPathObj(OBJ_POLY, basegfx::B2DPolyPolygon(aSource));
			SetAttributes(pPath);
			InsertObj(pPath, false);
		}
	}
}

void ImpSdrGDIMetaFileImport::DoAction(MetaPolyPolygonAction& rAct)
{
	// #i73407# reformulation to use new B2DPolygon classes
	basegfx::B2DPolyPolygon aSource(rAct.GetPolyPolygon().getB2DPolyPolygon());

	if(aSource.count())
	{
		basegfx::B2DHomMatrix aTransform;

		aTransform.scale(fScaleX, fScaleY);
		aTransform.translate(aOfs.X(), aOfs.Y());
		aSource.transform(aTransform);

		if(!bLastObjWasPolyWithoutLine || !CheckLastPolyLineAndFillMerge(aSource))
		{
			// #i73407# make sure polygon is closed, it's a filled primitive
			aSource.setClosed(true);

			SdrPathObj* pPath = new SdrPathObj(OBJ_POLY, aSource);
			SetAttributes(pPath);
			InsertObj(pPath, false);
		}
	}
}

/**************************************************************************************************/

void ImpSdrGDIMetaFileImport::ImportText( const Point& rPos, const XubString& rStr, const MetaAction& rAct )
{
	// calc text box size, add 5% to make it fit safely

	FontMetric aFontMetric( aVD.GetFontMetric() );
	Font aFnt( aVD.GetFont() );
	FontAlign eAlg( aFnt.GetAlign() );

    sal_Int32 nTextWidth = (sal_Int32)( aVD.GetTextWidth( rStr ) * fScaleX );
    sal_Int32 nTextHeight = (sal_Int32)( aVD.GetTextHeight() * fScaleY );
	//sal_Int32 nDxWidth = 0;
	//sal_Int32 nLen = rStr.Len();

	Point aPos( FRound(rPos.X() * fScaleX + aOfs.X()), FRound(rPos.Y() * fScaleY + aOfs.Y()) );
    Size aSize( nTextWidth, nTextHeight );

	if ( eAlg == ALIGN_BASELINE )
		aPos.Y() -= FRound(aFontMetric.GetAscent() * fScaleY);
	else if ( eAlg == ALIGN_BOTTOM )
		aPos.Y() -= nTextHeight;

	Rectangle aTextRect( aPos, aSize );
	SdrRectObj* pText =new SdrRectObj( OBJ_TEXT, aTextRect );

	if ( aFnt.GetWidth() || ( rAct.GetType() == META_STRETCHTEXT_ACTION ) )
	{
		pText->ClearMergedItem( SDRATTR_TEXT_AUTOGROWWIDTH );
		pText->SetMergedItem( SdrTextAutoGrowHeightItem( FALSE ) );
		// don't let the margins eat the space needed for the text
		pText->SetMergedItem ( SdrTextUpperDistItem (0));
		pText->SetMergedItem ( SdrTextLowerDistItem (0));
		pText->SetMergedItem ( SdrTextRightDistItem (0));
		pText->SetMergedItem ( SdrTextLeftDistItem (0));
		pText->SetMergedItem( SdrTextFitToSizeTypeItem( SDRTEXTFIT_ALLLINES ) );
	}
	else
		pText->SetMergedItem( SdrTextAutoGrowWidthItem( sal_True ) );

	pText->SetModel( pModel );
	pText->SetLayer( nLayer );
	pText->NbcSetText( rStr );
	SetAttributes( pText, TRUE );
	pText->SetSnapRect( aTextRect );

	if (!aFnt.IsTransparent())
	{
		SfxItemSet aAttr(*pFillAttr->GetPool(),XATTR_FILL_FIRST,XATTR_FILL_LAST);
		pFillAttr->Put(XFillStyleItem(XFILL_SOLID));
		pFillAttr->Put(XFillColorItem(String(), aFnt.GetFillColor()));
		pText->SetMergedItemSet(aAttr);
	}
	sal_uInt32 nWink = aFnt.GetOrientation();
	if ( nWink )
	{
		nWink*=10;
		double a=nWink*nPi180;
		double nSin=sin(a);
		double nCos=cos(a);
		pText->NbcRotate(aPos,nWink,nSin,nCos);
	}
	InsertObj( pText, sal_False );
}

void ImpSdrGDIMetaFileImport::DoAction(MetaTextAction& rAct)
{
	XubString aStr(rAct.GetText());
	aStr.Erase(0,rAct.GetIndex());
	aStr.Erase(rAct.GetLen());
	ImportText( rAct.GetPoint(), aStr, rAct );
}

void ImpSdrGDIMetaFileImport::DoAction(MetaTextArrayAction& rAct)
{
	XubString aStr(rAct.GetText());
	aStr.Erase(0,rAct.GetIndex());
	aStr.Erase(rAct.GetLen());
	ImportText( rAct.GetPoint(), aStr, rAct );
}

void ImpSdrGDIMetaFileImport::DoAction(MetaStretchTextAction& rAct)
{
	XubString aStr(rAct.GetText());
	aStr.Erase(0,rAct.GetIndex());
	aStr.Erase(rAct.GetLen());
	ImportText( rAct.GetPoint(), aStr, rAct );
}

void ImpSdrGDIMetaFileImport::DoAction(MetaBmpAction& rAct)
{
	Rectangle aRect(rAct.GetPoint(),rAct.GetBitmap().GetSizePixel());
	aRect.Right()++; aRect.Bottom()++;
	SdrGrafObj* pGraf=new SdrGrafObj(Graphic(rAct.GetBitmap()),aRect);
	InsertObj(pGraf);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaBmpScaleAction& rAct)
{
	Rectangle aRect(rAct.GetPoint(),rAct.GetSize());
	aRect.Right()++; aRect.Bottom()++;
	SdrGrafObj* pGraf=new SdrGrafObj(Graphic(rAct.GetBitmap()),aRect);
	InsertObj(pGraf);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaBmpExAction& rAct)
{
	Rectangle aRect(rAct.GetPoint(),rAct.GetBitmapEx().GetSizePixel());
	aRect.Right()++; aRect.Bottom()++;
	SdrGrafObj* pGraf=new SdrGrafObj( rAct.GetBitmapEx(), aRect );
	InsertObj(pGraf);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaBmpExScaleAction& rAct)
{
	Rectangle aRect(rAct.GetPoint(),rAct.GetSize());
	aRect.Right()++; aRect.Bottom()++;
	SdrGrafObj* pGraf=new SdrGrafObj( rAct.GetBitmapEx(), aRect );
	InsertObj(pGraf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ImpSdrGDIMetaFileImport::DoAction( MetaHatchAction& rAct )
{
	// #i73407# reformulation to use new B2DPolygon classes
	basegfx::B2DPolyPolygon aSource(rAct.GetPolyPolygon().getB2DPolyPolygon());

	if(aSource.count())
	{
		basegfx::B2DHomMatrix aTransform;
		aTransform.scale(fScaleX, fScaleY);
		aTransform.translate(aOfs.X(), aOfs.Y());
		aSource.transform(aTransform);

		if(!bLastObjWasPolyWithoutLine || !CheckLastPolyLineAndFillMerge(aSource))
		{
			const Hatch& rHatch = rAct.GetHatch();
			SdrPathObj* pPath = new SdrPathObj(OBJ_POLY, aSource);
			SfxItemSet aHatchAttr(pModel->GetItemPool(),
				XATTR_FILLSTYLE, XATTR_FILLSTYLE,
				XATTR_FILLHATCH, XATTR_FILLHATCH, 0, 0 );
			XHatchStyle eStyle;

			switch(rHatch.GetStyle())
			{
				case(HATCH_TRIPLE) :
				{
					eStyle = XHATCH_TRIPLE;
					break;
				}

				case(HATCH_DOUBLE) :
				{
					eStyle = XHATCH_DOUBLE;
					break;
				}

				default:
				{
					eStyle = XHATCH_SINGLE;
					break;
				}
			}

			SetAttributes(pPath);
			aHatchAttr.Put(XFillStyleItem(XFILL_HATCH));
			aHatchAttr.Put(XFillHatchItem(&pModel->GetItemPool(), XHatch(rHatch.GetColor(), eStyle, rHatch.GetDistance(), rHatch.GetAngle())));
			pPath->SetMergedItemSet(aHatchAttr);

			InsertObj(pPath, false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ImpSdrGDIMetaFileImport::DoAction(MetaLineColorAction& rAct)
{
	rAct.Execute(&aVD);
}

void ImpSdrGDIMetaFileImport::DoAction(MetaMapModeAction& rAct)
{
	MapScaling();
	rAct.Execute(&aVD);
	bLastObjWasPolyWithoutLine=FALSE;
	bLastObjWasLine=FALSE;
}

void ImpSdrGDIMetaFileImport::MapScaling()
{
	sal_uInt32 i, nAnz = aTmpList.GetObjCount();
	const MapMode& rMap = aVD.GetMapMode();
	Point aMapOrg( rMap.GetOrigin() );
	sal_Bool bMov2 = aMapOrg.X() != 0 || aMapOrg.Y() != 0;
	if ( bMov2 )
    {
		for ( i = nMapScalingOfs; i < nAnz; i++ )
        {
			SdrObject* pObj = aTmpList.GetObj(i);
			if ( bMov2 )
				pObj->NbcMove( Size( aMapOrg.X(), aMapOrg.Y() ) );
		}
	}
	nMapScalingOfs = nAnz;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ImpSdrGDIMetaFileImport::DoAction( MetaCommentAction& rAct, GDIMetaFile* pMtf )
{
	ByteString aSkipComment;

	if( rAct.GetComment().CompareIgnoreCaseToAscii( "XGRAD_SEQ_BEGIN" ) == COMPARE_EQUAL )
	{
		MetaGradientExAction* pAct = (MetaGradientExAction*) pMtf->NextAction();

		if( pAct && pAct->GetType() == META_GRADIENTEX_ACTION )
		{
			// #i73407# reformulation to use new B2DPolygon classes
			basegfx::B2DPolyPolygon aSource(pAct->GetPolyPolygon().getB2DPolyPolygon());

			if(aSource.count())
			{
				basegfx::B2DHomMatrix aTransform;

				aTransform.scale(fScaleX, fScaleY);
				aTransform.translate(aOfs.X(), aOfs.Y());
				aSource.transform(aTransform);

				if(!bLastObjWasPolyWithoutLine || !CheckLastPolyLineAndFillMerge(aSource))
				{
					const Gradient&	rGrad = pAct->GetGradient();
					SdrPathObj* pPath = new SdrPathObj(OBJ_POLY, aSource);
					SfxItemSet aGradAttr(pModel->GetItemPool(),
					   XATTR_FILLSTYLE, XATTR_FILLSTYLE,
					   XATTR_FILLGRADIENT, XATTR_FILLGRADIENT, 0, 0 );
					XGradient aXGradient;

					aXGradient.SetGradientStyle((XGradientStyle)rGrad.GetStyle());
					aXGradient.SetStartColor(rGrad.GetStartColor());
					aXGradient.SetEndColor(rGrad.GetEndColor());
					aXGradient.SetAngle((USHORT)rGrad.GetAngle());
					aXGradient.SetBorder(rGrad.GetBorder());
					aXGradient.SetXOffset(rGrad.GetOfsX());
					aXGradient.SetYOffset(rGrad.GetOfsY());
					aXGradient.SetStartIntens(rGrad.GetStartIntensity());
					aXGradient.SetEndIntens(rGrad.GetEndIntensity());
					aXGradient.SetSteps(rGrad.GetSteps());

					SetAttributes(pPath);
					aGradAttr.Put(XFillStyleItem(XFILL_GRADIENT));
					aGradAttr.Put(XFillGradientItem(&pModel->GetItemPool(), aXGradient));
					pPath->SetMergedItemSet(aGradAttr);

					InsertObj(pPath);
				}
			}

			aSkipComment = "XGRAD_SEQ_END";
		}
	}

	if(aSkipComment.Len())
	{
		MetaAction* pSkipAct = pMtf->NextAction();

		while( pSkipAct
			&& ((pSkipAct->GetType() != META_COMMENT_ACTION )
				|| (((MetaCommentAction*)pSkipAct)->GetComment().CompareIgnoreCaseToAscii(aSkipComment.GetBuffer()) != COMPARE_EQUAL)))
		{
			pSkipAct = pMtf->NextAction();
		}
	}
}

// eof
