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

#include "arrdecl.hxx"

#include <cppuhelper/implbase1.hxx>

#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/util/XCloseBroadcaster.hpp>
#include <com/sun/star/util/XCloseListener.hpp>
#include <com/sun/star/util/XModifyBroadcaster.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XTitle.hpp>
#include <vos/mutex.hxx>

#ifndef _SV_RESARY_HXX
#include <tools/resary.hxx>
#endif
#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#ifndef _WRKWIN_HXX //autogen
#include <vcl/wrkwin.hxx>
#endif
#include <vcl/svapp.hxx>
#include <svtools/eitem.hxx>
#include <tools/rtti.hxx>
#include <svtools/lstner.hxx>
#include <sfxhelp.hxx>
#include <basic/sbstar.hxx>
#include <svtools/stritem.hxx>
#include <basic/sbx.hxx>
#include <svtools/eventcfg.hxx>

#include <sfx2/objsh.hxx>
#include <sfx2/signaturestate.hxx>

#ifndef _BASIC_SBUNO_HXX
#include <basic/sbuno.hxx>
#endif
#include <svtools/sfxecode.hxx>
#include <svtools/ehdl.hxx>
#include <svtools/printwarningoptions.hxx>
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif

#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <com/sun/star/script/DocumentDialogLibraryContainer.hpp>
#include <com/sun/star/script/DocumentScriptLibraryContainer.hpp>
#include <com/sun/star/document/XEmbeddedScripts.hpp>
#include <com/sun/star/document/XScriptInvocationContext.hpp>

#include <svtools/urihelper.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/sharecontrolfile.hxx>
#include <unotools/localfilehelper.hxx>
#include <unotools/ucbhelper.hxx>
#include <svtools/asynclink.hxx>
#include <tools/diagnose_ex.h>
#include <sot/clsids.hxx>

#include <sfx2/app.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/event.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/viewfrm.hxx>
#include "sfxresid.hxx"
#include "objshimp.hxx"
#include "appbas.hxx"
#include "sfxtypes.hxx"
#include <sfx2/evntconf.hxx>
#include <sfx2/request.hxx>
#include "doc.hrc"
#include "sfxlocal.hrc"
#include "appdata.hxx"
#include <sfx2/appuno.hxx>
#include <sfx2/sfxsids.hrc>
#include "basmgr.hxx"
#include "QuerySaveDocument.hxx"
#include "helpid.hrc"
#include <sfx2/msg.hxx>
#include "appbaslib.hxx"
#include <sfx2/sfxbasemodel.hxx>

#include <basic/basicmanagerrepository.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::document;

using ::basic::BasicManagerRepository;
#include <uno/mapping.hxx>

//====================================================================

DBG_NAME(SfxObjectShell)

#define DocumentInfo
#include "sfxslots.hxx"

extern svtools::AsynchronLink* pPendingCloser;
static WeakReference< XInterface > s_xCurrentComponent;

//=========================================================================


//=========================================================================

class SfxModelListener_Impl : public ::cppu::WeakImplHelper1< ::com::sun::star::util::XCloseListener >
{
    SfxObjectShell* mpDoc;
public:
    SfxModelListener_Impl( SfxObjectShell* pDoc ) : mpDoc(pDoc) {};
    virtual void SAL_CALL queryClosing( const com::sun::star::lang::EventObject& aEvent, sal_Bool bDeliverOwnership )
        throw ( com::sun::star::uno::RuntimeException, com::sun::star::util::CloseVetoException) ;
    virtual void SAL_CALL notifyClosing( const com::sun::star::lang::EventObject& aEvent ) throw ( com::sun::star::uno::RuntimeException ) ;
    virtual void SAL_CALL disposing( const com::sun::star::lang::EventObject& aEvent ) throw ( com::sun::star::uno::RuntimeException ) ;

};

void SAL_CALL SfxModelListener_Impl::queryClosing( const com::sun::star::lang::EventObject& , sal_Bool )
    throw ( com::sun::star::uno::RuntimeException, com::sun::star::util::CloseVetoException)
{
}

void SAL_CALL SfxModelListener_Impl::notifyClosing( const com::sun::star::lang::EventObject& ) throw ( com::sun::star::uno::RuntimeException )
{
    ::vos::OGuard aSolarGuard( Application::GetSolarMutex() );
    mpDoc->Broadcast( SfxSimpleHint(SFX_HINT_DEINITIALIZING) );
}

void SAL_CALL SfxModelListener_Impl::disposing( const com::sun::star::lang::EventObject& _rEvent ) throw ( com::sun::star::uno::RuntimeException )
{
    // am I ThisComponent in AppBasic?
    ::vos::OGuard aSolarGuard( Application::GetSolarMutex() );
    if ( SfxObjectShell::GetCurrentComponent() == _rEvent.Source )
    {
        // remove ThisComponent reference from AppBasic
        SfxObjectShell::SetCurrentComponent( Reference< XInterface >() );
    }

    if ( mpDoc->Get_Impl()->bHiddenLockedByAPI )
	{
        mpDoc->Get_Impl()->bHiddenLockedByAPI = FALSE;
        mpDoc->OwnerLock(FALSE);
	}
    else if ( !mpDoc->Get_Impl()->bClosing )
		// GCC stuerzt ab, wenn schon im dtor, also vorher Flag abfragen
        mpDoc->DoClose();
}

