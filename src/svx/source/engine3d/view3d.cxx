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

#include <vcl/wrkwin.hxx>
#include <svx/svdogrp.hxx>
#include <svx/svdopath.hxx>
#include <tools/shl.hxx>
#include "svditer.hxx"
#include <svx/svdpool.hxx>
#include <svx/svdorect.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svxids.hrc>
#include <svx/colritem.hxx>
#include <svx/xtable.hxx>
#include <svx/svdview.hxx>
#include <svx/dialogs.hrc>
#include <svx/dialmgr.hxx>
#include "globl3d.hxx"
#include <svx/obj3d.hxx>
#include <svx/lathe3d.hxx>
#include <svx/sphere3d.hxx>
#include <svx/extrud3d.hxx>
#include <svx/cube3d.hxx>
#include <svx/polysc3d.hxx>
#include "dragmt3d.hxx"
#include <svx/view3d.hxx>
#include <svx/svdundo.hxx>
#include <svx/xflclit.hxx>
#include <svx/xlnclit.hxx>
#include <svx/svdograf.hxx>
#include <svx/xbtmpit.hxx>
#include <svx/xflbmtit.hxx>
#include <basegfx/range/b2drange.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <svx/xlnwtit.hxx>
#include <svx/sdr/overlay/overlaypolypolygon.hxx>
#include <svx/sdr/overlay/overlaymanager.hxx>
#include <sdrpaintwindow.hxx>
#include <svx/sdr/contact/viewcontactofe3dscene.hxx>
#include <drawinglayer/geometry/viewinformation3d.hxx>
#include <svx/sdrpagewindow.hxx>
#include <svx/sdr/contact/displayinfo.hxx>
#include <svx/sdr/contact/objectcontact.hxx>
#include <svx/sdr/contact/viewobjectcontact.hxx>
#include <drawinglayer/primitive2d/unifiedalphaprimitive2d.hxx>
#include <svx/sdr/overlay/overlayprimitive2dsequenceobject.hxx>
#include <drawinglayer/primitive2d/transformprimitive2d.hxx>

#define ITEMVALUE(ItemSet,Id,Cast)	((const Cast&)(ItemSet).Get(Id)).GetValue()

TYPEINIT1(E3dView, SdrView);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Migrate Marking

class Impl3DMirrorConstructOverlay
{
	// The OverlayObjects
	::sdr::overlay::OverlayObjectList				maObjects;

	// the view
	const E3dView&									mrView;

	// the object count
	sal_uInt32										mnCount;

	// the unmirrored polygons
	basegfx::B2DPolyPolygon*						mpPolygons;

    // the overlay geometry from selected objects
    drawinglayer::primitive2d::Primitive2DSequence  maFullOverlay;

public:
	Impl3DMirrorConstructOverlay(const E3dView& rView);
	~Impl3DMirrorConstructOverlay();

	void SetMirrorAxis(Point aMirrorAxisA, Point aMirrorAxisB);
};

Impl3DMirrorConstructOverlay::Impl3DMirrorConstructOverlay(const E3dView& rView)
:	maObjects(),
    mrView(rView),
    mnCount(rView.GetMarkedObjectCount()),
    mpPolygons(0),
    maFullOverlay()
{
    if(mnCount)
    {
        if(mrView.IsSolidDragging())
        {
	        SdrPageView* pPV = rView.GetSdrPageView();

	        if(pPV && pPV->PageWindowCount())
	        {
		        sdr::contact::ObjectContact& rOC = pPV->GetPageWindow(0)->GetObjectContact();
    	        sdr::contact::DisplayInfo aDisplayInfo;

                // Do not use the last ViewPort set at the OC at the last ProcessDisplay()
                rOC.resetViewPort();

		        for(sal_uInt32 a(0);a < mnCount;a++)
		        {
			        SdrObject* pObject = mrView.GetMarkedObjectByIndex(a);

			        if(pObject)
			        {
				        sdr::contact::ViewContact& rVC = pObject->GetViewContact();
				        sdr::contact::ViewObjectContact& rVOC = rVC.GetViewObjectContact(rOC);
    				    
                        const drawinglayer::primitive2d::Primitive2DSequence aNewSequence(rVOC.getPrimitive2DSequenceHierarchy(aDisplayInfo));
                        drawinglayer::primitive2d::appendPrimitive2DSequenceToPrimitive2DSequence(maFullOverlay, aNewSequence);
			        }
		        }
	        }
        }
        else
        {
	        mpPolygons = new basegfx::B2DPolyPolygon[mnCount];

	        for(sal_uInt32 a(0); a < mnCount; a++)
	        {
			    SdrObject* pObject = mrView.GetMarkedObjectByIndex(a);
		        mpPolygons[mnCount - (a + 1)] = pObject->TakeXorPoly();
	        }
        }
    }
}

Impl3DMirrorConstructOverlay::~Impl3DMirrorConstructOverlay()
{
	// The OverlayObjects are cleared using the destructor of OverlayObjectList.
	// That destructor calls clear() at the list which removes all objects from the
	// OverlayManager and deletes them.
    if(!mrView.IsSolidDragging())
    {
    	delete[] mpPolygons;
    }
}

void Impl3DMirrorConstructOverlay::SetMirrorAxis(Point aMirrorAxisA, Point aMirrorAxisB)
{
	// get rid of old overlay objects
	maObjects.clear();

	// create new ones
	for(sal_uInt32 a(0); a < mrView.PaintWindowCount(); a++)
	{
		SdrPaintWindow* pCandidate = mrView.GetPaintWindow(a);
		::sdr::overlay::OverlayManager* pTargetOverlay = pCandidate->GetOverlayManager();

		if(pTargetOverlay)
		{
	        // buld transfoprmation: translate and rotate so that given edge is 
            // on x axis, them mirror in y and translate back
	        const basegfx::B2DVector aEdge(aMirrorAxisB.X() - aMirrorAxisA.X(), aMirrorAxisB.Y() - aMirrorAxisA.Y());
	        basegfx::B2DHomMatrix aMatrixTransform;

	        aMatrixTransform.translate(-aMirrorAxisA.X(), -aMirrorAxisA.Y());
	        aMatrixTransform.rotate(-atan2(aEdge.getY(), aEdge.getX()));
	        aMatrixTransform.scale(1.0, -1.0);
	        aMatrixTransform.rotate(atan2(aEdge.getY(), aEdge.getX()));
	        aMatrixTransform.translate(aMirrorAxisA.X(), aMirrorAxisA.Y());

            if(mrView.IsSolidDragging())
            {
                if(maFullOverlay.hasElements())
                {
					drawinglayer::primitive2d::Primitive2DSequence aContent(maFullOverlay);

					if(!aMatrixTransform.isIdentity())
					{
						// embed in transformation group
						drawinglayer::primitive2d::Primitive2DReference aTransformPrimitive2D(new drawinglayer::primitive2d::TransformPrimitive2D(aMatrixTransform, aContent));
						aContent = drawinglayer::primitive2d::Primitive2DSequence(&aTransformPrimitive2D, 1);
					}

                    // if we have full overlay from selected objects, embed with 50% transparence, the
                    // transformation is added to the OverlayPrimitive2DSequenceObject
		            drawinglayer::primitive2d::Primitive2DReference aUnifiedAlphaPrimitive2D(new drawinglayer::primitive2d::UnifiedAlphaPrimitive2D(aContent, 0.5));
                    aContent = drawinglayer::primitive2d::Primitive2DSequence(&aUnifiedAlphaPrimitive2D, 1);

					sdr::overlay::OverlayPrimitive2DSequenceObject* pNew = new sdr::overlay::OverlayPrimitive2DSequenceObject(aContent);
            		
		            pTargetOverlay->add(*pNew);
		            maObjects.append(*pNew);
                }
            }
            else
            {
		        for(sal_uInt32 b(0); b < mnCount; b++)
		        {
			        // apply to polygon
			        basegfx::B2DPolyPolygon aPolyPolygon(mpPolygons[b]);
			        aPolyPolygon.transform(aMatrixTransform);

			        ::sdr::overlay::OverlayPolyPolygonStriped* pNew = new ::sdr::overlay::OverlayPolyPolygonStriped(aPolyPolygon);
			        pTargetOverlay->add(*pNew);
			        maObjects.append(*pNew);
                }
            }
		}
	}
}

/*************************************************************************
|*
|* Konstruktor 1
|*
\************************************************************************/

E3dView::E3dView(SdrModel* pModel, OutputDevice* pOut) :
    SdrView(pModel, pOut)
{
	InitView ();
}

/*************************************************************************
|*
|* DrawMarkedObj ueberladen, da eventuell nur einzelne 3D-Objekte
|* gezeichnet werden sollen
|*
\************************************************************************/

