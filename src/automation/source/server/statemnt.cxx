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
#include "precompiled_automation.hxx"
#include <com/sun/star/frame/XFramesSupplier.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/uieventslogger.hxx>

#include <vcl/wintypes.hxx>
#ifndef _DIALOG_HXX //autogen
#include <vcl/dialog.hxx>
#endif
#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#include <vcl/menubtn.hxx>
#include <svtools/svtreebx.hxx>
#include <svtools/brwbox.hxx>
#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#ifndef _DOCKWIN_HXX //autogen
#include <vcl/dockwin.hxx>
#endif
#ifndef _FLOATWIN_HXX //autogen
#include <vcl/floatwin.hxx>
#endif
#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif
#ifndef _MOREBTN_HXX //autogen
#include <vcl/morebtn.hxx>
#endif
#ifndef _FIELD_HXX //autogen
#include <vcl/field.hxx>
#endif
#ifndef _TOOLBOX_HXX //autogen
#include <vcl/toolbox.hxx>
#endif
#include <vcl/tabctrl.hxx>
#include <vcl/tabpage.hxx>
#include <vcl/menu.hxx>
#include <vcl/status.hxx>
#include <svtools/prgsbar.hxx>
#include <svtools/editbrowsebox.hxx>
#include <vcl/splitwin.hxx>
#include <vcl/group.hxx>
#include <vcl/fixed.hxx>
#include <vcl/wrkwin.hxx>
#include <osl/diagnose.h>
#include <vcl/wintypes.hxx>
#include <svtools/valueset.hxx>
#include <svtools/roadmap.hxx>
#include <svtools/poolitem.hxx>
#include <svtools/extensionlistbox.hxx>
// Hat keinen Includeschutz
#include <svtools/svtdata.hxx>
#include <tools/time.hxx>
#include <svtools/stringtransfer.hxx>
#ifndef _PRINT_HXX //autogen
#include <vcl/print.hxx>
#endif
#ifndef _PRNDLG_HXX //autogen
#include <vcl/prndlg.hxx>
#endif
#include <tools/stream.hxx>
#ifndef _SV_DRAG_HXX //autogen
//#include <vcl/drag.hxx>
#endif
#include <tools/fsys.hxx>
#include <svtools/stritem.hxx>
#include <svtools/ttprops.hxx>
#ifndef _BASIC_TTRESHLP_HXX
#include <basic/ttstrhlp.hxx>
#endif
#include <basic/dispdefs.hxx>
#include <vos/socket.hxx>
#include <svtools/pickerhistory.hxx>
#include <com/sun/star/util/XCancellable.hpp>

#include <sot/storage.hxx>
#include <sot/storinfo.hxx>
#include "statemnt.hxx"
#include "scmdstrm.hxx"

#ifndef _RETSRTM_HXX
#include "retstrm.hxx"
#endif

#if OSL_DEBUG_LEVEL > 1
#include "editwin.hxx"
#endif
#include "rcontrol.hxx"
#include <automation/communi.hxx>
#include "testtool.hxx"

#include "profiler.hxx"

#include "recorder.hxx"

#include "testtool.hrc"
#include <basic/svtmsg.hrc>

#include <algorithm>


using namespace com::sun::star::frame;
using namespace com::sun::star::uno;
using namespace com::sun::star::util;
using namespace com::sun::star::beans;
using namespace svt;


#ifndef SBX_VALUE_DECL_DEFINED
#define SBX_VALUE_DECL_DEFINED
SV_DECL_REF(SbxValue)
#endif
SV_IMPL_REF(SbxValue)

CommunicationLink *StatementFlow::pCommLink = NULL;
BOOL StatementFlow::bUseIPC = TRUE;
BOOL StatementFlow::bSending = FALSE;
ImplRemoteControl *StatementFlow::pRemoteControl = NULL;

USHORT StatementCommand::nDirPos = 0;
Dir *StatementCommand::pDir = NULL;
pfunc_osl_printDebugMessage StatementCommand::pOriginal_osl_DebugMessageFunc = NULL;


#define RESET_APPLICATION_TO_BACKING_WINDOW


#define SET_WINP_CLOSING(pWin) \
	pWindowWaitPointer = pWin; \
	aWindowWaitUId = pControl->GetSmartUniqueOrHelpId(); \
	aWindowWaitOldHelpId = pWin->GetSmartHelpId(); \
	aWindowWaitOldUniqueId = pWin->GetSmartUniqueId(); \
	pWin->SetSmartHelpId( SmartId(4321) ); \
	pWin->SetSmartUniqueId( SmartId(1234) );

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

StatementFlow::StatementFlow( StatementList *pAfterThis, USHORT nArtP )
: nArt(nArtP)
, nParams(0)
, nSNr1(0)
, nLNr1(0)
, aString1()
, bBool1(FALSE)
{
	QueStatement( pAfterThis );
}

StatementFlow::StatementFlow( ULONG nServiceId, SCmdStream *pCmdIn, ImplRemoteControl *pRC )
: nArt(0)
, nParams(0)
, nSNr1(0)
, nLNr1(0)
, aString1()
, bBool1(FALSE)
{
	QueStatement( NULL );
	bUseIPC = (nServiceId == SI_IPCCommandBlock);
	pRemoteControl = pRC;
	pCmdIn->Read( nArt );
	pCmdIn->Read( nParams );

	if( nParams & PARAM_USHORT_1 )  pCmdIn->Read( nSNr1 );
	if( nParams & PARAM_ULONG_1 )   pCmdIn->Read( nLNr1 );
	if( nParams & PARAM_STR_1 )     pCmdIn->Read( aString1 );
	if( nParams & PARAM_BOOL_1 )    pCmdIn->Read( bBool1 );	// sollte nie auftreten!!

#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Reading FlowControl: " );
	m_pDbgWin->AddText( String::CreateFromInt32( nArt ) );
	m_pDbgWin->AddText( " Params:" );
	if( nParams & PARAM_USHORT_1 )  {m_pDbgWin->AddText( " n1:" );m_pDbgWin->AddText( String::CreateFromInt32( nSNr1 ) );}
	if( nParams & PARAM_ULONG_1 )   {m_pDbgWin->AddText( " l1:" );m_pDbgWin->AddText( String::CreateFromInt64( nLNr1 ) );}
	if( nParams & PARAM_STR_1 )     {m_pDbgWin->AddText( " s1:" );m_pDbgWin->AddText( aString1 );}
	if( nParams & PARAM_BOOL_1 )    {m_pDbgWin->AddText( " b2:" );m_pDbgWin->AddText( bBool1 ? "TRUE" : "FALSE" );}
	m_pDbgWin->AddText( "\n" );
#endif
}

void StatementFlow::SendViaSocket()
{
	if ( bSending )
	{
#if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "SendViaSocket called recursively. Aborted!!!\n" );
#endif
		DBG_ERROR("SendViaSocket called recursively. Aborted!!!");
		return;
	}
	bSending = TRUE;
	if ( pCommLink )
	{
		if ( !pCommLink->TransferDataStream( pRet->GetStream() ) )	// tritt ein Fehler auf, so wird sofort gel�scht ...
			pCommLink = NULL;
	}
	else
	{
		// Macht nix. Wenn das Basic nicht mehr da ist, ist sowiso alles egal
		DBG_ERROR("Cannot send results to TestTool");
	}

	pRet->Reset();
	bSending = FALSE;
	IsError = FALSE;
}

