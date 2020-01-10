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
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/script/XLibraryContainer.hpp>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/uno/Reference.h>
#include <basic/basrdll.hxx>
#include <tools/urlobj.hxx>
#include <svtools/macitem.hxx>
#include <basic/sbxfac.hxx>
#include <basic/sbx.hxx>
#include <vcl/gradient.hxx>
#include <svtools/rectitem.hxx>
#include <svtools/intitem.hxx>
#include <svtools/eitem.hxx>
#include <basic/sbmod.hxx>
#include <svtools/whiter.hxx>
#include <basic/sbmeth.hxx>
#include <basic/sbstar.hxx>
#include <vcl/wrkwin.hxx>
#include <vcl/msgbox.hxx>
#include <basic/sbuno.hxx>
#include <svtools/sfxecode.hxx>
#include <svtools/ehdl.hxx>

#include <svtools/undoopt.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/useroptions.hxx>
#include <unotools/bootstrap.hxx>

#include <sfx2/appuno.hxx>
#include <sfx2/module.hxx>
#include "arrdecl.hxx"
#include <sfx2/app.hxx>
#include "sfxtypes.hxx"
#include "sfxresid.hxx"
#include <sfx2/msg.hxx>
#include <sfx2/msgpool.hxx>
#include <sfx2/progress.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/objitem.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/dispatch.hxx>
#include "tplpitem.hxx"
#include "minfitem.hxx"
#include "app.hrc"
#include <sfx2/evntconf.hxx>
#include <sfx2/macrconf.hxx>
#include <sfx2/request.hxx>
#include <sfx2/dinfdlg.hxx>
#include "appdata.hxx"
#include "appbas.hxx"
#include "sfxhelp.hxx"
#include "basmgr.hxx"
#include "sorgitm.hxx"
#include "appbaslib.hxx"
#include <basic/basicmanagerrepository.hxx>

#define ITEMID_SEARCH SID_SEARCH_ITEM

#include <sfx2/srchitem.hxx>
#include <vos/socket.hxx>

#define SFX_TYPEMAP
#define Selection
#include "sfxslots.hxx"

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::script;

using ::basic::BasicManagerRepository;

// #ifndef STR_VERSION_ID
// #define STR_VERSION_ID 1
// #endif
// #ifndef STR_VERSION_TYPE
// #define STR_VERSION_TYPE 1
// #endif

