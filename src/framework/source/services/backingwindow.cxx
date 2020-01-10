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

// autogen include statement, do not remove
#include "precompiled_framework.hxx"

#include "backingwindow.hxx"
#include "framework.hrc"
#include "classes/fwkresid.hxx"
#include <services.h>

#include "vcl/metric.hxx"
#include "vcl/mnemonic.hxx"
#include "vcl/menu.hxx"
#include "vcl/svapp.hxx"

#include "tools/urlobj.hxx"

#include "svtools/dynamicmenuoptions.hxx"
#include "svtools/imagemgr.hxx"
#include "svtools/svtools.hrc"

#include "comphelper/processfactory.hxx"
#include "comphelper/sequenceashashmap.hxx"

#include "rtl/strbuf.hxx"
#include "rtl/ustrbuf.hxx"

#include "com/sun/star/lang/XMultiServiceFactory.hpp"
#include "com/sun/star/container/XNameAccess.hpp"
#include "com/sun/star/system/XSystemShellExecute.hpp"
#include "com/sun/star/system/SystemShellExecuteFlags.hpp"
#include "com/sun/star/task/XJobExecutor.hpp"


using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;
using namespace framework;

#define WRITER_URL      "private:factory/swriter"
#define CALC_URL        "private:factory/scalc"
#define IMPRESS_WIZARD_URL     "private:factory/simpress?slot=6686"
#define DRAW_URL        "private:factory/sdraw"
#define BASE_URL        "private:factory/sdatabase?Interactive"
#define MATH_URL        "private:factory/smath"
#define TEMPLATE_URL    "slot:5500"
#define OPEN_URL        ".uno:Open"

DecoToolBox::DecoToolBox( Window* pParent, WinBits nStyle ) :
    ToolBox( pParent, nStyle )
{
        SetBackground();
        SetPaintTransparent( TRUE );
}

void DecoToolBox::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );

    if ( rDCEvt.GetFlags() & SETTINGS_STYLE )
    {
        calcMinSize();
        SetBackground();
        SetPaintTransparent( TRUE );
    }
}

void DecoToolBox::calcMinSize()
{
    ToolBox aTbx( GetParent() );
    USHORT nItems = GetItemCount();
    for( USHORT i = 0; i < nItems; i++ )
    {
        USHORT nId = GetItemId( i );
        aTbx.InsertItem( nId, GetItemImage( nId ) );
    }
    aTbx.SetOutStyle( TOOLBOX_STYLE_FLAT );
    maMinSize = aTbx.CalcWindowSizePixel(); 
}

Size DecoToolBox::getMinSize()
{
    return maMinSize;
}

#define STC_BUTTON_STYLE  (WB_LEFT | WB_VCENTER | WB_FLATBUTTON | WB_BEVELBUTTON)