void E3dView::DrawMarkedObj(OutputDevice& rOut) const
{
	// Existieren 3D-Objekte, deren Szenen nicht selektiert sind?
	BOOL bSpecialHandling = FALSE;
	E3dScene *pScene = NULL;

	long nCnt = GetMarkedObjectCount();
	for(long nObjs = 0;nObjs < nCnt;nObjs++)
	{
		SdrObject *pObj = GetMarkedObjectByIndex(nObjs);
		if(pObj && pObj->ISA(E3dCompoundObject))
		{
			// zugehoerige Szene
			pScene = ((E3dCompoundObject*)pObj)->GetScene();
			if(pScene && !IsObjMarked(pScene))
				bSpecialHandling = TRUE;
		}
		// Alle SelectionFlags zuruecksetzen
		if(pObj && pObj->ISA(E3dObject))
		{
			pScene = ((E3dObject*)pObj)->GetScene();
			if(pScene)
				pScene->SetSelected(FALSE);
		}
	}

	if(bSpecialHandling)
	{
		// SelectionFlag bei allen zu 3D Objekten gehoerigen
		// Szenen und deren Objekten auf nicht selektiert setzen
		long nObjs;
		for(nObjs = 0;nObjs < nCnt;nObjs++)
		{
			SdrObject *pObj = GetMarkedObjectByIndex(nObjs);
			if(pObj && pObj->ISA(E3dCompoundObject))
			{
				// zugehoerige Szene
				pScene = ((E3dCompoundObject*)pObj)->GetScene();
				if(pScene)
					pScene->SetSelected(FALSE);
			}
		}

		// bei allen direkt selektierten Objekten auf selektiert setzen
		SdrMark* pM = NULL;

		for(nObjs = 0;nObjs < nCnt;nObjs++)
		{
			SdrObject *pObj = GetMarkedObjectByIndex(nObjs);
			if(pObj && pObj->ISA(E3dObject))
			{
				// Objekt markieren
				E3dObject* p3DObj = (E3dObject*)pObj;
				p3DObj->SetSelected(TRUE);
				pScene = p3DObj->GetScene();
				pM = GetSdrMarkByIndex(nObjs);
			}
		}

		if(pScene)
		{
			// code from parent
			SortMarkedObjects();

			pScene->SetDrawOnlySelected(TRUE);
			pScene->SingleObjectPainter(rOut); // #110094#-17
			pScene->SetDrawOnlySelected(FALSE);
		}

		// SelectionFlag zuruecksetzen
		for(nObjs = 0;nObjs < nCnt;nObjs++)
		{
			SdrObject *pObj = GetMarkedObjectByIndex(nObjs);
			if(pObj && pObj->ISA(E3dCompoundObject))
			{
				// zugehoerige Szene
				pScene = ((E3dCompoundObject*)pObj)->GetScene();
				if(pScene)
					pScene->SetSelected(FALSE);
			}
		}
	}
	else
	{
		// call parent
		SdrExchangeView::DrawMarkedObj(rOut);
	}
}

/*************************************************************************
|*
|* Model holen ueberladen, da bei einzelnen 3D Objekten noch eine Szene
|* untergeschoben werden muss
|*
\************************************************************************/

SdrModel* E3dView::GetMarkedObjModel() const
{
	// Existieren 3D-Objekte, deren Szenen nicht selektiert sind?
	bool bSpecialHandling(false);
	const sal_uInt32 nCount(GetMarkedObjectCount());
    sal_uInt32 nObjs(0);
	E3dScene *pScene = 0;

	for(nObjs = 0; nObjs < nCount; nObjs++)
	{
		const SdrObject* pObj = GetMarkedObjectByIndex(nObjs);

        if(!bSpecialHandling && pObj && pObj->ISA(E3dCompoundObject))
		{
			// if the object is selected, but it's scene not,
            // we need special handling
			pScene = ((E3dCompoundObject*)pObj)->GetScene();

			if(pScene && !IsObjMarked(pScene))
            {
				bSpecialHandling = true;
			}
		}

		if(pObj && pObj->ISA(E3dObject))
		{
            // reset all selection flags at 3D objects
			pScene = ((E3dObject*)pObj)->GetScene();
			
			if(pScene)
            {
				pScene->SetSelected(false);
            }
		}
	}

    if(!bSpecialHandling)
	{
		// call parent
		return SdrView::GetMarkedObjModel();
	}

	SdrModel* pNewModel = 0;
    Rectangle aSelectedSnapRect;

	// set 3d selection flags at all directly selected objects
    // and collect SnapRect of selected objects
	for(nObjs = 0; nObjs < nCount; nObjs++)
	{
		SdrObject *pObj = GetMarkedObjectByIndex(nObjs);

        if(pObj && pObj->ISA(E3dCompoundObject))
		{
			// mark object, but not scenes
			E3dCompoundObject* p3DObj = (E3dCompoundObject*)pObj;
			p3DObj->SetSelected(true);
            aSelectedSnapRect.Union(p3DObj->GetSnapRect());
		}
	}

	// create new mark list which contains all indirectly selected3d 
    // scenes as selected objects
    SdrMarkList aOldML(GetMarkedObjectList());
    SdrMarkList aNewML;
	SdrMarkList& rCurrentMarkList = ((E3dView*)this)->GetMarkedObjectListWriteAccess();
	rCurrentMarkList = aNewML;

	for(nObjs = 0; nObjs < nCount; nObjs++)
	{
		SdrObject *pObj = aOldML.GetMark(nObjs)->GetMarkedSdrObj();

		if(pObj && pObj->ISA(E3dObject))
		{
			pScene = ((E3dObject*)pObj)->GetScene();
			
            if(pScene && !IsObjMarked(pScene) && GetSdrPageView())
			{
				((E3dView*)this)->MarkObj(pScene, GetSdrPageView(), FALSE, TRUE);
			}
		}
	}

	// call parent. This will copy all scenes and the selection flags at the 3d objectss. So
    // it will be possible to delete all non-selected 3d objects from the cloned 3d scenes
	pNewModel = SdrView::GetMarkedObjModel();

	if(pNewModel)
	{
		for(sal_uInt16 nPg(0); nPg < pNewModel->GetPageCount(); nPg++)
		{
			const SdrPage* pSrcPg=pNewModel->GetPage(nPg);
			const sal_uInt32 nObAnz(pSrcPg->GetObjCount());

			for(sal_uInt32 nOb(0); nOb < nObAnz; nOb++)
			{
				const SdrObject* pSrcOb=pSrcPg->GetObj(nOb);

				if(pSrcOb->ISA(E3dScene))
				{
					pScene = (E3dScene*)pSrcOb;

                    // delete all not intentionally cloned 3d objects
                    pScene->removeAllNonSelectedObjects();

                    // reset select flags and set SnapRect of all selected objects
					pScene->SetSelected(false);
                    pScene->SetSnapRect(aSelectedSnapRect);
				}
			}
		}
	}

	// restore old selection
	rCurrentMarkList = aOldML;

	// model zurueckgeben
	return pNewModel;
}

/*************************************************************************
|*
|* Bei Paste muss - falls in eine Scene eingefuegt wird - die
|* Objekte der Szene eingefuegt werden, die Szene selbst aber nicht
|*
\************************************************************************/

BOOL E3dView::Paste(const SdrModel& rMod, const Point& rPos, SdrObjList* pLst, UINT32 nOptions)
{
	BOOL bRetval = FALSE;

	// Liste holen
    Point aPos(rPos);
	SdrObjList* pDstList = pLst;
    ImpGetPasteObjList(aPos, pDstList);
    
	if(!pDstList)
		return FALSE;

	// Owner der Liste holen
	SdrObject* pOwner = pDstList->GetOwnerObj();
	if(pOwner && pOwner->ISA(E3dScene))
	{
		E3dScene* pDstScene = (E3dScene*)pOwner;
	    BegUndo(SVX_RESSTR(RID_SVX_3D_UNDO_EXCHANGE_PASTE));

		// Alle Objekte aus E3dScenes kopieren und direkt einfuegen
	    for(sal_uInt16 nPg(0); nPg < rMod.GetPageCount(); nPg++)
		{
	        const SdrPage* pSrcPg=rMod.GetPage(nPg);
	        sal_uInt32 nObAnz(pSrcPg->GetObjCount());

			// calculate offset for paste
			Rectangle aR = pSrcPg->GetAllObjBoundRect();
			Point aDist(aPos - aR.Center());

			// Unterobjekte von Szenen einfuegen
			for(sal_uInt32 nOb(0); nOb < nObAnz; nOb++)
			{
				const SdrObject* pSrcOb = pSrcPg->GetObj(nOb);
				if(pSrcOb->ISA(E3dScene))
				{
					E3dScene* pSrcScene = (E3dScene*)pSrcOb;
					ImpCloneAll3DObjectsToDestScene(pSrcScene, pDstScene, aDist);
				}
			}
		}
		EndUndo();
	}
	else
	{
		// call parent
		bRetval = SdrView::Paste(rMod, rPos, pLst, nOptions);
	}

	// und Rueckgabewert liefern
	return bRetval;
}

