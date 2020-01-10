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

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif
// include ---------------------------------------------------------------

#define _SVX_OPTIMPROVE_CXX

#include <svx/optimprove.hxx>
#include <svx/dialmgr.hxx>
#include <vcl/msgbox.hxx>

#include "optimprove.hrc"
#include "helpid.hrc"
#include <svx/dialogs.hrc>

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/system/XSystemShellExecute.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#include <com/sun/star/oooimprovement/XCoreController.hpp>
#include <comphelper/configurationhelper.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/uieventslogger.hxx>
#include <tools/testtoolloader.hxx>

namespace lang  = ::com::sun::star::lang;
namespace uno   = ::com::sun::star::uno;
using namespace com::sun::star::system;

// class SvxImprovementPage ----------------------------------------------

SvxImprovementPage::SvxImprovementPage( Window* pParent ) :

    TabPage( pParent, SVX_RES( RID_SVXPAGE_IMPROVEMENT ) ),

    m_aImproveFL                ( this, SVX_RES( FL_IMPROVE ) ),
    m_aInvitationFT             ( this, SVX_RES( FT_INVITATION ) ),
    m_aYesRB                    ( this, SVX_RES( RB_YES ) ),
    m_aNoRB                     ( this, SVX_RES( RB_NO ) ),
    m_aDataFL                   ( this, SVX_RES( FL_DATA ) ),
    m_aNumberOfReportsFT        ( this, SVX_RES( FT_NR_REPORTS ) ),
    m_aNumberOfReportsValueFT   ( this, SVX_RES( FT_NR_REPORTS_VALUE ) ),
    m_aNumberOfActionsFT        ( this, SVX_RES( FT_NR_ACTIONS ) ),
    m_aNumberOfActionsValueFT   ( this, SVX_RES( FT_NR_ACTIONS_VALUE ) ),
    m_aShowDataPB               ( this, SVX_RES( PB_SHOWDATA ) ),

    m_sInfo                     (       SVX_RES( STR_INFO ) ),
    m_sMoreInfo                 (       SVX_RES( STR_MOREINFO ) )

{
	FreeResource();

    m_aInvitationFT.Show();
    m_aDataFL.Hide();
    m_aNumberOfReportsFT.Hide();
    m_aNumberOfReportsValueFT.Hide();
    m_aNumberOfActionsFT.Hide();
    m_aNumberOfActionsValueFT.Hide();
    m_aShowDataPB.Hide();

    Size aNewSize = m_aInvitationFT.GetSizePixel();
    const long nMinWidth = m_aYesRB.CalcMinimumSize().Width();
    const long nNewWidth = std::max( aNewSize.Width() * 4 / 5, nMinWidth );
    const long nWDelta = aNewSize.Width() - nNewWidth;
    aNewSize.Width() = nNewWidth;
    const Size aCalcSize = m_aInvitationFT.CalcMinimumSize( nNewWidth );
    const long nHDelta = aCalcSize.Height() - aNewSize.Height();
    aNewSize.Height() = aCalcSize.Height();
    m_aInvitationFT.SetSizePixel( aNewSize );

    aNewSize = m_aYesRB.GetSizePixel();
    aNewSize.Width() = nNewWidth;
    Point aNewPos = m_aYesRB.GetPosPixel();
    aNewPos.Y() += nHDelta;
    m_aYesRB.SetPosSizePixel( aNewPos, aNewSize );
    aNewSize = m_aNoRB.GetSizePixel();
    aNewSize.Width() = nNewWidth;
    aNewPos = m_aNoRB.GetPosPixel();
    aNewPos.Y() += nHDelta;
    m_aNoRB.SetPosSizePixel( aNewPos, aNewSize );
    aNewSize = m_aImproveFL.GetSizePixel();
    aNewSize.Width() -= nWDelta;
    m_aImproveFL.SetSizePixel( aNewSize );

    Size aSize = GetOutputSizePixel();
    aSize.Width() -= nWDelta;
    aSize.Height() = m_aDataFL.GetPosPixel().Y();
    aSize.Height() += nHDelta;
    SetSizePixel( aSize );
}

// -----------------------------------------------------------------------

SvxImprovementPage::~SvxImprovementPage()
{
}

// class SvxImprovementDialog --------------------------------------------

SvxImprovementDialog::SvxImprovementDialog( Window* pParent, const String& rInfoURL ) :

    SfxSingleTabDialog( pParent, RID_SVXPAGE_IMPROVEMENT, rInfoURL ),

    m_pPage( NULL )

