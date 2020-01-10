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
#include "precompiled_sfx2.hxx"

#include <stdlib.h>
#include <tools/rcid.h>
#ifndef GCC
#endif
#include <tools/stream.hxx>

#include <sfx2/module.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/msg.hxx>
#include <sfx2/app.hxx>
#include <sfx2/msgpool.hxx>
#include "sfxresid.hxx"
#include <sfx2/minarray.hxx>
#include <sfx2/objsh.hxx>

DBG_NAME(SfxInterface)

//====================================================================

EXTERN_C
#if defined( PM2 ) && (!defined( CSET ) && !defined ( MTW ) && !defined( WTC ))
int _stdcall
#else
#ifdef WNT
int _cdecl
#else
int
#endif
#endif

SfxCompareSlots_Impl( const void* pSmaller, const void* pBigger )
{
	DBG_MEMTEST();
	return ( (int) ((SfxSlot*)pSmaller)->GetSlotId() ) -
		   ( (int) ((SfxSlot*)pBigger)->GetSlotId() );
}

//=========================================================================

struct SfxObjectUI_Impl
{
	USHORT	nPos;
	ResId	aResId;
	BOOL	bVisible;
	BOOL	bContext;
	String* pName;
	sal_uInt32	nFeature;

	SfxObjectUI_Impl(USHORT n, const ResId& rResId, BOOL bVis, sal_uInt32 nFeat) :
		nPos(n),
		aResId(rResId.GetId(), *rResId.GetResMgr()),
		bVisible(bVis),
		bContext(FALSE),
		pName(0),
		nFeature(nFeat)
	{
		aResId.SetRT(rResId.GetRT());
	}

	~SfxObjectUI_Impl()
	{
		delete pName;
	}
};

DECL_PTRARRAY(SfxObjectUIArr_Impl, SfxObjectUI_Impl*, 2, 2)

struct SfxInterface_Impl
{
	SfxObjectUIArr_Impl*	pObjectBars;	// registered ObjectBars
	SfxObjectUIArr_Impl*	pChildWindows;	// registered ChildWindows
	ResId					aPopupRes;		// registered PopupMenu
	ResId					aStatBarRes;	// registered StatusBar
	SfxModule*				pModule;
    BOOL                    bRegistered;

	SfxInterface_Impl() :
		aPopupRes(0,*SfxApplication::GetOrCreate()->GetSfxResManager()),
		aStatBarRes(0,*SfxApplication::GetOrCreate()->GetSfxResManager())
    , bRegistered(FALSE)
	{
		pObjectBars   = new SfxObjectUIArr_Impl;
		pChildWindows = new SfxObjectUIArr_Impl;
	}

	~SfxInterface_Impl()
	{
		USHORT n;
		for (n=0; n<pObjectBars->Count(); n++)
			delete (*pObjectBars)[n];
		delete pObjectBars;

		for (n=0; n<pChildWindows->Count(); n++)
			delete (*pChildWindows)[n];
		delete pChildWindows;
	}
};

static SfxObjectUI_Impl* CreateObjectBarUI_Impl( USHORT nPos, const ResId& rResId, sal_uInt32 nFeature, const String *pStr );

//====================================================================

//====================================================================
// ctor, registeres a new unit

SfxInterface::SfxInterface( const char *pClassName,
							const ResId& rNameResId,
							SfxInterfaceId nId,
							const SfxInterface* pParent,
							SfxSlot &rSlotMap, USHORT nSlotCount ):
	pName(pClassName),
	pGenoType(pParent),
	nClassId(nId),
	aNameResId(rNameResId.GetId(),*rNameResId.GetResMgr()),
	pImpData(0)
{
	pImpData = new SfxInterface_Impl;
	SetSlotMap( rSlotMap, nSlotCount );
}

void SfxInterface::Register( SfxModule* pMod )
{
    pImpData->bRegistered = TRUE;
	pImpData->pModule = pMod;
    if ( pMod )
		pMod->GetSlotPool()->RegisterInterface(*this);
	else
        SFX_APP()->GetAppSlotPool_Impl().RegisterInterface(*this);
}