// #83403# Service routine used from local Clone() and from SdrCreateView::EndCreateObj(...)
BOOL E3dView::ImpCloneAll3DObjectsToDestScene(E3dScene* pSrcScene, E3dScene* pDstScene, Point /*aOffset*/)
{
	BOOL bRetval(FALSE);

	if(pSrcScene && pDstScene)
	{
		const sdr::contact::ViewContactOfE3dScene& rVCSceneDst = static_cast< sdr::contact::ViewContactOfE3dScene& >(pDstScene->GetViewContact());
		const drawinglayer::geometry::ViewInformation3D aViewInfo3DDst(rVCSceneDst.getViewInformation3D());
		const sdr::contact::ViewContactOfE3dScene& rVCSceneSrc = static_cast< sdr::contact::ViewContactOfE3dScene& >(pSrcScene->GetViewContact());
		const drawinglayer::geometry::ViewInformation3D aViewInfo3DSrc(rVCSceneSrc.getViewInformation3D());

		for(sal_uInt32 i(0); i < pSrcScene->GetSubList()->GetObjCount(); i++)
		{
			E3dCompoundObject* pCompoundObj = dynamic_cast< E3dCompoundObject* >(pSrcScene->GetSubList()->GetObj(i));

			if(pCompoundObj)
			{
				// #116235#
				E3dCompoundObject* pNewCompoundObj = dynamic_cast< E3dCompoundObject* >(pCompoundObj->Clone());
				
				if(pNewCompoundObj)
				{
                    // get dest scene's current range in 3D world coordinates
                    const basegfx::B3DHomMatrix aSceneToWorldTrans(pDstScene->GetFullTransform());
                	basegfx::B3DRange aSceneRange(pDstScene->GetBoundVolume());
                    aSceneRange.transform(aSceneToWorldTrans);

                    // get new object's implied object transformation
                    const basegfx::B3DHomMatrix aNewObjectTrans(pNewCompoundObj->GetTransform());

                    // get new object's range in 3D world coordinates in dest scene
                    // as if it were already added
                    const basegfx::B3DHomMatrix aObjectToWorldTrans(aSceneToWorldTrans * aNewObjectTrans);
                    basegfx::B3DRange aObjectRange(pNewCompoundObj->GetBoundVolume());
                    aObjectRange.transform(aObjectToWorldTrans);

                    // get scale adaption
                    const basegfx::B3DVector aSceneScale(aSceneRange.getRange());
                    const basegfx::B3DVector aObjectScale(aObjectRange.getRange());
                    double fScale(1.0);

                    // if new object's size in X,Y or Z is bigger that 80% of dest scene, adapt scale
                    // to not change the scene by the inserted object
                    const double fSizeFactor(0.5);

                    if(aObjectScale.getX() * fScale > aSceneScale.getX() * fSizeFactor)
                    {
                        const double fObjSize(aObjectScale.getX() * fScale);
                        const double fFactor((aSceneScale.getX() * fSizeFactor) / (basegfx::fTools::equalZero(fObjSize) ? 1.0 : fObjSize));
                        fScale *= fFactor;
                    }
                    
                    if(aObjectScale.getY() * fScale > aSceneScale.getY() * fSizeFactor)
                    {
                        const double fObjSize(aObjectScale.getY() * fScale);
                        const double fFactor((aSceneScale.getY() * fSizeFactor) / (basegfx::fTools::equalZero(fObjSize) ? 1.0 : fObjSize));
                        fScale *= fFactor;
					}

                    if(aObjectScale.getZ() * fScale > aSceneScale.getZ() * fSizeFactor)
                    {
                        const double fObjSize(aObjectScale.getZ() * fScale);
                        const double fFactor((aSceneScale.getZ() * fSizeFactor) / (basegfx::fTools::equalZero(fObjSize) ? 1.0 : fObjSize));
                        fScale *= fFactor;
                    }

                    // get translation adaption
                    const basegfx::B3DPoint aSceneCenter(aSceneRange.getCenter());
            		const basegfx::B3DPoint aObjectCenter(aObjectRange.getCenter());

                    // build full modification transform. The object's transformation
                    // shall be modified, so start at object coordinates; transform to 3d world coor
                    basegfx::B3DHomMatrix aModifyingTransform(aObjectToWorldTrans);

                    // translate to absolute center in 3d world coor
                    aModifyingTransform.translate(-aObjectCenter.getX(), -aObjectCenter.getY(), -aObjectCenter.getZ());

                    // scale to dest size in 3d world coor
                    aModifyingTransform.scale(fScale, fScale, fScale);

                    // translate to dest scene center in 3d world coor
                    aModifyingTransform.translate(aSceneCenter.getX(), aSceneCenter.getY(), aSceneCenter.getZ());

                    // transform from 3d world to dest object coordinates
                    basegfx::B3DHomMatrix aWorldToObject(aObjectToWorldTrans);
                    aWorldToObject.invert();
                    aModifyingTransform = aWorldToObject * aModifyingTransform;

                    // correct implied object transform by applying changing one in object coor
                    pNewCompoundObj->SetTransform(aModifyingTransform * aNewObjectTrans);

					// fill and insert new object
					pNewCompoundObj->SetModel(pDstScene->GetModel());
					pNewCompoundObj->SetPage(pDstScene->GetPage());
					pNewCompoundObj->NbcSetLayer(pCompoundObj->GetLayer());
					pNewCompoundObj->NbcSetStyleSheet(pCompoundObj->GetStyleSheet(), sal_True);
					pDstScene->Insert3DObj(pNewCompoundObj);
					bRetval = TRUE;

					// Undo anlegen
					if( GetModel()->IsUndoEnabled() )
						AddUndo(GetModel()->GetSdrUndoFactory().CreateUndoNewObject(*pNewCompoundObj));
				}
			}
		}
	}

	return bRetval;
}

/*************************************************************************
|*
|* 3D-Konvertierung moeglich?
|*
\************************************************************************/

BOOL E3dView::IsConvertTo3DObjPossible() const
{
	BOOL bAny3D(FALSE);
	BOOL bGroupSelected(FALSE);
	BOOL bRetval(TRUE);

	for(sal_uInt32 a=0;!bAny3D && a<GetMarkedObjectCount();a++)
	{
		SdrObject *pObj = GetMarkedObjectByIndex(a);
		if(pObj)
		{
			ImpIsConvertTo3DPossible(pObj, bAny3D, bGroupSelected);
		}
	}

	bRetval = !bAny3D
		&& (
		   IsConvertToPolyObjPossible(FALSE)
		|| IsConvertToPathObjPossible(FALSE)
		|| IsImportMtfPossible());
	return bRetval;
}

void E3dView::ImpIsConvertTo3DPossible(SdrObject* pObj, BOOL& rAny3D,
	BOOL& rGroupSelected) const
{
	if(pObj)
	{
		if(pObj->ISA(E3dObject))
		{
			rAny3D = TRUE;
		}
		else
		{
			if(pObj->IsGroupObject())
			{
				SdrObjListIter aIter(*pObj, IM_DEEPNOGROUPS);
				while(aIter.IsMore())
				{
					SdrObject* pNewObj = aIter.Next();
					ImpIsConvertTo3DPossible(pNewObj, rAny3D, rGroupSelected);
				}
				rGroupSelected = TRUE;
			}
		}
	}
}

/*************************************************************************
|*
|* 3D-Konvertierung zu Extrude ausfuehren
|*
\************************************************************************/
#include <svx/eeitem.hxx>

void E3dView::ImpChangeSomeAttributesFor3DConversion(SdrObject* pObj)
{
	if(pObj->ISA(SdrTextObj))
	{
		const SfxItemSet& rSet = pObj->GetMergedItemSet();
		const SvxColorItem& rTextColorItem = (const SvxColorItem&)rSet.Get(EE_CHAR_COLOR);
		if(rTextColorItem.GetValue() == RGB_Color(COL_BLACK))
		{
			// Bei schwarzen Textobjekten wird die Farbe auf grau gesetzt
			if(pObj->GetPage())
			{
				// #84864# if black is only default attribute from
				// pattern set it hard so that it is used in undo.
				pObj->SetMergedItem(SvxColorItem(RGB_Color(COL_BLACK), EE_CHAR_COLOR));

				// add undo now
				if( GetModel()->IsUndoEnabled() )
					AddUndo(GetModel()->GetSdrUndoFactory().CreateUndoAttrObject(*pObj, false, false));
			}

			pObj->SetMergedItem(SvxColorItem(RGB_Color(COL_GRAY), EE_CHAR_COLOR));
		}
	}
}

