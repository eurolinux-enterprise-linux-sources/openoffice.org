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
#include <svtools/zforlist.hxx>
#include <goodies/grfmgr.hxx>
#include <svtools/flagitem.hxx>
#include <sfx2/dispatch.hxx>
#include <svtools/lingucfg.hxx>
#include <svtools/szitem.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/imgmgr.hxx>
#include <vcl/configsettings.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/mnemonic.hxx>
#include <i18npool/mslangid.hxx>
#include <svtools/useroptions.hxx>
#include <svtools/cacheoptions.hxx>
#include <svtools/fontoptions.hxx>
#include <svtools/menuoptions.hxx>
#include <svtools/startoptions.hxx>
#include <svtools/languageoptions.hxx>
#include <svtools/miscopt.hxx>
#include <svtools/printwarningoptions.hxx>
#include <svtools/syslocaleoptions.hxx>
#include <svtools/helpopt.hxx>
#include <svtools/accessibilityoptions.hxx>
#include <unotools/configitem.hxx>
#include <sfx2/objsh.hxx>
#include <comphelper/types.hxx>
#include <svtools/ctloptions.hxx>
#include <svtools/langtab.hxx>
#include <unotools/localfilehelper.hxx>
#include <unotools/configmgr.hxx>
#include "cuioptgenrl.hxx"
#include "optpath.hxx"
#include "optsave.hxx"
#include <svx/optlingu.hxx>
#include <svx/xpool.hxx>
#include "dlgutil.hxx"
#ifndef _SVX_TAB_AREA_HXX
#include "cuitabarea.hxx"
#endif
#ifndef _SVX_DIALOGS_HRC
#include <svx/dialogs.hrc>
#endif
#include "unolingu.hxx"
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include <svx/langitem.hxx>
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif
#include <rtl/ustrbuf.hxx>


#include <svx/dialmgr.hxx>
#include <svtools/helpopt.hxx>
#include <svtools/saveopt.hxx>

#include <com/sun/star/container/XContentEnumerationAccess.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNameReplace.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/container/XContentEnumerationAccess.hpp>
#include <com/sun/star/container/XSet.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>

#include <vcl/svapp.hxx>

#include "optgdlg.hrc"
#include "optgdlg.hxx"
#include "ofaitem.hxx"
#include <svtools/apearcfg.hxx>
#include <svtools/optionsdrawinglayer.hxx>

#define CONFIG_LANGUAGES "OfficeLanguages"

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::util;
using namespace ::utl;
using ::rtl::OString;
using ::rtl::OUString;

#define C2U(cChar) OUString::createFromAscii(cChar)

#define MAX_PROGRAM_ENTRIES		3

// class OfaMiscTabPage --------------------------------------------------

int OfaMiscTabPage::DeactivatePage( SfxItemSet* pSet_ )
{
	if ( pSet_ )
		FillItemSet( *pSet_ );
	return LEAVE_PAGE;
}

namespace
{
		::rtl::OUString impl_SystemFileOpenServiceName()
		{
			const ::rtl::OUString &rDesktopEnvironment =
				Application::GetDesktopEnvironment();

			if ( rDesktopEnvironment.equalsIgnoreAsciiCaseAscii( "gnome" ) )
			{
                #ifdef ENABLE_GTK
				return ::rtl::OUString::createFromAscii( "com.sun.star.ui.dialogs.GtkFilePicker" );
                #else
                return rtl::OUString();
                #endif
			}
			else if ( rDesktopEnvironment.equalsIgnoreAsciiCaseAscii( "kde4" ) )
			{
                #ifdef ENABLE_KDE4
				return ::rtl::OUString::createFromAscii( "com.sun.star.ui.dialogs.KDE4FilePicker" );
                #else
                return rtl::OUString();
                #endif
			}
			else if ( rDesktopEnvironment.equalsIgnoreAsciiCaseAscii( "kde" ) )
			{
                #ifdef ENABLE_KDE
				return ::rtl::OUString::createFromAscii( "com.sun.star.ui.dialogs.KDEFilePicker" );
                #else
                return rtl::OUString();
                #endif
			}
            #if defined WNT
			return ::rtl::OUString::createFromAscii( "com.sun.star.ui.dialogs.SystemFilePicker" );
            #elif (defined MACOSX && defined QUARTZ)
			return ::rtl::OUString::createFromAscii( "com.sun.star.ui.dialogs.AquaFilePicker" );
            #else
            return rtl::OUString();
            #endif
		}

		sal_Bool lcl_HasSystemFilePicker()
		{
        	Reference< XMultiServiceFactory > xFactory = comphelper::getProcessServiceFactory();
			sal_Bool bRet = sal_False;

			Reference< XContentEnumerationAccess > xEnumAccess( xFactory, UNO_QUERY );
			Reference< XSet > xSet( xFactory, UNO_QUERY );

			if ( ! xEnumAccess.is() || ! xSet.is() )
				return bRet;

			try
			{
				::rtl::OUString aFileService = impl_SystemFileOpenServiceName();
				Reference< XEnumeration > xEnum = xEnumAccess->createContentEnumeration( aFileService );
				if ( xEnum.is() && xEnum->hasMoreElements() )
					bRet = sal_True;
			}

			catch( IllegalArgumentException ) {}
			catch( ElementExistException ) {}
			return bRet;
		}
}

// -----------------------------------------------------------------------

OfaMiscTabPage::OfaMiscTabPage(Window* pParent, const SfxItemSet& rSet ) :

	SfxTabPage( pParent, SVX_RES( OFA_TP_MISC ), rSet ),

	aHelpFL				( this, SVX_RES( FL_HELP ) ),
	aToolTipsCB			( this, SVX_RES( CB_TOOLTIP ) ),
	aExtHelpCB			( this, SVX_RES( CB_EXTHELP ) ),
	aHelpAgentCB		( this, SVX_RES( CB_HELPAGENT ) ),
	aHelpAgentResetBtn	( this, SVX_RES( PB_HELPAGENT_RESET ) ),
	aHelpFormatFT		( this, SVX_RES( FT_HELPFORMAT ) ),
	aHelpFormatLB		( this, SVX_RES( LB_HELPFORMAT ) ),
	aFileDlgFL			( this, SVX_RES( FL_FILEDLG ) ),
    aFileDlgROImage     ( this, SVX_RES( FI_FILEDLG_RO ) ),
	aFileDlgCB			( this, SVX_RES( CB_FILEDLG ) ),
	aPrintDlgFL			( this, SVX_RES( FL_PRINTDLG ) ),
	aPrintDlgCB			( this, SVX_RES( CB_PRINTDLG ) ),
	aDocStatusFL		( this, SVX_RES( FL_DOCSTATUS ) ),
	aDocStatusCB		( this, SVX_RES( CB_DOCSTATUS ) ),
	aTwoFigureFL		( this, SVX_RES( FL_TWOFIGURE ) ),
	aInterpretFT		( this, SVX_RES( FT_INTERPRET ) ),
	aYearValueField		( this, SVX_RES( NF_YEARVALUE ) ),
	aToYearFT			( this, SVX_RES( FT_TOYEAR ) )

{
	FreeResource();

	if (!lcl_HasSystemFilePicker())
	{
        aFileDlgFL.Hide();
        aFileDlgCB.Hide();
	}

    #if ! defined(QUARTZ)
    aPrintDlgFL.Hide();
    aPrintDlgCB.Hide();
    #endif

    if ( !aFileDlgCB.IsVisible() )
    {
        // rearrange the following controls
        Point aNewPos = aDocStatusFL.GetPosPixel();
        long nDelta = aNewPos.Y() - aFileDlgFL.GetPosPixel().Y();

        Window* pWins[] =
        {
            &aPrintDlgFL, &aPrintDlgCB, &aDocStatusFL, &aDocStatusCB, &aTwoFigureFL,
            &aInterpretFT, &aYearValueField, &aToYearFT
        };
        Window** pCurrent = pWins;
        const sal_Int32 nCount = sizeof( pWins ) / sizeof( pWins[ 0 ] );
        for ( sal_Int32 i = 0; i < nCount; ++i, ++pCurrent )
        {
            aNewPos = (*pCurrent)->GetPosPixel();
            aNewPos.Y() -= nDelta;
            (*pCurrent)->SetPosPixel( aNewPos );
        }
    }
    else if ( SvtMiscOptions().IsUseSystemFileDialogReadOnly() )
    {
        aFileDlgROImage.Show();
        aFileDlgCB.Disable();
    }

    if ( aPrintDlgCB.IsVisible() )
    {
        // rearrange the following controls
        Point aNewPos = aDocStatusFL.GetPosPixel();
        long nDelta = aNewPos.Y() - aFileDlgFL.GetPosPixel().Y();

        Window* pWins[] =
        {
            &aDocStatusFL, &aDocStatusCB, &aTwoFigureFL,
            &aInterpretFT, &aYearValueField, &aToYearFT
        };
        Window** pCurrent = pWins;
        const sal_Int32 nCount = sizeof( pWins ) / sizeof( pWins[ 0 ] );
        for ( sal_Int32 i = 0; i < nCount; ++i, ++pCurrent )
        {
            aNewPos = (*pCurrent)->GetPosPixel();
            aNewPos.Y() += nDelta;
            (*pCurrent)->SetPosPixel( aNewPos );
        }
    }

    // at least the button is as wide as its text
	long nTxtWidth = aHelpAgentResetBtn.GetTextWidth( aHelpAgentResetBtn.GetText() );
	Size aBtnSz = aHelpAgentResetBtn.GetSizePixel();
	if ( aBtnSz.Width() < nTxtWidth )
	{
		aBtnSz.Width() = nTxtWidth;
		aHelpAgentResetBtn.SetSizePixel( aBtnSz );
	}

    aStrDateInfo = aToYearFT.GetText();
    aYearValueField.SetModifyHdl( LINK( this, OfaMiscTabPage, TwoFigureHdl ) );
	Link aLink = LINK( this, OfaMiscTabPage, TwoFigureConfigHdl );
	aYearValueField.SetDownHdl( aLink );
	aYearValueField.SetUpHdl( aLink );
	aYearValueField.SetLoseFocusHdl( aLink );
	aYearValueField.SetFirstHdl( aLink );
	TwoFigureConfigHdl( &aYearValueField );

    SetExchangeSupport();

	aLink = LINK( this, OfaMiscTabPage, HelpCheckHdl_Impl );
	aToolTipsCB.SetClickHdl( aLink );
	aHelpAgentCB.SetClickHdl( aLink );
	aHelpAgentResetBtn.SetClickHdl( LINK( this, OfaMiscTabPage, HelpAgentResetHdl_Impl ) );

    //fill default names as user data
    static const char* aHelpFormatNames[] =
	{
        "Default",
        "HighContrast1",
        "HighContrast2",
        "HighContrastBlack",
        "HighContrastWhite"
    };

    for ( USHORT i = 0; i < aHelpFormatLB.GetEntryCount(); i++ )
    {
        String* pData = new String( String::CreateFromAscii( aHelpFormatNames[i] ) );
        aHelpFormatLB.SetEntryData( i, pData );
    }
}

