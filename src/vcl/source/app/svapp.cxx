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
#include "precompiled_vcl.hxx"

#ifndef _SV_SVSYS_HXX
#include <svsys.h>
#endif
#include <vcl/salinst.hxx>
#include <vcl/salframe.hxx>
#include <vcl/salsys.hxx>
#ifndef _VOS_PROCESS_HXX
#include <vos/process.hxx>
#endif
#ifndef _VOS_MUTEX_HXX
#include <vos/mutex.hxx>
#endif
#include <tools/tools.h>
#include <tools/debug.hxx>
#include <tools/time.hxx>
#include <i18npool/mslangid.hxx>
#include <vcl/svdata.hxx>
#include <vcl/settings.hxx>
#ifndef _ACCMGR_HXX
#include <vcl/accmgr.hxx>
#endif
#ifndef _SV_KEYCOD_HXX
#include <vcl/keycod.hxx>
#endif
#include <vcl/event.hxx>
#include <vcl/virdev.hxx>
#include <vcl/windata.hxx>
#include <vcl/window.h>
#include <vcl/wrkwin.hxx>
#include <vcl/idlemgr.hxx>
#include <vcl/svapp.hxx>
#include <vcl/cvtgrf.hxx>
#include <vcl/unowrap.hxx>
#include <vcl/xconnection.hxx>
#ifndef _SV_SVIDS_HRC
#include <vcl/svids.hrc>
#endif
#include <vcl/timer.hxx>

#include <vcl/unohelp.hxx>

#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/awt/XToolkit.hpp>
#include <com/sun/star/uno/XNamingService.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <comphelper/processfactory.hxx>

#include <osl/module.h>
#include <osl/file.hxx>

#include "osl/thread.h"
#include "rtl/tencinfo.h"
#include <rtl/instance.hxx>
#include <vcl/salimestatus.hxx>

#include <utility>
#include <vcl/lazydelete.hxx>

using namespace ::com::sun::star::uno;

// keycodes handled internally by VCL
class ImplReservedKey
{
public:
    ImplReservedKey( KeyCode aKeyCode, USHORT nResId ) :
      mKeyCode(aKeyCode), mnResId( nResId)
     {}

    KeyCode mKeyCode;
    USHORT  mnResId;
};

typedef std::pair<ImplReservedKey*, size_t> ReservedKeys;
namespace
{
    struct ImplReservedKeysImpl
    {
        ReservedKeys* operator()()
        {
            static ImplReservedKey ImplReservedKeys[] =
            {
			    ImplReservedKey(KeyCode(KEY_F1,0),                  SV_SHORTCUT_HELP),
			    ImplReservedKey(KeyCode(KEY_F1,KEY_SHIFT),          SV_SHORTCUT_ACTIVEHELP),
    			ImplReservedKey(KeyCode(KEY_F1,KEY_MOD1),           SV_SHORTCUT_CONTEXTHELP),
    			ImplReservedKey(KeyCode(KEY_F2,KEY_SHIFT),          SV_SHORTCUT_CONTEXTHELP),
    			ImplReservedKey(KeyCode(KEY_F4,KEY_MOD1),           SV_SHORTCUT_DOCKUNDOCK),
    			ImplReservedKey(KeyCode(KEY_F4,KEY_MOD2),           SV_SHORTCUT_DOCKUNDOCK),
    			ImplReservedKey(KeyCode(KEY_F4,KEY_MOD1|KEY_MOD2),  SV_SHORTCUT_DOCKUNDOCK),
    			ImplReservedKey(KeyCode(KEY_F6,0),                  SV_SHORTCUT_NEXTSUBWINDOW),
    			ImplReservedKey(KeyCode(KEY_F6,KEY_MOD1),           SV_SHORTCUT_TODOCUMENT),
    			ImplReservedKey(KeyCode(KEY_F6,KEY_SHIFT),          SV_SHORTCUT_PREVSUBWINDOW),
    			ImplReservedKey(KeyCode(KEY_F6,KEY_MOD1|KEY_SHIFT), SV_SHORTCUT_SPLITTER),
    			ImplReservedKey(KeyCode(KEY_F10,0),                 SV_SHORTCUT_MENUBAR)
#ifdef UNX
                ,
                ImplReservedKey(KeyCode(KEY_1,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_2,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_3,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_4,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_5,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_6,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_7,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_8,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_9,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_0,KEY_SHIFT|KEY_MOD1), 0),
                ImplReservedKey(KeyCode(KEY_ADD,KEY_SHIFT|KEY_MOD1), 0)
#endif
            };
            static ReservedKeys aKeys
            (
                &ImplReservedKeys[0],
                sizeof(ImplReservedKeys) / sizeof(ImplReservedKey)
            );
            return &aKeys;
        }
    };

    struct ImplReservedKeys
        : public rtl::StaticAggregate<ReservedKeys, ImplReservedKeysImpl> {};
}


// #include <usr/refl.hxx>
class Reflection;



extern "C" {
    typedef UnoWrapperBase* (SAL_CALL *FN_TkCreateUnoWrapper)();
}

// =======================================================================

// --------------
// - ImplHotKey -
// --------------

struct ImplHotKey
{
    ImplHotKey*             mpNext;
    void*                   mpUserData;
    KeyCode                 maKeyCode;
    Link                    maLink;
};

// =======================================================================

// -----------------
// - ImplEventHook -
// -----------------

struct ImplEventHook
{
    ImplEventHook*          mpNext;
    void*                   mpUserData;
    VCLEventHookProc        mpProc;
};

// =======================================================================

// ---------------------
// - ImplPostEventData -
// ---------------------

struct ImplPostEventData
{
    ULONG			mnEvent;
    const Window*	mpWin;
	ULONG			mnEventId;
    KeyEvent    	maKeyEvent;
	MouseEvent		maMouseEvent;
	

   	ImplPostEventData( ULONG nEvent, const Window* pWin, const KeyEvent& rKeyEvent ) :
    	mnEvent( nEvent ), mpWin( pWin ), mnEventId( 0 ), maKeyEvent( rKeyEvent ) {}
   	ImplPostEventData( ULONG nEvent, const Window* pWin, const MouseEvent& rMouseEvent ) :
    	mnEvent( nEvent ), mpWin( pWin ), mnEventId( 0 ), maMouseEvent( rMouseEvent ) {}

	~ImplPostEventData() {}
};

typedef ::std::pair< Window*, ImplPostEventData* > ImplPostEventPair;

static ::std::list< ImplPostEventPair > aPostedEventList;

// =======================================================================

Application* GetpApp()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( !pSVData )
        return NULL;
    return pSVData->mpApp;
}

// -----------------------------------------------------------------------

Application::Application()
{
    if( ! ImplGetSVData() )
        ImplInitSVData();
    ImplGetSVData()->mpApp = this;
    InitSalData();
}

// -----------------------------------------------------------------------

Application::~Application()
{
    ImplDeInitSVData();
    DeInitSalData();
    ImplGetSVData()->mpApp = NULL;
    ImplDestroySVData();
    GlobalDeInitTools();
}

// -----------------------------------------------------------------------

void Application::InitAppRes( const ResId& )
{
}

// -----------------------------------------------------------------------

BOOL Application::QueryExit()
{
    WorkWindow* pAppWin = ImplGetSVData()->maWinData.mpAppWin;

    // Aufruf des Close-Handlers des Applikationsfensters
    if ( pAppWin )
        return pAppWin->Close();
    else
        return TRUE;
}

// -----------------------------------------------------------------------

void Application::UserEvent( ULONG, void* )
{
}

// -----------------------------------------------------------------------

void Application::ShowStatusText( const XubString& )
{
}

// -----------------------------------------------------------------------

void Application::ShowHelpStatusText( const XubString& )
{
}

// -----------------------------------------------------------------------

void Application::ActivateExtHelp()
{
}

// -----------------------------------------------------------------------

void Application::DeactivateExtHelp()
{
}

// -----------------------------------------------------------------------

void Application::HideStatusText()
{
}

// -----------------------------------------------------------------------