//=========================================================================
/*ASDBG
const SfxConstant __FAR_DATA aConstants[] =
{
	SFX_USHORT_CONSTANT( "AlignBottom",      	ALIGN_BOTTOM ),
	SFX_USHORT_CONSTANT( "AlignTop",         	ALIGN_TOP ),
	SFX_USHORT_CONSTANT( "CharSetANSI",      	CHARSET_ANSI ),
	SFX_USHORT_CONSTANT( "CharSetDontKnow",  	CHARSET_DONTKNOW ),
	SFX_USHORT_CONSTANT( "CharSetIBMPC437",  	CHARSET_IBMPC_437 ),
	SFX_USHORT_CONSTANT( "CharSetIBMPC850",  	CHARSET_IBMPC_850 ),
	SFX_USHORT_CONSTANT( "CharSetIBMPC860",  	CHARSET_IBMPC_860 ),
	SFX_USHORT_CONSTANT( "CharSetIBMPC861",  	CHARSET_IBMPC_861 ),
	SFX_USHORT_CONSTANT( "CharSetIBMPC863",  	CHARSET_IBMPC_863 ),
	SFX_USHORT_CONSTANT( "CharSetIBMPC865",  	CHARSET_IBMPC_865 ),
	SFX_USHORT_CONSTANT( "CharSetIBMPC",     	CHARSET_IBMPC ),
	SFX_USHORT_CONSTANT( "CharSetMac",       	CHARSET_MAC ),
	SFX_USHORT_CONSTANT( "CharSetSymbol",    	CHARSET_SYMBOL ),
	SFX_USHORT_CONSTANT( "CharSetSystem",    	RTL_TEXTENCODING_UTF8 ),
	SFX_USHORT_CONSTANT( "FamilyDecorative", 	FAMILY_DECORATIVE ),
	SFX_USHORT_CONSTANT( "FamilyDontknow",   	FAMILY_DONTKNOW ),
	SFX_USHORT_CONSTANT( "FamilyModern",     	FAMILY_MODERN ),
	SFX_USHORT_CONSTANT( "FamilyRoman",      	FAMILY_ROMAN ),
	SFX_USHORT_CONSTANT( "FamilyScript",     	FAMILY_SCRIPT ),
	SFX_USHORT_CONSTANT( "FamilySwiss",      	FAMILY_SWISS ),
	SFX_USHORT_CONSTANT( "FamilySystem",     	FAMILY_SYSTEM ),
	SFX_USHORT_CONSTANT( "GradientAxial",    	GRADIENT_AXIAL ),
	SFX_USHORT_CONSTANT( "GradientElliptical", 	GRADIENT_ELLIPTICAL ),
	SFX_USHORT_CONSTANT( "GradientLinear",   	GRADIENT_LINEAR ),
	SFX_USHORT_CONSTANT( "GradientRadial",   	GRADIENT_RADIAL ),
	SFX_USHORT_CONSTANT( "GradientRect",     	GRADIENT_RECT ),
	SFX_USHORT_CONSTANT( "GradientSquare",   	GRADIENT_SQUARE ),
	SFX_USHORT_CONSTANT( "ItalicNone",       	ITALIC_NONE ),
	SFX_USHORT_CONSTANT( "ItalicOblique",    	ITALIC_OBLIQUE ),
	SFX_USHORT_CONSTANT( "StrikeThroughDouble",	STRIKEOUT_DOUBLE ),
	SFX_USHORT_CONSTANT( "StrikeThroughNone", 	STRIKEOUT_NONE ),
	SFX_USHORT_CONSTANT( "StrikeThroughSingle", STRIKEOUT_SINGLE ),
	SFX_USHORT_CONSTANT( "UnderlineDotted",  	UNDERLINE_DOTTED ),
	SFX_USHORT_CONSTANT( "UnderlineDouble",  	UNDERLINE_DOUBLE ),
	SFX_USHORT_CONSTANT( "UnderlineNone",    	UNDERLINE_NONE ),
	SFX_USHORT_CONSTANT( "UnderlineSingle",  	UNDERLINE_SINGLE ),
	SFX_USHORT_CONSTANT( "UnitFoot",         	FUNIT_FOOT ),
	SFX_USHORT_CONSTANT( "UnitInch",         	FUNIT_INCH ),
	SFX_USHORT_CONSTANT( "UnitMile",         	FUNIT_MILE ),
	SFX_USHORT_CONSTANT( "UnitPercent",      	FUNIT_PERCENT ),
	SFX_USHORT_CONSTANT( "UnitPoint",      		FUNIT_POINT ),
	SFX_USHORT_CONSTANT( "UnitTwip",         	FUNIT_TWIP ),
	SFX_USHORT_CONSTANT( "Unit_cm",          	FUNIT_CM ),
	SFX_USHORT_CONSTANT( "Unit_m",          	FUNIT_M ),
	SFX_USHORT_CONSTANT( "Unit_mm",          	FUNIT_MM ),
	SFX_USHORT_CONSTANT( "WeightBlack",      	WEIGHT_BLACK ),
	SFX_USHORT_CONSTANT( "WeightBold",       	WEIGHT_BOLD ),
	SFX_USHORT_CONSTANT( "WeightDontKnow",   	WEIGHT_DONTKNOW ),
	SFX_USHORT_CONSTANT( "WeightLight",      	WEIGHT_LIGHT ),
	SFX_USHORT_CONSTANT( "WeightMedium",     	WEIGHT_MEDIUM ),
	SFX_USHORT_CONSTANT( "WeightNormal",     	WEIGHT_NORMAL ),
	SFX_USHORT_CONSTANT( "WeightSemibold",   	WEIGHT_SEMIBOLD ),
	SFX_USHORT_CONSTANT( "WeightSemilight",  	WEIGHT_SEMILIGHT ),
	SFX_USHORT_CONSTANT( "WeightThin",       	WEIGHT_THIN ),
	SFX_USHORT_CONSTANT( "WeightUltrabold",  	WEIGHT_ULTRABOLD ),
	SFX_USHORT_CONSTANT( "WeightUltralight", 	WEIGHT_ULTRALIGHT )
};
*/
//========================================================================