// -----------------------------------------------------------------------

OfaMiscTabPage::~OfaMiscTabPage()
{
    for(USHORT i = 0; i < aHelpFormatLB.GetEntryCount(); i++)
    {
        delete static_cast< String* >( aHelpFormatLB.GetEntryData(i) );
    }
}

// -----------------------------------------------------------------------

SfxTabPage*	OfaMiscTabPage::Create( Window* pParent, const SfxItemSet& rAttrSet )
{
	return new OfaMiscTabPage( pParent, rAttrSet );
}

// -----------------------------------------------------------------------

BOOL OfaMiscTabPage::FillItemSet( SfxItemSet& rSet )
{
	BOOL bModified = FALSE;

    SvtHelpOptions aHelpOptions;
    BOOL bChecked = aToolTipsCB.IsChecked();
	if ( bChecked != aToolTipsCB.GetSavedValue() )
		aHelpOptions.SetHelpTips( bChecked );
    bChecked = ( aExtHelpCB.IsChecked() && aToolTipsCB.IsChecked() );
	if ( bChecked != aExtHelpCB.GetSavedValue() )
		aHelpOptions.SetExtendedHelp( bChecked );
    bChecked = aHelpAgentCB.IsChecked();
	if ( bChecked != aHelpAgentCB.GetSavedValue() )
		aHelpOptions.SetHelpAgentAutoStartMode( bChecked );
	USHORT nHelpFormatPos = aHelpFormatLB.GetSelectEntryPos();
    if ( nHelpFormatPos != LISTBOX_ENTRY_NOTFOUND &&
         nHelpFormatPos != aHelpFormatLB.GetSavedValue() )
    {
        aHelpOptions.SetHelpStyleSheet( *static_cast< String* >( aHelpFormatLB.GetEntryData( nHelpFormatPos ) ) );
    }

    if ( aFileDlgCB.IsChecked() != aFileDlgCB.GetSavedValue() )
    {
        SvtMiscOptions aMiscOpt;
        aMiscOpt.SetUseSystemFileDialog( !aFileDlgCB.IsChecked() );
        bModified = TRUE;
    }

    if ( aPrintDlgCB.IsChecked() != aPrintDlgCB.GetSavedValue() )
    {
        SvtMiscOptions aMiscOpt;
        aMiscOpt.SetUseSystemPrintDialog( !aPrintDlgCB.IsChecked() );
        bModified = TRUE;
    }

	if ( aDocStatusCB.IsChecked() != aDocStatusCB.GetSavedValue() )
    {
        SvtPrintWarningOptions aPrintOptions;
        aPrintOptions.SetModifyDocumentOnPrintingAllowed( aDocStatusCB.IsChecked() );
        bModified = TRUE;
    }

	const SfxUInt16Item* pUInt16Item =
		PTR_CAST( SfxUInt16Item, GetOldItem( rSet, SID_ATTR_YEAR2000 ) );
	USHORT nNum = (USHORT)aYearValueField.GetText().ToInt32();
    if ( pUInt16Item && pUInt16Item->GetValue() != nNum )
	{
		bModified = TRUE;
		rSet.Put( SfxUInt16Item( SID_ATTR_YEAR2000, nNum ) );
	}

    return bModified;
}

// -----------------------------------------------------------------------

void OfaMiscTabPage::Reset( const SfxItemSet& rSet )
{
	SvtHelpOptions aHelpOptions;
	aToolTipsCB.Check( aHelpOptions.IsHelpTips() );
	aExtHelpCB.Check( aHelpOptions.IsHelpTips() && aHelpOptions.IsExtendedHelp() );
	aHelpAgentCB.Check( aHelpOptions.IsHelpAgentAutoStartMode() );
	String sStyleSheet = aHelpOptions.GetHelpStyleSheet();
    for ( USHORT i = 0; i < aHelpFormatLB.GetEntryCount(); ++i )
    {
        if ( *static_cast< String* >( aHelpFormatLB.GetEntryData(i) ) == sStyleSheet )
        {
            aHelpFormatLB.SelectEntryPos(i);
            break;
        }
    }

	aToolTipsCB.SaveValue();
	aExtHelpCB.SaveValue();
	aHelpAgentCB.SaveValue();
    aHelpFormatLB.SaveValue();
    HelpCheckHdl_Impl( &aHelpAgentCB );

    SvtMiscOptions aMiscOpt;
    aFileDlgCB.Check( !aMiscOpt.UseSystemFileDialog() );
    aFileDlgCB.SaveValue();
    aPrintDlgCB.Check( !aMiscOpt.UseSystemPrintDialog() );
    aPrintDlgCB.SaveValue();

    SvtPrintWarningOptions aPrintOptions;
    aDocStatusCB.Check(aPrintOptions.IsModifyDocumentOnPrintingAllowed());
    aDocStatusCB.SaveValue();

	const SfxPoolItem* pItem = NULL;
	if ( SFX_ITEM_SET == rSet.GetItemState( SID_ATTR_YEAR2000, FALSE, &pItem ) )
    {
		aYearValueField.SetValue( ((SfxUInt16Item*)pItem)->GetValue() );
        TwoFigureConfigHdl( &aYearValueField );
    }
    else
    {
        aYearValueField.Enable(FALSE);
        aTwoFigureFL.Enable(FALSE);
        aInterpretFT.Enable(FALSE);
        aToYearFT.Enable(FALSE);
    }
}

// -----------------------------------------------------------------------