void Application::HideHelpStatusText()
{
}

// -----------------------------------------------------------------------

void Application::FocusChanged()
{
}

// -----------------------------------------------------------------------

void Application::DataChanged( const DataChangedEvent& )
{
}

// -----------------------------------------------------------------------
void Application::Init()
{
}

// -----------------------------------------------------------------------

void Application::DeInit()
{
}

// -----------------------------------------------------------------------

USHORT Application::GetCommandLineParamCount()
{
    vos::OStartupInfo aStartInfo;
    return (USHORT)aStartInfo.getCommandArgCount();
}

// -----------------------------------------------------------------------

XubString Application::GetCommandLineParam( USHORT nParam )
{
    vos::OStartupInfo   aStartInfo;
    rtl::OUString       aParam;
    aStartInfo.getCommandArg( nParam, aParam );
    return XubString( aParam );
}

// -----------------------------------------------------------------------

const XubString& Application::GetAppFileName()
{
    ImplSVData* pSVData = ImplGetSVData();
    DBG_ASSERT( pSVData->maAppData.mpAppFileName, "AppFileName vor SVMain ?!" );
    if ( pSVData->maAppData.mpAppFileName )
        return *pSVData->maAppData.mpAppFileName;

    /*
     *  #91147# provide a fallback for people without initialized
     *  vcl here (like setup in responsefile mode)
     */
    static String aAppFileName;
    if( !aAppFileName.Len() )
    {
        vos::OStartupInfo   aStartInfo;
        ::rtl::OUString		aExeFileName;

        aStartInfo.getExecutableFile( aExeFileName );

        // convert path to native file format
        rtl::OUString aNativeFileName;
        osl::FileBase::getSystemPathFromFileURL( aExeFileName, aNativeFileName );
        aAppFileName = aNativeFileName;
    }

    return aAppFileName;
}

// -----------------------------------------------------------------------

USHORT Application::Exception( USHORT nError )
{
    switch ( nError & EXC_MAJORTYPE )
    {
        // Bei System machen wir nichts und lassen dem System den
        // vortritt
        case EXC_SYSTEM:
            return 0;

        case EXC_DISPLAY:
        case EXC_REMOTE:
            return 0;

#ifdef DBG_UTIL
        case EXC_RSCNOTLOADED:
            Abort( XubString( RTL_CONSTASCII_USTRINGPARAM( "Resource not loaded" ) ) );
            break;
        case EXC_SYSOBJNOTCREATED:
            Abort( XubString( RTL_CONSTASCII_USTRINGPARAM( "System Object not created" ) ) );
            break;
        default:
            Abort( XubString( RTL_CONSTASCII_USTRINGPARAM( "Unknown Error" ) ) );
            break;
#else
        default:
            Abort( ImplGetSVEmptyStr() );
            break;
#endif
    }

    return 0;
}

// -----------------------------------------------------------------------

void Application::Abort( const XubString& rErrorText )
{
    SalAbort( rErrorText );
}

// -----------------------------------------------------------------------

ULONG   Application::GetReservedKeyCodeCount()
{
    return ImplReservedKeys::get()->second;
}

const KeyCode*  Application::GetReservedKeyCode( ULONG i )
{
    if( i >= GetReservedKeyCodeCount() )
        return NULL;
    else
        return &ImplReservedKeys::get()->first[i].mKeyCode;
}

String Application::GetReservedKeyCodeDescription( ULONG i )
{
    ResMgr* pResMgr = ImplGetResMgr();
    if( ! pResMgr )
        return String();
    ImplReservedKey *pImplReservedKeys = ImplReservedKeys::get()->first;
    if( i >= GetReservedKeyCodeCount() || ! pImplReservedKeys[i].mnResId )
        return String();
    else
        return String( ResId( pImplReservedKeys[i].mnResId, *pResMgr ) );
}

// -----------------------------------------------------------------------

void Application::Execute()
{
    DBG_STARTAPPEXECUTE();

    ImplSVData* pSVData = ImplGetSVData();
    pSVData->maAppData.mbInAppExecute = TRUE;

    while ( !pSVData->maAppData.mbAppQuit )
        Application::Yield();

    pSVData->maAppData.mbInAppExecute = FALSE;

    DBG_ENDAPPEXECUTE();
}

// -----------------------------------------------------------------------

void Application::Reschedule( bool bAllEvents )
{
    ImplSVData* pSVData = ImplGetSVData();

    // run timers that have timed out
    if ( !pSVData->mbNoCallTimer )
        while ( pSVData->mbNotAllTimerCalled )
            Timer::ImplTimerCallbackProc();

    pSVData->maAppData.mnDispatchLevel++;
    pSVData->mpDefInst->Yield( false, bAllEvents );
    pSVData->maAppData.mnDispatchLevel--;

    // flush lazy deleted objects
    if( pSVData->maAppData.mnDispatchLevel == 0 )
        vcl::LazyDelete::flush();
}

// -----------------------------------------------------------------------

void Application::Yield( bool bAllEvents )
{
    ImplSVData* pSVData = ImplGetSVData();

    // run timers that have timed out
    if ( !pSVData->mbNoCallTimer )
        while ( pSVData->mbNotAllTimerCalled )
            Timer::ImplTimerCallbackProc();

    // do not wait for events if application was already quit; in that
    // case only dispatch events already available
    pSVData->maAppData.mnDispatchLevel++;
    pSVData->mpDefInst->Yield( !pSVData->maAppData.mbAppQuit, bAllEvents );
    pSVData->maAppData.mnDispatchLevel--;
    
    // flush lazy deleted objects
    if( pSVData->maAppData.mnDispatchLevel == 0 )
        vcl::LazyDelete::flush();
}

// -----------------------------------------------------------------------

IMPL_STATIC_LINK_NOINSTANCE( ImplSVAppData, ImplQuitMsg, void*, EMPTYARG )
{
    ImplGetSVData()->maAppData.mbAppQuit = TRUE;
    return 0;
}

// -----------------------------------------------------------------------

void Application::Quit()
{
    Application::PostUserEvent( STATIC_LINK( NULL, ImplSVAppData, ImplQuitMsg ) );
}

// -----------------------------------------------------------------------

vos::IMutex& Application::GetSolarMutex()
{
    ImplSVData* pSVData = ImplGetSVData();
    return *(pSVData->mpDefInst->GetYieldMutex());
}

// -----------------------------------------------------------------------

vos::OThread::TThreadIdentifier Application::GetMainThreadIdentifier()
{
    return ImplGetSVData()->mnMainThreadId;
}

// -----------------------------------------------------------------------

ULONG Application::ReleaseSolarMutex()
{
    ImplSVData* pSVData = ImplGetSVData();
    return pSVData->mpDefInst->ReleaseYieldMutex();
}

// -----------------------------------------------------------------------

void Application::AcquireSolarMutex( ULONG nCount )
{
    ImplSVData* pSVData = ImplGetSVData();
    pSVData->mpDefInst->AcquireYieldMutex( nCount );
}

// -----------------------------------------------------------------------

BOOL Application::IsInMain()
{
    return ImplGetSVData()->maAppData.mbInAppMain;
}

// -----------------------------------------------------------------------

BOOL Application::IsInExecute()
{
    return ImplGetSVData()->maAppData.mbInAppExecute;
}

// -----------------------------------------------------------------------

BOOL Application::IsShutDown()
{
    return ImplGetSVData()->maAppData.mbAppQuit;
}

// -----------------------------------------------------------------------

BOOL Application::IsInModalMode()
{
    return (ImplGetSVData()->maAppData.mnModalMode != 0);
}

// -----------------------------------------------------------------------

USHORT Application::GetModalModeCount()
{
    return ImplGetSVData()->maAppData.mnModalMode;
}

// -----------------------------------------------------------------------

USHORT Application::GetDispatchLevel()
{
    return ImplGetSVData()->maAppData.mnDispatchLevel;
}

// -----------------------------------------------------------------------