void SfxInterface::SetSlotMap( SfxSlot& rSlotMap, USHORT nSlotCount )
{
	pSlots = &rSlotMap;
	nCount = nSlotCount;
	SfxSlot* pIter = pSlots;
	if ( 1 == nCount && !pIter->pNextSlot )
		pIter->pNextSlot = pIter;

	if ( !pIter->pNextSlot )
	{
		// sort the SfxSlots by id
		qsort( pSlots, nCount, sizeof(SfxSlot), SfxCompareSlots_Impl );

		// link masters and slaves
		USHORT nIter = 1;
		for ( pIter = pSlots; nIter <= nCount; ++pIter, ++nIter )
		{
			//! hier bitte sinnvoll pruefen
			//! DBG_ASSERT(!(pIter->IsMode(SFX_SLOT_CACHABLE) &&
			//!                 pIter->IsMode(SFX_SLOT_VOLATILE)),
			//!             "invalid Flags" );
			DBG_ASSERT( nIter == nCount ||
						pIter->GetSlotId() != (pIter+1)->GetSlotId(),
						"doppelte SID" );

			// jeder Master verweist auf seinen ersten Slave (ENUM), alle
			// Slaves auf ihren Master.
			// Slaves verweisen im Ring auf die anderen mit gleichem Master
			if ( pIter->GetKind() == SFX_KIND_ENUM )
			{
				pIter->pLinkedSlot = GetSlot( pIter->nMasterSlotId );
				DBG_ASSERT( pIter->pLinkedSlot, "slave without master" );
				if ( !pIter->pLinkedSlot->pLinkedSlot )
					( (SfxSlot*) pIter->pLinkedSlot)->pLinkedSlot = pIter;

				if ( 0 == pIter->GetNextSlot() )
				{
					SfxSlot *pLastSlot = pIter;
					for ( USHORT n = nIter; n < Count(); ++n )
					{
						SfxSlot *pCurSlot = (pSlots+n);
						if ( pCurSlot->nMasterSlotId == pIter->nMasterSlotId )
						{
							pLastSlot->pNextSlot = pCurSlot;
							pLastSlot = pCurSlot;
						}
					}
					pLastSlot->pNextSlot = pIter;
				}
			}
			else if ( 0 == pIter->GetNextSlot() )
			{
				// Slots verweisen im Ring auf den n"achten mit derselben Statusmethode
				SfxSlot *pLastSlot = pIter;
				for ( USHORT n = nIter; n < Count(); ++n )
				{
					SfxSlot *pCurSlot = (pSlots+n);
					if ( pCurSlot->GetStateFnc() == pIter->GetStateFnc() )
					{
						pLastSlot->pNextSlot = pCurSlot;
						pLastSlot = pCurSlot;
					}
				}
				pLastSlot->pNextSlot = pIter;
			}
		}
	}
#ifdef DBG_UTIL
	else
	{
		USHORT nIter = 1;
		for ( SfxSlot *pNext = pIter+1; nIter < nCount; ++pNext, ++nIter )
		{

			if ( pNext->GetSlotId() <= pIter->GetSlotId() )
				DBG_ERROR ("Falsche Reihenfolge!");

			if ( pIter->GetKind() == SFX_KIND_ENUM )
			{
				const SfxSlot *pMasterSlot = GetSlot(pIter->nMasterSlotId);
				const SfxSlot *pFirstSlave = pMasterSlot->pLinkedSlot;
				const SfxSlot *pSlave = pFirstSlave;
				do
				{
					if ( pSlave->pLinkedSlot != pMasterSlot )
					{
						ByteString aStr("Falsche Master/Slave-Verkettung : ");
						aStr += ByteString::CreateFromInt32(pMasterSlot->GetSlotId());
						aStr += " , ";
						aStr += ByteString::CreateFromInt32(pSlave->GetSlotId());
						DBG_ERROR(aStr.GetBuffer());
					}

					if ( pSlave->nMasterSlotId != pMasterSlot->GetSlotId() )
					{
						ByteString aStr("Falsche Master/Slave-Ids : ");
						aStr += ByteString::CreateFromInt32(pMasterSlot->GetSlotId());
						aStr += " , ";
						aStr += ByteString::CreateFromInt32(pSlave->GetSlotId());
						DBG_ERROR(aStr.GetBuffer());
					}

					pSlave = pSlave->pNextSlot;
				}
				while ( pSlave != pFirstSlave );
			}
			else
			{
				if ( pIter->pLinkedSlot )
				{
					if ( pIter->pLinkedSlot->GetKind() != SFX_KIND_ENUM )
					{
						ByteString aStr("Slave ist kein enum : ");
						aStr += ByteString::CreateFromInt32(pIter->GetSlotId());
						aStr += " , ";
						aStr += ByteString::CreateFromInt32(pIter->pLinkedSlot->GetSlotId());
						DBG_ERROR(aStr.GetBuffer());
					}
				}

				const SfxSlot *pCurSlot = pIter;
				do
				{
					pCurSlot = pCurSlot->pNextSlot;
					if ( pCurSlot->GetStateFnc() != pIter->GetStateFnc() )
					{
						ByteString aStr("Verkettete Slots mit verschiedenen StateMethods : ");
						aStr += ByteString::CreateFromInt32(pCurSlot->GetSlotId());
						aStr += " , ";
						aStr += ByteString::CreateFromInt32(pIter->GetSlotId());
						DBG_ERROR(aStr.GetBuffer());
					}
				}
				while ( pCurSlot != pIter );
			}

			pIter = pNext;
		}
	}
#endif
}