TYPEINIT1(SfxObjectShell, SfxShell);

//--------------------------------------------------------------------
SfxObjectShell_Impl::SfxObjectShell_Impl( SfxObjectShell& _rDocShell )
:mpObjectContainer(0)
    ,pAccMgr(0)
    ,pCfgMgr( 0)
    ,pBasicManager( new SfxBasicManagerHolder )
    ,rDocShell( _rDocShell )
    ,aMacroMode( *this )
    ,pProgress( 0)
    ,nTime()
    ,nVisualDocumentNumber( USHRT_MAX)
    ,nDocumentSignatureState( SIGNATURESTATE_UNKNOWN )
    ,nScriptingSignatureState( SIGNATURESTATE_UNKNOWN )
    ,bTemplateConfig( sal_False)
    ,bInList( sal_False)
    ,bClosing( sal_False)
    ,bSetInPlaceObj( sal_False)
    ,bIsSaving( sal_False)
    ,bPasswd( sal_False)
    ,bIsTmp( sal_False)
    ,bIsNamedVisible( sal_False)
    ,bIsTemplate(sal_False)
    ,bIsAbortingImport ( sal_False)
    ,bImportDone ( sal_False)
    ,bInPrepareClose( sal_False )
    ,bPreparedForClose( sal_False )
    ,bWaitingForPicklist( sal_False )
    ,bModuleSearched( sal_False )
    ,bIsHelpObjSh( sal_False )
    ,bForbidCaching( sal_False )
    ,bForbidReload( sal_False )
    ,bSupportsEventMacros( sal_True )
    ,bLoadingWindows( sal_False )
    ,bBasicInitialized( sal_False )
//    ,bHidden( sal_False )
    ,bIsPrintJobCancelable( sal_True )
    ,bOwnsStorage( sal_True )
    ,bNoBaseURL( sal_False )
    ,bInitialized( sal_False )
    ,bSignatureErrorIsShown( sal_False )
    ,bModelInitialized( sal_False )
    ,bPreserveVersions( sal_True )
    ,m_bMacroSignBroken( sal_False )
    ,m_bNoBasicCapabilities( sal_False )
    ,bQueryLoadTemplate( sal_True )
    ,bLoadReadonly( sal_False )
    ,bUseUserData( sal_True )
    ,bSaveVersionOnClose( sal_False )
    ,m_bSharedXMLFlag( sal_False )
    ,m_bAllowShareControlFileClean( sal_True )
    ,lErr(ERRCODE_NONE)
    ,nEventId ( 0)
    ,bDoNotTouchDocInfo( sal_False )
    ,pReloadTimer ( 0)
    ,pMarkData( 0 )
    ,nLoadedFlags ( SFX_LOADED_MAINDOCUMENT )
    ,nFlagsInProgress( 0 )
    ,bInFrame( sal_False )
    ,bModalMode( sal_False )
    ,bRunningMacro( sal_False )
    ,bReloadAvailable( sal_False )
    ,nAutoLoadLocks( 0 )
    ,pModule( 0 )
    ,pFrame( 0 )
    ,pTbxConfig( 0 )
    ,eFlags( SFXOBJECTSHELL_UNDEFINED )
    ,pCloser( 0 )
    ,bReadOnlyUI( sal_False )
    ,bHiddenLockedByAPI( sal_False )
    ,bInCloseEvent( sal_False )
    ,nStyleFilter( 0 )
    ,bDisposing( sal_False )
    ,m_bEnableSetModified( sal_True )
    ,m_bIsModified( sal_False )
    ,m_nMapUnit( MAP_100TH_MM )
    ,m_bCreateTempStor( sal_False )
    ,m_xDocInfoListener()
    ,m_bIsInit( sal_False )
    ,m_bIncomplEncrWarnShown( sal_False )
{
}

//--------------------------------------------------------------------

SfxObjectShell_Impl::~SfxObjectShell_Impl()
{
	if ( pPendingCloser == pCloser )
		pPendingCloser = 0;
	delete pCloser;
    delete pBasicManager;
}

// initializes a document from a file-description

SfxObjectShell::SfxObjectShell
(
	SfxObjectCreateMode	eMode	/*	Zweck, zu dem die SfxObjectShell
									erzeugt wird:

									SFX_CREATE_MODE_EMBEDDED (default)
										als SO-Server aus einem anderen
										Dokument heraus

									SFX_CREATE_MODE_STANDARD,
										als normales, selbst"aendig ge"offnetes
										Dokument

									SFX_CREATE_MODE_PREVIEW
										um ein Preview durchzuf"uhren,
										ggf. werden weniger Daten ben"otigt

									SFX_CREATE_MODE_ORGANIZER
										um im Organizer dargestellt zu
										werden, hier werden keine Inhalte
										ben"otigt */
)