BOOL Application::AnyInput( USHORT nType )
{
    return (BOOL)ImplGetSVData()->mpDefInst->AnyInput( nType );
}

// -----------------------------------------------------------------------

ULONG Application::GetLastInputInterval()
{
    return (Time::GetSystemTicks()-ImplGetSVData()->maAppData.mnLastInputTime);
}

// -----------------------------------------------------------------------

extern int nImplSysDialog;

BOOL Application::IsUICaptured()
{
    ImplSVData* pSVData = ImplGetSVData();
    // Wenn Mouse gecaptured, oder im TrackingModus oder im Auswahlmodus
    // eines FloatingWindows (wie Menus, Aufklapp-ToolBoxen) soll kein
    // weiteres Fenster aufgezogen werden
    // D&D aktive !!!
    if ( pSVData->maWinData.mpCaptureWin || pSVData->maWinData.mpTrackWin ||
         pSVData->maWinData.mpFirstFloat || nImplSysDialog )
        return TRUE;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

BOOL Application::IsUserActive( USHORT nTest )
{
    if ( nTest & (USERACTIVE_MOUSEDRAG | USERACTIVE_INPUT) )
    {
        if ( IsUICaptured() )
            return TRUE;
    }

    if ( nTest & USERACTIVE_INPUT )
    {
        if ( GetLastInputInterval() < 500 )
            return TRUE;

        if ( AnyInput( INPUT_KEYBOARD ) )
            return TRUE;
    }

    if ( nTest & USERACTIVE_MODALDIALOG )
    {
        if ( ImplGetSVData()->maAppData.mnModalDialog )
            return TRUE;
    }

    return FALSE;
}

// -----------------------------------------------------------------------

void Application::SystemSettingsChanging( AllSettings& /*rSettings*/,
                                          Window* /*pFrame*/ )
{
}

// -----------------------------------------------------------------------

void Application::MergeSystemSettings( AllSettings& rSettings )
{
    Window* pWindow = ImplGetSVData()->maWinData.mpFirstFrame;
    if( ! pWindow )
        pWindow = ImplGetDefaultWindow();
    if( pWindow )
    {
        ImplSVData* pSVData = ImplGetSVData();
        if ( !pSVData->maAppData.mbSettingsInit )
        {
            // side effect: ImplUpdateGlobalSettings does an ImplGetFrame()->UpdateSettings
            pWindow->ImplUpdateGlobalSettings( *pSVData->maAppData.mpSettings );
            pSVData->maAppData.mbSettingsInit = TRUE;
        }
        // side effect: ImplUpdateGlobalSettings does an ImplGetFrame()->UpdateSettings
        pWindow->ImplUpdateGlobalSettings( rSettings, FALSE );
    }
}

// -----------------------------------------------------------------------

bool Application::ValidateSystemFont()
{
    Window* pWindow = ImplGetSVData()->maWinData.mpFirstFrame;
    if( ! pWindow )
        pWindow = ImplGetDefaultWindow();

    if( pWindow )
    {
        AllSettings aSettings;
        pWindow->ImplGetFrame()->UpdateSettings( aSettings );
        return pWindow->ImplCheckUIFont( aSettings.GetStyleSettings().GetAppFont() );
    }
    return false;
}

// -----------------------------------------------------------------------

void Application::SetSettings( const AllSettings& rSettings )
{
    MsLangId::setConfiguredSystemLanguage( rSettings.GetLanguage());
    MsLangId::setConfiguredSystemUILanguage( rSettings.GetUILanguage());
    ImplSVData* pSVData = ImplGetSVData();
    if ( !pSVData->maAppData.mpSettings )
    {
        pSVData->maAppData.mpSettings = new AllSettings();
        *pSVData->maAppData.mpSettings = rSettings;
        ResMgr::SetDefaultLocale( rSettings.GetUILocale() ); 
    }
    else
    {
        AllSettings aOldSettings = *pSVData->maAppData.mpSettings;
        if( aOldSettings.GetUILanguage() != rSettings.GetUILanguage() && pSVData->mpResMgr )
        {
            delete pSVData->mpResMgr;
            pSVData->mpResMgr = NULL;
        }
        ResMgr::SetDefaultLocale( rSettings.GetUILocale() ); 
        *pSVData->maAppData.mpSettings = rSettings;
        ULONG nChangeFlags = aOldSettings.GetChangeFlags( *pSVData->maAppData.mpSettings );
        if ( nChangeFlags )
        {
            DataChangedEvent aDCEvt( DATACHANGED_SETTINGS, &aOldSettings, nChangeFlags );
            GetpApp()->DataChanged( aDCEvt );

            // notify data change handler
            ImplCallEventListeners( VCLEVENT_APPLICATION_DATACHANGED, NULL, &aDCEvt);

            // Update all windows
            Window* pFirstFrame = pSVData->maWinData.mpFirstFrame;
            // Daten, die neu berechnet werden muessen, zuruecksetzen
            long nOldDPIX = 0;
            long nOldDPIY = 0;
            if ( pFirstFrame )
            {
                nOldDPIX = pFirstFrame->mnDPIX;
                nOldDPIY = pFirstFrame->mnDPIY;
                pSVData->maGDIData.mnAppFontX = 0;
            }
            Window* pFrame = pFirstFrame;
            while ( pFrame )
            {
                // AppFont-Cache-Daten zuruecksetzen
                pFrame->mpWindowImpl->mpFrameData->meMapUnit = MAP_PIXEL;

                // UpdateSettings am ClientWindow aufrufen, damit
                // die Daten nicht doppelt geupdatet werden
                Window* pClientWin = pFrame;
                while ( pClientWin->ImplGetClientWindow() )
                    pClientWin = pClientWin->ImplGetClientWindow();
                pClientWin->UpdateSettings( rSettings, TRUE );

                Window* pTempWin = pFrame->mpWindowImpl->mpFrameData->mpFirstOverlap;
                while ( pTempWin )
                {
                    // UpdateSettings am ClientWindow aufrufen, damit
                    // die Daten nicht doppelt geupdatet werden
                    pClientWin = pTempWin;
                    while ( pClientWin->ImplGetClientWindow() )
                        pClientWin = pClientWin->ImplGetClientWindow();
                    pClientWin->UpdateSettings( rSettings, TRUE );
                    pTempWin = pTempWin->mpWindowImpl->mpNextOverlap;
                }

                pFrame = pFrame->mpWindowImpl->mpFrameData->mpNextFrame;
            }

            // Wenn sich die DPI-Aufloesung fuer Screen-Ausgaben
            // geaendert hat, setzen wir auch bei allen
            // Screen-Kompatiblen VirDev's die neue Aufloesung
            pFirstFrame = pSVData->maWinData.mpFirstFrame;
            if ( pFirstFrame )
            {
                if ( (pFirstFrame->mnDPIX != nOldDPIX) ||
                     (pFirstFrame->mnDPIY != nOldDPIY) )
                {
                    VirtualDevice* pVirDev = pSVData->maGDIData.mpFirstVirDev;
                    while ( pVirDev )
                    {
                        if ( pVirDev->mbScreenComp &&
                             (pVirDev->mnDPIX == nOldDPIX) &&
                             (pVirDev->mnDPIY == nOldDPIY) )
                        {
                            pVirDev->mnDPIX = pFirstFrame->mnDPIX;
                            pVirDev->mnDPIY = pFirstFrame->mnDPIY;
                            if ( pVirDev->IsMapMode() )
                            {
                                MapMode aMapMode = pVirDev->GetMapMode();
                                pVirDev->SetMapMode();
                                pVirDev->SetMapMode( aMapMode );
                            }
                        }

                        pVirDev = pVirDev->mpNext;
                    }
                }
            }
        }
    }
}

// -----------------------------------------------------------------------

const AllSettings& Application::GetSettings()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( !pSVData->maAppData.mpSettings )
        pSVData->maAppData.mpSettings = new AllSettings();
    return *(pSVData->maAppData.mpSettings);
}

// -----------------------------------------------------------------------