void E3dView::ImpChangeSomeAttributesFor3DConversion2(SdrObject* pObj)
{
	if(pObj->ISA(SdrPathObj))
	{
		const SfxItemSet& rSet = pObj->GetMergedItemSet();
		sal_Int32 nLineWidth = ((const XLineWidthItem&)(rSet.Get(XATTR_LINEWIDTH))).GetValue();
		XLineStyle eLineStyle = (XLineStyle)((const XLineStyleItem&)rSet.Get(XATTR_LINESTYLE)).GetValue();
		XFillStyle eFillStyle = ITEMVALUE(rSet, XATTR_FILLSTYLE, XFillStyleItem);

		if(((SdrPathObj*)pObj)->IsClosed() 
			&& eLineStyle == XLINE_SOLID 
			&& !nLineWidth 
			&& eFillStyle != XFILL_NONE)
		{
			if(pObj->GetPage() && GetModel()->IsUndoEnabled() )
				AddUndo(GetModel()->GetSdrUndoFactory().CreateUndoAttrObject(*pObj, false, false));
			pObj->SetMergedItem(XLineStyleItem(XLINE_NONE));
			pObj->SetMergedItem(XLineWidthItem(0L));
		}
	}
}

void E3dView::ImpCreateSingle3DObjectFlat(E3dScene* pScene, SdrObject* pObj, BOOL bExtrude, double fDepth, basegfx::B2DHomMatrix& rLatheMat)
{
	// Einzelnes PathObject, dieses umwanden
	SdrPathObj* pPath = PTR_CAST(SdrPathObj, pObj);

	if(pPath)
	{
		E3dDefaultAttributes aDefault = Get3DDefaultAttributes();
		if(bExtrude)
			aDefault.SetDefaultExtrudeCharacterMode(TRUE);
		else
			aDefault.SetDefaultLatheCharacterMode(TRUE);

		// ItemSet des Ursprungsobjektes holen
		SfxItemSet aSet(pObj->GetMergedItemSet());

		XFillStyle eFillStyle = ITEMVALUE(aSet, XATTR_FILLSTYLE, XFillStyleItem);

		// Linienstil ausschalten
		aSet.Put(XLineStyleItem(XLINE_NONE));

		// Feststellen, ob ein FILL_Attribut gesetzt ist.
		if(!pPath->IsClosed() || eFillStyle == XFILL_NONE)
		{
			// Das SdrPathObj ist nicht gefuellt, lasse die
			// vordere und hintere Flaeche weg. Ausserdem ist
			// eine beidseitige Darstellung notwendig.
			aDefault.SetDefaultExtrudeCloseFront(FALSE);
			aDefault.SetDefaultExtrudeCloseBack(FALSE);

			aSet.Put(Svx3DDoubleSidedItem(TRUE));

			// Fuellattribut setzen
			aSet.Put(XFillStyleItem(XFILL_SOLID));

			// Fuellfarbe muss auf Linienfarbe, da das Objekt vorher
			// nur eine Linie war
			Color aColorLine = ((const XLineColorItem&)(aSet.Get(XATTR_LINECOLOR))).GetColorValue();
			aSet.Put(XFillColorItem(String(), aColorLine));
		}

		// Neues Extrude-Objekt erzeugen
		E3dObject* p3DObj = NULL;
		if(bExtrude)
		{
			p3DObj = new E3dExtrudeObj(aDefault, pPath->GetPathPoly(), fDepth);
		}
		else
		{
			basegfx::B2DPolyPolygon aPolyPoly2D(pPath->GetPathPoly());
			aPolyPoly2D.transform(rLatheMat);
			p3DObj = new E3dLatheObj(aDefault, aPolyPoly2D);
		}

		// Attribute setzen
		if(p3DObj)
		{
			p3DObj->NbcSetLayer(pObj->GetLayer());

			p3DObj->SetMergedItemSet(aSet);
			
			p3DObj->NbcSetStyleSheet(pObj->GetStyleSheet(), sal_True);

			// Neues 3D-Objekt einfuegen
			pScene->Insert3DObj(p3DObj);
		}
	}
}

void E3dView::ImpCreate3DObject(E3dScene* pScene, SdrObject* pObj, BOOL bExtrude, double fDepth, basegfx::B2DHomMatrix& rLatheMat)
{
	if(pObj)
	{
		// change text color attribute for not so dark colors
		if(pObj->IsGroupObject())
		{
			SdrObjListIter aIter(*pObj, IM_DEEPWITHGROUPS);
			while(aIter.IsMore())
			{
				SdrObject* pGroupMember = aIter.Next();
				ImpChangeSomeAttributesFor3DConversion(pGroupMember);
			}
		}
		else
			ImpChangeSomeAttributesFor3DConversion(pObj);
		
		// convert completely to path objects
		SdrObject* pNewObj1 = pObj->ConvertToPolyObj(FALSE, FALSE);

		if(pNewObj1)
		{
			// change text color attribute for not so dark colors
			if(pNewObj1->IsGroupObject())
			{
				SdrObjListIter aIter(*pNewObj1, IM_DEEPWITHGROUPS);
				while(aIter.IsMore())
				{
					SdrObject* pGroupMember = aIter.Next();
					ImpChangeSomeAttributesFor3DConversion2(pGroupMember);
				}
			}
			else
				ImpChangeSomeAttributesFor3DConversion2(pNewObj1);
			
			// convert completely to path objects
			SdrObject* pNewObj2 = pObj->ConvertToContourObj(pNewObj1, TRUE);

			if(pNewObj2)
			{
				// add all to flat scene
				if(pNewObj2->IsGroupObject())
				{
					SdrObjListIter aIter(*pNewObj2, IM_DEEPWITHGROUPS);
					while(aIter.IsMore())
					{
						SdrObject* pGroupMember = aIter.Next();
						ImpCreateSingle3DObjectFlat(pScene, pGroupMember, bExtrude, fDepth, rLatheMat);
					}
				}
				else
					ImpCreateSingle3DObjectFlat(pScene, pNewObj2, bExtrude, fDepth, rLatheMat);

				// delete zwi object
				if(pNewObj2 != pObj && pNewObj2 != pNewObj1 && pNewObj2)
                    SdrObject::Free( pNewObj2 );
			}

			// delete zwi object
			if(pNewObj1 != pObj && pNewObj1)
				SdrObject::Free( pNewObj1 );
		}
	}
}

/*************************************************************************
|*
|* 3D-Konvertierung zu Extrude steuern
|*
\************************************************************************/