BackingWindow::BackingWindow( Window* i_pParent ) :
    Window( i_pParent, FwkResId( DLG_BACKING ) ),
    maWelcome( this, WB_LEFT ),
    maProduct( this, WB_LEFT ),
    maWriterButton( this, STC_BUTTON_STYLE ),
    maCalcButton( this, STC_BUTTON_STYLE ),
    maImpressButton( this, STC_BUTTON_STYLE ),
    maOpenButton( this, STC_BUTTON_STYLE ),
    maDrawButton( this, STC_BUTTON_STYLE ),
    maDBButton( this, STC_BUTTON_STYLE ),
    maMathButton( this, STC_BUTTON_STYLE ),
    maTemplateButton( this, STC_BUTTON_STYLE ),
    maToolbox( this, WB_DIALOGCONTROL ),
    maWelcomeString( FwkResId( STR_BACKING_WELCOME ) ),
    maProductString( FwkResId( STR_BACKING_WELCOMEPRODUCT ) ),
    maOpenString( FwkResId( STR_BACKING_FILE ) ),
    maTemplateString( FwkResId( STR_BACKING_TEMPLATE ) ),
    maButtonImageSize( 10, 10 ),
    mbInitControls( false ),
    mnLayoutStyle( 0 ),
    mpAccExec( NULL ),
    mnBtnPos( 120 ),
    mnBtnTop( 150 )
{
    mnColumnWidth[0] = mnColumnWidth[1] = 0;
    mnTextColumnWidth[0] = mnTextColumnWidth[1] = 0;

    try
    {
        Reference<lang::XMultiServiceFactory> xConfig( comphelper::getProcessServiceFactory()->createInstance(SERVICENAME_CFGPROVIDER),UNO_QUERY);
        if( xConfig.is() )
        {
            Sequence<Any> args(1);
            PropertyValue val(
                rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("nodepath") ),
                0,
                Any(rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("/org.openoffice.Office.Common/Help/StartCenter"))),
                PropertyState_DIRECT_VALUE);
            args.getArray()[0] <<= val;
            Reference<container::XNameAccess> xNameAccess(xConfig->createInstanceWithArguments(SERVICENAME_CFGREADACCESS,args), UNO_QUERY);
            if( xNameAccess.is() )
            {
                //throws css::container::NoSuchElementException, css::lang::WrappedTargetException
                Any value( xNameAccess->getByName(rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("StartCenterLayoutStyle"))) );
                mnLayoutStyle = value.get<sal_Int32>();
            }
        }
    }
    catch (Exception& )
    {
    }
    
    String aExtHelpText( FwkResId( STR_BACKING_EXTHELP ) );
    String aRegHelpText( FwkResId( STR_BACKING_REGHELP ) );
    String aInfoHelpText( FwkResId( STR_BACKING_INFOHELP ) );
    String aTplRepHelpText( FwkResId( STR_BACKING_TPLREP ) );
    
    // clean up resource stack
    FreeResource();
    
    maWelcome.SetPaintTransparent( TRUE );
    maProduct.SetPaintTransparent( TRUE );
    EnableChildTransparentMode();
    
    SetStyle( GetStyle() | WB_DIALOGCONTROL );
    
    // force tab cycling in toolbox
    maToolbox.SetStyle( maToolbox.GetStyle() | WB_FORCETABCYCLE );

    // insert toolbox items
    maToolbox.InsertItem( nItemId_TplRep, Image() );
    maToolbox.SetItemText( nItemId_TplRep, aTplRepHelpText );
    maToolbox.SetQuickHelpText( nItemId_TplRep, aTplRepHelpText );
    maToolbox.SetItemCommand( nItemId_TplRep, String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:TemplateRepository" ) ) );
    maToolbox.ShowItem( nItemId_TplRep );

    maToolbox.InsertItem( nItemId_Extensions, Image() );
    maToolbox.SetQuickHelpText( nItemId_Extensions, aExtHelpText );
    maToolbox.SetItemText( nItemId_Extensions, aExtHelpText );
    maToolbox.SetItemCommand( nItemId_Extensions, String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:Extensions" ) ) );
    maToolbox.ShowItem( nItemId_Extensions );
    
    maToolbox.InsertItem( nItemId_Reg, Image() );
    maToolbox.SetQuickHelpText( nItemId_Reg, aRegHelpText );
    maToolbox.SetItemText( nItemId_Reg, aRegHelpText );
    maToolbox.SetItemCommand( nItemId_Reg, String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:Register" ) ) );
    maToolbox.ShowItem( nItemId_Reg );
    
    maToolbox.InsertItem( nItemId_Info, Image() );
    maToolbox.SetItemText( nItemId_Info, aInfoHelpText );
    maToolbox.SetQuickHelpText( nItemId_Info, aInfoHelpText );
    maToolbox.SetItemCommand( nItemId_Info, String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:Info" ) ) );
    maToolbox.ShowItem( nItemId_Info );

    // get dispatch provider
    mxDesktop = Reference<XDesktop>( comphelper::getProcessServiceFactory()->createInstance(SERVICENAME_DESKTOP ),UNO_QUERY );
    if( mxDesktop.is() )
        mxDesktopDispatchProvider = Reference< XDispatchProvider >( mxDesktop, UNO_QUERY );
    
    maWriterButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:WriterButton" ) ) ) );
    maCalcButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:CalcButton" ) ) ) );
    maImpressButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:ImpressButton" ) ) ) );
    maDrawButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:DrawButton" ) ) ) );
    maDBButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:DBButton" ) ) ) );
    maMathButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:MathButton" ) ) ) );
    maTemplateButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:TemplateButton" ) ) ) );
    maOpenButton.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:OpenButton" ) ) ) );
    maToolbox.SetSmartHelpId( SmartId( String( RTL_CONSTASCII_USTRINGPARAM( ".HelpId:StartCenter:Toolbox" ) ) ) );
    
    // init background
    initBackground();

    // add some breathing space for the images
    maButtonImageSize.Width() += 12;
    maButtonImageSize.Height() += 12;
    
}


BackingWindow::~BackingWindow()
{
    delete mpAccExec;
}

class ImageContainerRes : public Resource
{
    public:
    ImageContainerRes( const ResId& i_rId ) : Resource( i_rId ) {}
    ~ImageContainerRes() { FreeResource(); }
};

void BackingWindow::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );

    if ( rDCEvt.GetFlags() & SETTINGS_STYLE )
    {
        initBackground();
        Invalidate();
    }
}

void BackingWindow::initBackground()
{
    SetBackground( GetSettings().GetStyleSettings().GetWorkspaceGradient() );
    
    bool bDark = GetSettings().GetStyleSettings().GetHighContrastMode();
    if( bDark )
        maWelcomeTextColor = maLabelTextColor = Color( COL_WHITE );
    else if( mnLayoutStyle == 1 )
        maWelcomeTextColor = maLabelTextColor = Color( COL_BLACK );
    else 
        maWelcomeTextColor = maLabelTextColor = Color( 0x26, 0x35, 0x42 );
    
    Color aTextBGColor( bDark ? COL_BLACK : COL_WHITE );

    // select image set
    ImageContainerRes aRes( FwkResId( bDark ? RES_BACKING_IMAGES_HC : RES_BACKING_IMAGES ) );

    // scale middle segment
    Size aMiddleSize;
    if( !! maBackgroundMiddle )
        aMiddleSize = maBackgroundMiddle.GetSizePixel();
    // load middle segment
    maBackgroundMiddle = BitmapEx( FwkResId( BMP_BACKING_BACKGROUND_MIDDLE ) );
    // and scale it to previous size
    if( aMiddleSize.Width() && aMiddleSize.Height() )
        maBackgroundMiddle.Scale( aMiddleSize );
    
    if( GetSettings().GetLayoutRTL() )
    {
        // replace images by RTL versions
        maBackgroundLeft = BitmapEx( FwkResId( BMP_BACKING_BACKGROUND_RTL_RIGHT ) );
        maBackgroundRight = BitmapEx( FwkResId( BMP_BACKING_BACKGROUND_RTL_LEFT) );
    }
    else
    {
        maBackgroundLeft = BitmapEx( FwkResId( BMP_BACKING_BACKGROUND_LEFT ) );
        maBackgroundRight = BitmapEx( FwkResId( BMP_BACKING_BACKGROUND_RIGHT ) );
    }
    maToolbox.SetItemImage( nItemId_Extensions, BitmapEx( FwkResId( BMP_BACKING_EXT ) ) );
    maToolbox.SetItemImage( nItemId_Reg, BitmapEx( FwkResId( BMP_BACKING_REG ) ) );
    maToolbox.SetItemImage( nItemId_Info, BitmapEx( FwkResId( BMP_BACKING_INFO ) ) );
    maToolbox.SetItemImage( nItemId_TplRep, BitmapEx( FwkResId( BMP_BACKING_TPLREP ) ) );

    maWelcome.SetControlForeground( maWelcomeTextColor );
    maWelcome.SetBackground();
    maProduct.SetControlForeground( maWelcomeTextColor );
    maProduct.SetBackground();

    if( mnLayoutStyle == 1 )
    {
        if( Application::GetSettings().GetLayoutRTL() )
            mnBtnPos = maBackgroundRight.GetSizePixel().Width() + 40;
        else
            mnBtnPos = maBackgroundLeft.GetSizePixel().Width() + 40;
    }

    // get icon images from fwk resource and set them on the appropriate buttons
    loadImage( FwkResId( BMP_BACKING_WRITER ), maWriterButton );
    loadImage( FwkResId( BMP_BACKING_CALC ), maCalcButton );
    loadImage( FwkResId( BMP_BACKING_IMPRESS ), maImpressButton );
    loadImage( FwkResId( BMP_BACKING_DRAW ), maDrawButton );
    loadImage( FwkResId( BMP_BACKING_DATABASE ), maDBButton );
    loadImage( FwkResId( BMP_BACKING_FORMULA ), maMathButton );
    loadImage( FwkResId( BMP_BACKING_OPENFILE ), maOpenButton );
    loadImage( FwkResId( BMP_BACKING_OPENTEMPLATE ), maTemplateButton );
    
}

void BackingWindow::initControls()
{
    if( mbInitControls )
        return;
    
    mbInitControls = true;
    
    // calculate dialog size
    // begin with background bitmap
    maControlRect = Rectangle( Point(), maBackgroundLeft.GetSizePixel() );
    maControlRect.Left() += nShadowLeft;
    maControlRect.Right() -= nShadowRight;
    maControlRect.Top() += nShadowTop;
    maControlRect.Bottom() -= nShadowBottom;
    
    long nYPos = 0;
    // set bigger welcome string
    maWelcome.SetText( maWelcomeString );
    maTextFont = GetSettings().GetStyleSettings().GetLabelFont();
    maTextFont.SetSize( Size( 0, 18 ) );
    maTextFont.SetWeight( WEIGHT_BOLD );
    maWelcome.SetFont( maTextFont );
    // get metric to get correct width factor and adjust
    long nW = (maWelcome.GetFontMetric().GetWidth()*95)/100;
    maTextFont.SetSize( Size( nW, 18 ) );

    maWelcome.SetFont( maTextFont );
    maWelcome.SetControlFont( maTextFont );
    maWelcomeSize = Size( maWelcome.GetTextWidth( maWelcomeString ), maWelcome.GetTextHeight() );
    maWelcomeSize.Width() = (maWelcomeSize.Width() * 20)/19;
    
    nYPos += (maWelcomeSize.Height()*3)/2;
    
    if( maControlRect.GetWidth() < mnBtnPos + maWelcomeSize.Width() + 20 )
        maControlRect.Right() = maControlRect.Left() + maWelcomeSize.Width() + mnBtnPos + 20;

    nYPos += maWelcomeSize.Height();
    
    // set product string
    maTextFont.SetSize( Size( 0, 30 ) );
    maProduct.SetFont( maTextFont );

    // get metric to get correct width factor and adjust
    nW = (maProduct.GetFontMetric().GetWidth()*95)/100;
    maTextFont.SetSize( Size( nW, 28 ) );

    maProduct.SetFont( maTextFont );
    maProduct.SetControlFont( maTextFont );
    maProduct.SetText( maProductString );
    maProductSize = Size( maProduct.GetTextWidth( maProductString ), maProduct.GetTextHeight() );
    maProductSize.Width() = (maProductSize.Width() * 20)/19;
    
    if( maControlRect.GetWidth() < maProductSize.Width() + mnBtnPos + 10 )
        maControlRect.Right() = maControlRect.Left() + maProductSize.Width() + mnBtnPos + 10;

    if( mnLayoutStyle == 1 )
    {
        maWelcome.Show();        
        maProduct.Show();
    }
    
    nYPos += (maProductSize.Height()*3)/2;
    
    // set a slighly larger font than normal labels on the texts
    maTextFont.SetSize( Size( 0, 11 ) );
    maTextFont.SetWeight( WEIGHT_NORMAL );
    
    // collect the URLs of the entries in the File/New menu
    SvtModuleOptions	aModuleOptions;
    std::set< rtl::OUString > aFileNewAppsAvailable;
    SvtDynamicMenuOptions aOpt;
    Sequence < Sequence < PropertyValue > > aNewMenu = aOpt.GetMenu( E_NEWMENU );
    const rtl::OUString sURLKey( RTL_CONSTASCII_USTRINGPARAM( "URL" ) );

    const Sequence< PropertyValue >* pNewMenu = aNewMenu.getConstArray();
    const Sequence< PropertyValue >* pNewMenuEnd = aNewMenu.getConstArray() + aNewMenu.getLength();
    for ( ; pNewMenu != pNewMenuEnd; ++pNewMenu )
    {
        comphelper::SequenceAsHashMap aEntryItems( *pNewMenu );
        rtl::OUString sURL( aEntryItems.getUnpackedValueOrDefault( sURLKey, rtl::OUString() ) );
        if ( sURL.getLength() )
            aFileNewAppsAvailable.insert( sURL );
    }
    
    // create mnemonics on the fly, preregister the mnemonics of the menu
    MnemonicGenerator aMnemns;
    maTemplateString = MnemonicGenerator::EraseAllMnemonicChars( maTemplateString );
    maOpenString = MnemonicGenerator::EraseAllMnemonicChars( maOpenString );
    
    SystemWindow* pSysWin = GetSystemWindow();
    if( pSysWin )
    {
        MenuBar* pMBar = pSysWin->GetMenuBar();
        if( pMBar )
        {
            for( USHORT i = 0; i < pMBar->GetItemCount(); i++ )
            {
                USHORT nItemId = pMBar->GetItemId( i );
                String aItemText( pMBar->GetItemText( nItemId ) );
                if( aItemText.Len() )
                    aMnemns.RegisterMnemonic( aItemText );
            }
        }
    }
    
    // layout the buttons
    layoutButton( WRITER_URL, 0, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SWRITER,
                  maWriterButton, aMnemns );
    layoutButton( DRAW_URL, 1, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SDRAW,
                  maDrawButton, aMnemns );
    nYPos += maButtonImageSize.Height() + 10;
    layoutButton( CALC_URL, 0, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SCALC,
                  maCalcButton, aMnemns );
    layoutButton( BASE_URL, 1, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SDATABASE,
                  maDBButton, aMnemns );
    nYPos += maButtonImageSize.Height() + 10;
    layoutButton( IMPRESS_WIZARD_URL, 0, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SIMPRESS,
                  maImpressButton, aMnemns );
    layoutButton( MATH_URL, 1, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SMATH,
                  maMathButton, aMnemns );
    
    nYPos += 3*maButtonImageSize.Height() / 2;
    
    layoutButton( NULL, 0, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SWRITER,
                  maOpenButton, aMnemns, maOpenString );
    layoutButton( NULL, 1, aFileNewAppsAvailable,
                  aModuleOptions, SvtModuleOptions::E_SWRITER,
                  maTemplateButton, aMnemns, maTemplateString );    
    nYPos += 10;
    
    DBG_ASSERT( nYPos < maControlRect.GetHeight(), "misformatting !" );
    if( mnColumnWidth[0] + mnColumnWidth[1] + mnBtnPos + 20 > maControlRect.GetWidth() )
        maControlRect.Right() = maControlRect.Left() + mnColumnWidth[0] + mnColumnWidth[1] + mnBtnPos + 20;
    
    mnTextColumnWidth[0] = mnColumnWidth[0];
    mnTextColumnWidth[1] = mnColumnWidth[1];
    
    if( mnTextColumnWidth[1] > mnTextColumnWidth[0] )
    {
        mnColumnWidth[0]     = mnColumnWidth[1];
        mnTextColumnWidth[0] = mnTextColumnWidth[1];
    }
    else
    {
        mnColumnWidth[1]     = mnColumnWidth[0];
        mnTextColumnWidth[1] = mnTextColumnWidth[0];
    }
    if( maControlRect.GetWidth() < maControlRect.GetHeight() * 3 / 2 )
    {
        maControlRect.Right() = maControlRect.Left() + maControlRect.GetHeight() * 3 / 2;
        long nDelta = (maControlRect.GetWidth() - mnBtnPos - mnColumnWidth[1] - mnColumnWidth[0] - 20);
        mnColumnWidth[0] += nDelta/2;
        mnColumnWidth[1] += nDelta/2;
    }
    
    maToolbox.SetSelectHdl( LINK( this, BackingWindow, ToolboxHdl ) );
    if( mnLayoutStyle == 0 )
        maToolbox.Show();
    
    // scale middle map to formatted width
    Size aMiddleSegmentSize( maControlRect.GetSize().Width() + nShadowLeft + nShadowRight,
                             maBackgroundMiddle.GetSizePixel().Height() );
    
    long nLW = maBackgroundLeft.GetSizePixel().Width();
    long nRW = maBackgroundRight.GetSizePixel().Width();
    if( aMiddleSegmentSize.Width() > nLW + nRW )
    {
        aMiddleSegmentSize.Width() -= nLW;
        aMiddleSegmentSize.Width() -= nRW;
        maBackgroundMiddle.Scale( aMiddleSegmentSize );
    }
    else
        maBackgroundMiddle = BitmapEx();
    
    Resize();
    
    maWriterButton.GrabFocus();
}

void BackingWindow::loadImage( const ResId& i_rId, ImageButton& i_rButton )
{
    BitmapEx aBmp( i_rId );
    Size aImgSize( aBmp.GetSizePixel() );
    if( aImgSize.Width() > maButtonImageSize.Width() )
        maButtonImageSize.Width() = aImgSize.Width();
    if( aImgSize.Height() > maButtonImageSize.Height() )
        maButtonImageSize.Height() = aImgSize.Height();
    i_rButton.SetModeImage( aBmp );
}

void BackingWindow::layoutButton(
                          const char* i_pURL, int nColumn,
                          const std::set<rtl::OUString>& i_rURLS,
                          SvtModuleOptions& i_rOpt, SvtModuleOptions::EModule i_eMod,
                          ImageButton& i_rBtn,
                          MnemonicGenerator& i_rMnemns,
                          const String& i_rStr
                          )
{
    rtl::OUString aURL( rtl::OUString::createFromAscii( i_pURL ? i_pURL : "" ) );
    // setup button
    i_rBtn.SetPaintTransparent( TRUE );
    i_rBtn.SetClickHdl( LINK( this, BackingWindow, ClickHdl ) );
    if( i_pURL && (! i_rOpt.IsModuleInstalled( i_eMod ) || i_rURLS.find( aURL ) == i_rURLS.end()) )
    {
        i_rBtn.Enable( FALSE );
    }
    
    i_rBtn.SetFont( maTextFont );
    i_rBtn.SetControlFont( maTextFont );
    String aText( i_rStr.Len() ? i_rStr : SvFileInformationManager::GetDescription( INetURLObject( aURL ) ) );
    i_rMnemns.CreateMnemonic( aText );
    i_rBtn.SetText( aText );
    
    long nTextWidth = i_rBtn.GetTextWidth( i_rBtn.GetText() );

    nTextWidth += maButtonImageSize.Width() + 8; // add some fuzz to be on the safe side
    if( nColumn >= 0 && nColumn < static_cast<int>(sizeof(mnColumnWidth)/sizeof(mnColumnWidth[0])) )
    {
        if( nTextWidth > mnColumnWidth[nColumn] )
            mnColumnWidth[nColumn] = nTextWidth;
    }
    
    i_rBtn.SetImageAlign( IMAGEALIGN_LEFT );
    // show the controls
    i_rBtn.Show();
}

void BackingWindow::Paint( const Rectangle& )
{
    // draw bitmap
    if( GetSettings().GetLayoutRTL() )
    {
        Point aTL( maControlRect.TopLeft() );
        aTL.X() -= nShadowRight;
        aTL.Y() -= nShadowTop;
        DrawBitmapEx( aTL, maBackgroundLeft );
        aTL.X() += maBackgroundLeft.GetSizePixel().Width();
        if( !!maBackgroundMiddle )
        {
            DrawBitmapEx( aTL, maBackgroundMiddle );
            aTL.X() += maBackgroundMiddle.GetSizePixel().Width();
        }
        DrawBitmapEx( aTL, maBackgroundRight );
    }
    else
    {
        Point aTL( maControlRect.TopLeft() );
        aTL.X() -= nShadowLeft;
        aTL.Y() -= nShadowTop;
        DrawBitmapEx( aTL, maBackgroundLeft );
        aTL.X() += maBackgroundLeft.GetSizePixel().Width();
        if( !!maBackgroundMiddle )
        {
            DrawBitmapEx( aTL, maBackgroundMiddle );
            aTL.X() += maBackgroundMiddle.GetSizePixel().Width();
        }
        DrawBitmapEx( aTL, maBackgroundRight );
    }
}

long BackingWindow::Notify( NotifyEvent& rNEvt )
{
    if( rNEvt.GetType() == EVENT_KEYINPUT )
    {
        if( ! mpAccExec )
        {
            mpAccExec = svt::AcceleratorExecute::createAcceleratorHelper();
            mpAccExec->init( comphelper::getProcessServiceFactory(), mxFrame);
        }
        
        const KeyEvent* pEvt = rNEvt.GetKeyEvent();
        const KeyCode& rKeyCode(pEvt->GetKeyCode());
        if( pEvt && mpAccExec->execute(rKeyCode) )
            return 1;
        // #i110344# extrawurst: specialized arrow key control
        if( rKeyCode.GetModifier() == 0 )
        {
            if( rKeyCode.GetCode() == KEY_RIGHT )
            {
                if( maWriterButton.HasFocus() )
                    maDrawButton.GrabFocus();
                else if( maCalcButton.HasFocus() )
                    maDBButton.GrabFocus();
                else if( maImpressButton.HasFocus() )
                    maMathButton.GrabFocus();
                else if( maOpenButton.HasFocus() )
                    maTemplateButton.GrabFocus();
                return 1;
            }
            else if( rKeyCode.GetCode() == KEY_LEFT )
            {
                if( maDrawButton.HasFocus() )
                    maWriterButton.GrabFocus();
                else if( maDBButton.HasFocus() )
                    maCalcButton.GrabFocus();
                else if( maMathButton.HasFocus() )
                    maImpressButton.GrabFocus();
                else if( maTemplateButton.HasFocus() )
                    maOpenButton.GrabFocus();
                return 1;
            }
            else if( rKeyCode.GetCode() == KEY_UP )
            {
                // first column
                if( maOpenButton.HasFocus() )
                    maImpressButton.GrabFocus();
                else if( maImpressButton.HasFocus() )
                    maCalcButton.GrabFocus();
                else if( maCalcButton.HasFocus() )
                    maWriterButton.GrabFocus();
                // second column
                else if( maTemplateButton.HasFocus() )
                    maMathButton.GrabFocus();
                else if( maMathButton.HasFocus() )
                    maDBButton.GrabFocus();
                else if( maDBButton.HasFocus() )
                    maDrawButton.GrabFocus();
                return 1;
            }
            else if( rKeyCode.GetCode() == KEY_DOWN )
            {
                // first column
                if( maWriterButton.HasFocus() )
                    maCalcButton.GrabFocus();
                else if( maCalcButton.HasFocus() )
                    maImpressButton.GrabFocus();
                else if( maImpressButton.HasFocus() )
                    maOpenButton.GrabFocus();
                // second column
                else if( maDrawButton.HasFocus() )
                    maDBButton.GrabFocus();
                else if( maDBButton.HasFocus() )
                    maMathButton.GrabFocus();
                else if( maMathButton.HasFocus() )
                    maTemplateButton.GrabFocus();                
                return 1;
            }
        }
    }
    return Window::Notify( rNEvt );
}

void BackingWindow::setOwningFrame( const com::sun::star::uno::Reference< com::sun::star::frame::XFrame >& xFrame )
{
    mxFrame = xFrame;
    if( ! mbInitControls )
        initControls();
}

void BackingWindow::Resize()
{
    Size aWindowSize( GetSizePixel() );
    Size aControlSize = maControlRect.GetSize();
    maControlRect = Rectangle( Point( (aWindowSize.Width() - aControlSize.Width()) / 2,
                                      (aWindowSize.Height() - aControlSize.Height()) / 2 ),
                               aControlSize );
    
    maToolbox.calcMinSize();
    Size aTBSize( maToolbox.getMinSize() );
    Point aTBPos( maControlRect.Left() + mnBtnPos,
                  maControlRect.Bottom() - aTBSize.Height() - 10 );
    if( Application::GetSettings().GetLayoutRTL() )
        aTBPos.X() = maControlRect.Right() - aTBSize.Width() - mnBtnPos;
    maToolbox.SetPosSizePixel( aTBPos, aTBSize );

    // #i93631# squeeze controls so they fit into the box
    // this can be necessary due to application font height which has small deviations
    // from the size set
    const long nWDelta    = maWelcomeSize.Height();
    const long nW2Delta   = (maWelcomeSize.Height()*3)/2;
    const long nPDelta    = (maProductSize.Height()*3)/2;
    const long nBDelta    = maButtonImageSize.Height() + 10;
    const long nB2Delta   = 3*maButtonImageSize.Height()/2;
    const long nLastDelta = maButtonImageSize.Height();
    long nDiff = 0;
    while( ( maControlRect.Top()   +
                 (nWDelta - nDiff) +
                 (nW2Delta- nDiff) +
                 (nPDelta - nDiff) +
             3 * (nBDelta - nDiff) +
                 (nB2Delta- nDiff) +
                 nLastDelta
            ) > aTBPos.Y() )
    {
        nDiff++;
    }
    
    long nYPos = maControlRect.Top();
    nYPos += nW2Delta - nDiff;
    maWelcome.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos, nYPos ),
                                Size( maControlRect.GetWidth() - mnBtnPos - 5, (maWelcomeSize.Height()*20)/19 ) );
    nYPos += nWDelta - nDiff;    
    maProduct.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos, nYPos ), Size( maControlRect.GetWidth() - mnBtnPos - 5, (maProductSize.Height()*20)/19 ) );
    nYPos += nPDelta - nDiff;
    
    nYPos += nWDelta/2 - nDiff;
    
    if( mnLayoutStyle != 1 )
        nYPos = maControlRect.Top() + mnBtnTop;

    maWriterButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos, nYPos ), Size( mnTextColumnWidth[0], maButtonImageSize.Height() ) );
    maDrawButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos + mnColumnWidth[0], nYPos ), Size( mnTextColumnWidth[1], maButtonImageSize.Height() ) );
    nYPos += nBDelta - nDiff;
    maCalcButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos, nYPos ), Size( mnTextColumnWidth[0], maButtonImageSize.Height() ) );
    maDBButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos + mnColumnWidth[0], nYPos ), Size( mnTextColumnWidth[1], maButtonImageSize.Height() ) );
    nYPos += nBDelta - nDiff;
    maImpressButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos, nYPos ), Size( mnTextColumnWidth[0], maButtonImageSize.Height() ) );
    maMathButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos + mnColumnWidth[0], nYPos ), Size( mnTextColumnWidth[1], maButtonImageSize.Height() ) );

    nYPos += nB2Delta - nDiff;
    maOpenButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos, nYPos ), Size( mnTextColumnWidth[0], maButtonImageSize.Height() ) );
    maTemplateButton.SetPosSizePixel( Point( maControlRect.Left() + mnBtnPos + mnColumnWidth[0], nYPos ), Size( mnTextColumnWidth[1], maButtonImageSize.Height() ) );
}