//------------------------------------------------------------------------
String lcl_GetVersionString(ResMgr* /*pAppData_ImplResMgr*/)
{
	::rtl::OUString aDefault;
	String aVersion( utl::Bootstrap::getBuildIdData( aDefault ));

	if ( aVersion.Len() == 0 )
    {
		DBG_ERROR( "No BUILDID in bootstrap file found" );
    }

	aVersion.Erase( 0, aVersion.Search( ':' ) + 1 );
	aVersion.Erase( aVersion.Search( ')' ) );
	return aVersion;
}

//=========================================================================
sal_uInt16 SfxApplication::SaveBasicManager() const
{
	return 0;
}

//--------------------------------------------------------------------
sal_uInt16 SfxApplication::SaveBasicAndDialogContainer() const
{
    if ( pAppData_Impl->pBasicManager->isValid() )
        pAppData_Impl->pBasicManager->storeAllLibraries();
	return 0;
}

//--------------------------------------------------------------------

void SfxApplication::RegisterBasicConstants
(
	const char*,        // Prefix vor Konstanten-Namen
	const SfxConstant*, // Array von <SfxConstant> Instanzen
	sal_uInt16			// Anahl der Kontanten in pConsts
)

/*	[Beschreibung]

	Diese Methode meldet Konstanten beim BASIC an. Sie sollte on-demand
	(in GetSbxObject() der Applikation) gerufen werden. Das Array mu\s
	alphabetisch nach den Namen sortiert sein!

	Durch den Prefix kann Speicher gespart und das Suchen beschleunigt
	werden. Im StarOffice soll der Prefix "so" verwendet werden.


	[Beispiel]

	const SfxConstant __FAR_DATA aConstants[] =
	{
		SFX_BOOL_CONSTANT( "False", sal_False ),
		SFX_BOOL_CONSTANT( "True",  sal_True ),
	};

	...
	SFX_APP()->RegisterBasicConstants( 0, aConstants, 2 );
	...

*/

{
//  DBG_ASSERT( pAppData_Impl->pBasicMgr, "no basic available" );

//  pAppData_Impl->pBasicMgr->GetLib(0)->Insert(
//				new SfxConstants_Impl( pPrefix, pConsts, nCount ) );
}

//--------------------------------------------------------------------

SbxVariable* MakeVariable( StarBASIC *pBas, SbxObject *pObject,
		   const char *pName, sal_uInt32 nSID, SbxDataType eType, SbxClassType eClassType )
{
	SbxVariable *pVar = pBas->Make( String::CreateFromAscii(pName), eClassType, eType ); //SbxCLASS_PROPERTY
	pVar->SetUserData( nSID );
	pVar->SetFlag( SBX_DONTSTORE );
	pObject->StartListening( pVar->GetBroadcaster() );
	return pVar;
}

//--------------------------------------------------------------------

BasicManager* SfxApplication::GetBasicManager()
{
//	DBG_ASSERT( pAppData_Impl->nBasicCallLevel != 0,
//				"unnecessary call to GetBasicManager() - inefficient!" );
	if ( pAppData_Impl->nBasicCallLevel == 0 )
		// sicherheitshalber
		EnterBasicCall();

    return BasicManagerRepository::getApplicationBasicManager( true );
}

//--------------------------------------------------------------------

Reference< XLibraryContainer > SfxApplication::GetDialogContainer()
{
    if ( !pAppData_Impl->pBasicManager->isValid() )
        GetBasicManager();
    return pAppData_Impl->pBasicManager->getLibraryContainer( SfxBasicManagerHolder::DIALOGS );
}

