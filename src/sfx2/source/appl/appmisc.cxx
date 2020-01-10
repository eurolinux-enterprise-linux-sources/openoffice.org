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
#include <vcl/status.hxx>
#include <vcl/msgbox.hxx>
#ifndef _VOS_PROCESS_HXX //autogen
#include <vos/process.hxx>
#endif
#include <vos/xception.hxx>
#include <svtools/whiter.hxx>
#include <svtools/stritem.hxx>
#include <svtools/intitem.hxx>
#include <svtools/eitem.hxx>
#include <svtools/filter.hxx>
#include <svtools/internaloptions.hxx>
#include <svtools/pathoptions.hxx>
#include <com/sun/star/registry/InvalidRegistryException.hpp>
#ifndef _COM_SUN_STAR_BEANS_PropertyValue_HPP_
#include <com/sun/star/beans/PropertyValue.hpp>
#endif
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/frame/XFramesSupplier.hpp>
#include <com/sun/star/uno/Reference.h>
#include <tools/config.hxx>
#include <tools/rcid.h>
#include <vos/mutex.hxx>
#ifndef GCC
#endif
#include <unotools/configmgr.hxx>
#include <com/sun/star/frame/XDesktop.hpp>

#include <unotools/ucbstreamhelper.hxx>
#include <framework/menuconfiguration.hxx>
#include <comphelper/processfactory.hxx>
#include <unotools/localfilehelper.hxx>
#include <unotools/bootstrap.hxx>
#include <svtools/moduleoptions.hxx>
#include <osl/file.hxx>

#include "sfxresid.hxx"
#include <sfx2/app.hxx>
#include "appdata.hxx"
#include "arrdecl.hxx"
#include <sfx2/tbxctrl.hxx>
#include "stbitem.hxx"
#include <sfx2/mnuitem.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/request.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include "workwin.hxx"
#include "intro.hxx"
#include <sfx2/fcontnr.hxx>
#include "sfxlocal.hrc"
#include <sfx2/sfx.hrc>
#include "app.hrc"
#include <sfx2/templdlg.hxx>
#include <sfx2/module.hxx>
#include <sfx2/msgpool.hxx>
#include <sfx2/topfrm.hxx>
#include "openflag.hxx"
#include <sfx2/viewsh.hxx>
#include <sfx2/objface.hxx>
#include "helper.hxx"	// SfxContentHelper::Kill()

using namespace ::vos;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;

//===================================================================

SV_IMPL_PTRARR( SfxTbxCtrlFactArr_Impl, SfxTbxCtrlFactory* );
SV_IMPL_PTRARR( SfxStbCtrlFactArr_Impl, SfxStbCtrlFactory* );
SV_IMPL_PTRARR( SfxMenuCtrlFactArr_Impl, SfxMenuCtrlFactory* );
SV_IMPL_PTRARR( SfxChildWinFactArr_Impl, SfxChildWinFactory* );
SV_IMPL_PTRARR( SfxModuleArr_Impl, SfxModule* );

//===================================================================

#define SfxApplication
#include "sfxslots.hxx"

class SfxSpecialConfigError_Impl
{
	String aError;

public:

	SfxSpecialConfigError_Impl( const String& rStr );
	DECL_LINK( TimerHdl, Timer*);
};


SfxSpecialConfigError_Impl::SfxSpecialConfigError_Impl( const String& rStr ) :
	aError( rStr )
{
	Timer *pTimer = new Timer;
	pTimer->SetTimeoutHdl( LINK(this, SfxSpecialConfigError_Impl, TimerHdl) );
	pTimer->SetTimeout( 0 );
	pTimer->Start();
}

IMPL_LINK( SfxSpecialConfigError_Impl, TimerHdl, Timer*, pTimer )
{
	delete pTimer;
	ErrorBox( 0, WinBits( WB_OK ) , aError ).Execute();
	delete this;
    SFX_APP()->GetAppDispatcher_Impl()->Execute( SID_QUITAPP );
	return 0L;
}

//====================================================================

#define SFX_ITEMTYPE_STATBAR			 4

SFX_IMPL_INTERFACE(SfxApplication,SfxShell,SfxResId(RID_DESKTOP))
{
    SFX_STATUSBAR_REGISTRATION(SfxResId(SFX_ITEMTYPE_STATBAR));
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_0);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_1);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_2);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_3);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_4);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_5);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_6);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_7);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_8);
    SFX_CHILDWINDOW_REGISTRATION(SID_DOCKWIN_9);
}

//--------------------------------------------------------------------

void SfxApplication::InitializeDisplayName_Impl()
{
    SfxAppData_Impl* pAppData = Get_Impl();
    if ( !pAppData->pLabelResMgr )
        return;

	String aTitle = Application::GetDisplayName();
    if ( !aTitle.Len() )
	{
		OClearableGuard aGuard( OMutex::getGlobalMutex() );

    	// create version string
/*!!! (pb) don't show a version number at the moment
    	USHORT nProductVersion = ProductVersion::GetVersion().ToInt32();
        String aVersion( String::CreateFromInt32( nProductVersion / 10 ) );
    	aVersion += 0x002E ; // 2Eh ^= '.'
    	aVersion += ( String::CreateFromInt32( nProductVersion % 10 ) );
*/
		// load application title
    	aTitle = String( ResId( RID_APPTITLE, *pAppData->pLabelResMgr ) );
		// merge version into title
    	aTitle.SearchAndReplaceAscii( "$(VER)", String() /*aVersion*/ );

		aGuard.clear();

#ifndef PRODUCT
		::rtl::OUString	aDefault;
		aTitle += DEFINE_CONST_UNICODE(" [");

		String aVerId( utl::Bootstrap::getBuildIdData( aDefault ));
		aTitle += aVerId;
		aTitle += ']';
#endif
		Application::SetDisplayName( aTitle );
	}
}