void E3dView::ConvertMarkedObjTo3D(BOOL bExtrude, basegfx::B2DPoint aPnt1, basegfx::B2DPoint aPnt2)
{
	if(AreObjectsMarked())
	{
		// Undo anlegen
        if(bExtrude)
			BegUndo(SVX_RESSTR(RID_SVX_3D_UNDO_EXTRUDE));
		else
			BegUndo(SVX_RESSTR(RID_SVX_3D_UNDO_LATHE));

		// Neue Szene fuer zu erzeugende 3D-Objekte anlegen
        E3dScene* pScene = new E3dPolyScene(Get3DDefaultAttributes());

		// Rechteck bestimmen und evtl. korrigieren
		Rectangle aRect = GetAllMarkedRect();
		if(aRect.GetWidth() <= 1)
			aRect.SetSize(Size(500, aRect.GetHeight()));
		if(aRect.GetHeight() <= 1)
			aRect.SetSize(Size(aRect.GetWidth(), 500));

		// Tiefe relativ zur Groesse der Selektion bestimmen
		double fDepth = 0.0;
		double fRot3D = 0.0;
		basegfx::B2DHomMatrix aLatheMat;

		if(bExtrude)
		{
			double fW = (double)aRect.GetWidth();
			double fH = (double)aRect.GetHeight();
			fDepth = sqrt(fW*fW + fH*fH) / 6.0;
		}
		if(!bExtrude)
		{
			// Transformation fuer Polygone Rotationskoerper erstellen
			if(aPnt1 != aPnt2)
			{
				// Rotation um Kontrollpunkt1 mit eigestelltem Winkel
				// fuer 3D Koordinaten
				basegfx::B2DPoint aDiff(aPnt1 - aPnt2);
				fRot3D = atan2(aDiff.getY(), aDiff.getX()) - F_PI2;

                if(basegfx::fTools::equalZero(fabs(fRot3D)))
					fRot3D = 0.0;

				if(fRot3D != 0.0)
				{
					aLatheMat.translate(-aPnt2.getX(), -aPnt2.getY());
					aLatheMat.rotate(-fRot3D);
					aLatheMat.translate(aPnt2.getX(), aPnt2.getY());
				}
			}

			if(aPnt2.getX() != 0.0)
			{
				// Translation auf Y=0 - Achse
				aLatheMat.translate(-aPnt2.getX(), 0.0);
			}
			else
			{
				aLatheMat.translate((double)-aRect.Left(), 0.0);
			}

			// Inverse Matrix bilden, um die Zielausdehnung zu bestimmen
			basegfx::B2DHomMatrix aInvLatheMat(aLatheMat);
			aInvLatheMat.invert();

			// SnapRect Ausdehnung mittels Spiegelung an der Rotationsachse
			// erweitern
			for(UINT32 a=0;a<GetMarkedObjectCount();a++)
			{
				SdrMark* pMark = GetSdrMarkByIndex(a);
				SdrObject* pObj = pMark->GetMarkedSdrObj();
				Rectangle aTurnRect = pObj->GetSnapRect();
				basegfx::B2DPoint aRot;
				Point aRotPnt;

				aRot = basegfx::B2DPoint(aTurnRect.Left(), -aTurnRect.Top());
				aRot *= aLatheMat;
				aRot.setX(-aRot.getX());
				aRot *= aInvLatheMat;
				aRotPnt = Point((long)(aRot.getX() + 0.5), (long)(-aRot.getY() - 0.5));
				aRect.Union(Rectangle(aRotPnt, aRotPnt));

				aRot = basegfx::B2DPoint(aTurnRect.Left(), -aTurnRect.Bottom());
				aRot *= aLatheMat;
				aRot.setX(-aRot.getX());
				aRot *= aInvLatheMat;
				aRotPnt = Point((long)(aRot.getX() + 0.5), (long)(-aRot.getY() - 0.5));
				aRect.Union(Rectangle(aRotPnt, aRotPnt));

				aRot = basegfx::B2DPoint(aTurnRect.Right(), -aTurnRect.Top());
				aRot *= aLatheMat;
				aRot.setX(-aRot.getX());
				aRot *= aInvLatheMat;
				aRotPnt = Point((long)(aRot.getX() + 0.5), (long)(-aRot.getY() - 0.5));
				aRect.Union(Rectangle(aRotPnt, aRotPnt));

				aRot = basegfx::B2DPoint(aTurnRect.Right(), -aTurnRect.Bottom());
				aRot *= aLatheMat;
				aRot.setX(-aRot.getX());
				aRot *= aInvLatheMat;
				aRotPnt = Point((long)(aRot.getX() + 0.5), (long)(-aRot.getY() - 0.5));
				aRect.Union(Rectangle(aRotPnt, aRotPnt));
			}
		}

		// Ueber die Selektion gehen und in 3D wandeln, komplett mit
		// Umwandeln in SdrPathObject, auch Schriften
		for(UINT32 a=0;a<GetMarkedObjectCount();a++)
		{
			SdrMark* pMark = GetSdrMarkByIndex(a);
			SdrObject* pObj = pMark->GetMarkedSdrObj();

			ImpCreate3DObject(pScene, pObj, bExtrude, fDepth, aLatheMat);
		}

		if(pScene->GetSubList() && pScene->GetSubList()->GetObjCount() != 0)
		{
			// Alle angelegten Objekte Tiefenarrangieren
			if(bExtrude)
				DoDepthArrange(pScene, fDepth);

			// 3D-Objekte auf die Mitte des Gesamtrechtecks zentrieren
			basegfx::B3DPoint aCenter(pScene->GetBoundVolume().getCenter());
			basegfx::B3DHomMatrix aMatrix;

            aMatrix.translate(-aCenter.getX(), -aCenter.getY(), -aCenter.getZ());
			pScene->SetTransform(aMatrix * pScene->GetTransform()); // #112587#

			// Szene initialisieren
			pScene->NbcSetSnapRect(aRect);
			basegfx::B3DRange aBoundVol = pScene->GetBoundVolume();
			InitScene(pScene, (double)aRect.GetWidth(), (double)aRect.GetHeight(), aBoundVol.getDepth());

			// Szene anstelle des ersten selektierten Objektes einfuegen
			// und alle alten Objekte weghauen
			SdrObject* pRepObj = GetMarkedObjectByIndex(0);
			SdrPageView* pPV = GetSdrPageViewOfMarkedByIndex(0);
			MarkObj(pRepObj, pPV, TRUE);
			ReplaceObjectAtView(pRepObj, *pPV, pScene, FALSE);
			DeleteMarked();
			MarkObj(pScene, pPV);

			// Rotationskoerper um Rotationsachse drehen
			basegfx::B3DHomMatrix aRotate;

			if(!bExtrude && fRot3D != 0.0)
			{
				aRotate.rotate(0.0, 0.0, fRot3D);
			}

			// Default-Rotation setzen
			{
	            double XRotateDefault = 20;
				aRotate.rotate(DEG2RAD(XRotateDefault), 0.0, 0.0);
			}

			if(!aRotate.isIdentity())
			{
				pScene->SetTransform(aRotate * pScene->GetTransform());
			}

			// SnapRects der Objekte ungueltig
			pScene->SetSnapRect(aRect);
		}
		else
        {
			// Es wurden keine 3D Objekte erzeugt, schmeiss alles weg
			delete pScene;
        }

		// Undo abschliessen
        EndUndo();
	}
}

/*************************************************************************
|*
|* Alle enthaltenen Extrude-Objekte Tiefenarrangieren
|*
\************************************************************************/

struct E3dDepthNeighbour
{
	E3dDepthNeighbour*	pNext;
	E3dExtrudeObj*		pObj;

	E3dDepthNeighbour() { pNext = NULL; pObj = NULL; }
};

struct E3dDepthLayer
{
	E3dDepthLayer*		pDown;
	E3dDepthNeighbour*	pNext;

	E3dDepthLayer() { pDown = NULL; pNext = NULL; }
	~E3dDepthLayer() { while(pNext) { E3dDepthNeighbour* pSucc = pNext->pNext; delete pNext; pNext = pSucc; }}
};

bool ImpDoesOverlap(const basegfx::B2DPolygon& rPolygonA, const basegfx::B2DPolygon& rPolygonB)
{
	bool bRetval(false);
	const basegfx::B2DRange aRangeA(basegfx::tools::getRange(rPolygonA));
	const basegfx::B2DRange aRangeB(basegfx::tools::getRange(rPolygonB));
	
	if(aRangeA.overlaps(aRangeB))
	{
		// A in B ?
		if(basegfx::tools::isInside(rPolygonA, rPolygonB))
			return true;

		// B in A ?
		if(basegfx::tools::isInside(rPolygonB, rPolygonA))
			return true;

		// A and B the same ?
		if(basegfx::tools::isInside(rPolygonB, rPolygonA, true))
			return true;
	}

	return bRetval;
}

bool ImpDoesOverlap(const basegfx::B2DPolyPolygon& rPolyPolygonA, const basegfx::B2DPolyPolygon& rPolyPolygonB)
{
	bool bRetval(false);
	const basegfx::B2DRange aRangeA(basegfx::tools::getRange(rPolyPolygonA));
	const basegfx::B2DRange aRangeB(basegfx::tools::getRange(rPolyPolygonB));
	
	if(aRangeA.overlaps(aRangeB))
	{
		const sal_uInt32 nCntA(rPolyPolygonA.count());
		const sal_uInt32 nCntB(rPolyPolygonB.count());
		
		for(sal_uInt32 a(0L); !bRetval && a < nCntA; a++)
		{
			const basegfx::B2DPolygon aPolygonA(rPolyPolygonA.getB2DPolygon(a));
			
			if(aPolygonA.isClosed())
			{
				for(sal_uInt32 b(0L); !bRetval && b < nCntB; b++)
				{
					const basegfx::B2DPolygon aPolygonB(rPolyPolygonB.getB2DPolygon(b));

					if(aPolygonB.isClosed())
					{
						bRetval = ImpDoesOverlap(aPolygonA, aPolygonB);
					}
				}
			}
		}
	}

	return bRetval;
}

