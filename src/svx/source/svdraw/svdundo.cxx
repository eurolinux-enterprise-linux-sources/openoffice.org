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

#include <svx/svdundo.hxx>
#include "svditext.hxx"
#include <svx/svdotext.hxx>
#include <svx/svdobj.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdlayer.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdview.hxx>
#include "svdstr.hrc"   // Namen aus der Resource
#include "svdglob.hxx"  // StringCache
#include <svx/scene3d.hxx>
#include <svx/outlobj.hxx>
#include <svx/svdogrp.hxx>
#include <svx/sdr/properties/itemsettools.hxx>
#include <svx/sdr/properties/properties.hxx>
#include <svx/svdocapt.hxx>
#include <svtools/whiter.hxx>
#include <svx/e3dsceneupdater.hxx>

#include "svdviter.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////

// iterates over all views and unmarks this SdrObject if it is marked
static void ImplUnmarkObject( SdrObject* pObj )
{
    SdrViewIter aIter( pObj );
    for ( SdrView* pView = aIter.FirstView(); pView; pView = aIter.NextView() )
	{
		pView->MarkObj( pObj, pView->GetSdrPageView(), TRUE );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TYPEINIT1(SdrUndoAction,SfxUndoAction);

BOOL SdrUndoAction::CanRepeat(SfxRepeatTarget& rView) const
{
	SdrView* pV=PTR_CAST(SdrView,&rView);
	if (pV!=NULL) return CanSdrRepeat(*pV);
	return FALSE;
}

void SdrUndoAction::Repeat(SfxRepeatTarget& rView)
{
	SdrView* pV=PTR_CAST(SdrView,&rView);
	if (pV!=NULL) SdrRepeat(*pV);
	DBG_ASSERT(pV!=NULL,"Repeat: Uebergebenes SfxRepeatTarget ist keine SdrView");
}

XubString SdrUndoAction::GetRepeatComment(SfxRepeatTarget& rView) const
{
	SdrView* pV=PTR_CAST(SdrView,&rView);
	if (pV!=NULL) return GetSdrRepeatComment(*pV);
	return String();
}

bool SdrUndoAction::CanSdrRepeat(SdrView& /*rView*/) const
{
	return FALSE;
}

void SdrUndoAction::SdrRepeat(SdrView& /*rView*/)
{
}

XubString SdrUndoAction::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	return String();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoGroup::SdrUndoGroup(SdrModel& rNewMod)
:	SdrUndoAction(rNewMod),
	aBuf(1024,32,32),
	eFunction(SDRREPFUNC_OBJ_NONE)		/*#72642#*/
{}

SdrUndoGroup::SdrUndoGroup(SdrModel& rNewMod,const String& rStr)
:	SdrUndoAction(rNewMod),
	aBuf(1024,32,32),
	aComment(rStr),
	eFunction(SDRREPFUNC_OBJ_NONE)
{}

SdrUndoGroup::~SdrUndoGroup()
{
	Clear();
}

void SdrUndoGroup::Clear()
{
	for (ULONG nu=0; nu<GetActionCount(); nu++) {
		SdrUndoAction* pAct=GetAction(nu);
		delete pAct;
	}
	aBuf.Clear();
}

void SdrUndoGroup::AddAction(SdrUndoAction* pAct)
{
	aBuf.Insert(pAct,CONTAINER_APPEND);
}

void SdrUndoGroup::push_front( SdrUndoAction* pAct )
{
	aBuf.Insert(pAct, (ULONG)0 );
}

void SdrUndoGroup::Undo()
{
	for (ULONG nu=GetActionCount(); nu>0;) {
		nu--;
		SdrUndoAction* pAct=GetAction(nu);
		pAct->Undo();
	}
}

void SdrUndoGroup::Redo()
{
	for (ULONG nu=0; nu<GetActionCount(); nu++) {
		SdrUndoAction* pAct=GetAction(nu);
		pAct->Redo();
	}
}

XubString SdrUndoGroup::GetComment() const
{
	XubString aRet(aComment);
	sal_Char aSearchText[] = "%1";
	String aSearchString(aSearchText, sizeof(aSearchText-1));

	aRet.SearchAndReplace(aSearchString, aObjDescription);

	return aRet;
}

bool SdrUndoGroup::CanSdrRepeat(SdrView& rView) const
{
	switch (eFunction) {
		case SDRREPFUNC_OBJ_NONE			:  return FALSE;
		case SDRREPFUNC_OBJ_DELETE          :  return rView.AreObjectsMarked();
		case SDRREPFUNC_OBJ_COMBINE_POLYPOLY:  return rView.IsCombinePossible(FALSE);
		case SDRREPFUNC_OBJ_COMBINE_ONEPOLY :  return rView.IsCombinePossible(TRUE);
		case SDRREPFUNC_OBJ_DISMANTLE_POLYS :  return rView.IsDismantlePossible(FALSE);
		case SDRREPFUNC_OBJ_DISMANTLE_LINES :  return rView.IsDismantlePossible(TRUE);
		case SDRREPFUNC_OBJ_CONVERTTOPOLY   :  return rView.IsConvertToPolyObjPossible(FALSE);
		case SDRREPFUNC_OBJ_CONVERTTOPATH   :  return rView.IsConvertToPathObjPossible(FALSE);
		case SDRREPFUNC_OBJ_GROUP           :  return rView.IsGroupPossible();
		case SDRREPFUNC_OBJ_UNGROUP         :  return rView.IsUnGroupPossible();
		case SDRREPFUNC_OBJ_PUTTOTOP        :  return rView.IsToTopPossible();
		case SDRREPFUNC_OBJ_PUTTOBTM        :  return rView.IsToBtmPossible();
		case SDRREPFUNC_OBJ_MOVTOTOP        :  return rView.IsToTopPossible();
		case SDRREPFUNC_OBJ_MOVTOBTM        :  return rView.IsToBtmPossible();
		case SDRREPFUNC_OBJ_REVORDER        :  return rView.IsReverseOrderPossible();
		case SDRREPFUNC_OBJ_IMPORTMTF       :  return rView.IsImportMtfPossible();
		default: break;
	} // switch
	return FALSE;
}

void SdrUndoGroup::SdrRepeat(SdrView& rView)
{
	switch (eFunction) {
		case SDRREPFUNC_OBJ_NONE			:  break;
		case SDRREPFUNC_OBJ_DELETE          :  rView.DeleteMarked();                break;
		case SDRREPFUNC_OBJ_COMBINE_POLYPOLY:  rView.CombineMarkedObjects(sal_False);   break;
		case SDRREPFUNC_OBJ_COMBINE_ONEPOLY :  rView.CombineMarkedObjects(sal_True);    break;
		case SDRREPFUNC_OBJ_DISMANTLE_POLYS :  rView.DismantleMarkedObjects(FALSE); break;
		case SDRREPFUNC_OBJ_DISMANTLE_LINES :  rView.DismantleMarkedObjects(TRUE);  break;
		case SDRREPFUNC_OBJ_CONVERTTOPOLY   :  rView.ConvertMarkedToPolyObj(FALSE); break;
		case SDRREPFUNC_OBJ_CONVERTTOPATH   :  rView.ConvertMarkedToPathObj(FALSE); break;
		case SDRREPFUNC_OBJ_GROUP           :  rView.GroupMarked();                 break;
		case SDRREPFUNC_OBJ_UNGROUP         :  rView.UnGroupMarked();               break;
		case SDRREPFUNC_OBJ_PUTTOTOP        :  rView.PutMarkedToTop();              break;
		case SDRREPFUNC_OBJ_PUTTOBTM        :  rView.PutMarkedToBtm();              break;
		case SDRREPFUNC_OBJ_MOVTOTOP        :  rView.MovMarkedToTop();              break;
		case SDRREPFUNC_OBJ_MOVTOBTM        :  rView.MovMarkedToBtm();              break;
		case SDRREPFUNC_OBJ_REVORDER        :  rView.ReverseOrderOfMarked();        break;
		case SDRREPFUNC_OBJ_IMPORTMTF       :  rView.DoImportMarkedMtf();           break;
		default: break;
	} // switch
}

XubString SdrUndoGroup::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	XubString aRet(aComment);
	sal_Char aSearchText[] = "%1";
	String aSearchString(aSearchText, sizeof(aSearchText-1));

	aRet.SearchAndReplace(aSearchString, ImpGetResStr(STR_ObjNameSingulPlural));

	return aRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   @@@@   @@@@@   @@@@@@  @@@@@   @@@@   @@@@@@   @@@@
//  @@  @@  @@  @@      @@  @@     @@  @@    @@    @@  @@
//  @@  @@  @@  @@      @@  @@     @@        @@    @@
//  @@  @@  @@@@@       @@  @@@@   @@        @@     @@@@
//  @@  @@  @@  @@      @@  @@     @@        @@        @@
//  @@  @@  @@  @@  @@  @@  @@     @@  @@    @@    @@  @@
//   @@@@   @@@@@    @@@@   @@@@@   @@@@     @@     @@@@
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoObj::SdrUndoObj(SdrObject& rNewObj):
	SdrUndoAction(*rNewObj.GetModel()),
	pObj(&rNewObj)
{
}

void SdrUndoObj::GetDescriptionStringForObject( const SdrObject& _rForObject, USHORT nStrCacheID, String& rStr, FASTBOOL bRepeat )
{
	rStr = ImpGetResStr(nStrCacheID);
	sal_Char aSearchText[] = "%1";
	String aSearchString(aSearchText, sizeof(aSearchText-1));

	xub_StrLen nPos = rStr.Search(aSearchString);

	if(nPos != STRING_NOTFOUND)
	{
		rStr.Erase(nPos, 2);

		if(bRepeat)
		{
			rStr.Insert(ImpGetResStr(STR_ObjNameSingulPlural), nPos);
		}
		else
		{
			XubString aStr;

			_rForObject.TakeObjNameSingul(aStr);
			rStr.Insert(aStr, nPos);
		}
	}
}

void SdrUndoObj::ImpTakeDescriptionStr(USHORT nStrCacheID, XubString& rStr, FASTBOOL bRepeat) const
{
    if ( pObj )
        GetDescriptionStringForObject( *pObj, nStrCacheID, rStr, bRepeat );
}

// #94278# common call method for evtl. page change when UNDO/REDO
// is triggered
void SdrUndoObj::ImpShowPageOfThisObject()
{
	if(pObj && pObj->IsInserted() && pObj->GetPage() && pObj->GetModel())
	{
		SdrHint aHint(HINT_SWITCHTOPAGE);

		aHint.SetObject(pObj);
		aHint.SetPage(pObj->GetPage());

		pObj->GetModel()->Broadcast(aHint);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoAttrObj::SdrUndoAttrObj(SdrObject& rNewObj, FASTBOOL bStyleSheet1, FASTBOOL bSaveText)
:	SdrUndoObj(rNewObj),
	pUndoSet(NULL),
	pRedoSet(NULL),
	pRepeatSet(NULL),
	pUndoStyleSheet(NULL),
	pRedoStyleSheet(NULL),
	pRepeatStyleSheet(NULL),
	bHaveToTakeRedoSet(TRUE),
	pTextUndo(NULL),

	// #i8508#
	pTextRedo(NULL),

	pUndoGroup(NULL)
{
	bStyleSheet = bStyleSheet1;

	SdrObjList* pOL = rNewObj.GetSubList();
	BOOL bIsGroup(pOL!=NULL && pOL->GetObjCount());
	BOOL bIs3DScene(bIsGroup && pObj->ISA(E3dScene));

	if(bIsGroup)
	{
		// Aha, Gruppenobjekt
		pUndoGroup = new SdrUndoGroup(*pObj->GetModel());
		sal_uInt32 nObjAnz(pOL->GetObjCount());

		for(sal_uInt32 nObjNum(0); nObjNum < nObjAnz; nObjNum++)
		{
			pUndoGroup->AddAction(
				new SdrUndoAttrObj(*pOL->GetObj(nObjNum), bStyleSheet1));
		}
	}

	if(!bIsGroup || bIs3DScene)
	{
		if(pUndoSet)
		{
			delete pUndoSet;
		}

		pUndoSet = new SfxItemSet(pObj->GetMergedItemSet());

		if(bStyleSheet)
			pUndoStyleSheet = pObj->GetStyleSheet();

		if(bSaveText)
		{
			pTextUndo = pObj->GetOutlinerParaObject();
			if(pTextUndo)
				pTextUndo = new OutlinerParaObject(*pTextUndo);
		}
	}
}

SdrUndoAttrObj::~SdrUndoAttrObj()
{
	if(pUndoSet)
		delete pUndoSet;
	if(pRedoSet)
		delete pRedoSet;
	if(pRepeatSet)
		delete pRepeatSet;
	if(pUndoGroup)
		delete pUndoGroup;
	if(pTextUndo)
		delete pTextUndo;

	// #i8508#
	if(pTextRedo)
		delete pTextRedo;
}

void SdrUndoAttrObj::SetRepeatAttr(const SfxItemSet& rSet)
{
	if(pRepeatSet)
		delete pRepeatSet;

	pRepeatSet = new SfxItemSet(rSet);
}

void SdrUndoAttrObj::Undo()
{
    E3DModifySceneSnapRectUpdater aUpdater(pObj);
	BOOL bIs3DScene(pObj && pObj->ISA(E3dScene));

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	if(!pUndoGroup || bIs3DScene)
	{
		if(bHaveToTakeRedoSet)
		{
			bHaveToTakeRedoSet = FALSE;

			if(pRedoSet)
			{
				delete pRedoSet;
			}

			pRedoSet = new SfxItemSet(pObj->GetMergedItemSet());

			if(bStyleSheet)
				pRedoStyleSheet=pObj->GetStyleSheet();

			if(pTextUndo)
			{
				// #i8508#
				pTextRedo = pObj->GetOutlinerParaObject();

				if(pTextRedo)
					pTextRedo = new OutlinerParaObject(*pTextRedo);
			}
		}

		if(bStyleSheet)
		{
			pRedoStyleSheet = pObj->GetStyleSheet();
			pObj->SetStyleSheet(pUndoStyleSheet, TRUE);
		}

		sdr::properties::ItemChangeBroadcaster aItemChange(*pObj);

		// #105122# Since ClearItem sets back everything to normal
		// it also sets fit-to-size text to non-fit-to-size text and
		// switches on autogrowheight (the default). That may lead to
		// loosing the geometry size info for the object when it is
		// re-layouted from AdjustTextFrameWidthAndHeight(). This makes
		// rescuing the size of the object necessary.
		const Rectangle aSnapRect = pObj->GetSnapRect();

		if(pUndoSet)
		{
			// #109587#
			if(pObj->ISA(SdrCaptionObj))
			{
				// do a more smooth item deletion here, else the text
				// rect will be reformatted, especially when information regarding
				// vertical text is changed. When clearing only set items it's
				// slower, but safer regarding such information (it's not changed
				// usually)
				SfxWhichIter aIter(*pUndoSet);
				sal_uInt16 nWhich(aIter.FirstWhich());

				while(nWhich)
				{
					if(SFX_ITEM_SET != pUndoSet->GetItemState(nWhich, sal_False))
					{
						pObj->ClearMergedItem(nWhich);
					}

					nWhich = aIter.NextWhich();
				}
			}
			else
			{
				pObj->ClearMergedItem();
			}

			pObj->SetMergedItemSet(*pUndoSet);
		}

		// #105122# Restore prev size here when it was changed.
		if(aSnapRect != pObj->GetSnapRect())
		{
			pObj->NbcSetSnapRect(aSnapRect);
		}

		pObj->GetProperties().BroadcastItemChange(aItemChange);

		if(pTextUndo)
		{
			pObj->SetOutlinerParaObject(new OutlinerParaObject(*pTextUndo));
		}
	}

	if(pUndoGroup)
	{
		pUndoGroup->Undo();
	}
}

void SdrUndoAttrObj::Redo()
{
    E3DModifySceneSnapRectUpdater aUpdater(pObj);
	BOOL bIs3DScene(pObj && pObj->ISA(E3dScene));

	if(!pUndoGroup || bIs3DScene)
	{
		if(bStyleSheet)
		{
			pUndoStyleSheet = pObj->GetStyleSheet();
			pObj->SetStyleSheet(pRedoStyleSheet, TRUE);
		}

		sdr::properties::ItemChangeBroadcaster aItemChange(*pObj);

		// #105122#
		const Rectangle aSnapRect = pObj->GetSnapRect();

		if(pRedoSet)
		{
			// #109587#
			if(pObj->ISA(SdrCaptionObj))
			{
				// do a more smooth item deletion here, else the text
				// rect will be reformatted, especially when information regarding
				// vertical text is changed. When clearing only set items it's
				// slower, but safer regarding such information (it's not changed
				// usually)
				SfxWhichIter aIter(*pRedoSet);
				sal_uInt16 nWhich(aIter.FirstWhich());

				while(nWhich)
				{
					if(SFX_ITEM_SET != pRedoSet->GetItemState(nWhich, sal_False))
					{
						pObj->ClearMergedItem(nWhich);
					}

					nWhich = aIter.NextWhich();
				}
			}
			else
			{
				pObj->ClearMergedItem();
			}

			pObj->SetMergedItemSet(*pRedoSet);
		}

		// #105122# Restore prev size here when it was changed.
		if(aSnapRect != pObj->GetSnapRect())
		{
			pObj->NbcSetSnapRect(aSnapRect);
		}

		pObj->GetProperties().BroadcastItemChange(aItemChange);

		// #i8508#
		if(pTextRedo)
		{
			pObj->SetOutlinerParaObject(new OutlinerParaObject(*pTextRedo));
		}
	}

	if(pUndoGroup)
	{
		pUndoGroup->Redo();
	}

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

XubString SdrUndoAttrObj::GetComment() const
{
	XubString aStr;

	if(bStyleSheet)
	{
		ImpTakeDescriptionStr(STR_EditSetStylesheet, aStr);
	}
	else
	{
		ImpTakeDescriptionStr(STR_EditSetAttributes, aStr);
	}

	return aStr;
}

void SdrUndoAttrObj::SdrRepeat(SdrView& rView)
{
	if(pRepeatSet)
	{
		rView.SetAttrToMarked(*pRepeatSet, FALSE);
	}
}

bool SdrUndoAttrObj::CanSdrRepeat(SdrView& rView) const
{
	return (pRepeatSet!=0L && rView.AreObjectsMarked());
}

XubString SdrUndoAttrObj::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	XubString aStr;

	if(bStyleSheet)
	{
		ImpTakeDescriptionStr(STR_EditSetStylesheet, aStr, TRUE);
	}
	else
	{
		ImpTakeDescriptionStr(STR_EditSetAttributes, aStr, TRUE);
	}

	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoMoveObj::Undo()
{
	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	pObj->Move(Size(-aDistance.Width(),-aDistance.Height()));
}

void SdrUndoMoveObj::Redo()
{
	pObj->Move(Size(aDistance.Width(),aDistance.Height()));

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

XubString SdrUndoMoveObj::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_EditMove,aStr);
	return aStr;
}

void SdrUndoMoveObj::SdrRepeat(SdrView& rView)
{
	rView.MoveMarkedObj(aDistance);
}

bool SdrUndoMoveObj::CanSdrRepeat(SdrView& rView) const
{
	return rView.AreObjectsMarked();
}

XubString SdrUndoMoveObj::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_EditMove,aStr,TRUE);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoGeoObj::SdrUndoGeoObj(SdrObject& rNewObj):
	SdrUndoObj(rNewObj),
	pUndoGeo(NULL),
	pRedoGeo(NULL),
	pUndoGroup(NULL)
{
	SdrObjList* pOL=rNewObj.GetSubList();
	if (pOL!=NULL && pOL->GetObjCount() && !rNewObj.ISA(E3dScene))
	{
		// Aha, Gruppenobjekt
		// AW: Aber keine 3D-Szene, dann nur fuer die Szene selbst den Undo anlegen
		pUndoGroup=new SdrUndoGroup(*pObj->GetModel());
		ULONG nObjAnz=pOL->GetObjCount();
		for (ULONG nObjNum=0; nObjNum<nObjAnz; nObjNum++) {
			pUndoGroup->AddAction(new SdrUndoGeoObj(*pOL->GetObj(nObjNum)));
		}
	} else {
		pUndoGeo=pObj->GetGeoData();
	}
}

SdrUndoGeoObj::~SdrUndoGeoObj()
{
	if (pUndoGeo!=NULL) delete pUndoGeo;
	if (pRedoGeo!=NULL) delete pRedoGeo;
	if (pUndoGroup!=NULL) delete pUndoGroup;
}

void SdrUndoGeoObj::Undo()
{
	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	if(pUndoGroup)
	{
		pUndoGroup->Undo();

		// #97172#
		// only repaint, no objectchange
		pObj->ActionChanged();
	}
	else
	{
		if (pRedoGeo!=NULL) delete pRedoGeo;
		pRedoGeo=pObj->GetGeoData();
		pObj->SetGeoData(*pUndoGeo);
	}
}

void SdrUndoGeoObj::Redo()
{
	if(pUndoGroup)
	{
		pUndoGroup->Redo();

		// #97172#
		// only repaint, no objectchange
		pObj->ActionChanged();
	}
	else
	{
		if (pUndoGeo!=NULL) delete pUndoGeo;
		pUndoGeo=pObj->GetGeoData();
		pObj->SetGeoData(*pRedoGeo);
	}

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

XubString SdrUndoGeoObj::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_DragMethObjOwn,aStr);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoObjList::SdrUndoObjList(SdrObject& rNewObj, bool bOrdNumDirect)
:	SdrUndoObj(rNewObj),
	bOwner(FALSE),
	pView(NULL),
	pPageView(NULL)
{
	pObjList=pObj->GetObjList();
	if (bOrdNumDirect) {
		nOrdNum=pObj->GetOrdNumDirect();
	} else {
		nOrdNum=pObj->GetOrdNum();
	}
}

SdrUndoObjList::~SdrUndoObjList()
{
	if (pObj!=NULL && IsOwner())
	{
		// Attribute muessen wieder in den regulaeren Pool
		SetOwner(FALSE);

		// nun loeschen
		SdrObject::Free( pObj );
	}
}

void SdrUndoObjList::SetOwner(bool bNew)
{
	bOwner = bNew;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoRemoveObj::Undo()
{
	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	DBG_ASSERT(!pObj->IsInserted(),"UndoRemoveObj: pObj ist bereits Inserted");
	if (!pObj->IsInserted())
	{
		// #i11426#
		// For UNDOs in Calc/Writer it is necessary to adapt the anchor
		// pos of the target object.
		Point aOwnerAnchorPos(0, 0);

		if(pObjList
			&& pObjList->GetOwnerObj()
			&& pObjList->GetOwnerObj()->ISA(SdrObjGroup))
		{
			aOwnerAnchorPos = pObjList->GetOwnerObj()->GetAnchorPos();
		}

        E3DModifySceneSnapRectUpdater aUpdater(pObjList->GetOwnerObj());
		SdrInsertReason aReason(SDRREASON_UNDO);
		pObjList->InsertObject(pObj,nOrdNum,&aReason);

		// #i11426#
		if(aOwnerAnchorPos.X() || aOwnerAnchorPos.Y())
		{
			pObj->NbcSetAnchorPos(aOwnerAnchorPos);
		}
	}
}

void SdrUndoRemoveObj::Redo()
{
	DBG_ASSERT(pObj->IsInserted(),"RedoRemoveObj: pObj ist nicht Inserted");
	if (pObj->IsInserted())
	{
		ImplUnmarkObject( pObj );
        E3DModifySceneSnapRectUpdater aUpdater(pObj);
		pObjList->RemoveObject(nOrdNum);
	}

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoInsertObj::Undo()
{
	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	DBG_ASSERT(pObj->IsInserted(),"UndoInsertObj: pObj ist nicht Inserted");
	if (pObj->IsInserted())
	{
		ImplUnmarkObject( pObj );

#ifdef DBG_UTIL
		SdrObject* pChkObj=
#endif
		pObjList->RemoveObject(nOrdNum);
		DBG_ASSERT(pChkObj==pObj,"UndoInsertObj: RemoveObjNum!=pObj");
	}
}

void SdrUndoInsertObj::Redo()
{
	DBG_ASSERT(!pObj->IsInserted(),"RedoInsertObj: pObj ist bereits Inserted");
    if (!pObj->IsInserted())
    {
        // --> OD 2005-05-10 #i45952# - restore anchor position of an object,
        // which becomes a member of a group, because its cleared in method
        // <InsertObject(..)>. Needed for correct ReDo in Writer.
        Point aAnchorPos( 0, 0 );
        if ( pObjList &&
             pObjList->GetOwnerObj() &&
             pObjList->GetOwnerObj()->ISA(SdrObjGroup) )
        {
            aAnchorPos = pObj->GetAnchorPos();
        }
        // <--

		SdrInsertReason aReason(SDRREASON_UNDO);
		pObjList->InsertObject(pObj,nOrdNum,&aReason);

        // --> OD 2005-05-10 #i45952#
        if ( aAnchorPos.X() || aAnchorPos.Y() )
        {
            pObj->NbcSetAnchorPos( aAnchorPos );
        }
        // <--
	}

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoDelObj::Undo()
{
	SdrUndoRemoveObj::Undo();
	DBG_ASSERT(IsOwner(),"UndoDeleteObj: pObj gehoert nicht der UndoAction");
	SetOwner(FALSE);
}

void SdrUndoDelObj::Redo()
{
	SdrUndoRemoveObj::Redo();
	DBG_ASSERT(!IsOwner(),"RedoDeleteObj: pObj gehoert bereits der UndoAction");
	SetOwner(TRUE);
}

XubString SdrUndoDelObj::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_EditDelete,aStr);
	return aStr;
}

void SdrUndoDelObj::SdrRepeat(SdrView& rView)
{
	rView.DeleteMarked();
}

bool SdrUndoDelObj::CanSdrRepeat(SdrView& rView) const
{
	return rView.AreObjectsMarked();
}

XubString SdrUndoDelObj::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_EditDelete,aStr,TRUE);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoNewObj::Undo()
{
	SdrUndoInsertObj::Undo();
	DBG_ASSERT(!IsOwner(),"RedoNewObj: pObj gehoert bereits der UndoAction");
	SetOwner(TRUE);
}

void SdrUndoNewObj::Redo()
{
	SdrUndoInsertObj::Redo();
	DBG_ASSERT(IsOwner(),"RedoNewObj: pObj gehoert nicht der UndoAction");
	SetOwner(FALSE);
}

String SdrUndoNewObj::GetComment( const SdrObject& _rForObject )
{
    String sComment;
    GetDescriptionStringForObject( _rForObject, STR_UndoInsertObj, sComment );
    return sComment;
}

XubString SdrUndoNewObj::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoInsertObj,aStr);
	return aStr;
}

SdrUndoReplaceObj::SdrUndoReplaceObj(SdrObject& rOldObj1, SdrObject& rNewObj1, bool bOrdNumDirect)
:	SdrUndoObj(rOldObj1),
	bOldOwner(FALSE),
	bNewOwner(FALSE),
	pNewObj(&rNewObj1)
{
	SetOldOwner(TRUE);

	pObjList=pObj->GetObjList();
	if (bOrdNumDirect) {
		nOrdNum=pObj->GetOrdNumDirect();
	} else {
		nOrdNum=pObj->GetOrdNum();
	}
}

SdrUndoReplaceObj::~SdrUndoReplaceObj()
{
	if (pObj!=NULL && IsOldOwner())
	{
		// Attribute muessen wieder in den regulaeren Pool
		SetOldOwner(FALSE);

		// nun loeschen
		SdrObject::Free( pObj );
	}
	if (pNewObj!=NULL && IsNewOwner())
	{
		// Attribute muessen wieder in den regulaeren Pool
		SetNewOwner(FALSE);

		// nun loeschen
		SdrObject::Free( pNewObj );
	}
}

void SdrUndoReplaceObj::Undo()
{
	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	if (IsOldOwner() && !IsNewOwner())
	{
		DBG_ASSERT(!pObj->IsInserted(),"SdrUndoReplaceObj::Undo(): Altes Objekt ist bereits inserted!");
		DBG_ASSERT(pNewObj->IsInserted(),"SdrUndoReplaceObj::Undo(): Neues Objekt ist nicht inserted!");
		SetOldOwner(FALSE);
		SetNewOwner(TRUE);

		ImplUnmarkObject( pNewObj );
		pObjList->ReplaceObject(pObj,nOrdNum);
	}
	else
	{
		DBG_ERROR("SdrUndoReplaceObj::Undo(): IsMine-Flags stehen verkehrt. Doppelter Undo-Aufruf?");
	}
}

void SdrUndoReplaceObj::Redo()
{
	if (!IsOldOwner() && IsNewOwner())
	{
		DBG_ASSERT(!pNewObj->IsInserted(),"SdrUndoReplaceObj::Redo(): Neues Objekt ist bereits inserted!");
		DBG_ASSERT(pObj->IsInserted(),"SdrUndoReplaceObj::Redo(): Altes Objekt ist nicht inserted!");
		SetOldOwner(TRUE);
		SetNewOwner(FALSE);

		ImplUnmarkObject( pObj );
		pObjList->ReplaceObject(pNewObj,nOrdNum);

	}
	else
	{
		DBG_ERROR("SdrUndoReplaceObj::Redo(): IsMine-Flags stehen verkehrt. Doppelter Redo-Aufruf?");
	}

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

void SdrUndoReplaceObj::SetNewOwner(bool bNew)
{
	bNewOwner = bNew;
}

void SdrUndoReplaceObj::SetOldOwner(bool bNew)
{
	bOldOwner = bNew;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

XubString SdrUndoCopyObj::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoCopyObj,aStr);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// #i11702#

SdrUndoObjectLayerChange::SdrUndoObjectLayerChange(SdrObject& rObj, SdrLayerID aOldLayer, SdrLayerID aNewLayer)
:	SdrUndoObj(rObj),
	maOldLayer(aOldLayer),
	maNewLayer(aNewLayer)
{
}

void SdrUndoObjectLayerChange::Undo()
{
	ImpShowPageOfThisObject();
	pObj->SetLayer(maOldLayer);
}

void SdrUndoObjectLayerChange::Redo()
{
	pObj->SetLayer(maNewLayer);
	ImpShowPageOfThisObject();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoObjOrdNum::SdrUndoObjOrdNum(SdrObject& rNewObj, UINT32 nOldOrdNum1, UINT32 nNewOrdNum1):
	SdrUndoObj(rNewObj),
	nOldOrdNum(nOldOrdNum1),
	nNewOrdNum(nNewOrdNum1)
{
}

void SdrUndoObjOrdNum::Undo()
{
	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	SdrObjList* pOL=pObj->GetObjList();
	if (pOL==NULL) {
		DBG_ERROR("UndoObjOrdNum: pObj hat keine ObjList");
		return;
	}
	pOL->SetObjectOrdNum(nNewOrdNum,nOldOrdNum);
}

void SdrUndoObjOrdNum::Redo()
{
	SdrObjList* pOL=pObj->GetObjList();
	if (pOL==NULL) {
		DBG_ERROR("RedoObjOrdNum: pObj hat keine ObjList");
		return;
	}
	pOL->SetObjectOrdNum(nOldOrdNum,nNewOrdNum);

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

XubString SdrUndoObjOrdNum::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoObjOrdNum,aStr);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoObjSetText::SdrUndoObjSetText(SdrObject& rNewObj, sal_Int32 nText)
: SdrUndoObj(rNewObj)
, pOldText(NULL)
, pNewText(NULL)
, bNewTextAvailable(FALSE)
, bEmptyPresObj(FALSE)
, mnText(nText)
{
	SdrText* pText = static_cast< SdrTextObj*>( &rNewObj )->getText(mnText);
	if( pText && pText->GetOutlinerParaObject() )
		pOldText = new OutlinerParaObject(*pText->GetOutlinerParaObject());

	bEmptyPresObj = rNewObj.IsEmptyPresObj();
}

SdrUndoObjSetText::~SdrUndoObjSetText()
{
	if ( pOldText )
		delete pOldText;
	if ( pNewText )
		delete pNewText;
}

void SdrUndoObjSetText::AfterSetText()
{
	if (!bNewTextAvailable)
	{
		SdrText* pText = static_cast< SdrTextObj*>( pObj )->getText(mnText);
		if( pText && pText->GetOutlinerParaObject() )
			pNewText = new OutlinerParaObject(*pText->GetOutlinerParaObject());
		bNewTextAvailable=TRUE;
	}
}

void SdrUndoObjSetText::Undo()
{
	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();

	// alten Text sichern fuer Redo
	if (!bNewTextAvailable)
		AfterSetText();

	// Text fuer Undo kopieren, denn SetOutlinerParaObject() ist Eigentumsuebereignung
	OutlinerParaObject* pText1 = pOldText;
	if(pText1)
		pText1 = new OutlinerParaObject(*pText1);

	SdrText* pText = static_cast< SdrTextObj*>( pObj )->getText(mnText);
	if( pText )
		pText->SetOutlinerParaObject(pText1);

	pObj->SetEmptyPresObj( bEmptyPresObj );
	pObj->ActionChanged();
}

void SdrUndoObjSetText::Redo()
{
	// Text fuer Undo kopieren, denn SetOutlinerParaObject() ist Eigentumsuebereignung
	OutlinerParaObject* pText1 = pNewText;

	if(pText1)
		pText1 = new OutlinerParaObject(*pText1);

	SdrText* pText = static_cast< SdrTextObj*>( pObj )->getText(mnText);
	if( pText )
		static_cast< SdrTextObj* >( pObj )->NbcSetOutlinerParaObjectForText( pText1, pText );

	pObj->ActionChanged();

	// #94278# Trigger PageChangeCall
	ImpShowPageOfThisObject();
}

XubString SdrUndoObjSetText::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoObjSetText,aStr);
	return aStr;
}

XubString SdrUndoObjSetText::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoObjSetText,aStr);
	return aStr;
}