IMPL_LINK( BackingWindow, ToolboxHdl, void*, EMPTYARG )
{
    const char* pNodePath = NULL;
    const char* pNode = NULL;
    
    switch( maToolbox.GetCurItemId() )
    {
    case nItemId_Extensions:
        pNodePath = "/org.openoffice.Office.Common/Help/StartCenter";
        pNode = "AddFeatureURL";
        break;
    case nItemId_Reg:
        try
        {
            // create the Desktop component which can load components
            Reference < lang::XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();
            if( xFactory.is() )
            {
                Reference< task::XJobExecutor > xProductRegistration(
                    xFactory->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.setup.ProductRegistration" ) ) ),
                    UNO_QUERY_THROW );

                 // tell it that the user wants to register
                 xProductRegistration->trigger( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "RegistrationRequired" ) ) );
            }
        }
        catch( const Exception& )
        {
        }
        break;
    case nItemId_Info:
        pNodePath = "/org.openoffice.Office.Common/Help/StartCenter";
        pNode = "InfoURL";
        break;
    case nItemId_TplRep:
        pNodePath = "/org.openoffice.Office.Common/Help/StartCenter";
        pNode = "TemplateRepositoryURL";
        break;
    default:
        break;
    }
    if( pNodePath && pNode )
    {
        try
        {
            Reference<lang::XMultiServiceFactory> xConfig( comphelper::getProcessServiceFactory()->createInstance(SERVICENAME_CFGPROVIDER),UNO_QUERY);
            if( xConfig.is() )
            {
                Sequence<Any> args(1);
                PropertyValue val(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM("nodepath") ),
                    0,
                    Any(rtl::OUString::createFromAscii(pNodePath)),
                    PropertyState_DIRECT_VALUE);
                args.getArray()[0] <<= val;
                Reference<container::XNameAccess> xNameAccess(xConfig->createInstanceWithArguments(SERVICENAME_CFGREADACCESS,args), UNO_QUERY);
                if( xNameAccess.is() )
                {
                    rtl::OUString sURL;
                    //throws css::container::NoSuchElementException, css::lang::WrappedTargetException
                    Any value( xNameAccess->getByName(rtl::OUString::createFromAscii(pNode)) );
                    sURL = value.get<rtl::OUString> ();
                    
                    Reference< com::sun::star::system::XSystemShellExecute > xSystemShellExecute(
                        comphelper::getProcessServiceFactory()->createInstance(
                            rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.system.SystemShellExecute" ) ) ),
                        UNO_QUERY_THROW);
                    //throws css::lang::IllegalArgumentException, css::system::SystemShellExecuteException
                    xSystemShellExecute->execute( sURL, rtl::OUString(), com::sun::star::system::SystemShellExecuteFlags::DEFAULTS);
                }
            }
        }
        catch (Exception& )
        {
        }
    }
                    
    return 0;
}