void Application::NotifyAllWindows( DataChangedEvent& rDCEvt )
{
    ImplSVData* pSVData = ImplGetSVData();
    Window*     pFrame = pSVData->maWinData.mpFirstFrame;
    while ( pFrame )
    {
        pFrame->NotifyAllChilds( rDCEvt );

        Window* pSysWin = pFrame->mpWindowImpl->mpFrameData->mpFirstOverlap;
        while ( pSysWin )
        {
            pSysWin->NotifyAllChilds( rDCEvt );
            pSysWin = pSysWin->mpWindowImpl->mpNextOverlap;
        }

        pFrame = pFrame->mpWindowImpl->mpFrameData->mpNextFrame;
    }
}

// -----------------------------------------------------------------------

void Application::ImplCallEventListeners( ULONG nEvent, Window *pWin, void* pData )
{
    ImplSVData* pSVData = ImplGetSVData();
    VclWindowEvent aEvent( pWin, nEvent, pData );

    if ( pSVData->maAppData.mpEventListeners )
        if ( !pSVData->maAppData.mpEventListeners->empty() )
            pSVData->maAppData.mpEventListeners->Call( &aEvent );
}

// -----------------------------------------------------------------------

void Application::ImplCallEventListeners( VclSimpleEvent* pEvent )
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( pSVData->maAppData.mpEventListeners )
        if ( !pSVData->maAppData.mpEventListeners->empty() )
            pSVData->maAppData.mpEventListeners->Call( pEvent );
}

// -----------------------------------------------------------------------

void Application::AddEventListener( const Link& rEventListener )
{
    ImplSVData* pSVData = ImplGetSVData();
    if( !pSVData->maAppData.mpEventListeners )
        pSVData->maAppData.mpEventListeners = new VclEventListeners;
    pSVData->maAppData.mpEventListeners->push_back( rEventListener );
}

// -----------------------------------------------------------------------

void Application::RemoveEventListener( const Link& rEventListener )
{
    ImplSVData* pSVData = ImplGetSVData();
    if( pSVData->maAppData.mpEventListeners )
        pSVData->maAppData.mpEventListeners->remove( rEventListener );
}

// -----------------------------------------------------------------------
void Application::AddKeyListener( const Link& rKeyListener )
{
    ImplSVData* pSVData = ImplGetSVData();
    if( !pSVData->maAppData.mpKeyListeners )
        pSVData->maAppData.mpKeyListeners = new VclEventListeners;
    pSVData->maAppData.mpKeyListeners->push_back( rKeyListener );
}

// -----------------------------------------------------------------------

void Application::RemoveKeyListener( const Link& rKeyListener )
{
    ImplSVData* pSVData = ImplGetSVData();
    if( pSVData->maAppData.mpKeyListeners )
        pSVData->maAppData.mpKeyListeners->remove( rKeyListener );
}

// -----------------------------------------------------------------------

BOOL Application::HandleKey( ULONG nEvent, Window *pWin, KeyEvent* pKeyEvent )
{
    // let listeners process the key event
    VclWindowEvent aEvent( pWin, nEvent, (void *) pKeyEvent );

    ImplSVData* pSVData = ImplGetSVData();
    BOOL bProcessed = FALSE;

    if ( pSVData->maAppData.mpKeyListeners )
        if ( !pSVData->maAppData.mpKeyListeners->empty() )
            bProcessed = pSVData->maAppData.mpKeyListeners->Process( &aEvent );

    return bProcessed;
}

// -----------------------------------------------------------------------------

ULONG Application::PostKeyEvent( ULONG nEvent, Window *pWin, KeyEvent* pKeyEvent )
{
	const ::vos::OGuard	aGuard( GetSolarMutex() );
	ULONG 				nEventId = 0;
	
    if( pWin && pKeyEvent )
    {
		ImplPostEventData* pPostEventData = new ImplPostEventData( nEvent, pWin, *pKeyEvent );
	
        PostUserEvent( nEventId,
					   STATIC_LINK( NULL, Application, PostEventHandler ),
                       pPostEventData );
	
		if( nEventId )
		{
			pPostEventData->mnEventId = nEventId;
			aPostedEventList.push_back( ImplPostEventPair( pWin, pPostEventData ) );
		}			
		else
			delete pPostEventData;
    }

	return nEventId;
}

// -----------------------------------------------------------------------------

ULONG Application::PostMouseEvent( ULONG nEvent, Window *pWin, MouseEvent* pMouseEvent )
{
	const ::vos::OGuard	aGuard( GetSolarMutex() );
	ULONG 				nEventId = 0;
	
    if( pWin && pMouseEvent )
    {
		Point aTransformedPos( pMouseEvent->GetPosPixel() );
		
		aTransformedPos.X() += pWin->mnOutOffX;
		aTransformedPos.Y() += pWin->mnOutOffY;
		
		const MouseEvent aTransformedEvent( aTransformedPos, pMouseEvent->GetClicks(), pMouseEvent->GetMode(),
											pMouseEvent->GetButtons(), pMouseEvent->GetModifier() );

		ImplPostEventData* pPostEventData = new ImplPostEventData( nEvent, pWin, aTransformedEvent );
											
        PostUserEvent( nEventId,
					   STATIC_LINK( NULL, Application, PostEventHandler ),
                       pPostEventData );
	
		if( nEventId )
		{
			pPostEventData->mnEventId = nEventId;
			aPostedEventList.push_back( ImplPostEventPair( pWin, pPostEventData ) );
		}	
		else
			delete pPostEventData;
    }

	return nEventId;
}

// -----------------------------------------------------------------------------

IMPL_STATIC_LINK_NOINSTANCE( Application, PostEventHandler, void*, pCallData )
{
	const ::vos::OGuard	aGuard( GetSolarMutex() );
    ImplPostEventData*	pData = static_cast< ImplPostEventData * >( pCallData );
	const void*			pEventData;
    ULONG               nEvent;
	const ULONG			nEventId = pData->mnEventId;
	
    switch( pData->mnEvent )
    {
        case VCLEVENT_WINDOW_MOUSEMOVE: 
			nEvent = SALEVENT_EXTERNALMOUSEMOVE;
			pEventData = &pData->maMouseEvent; 
		break;
        
		case VCLEVENT_WINDOW_MOUSEBUTTONDOWN: 
			nEvent = SALEVENT_EXTERNALMOUSEBUTTONDOWN;
			pEventData = &pData->maMouseEvent;
		break;
        
		case VCLEVENT_WINDOW_MOUSEBUTTONUP: 
			nEvent = SALEVENT_EXTERNALMOUSEBUTTONUP; 
			pEventData = &pData->maMouseEvent;
		break;

        case VCLEVENT_WINDOW_KEYINPUT: 
			nEvent = SALEVENT_EXTERNALKEYINPUT; 
			pEventData = &pData->maKeyEvent;
		break;
        
		case VCLEVENT_WINDOW_KEYUP: 
			nEvent = SALEVENT_EXTERNALKEYUP; 
			pEventData = &pData->maKeyEvent;
		break;

        default:
			nEvent = 0;
			pEventData = NULL;
		break;
    };

    if( pData->mpWin && pData->mpWin->mpWindowImpl->mpFrameWindow && pEventData )
		ImplWindowFrameProc( pData->mpWin->mpWindowImpl->mpFrameWindow, NULL, (USHORT) nEvent, pEventData );
		
	// remove this event from list of posted events, watch for destruction of internal data
	::std::list< ImplPostEventPair >::iterator aIter( aPostedEventList.begin() );
	
	while( aIter != aPostedEventList.end() )
	{
		if( nEventId == (*aIter).second->mnEventId )
		{
			delete (*aIter).second;
			aIter = aPostedEventList.erase( aIter );
		}
		else
			++aIter;
	}
	
    return 0;
}

// -----------------------------------------------------------------------