//--------------------------------------------------------------------

Reference< XLibraryContainer > SfxApplication::GetBasicContainer()
{
    if ( !pAppData_Impl->pBasicManager->isValid() )
        GetBasicManager();
    return pAppData_Impl->pBasicManager->getLibraryContainer( SfxBasicManagerHolder::SCRIPTS );
}

//--------------------------------------------------------------------

StarBASIC* SfxApplication::GetBasic()
{
	return GetBasicManager()->GetLib(0);
}

//--------------------------------------------------------------------

FASTBOOL SfxApplication::IsInBasicCall() const
{
	return 0 != pAppData_Impl->nBasicCallLevel;
}

//--------------------------------------------------------------------

void SfxApplication::EnterBasicCall()
{
	if ( 1 == ++pAppData_Impl->nBasicCallLevel )
	{
		DBG_TRACE( "SfxShellObject: BASIC-on-demand" );

		// das kann l"anger dauern, da Progress nicht geht, wenigstens Sanduhr
//(mba)/task        SfxWaitCursor aWait;

		// zuerst das BASIC laden
		GetBasic();
/*
		// als erstes SfxShellObject das SbxObject der SfxApplication erzeugen
		SbxObject *pSbx = GetSbxObject();
		DBG_ASSERT( pSbx, "SfxShellObject: can't create SbxObject for SfxApplication" );

		// die SbxObjects aller Module erzeugen
		SfxModuleArr_Impl& rArr = GetModules_Impl();
		for ( sal_uInt16 n = 0; n < rArr.Count(); ++n )
		{
			SfxModule *pMod = rArr.GetObject(n);
			if ( pMod->IsLoaded() )
			{
				pSbx = pMod->GetSbxObject();
				DBG_ASSERT( pSbx, "SfxModule: can't create SbxObject" );
			}
		}

		// die SbxObjects aller Tasks erzeugen
		for ( SfxTask *pTask = SfxTask::GetFirst(); pTask; pTask = SfxTask::GetNext( *pTask ) )
			pTask->GetSbxObject();

		// die SbxObjects aller SfxObjectShells erzeugen (ggf. Frame-los!)
		for ( SfxObjectShell *pObjSh = SfxObjectShell::GetFirst( NULL, sal_False );
			  pObjSh;
			  pObjSh = SfxObjectShell::GetNext(*pObjSh, NULL, sal_False) )
		{
			// kein IP-Object oder wenn doch dann initialisiert?
			SvStorageRef aStorage;
			if ( !pObjSh->IsHandsOff() )
				aStorage = pObjSh->GetStorage();
			if ( !pObjSh->GetInPlaceObject() || aStorage.Is() )
			{
				DBG( DbgOutf( "SfxShellObject: BASIC-on-demand for %s",
							  pObjSh->SfxShell::GetName().GetBuffer() ) );
				pSbx = pObjSh->GetSbxObject();
				DBG_ASSERT( pSbx, "SfxShellObject: can't create SbxObject" );
			}
		}

		// die SbxObjects der SfxShells auf den Stacks der Frames erzeugen
		for ( SfxViewFrame *pFrame = SfxViewFrame::GetFirst(0,0,sal_False);
			  pFrame;
			  pFrame = SfxViewFrame::GetNext(*pFrame,0,0,sal_False) )
		{
			// den Dispatcher des Frames rausholen
			SfxDispatcher *pDispat = pFrame->GetDispatcher();
			pDispat->Flush();

			// "uber alle SfxShells auf dem Stack des Dispatchers iterieren
			// Frame selbst wird ausgespart, da er indirekt angezogen wird,
			// sofern er ein Dokument enth"alt.
			for ( sal_uInt16 nStackIdx = pDispat->GetShellLevel(*pFrame);
				  0 != nStackIdx;
				  --nStackIdx )
			{
				DBG( DbgOutf( "SfxShellObject: BASIC-on-demand for level %u", nStackIdx-1 ); )
				pSbx = pDispat->GetShell(nStackIdx - 1)->GetSbxObject();
				DBG_ASSERT( pSbx, "SfxShellObject: can't create SbxObject" );
			}

			if ( !pFrame->GetObjectShell() )
			{
				DBG( DbgOutf( "SfxShellObject: BASIC-on-demand for empty frame" ); )
				pSbx = pFrame->GetSbxObject();
				DBG_ASSERT( pSbx, "SfxShellObject: can't create SbxObject" );
			}
		}
*/
		// Factories anmelden
//        SbxBase::AddFactory( new SfxSbxObjectFactory_Impl );
	}
}