IMPL_LINK( BackingWindow, ClickHdl, Button*, pButton )
{
    // dispatch the appropriate URL and end the dialog
    if( pButton == &maWriterButton )
        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(WRITER_URL) ) );
    else if( pButton == &maCalcButton )
        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(CALC_URL) ) );
    else if( pButton == &maImpressButton )
        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(IMPRESS_WIZARD_URL) ) );
    else if( pButton == &maDrawButton )
        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(DRAW_URL) ) );
    else if( pButton == &maDBButton )
        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(BASE_URL) ) );
    else if( pButton == &maMathButton )
        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(MATH_URL) ) );
    else if( pButton == &maOpenButton )
    {
        Reference< XDispatchProvider > xFrame( mxFrame, UNO_QUERY );
        
        Sequence< com::sun::star::beans::PropertyValue > aArgs(1);
        PropertyValue* pArg = aArgs.getArray();
        pArg[0].Name = rtl::OUString::createFromAscii("Referer");
        pArg[0].Value <<= rtl::OUString::createFromAscii("private:user");

        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(OPEN_URL) ), rtl::OUString(), xFrame, aArgs );
    }
    else if( pButton == &maTemplateButton )
    {
        Reference< XDispatchProvider > xFrame( mxFrame, UNO_QUERY );
        
        Sequence< com::sun::star::beans::PropertyValue > aArgs(1);
        PropertyValue* pArg = aArgs.getArray();
        pArg[0].Name = rtl::OUString::createFromAscii("Referer");
        pArg[0].Value <<= rtl::OUString::createFromAscii("private:user");

        dispatchURL( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(TEMPLATE_URL) ), rtl::OUString(), xFrame, aArgs );
    }
    return 0;
}