void Application::RemoveMouseAndKeyEvents( Window* pWin )
{
	const ::vos::OGuard	aGuard( GetSolarMutex() );
	
	// remove all events for specific window, watch for destruction of internal data
	::std::list< ImplPostEventPair >::iterator aIter( aPostedEventList.begin() );
	
	while( aIter != aPostedEventList.end() )
	{
		if( pWin == (*aIter).first )
		{
			if( (*aIter).second->mnEventId )
				RemoveUserEvent( (*aIter).second->mnEventId );
			
			delete (*aIter).second;
			aIter = aPostedEventList.erase( aIter );
		}
		else
			++aIter;
	}
}

// -----------------------------------------------------------------------

BOOL Application::IsProcessedMouseOrKeyEvent( ULONG nEventId )
{
    const ::vos::OGuard aGuard( GetSolarMutex() );
    
    // find event
    ::std::list< ImplPostEventPair >::iterator aIter( aPostedEventList.begin() );
    
    while( aIter != aPostedEventList.end() )
    {
        if( (*aIter).second->mnEventId == nEventId )
            return FALSE;
            
        else
            ++aIter;
    }
    return TRUE;
}

// -----------------------------------------------------------------------

ULONG Application::PostUserEvent( ULONG nEvent, void* pEventData )
{
    ULONG nEventId;
    PostUserEvent( nEventId, nEvent, pEventData );
    return nEventId;
}

// -----------------------------------------------------------------------

ULONG Application::PostUserEvent( const Link& rLink, void* pCaller )
{
    ULONG nEventId;
    PostUserEvent( nEventId, rLink, pCaller );
    return nEventId;
}

// -----------------------------------------------------------------------

BOOL Application::PostUserEvent( ULONG& rEventId, ULONG nEvent, void* pEventData )
{
    ImplSVEvent* pSVEvent = new ImplSVEvent;
    pSVEvent->mnEvent   = nEvent;
    pSVEvent->mpData    = pEventData;
    pSVEvent->mpLink    = NULL;
    pSVEvent->mpWindow  = NULL;
    pSVEvent->mbCall    = TRUE;
    rEventId = (ULONG)pSVEvent;
    Window* pDefWindow = ImplGetDefaultWindow();
    if ( pDefWindow && pDefWindow->ImplGetFrame()->PostEvent( pSVEvent ) )
        return TRUE;
    else
    {
        rEventId = 0;
        delete pSVEvent;
        return FALSE;
    }
}

// -----------------------------------------------------------------------

BOOL Application::PostUserEvent( ULONG& rEventId, const Link& rLink, void* pCaller )
{
    ImplSVEvent* pSVEvent = new ImplSVEvent;
    pSVEvent->mnEvent   = 0;
    pSVEvent->mpData    = pCaller;
    pSVEvent->mpLink    = new Link( rLink );
    pSVEvent->mpWindow  = NULL;
    pSVEvent->mbCall    = TRUE;
    rEventId = (ULONG)pSVEvent;
    Window* pDefWindow = ImplGetDefaultWindow();
    if ( pDefWindow && pDefWindow->ImplGetFrame()->PostEvent( pSVEvent ) )
        return TRUE;
    else
    {
        rEventId = 0;
        delete pSVEvent;
        return FALSE;
    }
}

// -----------------------------------------------------------------------

void Application::RemoveUserEvent( ULONG nUserEvent )
{
    if(nUserEvent)
	{
		ImplSVEvent* pSVEvent = (ImplSVEvent*)nUserEvent;

		DBG_ASSERT( !pSVEvent->mpWindow,
					"Application::RemoveUserEvent(): Event is send to a window" );
		DBG_ASSERT( pSVEvent->mbCall,
					"Application::RemoveUserEvent(): Event is already removed" );

		if ( pSVEvent->mpWindow )
		{
            if( ! pSVEvent->maDelData.IsDelete() )
                pSVEvent->mpWindow->ImplRemoveDel( &(pSVEvent->maDelData) );
			pSVEvent->mpWindow = NULL;
		}

		pSVEvent->mbCall = FALSE;
	}
}

// -----------------------------------------------------------------------

BOOL Application::InsertIdleHdl( const Link& rLink, USHORT nPrio )
{
    ImplSVData* pSVData = ImplGetSVData();

    // Falls er noch nicht existiert, dann anlegen
    if ( !pSVData->maAppData.mpIdleMgr )
        pSVData->maAppData.mpIdleMgr = new ImplIdleMgr;

    return pSVData->maAppData.mpIdleMgr->InsertIdleHdl( rLink, nPrio );
}

// -----------------------------------------------------------------------

void Application::RemoveIdleHdl( const Link& rLink )
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( pSVData->maAppData.mpIdleMgr )
        pSVData->maAppData.mpIdleMgr->RemoveIdleHdl( rLink );
}

// -----------------------------------------------------------------------

WorkWindow* Application::GetAppWindow()
{
    return ImplGetSVData()->maWinData.mpAppWin;
}

// -----------------------------------------------------------------------

Window* Application::GetFocusWindow()
{
    return ImplGetSVData()->maWinData.mpFocusWin;
}

// -----------------------------------------------------------------------

OutputDevice* Application::GetDefaultDevice()
{
    return ImplGetDefaultWindow();
}

// -----------------------------------------------------------------------

Window* Application::GetFirstTopLevelWindow()
{
    ImplSVData* pSVData = ImplGetSVData();
    return pSVData->maWinData.mpFirstFrame;
}

// -----------------------------------------------------------------------

Window* Application::GetNextTopLevelWindow( Window* pWindow )
{
    return pWindow->mpWindowImpl->mpFrameData->mpNextFrame;
}

// -----------------------------------------------------------------------

long    Application::GetTopWindowCount()
{
    long nRet = 0;
    ImplSVData* pSVData = ImplGetSVData();
    Window *pWin = pSVData ? pSVData->maWinData.mpFirstFrame : NULL;
    while( pWin )
    {
        if( pWin->ImplGetWindow()->IsTopWindow() )
            nRet++;
        pWin = pWin->mpWindowImpl->mpFrameData->mpNextFrame;
    }
    return nRet;
}

// -----------------------------------------------------------------------

Window* Application::GetTopWindow( long nIndex )
{
    long nIdx = 0;
    ImplSVData* pSVData = ImplGetSVData();
    Window *pWin = pSVData ? pSVData->maWinData.mpFirstFrame : NULL;
    while( pWin )
    {
        if( pWin->ImplGetWindow()->IsTopWindow() )
        {
            if( nIdx == nIndex )
                return pWin->ImplGetWindow();
            else
                nIdx++;
        }
        pWin = pWin->mpWindowImpl->mpFrameData->mpNextFrame;
    }
    return NULL;
}

// -----------------------------------------------------------------------

Window* Application::GetActiveTopWindow()
{
    Window *pWin = ImplGetSVData()->maWinData.mpFocusWin;
    while( pWin )
    {
        if( pWin->IsTopWindow() )
            return pWin;
        pWin = pWin->mpWindowImpl->mpParent;
    }
    return NULL;
}

// -----------------------------------------------------------------------

void Application::SetAppName( const XubString& rUniqueName )
{
    ImplSVData* pSVData = ImplGetSVData();

    // Falls er noch nicht existiert, dann anlegen
    if ( !pSVData->maAppData.mpAppName )
        pSVData->maAppData.mpAppName = new XubString( rUniqueName );
    else
        *(pSVData->maAppData.mpAppName) = rUniqueName;
}

// -----------------------------------------------------------------------

XubString Application::GetAppName()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( pSVData->maAppData.mpAppName )
        return *(pSVData->maAppData.mpAppName);
    else
        return ImplGetSVEmptyStr();
}

// -----------------------------------------------------------------------

void Application::SetDisplayName( const UniString& rName )
{
    ImplSVData* pSVData = ImplGetSVData();

    // Falls er noch nicht existiert, dann anlegen
    if ( !pSVData->maAppData.mpDisplayName )
        pSVData->maAppData.mpDisplayName = new UniString( rName );
    else
        *(pSVData->maAppData.mpDisplayName) = rName;
}

// -----------------------------------------------------------------------