//--------------------------------------------------------------------



SfxInterface::~SfxInterface()
{
	SfxModule *pMod = pImpData->pModule;
    BOOL bRegistered = pImpData->bRegistered;
	delete pImpData;
    DBG_ASSERT( bRegistered, "Interface not registered!" );
    if ( bRegistered )
	{
		if ( pMod )
			pMod->GetSlotPool()->ReleaseInterface(*this);
		else
			SFX_APP()->GetAppSlotPool_Impl().ReleaseInterface(*this);
	}
}

//--------------------------------------------------------------------

// searches for the specified func


const SfxSlot* SfxInterface::GetSlot( USHORT nFuncId ) const
{
	DBG_MEMTEST();
	DBG_CHKTHIS(SfxInterface, 0);
	DBG_ASSERT( this && pSlots && nCount, "" );

	// find the id using binary search
	void* p = bsearch( &nFuncId, pSlots, nCount, sizeof(SfxSlot),
					   SfxCompareSlots_Impl );
	if ( !p && pGenoType )
		return pGenoType->GetSlot( nFuncId );

	return p ? (const SfxSlot*)p : 0;
}

const SfxSlot* SfxInterface::GetSlot( const String& rCommand ) const
{
    static const char UNO_COMMAND[] = ".uno:";

    String aCommand( rCommand );
    if ( aCommand.SearchAscii( UNO_COMMAND ) == 0 )
         aCommand.Erase( 0, sizeof( UNO_COMMAND )-1 );

    for ( USHORT n=0; n<nCount; n++ )
    {
        if ( (pSlots+n)->pUnoName && 
             aCommand.CompareIgnoreCaseToAscii( (pSlots+n)->GetUnoName() ) == COMPARE_EQUAL )
            return pSlots+n;
    }

    return pGenoType ? pGenoType->GetSlot( aCommand ) : NULL;
}

//--------------------------------------------------------------------


const SfxSlot* SfxInterface::GetRealSlot( const SfxSlot *pSlot ) const
{
	DBG_MEMTEST();
	DBG_CHKTHIS(SfxInterface, 0);
	DBG_ASSERT( this && pSlots && nCount, "" );

	if ( !ContainsSlot_Impl(pSlot) )
	{
		if(pGenoType)
			return pGenoType->GetRealSlot(pSlot);
		DBG_ERROR("fremder Slot");
		return 0;
	}

	return pSlot->pLinkedSlot;
}

//--------------------------------------------------------------------


const SfxSlot* SfxInterface::GetRealSlot( USHORT nSlotId ) const
{
	DBG_MEMTEST();
	DBG_CHKTHIS(SfxInterface, 0);
	DBG_ASSERT( this && pSlots && nCount, "" );

	const SfxSlot *pSlot = GetSlot(nSlotId);
	if ( !pSlot )
	{
		if(pGenoType)
			return pGenoType->GetRealSlot(nSlotId);
		DBG_ERROR("fremder Slot");
		return 0;
	}

	return pSlot->pLinkedSlot;
}

//--------------------------------------------------------------------


void SfxInterface::RegisterPopupMenu( const ResId& rResId )
{
	DBG_CHKTHIS(SfxInterface, 0);
	pImpData->aPopupRes = rResId;
}

//--------------------------------------------------------------------

void SfxInterface::RegisterObjectBar( USHORT nPos, const ResId& rResId,
		const String *pStr )
{
	RegisterObjectBar( nPos, rResId, 0UL, pStr );
}


void SfxInterface::RegisterObjectBar( USHORT nPos, const ResId& rResId, sal_uInt32 nFeature, const String *pStr )
{
    SfxObjectUI_Impl* pUI = CreateObjectBarUI_Impl( nPos, rResId, nFeature, pStr );
    if ( pUI )
        pImpData->pObjectBars->Append(pUI);
}