{
    m_pPage = new SvxImprovementPage( this );
    SetInfoLink( LINK( this, SvxImprovementDialog, HandleHyperlink ) );
    SetPage( m_pPage );
    if ( GetOKButton() )
        GetOKButton()->SetClickHdl( LINK( this, SvxImprovementDialog, HandleOK ) );
}

IMPL_LINK( SvxImprovementDialog, HandleHyperlink, svt::FixedHyperlinkImage*, pHyperlinkImage )
{
    ::rtl::OUString sURL( pHyperlinkImage->GetURL() );

    if ( sURL.getLength() > 0 )
    {
        try
        {
            uno::Reference< lang::XMultiServiceFactory > xSMGR =
                ::comphelper::getProcessServiceFactory();
            uno::Reference< XSystemShellExecute > xSystemShell(
                xSMGR->createInstance( ::rtl::OUString(
                    RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.system.SystemShellExecute" ) ) ),
                uno::UNO_QUERY_THROW );
            if ( xSystemShell.is() )
            {
                xSystemShell->execute(
                    sURL, ::rtl::OUString(), SystemShellExecuteFlags::DEFAULTS );
            }
        }
        catch( const uno::Exception& e )
        {
             OSL_TRACE( "Caught exception: %s\n thread terminated.\n",
                rtl::OUStringToOString( e.Message, RTL_TEXTENCODING_UTF8 ).getStr() );
        }
    }

    return 0;
}

IMPL_LINK( SvxImprovementDialog, HandleOK, OKButton*, EMPTYARG )
{
    uno::Reference< lang::XMultiServiceFactory > xSMGR = ::comphelper::getProcessServiceFactory();
    uno::Reference< com::sun::star::oooimprovement::XCoreController > core_c(
            xSMGR->createInstance( ::rtl::OUString::createFromAscii("com.sun.star.oooimprovement.CoreController")),
            uno::UNO_QUERY);
    if(core_c.is())
    {
        ::comphelper::ConfigurationHelper::writeDirectKey(
            xSMGR,
            ::rtl::OUString::createFromAscii("/org.openoffice.Office.OOoImprovement.Settings"),
            ::rtl::OUString::createFromAscii("Participation"),
            ::rtl::OUString::createFromAscii("ShowedInvitation"),
            uno::makeAny( true ),
            ::comphelper::ConfigurationHelper::E_STANDARD );
        ::comphelper::ConfigurationHelper::writeDirectKey(
            xSMGR,
            ::rtl::OUString::createFromAscii("/org.openoffice.Office.OOoImprovement.Settings"),
            ::rtl::OUString::createFromAscii("Participation"),
            ::rtl::OUString::createFromAscii("InvitationAccepted"),
            uno::makeAny( m_pPage->IsYesChecked() ),
            ::comphelper::ConfigurationHelper::E_STANDARD );
        // TODO: refactor
        ::comphelper::UiEventsLogger::reinit();
        ::tools::InitTestToolLib();
    }
    EndDialog( RET_OK );
    return 0;
}

// class SvxInfoWindow ---------------------------------------------------

SvxInfoWindow::SvxInfoWindow( Window* pParent, const ResId& rResId ) :
    Window( pParent, rResId ),
    m_aInfoText( this )
{
    m_aInfoText.SetPosSizePixel( Point( 10, 10 ), Size( 150, 10 ) );

	const StyleSettings& rSettings = GetSettings().GetStyleSettings();
    Wallpaper aWall( rSettings.GetWindowColor() );
    SetBackground( aWall );
    Font aNewFont( m_aInfoText.GetFont() );
	aNewFont.SetTransparent( TRUE );
    m_aInfoText.SetFont( aNewFont );
    m_aInfoText.SetBackground( aWall );
    m_aInfoText.SetControlForeground( rSettings.GetWindowTextColor() );
}

void SvxInfoWindow::SetInfoText( const String& rText )
{
    m_aInfoText.SetText( rText );
    Size aSize = m_aInfoText.CalcMinimumSize();
    Size aWinSize = GetSizePixel();
    Point aPos( ( aWinSize.Width() - aSize.Width() ) / 2, ( aWinSize.Height() - aSize.Height() ) / 2 );
    m_aInfoText.SetPosSizePixel( aPos, aSize );
}