/*	[Beschreibung]

	Konstruktor der Klasse SfxObjectShell.
*/

:	pImp( new SfxObjectShell_Impl( *this ) ),
	pMedium(0),
	pStyleSheetPool(0),
    eCreateMode(eMode)
{
	DBG_CTOR(SfxObjectShell, 0);

	bHasName = sal_False;
	nViewNo = 0;

	pImp->bWaitingForPicklist = sal_True;

	// Aggregation InPlaceObject+Automation
//(mba)    AddInterface( SvDispatch::ClassFactory() );

	SfxObjectShell *pThis = this;
	SfxObjectShellArr_Impl &rArr = SFX_APP()->GetObjectShells_Impl();
	rArr.C40_INSERT( SfxObjectShell, pThis, rArr.Count() );
	pImp->bInList = sal_True;
	pImp->nLoadedFlags = SFX_LOADED_ALL;
//REMOVE		SetObjectShell( TRUE );
}

//--------------------------------------------------------------------

// virtual dtor of typical base-class SfxObjectShell

SfxObjectShell::~SfxObjectShell()
{
	DBG_DTOR(SfxObjectShell, 0);

	if ( IsEnableSetModified() )
		EnableSetModified( sal_False );

	// Niemals GetInPlaceObject() aufrufen, der Zugriff auf den
	// Ableitungszweig SfxInternObject ist wegen eines Compiler Bugs nicht
	// erlaubt
	SfxObjectShell::Close();
    pImp->xModel = NULL;

//  DELETEX(pImp->pEventConfig);
//    DELETEX(pImp->pTbxConfig);
//    DELETEX(pImp->pAccMgr);
//    DELETEX(pImp->pCfgMgr);
    DELETEX(pImp->pReloadTimer );

	SfxApplication *pSfxApp = SFX_APP();
	if ( USHRT_MAX != pImp->nVisualDocumentNumber )
		pSfxApp->ReleaseIndex(pImp->nVisualDocumentNumber);

	// Basic-Manager zerst"oren
	pImp->pBasicManager->reset( NULL );

	if ( pSfxApp->GetDdeService() )
		pSfxApp->RemoveDdeTopic( this );

	if ( pImp->xModel.is() )
		pImp->xModel = ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModel > ();

    // don't call GetStorage() here, in case of Load Failure it's possible that a storage was never assigned!
    if ( pMedium && pMedium->HasStorage_Impl() && pMedium->GetStorage( sal_False ) == pImp->m_xDocStorage )
        pMedium->CanDisposeStorage_Impl( sal_False );

    if ( pImp->mpObjectContainer )
    {
        pImp->mpObjectContainer->CloseEmbeddedObjects();
        delete pImp->mpObjectContainer;
    }

    if ( pImp->bOwnsStorage && pImp->m_xDocStorage.is() )
    	pImp->m_xDocStorage->dispose();

	if ( pMedium )
	{
		pMedium->CloseAndReleaseStreams_Impl();

        if ( IsDocShared() )
            FreeSharedFile();

		DELETEX( pMedium );
	}

	// The removing of the temporary file must be done as the latest step in the document destruction
    if ( pImp->aTempName.Len() )
    {
        String aTmp;
        ::utl::LocalFileHelper::ConvertPhysicalNameToURL( pImp->aTempName, aTmp );
        ::utl::UCBContentHelper::Kill( aTmp );
    }

    delete pImp;
}

//--------------------------------------------------------------------

void SfxObjectShell::Stamp_SetPrintCancelState(sal_Bool bState)
{
    pImp->bIsPrintJobCancelable = bState;
}

//--------------------------------------------------------------------

sal_Bool SfxObjectShell::Stamp_GetPrintCancelState() const
{
    return pImp->bIsPrintJobCancelable;
}

//--------------------------------------------------------------------

void SfxObjectShell::ViewAssigned()

/*	[Beschreibung]

	Diese Methode wird gerufen, wenn eine View zugewiesen wird.
*/

{
}

//--------------------------------------------------------------------
// closes the Object and all its views

sal_Bool SfxObjectShell::Close()
{
	{DBG_CHKTHIS(SfxObjectShell, 0);}
    SfxObjectShellRef aRef(this);
	if ( !pImp->bClosing )
	{
		// falls noch ein Progress l"auft, nicht schlie\sen
		if ( !pImp->bDisposing && GetProgress() )
			return sal_False;

		pImp->bClosing = sal_True;
		Reference< util::XCloseable > xCloseable( GetBaseModel(), UNO_QUERY );

		if ( xCloseable.is() )
		{
			try
			{
				xCloseable->close( sal_True );
			}
			catch( Exception& )
			{
				pImp->bClosing = sal_False;
			}
		}

		if ( pImp->bClosing )
		{
			// aus Document-Liste austragen
			SfxApplication *pSfxApp = SFX_APP();
			SfxObjectShellArr_Impl &rDocs = pSfxApp->GetObjectShells_Impl();
			const SfxObjectShell *pThis = this;
			sal_uInt16 nPos = rDocs.GetPos(pThis);
			if ( nPos < rDocs.Count() )
				rDocs.Remove( nPos );
			pImp->bInList = sal_False;
		}
	}

	return sal_True;
}