IMPL_LINK( OfaMiscTabPage, TwoFigureHdl, NumericField*, pEd )
{
	(void)pEd;

	String aOutput( aStrDateInfo );
	String aStr( aYearValueField.GetText() );
    String sSep( SvtSysLocale().GetLocaleData().getNumThousandSep() );
    xub_StrLen nIndex = 0;
    while ((nIndex = aStr.Search( sSep, nIndex)) != STRING_NOTFOUND)
        aStr.Erase( nIndex, sSep.Len());
	long nNum = aStr.ToInt32();
    if ( aStr.Len() != 4 || nNum < aYearValueField.GetMin() || nNum > aYearValueField.GetMax() )
		aOutput.AppendAscii("????");
	else
	{
		nNum += 99;
		aOutput += String::CreateFromInt32( nNum );
	}
	aToYearFT.SetText( aOutput );
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( OfaMiscTabPage, TwoFigureConfigHdl, NumericField*, pEd )
{
	sal_Int64 nNum = aYearValueField.GetValue();
	String aOutput( String::CreateFromInt64( nNum ) );
	aYearValueField.SetText( aOutput );
	aYearValueField.SetSelection( Selection( 0, aOutput.Len() ) );
	TwoFigureHdl( pEd );
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( OfaMiscTabPage, HelpCheckHdl_Impl, CheckBox*, EMPTYARG )
{
	aExtHelpCB.Enable( aToolTipsCB.IsChecked() );
	aHelpAgentResetBtn.Enable( aHelpAgentCB.IsChecked() );
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( OfaMiscTabPage, HelpAgentResetHdl_Impl, PushButton*, EMPTYARG )
{
	SvtHelpOptions().resetAgentIgnoreURLCounter();
	return 0;
}

// -----------------------------------------------------------------------

// -------------------------------------------------------------------
class CanvasSettings
{
public:
    CanvasSettings();

    BOOL    IsHardwareAccelerationEnabled() const;
    BOOL    IsHardwareAccelerationAvailable() const;
    void    EnabledHardwareAcceleration( BOOL _bEnabled ) const;

private:
    typedef std::vector< std::pair<OUString,Sequence<OUString> > > ServiceVector;

    Reference<XNameAccess> mxForceFlagNameAccess;
    ServiceVector          maAvailableImplementations;
    mutable BOOL           mbHWAccelAvailable;
    mutable BOOL           mbHWAccelChecked;
};

// -------------------------------------------------------------------
CanvasSettings::CanvasSettings() :
    mxForceFlagNameAccess(),
    mbHWAccelAvailable(FALSE),
    mbHWAccelChecked(FALSE)
{
    try
    {
        Reference< XMultiServiceFactory > xFactory = comphelper::getProcessServiceFactory();
        Reference<XMultiServiceFactory> xConfigProvider(
            xFactory->createInstance(
                OUString::createFromAscii("com.sun.star.configuration.ConfigurationProvider")), 
				UNO_QUERY_THROW );

        Any propValue(
            makeAny( PropertyValue(
                         OUString::createFromAscii("nodepath"), -1,
                         makeAny( OUString::createFromAscii("/org.openoffice.Office.Canvas") ),
                         PropertyState_DIRECT_VALUE ) ) );

        mxForceFlagNameAccess.set(
            xConfigProvider->createInstanceWithArguments(
                OUString::createFromAscii("com.sun.star.configuration.ConfigurationUpdateAccess"),
                Sequence<Any>( &propValue, 1 ) ), 
            UNO_QUERY_THROW );

        propValue = makeAny( 
            PropertyValue(
                OUString::createFromAscii("nodepath"), -1,
                makeAny( OUString::createFromAscii("/org.openoffice.Office.Canvas/CanvasServiceList") ),
                PropertyState_DIRECT_VALUE ) );

        Reference<XNameAccess> xNameAccess(
            xConfigProvider->createInstanceWithArguments(
                OUString::createFromAscii("com.sun.star.configuration.ConfigurationAccess"),
                Sequence<Any>( &propValue, 1 ) ), UNO_QUERY_THROW );
        Reference<XHierarchicalNameAccess> xHierarchicalNameAccess(
            xNameAccess, UNO_QUERY_THROW);

        Sequence<OUString> serviceNames = xNameAccess->getElementNames();
        const OUString* pCurr = serviceNames.getConstArray();
        const OUString* const pEnd = pCurr + serviceNames.getLength();
        while( pCurr != pEnd )
        {
            Reference<XNameAccess> xEntryNameAccess(
                xHierarchicalNameAccess->getByHierarchicalName(*pCurr), 
                UNO_QUERY );

            if( xEntryNameAccess.is() )
            {
                Sequence<OUString> preferredImplementations;
                if( (xEntryNameAccess->getByName( OUString::createFromAscii("PreferredImplementations") ) >>= preferredImplementations) )
                    maAvailableImplementations.push_back( std::make_pair(*pCurr,preferredImplementations) );
            }

            ++pCurr;
        }
    }
    catch( Exception& )
    {
    }
}

// -------------------------------------------------------------------
BOOL CanvasSettings::IsHardwareAccelerationAvailable() const
{
    if( !mbHWAccelChecked )
    {
        mbHWAccelChecked = true;

        Reference< XMultiServiceFactory > xFactory = comphelper::getProcessServiceFactory();

        // check whether any of the service lists has an
        // implementation that presents the "HardwareAcceleration" property
        ServiceVector::const_iterator       aCurr=maAvailableImplementations.begin();
        const ServiceVector::const_iterator aEnd=maAvailableImplementations.end();
        while( aCurr != aEnd )
        {
            const OUString* pCurrImpl = aCurr->second.getConstArray();
            const OUString* const pEndImpl = pCurrImpl + aCurr->second.getLength();

            while( pCurrImpl != pEndImpl ) 
            {
                try 
                {
                    Reference<XPropertySet> xPropSet( xFactory->createInstance(
                                                          pCurrImpl->trim() ),
                                                      UNO_QUERY_THROW );
                    bool bHasAccel(false);
                    if( (xPropSet->getPropertyValue(OUString::createFromAscii("HardwareAcceleration")) >>= bHasAccel) )
                        if( bHasAccel )
                        {
                            mbHWAccelAvailable = true;
                            return mbHWAccelAvailable;
                        }
                }
                catch (Exception &) 
                {}

                ++pCurrImpl;
            }

			++aCurr;
        }
    }

    return mbHWAccelAvailable;
}

// -------------------------------------------------------------------
BOOL CanvasSettings::IsHardwareAccelerationEnabled() const
{
    bool bForceLastEntry(false);
    if( !mxForceFlagNameAccess.is() )
        return true;

    if( !(mxForceFlagNameAccess->getByName( OUString::createFromAscii("ForceSafeServiceImpl") ) >>= bForceLastEntry) )
        return true;

    return !bForceLastEntry;
}

// -------------------------------------------------------------------
void CanvasSettings::EnabledHardwareAcceleration( BOOL _bEnabled ) const
{
    Reference< XNameReplace > xNameReplace( 
        mxForceFlagNameAccess, UNO_QUERY );

    if( !xNameReplace.is() )
        return;
    
    xNameReplace->replaceByName( OUString::createFromAscii("ForceSafeServiceImpl"),
                                 makeAny(!_bEnabled) );

    Reference< XChangesBatch > xChangesBatch( 
        mxForceFlagNameAccess, UNO_QUERY );

    if( !xChangesBatch.is() )
        return;

    xChangesBatch->commitChanges();
}

// class OfaViewTabPage --------------------------------------------------

OfaViewTabPage::OfaViewTabPage(Window* pParent, const SfxItemSet& rSet ) :

	SfxTabPage( pParent, SVX_RES( OFA_TP_VIEW ), rSet ),

    aUserInterfaceFL    ( this, SVX_RES( FL_USERINTERFACE ) ),
    aWindowSizeFT       ( this, SVX_RES( FT_WINDOWSIZE ) ),
    aWindowSizeMF       ( this, SVX_RES( MF_WINDOWSIZE ) ),
    aIconSizeStyleFT    ( this, SVX_RES( FT_ICONSIZESTYLE ) ),
    aIconSizeLB              ( this, SVX_RES( LB_ICONSIZE ) ),
    aIconStyleLB        ( this, SVX_RES( LB_ICONSTYLE ) ),
    m_aSystemFont				(this, SVX_RES( CB_SYSTEM_FONT ) ),
#if defined( UNX )
	aFontAntiAliasing	( this, SVX_RES( CB_FONTANTIALIASING )),
	aAAPointLimitLabel	( this, SVX_RES( FT_POINTLIMIT_LABEL )),
	aAAPointLimit		( this, SVX_RES( NF_AA_POINTLIMIT )),
	aAAPointLimitUnits	( this, SVX_RES( FT_POINTLIMIT_UNIT )),
#endif
    aMenuFL             ( this, SVX_RES( FL_MENU ) ),
    aMenuIconsFT        ( this, SVX_RES( FT_MENU_ICONS )),
    aMenuIconsLB        ( this, SVX_RES( LB_MENU_ICONS )),
    aFontListsFL        ( this, SVX_RES( FL_FONTLISTS) ),
    aFontShowCB         ( this, SVX_RES( CB_FONT_SHOW ) ),
    aFontHistoryCB      ( this, SVX_RES( CB_FONT_HISTORY ) ),
    aRenderingFL        ( this, SVX_RES( FL_RENDERING ) ),
    aUseHardwareAccell  ( this, SVX_RES( CB_USE_HARDACCELL ) ),
    aUseAntiAliase		( this, SVX_RES( CB_USE_ANTIALIASE ) ),
    aMouseFL            ( this, SVX_RES( FL_MOUSE ) ),
    aMousePosFT         ( this, SVX_RES( FT_MOUSEPOS ) ),
    aMousePosLB         ( this, SVX_RES( LB_MOUSEPOS ) ),
    aMouseMiddleFT      ( this, SVX_RES( FT_MOUSEMIDDLE ) ),
    aMouseMiddleLB      ( this, SVX_RES( LB_MOUSEMIDDLE ) ),

	// #i97672#
    maSelectionFL(this, SVX_RES(FL_SELECTION)),
    maSelectionCB(this, SVX_RES(CB_SELECTION)),
    maSelectionMF(this, SVX_RES(MF_SELECTION)),

	nSizeLB_InitialSelection(0),
    nStyleLB_InitialSelection(0),
    pAppearanceCfg(new SvtTabAppearanceCfg),
    pCanvasSettings(new CanvasSettings),
	mpDrawinglayerOpt(new SvtOptionsDrawinglayer)
{
#if defined( UNX )
	aFontAntiAliasing.SetToggleHdl( LINK( this, OfaViewTabPage, OnAntialiasingToggled ) );

	// depending on the size of the text in aAAPointLimitLabel, we slightly re-arrange aAAPointLimit and aAAPointLimitUnits
    //#110391#  if the label has no mnemonic and we are in a CJK version the mnemonic "(X)" will be added which
    //          influences the width calculation
    MnemonicGenerator aMnemonicGenerator;
    String sLabel(aAAPointLimitLabel.GetText());
    aMnemonicGenerator.RegisterMnemonic( sLabel );
    aMnemonicGenerator.CreateMnemonic( sLabel );
    sLabel.EraseAllChars('~');

    sal_Int32 nLabelWidth = aAAPointLimitLabel.GetTextWidth( sLabel );
	nLabelWidth += 3;	// small gap
	// pixels to move both controls to the left
	Size aSize = aAAPointLimitLabel.GetSizePixel();
	sal_Int32 nMoveLeft = aSize.Width() - nLabelWidth;
	// resize the first label
	aSize.Width() = nLabelWidth;
	aAAPointLimitLabel.SetSizePixel( aSize );

	// move the numeric field
	Point aPos( aAAPointLimit.GetPosPixel() );
	aPos.X() -= nMoveLeft;
	aAAPointLimit.SetPosPixel( aPos );

	// move (and resize) the units FixedText
	aPos = ( aAAPointLimitUnits.GetPosPixel() );
	aPos.X() -= nMoveLeft;
	aSize = aAAPointLimitUnits.GetSizePixel();
	aSize.Width() += nMoveLeft;
	aAAPointLimitUnits.SetPosSizePixel( aPos, aSize );
#else
	// on this platform, we do not have the anti aliasing options - move the other checkboxes accordingly
	// (in the resource, the coordinates are calculated for the AA options beeing present)
	Control* pMiscOptions[] =
	{
        &aMenuFL, &aMenuIconsFT, &aMenuIconsLB,
        &aFontListsFL, &aFontShowCB, &aFontHistoryCB
	};

	// temporaryly create the checkbox for the anti aliasing (we need to to determine it's pos)
	CheckBox* pFontAntiAliasing	= new CheckBox( this, SVX_RES( CB_FONTANTIALIASING ) );
    sal_Int32 nMoveUp = aMenuFL.GetPosPixel().Y() - pFontAntiAliasing->GetPosPixel().Y();
	DELETEZ( pFontAntiAliasing );

	Point aPos;
	for ( sal_Int32 i = 0; i < sizeof( pMiscOptions ) / sizeof( pMiscOptions[0] ); ++i )
	{
		aPos = pMiscOptions[i]->GetPosPixel( );
		aPos.Y() -= nMoveUp;
		pMiscOptions[i]->SetPosPixel( aPos );
	}

#endif

	// #i97672#
	maSelectionCB.SetToggleHdl( LINK( this, OfaViewTabPage, OnSelectionToggled ) );

	FreeResource();

    if( ! Application::ValidateSystemFont() )
    {
        m_aSystemFont.Check( FALSE );
        m_aSystemFont.Enable( FALSE );
    }

    // add real theme name to 'auto' theme, e.g. 'auto' => 'auto (classic)'
    if( aIconStyleLB.GetEntryCount() > 1 )
    {
        ::rtl::OUString aAutoStr( aIconStyleLB.GetEntry( 0 ) );

        aAutoStr += ::rtl::OUString::createFromAscii( " (" );

        switch( Application::GetSettings().GetStyleSettings().GetAutoSymbolsStyle() )
        {
            case STYLE_SYMBOLS_DEFAULT:     aAutoStr += aIconStyleLB.GetEntry( 1 ); break;
            case STYLE_SYMBOLS_INDUSTRIAL:  aAutoStr += aIconStyleLB.GetEntry( 2 ); break;
            case STYLE_SYMBOLS_CRYSTAL:     aAutoStr += aIconStyleLB.GetEntry( 3 ); break;
            case STYLE_SYMBOLS_TANGO:       aAutoStr += aIconStyleLB.GetEntry( 4 ); break;
            case STYLE_SYMBOLS_CLASSIC:     aAutoStr += aIconStyleLB.GetEntry( 5 ); break;
            case STYLE_SYMBOLS_HICONTRAST:  aAutoStr += aIconStyleLB.GetEntry( 6 ); break;
        }

        aIconStyleLB.RemoveEntry( 0 );
        aIconStyleLB.InsertEntry( aAutoStr += ::rtl::OUString::createFromAscii( ")" ), 0 );
        aIconStyleLB.SetSeparatorPos( aIconStyleLB.GetEntryCount() - 2 );
    }
}

OfaViewTabPage::~OfaViewTabPage()
{
	delete mpDrawinglayerOpt;
    delete pCanvasSettings;
    delete pAppearanceCfg;
}

#if defined( UNX )
//--- 20.08.01 10:16:12 ---------------------------------------------------
IMPL_LINK( OfaViewTabPage, OnAntialiasingToggled, void*, NOTINTERESTEDIN )
{
	(void)NOTINTERESTEDIN;

	sal_Bool bAAEnabled = aFontAntiAliasing.IsChecked();

	aAAPointLimitLabel.Enable( bAAEnabled );
	aAAPointLimit.Enable( bAAEnabled );
	aAAPointLimitUnits.Enable( bAAEnabled );

	return 0L;
}
#endif

// #i97672#
IMPL_LINK( OfaViewTabPage, OnSelectionToggled, void*, NOTINTERESTEDIN )
{
	(void)NOTINTERESTEDIN;
	const bool bSelectionEnabled(maSelectionCB.IsChecked());
	maSelectionMF.Enable(bSelectionEnabled);
	return 0;
}

/*-----------------06.12.96 11.50-------------------

--------------------------------------------------*/

SfxTabPage*	OfaViewTabPage::Create( Window* pParent, const SfxItemSet& rAttrSet )
{
	return new OfaViewTabPage(pParent, rAttrSet);
}

/*-----------------06.12.96 11.50-------------------

--------------------------------------------------*/

BOOL OfaViewTabPage::FillItemSet( SfxItemSet& )
{
	SvtFontOptions aFontOpt;
	SvtMenuOptions aMenuOpt;
	SvtStartOptions aStartOpt;

	BOOL bModified = FALSE;
	BOOL bMenuOptModified = FALSE;
	bool bRepaintWindows(false);

    SvtMiscOptions aMiscOptions;
    UINT16 nSizeLB_NewSelection = aIconSizeLB.GetSelectEntryPos();
    if( nSizeLB_InitialSelection != nSizeLB_NewSelection )
	{
		// from now on it's modified, even if via auto setting the same size was set as now selected in the LB
        sal_Int16 eSet = SFX_SYMBOLS_SIZE_AUTO;
        switch( nSizeLB_NewSelection )
		{
            case 0: eSet = SFX_SYMBOLS_SIZE_AUTO;  break;
            case 1: eSet = SFX_SYMBOLS_SIZE_SMALL; break;
            case 2: eSet = SFX_SYMBOLS_SIZE_LARGE; break;
            default:
                DBG_ERROR( "OfaViewTabPage::FillItemSet(): This state of aIconSizeLB should not be possible!" );
		}
        aMiscOptions.SetSymbolsSize( eSet );
    }

    UINT16 nStyleLB_NewSelection = aIconStyleLB.GetSelectEntryPos();
    if( nStyleLB_InitialSelection != nStyleLB_NewSelection )
    {
        sal_Int16 eSet = SFX_SYMBOLS_STYLE_AUTO;
        switch( nStyleLB_NewSelection )
        {
            case 0: eSet = SFX_SYMBOLS_STYLE_AUTO;       break;
            case 1: eSet = SFX_SYMBOLS_STYLE_DEFAULT;    break;
            case 2: eSet = SFX_SYMBOLS_STYLE_HICONTRAST; break;
            case 3: eSet = SFX_SYMBOLS_STYLE_INDUSTRIAL; break;
            case 4: eSet = SFX_SYMBOLS_STYLE_CRYSTAL;    break;
            case 5: eSet = SFX_SYMBOLS_STYLE_TANGO;      break;
            case 6: eSet = SFX_SYMBOLS_STYLE_CLASSIC;    break;
            default:
                DBG_ERROR( "OfaViewTabPage::FillItemSet(): This state of aIconStyleLB should not be possible!" );
        }
        aMiscOptions.SetSymbolsStyle( eSet );
    }

    BOOL bAppearanceChanged = FALSE;


	// Screen Scaling
    UINT16 nOldScale = pAppearanceCfg->GetScaleFactor();
	UINT16 nNewScale = (UINT16)aWindowSizeMF.GetValue();

	if ( nNewScale != nOldScale )
	{
        pAppearanceCfg->SetScaleFactor(nNewScale);
        bAppearanceChanged = TRUE;
	}

	// Mouse Snap Mode
    short eOldSnap = pAppearanceCfg->GetSnapMode();
    short eNewSnap = aMousePosLB.GetSelectEntryPos();
    if(eNewSnap > 2)
        eNewSnap = 2;

	if ( eNewSnap != eOldSnap )
	{
        pAppearanceCfg->SetSnapMode(eNewSnap );
        bAppearanceChanged = TRUE;
    }

    // Middle Mouse Button
    short eOldMiddleMouse = pAppearanceCfg->GetMiddleMouseButton();
    short eNewMiddleMouse = aMouseMiddleLB.GetSelectEntryPos();
    if(eNewMiddleMouse > 2)
        eNewMiddleMouse = 2;

    if ( eNewMiddleMouse != eOldMiddleMouse )
	{
        pAppearanceCfg->SetMiddleMouseButton( eNewMiddleMouse );
        bAppearanceChanged = TRUE;
    }

#if defined( UNX )
	if ( aFontAntiAliasing.IsChecked() != aFontAntiAliasing.GetSavedValue() )
	{
        pAppearanceCfg->SetFontAntiAliasing( aFontAntiAliasing.IsChecked() );
        bAppearanceChanged = TRUE;
    }

	if ( aAAPointLimit.GetValue() != aAAPointLimit.GetSavedValue().ToInt32() )
	{
        pAppearanceCfg->SetFontAntialiasingMinPixelHeight( aAAPointLimit.GetValue() );
        bAppearanceChanged = TRUE;
    }
#endif

    if ( aFontShowCB.IsChecked() != aFontShowCB.GetSavedValue() )
	{
		aFontOpt.EnableFontWYSIWYG( aFontShowCB.IsChecked() );
		bModified = TRUE;
	}

    if(aMenuIconsLB.GetSelectEntryPos() != aMenuIconsLB.GetSavedValue())
    {
        aMenuOpt.SetMenuIconsState( aMenuIconsLB.GetSelectEntryPos() == 0 ? 2 : aMenuIconsLB.GetSelectEntryPos() - 1);
        bModified = TRUE;
        bMenuOptModified = TRUE;
    	bAppearanceChanged = TRUE;
	}

	if ( aFontHistoryCB.IsChecked() != aFontHistoryCB.GetSavedValue() )
	{
		aFontOpt.EnableFontHistory( aFontHistoryCB.IsChecked() );
		bModified = TRUE;
	}

	// #i95644#  if disabled, do not use value, see in ::Reset()
    if(aUseHardwareAccell.IsEnabled())
    {
        if(aUseHardwareAccell.IsChecked() != aUseHardwareAccell.GetSavedValue())
        {
            pCanvasSettings->EnabledHardwareAcceleration(aUseHardwareAccell.IsChecked());
	        bModified = TRUE;
        }
    }

	// #i95644#  if disabled, do not use value, see in ::Reset()
	if(aUseAntiAliase.IsEnabled())
	{
	    if(aUseAntiAliase.IsChecked() != mpDrawinglayerOpt->IsAntiAliasing())
	    {
			mpDrawinglayerOpt->SetAntiAliasing(aUseAntiAliase.IsChecked());
			bModified = TRUE;
			bRepaintWindows = true;
		}
	}

	// #i97672#
    if(maSelectionCB.IsEnabled())
    {
        const bool bNewSelection(maSelectionCB.IsChecked());
        const sal_uInt16 nNewTransparence((sal_uInt16)maSelectionMF.GetValue());

        if(bNewSelection != (bool)mpDrawinglayerOpt->IsTransparentSelection())
	    {
		    mpDrawinglayerOpt->SetTransparentSelection(maSelectionCB.IsChecked());
		    bModified = TRUE;
		    bRepaintWindows = true;
	    }

        // #i104150# even read the value when maSelectionMF is disabled; it may have been
        // modified by enabling-modify-disabling by the user
	    if(nNewTransparence != mpDrawinglayerOpt->GetTransparentSelectionPercent())
	    {
		    mpDrawinglayerOpt->SetTransparentSelectionPercent(nNewTransparence);
		    bModified = TRUE;
		    bRepaintWindows = true;
	    }
    }

	SvtAccessibilityOptions 	aAccessibilityOptions;
    if( aAccessibilityOptions.GetIsSystemFont() != m_aSystemFont.IsChecked() &&
        m_aSystemFont.IsEnabled() )
    {
        aAccessibilityOptions.SetIsSystemFont( m_aSystemFont.IsChecked() );
        bModified = TRUE;
        bMenuOptModified = TRUE;
    }

    if( bMenuOptModified )
    {
		// Set changed settings to the application instance
		AllSettings aAllSettings = Application::GetSettings();
        StyleSettings aStyleSettings = aAllSettings.GetStyleSettings();
        if( m_aSystemFont.IsEnabled() )
            aStyleSettings.SetUseSystemUIFonts( m_aSystemFont.IsChecked() );
        aAllSettings.SetStyleSettings(aStyleSettings);
        Application::MergeSystemSettings( aAllSettings );
		Application::SetSettings(aAllSettings);
    }

    if ( bAppearanceChanged )
    {
        pAppearanceCfg->Commit();
        pAppearanceCfg->SetApplicationDefaults ( GetpApp() );
    }

	if(bRepaintWindows)
	{
		Window* pAppWindow = Application::GetFirstTopLevelWindow();

		while(pAppWindow)
		{
			pAppWindow->Invalidate();
			pAppWindow = Application::GetNextTopLevelWindow(pAppWindow);
		}
	}

    return bModified;
}

/*-----------------06.12.96 11.50-------------------

--------------------------------------------------*/
void OfaViewTabPage::Reset( const SfxItemSet& )
{
    SvtMiscOptions aMiscOptions;

    if( aMiscOptions.GetSymbolsSize() != SFX_SYMBOLS_SIZE_AUTO )
        nSizeLB_InitialSelection = ( aMiscOptions.AreCurrentSymbolsLarge() )? 2 : 1;
    aIconSizeLB.SelectEntryPos( nSizeLB_InitialSelection );
    aIconSizeLB.SaveValue();

    if( aMiscOptions.GetSymbolsStyle() != SFX_SYMBOLS_STYLE_AUTO )
    {
        switch ( aMiscOptions.GetCurrentSymbolsStyle() )
        {
            case SFX_SYMBOLS_STYLE_DEFAULT:    nStyleLB_InitialSelection = 1; break;
            case SFX_SYMBOLS_STYLE_HICONTRAST: nStyleLB_InitialSelection = 2; break;
            case SFX_SYMBOLS_STYLE_INDUSTRIAL: nStyleLB_InitialSelection = 3; break;
            case SFX_SYMBOLS_STYLE_CRYSTAL:    nStyleLB_InitialSelection = 4; break;
            case SFX_SYMBOLS_STYLE_TANGO:      nStyleLB_InitialSelection = 5; break;
            case SFX_SYMBOLS_STYLE_CLASSIC:    nStyleLB_InitialSelection = 6; break;
            default:                           nStyleLB_InitialSelection = 0; break;
        }
    }

    aIconStyleLB.SelectEntryPos( nStyleLB_InitialSelection );
    aIconStyleLB.SaveValue();

    if( m_aSystemFont.IsEnabled() )
    {
        SvtAccessibilityOptions aAccessibilityOptions;
        m_aSystemFont.Check( aAccessibilityOptions.GetIsSystemFont() );
    }

	// Screen Scaling
    aWindowSizeMF.SetValue ( pAppearanceCfg->GetScaleFactor() );
	// Mouse Snap
    aMousePosLB.SelectEntryPos(pAppearanceCfg->GetSnapMode());
    aMousePosLB.SaveValue();

    // Mouse Snap
    aMouseMiddleLB.SelectEntryPos(pAppearanceCfg->GetMiddleMouseButton());
    aMouseMiddleLB.SaveValue();

#if defined( UNX )
    aFontAntiAliasing.Check( pAppearanceCfg->IsFontAntiAliasing() );
    aAAPointLimit.SetValue( pAppearanceCfg->GetFontAntialiasingMinPixelHeight() );
#endif

    // WorkingSet
    SvtFontOptions aFontOpt;
	aFontShowCB.Check( aFontOpt.IsFontWYSIWYGEnabled() );
    SvtMenuOptions aMenuOpt;
    aMenuIconsLB.SelectEntryPos(aMenuOpt.GetMenuIconsState() == 2 ? 0 : aMenuOpt.GetMenuIconsState() + 1);
    aMenuIconsLB.SaveValue();
    aFontHistoryCB.Check( aFontOpt.IsFontHistoryEnabled() );

    { // #i95644# HW accel (unified to disable mechanism)
        if(pCanvasSettings->IsHardwareAccelerationAvailable())
        {
            aUseHardwareAccell.Check(pCanvasSettings->IsHardwareAccelerationEnabled());
        }
        else
        {
            aUseHardwareAccell.Check(false);
		    aUseHardwareAccell.Disable();
        }

        aUseHardwareAccell.SaveValue();
    }

    { // #i95644# AntiAliasing
	    if(mpDrawinglayerOpt->IsAAPossibleOnThisSystem())
        {
	        aUseAntiAliase.Check(mpDrawinglayerOpt->IsAntiAliasing());
        }
        else
	    {
            aUseAntiAliase.Check(false);
		    aUseAntiAliase.Disable();
	    }

        aUseAntiAliase.SaveValue();
    }

	{
        // #i97672# Selection
        // check if transparent selection is possible on this system
        const bool bTransparentSelectionPossible(
            !GetSettings().GetStyleSettings().GetHighContrastMode()
            && supportsOperation(OutDevSupport_TransparentRect));

        // enter values
        if(bTransparentSelectionPossible)
        {
            maSelectionCB.Check(mpDrawinglayerOpt->IsTransparentSelection());
        }
        else
        {
            maSelectionCB.Enable(false);
        }

        maSelectionMF.SetValue(mpDrawinglayerOpt->GetTransparentSelectionPercent());
		maSelectionMF.Enable(mpDrawinglayerOpt->IsTransparentSelection() && bTransparentSelectionPossible);
    }

#if defined( UNX )
	aFontAntiAliasing.SaveValue();
	aAAPointLimit.SaveValue();
#endif
	aFontShowCB.SaveValue();
	aFontHistoryCB.SaveValue();

#if defined( UNX )
	LINK( this, OfaViewTabPage, OnAntialiasingToggled ).Call( NULL );
#endif
}
/* -----------------------------23.11.00 14:55--------------------------------

 ---------------------------------------------------------------------------*/
class LangConfigItem_Impl : public ConfigItem
{
	Any 		aValue;
	OUString	aPropertyName;
public:
	LangConfigItem_Impl(const OUString& rTree, const OUString& rProperty);
	~LangConfigItem_Impl();

	virtual void			Commit();

	const Any& 	GetValue() const {return aValue;}
	void 		SetValue(Any& rValue)  {aValue = rValue; SetModified();}
};
/* -----------------------------23.11.00 15:06--------------------------------

 ---------------------------------------------------------------------------*/
LangConfigItem_Impl::LangConfigItem_Impl(
	const OUString& rTree, const OUString& rProperty) :
	ConfigItem(rTree),
	aPropertyName(rProperty)
{
	Sequence<OUString> aNames(1);
	aNames.getArray()[0] = aPropertyName;
	Sequence<Any> aValues = GetProperties(aNames);
	aValue = aValues.getConstArray()[0];
}
/* -----------------------------23.11.00 15:06--------------------------------

 ---------------------------------------------------------------------------*/
LangConfigItem_Impl::~LangConfigItem_Impl()
{}
/* -----------------------------23.11.00 15:10--------------------------------

 ---------------------------------------------------------------------------*/
void LangConfigItem_Impl::Commit()
{
	Sequence<OUString> aNames(1);
	aNames.getArray()[0] = aPropertyName;
	Sequence<Any> aValues(1);
	aValues.getArray()[0] = aValue;
	PutProperties(aNames, aValues);
}
/* -----------------22.07.2003 10:33-----------------

 --------------------------------------------------*/
struct LanguageConfig_Impl
{
    SvtLanguageOptions aLanguageOptions;
    SvtSysLocaleOptions aSysLocaleOptions;
    SvtLinguConfig aLinguConfig;
};
/* -----------------------------23.11.00 13:06--------------------------------

 ---------------------------------------------------------------------------*/
static sal_Bool bLanguageCurrentDoc_Impl = sal_False;

// some things we'll need...
static const OUString sConfigSrvc = OUString::createFromAscii("com.sun.star.configuration.ConfigurationProvider");
static const OUString sAccessSrvc = OUString::createFromAscii("com.sun.star.configuration.ConfigurationAccess");
static const OUString sAccessUpdSrvc = OUString::createFromAscii("com.sun.star.configuration.ConfigurationUpdateAccess");
static const OUString sInstalledLocalesPath = OUString::createFromAscii("org.openoffice.Setup/Office/InstalledLocales");
static OUString sUserLocalePath = OUString::createFromAscii("org.openoffice.Office.Linguistic/General");
//static const OUString sUserLocalePath = OUString::createFromAscii("org.openoffice.Office/Linguistic");
static const OUString sUserLocaleKey = OUString::createFromAscii("UILocale");
static const OUString sSystemLocalePath = OUString::createFromAscii("org.openoffice.System/L10N");
static const OUString sSystemLocaleKey = OUString::createFromAscii("UILocale");
static const OUString sOfficeLocalePath = OUString::createFromAscii("org.openoffice.Office/L10N");
static const OUString sOfficeLocaleKey = OUString::createFromAscii("ooLocale");
static Sequence< OUString > seqInstalledLanguages;

OfaLanguagesTabPage::OfaLanguagesTabPage( Window* pParent, const SfxItemSet& rSet ) :
	SfxTabPage( pParent, SVX_RES( OFA_TP_LANGUAGES ), rSet ),
	aUILanguageGB(this, 		SVX_RES(FL_UI_LANG		)),
    aLocaleSettingFI(this,      SVX_RES(FI_LOCALESETTING)),
    aUserInterfaceFT(this,      SVX_RES(FT_USERINTERFACE)),
    aUserInterfaceLB(this,      SVX_RES(LB_USERINTERFACE)),
    aLocaleSettingFT(this,      SVX_RES(FT_LOCALESETTING)),
    aLocaleSettingLB(this,      SVX_RES(LB_LOCALESETTING)),
    aCurrencyFI( this,          SVX_RES(FI_CURRENCY       )),
    aDecimalSeparatorFT(this,   SVX_RES(FT_DECIMALSEPARATOR)),
    aDecimalSeparatorCB(this,   SVX_RES(CB_DECIMALSEPARATOR)),
    aCurrencyFT( this,          SVX_RES(FT_CURRENCY       )),
    aCurrencyLB( this,          SVX_RES(LB_CURRENCY       )),
	aLinguLanguageGB(this, 		SVX_RES(FL_LINGU_LANG		)),
    aWesternLanguageFI(this,    SVX_RES(FI_WEST_LANG      )),
    aWesternLanguageFT(this,    SVX_RES(FT_WEST_LANG      )),
	aWesternLanguageLB(this, 	SVX_RES(LB_WEST_LANG		)),
    aAsianLanguageFI(this,      SVX_RES(FI_ASIAN_LANG     )),
    aAsianLanguageFT(this,      SVX_RES(FT_ASIAN_LANG     )),
	aAsianLanguageLB(this, 		SVX_RES(LB_ASIAN_LANG		)),
    aComplexLanguageFI(this,    SVX_RES(FI_COMPLEX_LANG   )),
    aComplexLanguageFT(this,    SVX_RES(FT_COMPLEX_LANG   )),
	aComplexLanguageLB(this, 	SVX_RES(LB_COMPLEX_LANG	)),
	aCurrentDocCB(this, 		SVX_RES(CB_CURRENT_DOC	)),
    aEnhancedFL(this,           SVX_RES(FL_ENHANCED    )),
    aAsianSupportFI(this,       SVX_RES(FI_ASIANSUPPORT   )),
    aAsianSupportCB(this,       SVX_RES(CB_ASIANSUPPORT   )),
    aCTLSupportFI(this,         SVX_RES(FI_CTLSUPPORT    )),
    aCTLSupportCB(this,         SVX_RES(CB_CTLSUPPORT   )),
    sDecimalSeparatorLabel(aDecimalSeparatorCB.GetText()),
    pLangConfig(new LanguageConfig_Impl)
{
    FreeResource();

    // initialize user interface language selection
    SvtLanguageTable* pLanguageTable = new SvtLanguageTable;
    const String aStr( pLanguageTable->GetString( LANGUAGE_SYSTEM ) );

    String aUILang(aStr);
    aUILang += String::CreateFromAscii(" - ");
    aUILang += pLanguageTable->GetString( Application::GetSettings().GetUILanguage() );

    aUserInterfaceLB.InsertEntry(aUILang);
    aUserInterfaceLB.SetEntryData(0, 0);
    aUserInterfaceLB.SelectEntryPos(0);
    try
    {
        OUString sOfficeLocaleValue;
        OUString sSystemLocaleValue;

        Reference< XMultiServiceFactory > theMSF = comphelper::getProcessServiceFactory();
        Reference< XMultiServiceFactory > theConfigProvider = Reference< XMultiServiceFactory > (
            theMSF->createInstance( sConfigSrvc ),UNO_QUERY_THROW);
        Sequence< Any > theArgs(2);
        Reference< XNameAccess > theNameAccess;

        // find out which locales are currently installed and add them to the listbox
        theArgs[0] = makeAny(NamedValue(OUString::createFromAscii("NodePath"), makeAny(sInstalledLocalesPath)));
        theArgs[1] = makeAny(NamedValue(OUString::createFromAscii("reload"), makeAny(sal_True)));
	theNameAccess = Reference< XNameAccess > (
            theConfigProvider->createInstanceWithArguments(sAccessSrvc, theArgs ), UNO_QUERY_THROW );
        seqInstalledLanguages = theNameAccess->getElementNames();
        LanguageType aLang = LANGUAGE_DONTKNOW;
        for (sal_Int32 i=0; i<seqInstalledLanguages.getLength(); i++)
        {
            aLang = MsLangId::convertIsoStringToLanguage(seqInstalledLanguages[i]);
            if (aLang != LANGUAGE_DONTKNOW)
            {
                //USHORT p = aUserInterfaceLB.InsertLanguage(aLang);
                String aLangStr( pLanguageTable->GetString( aLang ) );
                USHORT p = aUserInterfaceLB.InsertEntry(aLangStr);
                aUserInterfaceLB.SetEntryData(p, (void*)(i+1));
            }
        }

        // find out whether the user has a specific locale specified
        Sequence< Any > theArgs2(1);
        theArgs2[0] = makeAny(NamedValue(OUString::createFromAscii("NodePath"), makeAny(sUserLocalePath)));
        theNameAccess = Reference< XNameAccess > (
            theConfigProvider->createInstanceWithArguments(sAccessSrvc, theArgs2 ), UNO_QUERY_THROW );
        if (theNameAccess->hasByName(sUserLocaleKey))
            theNameAccess->getByName(sUserLocaleKey) >>= m_sUserLocaleValue;
        // select the user specified locale in the listbox
        if (m_sUserLocaleValue.getLength() > 0)
        {
            sal_Int32 d = 0;
            for (USHORT i=0; i < aUserInterfaceLB.GetEntryCount(); i++)
            {
                d = (sal_Int32)(sal_IntPtr)aUserInterfaceLB.GetEntryData(i);
                if ( d > 0 && seqInstalledLanguages.getLength() > d-1 && seqInstalledLanguages[d-1].equals(m_sUserLocaleValue))
                    aUserInterfaceLB.SelectEntryPos(i);
            }
        }

    }
    catch (Exception &e)
    {
        // we'll just leave the box in it's default setting and won't
        // even give it event handler...
        OString aMsg = OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US);
        OSL_ENSURE(sal_False, aMsg.getStr());
    }

    aWesternLanguageLB.SetLanguageList( LANG_LIST_WESTERN | LANG_LIST_ONLY_KNOWN, TRUE,  FALSE, TRUE );
    aWesternLanguageLB.InsertDefaultLanguage( ::com::sun::star::i18n::ScriptType::LATIN );
    aAsianLanguageLB.SetLanguageList( LANG_LIST_CJK     | LANG_LIST_ONLY_KNOWN, TRUE,  FALSE, TRUE );
    aAsianLanguageLB.InsertDefaultLanguage( ::com::sun::star::i18n::ScriptType::ASIAN );
    aComplexLanguageLB.SetLanguageList( LANG_LIST_CTL     | LANG_LIST_ONLY_KNOWN, TRUE,  FALSE, TRUE );
    aComplexLanguageLB.InsertDefaultLanguage( ::com::sun::star::i18n::ScriptType::COMPLEX );

    aLocaleSettingLB.SetLanguageList( LANG_LIST_ALL     | LANG_LIST_ONLY_KNOWN, FALSE, FALSE, FALSE);
    aLocaleSettingLB.InsertDefaultLanguage( ::com::sun::star::i18n::ScriptType::WEAK );

    const NfCurrencyTable& rCurrTab = SvNumberFormatter::GetTheCurrencyTable();
    const NfCurrencyEntry& rCurr = SvNumberFormatter::GetCurrencyEntry( LANGUAGE_SYSTEM );
    // insert SYSTEM entry
    String aDefaultCurr(aStr);
    aDefaultCurr += String::CreateFromAscii(" - ");
    aDefaultCurr += rCurr.GetBankSymbol();
    aCurrencyLB.InsertEntry( aDefaultCurr );
    // all currencies
    String aTwoSpace( RTL_CONSTASCII_USTRINGPARAM( "  " ) );
    USHORT nCurrCount = rCurrTab.Count();
    // first entry is SYSTEM, skip it
    for ( USHORT j=1; j < nCurrCount; ++j )
    {
        const NfCurrencyEntry* pCurr = rCurrTab[j];
        String aStr_( pCurr->GetBankSymbol() );
        aStr_ += aTwoSpace;
        aStr_ += pCurr->GetSymbol();
        aStr_ = ApplyLreOrRleEmbedding( aStr_ );
        aStr_ += aTwoSpace;
        aStr_ += ApplyLreOrRleEmbedding( pLanguageTable->GetString( pCurr->GetLanguage() ) );
        USHORT nPos = aCurrencyLB.InsertEntry( aStr_ );
        aCurrencyLB.SetEntryData( nPos, (void*) pCurr );
    }
    delete pLanguageTable;

    aLocaleSettingLB.SetSelectHdl( LINK( this, OfaLanguagesTabPage, LocaleSettingHdl ) );
	Link aLink( LINK( this, OfaLanguagesTabPage, SupportHdl ) );
	aAsianSupportCB.SetClickHdl( aLink );
    aCTLSupportCB.SetClickHdl( aLink );

    aAsianSupportCB.Check( m_bOldAsian = pLangConfig->aLanguageOptions.IsAnyEnabled() );
    aAsianSupportCB.SaveValue();
    sal_Bool bReadonly = pLangConfig->aLanguageOptions.IsReadOnly(SvtLanguageOptions::E_ALLCJK);
    aAsianSupportCB.Enable(!bReadonly);
    aAsianSupportFI.Show(bReadonly);
    SupportHdl( &aAsianSupportCB );

    aCTLSupportCB.Check( m_bOldCtl = pLangConfig->aLanguageOptions.IsCTLFontEnabled() );
    aCTLSupportCB.SaveValue();
    bReadonly = pLangConfig->aLanguageOptions.IsReadOnly(SvtLanguageOptions::E_CTLFONT);
    aCTLSupportCB.Enable(!bReadonly);
    aCTLSupportFI.Show(bReadonly);
    SupportHdl( &aCTLSupportCB );
}
/*-- 23.11.00 13:06:40---------------------------------------------------

  -----------------------------------------------------------------------*/
OfaLanguagesTabPage::~OfaLanguagesTabPage()
{
    delete pLangConfig;
}
/*-- 23.11.00 13:06:40---------------------------------------------------

  -----------------------------------------------------------------------*/
SfxTabPage*	OfaLanguagesTabPage::Create( Window* pParent, const SfxItemSet& rAttrSet )
{
	return new OfaLanguagesTabPage(pParent, rAttrSet);
}
/*-- 23.11.00 13:06:41---------------------------------------------------

  -----------------------------------------------------------------------*/
LanguageType lcl_LangStringToLangType(const OUString& rLang)
{
	Locale aLocale;
    sal_Int32 nSep = rLang.indexOf('-');
    if (nSep < 0)
        aLocale.Language = rLang;
    else
    {
        aLocale.Language = rLang.copy(0, nSep);
        if (nSep < rLang.getLength())
            aLocale.Country = rLang.copy(nSep+1, rLang.getLength() - (nSep+1));
    }
	LanguageType eLangType = SvxLocaleToLanguage( aLocale );
	return eLangType;
}

/*-- 23.11.00 13:06:40---------------------------------------------------

  -----------------------------------------------------------------------*/
void lcl_UpdateAndDelete(SfxVoidItem* pInvalidItems[], SfxBoolItem* pBoolItems[], sal_uInt16 nCount)
{
    SfxViewFrame* pCurrentFrm = SfxViewFrame::Current();
    SfxViewFrame* pViewFrm = SfxViewFrame::GetFirst();
    while(pViewFrm)
    {
        SfxBindings& rBind = pViewFrm->GetBindings();
        for(sal_Int16 i = 0; i < nCount; i++)
        {
            if(pCurrentFrm == pViewFrm)
                rBind.InvalidateAll(sal_False);
            rBind.SetState( *pInvalidItems[i] );
            rBind.SetState( *pBoolItems[i] );
        }
        pViewFrm = SfxViewFrame::GetNext(*pViewFrm);
    }
    for(sal_Int16 i = 0; i < nCount; i++)
    {
        delete pInvalidItems[i];
        delete pBoolItems[i] ;
    }
}

BOOL OfaLanguagesTabPage::FillItemSet( SfxItemSet& rSet )
{
    if(aCTLSupportCB.IsChecked() &&
            (aCTLSupportCB.GetSavedValue() != aCTLSupportCB.IsChecked()) ||
            (aComplexLanguageLB.GetSavedValue() != aComplexLanguageLB.GetSelectEntryPos()))
    {
        //sequence checking has to be switched on depending on the selected CTL language
        LanguageType eCTLLang = aComplexLanguageLB.GetSelectLanguage();
        sal_Bool bOn = MsLangId::needsSequenceChecking( eCTLLang);
        pLangConfig->aLanguageOptions.SetCTLSequenceCheckingRestricted(bOn);
        pLangConfig->aLanguageOptions.SetCTLSequenceChecking(bOn);
        pLangConfig->aLanguageOptions.SetCTLSequenceCheckingTypeAndReplace(bOn);
    }
    try
    {
        // handle settings for UI Language
        // a change of setting needs to bring up a warning message
        OUString aLangString;
        sal_Int32 d = (sal_Int32)(sal_IntPtr)aUserInterfaceLB.GetEntryData(aUserInterfaceLB.GetSelectEntryPos());
        if( d > 0 && seqInstalledLanguages.getLength() > d-1)
            aLangString = seqInstalledLanguages[d-1];

        /*
        if( aUserInterfaceLB.GetSelectEntryPos() > 0)
            aLangString = ConvertLanguageToIsoString(aUserInterfaceLB.GetSelectLanguage());
        */
        Reference< XMultiServiceFactory > theMSF = comphelper::getProcessServiceFactory();
        Reference< XMultiServiceFactory > theConfigProvider = Reference< XMultiServiceFactory > (
            theMSF->createInstance( sConfigSrvc ),UNO_QUERY_THROW);
        Sequence< Any > theArgs(1);
        theArgs[0] = makeAny(sUserLocalePath);
        Reference< XPropertySet >xProp(
            theConfigProvider->createInstanceWithArguments(sAccessUpdSrvc, theArgs ), UNO_QUERY_THROW );
        if ( !m_sUserLocaleValue.equals(aLangString))
        {
            // OSL_ENSURE(sal_False, "UserInterface language was changed, restart.");
            // write new value
            xProp->setPropertyValue(sUserLocaleKey, makeAny(aLangString));
            Reference< XChangesBatch >(xProp, UNO_QUERY_THROW)->commitChanges();
            // display info
            InfoBox aBox(this, SVX_RES(RID_SVX_MSGBOX_LANGUAGE_RESTART));
            aBox.Execute();

            // tell quickstarter to stop being a veto listener

            Reference< XInitialization > xInit(theMSF->createInstance(
                OUString::createFromAscii("com.sun.star.office.Quickstart")), UNO_QUERY);
            if (xInit.is())
            {
                Sequence< Any > args(3);
                args[0] = makeAny(sal_False); // will be ignored
                args[1] = makeAny(sal_False); // will be ignored
                args[2] = makeAny(sal_False); // disable veto
                xInit->initialize(args);
            }
        }
    }
    catch (Exception& e)
    {
        // we'll just leave the box in it's default setting and won't
        // even give it event handler...
        OString aMsg = OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US);
        OSL_ENSURE(sal_False, aMsg.getStr());
    }

    pLangConfig->aSysLocaleOptions.BlockBroadcasts( TRUE );

    OUString sLang = pLangConfig->aSysLocaleOptions.GetLocaleConfigString();
    LanguageType eOldLocale = (sLang.getLength() ?
        lcl_LangStringToLangType( sLang ) : LANGUAGE_SYSTEM);
    LanguageType eNewLocale = aLocaleSettingLB.GetSelectLanguage();
    if ( eOldLocale != eNewLocale )
    {
        // an empty string denotes SYSTEM locale
        OUString sNewLang;
        if ( eNewLocale != LANGUAGE_SYSTEM )
        {
            Locale aLocale;
            SvxLanguageToLocale( aLocale, eNewLocale );
            sNewLang = aLocale.Language;
            if ( aLocale.Country.getLength() > 0 )
            {
                sNewLang += C2U("-");
                sNewLang += aLocale.Country;
            }
        }
        // Set application settings before options, so listeners at the
        // options will access the new locale.
        AllSettings aSettings( Application::GetSettings() );
        aSettings.SetLanguage( eNewLocale );
        Application::SetSettings( aSettings );
        pLangConfig->aSysLocaleOptions.SetLocaleConfigString( sNewLang );
        rSet.Put( SfxBoolItem( SID_OPT_LOCALE_CHANGED, TRUE ) );
    }

    //
    if(aDecimalSeparatorCB.GetSavedValue() != aDecimalSeparatorCB.IsChecked())
        pLangConfig->aSysLocaleOptions.SetDecimalSeparatorAsLocale(aDecimalSeparatorCB.IsChecked());

    // Configured currency, for example, USD-en-US or EUR-de-DE, or empty for
    // locale default. This must be set _after_ the locale above in order to
    // have a valid locale for broadcasting the currency change.
    OUString sOldCurr = pLangConfig->aSysLocaleOptions.GetCurrencyConfigString();
    USHORT nCurrPos = aCurrencyLB.GetSelectEntryPos();
    const NfCurrencyEntry* pCurr = (const NfCurrencyEntry*)
        aCurrencyLB.GetEntryData( nCurrPos );
    OUString sNewCurr;
    if ( pCurr )
        sNewCurr = SvtSysLocaleOptions::CreateCurrencyConfigString(
            pCurr->GetBankSymbol(), pCurr->GetLanguage() );
    if ( sOldCurr != sNewCurr )
        pLangConfig->aSysLocaleOptions.SetCurrencyConfigString( sNewCurr );

    BOOL bRet = FALSE;
	SfxObjectShell* pCurrentDocShell = SfxObjectShell::Current();
	Reference< XPropertySet > xLinguProp( LinguMgr::GetLinguPropertySet(), UNO_QUERY );
	BOOL bCurrentDocCBChecked = aCurrentDocCB.IsChecked();
    if(aCurrentDocCB.IsEnabled())
        bLanguageCurrentDoc_Impl = bCurrentDocCBChecked;
	BOOL bCurrentDocCBChanged = bCurrentDocCBChecked != aCurrentDocCB.GetSavedValue();

	BOOL bValChanged = aWesternLanguageLB.GetSavedValue() != aWesternLanguageLB.GetSelectEntryPos();
	if( (bCurrentDocCBChanged && !bCurrentDocCBChecked) || bValChanged)
	{
		LanguageType eSelectLang = aWesternLanguageLB.GetSelectLanguage();
		if(!bCurrentDocCBChecked)
		{
			Any aValue;
			Locale aLocale = MsLangId::convertLanguageToLocale( eSelectLang, false );
			aValue <<= aLocale;
			OUString aPropName( C2U("DefaultLocale") );
            pLangConfig->aLinguConfig.SetProperty( aPropName, aValue );
            if (xLinguProp.is())
				xLinguProp->setPropertyValue( aPropName, aValue );
		}
		if(pCurrentDocShell)
		{
			rSet.Put(SvxLanguageItem(MsLangId::resolveSystemLanguageByScriptType(eSelectLang, ::com::sun::star::i18n::ScriptType::LATIN),
                SID_ATTR_LANGUAGE));
			bRet = TRUE;
		}
	}
	bValChanged = aAsianLanguageLB.GetSavedValue() != aAsianLanguageLB.GetSelectEntryPos();
	if( (bCurrentDocCBChanged && !bCurrentDocCBChecked) || bValChanged)
	{
		LanguageType eSelectLang = aAsianLanguageLB.GetSelectLanguage();
		if(!bCurrentDocCBChecked)
		{
			Any aValue;
			Locale aLocale = MsLangId::convertLanguageToLocale( eSelectLang, false );
			aValue <<= aLocale;
			OUString aPropName( C2U("DefaultLocale_CJK") );
            pLangConfig->aLinguConfig.SetProperty( aPropName, aValue );
            if (xLinguProp.is())
				xLinguProp->setPropertyValue( aPropName, aValue );
		}
		if(pCurrentDocShell)
		{
			rSet.Put(SvxLanguageItem(MsLangId::resolveSystemLanguageByScriptType(eSelectLang, ::com::sun::star::i18n::ScriptType::ASIAN),
				SID_ATTR_CHAR_CJK_LANGUAGE));
			bRet = TRUE;
		}
	}
	bValChanged = aComplexLanguageLB.GetSavedValue() != aComplexLanguageLB.GetSelectEntryPos();
	if( (bCurrentDocCBChanged && !bCurrentDocCBChecked) || bValChanged)
	{
		LanguageType eSelectLang = aComplexLanguageLB.GetSelectLanguage();
		if(!bCurrentDocCBChecked)
		{
			Any aValue;
			Locale aLocale = MsLangId::convertLanguageToLocale( eSelectLang, false );
			aValue <<= aLocale;
			OUString aPropName( C2U("DefaultLocale_CTL") );
            pLangConfig->aLinguConfig.SetProperty( aPropName, aValue );
            if (xLinguProp.is())
				xLinguProp->setPropertyValue( aPropName, aValue );
		}
		if(pCurrentDocShell)
		{
			rSet.Put(SvxLanguageItem(MsLangId::resolveSystemLanguageByScriptType(eSelectLang, ::com::sun::star::i18n::ScriptType::COMPLEX),
				SID_ATTR_CHAR_CTL_LANGUAGE));
			bRet = TRUE;
		}
	}

    if(aAsianSupportCB.GetSavedValue() != aAsianSupportCB.IsChecked() )
    {
        sal_Bool bChecked = aAsianSupportCB.IsChecked();
        pLangConfig->aLanguageOptions.SetAll(bChecked);
        //iterate over all bindings to invalidate vertical text direction

      const sal_uInt16 STATE_COUNT = 2;

      SfxBoolItem* pBoolItems[STATE_COUNT];
      pBoolItems[0] = new SfxBoolItem(SID_VERTICALTEXT_STATE, FALSE);
      pBoolItems[1] = new SfxBoolItem(SID_TEXT_FITTOSIZE_VERTICAL, FALSE);

      SfxVoidItem* pInvalidItems[STATE_COUNT];
      pInvalidItems[0] = new SfxVoidItem(SID_VERTICALTEXT_STATE);
      pInvalidItems[1] = new SfxVoidItem(SID_TEXT_FITTOSIZE_VERTICAL);

    lcl_UpdateAndDelete(pInvalidItems, pBoolItems, STATE_COUNT);
    }

	if ( aCTLSupportCB.GetSavedValue() != aCTLSupportCB.IsChecked() )
    {
        pLangConfig->aLanguageOptions.SetCTLFontEnabled( aCTLSupportCB.IsChecked() );

        const sal_uInt16 STATE_COUNT = 1;
        SfxBoolItem* pBoolItems[STATE_COUNT];
        pBoolItems[0] = new SfxBoolItem(SID_CTLFONT_STATE, FALSE);
        SfxVoidItem* pInvalidItems[STATE_COUNT];
        pInvalidItems[0] = new SfxVoidItem(SID_CTLFONT_STATE);
        lcl_UpdateAndDelete(pInvalidItems, pBoolItems, STATE_COUNT);
    }



    if ( pLangConfig->aSysLocaleOptions.IsModified() )
        pLangConfig->aSysLocaleOptions.Commit();
    pLangConfig->aSysLocaleOptions.BlockBroadcasts( FALSE );

	return FALSE;
}
//-----------------------------------------------------------------------------
void OfaLanguagesTabPage::Reset( const SfxItemSet& rSet )
{
    OUString sLang = pLangConfig->aSysLocaleOptions.GetLocaleConfigString();
    if ( sLang.getLength() )
        aLocaleSettingLB.SelectLanguage(lcl_LangStringToLangType(sLang));
    else
        aLocaleSettingLB.SelectLanguage( LANGUAGE_SYSTEM );
    sal_Bool bReadonly = pLangConfig->aSysLocaleOptions.IsReadOnly(SvtSysLocaleOptions::E_LOCALE);
    aLocaleSettingLB.Enable(!bReadonly);
    aLocaleSettingFT.Enable(!bReadonly);
    aLocaleSettingFI.Show(bReadonly);

    //
    aDecimalSeparatorCB.Check( pLangConfig->aSysLocaleOptions.IsDecimalSeparatorAsLocale());
    aDecimalSeparatorCB.SaveValue();

    // let LocaleSettingHdl enable/disable checkboxes for CJK/CTL support
    // #i15812# must be done *before* the configured currency is set
    // and update the decimal separator used for the given locale
    LocaleSettingHdl(&aLocaleSettingLB);

    // configured currency, for example, USD-en-US or EUR-de-DE, or empty for locale default
    String aAbbrev;
    LanguageType eLang;
    const NfCurrencyEntry* pCurr = NULL;
    sLang = pLangConfig->aSysLocaleOptions.GetCurrencyConfigString();
    if ( sLang.getLength() )
    {
        SvtSysLocaleOptions::GetCurrencyAbbrevAndLanguage( aAbbrev, eLang, sLang );
        pCurr = SvNumberFormatter::GetCurrencyEntry( aAbbrev, eLang );
    }
    // if pCurr==NULL the SYSTEM entry is selected
    USHORT nPos = aCurrencyLB.GetEntryPos( (void*) pCurr );
    aCurrencyLB.SelectEntryPos( nPos );
    bReadonly = pLangConfig->aSysLocaleOptions.IsReadOnly(SvtSysLocaleOptions::E_CURRENCY);
    aCurrencyLB.Enable(!bReadonly);
    aCurrencyFT.Enable(!bReadonly);
    aCurrencyFI.Show(bReadonly);

	//western/CJK/CLK language
	LanguageType eCurLang = LANGUAGE_NONE;
	LanguageType eCurLangCJK = LANGUAGE_NONE;
	LanguageType eCurLangCTL = LANGUAGE_NONE;
	SfxObjectShell* pCurrentDocShell = SfxObjectShell::Current();
    //collect the configuration values first
    aCurrentDocCB.Enable(FALSE);
    //
    Any aWestLang;
    Any aCJKLang;
    Any aCTLLang;
    try
    {
        aWestLang = pLangConfig->aLinguConfig.GetProperty(C2U("DefaultLocale"));
        Locale aLocale;
        aWestLang >>= aLocale;

        eCurLang = MsLangId::convertLocaleToLanguage( aLocale );

        aCJKLang = pLangConfig->aLinguConfig.GetProperty(C2U("DefaultLocale_CJK"));
        aLocale = Locale();
        aCJKLang >>= aLocale;
        eCurLangCJK = MsLangId::convertLocaleToLanguage( aLocale );

        aCTLLang = pLangConfig->aLinguConfig.GetProperty(C2U("DefaultLocale_CTL"));
        aLocale = Locale();
        aCTLLang >>= aLocale;
        eCurLangCTL = MsLangId::convertLocaleToLanguage( aLocale );
    }
    catch(Exception&)
    {
    }
    //overwrite them by the values provided by the DocShell
    if(pCurrentDocShell)
	{
        aCurrentDocCB.Enable(TRUE);
        aCurrentDocCB.Check(bLanguageCurrentDoc_Impl);
		const SfxPoolItem* pLang;
		if( SFX_ITEM_SET == rSet.GetItemState(SID_ATTR_LANGUAGE, FALSE, &pLang))
		{
			LanguageType eTempCurLang = ((const SvxLanguageItem*)pLang)->GetValue();
			if (MsLangId::resolveSystemLanguageByScriptType(eCurLang, ::com::sun::star::i18n::ScriptType::LATIN) != eTempCurLang)
				eCurLang = eTempCurLang;
		}

		if( SFX_ITEM_SET == rSet.GetItemState(SID_ATTR_CHAR_CJK_LANGUAGE, FALSE, &pLang))
		{
			LanguageType eTempCurLang = ((const SvxLanguageItem*)pLang)->GetValue();
			if (MsLangId::resolveSystemLanguageByScriptType(eCurLangCJK, ::com::sun::star::i18n::ScriptType::ASIAN) != eTempCurLang)
				eCurLangCJK = eTempCurLang;
		}

		if( SFX_ITEM_SET == rSet.GetItemState(SID_ATTR_CHAR_CTL_LANGUAGE, FALSE, &pLang))
		{
			LanguageType eTempCurLang = ((const SvxLanguageItem*)pLang)->GetValue();
			if (MsLangId::resolveSystemLanguageByScriptType(eCurLangCTL, ::com::sun::star::i18n::ScriptType::COMPLEX) != eTempCurLang)
				eCurLangCTL = eTempCurLang;
		}
	}
    if(LANGUAGE_NONE == eCurLang || LANGUAGE_DONTKNOW == eCurLang)
        aWesternLanguageLB.SelectLanguage(LANGUAGE_NONE);
	else
		aWesternLanguageLB.SelectLanguage(eCurLang);

    if(LANGUAGE_NONE == eCurLangCJK || LANGUAGE_DONTKNOW == eCurLangCJK)
        aAsianLanguageLB.SelectLanguage(LANGUAGE_NONE);
	else
		aAsianLanguageLB.SelectLanguage(eCurLangCJK);

    if(LANGUAGE_NONE == eCurLangCTL || LANGUAGE_DONTKNOW == eCurLangCTL)
        aComplexLanguageLB.SelectLanguage(LANGUAGE_NONE);
	else
		aComplexLanguageLB.SelectLanguage(eCurLangCTL);

	aWesternLanguageLB.SaveValue();
	aAsianLanguageLB.SaveValue();
	aComplexLanguageLB.SaveValue();
	aCurrentDocCB.SaveValue();

	sal_Bool bEnable = !pLangConfig->aLinguConfig.IsReadOnly( C2U("DefaultLocale") );
    aWesternLanguageFT.Enable( bEnable );
    aWesternLanguageLB.Enable( bEnable );


    aWesternLanguageFI.Show(!bEnable);

    // #i15812# controls for CJK/CTL already enabled/disabled from LocaleSettingHdl
#if 0
    bEnable = ( !pLangConfig->aLinguConfig.IsReadOnly( C2U("DefaultLocale_CJK") ) && aAsianSupportCB.IsChecked() );
    aAsianLanguageFT.Enable( bEnable );
	aAsianLanguageLB.Enable( bEnable );

    bEnable = ( !pLangConfig->aLinguConfig.IsReadOnly( C2U("DefaultLocale_CTL") ) && aCTLSupportCB.IsChecked() );
    aComplexLanguageFT.Enable( bEnable );
	aComplexLanguageLB.Enable( bEnable );
#endif
	/*---------------------07-05-07--------------------------
	check the box "For the current document only"
	set the focus to the Western Language box
	--------------------------------------------------------*/
    const SfxPoolItem* pLang = 0;
    if ( SFX_ITEM_SET == rSet.GetItemState(SID_SET_DOCUMENT_LANGUAGE, FALSE, &pLang ) &&( (const SfxBoolItem*)pLang)->GetValue() == TRUE )
	{
		aWesternLanguageLB.GrabFocus();
		aCurrentDocCB.Enable(TRUE);
		aCurrentDocCB.Check(TRUE);
	}
}
/* -----------------------------20.04.01 15:09--------------------------------

 ---------------------------------------------------------------------------*/
IMPL_LINK(  OfaLanguagesTabPage, SupportHdl, CheckBox*, pBox )
{
	DBG_ASSERT( pBox, "OfaLanguagesTabPage::SupportHdl(): pBox invalid" );

    sal_Bool bCheck = pBox->IsChecked();
	if ( &aAsianSupportCB == pBox )
	{
        sal_Bool bReadonly = pLangConfig->aLinguConfig.IsReadOnly( C2U("DefaultLocale_CJK"));
        bCheck = ( bCheck && !bReadonly );
        aAsianLanguageFT.Enable( bCheck );
    	aAsianLanguageLB.Enable( bCheck );
        aAsianLanguageFI.Show(bReadonly);
        if( pBox->IsEnabled() )
            m_bOldAsian = bCheck;
	}
	else if ( &aCTLSupportCB == pBox )
	{
        sal_Bool bReadonly = pLangConfig->aLinguConfig.IsReadOnly( C2U("DefaultLocale_CTL"));
        bCheck = ( bCheck && !bReadonly  );
        aComplexLanguageFT.Enable( bCheck );
    	aComplexLanguageLB.Enable( bCheck );
        aComplexLanguageFI.Show(bReadonly);
        if( pBox->IsEnabled() )
            m_bOldCtl = bCheck;
	}
	else
	{
		DBG_ERRORFILE( "OfaLanguagesTabPage::SupportHdl(): wrong pBox" );
	}

    return 0;
}

namespace
{
	void lcl_checkLanguageCheckBox(CheckBox& _rCB,sal_Bool _bNewValue,sal_Bool _bOldValue)
	{
		if ( _bNewValue )
			_rCB.Check(TRUE);
		else
			_rCB.Check( _bOldValue );
// #i15082# do not call SaveValue() in running dialog...
//		_rCB.SaveValue();
		_rCB.Enable( !_bNewValue );
	}
}
/* -----------------08.06.01 17:56-------------------

 --------------------------------------------------*/
IMPL_LINK( OfaLanguagesTabPage, LocaleSettingHdl, SvxLanguageBox*, pBox )
{
    LanguageType eLang = pBox->GetSelectLanguage();
	sal_uInt16 nType = SvtLanguageOptions::GetScriptTypeOfLanguage(eLang);
	// first check if CTL must be enabled
    // #103299# - if CTL font setting is not readonly
    if(!pLangConfig->aLanguageOptions.IsReadOnly(SvtLanguageOptions::E_CTLFONT))
    {
        bool bIsCTLFixed = (nType & SCRIPTTYPE_COMPLEX) != 0;
        lcl_checkLanguageCheckBox(aCTLSupportCB, bIsCTLFixed, m_bOldCtl);
        SupportHdl( &aCTLSupportCB );
    }
    // second check if CJK must be enabled
    // #103299# - if CJK support is not readonly
    if(!pLangConfig->aLanguageOptions.IsReadOnly(SvtLanguageOptions::E_ALLCJK))
    {
        bool bIsCJKFixed = (nType & SCRIPTTYPE_ASIAN) != 0;
        lcl_checkLanguageCheckBox(aAsianSupportCB, bIsCJKFixed, m_bOldAsian);
        SupportHdl( &aAsianSupportCB );
    }

    USHORT nPos;
    if ( eLang == LANGUAGE_SYSTEM )
        nPos = aCurrencyLB.GetEntryPos( (void*) NULL );
    else
    {
        const NfCurrencyEntry* pCurr = &SvNumberFormatter::GetCurrencyEntry( eLang );
        nPos = aCurrencyLB.GetEntryPos( (void*) pCurr );
    }
    aCurrencyLB.SelectEntryPos( nPos );

    //update the decimal separator key of the related CheckBox
    Locale aTempLocale;
    SvxLanguageToLocale( aTempLocale, eLang );
    LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), aTempLocale );
    String sTempLabel(sDecimalSeparatorLabel);
    sTempLabel.SearchAndReplaceAscii("%1", aLocaleWrapper.getNumDecimalSep() );
    aDecimalSeparatorCB.SetText(sTempLabel);

    return 0;
}