UniString Application::GetDisplayName()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( pSVData->maAppData.mpDisplayName )
        return *(pSVData->maAppData.mpDisplayName);
    else if ( pSVData->maWinData.mpAppWin )
        return pSVData->maWinData.mpAppWin->GetText();
    else
        return ImplGetSVEmptyStr();
}

// -----------------------------------------------------------------------

unsigned int Application::GetScreenCount()
{
    SalSystem* pSys = ImplGetSalSystem();
    return pSys ? pSys->GetDisplayScreenCount() : 0;
}

rtl::OUString Application::GetScreenName( unsigned int nScreen )
{
    SalSystem* pSys = ImplGetSalSystem();
    return pSys ? pSys->GetScreenName( nScreen ) : rtl::OUString();
}

bool Application::IsMultiDisplay()
{
    SalSystem* pSys = ImplGetSalSystem();
    return pSys ? pSys->IsMultiDisplay() : false;
}

unsigned int Application::GetDefaultDisplayNumber()
{
    SalSystem* pSys = ImplGetSalSystem();
    return pSys ? pSys->GetDefaultDisplayNumber() : 0;
}

Rectangle Application::GetScreenPosSizePixel( unsigned int nScreen )
{
    SalSystem* pSys = ImplGetSalSystem();
    return pSys ? pSys->GetDisplayScreenPosSizePixel( nScreen ) : Rectangle();
}

Rectangle Application::GetWorkAreaPosSizePixel( unsigned int nScreen )
{
    SalSystem* pSys = ImplGetSalSystem();
    return pSys ? pSys->GetDisplayWorkAreaPosSizePixel( nScreen ) : Rectangle();
}

namespace {
unsigned long calcDistSquare( const Point& i_rPoint, const Rectangle& i_rRect )
{
    const Point aRectCenter( (i_rRect.Left() + i_rRect.Right())/2,
                       (i_rRect.Top() + i_rRect.Bottom())/ 2 );
    const long nDX = aRectCenter.X() - i_rPoint.X();
    const long nDY = aRectCenter.Y() - i_rPoint.Y();
    return nDX*nDX + nDY*nDY;
}
}

unsigned int Application::GetBestScreen( const Rectangle& i_rRect )
{
    if( IsMultiDisplay() )
        return GetDefaultDisplayNumber();
    
    const unsigned int nScreens = GetScreenCount();    
    unsigned int nBestMatchScreen = 0;
    unsigned long nOverlap = 0;
    for( unsigned int i = 0; i < nScreens; i++ )
    {
        const Rectangle aCurScreenRect( GetScreenPosSizePixel( i ) );
        // if a screen contains the rectangle completely it is obviously the best screen
        if( aCurScreenRect.IsInside( i_rRect ) )
            return i;
        // next the screen which contains most of the area of the rect is the best
        Rectangle aIntersection( aCurScreenRect.GetIntersection( i_rRect ) );
        if( ! aIntersection.IsEmpty() )
        {
            const unsigned long nCurOverlap( aIntersection.GetWidth() * aIntersection.GetHeight() );
            if( nCurOverlap > nOverlap )
            {
                nOverlap = nCurOverlap;
                nBestMatchScreen = i;
            }
        }
    }
    if( nOverlap > 0 )
        return nBestMatchScreen;
    
    // finally the screen which center is nearest to the rect is the best
    const Point aCenter( (i_rRect.Left() + i_rRect.Right())/2,
                         (i_rRect.Top() + i_rRect.Bottom())/2 );
    unsigned long nDist = ULONG_MAX;
    for( unsigned int i = 0; i < nScreens; i++ )
    {
        const Rectangle aCurScreenRect( GetScreenPosSizePixel( i ) );
        const unsigned long nCurDist( calcDistSquare( aCenter, aCurScreenRect ) );
        if( nCurDist < nDist )
        {
            nBestMatchScreen = i;
            nDist = nCurDist;
        }
    }
    return nBestMatchScreen;
}

// -----------------------------------------------------------------------

BOOL Application::InsertAccel( Accelerator* pAccel )
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( !pSVData->maAppData.mpAccelMgr )
        pSVData->maAppData.mpAccelMgr = new ImplAccelManager();
    return pSVData->maAppData.mpAccelMgr->InsertAccel( pAccel );
}

// -----------------------------------------------------------------------

void Application::RemoveAccel( Accelerator* pAccel )
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( pSVData->maAppData.mpAccelMgr )
        pSVData->maAppData.mpAccelMgr->RemoveAccel( pAccel );
}

// -----------------------------------------------------------------------

void Application::FlushAccel()
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( pSVData->maAppData.mpAccelMgr )
        pSVData->maAppData.mpAccelMgr->FlushAccel();
}

// -----------------------------------------------------------------------

BOOL Application::CallAccel( const KeyCode& rKeyCode, USHORT nRepeat )
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( pSVData->maAppData.mpAccelMgr )
    {
        if ( pSVData->maAppData.mpAccelMgr->IsAccelKey( rKeyCode, nRepeat ) )
            return TRUE;
    }

    return FALSE;
}

// -----------------------------------------------------------------------

void Application::SetHelp( Help* pHelp )
{
    ImplGetSVData()->maAppData.mpHelp = pHelp;
}

// -----------------------------------------------------------------------

Help* Application::GetHelp()
{
    return ImplGetSVData()->maAppData.mpHelp;
}

// -----------------------------------------------------------------------

void Application::EnableAutoHelpId( BOOL bEnabled )
{
    ImplGetSVData()->maHelpData.mbAutoHelpId = bEnabled;
}

// -----------------------------------------------------------------------

BOOL Application::IsAutoHelpIdEnabled()
{
    return ImplGetSVData()->maHelpData.mbAutoHelpId;
}

// -----------------------------------------------------------------------

void Application::EnableAutoMnemonic( BOOL bEnabled )
{
    AllSettings aSettings = GetSettings();
    StyleSettings aStyle = aSettings.GetStyleSettings();
    aStyle.SetAutoMnemonic( bEnabled );
    aSettings.SetStyleSettings( aStyle );
    SetSettings( aSettings );
}

// -----------------------------------------------------------------------

BOOL Application::IsAutoMnemonicEnabled()
{
    return GetSettings().GetStyleSettings().GetAutoMnemonic();
}

// -----------------------------------------------------------------------

void Application::SetDialogScaleX( short nScale )
{
    ImplSVData* pSVData = ImplGetSVData();
    pSVData->maAppData.mnDialogScaleX = nScale;
    pSVData->maGDIData.mnAppFontX = pSVData->maGDIData.mnRealAppFontX;
    if ( nScale )
        pSVData->maGDIData.mnAppFontX += (pSVData->maGDIData.mnAppFontX*nScale)/100;
}

// -----------------------------------------------------------------------

short Application::GetDialogScaleX()
{
    return ImplGetSVData()->maAppData.mnDialogScaleX;
}

// -----------------------------------------------------------------------

void Application::SetDefDialogParent( Window* pWindow )
{
    ImplGetSVData()->maWinData.mpDefDialogParent = pWindow;
}

// -----------------------------------------------------------------------