//--------------------------------------------------------------------

// returns a pointer the first SfxDocument of specified type

SfxObjectShell* SfxObjectShell::GetFirst
(
	const TypeId* pType ,
	sal_Bool 			bOnlyVisible
)
{
	SfxObjectShellArr_Impl &rDocs = SFX_APP()->GetObjectShells_Impl();

	// seach for a SfxDocument of the specified type
	for ( sal_uInt16 nPos = 0; nPos < rDocs.Count(); ++nPos )
	{
		SfxObjectShell* pSh = rDocs.GetObject( nPos );
		if ( bOnlyVisible && pSh->IsPreview() && pSh->IsReadOnly() )
			continue;

		if ( ( !pType || pSh->IsA(*pType) ) &&
			 ( !bOnlyVisible || SfxViewFrame::GetFirst( pSh, 0, sal_True )))
			return pSh;
	}

	return 0;
}
//--------------------------------------------------------------------

// returns a pointer to the next SfxDocument of specified type behind *pDoc

SfxObjectShell* SfxObjectShell::GetNext
(
	const SfxObjectShell& 	rPrev,
	const TypeId* 			pType,
	sal_Bool 					bOnlyVisible
)
{
	SfxObjectShellArr_Impl &rDocs = SFX_APP()->GetObjectShells_Impl();

	// refind the specified predecessor
	sal_uInt16 nPos;
	for ( nPos = 0; nPos < rDocs.Count(); ++nPos )
		if ( rDocs.GetObject(nPos) == &rPrev )
			break;

	// search for the next SfxDocument of the specified type
	for ( ++nPos; nPos < rDocs.Count(); ++nPos )
	{
		SfxObjectShell* pSh = rDocs.GetObject( nPos );
		if ( bOnlyVisible && pSh->IsPreview() && pSh->IsReadOnly() )
			continue;

		if ( ( !pType || pSh->IsA(*pType) ) &&
			 ( !bOnlyVisible || SfxViewFrame::GetFirst( pSh, 0, sal_True )))
			return pSh;
	}
	return 0;
}

//--------------------------------------------------------------------

SfxObjectShell* SfxObjectShell::Current()
{
    SfxViewFrame *pFrame = SfxViewFrame::Current();
	return pFrame ? pFrame->GetObjectShell() : 0;
}

//------------------------------------------------------------------------

struct BoolEnv_Impl
{
	SfxObjectShell_Impl* pImp;
	BoolEnv_Impl( SfxObjectShell_Impl* pImpP) : pImp( pImpP )
	{ pImpP->bInPrepareClose = sal_True; }
	~BoolEnv_Impl() { pImp->bInPrepareClose = sal_False; }
};


