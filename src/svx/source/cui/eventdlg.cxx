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
#include "precompiled_svx.hxx"

// include ***************************************************************
#include <svtools/svmedit.hxx>
#include <tools/diagnose_ex.h>
#include <com/sun/star/document/XEventsSupplier.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>

#include <comphelper/processfactory.hxx>
#include <comphelper/documentinfo.hxx>
#include <unotools/configmgr.hxx>
#include <rtl/ustring.hxx>

#include "eventdlg.hxx"

#include <sfx2/viewfrm.hxx>
#include <sfx2/evntconf.hxx>
#include <sfx2/macrconf.hxx>
#include <sfx2/minfitem.hxx>
#include <sfx2/app.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/fcontnr.hxx>
#include <svtools/eventcfg.hxx>
 
#include "headertablistbox.hxx"
#include "macropg_impl.hxx"

#include <svx/dialmgr.hxx>
#include <svx/dialogs.hrc>
#include "eventdlg.hrc"
#include "helpid.hrc"
#include "selector.hxx"
#include "cfg.hxx"


using ::rtl::OUString;
using namespace ::com::sun::star;
// -----------------------------------------------------------------------

SvxEventConfigPage::SvxEventConfigPage( Window *pParent, const SfxItemSet& rSet, SvxEventConfigPage::EarlyInit ) :

	_SvxMacroTabPage( pParent, SVX_RES(RID_SVXPAGE_EVENTS), rSet ),
    aSaveInText( this, SVX_RES( TXT_SAVEIN ) ),
	aSaveInListBox( this, SVX_RES( LB_SAVEIN ) ),
	bAppConfig	( TRUE )
{
	mpImpl->pStrEvent			= new String( SVX_RES( STR_EVENT ));
	mpImpl->pAssignedMacro		= new String( SVX_RES( STR_ASSMACRO ));
	mpImpl->pEventLB			= new _HeaderTabListBox( this, SVX_RES( LB_EVENT ));
    mpImpl->pAssignFT			= new FixedText( this,  SVX_RES( FT_ASSIGN ));
	mpImpl->pAssignPB			= new PushButton( this, SVX_RES( PB_ASSIGN ));
	mpImpl->pDeletePB			= new PushButton( this, SVX_RES( PB_DELETE ));
	mpImpl->pMacroImg			= new Image( SVX_RES( IMG_MACRO) );
	mpImpl->pComponentImg		= new Image( SVX_RES( IMG_COMPONENT) );
	mpImpl->pMacroImg_h			= new Image( SVX_RES( IMG_MACRO_H) );
	mpImpl->pComponentImg_h		= new Image( SVX_RES( IMG_COMPONENT_H) );

	FreeResource();

    // must be done after FreeResource is called
    InitResources();

    mpImpl->pEventLB->GetListBox().SetHelpId( HID_SVX_MACRO_LB_EVENT );

    aSaveInListBox.SetSelectHdl( LINK( this, SvxEventConfigPage, 
                SelectHdl_Impl ) );

    uno::Reference< document::XEventsSupplier > xSupplier;

//    xSupplier = uno::Reference< document::XEventsSupplier >( new GlobalEventConfig());
    xSupplier = uno::Reference< document::XEventsSupplier > (
        ::comphelper::getProcessServiceFactory()->createInstance(
            OUString::createFromAscii(
                "com.sun.star.frame.GlobalEventBroadcaster" ) ),
		uno::UNO_QUERY );

    USHORT nPos(0);
    if ( xSupplier.is() )
    {
        m_xAppEvents = xSupplier->getEvents();
		OUString label;
		utl::ConfigManager::GetDirectConfigProperty(
			utl::ConfigManager::PRODUCTNAME ) >>= label;
        nPos = aSaveInListBox.InsertEntry( label );
        aSaveInListBox.SetEntryData( nPos, new bool(true) );
        aSaveInListBox.SelectEntryPos( nPos, TRUE );
    }
}

// -----------------------------------------------------------------------
void SvxEventConfigPage::LateInit( const uno::Reference< frame::XFrame >& _rxFrame  )
{
    SetFrame( _rxFrame );
    ImplInitDocument();

	InitAndSetHandler( m_xAppEvents, m_xDocumentEvents, m_xDocumentModifiable );

    SelectHdl_Impl( NULL );
}

// -----------------------------------------------------------------------

SvxEventConfigPage::~SvxEventConfigPage()
{
    //DF Do I need to delete bools?
}

// -----------------------------------------------------------------------

void SvxEventConfigPage::ImplInitDocument()
{
    uno::Reference< frame::XFrame > xFrame( GetFrame() );
    OUString aModuleId = SvxConfigPage::GetFrameWithDefaultAndIdentify( xFrame );
    if ( !xFrame.is() )
        return;

    try
    {
        uno::Reference< frame::XModel > xModel;
		if ( !SvxConfigPage::CanConfig( aModuleId ) )
            return;

        uno::Reference< frame::XController > xController =
			xFrame->getController();

		if ( xController.is() )
		{
			xModel = xController->getModel();
		}

        if ( !xModel.is() )
            return;

        uno::Reference< document::XEventsSupplier > xSupplier( xModel, uno::UNO_QUERY );

        if ( xSupplier.is() )
        {
            m_xDocumentEvents = xSupplier->getEvents();
            m_xDocumentModifiable = m_xDocumentModifiable.query( xModel );

			OUString aTitle = ::comphelper::DocumentInfo::getDocumentTitle( xModel );
            USHORT nPos = aSaveInListBox.InsertEntry( aTitle );

            aSaveInListBox.SetEntryData( nPos, new bool(false) );
            aSaveInListBox.SelectEntryPos( nPos, TRUE );

            bAppConfig = false;
        }
    }
    catch( const uno::Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxEventConfigPage, SelectHdl_Impl, ListBox *, pBox )
{
	(void)pBox;

	bool* bApp = (bool*) aSaveInListBox.GetEntryData(
            aSaveInListBox.GetSelectEntryPos());
	
	mpImpl->pEventLB->SetUpdateMode( FALSE );
	bAppConfig = *bApp;
	if ( *bApp )
	{
		SetReadOnly( FALSE );
		_SvxMacroTabPage::DisplayAppEvents( true );
	}
	else
	{
		bool isReadonly = FALSE;

		uno::Reference< frame::XFramesSupplier > xFramesSupplier(
			::comphelper::getProcessServiceFactory()->createInstance(
				OUString::createFromAscii( "com.sun.star.frame.Desktop" ) ),
			uno::UNO_QUERY );

		uno::Reference< frame::XFrame > xFrame =
			xFramesSupplier->getActiveFrame();

		if ( xFrame.is() )
		{
			uno::Reference< frame::XController > xController =
				xFrame->getController();

			if ( xController.is() )
			{
				uno::Reference< frame::XStorable > xStorable(
					xController->getModel(), uno::UNO_QUERY );
				isReadonly = xStorable->isReadonly();
			}
		}

		SetReadOnly( isReadonly );
		_SvxMacroTabPage::DisplayAppEvents( false );
	}

	mpImpl->pEventLB->SetUpdateMode( TRUE );
	return TRUE;
}

// -----------------------------------------------------------------------

BOOL SvxEventConfigPage::FillItemSet( SfxItemSet& rSet )
{
	return _SvxMacroTabPage::FillItemSet( rSet );
}

// -----------------------------------------------------------------------

void SvxEventConfigPage::Reset( const SfxItemSet& )
{
	_SvxMacroTabPage::Reset();
}