void E3dView::DoDepthArrange(E3dScene* pScene, double fDepth)
{
	if(pScene && pScene->GetSubList() && pScene->GetSubList()->GetObjCount() > 1)
	{
		SdrObjList* pSubList = pScene->GetSubList();
		SdrObjListIter aIter(*pSubList, IM_FLAT);
		E3dDepthLayer* pBaseLayer = NULL;
		E3dDepthLayer* pLayer = NULL;
		INT32 nNumLayers = 0;
		//SfxItemPool& rPool = pMod->GetItemPool();

		while(aIter.IsMore())
		{
			E3dObject* pSubObj = (E3dObject*)aIter.Next();

			if(pSubObj && pSubObj->ISA(E3dExtrudeObj))
			{
				E3dExtrudeObj* pExtrudeObj = (E3dExtrudeObj*)pSubObj;
				const basegfx::B2DPolyPolygon aExtrudePoly(pExtrudeObj->GetExtrudePolygon());

				const SfxItemSet& rLocalSet = pExtrudeObj->GetMergedItemSet();
				XFillStyle eLocalFillStyle = ITEMVALUE(rLocalSet, XATTR_FILLSTYLE, XFillStyleItem);
				Color aLocalColor = ((const XFillColorItem&)(rLocalSet.Get(XATTR_FILLCOLOR))).GetColorValue();

				// ExtrudeObj einordnen
				if(pLayer)
				{
					// Gibt es eine Ueberschneidung mit einem Objekt dieses
					// Layers?
					BOOL bOverlap(FALSE);
					E3dDepthNeighbour* pAct = pLayer->pNext;

					while(!bOverlap && pAct)
					{
						// ueberlappen sich pAct->pObj und pExtrudeObj ?
						const basegfx::B2DPolyPolygon aActPoly(pAct->pObj->GetExtrudePolygon());
						bOverlap = ImpDoesOverlap(aExtrudePoly, aActPoly);

						if(bOverlap)
						{
							// second ciriteria: is another fillstyle or color used?
							const SfxItemSet& rCompareSet = pAct->pObj->GetMergedItemSet();
							
							XFillStyle eCompareFillStyle = ITEMVALUE(rCompareSet, XATTR_FILLSTYLE, XFillStyleItem);

							if(eLocalFillStyle == eCompareFillStyle)
							{				  
								if(eLocalFillStyle == XFILL_SOLID)
								{
									Color aCompareColor = ((const XFillColorItem&)(rCompareSet.Get(XATTR_FILLCOLOR))).GetColorValue();

									if(aCompareColor == aLocalColor)
									{
										bOverlap = FALSE;
									}
								}
								else if(eLocalFillStyle == XFILL_NONE)
								{
									bOverlap = FALSE;
								}
							}
						}

						pAct = pAct->pNext;
					}

					if(bOverlap)
					{
						// ja, beginne einen neuen Layer
						pLayer->pDown = new E3dDepthLayer;
						pLayer = pLayer->pDown;
						nNumLayers++;
						pLayer->pNext = new E3dDepthNeighbour;
						pLayer->pNext->pObj = pExtrudeObj;
					}
					else
					{
						// nein, Objekt kann in aktuellen Layer
						E3dDepthNeighbour* pNewNext = new E3dDepthNeighbour;
						pNewNext->pObj = pExtrudeObj;
						pNewNext->pNext = pLayer->pNext;
						pLayer->pNext = pNewNext;
					}
				}
				else
				{
					// erster Layer ueberhaupt
					pBaseLayer = new E3dDepthLayer;
					pLayer = pBaseLayer;
					nNumLayers++;
					pLayer->pNext = new E3dDepthNeighbour;
					pLayer->pNext->pObj = pExtrudeObj;
				}
			}
		}

		// Anzahl Layer steht fest
		if(nNumLayers > 1)
		{
			// Arrangement ist notwendig
			double fMinDepth = fDepth * 0.8;
			double fStep = (fDepth - fMinDepth) / (double)nNumLayers;
			pLayer = pBaseLayer;

			while(pLayer)
			{
				// an pLayer entlangspazieren
				E3dDepthNeighbour* pAct = pLayer->pNext;

				while(pAct)
				{
					// Anpassen
					pAct->pObj->SetMergedItem(SfxUInt32Item(SDRATTR_3DOBJ_DEPTH, sal_uInt32(fMinDepth + 0.5)));

					// Naechster Eintrag
					pAct = pAct->pNext;
				}

				// naechster Layer
				pLayer = pLayer->pDown;
				fMinDepth += fStep;
			}
		}

		// angelegte Strukturen aufraeumen
		while(pBaseLayer)
		{
			pLayer = pBaseLayer->pDown;
			delete pBaseLayer;
			pBaseLayer = pLayer;
		}
	}
}

/*************************************************************************
|*
|* Drag beginnen, vorher ggf. Drag-Methode fuer 3D-Objekte erzeugen
|*
\************************************************************************/

BOOL E3dView::BegDragObj(const Point& rPnt, OutputDevice* pOut,
	SdrHdl* pHdl, short nMinMov,
	SdrDragMethod* pForcedMeth)
{
    if(Is3DRotationCreationActive() && GetMarkedObjectCount())
	{
		// bestimme alle selektierten Polygone und gebe die gespiegelte Hilfsfigur aus
		mpMirrorOverlay->SetMirrorAxis(aRef1, aRef2);
	}
	else
    {
        BOOL bOwnActionNecessary;
        if (pHdl == NULL)
        {
           bOwnActionNecessary = TRUE;
        }
        else if (pHdl->IsVertexHdl() || pHdl->IsCornerHdl())
        {
           bOwnActionNecessary = TRUE;
        }
        else
        {
           bOwnActionNecessary = FALSE;
        }

        if(bOwnActionNecessary && GetMarkedObjectCount() >= 1)
        {
            E3dDragConstraint eConstraint = E3DDRAG_CONSTR_XYZ;
			BOOL bThereAreRootScenes = FALSE;
			BOOL bThereAre3DObjects = FALSE;
			long nCnt = GetMarkedObjectCount();
			for(long nObjs = 0;nObjs < nCnt;nObjs++)
			{
				SdrObject *pObj = GetMarkedObjectByIndex(nObjs);
				if(pObj)
				{
					if(pObj->ISA(E3dScene) && ((E3dScene*)pObj)->GetScene() == pObj)
						bThereAreRootScenes = TRUE;
					if(pObj->ISA(E3dObject))
						bThereAre3DObjects = TRUE;
				}
			}
			if( bThereAre3DObjects )
			{
                eDragHdl = ( pHdl == NULL ? HDL_MOVE : pHdl->GetKind() );
                switch ( eDragMode )
                {
                    case SDRDRAG_ROTATE:
                    case SDRDRAG_SHEAR:
                    {
                        switch ( eDragHdl )
                        {
                            case HDL_LEFT:
                            case HDL_RIGHT:
                            {
                                eConstraint = E3DDRAG_CONSTR_X;
                            }
                            break;

                            case HDL_UPPER:
                            case HDL_LOWER:
                            {
                                eConstraint = E3DDRAG_CONSTR_Y;
                            }
                            break;

                            case HDL_UPLFT:
                            case HDL_UPRGT:
                            case HDL_LWLFT:
                            case HDL_LWRGT:
                            {
                                eConstraint = E3DDRAG_CONSTR_Z;
                            }
                            break;
							default: break;
                        }

                        // die nicht erlaubten Rotationen ausmaskieren
                        eConstraint = E3dDragConstraint(eConstraint& eDragConstraint);
                        pForcedMeth = new E3dDragRotate(*this, GetMarkedObjectList(), eConstraint, IsSolidDragging());
                    }
                    break;

                    case SDRDRAG_MOVE:
                    {
                        if(!bThereAreRootScenes)
						{
							pForcedMeth = new E3dDragMove(*this, GetMarkedObjectList(), eDragHdl, eConstraint, IsSolidDragging());
						}
                    }
                    break;

                    // spaeter mal
                    case SDRDRAG_MIRROR:
                    case SDRDRAG_CROOK:
                    case SDRDRAG_DISTORT:
                    case SDRDRAG_TRANSPARENCE:
                    case SDRDRAG_GRADIENT:
                    default:
                    {
                    }
                    break;
                }
			}
        }
    }
    return SdrView::BegDragObj(rPnt, pOut, pHdl, nMinMov, pForcedMeth);
}

/*************************************************************************
|*
|* Pruefen, obj 3D-Szene markiert ist
|*
\************************************************************************/

BOOL E3dView::HasMarkedScene()
{
	return (GetMarkedScene() != NULL);
}

/*************************************************************************
|*
|* Pruefen, obj 3D-Szene markiert ist
|*
\************************************************************************/

E3dScene* E3dView::GetMarkedScene()
{
	ULONG nCnt = GetMarkedObjectCount();

	for ( ULONG i = 0; i < nCnt; i++ )
		if ( GetMarkedObjectByIndex(i)->ISA(E3dScene) )
			return (E3dScene*) GetMarkedObjectByIndex(i);

	return NULL;
}

/*************************************************************************
|*
|* aktuelles 3D-Zeichenobjekt setzen, dafuer Szene erzeugen
|*
\************************************************************************/

E3dScene* E3dView::SetCurrent3DObj(E3dObject* p3DObj)
{
	DBG_ASSERT(p3DObj != NULL, "Nana, wer steckt denn hier 'nen NULL-Zeiger rein?");
	E3dScene* pScene = NULL;

	// get transformed BoundVolume of the object
	basegfx::B3DRange aVolume(p3DObj->GetBoundVolume());
	aVolume.transform(p3DObj->GetTransform());
	double fW(aVolume.getWidth());
	double fH(aVolume.getHeight());
	
	Rectangle aRect(0,0, (long) fW, (long) fH);

	pScene = new E3dPolyScene(Get3DDefaultAttributes());

	InitScene(pScene, fW, fH, aVolume.getMaxZ() + ((fW + fH) / 4.0));

	pScene->Insert3DObj(p3DObj);
	pScene->NbcSetSnapRect(aRect);

	return pScene;
}