sal_uInt16 SfxObjectShell::PrepareClose
(
	sal_Bool	bUI,		// sal_True: Dialoge etc. erlaubt, sal_False: silent-mode
	sal_Bool	bForBrowsing
)
{
	if( pImp->bInPrepareClose || pImp->bPreparedForClose )
		return sal_True;
	BoolEnv_Impl aBoolEnv( pImp );

	// DocModalDialog?
	if ( IsInModalMode() )
		return sal_False;

	SfxViewFrame* pFirst = SfxViewFrame::GetFirst( this );
	if( pFirst && !pFirst->GetFrame()->PrepareClose_Impl( bUI, bForBrowsing ) )
		return sal_False;

	// prepare views for closing
	for ( SfxViewFrame* pFrm = SfxViewFrame::GetFirst(
		this, TYPE(SfxViewFrame));
		  pFrm; pFrm = SfxViewFrame::GetNext( *pFrm, this ) )
	{
		DBG_ASSERT(pFrm->GetViewShell(),"KeineShell");
		if ( pFrm->GetViewShell() )
		{
			sal_uInt16 nRet = pFrm->GetViewShell()->PrepareClose( bUI, bForBrowsing );
			if ( nRet != sal_True )
				return nRet;
		}
	}

    SfxApplication *pSfxApp = SFX_APP();
    pSfxApp->NotifyEvent( SfxEventHint(SFX_EVENT_PREPARECLOSEDOC, GlobalEventConfig::GetEventName(STR_EVENT_PREPARECLOSEDOC), this) );
    
    if( GetCreateMode() == SFX_CREATE_MODE_EMBEDDED )
	{
		pImp->bPreparedForClose = sal_True;
		return sal_True;
	}

	// ggf. nachfragen, ob gespeichert werden soll
		// nur fuer in sichtbaren Fenstern dargestellte Dokumente fragen
	SfxViewFrame *pFrame = SfxObjectShell::Current() == this
		? SfxViewFrame::Current() : SfxViewFrame::GetFirst( this );
	while ( pFrame && (pFrame->GetFrameType() & SFXFRAME_SERVER ) )
		pFrame = SfxViewFrame::GetNext( *pFrame, this );

	sal_Bool bClose = sal_False;
	if ( bUI && IsModified() )
	{
		if ( pFrame )
		{
			// minimierte restoren
            SfxFrame* pTop = pFrame->GetTopFrame();
            SfxViewFrame::SetViewFrame( pTop->GetCurrentViewFrame() );
            pFrame->GetFrame()->Appear();

			// fragen, ob gespeichert werden soll
			short nRet = RET_YES;
            //TODO/CLEANUP
            //brauchen wir UI=2 noch?
            //if( SfxApplication::IsPlugin() == sal_False || bUI == 2 )
			{
                //initiate help agent to inform about "print modifies the document"
                SvtPrintWarningOptions aPrintOptions;
                if (aPrintOptions.IsModifyDocumentOnPrintingAllowed() &&
                    HasName() && getDocProperties()->getPrintDate().Month > 0)
                {
                    SfxHelp::OpenHelpAgent(pFirst->GetFrame(), HID_CLOSE_WARNING);
                }
                const Reference< XTitle > xTitle(pImp->xModel, UNO_QUERY_THROW);
                const ::rtl::OUString     sTitle = xTitle->getTitle ();
				nRet = ExecuteQuerySaveDocument(&pFrame->GetWindow(),sTitle);
			}
			/*HACK for plugin::destroy()*/

			if ( RET_YES == nRet )
			{
				// per Dispatcher speichern
				const SfxPoolItem *pPoolItem;
				if ( IsSaveVersionOnClose() )
				{
                    SfxStringItem aItem( SID_DOCINFO_COMMENTS, String( SfxResId( STR_AUTOMATICVERSION ) ) );
                    SfxBoolItem aWarnItem( SID_FAIL_ON_WARNING, bUI );
                    const SfxPoolItem* ppArgs[] = { &aItem, &aWarnItem, 0 };
                    pPoolItem = pFrame->GetBindings().ExecuteSynchron( SID_SAVEDOC, ppArgs );
				}
				else
                {
                    SfxBoolItem aWarnItem( SID_FAIL_ON_WARNING, bUI );
                    const SfxPoolItem* ppArgs[] = { &aWarnItem, 0 };
                    pPoolItem = pFrame->GetBindings().ExecuteSynchron( SID_SAVEDOC, ppArgs );
                }

                if ( !pPoolItem || pPoolItem->ISA(SfxVoidItem) || ( pPoolItem->ISA(SfxBoolItem) && !( (const SfxBoolItem*) pPoolItem )->GetValue() ) )
					return sal_False;
				else
					bClose = sal_True;
			}
			else if ( RET_CANCEL == nRet )
				// abgebrochen
				return sal_False;
			else if ( RET_NEWTASK == nRet )
			{
				return RET_NEWTASK;
			}
			else
			{
				// Bei Nein nicht noch Informationlost
				bClose = sal_True;
			}
		}
	}

	// ggf. hinweisen, da\s unter Fremdformat gespeichert
	if( pMedium )
	{
		SFX_ITEMSET_ARG( pMedium->GetItemSet(), pIgnoreInformationLost,
						 SfxBoolItem, SID_DOC_IGNOREINFORMATIONLOST, sal_False);
		if( pIgnoreInformationLost && pIgnoreInformationLost->GetValue() )
			bUI = sal_False;
	}

	pImp->bPreparedForClose = sal_True;
	return sal_True;
}

//--------------------------------------------------------------------
namespace
{
    static BasicManager* lcl_getBasicManagerForDocument( const SfxObjectShell& _rDocument )
    {
        if ( !_rDocument.Get_Impl()->m_bNoBasicCapabilities )
        {
            if ( !_rDocument.Get_Impl()->bBasicInitialized )
                const_cast< SfxObjectShell& >( _rDocument ).InitBasicManager_Impl();
            return _rDocument.Get_Impl()->pBasicManager->get();
        }

        // assume we do not have Basic ourself, but we can refer to another
        // document which does (by our model's XScriptInvocationContext::getScriptContainer).
        // In this case, we return the BasicManager of this other document.

        OSL_ENSURE( !Reference< XEmbeddedScripts >( _rDocument.GetModel(), UNO_QUERY ).is(),
            "lcl_getBasicManagerForDocument: inconsistency: no Basic, but an XEmbeddedScripts?" );
        Reference< XModel > xForeignDocument;
        Reference< XScriptInvocationContext > xContext( _rDocument.GetModel(), UNO_QUERY );
        if ( xContext.is() )
        {
            xForeignDocument.set( xContext->getScriptContainer(), UNO_QUERY );
            OSL_ENSURE( xForeignDocument.is() && xForeignDocument != _rDocument.GetModel(),
                "lcl_getBasicManagerForDocument: no Basic, but providing ourself as script container?" );
        }

        BasicManager* pBasMgr = NULL;
        if ( xForeignDocument.is() )
            pBasMgr = ::basic::BasicManagerRepository::getDocumentBasicManager( xForeignDocument );

        return pBasMgr;
    }
}

