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

#ifndef GCC
#endif

#include <stdio.h>
#include <tools/rcid.h>

#include <cstdarg>
#include <sfx2/module.hxx>
#include <sfx2/app.hxx>
#include "arrdecl.hxx"
#include "sfxresid.hxx"
#include <sfx2/msgpool.hxx>
#include <sfx2/tbxctrl.hxx>
#include "stbitem.hxx"
#include <sfx2/mnuitem.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/mnumgr.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/viewfrm.hxx>

#define SfxModule
#include "sfxslots.hxx"

static SfxModuleArr_Impl* pModules=0;

class SfxModule_Impl
{
public:

	SfxSlotPool*				pSlotPool;
	SfxTbxCtrlFactArr_Impl* 	pTbxCtrlFac;
	SfxStbCtrlFactArr_Impl* 	pStbCtrlFac;
	SfxMenuCtrlFactArr_Impl*	pMenuCtrlFac;
	SfxChildWinFactArr_Impl*	pFactArr;
    ImageList*                  pImgListSmall;
    ImageList*                  pImgListBig;
	ImageList*					pImgListHiSmall;
	ImageList*					pImgListHiBig;

								SfxModule_Impl();
								~SfxModule_Impl();
    ImageList*                  GetImageList( ResMgr*, BOOL, BOOL bHiContrast = FALSE );
};

SfxModule_Impl::SfxModule_Impl()
 : pSlotPool(0)
{
}

SfxModule_Impl::~SfxModule_Impl()
{
    delete pSlotPool;
    delete pTbxCtrlFac;
    delete pStbCtrlFac;
    delete pMenuCtrlFac;
    delete pFactArr;
    delete pImgListSmall;
    delete pImgListBig;
    delete pImgListHiSmall;
    delete pImgListHiBig;
}

ImageList* SfxModule_Impl::GetImageList( ResMgr* pResMgr, BOOL bBig, BOOL bHiContrast )
{
    ImageList*& rpList = bBig ? ( bHiContrast ? pImgListHiBig: pImgListBig ) :
								( bHiContrast ? pImgListHiSmall : pImgListSmall );
    if ( !rpList )
    {
        ResId aResId( bBig ? ( bHiContrast ? RID_DEFAULTIMAGELIST_LCH : RID_DEFAULTIMAGELIST_LC ) :
							 ( bHiContrast ? RID_DEFAULTIMAGELIST_SCH : RID_DEFAULTIMAGELIST_SC ), *pResMgr );
        aResId.SetRT( RSC_IMAGELIST );

        DBG_ASSERT( pResMgr->IsAvailable(aResId), "No default ImageList!" );

        if ( pResMgr->IsAvailable(aResId) )
            rpList = new ImageList( aResId );
        else
            rpList = new ImageList();
    }

    return rpList; }

TYPEINIT1(SfxModule, SfxShell);

//=========================================================================

SFX_IMPL_INTERFACE(SfxModule,SfxShell,SfxResId(0))
{
}

//====================================================================

ResMgr* SfxModule::GetResMgr()
{
	return pResMgr;
}

//====================================================================
/*
SfxModule::SfxModule( ResMgr* pMgrP, BOOL bDummyP,
					  SfxObjectFactory* pFactoryP )
	: pResMgr( pMgrP ), bDummy( bDummyP ), pImpl(0L)
{
	Construct_Impl();
	if ( pFactoryP )
		pFactoryP->SetModule_Impl( this );
}
*/
SfxModule::SfxModule( ResMgr* pMgrP, BOOL bDummyP,
					  SfxObjectFactory* pFactoryP, ... )
	: pResMgr( pMgrP ), bDummy( bDummyP ), pImpl(0L)
{
	Construct_Impl();
	va_list pVarArgs;
	va_start( pVarArgs, pFactoryP );
	for ( SfxObjectFactory *pArg = pFactoryP; pArg;
		 pArg = va_arg( pVarArgs, SfxObjectFactory* ) )
		pArg->SetModule_Impl( this );
	va_end(pVarArgs);
}