//--------------------------------------------------------------------
SfxProgress* SfxApplication::GetProgress() const

/*  [Beschreibung]

    Liefert den f"ur die gesamte Applikation laufenden SfxProgress
    oder 0, falls keiner f"ur die gesamte Applikation l"auft.


    [Querverweise]

    <SfxProgress::GetActiveProgress(SfxViewFrame*)>
    <SfxViewFrame::GetProgress()const>
*/

{
    return pAppData_Impl->pProgress;
}

//------------------------------------------------------------------------

SvUShorts* SfxApplication::GetDisabledSlotList_Impl()
{
	sal_Bool bError = sal_False;
	SvUShorts* pList = pAppData_Impl->pDisabledSlotList;
	if ( !pList )
	{
		// Gibt es eine Slotdatei ?
        INetURLObject aUserObj( SvtPathOptions().GetUserConfigPath() );
        aUserObj.insertName( DEFINE_CONST_UNICODE( "slots.cfg" ) );
        SvStream* pStream = ::utl::UcbStreamHelper::CreateStream( aUserObj.GetMainURL( INetURLObject::NO_DECODE ), STREAM_STD_READ );
        if ( !pStream || pStream->GetError() == ERRCODE_IO_NOTEXISTS )
        {
            delete pStream;
            INetURLObject aObj( SvtPathOptions().GetConfigPath() );
            aObj.insertName( DEFINE_CONST_UNICODE( "slots.cfg" ) );
            pStream = ::utl::UcbStreamHelper::CreateStream( aObj.GetMainURL( INetURLObject::NO_DECODE ), STREAM_STD_READ );
        }

        BOOL bSlotsEnabled = SvtInternalOptions().SlotCFGEnabled();
        BOOL bSlots = ( pStream && !pStream->GetError() );
        if( bSlots && bSlotsEnabled )
		{
            // SlotDatei einlesen
            String aTitle;
            pStream->ReadByteString(aTitle);
            if ( aTitle.CompareToAscii("SfxSlotFile" ) == COMPARE_EQUAL )
            {
                sal_uInt16 nCount;
                (*pStream) >> nCount;
                pList = pAppData_Impl->pDisabledSlotList =
                        new SvUShorts( nCount < 255 ? (sal_Int8) nCount : 255, 255 );

                sal_uInt16 nSlot;
                for ( sal_uInt16 n=0; n<nCount; n++ )
                {
                    (*pStream) >> nSlot;
                    pList->Insert( nSlot, n );
                }

                pStream->ReadByteString(aTitle);
                if ( aTitle.CompareToAscii("END" ) != COMPARE_EQUAL || pStream->GetError() )
                {
                    // Lesen schief gegangen
                    DELETEZ( pList );
                    bError = sal_True;
                }
            }
            else
            {
                // Streamerkennung  fehlgeschlagen
                bError = sal_True;
            }
		}
        else if ( bSlots != bSlotsEnabled )
		{
			// Wenn kein Slotlist-Eintrag, dann darf auch keine SlotDatei
			// vorhanden sein
            bError = sal_True;
		}

        delete pStream;
	}
	else if ( pList == (SvUShorts*) -1L )
	{
		return NULL;
	}

	if ( !pList )
		pAppData_Impl->pDisabledSlotList = (SvUShorts*) -1L;

	if ( bError )
	{
		// Wenn ein Sloteintrag vorhanden ist, aber keine oder eine fehlerhafte
		// SlotDatei, oder aber eine Slotdatei, aber kein Sloteintrag, dann
		// gilt dies als fehlerhafte Konfiguration
        new SfxSpecialConfigError_Impl( String( SfxResId( RID_SPECIALCONFIG_ERROR ) ) );
	}

   	return pList;
}


SfxModule* SfxApplication::GetModule_Impl()
{
    SfxModule* pModule = SfxModule::GetActiveModule();
    if ( !pModule )
        pModule = SfxModule::GetActiveModule( SfxViewFrame::GetFirst( FALSE ) );
	if( pModule )
		return pModule;
    else
    {
        DBG_ERROR( "No module!" );
        return NULL;
    }
}

ISfxTemplateCommon* SfxApplication::GetCurrentTemplateCommon( SfxBindings& rBindings )
{
	if( pAppData_Impl->pTemplateCommon )
		return pAppData_Impl->pTemplateCommon;
	SfxChildWindow *pChild = rBindings.GetWorkWindow_Impl()->GetChildWindow_Impl(
		SfxTemplateDialogWrapper::GetChildWindowId() );
	if ( pChild )
		return ((SfxTemplateDialog*) pChild->GetWindow())->GetISfxTemplateCommon();
	return 0;
}

SfxCancelManager* SfxApplication::GetCancelManager() const
{
	if ( !pAppData_Impl->pCancelMgr )
		pAppData_Impl->pCancelMgr = new SfxCancelManager;
	return pAppData_Impl->pCancelMgr;
}

SfxResourceManager& SfxApplication::GetResourceManager() const { return *pAppData_Impl->pResMgr; }
BOOL  SfxApplication::IsDowning() const { return pAppData_Impl->bDowning; }
SfxDispatcher* SfxApplication::GetAppDispatcher_Impl() { return pAppData_Impl->pAppDispat; }
SfxSlotPool& SfxApplication::GetAppSlotPool_Impl() const { return *pAppData_Impl->pSlotPool; }
//SfxOptions&  SfxApplication::GetOptions() { return *pAppData_Impl->pOptions; }
//const SfxOptions& SfxApplication::GetOptions() const { return *pAppData_Impl->pOptions; }