/*************************************************************************
|*
|* neu erzeugte Szene initialisieren
|*
\************************************************************************/

void E3dView::InitScene(E3dScene* pScene, double fW, double fH, double fCamZ)
{
	Camera3D aCam(pScene->GetCamera());

	aCam.SetAutoAdjustProjection(FALSE);
	aCam.SetViewWindow(- fW / 2, - fH / 2, fW, fH);
	basegfx::B3DPoint aLookAt;

	double fDefaultCamPosZ = GetDefaultCamPosZ();
	basegfx::B3DPoint aCamPos(0.0, 0.0, fCamZ < fDefaultCamPosZ ? fDefaultCamPosZ : fCamZ);

	aCam.SetPosAndLookAt(aCamPos, aLookAt);
	aCam.SetFocalLength(GetDefaultCamFocal());
	aCam.SetDefaults(basegfx::B3DPoint(0.0, 0.0, fDefaultCamPosZ), aLookAt, GetDefaultCamFocal());
	pScene->SetCamera(aCam);
}

/*************************************************************************
|*
|* startsequenz fuer die erstellung eines 3D-Rotationskoerpers
|*
\************************************************************************/

void E3dView::Start3DCreation()
{
	if (GetMarkedObjectCount())
	{
		// irgendwelche Markierungen ermitteln und ausschalten
		//HMHBOOL bVis = IsMarkHdlShown();

		//HMHif (bVis) HideMarkHdl();

		// bestimme die koordinaten fuer JOEs Mirrorachse
		// entgegen der normalen Achse wird diese an die linke Seite des Objektes
		// positioniert
		long		  nOutMin = 0;
		long		  nOutMax = 0;
		long		  nMinLen = 0;
		long		  nObjDst = 0;
		long		  nOutHgt = 0;
		OutputDevice* pOut	  = GetFirstOutputDevice(); //GetWin(0);

		// erstmal Darstellungsgrenzen bestimmen
		if (pOut != NULL)
		{
			nMinLen = pOut->PixelToLogic(Size(0,50)).Height();
			nObjDst = pOut->PixelToLogic(Size(0,20)).Height();

			long nDst = pOut->PixelToLogic(Size(0,10)).Height();

			nOutMin =  -pOut->GetMapMode().GetOrigin().Y();
			nOutMax =  pOut->GetOutputSize().Height() - 1 + nOutMin;
			nOutMin += nDst;
			nOutMax -= nDst;

			if (nOutMax - nOutMin < nDst)
			{
				nOutMin += nOutMax + 1;
				nOutMin /= 2;
				nOutMin -= (nDst + 1) / 2;
				nOutMax  = nOutMin + nDst;
			}

			nOutHgt = nOutMax - nOutMin;

			long nTemp = nOutHgt / 4;
			if (nTemp > nMinLen) nMinLen = nTemp;
		}

		// und dann die Markierungen oben und unten an das Objekt heften
		basegfx::B2DRange aR;
		for(sal_uInt32 nMark(0L); nMark < GetMarkedObjectCount(); nMark++)
		{
			SdrObject* pMark = GetMarkedObjectByIndex(nMark);
			basegfx::B2DPolyPolygon aXPP(pMark->TakeXorPoly());
			aR.expand(basegfx::tools::getRange(aXPP));
		}

		basegfx::B2DPoint aCenter(aR.getCenter());
        long	  nMarkHgt = basegfx::fround(aR.getHeight()) - 1;
		long	  nHgt	   = nMarkHgt + nObjDst * 2;

		if (nHgt < nMinLen) nHgt = nMinLen;

		long nY1 = basegfx::fround(aCenter.getY()) - (nHgt + 1) / 2;
		long nY2 = nY1 + nHgt;

		if (pOut && (nMinLen > nOutHgt)) nMinLen = nOutHgt;
		if (pOut)
		{
			if (nY1 < nOutMin)
			{
				nY1 = nOutMin;
				if (nY2 < nY1 + nMinLen) nY2 = nY1 + nMinLen;
			}
			if (nY2 > nOutMax)
			{
				nY2 = nOutMax;
				if (nY1 > nY2 - nMinLen) nY1 = nY2 - nMinLen;
			}
		}

        aRef1.X() = basegfx::fround(aR.getMinX());    // Initial Achse um 2/100mm nach links
		aRef1.Y() = nY1;
        aRef2.X() = aRef1.X();
		aRef2.Y() = nY2;

		// Markierungen einschalten
		SetMarkHandles();

		//HMHif (bVis) ShowMarkHdl();
		if (AreObjectsMarked()) MarkListHasChanged();

		// SpiegelPolygone SOFORT zeigen
		const SdrHdlList &aHdlList = GetHdlList();
		mpMirrorOverlay = new Impl3DMirrorConstructOverlay(*this);
		mpMirrorOverlay->SetMirrorAxis(aHdlList.GetHdl(HDL_REF1)->GetPos(), aHdlList.GetHdl(HDL_REF2)->GetPos());
		//CreateMirrorPolygons ();
		//ShowMirrorPolygons (aHdlList.GetHdl (HDL_REF1)->GetPos (),
		//					aHdlList.GetHdl (HDL_REF2)->GetPos ());
	}
}

/*************************************************************************
|*
|* was passiert bei einer Mausbewegung, wenn das Objekt erstellt wird ?
|*
\************************************************************************/

void E3dView::MovAction(const Point& rPnt)
{
    if(Is3DRotationCreationActive())
	{
		SdrHdl* pHdl = GetDragHdl();

		if (pHdl)
		{
			SdrHdlKind eHdlKind = pHdl->GetKind();

			// reagiere nur bei einer spiegelachse
			if ((eHdlKind == HDL_REF1) ||
				(eHdlKind == HDL_REF2) ||
				(eHdlKind == HDL_MIRX))
			{
				const SdrHdlList &aHdlList = GetHdlList ();

				// loesche das gespiegelte Polygon, spiegele das Original und zeichne es neu
                //ShowMirrored ();
                SdrView::MovAction (rPnt);
				mpMirrorOverlay->SetMirrorAxis(
					aHdlList.GetHdl (HDL_REF1)->GetPos(),
					aHdlList.GetHdl (HDL_REF2)->GetPos());
            }
		}
        else
        {
            SdrView::MovAction (rPnt);
        }
	}
    else
    {
        SdrView::MovAction (rPnt);
    }
}

/*************************************************************************
|*
|* Schluss. Objekt und evtl. Unterobjekte ueber ImpCreate3DLathe erstellen
|*          [FG] Mit dem Parameterwert TRUE (SDefault: FALSE) wird einfach ein
|*               Rotationskoerper erzeugt, ohne den Benutzer die Lage der
|*               Achse fetlegen zu lassen. Es reicht dieser Aufruf, falls
|*               ein Objekt selektiert ist. (keine Initialisierung noetig)
|*
\************************************************************************/

void E3dView::End3DCreation(BOOL bUseDefaultValuesForMirrorAxes)
{
	ResetCreationActive();

	if(AreObjectsMarked())
	{
		if(bUseDefaultValuesForMirrorAxes)
		{
			Rectangle aRect = GetAllMarkedRect();
			if(aRect.GetWidth() <= 1)
				aRect.SetSize(Size(500, aRect.GetHeight()));
			if(aRect.GetHeight() <= 1)
				aRect.SetSize(Size(aRect.GetWidth(), 500));

			basegfx::B2DPoint aPnt1(aRect.Left(), -aRect.Top());
			basegfx::B2DPoint aPnt2(aRect.Left(), -aRect.Bottom());

			ConvertMarkedObjTo3D(FALSE, aPnt1, aPnt2);
		}
		else
		{
			// Hilfsfigur ausschalten
		    // bestimme aus den Handlepositionen und den Versatz der Punkte
            const SdrHdlList &aHdlList = GetHdlList();
    		Point aMirrorRef1 = aHdlList.GetHdl(HDL_REF1)->GetPos();
	    	Point aMirrorRef2 = aHdlList.GetHdl(HDL_REF2)->GetPos();

			basegfx::B2DPoint aPnt1(aMirrorRef1.X(), -aMirrorRef1.Y());
			basegfx::B2DPoint aPnt2(aMirrorRef2.X(), -aMirrorRef2.Y());

			ConvertMarkedObjTo3D(FALSE, aPnt1, aPnt2);
		}
	}
}

/*************************************************************************
|*
|* Destruktor
|*
\************************************************************************/

E3dView::~E3dView ()
{
}

/*************************************************************************
|*
|* beende das erzeugen und loesche die polygone
|*
\************************************************************************/

void E3dView::ResetCreationActive ()
{
	if(mpMirrorOverlay)
	{
		delete mpMirrorOverlay;
		mpMirrorOverlay = 0L;
	}
}