//--------------------------------------------------------------------

void SfxApplication::LeaveBasicCall()
{
	--pAppData_Impl->nBasicCallLevel;
}

//-------------------------------------------------------------------------
void SfxApplication::PropExec_Impl( SfxRequest &rReq )
{
	rReq.GetArgs();
	sal_uInt16 nSID = rReq.GetSlot();
	switch ( nSID )
	{
		case SID_CREATE_BASICOBJECT:
		{
			SFX_REQUEST_ARG(rReq, pItem, SfxStringItem, nSID, sal_False);
			if ( pItem )
			{
				SbxObject* pObject = SbxBase::CreateObject( pItem->GetValue() );
				pObject->AddRef();
//(mba)                rReq.SetReturnValue( SfxObjectItem( 0, pObject ) );
				rReq.Done();
			}
			break;
		}

		case SID_DELETE_BASICOBJECT:
		{
			SFX_REQUEST_ARG(rReq, pItem, SfxObjectItem, nSID, sal_False);
			if ( pItem )
			{
//(mba)                SbxObject* pObject = pItem->GetObject();
//(mba)                pObject->ReleaseRef();
			}
			break;
		}

		case SID_WIN_POSSIZE:
		{
			break;
		}

		case SID_INTERACTIVEMODE:
		{
			break;
		}

		case SID_ATTR_UNDO_COUNT:
		{
			SFX_REQUEST_ARG(rReq, pCountItem, SfxUInt16Item, nSID, sal_False);
            SvtUndoOptions().SetUndoCount( pCountItem->GetValue() );
			break;
		}

		case SID_WIN_VISIBLE:
		{
			break;
		}

		case SID_STATUSBARTEXT:
		{
			SFX_REQUEST_ARG(rReq, pStringItem, SfxStringItem, nSID, sal_False);
			String aText = pStringItem->GetValue();
			if ( aText.Len() )
				GetpApp()->ShowStatusText( aText );
			else
				GetpApp()->HideStatusText();
			break;
		}

		case SID_PLAYMACRO:
			PlayMacro_Impl( rReq, GetBasic() );
			break;

		case SID_OFFICE_PRIVATE_USE:
		case SID_OFFICE_COMMERCIAL_USE:
		{
			DBG_ASSERT( sal_False, "SfxApplication::PropExec_Impl()\nSID_OFFICE_PRIVATE_USE & SID_OFFICE_COMMERCIAL_USE are obsolete!\n" );
			break;
		}

		case SID_OFFICE_CUSTOMERNUMBER:
		{
			SFX_REQUEST_ARG(rReq, pStringItem, SfxStringItem, nSID, sal_False);

			if ( pStringItem )
				SvtUserOptions().SetCustomerNumber( pStringItem->GetValue() );
			break;
		}
	}
}