Window* Application::GetDefDialogParent()
{
    ImplSVData* pSVData = ImplGetSVData();
    // #103442# find some useful dialog parent if there
    // was no default set
    // NOTE: currently even the default is not used
    if( FALSE && pSVData->maWinData.mpDefDialogParent != NULL )
        return pSVData->maWinData.mpDefDialogParent;
    else
    {
        // always use the topmost parent of the candidate
        // window to avoid using dialogs or floaters
        // as DefDialogParent

        // current focus frame
        Window *pWin = NULL;
        if( (pWin = pSVData->maWinData.mpFocusWin) != NULL )
        {
            while( pWin->mpWindowImpl && pWin->mpWindowImpl->mpParent )
                pWin = pWin->mpWindowImpl->mpParent;

            if( (pWin->mpWindowImpl->mnStyle & WB_INTROWIN) == 0 )
            {
                // check for corrupted window hierarchy, #122232#, may be we now crash somewhere else
                if( !pWin->mpWindowImpl )
                {
                    DBG_ERROR( "Window hierarchy corrupted!" );
                    pSVData->maWinData.mpFocusWin = NULL;   // avoid further access
                    return NULL;       
                }
    
                // MAV: before the implementation has used only decorated windows,
                //      but it is not true in case of ActiveX or plugin scenario,
                //      so this check is commented out 
                // if( pWin->mpWindowImpl->mpFrameWindow->GetStyle() & (WB_MOVEABLE | WB_SIZEABLE) )
                    return pWin->mpWindowImpl->mpFrameWindow->ImplGetWindow();
                // else
                //    return NULL;
            }
        }
        // last active application frame
        if( NULL != (pWin = pSVData->maWinData.mpActiveApplicationFrame) )
        {
            return pWin->mpWindowImpl->mpFrameWindow->ImplGetWindow();
        }
        else
        {
            // first visible top window (may be totally wrong....)
            pWin = pSVData->maWinData.mpFirstFrame;
            while( pWin )
            {
                if( pWin->ImplGetWindow()->IsTopWindow() &&
                    pWin->mpWindowImpl->mbReallyVisible &&
                    (pWin->mpWindowImpl->mnStyle & WB_INTROWIN) == 0
                )
                {
                    while( pWin->mpWindowImpl->mpParent )
                        pWin = pWin->mpWindowImpl->mpParent;
                    return pWin->mpWindowImpl->mpFrameWindow->ImplGetWindow();
                }
                pWin = pWin->mpWindowImpl->mpFrameData->mpNextFrame;
            }
            // use the desktop
            return NULL;
        }
    }
}

// -----------------------------------------------------------------------

void Application::EnableDialogCancel( BOOL bDialogCancel )
{
    ImplGetSVData()->maAppData.mbDialogCancel = bDialogCancel;
}

// -----------------------------------------------------------------------

BOOL Application::IsDialogCancelEnabled()
{
    return ImplGetSVData()->maAppData.mbDialogCancel;
}

// -----------------------------------------------------------------------

void Application::SetSystemWindowMode( USHORT nMode )
{
    ImplGetSVData()->maAppData.mnSysWinMode = nMode;
}

// -----------------------------------------------------------------------

USHORT Application::GetSystemWindowMode()
{
    return ImplGetSVData()->maAppData.mnSysWinMode;
}

// -----------------------------------------------------------------------

const String& Application::GetFontPath()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( !pSVData->maAppData.mpFontPath )
    {
        if( const char* pFontPath = ::getenv( "SAL_FONTPATH_PRIVATE" ) )
            pSVData->maAppData.mpFontPath = new String( String::CreateFromAscii( pFontPath ) );
    }

    if( pSVData->maAppData.mpFontPath )
        return *(pSVData->maAppData.mpFontPath);
    return ImplGetSVEmptyStr();
}

// -----------------------------------------------------------------------

void Application::SetFontPath( const String& rPath )
{
    ImplSVData* pSVData = ImplGetSVData();

    // if it doesn't exist create a new one
    if( !pSVData->maAppData.mpFontPath )
        pSVData->maAppData.mpFontPath = new String( rPath );
    else
        *(pSVData->maAppData.mpFontPath) = rPath;
}

// -----------------------------------------------------------------------

UniqueItemId Application::CreateUniqueId()
{
    ImplSVData* pSVData = ImplGetSVData();

    if ( !pSVData->maAppData.mpUniqueIdCont )
        pSVData->maAppData.mpUniqueIdCont = new UniqueIdContainer( UNIQUEID_SV_BEGIN );
    return pSVData->maAppData.mpUniqueIdCont->CreateId();
}

// -----------------------------------------------------------------------

::com::sun::star::uno::Reference< ::com::sun::star::awt::XToolkit > Application::GetVCLToolkit()
{
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XToolkit > xT;
    UnoWrapperBase* pWrapper = Application::GetUnoWrapper( TRUE );
    if ( pWrapper )
        xT = pWrapper->GetVCLToolkit();
    return xT;
}

// -----------------------------------------------------------------------

extern "C" { static void SAL_CALL thisModule() {} }

UnoWrapperBase* Application::GetUnoWrapper( BOOL bCreateIfNotExist )
{
    ImplSVData* pSVData = ImplGetSVData();
    static BOOL bAlreadyTriedToCreate = FALSE;
    if ( !pSVData->mpUnoWrapper && bCreateIfNotExist && !bAlreadyTriedToCreate )
    {
        ::rtl::OUString aLibName = ::vcl::unohelper::CreateLibraryName( "tk", TRUE );
        oslModule hTkLib = osl_loadModuleRelative(
            &thisModule, aLibName.pData, SAL_LOADMODULE_DEFAULT );
        if ( hTkLib )
        {
            ::rtl::OUString aFunctionName( RTL_CONSTASCII_USTRINGPARAM( "CreateUnoWrapper" ) );
            FN_TkCreateUnoWrapper fnCreateWrapper = (FN_TkCreateUnoWrapper)osl_getFunctionSymbol( hTkLib, aFunctionName.pData );
            if ( fnCreateWrapper )
            {
                pSVData->mpUnoWrapper = fnCreateWrapper();
            }
        }
        DBG_ASSERT( pSVData->mpUnoWrapper, "UnoWrapper could not be created!" );
        bAlreadyTriedToCreate = TRUE;
    }
    return pSVData->mpUnoWrapper;
}

// -----------------------------------------------------------------------

void Application::SetUnoWrapper( UnoWrapperBase* pWrapper )
{
    ImplSVData* pSVData = ImplGetSVData();
    DBG_ASSERT( !pSVData->mpUnoWrapper, "SetUnoWrapper: Wrapper allready exists" );
    pSVData->mpUnoWrapper = pWrapper;
}

// -----------------------------------------------------------------------

::com::sun::star::uno::Reference< ::com::sun::star::awt::XDisplayConnection > Application::GetDisplayConnection()
{
    ImplSVData* pSVData = ImplGetSVData();

    if( !pSVData->mxDisplayConnection.is() )
        pSVData->mxDisplayConnection.set( new ::vcl::DisplayConnection );

    return pSVData->mxDisplayConnection;
}

// -----------------------------------------------------------------------

void Application::SetFilterHdl( const Link& rLink )
{
    ImplGetSVData()->maGDIData.mpGrfConverter->SetFilterHdl( rLink );
}

// -----------------------------------------------------------------------

const Link& Application::GetFilterHdl()
{
    return ImplGetSVData()->maGDIData.mpGrfConverter->GetFilterHdl();
}

// -----------------------------------------------------------------------

BOOL ImplCallHotKey( const KeyCode& rKeyCode )
{
    ImplSVData*     pSVData = ImplGetSVData();
    ImplHotKey*     pHotKeyData = pSVData->maAppData.mpFirstHotKey;
    while ( pHotKeyData )
    {
        if ( pHotKeyData->maKeyCode.IsDefinedKeyCodeEqual( rKeyCode ) )
        {
            pHotKeyData->maLink.Call( pHotKeyData->mpUserData );
            return TRUE;
        }

        pHotKeyData = pHotKeyData->mpNext;
    }

    return FALSE;
}

// -----------------------------------------------------------------------

void ImplFreeHotKeyData()
{
    ImplSVData*     pSVData = ImplGetSVData();
    ImplHotKey*     pTempHotKeyData;
    ImplHotKey*     pHotKeyData = pSVData->maAppData.mpFirstHotKey;
    while ( pHotKeyData )
    {
        pTempHotKeyData = pHotKeyData->mpNext;
        delete pHotKeyData;
        pHotKeyData = pTempHotKeyData;
    }

    pSVData->maAppData.mpFirstHotKey = NULL;
}

// -----------------------------------------------------------------------

ULONG Application::AddHotKey( const KeyCode& rKeyCode, const Link& rLink, void* pData )
{
    ImplSVData*     pSVData = ImplGetSVData();
    ImplHotKey*     pHotKeyData = new ImplHotKey;
    pHotKeyData->mpUserData = pData;
    pHotKeyData->maKeyCode  = rKeyCode;
    pHotKeyData->maLink     = rLink;
    pHotKeyData->mpNext     = pSVData->maAppData.mpFirstHotKey;
    pSVData->maAppData.mpFirstHotKey = pHotKeyData;
    return (ULONG)pHotKeyData;
}