BOOL StatementFlow::Execute()
{
#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Executing Flow: " );
	m_pDbgWin->AddText( String::CreateFromInt32( nArt ) );
	m_pDbgWin->AddText( "\n" );
#endif
	switch ( nArt )
	{
	case F_EndCommandBlock:
		{

			if ( !bUseIPC )
			{
				// bBool1 wurde im CTOR auf FALSE initialisiert
				if ( !bBool1 )	// also erster Durchlauf
				{
					pRemoteControl->pRetStream = pRet->GetStream();
					bBool1 = TRUE;	// wurde im CTOR auf FALSE initialisiert
					nRetryCount = nRetryCount * 4;
				}
				if ( pRemoteControl->pRetStream && (nRetryCount--) )	// also solange nicht abgeholt
				{
					return FALSE;	// Bitte einmal vom Callstack runter
				}
			}

		}
		break;
	}

	Advance();
	switch ( nArt )
	{
	case F_EndCommandBlock:
		if ( !bUseIPC )
		{	// wird oben abgehandelt
			pRet->Reset();
			IsError = FALSE;
		}
		else
			SendViaSocket();

		break;

	case F_Sequence:

		pRet->GenReturn(RET_Sequence,nLNr1);
        #if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "Sending Sequence Nr: " );
		m_pDbgWin->AddText( String::CreateFromInt64( nLNr1 ) );
		m_pDbgWin->AddText( "\n" );
		#endif

		break;
//	case RET_:
	default:
		DBG_ERROR( "Unknown Flowcontrol" );
		break;
	}

	delete this;
	return TRUE;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// neue Hilfsfunktion, die stetig erweitert werden muss
static short ImpGetRType( Window *pWin, SmartId aUId )
{
    ULONG nUId = aUId.GetNum(); // GetNum() is always zero if no num is defined
	// GGGg gggg::gggg gggg::ggLL LLLl::llll llll
	DBG_ASSERT( pWin, "missing Parameter" );
	short nRT;
	if( (nUId & 0xE0000000) == 0 ) // keine auto ID
		nRT = C_NoType;
	else {
//		if( (nUId & 0x1F) == 0 ) // globale Resource
		if( (nUId & 0x1FF) == 0 ) // globale Resource GH: soweit ich das verstanden habe 1FF
			nRT = (short)(C_TabPage - 1 + (nUId >> 29));
		else
			nRT = (short)((nUId & 0x00003E00) >> 9);
		if ( nRT == C_TabPage ) // wegen TabDialog.UId = TabPage.UId
			nRT = C_NoType;
		else
		{
#ifndef DBG_UTIL
			return nRT;
#endif
		}
	}

#ifdef DBG_UTIL
	short n = nRT;
#endif
	WindowType eRT = pWin->GetType();
	switch( eRT ) {
		case WINDOW_WINDOW:				nRT = C_Window		      ; break;

		case WINDOW_TABCONTROL:			nRT = C_TabControl        ; break;
		case WINDOW_RADIOBUTTON:		nRT = C_RadioButton       ; break;
		case WINDOW_CHECKBOX:			nRT = C_CheckBox          ; break;
		case WINDOW_TRISTATEBOX:		nRT = C_TriStateBox       ; break;
		case WINDOW_EDIT:				nRT = C_Edit              ; break;
		case WINDOW_MULTILINEEDIT:		nRT = C_MultiLineEdit     ; break;
		case WINDOW_MULTILISTBOX:		nRT = C_MultiListBox      ; break;
		case WINDOW_LISTBOX:			nRT = C_ListBox           ; break;
		case WINDOW_COMBOBOX:			nRT = C_ComboBox          ; break;
		case WINDOW_PUSHBUTTON:			nRT = C_PushButton        ; break;
		case WINDOW_SPINFIELD:			nRT = C_SpinField         ; break;
		case WINDOW_PATTERNFIELD:		nRT = C_PatternField      ; break;
		case WINDOW_NUMERICFIELD:		nRT = C_NumericField      ; break;
		case WINDOW_METRICFIELD:		nRT = C_MetricField       ; break;
		case WINDOW_CURRENCYFIELD:		nRT = C_CurrencyField     ; break;
		case WINDOW_DATEFIELD:			nRT = C_DateField         ; break;
		case WINDOW_TIMEFIELD:			nRT = C_TimeField         ; break;
		case WINDOW_IMAGERADIOBUTTON:	nRT = C_ImageRadioButton  ; break;
		case WINDOW_NUMERICBOX:			nRT = C_NumericBox        ; break;
		case WINDOW_METRICBOX:			nRT = C_MetricBox         ; break;
		case WINDOW_CURRENCYBOX:		nRT = C_CurrencyBox       ; break;
		case WINDOW_DATEBOX:			nRT = C_DateBox           ; break;
		case WINDOW_TIMEBOX:			nRT = C_TimeBox           ; break;
		case WINDOW_IMAGEBUTTON:		nRT = C_ImageButton       ; break;
		case WINDOW_MENUBUTTON:			nRT = C_MenuButton        ; break;
		case WINDOW_MOREBUTTON:			nRT = C_MoreButton        ; break;


		case WINDOW_TABPAGE:			nRT = C_TabPage;		break;
		case WINDOW_MODALDIALOG:		nRT = C_ModalDlg;		break;
		case WINDOW_FLOATINGWINDOW:		nRT = C_FloatWin;		break;
		case WINDOW_MODELESSDIALOG:		nRT = C_ModelessDlg;	break;
		case WINDOW_WORKWINDOW:			nRT = C_WorkWin;		break;
		case WINDOW_DOCKINGWINDOW:		nRT = C_DockingWin;		break;

		case WINDOW_MESSBOX:			nRT = C_MessBox;		break;
		case WINDOW_INFOBOX:			nRT = C_InfoBox;		break;
		case WINDOW_WARNINGBOX:			nRT = C_WarningBox;		break;
		case WINDOW_ERRORBOX:			nRT = C_ErrorBox;		break;
		case WINDOW_QUERYBOX:			nRT = C_QueryBox;		break;
#if 0 //ifndef VCL
		case WINDOW_FILEDIALOG:			nRT = C_FileDlg;		break;
		case WINDOW_PATHDIALOG:			nRT = C_PathDlg;		break;
		case WINDOW_PRINTDIALOG:		nRT = C_PrintDlg;		break;
		case WINDOW_PRINTERSETUPDIALOG:	nRT = C_PrinterSetupDlg;break;
		case WINDOW_COLORDIALOG:		nRT = C_ColorDlg;		break;
#endif
		case WINDOW_TABDIALOG:			nRT = C_TabDlg;			break;
//		case WINDOW_TABDIALOG:			nRT = C_SingleTabDlg;	break;

		case WINDOW_PATTERNBOX:			nRT = C_PatternBox;		break;
		case WINDOW_TOOLBOX:			nRT = C_ToolBox;		break;
// Gibts nicht       case WINDOW_VALUESET:			nRT = C_ValueSet;		break;
		case WINDOW_CONTROL:			nRT = C_Control;		break;
		case WINDOW_OKBUTTON:			nRT = C_OkButton;		break;
		case WINDOW_CANCELBUTTON:		nRT = C_CancelButton;	break;
		case WINDOW_BUTTONDIALOG:		nRT = C_ButtonDialog;	break;
		case WINDOW_TREELISTBOX:		nRT = C_TreeListBox;	break;

        case WINDOW_DIALOG:				nRT = C_Dialog;			break;
	}
#ifdef DBG_UTIL
	// Die Werte n sind bei den WindowTypen nicht mehr vergeben, werden aber in der AutoID noch verwendet
	// Sie werden jetzt durch den Typ nRT implementiert.
	DBG_ASSERT( n == C_NoType || n == nRT
		|| ( n == C_ImageButton && nRT == C_PushButton )
		|| ( n == C_TriStateBox && nRT == C_CheckBox )
		|| ( n == C_ImageRadioButton && nRT == C_RadioButton )
		, "error in algorithm" );
#endif
	return nRT;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

StatementSlot::StatementSlot( SCmdStream *pCmdIn )
: pItemArr(NULL)
{
	QueStatement( NULL );
	pCmdIn->Read( nFunctionId );
#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Reading Slot: " );
	m_pDbgWin->AddText( String::CreateFromInt32( nFunctionId ) );
	m_pDbgWin->AddText( "\n" );
#endif
	pCmdIn->Read( nAnzahl );
	if ( nAnzahl )
	{
        switch ( pCmdIn->GetNextType() )
        {
            case BinUSHORT: // use old calling method
                {
		            nAnzahl++;
		            pItemArr = new SfxPoolItem*[nAnzahl];
		            for (USHORT i = 0 ; i+1 < nAnzahl ; i++)
			            pCmdIn->Read( pItemArr[i] );
    		        pItemArr[nAnzahl-1] = NULL;
                }
                break;
            case BinString: // new Method
                {
                    aArgs.realloc(nAnzahl);
                    PropertyValue* pArg = aArgs.getArray();
		            for (USHORT i = 0 ; i < nAnzahl ; i++)
			            pCmdIn->Read( pArg[i] );
                }
                break;
        }
	}
}

// Constructor for UnoSlot
StatementSlot::StatementSlot()
: nAnzahl( 0 )
, pItemArr(NULL)
, nFunctionId( 0 )
, bMenuClosed(FALSE)
{}

StatementSlot::StatementSlot( ULONG nSlot, SfxPoolItem* pItem )
: pItemArr(NULL)
, bMenuClosed(FALSE)
{
	QueStatement( NULL );
	nFunctionId = USHORT(nSlot);
#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Reading Slot: " );
	m_pDbgWin->AddText( String::CreateFromInt32( nFunctionId ) );
	m_pDbgWin->AddText( "\n" );
#endif
	if ( pItem )
	{
		nAnzahl = 2;
		pItemArr = new SfxPoolItem*[2];
		pItemArr[0] = pItem;
		pItemArr[1] = NULL;
	}
	else
		nAnzahl = 0;
}

StatementSlot::~StatementSlot()
{
	if (nAnzahl)
	{
        if ( pItemArr )
        {
		    for (USHORT i = 0 ; i+1 < nAnzahl ; i++)
			    delete pItemArr[i];
		    delete[] pItemArr;
        }

        aArgs.realloc( 0 );
	}
}

void StatementSlot::AddReferer()
{
    HACK( "only to test!" );
// because slot 6102 /*SID_VERB_START*/ crashes when called with Property Referer
// We return to the previous behavior (which was a bug realy) of not adding this Property to calls which have no properties at all
// according to MBA most likely this Property can be removed at all and is maybe only needed for Slots with URLs
    if ( !nAnzahl )
        return;

    PropertyValue* pArg;

    nAnzahl++;
    aArgs.realloc(nAnzahl);
    pArg = aArgs.getArray();
    pArg[nAnzahl-1].Name = rtl::OUString::createFromAscii("Referer");
    pArg[nAnzahl-1].Value <<= ::rtl::OUString::createFromAscii("private:user");

    nAnzahl++;
    aArgs.realloc(nAnzahl);
    pArg = aArgs.getArray();
    pArg[nAnzahl-1].Name = rtl::OUString::createFromAscii("SynchronMode");
    pArg[nAnzahl-1].Value <<= sal_Bool( TRUE );
}

class SlotStatusListener : public cppu::WeakImplHelper1< XStatusListener >
{
public:
    SlotStatusListener();

    // XStatusListener
    virtual void SAL_CALL statusChanged( const ::com::sun::star::frame::FeatureStateEvent& Event ) throw (::com::sun::star::uno::RuntimeException);
    // XEventListener
    virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);

    // local Members
    BOOL bDisposed;
    BOOL bEnabled;
};
/*
struct FeatureStateEvent : public ::com::sun::star::lang::EventObject
    ::com::sun::star::util::URL FeatureURL;
    ::rtl::OUString FeatureDescriptor;
    sal_Bool IsEnabled;
    sal_Bool Requery;
    ::com::sun::star::uno::Any State;

    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > Source;
*/

SlotStatusListener::SlotStatusListener()
: bDisposed( FALSE )
, bEnabled( TRUE )
{}

// XStatusListener
void SAL_CALL SlotStatusListener::statusChanged( const ::com::sun::star::frame::FeatureStateEvent& Event ) throw (::com::sun::star::uno::RuntimeException)
{
//    DBG_ERROR1("FeatureURL: %s", ByteString( String( Event.FeatureURL.Complete ), RTL_TEXTENCODING_UTF8 ).GetBuffer() );
//    DBG_ERROR1("FeatureDescriptor: %s", ByteString( String( Event.FeatureDescriptor ), RTL_TEXTENCODING_UTF8 ).GetBuffer() );
    bEnabled = Event.IsEnabled;
//    DBG_ASSERT( Event.IsEnabled, "Not enabled" );
//    DBG_ASSERT( !Event.Requery, "Requery" );
}

// XEventListener
void SAL_CALL SlotStatusListener::disposing( const ::com::sun::star::lang::EventObject& ) throw (::com::sun::star::uno::RuntimeException)
{
    bDisposed = TRUE;
}

BOOL StatementSlot::Execute()
{
	if ( IsError )
	{
        #if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "Skipping Slot: " );
		m_pDbgWin->AddText( String::CreateFromInt32( nFunctionId ) );
		m_pDbgWin->AddText( "\n" );
		#endif

		Advance();
		delete this;
		return TRUE;
	}

	InitProfile();
#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Executing Slot: " );
	m_pDbgWin->AddText( String::CreateFromInt32( nFunctionId ) );
	m_pDbgWin->AddText( "\n" );
#endif

    PopupMenu *pPopup = NULL;
    MenuBar *pMenuBar = NULL;
	Menu *pMenu;

    GetCurrentMenues( pPopup, pMenuBar, pMenu );
    if ( pPopup )
    {
        if ( !bMenuClosed )
        {
            pPopup->EndExecute(0);
			aSubMenuId1 = SmartId();
			aSubMenuId2 = SmartId();
			aSubMenuId3 = SmartId();
			pMenuWindow = NULL;
            bMenuClosed = TRUE;
#if OSL_DEBUG_LEVEL > 1
	        m_pDbgWin->AddText( "Closed contextmenu\n" );
#endif
	        return FALSE;
        }
		else if ( nRetryCount-- )
		{
#if OSL_DEBUG_LEVEL > 1
			m_pDbgWin->AddText( "Waiting for contextmenu to close\n" );
#endif
			return FALSE;
		}
		else
			ReportError( GEN_RES_STR0( S_MENU_NOT_CLOSING ) );
    }

	Advance();

	if ( !IsError )
	{
        if ( ( nAnzahl == 0 && !getenv("OLDSLOTHANDLING") ) || aArgs.hasElements() )
        {   // trying to call slots via uno
            AddReferer();
            if ( !aUnoUrl.Len() )
                aUnoUrl = CUniString("slot:").Append( String::CreateFromInt32( nFunctionId ) );
            URL aTargetURL;
            aTargetURL.Complete = aUnoUrl;
            Reference < XFramesSupplier > xDesktop = Reference < XFramesSupplier >( ::comphelper::getProcessServiceFactory()->createInstance( CUniString("com.sun.star.frame.Desktop") ), UNO_QUERY );
            Reference < XFrame > xFrame;

            if ( xDesktop.is() )
            {
                xFrame = xDesktop->getActiveFrame();
                if ( !xFrame.is() )
                {
                    Reference < XFrames > xFrames;
                    xFrames = xDesktop->getFrames();
                    if ( xFrames.is() && xFrames->getCount() > 0 )
                    {
                        Any aFrame = xFrames->getByIndex( xFrames->getCount() -1 );
                        aFrame >>= xFrame;
                    }
                }
                if ( !xFrame.is() )
                {
                    if ( GetFirstDocFrame() )
                        GetFirstDocFrame()->ToTop();
                    xFrame = xDesktop->getActiveFrame();
                }
            }

            if ( xFrame.is() )
                xDesktop = Reference < XFramesSupplier >( xFrame, UNO_QUERY );
            else
                xDesktop.clear();

            while ( xDesktop.is() && xDesktop->getActiveFrame().is() )
            {
                xFrame = xDesktop->getActiveFrame();
#if OSL_DEBUG_LEVEL > 1
                ::rtl::OUString aName;
                if ( xFrame.is() )
                    aName = xFrame->getName();
#endif
                xDesktop = Reference < XFramesSupplier >( xFrame, UNO_QUERY );
            }

            if ( !xFrame.is() )
			    ReportError( GEN_RES_STR1( S_UNO_URL_EXECUTE_FAILED_NO_FRAME, aTargetURL.Complete ) );
            else
            {
                Reference < XURLTransformer > xTrans( ::comphelper::getProcessServiceFactory()->createInstance( CUniString("com.sun.star.util.URLTransformer" )), UNO_QUERY );
                xTrans->parseStrict( aTargetURL );

                Reference < XDispatchProvider > xProv( xFrame, UNO_QUERY );
                Reference < XDispatch > xDisp;
                while ( xProv.is() && !xDisp.is() )
                {
                    xDisp = xProv->queryDispatch( aTargetURL, ::rtl::OUString(), 0 );
                    if ( !xDisp.is() )
                    {
                        xFrame = Reference < XFrame > ( xFrame->getCreator(), UNO_QUERY );
                        xProv = Reference < XDispatchProvider > ( xFrame, UNO_QUERY );
                    }
                }

                if ( xDisp.is() )
                {
                    Reference < XStatusListener > xListener = ( XStatusListener* )new SlotStatusListener;
                    xDisp->addStatusListener( xListener, aTargetURL );
                    if ( static_cast< SlotStatusListener* >(xListener.get())->bEnabled )
                    {
                        if ( bIsSlotInExecute )
                            ReportError( GEN_RES_STR0( S_SLOT_IN_EXECUTE ) );
                        else
                        {
                            bIsSlotInExecute = TRUE;
                            xDisp->dispatch( aTargetURL, aArgs );
                            bIsSlotInExecute = FALSE;
                        }
                    }
                    else
    			        ReportError( GEN_RES_STR1( S_UNO_URL_EXECUTE_FAILED_DISABLED, aTargetURL.Complete ) );
                    xDisp->removeStatusListener( xListener, aTargetURL );
                }
                else
			        ReportError( GEN_RES_STR1( S_UNO_URL_EXECUTE_FAILED_NO_DISPATCHER, aTargetURL.Complete ) );
            }
        }
        else
        {
            DirectLog( S_QAError, GEN_RES_STR0( S_DEPRECATED ) );
            if ( !pTTProperties )
			    pTTProperties = new TTProperties();

		    switch ( pTTProperties->ExecuteFunction( nFunctionId, pItemArr, EXECUTEMODE_DIALOGASYNCHRON | nUseBindings ) )
		    {
		    case TT_PR_ERR_NODISPATCHER:
			    {
				    ReportError( SmartId(nFunctionId), GEN_RES_STR0( S_SID_EXECUTE_FAILED_NO_DISPATCHER ) );
			    }
			    break;
		    case TT_PR_ERR_NOEXECUTE:
			    {
				    ReportError(SmartId(nFunctionId), GEN_RES_STR0( S_SID_EXECUTE_FAILED ) );
			    }
			    break;
		    }
	    }
    }


/*	Neues Verfahren ab 334!
	Neue Methode zum einstellen, da� Modale Dialoge immer Asynchron aufgerufen werden
	und echter Returnwert, ob Slot geklappt hat
	und Testen ob Slot �berhaupt durch UI aufgerufen werden kann        */


	SendProfile( SlotString( nFunctionId ) );
	delete this;
	return TRUE;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

StatementUnoSlot::StatementUnoSlot(SCmdStream *pIn)
{
	QueStatement( NULL );

	pIn->Read( aUnoUrl );

#if OSL_DEBUG_LEVEL > 1
	StatementList::m_pDbgWin->AddText( "UnoUrl:" );
	StatementList::m_pDbgWin->AddText( aUnoUrl );
	StatementList::m_pDbgWin->AddText( "\n" );
#endif

}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

StatementCommand::StatementCommand( StatementList *pAfterThis, USHORT MethodId, USHORT Params, USHORT Nr1 )
: nMethodId( MethodId )
, aSmartMethodId()
, nParams(Params)
, nNr1(Nr1)
, nNr2(0)
, nNr3(0)
, nNr4(0)
, nLNr1(0)
, aString1()
, aString2()
, bBool1(FALSE)
, bBool2(FALSE)
{
	QueStatement( pAfterThis );
    aSmartMethodId = SmartId( nMethodId );

#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Directly adding Conmmand:" );
	m_pDbgWin->AddText( " Methode: " );
	m_pDbgWin->AddText( aSmartMethodId.GetText() );
	m_pDbgWin->AddText( " Params:" );
	if( nParams & PARAM_USHORT_1 )	{m_pDbgWin->AddText( " n1:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr1 ) );}
	if( nParams & PARAM_USHORT_2 )	{m_pDbgWin->AddText( " n2:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr2 ) );}
	if( nParams & PARAM_USHORT_3 )	{m_pDbgWin->AddText( " n3:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr3 ) );}
	if( nParams & PARAM_USHORT_4 )	{m_pDbgWin->AddText( " n4:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr4 ) );}
	if( nParams & PARAM_ULONG_1 )	{m_pDbgWin->AddText( " nl1:" );m_pDbgWin->AddText( String::CreateFromInt64( nLNr1 ) );}
	if( nParams & PARAM_STR_1 )		{m_pDbgWin->AddText( " s1:" );m_pDbgWin->AddText( aString1 );}
	if( nParams & PARAM_STR_2 )		{m_pDbgWin->AddText( " s2:" );m_pDbgWin->AddText( aString2 );}
	if( nParams & PARAM_BOOL_1 )    {m_pDbgWin->AddText( " b1:" );m_pDbgWin->AddText( bBool1 ? "TRUE" : "FALSE" );}
	if( nParams & PARAM_BOOL_2 )    {m_pDbgWin->AddText( " b2:" );m_pDbgWin->AddText( bBool2 ? "TRUE" : "FALSE" );}
	m_pDbgWin->AddText( "\n" );
#endif
}


StatementCommand::StatementCommand( SCmdStream *pCmdIn )
: nMethodId(0)
, aSmartMethodId()
, nParams(0)
, nNr1(0)
, nNr2(0)
, nNr3(0)
, nNr4(0)
, nLNr1(0)
, aString1()
, aString2()
, bBool1(FALSE)
, bBool2(FALSE)
{
	QueStatement( NULL );
	pCmdIn->Read( nMethodId );
    aSmartMethodId = SmartId( nMethodId );
	pCmdIn->Read( nParams );

	if( nParams & PARAM_USHORT_1 )	pCmdIn->Read( nNr1 );
	if( nParams & PARAM_USHORT_2 )	pCmdIn->Read( nNr2 );
	if( nParams & PARAM_USHORT_3 )	pCmdIn->Read( nNr3 );
	if( nParams & PARAM_USHORT_4 )	pCmdIn->Read( nNr4 );
	if( nParams & PARAM_ULONG_1 )	pCmdIn->Read( nLNr1 );
	if( nParams & PARAM_STR_1 )		pCmdIn->Read( aString1 );
	if( nParams & PARAM_STR_2 )		pCmdIn->Read( aString2 );
	if( nParams & PARAM_BOOL_1 )	pCmdIn->Read( bBool1 );
	if( nParams & PARAM_BOOL_2 )	pCmdIn->Read( bBool2 );

#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Reading Conmmand:" );
	m_pDbgWin->AddText( " Methode: " );
	m_pDbgWin->AddText( aSmartMethodId.GetText() );
	m_pDbgWin->AddText( " Params:" );
	if( nParams & PARAM_USHORT_1 )	{m_pDbgWin->AddText( " n1:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr1 ) );}
	if( nParams & PARAM_USHORT_2 )	{m_pDbgWin->AddText( " n2:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr2 ) );}
	if( nParams & PARAM_USHORT_3 )	{m_pDbgWin->AddText( " n3:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr3 ) );}
	if( nParams & PARAM_USHORT_4 )	{m_pDbgWin->AddText( " n4:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr4 ) );}
	if( nParams & PARAM_ULONG_1 )	{m_pDbgWin->AddText( " nl1:" );m_pDbgWin->AddText( String::CreateFromInt64( nLNr1 ) );}
	if( nParams & PARAM_STR_1 )		{m_pDbgWin->AddText( " s1:" );m_pDbgWin->AddText( aString1 );}
	if( nParams & PARAM_STR_2 )		{m_pDbgWin->AddText( " s2:" );m_pDbgWin->AddText( aString2 );}
	if( nParams & PARAM_BOOL_1 )    {m_pDbgWin->AddText( " b1:" );m_pDbgWin->AddText( bBool1 ? "TRUE" : "FALSE" );}
	if( nParams & PARAM_BOOL_2 )    {m_pDbgWin->AddText( " b2:" );m_pDbgWin->AddText( bBool2 ? "TRUE" : "FALSE" );}
	m_pDbgWin->AddText( "\n" );
#endif

	if ( nMethodId == RC_AppAbort )
	{
        #if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "*Deleting all Commands:\n" );
		#endif
		bReadingCommands = FALSE;
		while ( StatementList::pFirst != this ) // Alles L�schen au�er mich selbst
		{
			StatementList *pDeQue = StatementList::pFirst;
			pDeQue->Advance();
			delete pDeQue;
		}
		bReadingCommands = TRUE;
        #if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "*Done deleting all Commands:\n" );
		#endif
	}

}

void StatementCommand::WriteControlData( Window *pBase, ULONG nConf, BOOL bFirst )
{

	if ( IsDialog(pBase) && !bFirst )
		return;

	if ( bFirst )
		pRet->GenReturn ( RET_WinInfo, SmartId(), (comm_ULONG)nConf | DH_MODE_DATA_VALID, UniString(), TRUE );

	{	// Klammerung, so da� der String nicht w�hrend der Rekursion bestehen bleibt
		String aName;
		BOOL bSkip = FALSE;

		switch ( pBase->GetType() )
		{
			case WINDOW_RADIOBUTTON:
			case WINDOW_CHECKBOX:
			case WINDOW_TRISTATEBOX:
			case WINDOW_PUSHBUTTON:
			case WINDOW_OKBUTTON:
			case WINDOW_CANCELBUTTON:
			case WINDOW_IMAGERADIOBUTTON:
			case WINDOW_IMAGEBUTTON:
			case WINDOW_MENUBUTTON:
			case WINDOW_MOREBUTTON:
			case WINDOW_TABPAGE:
			case WINDOW_MODALDIALOG:
			case WINDOW_FLOATINGWINDOW:
			case WINDOW_MODELESSDIALOG:
			case WINDOW_WORKWINDOW:
			case WINDOW_DOCKINGWINDOW:
			case WINDOW_CONTROL:

			case WINDOW_FILEDIALOG:
			case WINDOW_PATHDIALOG:
			case WINDOW_PRINTDIALOG:
			case WINDOW_PRINTERSETUPDIALOG:
			case WINDOW_COLORDIALOG:
			case WINDOW_TABDIALOG:

			case WINDOW_BUTTONDIALOG:

			case WINDOW_MENUBARWINDOW:
				aName = pBase->GetText().EraseAllChars('~');
				break;

			case WINDOW_EDIT:
			case WINDOW_MULTILINEEDIT:
			case WINDOW_MULTILISTBOX:
			case WINDOW_LISTBOX:
			case WINDOW_COMBOBOX:
			case WINDOW_SPINFIELD:
			case WINDOW_PATTERNFIELD:
			case WINDOW_NUMERICFIELD:
			case WINDOW_METRICFIELD:
			case WINDOW_CURRENCYFIELD:
			case WINDOW_DATEFIELD:
			case WINDOW_TIMEFIELD:
			case WINDOW_NUMERICBOX:
			case WINDOW_METRICBOX:
			case WINDOW_CURRENCYBOX:
			case WINDOW_DATEBOX:
			case WINDOW_TIMEBOX:
			case WINDOW_PATTERNBOX:
			case WINDOW_TOOLBOX:
				aName = pBase->GetQuickHelpText();
				break;

			case WINDOW_MESSBOX:
			case WINDOW_INFOBOX:
			case WINDOW_WARNINGBOX:
			case WINDOW_ERRORBOX:
			case WINDOW_QUERYBOX:
				aName = ((MessBox*)pBase)->GetMessText();
				break;

			default:
				if ( pBase->GetSmartUniqueOrHelpId().Equals( SmartId() ) && !( nConf & DH_MODE_ALLWIN ) )
					bSkip = TRUE;
		}

		if ( !bSkip )
		{
			if ( aName.Len() == 0 )
				aName = pBase->GetQuickHelpText();
			if ( aName.Len() == 0 )
				aName = pBase->GetHelpText();
			if ( aName.Len() == 0 )
				aName = pBase->GetText();


    		String aTypeSuffix;
            if ( pBase->GetType() == WINDOW_CONTROL )
            {
                if ( dynamic_cast< EditBrowseBox* >(pBase) )
                    aTypeSuffix.AppendAscii( "/BrowseBox", 10 );
                else if ( dynamic_cast< ValueSet* >(pBase) )
                    aTypeSuffix.AppendAscii( "/ValueSet", 9 );
                else if ( dynamic_cast< ORoadmap* >(pBase) )
                    aTypeSuffix.AppendAscii( "/RoadMap", 8 );
                else if ( dynamic_cast< IExtensionListBox* >(pBase) )
                    aTypeSuffix.AppendAscii( "/ExtensionListBox" );
                else
                    aTypeSuffix.AppendAscii( "/Unknown", 8 );
            }

            SmartId aId = pBase->GetSmartUniqueOrHelpId();
			if ( aId.HasString() || ( nConf & DH_MODE_ALLWIN ) )
                pRet->GenReturn ( RET_WinInfo, SmartId( aId.GetStr() ), (comm_ULONG)pBase->GetType(),
				    TypeString(pBase->GetType()).Append(aTypeSuffix).AppendAscii(": ").Append(aName), FALSE );
			if ( !aId.HasString() || ( nConf & DH_MODE_ALLWIN ) )
			    pRet->GenReturn ( RET_WinInfo, SmartId( aId.GetNum() ), (comm_ULONG)pBase->GetType(),
				    TypeString(pBase->GetType()).Append(aTypeSuffix).AppendAscii(": ").Append(aName), FALSE );


			if ( pBase->GetType() == WINDOW_TOOLBOX )	// Buttons und Controls auf Toolboxen.
			{
				ToolBox *pTB = ((ToolBox*)pBase);
				USHORT i;
				for ( i = 0; i < pTB->GetItemCount() ; i++ )
				{
					aName = String();
//					if ( aName.Len() == 0 )
//						aName = pTB->GetQuickHelpText();
					if ( aName.Len() == 0 )
						aName = pTB->GetHelpText( pTB->GetItemId( i ) );
					if ( aName.Len() == 0 )
						aName = pTB->GetItemText( pTB->GetItemId( i ) );

					Window *pItemWin;
					pItemWin = pTB->GetItemWindow( pTB->GetItemId( i ) );
					if ( pTB->GetItemType( i ) == TOOLBOXITEM_BUTTON && ( !pItemWin || !pItemWin->IsVisible() ) )
					{
						if ( pTB->GetItemCommand(pTB->GetItemId( i )).Len() || ( nConf & DH_MODE_ALLWIN ) )
                            pRet->GenReturn ( RET_WinInfo, SmartId( pTB->GetItemCommand(pTB->GetItemId( i )) ), (comm_ULONG)WINDOW_BUTTON,
							    TypeString(WINDOW_BUTTON).AppendAscii(": ").Append(aName), FALSE );
						if ( !pTB->GetItemCommand(pTB->GetItemId( i )).Len() || ( nConf & DH_MODE_ALLWIN ) )
    						pRet->GenReturn ( RET_WinInfo, SmartId( pTB->GetHelpId(pTB->GetItemId( i )) ), (comm_ULONG)WINDOW_BUTTON,
	    						TypeString(WINDOW_BUTTON).AppendAscii(": ").Append(aName), FALSE );
					}
					else
					{
						if ( pItemWin )
						{
                            if ( pTB->GetItemCommand(pTB->GetItemId( i )).Len() || ( nConf & DH_MODE_ALLWIN ) )
							    pRet->GenReturn ( RET_WinInfo, SmartId( pTB->GetItemCommand(pTB->GetItemId( i )) ), (comm_ULONG)pItemWin->GetType(),
								    TypeString(pItemWin->GetType()).AppendAscii(": ").Append(aName), FALSE );
                            if ( !pTB->GetItemCommand(pTB->GetItemId( i )).Len() || ( nConf & DH_MODE_ALLWIN ) )
							    pRet->GenReturn ( RET_WinInfo, SmartId( pTB->GetHelpId(pTB->GetItemId( i )) ), (comm_ULONG)pItemWin->GetType(),
								    TypeString(pItemWin->GetType()).AppendAscii(": ").Append(aName), FALSE );
                            USHORT ii;
							for( ii = 0 ; ii < pItemWin->GetChildCount(); ii++ )
								WriteControlData( pItemWin->GetChild(ii), nConf, FALSE );
						}
						else
						{
							if ( nConf & DH_MODE_ALLWIN )
							{
								String aToolBoxItemType;
								switch ( pTB->GetItemType( i ) )
								{
								case TOOLBOXITEM_DONTKNOW:
									aToolBoxItemType.AssignAscii("TOOLBOXITEM_DONTKNOW");
									break;
								case TOOLBOXITEM_BUTTON:
									aToolBoxItemType.AssignAscii("TOOLBOXITEM_BUTTON");
									break;
								case TOOLBOXITEM_SPACE:
									aToolBoxItemType.AssignAscii("TOOLBOXITEM_SPACE");
									break;
								case TOOLBOXITEM_SEPARATOR:
									aToolBoxItemType.AssignAscii("TOOLBOXITEM_SEPARATOR");
									break;
								case TOOLBOXITEM_BREAK:
									aToolBoxItemType.AssignAscii("TOOLBOXITEM_BREAK");
									break;
								default:
									DBG_ERROR1( "Unknown TOOLBOXITEM %i", pTB->GetItemType( i ) );
								}
                                if ( pTB->GetItemCommand(pTB->GetItemId( i )).Len() || ( nConf & DH_MODE_ALLWIN ) )
								    pRet->GenReturn ( RET_WinInfo, SmartId( pTB->GetItemCommand(pTB->GetItemId( i )) ), (comm_ULONG)WINDOW_BASE,
									    aToolBoxItemType.AppendAscii(": ").Append(aName), FALSE );
                                if ( !pTB->GetItemCommand(pTB->GetItemId( i )).Len() || ( nConf & DH_MODE_ALLWIN ) )
								    pRet->GenReturn ( RET_WinInfo, SmartId( pTB->GetHelpId(pTB->GetItemId( i )) ), (comm_ULONG)WINDOW_BASE,
									    aToolBoxItemType.AppendAscii(": ").Append(aName), FALSE );
							}
						}
					}
				}

				return;	// ToolBox ist hier schon komplett abgehandelt.
			}


			if ( pBase->GetType() == WINDOW_BUTTONDIALOG	// Buttons auf Buttondialogen mit ID
				|| pBase->GetType() == WINDOW_MESSBOX
				|| pBase->GetType() == WINDOW_INFOBOX
				|| pBase->GetType() == WINDOW_WARNINGBOX
				|| pBase->GetType() == WINDOW_ERRORBOX
				|| pBase->GetType() == WINDOW_QUERYBOX )
			{
				ButtonDialog *pBD = ((ButtonDialog*)pBase);
				USHORT i;
				for ( i = 0; i < pBD->GetButtonCount() ; i++ )
				{
					aName = String();
					if ( aName.Len() == 0 )
						aName = pBD->GetPushButton( pBD->GetButtonId(i) )->GetText();
					String aID;
					switch ( pBD->GetButtonId(i) )
					{
						case BUTTONID_OK:
							aID.AssignAscii("Ok");
							break;
						case BUTTONID_CANCEL:
							aID.AssignAscii("Cancel");
							break;
						case BUTTONID_YES:
							aID.AssignAscii("Yes");
							break;
						case BUTTONID_NO:
							aID.AssignAscii("No");
							break;
						case BUTTONID_RETRY:
							aID.AssignAscii("Retry");
							break;
						case BUTTONID_HELP:
							aID.AssignAscii("Help");
							break;
						default:
							aID = UniString::CreateFromInt32( pBD->GetButtonId(i) );
							break;
					}

					pRet->GenReturn ( RET_WinInfo, SmartId( aID ), (comm_ULONG)pBD->GetPushButton( pBD->GetButtonId(i) )->GetType(),	// So da� der Text angezeigt wird!
						TypeString(pBD->GetPushButton( pBD->GetButtonId(i) )->GetType()).AppendAscii(": ").Append(aName)
						.AppendAscii(" ButtonId = ").Append(aID), FALSE );
				}

				return;	// ButtonDialog ist hier schon komplett abgehandelt.
			}


            Menu* pMenu = GetMatchingMenu( pBase );

			if ( pMenu )	// Menus
			{
				USHORT i;
				for ( i = 0; i < pMenu->GetItemCount() ; i++ )
				{
                    USHORT nID = pMenu->GetItemId( i );
                    
                    aName = String();
					if ( aName.Len() == 0 )
						aName = pMenu->GetHelpText( nID );
					if ( aName.Len() == 0 )
						aName = pMenu->GetItemText( nID );


					if ( pMenu->GetItemType( i ) == MENUITEM_STRING || pMenu->GetItemType( i ) ==  MENUITEM_IMAGE || pMenu->GetItemType( i ) == MENUITEM_STRINGIMAGE || (nConf & DH_MODE_ALLWIN) )
					{
						String aMenuItemType;
						switch ( pMenu->GetItemType( i ) )
						{
						case MENUITEM_STRING:
							aMenuItemType.AssignAscii("MENUITEM_STRING");
							break;
						case MENUITEM_STRINGIMAGE:
							aMenuItemType.AssignAscii("MENUITEM_STRINGIMAGE");
							break;
						case MENUITEM_IMAGE:
							aMenuItemType.AssignAscii("MENUITEM_IMAGE");
							break;
						case MENUITEM_SEPARATOR:
							aMenuItemType.AssignAscii("MENUITEM_SEPARATOR");
							break;
						case MENUITEM_DONTKNOW:
							aMenuItemType.AssignAscii("MENUITEM_DONTKNOW");
							break;
						default:
							DBG_ERROR1( "Unknown MENUITEM %i", pMenu->GetItemType( i ) );
						}
						if ( pMenu->GetItemCommand(nID).Len() || ( nConf & DH_MODE_ALLWIN ) )
                            pRet->GenReturn ( RET_WinInfo, SmartId( pMenu->GetItemCommand(nID) ), (comm_ULONG)0,
                                aMenuItemType.AppendAscii(": ").Append(aName), FALSE );
						if ( !pMenu->GetItemCommand(nID).Len() || ( nConf & DH_MODE_ALLWIN ) )
    						pRet->GenReturn ( RET_WinInfo, SmartId( nID ), (comm_ULONG)0,
	    						aMenuItemType.AppendAscii(": ").Append(aName), FALSE );
					}
				}

				return;	// Menu ist hier schon komplett abgehandelt.
			}
		}
	}

	for( USHORT i = 0 ; i < pBase->GetChildCount(); i++ )
		WriteControlData( pBase->GetChild(i), nConf, FALSE );
}

class SysWinContainer : public WorkWindow
{
private:
	ToolBox *pClientWin;
	DockingWindow *pDock;
public:
	SysWinContainer( ToolBox *pClient );
	~SysWinContainer();
	virtual void	Resize();
	virtual void	Resizing( Size& rSize );
};

SysWinContainer::SysWinContainer( ToolBox *pClient )
: WorkWindow( NULL, WB_BORDER | WB_SIZEMOVE | WB_CLOSEABLE )
, pClientWin( pClient )
{
	pDock = new DockingWindow( this );
	pClientWin->SetParent( pDock );
	pClientWin->SetFloatingMode( FALSE );
	SetText( pClient->GetText() );
	SetPosPixel( Point( 1,40 ) );
	Resize();
	pDock->Show();
	EnableAlwaysOnTop();
	Show();
}

SysWinContainer::~SysWinContainer()
{
	delete pDock;
}

void SysWinContainer::Resize()
{
	Size aSize( GetOutputSizePixel() );
	Resizing( aSize );
//	aSize = pClientWin->GetSizePixel();
//	aSize = pClientWin->CalcWindowSizePixel();
	if ( aSize != GetSizePixel() )
	{
		SetOutputSizePixel( aSize );
		pDock->SetSizePixel( aSize );
		pClientWin->SetSizePixel( aSize );
	}
}

void SysWinContainer::Resizing( Size& rSize )
{
	Size aSize;
	Size aBestSize;
	USHORT i;
	BOOL bHasValue = FALSE;
	ULONG nBestValue = 0;
	ULONG nThisValue;
	for ( i=1 ; i<=1 ; i++ )
	{
		aSize = pClientWin->CalcWindowSizePixel( i );
		nThisValue = Abs( aSize.Width() - rSize.Width() ) + Abs( aSize.Height() - rSize.Height() );
		if ( !bHasValue || ( nThisValue < nBestValue ) )
		{
			nBestValue = nThisValue;
			aBestSize = aSize;
			bHasValue = TRUE;
		}
	}
	rSize = aBestSize;
}


class DisplayHidWin : public ToolBox
{
	Edit	*pEdit;
	Size	aMinEditSize;
	USHORT	nLastItemID;
	BOOL bIsDraging;
	BOOL bIsPermanentDraging;
	void SetDraging( BOOL bNewDraging );
	Image *pShow, *pShow2;
	BOOL bConfigChanged;
	void EnableButtons( ULONG nConf );

	ULONG nEventHookID;
	static long stub_VCLEventHookProc( NotifyEvent& rEvt, void* pData )
	{
		return ((DisplayHidWin*)pData)->VCLEventHook( rEvt );
	}

	long VCLEventHook( NotifyEvent& rEvt );
	Window *pLastMouseMoveWin;

	SysWinContainer *pContainer;

    // aborting by pressing shist twice
    BOOL bOldShift;
    Time aLatest;
    USHORT nShiftCount;

public:
	DisplayHidWin();
	~DisplayHidWin();

	virtual void	Tracking( const TrackingEvent& rTEvt );
	virtual void	Click();
	virtual void	Select();
	virtual void	SetText( const XubString& rStr );

	void SetDisplayText( const String &aNewText ){ pEdit->SetText(aNewText); }
	String GetDisplayText() const { return pEdit->GetText(); }
	BOOL IsDisplayTextModified() const { return pEdit->IsModified(); }
	void ClearDisplayTextModified() const { pEdit->ClearModifyFlag(); }

	void SetConfig( ULONG nConf );
	ULONG GetConfig();

	BOOL IsConfigChanged() { return bConfigChanged; }
	void ConfigSent() { bConfigChanged = FALSE; }

	BOOL IsSendData() { return GetItemState( TT_SEND_DATA ) == STATE_CHECK; }
//	BOOL IsAllWin() { return GetItemState( TT_ALLWIN ) == STATE_CHECK; }

	BOOL IsDraging() { return bIsDraging; }

	Window* LastMouseMoveWin() { return pLastMouseMoveWin; }
};

DisplayHidWin::DisplayHidWin()
: ToolBox( StatementList::GetFirstDocFrame(), TTProperties::GetSvtResId(DisplayHidToolBox) )
, bIsDraging( FALSE )
, bIsPermanentDraging( FALSE )
, pShow( NULL )
, pShow2( NULL )
, pLastMouseMoveWin( NULL )
, bOldShift( 0 )
, nShiftCount( 0 )
{
	SetOutStyle( TOOLBOX_STYLE_HANDPOINTER | TOOLBOX_STYLE_FLAT );
	pEdit = new Edit( this, WB_CENTER | WB_BORDER );
	aMinEditSize = GetItemRect( TT_OUTPUT ).GetSize();
/**/	aMinEditSize=Size(20,20);
	aMinEditSize.Width() *= 12;
	pEdit->SetSizePixel( aMinEditSize );
	pEdit->Show();
	SetItemWindow( TT_OUTPUT, pEdit );
	Resize();
	pContainer = new SysWinContainer( this );
	nEventHookID = Application::AddEventHook( stub_VCLEventHookProc, this );
}

DisplayHidWin::~DisplayHidWin()
{
	Application::RemoveEventHook( nEventHookID );
    Hide(); // so GetFirstDocFrame won't return ourselves (needed for SOPlayer)
	SetParent( StatementList::GetFirstDocFrame() );
	delete pContainer;
	delete pEdit;
}

void DisplayHidWin::SetDraging( BOOL bNewDraging )
{
	if ( !pShow )
		pShow = new Image( GetItemImage( TT_SHOW ) );
	if ( !pShow2 )
		pShow2 = new Image( Bitmap( TTProperties::GetSvtResId( TT_SHOW2 ) ) );

	if ( bNewDraging )
		SetItemImage( TT_SHOW, *pShow2 );
	else
		SetItemImage( TT_SHOW, *pShow );

	bIsDraging = bNewDraging;
}

void DisplayHidWin::EnableButtons( ULONG nConf )
{
	BOOL bSend = BOOL(nConf & DH_MODE_SEND_DATA);
	EnableItem( TT_ALLWIN, bSend );
	EnableItem( TT_KURZNAME, bSend );
	EnableItem( TT_LANGNAME, bSend );
}

void DisplayHidWin::Tracking( const TrackingEvent& rTEvt )
{
//	if ( nLastItemID == TT_SHOW && GetItemState( TT_SHOW ) == STATE_NOCHECK && ( rTEvt.IsTrackingEnded() || rTEvt.IsTrackingCanceled() ) )
//		SetDraging( FALSE );
	if ( nLastItemID == TT_SHOW && GetItemState( TT_SHOW ) == STATE_NOCHECK )
		EndTracking( ENDTRACK_CANCEL );
	ToolBox::Tracking( rTEvt);
}

void DisplayHidWin::Click()
{
	nLastItemID = GetCurItemId();
	if ( nLastItemID == TT_SHOW )
	{
		SetDraging( TRUE );
	}
	ToolBox::Click();
}

void DisplayHidWin::Select()
{
	if ( GetItemState( GetCurItemId() ) == STATE_NOCHECK )
	{
		SetItemState( GetCurItemId(), STATE_CHECK );
		if ( GetCurItemId() == TT_SHOW )
		{
			bIsPermanentDraging = TRUE;
			SetDraging( TRUE );
		}
	}
	else
	{
		SetItemState( GetCurItemId(), STATE_NOCHECK );
		if ( GetCurItemId() == TT_SHOW )
		{
			bIsPermanentDraging = FALSE;
			SetDraging( FALSE );
		}
	}
	if ( GetCurItemId() == TT_SEND_DATA )
	{
		EnableButtons( GetConfig() );
	}
}

void DisplayHidWin::SetConfig( ULONG nConf )
{
	SetItemState( TT_KURZNAME,  ( nConf & DH_MODE_KURZNAME )  ? STATE_CHECK : STATE_NOCHECK );
	SetItemState( TT_LANGNAME,  ( nConf & DH_MODE_LANGNAME )  ? STATE_CHECK : STATE_NOCHECK );
	SetItemState( TT_ALLWIN,    ( nConf & DH_MODE_ALLWIN )    ? STATE_CHECK : STATE_NOCHECK );
	SetItemState( TT_SEND_DATA, ( nConf & DH_MODE_SEND_DATA ) ? STATE_CHECK : STATE_NOCHECK );
	EnableButtons( nConf );
}

ULONG DisplayHidWin::GetConfig()
{
	ULONG nConf = 0;
	if ( GetItemState( TT_KURZNAME ) == STATE_CHECK )
		nConf |= DH_MODE_KURZNAME;
	if ( GetItemState( TT_LANGNAME ) == STATE_CHECK )
		nConf |= DH_MODE_LANGNAME;
	if ( GetItemState( TT_ALLWIN ) == STATE_CHECK )
		nConf |= DH_MODE_ALLWIN;
	if ( IsSendData() )
		nConf |= DH_MODE_SEND_DATA;

	return nConf;
}

void DisplayHidWin::SetText( const XubString& rStr )
{
	pContainer->SetText( rStr );
}

long DisplayHidWin::VCLEventHook( NotifyEvent& rEvt )
{
	if ( EVENT_MOUSEMOVE == rEvt.GetType() )
    {
		pLastMouseMoveWin = rEvt.GetWindow();

        // check if abort with pressing shift twice
        MouseEvent* pMEvt = (MouseEvent*)rEvt.GetMouseEvent();

        if ( (  pMEvt->IsShift() && !bOldShift ) )    // Shift pressed
        {
            if ( aLatest < Time() )
            {
                nShiftCount = 0;
                aLatest = Time()+Time( 0, 0, 0, 50 );
            }
            nShiftCount++;
        }
        if ( ( !pMEvt->IsShift() &&  bOldShift ) )    // Shift released
        {
            nShiftCount++;
            if ( nShiftCount == 4 && aLatest > Time() )
            {
			    bIsPermanentDraging = FALSE;
			    SetDraging( FALSE );
		        SetItemState( TT_SHOW, STATE_NOCHECK );
            }
        }
        bOldShift = pMEvt->IsShift();
    }
	if ( ( ( EVENT_MOUSEBUTTONUP == rEvt.GetType() && rEvt.GetMouseEvent()->GetButtons() == MOUSE_LEFT) || ( EVENT_MOUSEMOVE == rEvt.GetType() && !rEvt.GetMouseEvent()->GetButtons() ) )
			&& IsDraging() && !bIsPermanentDraging )
		SetDraging( FALSE );
	return 0;
}


BOOL StatementCommand::DisplayHID()
{
	// Return TRUE -> reexecute command

	if ( !bBool2 )	// Wird auf FALSE initialisiert
	{
		bBool2 = TRUE;				// Wir sind initialisiert.
		GetTTSettings()->pDisplayInstance = this;	// Und haben die Macht (Alle anderen beenden sich)

		if ( !(nParams & PARAM_ULONG_1) )
		{
			if( GetTTSettings()->pDisplayHidWin )	// Nichts ver�ndern
				nLNr1 = GetTTSettings()->pDisplayHidWin->GetConfig();
			else	// Beim ersten Aufruf wollen wir alles richtig einstellen
				nLNr1 = DH_MODE_KURZNAME | DH_MODE_LANGNAME;

			if( ((nParams & PARAM_BOOL_1) && bBool1) )
				nLNr1 |= DH_MODE_SEND_DATA;
			else
				nLNr1 &= ( ~DH_MODE_SEND_DATA );
		}

		if ( GetTTSettings()->pDisplayHidWin )
			GetTTSettings()->pDisplayHidWin->SetConfig( nLNr1 );
	}

	if ( GetTTSettings()->pDisplayInstance && GetTTSettings()->pDisplayInstance != this )
	{
		DBG_WARNING("Mehrere DisplayHID am laufen");
		return FALSE;		// Noch eine andere Instanz macht das gleiche!
	}

	if ( !GetTTSettings()->pDisplayHidWin )
	{
		GetTTSettings()->pDisplayHidWin = new DisplayHidWin();
		GetTTSettings()->aOriginalCaption = GetTTSettings()->pDisplayHidWin->GetText();
		GetTTSettings()->pDisplayHidWin->Show();
		if ( bBool1 )
			nLNr1 |= DH_MODE_SEND_DATA;
		GetTTSettings()->pDisplayHidWin->SetConfig( nLNr1 );

		GetTTSettings()->Old = NULL;
		GetTTSettings()->Act = NULL;
		GetTTSettings()->pDisplayInstance = this;
	}
	else
	{
		GetTTSettings()->pDisplayHidWin->GetWindow( WINDOW_OVERLAP )->Enable( TRUE, TRUE );
		GetTTSettings()->pDisplayHidWin->GetWindow( WINDOW_OVERLAP )->EnableInput( TRUE, TRUE );
	}


	if ( GetTTSettings()->pDisplayHidWin->IsVisible() && !bDying )
	{

		if ( GetTTSettings()->pDisplayHidWin->IsDraging() )
		{


#define HIGHLIGHT_WIN( WinPtr )										\
		{															\
			Color aLineColMem = WinPtr->GetLineColor();				\
			WinPtr->SetLineColor( Color( COL_WHITE ) );				\
			Color aFillColMem = WinPtr->GetFillColor();				\
			WinPtr->SetFillColor( Color( COL_LIGHTRED ) );			\
			RasterOp aROp = WinPtr->GetRasterOp();					\
			WinPtr->SetRasterOp( ROP_XOR );							\
			Size aSz = WinPtr->PixelToLogic( WinPtr->GetSizePixel() );\
			ULONG nMaxCornerRadius = WinPtr->PixelToLogic( Point( 80, 0 ) ).X();\
			ULONG iCorner = std::max ((ULONG) 8, (ULONG) std::min( nMaxCornerRadius, (ULONG) std::min((ULONG) (aSz.Width() / 6), (ULONG)(aSz.Height() / 6))));\
			WinPtr->DrawRect(Rectangle(Point(),aSz), iCorner, iCorner);\
			WinPtr->SetLineColor( aLineColMem );					\
			WinPtr->SetFillColor( aFillColMem );					\
			WinPtr->SetRasterOp( aROp );							\
		}


#define SET_WIN( WinPtr )											\
			if ( StatementList::WinPtrValid(WinPtr) )	\
			{														\
				HIGHLIGHT_WIN ( WinPtr );							\
			}

#define RESET_WIN( WinPtr )											\
			if ( StatementList::WinPtrValid(WinPtr) )	\
			{														\
				WinPtr->Invalidate( INVALIDATE_NOTRANSPARENT );		\
				WinPtr->Update();		\
			}


			GetTTSettings()->Old = GetTTSettings()->Act;
//			GetTTSettings()->Act = GetMouseWin();
			GetTTSettings()->Act = GetTTSettings()->pDisplayHidWin->LastMouseMoveWin();

            if ( !StatementList::WinPtrValid ( GetTTSettings()->Old ) )
                GetTTSettings()->Old = NULL;
            if ( !StatementList::WinPtrValid ( GetTTSettings()->Act ) )
                GetTTSettings()->Act = NULL;

			if ( GetTTSettings()->Act && GetTTSettings()->Act->GetType() == WINDOW_BORDERWINDOW )
				GetTTSettings()->Act = GetTTSettings()->Act->GetWindow( WINDOW_CLIENT );

			if ( GetTTSettings()->Act != GetTTSettings()->Old )
			{
				if ( GetTTSettings()->Old )
				{
					RESET_WIN(GetTTSettings()->Old);
				}
				if ( GetTTSettings()->Act )
				{
					SET_WIN(GetTTSettings()->Act);
					GetTTSettings()->pDisplayHidWin->SetDisplayText(GetTTSettings()->Act->GetSmartUniqueOrHelpId().GetText().AppendAscii(" WinType: ")
						.Append(UniString::CreateFromInt64(GetTTSettings()->Act->GetType())).AppendAscii("  ").Append(GetTTSettings()->Act->GetText()));
					if ( GetTTSettings()->Act && !GetTTSettings()->Act->GetSmartUniqueId().Equals( GetTTSettings()->Act->GetSmartHelpId() ) )
						GetTTSettings()->pDisplayHidWin->SetText(UniString( TTProperties::GetSvtResId( TT_ALTERNATE_CAPTION ) ).Append(GetTTSettings()->Act->GetSmartHelpId().GetText()));
					else
						GetTTSettings()->pDisplayHidWin->SetText( GetTTSettings()->aOriginalCaption );
				}
				else
					GetTTSettings()->pDisplayHidWin->SetDisplayText(CUniString("Kein Window/Control gefunden"));
			}
			else if ( GetTTSettings()->Act )
			{
//				SET_WIN(GetTTSettings()->Act);
				if ( GetTTSettings()->pDisplayHidWin->IsDisplayTextModified() && GetTTSettings()->pDisplayHidWin->GetDisplayText().Len() > 0 )
				{
					GetTTSettings()->Act->SetSmartUniqueId( SmartId( GetTTSettings()->pDisplayHidWin->GetDisplayText().ToInt32() ) );
					GetTTSettings()->pDisplayHidWin->ClearDisplayTextModified();
				}
			}
/*			if ( Application::GetLastInputInterval() > 5000 )	// 5 Sekunden lang nix geschehen
			{
				GetTTSettings()->pDisplayHidWin->ToTop( TOTOP_NOGRABFOCUS );
			}
*/
			if ( GetTTSettings()->pDisplayHidWin->IsSendData() /*&& bBool2*/ && GetTTSettings()->Act )
			{
				if ( !StatementFlow::bSending )
				{	// Normalerweise syncronisierung �ber Protokoll. Hier ist das aber asyncron!!!
					WriteControlData( GetTTSettings()->Act, GetTTSettings()->pDisplayHidWin->GetConfig() );
					new StatementFlow( this, F_EndCommandBlock );	// Kommando zum Senden erzeugen und in que eintragen
				}
			}
		}	//if ( GetTTSettings()->pDisplayHidWin->IsDraging() )
		else
		{
			if ( GetTTSettings()->Act )
			{
				RESET_WIN(GetTTSettings()->Act);
				GetTTSettings()->Act = NULL;
			}
		}

		if ( pFirst == this )	// Sollte immer so sein, aber besser isses
			if ( pNext )		// Befehle warten auf Ausf�hrung
			{					// An Ende neu einsortieren
				Advance();
				QueStatement( NULL );
			}
//			{					// Ersten und 2. austauschen.
//				pFirst = pNext;
//				pNext = pNext->pNext;
//				pFirst->pNext = this;
//			}

	}
	else
	{
		delete GetTTSettings()->pDisplayHidWin;
		GetTTSettings()->pDisplayHidWin = NULL;
		GetTTSettings()->pDisplayInstance = NULL;
	}

	return GetTTSettings()->pDisplayHidWin != NULL;
}

class TranslateWin : public WorkWindow
{
private:
	DECL_LINK( DoAccept, PushButton* );
	DECL_LINK( DoNext, PushButton* );
	DECL_LINK( DoSelect, PushButton* );
	DECL_LINK( DoRestore, PushButton* );
	DECL_LINK( TranslationChanged, Edit* );
	DECL_LINK( ShowInplace, Timer* );

	Timer InplaceTimer;

//	virtual void MouseButtonUp( const MouseEvent& rMEvt );
//	virtual void MouseMove( const MouseEvent& rMEvt );

	PushButton PushButtonTT_PB_NEXT;
	GroupBox GroupBoxTT_GB_TRANSLATION;
	Edit EditTT_E_NEW;
	GroupBox GroupBoxTT_GB_COMMENT;
	Edit EditTT_E_COMMENT;
	PushButton PushButtonTT_PB_ACCEPT;
	FixedText FixedTextTT_FT_OLD;
	PushButton PushButtonTT_PB_SELECT;
	PushButton PushButtonTT_PB_RESTORE;

	Window *Old;
	Window *Act;
	Window *pTranslateWin;
	BOOL bSelecting;

	BOOL bAvailable;
	BOOL bNext;

	BOOL TestChangedDataSaved();


	ULONG nEventHookID;
	static long stub_VCLEventHookProc( NotifyEvent& rEvt, void* pData )
	{
		return ((TranslateWin*)pData)->VCLEventHook( rEvt );
	}

	long VCLEventHook( NotifyEvent& rEvt );

public:
	TranslateWin();
	~TranslateWin();

	static String MarkShortcutErrors( Window* pBase, BOOL bMark );

	BOOL IsTranslationAvailable(){ return bAvailable; }
	BOOL IsNextDialog(){ return bNext; }
	void ResetNextDialog(){ bNext = FALSE; }

	Window* GetTranslationWindow(){ return pTranslateWin; }
	String GetOriginalText(){ return FixedTextTT_FT_OLD.GetText(); }
	String GetTranslationText(){ return EditTT_E_NEW.GetText(); }
	String GetComment(){ return EditTT_E_COMMENT.GetText(); }

	void EnableTranslation();
};

TranslateWin::TranslateWin()
: WorkWindow( NULL, TTProperties::GetSvtResId( TT_INLINE_TRANSLATION ) )
, PushButtonTT_PB_NEXT( this, TTProperties::GetSvtResId( TT_PB_NEXT ) )
, GroupBoxTT_GB_TRANSLATION( this, TTProperties::GetSvtResId( TT_GB_TRANSLATION ) )
, EditTT_E_NEW( this, TTProperties::GetSvtResId( TT_E_NEW ) )
, GroupBoxTT_GB_COMMENT( this, TTProperties::GetSvtResId( TT_GB_COMMENT ) )
, EditTT_E_COMMENT( this, TTProperties::GetSvtResId( TT_E_COMMENT ) )
, PushButtonTT_PB_ACCEPT( this, TTProperties::GetSvtResId( TT_PB_ACCEPT ) )
, FixedTextTT_FT_OLD( this, TTProperties::GetSvtResId( TT_FT_OLD ) )
, PushButtonTT_PB_SELECT( this, TTProperties::GetSvtResId( TT_PB_SELECT ) )
, PushButtonTT_PB_RESTORE( this, TTProperties::GetSvtResId( TT_PB_RESTORE ) )
, Old( NULL )
, Act( NULL )
, pTranslateWin( NULL )
, bSelecting( FALSE )
, bAvailable( FALSE )
, bNext( FALSE )
{
    FreeResource();
	PushButtonTT_PB_NEXT.SetClickHdl( LINK( this, TranslateWin, DoNext ) );
	PushButtonTT_PB_ACCEPT.SetClickHdl( LINK( this, TranslateWin, DoAccept ) );
	PushButtonTT_PB_SELECT.SetClickHdl( LINK( this, TranslateWin, DoSelect ) );
	PushButtonTT_PB_RESTORE.SetClickHdl( LINK( this, TranslateWin, DoRestore ) );
	EditTT_E_NEW.SetModifyHdl( LINK( this, TranslateWin, TranslationChanged ) );
	InplaceTimer.SetTimeout( 250 );
	InplaceTimer.SetTimeoutHdl( LINK( this, TranslateWin, ShowInplace ) );
	EnableAlwaysOnTop();
	nEventHookID = Application::AddEventHook( stub_VCLEventHookProc, this );
}

TranslateWin::~TranslateWin()
{
	Application::RemoveEventHook( nEventHookID );
}

BOOL TranslateWin::TestChangedDataSaved()
{
	if ( ( EditTT_E_NEW.GetText().CompareTo( FixedTextTT_FT_OLD.GetText() ) != COMPARE_EQUAL
			|| EditTT_E_COMMENT.GetText().Len() )
		&& PushButtonTT_PB_ACCEPT.IsEnabled() )
	{
		return MessBox( this, TTProperties::GetSvtResId( TT_DISCARD_CHANGED_DATA ) ).Execute() == RET_YES;
	}
	else
		return TRUE;
}

IMPL_LINK( TranslateWin, DoAccept, PushButton*, EMPTYARG )
{
	PushButtonTT_PB_SELECT.Disable();
	PushButtonTT_PB_NEXT.Disable();
		PushButtonTT_PB_RESTORE.Disable();
	EditTT_E_NEW.Disable();
	EditTT_E_COMMENT.Disable();
	PushButtonTT_PB_ACCEPT.Disable();
	bAvailable = TRUE;
	return 0;
}

IMPL_LINK( TranslateWin, DoNext, PushButton*, EMPTYARG )
{
	if ( TestChangedDataSaved() )
	{
		PushButtonTT_PB_SELECT.Disable();
		PushButtonTT_PB_NEXT.Disable();
		PushButtonTT_PB_RESTORE.Disable();
		EditTT_E_NEW.Disable();
		EditTT_E_COMMENT.Disable();
		PushButtonTT_PB_ACCEPT.Disable();
		bNext = TRUE;
	}
	return 0;
}

IMPL_LINK( TranslateWin, DoSelect, PushButton*, EMPTYARG )
{
	if ( bSelecting )
	{
//		ReleaseMouse();
		bSelecting = FALSE;
	}
	else
	{
		if ( TestChangedDataSaved() )
		{
			PushButtonTT_PB_RESTORE.Disable();
//			CaptureMouse();
			bSelecting = TRUE;
		}
	}
	return 0;
}

IMPL_LINK( TranslateWin, DoRestore, PushButton*, EMPTYARG )
{
	String sTT_E_OLD( FixedTextTT_FT_OLD.GetText());
	sTT_E_OLD.SearchAndReplaceAll( CUniString("\\n"), CUniString("\n") );
	sTT_E_OLD.SearchAndReplaceAll( CUniString("\\t"), CUniString("\t") );

	String sTT_E_NEW( EditTT_E_NEW.GetText());
	sTT_E_NEW.SearchAndReplaceAll( CUniString("\\n"), CUniString("\n") );
	sTT_E_NEW.SearchAndReplaceAll( CUniString("\\t"), CUniString("\t") );

	if ( StatementList::WinPtrValid( pTranslateWin ) && pTranslateWin->GetText().CompareTo( sTT_E_NEW ) == COMPARE_EQUAL )
	{	// Im ersten schritt nur in der UI zur�ck
		pTranslateWin->SetText( sTT_E_OLD );
	}
	else
	{	// Im zweite Schritt auch den eingegebenen Text
		EditTT_E_NEW.SetText( FixedTextTT_FT_OLD.GetText() );
		PushButtonTT_PB_RESTORE.Disable();
	}
	if ( StatementList::WinPtrValid( pTranslateWin ) )
		MarkShortcutErrors( pTranslateWin->GetWindow( WINDOW_OVERLAP ), TRUE );
	return 0;
}

IMPL_LINK( TranslateWin, TranslationChanged, Edit*, pEdit )
{
    (void) pEdit; /* avoid warning about unused parameter */
	PushButtonTT_PB_RESTORE.Enable();
	InplaceTimer.Start();
	return 0;
}

IMPL_LINK( TranslateWin, ShowInplace, Timer*, EMPTYARG )
{
	PushButtonTT_PB_RESTORE.Enable();
	if ( StatementList::WinPtrValid( pTranslateWin ) )
	{
		String sTT_E_NEW( EditTT_E_NEW.GetText());
		// alle CRs UnQuoten
		sTT_E_NEW.SearchAndReplaceAll( CUniString("\\n"), CUniString("\n") );
		// alle TABSs UnQuoten
		sTT_E_NEW.SearchAndReplaceAll( CUniString("\\t"), CUniString("\t") );
		pTranslateWin->SetText( sTT_E_NEW );

		MarkShortcutErrors( pTranslateWin->GetWindow( WINDOW_OVERLAP ), TRUE );
	}
	return 0;
}

long TranslateWin::VCLEventHook( NotifyEvent& rEvt )
{
	if ( EVENT_MOUSEMOVE == rEvt.GetType() )
	{
		if ( bSelecting )
		{
			const MouseEvent *pMEvt = rEvt.GetMouseEvent();
			Old = Act;
			Act = rEvt.GetWindow();

			if ( Act )
			{
				Window *pWin = Act;
				USHORT i;
				for ( i = 0 ; i < Act->GetChildCount() ; i++ )
				{
					pWin = Act->GetChild(i);
					Rectangle aWinPosSize( pWin->GetPosPixel(), pWin->GetSizePixel() );

					if ( ( pWin->IsMouseTransparent() || !pWin->IsEnabled() ) && aWinPosSize.IsInside( pMEvt->GetPosPixel() ) )
					{
						Act = pWin;
						break;
					}
				}
			}

            if ( !StatementList::WinPtrValid ( Old ) )
                Old = NULL;

			if ( Act != Old )
			{
				if ( Old )
				{
					Window *pWin;
					if ( Old->IsMouseTransparent() && Old->GET_REAL_PARENT() )
						pWin = Old->GET_REAL_PARENT();
					else
						pWin = Old;
					RESET_WIN(pWin);
				}
				if ( Act )
				{
					SET_WIN(Act);
					FixedTextTT_FT_OLD.SetText( Act->GetText() );
				}
				else
					FixedTextTT_FT_OLD.SetText( String() );
			}
			else if ( Act )
			{
	//			SET_WIN(Act);
			}
	/*		if ( Application::GetLastInputInterval() > 5000 )	// 5 Sekunden lang nix geschehen
			{
				ToTop();
			}
	*/
		}	//if ( bSelecting )
		else
		{
			if ( Act )
			{
				if ( Act->IsMouseTransparent() && Act->GET_REAL_PARENT() )
					Act = Act->GET_REAL_PARENT();
				RESET_WIN(Act);
				Act = NULL;
			}
		}
	}
	else if ( EVENT_MOUSEBUTTONUP == rEvt.GetType() )
	{
		if ( bSelecting )
		{
			pTranslateWin = Act;
			if ( pTranslateWin )
			{
				MarkShortcutErrors( pTranslateWin->GetWindow( WINDOW_OVERLAP ), TRUE );
				// alle CRs quoten (NF)
				String sTT_E_NEW( pTranslateWin->GetText());
				sTT_E_NEW.SearchAndReplaceAll( CUniString("\n"), CUniString("\\n") );
				// alle TABSs quoten ()
				sTT_E_NEW.SearchAndReplaceAll( CUniString("\t"), CUniString("\\t") );

				FixedTextTT_FT_OLD.SetText( sTT_E_NEW );
				EditTT_E_NEW.SetText( sTT_E_NEW );
				EditTT_E_NEW.Enable();
				EditTT_E_NEW.GrabFocus();
				EditTT_E_COMMENT.SetText( String() );
				EditTT_E_COMMENT.Enable();
				PushButtonTT_PB_ACCEPT.Enable();
			}
	//		ReleaseMouse();
			bSelecting = FALSE;
		}
	}

	return 0;
}

#define FDS_ACTION_COLLECT	1
#define FDS_ACTION_MARK		2
#define FDS_ACTION_UNMARK	3

class FindShortcutErrors: public Search
{
	String aShortcuts,aDoubleShortcuts;
	USHORT nAction;
public:
	FindShortcutErrors();
	virtual BOOL IsWinOK( Window *pWin );
	void SetAction( USHORT nA );
	String GetDoubleShortcuts() { return aDoubleShortcuts; }
};

FindShortcutErrors::FindShortcutErrors()
: Search( SEARCH_NOOVERLAP | SEARCH_NO_TOPLEVEL_WIN )
{
	SetAction( FDS_ACTION_COLLECT );	// Wir fange immer mit sammeln an, ODER??
}

void FindShortcutErrors::SetAction( USHORT nA )
{
	nAction = nA;
	if ( FDS_ACTION_COLLECT == nAction )
	{
		aShortcuts = UniString();
		aDoubleShortcuts = UniString();
	}
}

BOOL FindShortcutErrors::IsWinOK( Window *pWin )
{
	if ( pWin->IsReallyVisible() )
	{
		String aText = pWin->GetText();
		xub_StrLen nPos = aText.Search('~');
		String aShortcut;
		BOOL bHasAccel = FALSE;
		if ( nPos != STRING_NOTFOUND )
		{
			aShortcut = aText.Copy( nPos+1, 1 );
			aShortcut.ToLowerAscii();
			bHasAccel = aShortcut.Len() == 1;
		}

		switch ( nAction )
		{
			case FDS_ACTION_COLLECT:
				{
					if ( aShortcuts.Search( aShortcut ) != STRING_NOTFOUND )
						aDoubleShortcuts += aShortcut;
					else
						aShortcuts += aShortcut;
				}
				break;
			case FDS_ACTION_MARK:
				{
					BOOL bMissing = FALSE;
					if ( !bHasAccel && aText.Len() )	// should there be an accelarator defined
					{

						Window* 	pChild;
						pChild = pWin->GetWindow( WINDOW_CLIENT );

						if ( (pChild->GetType() == WINDOW_RADIOBUTTON) ||
							 (pChild->GetType() == WINDOW_IMAGERADIOBUTTON) ||
							 (pChild->GetType() == WINDOW_CHECKBOX) ||
							 (pChild->GetType() == WINDOW_TRISTATEBOX) ||
							 (pChild->GetType() == WINDOW_PUSHBUTTON) )
						{
							if ( !pChild->GetText().EqualsAscii( "..." ) )
								bMissing = TRUE;
						}

						if ( pChild->GetType() == WINDOW_FIXEDTEXT )
						{
							Window* pTempChild = pWin->GetWindow( WINDOW_NEXT );
							if ( pTempChild )
								pTempChild = pTempChild->GetWindow( WINDOW_CLIENT );

							if ( pTempChild && pChild->GetText().Len() )
							{
								if ( (pTempChild->GetType() == WINDOW_EDIT) ||
									 (pTempChild->GetType() == WINDOW_MULTILINEEDIT) ||
									 (pTempChild->GetType() == WINDOW_SPINFIELD) ||
									 (pTempChild->GetType() == WINDOW_PATTERNFIELD) ||
									 (pTempChild->GetType() == WINDOW_NUMERICFIELD) ||
									 (pTempChild->GetType() == WINDOW_METRICFIELD) ||
									 (pTempChild->GetType() == WINDOW_CURRENCYFIELD) ||
									 (pTempChild->GetType() == WINDOW_DATEFIELD) ||
									 (pTempChild->GetType() == WINDOW_TIMEFIELD) ||
									 (pTempChild->GetType() == WINDOW_LISTBOX) ||
									 (pTempChild->GetType() == WINDOW_MULTILISTBOX) ||
									 (pTempChild->GetType() == WINDOW_COMBOBOX) ||
									 (pTempChild->GetType() == WINDOW_PATTERNBOX) ||
									 (pTempChild->GetType() == WINDOW_NUMERICBOX) ||
									 (pTempChild->GetType() == WINDOW_METRICBOX) ||
									 (pTempChild->GetType() == WINDOW_CURRENCYBOX) ||
									 (pTempChild->GetType() == WINDOW_DATEBOX) ||
									 (pTempChild->GetType() == WINDOW_TIMEBOX) )
								{
									bMissing = TRUE;
								}
							}
						}
					}

					if ( bHasAccel && aDoubleShortcuts.Search( aShortcut ) != STRING_NOTFOUND )
					{
						if ( pWin->GetType() == WINDOW_GROUPBOX )
							pWin->SetControlForeground( Color( COL_LIGHTRED ) );
						else
						{
							pWin->SetControlBackground();
							Color aCol(COL_GRAY);
							aCol.SetRed( 0xff );
							pWin->SetControlBackground( aCol );
						}
					}
					else if ( bMissing )
					{
						pWin->SetControlBackground();
						Color aCol(COL_GRAY);
						aCol.SetRed( 0xff );
						aCol.SetGreen( 0xff );
						pWin->SetControlBackground( aCol );
					}
					else
					{
						pWin->SetControlForeground();
						pWin->SetControlBackground();
					}
				}
				break;
			case FDS_ACTION_UNMARK:
				{
					pWin->SetControlForeground();
					pWin->SetControlBackground();
				}
				break;
		}
	}
	else
		if ( FDS_ACTION_MARK == nAction || FDS_ACTION_UNMARK == nAction )
		{
			pWin->SetControlForeground();
			pWin->SetControlBackground();
		}

	return FALSE;
}

String TranslateWin::MarkShortcutErrors( Window* pBase, BOOL bMark )
{
	if ( pBase )
	{
		FindShortcutErrors aFinder;
		if ( bMark )
		{
			StatementList::SearchAllWin( pBase, aFinder, TRUE );	// collect Shortcuts first
			aFinder.SetAction( FDS_ACTION_MARK );
		}
		else
			aFinder.SetAction( FDS_ACTION_UNMARK );
		StatementList::SearchAllWin( pBase, aFinder, TRUE );
		return aFinder.GetDoubleShortcuts();
	}
	return UniString();
}

void TranslateWin::EnableTranslation()
{
	PushButtonTT_PB_SELECT.Enable();
	PushButtonTT_PB_NEXT.Enable();
	bAvailable = FALSE;
	bNext = FALSE;
}

void StatementCommand::Translate()
{
	// Es wurde eine initale UniqueId mitgegeben. Dann nur die dopelten Shortcuts liefern
	if( (nParams & PARAM_ULONG_1) && nLNr1 )
	{
		String aDouble;
		Window *pWin = SearchTree( SmartId( nLNr1 ) ,FALSE );
		if ( pWin )
		{
			pWin = pWin->GetWindow( WINDOW_OVERLAP );
			aDouble = TranslateWin::MarkShortcutErrors( pWin, TRUE );
		}
		pRet->GenReturn ( RET_Value, aSmartMethodId, aDouble );
		return;
	}

	if ( !GetTTSettings()->pTranslateWin )
	{
		GetTTSettings()->pTranslateWin = new TranslateWin;
		GetTTSettings()->bToTop = TRUE;
	}

	GetTTSettings()->pTranslateWin->Show();
	if ( GetTTSettings()->bToTop )
	{
		GetTTSettings()->pTranslateWin->ToTop();
		GetTTSettings()->bToTop = FALSE;
	}

//	GetTTSettings()->pTranslateWin->GetWindow( WINDOW_OVERLAP )->Enable( TRUE, TRUE );
	GetTTSettings()->pTranslateWin->GetWindow( WINDOW_OVERLAP )->EnableInput( TRUE, TRUE );

	if ( GetTTSettings()->pTranslateWin->IsTranslationAvailable() )
	{
		String aTranslation;
		Window* pTranslationWindow = GetTTSettings()->pTranslateWin->GetTranslationWindow();

		DBG_ASSERT( pTranslationWindow, "Kein Translation Window" );

		if ( WinPtrValid( pTranslationWindow ) )
		{
			if ( pTranslationWindow->GetType() == WINDOW_BORDERWINDOW && pTranslationWindow->GetWindow( WINDOW_CLIENT ) )
			{
				Window* pNew = pTranslationWindow->GetWindow( WINDOW_CLIENT );
				// Bei Dockingwindoes das kanze Geraffel von Docking Floating �berspringen
				while ( IsDialog( pNew ) && !pNew->GetSmartUniqueOrHelpId().HasAny() && pNew->GetChildCount() == 1 )
					pNew = pNew->GetChild( 0 );
				pTranslationWindow = pNew;
			}

			aTranslation = CUniString("0;");

			aTranslation += pTranslationWindow->GetSmartUniqueOrHelpId().GetText();
			aTranslation += ';';

			aTranslation += TypeString( pTranslationWindow->GetType() );
			aTranslation += ';';

			Window* pParentDialog = pTranslationWindow;
			while ( pParentDialog && !IsDialog( pParentDialog ) )
			{
				pParentDialog = pParentDialog->GET_REAL_PARENT();
			}

			if ( pParentDialog )
			{
				aTranslation += pParentDialog->GetSmartUniqueOrHelpId().GetText();
				aTranslation += ';';
				aTranslation += TypeString( pParentDialog->GetType() );
			}
			else
				aTranslation.AppendAscii( "0;" );		// Zahl + leerer String
			aTranslation += ';';

			aTranslation += '\"';
			aTranslation += GetTTSettings()->pTranslateWin->GetOriginalText();
			aTranslation += '\"';

			aTranslation += ';';

			aTranslation += '\"';
			aTranslation += GetTTSettings()->pTranslateWin->GetTranslationText();
			aTranslation += '\"';

			aTranslation += ';';

			aTranslation += '\"';
			aTranslation += GetTTSettings()->pTranslateWin->GetComment();
			aTranslation += '\"';

			// alle CRs quoten (NF)
			aTranslation.SearchAndReplaceAll( CUniString("\n"), CUniString("\\n") );
			// alle TABSs quoten ()
			aTranslation.SearchAndReplaceAll( CUniString("\t"), CUniString("\\t") );

			pRet->GenReturn ( RET_Value, aSmartMethodId, aTranslation );
			GetTTSettings()->pTranslateWin->EnableTranslation();
			GetTTSettings()->bToTop = TRUE;
		}
		else
		{
			pRet->GenReturn ( RET_Value, aSmartMethodId, String() );
			GetTTSettings()->pTranslateWin->EnableTranslation();
			ErrorBox err( GetTTSettings()->pTranslateWin, TTProperties::GetSvtResId( TT_NO_CONTROL ));
			err.Execute();
			GetTTSettings()->bToTop = TRUE;
		}

	}
	else if ( GetTTSettings()->pTranslateWin->IsNextDialog() )
	{
		pRet->GenReturn ( RET_Value, aSmartMethodId, CUniString("1") );
		GetTTSettings()->pTranslateWin->ResetNextDialog();
		GetTTSettings()->pTranslateWin->LoseFocus();
		GetTTSettings()->bToTop = TRUE;
	}
	else
	{
		GetTTSettings()->pTranslateWin->EnableTranslation();
		pRet->GenReturn ( RET_Value, aSmartMethodId, String() );
	}
}

Window* StatementCommand::GetNextOverlap( Window* pBase )
{	// Findet irgendwelche Overlap-Fenster, die schlie�bar aussehen
	// Eventuell mu� noch die Auswahl verfeinert werden.

	if ( pBase->GetType() != WINDOW_BORDERWINDOW )
		pBase = pBase->GetWindow( WINDOW_OVERLAP );

	Window *pControl = NULL;
	if ( pBase->GetWindow( WINDOW_FIRSTOVERLAP ) )
	{
		pControl = GetNextOverlap( pBase->GetWindow( WINDOW_FIRSTOVERLAP ) );
	}

	if ( !pControl && pBase->GetWindow( WINDOW_NEXT ) )
	{
		pControl = GetNextOverlap( pBase->GetWindow( WINDOW_NEXT ) );
	}

	if ( !pControl )
	{
		Window *pTest = pBase->GetWindow( WINDOW_CLIENT );
		if ( IsAccessable (pTest)
			&& pTest->IsEnabled()
			&& pTest->IsVisible()
			&& ((pTest->GetStyle() & WB_CLOSEABLE ) || (pBase->GetStyle() & WB_CLOSEABLE )) )
			return pTest;
		else
			return NULL;
	}
	else
		return pControl;
}

Window* StatementCommand::GetNextRecoverWin()
{
	// �ber die TopLevelWindows der App iterieren
	Window* pBase = Application::GetFirstTopLevelWindow();
	Window *pControl = NULL;
    Window* pMyFirstDocFrame = NULL;
	while ( pBase )
	{
		// zuerst weitere Fenster auf dem Fenster suchen und schliessen
		pControl = GetNextOverlap( pBase );
        if ( pControl && pControl->GetType() == WINDOW_HELPTEXTWINDOW )
        {}  // skip it
        else
        {
		    if ( pControl && pControl->IsVisible() && !IsFirstDocFrame( pControl ) && !IsIMEWin( pControl ) )
		    {
                Window* pTB = pControl->GetChild( 0 );
                if ( pControl->GetChildCount() == 1 && pTB->GetType() == WINDOW_TOOLBOX )
//				    return pTB;
                    ;   // do not act on floating toolboxes #i38796
			    else
				    return pControl;
		    }

		    // dann das Fenster selbst Schliessen
       	    // erstes DocWin �berspringen
            // Assumption that Doc Windows are Borderwindows and ButtonDialog and such are not
		    if ( pBase->IsVisible() && !IsFirstDocFrame( pBase ) && pBase->GetType() != WINDOW_BORDERWINDOW && !IsIMEWin( pBase ) )
			    return pBase;

            if ( !pMyFirstDocFrame && IsFirstDocFrame( pBase ) )
                pMyFirstDocFrame = pBase;
        }

		pBase = Application::GetNextTopLevelWindow( pBase );
	}
#ifdef RESET_APPLICATION_TO_BACKING_WINDOW
    // close the FirstDocFrame last, It will not be closed, but the Document inside will be closed.
    if ( IsDocWin( pMyFirstDocFrame ) )
        return pMyFirstDocFrame;
#endif // def RESET_APPLICATION_TO_BACKING_WINDOW

	return NULL;
}

BOOL StatementCommand::Execute()
{
	if ( IsError )
	{
#if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "Skipping Command: " );
		m_pDbgWin->AddText( aSmartMethodId.GetText() );
		m_pDbgWin->AddText( "\n" );
		#endif

		Advance();
		delete this;
		return TRUE;
	}

	InitProfile();
#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Executing Command: " );
	m_pDbgWin->AddText( aSmartMethodId.GetText() );
	m_pDbgWin->AddText( "\n" );
#endif





#if OSL_DEBUG_LEVEL > 1
#define	REPORT_WIN_CLOSED(pControl, aInfo)			\
	_REPORT_WIN_CLOSED(pControl, aInfo)				\
	m_pDbgWin->AddText( aInfo.AppendAscii(" \"").Append( pControl->GetText() ).AppendAscii("\" geschlossen, RType = ").Append( TypeString(pControl->GetType()) ).AppendAscii(", UId = ").Append( UIdString(pControl->GetSmartUniqueOrHelpId()) ) );
#else
#define	REPORT_WIN_CLOSED(pControl, aInfo) _REPORT_WIN_CLOSED(pControl, aInfo)
#endif

#define	REPORT_WIN_CLOSEDc(pControl, aInfo )		\
	REPORT_WIN_CLOSED(pControl, CUniString(aInfo) )

#define	_REPORT_WIN_CLOSED(pControl, aInfo)			\
	if ( aString1.Len() )							\
		aString1 += '\n';							\
	aString1 += aInfo;								\
	aString1.AppendAscii(" \"");					\
	aString1 += pControl->GetText();				\
	aString1.AppendAscii("\" geschlossen, RType = ");\
	aString1 += TypeString(pControl->GetType());	\
	aString1.AppendAscii(", UId = ");				\
	aString1 += UIdString(pControl->GetSmartUniqueOrHelpId());


	switch ( nMethodId )
	{
		case RC_AppDelay:
			if ( !bBool1 )
			{
				nLNr1 = Time().GetTime() + nNr1/10;
				bBool1 = TRUE;
			}
			if ( Time().GetTime() < long(nLNr1) )	// Aktuelle Zeit kleiner Endzeit
				return FALSE;
			break;
		case RC_DisplayHid:
			if ( DisplayHID() )
				return FALSE;
			break;
		case RC_ResetApplication:
			{
				if ( !bBool1 )
				{
					nRetryCount = 150;		// das sollte reichen.
					bBool1 = TRUE;			// Nur beim ersten mal!
					nNr1 = 1;				// Welcher Button ist dran?
					nLNr1 = 0;				// Speichern des AppWin
					aString1 = UniString();	// Liste der geschlossenen Fenster

					// So da� nacher auch wieder alles auf Default steht
					nUseBindings = 0;
                    bCatchGPF = TRUE;
                    bUsePostEvents = TRUE;

				    aSubMenuId1 = SmartId();
				    aSubMenuId2 = SmartId();
				    aSubMenuId3 = SmartId();
                    pMenuWindow = NULL;
				}
				if ( !nRetryCount )
					ReportError( GEN_RES_STR0( S_RESETAPPLICATION_FAILED_COMPLEX ) );

				Window *pControl = GetNextRecoverWin();

				if ( pControl )
				{
                    bBool2 = FALSE; // flag for wait when all windows are closed
					pControl->GrabFocus();

					if (	pControl->GetType() != WINDOW_DOCKINGWINDOW
						 && pControl->GetType() != WINDOW_FLOATINGWINDOW
						 && pControl->GetType() != WINDOW_MODELESSDIALOG
						 && pControl->GetType() != WINDOW_WORKWINDOW
						 && pControl->GetType() != WINDOW_TOOLBOX
						 && pControl->GetType() != WINDOW_BORDERWINDOW
						 && nRetryCount-- )
					{
						short nRT = ImpGetRType( pControl, pControl->GetSmartUniqueOrHelpId() );

						if ( nRT == C_TabControl && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_TABDIALOG )
						{	// Bei Tabcontrol den zugeh�rigen Tabdialog nehmen
							pControl = pControl->GET_REAL_PARENT();
							nRT = ImpGetRType( pControl, pControl->GetSmartUniqueOrHelpId() );
						}

						switch( nRT )
						{
							case C_ModalDlg:
							case C_Dialog:
							case C_TabDlg:
								REPORT_WIN_CLOSEDc(pControl, "Dialog");
								SET_WINP_CLOSING(pControl);
								((SystemWindow*)pControl)->Close();
								break;
							case C_WorkWin:
								break;
							case C_MessBox:
							case C_InfoBox:
							case C_WarningBox:
							case C_ErrorBox:
							case C_QueryBox:
							case C_ButtonDialog:
								{
									ButtonDialog* pBD = (ButtonDialog*)pControl;
									// nNr1 >= 10 bedeutet (Custom)-Buttons durchgehen
									if ( nNr1 >= 10+pBD->GetButtonCount() ) nNr1 = 1;
									switch( nNr1 )
									{
										case 5:
											if ( pBD->GetPushButton( BUTTONID_OK ) )
											{
												REPORT_WIN_CLOSEDc(pControl, "Message Box (OK)");
												SET_WINP_CLOSING(pControl);
												pBD->EndDialog(RET_OK);
											}
											nNr1 = 10;	// Nochmal alle Buttons der Reihe nach
											break;
										case 4:
											if ( pBD->GetPushButton( BUTTONID_CANCEL ) )
											{
												REPORT_WIN_CLOSEDc(pControl, "Message Box (Cancel)");
												SET_WINP_CLOSING(pControl);
												pBD->EndDialog(RET_CANCEL);
											}
											nNr1++;
											break;
										case 3:
											if ( pBD->GetPushButton( BUTTONID_YES ) )
											{
												REPORT_WIN_CLOSEDc(pControl, "Message Box (Yes)");
												SET_WINP_CLOSING(pControl);
												pBD->EndDialog(RET_YES);
											}
											nNr1++;
											break;
										case 2:		// BUG 48239
										case 1:
											if ( pBD->GetPushButton( BUTTONID_NO ) )
											{
												REPORT_WIN_CLOSEDc(pControl, "Message Box (No)");
												SET_WINP_CLOSING(pControl);
												pBD->EndDialog(RET_NO);
											}
											nNr1++;
											break;
										default:
											{
												USHORT nID = pBD->GetButtonId( nNr1-10 );
												if ( nID != BUTTONID_HELP )
												{
													REPORT_WIN_CLOSED(pControl, CUniString("Message Box (").Append( UniString::CreateFromInt32(nID) ).AppendAscii(")"));
													SET_WINP_CLOSING(pControl);
													pBD->EndDialog(nID);
												}
												nNr1++;
											}
									}
									break;
								}
							default:
								DBG_ERROR( "Unknown Windowtype" );
								REPORT_WIN_CLOSEDc(pControl, "Unknown Windowtype");
								ReportError( GEN_RES_STR0( S_RESETAPPLICATION_FAILED_UNKNOWN ), pControl->GetType() );
                                #if OSL_DEBUG_LEVEL > 1
								m_pDbgWin->AddText( " Unbekannter Objekttyp aus UId" );
								#endif
								break;
						}
						return FALSE;
					}
					else
					{
						if ( (pControl->GetType() == WINDOW_DOCKINGWINDOW || pControl->GetType() == WINDOW_TOOLBOX) && nRetryCount-- )
						{
							if ( (((DockingWindow*)pControl)->GetStyle() | ((DockingWindow*)pControl)->GetFloatStyle()) & WB_CLOSEABLE )
							{
								REPORT_WIN_CLOSED(pControl, TypeString(pControl->GetType()));
								SET_WINP_CLOSING(pControl);
								((DockingWindow*)pControl)->Close();

								// Eigentlich nur bei TaskWindows! Hoffen wir mal, da� keine anderen DockingWindows dazwischen hauen.
								if ( (Window*)nLNr1 != pControl )
									nNr1 = 1;		// Zum durchprobieren der Buttons beim Schlie�en
								nLNr1 = (ULONG)pControl;

								return FALSE;
							}
						}
						if ( nRetryCount--
								&& (	(pControl->GetType() == WINDOW_FLOATINGWINDOW)
									||	(pControl->GetType() == WINDOW_MODELESSDIALOG)
									||	(pControl->GetType() == WINDOW_WORKWINDOW)
									||	(pControl->GetType() == WINDOW_BORDERWINDOW) ) )
						{
	//						if ( pControl->GetStyle() & WB_CLOSEABLE )
							{
#ifdef RESET_APPLICATION_TO_BACKING_WINDOW
                                // Special handling for last Document; do not close the Frame, only the Document
                                if ( GetDocWinCount() == 1 && IsDocFrame( pControl ) )
                                {
                                    if ( IsDocWin( pControl ) )
                                    {
                                        if ( GetDocFrameMenuBar( pControl ) )
                                        {
                                            MenuBar* pMenu = GetDocFrameMenuBar( pControl );
                                            if ( pMenu->HasCloser() )
                                            {
								                REPORT_WIN_CLOSED( pControl, TypeString(pControl->GetType()));
								                SET_WINP_CLOSING(pControl);

                                                pMenu->GetCloserHdl().Call( pMenu );

                                                // nur bei TaskWindows!
                                                if ( (Window*)nLNr1 != pControl )
									                nNr1 = 1;		// Zum durchprobieren der Buttons beim Schlie�en
								                nLNr1 = (ULONG)pControl;

                                                return FALSE;
                                            }
                                        }
                                    }
                                }
                                else
#endif // def RESET_APPLICATION_TO_BACKING_WINDOW
                                {
								    REPORT_WIN_CLOSED( pControl, TypeString(pControl->GetType()));
								    SET_WINP_CLOSING(pControl);
								    ((SystemWindow*)pControl)->Close();

                                    // Eigentlich nur bei TaskWindows!
                                    if ( (Window*)nLNr1 != pControl )
									    nNr1 = 1;		// Zum durchprobieren der Buttons beim Schlie�en
								    nLNr1 = (ULONG)pControl;

                                    return FALSE;
                                }
							}
						}
					}
				}
                // wait for some time if more windows show up
                // E.g.: Floating toolbars on a Task which was hidden by another Task before
			    if ( !bBool2 )
			    {
				    nLNr1 = Time().GetTime() + 100; // 100 = 1 Second
				    bBool2 = TRUE;
			    }
			    if ( Time().GetTime() < long(nLNr1) )	// Aktuelle Zeit kleiner Endzeit
				    return FALSE;
                else
				    pRet->GenReturn ( RET_Value, aSmartMethodId, aString1);
			}
		    break;
		case RC_WaitSlot:
            {
			    if ( ! (nParams & PARAM_USHORT_1) )
				    nNr1 = 1000;    // defaults to 1000 = 1 Sec.
			    if ( !bBool1 )
			    {
				    nLNr1 = Time().GetTime() + nNr1/10;
				    bBool1 = TRUE;
			    }

                if ( !bIsSlotInExecute )
    			    pRet->GenReturn ( RET_Value, aSmartMethodId, comm_USHORT(CONST_WSFinished) );
                else
                {
			        if ( Time().GetTime() < long(nLNr1) )	// Aktuelle Zeit kleiner Endzeit
				        return FALSE;
    			    pRet->GenReturn ( RET_Value, aSmartMethodId, comm_USHORT(CONST_WSTimeout) );
                }
            }
		    break;
	}


	Advance();


	switch ( nMethodId )
	{
		case RC_AppDelay:		// Diese Befehle werden anderswo behandelt
		case RC_DisplayHid:
		case RC_ResetApplication:
		case RC_WaitSlot:

		case RC_AppAbort:		// Sofortiges L�schen aller Befehle
			break;
		case RC_Assert:
            {
                ByteString aAssertion( "Diese Assertion wurde vom Testtool per Befehl ausgel�st" );
                aAssertion = ByteString( String( aAssertion, RTL_TEXTENCODING_MS_1252 ), RTL_TEXTENCODING_UTF8 );
			    DBG_ASSERT( !aString1.Len(), ByteString( aString1, RTL_TEXTENCODING_UTF8 ).GetBuffer() );
			    DBG_ASSERT(  aString1.Len(), aAssertion.GetBuffer() );
			    OSL_ENSURE( !aString1.Len(), ByteString( aString1, RTL_TEXTENCODING_UTF8 ).GetBuffer() );
			    OSL_ENSURE(  aString1.Len(), aAssertion.GetBuffer() );
            }
			break;
		case RC_CaptureAssertions:
#ifdef DBG_UTIL
			if( !(nParams & PARAM_BOOL_1) || bBool1 )
			{
				DBG_INSTOUTERROR( DBG_OUT_TESTTOOL );
				osl_setDebugMessageFunc( osl_TestToolDebugPrint );
			}
			else
			{
				DBG_INSTOUTERROR( DBG_OUT_MSGBOX );
				osl_setDebugMessageFunc( pOriginal_osl_DebugMessageFunc );
			}
#endif
			break;
		case RC_Translate:
			Translate();
			break;
		case RC_ApplicationBusy:
		{
			BOOL bWait = FALSE;
			ReportError( GEN_RES_STR0( S_NO_ACTIVE_WINDOW ) );
//			if ( Application::GetAppWindow() )
//				bWait = Application::GetAppWindow()->IsWait();
			pRet->GenReturn ( RET_Value, aSmartMethodId, bWait );
			break;
		}
		case RC_GetClipboard:
			{
				::rtl::OUString aTemp;
				::svt::OStringTransfer::PasteString( aTemp, GetFirstDocFrame() );
				pRet->GenReturn ( RET_Value, aSmartMethodId, String( aTemp ) );
			}
			break;
		case RC_SetClipboard:
			::svt::OStringTransfer::CopyString(aString1,GetFirstDocFrame());
			break;
		case RC_WinTree:
			pRet->GenReturn ( RET_Value, aSmartMethodId, Tree( NULL, 0));
			break;
    #if OSL_DEBUG_LEVEL > 1
		case RC_NoDebug:
			m_pDbgWin->bQuiet = TRUE;
			m_pDbgWin->Hide();
			m_pDbgWin->Clear();
			break;
		case RC_Debug:
			m_pDbgWin->bQuiet = FALSE;
			m_pDbgWin->Show();
			break;
	#endif
		case RC_GPF:
			((TabControl*)NULL)->SetCurPageId( 12345 );
			break;
		case RC_GetNextCloseWindow:
			{
				Window *pWin = GetActive( WINDOW_BASE );      // WINDOW_ANYTYPE
				if ( !pWin )
					ReportError( GEN_RES_STR0( S_NO_ACTIVE_WINDOW ) );
				else if ( !IsDialog(pWin) )
					ReportError( GEN_RES_STR0( S_NO_DIALOG_IN_GETACTIVE ) );
				else
					pRet->GenReturn( RET_Value, aSmartMethodId, static_cast<comm_ULONG>(pWin->GetSmartUniqueOrHelpId().GetNum())); //GetNum() ULONG != comm_ULONG on 64bit
			}
			break;
		case RC_UseBindings:
			if( !(nParams & PARAM_BOOL_1) || bBool1 )
				nUseBindings = SFX_USE_BINDINGS;
			else
				nUseBindings = 0;
			break;
		case RC_Profile:
			//	Bei folgenden Parametern passiert folgendes:
			//	ein boolean=false					Alles Profiling stoppen (Ergebnisse liefern)
			//	ein boolean=true, 1-4 ints			Einteilung der Zeiten in K�stchen
			//	kein! boolean keine ints			loggen jeden Befehls
			//	kein! boolean 1 int					loggen alle int Millisekunden
			//  ein String							wird in das Logfile �bernommen(sonst passiert nichts)
			if( !(nParams & PARAM_BOOL_1) || bBool1 )
			{
				if ( !pProfiler )
				{
					pProfiler = new TTProfiler;
					InitProfile();
				}

				if( !(nParams & PARAM_BOOL_1) && (nParams & PARAM_USHORT_1) )
				{	// Autoprofiling: Profile nNr
					if ( pProfiler->IsProfilingPerCommand() )
					{
						pProfiler->StopProfilingPerCommand();
					}
					pProfiler->StartAutoProfiling( nNr1 );

					// Der Header ist abh�ngig vom Modus
					pRet->GenReturn( RET_ProfileInfo, SmartId(), pProfiler->GetProfileHeader() );
				}
				else if ( nParams & PARAM_USHORT_1 )
				{	// Partitioning initialisieren: Profile true [,nNr][,nNr][,nNr][,nNr]
					comm_ULONG nAnzahl=0;
					if ( nParams & PARAM_USHORT_1 ) { nAnzahl++; };
					if ( nParams & PARAM_USHORT_2 ) { nAnzahl++; };
					if ( nParams & PARAM_USHORT_3 ) { nAnzahl++; };
					if ( nParams & PARAM_USHORT_4 ) { nAnzahl++; };

					// Hier werden die Parameter ans Testtool zur�ck �bertragen.
					// Das ist zwar etwas eigenartig, aber ansonsten m�sste man im Testtool
					// die einfache Struktur der Remotebefehle aufbrechen.
					pRet->GenReturn( RET_ProfileInfo, SmartId( S_ProfileReset ), nAnzahl );

					// Und die einzelnen Grenzen
					if ( nParams & PARAM_USHORT_1 ) { pRet->GenReturn( RET_ProfileInfo, SmartId( S_ProfileBorder1 ), (comm_ULONG)nNr1 ); };
					if ( nParams & PARAM_USHORT_2 ) { pRet->GenReturn( RET_ProfileInfo, SmartId( S_ProfileBorder2 ), (comm_ULONG)nNr2 ); };
					if ( nParams & PARAM_USHORT_3 ) { pRet->GenReturn( RET_ProfileInfo, SmartId( S_ProfileBorder3 ), (comm_ULONG)nNr3 ); };
					if ( nParams & PARAM_USHORT_4 ) { pRet->GenReturn( RET_ProfileInfo, SmartId( S_ProfileBorder4 ), (comm_ULONG)nNr4 ); };

					pProfiler->StartPartitioning();
				}
				else if( nParams == PARAM_STR_1 )	// Genau ein String!
				{	// Nur einen String ins Profiling aufnehmen
					aString1 += '\n';
					pRet->GenReturn( RET_ProfileInfo, SmartId(), aString1 );
				}
				else
				{	// Normales Profiling je Kommando: profile
					if ( pProfiler->IsAutoProfiling() )
					{
						pRet->GenReturn( RET_ProfileInfo, SmartId(), pProfiler->GetAutoProfiling() );
						pProfiler->StopAutoProfiling();
					}
					pProfiler->StartProfilingPerCommand();

					// Der Header ist abh�ngig vom Modus
					pRet->GenReturn( RET_ProfileInfo, SmartId(), pProfiler->GetProfileHeader() );
				}
			}
			else		// Profiling wieder ausschalten: Profile false
				if ( pProfiler )
				{
					if ( pProfiler->IsProfilingPerCommand() )
						pProfiler->StopProfilingPerCommand();

					if ( pProfiler->IsAutoProfiling() )
					{
						pRet->GenReturn( RET_ProfileInfo, SmartId(), pProfiler->GetAutoProfiling() );
						pProfiler->StopAutoProfiling();
					}

					if ( pProfiler->IsPartitioning() )
					{
						pRet->GenReturn( RET_ProfileInfo, SmartId( S_ProfileDump ), (comm_ULONG)0 );
						pProfiler->StopPartitioning();
					}

					delete pProfiler;
					pProfiler = NULL;
				}
			break;
		case RC_MenuGetItemCount:
		case RC_MenuGetItemId:
		case RC_MenuGetItemPos:
		case RC_MenuIsSeperator:
		case RC_MenuIsItemChecked:
		case RC_MenuIsItemEnabled:
		case RC_MenuGetItemText:
		case RC_MenuGetItemCommand:
        case RC_MenuHasSubMenu:
        case RC_MenuSelect:
			{
                PopupMenu *pPopup = NULL;
                MenuBar *pMenuBar = NULL;
				Menu *pMenu;

                USHORT nErr = GetCurrentMenues( pPopup, pMenuBar, pMenu );

				if ( !pMenu )
				{
                    if ( nErr == 1 )
					    ReportError( GEN_RES_STR0( S_NO_POPUP ) );
                    else 
                        ReportError( GEN_RES_STR0( S_NO_SUBMENU ) );
					break;
				}

				USHORT nItemCount = 0;
				switch ( nMethodId )
				{
					case RC_MenuGetItemCount:
					case RC_MenuGetItemId:
					case RC_MenuIsSeperator:
						{
							nItemCount = pMenu->GetItemCount();
							if ( pMenu->GetMenuFlags() & MENU_FLAG_HIDEDISABLEDENTRIES )
							{	// jep, we have to adjust the count
								BOOL bLastWasSeperator = TRUE;	// TRUE for Separator at the top
								for ( USHORT i = 0 ; i < pMenu->GetItemCount() ; i++ )
								{
									if ( !pMenu->IsItemEnabled( pMenu->GetItemId( i ) ) )
										nItemCount--;
									else
									{
										if ( pMenu->GetItemType( i ) == MENUITEM_SEPARATOR && bLastWasSeperator )
											nItemCount--;
										bLastWasSeperator = pMenu->GetItemType( i ) == MENUITEM_SEPARATOR;
									}
								}
								if ( bLastWasSeperator )	// Separator at bottom
									nItemCount--;
							}
						}
						break;
				}

				// for certain methods calculate the physical index (reinserting the hidden entries)
				USHORT nPhysicalIndex = 0;
				switch ( nMethodId )
				{
					case RC_MenuGetItemId:
					case RC_MenuIsSeperator:
						{
							nPhysicalIndex = nNr1;
							if ( pMenu->GetMenuFlags() & MENU_FLAG_HIDEDISABLEDENTRIES )
							{	// jep, we have to adjust the position
								BOOL bLastWasSeperator = TRUE;	// TRUE for Separator at the top
								USHORT nVisibleCount = 0;
								for ( USHORT i = 0 ; i < pMenu->GetItemCount() && nVisibleCount < nNr1 ; i++ )
								{
									if ( pMenu->IsItemEnabled( pMenu->GetItemId( i ) )
										 && !( pMenu->GetItemType( i ) == MENUITEM_SEPARATOR && bLastWasSeperator ) )
									{
										nVisibleCount++;
										bLastWasSeperator = pMenu->GetItemType( i ) == MENUITEM_SEPARATOR;
									}
									else
										nPhysicalIndex++;
								}
								DBG_ASSERT( nVisibleCount == nNr1, "Adaption of Index failed: nVisibleCount != nNr1" );
							}
						}
						break;
				}



				switch ( nMethodId )
				{
					case RC_MenuGetItemCount:
						{
							pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_ULONG)nItemCount );
						}
						break;
					case RC_MenuGetItemId:
						{
							if ( ValueOK(aSmartMethodId, RcString( nMethodId ),nNr1,nItemCount) )
								pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_ULONG)pMenu->GetItemId(nPhysicalIndex-1) );
						}
						break;
					case RC_MenuGetItemPos:
						{
							USHORT nLogicalPos = pMenu->GetItemPos(nNr1);
							if ( MENU_ITEM_NOTFOUND != nLogicalPos && pMenu->GetMenuFlags() & MENU_FLAG_HIDEDISABLEDENTRIES )
							{	// jep, we have to adjust the position
								if ( !pMenu->IsItemEnabled( nNr1 ) )
									nLogicalPos = MENU_ITEM_NOTFOUND;
								else
								{
									BOOL bLastWasSeperator = FALSE;
									for ( int i = nLogicalPos ; i >= 0 ; i-- )
									{
										if ( !pMenu->IsItemEnabled( pMenu->GetItemId( sal::static_int_cast< USHORT >(i) ) ) ||
											 ( pMenu->GetItemType( sal::static_int_cast< USHORT >(i) ) == MENUITEM_SEPARATOR && bLastWasSeperator ) )
											nLogicalPos--;
										bLastWasSeperator = pMenu->GetItemType( sal::static_int_cast< USHORT >(i) ) == MENUITEM_SEPARATOR;
									}
								}
							}
							pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_ULONG)(nLogicalPos+1) );
						}
						break;
					case RC_MenuIsSeperator:
						{
							if ( ValueOK(aSmartMethodId, RcString( nMethodId ),nNr1,nItemCount) )
								pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_BOOL)(pMenu->GetItemType(nPhysicalIndex-1) == MENUITEM_SEPARATOR) );
						}
						break;
					case RC_MenuIsItemChecked:
						{
							pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_BOOL)pMenu->IsItemChecked(nNr1) );
						}
						break;
					case RC_MenuIsItemEnabled:
						{
							pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_BOOL)pMenu->IsItemEnabled(nNr1) );
						}
						break;
					case RC_MenuGetItemText:
						{
							pRet->GenReturn ( RET_Value, aSmartMethodId, (String)pMenu->GetItemText(nNr1) );
						}
						break;
					case RC_MenuGetItemCommand:
						{
							pRet->GenReturn ( RET_Value, aSmartMethodId, (String)pMenu->GetItemCommand(nNr1) );
						}
						break;
                    case RC_MenuHasSubMenu:
						{
							pRet->GenReturn ( RET_Value, aSmartMethodId, (BOOL)(pMenu->GetPopupMenu(nNr1) != NULL) );
						}
                        break;
					case RC_MenuSelect:
						{
							if ( pMenu->GetPopupMenu(nNr1) )
							{
								if ( !aSubMenuId1.GetNum() )
									aSubMenuId1 = SmartId(nNr1);
								else if ( !aSubMenuId2.GetNum() )
									aSubMenuId2 = SmartId(nNr1);
								else if ( !aSubMenuId3.GetNum() )
									aSubMenuId3 = SmartId(nNr1);

                                if ( pPopup )
								    pPopup->SelectEntry(nNr1);
                                else
                                    pMenuBar->SelectEntry(nNr1);
							}
							else
							{
                                if ( pPopup )
                                {
    								pPopup->EndExecute(nNr1);
								    aSubMenuId1 = SmartId();
								    aSubMenuId2 = SmartId();
									aSubMenuId3 = SmartId();
									pMenuWindow = NULL;
                                }
                                else
								{
                                    pMenuBar->SelectEntry(nNr1);
								    aSubMenuId1 = SmartId();
								    aSubMenuId2 = SmartId();
								    aSubMenuId3 = SmartId();
									pMenuWindow = NULL;
								}
							}
						}
						break;
				}
			}
			break;
		case RC_SetControlType:
			{
                DirectLog( S_QAError, GEN_RES_STR0( S_DEPRECATED ) );
			}
			break;
		case RC_Kill:
		case RC_RmDir:
		case RC_MkDir:
		case RC_FileCopy:
		case RC_Name:
		case RC_Dir:
		case RC_FileLen:
		case RC_FileDateTime:
			{
				long nErrorcode = FSYS_ERR_OK;
				switch ( nMethodId )
				{
					case RC_Kill:
						{
							DirEntry aFile( aString1 );
							nErrorcode = aFile.GetError();
							if ( FSYS_ERR_OK == nErrorcode && FileStat( aFile ).IsKind( FSYS_KIND_FILE ) )
								nErrorcode = aFile.Kill();
							else
								nErrorcode = FSYS_ERR_NOTAFILE;
						}
						break;
					case RC_RmDir:
						{
							DirEntry aDir( aString1 );
							nErrorcode = aDir.GetError();
							if ( FSYS_ERR_OK == nErrorcode && FileStat( aDir ).IsKind( FSYS_KIND_DIR ) )
								nErrorcode = aDir.Kill();
							else
								nErrorcode = FSYS_ERR_NOTADIRECTORY;
						}
						break;
					case RC_MkDir:
						{
							DirEntry aDir( aString1 );
							nErrorcode = aDir.GetError();
							if ( !nErrorcode && !aDir.MakeDir() )
								nErrorcode = FSYS_ERR_UNKNOWN;
//	Workaround f�r Bug 60693
//								nErrorcode = aDir.GetError();
						}
						break;
					case RC_FileCopy:
						{
							nErrorcode = DirEntry( aString1 ).CopyTo( DirEntry( aString2 ), FSYS_ACTION_COPYFILE );
						}
						break;
					case RC_Name:
						{
							nErrorcode = DirEntry( aString1 ).MoveTo( DirEntry( aString2 ) );
						}
						break;
					case RC_Dir:
						{

                            String aPath;
                            USHORT nDirFlags = 0;
// from basic/source/inc/runtime.hxx
#define Sb_ATTR_NORMAL          0x0000
#define Sb_ATTR_HIDDEN          0x0002
#define Sb_ATTR_SYSTEM          0x0004
#define Sb_ATTR_VOLUME          0x0008
#define Sb_ATTR_DIRECTORY       0x0010
#define Sb_ATTR_ARCHIVE         0x0020
                            // copied from Basic and adapted  basic/source/runtime/methods.cxx Revision 1.54
			                if ( (nParams & PARAM_STR_1) )
			                {
				                delete pDir;
								pDir = NULL; // wg. Sonderbehandlung Sb_ATTR_VOLUME
				                DirEntry aEntry( aString1 );
				                FileStat aStat( aEntry );
				                if(!aStat.GetError() && (aStat.GetKind() & FSYS_KIND_FILE))
				                {
					                // OK, only a filename
					                // cut off path (VB4)
					                aPath = aEntry.GetName();
				                }
                                else
                                {
				                    USHORT nFlags = 0;
				                    if ( (nParams & PARAM_USHORT_1) )
					                    nDirFlags = nFlags = nNr1;
				                    else
					                    nDirFlags = nFlags = Sb_ATTR_HIDDEN | Sb_ATTR_SYSTEM | Sb_ATTR_DIRECTORY;

				                    // Nur diese Bitmaske ist unter Windows erlaubt
	                    #ifdef WIN
				                    if( nFlags & ~0x1E )
                                    {
                                        nDirFlags = 0;
					                    StarBASIC::Error( SbERR_BAD_ARGUMENT );
                                    }
	                    #endif
				                    // Sb_ATTR_VOLUME wird getrennt gehandelt
				                    if( nDirFlags & Sb_ATTR_VOLUME )
					                    aPath = aEntry.GetVolume();
				                    else
				                    {
					                    // Die richtige Auswahl treffen
					                    USHORT nMode = FSYS_KIND_FILE;
					                    if( nFlags & Sb_ATTR_DIRECTORY )
						                    nMode |= FSYS_KIND_DIR;
					                    if( nFlags == Sb_ATTR_DIRECTORY )
						                    nMode = FSYS_KIND_DIR;
					                    pDir = new Dir( aEntry, (DirEntryKind) nMode );
                                        nErrorcode = pDir->GetError();
					                    nDirPos = 0;
				                    }
                                }
			                }

			                if( pDir )
			                {
				                for( ;; )
				                {
					                if( nDirPos >= pDir->Count() )
					                {
						                delete pDir;
						                pDir = NULL;
						                aPath.Erase();
						                break;
					                }
					                DirEntry aNextEntry=(*(pDir))[nDirPos++];
					                aPath = aNextEntry.GetName(); //Full();
	                #ifdef WIN
					                aNextEntry.ToAbs();
					                String sFull(aNextEntry.GetFull());
					                unsigned nFlags;

					                if (_dos_getfileattr( sFull.GetStr(), &nFlags ))
						                nErrorcode = FSYS_ERR_NOTEXISTS;
					                else
					                {
						                INT16 nCurFlags = nDirFlags;
						                if( (nCurFlags == Sb_ATTR_NORMAL)
						                && !(nFlags & ( _A_HIDDEN | _A_SYSTEM | _A_VOLID | _A_SUBDIR ) ) )
							                break;
						                else if( (nCurFlags & Sb_ATTR_HIDDEN) && (nFlags & _A_HIDDEN) )
							                break;
						                else if( (nCurFlags & Sb_ATTR_SYSTEM) && (nFlags & _A_SYSTEM) )
							                break;
						                else if( (nCurFlags & Sb_ATTR_VOLUME) && (nFlags & _A_VOLID) )
							                break;
						                else if( (nCurFlags & Sb_ATTR_DIRECTORY) && (nFlags & _A_SUBDIR) )
							                break;
					                }
	                #else
					                break;
	                #endif
				                }
			                }
							if ( !nErrorcode )
							{
								pRet->GenReturn ( RET_Value, aSmartMethodId, aPath );
                            }
                            
                            
                            
/* keep old Implementation for now                            
                            // neues Verzeichnis einlesen
							if ( (nParams & PARAM_STR_1) )
							{
								if ( pDir )
								{
									delete pDir;
									pDir = NULL;
								}
								DirEntryKind aDirEntryKind = FSYS_KIND_FILE | FSYS_KIND_DIR;
								if ( (nParams & PARAM_USHORT_1) && nNr1 )
								{
									if ( nNr1 & 16 )
										aDirEntryKind = FSYS_KIND_DIR;
									else
										ReportError( GEN_RES_STR0( S_SELECTION_BY_ATTRIBUTE_ONLY_DIRECTORIES ) );
								}

								DirEntry aEntry( aString1 );
								nErrorcode = aEntry.GetError();
								if ( !nErrorcode )
								{
									nDirPos = 0;
									FileStat aFS( aEntry );
									if ( !aFS.IsKind( FSYS_KIND_WILD ) && !aFS.IsKind( FSYS_KIND_DIR ) && aEntry.Exists() )
									{	// Sonderbehandlung f�r genau einen Eintrag
										if ( !aFS.IsKind( FSYS_KIND_DIR ) && ( aDirEntryKind == FSYS_KIND_DIR ) )
											pRet->GenReturn ( RET_Value, aSmartMethodId, String() );
										else
											pRet->GenReturn ( RET_Value, aSmartMethodId, (String)(aEntry.GetName()) );

										break;
									}
									else
									{
										pDir = new Dir( aEntry, aDirEntryKind );
										nErrorcode = pDir->GetError();
									}
								}
							}

							if ( !pDir )
								pDir = new Dir;

							if ( !nErrorcode && ValueOK( aSmartMethodId, GEN_RES_STR0( S_NO_MORE_FILES ), nDirPos+1, pDir->Count()+1 ) )
							{
								if ( nDirPos == pDir->Count() )
									pRet->GenReturn ( RET_Value, aSmartMethodId, String() );
								else
									pRet->GenReturn ( RET_Value, aSmartMethodId, (String)((*pDir)[ nDirPos ].GetName()) );
								nDirPos++;
							}*/
						}
						break;
					case RC_FileLen:
						{
							DirEntry aFile( aString1 );
							nErrorcode = aFile.GetError();
							if ( FSYS_ERR_OK == nErrorcode )
							{
								FileStat aFS( aFile );
								pRet->GenReturn ( RET_Value, aSmartMethodId, static_cast<comm_ULONG>(aFS.GetSize()) ); //GetSize() ULONG != comm_ULONG on 64bit
								nErrorcode = aFS.GetError();
							}
						}
						break;
					case RC_FileDateTime:
						{
							DirEntry aFile( aString1 );
							nErrorcode = aFile.GetError();
							if ( FSYS_ERR_OK == nErrorcode )
							{
								FileStat aStat( aFile );
								Time aTime( aStat.TimeModified() );
								Date aDate( aStat.DateModified() );
								nErrorcode = aStat.GetError();

								double fSerial = (double)( aDate - Date(30,12,1899) );
								long nSeconds = aTime.GetHour();
								nSeconds *= 3600;
								nSeconds += aTime.GetMin() * 60;
								nSeconds += aTime.GetSec();
								double nDays = ((double)nSeconds) / (double)(24.0*3600.0);
								fSerial += nDays;

								SbxValueRef xValue = new SbxValue( SbxDATE );
								xValue->PutDate( fSerial );

								pRet->GenReturn ( RET_Value, aSmartMethodId, *xValue );
							}
						}
						break;
				}
				switch ( nErrorcode )
				{
					case FSYS_ERR_OK:
						break;
					case FSYS_ERR_MISPLACEDCHAR:
						{
							ReportError( CUniString("MISPLACEDCHAR") );
						}
						break;
					case FSYS_ERR_INVALIDCHAR:
						{
							ReportError( CUniString("INVALIDCHAR") );
						}
						break;
					case FSYS_ERR_NOTEXISTS:
						{
							ReportError( CUniString("NOTEXISTS") );
						}
						break;
					case FSYS_ERR_ALREADYEXISTS:
						{
							ReportError( CUniString("ALREADYEXISTS") );
						}
						break;
					case FSYS_ERR_NOTADIRECTORY:
						{
							ReportError( CUniString("NOTADIRECTORY") );
						}
						break;
					case FSYS_ERR_NOTAFILE:
						{
							ReportError( CUniString("NOTAFILE") );
						}
						break;
					case FSYS_ERR_INVALIDDEVICE:
						{
							ReportError( CUniString("INVALIDDEVICE") );
						}
						break;
					case FSYS_ERR_ACCESSDENIED:
						{
							ReportError( CUniString("ACCESSDENIED") );
						}
						break;
					case FSYS_ERR_LOCKVIOLATION:
						{
							ReportError( CUniString("LOCKVIOLATION") );
						}
						break;
					case FSYS_ERR_VOLUMEFULL:
						{
							ReportError( CUniString("VOLUMEFULL") );
						}
						break;
					case FSYS_ERR_ISWILDCARD:
						{
							ReportError( CUniString("ISWILDCARD") );
						}
						break;
					case FSYS_ERR_NOTSUPPORTED:
						{
							ReportError( CUniString("NOTSUPPORTED") );
						}
						break;
					case FSYS_ERR_UNKNOWN:
						{
							ReportError( CUniString("UNKNOWN") );
						}
						break;
					default:
						{
							ReportError( CUniString("Not an FSYS Error") );
						}
				}

			}
			break;
		case RC_TypeKeysDelay:
			{
				if( (nParams & PARAM_BOOL_1) )
				{
					bDoTypeKeysDelay = bBool1;
				}
				else if( nParams & PARAM_USHORT_1 )
				{
					nMinTypeKeysDelay = nNr1;
					if( nParams & PARAM_USHORT_2 )
						nMaxTypeKeysDelay = nNr2;
					else
						nMaxTypeKeysDelay = nMinTypeKeysDelay;
				}
				else
					ReportError( GEN_RES_STR0( S_INVALID_PARAMETERS ) );
			}
			break;
		case RC_GetMouseStyle:
			{
				Pointer aPointer;
//				if ( DragManager::GetDragManager() )
//					aPointer = DragManager::GetDragManager()->GetDragPointer();
//				else
				{
					Window *pActualWin = GetMouseWin();
					if ( pActualWin )
						aPointer = pActualWin->GetPointer();
					else
					{
						ReportError( GEN_RES_STR1( S_POINTER_OUTSIDE_APPWIN, RcString( nMethodId ) ) );
						aPointer = Pointer( POINTER_NULL );
					}
				}
				pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_ULONG)aPointer.GetStyle() );
			}
			break;
		case RC_UnpackStorage:
			{
				if( (nParams & PARAM_STR_1) )
				{
					String aFileName( aString1 );
					DirEntry aDestPath;
					if( (nParams & PARAM_STR_2) )
						aDestPath = DirEntry( aString2 );
					else
					{
						aDestPath = DirEntry( aFileName );
						aDestPath.SetExtension( CUniString( "plaintext" ) );
					}

#if OSL_DEBUG_LEVEL > 1
                    USHORT nEntries = Dir( aDestPath, FSYS_KIND_FILE | FSYS_KIND_DIR ).Count();
                    (void) nEntries; /* avoid warning about unused parameter */ 
#endif
                    // The Count is only larger than 2 is the path is a directory which is not empty
                    // the Count of 2 results from the "." and ".." directory
                    if ( Dir( aDestPath, FSYS_KIND_FILE | FSYS_KIND_DIR ).Count() > 2 )
                        DirectLog( S_QAError, GEN_RES_STR1( S_DIRECTORY_NOT_EMPTY, aDestPath.GetFull() ) );

					SotStorageRef xStorage = new SotStorage( aFileName, STREAM_STD_READ );
					if ( xStorage->GetError() )
						ReportError( GEN_RES_STR2(S_UNPACKING_STORAGE_FAILED, aFileName, aDestPath.GetFull()) );
					else
						UnpackStorage( xStorage, aDestPath );
				}
				else
					ReportError( GEN_RES_STR0( S_INVALID_PARAMETERS ) );
			}
			break;
		case RC_CloseSysDialog:
		case RC_ExistsSysDialog:
			{
				if( (nParams & PARAM_USHORT_1) )
				{
                    Reference < XCancellable > xPicker;
                    switch( nNr1 )
                    {
                        case CONST_FilePicker:
                            {
                                xPicker.set( Reference < XCancellable >( svt::GetTopMostFilePicker(), UNO_QUERY ) );
                            }
                            break;
                        case CONST_FolderPicker:
                            {
                                xPicker.set( Reference < XCancellable >( svt::GetTopMostFolderPicker(), UNO_QUERY ) );
                            }
                            break;
                        default:
    					    ReportError( GEN_RES_STR0( S_INVALID_PARAMETERS ) );
                    }
                    switch( nMethodId )
                    {
		                case RC_CloseSysDialog:
                            {
                                if ( xPicker.is() )
                                    xPicker->cancel();
                                else
                                    ReportError( GEN_RES_STR0( S_INVALID_PARAMETERS ) );
                            }
                            break;
		                case RC_ExistsSysDialog:
                            {
                				pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_BOOL)xPicker.is() );
                            }
                            break;
                        default:
    					    ReportError( GEN_RES_STR0( S_INVALID_PARAMETERS ) );
                    }
				}
				else
					ReportError( GEN_RES_STR0( S_INVALID_PARAMETERS ) );
			}
			break;
		case RC_SAXCheckWellformed:
		case RC_SAXReadFile:

		case RC_SAXGetNodeType:
		case RC_SAXGetAttributeCount:
		case RC_SAXGetAttributeName:
		case RC_SAXGetAttributeValue:
		case RC_SAXGetChildCount:
		case RC_SAXGetElementName:
		case RC_SAXGetChars:

        case RC_SAXSeekElement:
		case RC_SAXHasElement:
		case RC_SAXGetElementPath:

        case RC_SAXRelease:
            {
                HandleSAXParser();
			}
			break;
        case RC_RecordMacro:
            {
				if ( ! (nParams & PARAM_BOOL_1) )
					bBool1 = TRUE;

                MacroRecorder::GetMacroRecorder()->SetActionRecord( bBool1 );
            }
            break;
        case RC_GetDocumentCount :
            {
                pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_USHORT)GetDocWinCount() );
            }
            break;
        case RC_ActivateDocument :
            {
				if( nParams & PARAM_USHORT_1 )
                {
                    if ( ValueOK(aSmartMethodId, RcString( nMethodId ), nNr1, GetDocWinCount() ) )
                    {
                        Window* pWin = GetDocWin( nNr1-1 );
                        if ( pWin )
                        {
                            pWin->ToTop();
                            pWin->GrabFocus();
                        }
                    }
                }
                else
				    ReportError( GEN_RES_STR0( S_INVALID_PARAMETERS ) );
            }
            break;
        case RC_GetSystemLanguage :
            {
                pRet->GenReturn ( RET_Value, aSmartMethodId, (comm_USHORT)Application::GetSettings().GetLanguage() );
            }
            break;
        case RC_CatchGPF :
            {
				if( (nParams & PARAM_BOOL_1) )
                    bCatchGPF = bBool1;
                else
					bCatchGPF = TRUE;
            }
            break;
        case RC_IsProduct :
            {
                BOOL bIsProduct;
                #ifdef DBG_UTIL
                    bIsProduct = FALSE;
                #else
                    bIsProduct = TRUE;
                #endif
                pRet->GenReturn ( RET_Value, aSmartMethodId, (BOOL)bIsProduct );
            }
            break;
        case RC_UsePostEvents :
            {
				if( (nParams & PARAM_BOOL_1) )
                    bUsePostEvents = bBool1;
                else
					bUsePostEvents = TRUE;
            }
            break;
        default:
			ReportError( GEN_RES_STR1( S_UNKNOWN_COMMAND, RcString( nMethodId ) ) );
	}
	SendProfile( RcString(nMethodId) );
	delete this;
	return TRUE;
}