void SfxModule::Construct_Impl()
{
	if( !bDummy )
	{
		SfxApplication *pApp = SFX_APP();
        SfxModuleArr_Impl& rArr = GetModules_Impl();
		SfxModule* pPtr = (SfxModule*)this;
		rArr.C40_INSERT( SfxModule, pPtr, rArr.Count() );
		pImpl = new SfxModule_Impl;
		pImpl->pSlotPool = new SfxSlotPool( &pApp->GetAppSlotPool_Impl(), pResMgr );

		pImpl->pTbxCtrlFac=0;
		pImpl->pStbCtrlFac=0;
		pImpl->pMenuCtrlFac=0;
		pImpl->pFactArr=0;
        pImpl->pImgListSmall=0;
        pImpl->pImgListBig=0;
        pImpl->pImgListHiSmall=0;
        pImpl->pImgListHiBig=0;

		SetPool( &pApp->GetPool() );
	}
}

//====================================================================

SfxModule::~SfxModule()
{
	if( !bDummy )
	{
		if ( SFX_APP()->Get_Impl() )
		{
			// Das Modul wird noch vor dem DeInitialize zerst"ort, also auis dem Array entfernen
            SfxModuleArr_Impl& rArr = GetModules_Impl();
			for( USHORT nPos = rArr.Count(); nPos--; )
			{
				if( rArr[ nPos ] == this )
				{
					rArr.Remove( nPos );
					break;
				}
			}

			delete pImpl;
		}

		delete pResMgr;
	}
}

//-------------------------------------------------------------------------

SfxSlotPool* SfxModule::GetSlotPool() const
{
	return pImpl->pSlotPool;
}

//-------------------------------------------------------------------------

void SfxModule::RegisterChildWindow(SfxChildWinFactory *pFact)
{
	DBG_ASSERT( pImpl, "Kein echtes Modul!" );

	if (!pImpl->pFactArr)
		pImpl->pFactArr = new SfxChildWinFactArr_Impl;

//#ifdef DBG_UTIL
	for (USHORT nFactory=0; nFactory<pImpl->pFactArr->Count(); ++nFactory)
	{
		if (pFact->nId ==  (*pImpl->pFactArr)[nFactory]->nId)
		{
			pImpl->pFactArr->Remove( nFactory );
			DBG_ERROR("ChildWindow mehrfach registriert!");
			return;
		}
	}
//#endif

	pImpl->pFactArr->C40_INSERT(
		SfxChildWinFactory, pFact, pImpl->pFactArr->Count() );
}

//-------------------------------------------------------------------------

void SfxModule::RegisterChildWindowContext( USHORT nId,
		SfxChildWinContextFactory *pFact)
{
	DBG_ASSERT( pImpl, "Kein echtes Modul!" );

	USHORT nCount = pImpl->pFactArr->Count();
	for (USHORT nFactory=0; nFactory<nCount; ++nFactory)
	{
		SfxChildWinFactory *pF = (*pImpl->pFactArr)[nFactory];
		if ( nId == pF->nId )
		{
			if ( !pF->pArr )
				pF->pArr = new SfxChildWinContextArr_Impl;
			pF->pArr->C40_INSERT( SfxChildWinContextFactory, pFact, pF->pArr->Count() );
			return;
		}
	}

	DBG_ERROR( "Kein ChildWindow fuer diesen Context!" );
}

//-------------------------------------------------------------------------

void SfxModule::RegisterToolBoxControl( SfxTbxCtrlFactory *pFact )
{
	if (!pImpl->pTbxCtrlFac)
		pImpl->pTbxCtrlFac = new SfxTbxCtrlFactArr_Impl;

#ifdef DBG_UTIL
	for ( USHORT n=0; n<pImpl->pTbxCtrlFac->Count(); n++ )
	{
		SfxTbxCtrlFactory *pF = (*pImpl->pTbxCtrlFac)[n];
		if ( pF->nTypeId && pF->nTypeId == pFact->nTypeId &&
			(pF->nSlotId == pFact->nSlotId || pF->nSlotId == 0) )
		{
			DBG_WARNING("TbxController-Registrierung ist nicht eindeutig!");
		}
	}
#endif

	pImpl->pTbxCtrlFac->C40_INSERT( SfxTbxCtrlFactory, pFact, pImpl->pTbxCtrlFac->Count() );
}

//-------------------------------------------------------------------------