SfxObjectUI_Impl* CreateObjectBarUI_Impl( USHORT nPos, const ResId& rResId, sal_uInt32 nFeature, const String *pStr )
{
	if ((nPos & SFX_VISIBILITY_MASK) == 0)
        nPos |= SFX_VISIBILITY_STANDARD;

	SfxObjectUI_Impl* pUI = new SfxObjectUI_Impl(nPos, rResId, TRUE, nFeature);

	if (pStr == 0)
	{
		ResId aResId(rResId);
		aResId.SetRT(RSC_STRING);
		aResId.SetResMgr(rResId.GetResMgr());
        if( ! aResId.GetResMgr() )
            aResId.SetResMgr( SfxApplication::GetOrCreate()->GetOffResManager_Impl() );
		if ( !aResId.GetResMgr()->IsAvailable(aResId) )
			pUI->pName = new String (DEFINE_CONST_UNICODE("NoName"));
		else
			pUI->pName = new String(aResId);
	}
	else
		pUI->pName = new String(*pStr);

    return pUI;
}

const ResId& SfxInterface::GetObjectBarResId( USHORT nNo ) const
{
	BOOL bGenoType = (pGenoType != 0 && !pGenoType->HasName());
	if ( bGenoType )
	{
		// Gibt es Toolbars in der Superklasse ?
		USHORT nBaseCount = pGenoType->GetObjectBarCount();
		if ( nNo < nBaseCount )
			// Die der Superklasse kommen zuerst
			return pGenoType->GetObjectBarResId( nNo );
		else
			nNo = nNo - nBaseCount;
	}

#ifdef DBG_UTIL
	USHORT nObjBarCount = pImpData->pObjectBars->Count();
	DBG_ASSERT( nNo<nObjBarCount,"Objectbar ist unbekannt!" );
#endif
	return (*pImpData->pObjectBars)[nNo]->aResId;
}

//--------------------------------------------------------------------


USHORT SfxInterface::GetObjectBarPos( USHORT nNo ) const
{
	BOOL bGenoType = (pGenoType != 0 && !pGenoType->HasName());
	if ( bGenoType )
	{
		// Gibt es Toolbars in der Superklasse ?
		USHORT nBaseCount = pGenoType->GetObjectBarCount();
		if ( nNo < nBaseCount )
			// Die der Superklasse kommen zuerst
			return pGenoType->GetObjectBarPos( nNo );
		else
			nNo = nNo - nBaseCount;
	}

#ifdef DBG_UTIL
	USHORT nObjBarCount = pImpData->pObjectBars->Count();
	DBG_ASSERT( nNo<nObjBarCount,"Objectbar ist unbekannt!" );
#endif
	return (*pImpData->pObjectBars)[nNo]->nPos;
}

//--------------------------------------------------------------------


USHORT SfxInterface::GetObjectBarCount() const
{
	if (pGenoType && ! pGenoType->HasName())
		return pImpData->pObjectBars->Count() + pGenoType->GetObjectBarCount();
	else
		return pImpData->pObjectBars->Count();
}

//--------------------------------------------------------------------
void SfxInterface::RegisterChildWindow(USHORT nId, BOOL bContext, const String* pChildWinName)
{
	RegisterChildWindow( nId, bContext, 0UL, pChildWinName );
}

void SfxInterface::RegisterChildWindow(USHORT nId, BOOL bContext, sal_uInt32 nFeature, const String*)
{
	SfxObjectUI_Impl* pUI = new SfxObjectUI_Impl(0, ResId(nId, *SfxApplication::GetOrCreate()->GetOffResManager_Impl()), TRUE, nFeature);
	pUI->bContext = bContext;
	pImpData->pChildWindows->Append(pUI);
}

void SfxInterface::RegisterStatusBar(const ResId& rResId)
{
	pImpData->aStatBarRes = rResId;
}


sal_uInt32 SfxInterface::GetChildWindowId (USHORT nNo) const
{
	if ( pGenoType )
	{
		// Gibt es ChildWindows in der Superklasse ?
		USHORT nBaseCount = pGenoType->GetChildWindowCount();
		if ( nNo < nBaseCount )
			// Die der Superklasse kommen zuerst
			return pGenoType->GetChildWindowId( nNo );
		else
			nNo = nNo - nBaseCount;
	}

#ifdef DBG_UTIL
	USHORT nCWCount = pImpData->pChildWindows->Count();
	DBG_ASSERT( nNo<nCWCount,"ChildWindow ist unbekannt!" );
#endif
	sal_uInt32 nRet = (*pImpData->pChildWindows)[nNo]->aResId.GetId();
	if ( (*pImpData->pChildWindows)[nNo]->bContext )
		nRet += sal_uInt32( nClassId ) << 16;
	return nRet;
}