void SdrUndoObjSetText::SdrRepeat(SdrView& rView)
{
	if (bNewTextAvailable && rView.AreObjectsMarked())
	{
		const SdrMarkList& rML=rView.GetMarkedObjectList();

		const bool bUndo = rView.IsUndoEnabled();
		if( bUndo )
		{
			XubString aStr;
			ImpTakeDescriptionStr(STR_UndoObjSetText,aStr);
			rView.BegUndo(aStr);
		}

		ULONG nAnz=rML.GetMarkCount();
		for (ULONG nm=0; nm<nAnz; nm++)
		{
			SdrObject* pObj2=rML.GetMark(nm)->GetMarkedSdrObj();
			SdrTextObj* pTextObj=PTR_CAST(SdrTextObj,pObj2);
			if (pTextObj!=NULL)
			{
				if( bUndo )
					rView.AddUndo(new SdrUndoObjSetText(*pTextObj,0));

				OutlinerParaObject* pText1=pNewText;
				if (pText1!=NULL)
					pText1 = new OutlinerParaObject(*pText1);
				pTextObj->SetOutlinerParaObject(pText1);
			}
		}

		if( bUndo )
			rView.EndUndo();
	}
}

bool SdrUndoObjSetText::CanSdrRepeat(SdrView& rView) const
{
	FASTBOOL bOk=FALSE;
	if (bNewTextAvailable && rView.AreObjectsMarked()) {
		bOk=TRUE;
	}
	return bOk;
}