//--------------------------------------------------------------------

BasicManager* SfxObjectShell::GetBasicManager() const
{
    BasicManager* pBasMgr = lcl_getBasicManagerForDocument( *this );
    if ( !pBasMgr )
        pBasMgr = SFX_APP()->GetBasicManager();
    return pBasMgr;
}

//--------------------------------------------------------------------

void SfxObjectShell::SetHasNoBasic()
{
    pImp->m_bNoBasicCapabilities = sal_True;
}

//--------------------------------------------------------------------

sal_Bool SfxObjectShell::HasBasic() const
{
    if ( pImp->m_bNoBasicCapabilities )
        return sal_False;

    if ( !pImp->bBasicInitialized )
        const_cast< SfxObjectShell* >( this )->InitBasicManager_Impl();

    return pImp->pBasicManager->isValid();
}

//--------------------------------------------------------------------
namespace
{
    const Reference< XLibraryContainer >&
    lcl_getOrCreateLibraryContainer( bool _bScript, Reference< XLibraryContainer >& _rxContainer,
        const Reference< XModel >& _rxDocument )
    {
        if ( !_rxContainer.is() )
        {
            try
            {
                Reference< XStorageBasedDocument > xStorageDoc( _rxDocument, UNO_QUERY );
                const Reference< XComponentContext > xContext(
                    ::comphelper::getProcessComponentContext() );
                _rxContainer.set (   _bScript
                                ?   DocumentScriptLibraryContainer::create(
                                        xContext, xStorageDoc )
                                :   DocumentDialogLibraryContainer::create(
                                        xContext, xStorageDoc )
                                ,   UNO_QUERY_THROW );
            }
            catch( const Exception& )
            {
        	    DBG_UNHANDLED_EXCEPTION();
            }
        }
        return _rxContainer;
    }
}

//--------------------------------------------------------------------

Reference< XLibraryContainer > SfxObjectShell::GetDialogContainer()
{
    if ( !pImp->m_bNoBasicCapabilities )
        return lcl_getOrCreateLibraryContainer( false, pImp->xDialogLibraries, GetModel() );

    BasicManager* pBasMgr = lcl_getBasicManagerForDocument( *this );
    if ( pBasMgr )
        return pBasMgr->GetDialogLibraryContainer().get();

    OSL_ENSURE( false, "SfxObjectShell::GetDialogContainer: falling back to the application - is this really expected here?" );
    return SFX_APP()->GetDialogContainer();
}

//--------------------------------------------------------------------

Reference< XLibraryContainer > SfxObjectShell::GetBasicContainer()
{
    if ( !pImp->m_bNoBasicCapabilities )
        return lcl_getOrCreateLibraryContainer( true, pImp->xBasicLibraries, GetModel() );

    BasicManager* pBasMgr = lcl_getBasicManagerForDocument( *this );
    if ( pBasMgr )
        return pBasMgr->GetScriptLibraryContainer().get();

    OSL_ENSURE( false, "SfxObjectShell::GetBasicContainer: falling back to the application - is this really expected here?" );
    return SFX_APP()->GetBasicContainer();
}

//--------------------------------------------------------------------

StarBASIC* SfxObjectShell::GetBasic() const
{
	return GetBasicManager()->GetLib(0);
}

//--------------------------------------------------------------------

void SfxObjectShell::InitBasicManager_Impl()
/*	[Beschreibung]

    creates a document's BasicManager and loads it, if we are already based on
    a storage.

	[Anmerkung]

	Diese Methode mu"s aus den "Uberladungen von <SvPersist::Load()> (mit
	dem pStor aus dem Parameter von Load()) sowie aus der "Uberladung
	von <SvPersist::InitNew()> (mit pStor = 0) gerufen werden.
*/

{
    DBG_ASSERT( !pImp->bBasicInitialized && !pImp->pBasicManager->isValid(), "Lokaler BasicManager bereits vorhanden");
    pImp->bBasicInitialized = TRUE;

    pImp->pBasicManager->reset( BasicManagerRepository::getDocumentBasicManager( GetModel() ) );
    DBG_ASSERT( pImp->pBasicManager->isValid(), "SfxObjectShell::InitBasicManager_Impl: did not get a BasicManager!" );
}

//--------------------------------------------------------------------
#if 0 //(mba)
SotObjectRef SfxObjectShell::CreateAggObj( const SotFactory* pFact )
{
	// SvDispatch?
	SotFactory* pDispFact = SvDispatch::ClassFactory();
	if( pFact == pDispFact )
		return( (SfxShellObject*)GetSbxObject() );

	// sonst unbekannte Aggregation
	DBG_ERROR("unkekannte Factory");
	SotObjectRef aSvObjectRef;
	return aSvObjectRef;
}
#endif