// -----------------------------------------------------------------------

void Application::RemoveHotKey( ULONG nId )
{
    ImplSVData*     pSVData = ImplGetSVData();
    ImplHotKey*     pFindHotKeyData = (ImplHotKey*)nId;
    ImplHotKey*     pPrevHotKeyData = NULL;
    ImplHotKey*     pHotKeyData = pSVData->maAppData.mpFirstHotKey;
    while ( pHotKeyData )
    {
        if ( pHotKeyData == pFindHotKeyData )
        {
            if ( pPrevHotKeyData )
                pPrevHotKeyData->mpNext = pFindHotKeyData->mpNext;
            else
                pSVData->maAppData.mpFirstHotKey = pFindHotKeyData->mpNext;
            delete pFindHotKeyData;
            break;
        }

        pPrevHotKeyData = pHotKeyData;
        pHotKeyData = pHotKeyData->mpNext;
    }

    DBG_ASSERT( pHotKeyData, "Application::RemoveHotKey() - HotKey is not added" );
}

// -----------------------------------------------------------------------

void ImplFreeEventHookData()
{
    ImplSVData*     pSVData = ImplGetSVData();
    ImplEventHook*  pTempEventHookData;
    ImplEventHook*  pEventHookData = pSVData->maAppData.mpFirstEventHook;
    while ( pEventHookData )
    {
        pTempEventHookData = pEventHookData->mpNext;
        delete pEventHookData;
        pEventHookData = pTempEventHookData;
    }

    pSVData->maAppData.mpFirstEventHook = NULL;
}

// -----------------------------------------------------------------------

ULONG Application::AddEventHook( VCLEventHookProc pProc, void* pData )
{
    ImplSVData*     pSVData = ImplGetSVData();
    ImplEventHook*  pEventHookData = new ImplEventHook;
    pEventHookData->mpUserData = pData;
    pEventHookData->mpProc     = pProc;
    pEventHookData->mpNext     = pSVData->maAppData.mpFirstEventHook;
    pSVData->maAppData.mpFirstEventHook = pEventHookData;
    return (ULONG)pEventHookData;
}

// -----------------------------------------------------------------------

void Application::RemoveEventHook( ULONG nId )
{
    ImplSVData*     pSVData = ImplGetSVData();
    ImplEventHook*  pFindEventHookData = (ImplEventHook*)nId;
    ImplEventHook*  pPrevEventHookData = NULL;
    ImplEventHook*  pEventHookData = pSVData->maAppData.mpFirstEventHook;
    while ( pEventHookData )
    {
        if ( pEventHookData == pFindEventHookData )
        {
            if ( pPrevEventHookData )
                pPrevEventHookData->mpNext = pFindEventHookData->mpNext;
            else
                pSVData->maAppData.mpFirstEventHook = pFindEventHookData->mpNext;
            delete pFindEventHookData;
            break;
        }

        pPrevEventHookData = pEventHookData;
        pEventHookData = pEventHookData->mpNext;
    }

    DBG_ASSERT( pEventHookData, "Application::RemoveEventHook() - EventHook is not added" );
}

// -----------------------------------------------------------------------

long Application::CallEventHooks( NotifyEvent& rEvt )
{
    ImplSVData*     pSVData = ImplGetSVData();
    long            nRet = 0;
    ImplEventHook*  pTempEventHookData;
    ImplEventHook*  pEventHookData = pSVData->maAppData.mpFirstEventHook;
    while ( pEventHookData )
    {
        pTempEventHookData = pEventHookData->mpNext;
        nRet = pEventHookData->mpProc( rEvt, pEventHookData->mpUserData );
        if ( nRet )
            break;
        pEventHookData = pTempEventHookData;
    }

    return nRet;
}

// -----------------------------------------------------------------------

long Application::CallPreNotify( NotifyEvent& rEvt )
{
    return ImplCallPreNotify( rEvt );
}

// -----------------------------------------------------------------------

long Application::CallEvent( NotifyEvent& rEvt )
{
    return ImplCallEvent( rEvt );
}

// -----------------------------------------------------------------------

const LocaleDataWrapper& Application::GetAppLocaleDataWrapper()
{
    return GetSettings().GetLocaleDataWrapper();
}

// -----------------------------------------------------------------------

void Application::EnableHeadlessMode( BOOL bEnable )
{
    EnableDialogCancel( bEnable );
}

// -----------------------------------------------------------------------

BOOL Application::IsHeadlessModeEnabled()
{
    return IsDialogCancelEnabled();
}

// -----------------------------------------------------------------------

void Application::ShowNativeErrorBox(const String& sTitle  ,
                                     const String& sMessage)
{
    ImplGetSalSystem()->ShowNativeMessageBox (
            sTitle,
            sMessage,
            SALSYSTEM_SHOWNATIVEMSGBOX_BTNCOMBI_OK,
            SALSYSTEM_SHOWNATIVEMSGBOX_BTN_OK);
}

// -----------------------------------------------------------------------

bool Application::CanToggleImeStatusWindow()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->mpImeStatus )
        pSVData->mpImeStatus  = pSVData->mpDefInst->CreateI18NImeStatus();
    return pSVData->mpImeStatus->canToggle();
}

void Application::ShowImeStatusWindow(bool bShow)
{
    ImplGetSVData()->maAppData.meShowImeStatusWindow = bShow
        ? ImplSVAppData::ImeStatusWindowMode_SHOW
        : ImplSVAppData::ImeStatusWindowMode_HIDE;

    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->mpImeStatus )
        pSVData->mpImeStatus  = pSVData->mpDefInst->CreateI18NImeStatus();
    pSVData->mpImeStatus->toggle();
}

bool Application::GetShowImeStatusWindowDefault()
{
    rtl_TextEncodingInfo aInfo;
    aInfo.StructSize = sizeof aInfo;
    return rtl_getTextEncodingInfo(osl_getThreadTextEncoding(), &aInfo)
        && aInfo.MaximumCharSize > 1;
}

const ::rtl::OUString& Application::GetDesktopEnvironment()
{
    return SalGetDesktopEnvironment();
}

void Application::AddToRecentDocumentList(const rtl::OUString& rFileUrl, const rtl::OUString& rMimeType)
{
    ImplSVData* pSVData = ImplGetSVData();
    pSVData->mpDefInst->AddToRecentDocumentList(rFileUrl, rMimeType);
}

BOOL Application::IsAccessibilityEnabled()
{
    return FALSE;
}

BOOL InitAccessBridge( BOOL bShowCancel, BOOL &rCancelled )
{
    BOOL bRet = true;

// Disable Java bridge on UNIX
#if defined UNX
    (void) bShowCancel; // unsued
    (void) rCancelled; // unused
#else
    bRet = ImplInitAccessBridge( bShowCancel, rCancelled );
    
    if( !bRet && bShowCancel && !rCancelled )
    {
        // disable accessibility if the user chooses to continue
        AllSettings aSettings = Application::GetSettings();
        MiscSettings aMisc = aSettings.GetMiscSettings();
        aMisc.SetEnableATToolSupport( FALSE );
        aSettings.SetMiscSettings( aMisc );
        Application::SetSettings( aSettings );
    }
#endif // !UNX

    return bRet;
}

// MT: AppProperty, AppEvent was in oldsv.cxx, but is still needed...
// ------------------------------------------------------------------------

TYPEINIT0(ApplicationProperty)

// ------------------------------------------------------------------------

static PropertyHandler* pHandler=NULL;

void Application::Property( ApplicationProperty& rProp )
{
    if ( pHandler )
        pHandler->Property( rProp );
}

void Application::SetPropertyHandler( PropertyHandler* p )
{
    if ( pHandler )
        delete pHandler;
    pHandler = p;
}



void Application::AppEvent( const ApplicationEvent& /*rAppEvent*/ )
{
}