/*************************************************************************
|*
|* Klasse initialisieren
|*
\************************************************************************/

void E3dView::InitView ()
{
	eDragConstraint 		 = E3DDRAG_CONSTR_XYZ;
	fDefaultScaleX			 =
	fDefaultScaleY			 =
	fDefaultScaleZ			 = 1.0;
	fDefaultRotateX 		 =
	fDefaultRotateY 		 =
	fDefaultRotateZ 		 = 0.0;
	fDefaultExtrusionDeepth  = 1000; // old: 2000;
	fDefaultLightIntensity	 = 0.8; // old: 0.6;
	fDefaultAmbientIntensity = 0.4;
    nHDefaultSegments        = 12;
    nVDefaultSegments        = 12;
    aDefaultLightColor       = RGB_Color(COL_WHITE);
    aDefaultAmbientColor     = RGB_Color(COL_BLACK);
    bDoubleSided             = FALSE;
	mpMirrorOverlay = 0L;
}

/*************************************************************************
|*
|* Koennen die selektierten Objekte aufgebrochen werden?
|*
\************************************************************************/

BOOL E3dView::IsBreak3DObjPossible() const
{
    ULONG nCount = GetMarkedObjectCount();

    if (nCount > 0)
    {
        ULONG i = 0;

        while (i < nCount)
        {
            SdrObject* pObj = GetMarkedObjectByIndex(i);

            if (pObj && pObj->ISA(E3dObject))
            {
                if(!(((E3dObject*)pObj)->IsBreakObjPossible()))
                    return FALSE;
            }
            else
            {
                return FALSE;
            }

            i++;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/*************************************************************************
|*
|* Selektierte Lathe-Objekte aufbrechen
|*
\************************************************************************/

void E3dView::Break3DObj()
{
	if(IsBreak3DObjPossible())
	{
		// ALLE selektierten Objekte werden gewandelt
	    UINT32 nCount = GetMarkedObjectCount();

		BegUndo(String(SVX_RESSTR(RID_SVX_3D_UNDO_BREAK_LATHE)));
		for(UINT32 a=0;a<nCount;a++)
		{
			E3dObject* pObj = (E3dObject*)GetMarkedObjectByIndex(a);
			BreakSingle3DObj(pObj);
		}
		DeleteMarked();
		EndUndo();
	}
}

void E3dView::BreakSingle3DObj(E3dObject* pObj)
{
	if(pObj->ISA(E3dScene))
	{
		SdrObjList* pSubList = pObj->GetSubList();
		SdrObjListIter aIter(*pSubList, IM_FLAT);

		while(aIter.IsMore())
		{
			E3dObject* pSubObj = (E3dObject*)aIter.Next();
			BreakSingle3DObj(pSubObj);
		}
	}
	else
	{
		SdrAttrObj* pNewObj = pObj->GetBreakObj();
		if(pNewObj)
		{
			InsertObjectAtView(pNewObj, *GetSdrPageView(), SDRINSERT_DONTMARK);
			pNewObj->SetChanged();
			pNewObj->BroadcastObjectChange();
		}
	}
}

/*************************************************************************
|*
|* Szenen mischen
|*
\************************************************************************/

void E3dView::MergeScenes ()
{
    ULONG nCount = GetMarkedObjectCount();

    if (nCount > 0)
    {
        ULONG     nObj    = 0;
        SdrObject *pObj   = GetMarkedObjectByIndex(nObj);
		E3dScene  *pScene = new E3dPolyScene(Get3DDefaultAttributes());
        basegfx::B3DRange aBoundVol;
        Rectangle aAllBoundRect (GetMarkedObjBoundRect ());
		Point     aCenter (aAllBoundRect.Center());

        while (pObj)
        {
            if (pObj->ISA(E3dScene))
            {
                /**********************************************************
                * Es ist eine 3D-Scene oder 3D-PolyScene
                **********************************************************/
                SdrObjList* pSubList = ((E3dObject*) pObj)->GetSubList();

                SdrObjListIter aIter(*pSubList, IM_FLAT);

                while (aIter.IsMore())
                {
                    /******************************************************
                    * LatheObjekte suchen
                    ******************************************************/
                    SdrObject* pSubObj = aIter.Next();

                        E3dObject *pNewObj = 0;

                        switch (pSubObj->GetObjIdentifier())
                        {
			                case E3D_CUBEOBJ_ID	:
								pNewObj = new E3dCubeObj;
								*(E3dCubeObj*)pNewObj = *(E3dCubeObj*)pSubObj;
				                break;

			                case E3D_SPHEREOBJ_ID:
								pNewObj = new E3dSphereObj;
								*(E3dSphereObj*)pNewObj = *(E3dSphereObj*)pSubObj;
				                break;

			                case E3D_EXTRUDEOBJ_ID:
								pNewObj = new E3dExtrudeObj;
								*(E3dExtrudeObj*)pNewObj = *(E3dExtrudeObj*)pSubObj;
				                break;

			                case E3D_LATHEOBJ_ID:
								pNewObj = new E3dLatheObj;
								*(E3dLatheObj*)pNewObj = *(E3dLatheObj*)pSubObj;
				                break;

                            case E3D_COMPOUNDOBJ_ID:
								pNewObj = new E3dCompoundObject;
								*(E3dCompoundObject*)pNewObj = *(E3dCompoundObject*)pSubObj;
				                break;
                        }

                        Rectangle aBoundRect = pSubObj->GetCurrentBoundRect();

            			basegfx::B3DHomMatrix aMatrix;
            			aMatrix.translate(aBoundRect.Left() - aCenter.getX(), aCenter.getY(), 0.0);
			            pNewObj->SetTransform(aMatrix * pNewObj->GetTransform()); // #112587#

                        if (pNewObj) aBoundVol.expand(pNewObj->GetBoundVolume());
						pScene->Insert3DObj (pNewObj);
				}
            }

            nObj++;

            if (nObj < nCount)
            {
                pObj = GetMarkedObjectByIndex(nObj);
            }
            else
            {
                pObj = NULL;
            }
        }

	    double fW = aAllBoundRect.GetWidth();
	    double fH = aAllBoundRect.GetHeight();
	    Rectangle aRect(0,0, (long) fW, (long) fH);

	    InitScene(pScene, fW, fH, aBoundVol.getMaxZ() +  + ((fW + fH) / 4.0));
	    pScene->NbcSetSnapRect(aRect);

        Camera3D &aCamera  = (Camera3D&) pScene->GetCamera ();
		basegfx::B3DPoint aMinVec(aBoundVol.getMinimum());
        basegfx::B3DPoint aMaxVec(aBoundVol.getMaximum());
        double fDeepth(fabs(aMaxVec.getZ() - aMinVec.getZ()));

        aCamera.SetPRP(basegfx::B3DPoint(0.0, 0.0, 1000.0));
		double fDefaultCamPosZ(GetDefaultCamPosZ());
		aCamera.SetPosition(basegfx::B3DPoint(0.0, 0.0, fDefaultCamPosZ + fDeepth / 2.0));
	    aCamera.SetFocalLength(GetDefaultCamFocal());
        pScene->SetCamera (aCamera);

		// SnapRects der Objekte ungueltig
		pScene->SetRectsDirty();

		InsertObjectAtView(pScene, *(GetSdrPageViewOfMarkedByIndex(0)));

		// SnapRects der Objekte ungueltig
		pScene->SetRectsDirty();
    }
}

/*************************************************************************
|*
|* Possibilities, hauptsaechlich gruppieren/ungruppieren
|*
\************************************************************************/
void E3dView::CheckPossibilities()
{
	// call parent
	SdrView::CheckPossibilities();

	// Weitere Flags bewerten
	if(bGroupPossible || bUnGroupPossible || bGrpEnterPossible)
	{
		INT32 nMarkCnt = GetMarkedObjectCount();
		BOOL bCoumpound = FALSE;
		BOOL b3DObject = FALSE;
		for(INT32 nObjs = 0L; (nObjs < nMarkCnt) && !bCoumpound; nObjs++)
		{
			SdrObject *pObj = GetMarkedObjectByIndex(nObjs);
			if(pObj && pObj->ISA(E3dCompoundObject))
				bCoumpound = TRUE;
			if(pObj && pObj->ISA(E3dObject))
				b3DObject = TRUE;
		}

		// Bisher: Es sind ZWEI oder mehr beliebiger Objekte selektiert.
		// Nachsehen, ob CompoundObjects beteiligt sind. Falls ja,
		// das Gruppieren verbieten.
		if(bGroupPossible && bCoumpound)
			bGroupPossible = FALSE;

		if(bUnGroupPossible && b3DObject)
			bUnGroupPossible = FALSE;

		if(bGrpEnterPossible && bCoumpound)
			bGrpEnterPossible = FALSE;
	}
}

// eof