// --> OD 2009-07-09 #i73249#
SdrUndoObjStrAttr::SdrUndoObjStrAttr( SdrObject& rNewObj,
                                      const ObjStrAttrType eObjStrAttr,
                                      const String& sOldStr,
                                      const String& sNewStr)
    : SdrUndoObj( rNewObj ),
      meObjStrAttr( eObjStrAttr ),
      msOldStr( sOldStr ),
      msNewStr( sNewStr )
{
}

void SdrUndoObjStrAttr::Undo()
{
    ImpShowPageOfThisObject();

    switch ( meObjStrAttr )
    {
        case OBJ_NAME:
        {
            pObj->SetName( msOldStr );
        }
        break;
        case OBJ_TITLE:
        {
            pObj->SetTitle( msOldStr );
        }
        break;
        case OBJ_DESCRIPTION:
        {
            pObj->SetDescription( msOldStr );
        }
        break;
    }
}

void SdrUndoObjStrAttr::Redo()
{
    switch ( meObjStrAttr )
    {
        case OBJ_NAME:
        {
            pObj->SetName( msNewStr );
        }
        break;
        case OBJ_TITLE:
        {
            pObj->SetTitle( msNewStr );
        }
        break;
        case OBJ_DESCRIPTION:
        {
            pObj->SetDescription( msNewStr );
        }
        break;
    }

    ImpShowPageOfThisObject();
}

