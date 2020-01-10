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
#include <svtools/filedlg.hxx>
#include <stdio.h>
#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#include <tools/debug.hxx>
#include <svtools/testtool.hxx>
#include <svtools/ttprops.hxx>
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>
#include <osl/process.h>
#include <ucbhelper/contentbroker.hxx>
#include <ucbhelper/configurationkeys.hxx>
#include <comphelper/regpathhelper.hxx>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/servicefactory.hxx>
#include <com/sun/star/registry/XImplementationRegistration.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/ucb/XContentProviderManager.hpp>

#include "servres.hrc"
#include "servres.hxx"
#include "testapp.hxx"

using namespace comphelper;
using namespace cppu;
using namespace rtl;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::registry;
using namespace com::sun::star::ucb;

static ResMgr* pAppMgr = NULL;

MainWindow::MainWindow(MyApp *pAppl)
: WorkWindow(NULL, WB_STDWORK)
, pApp(pAppl)
{}

IMPL_LINK(MainWindow,MenuSelectHdl,MenuBar*,aMenu)
{

	return pApp->GetDispatcher()->ExecuteFunction(aMenu->GetCurItemId());

}

void MainWindow::FileOpen()
{
	FileDialog Dlg(this,WB_OPEN );

	Dlg.AddFilter(CUniString("Alle Dateien"), CUniString("*.*"));
	Dlg.SetCurFilter (CUniString("*.*"));

	if (Dlg.Execute() == RET_OK)
	{
		new EditFileWindow(this,Dlg.GetPath());
	}
}


void MainWindow::TestGross()
{
	ModalDialogGROSSER_TEST_DLG Dlg(this,ResId(GROSSER_TEST_DLG, *pAppMgr));

	if (Dlg.Execute() == RET_OK)
	{
	}
}


BOOL MainWindow::Close()
{
	WorkWindow::Close();
	FileExit();
	return TRUE;
}

void MainWindow::FileExit()
{
/*	WriteSTBProfile();*/

//	if (pApp->CloseAll())
		pApp->Quit();
}


void MainWindow::Tree(GHEditWindow *aEditWin, Window *pBase, USHORT Indent)
{
	String sIndent,aText;
	sIndent.Expand(5*Indent);

	aText = pBase->GetText();
	aText.SearchAndReplaceAllAscii("\n",CUniString("\\n"));

	aEditWin->AddText(String(sIndent).AppendAscii("Text: ").Append(aText).AppendAscii("\n"));
	aEditWin->AddText(String(sIndent).AppendAscii("Help: ").Append(String::CreateFromInt64(pBase->GetHelpId())).AppendAscii(":").Append(pBase->GetQuickHelpText()).AppendAscii(":").Append(pBase->GetHelpText()).AppendAscii("\n"));

	USHORT i;
	for (i = 0 ; i < pBase->GetChildCount() ; i++)
	{
		Tree(aEditWin,pBase->GetChild(i),Indent+1);
	}
}

void MainWindow::WinTree()
{

	GHEditWindow * aEditWin = new GHEditWindow(this,CUniString("Window Tree"));
	Tree(aEditWin,this,0);

}

void MainWindow::SysDlg()
{
	switch (QueryBox(this,WB_YES_NO_CANCEL | WB_DEF_YES, CUniString("Want to open another Dialog?")).Execute())
  	{
  		case RET_YES:
			while ( WarningBox(this,WB_OK_CANCEL | WB_DEF_OK,CUniString("Well this is the last box now!")).Execute() == RET_OK ) ;
  			break;
  		case RET_NO:
  			break;
		case RET_CANCEL:InfoBox(this,CUniString("Oh well..")).Execute();
  			break;
  	}

/*

#define WB_OK					((WinBits)0x0010)
#define WB_OK_CANCEL			((WinBits)0x0020)
#define WB_YES_NO				((WinBits)0x0040)
#define WB_YES_NO_CANCEL		((WinBits)0x0080)
#define WB_RETRY_CANCEL			((WinBits)0x0100)

#define WB_DEF_OK				((WinBits)0x0200)
#define WB_DEF_CANCEL			((WinBits)0x0400)
#define WB_DEF_RETRY			((WinBits)0x0800)
#define WB_DEF_YES				((WinBits)0x1000)
#define WB_DEF_NO				((WinBits)0x2000)

#define RET_OK				 TRUE
#define RET_CANCEL			 FALSE
#define RET_YES				 2
#define RET_NO				 3
#define RET_RETRY			 4
*/
}

MyApp aApp;

MyApp::MyApp()
{
	pMainWin = NULL;
}