BOOL StatementCommand::UnpackStorage( SotStorageRef xStorage, DirEntry &aBaseDir )
{
	SvStorageInfoList aList;
	xStorage->FillInfoList( &aList );

	for( USHORT i = 0; i < aList.Count(); i++ )
	{
		SvStorageInfo& rInfo = aList.GetObject( i );
		String aName = rInfo.GetName();
		DirEntry aPath ( aBaseDir );
		aPath += DirEntry( aName );
		BOOL bIsStorage = xStorage->IsStorage( aName );
		if ( bIsStorage )
		{
			SotStorageRef xSubStorage = xStorage->OpenSotStorage( aName, STREAM_STD_READ );
				if ( xSubStorage->GetError() )
				{
					ReportError( GEN_RES_STR2(S_UNPACKING_STORAGE_FAILED, aName, aPath.GetFull()) );
					return FALSE;
				}
				UnpackStorage( xSubStorage, aPath );
		}
		else
		{
			if ( !aPath.MakeDir( TRUE ) )
			{
				ReportError( GEN_RES_STR1(S_CANNOT_CREATE_DIRECTORY, aPath.GetFull()) );
				return FALSE;
			}
			SotStorageStreamRef xStream = xStorage->OpenSotStream( aName, STREAM_STD_READ );
			SvFileStream aDestination( aPath.GetFull(), STREAM_STD_READWRITE | STREAM_TRUNC );
			(*xStream) >> aDestination;
			if ( aDestination.GetError() != ERRCODE_NONE )
			{
				ReportError( GEN_RES_STR2(S_UNPACKING_STORAGE_FAILED, aName, aPath.GetFull()) );
				return FALSE;
			}
			aDestination.Close();
		}
	}
	return TRUE;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

StatementControl::StatementControl( SCmdStream *pCmdIn, USHORT nControlIdType )
: StatementList()
, nNr1( 0 )
, nNr2( 0 )
, nNr3( 0 )
, nNr4( 0 )
, nLNr1( 0 )
, aString1()
, aString2()
, bBool1(FALSE)
, bBool2(FALSE)
{
	QueStatement( NULL );
    if ( nControlIdType == SIControl )
    {
        comm_ULONG nId;
        pCmdIn->Read( nId );
        aUId = SmartId( nId );
    }
    else if ( nControlIdType == SIStringControl )
    {
        String aId;
        pCmdIn->Read( aId );
        aUId = SmartId( aId );
    }
    else
    {
        DBG_ERROR( "Wrong ControlType" );
    }

	pCmdIn->Read( nMethodId );
	pCmdIn->Read( nParams );

	if( nParams & PARAM_USHORT_1 )	pCmdIn->Read( nNr1 );
	if( nParams & PARAM_USHORT_2 )	pCmdIn->Read( nNr2 );
	if( nParams & PARAM_USHORT_3 )	pCmdIn->Read( nNr3 );
	if( nParams & PARAM_USHORT_4 )	pCmdIn->Read( nNr4 );
	if( nParams & PARAM_ULONG_1 )	pCmdIn->Read( nLNr1 );
	if( nParams & PARAM_STR_1 )		pCmdIn->Read( aString1 );
	if( nParams & PARAM_STR_2 )		pCmdIn->Read( aString2 );
	if( nParams & PARAM_BOOL_1 )	pCmdIn->Read( bBool1 );
	if( nParams & PARAM_BOOL_2 )	pCmdIn->Read( bBool2 );

#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Reading Control: UId: " );
	m_pDbgWin->AddText( aUId.GetText() );
	m_pDbgWin->AddText( " Methode: " );
	m_pDbgWin->AddText( String::CreateFromInt32( nMethodId ) );
	m_pDbgWin->AddText( " Params:" );
	if( nParams & PARAM_USHORT_1 )	{m_pDbgWin->AddText( " n1:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr1 ) );}
	if( nParams & PARAM_USHORT_2 )	{m_pDbgWin->AddText( " n2:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr2 ) );}
	if( nParams & PARAM_USHORT_3 )	{m_pDbgWin->AddText( " n3:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr3 ) );}
	if( nParams & PARAM_USHORT_4 )	{m_pDbgWin->AddText( " n4:" );m_pDbgWin->AddText( String::CreateFromInt32( nNr4 ) );}
	if( nParams & PARAM_ULONG_1 )	{m_pDbgWin->AddText( " nl1:" );m_pDbgWin->AddText( String::CreateFromInt64( nLNr1 ) );}
	if( nParams & PARAM_STR_1 )		{m_pDbgWin->AddText( " s1:" );m_pDbgWin->AddText( aString1 );}
	if( nParams & PARAM_STR_2 )		{m_pDbgWin->AddText( " s2:" );m_pDbgWin->AddText( aString2 );}
/*    if( nParams & PARAM_BOOL_1 )    pCmdIn->Read( bBool1 );
	if( nParams & PARAM_BOOL_2 )    pCmdIn->Read( bBool2 );*/
	m_pDbgWin->AddText( "\n" );
#endif
}

BOOL IsDialog(Window *pWin)
{	// Alles was von SystemWindow abgeleitet ist
	if ( !pWin )
		return FALSE;

	switch (pWin->GetType())
	{
		case WINDOW_FLOATINGWINDOW:
		case WINDOW_DOCKINGWINDOW:
		case WINDOW_MODELESSDIALOG:
		case WINDOW_DIALOG:
		case WINDOW_MODALDIALOG:
		case WINDOW_WORKWINDOW:
		case WINDOW_TABDIALOG:

		case WINDOW_MESSBOX:
		case WINDOW_INFOBOX:
		case WINDOW_WARNINGBOX:
		case WINDOW_ERRORBOX:
		case WINDOW_QUERYBOX:
		case WINDOW_BUTTONDIALOG:
		case WINDOW_FILEDIALOG:
		case WINDOW_PRINTDIALOG:
		case WINDOW_PRINTERSETUPDIALOG:

// ab hier nicht ansprechbar (da nicht implementiert)
		case WINDOW_SYSWINDOW:
		case WINDOW_SYSTEMDIALOG:
		case WINDOW_COLORDIALOG:
		case WINDOW_FONTDIALOG:
		case WINDOW_PATHDIALOG:


			return TRUE;
//			break;
		default:
			return FALSE;
//			break;
	}
}


BOOL IsAccessable(Window *pWin)
{
	if ( pWin == NULL )
		return FALSE;

	return pWin->IsEnabled() && pWin->IsInputEnabled();
}



// neue Hilfsfunktion
static Window*ImpGetButton( Window *pBase, WinBits nMask, WinBits nWinBits )
{
	USHORT n = pBase->GetChildCount();
	for( USHORT i = 0 ; i < n; i++ ) {
		Window *pChild = pBase->GetChild(i);
		if(    pChild->GetType() == WINDOW_OKBUTTON
			|| pChild->GetType() == WINDOW_CANCELBUTTON
			|| pChild->GetType() == WINDOW_HELPBUTTON
			|| pChild->GetType() == WINDOW_PUSHBUTTON )
			if( !nMask || ( pChild->GetStyle() & nMask ) == nWinBits )
				return pChild;
	}
	return NULL;
}

BOOL StatementControl::ControlOK( Window *pControl, const sal_Char* cBezeichnung )
{
	if ( pControl && ( ( ( IsAccessable(pControl) || (nMethodId & M_WITH_RETURN) ) &&
						 pControl->IsVisible() ) ||
					     aUId.Matches( UID_ACTIVE ) ) )
		return TRUE;
	else
	{
		UniString aBezeichnung( cBezeichnung, RTL_TEXTENCODING_ASCII_US );
		if ( aBezeichnung.Len() > 0 )
		{
			if (!pControl)
				ReportError( aUId, GEN_RES_STR1( S_WIN_NOT_FOUND, aBezeichnung ) );
			else if ( !pControl->IsVisible() )
				ReportError( aUId, GEN_RES_STR1( S_WIN_INVISIBLE, aBezeichnung ) );
			else
				ReportError( aUId, GEN_RES_STR1( S_WIN_DISABLED, aBezeichnung ) );
		}
        #if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( aBezeichnung.AppendAscii(" NotFound or Disabled or Invisible") );
		#endif

		return FALSE;
	}
}


BOOL StatementList::ValueOK( SmartId aId, String aBezeichnung, ULONG nValue, ULONG nMax )
{

	if ( nMax < nValue )
	{
        if ( aBezeichnung.Len() > 0 )
		    ReportError( aId, GEN_RES_STR3( S_NUMBER_TOO_BIG, aBezeichnung, UniString::CreateFromInt32( nValue ), UniString::CreateFromInt32( nMax ) ) );
		return FALSE;
	}
	if ( nValue < 1 )
	{
        if ( aBezeichnung.Len() > 0 )
			ReportError( aId, GEN_RES_STR3c3( S_NUMBER_TOO_SMALL, aBezeichnung, UniString::CreateFromInt32( nValue ), "1" ) );
		return FALSE;
	}
	return TRUE;
}

USHORT StatementList::GetCurrentMenues( PopupMenu *&pPopup, MenuBar *&pMenuBar, Menu *&pMenu )
{
    if ( WinPtrValid( pMenuWindow ) )
        pMenuBar = pMenuWindow->GetMenuBar();

    if ( pMenuBar )     // use MenuBar as base
        pMenu = pMenuBar;
    else        // use contextmenu as base
    {
        pMenu = PopupMenu::GetActivePopupMenu();
        pPopup = PopupMenu::GetActivePopupMenu();
    }

	if ( !pMenu )
		return 1;

	if ( aSubMenuId1.GetNum() )
    {
		pPopup = pMenu->GetPopupMenu(
            sal::static_int_cast< USHORT >(aSubMenuId1.GetNum()));
        pMenu = pPopup;
    }

	if ( pMenu && aSubMenuId2.GetNum() )
    {
		pPopup = pMenu->GetPopupMenu(
            sal::static_int_cast< USHORT >(aSubMenuId2.GetNum()));
        pMenu = pPopup;
    }

	if ( pMenu && aSubMenuId3.GetNum() )
    {
		pPopup = pMenu->GetPopupMenu(
            sal::static_int_cast< USHORT >(aSubMenuId3.GetNum()));
        pMenu = pPopup;
    }

    return 0;
}

void StatementControl::AnimateMouse( Window *pControl, TTHotSpots aWohin )
{
	Point aZiel;

	switch (aWohin)
	{
		case MitteLinks:
			{
				long nHeight = pControl->GetSizePixel().Height();
				aZiel.X() += 5;
				aZiel.Y() += nHeight / 2;
			}
			break;
		case Mitte:
			{
				Size aSize = pControl->GetOutputSizePixel();
				aZiel.Move( aSize.Width() / 2, aSize.Height() / 2 );
			}
			break;
		case MitteOben:
			{
				long nWidth = pControl->GetSizePixel().Width();
				aZiel.X() += nWidth / 2;
				aZiel.Y() += 5;
			}
			break;
	}
	AnimateMouse( pControl, aZiel );
}


void StatementControl::AnimateMouse( Window *pControl, Point aWohin )
{
	Point aAkt = pControl->GetPointerPosPixel();
	Point aZiel = aWohin;

	long nSteps;
	Point aDiff = aAkt - aZiel;

	if ( Abs(aDiff.X()) < Abs(aDiff.Y()) )
		nSteps = Abs(aDiff.Y()) / 5;
	else
		nSteps = Abs(aDiff.X()) / 5;
	if ( nSteps == 0 )
		return;

	aDiff *= 1000;
	aDiff /= nSteps;

	StatementList::bExecuting = TRUE;		// Bah ist das ein ekliger Hack
												// Das verhindert, da� schon der n�chste Befehl ausgef�hrt wird.

	for ( ; nSteps ; nSteps-- )
	{
		if ( Abs((aAkt - pControl->GetPointerPosPixel()).X()) > 5 ||
			 Abs((aAkt - pControl->GetPointerPosPixel()).Y()) > 5 )
			nSteps = 1;
		aAkt = aZiel + aDiff * nSteps / 1000;
		pControl->SetPointerPosPixel(aAkt);
		SafeReschedule();
	}
	pControl->SetPointerPosPixel(aZiel);
	StatementList::bExecuting = FALSE;	// Bah ist das ein ekliger Hack
}


BOOL StatementControl::MaybeDoTypeKeysDelay( Window *pTestWindow )
{
	if ( bDoTypeKeysDelay )
	{
		ULONG nTimeWait = nMinTypeKeysDelay;
		if ( nMaxTypeKeysDelay != nMinTypeKeysDelay )
			nTimeWait  += Time::GetSystemTicks() % ( nMaxTypeKeysDelay - nMinTypeKeysDelay );
		Timer aTimer;
		aTimer.SetTimeout( nTimeWait );
		aTimer.Start();
		StatementList::bExecuting = TRUE;		// Bah ist das ein ekliger Hack
													// Das verhindert, da� schon der n�chste Befehl ausgef�hrt wird.
		while ( aTimer.IsActive() )
		{
			SafeReschedule( TRUE );
		}
		StatementList::bExecuting = FALSE;	// Bah ist das ein ekliger Hack
		if ( !WinPtrValid(pTestWindow ) )
		{
			ReportError( aUId, GEN_RES_STR1( S_WINDOW_DISAPPEARED, MethodString( nMethodId ) ) );
			return FALSE;
		}
	}
	return TRUE;
}

BOOL StatementControl::HandleVisibleControls( Window *pControl )
{
	if( pControl )		// Also auch bei Disabled nicht jedoch bei Invisible
	{
		switch( nMethodId )
		{
		case M_IsEnabled:
			pRet->GenReturn ( RET_Value, aUId, IsAccessable(pControl) );
			break;
		case M_IsVisible:
			pRet->GenReturn ( RET_Value, aUId, pControl->IsVisible() );
			break;
		case M_GetPosX:
			if ( pControl->GetType() == WINDOW_DOCKINGWINDOW && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_FLOATINGWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r FloatingWindows
			if ( pControl->GetType() == WINDOW_TABCONTROL && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_TABDIALOG )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r TabDialoge
			if ( pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_BORDERWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r Border
            if ( (nParams & PARAM_BOOL_1) && bBool1 )
                pControl = pControl->GetWindow( WINDOW_OVERLAP );

			if ( pControl->GetType() == WINDOW_DOCKINGWINDOW && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_SPLITWINDOW )
			{
				Point aPos = pControl->GetPosPixel();
				aPos = pControl->GET_REAL_PARENT()->OutputToScreenPixel( aPos );
				pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)aPos.X() );
			}
			else
				pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pControl->GetPosPixel().X() );
			break;
		case M_GetPosY:
			if ( pControl->GetType() == WINDOW_DOCKINGWINDOW && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_FLOATINGWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r FloatingWindows
			if ( pControl->GetType() == WINDOW_TABCONTROL && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_TABDIALOG )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r TabDialoge
			if ( pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_BORDERWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r Border
            if ( (nParams & PARAM_BOOL_1) && bBool1 )
                pControl = pControl->GetWindow( WINDOW_OVERLAP );

			if ( pControl->GetType() == WINDOW_DOCKINGWINDOW && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_SPLITWINDOW )
			{
				Point aPos = pControl->GetPosPixel();
				aPos = pControl->GET_REAL_PARENT()->OutputToScreenPixel( aPos );
				pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)aPos.Y() );
			}
			else
				pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pControl->GetPosPixel().Y() );
			break;
		case M_GetSizeX:
			if ( pControl->GetType() == WINDOW_DOCKINGWINDOW && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_FLOATINGWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r FloatingWindows
			if ( pControl->GetType() == WINDOW_TABCONTROL && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_TABDIALOG )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r TabDialoge
			if ( pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_BORDERWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r Border
            if ( (nParams & PARAM_BOOL_1) && bBool1 )
                pControl = pControl->GetWindow( WINDOW_OVERLAP );

			pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pControl->GetSizePixel().Width() );
			break;
		case M_GetSizeY:
			if ( pControl->GetType() == WINDOW_DOCKINGWINDOW && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_FLOATINGWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r FloatingWindows
			if ( pControl->GetType() == WINDOW_TABCONTROL && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_TABDIALOG )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r TabDialoge
			if ( pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_BORDERWINDOW )
				pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r Border
            if ( (nParams & PARAM_BOOL_1) && bBool1 )
                pControl = pControl->GetWindow( WINDOW_OVERLAP );

			pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pControl->GetSizePixel().Height() );
			break;
		case M_SnapShot:
			{
				if ( pControl->GetType() == WINDOW_DOCKINGWINDOW && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_FLOATINGWINDOW )
					pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r FloatingWindows
				if ( pControl->GetType() == WINDOW_TABCONTROL && pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_TABDIALOG )
					pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r TabDialoge
				if ( pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_BORDERWINDOW )
					pControl = pControl->GET_REAL_PARENT();		// Sonderbehandlung f�r Border
                if ( (nParams & PARAM_BOOL_1) && bBool1 )
                    pControl = pControl->GetWindow( WINDOW_OVERLAP );

				Bitmap aBmp = pControl->SnapShot();
				if ( pControl->GetType() == WINDOW_WORKWINDOW )
				{
					Point aStart = pControl->GetPosPixel();
					if ( !(nParams & PARAM_USHORT_4) )
					{
						nParams |= PARAM_USHORT_1;
						nParams |= PARAM_USHORT_2;
						nParams |= PARAM_USHORT_3;
						nParams |= PARAM_USHORT_4;
						nNr1 = (USHORT)-aStart.X();
						nNr2 = (USHORT)-aStart.Y();
						nNr3 = (USHORT)pControl->GetSizePixel().Width() + 2*(USHORT)aStart.X();
						nNr4 = (USHORT)pControl->GetSizePixel().Height() + 2*(USHORT)aStart.Y();
					}
					nNr1 = std::max((USHORT)-aStart.X(),nNr1);
					nNr2 = std::max((USHORT)-aStart.Y(),nNr2);
					nNr3 = std::min((USHORT)(pControl->GetSizePixel().Width() + 2*(USHORT)aStart.X()),nNr3);
					nNr4 = std::min((USHORT)(pControl->GetSizePixel().Height() + 2*(USHORT)aStart.Y()),nNr4);
				}
				if( nParams & PARAM_USHORT_4 )
				{	// Zuschneiden
					Point aPt(-nNr1,-nNr2);
					Size aSz(nNr3,nNr4);
					VirtualDevice aVDev( *pControl );

					aVDev.SetOutputSizePixel( aSz );
					aVDev.DrawBitmap( aPt, aBmp );
					aBmp = aVDev.GetBitmap( Point(), aSz );
				}

				SvFileStream fOut;
				fOut.Open(aString1,STREAM_STD_WRITE);
				aBmp.Write(fOut);
				if ( fOut.GetError() )
					ReportError( aUId, GEN_RES_STR1( S_ERROR_SAVING_IMAGE, UniString::CreateFromInt32( fOut.GetError() ) ) );
				fOut.Close();
			}
			break;
		case M_GetFixedTextCount:
            {
    			pRet->GenReturn ( RET_Value, aUId, CountWinByRT( pControl, WINDOW_FIXEDTEXT, TRUE ) );
            }
			break;
		case M_GetFixedText:
            {
				if( ( nParams & PARAM_USHORT_1 ) == 0 )
                    nNr1 = 1;

                FixedText* pFixedText = (FixedText*)GetWinByRT( pControl, WINDOW_FIXEDTEXT, TRUE, nNr1-1 );
                if ( pFixedText )
    			    pRet->GenReturn ( RET_Value, aUId, pFixedText->GetText() );
                else
					ValueOK(aUId, MethodString( nMethodId ),nNr1,CountWinByRT( pControl, WINDOW_FIXEDTEXT, TRUE ) );
            }
			break;
		default:
			return FALSE;
		}
		SendProfile( UIdString( aUId ).Append('.').Append( MethodString( nMethodId ) ) );
		return TRUE;
	}
	return FALSE;
}

BOOL StatementControl::HandleCommonMethods( Window *pControl )
{
	switch( nMethodId )		// Diese k�nnen an jedem Window ausgef�hrt werden
	{
		case M_Exists:			// Oben schon Behandelt. Unterdr�ckt hier nur Fehler
		case M_NotExists:
		case M_IsEnabled:
		case M_IsVisible:
		case M_SnapShot:
			break;
		case M_Caption :
			{
				if ( pControl->GetText().Len() == 0 && IsDocFrame( pControl->GetWindow( WINDOW_FRAME ) ) )
                    pRet->GenReturn ( RET_Value, aUId, pControl->GetWindow( WINDOW_FRAME )->GetText());
                else
                    pRet->GenReturn ( RET_Value, aUId, pControl->GetText());
			}
			break;
		case M_GetRT:
			{
				pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pControl->GetType() );
			}
			break;
		case M_TypeKeys:
			{
				if( !(nParams & PARAM_USHORT_1) )	// Anzahl wiederholungen
					nNr1 = 1;
				if( !(nParams & PARAM_BOOL_1) )		// Follow Focus
					bBool1 = FALSE;		// so bleibt das bisherige Verhalten

				if ( !bBool1 )			// Altes Verhalten
					pControl->GrabFocus();
                else    // If focus is not inside given control we grab it once.
                {
                    Window *pFocus = GetpApp()->GetFocusWindow();
					if ( !pFocus || !pControl->IsWindowOrChild( pFocus, TRUE ) )
    					pControl->GrabFocus();
                }


                // maybe this can get removed since we are using GetPreferredKeyInputWindow()
				if ( pControl->GetType() == WINDOW_COMBOBOX )
				{	// Bei COMBOBOX an das Edit direkt liefern
					Window *pTemp = NULL;
					for ( USHORT i = 0 ; i < pControl->GetChildCount() && !pTemp ; i++ )
						if ( pControl->GetChild( i )->GetType() == WINDOW_EDIT )
							pTemp = pControl->GetChild( i );
					if ( pTemp )
						pControl = pTemp;
				}

				Window *pDeliverHere = pControl;
				for (int j = 0; j < nNr1; j++)
					for (xub_StrLen i = 0; i < aString1.Len(); i++)
					{
 						if ( StatementList::bUsePostEvents )
						{ // grab focus every time
							Window *pFocus = GetpApp()->GetFocusWindow();
							if ( !pFocus || !pControl->IsWindowOrChild( pFocus, TRUE ) )
    							pControl->GrabFocus();
						}
						if ( bBool1 )	// Jedesmal das FocusWindow finden
						{
							Window *pFocus = GetpApp()->GetFocusWindow();
							if ( pFocus && pControl->IsWindowOrChild( pFocus, TRUE ) )
								pDeliverHere = pFocus;
							else	// sonst fallback auf das Basisfenster
								pDeliverHere = pControl;
						}
                        pDeliverHere = pDeliverHere->GetPreferredKeyInputWindow();
						KeyEvent aEvent;
						if ( ((USHORT)aString1.GetChar(i)) <= 7 )
						{
							USHORT nVal = 0;
							switch (aString1.GetChar(i))
							{
								case 1: nVal = aString1.GetChar(i+1) + (aString1.GetChar(i+2) << 8);
										i += 2;
										break;
								case 3: nVal = (aString1.GetChar(i+1) << 8);
										i++;
										break;
								case 5: nVal = aString1.GetChar(i+1);
										i++;
										break;
								case 7: nVal = 0;
										break;
							}
                            // #105672#
                            // find out the keycode
                            USHORT nKeygroup = nVal & KEYGROUP_TYPE;
                            USHORT nKeyCode = nVal & KEY_CODE;
							sal_Unicode aCh;
							switch (nKeygroup)
							{
                                case KEYGROUP_NUM:
                                    aCh = nKeyCode - KEY_0 + '0';
                                    break;
                                case KEYGROUP_ALPHA:
                                    aCh = nKeyCode - KEY_A;
                                    if ( nVal & KEY_MOD1 )
                                    {}
                                    else if ( nVal & KEY_SHIFT )
                                        aCh += 'A';
                                    else
                                        aCh += 'a';
                                break;
                                case KEYGROUP_MISC:
									{							//  CR  ESC TAB BACK
                                        ByteString aPrintableMisc("\x0d\x1b\x09\x08 **+-*/.,<>=",16);
                                        if ( nKeyCode-KEY_RETURN < aPrintableMisc.Len()
                                            && nKeyCode != KEY_INSERT && nKeyCode != KEY_DELETE )
                                            aCh = aPrintableMisc.GetChar( nKeyCode-KEY_RETURN );
                                        else
                                            aCh = 0;
                                    }
                                    break;
                                case KEYGROUP_CURSOR:
                                case KEYGROUP_FKEYS:
                                default:
                                    aCh = 0;
                            }
							aEvent = KeyEvent(aCh,KeyCode(nVal & 0xFFF,nVal & 0xF000));
						}
						else
						{
															//   CR  ESC TAB BACK
							String aPrintableMisc = CUniString("\x0d\x1b\x09\x08 xx+-*/.,<>=");
							sal_Unicode aCh = aString1.GetChar(i);
							if ( aCh >= 'a' && aCh <= 'z' )
								aEvent = KeyEvent(aCh, KeyCode(KEYGROUP_ALPHA + aCh-'a', 0));
							else if ( aCh >= 'A' && aCh <= 'Z' )
								aEvent = KeyEvent(aCh, KeyCode(KEYGROUP_ALPHA + aCh-'a', KEY_SHIFT));
							else if ( aCh >= '0' && aCh <= '9' )
								aEvent = KeyEvent(aCh, KeyCode(KEYGROUP_NUM + aCh-'0', 0));
							else if ( aPrintableMisc.Search(aCh) != STRING_NOTFOUND )
								aEvent = KeyEvent(aCh, KeyCode(KEYGROUP_MISC + (USHORT)aPrintableMisc.Search(aCh), 0));
							else	// Sollte eigentlich nicht auftreten
								aEvent = KeyEvent(aCh, KeyCode());
						}
						ImplKeyInput( pDeliverHere, aEvent );
						if ( !MaybeDoTypeKeysDelay( pControl ) )
							break;
                        else
                            SafeReschedule();SafeReschedule();SafeReschedule();
					}
			}
			break;

#define CalcMouseButton\
	USHORT nButton = MOUSE_LEFT;\
	if ( (nParams & PARAM_USHORT_3) )\
	{\
		switch ( nNr3 )\
		{\
			case 1: nButton = MOUSE_LEFT; break;\
			case 2: nButton = MOUSE_MIDDLE; break;\
			case 3: nButton = MOUSE_RIGHT; break;\
		}\
	}\

		case M_MouseDown:
			{
				CalcMouseButton;
				Size aS = pControl->GetOutputSizePixel();
				Point aPos = Point(aS.Width() * nNr1 / 100,aS.Height() * nNr2 / 100);
				Window *pActualWin = pControl->FindWindow( aPos );
//					AnimateMouse( pControl, aPos );

				if ( pActualWin )
                    aPos = pActualWin->AbsoluteScreenToOutputPixel( pControl->OutputToAbsoluteScreenPixel ( aPos ) );
//					aPos = pActualWin->ScreenToOutputPixel( pControl->OutputToScreenPixel ( aPos ) );
				else
					pActualWin = pControl;

				AnimateMouse( pActualWin, aPos );
				pActualWin->GrabFocus();
				MouseEvent aMEvnt(aPos,1,MOUSE_SIMPLECLICK|MOUSE_SELECT,nButton);
				ImplMouseButtonDown( pActualWin, aMEvnt );
			}
			break;
		case M_MouseUp:
			{
				CalcMouseButton;
				Size aS = pControl->GetOutputSizePixel();
				Point aPos = Point(aS.Width() * nNr1 / 100,aS.Height() * nNr2 / 100);
				Window *pActualWin = pControl->FindWindow( aPos );

				if ( pActualWin )
                    aPos = pActualWin->AbsoluteScreenToOutputPixel( pControl->OutputToAbsoluteScreenPixel ( aPos ) );
//					aPos = pActualWin->ScreenToOutputPixel( pControl->OutputToScreenPixel ( aPos ) );
				else
					pActualWin = pControl;

				AnimateMouse( pActualWin, aPos );
//					pActualWin->GrabFocus();
				MouseEvent aMEvt( aPos, 1, MOUSE_SIMPLECLICK|MOUSE_SELECT, nButton );
				ImplMouseButtonUp( pActualWin, aMEvt );
			}
			break;
		case M_MouseMove:
			{
				CalcMouseButton;
				Size aS = pControl->GetOutputSizePixel();
				Point aPos = Point(aS.Width() * nNr1 / 100,aS.Height() * nNr2 / 100);
				Window *pActualWin = pControl->FindWindow( aPos );

				if ( pActualWin )
				{
                    aPos = pActualWin->AbsoluteScreenToOutputPixel( pControl->OutputToAbsoluteScreenPixel ( aPos ) );
//					aPos = pActualWin->ScreenToOutputPixel( pControl->OutputToScreenPixel ( aPos ) );
				}
				else
					pActualWin = pControl;

				AnimateMouse( pActualWin, aPos );
//					pActualWin->GrabFocus();
				MouseEvent aMEvt( aPos, 0, MOUSE_SIMPLEMOVE|MOUSE_DRAGMOVE, nButton );
				ImplMouseMove( pActualWin, aMEvt );
			}
			break;
		case M_MouseDoubleClick:
			{
				CalcMouseButton;
				Size aS = pControl->GetOutputSizePixel();
				Point aPos = Point(aS.Width() * nNr1 / 100,aS.Height() * nNr2 / 100);
				Window *pActualWin = pControl->FindWindow( aPos );

				if ( pActualWin )
				{
                    aPos = pActualWin->AbsoluteScreenToOutputPixel( pControl->OutputToAbsoluteScreenPixel ( aPos ) );
//					aPos = pActualWin->ScreenToOutputPixel( pControl->OutputToScreenPixel ( aPos ) );
				}
				else
					pActualWin = pControl;

				AnimateMouse( pActualWin, aPos );
				pActualWin->GrabFocus();
				MouseEvent aMEvnt;
				aMEvnt = MouseEvent(aPos,1,MOUSE_SIMPLECLICK|MOUSE_SELECT,nButton);
				ImplMouseButtonDown( pActualWin, aMEvnt );
				ImplMouseButtonUp  ( pActualWin, aMEvnt );
				aMEvnt = MouseEvent(aPos,2,MOUSE_SIMPLECLICK|MOUSE_SELECT,nButton);
				ImplMouseButtonDown( pActualWin, aMEvnt );
				ImplMouseButtonUp  ( pActualWin, aMEvnt );
			}
			break;
		case M_DisplayPercent:
			{
				ModelessDialog *pDlg = new ModelessDialog(NULL);
				pDlg->SetOutputSizePixel(Size(100,30));

				Edit *pMyEd = new Edit(pDlg,WB_CENTER | WB_BORDER );
				pMyEd->SetSizePixel(Size(100,30));
				pDlg->SetText(CUniString("Schlie�en"));
				pDlg->Show();
				pMyEd->Show();
				ULONG nTime = Time().GetTime();

				while (pDlg->IsVisible())
				{
					pDlg->ToTop();
					for (int i = 1 ; i<10 ; i++)
						SafeReschedule();
					Point Pos = pControl->GetPointerPosPixel();
					Size Siz=pControl->GetOutputSizePixel();
					if ( Time().GetTime() - nTime > 10 )
					{
						nTime = Time().GetTime();
						pMyEd->SetText(UniString::CreateFromInt32(Pos.X()*100/Siz.Width()).AppendAscii("%x").Append( UniString::CreateFromInt32(Pos.Y()*100/Siz.Height()) ).Append('%'));
					}
				}

				delete pMyEd;
				delete pDlg;
			}
			break;
		case M_OpenContextMenu:
			{
				aSubMenuId1 = SmartId();
				aSubMenuId2 = SmartId();
				aSubMenuId3 = SmartId();
                pMenuWindow = NULL;
				Point aPos;
                ToolBox* pTB = (ToolBox*)pControl;
                if ( (pControl->GetType() == WINDOW_TOOLBOX) && pTB->IsMenuEnabled() )
                {
                    pTB->ExecuteCustomMenu();
/*                    Rectangle aRect = pTB->GetMenubuttonRect();
					AnimateMouse( pControl, aRect.Center() );
					MouseEvent aMEvnt(aRect.Center(),1,MOUSE_SIMPLECLICK,MOUSE_LEFT);
					ImplMouseButtonDown( pTB, aMEvnt );*/
                }
                else
                {
				    BOOL bAtMousePos = ( nParams & PARAM_BOOL_1 ) && bBool1;
				    if ( bAtMousePos )
				    {
					    aPos = pControl->GetPointerPosPixel();
					    Window *pActualWin = pControl->FindWindow( aPos );

					    if ( pActualWin )
					    {
                            aPos = pActualWin->AbsoluteScreenToOutputPixel( pControl->OutputToAbsoluteScreenPixel ( aPos ) );
    //		    			aPos = pActualWin->ScreenToOutputPixel( pControl->OutputToScreenPixel ( aPos ) );
						    pControl = pActualWin;
					    }
				    }
				    CommandEvent aEvent( aPos, COMMAND_CONTEXTMENU, bAtMousePos );
				    ImplCommand( pControl, aEvent );
                }
			}
			break;
		case M_UseMenu:
			{
				aSubMenuId1 = SmartId();
				aSubMenuId2 = SmartId();
				aSubMenuId3 = SmartId();
                pMenuWindow = NULL;

                while ( pControl && !( ( pControl->GetType() == WINDOW_SYSWINDOW || pControl->GetType() == WINDOW_WORKWINDOW ) && ControlOK( pControl, "" ) ) )
                    pControl = pControl->GET_REAL_PARENT();

                if ( pControl && ((SystemWindow*)pControl)->GetMenuBar() )
                    pMenuWindow = ((SystemWindow*)pControl);
                else
					ReportError( GEN_RES_STR1( S_NO_MENU, MethodString( nMethodId ) ) );
			}
			break;
		case M_FadeIn:
		case M_FadeOut:
		case M_Pin:
		case M_IsFadeIn:
		case M_IsPin:
			{
				WindowAlign aWindowAlign = WINDOWALIGN_LEFT;
				if ( (nParams & PARAM_USHORT_1) )
				{
					switch ( nNr1 )
					{
						case CONST_ALIGN_LEFT:
							aWindowAlign = WINDOWALIGN_LEFT;
							break;
						case CONST_ALIGN_TOP:
							aWindowAlign = WINDOWALIGN_TOP;
							break;
						case CONST_ALIGN_RIGHT:
							aWindowAlign = WINDOWALIGN_RIGHT;
							break;
						case CONST_ALIGN_BOTTOM:
							aWindowAlign = WINDOWALIGN_BOTTOM;
							break;
						default:
							ReportError( aUId, GEN_RES_STR1( S_INVALID_POSITION, MethodString( nMethodId ) ) );
					}
				}

				Window* pTemp = NULL;
				while ( !pTemp && pControl )
				{
					pTemp = GetFadeSplitWin( pControl, aWindowAlign );
					pControl = pControl->GET_REAL_PARENT();
				}

				if ( !pTemp )
				{
					ReportError( aUId, GEN_RES_STR1( S_SPLITWIN_NOT_FOUND, MethodString( nMethodId ) ) );
					break;
				}

				pControl = pTemp;	// So da� wir unten ohne Fehler durchkommen
				SplitWindow *pSW = (SplitWindow*) pTemp;

//	Rectangle			GetAutoHideRect() const;
//	Rectangle			GetFadeInRect() const;
//	Rectangle			GetFadeOutRect() const;

				switch( nMethodId )
				{
					case M_FadeIn:
						if ( pSW->IsFadeInButtonVisible() )
							pSW->FadeIn();
						break;
					case M_FadeOut:
						if ( pSW->IsFadeOutButtonVisible() )
							pSW->FadeOut();
						break;
					case M_Pin:
						if (   ( pSW->GetAutoHideState() && bBool1 )
							|| ( !pSW->GetAutoHideState() && !bBool1 ) )
						{
							MouseEvent aMEvnt;
							Point aPt( pSW->GetAutoHideRect().Center() );
							aMEvnt = MouseEvent( aPt,1,MOUSE_SIMPLECLICK,MOUSE_LEFT );
							ImplMouseButtonDown( pControl, aMEvnt, FORCE_DIRECT_CALL );
							ImplMouseButtonUp  ( pControl, aMEvnt, FORCE_DIRECT_CALL );
						}
//								pSW->AutoHide();
						break;
					case M_IsFadeIn:
						pRet->GenReturn ( RET_Value, aUId, pSW->IsFadeOutButtonVisible() );
						break;
					case M_IsPin:
						pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)!pSW->GetAutoHideState() );
						break;
					default:
						ReportError( aUId, GEN_RES_STR1( S_INTERNAL_ERROR, MethodString( nMethodId ) ) );
				}
				SendProfile( UIdString( aUId ).Append('.').Append( MethodString( nMethodId ) ) );
			}
			break;
		case M_StatusGetText:
		case M_StatusIsProgress:
		case M_StatusGetItemCount:
		case M_StatusGetItemId:
			{
				StatusBar *pStatus = NULL;
				while ( !pStatus && pControl )
				{
					pStatus = (StatusBar*)GetWinByRT( pControl, WINDOW_STATUSBAR, TRUE );
					pControl = pControl->GET_REAL_PARENT();
				}

				if ( !pStatus )
				{
					ReportError( aUId, GEN_RES_STR1( S_NO_STATUSBAR, MethodString( nMethodId ) ) );
					break;
				}

				switch ( nMethodId )
				{
					case M_StatusGetText:
						{
							if ( (nParams & PARAM_USHORT_1) )
							{
								if ( pStatus->AreItemsVisible() )
									pRet->GenReturn ( RET_Value, aUId, String(pStatus->GetItemText(nNr1)));
								else
									ReportError( aUId, GEN_RES_STR1( S_ITEMS_INVISIBLE, MethodString( nMethodId ) ) );
							}
							else
							{
								if ( pStatus->AreItemsVisible() )
								{
									if ( pStatus->GetItemCount() == 1 )
									{
										pRet->GenReturn ( RET_Value, aUId, pStatus->GetItemText( pStatus->GetItemId(0) ));
									}
									else
									{
										pRet->GenReturn ( RET_Value, aUId, String() );
									}
								}
								else
									pRet->GenReturn ( RET_Value, aUId, (String)pStatus->GetText() );
							}
						}
						break;
					case M_StatusIsProgress:
						{
							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)pStatus->IsProgressMode() );
						}
						break;
					case M_StatusGetItemCount:
						if ( pStatus->AreItemsVisible() )
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(pStatus->GetItemCount()));
						else
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(0));
						break;
					case M_StatusGetItemId:
						if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,pStatus->GetItemCount()) )
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(pStatus->GetItemId(nNr1-1)));
						break;
				}
			}
			break;
		case M_HasScrollBar:
		case M_IsScrollBarEnabled:
			{
                if ( (nParams | PARAM_USHORT_1) != PARAM_USHORT_1 )     // so there are other params
                {
					ReportError( aUId, GEN_RES_STR0( S_INVALID_PARAMETERS ) );
                    break;
                }

                if( !(nParams & PARAM_USHORT_1) )
					nNr1 = CONST_ALIGN_RIGHT;		// default is right Scrollbar(vertical)

                if ( (nNr1 != CONST_ALIGN_RIGHT) && (nNr1 != CONST_ALIGN_BOTTOM) )
                {
					ReportError( aUId, GEN_RES_STR1( S_INVALID_POSITION, MethodString( nMethodId ) ) );
                    break;
                }

                ScrollBar *pScroll = NULL;

				USHORT nSteps = 2;
                while ( !pScroll && pControl && nSteps-- )
				{
					pScroll = GetScrollBar( pControl, nNr1, TRUE );
					pControl = pControl->GET_REAL_PARENT();
				}

				switch ( nMethodId )
				{
					case M_HasScrollBar:
						{
				            if ( pScroll )
    							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)TRUE );
                            else
    							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)FALSE );
						}
						break;
					case M_IsScrollBarEnabled:
						{
				            if ( !pScroll )
				            {
					            ReportError( aUId, GEN_RES_STR1( S_NO_SCROLLBAR, MethodString( nMethodId ) ) );
					            break;
				            }
   							pRet->GenReturn ( RET_Value, aUId, pScroll->IsEnabled() );
						}
						break;
                }
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}