//-------------------------------------------------------------------------
void SfxApplication::PropState_Impl( SfxItemSet &rSet )
{
//	SfxViewFrame *pFrame = SfxViewFrame::Current();
	SfxWhichIter aIter(rSet);
	for ( sal_uInt16 nSID = aIter.FirstWhich(); nSID; nSID = aIter.NextWhich() )
	{
		switch ( nSID )
		{
			case SID_PROGNAME:
				rSet.Put( SfxStringItem( SID_PROGNAME, GetName() ) );
				break;

			case SID_ACTIVEDOCUMENT:
				rSet.Put( SfxObjectItem( SID_ACTIVEDOCUMENT, SfxObjectShell::Current() ) );
				break;

			case SID_APPLICATION:
                rSet.Put( SfxObjectItem( SID_APPLICATION, this ) );
				break;

			case SID_PROGFILENAME:
				rSet.Put( SfxStringItem( SID_PROGFILENAME, Application::GetAppFileName() ) );
				break;

			case SID_ATTR_UNDO_COUNT:
                rSet.Put( SfxUInt16Item( SID_ATTR_UNDO_COUNT, sal::static_int_cast< UINT16 >( SvtUndoOptions().GetUndoCount() ) ) );
				break;

			case SID_UPDATE_VERSION:
				rSet.Put( SfxUInt32Item( SID_UPDATE_VERSION, SUPD ) );
				break;

			case SID_BUILD_VERSION:
			{
				String aVersion = lcl_GetVersionString(pAppData_Impl->pLabelResMgr);
				rSet.Put( SfxUInt32Item( SID_BUILD_VERSION, (sal_uInt32) aVersion.ToInt32() ) );
				break;
			}

			case SID_OFFICE_PRIVATE_USE:
			case SID_OFFICE_COMMERCIAL_USE:
			{
				DBG_ASSERT( sal_False, "SfxApplication::PropState_Impl()\nSID_OFFICE_PRIVATE_USE & SID_OFFICE_COMMERCIAL_USE are obsolete!\n" );
				break;
			}

			case SID_OFFICE_CUSTOMERNUMBER:
			{
				rSet.Put( SfxStringItem( nSID, SvtUserOptions().GetCustomerNumber() ) );
				break;
			}
		}
	}
}

//--------------------------------------------------------------------
void SfxApplication::MacroExec_Impl( SfxRequest& rReq )
{
	DBG_MEMTEST();
	if ( SfxMacroConfig::IsMacroSlot( rReq.GetSlot() ) )
	{
		// SlotId referenzieren, damit nicht im Execute der Slot abgeschossen
		// werden kann
		GetMacroConfig()->RegisterSlotId(rReq.GetSlot());
		SFX_REQUEST_ARG(rReq, pArgs, SfxStringItem,
						rReq.GetSlot(), sal_False);
		String aArgs;
		if( pArgs ) aArgs = pArgs->GetValue();
		if ( GetMacroConfig()->ExecuteMacro(rReq.GetSlot(), aArgs ) )
			rReq.Done();
		GetMacroConfig()->ReleaseSlotId(rReq.GetSlot());
	}
}

//--------------------------------------------------------------------
void SfxApplication::MacroState_Impl( SfxItemSet& )
{
	DBG_MEMTEST();
}

//-------------------------------------------------------------------------

void SfxApplication::PlayMacro_Impl( SfxRequest &rReq, StarBASIC *pBasic )
{
	EnterBasicCall();
	sal_Bool bOK = sal_False;

	// Makro und asynch-Flag
	SFX_REQUEST_ARG(rReq,pMacro,SfxStringItem,SID_STATEMENT,sal_False);
	SFX_REQUEST_ARG(rReq,pAsynch,SfxBoolItem,SID_ASYNCHRON,sal_False);

	if ( pAsynch && pAsynch->GetValue() )
	{
		// asynchron ausf"uhren
		GetDispatcher_Impl()->Execute( SID_PLAYMACRO, SFX_CALLMODE_ASYNCHRON, pMacro, 0L );
		rReq.Done();
	}
	else if ( pMacro )
	{
		// Statement aufbereiten
		DBG_ASSERT( pBasic, "no BASIC found" ) ;
		String aStatement( '[' );
		aStatement += pMacro->GetValue();
		aStatement += ']';

		// P"aventiv den Request abschlie\sen, da er ggf. zerst"ort wird
		rReq.Done();
		rReq.ReleaseArgs();

		// Statement ausf"uhren
		pBasic->Execute( aStatement );
		bOK = 0 == SbxBase::GetError();
		SbxBase::ResetError();
	}

	LeaveBasicCall();
	rReq.SetReturnValue(SfxBoolItem(0,bOK));
}