void SfxModule::RegisterStatusBarControl( SfxStbCtrlFactory *pFact )
{
	if (!pImpl->pStbCtrlFac)
		pImpl->pStbCtrlFac = new SfxStbCtrlFactArr_Impl;

#ifdef DBG_UTIL
	for ( USHORT n=0; n<pImpl->pStbCtrlFac->Count(); n++ )
	{
		SfxStbCtrlFactory *pF = (*pImpl->pStbCtrlFac)[n];
		if ( pF->nTypeId && pF->nTypeId == pFact->nTypeId &&
			(pF->nSlotId == pFact->nSlotId || pF->nSlotId == 0) )
		{
			DBG_WARNING("StbController-Registrierung ist nicht eindeutig!");
		}
	}
#endif

	pImpl->pStbCtrlFac->C40_INSERT( SfxStbCtrlFactory, pFact, pImpl->pStbCtrlFac->Count() );
}

//-------------------------------------------------------------------------

void SfxModule::RegisterMenuControl( SfxMenuCtrlFactory *pFact )
{
	if (!pImpl->pMenuCtrlFac)
		pImpl->pMenuCtrlFac = new SfxMenuCtrlFactArr_Impl;

#ifdef DBG_UTIL
	for ( USHORT n=0; n<pImpl->pMenuCtrlFac->Count(); n++ )
	{
		SfxMenuCtrlFactory *pF = (*pImpl->pMenuCtrlFac)[n];
		if ( pF->nTypeId && pF->nTypeId == pFact->nTypeId &&
			(pF->nSlotId == pFact->nSlotId || pF->nSlotId == 0) )
		{
			DBG_WARNING("MenuController-Registrierung ist nicht eindeutig!");
		}
	}
#endif

	pImpl->pMenuCtrlFac->C40_INSERT( SfxMenuCtrlFactory, pFact, pImpl->pMenuCtrlFac->Count() );
}

//-------------------------------------------------------------------------

SfxTbxCtrlFactArr_Impl*  SfxModule::GetTbxCtrlFactories_Impl() const
{
	return pImpl->pTbxCtrlFac;
}

//-------------------------------------------------------------------------

SfxStbCtrlFactArr_Impl*  SfxModule::GetStbCtrlFactories_Impl() const
{
	return pImpl->pStbCtrlFac;
}

//-------------------------------------------------------------------------

SfxMenuCtrlFactArr_Impl* SfxModule::GetMenuCtrlFactories_Impl() const
{
	return pImpl->pMenuCtrlFac;
}

//-------------------------------------------------------------------------

SfxChildWinFactArr_Impl* SfxModule::GetChildWinFactories_Impl() const
{
	return pImpl->pFactArr;
}

ImageList* SfxModule::GetImageList_Impl( BOOL bBig )
{
	return pImpl->GetImageList( pResMgr, bBig, FALSE );
}

ImageList* SfxModule::GetImageList_Impl( BOOL bBig, BOOL bHiContrast )
{
    return pImpl->GetImageList( pResMgr, bBig, bHiContrast );
}

SfxTabPage*	SfxModule::CreateTabPage( USHORT, Window*, const SfxItemSet& )
{
	return NULL;
}

SfxModuleArr_Impl& SfxModule::GetModules_Impl()
{
    if( !pModules )
        pModules = new SfxModuleArr_Impl;
    return *pModules;
};

void SfxModule::DestroyModules_Impl()
{
    if ( pModules )
    {
        SfxModuleArr_Impl& rModules = *pModules;
        for( USHORT nPos = rModules.Count(); nPos--; )
	{
	    SfxModule* pMod = rModules.GetObject(nPos);
	    delete pMod;
	}
    }
}

void SfxModule::Invalidate( USHORT nId )
{
    for( SfxViewFrame* pFrame = SfxViewFrame::GetFirst(); pFrame; pFrame = SfxViewFrame::GetNext( *pFrame ) )
        if ( pFrame->GetObjectShell()->GetModule() == this )
            Invalidate_Impl( pFrame->GetBindings(), nId );
}

BOOL SfxModule::IsActive() const
{
	SfxViewFrame* pFrame = SfxViewFrame::Current();
	if ( pFrame && pFrame->GetObjectShell()->GetFactory().GetModule() == this )
		return TRUE;
	return FALSE;
}

SfxModule* SfxModule::GetActiveModule( SfxViewFrame* pFrame )
{
	if ( !pFrame )
		pFrame = SfxViewFrame::Current();
	SfxObjectShell* pSh = 0;
    if( pFrame )
        pSh = pFrame->GetObjectShell();
	return pSh ? pSh->GetModule() : 0;
}