BOOL StatementControl::Execute()
{
	Window *pControl;
	BOOL bStatementDone = TRUE;


	if ( IsError )
	{
        #if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "Skipping Window: " );
		m_pDbgWin->AddText( aUId.GetText() );
		m_pDbgWin->AddText( " Method: " );
		m_pDbgWin->AddText( String::CreateFromInt32( nMethodId ) );
		m_pDbgWin->AddText( "\n" );
		#endif
		Advance();
		delete this;
		return TRUE;
	}

	InitProfile();
#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "Executing Window: " );
	m_pDbgWin->AddText( aUId.GetText() );
	m_pDbgWin->AddText( " Method: " );
	m_pDbgWin->AddText( String::CreateFromInt32( nMethodId ) );
	m_pDbgWin->AddText( "\n" );
#endif


	if ( aUId.Matches( UID_ACTIVE ) )
		pControl = GetAnyActive();
	else
	{
		BOOL bSearchButtonOnToolbox = (nParams == PARAM_NONE) && ((M_Click == nMethodId) || (M_TearOff == nMethodId) || (M_IsEnabled == nMethodId) || (M_OpenMenu == nMethodId));
		bSearchButtonOnToolbox |= (nParams == PARAM_USHORT_1) && (M_GetState == nMethodId);
        if ( nMethodId == M_TypeKeys || nMethodId == M_MouseDown
			|| nMethodId == M_MouseUp || nMethodId ==  M_MouseMove
			|| nMethodId == M_SnapShot )
		{
			pControl = NULL;
			if ( /*(nMethodId == M_SnapShot || nMethodId == M_TypeKeys) &&*/ !pControl )
				pControl = SearchTree( aUId ,bSearchButtonOnToolbox );
		}
		else
			pControl = SearchTree( aUId ,bSearchButtonOnToolbox );
	}


	if ( pControl && pControl->GetType() == WINDOW_TOOLBOX )
	{
        if ( !aUId.Matches( pControl->GetSmartUniqueOrHelpId() ) )
        {   // Also wenn wir irgendwas auf einer Toolbox gefunden haben
		    switch ( nMethodId )
		    {
			    case M_Click:
			    case M_TearOff:
			    case M_OpenMenu:
			    case M_GetState:
                    break;
			    case M_IsEnabled:
				    nMethodId = _M_IsEnabled;	// Umlabeln, da die Behandlung essentiell anders ist!
				    break;
			    default:
				    pControl = NULL;
		    }
        }
	}


	switch ( nMethodId )
	{
		case M_Exists:
		case M_NotExists:
			Time aT;
			USHORT aSeconds = aT.GetMin()*60+aT.GetSec();
			if ( !bBool2 )			// wurde im Konstruktor auf FALSE gesetzt
			{
				bBool2 = TRUE;
				nNr2 = aSeconds;
				if( !(nParams & PARAM_USHORT_1) )
					nNr1 = 0;		// defaultm��ig sofort zur�ck
			}
			if ( aSeconds < nNr2 )			// Falls die Stunde umgesprungen ist
				aSeconds += 60*60;

			if ( /* !IsAccessable(pControl)#87019# */ !pControl || !pControl->IsVisible() )
				pControl = NULL;
			if ( ((nMethodId == M_Exists) && pControl) ||
				 ((nMethodId == M_NotExists) && !pControl) )
			{	// Wenn Bedingung erf�llt
				pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)TRUE );
			}
			else
				if ( aSeconds <= nNr2 + nNr1 )		// Zeit ist noch nicht abgelaufen
					return FALSE;
				else
					pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)FALSE );

			Advance();
			delete this;
			return TRUE;
