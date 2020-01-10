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
#include <svtools/stritem.hxx>
#include <svtools/eitem.hxx>
#include <svtools/whiter.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/toolbox.hxx>
#include <svtools/intitem.hxx>
#include <svtools/sfxecode.hxx>
#include <svtools/ehdl.hxx>
#include <com/sun/star/frame/XLayoutManager.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/embed/EmbedStates.hpp>
#include <com/sun/star/embed/EmbedMisc.hpp>
#include <com/sun/star/system/XSystemShellExecute.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#include <com/sun/star/container/XContainerQuery.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardNotifier.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <cppuhelper/implbase1.hxx>

#include <osl/file.hxx>
#include <vos/mutex.hxx>
#include <tools/urlobj.hxx>
#include <unotools/tempfile.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/miscopt.hxx>
#include <svtools/soerr.hxx>
#include <svtools/internaloptions.hxx>

#include <svtools/javaoptions.hxx>
#include <basic/basmgr.hxx>
#include <basic/sbuno.hxx>
#include <framework/actiontriggerhelper.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <toolkit/unohlp.hxx>


#include <sfx2/app.hxx>
#include "view.hrc"
#include <sfx2/viewsh.hxx>
#include "viewimp.hxx"
#include "sfxresid.hxx"
#include <sfx2/request.hxx>
#include <sfx2/templdlg.hxx>
#include <sfx2/printer.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/dispatch.hxx>
#include "arrdecl.hxx"
#include <sfx2/docfac.hxx>
#include "view.hrc"
#include "sfxlocal.hrc"
#include <sfx2/sfxbasecontroller.hxx>
#include <sfx2/topfrm.hxx>
#include "mailmodelapi.hxx"
#include <sfx2/event.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/ipclient.hxx>
#include "workwin.hxx"
#include <sfx2/objface.hxx>
#include <sfx2/docfilt.hxx>

// #110897#
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::system;
using namespace ::cppu;
namespace css = ::com::sun::star;

//=========================================================================
DBG_NAME(SfxViewShell)

#define SfxViewShell
#include "sfxslots.hxx"

//=========================================================================

class SfxClipboardChangeListener : public ::cppu::WeakImplHelper1<
	datatransfer::clipboard::XClipboardListener >
{
	SfxViewShell* pViewShell;

	// XEventListener
	virtual void SAL_CALL disposing( const lang::EventObject& rEventObject )
		throw ( uno::RuntimeException );

	// XClipboardListener
	virtual void SAL_CALL changedContents( const datatransfer::clipboard::ClipboardEvent& rEventObject )
		throw ( uno::RuntimeException );

public:
	SfxClipboardChangeListener( SfxViewShell* pView );
	virtual	~SfxClipboardChangeListener();

    void DisconnectViewShell() { pViewShell = NULL; }
};

SfxClipboardChangeListener::SfxClipboardChangeListener( SfxViewShell* pView )
: pViewShell( 0 )
{
	uno::Reference < lang::XComponent > xCtrl( pView->GetController(), uno::UNO_QUERY );
	if ( xCtrl.is() )
	{
		xCtrl->addEventListener( uno::Reference < lang::XEventListener > ( static_cast < lang::XEventListener* >( this ) ) );
		pViewShell = pView;
	}
}

SfxClipboardChangeListener::~SfxClipboardChangeListener()
{
}

void SAL_CALL SfxClipboardChangeListener::disposing( const lang::EventObject& /*rEventObject*/ )
throw ( uno::RuntimeException )
{
	// either clipboard or ViewShell is going to be destroyed -> no interest in listening anymore
    const ::vos::OGuard aGuard( Application::GetSolarMutex() );
	if ( pViewShell )
	{
		uno::Reference < lang::XComponent > xCtrl( pViewShell->GetController(), uno::UNO_QUERY );
		if ( xCtrl.is() )
			xCtrl->removeEventListener( uno::Reference < lang::XEventListener > ( static_cast < lang::XEventListener* >( this ) ) );
		pViewShell->AddRemoveClipboardListener( uno::Reference < datatransfer::clipboard::XClipboardListener > (this), FALSE );
		pViewShell = 0;
	}
}

void SAL_CALL SfxClipboardChangeListener::changedContents( const datatransfer::clipboard::ClipboardEvent& )
	throw ( RuntimeException )
{
    const ::vos::OGuard aGuard( Application::GetSolarMutex() );
    if( pViewShell )
	{
		SfxBindings& rBind = pViewShell->GetViewFrame()->GetBindings();
		rBind.Invalidate( SID_PASTE );
        rBind.Invalidate( SID_PASTE_SPECIAL );
        rBind.Invalidate( SID_CLIPBOARD_FORMAT_ITEMS );
	}
}


static ::rtl::OUString RetrieveLabelFromCommand( 
    const ::rtl::OUString& rCommandURL, 
    const css::uno::Reference< css::frame::XFrame >& rFrame )
{
    static css::uno::WeakReference< frame::XModuleManager > s_xModuleManager;
    static css::uno::WeakReference< container::XNameAccess > s_xNameAccess;

    ::rtl::OUString aLabel;
    css::uno::Reference< css::frame::XModuleManager > xModuleManager( s_xModuleManager );
    css::uno::Reference< css::container::XNameAccess > xNameAccess( s_xNameAccess );
    css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR( 
        ::comphelper::getProcessServiceFactory(), css::uno::UNO_QUERY_THROW);
        
    try
    {
        if ( !xModuleManager.is() )
        {
            xModuleManager = css::uno::Reference< css::frame::XModuleManager >( 
                xSMGR->createInstance( 
                    ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.frame.ModuleManager" ))), 
                    css::uno::UNO_QUERY_THROW );
            s_xModuleManager = xModuleManager;
        }
    
        ::rtl::OUString aModuleIdentifier = xModuleManager->identify( rFrame );
        
        if ( !xNameAccess.is() )
        {
            xNameAccess = css::uno::Reference< css::container::XNameAccess >( 
                xSMGR->createInstance( 
                    ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.frame.UICommandDescription" ))), 
                    css::uno::UNO_QUERY_THROW );
            s_xNameAccess = xNameAccess;
        }
        
        css::uno::Any a = xNameAccess->getByName( aModuleIdentifier );
        css::uno::Reference< css::container::XNameAccess > xUICommands;
        a >>= xUICommands;

        rtl::OUString aStr;
        css::uno::Sequence< css::beans::PropertyValue > aPropSeq;
        
        a = xUICommands->getByName( rCommandURL );
        if ( a >>= aPropSeq )
        {
            for ( sal_Int32 i = 0; i < aPropSeq.getLength(); i++ )
            {
                if ( aPropSeq[i].Name.equalsAscii( "Label" ))
                {
                    aPropSeq[i].Value >>= aStr;
                    break;
                }
            }
            aLabel = aStr;
        }
    }
    catch ( css::uno::Exception& )
    {
    }
    
    return aLabel;
}

//=========================================================================
SfxViewShell_Impl::SfxViewShell_Impl()
: aInterceptorContainer( aMutex )
, pAccExec(0)
{}

//=========================================================================
SFX_IMPL_INTERFACE(SfxViewShell,SfxShell,SfxResId(0))
{
	SFX_CHILDWINDOW_REGISTRATION( SID_MAIL_CHILDWIN );
}

TYPEINIT2(SfxViewShell,SfxShell,SfxListener);

//--------------------------------------------------------------------
/** search for a filter name dependent on type and module
 */

static ::rtl::OUString impl_retrieveFilterNameFromTypeAndModule(
    const css::uno::Reference< css::container::XContainerQuery >& rContainerQuery,
    const ::rtl::OUString& rType,
    const ::rtl::OUString& rModuleIdentifier,
    const sal_Int32 nFlags )
{
    // Retrieve filter from type
    css::uno::Sequence< css::beans::NamedValue > aQuery( 2 );
    aQuery[0].Name  = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Type" ));
    aQuery[0].Value = css::uno::makeAny( rType );
    aQuery[1].Name  = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "DocumentService" ));
    aQuery[1].Value = css::uno::makeAny( rModuleIdentifier );

    css::uno::Reference< css::container::XEnumeration > xEnumeration =
        rContainerQuery->createSubSetEnumerationByProperties( aQuery );

    ::rtl::OUString aFoundFilterName;
    while ( xEnumeration->hasMoreElements() )
    {
        ::comphelper::SequenceAsHashMap aFilterPropsHM( xEnumeration->nextElement() );
        ::rtl::OUString aFilterName = aFilterPropsHM.getUnpackedValueOrDefault(
                                    ::rtl::OUString::createFromAscii( "Name" ),
                                    ::rtl::OUString() );

        sal_Int32 nFilterFlags = aFilterPropsHM.getUnpackedValueOrDefault(
                                    ::rtl::OUString::createFromAscii( "Flags" ),
                                    sal_Int32( 0 ) );

        if ( nFilterFlags & nFlags )
        {
            aFoundFilterName = aFilterName;
            break;
        }
    }

    return aFoundFilterName;
}

//--------------------------------------------------------------------
/** search for an internal typename, which map to the current app module
    and map also to a "family" of file formats as e.g. PDF/MS Doc/OOo Doc.
 */
enum ETypeFamily
{
    E_MS_DOC,
    E_OOO_DOC
};