sal_uInt32 SfxInterface::GetChildWindowFeature (USHORT nNo) const
{
	if ( pGenoType )
	{
		// Gibt es ChildWindows in der Superklasse ?
		USHORT nBaseCount = pGenoType->GetChildWindowCount();
		if ( nNo < nBaseCount )
			// Die der Superklasse kommen zuerst
			return pGenoType->GetChildWindowFeature( nNo );
		else
			nNo = nNo - nBaseCount;
	}

#ifdef DBG_UTIL
	USHORT nCWCount = pImpData->pChildWindows->Count();
	DBG_ASSERT( nNo<nCWCount,"ChildWindow ist unbekannt!" );
#endif
	return (*pImpData->pChildWindows)[nNo]->nFeature;
}

//--------------------------------------------------------------------


USHORT SfxInterface::GetChildWindowCount() const
{
	if (pGenoType)
		return pImpData->pChildWindows->Count() + pGenoType->GetChildWindowCount();
	else
		return pImpData->pChildWindows->Count();
}


const ResId& SfxInterface::GetPopupMenuResId() const
{
	return pImpData->aPopupRes;
}


const ResId& SfxInterface::GetStatusBarResId() const
{
	if (pImpData->aStatBarRes.GetId() == 0 && pGenoType)
		return pGenoType->GetStatusBarResId();
	else
		return pImpData->aStatBarRes;
}



const String* SfxInterface::GetObjectBarName ( USHORT nNo ) const
{
	BOOL bGenoType = (pGenoType != 0 && !pGenoType->HasName());
	if ( bGenoType )
	{
		// Gibt es Toolbars in der Superklasse ?
		USHORT nBaseCount = pGenoType->GetObjectBarCount();
		if ( nNo < nBaseCount )
			// Die der Superklasse kommen zuerst
			return pGenoType->GetObjectBarName( nNo );
		else
			nNo = nNo - nBaseCount;
	}

#ifdef DBG_UTIL
	USHORT nObjBarCount = pImpData->pObjectBars->Count();
	DBG_ASSERT( nNo<nObjBarCount,"Objectbar ist unbekannt!" );
#endif
	return (*pImpData->pObjectBars)[nNo]->pName;
}

sal_uInt32 SfxInterface::GetObjectBarFeature ( USHORT nNo ) const
{
	BOOL bGenoType = (pGenoType != 0 && !pGenoType->HasName());
	if ( bGenoType )
	{
		// Gibt es Toolbars in der Superklasse ?
		USHORT nBaseCount = pGenoType->GetObjectBarCount();
		if ( nNo < nBaseCount )
			// Die der Superklasse kommen zuerst
			return pGenoType->GetObjectBarFeature( nNo );
		else
			nNo = nNo - nBaseCount;
	}

#ifdef DBG_UTIL
	USHORT nObjBarCount = pImpData->pObjectBars->Count();
	DBG_ASSERT( nNo<nObjBarCount,"Objectbar ist unbekannt!" );
#endif
	return (*pImpData->pObjectBars)[nNo]->nFeature;
}

BOOL SfxInterface::IsObjectBarVisible(USHORT nNo) const
{
	BOOL bGenoType = (pGenoType != 0 && !pGenoType->HasName());
	if ( bGenoType )
	{
		// Gibt es Toolbars in der Superklasse ?
		USHORT nBaseCount = pGenoType->GetObjectBarCount();
		if ( nNo < nBaseCount )
			// Die der Superklasse kommen zuerst
			return pGenoType->IsObjectBarVisible( nNo );
		else
			nNo = nNo - nBaseCount;
	}

#ifdef DBG_UTIL
	USHORT nObjBarCount = pImpData->pObjectBars->Count();
	DBG_ASSERT( nNo<nObjBarCount,"Objectbar ist unbekannt!" );
#endif
	return (*pImpData->pObjectBars)[nNo]->bVisible;
}

const SfxInterface* SfxInterface::GetRealInterfaceForSlot( const SfxSlot *pRealSlot ) const
{
    DBG_ASSERT( pImpData->bRegistered, "Interface not registered!" );
	const SfxInterface* pInterface = this;

	// Der Slot k"onnte auch aus dem Interface einer Shell-Basisklasse stammen
	do
	{
		const SfxSlot *pLastSlot  = (*pInterface)[pInterface->Count()-1];
		const SfxSlot *pFirstSlot = (*pInterface)[0];

		// Ist pInterface der Owner von pRealSlot ?
		if ( pFirstSlot <= pRealSlot && pRealSlot <= pLastSlot )
			break;

		// Sonst Interface der Superklasse probieren
		pInterface = pInterface->pGenoType;
	}
	while ( pInterface );

	return pInterface;
}