//--------------------------------------------------------------------

sal_uInt16 SfxObjectShell::Count()
{
	return SFX_APP()->GetObjectShells_Impl().Count();
}

//--------------------------------------------------------------------

sal_Bool SfxObjectShell::DoClose()
{
	return Close();
}

//--------------------------------------------------------------------

void SfxObjectShell::SetLastMark_Impl( const String &rMark )
{
	pImp->aMark = rMark;
}

//--------------------------------------------------------------------

const String& SfxObjectShell::GetLastMark_Impl() const
{
	return pImp->aMark;
}

//--------------------------------------------------------------------

SfxObjectShell* SfxObjectShell::GetObjectShell()
{
	return this;
}

//--------------------------------------------------------------------

SEQUENCE< OUSTRING > SfxObjectShell::GetEventNames()
{
	static uno::Sequence< ::rtl::OUString >* pEventNameContainer = NULL;

	if ( !pEventNameContainer )
	{
    	::vos::OGuard aGuard( Application::GetSolarMutex() );
		if ( !pEventNameContainer )
		{
			static uno::Sequence< ::rtl::OUString > aEventNameContainer = GlobalEventConfig().getElementNames();
			pEventNameContainer = &aEventNameContainer;
		}
	}

	return *pEventNameContainer;
}

SEQUENCE< OUSTRING > SfxObjectShell::GetEventNames_Impl()
{
	if (!pImp->xEventNames.getLength())
		pImp->xEventNames = GetEventNames();
	return pImp->xEventNames;
}

//--------------------------------------------------------------------

void SfxObjectShell::SetModel( SfxBaseModel* pModel )
{
	OSL_ENSURE( !pImp->xModel.is() || pModel == NULL, "Model already set!" );
    pImp->xModel = pModel;
    if ( pModel ) {
        pModel->addCloseListener( new SfxModelListener_Impl(this) );
        //pImp->m_xDocInfoListener = new SfxDocInfoListener_Impl(*this);
        //uno::Reference<util::XModifyBroadcaster> xMB(
        //    pModel->getDocumentProperties(), uno::UNO_QUERY_THROW);
        //xMB->addModifyListener(pImp->m_xDocInfoListener);
    }
}

//--------------------------------------------------------------------

const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModel >& SfxObjectShell::GetModel() const
{
	return pImp->xModel;
}

void SfxObjectShell::SetBaseModel( SfxBaseModel* pModel )
{
    SetModel(pModel);
}

//--------------------------------------------------------------------

::com::sun::star::uno::Reference< ::com::sun::star::frame::XModel > SfxObjectShell::GetBaseModel()
{
	return pImp->xModel;
}
/* -----------------------------10.09.2001 15:56------------------------------

 ---------------------------------------------------------------------------*/
void SfxObjectShell::SetAutoStyleFilterIndex(sal_uInt16 nSet)
{
    pImp->nStyleFilter = nSet;
}

sal_uInt16 SfxObjectShell::GetAutoStyleFilterIndex()
{
    return pImp->nStyleFilter;
}


void SfxObjectShell::SetCurrentComponent( const Reference< XInterface >& _rxComponent )
{
    Reference< XInterface > xTest(s_xCurrentComponent);
    if ( _rxComponent == xTest )
        // nothing to do
        return;
    // note that "_rxComponent.get() == s_xCurrentComponent.get().get()" is /sufficient/, but not
    // /required/ for "_rxComponent == s_xCurrentComponent.get()".
    // In other words, it's still possible that we here do something which is not necessary,
    // but we should have filtered quite some unnecessary calls already.

    BasicManager* pAppMgr = SFX_APP()->GetBasicManager();
    s_xCurrentComponent = _rxComponent;
    if ( pAppMgr )
        pAppMgr->SetGlobalUNOConstant( "ThisComponent", makeAny( _rxComponent ) );

#if OSL_DEBUG_LEVEL > 0
    const char* pComponentImplName = _rxComponent.get() ? typeid( *_rxComponent.get() ).name() : "void";
    OSL_TRACE( "current component is a %s\n", pComponentImplName );
#endif
}

Reference< XInterface > SfxObjectShell::GetCurrentComponent()
{
	return s_xCurrentComponent;
}