::rtl::OUString impl_searchFormatTypeForApp(const css::uno::Reference< css::frame::XFrame >& xFrame     ,
                                                  ETypeFamily                                eTypeFamily)
{
    static ::rtl::OUString SERVICENAME_MODULEMANAGER = ::rtl::OUString::createFromAscii("com.sun.star.frame.ModuleManager");

    try
    {
        css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR         (::comphelper::getProcessServiceFactory()        , css::uno::UNO_QUERY_THROW);
        css::uno::Reference< css::frame::XModuleManager >      xModuleManager(xSMGR->createInstance(SERVICENAME_MODULEMANAGER), css::uno::UNO_QUERY_THROW);

        ::rtl::OUString sModule = xModuleManager->identify(xFrame);
        ::rtl::OUString sType   ;

        switch(eTypeFamily)
        {
            case E_MS_DOC:
                 {
                    if (sModule.equalsAscii( "com.sun.star.text.TextDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "writer_MS_Word_97" ));
                    else
                    if (sModule.equalsAscii( "com.sun.star.sheet.SpreadsheetDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "calc_MS_Excel_97" ));
                    else
                    if (sModule.equalsAscii( "com.sun.star.drawing.DrawingDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "impress_MS_PowerPoint_97" ));
                    else
                    if (sModule.equalsAscii( "com.sun.star.presentation.PresentationDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "impress_MS_PowerPoint_97" ));
                 }
                 break;

            case E_OOO_DOC:
                 {
                    if (sModule.equalsAscii( "com.sun.star.text.TextDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "writer8" ));
                    else
                    if (sModule.equalsAscii( "com.sun.star.sheet.SpreadsheetDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "calc8" ));
                    else
                    if (sModule.equalsAscii( "com.sun.star.drawing.DrawingDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "draw8" ));
                    else
                    if (sModule.equalsAscii( "com.sun.star.presentation.PresentationDocument" ))
                        sType = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "impress8" ));
                 }
                 break;
        }

        return sType;
    }
    catch(const css::uno::RuntimeException& exRun)
        { throw exRun; }
    catch(const css::uno::Exception&)
        {}

    return ::rtl::OUString();
}

//--------------------------------------------------------------------

void SfxViewShell::ExecMisc_Impl( SfxRequest &rReq )
{
	const USHORT nId = rReq.GetSlot();
	switch( nId )
	{
		case SID_STYLE_FAMILY :
		{
			SFX_REQUEST_ARG(rReq, pItem, SfxUInt16Item, nId, FALSE);
			if (pItem)
				pImp->nFamily = pItem->GetValue();
			break;
		}

		case SID_STYLE_CATALOG:
		{
			SfxTemplateCatalog aCatalog(
				SFX_APP()->GetTopWindow(), &GetViewFrame()->GetBindings());
			aCatalog.Execute();
            rReq.Ignore();
			break;
		}
        case SID_ACTIVATE_STYLE_APPLY:
        {
            com::sun::star::uno::Reference< com::sun::star::frame::XFrame > xFrame(
                    GetViewFrame()->GetFrame()->GetFrameInterface(),
                    com::sun::star::uno::UNO_QUERY);

            Reference< com::sun::star::beans::XPropertySet > xPropSet( xFrame, UNO_QUERY );
            Reference< ::com::sun::star::frame::XLayoutManager > xLayoutManager;
            if ( xPropSet.is() )
            {
                try
                {
                    Any aValue = xPropSet->getPropertyValue( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "LayoutManager" )));
                    aValue >>= xLayoutManager;
                    if ( xLayoutManager.is() )
                    {
                        rtl::OUString aTextResString( RTL_CONSTASCII_USTRINGPARAM( "private:resource/toolbar/textobjectbar" ));
                        uno::Reference< ui::XUIElement > xElement = xLayoutManager->getElement( aTextResString );
                        if(!xElement.is())
                        {
                            rtl::OUString aFrameResString( RTL_CONSTASCII_USTRINGPARAM( "private:resource/toolbar/frameobjectbar" ));
                            xElement = xLayoutManager->getElement( aFrameResString );
                        }
                        if(!xElement.is())
                        {
                            rtl::OUString aOleResString( RTL_CONSTASCII_USTRINGPARAM( "private:resource/toolbar/oleobjectbar" ));
                            xElement = xLayoutManager->getElement( aOleResString );
                        }
                        if(xElement.is())
                        {
                            uno::Reference< awt::XWindow > xWin( xElement->getRealInterface(), uno::UNO_QUERY_THROW );
                            Window* pWin = VCLUnoHelper::GetWindow( xWin );
                            ToolBox* pTextToolbox = dynamic_cast< ToolBox* >( pWin );
                            if( pTextToolbox )
                            {
                                USHORT nItemCount = pTextToolbox->GetItemCount();
                                for( USHORT nItem = 0; nItem < nItemCount; ++nItem )
                                {
                                    USHORT nItemId = pTextToolbox->GetItemId( nItem );
                                    const XubString& rCommand = pTextToolbox->GetItemCommand( nItemId );
                                    if( rCommand.EqualsAscii( ".uno:StyleApply" ) )
                                    {
                                        Window* pItemWin = pTextToolbox->GetItemWindow( nItemId );
                                        if( pItemWin )
                                            pItemWin->GrabFocus();
                                        break;
                                    }    
                                }
                            }    
                        }
                    }
                }
                catch ( Exception& )
                {
                }
            }
            rReq.Done();
        }
        break;        
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

		case SID_MAIL_SENDDOCASMS:
		case SID_MAIL_SENDDOCASOOO:
		case SID_MAIL_SENDDOCASPDF:
		case SID_MAIL_SENDDOC:
        case SID_MAIL_SENDDOCASFORMAT:
		{
			SfxObjectShell* pDoc = GetObjectShell();
			if ( pDoc && pDoc->QueryHiddenInformation(
							WhenSaving, &GetViewFrame()->GetWindow() ) != RET_YES )
				break;

			if ( SvtInternalOptions().MailUIEnabled() )
            {
                GetViewFrame()->SetChildWindow( SID_MAIL_CHILDWIN, TRUE );
            }
            else
            {
				SfxMailModel  aModel;
                rtl::OUString aDocType;

				SFX_REQUEST_ARG(rReq, pMailSubject, SfxStringItem, SID_MAIL_SUBJECT, FALSE );
				if ( pMailSubject )
					aModel.SetSubject( pMailSubject->GetValue() );

				SFX_REQUEST_ARG(rReq, pMailRecipient, SfxStringItem, SID_MAIL_RECIPIENT, FALSE );
				if ( pMailRecipient )
				{
					String aRecipient( pMailRecipient->GetValue() );
					String aMailToStr( String::CreateFromAscii( "mailto:" ));

					if ( aRecipient.Search( aMailToStr ) == 0 )
						aRecipient = aRecipient.Erase( 0, aMailToStr.Len() );
					aModel.AddAddress( aRecipient, SfxMailModel::ROLE_TO );
				}
                SFX_REQUEST_ARG(rReq, pMailDocType, SfxStringItem, SID_TYPE_NAME, FALSE );
                if ( pMailDocType )
                    aDocType = pMailDocType->GetValue();

                uno::Reference < frame::XFrame > xFrame( pFrame->GetFrame()->GetFrameInterface() );
				SfxMailModel::SendMailResult eResult = SfxMailModel::SEND_MAIL_ERROR;

                if ( nId == SID_MAIL_SENDDOC )
					eResult = aModel.SaveAndSend( xFrame, rtl::OUString() );
				else
				if ( nId == SID_MAIL_SENDDOCASPDF )
                    eResult = aModel.SaveAndSend( xFrame, rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "pdf_Portable_Document_Format" )));
                else
				if ( nId == SID_MAIL_SENDDOCASMS )
                {
                    aDocType = impl_searchFormatTypeForApp(xFrame, E_MS_DOC);
                    if (aDocType.getLength() > 0)
                        eResult = aModel.SaveAndSend( xFrame, aDocType );
                }
                else
				if ( nId == SID_MAIL_SENDDOCASOOO )
                {
                    aDocType = impl_searchFormatTypeForApp(xFrame, E_OOO_DOC);
                    if (aDocType.getLength() > 0)
                        eResult = aModel.SaveAndSend( xFrame, aDocType );
                }

				if ( eResult == SfxMailModel::SEND_MAIL_ERROR )
				{
					InfoBox aBox( SFX_APP()->GetTopWindow(), SfxResId( MSG_ERROR_SEND_MAIL ));
					aBox.Execute();
                    rReq.Ignore();
				}
                else
                    rReq.Done();
			}

			break;
		}

 		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case SID_WEBHTML:
		{
            static const char HTML_DOCUMENT_TYPE[] = "writer_web_HTML";
            static const char HTML_GRAPHIC_TYPE[]  = "graphic_HTML";
            const sal_Int32   FILTERFLAG_EXPORT    = 0x00000002;

            css::uno::Reference< lang::XMultiServiceFactory > xSMGR(::comphelper::getProcessServiceFactory(), css::uno::UNO_QUERY_THROW);
            css::uno::Reference < css::frame::XFrame >        xFrame( pFrame->GetFrame()->GetFrameInterface() );
            css::uno::Reference< css::frame::XModel >         xModel;

            const rtl::OUString aModuleManager( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.frame.ModuleManager" ));
            css::uno::Reference< css::frame::XModuleManager > xModuleManager( xSMGR->createInstance( aModuleManager ), css::uno::UNO_QUERY_THROW );
            if ( !xModuleManager.is() )
            {
                rReq.Done(FALSE);
                return;
            }

            rtl::OUString aModule;
            try
            {
                 aModule = xModuleManager->identify( xFrame );
            }
            catch ( css::uno::RuntimeException& )
            {
                throw;
            }
            catch ( css::uno::Exception& )
            {
            }

            if ( xFrame.is() )
            {
                css::uno::Reference< css::frame::XController > xController = xFrame->getController();
                if ( xController.is() )
                    xModel = xController->getModel();
            }

            // We need at least a valid module name and model reference
            css::uno::Reference< css::frame::XStorable > xStorable( xModel, css::uno::UNO_QUERY );
            if ( xModel.is() && xStorable.is() )
            {
                rtl::OUString aFilterName;
                rtl::OUString aTypeName( RTL_CONSTASCII_USTRINGPARAM( HTML_DOCUMENT_TYPE ));
                rtl::OUString aFileName;
                rtl::OUString aExtension( RTL_CONSTASCII_USTRINGPARAM( "htm" ));

                rtl::OUString aLocation = xStorable->getLocation();
                INetURLObject aFileObj( aLocation );

                bool bPrivateProtocol = ( aFileObj.GetProtocol() == INET_PROT_PRIV_SOFFICE );
                bool bHasLocation = ( aLocation.getLength() > 0 ) && !bPrivateProtocol;

                css::uno::Reference< css::container::XContainerQuery > xContainerQuery(
                    xSMGR->createInstance( rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.document.FilterFactory" ))),
                        css::uno::UNO_QUERY_THROW );

                // Retrieve filter from type
                sal_Int32 nFilterFlags = FILTERFLAG_EXPORT;
                aFilterName = impl_retrieveFilterNameFromTypeAndModule( xContainerQuery, aTypeName, aModule, nFilterFlags );
                if ( aFilterName.getLength() == 0 )
                {
                    // Draw/Impress uses a different type. 2nd chance try to use alternative type name
                    aFilterName = impl_retrieveFilterNameFromTypeAndModule(
                        xContainerQuery, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( HTML_GRAPHIC_TYPE )), aModule, nFilterFlags );
                }

                // No filter found => error
                // No type and no location => error
                if (( aFilterName.getLength() == 0 ) || ( aTypeName.getLength() == 0 ))
                {
                    rReq.Done(FALSE);
                    return;
                }

                // Use provided save file name. If empty determine file name
                if ( !bHasLocation )
                {
                    // Create a default file name with the correct extension
                    const rtl::OUString aPreviewFileName( RTL_CONSTASCII_USTRINGPARAM( "webpreview" ));
                    aFileName = aPreviewFileName;
                }
                else
                {
                    // Determine file name from model
                    INetURLObject aFObj( xStorable->getLocation() );
                    aFileName = aFObj.getName( INetURLObject::LAST_SEGMENT, true, INetURLObject::NO_DECODE );
                }

                OSL_ASSERT( aFilterName.getLength() > 0 );
                OSL_ASSERT( aFileName.getLength() > 0 );

                // Creates a temporary directory to store our predefined file into it.
                ::utl::TempFile aTempDir( NULL, sal_True );

                INetURLObject aFilePathObj( aTempDir.GetURL() );
                aFilePathObj.insertName( aFileName );
                aFilePathObj.setExtension( aExtension );

                rtl::OUString aFileURL = aFilePathObj.GetMainURL( INetURLObject::NO_DECODE );

                css::uno::Sequence< css::beans::PropertyValue > aArgs( 1 );
                aArgs[0].Name  = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "FilterName" ));
                aArgs[0].Value = css::uno::makeAny( aFilterName );

                // Store document in the html format
                try
                {
                    xStorable->storeToURL( aFileURL, aArgs );
                }
                catch ( com::sun::star::io::IOException& )
                {
                    rReq.Done(FALSE);
                    return;
                }

                ::com::sun::star::uno::Reference< XSystemShellExecute > xSystemShellExecute( xSMGR->createInstance(
                    ::rtl::OUString::createFromAscii( "com.sun.star.system.SystemShellExecute" )),
                    css::uno::UNO_QUERY );

		        BOOL bRet( TRUE );
                if ( xSystemShellExecute.is() )
                {
                    try
                    {
		                xSystemShellExecute->execute(
						    aFileURL, ::rtl::OUString(), SystemShellExecuteFlags::DEFAULTS );
                    }
                    catch ( uno::Exception& )
                    {
					    vos::OGuard aGuard( Application::GetSolarMutex() );
                        Window *pParent = SFX_APP()->GetTopWindow();
					    ErrorBox( pParent, SfxResId( MSG_ERROR_NO_WEBBROWSER_FOUND )).Execute();
                        bRet = FALSE;
                    }
                }

                rReq.Done(bRet);
			    break;
            }
            else
            {
                rReq.Done(FALSE);
                return;
            }
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case SID_PLUGINS_ACTIVE:
		{
			SFX_REQUEST_ARG(rReq, pShowItem, SfxBoolItem, nId, FALSE);
			BOOL bActive = pShowItem ? pShowItem->GetValue() : !pImp->bPlugInsActive;
			// ggf. recorden
			if ( !rReq.IsAPI() )
				rReq.AppendItem( SfxBoolItem( nId, bActive ) );

			// Jetzt schon DONE aufrufen, da die Argumente evtl. einen Pool
			// benutzen, der demn"achst weg ist
			rReq.Done(TRUE);

			// ausfuehren
			if ( !pShowItem || bActive != pImp->bPlugInsActive )
			{
				SfxFrame* pTopFrame = GetFrame()->GetTopFrame();
				if ( pTopFrame != GetFrame()->GetFrame() )
				{
					// FramesetDocument
					SfxViewShell *pShell = pTopFrame->GetCurrentViewFrame()->GetViewShell();
					if ( pShell->GetInterface()->GetSlot( nId ) )
						pShell->ExecuteSlot( rReq );
					break;
				}

				SfxFrameIterator aIter( *pTopFrame );
				while ( pTopFrame )
				{
					if ( pTopFrame->GetCurrentViewFrame() )
					{
						SfxViewShell *pView = pTopFrame->GetCurrentViewFrame()->GetViewShell();
						if ( pView )
						{
							pView->pImp->bPlugInsActive = bActive;
                            Rectangle aVisArea = GetObjectShell()->GetVisArea();
                            VisAreaChanged(aVisArea);

							// the plugins might need change in their state
							SfxInPlaceClientList *pClients = pView->GetIPClientList_Impl(FALSE);
							if ( pClients )
							{
								for (USHORT n=0; n < pClients->Count(); n++)
								{
									SfxInPlaceClient* pIPClient = pClients->GetObject(n);
									if ( pIPClient )
										pView->CheckIPClient_Impl( pIPClient, aVisArea );
								}
							}
						}
					}

					if ( !pTopFrame->GetParentFrame() )
						pTopFrame = aIter.FirstFrame();
					else
						pTopFrame = aIter.NextFrame( *pTopFrame );
				}
			}

			break;
		}
	}
}

//--------------------------------------------------------------------

void SfxViewShell::GetState_Impl( SfxItemSet &rSet )
{
	DBG_CHKTHIS(SfxViewShell, 0);

	SfxWhichIter aIter( rSet );
	for ( USHORT nSID = aIter.FirstWhich(); nSID; nSID = aIter.NextWhich() )
	{
		switch ( nSID )
		{
			case SID_STYLE_CATALOG:
			{
                if ( !GetViewFrame()->KnowsChildWindow( SID_STYLE_DESIGNER ) )
					rSet.DisableItem( nSID );
				break;
			}

			// Printer-Funktionen
			case SID_PRINTDOC:
			case SID_PRINTDOCDIRECT:
			case SID_SETUPPRINTER:
			case SID_PRINTER_NAME:
			{
				BOOL bEnabled = pImp->bCanPrint && !pImp->nPrinterLocks;
				bEnabled = bEnabled  && !Application::GetSettings().GetMiscSettings().GetDisablePrinting();
                if ( bEnabled )
                {
                    SfxPrinter *pPrinter = GetPrinter(FALSE);
                    
                    if ( SID_PRINTDOCDIRECT == nSID )
                    {
                        rtl::OUString aPrinterName;
                        if ( pPrinter != NULL )
                            aPrinterName = pPrinter->GetName();
                        else
                            aPrinterName = Printer::GetDefaultPrinterName();
                        if ( aPrinterName.getLength() > 0 )
                        {
                            uno::Reference < frame::XFrame > xFrame( pFrame->GetFrame()->GetFrameInterface() );
                            
                            ::rtl::OUStringBuffer aBuffer( 60 );
                            aBuffer.append( RetrieveLabelFromCommand( 
                                                ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:PrintDefault" )), 
                                                xFrame ));
                            aBuffer.appendAscii( " (" );
                            aBuffer.append( aPrinterName );
                            aBuffer.appendAscii( ")" );
                                
                            rSet.Put( SfxStringItem( SID_PRINTDOCDIRECT, aBuffer.makeStringAndClear() ) );
                        }
                    }
                    bEnabled = !pPrinter || !pPrinter->IsPrinting();
                }
				if ( !bEnabled )
				{
					// will now be handled by requeing the request
				/*	rSet.DisableItem( SID_PRINTDOC );
					rSet.DisableItem( SID_PRINTDOCDIRECT );
					rSet.DisableItem( SID_SETUPPRINTER ); */
				}
				break;
			}

			// Mail-Funktionen
			case SID_MAIL_SENDDOCASPDF:
			case SID_MAIL_SENDDOC:
            case SID_MAIL_SENDDOCASFORMAT:
			{
                BOOL bEnable = !GetViewFrame()->HasChildWindow( SID_MAIL_CHILDWIN );
				if ( !bEnable )
					rSet.DisableItem( nSID );
				break;
			}

			// PlugIns running
			case SID_PLUGINS_ACTIVE:
			{
                rSet.Put( SfxBoolItem( SID_PLUGINS_ACTIVE, !pImp->bPlugInsActive) );
				break;
			}
/*
			// SelectionText
			case SID_SELECTION_TEXT:
			{
				rSet.Put( SfxStringItem( SID_SELECTION_TEXT, GetSelectionText() ) );
				break;
			}

			// SelectionTextExt
			case SID_SELECTION_TEXT_EXT:
			{
				rSet.Put( SfxStringItem( SID_SELECTION_TEXT_EXT, GetSelectionText(TRUE) ) );
				break;
			}
*/
			case SID_STYLE_FAMILY :
			{
				rSet.Put( SfxUInt16Item( SID_STYLE_FAMILY, pImp->nFamily ) );
				break;
			}
		}
	}
}

//--------------------------------------------------------------------

void SfxViewShell::SetZoomFactor( const Fraction &rZoomX,
								  const Fraction &rZoomY )
{
	DBG_ASSERT( GetWindow(), "no window" );
	MapMode aMap( GetWindow()->GetMapMode() );
	aMap.SetScaleX( rZoomX );
	aMap.SetScaleY( rZoomY );
	GetWindow()->SetMapMode( aMap );
}

//--------------------------------------------------------------------
ErrCode SfxViewShell::DoVerb(long /*nVerb*/)

/*  [Beschreibung]

	Virtuelle Methode, um am selektierten Objekt ein Verb auszuf"uhren.
    Da dieses Objekt nur den abgeleiteten Klassen bekannt ist, muss DoVerb
    dort "uberschrieben werden.

*/

{
	return ERRCODE_SO_NOVERBS;
}

//--------------------------------------------------------------------

void SfxViewShell::OutplaceActivated( sal_Bool bActive, SfxInPlaceClient* /*pClient*/ )
{
	if ( !bActive )
		GetFrame()->GetFrame()->Appear();
}

//--------------------------------------------------------------------

void SfxViewShell::InplaceActivating( SfxInPlaceClient* /*pClient*/ )
{
	// TODO/LATER: painting of the bitmap can be stopped, it is required if CLIPCHILDREN problem #i25788# is not solved,
	// but may be the bug will not affect the real office vcl windows, then it is not required
}

//--------------------------------------------------------------------

void SfxViewShell::InplaceDeactivated( SfxInPlaceClient* /*pClient*/ )
{
	// TODO/LATER: paint the replacement image in normal way if the painting was stopped
}

//--------------------------------------------------------------------

void SfxViewShell::UIActivating( SfxInPlaceClient* /*pClient*/ )
{
    uno::Reference < frame::XFrame > xOwnFrame( pFrame->GetFrame()->GetFrameInterface() );
    uno::Reference < frame::XFramesSupplier > xParentFrame( xOwnFrame->getCreator(), uno::UNO_QUERY );
    if ( xParentFrame.is() )
        xParentFrame->setActiveFrame( xOwnFrame );

    pFrame->GetBindings().HidePopups(TRUE);
    pFrame->GetDispatcher()->Update_Impl( TRUE );
}

//--------------------------------------------------------------------

void SfxViewShell::UIDeactivated( SfxInPlaceClient* /*pClient*/ )
{
    if ( !pFrame->GetFrame()->IsClosing_Impl() ||
        SfxViewFrame::Current() != pFrame )
            pFrame->GetDispatcher()->Update_Impl( TRUE );
    pFrame->GetBindings().HidePopups(FALSE);

    // uno::Reference < frame::XFrame > xOwnFrame( pFrame->GetFrame()->GetFrameInterface() );
    // uno::Reference < frame::XFramesSupplier > xParentFrame( xOwnFrame->getCreator(), uno::UNO_QUERY );
    // if ( xParentFrame.is() )
    //     xParentFrame->setActiveFrame( uno::Reference < frame::XFrame >() );
}

//--------------------------------------------------------------------

SfxInPlaceClient* SfxViewShell::FindIPClient
(
    const uno::Reference < embed::XEmbeddedObject >& xObj,
    Window*             pObjParentWin
)   const
{
    SfxInPlaceClientList *pClients = GetIPClientList_Impl(FALSE);
	if ( !pClients )
		return 0;

	if( !pObjParentWin )
		pObjParentWin = GetWindow();
	for (USHORT n=0; n < pClients->Count(); n++)
	{
		SfxInPlaceClient *pIPClient = (SfxInPlaceClient*) pClients->GetObject(n);
        if ( pIPClient->GetObject() == xObj && pIPClient->GetEditWin() == pObjParentWin )
			return pIPClient;
	}

	return 0;
}

//--------------------------------------------------------------------

SfxInPlaceClient* SfxViewShell::GetIPClient() const
{
	return GetUIActiveClient();
}

//--------------------------------------------------------------------

SfxInPlaceClient* SfxViewShell::GetUIActiveIPClient_Impl() const
{
    // this method is needed as long as SFX still manages the border space for ChildWindows (see SfxFrame::Resize)
    SfxInPlaceClientList *pClients = GetIPClientList_Impl(FALSE);
	if ( !pClients )
		return 0;

	for (USHORT n=0; n < pClients->Count(); n++)
	{
        SfxInPlaceClient* pIPClient = pClients->GetObject(n);
        if ( pIPClient->IsUIActive() )
            return pIPClient;
	}

    return NULL;
}

SfxInPlaceClient* SfxViewShell::GetUIActiveClient() const
{
    SfxInPlaceClientList *pClients = GetIPClientList_Impl(FALSE);
	if ( !pClients )
		return 0;

	for (USHORT n=0; n < pClients->Count(); n++)
	{
        SfxInPlaceClient* pIPClient = pClients->GetObject(n);
        if ( pIPClient->IsObjectUIActive() )
            return pIPClient;
	}

    return NULL;
}

//--------------------------------------------------------------------

void SfxViewShell::Activate( BOOL bMDI )
{
	DBG_CHKTHIS(SfxViewShell, 0);
	if ( bMDI )
	{
		SfxObjectShell *pSh = GetViewFrame()->GetObjectShell();
		if ( pSh->GetModel().is() )
			pSh->GetModel()->setCurrentController( GetViewFrame()->GetFrame()->GetController() );

        SetCurrentDocument();
	}
}

//--------------------------------------------------------------------

void SfxViewShell::Deactivate(BOOL /*bMDI*/)
{
	DBG_CHKTHIS(SfxViewShell, 0);
}

//--------------------------------------------------------------------

void SfxViewShell::AdjustPosSizePixel
(
	const Point&    /*rToolOffset*/,// linke obere Ecke der Tools im Frame-Window
	const Size&     /*rSize*/       // gesamte zur Verf"ugung stehende Gr"o\se
)

{
	DBG_CHKTHIS(SfxViewShell, 0);
}

//--------------------------------------------------------------------

void SfxViewShell::Move()

/*  [Beschreibung]

	Diese virtuelle Methode wird gerufen, wenn das Fenster, in dem die
	SfxViewShell dargestellt wird eine StarView-Move() Nachricht erh"alt.

	Die Basisimplementierung braucht nicht gerufen zu werden.


	[Anmerkung]

	Diese Methode kann dazu verwendet werden, eine Selektion abzubrechen,
	um durch das Moven des Fensters erzeugte Maus-Bewegungen anzufangen.

	Zur Zeit funktioniert die Benachrichtigung nicht In-Place.
*/

{
}

//--------------------------------------------------------------------

void SfxViewShell::OuterResizePixel
(
	const Point&    /*rToolOffset*/,// linke obere Ecke der Tools im Frame-Window
	const Size&     /*rSize*/       // gesamte zur Verf"ugung stehende Gr"o\se
)

/*  [Beschreibung]

    Diese Methode muss ueberladen werden, um auf "Anderungen der Groesse
	der View zu reagieren. Dabei definieren wir die View als das Edit-Window
	zuz"uglich der um das Edit-Window angeordnenten Tools (z.B. Lineale).

	Das Edit-Window darf weder in Gr"o\se noch Position ver"andert werden.

	Die Vis-Area der SfxObjectShell, dessen Skalierung und Position
	d"urfen hier ver"andert werden. Der Hauptanwendungsfall ist dabei,
	das Ver"andern der Gr"o\se der Vis-Area.

	"Andert sich durch die neue Berechnung der Border, so mu\s dieser
	mit <SfxViewShell::SetBorderPixel(const SvBorder&)> gesetzt werden.
	Erst nach Aufruf von 'SetBorderPixel' ist das Positionieren von
	Tools erlaubt.


	[Beispiel]

	void AppViewSh::OuterViewResizePixel( const Point &rOfs, const Size &rSz )
	{
		// Tool-Positionen und Gr"o\sen von au\sen berechnen, NICHT setzen!
		// (wegen folgender Border-Berechnung)
		Point aHLinPos...; Size aHLinSz...;
		...

		// Border f"ur Tools passend zu rSize berechnen und setzen
		SvBorder aBorder...
		SetBorderPixel( aBorder ); // ab jetzt sind Positionierungen erlaubt

		// Tools anordnen
		pHLin->SetPosSizePixel( aHLinPos, aHLinSz );
		...
	}


	[Querverweise]

	<SfxViewShell::InnerResizePixel(const Point&,const Size& rSize)>
*/

{
	DBG_CHKTHIS(SfxViewShell, 0);
	SetBorderPixel( SvBorder() );
}

//--------------------------------------------------------------------

void SfxViewShell::InnerResizePixel
(
	const Point&    /*rToolOffset*/,// linke obere Ecke der Tools im Frame-Window
	const Size&     /*rSize*/       // dem Edit-Win zur Verf"ugung stehende Gr"o\se
)

/*  [Beschreibung]

    Diese Methode muss ueberladen werden, um auf "Anderungen der Groesse
	des Edit-Windows zu reagieren.

	Das Edit-Window darf weder in Gr"o\se noch Position ver"andert werden.
	Weder die Vis-Area der SfxObjectShell noch dessen Skalierung oder
	Position d"urfen ver"andert werden.

	"Andert sich durch die neue Berechnung der Border, so mu\s dieser
	mit <SfxViewShell::SetBorderPixel(const SvBorder&)> gesetzt werden.
	Erst nach Aufruf von 'SetBorderPixel' ist das Positionieren von
	Tools erlaubt.


	[Beispiel]

	void AppViewSh::InnerViewResizePixel( const Point &rOfs, const Size &rSz )
	{
		// Tool-Positionen und Gr"o\sen von innen berechnen, NICHT setzen!
		// (wegen folgender Border-Berechnung)
		Point aHLinPos...; Size aHLinSz...;
		...

		// Border f"ur Tools passend zu rSz berechnen und setzen
		SvBorder aBorder...
		SetBorderPixel( aBorder ); // ab jetzt sind Positionierungen erlaubt

		// Tools anordnen
		pHLin->SetPosSizePixel( aHLinPos, aHLinSz );
		...
	}


	[Querverweise]

	<SfxViewShell::OuterResizePixel(const Point&,const Size& rSize)>
*/

{
	DBG_CHKTHIS(SfxViewShell, 0);
	SetBorderPixel( SvBorder() );
}

//--------------------------------------------------------------------

void SfxViewShell::InvalidateBorder()
{
	DBG_CHKTHIS(SfxViewShell, 0);
	DBG_ASSERT( GetViewFrame(), "SfxViewShell without SfxViewFrame" );

	GetViewFrame()->InvalidateBorderImpl( this );
    if ( pImp->pController )
        pImp->pController->BorderWidthsChanged_Impl();
}

//--------------------------------------------------------------------

void SfxViewShell::SetBorderPixel( const SvBorder &rBorder )
{
	DBG_CHKTHIS(SfxViewShell, 0);
	DBG_ASSERT( GetViewFrame(), "SfxViewShell without SfxViewFrame" );

    //if ( rBorder != GetBorderPixel())
    {
        GetViewFrame()->SetBorderPixelImpl( this, rBorder );

        // notify related controller that border size is changed
        if ( pImp->pController )
            pImp->pController->BorderWidthsChanged_Impl();
    }
}

//--------------------------------------------------------------------

const SvBorder& SfxViewShell::GetBorderPixel() const
{
	DBG_CHKTHIS(SfxViewShell, 0);
	DBG_ASSERT( GetViewFrame(), "SfxViewShell without SfxViewFrame" );

	return GetViewFrame()->GetBorderPixelImpl( this );
}

//--------------------------------------------------------------------

void SfxViewShell::SetWindow
(
	Window*     pViewPort   // Pointer auf das Datenfenster bzw. 0 im Destruktor
)

/*  [Beschreibung]

	Mit dieser Methode wird der SfxViewShell das Datenfenster mitgeteilt.
	Dieses wird f"ur den In-Place-Container und f"ur das korrekte
	Wiederherstellen des Focus ben"otigt.

	Selbst In-Place-aktiv ist das Umsetzen des ViewPort-Windows verboten.
*/

{
	if( pWindow == pViewPort )
		return;

	// ggf. vorhandene IP-Clients disconnecten
    DisconnectAllClients();

    //TODO: should we have a "ReconnectAllClients" method?
    DiscardClients_Impl();

	// View-Port austauschen
    BOOL bHadFocus = pWindow ? pWindow->HasChildPathFocus( TRUE ) : FALSE;
	pWindow = pViewPort;

    if( pWindow )
    {
        // Disable automatic GUI mirroring (right-to-left) for document windows
        pWindow->EnableRTL( FALSE );
    }

    if ( bHadFocus && pWindow )
        pWindow->GrabFocus();
        //TODO/CLEANUP
        //brauchen wir die Methode doch noch?!
        //SFX_APP()->GrabFocus( pWindow );
}

//--------------------------------------------------------------------

Size SfxViewShell::GetOptimalSizePixel() const
{
    DBG_ERROR( "Useless call!" );
    return Size();
}

//------------------------------------------------------------------------

SfxViewShell::SfxViewShell
(
	SfxViewFrame*   pViewFrame,     /*  <SfxViewFrame>, in dem diese View
										dargestellt wird */
	USHORT          nFlags          /*  siehe <SfxViewShell-Flags> */
)

:   SfxShell(this)
	,pImp( new SfxViewShell_Impl )
	,pIPClientList( 0 )
	,pFrame(pViewFrame)
	,pSubShell(0)
	,pWindow(0)
	,bMaximizeFirst( 0 != (nFlags & SFX_VIEW_MAXIMIZE_FIRST) )
	,bOptimizeEach(0 != (nFlags & SFX_VIEW_OPTIMIZE_EACH))
	,bNoNewWindow( 0 != (nFlags & SFX_VIEW_NO_NEWWINDOW) )
{
	DBG_CTOR(SfxViewShell, 0);

	pImp->pPrinterCommandQueue = new SfxAsyncPrintExec_Impl( this );
    pImp->pController = 0;
	pImp->bIsShowView =
		!(SFX_VIEW_NO_SHOW == (nFlags & SFX_VIEW_NO_SHOW));

    pImp->bUseObjectSize = FALSE;
//        SFX_CREATE_MODE_EMBEDDED==pFrame->GetObjectShell()->GetCreateMode() &&
//        SFX_VIEW_OBJECTSIZE_EMBEDDED == (nFlags & SFX_VIEW_OBJECTSIZE_EMBEDDED);
	pImp->bCanPrint = SFX_VIEW_CAN_PRINT == (nFlags & SFX_VIEW_CAN_PRINT);
	pImp->bFrameSetImpl = (nFlags & SFX_VIEW_IMPLEMENTED_AS_FRAMESET) != 0;
	pImp->bHasPrintOptions =
		SFX_VIEW_HAS_PRINTOPTIONS == (nFlags & SFX_VIEW_HAS_PRINTOPTIONS);
	pImp->bPlugInsActive = TRUE;
	pImp->bGotOwnerShip = FALSE;
    pImp->bGotFrameOwnerShip = FALSE;
    if ( pViewFrame->GetParentViewFrame() )
        pImp->bPlugInsActive = pViewFrame->GetParentViewFrame()->GetViewShell()->pImp->bPlugInsActive;
	pImp->eScroll = SCROLLING_DEFAULT;
	pImp->nPrinterLocks = 0;
	pImp->bControllerSet = FALSE;
	pImp->nFamily = 0xFFFF;					// undefined, default set by TemplateDialog
    SetMargin( pViewFrame->GetMargin_Impl() );

	SetPool( &pViewFrame->GetObjectShell()->GetPool() );
    StartListening(*pViewFrame->GetObjectShell());

	// in Liste eintragen
	const SfxViewShell *pThis = this; // wegen der kranken Array-Syntax
	SfxViewShellArr_Impl &rViewArr = SFX_APP()->GetViewShells_Impl();
    rViewArr.Insert(pThis, rViewArr.Count() );
}

//--------------------------------------------------------------------

SfxViewShell::~SfxViewShell()
{
	DBG_DTOR(SfxViewShell, 0);

	// aus Liste austragen
	const SfxViewShell *pThis = this;
    SfxViewShellArr_Impl &rViewArr = SFX_APP()->GetViewShells_Impl();
    rViewArr.Remove( rViewArr.GetPos(pThis) );

    if ( pImp->xClipboardListener.is() )
    {
        pImp->xClipboardListener->DisconnectViewShell();
        pImp->xClipboardListener = NULL;
    }

    if ( pImp->pController )
    {
        pImp->pController->ReleaseShell_Impl();
        pImp->pController->release();
        pImp->pController = NULL;
    }

    if (pImp->pAccExec)
    {
        DELETEZ( pImp->pAccExec );
    }

	DELETEZ( pImp->pPrinterCommandQueue );
	DELETEZ( pImp );
	DELETEZ( pIPClientList );
}

//--------------------------------------------------------------------

USHORT SfxViewShell::PrepareClose
(
	BOOL bUI,     // TRUE: Dialoge etc. erlaubt, FALSE: silent-mode
	BOOL /*bForBrowsing*/
)
{
	SfxPrinter *pPrinter = GetPrinter();
	if ( pPrinter && pPrinter->IsPrinting() )
	{
		if ( bUI )
		{
			InfoBox aInfoBox( &GetViewFrame()->GetWindow(), SfxResId( MSG_CANT_CLOSE ) );
			aInfoBox.Execute();
		}

		return FALSE;
	}

	if( GetViewFrame()->IsInModalMode() )
		return FALSE;

    if( bUI && GetViewFrame()->GetDispatcher()->IsLocked() )
		return FALSE;

	return TRUE;
}

//--------------------------------------------------------------------

SfxViewShell* SfxViewShell::Current()
{
    SfxViewFrame *pCurrent = SfxViewFrame::Current();
    return pCurrent ? pCurrent->GetViewShell() : NULL;
}

//--------------------------------------------------------------------

SdrView* SfxViewShell::GetDrawView() const

/*	[Beschreibung]

	Diese virtuelle Methode mu\s von den Subklassen "uberladen werden, wenn
	der Property-Editor zur Verf"ugung stehen soll.

	Die Default-Implementierung liefert immer 0.
*/

{
	return 0;
}

//--------------------------------------------------------------------

String SfxViewShell::GetSelectionText
(
	BOOL /*bCompleteWords*/		/* 	FALSE (default)
								Nur der tats"achlich selektierte Text wird
								zur"uckgegeben.

								TRUE
								Der selektierte Text wird soweit erweitert,
								da\s nur ganze W"orter zur"uckgegeben werden.
								Als Worttrenner gelten White-Spaces und die
                                Satzzeichen ".,;" sowie einfache und doppelte
								Anf"uhrungszeichen.
							*/
)

/*  [Beschreibung]

	Diese Methode kann von Anwendungsprogrammierer "uberladen werden,
	um einen Text zur"uckzuliefern, der in der aktuellen Selektion
	steht. Dieser wird z.B. beim Versenden (email) verwendet.

    Mit "CompleteWords == TRUE" ger"ufen, reicht z.B. auch der Cursor,
	der in einer URL steht, um die gesamte URL zu liefern.
*/

{
	return String();
}

//--------------------------------------------------------------------

BOOL SfxViewShell::HasSelection( BOOL ) const

/*  [Beschreibung]

	Mit dieser virtuellen Methode kann z.B. ein Dialog abfragen, ob in der
	aktuellen View etwas selektiert ist. Wenn der Parameter <BOOL> TRUE ist,
	wird abgefragt, ob Text selektiert ist.
*/

{
	return FALSE;
}

//--------------------------------------------------------------------

void SfxViewShell::SetSubShell( SfxShell *pShell )

/*  [Beschreibung]

	Mit dieser Methode kann eine Selektions- oder Cursor-Shell angemeldet
	werden, die automatisch unmittelbar nach der SfxViewShell auf den
	SfxDispatcher gepusht wird, und automatisch umittelbar vor ihr
	gepoppt wird.

	Ist die SfxViewShell-Instanz bereits gepusht, dann wird pShell
	sofort ebenfalls gepusht. Wird mit SetSubShell eine andere SfxShell
	Instanz angemeldet, als vorher angemeldet war, wird die zuvor angemeldete
	ggf. automatisch gepoppt. Mit pShell==0 kann daher die aktuelle
	Sub-Shell abgemeldet werden.
*/

{
	// ist diese ViewShell "uberhaupt aktiv?
	SfxDispatcher *pDisp = pFrame->GetDispatcher();
	if ( pDisp->IsActive(*this) )
	{
		// Dispatcher updaten
		if ( pSubShell )
			pDisp->Pop(*pSubShell);
		if ( pShell )
			pDisp->Push(*pShell);
		pDisp->Flush();
	}

	pSubShell = pShell;
}

void SfxViewShell::AddSubShell( SfxShell& rShell )
{
	pImp->aArr.Insert( &rShell, pImp->aArr.Count() );
	SfxDispatcher *pDisp = pFrame->GetDispatcher();
	if ( pDisp->IsActive(*this) )
	{
		pDisp->Push(rShell);
		pDisp->Flush();
	}
}

void SfxViewShell::RemoveSubShell( SfxShell* pShell )
{
	SfxDispatcher *pDisp = pFrame->GetDispatcher();
	if ( !pShell )
	{
		USHORT nCount = pImp->aArr.Count();
		if ( pDisp->IsActive(*this) )
		{
			for ( USHORT n=nCount; n>0; n-- )
				pDisp->Pop( *pImp->aArr[n-1] );
			pDisp->Flush();
		}

		pImp->aArr.Remove(0, nCount);
	}
	else
	{
		USHORT nPos = pImp->aArr.GetPos( pShell );
		if ( nPos != 0xFFFF )
		{
			pImp->aArr.Remove( nPos );
			if ( pDisp->IsActive(*this) )
			{
				pDisp->RemoveShell_Impl( *pShell );
				pDisp->Flush();
			}
		}
	}
}

SfxShell* SfxViewShell::GetSubShell( USHORT nNo )
{
	USHORT nCount = pImp->aArr.Count();
	if ( nNo<nCount )
		return pImp->aArr[nCount-nNo-1];
	return NULL;
}

void SfxViewShell::PushSubShells_Impl( BOOL bPush )
{
	USHORT nCount = pImp->aArr.Count();
	SfxDispatcher *pDisp = pFrame->GetDispatcher();
	if ( bPush )
	{
		for ( USHORT n=0; n<nCount; n++ )
			pDisp->Push( *pImp->aArr[n] );

//		HACK(evtl. PushSubShells fuer SW virtuell machen oder im SW umbauen)
//		Notify( *this, SfxSimpleHint( SFX_HINT_RESERVED4 ) );
	}
	else if ( nCount )
	{
		pDisp->Pop( *pImp->aArr[0], SFX_SHELL_POP_UNTIL );

//		HACK(evtl. PushSubShells fuer SW virtuell machen oder im SW umbauen)
//		Notify( *this, SfxSimpleHint( SFX_HINT_RESERVED3 ) );
	}

	pDisp->Flush();
}

//--------------------------------------------------------------------

void SfxViewShell::WriteUserData( String&, BOOL )
{
}

//--------------------------------------------------------------------

void SfxViewShell::ReadUserData(const String&, BOOL )
{
}

void SfxViewShell::ReadUserDataSequence ( const ::com::sun::star::uno::Sequence < ::com::sun::star::beans::PropertyValue >&, sal_Bool )
{
}
void SfxViewShell::WriteUserDataSequence ( ::com::sun::star::uno::Sequence < ::com::sun::star::beans::PropertyValue >&, sal_Bool )
{
}


//--------------------------------------------------------------------
// returns the first shell of spec. type viewing the specified doc.

SfxViewShell* SfxViewShell::GetFirst
(
	const TypeId*	pType,
	BOOL			bOnlyVisible
)
{
    // search for a SfxViewShell of the specified type
    SfxViewShellArr_Impl &rShells = SFX_APP()->GetViewShells_Impl();
    SfxViewFrameArr_Impl &rFrames = SFX_APP()->GetViewFrames_Impl();
	for ( USHORT nPos = 0; nPos < rShells.Count(); ++nPos )
	{
		SfxViewShell *pShell = rShells.GetObject(nPos);
        if ( pShell )
        {
            // sometimes dangling SfxViewShells exist that point to a dead SfxViewFrame
            // these ViewShells shouldn't be accessible anymore
            // a destroyed ViewFrame is not in the ViewFrame array anymore, so checking this array helps
            for ( sal_uInt16 n=0; n<rFrames.Count(); ++n )
            {
                SfxViewFrame *pFrame = rFrames.GetObject(n);
                if ( pFrame == pShell->GetViewFrame() )
                {
                    // only ViewShells with a valid ViewFrame will be returned
                    if ( ( !bOnlyVisible || pFrame->IsVisible_Impl() ) && ( !pType || pShell->IsA(*pType) ) )
                        return pShell;
                    break;
                }
            }
        }
    }

	return 0;
}

//--------------------------------------------------------------------
// returns the next shell of spec. type viewing the specified doc.

SfxViewShell* SfxViewShell::GetNext
(
	const SfxViewShell&	rPrev,
	const TypeId*		pType,
	BOOL				bOnlyVisible
)
{
    SfxViewShellArr_Impl &rShells = SFX_APP()->GetViewShells_Impl();
    SfxViewFrameArr_Impl &rFrames = SFX_APP()->GetViewFrames_Impl();
	USHORT nPos;
	for ( nPos = 0; nPos < rShells.Count(); ++nPos )
		if ( rShells.GetObject(nPos) == &rPrev )
			break;

    for ( ++nPos; nPos < rShells.Count(); ++nPos )
	{
		SfxViewShell *pShell = rShells.GetObject(nPos);
        if ( pShell )
        {
            // sometimes dangling SfxViewShells exist that point to a dead SfxViewFrame
            // these ViewShells shouldn't be accessible anymore
            // a destroyed ViewFrame is not in the ViewFrame array anymore, so checking this array helps
            for ( sal_uInt16 n=0; n<rFrames.Count(); ++n )
            {
                SfxViewFrame *pFrame = rFrames.GetObject(n);
                if ( pFrame == pShell->GetViewFrame() )
                {
                    // only ViewShells with a valid ViewFrame will be returned
                    if ( ( !bOnlyVisible || pFrame->IsVisible_Impl() ) && ( !pType || pShell->IsA(*pType) ) )
                        return pShell;
                    break;
                }
            }
        }
    }

	return 0;
}

//--------------------------------------------------------------------

void SfxViewShell::Notify( SfxBroadcaster& rBC,
                            const SfxHint& rHint )
{
    if ( rHint.IsA(TYPE(SfxEventHint)) )
    {
        switch ( ((SfxEventHint&)rHint).GetEventId() )
        {
            case SFX_EVENT_LOADFINISHED:
            {
                if ( GetController().is() )
                {
                    // avoid access to dangling ViewShells
                    SfxViewFrameArr_Impl &rFrames = SFX_APP()->GetViewFrames_Impl();
                    for ( sal_uInt16 n=0; n<rFrames.Count(); ++n )
                    {
                        SfxViewFrame *frame = rFrames.GetObject(n);
                        if ( frame == GetViewFrame() && &rBC == GetObjectShell() )
                        {
                            SfxItemSet* pSet = GetObjectShell()->GetMedium()->GetItemSet();
                            SFX_ITEMSET_ARG( pSet, pItem, SfxUnoAnyItem, SID_VIEW_DATA, sal_False );
                            if ( pItem )
                            {
                                pImp->pController->restoreViewData( pItem->GetValue() );
                                pSet->ClearItem( SID_VIEW_DATA );
                            }

                            break;
                        }
                    }
                }

                break;
            }
        }
    }
}

//--------------------------------------------------------------------

BOOL SfxViewShell::ExecKey_Impl(const KeyEvent& aKey)
{
    if (!pImp->pAccExec)
    {
        pImp->pAccExec = ::svt::AcceleratorExecute::createAcceleratorHelper();
        pImp->pAccExec->init(::comphelper::getProcessServiceFactory(), pFrame->GetFrame()->GetFrameInterface());
    }

    return pImp->pAccExec->execute(aKey.GetKeyCode());
}

//--------------------------------------------------------------------

FASTBOOL SfxViewShell::KeyInput( const KeyEvent &rKeyEvent )

/*  [Beschreibung]

	Diese Methode f"uhrt das KeyEvent 'rKeyEvent' "uber die an dieser
	SfxViewShell direkt oder indirekt (z.B. via Applikation) konfigurierten
	Tasten (Accelerator) aus.


	[R"uckgabewert]

	FASTBOOL                TRUE
							die Taste ist konfiguriert, der betreffende
							Handler wurde gerufen

							FALSE
							die Taste ist nicht konfiguriert, es konnte
							also kein Handler gerufen werden


	[Querverweise]
	<SfxApplication::KeyInput(const KeyEvent&)>
*/
{
    return ExecKey_Impl(rKeyEvent);
}

FASTBOOL SfxViewShell::GlobalKeyInput_Impl( const KeyEvent &rKeyEvent )
{
    return ExecKey_Impl(rKeyEvent);
}

//--------------------------------------------------------------------

void SfxViewShell::ShowCursor( FASTBOOL /*bOn*/ )

/*  [Beschreibung]

	Diese Methode mu\s von Subklassen "uberladen werden, damit vom SFx
	aus der Cursor ein- und ausgeschaltet werden kann. Dies geschieht
	z.B. bei laufendem <SfxProgress>.
*/

{
}

//--------------------------------------------------------------------

void SfxViewShell::GotFocus() const

/*  [Beschreibung]

	Diese Methode mu\s vom Applikationsentwickler gerufen werden, wenn
	das Edit-Window den Focus erhalten hat. Der SFx hat so z.B. die
	M"oglichkeit, den Accelerator einzuschalten.


	[Anmerkung]

	<StarView> liefert leider keine M"oglichkeit, solche Events
	'von der Seite' einzuh"angen.
*/

{
}

//--------------------------------------------------------------------
void SfxViewShell::ResetAllClients_Impl( SfxInPlaceClient *pIP )
{

    SfxInPlaceClientList *pClients = GetIPClientList_Impl(FALSE);
	if ( !pClients )
		return;

    for ( USHORT n=0; n < pClients->Count(); n++ )
	{
        SfxInPlaceClient* pIPClient = pClients->GetObject(n);
        if( pIPClient != pIP )
            pIPClient->ResetObject();
	}
}

//--------------------------------------------------------------------

void SfxViewShell::DisconnectAllClients()
{
    SfxInPlaceClientList *pClients = GetIPClientList_Impl(FALSE);
	if ( !pClients )
		return;

	for ( USHORT n=0; n<pClients->Count(); )
        // clients will remove themselves from the list
        delete pClients->GetObject(n);
}

//--------------------------------------------------------------------

BOOL SfxViewShell::UseObjectSize() const
{
	return pImp->bUseObjectSize;
}

//--------------------------------------------------------------------

void SfxViewShell::QueryObjAreaPixel( Rectangle& ) const
{
}

//--------------------------------------------------------------------

void SfxViewShell::AdjustVisArea(const Rectangle& rRect)
{
	DBG_ASSERT (pFrame, "Kein Frame?");
	if ( UseObjectSize() )
	{
		Point aPos = rRect.TopLeft();
        Size aSize = GetObjectShell()->GetVisArea().GetSize();
        GetObjectShell()->SetVisArea( Rectangle(aPos, aSize) );
	}
	else
        GetObjectShell()->SetVisArea( rRect );
}

//--------------------------------------------------------------------

void SfxViewShell::VisAreaChanged(const Rectangle& /*rVisArea*/)
{
    SfxInPlaceClientList *pClients = GetIPClientList_Impl(FALSE);
    if ( !pClients )
		return;

	for (USHORT n=0; n < pClients->Count(); n++)
	{
        SfxInPlaceClient* pIPClient = pClients->GetObject(n);
        if ( pIPClient->IsObjectInPlaceActive() )
            // client is active, notify client that the VisArea might have changed
            pIPClient->VisAreaChanged();
    }
}

//--------------------------------------------------------------------
void SfxViewShell::CheckIPClient_Impl( SfxInPlaceClient *pIPClient, const Rectangle& rVisArea )
{
	if ( GetObjectShell()->IsInClose() )
		return;

	sal_Bool bAlwaysActive =
		( ( pIPClient->GetObjectMiscStatus() & embed::EmbedMisc::EMBED_ACTIVATEIMMEDIATELY ) != 0 );
	sal_Bool bActiveWhenVisible =
		( ( pIPClient->GetObjectMiscStatus() & embed::EmbedMisc::MS_EMBED_ACTIVATEWHENVISIBLE ) != 0 );

    // this method is called when either a client is created or the "Edit/Plugins" checkbox is checked
    if ( !pIPClient->IsObjectInPlaceActive() && pImp->bPlugInsActive )
	{
       	// object in client is currently not active
       	// check if the object wants to be activated always or when it becomes at least partially visible
       	// TODO/LATER: maybe we should use the scaled area instead of the ObjArea?!
       	if ( bAlwaysActive || (bActiveWhenVisible && rVisArea.IsOver(pIPClient->GetObjArea())) )
		{
			try
			{
				pIPClient->GetObject()->changeState( embed::EmbedStates::INPLACE_ACTIVE );
			}
			catch ( uno::Exception& )
			{
			}
		}
    }
	else if ( !pImp->bPlugInsActive )
	{
       	// object in client is currently active and "Edit/Plugins" checkbox is selected
       	// check if the object wants to be activated always or when it becomes at least partially visible
		// in this case selecting of the "Edit/Plugin" checkbox should let such objects deactivate
       	if ( bAlwaysActive || bActiveWhenVisible )
           	pIPClient->GetObject()->changeState( embed::EmbedStates::RUNNING );
	}
}

//--------------------------------------------------------------------

BOOL SfxViewShell::PlugInsActive() const
{
	return pImp->bPlugInsActive;
}

//--------------------------------------------------------------------
void SfxViewShell::DiscardClients_Impl()

/*  [Beschreibung]

	Diese Methode dient dazu, vor dem Schlie\sen eines Dokuments das
	Speichern der Objekte zu verhindern, wenn der Benutzer Schlie\en ohne
	Speichern gew"ahlt hatte.
*/

{
    SfxInPlaceClientList *pClients = GetIPClientList_Impl(FALSE);
	if ( !pClients )
		return;

    for (USHORT n=0; n < pClients->Count(); )
        delete pClients->GetObject(n);
}

//--------------------------------------------------------------------

SfxScrollingMode SfxViewShell::GetScrollingMode() const
{
	return pImp->eScroll;
}

//--------------------------------------------------------------------

void SfxViewShell::SetScrollingMode( SfxScrollingMode eMode )
{
	pImp->eScroll = eMode;
}

//--------------------------------------------------------------------

SfxObjectShell* SfxViewShell::GetObjectShell()
{
    return pFrame ? pFrame->GetObjectShell() : NULL;
}

//--------------------------------------------------------------------

Reference< XModel > SfxViewShell::GetCurrentDocument() const
{
    Reference< XModel > xDocument;

    const SfxObjectShell* pDocShell( const_cast< SfxViewShell* >( this )->GetObjectShell() );
    OSL_ENSURE( pDocShell, "SfxViewFrame::GetCurrentDocument: no DocShell!?" );
    if ( pDocShell )
        xDocument = pDocShell->GetModel();
    return xDocument;
}

//--------------------------------------------------------------------

void SfxViewShell::SetCurrentDocument() const
{
    uno::Reference< frame::XModel > xDocument( GetCurrentDocument() );
    if ( xDocument.is() )
        SfxObjectShell::SetCurrentComponent( xDocument );
}

//--------------------------------------------------------------------

const Size& SfxViewShell::GetMargin() const
{
	return pImp->aMargin;
}

//--------------------------------------------------------------------

void SfxViewShell::SetMargin( const Size& rSize )
{
	// Der default-Margin wurde "geeicht" mit www.apple.com !!
	Size aMargin = rSize;
	if ( aMargin.Width() == -1 )
		aMargin.Width() = DEFAULT_MARGIN_WIDTH;
	if ( aMargin.Height() == -1 )
		aMargin.Height() = DEFAULT_MARGIN_HEIGHT;

	if ( aMargin != pImp->aMargin )
	{
		pImp->aMargin = aMargin;
		MarginChanged();
	}
}

//--------------------------------------------------------------------

void SfxViewShell::MarginChanged()
{
}

//--------------------------------------------------------------------

BOOL SfxViewShell::IsShowView_Impl() const
{
	return pImp->bIsShowView;
}

//--------------------------------------------------------------------

SfxFrame* SfxViewShell::GetSmartSelf( SfxFrame* pSelf, SfxMedium& /*rMedium*/ )
{
	return pSelf;
}

//--------------------------------------------------------------------

BOOL SfxViewShell::IsImplementedAsFrameset_Impl( ) const
{
	return pImp->bFrameSetImpl;
}

//------------------------------------------------------------------------

void SfxViewShell::JumpToMark( const String& rMark )
{
	SfxStringItem aMarkItem( SID_JUMPTOMARK, rMark );
	GetViewFrame()->GetDispatcher()->Execute(
			SID_JUMPTOMARK,
			SFX_CALLMODE_SYNCHRON|SFX_CALLMODE_RECORD,
			&aMarkItem, 0L );
}

//------------------------------------------------------------------------

SfxInPlaceClientList* SfxViewShell::GetIPClientList_Impl( BOOL bCreate ) const
{
	if ( !pIPClientList && bCreate )
        ( (SfxViewShell*) this )->pIPClientList = new SfxInPlaceClientList;
	return pIPClientList;
}

void SfxViewShell::SetController( SfxBaseController* pController )
{
    pImp->pController = pController;
    pImp->pController->acquire();
    pImp->bControllerSet = TRUE;
    
    // there should be no old listener, but if there is one, it should be disconnected
    if (  pImp->xClipboardListener.is() )
        pImp->xClipboardListener->DisconnectViewShell();

    pImp->xClipboardListener = new SfxClipboardChangeListener( this );
	AddRemoveClipboardListener( pImp->xClipboardListener.get(), TRUE );
}

Reference < XController > SfxViewShell::GetController()
{
    return pImp->pController;
}

void SfxViewShell::AddContextMenuInterceptor_Impl( const REFERENCE< XCONTEXTMENUINTERCEPTOR >& xInterceptor )
{
    pImp->aInterceptorContainer.addInterface( xInterceptor );
}

void SfxViewShell::RemoveContextMenuInterceptor_Impl( const REFERENCE< XCONTEXTMENUINTERCEPTOR >& xInterceptor )
{
    pImp->aInterceptorContainer.removeInterface( xInterceptor );
}

::cppu::OInterfaceContainerHelper& SfxViewShell::GetContextMenuInterceptors() const
{
    return pImp->aInterceptorContainer;
}

void Change( Menu* pMenu, SfxViewShell* pView )
{
	SfxDispatcher *pDisp = pView->GetViewFrame()->GetDispatcher();
	USHORT nCount = pMenu->GetItemCount();
	for ( USHORT nPos=0; nPos<nCount; ++nPos )
	{
		USHORT nId = pMenu->GetItemId(nPos);
		String aCmd = pMenu->GetItemCommand(nId);
		PopupMenu* pPopup = pMenu->GetPopupMenu(nId);
		if ( pPopup )
		{
			Change( pPopup, pView );
		}
		else if ( nId < 5000 )
		{
			if ( aCmd.CompareToAscii(".uno:", 5) == 0 )
			{
		        for (USHORT nIdx=0;;)
		        {
                    SfxShell *pShell=pDisp->GetShell(nIdx++);
                    if (pShell == NULL)
                        break;
		            const SfxInterface *pIFace = pShell->GetInterface();
		            const SfxSlot* pSlot = pIFace->GetSlot( aCmd );
		            if ( pSlot )
		            {
						pMenu->InsertItem( pSlot->GetSlotId(), pMenu->GetItemText( nId ), pMenu->GetItemBits( nId ), nPos );
						pMenu->RemoveItem( nPos+1 );
		                break;
		            }
		        }
			}
		}
	}
}


BOOL SfxViewShell::TryContextMenuInterception( Menu& rIn, const ::rtl::OUString& rMenuIdentifier, Menu*& rpOut, ::com::sun::star::ui::ContextMenuExecuteEvent aEvent )
{
    rpOut = NULL;
    BOOL bModified = FALSE;

    // create container from menu
	// #110897#
    // aEvent.ActionTriggerContainer = ::framework::ActionTriggerHelper::CreateActionTriggerContainerFromMenu( &rIn );
    aEvent.ActionTriggerContainer = ::framework::ActionTriggerHelper::CreateActionTriggerContainerFromMenu(
        ::comphelper::getProcessServiceFactory(), &rIn, &rMenuIdentifier );

    // get selection from controller
    aEvent.Selection = ::com::sun::star::uno::Reference < ::com::sun::star::view::XSelectionSupplier > ( GetController(), ::com::sun::star::uno::UNO_QUERY );

    // call interceptors
    ::cppu::OInterfaceIteratorHelper aIt( pImp->aInterceptorContainer );
    while( aIt.hasMoreElements() )
    {
        try
        {
            ::com::sun::star::ui::ContextMenuInterceptorAction eAction =
                ((::com::sun::star::ui::XContextMenuInterceptor*)aIt.next())->notifyContextMenuExecute( aEvent );
            switch ( eAction )
            {
                case ::com::sun::star::ui::ContextMenuInterceptorAction_CANCELLED :
                    // interceptor does not want execution
                    return FALSE;
                case ::com::sun::star::ui::ContextMenuInterceptorAction_EXECUTE_MODIFIED :
                    // interceptor wants his modified menu to be executed
                    bModified = TRUE;
                    break;
                case ::com::sun::star::ui::ContextMenuInterceptorAction_CONTINUE_MODIFIED :
                    // interceptor has modified menu, but allows for calling other interceptors
                    bModified = TRUE;
                    continue;
                case ::com::sun::star::ui::ContextMenuInterceptorAction_IGNORED :
                    // interceptor is indifferent
                    continue;
                default:
                    DBG_ERROR("Wrong return value of ContextMenuInterceptor!");
                    continue;
            }
        }
        catch( ::com::sun::star::uno::RuntimeException& )
        {
            aIt.remove();
        }

        break;
    }

    if ( bModified )
    {
        // container was modified, create a new window out of it
        rpOut = new PopupMenu;
        ::framework::ActionTriggerHelper::CreateMenuFromActionTriggerContainer( rpOut, aEvent.ActionTriggerContainer );

		Change( rpOut, this );
	}

    return TRUE;
}

void SfxViewShell::TakeOwnerShip_Impl()
{
	// currently there is only one reason to take OwnerShip: a hidden frame is printed
	// so the ViewShell will check this on EndPrint (->prnmon.cxx)
    pImp->bGotOwnerShip = TRUE;
}

void SfxViewShell::TakeFrameOwnerShip_Impl()
{
	// currently there is only one reason to take OwnerShip: a hidden frame is printed
	// so the ViewShell will check this on EndPrint (->prnmon.cxx)
    pImp->bGotFrameOwnerShip = TRUE;
}

void SfxViewShell::CheckOwnerShip_Impl()
{
    BOOL bSuccess = FALSE;
    if( pImp->bGotOwnerShip )
	{
        com::sun::star::uno::Reference < com::sun::star::util::XCloseable > xModel(
                GetObjectShell()->GetModel(), com::sun::star::uno::UNO_QUERY );
		if ( xModel.is() )
		{
			try
			{
                // this call will destroy this object in case of success!
				xModel->close( sal_True );
                bSuccess = TRUE;
			}
			catch ( com::sun::star::util::CloseVetoException& )
			{
			}
		}
	}

    if( !bSuccess && pImp->bGotFrameOwnerShip )
	{
        // document couldn't be closed or it shouldn't, now try at least to close the frame
        com::sun::star::uno::Reference < com::sun::star::util::XCloseable > xFrame(
                GetViewFrame()->GetFrame()->GetFrameInterface(), com::sun::star::uno::UNO_QUERY );
        if ( xFrame.is() )
		{
			try
			{
                xFrame->close( sal_True );
			}
			catch ( com::sun::star::util::CloseVetoException& )
			{
			}
		}
	}
}

long SfxViewShell::HandleNotifyEvent_Impl( NotifyEvent& rEvent )
{
	if ( pImp->pController )
		return pImp->pController->HandleEvent_Impl( rEvent );
	return 0;
}

BOOL SfxViewShell::HasKeyListeners_Impl()
{
	return pImp->pController ? pImp->pController->HasKeyListeners_Impl() : FALSE;
}

BOOL SfxViewShell::HasMouseClickListeners_Impl()
{
	return pImp->pController ? pImp->pController->HasMouseClickListeners_Impl() : FALSE;
}

void SfxViewShell::SetAdditionalPrintOptions( const com::sun::star::uno::Sequence < com::sun::star::beans::PropertyValue >& rOpts )
{
	pImp->aPrintOpts = rOpts;
 	GetObjectShell()->Broadcast( SfxPrintingHint( -3, NULL, NULL, rOpts ) );
}

BOOL SfxViewShell::Escape()
{
    return GetViewFrame()->GetBindings().Execute( SID_TERMINATE_INPLACEACTIVATION );
}

void SfxViewShell::AddRemoveClipboardListener( const uno::Reference < datatransfer::clipboard::XClipboardListener >& rClp, BOOL bAdd )
{
    try
    {
        if ( GetViewFrame() )
        {
            uno::Reference< datatransfer::clipboard::XClipboard > xClipboard( GetViewFrame()->GetWindow().GetClipboard() );
            if( xClipboard.is() )
            {
                uno::Reference< datatransfer::clipboard::XClipboardNotifier > xClpbrdNtfr( xClipboard, uno::UNO_QUERY );
                if( xClpbrdNtfr.is() )
                {
                    if( bAdd )
                        xClpbrdNtfr->addClipboardListener( rClp );
                    else
                        xClpbrdNtfr->removeClipboardListener( rClp );
                }
            }
        }
    }
    catch( const uno::Exception& )
    {
    }
}