void MyApp::Property( ApplicationProperty& rProp )
{
	TTProperties* pTTProperties = PTR_CAST( TTProperties, &rProp );
	if ( pTTProperties )
	{
		pTTProperties->nPropertyVersion = TT_PROPERTIES_VERSION;
		switch ( pTTProperties->nActualPR )
		{
/*			case TT_PR_SLOTS:
			{
				pTTProperties->nSidOpenUrl = SID_OPENURL;
				pTTProperties->nSidFileName = SID_FILE_NAME;
				pTTProperties->nSidNewDocDirect = SID_NEWDOCDIRECT;
				pTTProperties->nSidCopy = SID_COPY;
				pTTProperties->nSidPaste = SID_PASTE;
				pTTProperties->nSidSourceView = SID_SOURCEVIEW;
				pTTProperties->nSidSelectAll = SID_SELECTALL;
				pTTProperties->nSidReferer = SID_REFERER;
				pTTProperties->nActualPR = 0;
			}
			break;*/
			case TT_PR_DISPATCHER:
			{
				PlugInDispatcher* pDispatcher = GetDispatcher();
				if ( !pDispatcher )
					pTTProperties->nActualPR = TT_PR_ERR_NODISPATCHER;
				else
				{
					pDispatcher->SetExecuteMode(EXECUTEMODE_DIALOGASYNCHRON);
					if ( pDispatcher->ExecuteFunction(
							pTTProperties->mnSID, pTTProperties->mppArgs, pTTProperties->mnMode )
						 == EXECUTE_NO )
						pTTProperties->nActualPR = TT_PR_ERR_NOEXECUTE;
					else
						pTTProperties->nActualPR = 0;
				}
			}
			break;
/*			case TT_PR_IMG:
			{
				SvDataMemberObjectRef aDataObject = new SvDataMemberObject();
				SvData* pDataBmp = new SvData( FORMAT_BITMAP );
				pDataBmp->SetData( pTTProperties->mpBmp );
				aDataObject->Append( pDataBmp );
				aDataObject->CopyClipboard();
				pTTProperties->nActualPR = 0;
			}
			break;*/
			default:
			{
				pTTProperties->nPropertyVersion = 0;
			}
		}
		return;
	}
}


USHORT MyDispatcher::ExecuteFunction( USHORT nSID, SfxPoolItem** ppArgs, USHORT nMode)
{
    (void) ppArgs; /* avoid warning about unused parameter */
    (void) nMode; /* avoid warning about unused parameter */

	switch (nSID)
	{
		case IDM_FILE_EXIT:       pMainWin->FileExit();				break;
		case IDM_FILE_OPEN_TEST:  pMainWin->FileOpen();				break;
		case IDM_TEST_GROSS:      pMainWin->TestGross();			break;
		case IDM_TEST_WINTREE:	  pMainWin->WinTree();				break;
		case IDM_SYS_DLG:		  pMainWin->SysDlg();				break;
		default:
			{
				DBG_ERROR1("Dispatcher kennt Funktion nicht %s",ByteString::CreateFromInt64(nSID).GetBuffer());
				return EXECUTE_NO;
			}

	}
	return EXECUTE_YES;
}

PlugInDispatcher* MyApp::GetDispatcher()
{
	return pMyDispatcher;
}

Reference< XContentProviderManager > InitializeUCB( void )
{
    OUString path;
    if( osl_Process_E_None != osl_getExecutableFile( (rtl_uString**)&path ) )
    {
        InfoBox( NULL, String::CreateFromAscii( "Couldn't retrieve directory of executable" ) ).Execute();
        exit( 1 );
    }
    OSL_ASSERT( path.lastIndexOf( '/' ) >= 0 );
    

    ::rtl::OUStringBuffer bufServices( path.copy( 0, path.lastIndexOf( '/' )+1 ) );
    bufServices.appendAscii("services.rdb");
    OUString services = bufServices.makeStringAndClear();

    ::rtl::OUStringBuffer bufTypes( path.copy( 0, path.lastIndexOf( '/' )+1 ) );
    bufTypes.appendAscii("types.rdb");
    OUString types = bufTypes.makeStringAndClear();
    
    
    Reference< XMultiServiceFactory > xSMgr;
    try
    {
        xSMgr = createRegistryServiceFactory( types, services, sal_True );
    }
    catch( com::sun::star::uno::Exception & exc )
    {
        fprintf( stderr, "Couldn't bootstrap uno servicemanager for reason : %s\n" ,
                 OUStringToOString( exc.Message, RTL_TEXTENCODING_ASCII_US ).getStr() );
        InfoBox( NULL, String( exc.Message ) ).Execute();
        throw ;
    }
        

	//////////////////////////////////////////////////////////////////////
	// set global factory
	setProcessServiceFactory( xSMgr );

//  Create unconfigured Ucb:
	Sequence< Any > aArgs;
	ucbhelper::ContentBroker::initialize( xSMgr, aArgs );
    Reference< XContentProviderManager > xUcb =
        ucbhelper::ContentBroker::get()->getContentProviderManagerInterface();

	Reference< XContentProvider > xFileProvider
		( xSMgr->createInstance( OUString::createFromAscii( "com.sun.star.ucb.FileContentProvider" ) ), UNO_QUERY );
    xUcb->registerContentProvider( xFileProvider, OUString::createFromAscii( "file" ), sal_True );

	return xUcb;
}

void MyApp::Main()
{
	Reference< XContentProviderManager > xUcb = InitializeUCB();
	LanguageType aRequestedLanguage;
	aRequestedLanguage = LanguageType( LANGUAGE_GERMAN );

	AllSettings aSettings = GetSettings();
    aSettings.SetUILanguage( aRequestedLanguage );
    aSettings.SetLanguage( aRequestedLanguage );
	SetSettings( aSettings );
	pAppMgr =  CREATEVERSIONRESMGR( tma );

	MainWindow  MainWin(this);
	pMainWin = &MainWin;

	MenuBar aMenu(ResId(MENU_CLIENT,*pAppMgr));
	MainWin.SetMenuBar( &aMenu );
	aMenu.GetPopupMenu( IDM_FILE )->SetSelectHdl(LINK(&MainWin, MainWindow, MenuSelectHdl));
	aMenu.GetPopupMenu( IDM_TEST )->SetSelectHdl(LINK(&MainWin, MainWindow, MenuSelectHdl));

	MyDispatcher MyDsp(pMainWin);
	pMyDispatcher = &MyDsp;

	MainWin.SetText(CUniString("Star Division Test Tool Client Window"));
	MainWin.Show();

	RemoteControl aRC;

	Execute();
}