//			break;
	}


	short nRT = 0;

	if( pControl )			// Das Fenster Existiert irgendwo, kann aber auch hidden sein!
	{
		nRT = ImpGetRType( pControl, aUId );   // If not Numeric pControl is used
#if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( "Type is: " );
		m_pDbgWin->AddText( String::CreateFromInt32( nRT ) );
		m_pDbgWin->AddText( "\n" );
#endif
	}

    if (   nRT == C_Window &&       // Search for WorkWindow to satisfy these commands
         ( nMethodId == M_Close
//		|| nMethodId == M_Size
//		|| nMethodId == M_Move
		|| nMethodId == M_IsMax
		|| nMethodId == M_IsMin
		|| nMethodId == M_IsRestore
		|| nMethodId == M_Minimize
		|| nMethodId == M_Maximize
		|| nMethodId == M_Restore ) )
    {
        Window* pNewControl = pControl;
        while ( pNewControl && pNewControl->GetType() != WINDOW_WORKWINDOW )
            pNewControl = pNewControl->GET_REAL_PARENT();

        if ( pNewControl )
        {
            pControl = pNewControl;
            nRT = C_WorkWin;
        }
    }


	if ( (!ControlOK( pControl, "" )) && ( nMethodId != M_SnapShot ) && (nRetryCount--))
	{
#if OSL_DEBUG_LEVEL > 1
		m_pDbgWin->AddText( CUniString("Reschedule command (").Append( UniString::CreateFromInt32(nRetryCount) ).AppendAscii(")\n") );
#endif
		return FALSE;
	}

	if( ControlOK( pControl, "" ) )
	{
		if ( nMethodId == M_OpenContextMenu && !bBool2 )
        {
			pControl->GrabFocus();  // to get asyncron focus on unix
            bBool2 = TRUE;
            return FALSE;
        }
        // TODO: handle GetFocus for all Methods and Windows like this (remove part below)
        //       See for impact of changed focus for HandleVisibleControls() (taking Snapshots might be different, possible exclude those methods)
		if (( (nRT == C_TreeListBox) && !bBool2 )
			&& nMethodId != M_TypeKeys			// TypeKeys macht das selbst, falls eigenes Focushandling gew�nscht
			&& nMethodId != M_MouseDown
			&& nMethodId != M_MouseUp
			&& nMethodId != M_MouseMove
			/*&& nMethodId != M_MouseDoubleClick*/ )
        {
            if ( !pControl->HasFocus() )
            {
			    pControl->GrabFocus();
                int i = 10;
                while ( i-- && !pControl->HasFocus() )    // reschedule a bit
                {
                    SafeReschedule();
                    if ( !WinPtrValid( pControl ) )
                        return FALSE;
                }
                if ( !pControl->HasFocus() )  // to get asyncronous focus
                {
                    bBool2 = TRUE;
                    return FALSE;
                }
            }
        }
    }

    Advance();

	if ( HandleVisibleControls( pControl ) )
	{
		delete this;
		return TRUE;
	}
	if( ControlOK( pControl, "Window/Control" ) )
	{
		if (((( nRT < C_TabPage && nRT > C_TabControl )
              || nRT == C_PatternBox
              || nRT == C_ToolBox
              || nRT == C_ValueSet
              || nRT == C_Control
              || nRT == C_TreeListBox
             )
			|| nMethodId == M_OpenContextMenu )
			&& nMethodId != M_TypeKeys			// TypeKeys macht das selbst, falls eigenes Focushandling gew�nscht
			&& nMethodId != M_MouseDown
			&& nMethodId != M_MouseUp
			&& nMethodId != M_MouseMove
			/*&& nMethodId != M_MouseDoubleClick*/ )
			pControl->GrabFocus();

/*  leads to problems because settext sets the text whereas typekeys adds to the text.
        if ( bDoTypeKeysDelay && nMethodId == M_SetText && ( nParams & PARAM_STR_1 ) )
		{	// Hier wird das Statement auf ein TypeKeys umgebogen
			nMethodId = M_TypeKeys;
			nParams = PARAM_BOOL_1 | PARAM_STR_1;
			bBool1 = TRUE;
			pControl->GrabFocus();
		}
*/
		if ( !HandleCommonMethods( pControl ) )
		{
			switch( nRT )
			{
				case C_TabControl:
					switch( nMethodId )
					{
						case M_GetPageId:
							if ( (nParams & PARAM_USHORT_1) )
							{
								if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((TabControl*)pControl)->GetPageCount() ) )
									pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)((TabControl*)pControl)->GetPageId(nNr1-1));
							}
							else
								pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)((TabControl*)pControl)->GetCurPageId());
							break;
						case M_GetPageCount:
							pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)((TabControl*)pControl)->GetPageCount());
							break;
						case M_SetPageId:
							if (((TabControl*)pControl)->GetCurPageId())
								((TabControl*)pControl)->DeactivatePage();
							((TabControl*)pControl)->SetCurPageId( nNr1 );
							((TabControl*)pControl)->ActivatePage();
							break;
						case M_SetPageNr:
							if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((TabControl*)pControl)->GetPageCount() ) )
							{
								if (((TabControl*)pControl)->GetCurPageId())
									((TabControl*)pControl)->DeactivatePage();
								((TabControl*)pControl)->SetCurPageId( ((TabControl*)pControl)->GetPageId( nNr1-1 ) );
								((TabControl*)pControl)->ActivatePage();
							}
							break;
						case M_GetPage:
							pRet->GenReturn ( RET_Value, aUId, static_cast<comm_ULONG>(((TabControl*)pControl)->GetTabPage(((TabControl*)pControl)->GetCurPageId())->GetSmartUniqueOrHelpId().GetNum())); //GetNum() ULONG != comm_ULONG on 64bit
							break;
						case M_SetPage :
							{		// Wegen lokaler Variablen
								TabControl *pTControl = ((TabControl*)pControl);
								USHORT nActive = pTControl->GetCurPageId();
								USHORT i,anz;
								ULONG nID = 0;
								i = pTControl->GetPagePos( pTControl->GetCurPageId() );
								for ( anz=0 ; anz < pTControl->GetPageCount() && nID != nLNr1 ; anz++ )
								{
									pTControl->SelectTabPage( pTControl->GetPageId(i) );
									/*if (pTControl->GetCurPageId())
										pTControl->DeactivatePage();
									pTControl->SetCurPageId( pTControl->GetPageId(i) );
									pTControl->ActivatePage();*/
									nID = pTControl->GetTabPage(pTControl->GetCurPageId())->GetSmartUniqueOrHelpId().GetNum();
									i++;
									if ( i >= pTControl->GetPageCount() )
										i = 0;
									if ( !MaybeDoTypeKeysDelay( pTControl ) || !MaybeDoTypeKeysDelay( pTControl ) || !MaybeDoTypeKeysDelay( pTControl ) )	// 3 Mal aufrufen
										break;
								}
								if ( nID != nLNr1 )
								{
									pTControl->SelectTabPage( nActive );
									/*if (pTControl->GetCurPageId())
										pTControl->DeactivatePage();
									pTControl->SetCurPageId( nActive );
									pTControl->ActivatePage();*/
									ReportError( SmartId( nLNr1 ), GEN_RES_STR1( S_TABPAGE_NOT_FOUND, MethodString( nMethodId ) ) );
								}
							}
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "TabControl" ) );
							break;
					}
					break;
				case C_RadioButton:
				case C_ImageRadioButton:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteLinks);
							break;
						case M_IsChecked :
							pRet->GenReturn ( RET_Value, aUId, ((RadioButton*)pControl)->IsChecked());
							break;
						case M_Check :
							((RadioButton*)pControl)->Check();
							((RadioButton*)pControl)->Click();
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "RadioButton" ) );
							break;
					}
					break;
				case C_CheckBox:
				case C_TriStateBox:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteLinks);
							break;
						case M_IsChecked :
							pRet->GenReturn ( RET_Value, aUId, comm_BOOL( ((TriStateBox*)pControl)->GetState() == STATE_CHECK) );
							break;
						case M_IsTristate :
							pRet->GenReturn ( RET_Value, aUId, comm_BOOL( ((TriStateBox*)pControl)->GetState() == STATE_DONTKNOW) );
							break;
						case M_GetState :
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((TriStateBox*)pControl)->GetState()));
							break;
						case M_Check :
							((TriStateBox*)pControl)->SetState( STATE_CHECK );
							((TriStateBox*)pControl)->Click();
							break;
						case M_UnCheck :
							((TriStateBox*)pControl)->SetState( STATE_NOCHECK );
							((TriStateBox*)pControl)->Click();
							break;
						case M_TriState :
							if ( ((TriStateBox*)pControl)->IsTriStateEnabled() )
							{
								((TriStateBox*)pControl)->SetState( STATE_DONTKNOW );
								((TriStateBox*)pControl)->Click();
							}
							else
							{
								ReportError( aUId, GEN_RES_STR0( S_TRISTATE_NOT_ALLOWED ) );
							}
							break;
						case M_Click :
							{
								TriStateBox *pTB = ((TriStateBox*)pControl);
								if ( pTB->GetState() == STATE_NOCHECK )
									pTB->SetState( STATE_CHECK );
								else if ( pTB->GetState() == STATE_CHECK )
								{
									if ( pTB->IsTriStateEnabled() )
										pTB->SetState( STATE_DONTKNOW );
									else
										pTB->SetState( STATE_NOCHECK );
								}
								else
									pTB->SetState( STATE_NOCHECK );
								pTB->Click();
								}
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "TriStateBox" ) );
							break;
					}
					break;
				case C_Edit:
				case C_MultiLineEdit:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, Mitte);
							break;
						case M_GetText :
							pRet->GenReturn ( RET_Value, aUId, ((Edit*)pControl)->GetText());
							break;
						case M_IsWritable:
							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL) !((Edit*)pControl)->IsReadOnly() );
							break;
						default:
							if ( ! ((Edit*)pControl)->IsReadOnly() )
							{
								switch( nMethodId )
								{
									case M_SetText :
										((Edit*)pControl)->SetText( aString1 );
                                        if ( nRT == C_MultiLineEdit )   // since SetModifyFlag is not virtual we have to do this
                                            ((MultiLineEdit*)pControl)->SetModifyFlag();
                                        else
                                            ((Edit*)pControl)->SetModifyFlag();
										((Edit*)pControl)->Modify();
										if ( ((Edit*)pControl)->GetText().CompareTo(aString1) != COMPARE_EQUAL )
											ReportError( aUId, GEN_RES_STR1( S_ERROR_IN_SET_TEXT, MethodString( nMethodId ) ) );
										break;
									default:
										ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "(MultiLine)Edit" ) );
										break;
								}
							}
							else
								ReportError( aUId, GEN_RES_STR1c( S_ATTEMPT_TO_WRITE_READONLY, "(MultiLine)Edit" ) );
					}
					break;
				case C_MultiListBox:
				case C_ListBox:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						case M_GetSelCount :
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((ListBox*)pControl)->GetSelectEntryCount()));
							break;
						case M_GetSelIndex :
							if ( ! (nParams & PARAM_USHORT_1) )
							{
								if ( ((ListBox*)pControl)->GetSelectEntryCount() == 0 )
								{
									pRet->GenReturn ( RET_Value, aUId, comm_ULONG(0));
									break;
								}
								nNr1 = 1;
							}
							ValueOK(aUId, MethodString( nMethodId ),nNr1,((ListBox*)pControl)->GetSelectEntryCount());
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((ListBox*)pControl)->GetSelectEntryPos(nNr1-1)) +1);
							break;
						case M_GetSelText :
							if ( ! (nParams & PARAM_USHORT_1) )
								nNr1 = 1;
							pRet->GenReturn ( RET_Value, aUId, ((ListBox*)pControl)->GetSelectEntry(nNr1-1));
							break;
						case M_GetItemCount :
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((ListBox*)pControl)->GetEntryCount()));
							break;
						case M_GetItemText :
							pRet->GenReturn ( RET_Value, aUId, ((ListBox*)pControl)->GetEntry(nNr1-1));
							break;
						case M_Select:
						case M_MultiSelect:
							{
								BOOL bUnselectBeforeSelect = ( nMethodId == M_Select );
								BOOL bFehler = FALSE;
								if ( ! (nParams & PARAM_BOOL_1) )
									bBool1 = TRUE;

								if ( nMethodId == M_MultiSelect && nRT == C_ListBox )
								{
									ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "ListBox" ) );
									bFehler = TRUE;
								}

								if ( !bBool1 && nMethodId == M_Select )
								{
									ReportError( aUId, GEN_RES_STR1( S_NO_SELECT_FALSE, MethodString( nMethodId ) ) );
									bFehler = TRUE;
								}

								if ( !bFehler )
								{
									if( nParams & PARAM_STR_1 )
									{
										ListBox *pLB = ((ListBox*)pControl);
										USHORT nPos;
										if ( (nPos = pLB->GetEntryPos( aString1 )) == LISTBOX_ENTRY_NOTFOUND )
											ReportError( aUId, GEN_RES_STR2( S_ENTRY_NOT_FOUND, MethodString( nMethodId ), aString1 ) );
										else
										{
											if ( bUnselectBeforeSelect )
												pLB->SetNoSelection();
											pLB->SelectEntryPos( nPos, bBool1 );
											if ( pLB->IsEntryPosSelected( nPos ) ? !bBool1 : bBool1 )	// XOR rein mit BOOL
												ReportError( aUId, GEN_RES_STR2( S_METHOD_FAILED, MethodString( nMethodId ), aString1 ) );
										}
									}
									else
									{
										ListBox *pLB = ((ListBox*)pControl);
										pLB = static_cast<ListBox*>(pControl);
										if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,pLB->GetEntryCount()) )
										{
											if ( bUnselectBeforeSelect )
												pLB->SetNoSelection();
											pLB->SelectEntryPos( nNr1-1, bBool1 );
											if ( pLB->IsEntryPosSelected( nNr1-1 ) ? !bBool1 : bBool1 )	// XOR rein mit BOOL
												ReportError( aUId, GEN_RES_STR2( S_METHOD_FAILED, MethodString( nMethodId ), UniString::CreateFromInt32( nNr1 ) ) );
										}
									}
									((ListBox*)pControl)->Select();
								}
							}
							break;
						case M_SetNoSelection :
							((ListBox*)pControl)->SetNoSelection();
							((ListBox*)pControl)->Select();
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "(Multi)ListBox" ) );
							break;
					}
					break;
				case C_ComboBox:
				case C_PatternBox:
				case C_NumericBox:
				case C_MetricBox:
				case C_CurrencyBox:
				case C_DateBox:
				case C_TimeBox:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						case M_GetSelText :
							pRet->GenReturn ( RET_Value, aUId, ((ComboBox*)pControl)->GetText());
							break;
						case M_GetSelIndex :
							{
								USHORT nPos = ((ComboBox*)pControl)->GetEntryPos(((ComboBox*)pControl)->GetText());
								if ( nPos == COMBOBOX_ENTRY_NOTFOUND )
									nPos = 0;
								else
									nPos++;
								pRet->GenReturn ( RET_Value, aUId, (comm_ULONG) nPos);
							}
							break;
						case M_GetItemCount :
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((ComboBox*)pControl)->GetEntryCount()));
							break;
						case M_GetItemText :
							pRet->GenReturn ( RET_Value, aUId, ((ComboBox*)pControl)->GetEntry(nNr1-1));
							break;
						case M_IsWritable:
							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL) !((ComboBox*)pControl)->IsReadOnly() );
							break;
						case M_Select :
							if( nParams & PARAM_USHORT_1 )
							{
								if ( !ValueOK(aUId, MethodString( nMethodId ),nNr1,((ComboBox*)pControl)->GetEntryCount()) )
									break;
								aString1 = ((ComboBox*)pControl)->GetEntry(nNr1-1);
							}
							else
							{
								if ( ((ComboBox*)pControl)->GetEntryPos( aString1 ) == COMBOBOX_ENTRY_NOTFOUND )
								{
									ReportError( aUId, GEN_RES_STR2( S_ENTRY_NOT_FOUND, MethodString( nMethodId ), aString1 ) );
									break;
								}
							}


							((ComboBox*)pControl)->SetText( aString1 );
							((ComboBox*)pControl)->SetModifyFlag();
							((ComboBox*)pControl)->Modify();
							break;
						case M_SetText :
							if ( ! ((ComboBox*)pControl)->IsReadOnly() )
							{
								if ( ! (nParams & PARAM_STR_1) )
									aString1 = String();
								((ComboBox*)pControl)->SetText( aString1 );
								((ComboBox*)pControl)->SetModifyFlag();
								((ComboBox*)pControl)->Modify();
							}
							else
								ReportError( aUId, GEN_RES_STR1c( S_ATTEMPT_TO_WRITE_READONLY, "ComboBox" ) );
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "ComboBox" ) );
							break;
					}
					break;
				case C_PushButton:
				case C_OkButton:
				case C_CancelButton:
				case C_ImageButton:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, Mitte);
							break;
						case M_Click :
							((PushButton*)pControl)->Click();
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "PushButton" ) );
							break;
					}
					break;
				case C_MoreButton:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, Mitte);
							break;
						case M_IsOpen :
							pRet->GenReturn ( RET_Value, aUId, ((MoreButton*)pControl)->GetState());
							break;
						case M_Click :
							((MoreButton*)pControl)->Click();
							break;
						case M_Open :
							((MoreButton*)pControl)->SetState(TRUE);
							break;
						case M_Close :
							((MoreButton*)pControl)->SetState(FALSE);
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "MoreButton" ) );
							break;
					}
					break;
				case C_SpinField:
				case C_PatternField:
				case C_NumericField:
				case C_MetricField:
				case C_CurrencyField:
				case C_DateField:
				case C_TimeField:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, Mitte);
							break;
						case M_GetText :
							pRet->GenReturn ( RET_Value, aUId, ((SpinField*)pControl)->GetText());
							break;
						case M_IsWritable:
							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL) !((SpinField*)pControl)->IsReadOnly() );
							break;
						case M_SetText :
							if ( ! ((SpinField*)pControl)->IsReadOnly() )
							{
								((SpinField*)pControl)->SetText( aString1 );
								((SpinField*)pControl)->SetModifyFlag();
								((SpinField*)pControl)->Modify();
							}
							else
								ReportError( aUId, GEN_RES_STR1c( S_ATTEMPT_TO_WRITE_READONLY, "SpinField" ) );
							break;
						case M_More :
							{
								if ( !(nParams & PARAM_USHORT_1) )
									nNr1 = 1;
								for (int i = 1; i<= nNr1; i++)
								{
									((SpinField*)pControl)->Up();
									((SpinField*)pControl)->SetModifyFlag();
									((SpinField*)pControl)->Modify();
								}
							}
							break;
						case M_Less :
							{
								if ( !(nParams & PARAM_USHORT_1) )
									nNr1 = 1;
								for (int i = 1; i<= nNr1; i++)
								{
									((SpinField*)pControl)->Down();
									((SpinField*)pControl)->SetModifyFlag();
									((SpinField*)pControl)->Modify();
								}
							}
							break;
						case M_ToMin :
							((SpinField*)pControl)->First();
							((SpinField*)pControl)->SetModifyFlag();
							((SpinField*)pControl)->Modify();
							break;
						case M_ToMax :
							((SpinField*)pControl)->Last();
							((SpinField*)pControl)->SetModifyFlag();
							((SpinField*)pControl)->Modify();
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "SpinField" ) );
							break;
					}
					break;

				case C_MenuButton:
						switch( nMethodId )
						{
						case M_AnimateMouse :
							AnimateMouse( pControl, Mitte);
							break;
						case M_Click :
							{
								MouseEvent aMEvnt;
								Point aPt( pControl->GetSizePixel().Width() / 2, pControl->GetSizePixel().Height() / 2 );
								aMEvnt = MouseEvent( aPt,1,MOUSE_SIMPLECLICK,MOUSE_LEFT );
								ImplMouseButtonDown( pControl, aMEvnt, FORCE_DIRECT_CALL );
								ImplMouseButtonUp  ( pControl, aMEvnt, FORCE_DIRECT_CALL );
							}
							break;
						case M_Open :
						case M_OpenMenu :
							{
								MouseEvent aMEvnt;
								Point aPt( pControl->GetSizePixel().Width() / 2, pControl->GetSizePixel().Height() / 2 );
								aMEvnt = MouseEvent( aPt,1,MOUSE_SIMPLECLICK,MOUSE_LEFT );
								ImplMouseButtonDown( pControl, aMEvnt, FORCE_DIRECT_CALL );

								ULONG nStart = Time::GetSystemTicks();
								ULONG nDelay = pControl->GetSettings().GetMouseSettings().GetActionDelay();
								while ( ( Time::GetSystemTicks() - nStart ) < nDelay + 100 )
									SafeReschedule();

								ImplMouseButtonUp  ( pControl, aMEvnt, FORCE_DIRECT_CALL );

                                aSubMenuId1 = SmartId();
				                aSubMenuId2 = SmartId();
				                aSubMenuId3 = SmartId();
                                pMenuWindow = NULL;
							}
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "MenuButton" ) );
							break;
					}
					break;
				case C_ToolBox:
					{
						ToolBox *pTB = ((ToolBox*)pControl);
						if ( !aUId.Matches( pTB->GetSmartUniqueOrHelpId() ) )	// Also Button auf der ToolBox gefunden
						{
							if ( (nParams == PARAM_NONE) || (nParams == PARAM_USHORT_1) )
							{			// Wir f�lschen einen Parameter
                                if ( aUId.HasNumeric() )
                                {
								    nParams = PARAM_ULONG_1;
								    nLNr1 = USHORT( aUId.GetNum() );
								    DBG_ASSERT( nLNr1 <= 0xFFFF, "ID on ToolBox > 0xFFFF" );
                                }
                                else
                                {
								    nParams = PARAM_STR_1;
                                    aString1 = aUId.GetStr();
                                }
							}
							else
								ReportError( aUId, GEN_RES_STR1( S_INTERNAL_ERROR, MethodString( nMethodId ) ) );
						}

#define FIND_ITEM\
    USHORT nItemPos = 0;\
    BOOL bItemFound = FALSE;\
	{\
		SmartId aButtonId;\
		if( nParams == PARAM_STR_1 )\
			aButtonId = SmartId( aString1 );\
		if( nParams == PARAM_ULONG_1 )\
			aButtonId = SmartId( nLNr1 );\
		for ( nItemPos = 0; nItemPos < pTB->GetItemCount() && !aButtonId.Matches(pTB->GetItemCommand(pTB->GetItemId(nItemPos))) &&\
		                                              !aButtonId.Matches(pTB->GetHelpId(pTB->GetItemId(nItemPos))) ; nItemPos++ ) {}\
		bItemFound = aButtonId.Matches(pTB->GetItemCommand(pTB->GetItemId(nItemPos))) || aButtonId.Matches(pTB->GetHelpId(pTB->GetItemId(nItemPos)));\
		if ( !bItemFound )\
			ReportError( aUId, GEN_RES_STR1( S_HELPID_ON_TOOLBOX_NOT_FOUND, MethodString( nMethodId ) ) );\
		else\
		{\
			if ( !pTB->IsItemEnabled( pTB->GetItemId(nItemPos) ) && nMethodId != _M_IsEnabled )\
			{\
				ReportError( aUId, GEN_RES_STR1( S_BUTTON_DISABLED_ON_TOOLBOX, MethodString( nMethodId ) ) );\
				bItemFound = FALSE;\
			}\
			else if ( !pTB->IsItemVisible( pTB->GetItemId(nItemPos) ) )\
			{\
				ReportError( aUId, GEN_RES_STR1( S_BUTTON_HIDDEN_ON_TOOLBOX, MethodString( nMethodId ) ) );\
				bItemFound = FALSE;\
			}\
			else\
			{\
                if ( pTB->IsMenuEnabled() )\
                {   /* button is in Menu */\
                }\
                else\
                {   /* Try the multi line way */\
				    if ( pTB->GetItemRect(pTB->GetItemId(nItemPos)).IsEmpty() )\
				    {\
					    USHORT nLine = pTB->GetCurLine();\
					    do\
					    {\
						    pTB->ShowLine( FALSE );\
						    for ( int i = 1 ; i < 30 ; i++ )\
							    SafeReschedule();\
					    }\
					    while ( pTB->GetCurLine() != nLine && pTB->GetItemRect(pTB->GetItemId(nItemPos)).IsEmpty() );\
					    pTB->Invalidate( pTB->GetScrollRect() );\
				    }\
				    if ( pTB->GetItemRect(pTB->GetItemId(nItemPos)).IsEmpty() )\
				    {\
					    ReportError( aUId, GEN_RES_STR1( S_CANNOT_MAKE_BUTTON_VISIBLE_IN_TOOLBOX, MethodString( nMethodId ) ) );\
					    bItemFound = FALSE;\
				    }\
                }\
			}\
		}\
	}\

						switch( nMethodId )
						{
							case M_AnimateMouse :
								AnimateMouse( pControl, MitteLinks);
								break;
							case M_Click :
								{
                                    FIND_ITEM;
									if ( bItemFound )	// FIND_ITEM Erfolgreich
									{
										Rectangle aRect = pTB->GetItemRect(pTB->GetItemId(nItemPos));
                                        if ( aRect.IsEmpty() )
                                        {
                                            pTB->ExecuteCustomMenu();
/*					                        aRect = pTB->GetMenubuttonRect();
					                        MouseEvent aMEvnt(aRect.Center(),1,MOUSE_SIMPLECLICK,MOUSE_LEFT);
					                        ImplMouseButtonDown( pTB, aMEvnt );*/

				                            aSubMenuId1 = SmartId();
				                            aSubMenuId2 = SmartId();
				                            aSubMenuId3 = SmartId();
                                            pMenuWindow = NULL;

                                            new StatementCommand( this, RC_MenuSelect, PARAM_USHORT_1, pTB->GetItemId(nItemPos) + TOOLBOX_MENUITEM_START );
                                        }
                                        else
                                        {
										    aRect = pTB->GetItemRect(pTB->GetItemId(nItemPos));
										    MouseEvent aMEvnt;
										    aMEvnt = MouseEvent(aRect.Center(),1,MOUSE_SIMPLECLICK,MOUSE_LEFT);
										    ImplMouseButtonDown( pTB, aMEvnt, FORCE_DIRECT_CALL );
										    ImplMouseButtonUp  ( pTB, aMEvnt, FORCE_DIRECT_CALL );
                                        }
									}
								}
								break;
							case M_TearOff :
								{
									FIND_ITEM;
									if ( bItemFound )	// FIND_ITEM Erfolgreich
									{
										Rectangle aRect = pTB->GetItemPosDropDownRect( nItemPos );
										AnimateMouse( pControl, aRect.Center() );
										MouseEvent aMEvnt(aRect.Center(),1,MOUSE_SIMPLECLICK,MOUSE_LEFT);
										ImplMouseButtonDown( pTB, aMEvnt, FORCE_DIRECT_CALL );

										Window *pWin = NULL;
										// Wait for the window to open.
										StatementList::bExecuting = TRUE;		// Bah ist das ein ekliger Hack
										{											// Das verhindert, da� schon der n�chste Befehl ausgef�hrt wird.
											Time aDelay;
											while ( !pWin && ( (pWin = GetPopupFloatingWin()) == NULL ) && ( Time() - aDelay ).GetSec() < 15 )
												SafeReschedule();
										}
										StatementList::bExecuting = FALSE;	// Bah ist das ein ekliger Hack

										if ( pWin && pWin->GetType() == WINDOW_FLOATINGWINDOW )
										{
											aMEvnt = MouseEvent(aRect.Center(),1,MOUSE_SIMPLECLICK,MOUSE_LEFT);
											ImplMouseButtonUp( pTB, aMEvnt, FORCE_DIRECT_CALL );
											((FloatingWindow*)pWin)->EndPopupMode( FLOATWIN_POPUPMODEEND_TEAROFF );
										}
										else
										{
											aMEvnt = MouseEvent(Point(1,-10), 1, MOUSE_SIMPLECLICK,MOUSE_LEFT);
											ImplMouseButtonUp( pTB, aMEvnt, FORCE_DIRECT_CALL );
											ReportError( aUId, GEN_RES_STR1( S_TEAROFF_FAILED, MethodString( nMethodId ) ) );
										}
									}
								}
								break;
							case M_OpenMenu :
								{
									FIND_ITEM;
									if ( bItemFound )	// FIND_ITEM Erfolgreich
									{
										Rectangle aRect = pTB->GetItemPosDropDownRect( nItemPos );
										AnimateMouse( pControl, aRect.Center() );
										MouseEvent aMEvnt(aRect.Center(),1,MOUSE_SIMPLECLICK,MOUSE_LEFT);
										ImplMouseButtonDown( pTB, aMEvnt);
										ImplMouseButtonUp( pTB, aMEvnt);

                                        // Das Fenster ist offen.
				                        aSubMenuId1 = SmartId();
				                        aSubMenuId2 = SmartId();
				                        aSubMenuId3 = SmartId();
                                        pMenuWindow = NULL;
									}
								}
								break;
							case _M_IsEnabled:
								{
									FIND_ITEM;
									if ( bItemFound )	// FIND_ITEM Erfolgreich
									{
										pRet->GenReturn ( RET_Value, aUId, pTB->IsItemEnabled( pTB->GetItemId(nItemPos) ) );
									}
								}
								break;
							case M_GetState :
								{
									FIND_ITEM;
									if ( bItemFound )	// FIND_ITEM Erfolgreich
									{
								        switch (nNr1)
								        {
								        case 0:
									        pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pTB->GetHelpId(pTB->GetItemId(nItemPos)));
									        break;
								        case 1:
									        pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pTB->GetItemType(nItemPos));
									        break;
								        case 2:
									        pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pTB->GetItemState(pTB->GetItemId(nItemPos)));
									        break;
								        case 3:
									        pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pTB->GetItemId(nItemPos));
									        break;
								        default:
									        pRet->GenReturn ( RET_Value, aUId, comm_ULONG(0));
									        break;
								        }
									}
								}
								break;
							case M_GetItemText :
								pRet->GenReturn ( RET_Value, aUId, (String)pTB->GetItemText(nNr1));
								break;
							case M_GetText :
								pRet->GenReturn ( RET_Value, aUId, (String)pTB->GetText());
								break;
							case M_GetItemCount :
								pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pTB->GetItemCount());
								break;
							case M_SetNextToolBox :
								if ( (nParams & PARAM_STR_1) )
									pTB->SetNextToolBox( aString1 );
								else
									pTB->SetNextToolBox( pTB->GetNextToolBox() );
								pTB->NextToolBox();
								break;
							case M_GetNextToolBox :
								pRet->GenReturn ( RET_Value, aUId, (String)pTB->GetNextToolBox());
								break;
						    case M_Dock :
						    case M_Undock :
						    case M_IsDocked :
						    case M_Close:
						    case M_Size:
						    case M_Move:
						    case M_IsMax:
						    case M_Minimize:
						    case M_Maximize:
						    case M_Help:		// Alles was unten weiterbehandelt werden soll
                                goto DockingWin;
							default:
								ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "ToolBox" ) );
								break;
						}
					}
					break;

				case C_TreeListBox:
					switch( nMethodId )
					{



#define GET_NTH_ENTRY_LBOX( First, Next, Anzahl)	\
	SvLBoxEntry *pThisEntry = ((SvTreeListBox*)pControl)->First(); \
	{ \
		int niTemp = Anzahl; \
		while ( niTemp-- ) \
		{ \
			pThisEntry = ((SvTreeListBox*)pControl)->Next( pThisEntry ); \
		} \
	}

						case M_GetText :               // Get the first text of the given (default=1) line
							{                          // should get removed some time
								SvTreeListBox *pTree = (SvTreeListBox*)pControl;
								SvLBoxEntry *pThisEntry = pTree->GetCurEntry();
								if ( ! (nParams & PARAM_USHORT_1) )
									nNr1 = 1;
								if ( pThisEntry )
                                {
                                    SvLBoxString* pItem = NULL;
                                    USHORT nValidTextItemCount = 0;
                                    {
                                        USHORT nIndex = 0;
                                        SvLBoxItem *pMyItem;
                                        while ( ( nValidTextItemCount < nNr1 ) && nIndex < pThisEntry->ItemCount() )
                                        {
                                            pMyItem = pThisEntry->GetItem( nIndex );
                                            if ( pMyItem->IsA() == SV_ITEM_ID_LBOXSTRING )
                                            {
                                                pItem = (SvLBoxString*)pMyItem;
                                                nValidTextItemCount++;
                                            }
                                            nIndex++;
                                        }
                                    }
									if ( ValueOK( aUId, CUniString("GetText"), nNr1, nValidTextItemCount ) )
                                        pRet->GenReturn ( RET_Value, aUId, pItem->GetText() );
                                }
								else
									ReportError( aUId, GEN_RES_STR2c2( S_NO_SELECTED_ENTRY, MethodString( nMethodId ), "TreeListBox" ) );
							}
							break;
						case M_GetSelCount :
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((SvLBox*)pControl)->GetSelectionCount()));
							break;
						case M_GetItemCount :
							pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((SvLBox*)pControl)->GetVisibleCount()) );
							break;
						case M_GetSelIndex :
							if ( ! (nParams & PARAM_USHORT_1) )
								nNr1 = 1;
							if ( ValueOK(aUId, CUniString("GetSelIndex"),nNr1,((SvLBox*)pControl)->GetSelectionCount()) )
							{
								nNr1--;
								GET_NTH_ENTRY_LBOX( FirstSelected, NextSelected, nNr1);
								pRet->GenReturn ( RET_Value, aUId, comm_ULONG( ((SvTreeListBox*)pControl)->GetVisiblePos( pThisEntry )) +1 );
							}
							break;
						case M_Select :
							if ( ! (nParams & PARAM_BOOL_1) )
								bBool1 = TRUE;
							if( nParams & PARAM_STR_1 )
							{
/*									ListBox *pLB = ((ListBox*)pControl);
								if ( pLB->GetEntryPos( aString1 ) == LISTBOX_ENTRY_NOTFOUND )
									ReportError( aUId, GEN_RES_STR2( S_ENTRY_NOT_FOUND, MethodString( nMethodId ), aString1 ) );
								else
								{
									pLB->SelectEntry( aString1, bBool1 );
									if ( pLB->IsEntrySelected( aString1 ) ? !bBool1 : bBool1 )	// XOR rein mit BOOL
										ReportError( aUId, GEN_RES_STR2( S_METHOD_FAILED, MethodString( nMethodId ), aString1 ) );
								}
*/									ReportError( aUId, GEN_RES_STR1( S_SELECT_DESELECT_VIA_STRING_NOT_IMPLEMENTED, MethodString( nMethodId ) ) );
							}
							else
							{
								if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((SvLBox*)pControl)->GetVisibleCount()) )
								{
									SvLBoxEntry *pEntry = (SvLBoxEntry*)((SvTreeListBox*)pControl)->GetEntryAtVisPos( nNr1-1 );
									((SvTreeListBox*)pControl)->Select ( pEntry, bBool1 );
								}
							}
							break;
						case M_GetSelText :
							if ( ! (nParams & PARAM_USHORT_1) )
								nNr1 = 1;
							if ( ! (nParams & PARAM_USHORT_2) )
								nNr2 = 1;
							if ( ValueOK(aUId, CUniString("GetSelText"),nNr1,((SvLBox*)pControl)->GetSelectionCount()) )
							{
								nNr1--;
								GET_NTH_ENTRY_LBOX( FirstSelected, NextSelected, nNr1);
                                if ( ValueOK( aUId, MethodString( nMethodId ),nNr2,pThisEntry->ItemCount() ) )
                                {
                                    SvLBoxString* pItem = NULL;
							        if ( ! (nParams & PARAM_USHORT_2) )
								        pItem = (SvLBoxString*)pThisEntry->GetFirstItem( SV_ITEM_ID_LBOXSTRING );
                                    else
                                    {
                                        SvLBoxItem *pMyItem = pThisEntry->GetItem( nNr2-1 );
                                        if ( pMyItem->IsA() == SV_ITEM_ID_LBOXSTRING )
                                            pItem = (SvLBoxString*)pMyItem;
                                    }

                                    if ( pItem )
                                        pRet->GenReturn ( RET_Value, aUId, pItem->GetText() );
                                    else
                                        ReportError( aUId, GEN_RES_STR1( S_NO_LIST_BOX_STRING, MethodString( nMethodId ) ) );
                                }
							}
							break;
						case M_GetItemText :
							if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((SvLBox*)pControl)->GetVisibleCount()) )
							{
								SvLBoxEntry *pThisEntry = (SvLBoxEntry*)((SvTreeListBox*)pControl)->GetEntryAtVisPos( nNr1-1 );
								if ( ! (nParams & PARAM_USHORT_2) )
									nNr2 = 1;
                                if ( ValueOK( aUId, MethodString( nMethodId ),nNr2,pThisEntry->ItemCount() ) )
                                {
                                    SvLBoxString* pItem = NULL;
							        if ( ! (nParams & PARAM_USHORT_2) )
								        pItem = (SvLBoxString*)pThisEntry->GetFirstItem( SV_ITEM_ID_LBOXSTRING );
                                    else
                                    {
                                        SvLBoxItem *pMyItem = pThisEntry->GetItem( nNr2-1 );
                                        if ( pMyItem->IsA() == SV_ITEM_ID_LBOXSTRING )
                                            pItem = (SvLBoxString*)pMyItem;
                                    }

        						    if ( pItem )
                                        pRet->GenReturn ( RET_Value, aUId, pItem->GetText() );
                                    else
                                        ReportError( aUId, GEN_RES_STR1( S_NO_LIST_BOX_STRING, MethodString( nMethodId ) ) );
                                }
							}
							break;
						case M_IsChecked :
						case M_IsTristate :
						case M_GetState :
						case M_Check :
						case M_UnCheck :
						case M_TriState :
							{
								SvTreeListBox *pTree = (SvTreeListBox*)pControl;
                                SvLBoxEntry *pThisEntry = NULL;

							    if ( ! (nParams & PARAM_USHORT_1) )
                                {
                                    pThisEntry = pTree->GetCurEntry();
                                    if ( !pThisEntry )
									    ReportError( aUId, GEN_RES_STR2c2( S_NO_SELECTED_ENTRY, MethodString( nMethodId ), "TreeListBox" ) );
                                }
                                else
                                {
							        if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((SvLBox*)pControl)->GetVisibleCount()) )
							        {
                                        pThisEntry = (SvLBoxEntry*)pTree->GetEntryAtVisPos( nNr1-1 );
                                    }
                                }

							    if ( ! (nParams & PARAM_USHORT_2) )
									nNr2 = 1;

							    if ( pThisEntry )
							    {
                                    if ( ValueOK( aUId, MethodString( nMethodId ),nNr2,pThisEntry->ItemCount() ) )
                                    {
                                        SvLBoxButton* pItem = NULL;
							            if ( ! (nParams & PARAM_USHORT_2) )
								            pItem = (SvLBoxButton*)pThisEntry->GetFirstItem( SV_ITEM_ID_LBOXBUTTON );
                                        else
                                        {
                                            SvLBoxItem *pMyItem = pThisEntry->GetItem( nNr2-1 );
                                            if ( pMyItem->IsA() == SV_ITEM_ID_LBOXBUTTON )
                                                pItem = (SvLBoxButton*)pMyItem;
                                        }

                                        if ( pItem )
                                        {
										    switch( nMethodId )
										    {
											    case M_IsChecked :
												    pRet->GenReturn ( RET_Value, aUId, comm_BOOL( pItem->IsStateChecked() ) );
												    break;
											    case M_IsTristate :
												    pRet->GenReturn ( RET_Value, aUId, comm_BOOL( pItem->IsStateTristate() ) );
												    break;
											    case M_GetState :
												    pRet->GenReturn ( RET_Value, aUId, comm_ULONG( pItem->GetButtonFlags() & ~SV_STATE_MASK ));
												    break;
											    case M_Check :
                                                    pItem->SetStateChecked();
                                                    pTree->CheckButtonHdl();
                                                    pTree->InvalidateEntry( pThisEntry );
												    break;
											    case M_UnCheck :
                                                    pItem->SetStateUnchecked();
                                                    pTree->CheckButtonHdl();
                                                    pTree->InvalidateEntry( pThisEntry );
												    break;
											    case M_TriState :
                                                    pItem->SetStateTristate();
                                                    pTree->CheckButtonHdl();
                                                    pTree->InvalidateEntry( pThisEntry );
												    break;
											    default:
												    ReportError( aUId, GEN_RES_STR1( S_INTERNAL_ERROR, MethodString( nMethodId ) ) );
												    break;
										    }
                                        }
									    else
										    ReportError( aUId, GEN_RES_STR1( S_NO_LIST_BOX_BUTTON, MethodString( nMethodId ) ) );
                                    }
                                }
							}
							break;
						case M_GetItemType :
                            {
								SvTreeListBox *pTree = (SvTreeListBox*)pControl;
                                SvLBoxEntry *pThisEntry = NULL;

							    if ( ! (nParams & PARAM_USHORT_1) )
                                {
                                    pThisEntry = pTree->GetCurEntry();
                                    if ( !pThisEntry )
									    ReportError( aUId, GEN_RES_STR2c2( S_NO_SELECTED_ENTRY, MethodString( nMethodId ), "TreeListBox" ) );
                                }
                                else
                                {
							        if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((SvLBox*)pControl)->GetVisibleCount()) )
							        {
                                        pThisEntry = (SvLBoxEntry*)pTree->GetEntryAtVisPos( nNr1-1 );
                                    }
                                }
                            
							    if ( pThisEntry )
							    {
								    if ( ! (nParams & PARAM_USHORT_2) )
									    nNr2 = 1;
                                    if ( ValueOK( aUId, MethodString( nMethodId ),nNr2,pThisEntry->ItemCount() ) )
                                    {
                                        SvLBoxItem *pMyItem = pThisEntry->GetItem( nNr2-1 );
                                        comm_USHORT nType;
                                        switch ( pMyItem->IsA() )
                                        {
                                            case SV_ITEM_ID_LBOXSTRING: nType = CONST_ItemTypeText ; break;
                                            case SV_ITEM_ID_LBOXBMP: nType = CONST_ItemTypeBMP ; break;
                                            case SV_ITEM_ID_LBOXBUTTON: nType = CONST_ItemTypeCheckbox ; break;
                                            case SV_ITEM_ID_LBOXCONTEXTBMP: nType = CONST_ItemTypeContextBMP ; break;
                                            default: nType = CONST_ItemTypeUnknown;
                                        }
                                        pRet->GenReturn ( RET_Value, aUId, nType );
                                    }
							    }
                            }
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "TreeListBox" ) );
							break;
					}
					break;
			    case C_Control:
                {
                    USHORT nRealControlType = 0;
                    if ( dynamic_cast< EditBrowseBox* >(pControl) )
                        nRealControlType = CONST_CTBrowseBox;
                    else if ( dynamic_cast< ValueSet* >(pControl) )
                        nRealControlType = CONST_CTValueSet;
                    else if ( dynamic_cast< ORoadmap* >(pControl) )
                        nRealControlType = CONST_CTORoadmap;
                    else if ( dynamic_cast< IExtensionListBox* >(pControl) )
                        nRealControlType = CONST_CTIExtensionListBox;
                    else
                        nRealControlType = CONST_CTUnknown;

					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						default:
							switch( nRealControlType )
							{
								case CONST_CTBrowseBox:
                                    {
                                        EditBrowseBox* pEBBox = dynamic_cast< EditBrowseBox* >(pControl);
                                        switch( nMethodId )
									    {


    /*


    BOOL			MakeFieldVisible( long nRow, USHORT nColId, BOOL bComplete = FALSE );
    // access to dynamic values of cursor row
    String          GetColumnTitle( USHORT nColumnId ) const;
    USHORT          GetColumnId( USHORT nPos ) const;
    USHORT          GetColumnPos( USHORT nColumnId ) const;
    // access and movement of cursor
    long            GetCurRow() const { return nCurRow; }
    USHORT          GetCurColumnId() const { return nCurColId; }
    BOOL            GoToRow( long nRow );
    BOOL			GoToRowAndDoNotModifySelection( long nRow );
    BOOL            GoToColumnId( USHORT nColId );
    BOOL            GoToRowColumnId( long nRow, USHORT nColId );
    // selections
    void            SetNoSelection();
    void            SelectAll();
    void            SelectRow( long nRow, BOOL bSelect = TRUE, BOOL bExpand = TRUE );
    void            SelectColumnPos( USHORT nCol, BOOL bSelect = TRUE )
					    { SelectColumnPos( nCol, bSelect, TRUE); }
    void            SelectColumnId( USHORT nColId, BOOL bSelect = TRUE )
					    { SelectColumnPos( GetColumnPos(nColId), bSelect, TRUE); }
    long            GetSelectRowCount() const;
    USHORT          GetSelectColumnCount() const;
    BOOL            IsRowSelected( long nRow ) const;
    BOOL            IsColumnSelected( USHORT nColumnId ) const;
    long            FirstSelectedRow( BOOL bInverse = FALSE );
    long            LastSelectedRow( BOOL bInverse = FALSE );
    long            PrevSelectedRow();
    long            NextSelectedRow();
    const MultiSelection* GetSelection() const
				    { return bMultiSelection ? uRow.pSel : 0; }
    void			SetSelection( const MultiSelection &rSelection );

    virtual String  GetCellText(long _nRow, USHORT _nColId) const;
    USHORT GetColumnCount() const { return ColCount(); }
protected:   
    virtual long    GetRowCount() const;


    EditBrowseBox

		    sal_Bool IsEditing() const {return aController.Is();}
		    void InvalidateStatusCell(long nRow) {RowModified(nRow, 0);}
		    void InvalidateHandleColumn();

		    CellControllerRef Controller() const { return aController; }
		    sal_Int32	GetBrowserFlags() const { return m_nBrowserFlags; }

		    virtual void ActivateCell(long nRow, sal_uInt16	nCol, sal_Bool bSetCellFocus = sal_True);
		    virtual void DeactivateCell(sal_Bool bUpdate = sal_True);



    */
										    case M_GetSelText :
											    {
                                                    pRet->GenReturn ( RET_Value, aUId, pEBBox->GetCellText( pEBBox->GetCurrRow(), pEBBox->GetColumnId( pEBBox->GetCurrColumn() )));
											    }
											    break;
										    case M_GetColumnCount :
											    {
                                                    USHORT nColCount = pEBBox->GetColumnCount();
                                                    comm_USHORT nUnfrozenColCount = 0;
                                                    USHORT i;
                                                    for ( i=0 ; i < nColCount ; i++ )
                                                    {
                                                        if ( !pEBBox->IsFrozen( pEBBox->GetColumnId( i ) ) )
                                                            nUnfrozenColCount++;
                                                    }
                                                    pRet->GenReturn ( RET_Value, aUId, nUnfrozenColCount );
											    }
											    break;
										    case M_GetRowCount :
											    {
                                                    pRet->GenReturn ( RET_Value, aUId, (comm_ULONG)pEBBox->GetRowCount() );
											    }
											    break;
										    case M_IsEditing :
											    {
                                                    CellControllerRef aControler;
                                                    aControler = pEBBox->Controller();
    											    pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)aControler.Is() );
											    }
											    break;
										    case M_Select :
											    {
                                                    if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,pEBBox->GetRowCount() ) )
                                                    {
                                                        USHORT nColCount = pEBBox->GetColumnCount();
                                                        comm_USHORT nUnfrozenColCount = 0;
                                                        USHORT i;
                                                        for ( i=0 ; i < nColCount ; i++ )
                                                        {
                                                            if ( !pEBBox->IsFrozen( pEBBox->GetColumnId( i ) ) )
                                                                nUnfrozenColCount++;
                                                        }
                                                        if ( ValueOK(aUId, MethodString( nMethodId ),nNr2,nUnfrozenColCount ) )
                                                            pEBBox->GoToRowColumnId( nNr1-1, pEBBox->GetColumnId( nNr2 ) );
                                                    }
											    }
											    break;
                                                
                                                
                                                
                                                /*
										    case M_GetSelCount :
											    pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((SvLBox*)pControl)->GetSelectionCount()));
											    break;
										    case M_GetSelIndex :
											    if ( ! (nParams & PARAM_USHORT_1) )
												    nNr1 = 1;
											    if ( ValueOK(aUId, CUniString("GetSelIndex"),nNr1,((SvLBox*)pControl)->GetSelectionCount()) )
											    {
												    nNr1--;
												    COUNT_LBOX( FirstSelected, NextSelected, nNr1);
												    pRet->GenReturn ( RET_Value, aUId, comm_ULONG( ((SvTreeListBox*)pControl)->GetVisiblePos( pThisEntry )) +1 );
											    }
											    break;
										    case M_GetSelText :
											    if ( ! (nParams & PARAM_USHORT_1) )
												    nNr1 = 1;
											    if ( ValueOK(aUId, CUniString("GetSelText"),nNr1,((SvLBox*)pControl)->GetSelectionCount()) )
											    {
												    nNr1--;
												    COUNT_LBOX( FirstSelected, NextSelected, nNr1);
                                                    GetFirstValidTextItem( pThisEntry );
                                                    pRet->GenReturn ( RET_Value, aUId, pItem->GetText() );
											    }
											    break;
										    case M_GetItemCount :
											    pRet->GenReturn ( RET_Value, aUId, comm_ULONG(((SvLBox*)pControl)->GetVisibleCount()) );
											    break;
										    case M_GetItemText :
											    if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((SvLBox*)pControl)->GetVisibleCount()) )
											    {
												    SvLBoxEntry *pEntry = (SvLBoxEntry*)((SvTreeListBox*)pControl)->GetEntryAtVisPos( nNr1-1 );
                                                    GetFirstValidTextItem( pEntry );
    											    pRet->GenReturn ( RET_Value, aUId, pItem->GetText() );
											    }
											    break;
										    case M_Select :
											    if ( ! (nParams & PARAM_BOOL_1) )
												    bBool1 = TRUE;
											    if( nParams & PARAM_STR_1 )
											    {
			    / *									ListBox *pLB = ((ListBox*)pControl);
												    if ( pLB->GetEntryPos( aString1 ) == LISTBOX_ENTRY_NOTFOUND )
													    ReportError( aUId, GEN_RES_STR2( S_ENTRY_NOT_FOUND, MethodString( nMethodId ), aString1 ) );
												    else
												    {
													    pLB->SelectEntry( aString1, bBool1 );
													    if ( pLB->IsEntrySelected( aString1 ) ? !bBool1 : bBool1 )	// XOR rein mit BOOL
														    ReportError( aUId, GEN_RES_STR2( S_METHOD_FAILED, MethodString( nMethodId ), aString1 ) );
												    }
			    * /									ReportError( aUId, GEN_RES_STR1( S_SELECT_DESELECT_VIA_STRING_NOT_IMPLEMENTED, MethodString( nMethodId ) ) );
											    }
											    else
											    {
												    if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,((SvLBox*)pControl)->GetVisibleCount()) )
												    {
													    SvLBoxEntry *pEntry = (SvLBoxEntry*)((SvTreeListBox*)pControl)->GetEntryAtVisPos( nNr1-1 );
													    ((SvTreeListBox*)pControl)->Select ( pEntry, bBool1 );
												    }
											    }
											    break;*/
										    default:
											    ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "EditBrowseBox" ) );
											    break;
									    }
                                    }
									break;
								case CONST_CTValueSet:
                                    {
                                        ValueSet *pVS = dynamic_cast< ValueSet* >(pControl);
									    switch ( nMethodId )
									    {
                                        case M_GetItemCount:
											pRet->GenReturn ( RET_Value, aUId, comm_ULONG( pVS->GetItemCount()));
                                            break;
                                        case M_GetItemText:
    										if ( ValueOK( aUId, MethodString( nMethodId ), nNr1, pVS->GetItemCount() ))
                                             	pRet->GenReturn ( RET_Value, aUId, pVS->GetItemText( pVS->GetItemId( nNr1-1 ) ) );
                                            break;
                                        case M_Select:
    										if ( ValueOK( aUId, MethodString( nMethodId ), nNr1, pVS->GetItemCount() ))
                                             	pVS->SelectItem( pVS->GetItemId( nNr1-1 ) );
                                            break;
						                case M_GetSelIndex :
								            if ( pVS->IsNoSelection() )
									            pRet->GenReturn ( RET_Value, aUId, comm_ULONG(0));
                                            else
        							            pRet->GenReturn ( RET_Value, aUId, comm_ULONG( pVS->GetItemPos( pVS->GetSelectItemId() ) +1));
							                break;
						                case M_GetSelText :
								            if ( pVS->IsNoSelection() )
									            pRet->GenReturn ( RET_Value, aUId, String() );
                                            else
        							            pRet->GenReturn ( RET_Value, aUId, pVS->GetItemText( pVS->GetSelectItemId() ) );
							                break;
						                case M_SetNoSelection :
								            pVS->SetNoSelection();
							                break;
                                        default:
											ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "ValueSet" ) );
											break;
									    }
                                    }
									break;
								case CONST_CTORoadmap:
                                    {
                                        ORoadmap *pRM = dynamic_cast< ORoadmap* >(pControl);
									    switch ( nMethodId )
									    {
                                        case M_GetItemCount:
											pRet->GenReturn ( RET_Value, aUId, comm_ULONG( pRM->GetItemCount()));
                                            break;
                                        case M_GetItemText:
    										if ( ValueOK( aUId, MethodString( nMethodId ), nNr1, pRM->GetItemCount() ))
                                             	pRet->GenReturn ( RET_Value, aUId, pRM->GetRoadmapItemLabel( pRM->GetItemID( nNr1-1 ) ) );
                                            break;
                                        case M_Select:
    										if ( ValueOK( aUId, MethodString( nMethodId ), nNr1, pRM->GetItemCount() ))
                                            {
                                                if ( pRM->IsRoadmapItemEnabled( pRM->GetItemID( nNr1-1 ) ) )
                                             	    pRM->SelectRoadmapItemByID( pRM->GetItemID( nNr1-1 ) );
                                                else
                									ReportError( aUId, GEN_RES_STR1c( S_WIN_DISABLED, "RoadmapItem" ) );
                                            }
                                            break;
						                case M_GetSelIndex :
        							            pRet->GenReturn ( RET_Value, aUId, comm_ULONG( pRM->GetItemIndex( pRM->GetCurrentRoadmapItemID() ) +1));
							                break;
						                case M_GetSelText :
        							            pRet->GenReturn ( RET_Value, aUId, pRM->GetRoadmapItemLabel( pRM->GetCurrentRoadmapItemID() ) );
							                break;
						                case M_IsItemEnabled :
    										if ( ValueOK( aUId, MethodString( nMethodId ), nNr1, pRM->GetItemCount() ))
        							            pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)pRM->IsRoadmapItemEnabled( pRM->GetItemID( nNr1-1 ) ) );
							                break;
                                        default:
                                            ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "RoadMap" ) );
											break;
									    }
                                    }
									break;
								case CONST_CTIExtensionListBox:
                                    {
                                        IExtensionListBox *pELB = dynamic_cast< IExtensionListBox* >(pControl);
									    switch ( nMethodId )
									    {
                                        case M_GetItemCount:
										    pRet->GenReturn ( RET_Value, aUId, comm_ULONG( pELB->getItemCount()));
                                            break;
                                        case M_GetItemText:
    										if ( ValueOK( aUId, MethodString( nMethodId ), nNr1, pELB->getItemCount() ))
                                                switch ( nNr2 )
                                                {
                                                case 1:
                                                    pRet->GenReturn ( RET_Value, aUId, pELB->getItemName( nNr1 -1 ) );
                                                    break;
                                                case 2:
                                                    pRet->GenReturn ( RET_Value, aUId, pELB->getItemVersion( nNr1 -1 ) );
                                                    break;
                                                case 3:
                                                    pRet->GenReturn ( RET_Value, aUId, pELB->getItemDescription( nNr1 -1 ) );
                                                    break;
                                                case 4:
                                                    pRet->GenReturn ( RET_Value, aUId, pELB->getItemPublisher( nNr1 -1 ) );
                                                    break;
                                                case 5:
                                                    pRet->GenReturn ( RET_Value, aUId, pELB->getItemPublisherLink( nNr1 -1 ) );
                                                    break;
                                                default:
                                                    ValueOK( aUId, MethodString( nMethodId ).AppendAscii(" String Number"), nNr2, 5 );
                                                }
                                            break;
                                        case M_Select:
                                            if ( (nParams & PARAM_USHORT_1) )
                                            {
    										    if ( ValueOK( aUId, MethodString( nMethodId ), nNr1, pELB->getItemCount() ))
                                                {
                                                    pELB->select( nNr1-1 );
                                                }
                                            }
                                            else if ( (nParams & PARAM_STR_1) )
                                            {
                                                pELB->select( aString1 );
                                                BOOL bSuccess = TRUE;
                                                if ( pELB->getSelIndex() == EXTENSION_LISTBOX_ENTRY_NOTFOUND )
                                                    bSuccess = FALSE;
                                                else
                                                {
                                                    if ( !aString1.Equals( String( pELB->getItemName( pELB->getSelIndex() ) ) ) )
                                                        bSuccess = FALSE;
                                                }
                                                if ( !bSuccess )
                                                    ReportError( aUId, GEN_RES_STR2( S_ENTRY_NOT_FOUND, MethodString( nMethodId ), aString1 ) );
                                            }
                                            break;
						                case M_GetSelCount :
                                            if ( pELB->getSelIndex() == EXTENSION_LISTBOX_ENTRY_NOTFOUND )
                                                pRet->GenReturn ( RET_Value, aUId, comm_ULONG( 0 ));
                                            else
                                                pRet->GenReturn ( RET_Value, aUId, comm_ULONG( 1 ));
							                break;
						                case M_GetSelIndex :
                                            if ( pELB->getSelIndex() == EXTENSION_LISTBOX_ENTRY_NOTFOUND )
                                                pRet->GenReturn ( RET_Value, aUId, comm_ULONG( 0 ));
                                            else
       							                pRet->GenReturn ( RET_Value, aUId, comm_ULONG( pELB->getSelIndex() +1));
							                break;
/*						                xxxcase M_SetNoSelection :
							                ((ListBox*)pControl)->SetNoSelection();
							                ((ListBox*)pControl)->Select();
							                break; */
                                        default:
                                            ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "RoadMap" ) );
											break;
									    }
                                    }
									break;
								case CONST_CTUnknown:
                					ReportError( aUId, GEN_RES_STR2( S_UNKNOWN_TYPE, UniString::CreateFromInt32( nRT ), MethodString(nMethodId) ) );
									break;
								default:
									ReportError( aUId, GEN_RES_STR1( S_INTERNAL_ERROR, MethodString( nMethodId ) ) );
									break;
							}
					}
					break;
                }
				case C_Window:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "Window" ) );
							break;
					}
					break;

				case C_DockingWin:
                    DockingWin:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						case M_Dock :
							if ( ((DockingWindow*)pControl)->IsFloatingMode() )
								((DockingWindow*)pControl)->SetFloatingMode(FALSE);
							else
								ReportError( aUId, GEN_RES_STR1( S_ALLOWED_ONLY_IN_FLOATING_MODE, MethodString( nMethodId ) ) );
							break;
						case M_Undock :
							if ( !((DockingWindow*)pControl)->IsFloatingMode() )
								((DockingWindow*)pControl)->SetFloatingMode(TRUE);
							else
								ReportError( aUId, GEN_RES_STR1( S_ALLOWED_ONLY_IN_FLOATING_MODE, MethodString( nMethodId ) ) );
							break;
						case M_IsDocked :
							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL) !((DockingWindow*)pControl)->IsFloatingMode());
							break;
						case M_Close:
								//aWindowWaitUId = aUId;
							DBG_ASSERT( aUId.Matches( pControl->GetSmartUniqueOrHelpId() ), "aUID != UniqueOrHelpId");
							SET_WINP_CLOSING(pControl);
							((DockingWindow*)pControl)->Close();
							break;
						case M_Size:
						case M_Move:
						case M_IsMax:
						case M_Minimize:
						case M_Maximize:
							if ( ((DockingWindow*)pControl)->IsFloatingMode() )
							{
								Window* pFloat = ((DockingWindow*)pControl)->GetFloatingWindow();
                                if ( !pFloat && ((DockingWindow*)pControl)->IsFloatingMode() )
                                {
                                    if ( pControl->GET_REAL_PARENT() && pControl->GET_REAL_PARENT()->GetType() == WINDOW_FLOATINGWINDOW )
                                        pFloat = pControl->GET_REAL_PARENT();
                                    else
                                    {
                                        DBG_ERROR("FloatingMode set but Parent is no FloatingWindow");
                                    }
                                }
                                if ( pFloat && pFloat->GetType() == WINDOW_FLOATINGWINDOW )
                                {
                                    pControl = pFloat;
								    goto FloatWin;
                                }
                                else
    								ReportError( aUId, GEN_RES_STR1( S_CANNOT_FIND_FLOATING_WIN, MethodString( nMethodId ) ) );
							}
							else
								ReportError( aUId, GEN_RES_STR1( S_ALLOWED_ONLY_IN_DOCKING_MODE, MethodString( nMethodId ) ) );
							break;
						case M_Help:		// Alles was unten weiterbehandelt werden soll
							goto MoreDialog;

						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "DockingWindow" ) );
							break;
					}
					break;
				case C_FloatWin:
					FloatWin:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						case M_IsMax :
							pRet->GenReturn ( RET_Value, aUId, (comm_BOOL)!((FloatingWindow*)pControl)->IsRollUp());
							break;
						case M_Minimize :
							((FloatingWindow*)pControl)->RollUp();
							break;
						case M_Maximize :
							((FloatingWindow*)pControl)->RollDown();
							break;
						case M_Size:
						{
							if ( pControl->GetStyle() & WB_SIZEABLE )
							{
                                Size aMin = ((FloatingWindow*)pControl)->GetMinOutputSizePixel();
                                if ( aMin.Width() <= nNr1 && aMin.Height() <= nNr2 )
                                {
								    pControl->SetSizePixel(Size(nNr1,nNr2));
								    pControl->Resize();
                                }
                                else
                                {
                                    ReportError( aUId, GEN_RES_STR2( S_SIZE_BELOW_MINIMUM, String::CreateFromInt32( aMin.Width() ), String::CreateFromInt32( aMin.Height() ) ) );
                                }
							}
							else
								ReportError( aUId, GEN_RES_STR1( S_SIZE_NOT_CHANGEABLE, MethodString( nMethodId ) ) );
							break;
						}
						case M_Close:
							DBG_ASSERT( aUId.Matches( pControl->GetSmartUniqueOrHelpId() ), "aUID != UniqueOrHelpId");
							SET_WINP_CLOSING(pControl);
							((FloatingWindow*)pControl)->Close();
							break;
						case M_Help:		// Alles was unten weiterbehandelt werden soll
						case M_Move:
							goto MoreDialog;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "FloatingWin" ) );
							break;
					}
					break;
				case C_ModelessDlg:
				case C_ModalDlg:
                case C_Dialog:
                case C_TabDlg:
					MoreDialog:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						case M_Close:
							DBG_ASSERT( aUId.Matches( pControl->GetSmartUniqueOrHelpId() ), "aUID != UniqueOrHelpId");
							SET_WINP_CLOSING(pControl);
							((SystemWindow*)pControl)->Close();
							break;
						case M_OK:
						{
							Window *pChild = GetWinByRT( pControl, WINDOW_OKBUTTON );
							if( ControlOK( pChild, "OK Button" ) )
							{
								DBG_ASSERT( aUId.Matches( pControl->GetSmartUniqueOrHelpId() ), "aUID != UniqueOrHelpId");
								SET_WINP_CLOSING(pControl);
								((Button*)pChild)->Click();
							}
							break;
						}
						case M_Cancel:
						{
							Window *pChild = GetWinByRT( pControl, WINDOW_CANCELBUTTON );
							if( ControlOK( pChild, "Cancel Button" ) )
							{
								DBG_ASSERT( aUId.Matches( pControl->GetSmartUniqueOrHelpId() ), "aUID != UniqueOrHelpId");
								SET_WINP_CLOSING(pControl);
								((Button*)pChild)->Click();
							}
							break;
						}
						case M_Help:
						{
							Window *pChild = GetWinByRT( pControl, WINDOW_HELPBUTTON );
							if( ControlOK( pChild, "Help Button" ) )
								((Button*)pChild)->Click();
							break;
						}
						case M_Default:
						{
							Window *pChild = ImpGetButton( pControl, WB_DEFBUTTON, WB_DEFBUTTON );
							if( ControlOK( pChild, "Default Button" ) )
								((Button*)pChild)->Click();
							break;
						}
						case M_Move:
						{
							pControl->SetPosPixel(Point(nNr1,nNr2));
							break;
						}
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "Dialog" ) );
							break;
					}
					break;
				case C_WorkWin:
					switch( nMethodId )
					{
						case M_AnimateMouse :
							AnimateMouse( pControl, MitteOben);
							break;
						case M_Close:
							DBG_ASSERT( aUId.Matches( pControl->GetSmartUniqueOrHelpId() ), "aUID != UniqueOrHelpId");
							SET_WINP_CLOSING(pControl);
							((WorkWindow*)pControl)->Close();
							break;
						case M_Size:
						case M_Move:
							goto FloatWin;
//							break;
						case M_IsMax :
                            pRet->GenReturn ( RET_Value, aUId, ((WorkWindow*)pControl)->IsMaximized() );
                            break;
						case M_IsMin :
                            pRet->GenReturn ( RET_Value, aUId, ((WorkWindow*)pControl)->IsMinimized() );
                            break;
						case M_IsRestore :
                            pRet->GenReturn ( RET_Value, aUId, comm_BOOL (!((WorkWindow*)pControl)->IsMaximized() && !((WorkWindow*)pControl)->IsMinimized()) );
                            break;
                        case M_Minimize :
                            ((WorkWindow*)pControl)->Maximize( FALSE );
                            ((WorkWindow*)pControl)->Minimize();
                            break;
						case M_Maximize :
                            ((WorkWindow*)pControl)->Maximize();
                            break;
						case M_Restore :
                            ((WorkWindow*)pControl)->Maximize( FALSE );
                            ((WorkWindow*)pControl)->Restore();
                            break;
						case M_Help:		// Alles was unten weiterbehandelt werden soll
							goto MoreDialog;
						default:
							ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "WorkWindow" ) );
							break;
					}
					break;
				case C_TabPage:
					ReportError( aUId, GEN_RES_STR1( S_INTERNAL_ERROR, MethodString( nMethodId ) ) );
					break;
				case C_MessBox:
				case C_InfoBox:
				case C_WarningBox:
				case C_ErrorBox:
				case C_QueryBox:
					{
						BOOL bDone = TRUE;
						MessBox* pMB = (MessBox*)pControl;
						switch( nMethodId )
						{
							case M_GetCheckBoxText:
								pRet->GenReturn ( RET_Value, aUId, pMB->GetCheckBoxText() );
								break;
							case M_IsChecked :
								pRet->GenReturn ( RET_Value, aUId, comm_BOOL( pMB->GetCheckBoxState() == STATE_CHECK) );
								break;
							case M_Check :
								pMB->SetCheckBoxState( TRUE );
								break;
							case M_UnCheck :
								pMB->SetCheckBoxState( FALSE );
								break;
							case M_GetText :
								pRet->GenReturn ( RET_Value, aUId, pMB->GetMessText());
								break;

							default:
								bDone = FALSE;
								break;
						}
						if ( bDone )
							break;	// break the case here else continue at C_ButtonDialog
					}
				case C_ButtonDialog:
					{
						ButtonDialog* pBD = (ButtonDialog*)pControl;
#if OSL_DEBUG_LEVEL > 1
						m_pDbgWin->AddText( "Working MessBox: " );
						if (pControl->IsVisible())
							m_pDbgWin->AddText("*(Visible)\n");
						else
							m_pDbgWin->AddText("*(nicht Visible)\n");
#endif
						switch( nMethodId )
						{
							case M_AnimateMouse :
								AnimateMouse( pControl, Mitte);
								break;
							case M_OK:
#if ( BUTTONID_OK != RET_OK )
#error BUTTONID_OK != RET_OK
#endif
								if ( pBD->GetPushButton( BUTTONID_OK ) )
								{
									SET_WINP_CLOSING(pControl);
									pBD->EndDialog(RET_OK);
								}
								else
									ReportError( aUId, GEN_RES_STR1( S_NO_OK_BUTTON, MethodString( nMethodId ) ) );
								break;
							case M_Cancel:
								if ( pBD->GetPushButton( BUTTONID_CANCEL ) )
								{
									SET_WINP_CLOSING(pControl);
									pBD->EndDialog(RET_CANCEL);
								}
								else
									ReportError( aUId, GEN_RES_STR1( S_NO_CANCEL_BUTTON, MethodString( nMethodId ) ) );
								break;
							case M_Yes:
								if ( pBD->GetPushButton( BUTTONID_YES ) )
								{
									SET_WINP_CLOSING(pControl);
									pBD->EndDialog(RET_YES);
								}
								else
									ReportError( aUId, GEN_RES_STR1( S_NO_YES_BUTTON, MethodString( nMethodId ) ) );
								break;
							case M_No:
								if ( pBD->GetPushButton( BUTTONID_NO ) )
								{
									SET_WINP_CLOSING(pControl);
									pBD->EndDialog(RET_NO);
								}
								else
									ReportError( aUId, GEN_RES_STR1( S_NO_NO_BUTTON, MethodString( nMethodId ) ) );
								break;
							case M_Repeat:
								if ( pBD->GetPushButton( BUTTONID_RETRY ) )
								{
									SET_WINP_CLOSING(pControl);
									pBD->EndDialog(RET_RETRY);
								}
								else
									ReportError( aUId, GEN_RES_STR1( S_NO_RETRY_BUTTON, MethodString( nMethodId ) ) );
								break;
							case M_Help:
								if ( pBD->GetPushButton( BUTTONID_HELP ) )
								{
									SET_WINP_CLOSING(pControl);
									pBD->EndDialog(BUTTONID_HELP);
								}
								else
									ReportError( aUId, GEN_RES_STR1( S_NO_HELP_BUTTON, MethodString( nMethodId ) ) );
								break;
							case M_Default:
								{
									WinBits Style = pControl->GetStyle();
									if      ( Style & WB_DEF_OK )
									{
										SET_WINP_CLOSING(pControl);
										pBD->EndDialog(RET_OK);
									}
									else if ( Style & WB_DEF_CANCEL )
									{
										SET_WINP_CLOSING(pControl);
										pBD->EndDialog(RET_CANCEL);
									}
									else if ( Style & WB_DEF_YES )
									{
										SET_WINP_CLOSING(pControl);
										pBD->EndDialog(RET_YES);
									}
									else if ( Style & WB_DEF_NO )
									{
										SET_WINP_CLOSING(pControl);
										pBD->EndDialog(RET_NO);
									}
									else if ( Style & WB_DEF_RETRY )
									{
										SET_WINP_CLOSING(pControl);
										pBD->EndDialog(RET_RETRY);
									}
									else
										ReportError( aUId, GEN_RES_STR1( S_NO_DEFAULT_BUTTON, MethodString( nMethodId ) ) );
								}
								break;
							case M_GetText :
								pRet->GenReturn ( RET_Value, aUId, pControl->GetText());
								break;
							case M_Click:
								if ( nParams & PARAM_USHORT_1 )
								{
									if ( pBD->GetPushButton( nNr1 ) )
									{
										if ( nNr1 != BUTTONID_HELP )
										{
											SET_WINP_CLOSING(pControl);
										}
										pBD->GetPushButton( nNr1 )->Click();
									}
									else
										ReportError( aUId, GEN_RES_STR2( S_NO_DEFAULT_BUTTON, UniString::CreateFromInt32( nNr1 ), MethodString( nMethodId ) ) );
								}
								else
									ReportError( aUId, GEN_RES_STR1( S_BUTTONID_REQUIRED, MethodString( nMethodId ) ) );
								break;
							case M_GetButtonCount :
								pRet->GenReturn ( RET_Value, aUId, comm_ULONG(pBD->GetButtonCount()));
								break;
							case M_GetButtonId :
								if ( ValueOK(aUId, MethodString( nMethodId ),nNr1,pBD->GetButtonCount()) )
									pRet->GenReturn ( RET_Value, aUId, comm_ULONG(pBD->GetButtonId(nNr1-1)));
								break;
							default:
								ReportError( aUId, GEN_RES_STR2c2( S_UNKNOWN_METHOD, MethodString(nMethodId), "MessageBox" ) );
								break;
						}
						break;
					}
				default:
					DBG_ERROR( "Unknown Objekttype from UId or Method not suported" );
					ReportError( aUId, GEN_RES_STR2( S_UNKNOWN_TYPE, UniString::CreateFromInt32( nRT ), MethodString(nMethodId) ) );
#if OSL_DEBUG_LEVEL > 1
					m_pDbgWin->AddText( " Unknown Objekttype from UId or Method not suported" );
#endif
					break;
			}
		}
		for( int i = 0; i < 32; i++ )
			SafeReschedule();
	}
#if OSL_DEBUG_LEVEL > 1
	m_pDbgWin->AddText( "\n" );
#endif
	if ( bStatementDone )
	{
		SendProfile( UIdString( aUId ).Append('.').Append( MethodString( nMethodId ) ) );
		delete this;
	}
	else
	{
		if ( nRetryCount-- )
		{
#if OSL_DEBUG_LEVEL > 1
			m_pDbgWin->AddText( CUniString("Reschedule command (requed) (").Append( UniString::CreateFromInt32(nRetryCount) ).AppendAscii(")\n") );
#endif
			QueStatement( this );	// will que at the start of the list
		}
		else
		{
			bStatementDone=TRUE;
		}
	}
	return bStatementDone;

#define FINISH_NEXT
#define FINISH_SAME

}