String SdrUndoObjStrAttr::GetComment() const
{
    String aStr;
    switch ( meObjStrAttr )
    {
        case OBJ_NAME:
        {
            ImpTakeDescriptionStr( STR_UndoObjName, aStr );
            aStr += sal_Unicode(' ');
            aStr += sal_Unicode('\'');
            aStr += msNewStr;
            aStr += sal_Unicode('\'');
        }
        break;
        case OBJ_TITLE:
        {
            ImpTakeDescriptionStr( STR_UndoObjTitle, aStr );
        }
        break;
        case OBJ_DESCRIPTION:
        {
            ImpTakeDescriptionStr( STR_UndoObjDescription, aStr );
        }
        break;
    }

    return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@      @@@@   @@  @@  @@@@@  @@@@@
//  @@     @@  @@  @@  @@  @@     @@  @@
//  @@     @@  @@  @@  @@  @@     @@  @@
//  @@     @@@@@@   @@@@   @@@@   @@@@@
//  @@     @@  @@    @@    @@     @@  @@
//  @@     @@  @@    @@    @@     @@  @@
//  @@@@@  @@  @@    @@    @@@@@  @@  @@
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoLayer::SdrUndoLayer(USHORT nLayerNum, SdrLayerAdmin& rNewLayerAdmin, SdrModel& rNewModel):
	SdrUndoAction(rNewModel),
	pLayer(rNewLayerAdmin.GetLayer(nLayerNum)),
	pLayerAdmin(&rNewLayerAdmin),
	nNum(nLayerNum),
	bItsMine(FALSE)
{
}

SdrUndoLayer::~SdrUndoLayer()
{
	if (bItsMine) {
		delete pLayer;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoNewLayer::Undo()
{
	DBG_ASSERT(!bItsMine,"SdrUndoNewLayer::Undo(): Layer gehoert bereits der UndoAction");
	bItsMine=TRUE;
#ifdef DBG_UTIL
	SdrLayer* pCmpLayer=
#endif
	pLayerAdmin->RemoveLayer(nNum);
	DBG_ASSERT(pCmpLayer==pLayer,"SdrUndoNewLayer::Undo(): Removter Layer ist != pLayer");
}

void SdrUndoNewLayer::Redo()
{
	DBG_ASSERT(bItsMine,"SdrUndoNewLayer::Undo(): Layer gehoert nicht der UndoAction");
	bItsMine=FALSE;
	pLayerAdmin->InsertLayer(pLayer,nNum);
}

XubString SdrUndoNewLayer::GetComment() const
{
	return ImpGetResStr(STR_UndoNewLayer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoDelLayer::Undo()
{
	DBG_ASSERT(bItsMine,"SdrUndoDelLayer::Undo(): Layer gehoert nicht der UndoAction");
	bItsMine=FALSE;
	pLayerAdmin->InsertLayer(pLayer,nNum);
}

void SdrUndoDelLayer::Redo()
{
	DBG_ASSERT(!bItsMine,"SdrUndoDelLayer::Undo(): Layer gehoert bereits der UndoAction");
	bItsMine=TRUE;
#ifdef DBG_UTIL
	SdrLayer* pCmpLayer=
#endif
	pLayerAdmin->RemoveLayer(nNum);
	DBG_ASSERT(pCmpLayer==pLayer,"SdrUndoDelLayer::Redo(): Removter Layer ist != pLayer");
}

XubString SdrUndoDelLayer::GetComment() const
{
	return ImpGetResStr(STR_UndoDelLayer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoMoveLayer::Undo()
{
#ifdef DBG_UTIL
	SdrLayer* pCmpLayer=
#endif
	pLayerAdmin->RemoveLayer(nNeuPos);
	DBG_ASSERT(pCmpLayer==pLayer,"SdrUndoMoveLayer::Undo(): Removter Layer ist != pLayer");
	pLayerAdmin->InsertLayer(pLayer,nNum);
}

void SdrUndoMoveLayer::Redo()
{
#ifdef DBG_UTIL
	SdrLayer* pCmpLayer=
#endif
	pLayerAdmin->RemoveLayer(nNum);
	DBG_ASSERT(pCmpLayer==pLayer,"SdrUndoMoveLayer::Redo(): Removter Layer ist != pLayer");
	pLayerAdmin->InsertLayer(pLayer,nNeuPos);
}

XubString SdrUndoMoveLayer::GetComment() const
{
	return ImpGetResStr(STR_UndoMovLayer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@@@@    @@@@    @@@@   @@@@@   @@@@
//  @@  @@  @@  @@  @@  @@  @@     @@  @@
//  @@  @@  @@  @@  @@      @@     @@
//  @@@@@   @@@@@@  @@ @@@  @@@@    @@@@
//  @@      @@  @@  @@  @@  @@         @@
//  @@      @@  @@  @@  @@  @@     @@  @@
//  @@      @@  @@   @@@@@  @@@@@   @@@@
//
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoPage::SdrUndoPage(SdrPage& rNewPg)
:	SdrUndoAction(*rNewPg.GetModel()),
	mrPage(rNewPg)
{
}

void SdrUndoPage::ImpInsertPage(USHORT nNum)
{
	DBG_ASSERT(!mrPage.IsInserted(),"SdrUndoPage::ImpInsertPage(): mrPage ist bereits Inserted");
	if (!mrPage.IsInserted()) {
		if (mrPage.IsMasterPage()) {
			rMod.InsertMasterPage(&mrPage,nNum);
		} else {
			rMod.InsertPage(&mrPage,nNum);
		}
	}
}

void SdrUndoPage::ImpRemovePage(USHORT nNum)
{
	DBG_ASSERT(mrPage.IsInserted(),"SdrUndoPage::ImpRemovePage(): mrPage ist nicht Inserted");
	if (mrPage.IsInserted()) {
		SdrPage* pChkPg=NULL;
		if (mrPage.IsMasterPage()) {
			pChkPg=rMod.RemoveMasterPage(nNum);
		} else {
			pChkPg=rMod.RemovePage(nNum);
		}
		DBG_ASSERT(pChkPg==&mrPage,"SdrUndoPage::ImpRemovePage(): RemovePage!=&mrPage");
	}
}

void SdrUndoPage::ImpMovePage(USHORT nOldNum, USHORT nNewNum)
{
	DBG_ASSERT(mrPage.IsInserted(),"SdrUndoPage::ImpMovePage(): mrPage ist nicht Inserted");
	if (mrPage.IsInserted()) {
		if (mrPage.IsMasterPage()) {
			rMod.MoveMasterPage(nOldNum,nNewNum);
		} else {
			rMod.MovePage(nOldNum,nNewNum);
		}
	}
}

void SdrUndoPage::ImpTakeDescriptionStr(USHORT nStrCacheID, XubString& rStr, USHORT /*n*/, FASTBOOL /*bRepeat*/) const
{
	rStr=ImpGetResStr(nStrCacheID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoPageList::SdrUndoPageList(SdrPage& rNewPg):
	SdrUndoPage(rNewPg),
	bItsMine(FALSE)
{
	nPageNum=rNewPg.GetPageNum();
}

SdrUndoPageList::~SdrUndoPageList()
{
	if(bItsMine)
	{
		delete (&mrPage);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoDelPage::SdrUndoDelPage(SdrPage& rNewPg):
	SdrUndoPageList(rNewPg),
	pUndoGroup(NULL)
{
	bItsMine = TRUE;

	// Und nun ggf. die MasterPage-Beziehungen merken
	if(mrPage.IsMasterPage())
	{
		sal_uInt16 nPageAnz(rMod.GetPageCount());

		for(sal_uInt16 nPageNum2(0); nPageNum2 < nPageAnz; nPageNum2++)
		{
			SdrPage* pDrawPage = rMod.GetPage(nPageNum2);

			if(pDrawPage->TRG_HasMasterPage())
			{
				SdrPage& rMasterPage = pDrawPage->TRG_GetMasterPage();

				if(&mrPage == &rMasterPage)
				{
					if(!pUndoGroup)
					{
						pUndoGroup = new SdrUndoGroup(rMod);
					}

					pUndoGroup->AddAction(rMod.GetSdrUndoFactory().CreateUndoPageRemoveMasterPage(*pDrawPage));
				}
			}
		}
	}
}

SdrUndoDelPage::~SdrUndoDelPage()
{
	if (pUndoGroup!=NULL) {
		delete pUndoGroup;
	}
}

void SdrUndoDelPage::Undo()
{
	ImpInsertPage(nPageNum);
	if (pUndoGroup!=NULL) { // MasterPage-Beziehungen wiederherstellen
		pUndoGroup->Undo();
	}
	DBG_ASSERT(bItsMine,"UndoDeletePage: mrPage gehoert nicht der UndoAction");
	bItsMine=FALSE;
}

void SdrUndoDelPage::Redo()
{
	ImpRemovePage(nPageNum);
	// Die MasterPage-Beziehungen werden ggf. von selbst geloesst
	DBG_ASSERT(!bItsMine,"RedoDeletePage: mrPage gehoert bereits der UndoAction");
	bItsMine=TRUE;
}

XubString SdrUndoDelPage::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoDelPage,aStr,0,FALSE);
	return aStr;
}

XubString SdrUndoDelPage::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoDelPage,aStr,0,FALSE);
	return aStr;
}

void SdrUndoDelPage::SdrRepeat(SdrView& /*rView*/)
{
}

bool SdrUndoDelPage::CanSdrRepeat(SdrView& /*rView*/) const
{
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoNewPage::Undo()
{
	ImpRemovePage(nPageNum);
	DBG_ASSERT(!bItsMine,"UndoNewPage: mrPage gehoert bereits der UndoAction");
	bItsMine=TRUE;
}

void SdrUndoNewPage::Redo()
{
	ImpInsertPage(nPageNum);
	DBG_ASSERT(bItsMine,"RedoNewPage: mrPage gehoert nicht der UndoAction");
	bItsMine=FALSE;
}

XubString SdrUndoNewPage::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoNewPage,aStr,0,FALSE);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

XubString SdrUndoCopyPage::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoCopPage,aStr,0,FALSE);
	return aStr;
}

XubString SdrUndoCopyPage::GetSdrRepeatComment(SdrView& /*rView*/) const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoCopPage,aStr,0,FALSE);
	return aStr;
}

void SdrUndoCopyPage::SdrRepeat(SdrView& /*rView*/)
{

}

bool SdrUndoCopyPage::CanSdrRepeat(SdrView& /*rView*/) const
{
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrUndoSetPageNum::Undo()
{
	ImpMovePage(nNewPageNum,nOldPageNum);
}

void SdrUndoSetPageNum::Redo()
{
	ImpMovePage(nOldPageNum,nNewPageNum);
}

XubString SdrUndoSetPageNum::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoMovPage,aStr,0,FALSE);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@   @@  @@@@   @@@@  @@@@@@ @@@@@ @@@@@   @@@@@   @@@@   @@@@  @@@@@  @@@@
//  @@@ @@@ @@  @@ @@  @@   @@   @@    @@  @@  @@  @@ @@  @@ @@  @@ @@    @@  @@
//  @@@@@@@ @@  @@ @@       @@   @@    @@  @@  @@  @@ @@  @@ @@     @@    @@
//  @@@@@@@ @@@@@@  @@@@    @@   @@@@  @@@@@   @@@@@  @@@@@@ @@ @@@ @@@@   @@@@
//  @@ @ @@ @@  @@     @@   @@   @@    @@  @@  @@     @@  @@ @@  @@ @@        @@
//  @@   @@ @@  @@ @@  @@   @@   @@    @@  @@  @@     @@  @@ @@  @@ @@    @@  @@
//  @@   @@ @@  @@  @@@@    @@   @@@@@ @@  @@  @@     @@  @@  @@@@@ @@@@@  @@@@
//
////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoPageMasterPage::SdrUndoPageMasterPage(SdrPage& rChangedPage)
:	SdrUndoPage(rChangedPage),
	mbOldHadMasterPage(mrPage.TRG_HasMasterPage())
{
	// get current state from page
	if(mbOldHadMasterPage)
	{
		maOldSet = mrPage.TRG_GetMasterPageVisibleLayers();
		maOldMasterPageNumber = mrPage.TRG_GetMasterPage().GetPageNum();
	}
}

SdrUndoPageMasterPage::~SdrUndoPageMasterPage()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoPageRemoveMasterPage::SdrUndoPageRemoveMasterPage(SdrPage& rChangedPage)
:	SdrUndoPageMasterPage(rChangedPage)
{
}

void SdrUndoPageRemoveMasterPage::Undo()
{
	if(mbOldHadMasterPage)
	{
		mrPage.TRG_SetMasterPage(*mrPage.GetModel()->GetMasterPage(maOldMasterPageNumber));
		mrPage.TRG_SetMasterPageVisibleLayers(maOldSet);
	}
}

void SdrUndoPageRemoveMasterPage::Redo()
{
	mrPage.TRG_ClearMasterPage();
}

XubString SdrUndoPageRemoveMasterPage::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoDelPageMasterDscr,aStr,0,FALSE);
	return aStr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SdrUndoPageChangeMasterPage::SdrUndoPageChangeMasterPage(SdrPage& rChangedPage)
:	SdrUndoPageMasterPage(rChangedPage),
	mbNewHadMasterPage(sal_False)
{
}

void SdrUndoPageChangeMasterPage::Undo()
{
	// remember values from new page
	if(mrPage.TRG_HasMasterPage())
	{
		mbNewHadMasterPage = sal_True;
		maNewSet = mrPage.TRG_GetMasterPageVisibleLayers();
		maNewMasterPageNumber = mrPage.TRG_GetMasterPage().GetPageNum();
	}

	// restore old values
	if(mbOldHadMasterPage)
	{
		mrPage.TRG_ClearMasterPage();
		mrPage.TRG_SetMasterPage(*mrPage.GetModel()->GetMasterPage(maOldMasterPageNumber));
		mrPage.TRG_SetMasterPageVisibleLayers(maOldSet);
	}
}

void SdrUndoPageChangeMasterPage::Redo()
{
	// restore new values
	if(mbNewHadMasterPage)
	{
		mrPage.TRG_ClearMasterPage();
		mrPage.TRG_SetMasterPage(*mrPage.GetModel()->GetMasterPage(maNewMasterPageNumber));
		mrPage.TRG_SetMasterPageVisibleLayers(maNewSet);
	}
}

XubString SdrUndoPageChangeMasterPage::GetComment() const
{
	XubString aStr;
	ImpTakeDescriptionStr(STR_UndoChgPageMasterDscr,aStr,0,FALSE);
	return aStr;
}

///////////////////////////////////////////////////////////////////////
SdrUndoFactory::~SdrUndoFactory(){}
// shapes
SdrUndoAction* SdrUndoFactory::CreateUndoMoveObject( SdrObject& rObject )
{
	return new SdrUndoMoveObj( rObject );
}

SdrUndoAction* SdrUndoFactory::CreateUndoMoveObject( SdrObject& rObject, const Size& rDist )
{
	return new SdrUndoMoveObj( rObject, rDist );
}

SdrUndoAction* SdrUndoFactory::CreateUndoGeoObject( SdrObject& rObject )
{
	return new SdrUndoGeoObj( rObject );
}

SdrUndoAction* SdrUndoFactory::CreateUndoAttrObject( SdrObject& rObject, bool bStyleSheet1, bool bSaveText )
{
	return new SdrUndoAttrObj( rObject, bStyleSheet1 ? TRUE : FALSE, bSaveText ? TRUE : FALSE );
}

SdrUndoAction* SdrUndoFactory::CreateUndoRemoveObject( SdrObject& rObject, bool bOrdNumDirect )
{
	return new SdrUndoRemoveObj( rObject, bOrdNumDirect ? TRUE : FALSE );
}

SdrUndoAction* SdrUndoFactory::CreateUndoInsertObject( SdrObject& rObject, bool bOrdNumDirect )
{
	return new SdrUndoInsertObj( rObject, bOrdNumDirect ? TRUE : FALSE );
}

SdrUndoAction* SdrUndoFactory::CreateUndoDeleteObject( SdrObject& rObject, bool bOrdNumDirect )
{
	return new SdrUndoDelObj( rObject, bOrdNumDirect ? TRUE : FALSE );
}

SdrUndoAction* SdrUndoFactory::CreateUndoNewObject( SdrObject& rObject, bool bOrdNumDirect )
{
	return new SdrUndoNewObj( rObject, bOrdNumDirect ? TRUE : FALSE );
}

SdrUndoAction* SdrUndoFactory::CreateUndoCopyObject( SdrObject& rObject, bool bOrdNumDirect )
{
	return new SdrUndoCopyObj( rObject, bOrdNumDirect ? TRUE : FALSE );
}

SdrUndoAction* SdrUndoFactory::CreateUndoObjectOrdNum( SdrObject& rObject, sal_uInt32 nOldOrdNum1, sal_uInt32 nNewOrdNum1)
{
	return new SdrUndoObjOrdNum( rObject, nOldOrdNum1, nNewOrdNum1 );
}

SdrUndoAction* SdrUndoFactory::CreateUndoReplaceObject( SdrObject& rOldObject, SdrObject& rNewObject, bool bOrdNumDirect )
{
	return new SdrUndoReplaceObj( rOldObject, rNewObject, bOrdNumDirect ? TRUE : FALSE );
}

SdrUndoAction* SdrUndoFactory::CreateUndoObjectLayerChange( SdrObject& rObject, SdrLayerID aOldLayer, SdrLayerID aNewLayer )
{
	return new SdrUndoObjectLayerChange( rObject, aOldLayer, aNewLayer );
}

SdrUndoAction* SdrUndoFactory::CreateUndoObjectSetText( SdrObject& rNewObj, sal_Int32 nText )
{
	return new SdrUndoObjSetText( rNewObj, nText );
}

SdrUndoAction* SdrUndoFactory::CreateUndoObjectStrAttr( SdrObject& rObject,
                                                        SdrUndoObjStrAttr::ObjStrAttrType eObjStrAttrType,
                                                        String sOldStr,
                                                        String sNewStr )
{
    return new SdrUndoObjStrAttr( rObject, eObjStrAttrType, sOldStr, sNewStr );
}


// layer
SdrUndoAction* SdrUndoFactory::CreateUndoNewLayer(sal_uInt16 nLayerNum, SdrLayerAdmin& rNewLayerAdmin, SdrModel& rNewModel)
{
	return new SdrUndoNewLayer( nLayerNum, rNewLayerAdmin, rNewModel );
}

SdrUndoAction* SdrUndoFactory::CreateUndoDeleteLayer(sal_uInt16 nLayerNum, SdrLayerAdmin& rNewLayerAdmin, SdrModel& rNewModel)
{
	return new SdrUndoDelLayer( nLayerNum, rNewLayerAdmin, rNewModel );
}

SdrUndoAction* SdrUndoFactory::CreateUndoMoveLayer(sal_uInt16 nLayerNum, SdrLayerAdmin& rNewLayerAdmin, SdrModel& rNewModel, sal_uInt16 nNeuPos1)
{
	return new SdrUndoMoveLayer( nLayerNum, rNewLayerAdmin, rNewModel, nNeuPos1 );
}

// page
SdrUndoAction*	SdrUndoFactory::CreateUndoDeletePage(SdrPage& rPage)
{
	return new SdrUndoDelPage( rPage );
}

SdrUndoAction* SdrUndoFactory::CreateUndoNewPage(SdrPage& rPage)
{
	return new SdrUndoNewPage( rPage );
}

SdrUndoAction* SdrUndoFactory::CreateUndoCopyPage(SdrPage& rPage)
{
	return new SdrUndoCopyPage( rPage );
}

SdrUndoAction* SdrUndoFactory::CreateUndoSetPageNum(SdrPage& rNewPg, sal_uInt16 nOldPageNum1, sal_uInt16 nNewPageNum1)
{
	return new SdrUndoSetPageNum( rNewPg, nOldPageNum1, nNewPageNum1 );
}
	// master page
SdrUndoAction* SdrUndoFactory::CreateUndoPageRemoveMasterPage(SdrPage& rChangedPage)
{
	return new SdrUndoPageRemoveMasterPage( rChangedPage );
}

SdrUndoAction* SdrUndoFactory::CreateUndoPageChangeMasterPage(SdrPage& rChangedPage)
{
	return new SdrUndoPageChangeMasterPage(rChangedPage);
}

// eof