String SfxObjectShell::GetServiceNameFromFactory( const String& rFact )
{
	//! Remove everything behind name!
	String aFact( rFact );
	String aPrefix = String::CreateFromAscii( "private:factory/" );
	if ( aPrefix.Len() == aFact.Match( aPrefix ) )
		aFact.Erase( 0, aPrefix.Len() );
	USHORT nPos = aFact.Search( '?' );
	String aParam;
	if ( nPos != STRING_NOTFOUND )
	{
		aParam = aFact.Copy( nPos, aFact.Len() );
		aFact.Erase( nPos, aFact.Len() );
		aParam.Erase(0,1);
	}
	aFact.EraseAllChars('4').ToLowerAscii();

    // HACK: sometimes a real document service name is given here instead of
    // a factory short name. Set return value directly to this service name as fallback
    // in case next lines of code does nothing ...
    // use rFact instead of normed aFact value !
	::rtl::OUString aServiceName = rFact;

	if ( aFact.EqualsAscii("swriter") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.text.TextDocument");
	}
	else if ( aFact.EqualsAscii("sweb") || aFact.EqualsAscii("swriter/web") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.text.WebDocument");
	}
	else if ( aFact.EqualsAscii("sglobal") || aFact.EqualsAscii("swriter/globaldocument") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.text.GlobalDocument");
	}
	else if ( aFact.EqualsAscii("scalc") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.sheet.SpreadsheetDocument");
	}
	else if ( aFact.EqualsAscii("sdraw") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.drawing.DrawingDocument");
	}
	else if ( aFact.EqualsAscii("simpress") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.presentation.PresentationDocument");
	}
	else if ( aFact.EqualsAscii("schart") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.chart.ChartDocument");
	}
	else if ( aFact.EqualsAscii("smath") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.formula.FormulaProperties");
	}
	else if ( aFact.EqualsAscii("sbasic") )
	{
		aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.script.BasicIDE");
	}
    else if ( aFact.EqualsAscii("sdatabase") )
    {
        aServiceName = ::rtl::OUString::createFromAscii("com.sun.star.sdb.OfficeDatabaseDocument");
    }

	return aServiceName;
}

SfxObjectShell* SfxObjectShell::CreateObjectByFactoryName( const String& rFact, SfxObjectCreateMode eMode )
{
	return CreateObject( GetServiceNameFromFactory( rFact ), eMode );
}


SfxObjectShell* SfxObjectShell::CreateObject( const String& rServiceName, SfxObjectCreateMode eCreateMode )
{
    if ( rServiceName.Len() )
    {
        ::com::sun::star::uno::Reference < ::com::sun::star::frame::XModel > xDoc(
        ::comphelper::getProcessServiceFactory()->createInstance( rServiceName ), UNO_QUERY );
        if ( xDoc.is() )
        {
            ::com::sun::star::uno::Reference < ::com::sun::star::lang::XUnoTunnel > xObj( xDoc, UNO_QUERY );
            ::com::sun::star::uno::Sequence < sal_Int8 > aSeq( SvGlobalName( SFX_GLOBAL_CLASSID ).GetByteSequence() );
            sal_Int64 nHandle = xObj->getSomething( aSeq );
            if ( nHandle )
            {
                SfxObjectShell* pRet = reinterpret_cast< SfxObjectShell* >( sal::static_int_cast< sal_IntPtr >( nHandle ));
                pRet->SetCreateMode_Impl( eCreateMode );
                return pRet;
            }
        }
    }

    return 0;
}

SfxObjectShell* SfxObjectShell::CreateAndLoadObject( const SfxItemSet& rSet, SfxFrame* pFrame )
{
    uno::Sequence < beans::PropertyValue > aProps;
    TransformItems( SID_OPENDOC, rSet, aProps );
    SFX_ITEMSET_ARG(&rSet, pFileNameItem, SfxStringItem, SID_FILE_NAME, FALSE);
    SFX_ITEMSET_ARG(&rSet, pTargetItem, SfxStringItem, SID_TARGETNAME, FALSE);
    ::rtl::OUString aURL;
    ::rtl::OUString aTarget = rtl::OUString::createFromAscii("_blank");
    if ( pFileNameItem )
        aURL = pFileNameItem->GetValue();
    if ( pTargetItem )
        aTarget = pTargetItem->GetValue();

    uno::Reference < frame::XComponentLoader > xLoader;
    if ( pFrame )
    {
        xLoader = uno::Reference < frame::XComponentLoader >( pFrame->GetFrameInterface(), uno::UNO_QUERY );
    }
    else
        xLoader = uno::Reference < frame::XComponentLoader >( comphelper::getProcessServiceFactory()->createInstance(
            ::rtl::OUString::createFromAscii("com.sun.star.frame.Desktop") ), uno::UNO_QUERY );

    uno::Reference < lang::XUnoTunnel > xObj;
	try
	{
		xObj = uno::Reference< lang::XUnoTunnel >( xLoader->loadComponentFromURL( aURL, aTarget, 0, aProps ), uno::UNO_QUERY );
	}
	catch( uno::Exception& )
	{}

    if ( xObj.is() )
    {
        ::com::sun::star::uno::Sequence < sal_Int8 > aSeq( SvGlobalName( SFX_GLOBAL_CLASSID ).GetByteSequence() );
        sal_Int64 nHandle = xObj->getSomething( aSeq );
        if ( nHandle )
            return reinterpret_cast< SfxObjectShell* >(sal::static_int_cast< sal_IntPtr >(  nHandle ));
    }

    return NULL;
}