struct ImplDelayedDispatch
{
    Reference< XDispatch >      xDispatch;
    com::sun::star::util::URL   aDispatchURL;
    Sequence< PropertyValue >   aArgs;
    
    ImplDelayedDispatch( const Reference< XDispatch >& i_xDispatch,
                         const com::sun::star::util::URL& i_rURL,
                         const Sequence< PropertyValue >& i_rArgs )
    : xDispatch( i_xDispatch ),
      aDispatchURL( i_rURL ),
      aArgs( i_rArgs )
    {
    }
    ~ImplDelayedDispatch() {}
};

static long implDispatchDelayed( void*, void* pArg )
{
    struct ImplDelayedDispatch* pDispatch = reinterpret_cast<ImplDelayedDispatch*>(pArg);
    try
    {
        pDispatch->xDispatch->dispatch( pDispatch->aDispatchURL, pDispatch->aArgs );
    }
    catch( Exception )
    {
    }
    
    // clean up
    delete pDispatch;
    
    return 0;
}

void BackingWindow::dispatchURL( const rtl::OUString& i_rURL,
                                 const rtl::OUString& rTarget,
                                 const Reference< XDispatchProvider >& i_xProv,
                                 const Sequence< PropertyValue >& i_rArgs )
{
    // if no special dispatch provider is given, get the desktop
    Reference< XDispatchProvider > xProvider( i_xProv.is() ? i_xProv : mxDesktopDispatchProvider );

    // check for dispatch provider
    if( !xProvider.is())
        return;
    
    // get an URL transformer to clean up the URL
    com::sun::star::util::URL aDispatchURL;
    aDispatchURL.Complete = i_rURL;

    Reference < com::sun::star::util::XURLTransformer > xURLTransformer(
        comphelper::getProcessServiceFactory()->createInstance( rtl::OUString::createFromAscii("com.sun.star.util.URLTransformer") ),
        com::sun::star::uno::UNO_QUERY );
    if ( xURLTransformer.is() )
    {
        try
        {
            // clean up the URL
            xURLTransformer->parseStrict( aDispatchURL );
            // get a Dispatch for the URL and target
            Reference< XDispatch > xDispatch(
                xProvider->queryDispatch( aDispatchURL, rTarget, 0 )
                );
            // dispatch the URL
            if ( xDispatch.is() )
            {
                ImplDelayedDispatch* pDisp = new ImplDelayedDispatch( xDispatch, aDispatchURL, i_rArgs );
                ULONG nEventId = 0;
                if( ! Application::PostUserEvent( nEventId, Link( NULL, implDispatchDelayed ), pDisp ) )
                    delete pDisp; // event could not be posted for unknown reason, at least don't leak
            }
        }
        catch ( com::sun::star::uno::RuntimeException& )
        {
            throw;
        }
        catch ( com::sun::star::uno::Exception& )
        {
        }
    }
}

