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

#ifdef SD_DLLIMPLEMENTATION
#undef SD_DLLIMPLEMENTATION
#endif

#include "OutlineBulletDlg.hxx"

#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include <sfx2/objsh.hxx>
#include <svx/drawitem.hxx>
#include <svx/bulitem.hxx>
#include <svx/eeitem.hxx>

#include <svx/numitem.hxx>

#include <svx/dialogs.hrc>
#include <svtools/intitem.hxx>
#include <svx/svdmark.hxx>
#include "View.hxx"
#include <svx/svdobj.hxx>
#include <svtools/style.hxx>
#include <drawdoc.hxx>

#ifndef _SD_SDRESID_HXX
#include "sdresid.hxx"
#endif

#include "glob.hrc"
#include "dlgolbul.hrc"
#include "bulmaper.hxx"
#include "DrawDocShell.hxx"
#include <svx/svxids.hrc>
#include <svtools/aeitem.hxx>

namespace sd {

/*************************************************************************
|*
|* Konstruktor des Tab-Dialogs: Fuegt die Seiten zum Dialog hinzu
|*
\************************************************************************/

OutlineBulletDlg::OutlineBulletDlg(
    ::Window* pParent,
    const SfxItemSet* pAttr,
    ::sd::View* pView )
    : SfxTabDialog	( pParent, SdResId(TAB_OUTLINEBULLET) ),
      aInputSet		( *pAttr ),
      bTitle			( FALSE ),
      pSdView			( pView )
{
	FreeResource();

	aInputSet.MergeRange( SID_PARAM_NUM_PRESET, SID_PARAM_CUR_NUM_LEVEL );
	aInputSet.Put( *pAttr );

	pOutputSet = new SfxItemSet( *pAttr );
	pOutputSet->ClearItem();

	BOOL bOutliner = FALSE;

	// Sonderbehandlung wenn eine Title Objekt selektiert wurde
	if( pView )
	{
		const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
		const ULONG nCount = rMarkList.GetMarkCount();
		for(ULONG nNum = 0; nNum < nCount; nNum++)
		{
			SdrObject* pObj = rMarkList.GetMark(nNum)->GetMarkedSdrObj();
			if( pObj->GetObjInventor() == SdrInventor )
			{

				switch(pObj->GetObjIdentifier())
				{
				case OBJ_TITLETEXT:
					bTitle = TRUE;
					break;
				case OBJ_OUTLINETEXT:
					bOutliner = TRUE;
					break;
				}
			}
		}
	}

	if( SFX_ITEM_SET != aInputSet.GetItemState(EE_PARA_NUMBULLET))
	{
		const SvxNumBulletItem *pItem = NULL;
		if(bOutliner)
		{
			SfxStyleSheetBasePool* pSSPool = pView->GetDocSh()->GetStyleSheetPool();
			String aStyleName((SdResId(STR_LAYOUT_OUTLINE)));
			aStyleName.AppendAscii( RTL_CONSTASCII_STRINGPARAM( " 1" ) );
			SfxStyleSheetBase* pFirstStyleSheet = pSSPool->Find( aStyleName, SD_STYLE_FAMILY_PSEUDO);
			if( pFirstStyleSheet )
				pFirstStyleSheet->GetItemSet().GetItemState(EE_PARA_NUMBULLET, FALSE, (const SfxPoolItem**)&pItem);
		}

		if( pItem == NULL )
			pItem = (SvxNumBulletItem*) aInputSet.GetPool()->GetSecondaryPool()->GetPoolDefaultItem(EE_PARA_NUMBULLET);

		DBG_ASSERT( pItem, "Kein EE_PARA_NUMBULLET im Pool! [CL]" );

		aInputSet.Put(*pItem, EE_PARA_NUMBULLET);
	}

	/* debug
	if( SFX_ITEM_SET == aInputSet.GetItemState(EE_PARA_NUMBULLET, FALSE, &pItem ))
	{
		SvxNumRule& rItem = *((SvxNumBulletItem*)pItem)->GetNumRule();
		for( int i = 0; i < 9; i++ )
		{
			SvxNumberFormat aNumberFormat = rItem.GetLevel(i);
		}
	}
	*/

	if(bTitle && aInputSet.GetItemState(EE_PARA_NUMBULLET,TRUE) == SFX_ITEM_ON )
	{
		SvxNumBulletItem* pItem = (SvxNumBulletItem*)aInputSet.GetItem(EE_PARA_NUMBULLET,TRUE);
		SvxNumRule* pRule = pItem->GetNumRule();
		if(pRule)
		{
			SvxNumRule aNewRule( *pRule );
			aNewRule.SetFeatureFlag( NUM_NO_NUMBERS, TRUE );

			SvxNumBulletItem aNewItem( aNewRule, EE_PARA_NUMBULLET );
			aInputSet.Put(aNewItem);
		}
	}

	SetInputSet( &aInputSet );

	if(!bTitle)
		AddTabPage(RID_SVXPAGE_PICK_SINGLE_NUM);
	else
		RemoveTabPage( RID_SVXPAGE_PICK_SINGLE_NUM );

	AddTabPage( RID_SVXPAGE_PICK_BULLET  );
	AddTabPage( RID_SVXPAGE_PICK_BMP   );
	AddTabPage(RID_SVXPAGE_NUM_OPTIONS 	);
	AddTabPage(RID_SVXPAGE_NUM_POSITION	);

}

OutlineBulletDlg::~OutlineBulletDlg()
{
	delete pOutputSet;
}

void OutlineBulletDlg::PageCreated( USHORT nId, SfxTabPage &rPage )
{
	switch ( nId )
	{
		case RID_SVXPAGE_NUM_OPTIONS:
		{
			if( pSdView )
			{
				FieldUnit eMetric = pSdView->GetDoc()->GetUIUnit();
				SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
				aSet.Put ( SfxAllEnumItem(SID_METRIC_ITEM,(USHORT)eMetric));
				rPage.PageCreated(aSet);
			}
		}
		break;
		case RID_SVXPAGE_NUM_POSITION:
		{
			if( pSdView )
			{
				FieldUnit eMetric = pSdView->GetDoc()->GetUIUnit();
				SfxAllItemSet aSet(*(GetInputSetImpl()->GetPool()));
				aSet.Put ( SfxAllEnumItem(SID_METRIC_ITEM,(USHORT)eMetric));
				rPage.PageCreated(aSet);
			}
		}
		break;
	}
}

const SfxItemSet* OutlineBulletDlg::GetOutputItemSet() const
{
	SfxItemSet aSet( *SfxTabDialog::GetOutputItemSet() );
	pOutputSet->Put( aSet );

	const SfxPoolItem *pItem = NULL;
	if( SFX_ITEM_SET == pOutputSet->GetItemState(pOutputSet->GetPool()->GetWhich(SID_ATTR_NUMBERING_RULE), FALSE, &pItem ))
	{
		SdBulletMapper::MapFontsInNumRule( *((SvxNumBulletItem*)pItem)->GetNumRule(), *pOutputSet );

/* #i35937#
		SfxUInt16Item aBulletState( EE_PARA_BULLETSTATE, 1 );
		pOutputSet->Put(aBulletState);
*/
	}

/* #i35937#
	SdBulletMapper::PostMapNumBulletForDialog( *pOutputSet );
*/

	if(bTitle && pOutputSet->GetItemState(EE_PARA_NUMBULLET,TRUE) == SFX_ITEM_ON )
	{
		SvxNumBulletItem* pBulletItem = (SvxNumBulletItem*)pOutputSet->GetItem(EE_PARA_NUMBULLET,TRUE);
		SvxNumRule* pRule = pBulletItem->GetNumRule();
		if(pRule)
			pRule->SetFeatureFlag( NUM_NO_NUMBERS, FALSE );
	}

	return pOutputSet;
}

} // end of namespace sd
