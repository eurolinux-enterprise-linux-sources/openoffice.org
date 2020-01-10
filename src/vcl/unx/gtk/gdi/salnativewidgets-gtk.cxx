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

#include "plugins/gtk/gtkframe.hxx"
#include "plugins/gtk/gtkdata.hxx"
#include "plugins/gtk/gtkinst.hxx"
#include "plugins/gtk/gtkgdi.hxx"

#include "pspgraphics.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <hash_map>

#include "saldata.hxx"
#include "saldisp.hxx"
#include "vcl/svapp.hxx"

// initialize statics
BOOL GtkSalGraphics::bThemeChanged = TRUE;
BOOL GtkSalGraphics::bNeedPixmapPaint = FALSE;
BOOL GtkSalGraphics::bGlobalNeedPixmapPaint = FALSE;
BOOL GtkSalGraphics::bToolbarGripWorkaround = FALSE;
BOOL GtkSalGraphics::bNeedButtonStyleAsEditBackgroundWorkaround = FALSE;

GtkSalGraphics::~GtkSalGraphics()
{
}


using namespace rtl;

/*************************************
 * Cached native widget objects
 *************************************/
class NWPixmapCacheList;
class NWPixmapCache;
struct NWFWidgetData
{
    GtkWidget *	gCacheWindow;
    GtkWidget *	gDumbContainer;

    GtkWidget *	gBtnWidget;
    GtkWidget *	gRadioWidget;
    GtkWidget *	gRadioWidgetSibling;
    GtkWidget *	gCheckWidget;
    GtkWidget *	gScrollHorizWidget;
    GtkWidget *	gScrollVertWidget;
    GtkWidget *	gArrowWidget;
    GtkWidget *	gDropdownWidget;
    GtkWidget *	gEditBoxWidget;
    GtkWidget *	gSpinButtonWidget;
    GtkWidget *	gNotebookWidget;
    GtkWidget *	gOptionMenuWidget;
    GtkWidget *	gComboWidget;
    GtkWidget *	gScrolledWindowWidget;
    GtkWidget *  gToolbarWidget;
    GtkWidget *  gToolbarButtonWidget;
    GtkWidget *  gToolbarToggleWidget;
    GtkWidget *  gHandleBoxWidget;
    GtkWidget *  gMenubarWidget;
    GtkWidget *  gMenuItemMenubarWidget;
    GtkWidget *  gMenuWidget;
    GtkWidget *  gMenuItemMenuWidget;
    GtkWidget *  gMenuItemCheckMenuWidget;
    GtkWidget *  gMenuItemRadioMenuWidget;
    GtkWidget *  gImageMenuItem;
    GtkWidget *  gTooltipPopup;
    GtkWidget *  gProgressBar;
    GtkWidget *  gTreeView;

    NWPixmapCacheList* gNWPixmapCacheList;
    NWPixmapCache* gCacheTabItems;
    NWPixmapCache* gCacheTabPages;
    
    NWFWidgetData() :
        gCacheWindow( NULL ),
        gDumbContainer( NULL ),
        gBtnWidget( NULL ),
        gRadioWidget( NULL ),
        gRadioWidgetSibling( NULL ),
        gCheckWidget( NULL ),
        gScrollHorizWidget( NULL ),
        gScrollVertWidget( NULL ),
        gArrowWidget( NULL ),
        gDropdownWidget( NULL ),
        gEditBoxWidget( NULL ),
        gSpinButtonWidget( NULL ),
        gNotebookWidget( NULL ),
        gOptionMenuWidget( NULL ),
        gComboWidget( NULL ),
        gScrolledWindowWidget( NULL ),
        gToolbarWidget( NULL ),
        gToolbarButtonWidget( NULL ),
        gToolbarToggleWidget( NULL ),
        gHandleBoxWidget( NULL ),
        gMenubarWidget( NULL ),
        gMenuItemMenubarWidget( NULL ),
        gMenuWidget( NULL ),
        gMenuItemMenuWidget( NULL ),
        gMenuItemCheckMenuWidget( NULL ),
        gMenuItemRadioMenuWidget( NULL ),
	gImageMenuItem( NULL ),
        gTooltipPopup( NULL ),
        gProgressBar( NULL ),
        gTreeView( NULL ),
        gNWPixmapCacheList( NULL ),
        gCacheTabItems( NULL ),
        gCacheTabPages( NULL )
    {}
};

// Keep a hash table of Widgets->default flags so that we can
// easily and quickly reset each to a default state before using
// them
static std::hash_map<long, guint>	gWidgetDefaultFlags;
static std::vector<NWFWidgetData>   gWidgetData;

static const GtkBorder aDefDefBorder		= { 1, 1, 1, 1 };

// Some GTK defaults
#define MIN_ARROW_SIZE					11
#define BTN_CHILD_SPACING				1
#define MIN_SPIN_ARROW_WIDTH				6


static void NWEnsureGTKRadio			( int nScreen );
static void NWEnsureGTKButton			( int nScreen );
static void NWEnsureGTKCheck			( int nScreen );
static void NWEnsureGTKScrollbars		( int nScreen );
static void NWEnsureGTKArrow			( int nScreen );
static void NWEnsureGTKEditBox			( int nScreen );
static void NWEnsureGTKSpinButton		( int nScreen );
static void NWEnsureGTKNotebook			( int nScreen );
static void NWEnsureGTKOptionMenu		( int nScreen );
static void NWEnsureGTKCombo			( int nScreen );
static void NWEnsureGTKScrolledWindow	( int nScreen );
static void NWEnsureGTKToolbar			( int nScreen );
static void NWEnsureGTKMenubar          ( int nScreen );
static void NWEnsureGTKMenu             ( int nScreen );
static void NWEnsureGTKTooltip          ( int nScreen );
static void NWEnsureGTKProgressBar      ( int nScreen );
static void NWEnsureGTKTreeView         ( int nScreen );

static void NWConvertVCLStateToGTKState( ControlState nVCLState, GtkStateType* nGTKState, GtkShadowType* nGTKShadow );
static void NWAddWidgetToCacheWindow( GtkWidget* widget, int nScreen );
static void NWSetWidgetState( GtkWidget* widget, ControlState nState, GtkStateType nGtkState );

static void NWCalcArrowRect( const Rectangle& rButton, Rectangle& rArrow );

/*
 * Individual helper functions
 *
 */

//---
static Rectangle NWGetButtonArea( int nScreen, ControlType nType, ControlPart nPart, Rectangle aAreaRect, ControlState nState,
								const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption );

//---
static Rectangle NWGetEditBoxPixmapRect( int nScreen, ControlType nType, ControlPart nPart, Rectangle aAreaRect, ControlState nState,
							const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption );

static void NWPaintOneEditBox( int nScreen, GdkDrawable * gdkDrawable, GdkRectangle *gdkRect, 
                               ControlType nType, ControlPart nPart, Rectangle aEditBoxRect,
                               ControlState nState, const ImplControlValue& aValue,
                               SalControlHandle& rControlHandle, const OUString& rCaption );

//---
static Rectangle NWGetSpinButtonRect( int nScreen, ControlType nType, ControlPart nPart, Rectangle aAreaRect, ControlState nState,
							const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption );

static void NWPaintOneSpinButton( int nScreen, GdkPixmap * pixmap, ControlType nType, ControlPart nPart, Rectangle aAreaRect,
							ControlState nState, const ImplControlValue& aValue, SalControlHandle& rControlHandle,
							const OUString& rCaption );
//---
static Rectangle NWGetComboBoxButtonRect( int nScreen, ControlType nType, ControlPart nPart, Rectangle aAreaRect, ControlState nState,
							const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption );

//---
static Rectangle NWGetListBoxButtonRect( int nScreen, ControlType nType, ControlPart nPart, Rectangle aAreaRect, ControlState nState,
							const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption );

static Rectangle NWGetListBoxIndicatorRect( int nScreen, ControlType nType, ControlPart nPart, Rectangle aAreaRect, ControlState nState,
							const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption );

static Rectangle NWGetToolbarRect( int nScreen,
                                   ControlType nType,
                                   ControlPart nPart,
                                   Rectangle aAreaRect,
                                   ControlState nState,
                                   const ImplControlValue& aValue,
                                   SalControlHandle& rControlHandle,
                                   const OUString& rCaption );
//---

static Rectangle NWGetScrollButtonRect(	int nScreen, ControlPart nPart, Rectangle aAreaRect );
//---
								
/*********************************************************
 * PixmapCache
 *********************************************************/

// as some native widget drawing operations are pretty slow
// with certain themes (eg tabpages)
// this cache can be used to cache the corresponding pixmap
// see NWPaintGTKTabItem

class NWPixmapCacheData
{
public:
    ControlType m_nType;
    ControlState m_nState;
    Rectangle   m_pixmapRect;
    GdkPixmap*  m_pixmap;
    
    NWPixmapCacheData() : m_nType(0), m_nState(0), m_pixmap(0) {}
    ~NWPixmapCacheData()
        { SetPixmap( NULL ); };
    void SetPixmap( GdkPixmap* pPixmap );
};

class NWPixmapCache
{
    int m_size;
    int m_idx;
    int m_screen;
    NWPixmapCacheData* pData;
public:
    NWPixmapCache( int nScreen );
    ~NWPixmapCache();
    
    void SetSize( int n)
        { delete [] pData; m_idx = 0; m_size = n; pData = new NWPixmapCacheData[m_size]; }
    int GetSize() { return m_size; }
    
    BOOL Find( ControlType aType, ControlState aState, const Rectangle& r_pixmapRect, GdkPixmap** pPixmap );
    void Fill( ControlType aType, ControlState aState, const Rectangle& r_pixmapRect, GdkPixmap* pPixmap );
    
    void ThemeChanged();    
};

class NWPixmapCacheList
{
public:
    ::std::vector< NWPixmapCache* > mCaches;

    void AddCache( NWPixmapCache *pCache );
    void RemoveCache( NWPixmapCache *pCache );
    void ThemeChanged();
};

// --- implementation ---

void NWPixmapCacheData::SetPixmap( GdkPixmap* pPixmap )
{
    if( m_pixmap )
        g_object_unref( m_pixmap );
    
    m_pixmap = pPixmap;
    
    if( m_pixmap )
        g_object_ref( m_pixmap );
}


NWPixmapCache::NWPixmapCache( int nScreen )
{
    m_idx = 0;
    m_size = 0;
    m_screen = nScreen;
    pData = NULL;
    if( gWidgetData[m_screen].gNWPixmapCacheList )
        gWidgetData[m_screen].gNWPixmapCacheList->AddCache(this);
} 
NWPixmapCache::~NWPixmapCache()
{
    if( gWidgetData[m_screen].gNWPixmapCacheList )
        gWidgetData[m_screen].gNWPixmapCacheList->RemoveCache(this);
    delete[] pData;
}
void NWPixmapCache::ThemeChanged()
{
    // throw away cached pixmaps
    int i;
    for(i=0; i<m_size; i++)
        pData[i].SetPixmap( NULL );
}

BOOL  NWPixmapCache::Find( ControlType aType, ControlState aState, const Rectangle& r_pixmapRect, GdkPixmap** pPixmap )
{
    aState &= ~CTRL_CACHING_ALLOWED; // mask clipping flag
    int i;
    for(i=0; i<m_size; i++)
    {
        if( pData[i].m_nType == aType &&
            pData[i].m_nState == aState &&    
            pData[i].m_pixmapRect.GetWidth() == r_pixmapRect.GetWidth() &&    
            pData[i].m_pixmapRect.GetHeight() == r_pixmapRect.GetHeight() &&    
            pData[i].m_pixmap != NULL )
        {
            *pPixmap = pData[i].m_pixmap;
            return TRUE;
        }
    }
    return FALSE;
}

void NWPixmapCache::Fill( ControlType aType, ControlState aState, const Rectangle& r_pixmapRect, GdkPixmap* pPixmap )
{
    if( !(aState & CTRL_CACHING_ALLOWED) )
        return; 

    aState &= ~CTRL_CACHING_ALLOWED; // mask clipping flag
    m_idx = (m_idx+1) % m_size; // just wrap
    pData[m_idx].m_nType = aType;
    pData[m_idx].m_nState = aState;
    pData[m_idx].m_pixmapRect = r_pixmapRect;
    pData[m_idx].SetPixmap( pPixmap );
}


void NWPixmapCacheList::AddCache( NWPixmapCache* pCache )
{
    mCaches.push_back( pCache );
}
void NWPixmapCacheList::RemoveCache( NWPixmapCache* pCache )
{
    ::std::vector< NWPixmapCache* >::iterator p;
    p = ::std::find( mCaches.begin(), mCaches.end(), pCache );
    if( p != mCaches.end() )
        mCaches.erase( p );
}
void NWPixmapCacheList::ThemeChanged( )
{
    ::std::vector< NWPixmapCache* >::iterator p = mCaches.begin();
    while( p != mCaches.end() )
    {
        (*p)->ThemeChanged();
        p++;
    }
}


/*********************************************************
 * Make border manipulation easier
 *********************************************************/
inline void NW_gtk_border_set_from_border( GtkBorder& aDst, const GtkBorder * pSrc )
{
	aDst.left		= pSrc->left;
	aDst.top		= pSrc->top;
	aDst.right	= pSrc->right;
	aDst.bottom	= pSrc->bottom;
}


/*********************************************************
 * Initialize GTK and local stuff                         
 *********************************************************/
void GtkData::initNWF( void )
{
    ImplSVData* pSVData = ImplGetSVData();

    // draw no border for popup menus (NWF draws its own)
    pSVData->maNWFData.mbFlatMenu = true;

    // draw separate buttons for toolbox dropdown items
    pSVData->maNWFData.mbToolboxDropDownSeparate = true;
    
    // small extra border around menu items
    pSVData->maNWFData.mnMenuFormatExtraBorder = 1;
    
    // draw toolbars in separate lines
    pSVData->maNWFData.mbDockingAreaSeparateTB = true;
    
    // open first menu on F10
    pSVData->maNWFData.mbOpenMenuOnF10 = true;
    
    int nScreens = GetX11SalData()->GetDisplay()->GetScreenCount();
    gWidgetData = std::vector<NWFWidgetData>( nScreens );
    for( int i = 0; i < nScreens; i++ )
        gWidgetData[i].gNWPixmapCacheList = new NWPixmapCacheList;

    
    if( SalGetDesktopEnvironment().equalsAscii( "KDE" ) )
    {
        // #i97196# ensure a widget exists and the style engine was loaded
        NWEnsureGTKButton( 0 );
        if( g_type_from_name( "QtEngineStyle" ) )
        {
            // KDE 3.3 invented a bug in the qt<->gtk theme engine
            // that makes direct rendering impossible: they totally
            // ignore the clip rectangle passed to the paint methods
            GtkSalGraphics::bNeedPixmapPaint = GtkSalGraphics::bGlobalNeedPixmapPaint = true;
        }
    }
    static const char* pEnv = getenv( "SAL_GTK_USE_PIXMAPPAINT" );
    if( pEnv && *pEnv )
        GtkSalGraphics::bNeedPixmapPaint = GtkSalGraphics::bGlobalNeedPixmapPaint = true;

    #if OSL_DEBUG_LEVEL > 1
    std::fprintf( stderr, "GtkPlugin: using %s NWF\n",
             GtkSalGraphics::bNeedPixmapPaint ? "offscreen" : "direct" );
    #endif
}


/*********************************************************
 * Release GTK and local stuff                         
 *********************************************************/
void GtkData::deInitNWF( void )
{

    for( unsigned int i = 0; i < gWidgetData.size(); i++ )
    {
        // free up global widgets
        // gtk_widget_destroy will in turn destroy the child hierarchy
        // so only destroy disjunct hierachies
        if( gWidgetData[i].gCacheWindow )
            gtk_widget_destroy( gWidgetData[i].gCacheWindow );
        if( gWidgetData[i].gMenuWidget )
            gtk_widget_destroy( gWidgetData[i].gMenuWidget );
        if( gWidgetData[i].gTooltipPopup )
            gtk_widget_destroy( gWidgetData[i].gTooltipPopup );
        delete gWidgetData[i].gCacheTabPages;
        gWidgetData[i].gCacheTabPages = NULL;
        delete gWidgetData[i].gCacheTabItems;
        gWidgetData[i].gCacheTabItems = NULL;
        delete gWidgetData[i].gNWPixmapCacheList;
        gWidgetData[i].gNWPixmapCacheList = NULL;
    }
}


/**********************************************************
 * track clip region
 **********************************************************/
void GtkSalGraphics::ResetClipRegion()
{
    m_aClipRegion.SetNull();
    X11SalGraphics::ResetClipRegion();
}

void GtkSalGraphics::BeginSetClipRegion( ULONG nCount )
{
    m_aClipRegion.SetNull();
    X11SalGraphics::BeginSetClipRegion( nCount );
}

BOOL GtkSalGraphics::unionClipRegion( long nX, long nY, long nWidth, long nHeight )
{
    Rectangle aRect( Point( nX, nY ), Size( nWidth, nHeight ) );
    m_aClipRegion.Union( aRect );
    return X11SalGraphics::unionClipRegion( nX, nY, nWidth, nHeight );
}

bool GtkSalGraphics::unionClipRegion( const ::basegfx::B2DPolyPolygon& )
{
        // TODO: implement and advertise OutDevSupport_B2DClip support
        return false;
}

void GtkSalGraphics::EndSetClipRegion()
{
    if( m_aClipRegion.IsEmpty() )
        m_aClipRegion.SetNull();
    X11SalGraphics::EndSetClipRegion();
}

void GtkSalGraphics::copyBits( const SalTwoRect* pPosAry,
                               SalGraphics* pSrcGraphics )
{
    GtkSalFrame* pFrame = GetGtkFrame();
    XLIB_Window aWin = None;
    if( pFrame && m_pWindow )
    {
        /* #i64117# some themes set the background pixmap VERY frequently */
        GdkWindow* pWin = GTK_WIDGET(m_pWindow)->window;
        if( pWin )
        {
            aWin = GDK_WINDOW_XWINDOW(pWin);
            if( aWin != None )
                XSetWindowBackgroundPixmap( pFrame->getDisplay()->GetDisplay(),
                                            aWin,
                                            None );
        }
    }
    X11SalGraphics::copyBits( pPosAry, pSrcGraphics );
    if( pFrame && pFrame->getBackgroundPixmap() != None )
        XSetWindowBackgroundPixmap( pFrame->getDisplay()->GetDisplay(),
                                    aWin,
                                    pFrame->getBackgroundPixmap() );
}

/*
 * IsNativeControlSupported()
 *
 *  Returns TRUE if the platform supports native
 *  drawing of the control defined by nPart
 */
BOOL GtkSalGraphics::IsNativeControlSupported( ControlType nType, ControlPart nPart )
{
	if (
		((nType==CTRL_PUSHBUTTON)  && (nPart==PART_ENTIRE_CONTROL)) 	||
 		((nType==CTRL_RADIOBUTTON) && (nPart==PART_ENTIRE_CONTROL))		||
		((nType==CTRL_CHECKBOX)    && (nPart==PART_ENTIRE_CONTROL))		||
		((nType==CTRL_SCROLLBAR) &&
				(  (nPart==PART_DRAW_BACKGROUND_HORZ)
				|| (nPart==PART_DRAW_BACKGROUND_VERT)
				|| (nPart==PART_ENTIRE_CONTROL)
                || (nPart==HAS_THREE_BUTTONS) )  				)	||
		((nType==CTRL_EDITBOX) &&
				(  (nPart==PART_ENTIRE_CONTROL)
				|| (nPart==HAS_BACKGROUND_TEXTURE) ) 			)	||
		((nType==CTRL_MULTILINE_EDITBOX) &&
				(  (nPart==PART_ENTIRE_CONTROL)
				|| (nPart==HAS_BACKGROUND_TEXTURE) ) 			)	||
		((nType==CTRL_SPINBOX) &&
				(  (nPart==PART_ENTIRE_CONTROL)
				|| (nPart==PART_ALL_BUTTONS)
				|| (nPart==HAS_BACKGROUND_TEXTURE) )			)	||
		((nType==CTRL_SPINBUTTONS) &&
				(  (nPart==PART_ENTIRE_CONTROL)
				|| (nPart==PART_ALL_BUTTONS)	)				)	||
		((nType==CTRL_COMBOBOX) &&
				(  (nPart==PART_ENTIRE_CONTROL)
				|| (nPart==HAS_BACKGROUND_TEXTURE)	)			)	||
		(((nType==CTRL_TAB_ITEM) || (nType==CTRL_TAB_PANE) ||
		  (nType==CTRL_TAB_BODY) || (nType==CTRL_FIXEDBORDER)) &&
				(  (nPart==PART_ENTIRE_CONTROL)
				|| (nPart==PART_TABS_DRAW_RTL) )				)	||
		((nType==CTRL_LISTBOX) &&
				(  (nPart==PART_ENTIRE_CONTROL)
				|| (nPart==PART_WINDOW)
				|| (nPart==HAS_BACKGROUND_TEXTURE) )			)   ||
        ((nType == CTRL_TOOLBAR) &&
         		(	(nPart==PART_ENTIRE_CONTROL)
                ||  (nPart==PART_DRAW_BACKGROUND_HORZ)
                ||  (nPart==PART_DRAW_BACKGROUND_VERT)
                ||  (nPart==PART_THUMB_HORZ)
                ||  (nPart==PART_THUMB_VERT)
                ||  (nPart==PART_BUTTON)
                )
                                                                )   ||
        ((nType == CTRL_MENUBAR) &&
                (   (nPart==PART_ENTIRE_CONTROL) )              )   ||
        ((nType == CTRL_TOOLTIP) &&
                (   (nPart==PART_ENTIRE_CONTROL) )              )   ||
        ((nType == CTRL_MENU_POPUP) &&
                (   (nPart==PART_ENTIRE_CONTROL)
                ||  (nPart==PART_MENU_ITEM)
                ||  (nPart==PART_MENU_ITEM_CHECK_MARK)
                ||  (nPart==PART_MENU_ITEM_RADIO_MARK)
                )
                                                                )   ||
        ((nType == CTRL_PROGRESS) &&
                (   (nPart == PART_ENTIRE_CONTROL) )
                )                                                   ||
        ((nType == CTRL_LISTNODE || nType == CTRL_LISTNET) &&
                (   (nPart == PART_ENTIRE_CONTROL) )
                )
        )
		return( TRUE );

	return( FALSE );
}


/*
 * HitTestNativeControl()
 *
 *  bIsInside is set to TRUE if aPos is contained within the
 *  given part of the control, whose bounding region is
 *  given by rControlRegion (in VCL frame coordinates).
 *
 *  returns whether bIsInside was really set.
 */
BOOL GtkSalGraphics::hitTestNativeControl( ControlType		nType,
								ControlPart		nPart,
								const Region&		rControlRegion,
								const Point&		aPos,
								SalControlHandle&,
								BOOL&			rIsInside )
{
    if ( ( nType == CTRL_SCROLLBAR ) &&
         ( ( nPart == PART_BUTTON_UP ) ||
           ( nPart == PART_BUTTON_DOWN ) ||
           ( nPart == PART_BUTTON_LEFT ) ||
           ( nPart == PART_BUTTON_RIGHT ) ) )
    {
        NWEnsureGTKScrollbars( m_nScreen );

        // Grab some button style attributes
        gboolean has_forward;
        gboolean has_forward2;
        gboolean has_backward;
        gboolean has_backward2;
        
	    gtk_widget_style_get( gWidgetData[m_nScreen].gScrollHorizWidget, "has-forward-stepper", &has_forward,
									    "has-secondary-forward-stepper", &has_forward2,
									    "has-backward-stepper", &has_backward,
	   								    "has-secondary-backward-stepper", &has_backward2, (char *)NULL );
        Rectangle aForward;
        Rectangle aBackward;

        rIsInside = FALSE;

        ControlPart nCounterPart = 0;
        if ( nPart == PART_BUTTON_UP )
            nCounterPart = PART_BUTTON_DOWN;
        else if ( nPart == PART_BUTTON_DOWN )
            nCounterPart = PART_BUTTON_UP;
        else if ( nPart == PART_BUTTON_LEFT )
            nCounterPart = PART_BUTTON_RIGHT;
        else if ( nPart == PART_BUTTON_RIGHT )
            nCounterPart = PART_BUTTON_LEFT;

        aBackward = NWGetScrollButtonRect( m_nScreen, nPart, rControlRegion.GetBoundRect() );
        aForward = NWGetScrollButtonRect( m_nScreen, nCounterPart, rControlRegion.GetBoundRect() );

        if ( has_backward && has_forward2 )
        {
            Size aSize( aBackward.GetSize() );
            if ( ( nPart == PART_BUTTON_UP ) || ( nPart == PART_BUTTON_DOWN ) )
                aSize.setHeight( aBackward.GetHeight() / 2 );
            else
                aSize.setWidth( aBackward.GetWidth() / 2 );
            aBackward.SetSize( aSize );

            if ( nPart == PART_BUTTON_DOWN )
                aBackward.Move( 0, aBackward.GetHeight() / 2 );
            else if ( nPart == PART_BUTTON_RIGHT )
                aBackward.Move( aBackward.GetWidth() / 2, 0 );
        }

        if ( has_backward2 && has_forward )
        {
            Size aSize( aForward.GetSize() );
            if ( ( nPart == PART_BUTTON_UP ) || ( nPart == PART_BUTTON_DOWN ) )
                aSize.setHeight( aForward.GetHeight() / 2 );
            else
                aSize.setWidth( aForward.GetWidth() / 2 );
            aForward.SetSize( aSize );

            if ( nPart == PART_BUTTON_DOWN )
                aForward.Move( 0, aForward.GetHeight() / 2 );
            else if ( nPart == PART_BUTTON_RIGHT )
                aForward.Move( aForward.GetWidth() / 2, 0 );
        }

        if ( ( nPart == PART_BUTTON_UP ) || ( nPart == PART_BUTTON_LEFT ) )
        {
            if ( has_backward )
                rIsInside |= aBackward.IsInside( aPos );
            if ( has_backward2 )
                rIsInside |= aForward.IsInside( aPos );
        }
        else
        {
            if ( has_forward )
                rIsInside |= aBackward.IsInside( aPos );
            if ( has_forward2 )
                rIsInside |= aForward.IsInside( aPos );
        }
        return ( TRUE );
    }

    if( IsNativeControlSupported(nType, nPart) )
	{
		rIsInside = rControlRegion.IsInside( aPos );
		return( TRUE );
	}
	else
	{
		return( FALSE );
	}
}


/*
 * DrawNativeControl()
 *
 *  Draws the requested control described by nPart/nState.
 *
 *  rControlRegion:	The bounding region of the complete control in VCL frame coordinates.
 *  aValue:  		An optional value (tristate/numerical/string)
 *  rControlHandle:	Carries platform dependent data and is maintained by the SalFrame implementation.
 *  rCaption:  	A caption or title string (like button text etc)
 */
BOOL GtkSalGraphics::drawNativeControl(	ControlType nType,
							ControlPart nPart,
							const Region& rControlRegion,
							ControlState nState,
							const ImplControlValue& aValue,
							SalControlHandle& rControlHandle,
							const OUString& rCaption )
{
    if( (nType==CTRL_CHECKBOX) && (nPart==PART_ENTIRE_CONTROL) &&
        aValue.getTristateVal() == BUTTONVALUE_MIXED )
    {
        return drawNativeMixedStateCheck( nType, nPart, rControlRegion, nState, aValue, rControlHandle, rCaption );
    }

	BOOL			returnVal = FALSE;
	// get a GC with current clipping region set
    SelectFont();
    
    
    // theme changed ?
    if( GtkSalGraphics::bThemeChanged )
    {
        // invalidate caches
        for( unsigned int i = 0; i < gWidgetData.size(); i++ )
            if( gWidgetData[i].gNWPixmapCacheList )
                gWidgetData[i].gNWPixmapCacheList->ThemeChanged();
        GtkSalGraphics::bThemeChanged = FALSE;    
    }
    
    Rectangle aCtrlRect = rControlRegion.GetBoundRect();
    Region aClipRegion( m_aClipRegion );
    if( aClipRegion.IsNull() )
        aClipRegion = aCtrlRect;
            
    clipList aClip;        
    GdkDrawable* gdkDrawable = GDK_DRAWABLE( GetGdkWindow() );
    GdkPixmap* pixmap = NULL;
    Rectangle aPixmapRect;
    if( ( bNeedPixmapPaint )
        && nType != CTRL_SCROLLBAR
        && nType != CTRL_SPINBOX
        && nType != CTRL_TAB_ITEM
        && nType != CTRL_TAB_PANE
        && nType != CTRL_PROGRESS
        && ! (bToolbarGripWorkaround && nType == CTRL_TOOLBAR && (nPart == PART_THUMB_HORZ || nPart == PART_THUMB_VERT) )
        ) 
    {
        // make pixmap a little larger since some themes draw decoration
        // outside the rectangle, see e.g. checkbox
        aPixmapRect = Rectangle( Point( aCtrlRect.Left()-1, aCtrlRect.Top()-1 ),
                                 Size( aCtrlRect.GetWidth()+2, aCtrlRect.GetHeight()+2) );
        pixmap = NWGetPixmapFromScreen( aPixmapRect );
        if( ! pixmap )
            return FALSE;
        gdkDrawable = GDK_DRAWABLE( pixmap );
        aCtrlRect = Rectangle( Point(1,1), aCtrlRect.GetSize() );
        aClip.push_back( aCtrlRect );
    }
    else
    {
        RegionHandle aHdl = aClipRegion.BeginEnumRects();
        Rectangle aPaintRect;
        while( aClipRegion.GetNextEnumRect( aHdl, aPaintRect ) )
        {
            aPaintRect = aCtrlRect.GetIntersection( aPaintRect );
            if( aPaintRect.IsEmpty() )
                continue;
            aClip.push_back( aPaintRect );
        }
        aClipRegion.EndEnumRects( aHdl );
    }
    
    if ( (nType==CTRL_PUSHBUTTON) && (nPart==PART_ENTIRE_CONTROL) )
    {
        returnVal = NWPaintGTKButton( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( (nType==CTRL_RADIOBUTTON) && (nPart==PART_ENTIRE_CONTROL) )
    {
        returnVal = NWPaintGTKRadio( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( (nType==CTRL_CHECKBOX) && (nPart==PART_ENTIRE_CONTROL) )
    {
        returnVal = NWPaintGTKCheck( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( (nType==CTRL_SCROLLBAR) && ((nPart==PART_DRAW_BACKGROUND_HORZ) || (nPart==PART_DRAW_BACKGROUND_VERT)) )
    {
        returnVal = NWPaintGTKScrollbar( nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( ((nType==CTRL_EDITBOX) && ((nPart==PART_ENTIRE_CONTROL) || (nPart==HAS_BACKGROUND_TEXTURE)) )
        || ((nType==CTRL_SPINBOX) && (nPart==HAS_BACKGROUND_TEXTURE))
    || ((nType==CTRL_COMBOBOX) && (nPart==HAS_BACKGROUND_TEXTURE))
    || ((nType==CTRL_LISTBOX) && (nPart==HAS_BACKGROUND_TEXTURE)) )
    {
        returnVal = NWPaintGTKEditBox( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( ((nType==CTRL_MULTILINE_EDITBOX) && ((nPart==PART_ENTIRE_CONTROL) || (nPart==HAS_BACKGROUND_TEXTURE)) ) )
    {
        returnVal = NWPaintGTKEditBox( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( ((nType==CTRL_SPINBOX) || (nType==CTRL_SPINBUTTONS))
        && ((nPart==PART_ENTIRE_CONTROL) || (nPart==PART_ALL_BUTTONS)) )
    {
        returnVal = NWPaintGTKSpinBox( nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( (nType == CTRL_COMBOBOX) &&
        ( (nPart==PART_ENTIRE_CONTROL)
        ||(nPart==PART_BUTTON_DOWN)
        ) )
    {
        returnVal = NWPaintGTKComboBox( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( (nType==CTRL_TAB_ITEM) || (nType==CTRL_TAB_PANE) || (nType==CTRL_TAB_BODY) || (nType==CTRL_FIXEDBORDER) )
    {
        if ( nType == CTRL_TAB_BODY )
            returnVal = TRUE;
        else
            returnVal = NWPaintGTKTabItem( nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption);
    }
    else if ( (nType==CTRL_LISTBOX) && ((nPart==PART_ENTIRE_CONTROL) || (nPart==PART_WINDOW)) )
    {
        returnVal = NWPaintGTKListBox( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( (nType== CTRL_TOOLBAR) )
    {
        returnVal = NWPaintGTKToolbar( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if ( (nType== CTRL_MENUBAR) )
    {
        returnVal = NWPaintGTKMenubar( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if(    (nType == CTRL_MENU_POPUP)
        && (  (nPart == PART_ENTIRE_CONTROL)
    || (nPart == PART_MENU_ITEM)
    || (nPart == PART_MENU_ITEM_CHECK_MARK)
    || (nPart == PART_MENU_ITEM_RADIO_MARK)
    )
    )
    {
        returnVal = NWPaintGTKPopupMenu( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if( (nType == CTRL_TOOLTIP) && (nPart == PART_ENTIRE_CONTROL) )
    {
        returnVal = NWPaintGTKTooltip( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if( (nType == CTRL_PROGRESS) && (nPart == PART_ENTIRE_CONTROL) )
    {
        returnVal = NWPaintGTKProgress( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if( (nType == CTRL_LISTNODE) && (nPart == PART_ENTIRE_CONTROL) )
    {
        returnVal = NWPaintGTKListNode( gdkDrawable, nType, nPart, aCtrlRect, aClip, nState, aValue, rControlHandle, rCaption );
    }
    else if( (nType == CTRL_LISTNET) && (nPart == PART_ENTIRE_CONTROL) )
    {
        // don't actually draw anything; gtk treeviews do not draw lines
        returnVal = true;
    }
    
    if( pixmap )
    {
        returnVal = NWRenderPixmapToScreen( pixmap, aPixmapRect ) && returnVal;
        g_object_unref( pixmap );
    }

	return( returnVal );
}

BOOL GtkSalGraphics::drawNativeMixedStateCheck(	ControlType nType,
							                    ControlPart nPart,
                                                const Region& rControlRegion,
                                                ControlState nState,
                                                const ImplControlValue& aValue,
                                                SalControlHandle& rControlHandle,
                                                const OUString& rCaption )
{
    // need to emulate something for mixed state

    // do this via pixmap since some themes don't care for regions
    bool bOldNeedPixmapPaint = bNeedPixmapPaint;
    bNeedPixmapPaint = true;
    
    Rectangle aCtrlRect = rControlRegion.GetBoundRect();
	BOOL      returnVal = FALSE;
    SelectFont();

    // draw upper half in off state
    const_cast<ImplControlValue&>(aValue).setTristateVal( BUTTONVALUE_OFF );
    XLIB_Region aRegion = XCreateRegion();
    XRectangle aXRect = { aCtrlRect.Left(), aCtrlRect.Top(), aCtrlRect.GetWidth(), aCtrlRect.GetHeight() };
    const unsigned short nH = aXRect.height/2;
    aXRect.height -= nH;
    XUnionRectWithRegion( &aXRect, aRegion, aRegion );
    SetClipRegion( pFontGC_, aRegion );
    XDestroyRegion( aRegion );
    
    returnVal = drawNativeControl( nType, nPart, rControlRegion, nState, aValue, rControlHandle, rCaption );
    
    if( returnVal )
    {
        // draw lower half in on state
        const_cast<ImplControlValue&>(aValue).setTristateVal( BUTTONVALUE_ON );
        aXRect.y += nH;
        aRegion = XCreateRegion();
        XUnionRectWithRegion( &aXRect, aRegion, aRegion );
        SetClipRegion( pFontGC_, aRegion );
        XDestroyRegion( aRegion );
        returnVal = drawNativeControl( nType, nPart, rControlRegion, nState, aValue, rControlHandle, rCaption );
    }
    
    // clean up
    bNeedPixmapPaint = bOldNeedPixmapPaint;
    const_cast<ImplControlValue&>(aValue).setTristateVal( BUTTONVALUE_MIXED );
    SetClipRegion( pFontGC_ );
    return returnVal;
}


/*
 * DrawNativeControlText()
 *
 *  OPTIONAL.  Draws the requested text for the control described by nPart/nState.
 *     Used if text not drawn by DrawNativeControl().
 *
 *  rControlRegion:	The bounding region of the complete control in VCL frame coordinates.
 *  aValue:  		An optional value (tristate/numerical/string)
 *  rControlHandle:	Carries platform dependent data and is maintained by the SalFrame implementation.
 *  rCaption:  	A caption or title string (like button text etc)
 */
BOOL GtkSalGraphics::drawNativeControlText(	ControlType,
								ControlPart,
								const Region&,
								ControlState,
								const ImplControlValue&,
								SalControlHandle&,
								const OUString& )
{
	return( FALSE );
}


/*
 * GetNativeControlRegion()
 *
 *  If the return value is TRUE, rNativeBoundingRegion
 *  contains the TRUE bounding region covered by the control
 *  including any adornment, while rNativeContentRegion contains the area
 *  within the control that can be safely drawn into without drawing over
 *  the borders of the control.
 *
 *  rControlRegion:	The bounding region of the control in VCL frame coordinates.
 *  aValue:		An optional value (tristate/numerical/string)
 *  rControlHandle:	Carries platform dependent data and is maintained by the SalFrame implementation.
 *  rCaption:		A caption or title string (like button text etc)
 */
BOOL GtkSalGraphics::getNativeControlRegion(  ControlType nType,
								ControlPart nPart,
								const Region& rControlRegion,
								ControlState nState,
								const ImplControlValue& aValue,
								SalControlHandle& rControlHandle,
								const OUString& rCaption,
								Region &rNativeBoundingRegion,
								Region &rNativeContentRegion )
{
	BOOL returnVal = FALSE;

    if ( (nType==CTRL_PUSHBUTTON) && (nPart==PART_ENTIRE_CONTROL)
        && (rControlRegion.GetBoundRect().GetWidth() > 16)
    && (rControlRegion.GetBoundRect().GetHeight() > 16) )
    {
        rNativeBoundingRegion = NWGetButtonArea( m_nScreen, nType, nPart, rControlRegion.GetBoundRect(),
        nState, aValue, rControlHandle, rCaption );
        rNativeContentRegion = rControlRegion;
        
        returnVal = TRUE;
    }
    if ( (nType==CTRL_COMBOBOX) && ((nPart==PART_BUTTON_DOWN) || (nPart==PART_SUB_EDIT)) )
    {
        rNativeBoundingRegion = NWGetComboBoxButtonRect( m_nScreen, nType, nPart, rControlRegion.GetBoundRect(), nState,
        aValue, rControlHandle, rCaption );
        rNativeContentRegion = rNativeBoundingRegion;
        
        returnVal = TRUE;
    }
    if ( (nType==CTRL_SPINBOX) && ((nPart==PART_BUTTON_UP) || (nPart==PART_BUTTON_DOWN) || (nPart==PART_SUB_EDIT)) )
    {
        
        rNativeBoundingRegion = NWGetSpinButtonRect( m_nScreen, nType, nPart, rControlRegion.GetBoundRect(), nState,
        aValue, rControlHandle, rCaption );
        rNativeContentRegion = rNativeBoundingRegion;
        
        returnVal = TRUE;
    }
    if ( (nType==CTRL_LISTBOX) && ((nPart==PART_BUTTON_DOWN) || (nPart==PART_SUB_EDIT)) )
    {
        rNativeBoundingRegion = NWGetListBoxButtonRect( m_nScreen, nType, nPart, rControlRegion.GetBoundRect(), nState,
        aValue, rControlHandle, rCaption );
        rNativeContentRegion = rNativeBoundingRegion;
        
        returnVal = TRUE;
    }
    if ( (nType==CTRL_TOOLBAR) &&
        ((nPart==PART_DRAW_BACKGROUND_HORZ)	||
        (nPart==PART_DRAW_BACKGROUND_VERT)	||
        (nPart==PART_THUMB_HORZ)			||
        (nPart==PART_THUMB_VERT)            ||
        (nPart==PART_BUTTON)
        ))
    {
        rNativeBoundingRegion = NWGetToolbarRect( m_nScreen, nType, nPart, rControlRegion.GetBoundRect(), nState, aValue, rControlHandle, rCaption );
        rNativeContentRegion = rNativeBoundingRegion;
        returnVal = TRUE;
    }
    if ( (nType==CTRL_SCROLLBAR) && ((nPart==PART_BUTTON_LEFT) || (nPart==PART_BUTTON_RIGHT) ||
        (nPart==PART_BUTTON_UP) || (nPart==PART_BUTTON_DOWN)  ) )
    {
        rNativeBoundingRegion = NWGetScrollButtonRect( m_nScreen, nPart, rControlRegion.GetBoundRect() );
        rNativeContentRegion = rNativeBoundingRegion;
        
        returnVal = TRUE;
    }
    if( (nType == CTRL_MENUBAR) && (nPart == PART_ENTIRE_CONTROL) )
    {
        NWEnsureGTKMenubar( m_nScreen );
        GtkRequisition aReq;
        gtk_widget_size_request( gWidgetData[m_nScreen].gMenubarWidget, &aReq );
        Rectangle aMenuBarRect = rControlRegion.GetBoundRect();
        aMenuBarRect = Rectangle( aMenuBarRect.TopLeft(),
                                  Size( aMenuBarRect.GetWidth(), aReq.height+1 ) );
        rNativeBoundingRegion = Region( aMenuBarRect );
        rNativeContentRegion = rNativeBoundingRegion;
        returnVal = TRUE;
    }
    if( (nType == CTRL_MENU_POPUP) )
    {
        if( (nPart == PART_MENU_ITEM_CHECK_MARK) ||
            (nPart == PART_MENU_ITEM_RADIO_MARK) )
        {
            NWEnsureGTKMenu( m_nScreen );

            gint indicator_size = 0;
            GtkWidget* pWidget = (nPart == PART_MENU_ITEM_CHECK_MARK) ?
                                 gWidgetData[m_nScreen].gMenuItemCheckMenuWidget : gWidgetData[m_nScreen].gMenuItemRadioMenuWidget;
            gtk_widget_style_get( pWidget,
                                  "indicator_size", &indicator_size,
                                  (char *)NULL );
            rNativeBoundingRegion = rControlRegion;
            Rectangle aIndicatorRect( Point( 0,
                                             (rControlRegion.GetBoundRect().GetHeight()-indicator_size)/2),
                                      Size( indicator_size, indicator_size ) );
            rNativeContentRegion = Region( aIndicatorRect );
            returnVal = TRUE;
        }
    }
    if( (nType == CTRL_RADIOBUTTON || nType == CTRL_CHECKBOX) )
    {
        NWEnsureGTKRadio( m_nScreen );
        NWEnsureGTKCheck( m_nScreen );
        GtkWidget* widget = (nType == CTRL_RADIOBUTTON) ? gWidgetData[m_nScreen].gRadioWidget : gWidgetData[m_nScreen].gCheckWidget;
        gint indicator_size, indicator_spacing;
        gtk_widget_style_get( widget,
                              "indicator_size", &indicator_size,
                              "indicator_spacing", &indicator_spacing,
                              (char *)NULL);
        indicator_size += 2*indicator_spacing; // guess overpaint of theme
        rNativeBoundingRegion = rControlRegion;
        Rectangle aIndicatorRect( Point( 0,
                                         (rControlRegion.GetBoundRect().GetHeight()-indicator_size)/2),
                                  Size( indicator_size, indicator_size ) );
        rNativeContentRegion = Region( aIndicatorRect );
        returnVal = TRUE;
    }
        
	return( returnVal );
}


/************************************************************************
 * Individual control drawing functions
 ************************************************************************/
BOOL GtkSalGraphics::NWPaintGTKButton(
            GdkDrawable* gdkDrawable,
            ControlType, ControlPart,
			const Rectangle& rControlRectangle,            
            const clipList& rClipList,
            ControlState nState, const ImplControlValue&,
			SalControlHandle&, const OUString& )
{
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	gboolean		interiorFocus;
	gint			focusWidth;
	gint			focusPad;
	BOOL			bDrawFocus = TRUE;
	gint			x, y, w, h;
	GtkBorder		aDefBorder;
	GtkBorder*		pBorder;
    GdkRectangle	clipRect;
    
	NWEnsureGTKButton( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

	x = rControlRectangle.Left();
    y = rControlRectangle.Top();
	w = rControlRectangle.GetWidth();
	h = rControlRectangle.GetHeight();

	// Grab some button style attributes
	gtk_widget_style_get( gWidgetData[m_nScreen].gBtnWidget,	"focus-line-width",	&focusWidth,
								"focus-padding", 	&focusPad,
					 			"interior_focus",	&interiorFocus,
								"default_border",	&pBorder,
								(char *)NULL );

	// Make sure the border values exist, otherwise use some defaults
	if ( pBorder )
	{
		NW_gtk_border_set_from_border( aDefBorder, pBorder );
		gtk_border_free( pBorder );
	}
	else NW_gtk_border_set_from_border( aDefBorder, &aDefDefBorder );

	// If the button is too small, don't ever draw focus or grab more space
	if ( (w < 16) || (h < 16) )
		bDrawFocus = FALSE;

	NWSetWidgetState( gWidgetData[m_nScreen].gBtnWidget, nState, stateType );

    gint xi = x, yi = y, wi = w, hi = h;
	if ( (nState & CTRL_STATE_DEFAULT) && bDrawFocus )
	{
		xi += aDefBorder.left;
		yi += aDefBorder.top;
		wi -= aDefBorder.left + aDefBorder.right;
		hi -= aDefBorder.top + aDefBorder.bottom;
	}

	if ( !interiorFocus && bDrawFocus )
	{
		xi += focusWidth + focusPad;
		yi += focusWidth + focusPad;
		wi -= 2 * (focusWidth + focusPad);
		hi -= 2 * (focusWidth + focusPad);
	}

    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it)
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        // Buttons must paint opaque since some themes have alpha-channel enabled buttons
        gtk_paint_flat_box( gWidgetData[m_nScreen].gBtnWidget->style, gdkDrawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE, 
                            &clipRect, m_pWindow, "base", x, y, w, h );
        
        if ( (nState & CTRL_STATE_DEFAULT) && (GTK_BUTTON(gWidgetData[m_nScreen].gBtnWidget)->relief == GTK_RELIEF_NORMAL) )
        {
            gtk_paint_box( gWidgetData[m_nScreen].gBtnWidget->style, gdkDrawable, GTK_STATE_NORMAL, GTK_SHADOW_IN, 
                           &clipRect, gWidgetData[m_nScreen].gBtnWidget, "buttondefault", x, y, w, h );
        }
        
        if ( (GTK_BUTTON(gWidgetData[m_nScreen].gBtnWidget)->relief != GTK_RELIEF_NONE)
            || (nState & CTRL_STATE_PRESSED)
		    || (nState & CTRL_STATE_ROLLOVER) )
        {
            gtk_paint_box( gWidgetData[m_nScreen].gBtnWidget->style, gdkDrawable, stateType, shadowType,
                           &clipRect, gWidgetData[m_nScreen].gBtnWidget, "button", xi, yi, wi, hi );
        }
    }
#if 0 // VCL draws focus rects
	// Draw focus rect
	if ( (nState & CTRL_STATE_FOCUSED) && (nState & CTRL_STATE_ENABLED) && bDrawFocus )
	{
		if (interiorFocus)
		{
			x += gWidgetData[m_nScreen].gBtnWidget->style->xthickness + focusPad;
			y += gWidgetData[m_nScreen].gBtnWidget->style->ythickness + focusPad;
			w -= 2 * (gWidgetData[m_nScreen].gBtnWidget->style->xthickness + focusPad);
			h -=  2 * (gWidgetData[m_nScreen].gBtnWidget->style->xthickness + focusPad);
		}
		else
		{
			x -= focusWidth + focusPad;
			y -= focusWidth + focusPad;
			w += 2 * (focusWidth + focusPad);
			h += 2 * (focusWidth + focusPad);
		}
		if ( !interiorFocus )
			gtk_paint_focus( gWidgetData[m_nScreen].gBtnWidget->style, gdkDrawable, stateType, &clipRect,
                             gWidgetData[m_nScreen].gBtnWidget, "button", x, y, w, h );
	}
#endif

	return( TRUE );
}

static Rectangle NWGetButtonArea( int nScreen,
                                  ControlType, ControlPart, Rectangle aAreaRect, ControlState nState,
							      const ImplControlValue&, SalControlHandle&, const OUString& )
{
	gboolean		interiorFocus;
	gint			focusWidth;
	gint			focusPad;
	GtkBorder		aDefBorder;
	GtkBorder *	pBorder;
	BOOL			bDrawFocus = TRUE;
	Rectangle		aRect;
	gint			x, y, w, h;

	NWEnsureGTKButton( nScreen );
	gtk_widget_style_get( gWidgetData[nScreen].gBtnWidget,
                            	"focus-line-width",	&focusWidth,
								"focus-padding", 	&focusPad,
					 			"interior_focus",	&interiorFocus,
								"default_border",	&pBorder,
								(char *)NULL );

	// Make sure the border values exist, otherwise use some defaults
	if ( pBorder )
	{
		NW_gtk_border_set_from_border( aDefBorder, pBorder );
		gtk_border_free( pBorder );
	}
	else NW_gtk_border_set_from_border( aDefBorder, &aDefDefBorder );

	x = aAreaRect.Left();
	y = aAreaRect.Top();
	w = aAreaRect.GetWidth();
	h = aAreaRect.GetHeight();

	// If the button is too small, don't ever draw focus or grab more space
	if ( (w < 16) || (h < 16) )
		bDrawFocus = FALSE;

	if ( (nState & CTRL_STATE_DEFAULT) && bDrawFocus )
	{
		x -= aDefBorder.left;
		y -= aDefBorder.top;
		w += aDefBorder.left + aDefBorder.right;
		h += aDefBorder.top + aDefBorder.bottom;
	}

    aRect = Rectangle( Point( x, y ), Size( w, h ) );

	return( aRect );
}

//-------------------------------------

BOOL GtkSalGraphics::NWPaintGTKRadio( GdkDrawable* gdkDrawable,
                                      ControlType, ControlPart,
                                      const Rectangle& rControlRectangle,
                                      const clipList& rClipList,
                                      ControlState nState, 
                                      const ImplControlValue& aValue, SalControlHandle&, 
                                      const OUString& )
{
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	BOOL			isChecked = (aValue.getTristateVal()==BUTTONVALUE_ON);
    gint            x, y;
    GdkRectangle	clipRect;
   
	NWEnsureGTKButton( m_nScreen );
	NWEnsureGTKRadio( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

    gint indicator_size;
    gtk_widget_style_get( gWidgetData[m_nScreen].gRadioWidget, "indicator_size", &indicator_size, (char *)NULL);

    x = rControlRectangle.Left() + (rControlRectangle.GetWidth()-indicator_size)/2;
    y = rControlRectangle.Top() + (rControlRectangle.GetHeight()-indicator_size)/2;
    
	// Set the shadow based on if checked or not so we get a freakin checkmark.
	shadowType = isChecked ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
	NWSetWidgetState( gWidgetData[m_nScreen].gRadioWidget, nState, stateType );
	NWSetWidgetState( gWidgetData[m_nScreen].gRadioWidgetSibling, nState, stateType );

	// GTK enforces radio groups, so that if we don't have 2 buttons in the group,
	// the single button will always be active.  So we have to have 2 buttons.
    
    // #i59666# set the members directly where we should use
    // gtk_toggle_button_set_active. reason: there are animated themes
    // which are in active state only after a while leading to painting
    // intermediate states between active/inactive. Let's hope that
    // GtkToggleButtone stays binary compatible.
	if (!isChecked)
		GTK_TOGGLE_BUTTON(gWidgetData[m_nScreen].gRadioWidgetSibling)->active = TRUE;
	GTK_TOGGLE_BUTTON(gWidgetData[m_nScreen].gRadioWidget)->active = isChecked;

    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        gtk_paint_option( gWidgetData[m_nScreen].gRadioWidget->style, gdkDrawable, stateType, shadowType, 
                          &clipRect, gWidgetData[m_nScreen].gRadioWidget, "radiobutton",
                          x, y, indicator_size, indicator_size );
    }
    
	return( TRUE );
}

//-------------------------------------

BOOL GtkSalGraphics::NWPaintGTKCheck( GdkDrawable* gdkDrawable,
                                      ControlType, ControlPart,
                                      const Rectangle& rControlRectangle,
                                      const clipList& rClipList,
                                      ControlState nState, 
                                      const ImplControlValue& aValue,
                                      SalControlHandle&, const OUString& )
{
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	BOOL			isChecked = (aValue.getTristateVal()==BUTTONVALUE_ON) ? TRUE : FALSE;
    GdkRectangle	clipRect;
    gint			x,y;

	NWEnsureGTKButton( m_nScreen );
	NWEnsureGTKCheck( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

    gint indicator_size;
    gtk_widget_style_get( gWidgetData[m_nScreen].gCheckWidget, "indicator_size", &indicator_size, (char *)NULL);

    x = rControlRectangle.Left() + (rControlRectangle.GetWidth()-indicator_size)/2;
    y = rControlRectangle.Top() + (rControlRectangle.GetHeight()-indicator_size)/2;
    
	// Set the shadow based on if checked or not so we get a checkmark.
	shadowType = isChecked ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
	NWSetWidgetState( gWidgetData[m_nScreen].gCheckWidget, nState, stateType );
	GTK_TOGGLE_BUTTON(gWidgetData[m_nScreen].gCheckWidget)->active = isChecked;

    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        gtk_paint_check( gWidgetData[m_nScreen].gCheckWidget->style, gdkDrawable, stateType, shadowType, 
                         &clipRect, gWidgetData[m_nScreen].gCheckWidget, "checkbutton",
                         x, y, indicator_size, indicator_size );
    }

	return( TRUE );
}

//-------------------------------------
static void NWCalcArrowRect( const Rectangle& rButton, Rectangle& rArrow )
{
	// Size the arrow appropriately
    Size aSize( rButton.GetWidth()/2, rButton.GetHeight()/2 );
    rArrow.SetSize( aSize );

	rArrow.SetPos( Point(
        rButton.Left() + ( rButton.GetWidth()  - rArrow.GetWidth()  ) / 2,
        rButton.Top() + ( rButton.GetHeight() - rArrow.GetHeight() ) / 2
        ) );
}

BOOL GtkSalGraphics::NWPaintGTKScrollbar( ControlType, ControlPart nPart,
                                          const Rectangle& rControlRectangle,
                                          const clipList&,
                                          ControlState nState, 
                                          const ImplControlValue& aValue, SalControlHandle&, 
                                          const OUString& )
{
	ScrollbarValue* pScrollbarVal = (ScrollbarValue *)(aValue.getOptionalVal());
    GdkPixmap*      pixmap = NULL;
	Rectangle		pixmapRect, scrollbarRect;
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	GtkScrollbar *	scrollbarWidget;
	GtkStyle *	style;
	GtkAdjustment* scrollbarValues = NULL;
	GtkOrientation	scrollbarOrientation;
	Rectangle		thumbRect = pScrollbarVal->maThumbRect;
	Rectangle		button11BoundRect = pScrollbarVal->maButton1Rect;   // backward
	Rectangle		button22BoundRect = pScrollbarVal->maButton2Rect;   // forward
	Rectangle		button12BoundRect = pScrollbarVal->maButton1Rect;   // secondary forward
	Rectangle		button21BoundRect = pScrollbarVal->maButton2Rect;   // secondary backward
	GtkArrowType	button1Type;                                        // backward
	GtkArrowType	button2Type;                                        // forward
	gchar *		scrollbarTagH = (gchar *) "hscrollbar";
	gchar *		scrollbarTagV = (gchar *) "vscrollbar";
	gchar *		scrollbarTag = NULL;
	Rectangle		arrowRect;
	gint			slider_width = 0;
	gint			stepper_size = 0;
	gint			stepper_spacing = 0;
	gint			trough_border = 0;
	gint			min_slider_length = 0;
	gint			vShim = 0;
	gint			hShim = 0;
	gint			x,y,w,h;
    
    // make controlvalue rectangles relative to area
    thumbRect.Move( -rControlRectangle.Left(), -rControlRectangle.Top() );
    button11BoundRect.Move( -rControlRectangle.Left(), -rControlRectangle.Top() );
    button22BoundRect.Move( -rControlRectangle.Left(), -rControlRectangle.Top() );
    button12BoundRect.Move( -rControlRectangle.Left(), -rControlRectangle.Top() );
    button21BoundRect.Move( -rControlRectangle.Left(), -rControlRectangle.Top() );

	NWEnsureGTKButton( m_nScreen );
	NWEnsureGTKScrollbars( m_nScreen );
	NWEnsureGTKArrow( m_nScreen );
 
	// Find the overall bounding rect of the control
	pixmapRect = rControlRectangle;
    pixmapRect.SetSize( Size( pixmapRect.GetWidth() + 1,
                              pixmapRect.GetHeight() + 1 ) );
	scrollbarRect = pixmapRect;

	if ( (scrollbarRect.GetWidth() <= 1) || (scrollbarRect.GetHeight() <= 1) )
		return( TRUE );

	// Grab some button style attributes
	gtk_widget_style_get( gWidgetData[m_nScreen].gScrollHorizWidget,
                                      "slider_width", &slider_width,
									  "stepper_size", &stepper_size,
									  "trough_border", &trough_border,
									  "stepper_spacing", &stepper_spacing,
									  "min_slider_length", &min_slider_length, (char *)NULL );
    gboolean has_forward;
    gboolean has_forward2;
    gboolean has_backward;
    gboolean has_backward2;
    
	gtk_widget_style_get( gWidgetData[m_nScreen].gScrollHorizWidget, "has-forward-stepper", &has_forward,
									  "has-secondary-forward-stepper", &has_forward2,
									  "has-backward-stepper", &has_backward,
	   								  "has-secondary-backward-stepper", &has_backward2, (char *)NULL );
	gint magic = trough_border ? 1 : 0;
    gint nFirst = 0;

    if ( has_backward )  nFirst  += 1;
    if ( has_forward2 )  nFirst  += 1;

	if ( nPart == PART_DRAW_BACKGROUND_HORZ )
	{
		unsigned int sliderHeight = slider_width + (trough_border * 2);
		vShim = (pixmapRect.GetHeight() - sliderHeight) / 2;

		scrollbarRect.Move( 0, vShim );
		scrollbarRect.SetSize( Size( scrollbarRect.GetWidth(), sliderHeight ) );

		scrollbarWidget = GTK_SCROLLBAR( gWidgetData[m_nScreen].gScrollHorizWidget );
		scrollbarOrientation = GTK_ORIENTATION_HORIZONTAL;
		scrollbarTag = scrollbarTagH;
		button1Type = GTK_ARROW_LEFT;
		button2Type = GTK_ARROW_RIGHT;

        if ( has_backward )
        {
            button12BoundRect.Move( stepper_size - trough_border,
                                    (scrollbarRect.GetHeight() - slider_width) / 2 );
        }

        button11BoundRect.Move( trough_border, (scrollbarRect.GetHeight() - slider_width) / 2 );
        button11BoundRect.SetSize( Size( stepper_size, slider_width ) );
        button12BoundRect.SetSize( Size( stepper_size, slider_width ) );

        if ( has_backward2 )
        {
            button22BoundRect.Move( stepper_size+(trough_border+1)/2, (scrollbarRect.GetHeight() - slider_width) / 2 );
            button21BoundRect.Move( (trough_border+1)/2, (scrollbarRect.GetHeight() - slider_width) / 2 );
        }
        else
        {
            button22BoundRect.Move( (trough_border+1)/2, (scrollbarRect.GetHeight() - slider_width) / 2 );
        }

        button21BoundRect.SetSize( Size( stepper_size, slider_width ) );
        button22BoundRect.SetSize( Size( stepper_size, slider_width ) );

		thumbRect.Bottom() = thumbRect.Top() + slider_width - 1;
		// Make sure the thumb is at least the default width (so we don't get tiny thumbs),
		// but if the VCL gives us a size smaller than the theme's default thumb size,
		// honor the VCL size
#if 0
		if ( (thumbRect.GetWidth() < min_slider_length)
			&& ((scrollbarRect.GetWidth()-button1BoundRect.GetWidth()-button2BoundRect.GetWidth()) > min_slider_length) )
			thumbRect.SetSize( Size( min_slider_length, thumbRect.GetHeight() ) );
#endif

        thumbRect.Right() += magic;
		// Center vertically in the track
		thumbRect.Move( 0, (scrollbarRect.GetHeight() - slider_width) / 2 );
	}
	else
	{
		unsigned int sliderWidth = slider_width + (trough_border * 2);
		hShim = (pixmapRect.GetWidth() - sliderWidth) / 2;

		scrollbarRect.Move( hShim, 0 );
		scrollbarRect.SetSize( Size( sliderWidth, scrollbarRect.GetHeight() ) );

		scrollbarWidget = GTK_SCROLLBAR( gWidgetData[m_nScreen].gScrollVertWidget );
		scrollbarOrientation = GTK_ORIENTATION_VERTICAL;
		scrollbarTag = scrollbarTagV;
		button1Type = GTK_ARROW_UP;
		button2Type = GTK_ARROW_DOWN;

        if ( has_backward )
        {
            button12BoundRect.Move( (scrollbarRect.GetWidth() - slider_width) / 2,
                                    stepper_size + trough_border );
        }
        button11BoundRect.Move( (scrollbarRect.GetWidth() - slider_width) / 2, trough_border );
        button11BoundRect.SetSize( Size( slider_width, stepper_size ) );
		button12BoundRect.SetSize( Size( slider_width, stepper_size ) );

        if ( has_backward2 )
        {
            button22BoundRect.Move( (scrollbarRect.GetWidth() - slider_width) / 2, stepper_size+(trough_border+1)/2 );
            button21BoundRect.Move( (scrollbarRect.GetWidth() - slider_width) / 2, (trough_border+1)/2 );
        }
        else
        {
            button22BoundRect.Move( (scrollbarRect.GetWidth() - slider_width) / 2, (trough_border+1)/2 );
        }

        button21BoundRect.SetSize( Size( slider_width, stepper_size ) );
        button22BoundRect.SetSize( Size( slider_width, stepper_size ) );

		thumbRect.Right() = thumbRect.Left() + slider_width - 1;
#if 0
		// Make sure the thumb is at least the default width (so we don't get tiny thumbs),
		// but if the VCL gives us a size smaller than the theme's default thumb size,
		// honor the VCL size
		if ( (thumbRect.GetHeight() < min_slider_length)
			&& ((scrollbarRect.GetHeight()-button1BoundRect.GetHeight()-button2BoundRect.GetHeight()) > min_slider_length) )
			thumbRect.SetSize( Size( thumbRect.GetWidth(), min_slider_length ) );
#endif

        thumbRect.Bottom() += magic;
		// Center horizontally in the track
		thumbRect.Move( (scrollbarRect.GetWidth() - slider_width) / 2, 0 );
	}

    BOOL has_slider = ( thumbRect.GetWidth() > 0 && thumbRect.GetHeight() > 0 );

	scrollbarValues = gtk_range_get_adjustment( GTK_RANGE(scrollbarWidget) );
	if ( scrollbarValues == NULL )
		scrollbarValues = GTK_ADJUSTMENT( gtk_adjustment_new(0, 0, 0, 0, 0, 0) );
	if ( nPart == PART_DRAW_BACKGROUND_HORZ )
	{
		scrollbarValues->lower = pScrollbarVal->mnMin;
		scrollbarValues->upper = pScrollbarVal->mnMax;
		scrollbarValues->value = pScrollbarVal->mnCur;
		scrollbarValues->page_size = scrollbarRect.GetWidth() / 2;
	}
	else
	{
		scrollbarValues->lower = pScrollbarVal->mnMin;
		scrollbarValues->upper = pScrollbarVal->mnMax;
		scrollbarValues->value = pScrollbarVal->mnCur;
		scrollbarValues->page_size = scrollbarRect.GetHeight() / 2;
	}
	gtk_adjustment_changed( scrollbarValues );
    
    // as multiple paints are required for the scrollbar
    // painting them directly to the window flickers
    pixmap = NWGetPixmapFromScreen( pixmapRect );
    if( ! pixmap )
        return FALSE;
    x = y = 0;
    
    w = pixmapRect.GetWidth();
    h = pixmapRect.GetHeight();    

    GdkDrawable* const &gdkDrawable = GDK_DRAWABLE( pixmap );
    GdkRectangle* gdkRect = NULL; 

	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );
	NWSetWidgetState( GTK_WIDGET(scrollbarWidget), nState, stateType );
	NWSetWidgetState( gWidgetData[m_nScreen].gBtnWidget, nState, stateType );
	style = GTK_WIDGET( scrollbarWidget )->style;

	// ----------------- TROUGH
	gtk_paint_flat_box( gWidgetData[m_nScreen].gBtnWidget->style, gdkDrawable, 
                        GTK_STATE_NORMAL, GTK_SHADOW_NONE, gdkRect,
                        m_pWindow, "base", x, y, 
                        w, h );
	gtk_paint_box( style, gdkDrawable, GTK_STATE_ACTIVE, GTK_SHADOW_IN, 
                   gdkRect, GTK_WIDGET(scrollbarWidget), "trough",
                   x, y,
                   scrollbarRect.GetWidth(), scrollbarRect.GetHeight() );

	if ( nState & CTRL_STATE_FOCUSED )
	{
		gtk_paint_focus( style, gdkDrawable, GTK_STATE_ACTIVE, 
                         gdkRect, GTK_WIDGET(scrollbarWidget), "trough",
                         x, y,
                         scrollbarRect.GetWidth(), scrollbarRect.GetHeight() );
	}

	// ----------------- THUMB
    if ( has_slider )
    {
	    NWConvertVCLStateToGTKState( pScrollbarVal->mnThumbState, &stateType, &shadowType );
	    if ( pScrollbarVal->mnThumbState & CTRL_STATE_PRESSED )  stateType = GTK_STATE_PRELIGHT;
	    gtk_paint_slider( style, gdkDrawable, stateType, GTK_SHADOW_OUT, 
                        gdkRect, GTK_WIDGET(scrollbarWidget), "slider",
                        x+hShim+thumbRect.Left(), y+vShim+thumbRect.Top(), 
                        thumbRect.GetWidth(), thumbRect.GetHeight(), scrollbarOrientation );
    }
	// ----------------- BUTTON 1 //
	if ( has_backward )
	{
        NWConvertVCLStateToGTKState( pScrollbarVal->mnButton1State, &stateType, &shadowType );
        if ( stateType == GTK_STATE_INSENSITIVE )	stateType = GTK_STATE_NORMAL;
        gtk_paint_box( style, gdkDrawable, stateType, shadowType, 
                       gdkRect, GTK_WIDGET(scrollbarWidget), "stepper",
                       x+hShim+button11BoundRect.Left(), y+vShim+button11BoundRect.Top(), 
                       button11BoundRect.GetWidth(), button11BoundRect.GetHeight() );
        // ----------------- ARROW 1
    	NWCalcArrowRect( button11BoundRect, arrowRect );
    	gtk_paint_arrow( style, gdkDrawable, stateType, shadowType, 
                         gdkRect, GTK_WIDGET(scrollbarWidget), scrollbarTag, button1Type, TRUE,
                         x+hShim+arrowRect.Left(), y+vShim+arrowRect.Top(), 
                         arrowRect.GetWidth(), arrowRect.GetHeight() );
    }
	if ( has_forward2 )
	{
        NWConvertVCLStateToGTKState( pScrollbarVal->mnButton2State, &stateType, &shadowType );
        if ( stateType == GTK_STATE_INSENSITIVE )	stateType = GTK_STATE_NORMAL;
        gtk_paint_box( style, gdkDrawable, stateType, shadowType, 
                       gdkRect, GTK_WIDGET(scrollbarWidget), "stepper",
                       x+hShim+button12BoundRect.Left(), y+vShim+button12BoundRect.Top(), 
                       button12BoundRect.GetWidth(), button12BoundRect.GetHeight() );
        // ----------------- ARROW 1
    	NWCalcArrowRect( button12BoundRect, arrowRect );
    	gtk_paint_arrow( style, gdkDrawable, stateType, shadowType, 
                         gdkRect, GTK_WIDGET(scrollbarWidget), scrollbarTag, button2Type, TRUE,
                         x+hShim+arrowRect.Left(), y+vShim+arrowRect.Top(), 
                         arrowRect.GetWidth(), arrowRect.GetHeight() );
    }
	// ----------------- BUTTON 2
    if ( has_backward2 )
    {
        NWConvertVCLStateToGTKState( pScrollbarVal->mnButton1State, &stateType, &shadowType );
        if ( stateType == GTK_STATE_INSENSITIVE )	stateType = GTK_STATE_NORMAL;
        gtk_paint_box( style, gdkDrawable, stateType, shadowType, gdkRect, 
                       GTK_WIDGET(scrollbarWidget), "stepper",
                       x+hShim+button21BoundRect.Left(), y+vShim+button21BoundRect.Top(), 
                       button21BoundRect.GetWidth(), button21BoundRect.GetHeight() );
    	// ----------------- ARROW 2
    	NWCalcArrowRect( button21BoundRect, arrowRect );
        gtk_paint_arrow( style, gdkDrawable, stateType, shadowType, 
                         gdkRect, GTK_WIDGET(scrollbarWidget), scrollbarTag, button1Type, TRUE,
                         x+hShim+arrowRect.Left(), y+vShim+arrowRect.Top(), 
                         arrowRect.GetWidth(), arrowRect.GetHeight() );
    }
    if ( has_forward )
    {
        NWConvertVCLStateToGTKState( pScrollbarVal->mnButton2State, &stateType, &shadowType );
        if ( stateType == GTK_STATE_INSENSITIVE )	stateType = GTK_STATE_NORMAL;
        gtk_paint_box( style, gdkDrawable, stateType, shadowType, gdkRect, 
                       GTK_WIDGET(scrollbarWidget), "stepper",
                       x+hShim+button22BoundRect.Left(), y+vShim+button22BoundRect.Top(), 
                       button22BoundRect.GetWidth(), button22BoundRect.GetHeight() );
    	// ----------------- ARROW 2
    	NWCalcArrowRect( button22BoundRect, arrowRect );
        gtk_paint_arrow( style, gdkDrawable, stateType, shadowType, 
                         gdkRect, GTK_WIDGET(scrollbarWidget), scrollbarTag, button2Type, TRUE,
                         x+hShim+arrowRect.Left(), y+vShim+arrowRect.Top(), 
                         arrowRect.GetWidth(), arrowRect.GetHeight() );
    }

    if( !NWRenderPixmapToScreen(pixmap, pixmapRect) )
    {
        g_object_unref( pixmap );
        return( FALSE );
    }
    g_object_unref( pixmap );
 
	return( TRUE );
}

//---

static Rectangle NWGetScrollButtonRect(	int nScreen, ControlPart nPart, Rectangle aAreaRect )
{
    gint slider_width;
    gint stepper_size;
    gint stepper_spacing;
    gint trough_border;

    NWEnsureGTKScrollbars( nScreen );

	// Grab some button style attributes
	gtk_widget_style_get( gWidgetData[nScreen].gScrollHorizWidget,
                                      "slider-width", &slider_width,
									  "stepper-size", &stepper_size,
									  "trough-border", &trough_border,
	   								  "stepper-spacing", &stepper_spacing, (char *)NULL );

    gboolean has_forward;
    gboolean has_forward2;
    gboolean has_backward;
    gboolean has_backward2;

	gtk_widget_style_get( gWidgetData[nScreen].gScrollHorizWidget,
                                      "has-forward-stepper", &has_forward,
									  "has-secondary-forward-stepper", &has_forward2,
									  "has-backward-stepper", &has_backward,
	   								  "has-secondary-backward-stepper", &has_backward2, (char *)NULL );
	gint       buttonWidth;
	gint       buttonHeight;
	Rectangle  buttonRect;

    gint nFirst = 0;
    gint nSecond = 0;
    
    if ( has_forward )   nSecond += 1;
    if ( has_forward2 )  nFirst  += 1;
    if ( has_backward )  nFirst  += 1;
    if ( has_backward2 ) nSecond += 1;

    if ( ( nPart == PART_BUTTON_UP ) || ( nPart == PART_BUTTON_DOWN ) )
    {
        buttonWidth = slider_width + 2 * trough_border;
        buttonHeight = stepper_size + trough_border + stepper_spacing;
    }
    else
    {
        buttonWidth = stepper_size + trough_border + stepper_spacing;
        buttonHeight = slider_width + 2 * trough_border;
    }
 
    if ( nPart == PART_BUTTON_UP )
    {
        buttonHeight *= nFirst;
        buttonHeight -= 1;
        buttonRect.setX( aAreaRect.Left() );
        buttonRect.setY( aAreaRect.Top() );
    }
    else if ( nPart == PART_BUTTON_LEFT )
    {
        buttonWidth *= nFirst;
        buttonWidth -= 1;
        buttonRect.setX( aAreaRect.Left() );
        buttonRect.setY( aAreaRect.Top() );
    }
    else if ( nPart == PART_BUTTON_DOWN )
    {
        buttonHeight *= nSecond;
        buttonRect.setX( aAreaRect.Left() );
        buttonRect.setY( aAreaRect.Top() + aAreaRect.GetHeight() - buttonHeight );
    }
    else if ( nPart == PART_BUTTON_RIGHT )
    {
        buttonWidth *= nSecond;
        buttonRect.setX( aAreaRect.Left() + aAreaRect.GetWidth() - buttonWidth );
        buttonRect.setY( aAreaRect.Top() );
    }

    buttonRect.SetSize( Size( buttonWidth, buttonHeight ) );

	return( buttonRect );
}

//-------------------------------------

BOOL GtkSalGraphics::NWPaintGTKEditBox( GdkDrawable* gdkDrawable,
                                        ControlType nType, ControlPart nPart,
                                        const Rectangle& rControlRectangle,
                                        const clipList& rClipList,
                                        ControlState nState, 
                                        const ImplControlValue& aValue, SalControlHandle& rControlHandle, 
                                        const OUString& rCaption )
{
	Rectangle		pixmapRect;
    GdkRectangle    clipRect;
    
	// Find the overall bounding rect of the buttons's drawing area,
	// plus its actual draw rect excluding adornment
	pixmapRect = NWGetEditBoxPixmapRect( m_nScreen, nType, nPart, rControlRectangle,
					                     nState, aValue, rControlHandle, rCaption );
    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        NWPaintOneEditBox( m_nScreen, gdkDrawable, &clipRect, nType, nPart, pixmapRect, nState, aValue, rControlHandle, rCaption );
    }

	return( TRUE );
}


/* Take interior/exterior focus into account and return
 * the bounding rectangle of the edit box including
 * any focus requirements.
 */
static Rectangle NWGetEditBoxPixmapRect(int nScreen,
                                        ControlType,
								        ControlPart,
                                        Rectangle aAreaRect,
                                        ControlState,
                                        const ImplControlValue&,
                                        SalControlHandle&,
                                        const OUString& )
{
	Rectangle		pixmapRect = aAreaRect;
	gboolean		interiorFocus;
	gint			focusWidth;

	NWEnsureGTKEditBox( nScreen );

	// Grab some entry style attributes
	gtk_widget_style_get( gWidgetData[nScreen].gEditBoxWidget,
                                	"focus-line-width",	&focusWidth,
					 				"interior-focus",	&interiorFocus, (char *)NULL );

	if ( !interiorFocus )
	{
		pixmapRect.Move( -(focusWidth), -(focusWidth) );
        pixmapRect.SetSize( Size( pixmapRect.GetWidth() + (2*(focusWidth)),
                                  pixmapRect.GetHeight() + (2*(focusWidth)) ) );
	}

	return( pixmapRect );
}


/* Paint a GTK Entry widget into the specified GdkPixmap.
 * All coordinates should be local to the Pixmap, NOT
 * screen/window coordinates.
 */
static void NWPaintOneEditBox(	int nScreen,
                                GdkDrawable * gdkDrawable,
                                GdkRectangle *	gdkRect,
                                ControlType			nType,
                                ControlPart,
                                Rectangle				aEditBoxRect,
                                ControlState			nState,
                                const ImplControlValue&,
                                SalControlHandle&,
                                const OUString& )
{
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	GtkWidget      *widget;

	NWEnsureGTKButton( nScreen );
	NWEnsureGTKEditBox( nScreen );
	NWEnsureGTKSpinButton( nScreen );
	NWEnsureGTKCombo( nScreen );
	NWEnsureGTKScrolledWindow( nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

    /* border's shadowType for gtk entries is always GTK_SHADOW_IN (see gtkentry.c)
    shadowType = GTK_SHADOW_IN;
    */
    
	switch ( nType )
	{
		case CTRL_SPINBOX:
			widget = gWidgetData[nScreen].gSpinButtonWidget;
			break;

        case CTRL_MULTILINE_EDITBOX:
            widget = gWidgetData[nScreen].gScrolledWindowWidget;
            break;
		case CTRL_COMBOBOX:
			widget = GTK_COMBO(gWidgetData[nScreen].gComboWidget)->entry;
			break;

		default:
			widget = gWidgetData[nScreen].gEditBoxWidget;
			break;
	}

	if ( stateType == GTK_STATE_PRELIGHT )
		stateType = GTK_STATE_NORMAL;

	// Blueprint needs to paint entry_bg with a Button widget, not an Entry widget to get
	// a nice white (or whatever default color) background
    GtkWidget* pBGWidget = widget;
    if( GtkSalGraphics::bNeedButtonStyleAsEditBackgroundWorkaround )
    {
        NWSetWidgetState( gWidgetData[nScreen].gBtnWidget, nState, stateType );
        pBGWidget = gWidgetData[nScreen].gBtnWidget;
    }
	NWSetWidgetState( widget, nState, stateType );

	gtk_paint_flat_box( pBGWidget->style, gdkDrawable, stateType, GTK_SHADOW_NONE, 
                        gdkRect, pBGWidget, "entry_bg",
                        aEditBoxRect.Left(), aEditBoxRect.Top(), 
                        aEditBoxRect.GetWidth(), aEditBoxRect.GetHeight() );
	gtk_paint_shadow( widget->style, gdkDrawable, GTK_STATE_NORMAL, GTK_SHADOW_IN, 
                      gdkRect, widget, "entry",
                      aEditBoxRect.Left(), aEditBoxRect.Top(), 
                      aEditBoxRect.GetWidth(), aEditBoxRect.GetHeight() );

}



//-------------------------------------

BOOL GtkSalGraphics::NWPaintGTKSpinBox( ControlType nType, ControlPart nPart,
                                        const Rectangle& rControlRectangle,
                                        const clipList&,
                                        ControlState nState, 
                                        const ImplControlValue& aValue,
                                        SalControlHandle& rControlHandle, const OUString& rCaption )
{
	GdkPixmap	*		pixmap;
	Rectangle			pixmapRect;
	GtkStateType		stateType;
	GtkShadowType		shadowType;
	SpinbuttonValue *	pSpinVal = (SpinbuttonValue *)(aValue.getOptionalVal());
	Rectangle			upBtnRect;
	ControlPart		upBtnPart = PART_BUTTON_UP;
	ControlState		upBtnState = CTRL_STATE_ENABLED;
	Rectangle			downBtnRect;
	ControlPart		downBtnPart = PART_BUTTON_DOWN;
	ControlState		downBtnState = CTRL_STATE_ENABLED;

	NWEnsureGTKButton( m_nScreen );
	NWEnsureGTKSpinButton( m_nScreen );
	NWEnsureGTKArrow( m_nScreen );

	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

	if ( pSpinVal )
	{
		upBtnPart = pSpinVal->mnUpperPart;
		upBtnState = pSpinVal->mnUpperState;

		downBtnPart = pSpinVal->mnLowerPart;
		downBtnState = pSpinVal->mnLowerState;
	}

	// CTRL_SPINBUTTONS pass their area in pSpinVal, not in rControlRectangle
	if ( nType == CTRL_SPINBUTTONS )
	{
		if ( !pSpinVal )
		{
            std::fprintf( stderr, "Tried to draw CTRL_SPINBUTTONS, but the SpinButtons data structure didn't exist!\n" );
			return( false );
		}
		pixmapRect = pSpinVal->maUpperRect;
		pixmapRect.Union( pSpinVal->maLowerRect );
	}
	else
		pixmapRect = rControlRectangle;


	pixmap = NWGetPixmapFromScreen( pixmapRect );
	if ( !pixmap )
		return( FALSE );

	upBtnRect = NWGetSpinButtonRect( m_nScreen, nType, upBtnPart, pixmapRect, upBtnState, aValue, rControlHandle, rCaption );
	downBtnRect = NWGetSpinButtonRect( m_nScreen, nType, downBtnPart, pixmapRect, downBtnState, aValue, rControlHandle, rCaption );

	if ( (nType==CTRL_SPINBOX) && (nPart!=PART_ALL_BUTTONS) )
	{
		// Draw an edit field for SpinBoxes and ComboBoxes
		Rectangle aEditBoxRect( pixmapRect );
		aEditBoxRect.SetSize( Size( upBtnRect.Left() - pixmapRect.Left(), aEditBoxRect.GetHeight() ) );
		aEditBoxRect.setX( 0 );
		aEditBoxRect.setY( 0 );

		NWPaintOneEditBox( m_nScreen, pixmap, NULL, nType, nPart, aEditBoxRect, nState, aValue, rControlHandle, rCaption );
	}

	NWSetWidgetState( gWidgetData[m_nScreen].gSpinButtonWidget, nState, stateType );
	gtk_widget_style_get( gWidgetData[m_nScreen].gSpinButtonWidget, "shadow_type", &shadowType, (char *)NULL );

	if ( shadowType != GTK_SHADOW_NONE )
	{
		Rectangle		shadowRect( upBtnRect );

		shadowRect.Union( downBtnRect );
		gtk_paint_box( gWidgetData[m_nScreen].gSpinButtonWidget->style, pixmap, GTK_STATE_NORMAL, shadowType, NULL,
			gWidgetData[m_nScreen].gSpinButtonWidget, "spinbutton",
			(shadowRect.Left() - pixmapRect.Left()), (shadowRect.Top() - pixmapRect.Top()),
			shadowRect.GetWidth(), shadowRect.GetHeight() );
	}

	NWPaintOneSpinButton( m_nScreen, pixmap, nType, upBtnPart, pixmapRect, upBtnState, aValue, rControlHandle, rCaption );
	NWPaintOneSpinButton( m_nScreen, pixmap, nType, downBtnPart, pixmapRect, downBtnState, aValue, rControlHandle, rCaption );

	if( !NWRenderPixmapToScreen(pixmap, pixmapRect) )
	{
		g_object_unref( pixmap );
		return( FALSE );
	}

	g_object_unref( pixmap );
	return( TRUE );
}

//---

static Rectangle NWGetSpinButtonRect( int nScreen,
                                      ControlType,
								      ControlPart			nPart,
                                      Rectangle 			aAreaRect,
                                      ControlState,
                                      const ImplControlValue&,
                                      SalControlHandle&,
                                      const OUString& )
{
	gint			buttonSize;
	Rectangle		buttonRect;

	NWEnsureGTKSpinButton( nScreen );

	buttonSize = MAX( PANGO_PIXELS( pango_font_description_get_size(GTK_WIDGET(gWidgetData[nScreen].gSpinButtonWidget)->style->font_desc) ),
				   MIN_SPIN_ARROW_WIDTH );
	buttonSize -= buttonSize % 2 - 1; /* force odd */
	buttonRect.SetSize( Size( buttonSize + 2 * gWidgetData[nScreen].gSpinButtonWidget->style->xthickness,
                              buttonRect.GetHeight() ) );
	buttonRect.setX( aAreaRect.Left() + (aAreaRect.GetWidth() - buttonRect.GetWidth()) );
	if ( nPart == PART_BUTTON_UP )
	{
		buttonRect.setY( aAreaRect.Top() );
		buttonRect.Bottom() = buttonRect.Top() + (aAreaRect.GetHeight() / 2);
	}
	else if( nPart == PART_BUTTON_DOWN )
	{
		buttonRect.setY( aAreaRect.Top() + (aAreaRect.GetHeight() / 2) );
		buttonRect.Bottom() = aAreaRect.Bottom(); // cover area completely
	}
    else
    {
        buttonRect.Right()  = buttonRect.Left()-1;
        buttonRect.Left()   = aAreaRect.Left();
        buttonRect.Top()    = aAreaRect.Top();
        buttonRect.Bottom() = aAreaRect.Bottom();
    }

	return( buttonRect );
}

//---

static void NWPaintOneSpinButton( int nScreen,
                                  GdkPixmap*			pixmap,
                                  ControlType			nType,
                                  ControlPart			nPart,
                                  Rectangle				aAreaRect,
                                  ControlState			nState,
                                  const ImplControlValue&	aValue,
                                  SalControlHandle&		rControlHandle,
                                  const OUString&				rCaption )
{
	Rectangle			buttonRect;
	GtkStateType		stateType;
	GtkShadowType		shadowType;
	Rectangle			arrowRect;
	gint				arrowSize;

	NWEnsureGTKSpinButton( nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

	buttonRect = NWGetSpinButtonRect( nScreen, nType, nPart, aAreaRect, nState, aValue, rControlHandle, rCaption );

	NWSetWidgetState( gWidgetData[nScreen].gSpinButtonWidget, nState, stateType );
	gtk_paint_box( gWidgetData[nScreen].gSpinButtonWidget->style, pixmap, stateType, shadowType, NULL, gWidgetData[nScreen].gSpinButtonWidget,
			(nPart == PART_BUTTON_UP) ? "spinbutton_up" : "spinbutton_down",
			(buttonRect.Left() - aAreaRect.Left()), (buttonRect.Top() - aAreaRect.Top()),
			buttonRect.GetWidth(), buttonRect.GetHeight() );

	arrowSize = (buttonRect.GetWidth() - (2 * gWidgetData[nScreen].gSpinButtonWidget->style->xthickness)) - 4;
	arrowSize -= arrowSize % 2 - 1; /* force odd */
    arrowRect.SetSize( Size( arrowSize, arrowSize ) );
	arrowRect.setX( buttonRect.Left() + (buttonRect.GetWidth() - arrowRect.GetWidth()) / 2 );
	if ( nPart == PART_BUTTON_UP )
		arrowRect.setY( buttonRect.Top() + (buttonRect.GetHeight() - arrowRect.GetHeight()) / 2 + 1);
	else
		arrowRect.setY( buttonRect.Top() + (buttonRect.GetHeight() - arrowRect.GetHeight()) / 2 - 1);

	gtk_paint_arrow( gWidgetData[nScreen].gSpinButtonWidget->style, pixmap, stateType, GTK_SHADOW_OUT, NULL, gWidgetData[nScreen].gSpinButtonWidget,
			"spinbutton", (nPart == PART_BUTTON_UP) ? GTK_ARROW_UP : GTK_ARROW_DOWN, TRUE,
			(arrowRect.Left() - aAreaRect.Left()), (arrowRect.Top() - aAreaRect.Top()),
			arrowRect.GetWidth(), arrowRect.GetHeight() );
}


//-------------------------------------

BOOL GtkSalGraphics::NWPaintGTKComboBox( GdkDrawable* gdkDrawable,
                                         ControlType nType, ControlPart nPart,
                                         const Rectangle& rControlRectangle,
                                         const clipList& rClipList,
                                         ControlState nState, 
                                         const ImplControlValue& aValue,
                                         SalControlHandle& rControlHandle, const OUString& rCaption )
{
	Rectangle		pixmapRect;
	Rectangle		buttonRect;
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	Rectangle		arrowRect;
    gint			x,y;
    GdkRectangle	clipRect;

	NWEnsureGTKButton( m_nScreen );
	NWEnsureGTKArrow( m_nScreen );
	NWEnsureGTKCombo( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

	// Find the overall bounding rect of the buttons's drawing area,
	// plus its actual draw rect excluding adornment
	pixmapRect = rControlRectangle;
    x = rControlRectangle.Left();
    y = rControlRectangle.Top();

	NWSetWidgetState( gWidgetData[m_nScreen].gBtnWidget, nState, stateType );
	NWSetWidgetState( gWidgetData[m_nScreen].gComboWidget, nState, stateType );
	NWSetWidgetState( gWidgetData[m_nScreen].gArrowWidget, nState, stateType );

    buttonRect = NWGetComboBoxButtonRect( m_nScreen, nType, PART_BUTTON_DOWN, pixmapRect, nState, aValue, rControlHandle, rCaption );
    if( nPart == PART_BUTTON_DOWN )
        buttonRect.Left() += 1;

	Rectangle		aEditBoxRect( pixmapRect );
	aEditBoxRect.SetSize( Size( pixmapRect.GetWidth() - buttonRect.GetWidth(), aEditBoxRect.GetHeight() ) );

    #define ARROW_EXTENT		0.7
	arrowRect.SetSize( Size( (gint)(MIN_ARROW_SIZE * ARROW_EXTENT),
	                         (gint)(MIN_ARROW_SIZE * ARROW_EXTENT) ) );
	arrowRect.SetPos( Point( buttonRect.Left() + (gint)((buttonRect.GetWidth() - arrowRect.GetWidth()) / 2),
                             buttonRect.Top() + (gint)((buttonRect.GetHeight() - arrowRect.GetHeight()) / 2) ) );

    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        if( nPart == PART_ENTIRE_CONTROL )
            NWPaintOneEditBox( m_nScreen, gdkDrawable, &clipRect, nType, nPart, aEditBoxRect, 
                               nState, aValue, rControlHandle, rCaption );
        
        // Buttons must paint opaque since some themes have alpha-channel enabled buttons
        gtk_paint_flat_box( gWidgetData[m_nScreen].gBtnWidget->style, gdkDrawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE, 
                            &clipRect, m_pWindow, "base", 
                            x+(buttonRect.Left() - pixmapRect.Left()), 
                            y+(buttonRect.Top() - pixmapRect.Top()),
                            buttonRect.GetWidth(), buttonRect.GetHeight() );
        gtk_paint_box( GTK_COMBO(gWidgetData[m_nScreen].gComboWidget)->button->style, gdkDrawable, stateType, shadowType,
                       &clipRect, GTK_COMBO(gWidgetData[m_nScreen].gComboWidget)->button, "button",
                       x+(buttonRect.Left() - pixmapRect.Left()), 
                       y+(buttonRect.Top() - pixmapRect.Top()),
                       buttonRect.GetWidth(), buttonRect.GetHeight() );

        gtk_paint_arrow( gWidgetData[m_nScreen].gArrowWidget->style, gdkDrawable, stateType, shadowType,
                         &clipRect, gWidgetData[m_nScreen].gArrowWidget, "arrow", GTK_ARROW_DOWN, TRUE,
                         x+(arrowRect.Left() - pixmapRect.Left()), y+(arrowRect.Top() - pixmapRect.Top()),
                         arrowRect.GetWidth(), arrowRect.GetHeight() );
    }
    
	return( TRUE );
}

//----

static Rectangle NWGetComboBoxButtonRect( int nScreen,
                                          ControlType,
									      ControlPart nPart,
                                          Rectangle				aAreaRect,
                                          ControlState,
                                          const ImplControlValue&,
                                          SalControlHandle&,
                                          const OUString& )
{
	Rectangle	aButtonRect;
	gint		nArrowWidth;
    gint        nButtonWidth;
	gint		nFocusWidth;
	gint		nFocusPad;

	NWEnsureGTKArrow( nScreen );

	// Grab some button style attributes
	gtk_widget_style_get( gWidgetData[nScreen].gDropdownWidget,
                                  	"focus-line-width",	&nFocusWidth,
									"focus-padding", 	&nFocusPad, (char *)NULL );

	nArrowWidth = MIN_ARROW_SIZE + (GTK_MISC(gWidgetData[nScreen].gArrowWidget)->xpad * 2);
    nButtonWidth = nArrowWidth +
                   ((BTN_CHILD_SPACING + gWidgetData[nScreen].gDropdownWidget->style->xthickness) * 2)
				   + (2 * (nFocusWidth+nFocusPad));
    if( nPart == PART_BUTTON_DOWN )
    {
        aButtonRect.SetSize( Size( nButtonWidth, aAreaRect.GetHeight() ) );
        aButtonRect.SetPos( Point( aAreaRect.Left() + aAreaRect.GetWidth() - nButtonWidth,
                                   aAreaRect.Top() ) );
    }
    else if( nPart == PART_SUB_EDIT )
    {
        NWEnsureGTKCombo( nScreen );
        
        gint adjust_x = GTK_CONTAINER(gWidgetData[nScreen].gComboWidget)->border_width +
                        nFocusWidth +
                        nFocusPad;
        gint adjust_y = adjust_x + gWidgetData[nScreen].gComboWidget->style->ythickness;
        adjust_x     += gWidgetData[nScreen].gComboWidget->style->xthickness;
        aButtonRect.SetSize( Size( aAreaRect.GetWidth() - nButtonWidth - 2 * adjust_x,
                                   aAreaRect.GetHeight() - 2 * adjust_y ) );
        Point aEditPos = aAreaRect.TopLeft();
        aEditPos.X() += adjust_x;
        aEditPos.Y() += adjust_y;
        aButtonRect.SetPos( aEditPos );
    }

	return( aButtonRect );
}

//-------------------------------------


    
BOOL GtkSalGraphics::NWPaintGTKTabItem( ControlType nType, ControlPart,
                                        const Rectangle& rControlRectangle,
                                        const clipList&,
                                        ControlState nState, 
                                        const ImplControlValue& aValue,
                                        SalControlHandle&, const OUString& )
{
	GdkPixmap *	pixmap;
	Rectangle		pixmapRect;
	Rectangle		tabRect;
	TabitemValue *	pTabitemValue = (TabitemValue *)(aValue.getOptionalVal());
	GtkStateType	stateType;
	GtkShadowType	shadowType;
    if( ! gWidgetData[ m_nScreen ].gCacheTabItems )
    {
        gWidgetData[ m_nScreen ].gCacheTabItems = new NWPixmapCache( m_nScreen );
        gWidgetData[ m_nScreen ].gCacheTabPages = new NWPixmapCache( m_nScreen );
    }
    NWPixmapCache& aCacheItems = *gWidgetData[ m_nScreen ].gCacheTabItems;
    NWPixmapCache& aCachePage = *gWidgetData[ m_nScreen ].gCacheTabPages;

    if( !aCacheItems.GetSize() )
        aCacheItems.SetSize( 20 );
    if( !aCachePage.GetSize() )
        aCachePage.SetSize( 1 );
    
	if ( !pTabitemValue && (nType==CTRL_TAB_ITEM) )
	{
        std::fprintf( stderr, "NWPaintGTKTabItem() received a NULL TabitemValue.  Cannot draw native tab\n" );
		return( false );
	}

	NWEnsureGTKButton( m_nScreen );
	NWEnsureGTKNotebook( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

	// Find the overall bounding rect of the buttons's drawing area,
	// plus its actual draw rect excluding adornment
	pixmapRect = rControlRectangle;
	if ( nType == CTRL_TAB_ITEM )
	{
		if ( !pTabitemValue->isFirst() )
		{
			// GTK+ tabs overlap on the right edge (the top tab obscures the
			// left edge of the tab right "below" it, so adjust the rectangle
			// to draw tabs slightly large so the overlap happens
			pixmapRect.Move( -2, 0 );
			pixmapRect.SetSize( Size( pixmapRect.GetWidth() + 2, pixmapRect.GetHeight() ) );
		}
		if ( nState & CTRL_STATE_SELECTED )
		{
			// In GTK+, the selected tab is 2px taller than all other tabs
			pixmapRect.Move( 0, -2 );
			pixmapRect.Bottom() += 2;
			tabRect = pixmapRect;
			// Only draw over 1 pixel of the tab pane that this tab is drawn on top of.
			tabRect.Bottom() -= 1;
		}
		else
			tabRect = pixmapRect;

		// Allow the tab to draw a right border if needed
		tabRect.Right() -= 1;
    
        // #129732# avoid degenerate cases which might lead to crashes
        if( tabRect.GetWidth() <= 1 || tabRect.GetHeight() <= 1 )
            return false;
	}

    if( nType == CTRL_TAB_ITEM )
    {
        if( aCacheItems.Find( nType, nState, pixmapRect, &pixmap ) )
            return NWRenderPixmapToScreen( pixmap, pixmapRect );
    }
    else
    {
        if( aCachePage.Find( nType, nState, pixmapRect, &pixmap ) )
            return NWRenderPixmapToScreen( pixmap, pixmapRect );
    }

        
//	gtk_widget_set_state( gWidgetData[m_nScreen].gNotebookWidget, stateType );

    pixmap = gdk_pixmap_new( NULL, pixmapRect.GetWidth(), pixmapRect.GetHeight(), 
                             GetX11SalData()->GetDisplay()->GetVisual( m_nScreen ).GetDepth() );
    GdkRectangle paintRect;
    paintRect.x = paintRect.y = 0;
    paintRect.width = pixmapRect.GetWidth();
    paintRect.height = pixmapRect.GetHeight();
                             
    gtk_paint_flat_box( m_pWindow->style, pixmap, GTK_STATE_NORMAL, 
		                GTK_SHADOW_NONE, &paintRect, m_pWindow, "base", 0, 0, -1, -1);

	NWSetWidgetState( gWidgetData[m_nScreen].gNotebookWidget, nState, stateType );
    
	switch( nType )
	{
		case CTRL_TAB_BODY:
			break;

		case CTRL_FIXEDBORDER:
		case CTRL_TAB_PANE:
			gtk_paint_box_gap( gWidgetData[m_nScreen].gNotebookWidget->style, pixmap, GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, gWidgetData[m_nScreen].gNotebookWidget,
				(char *)"notebook", 0, 0, pixmapRect.GetWidth(), pixmapRect.GetHeight(), GTK_POS_TOP, 0, 0 );
			break;

		case CTRL_TAB_ITEM:
			stateType = ( nState & CTRL_STATE_SELECTED ) ? GTK_STATE_NORMAL : GTK_STATE_ACTIVE;

			gtk_paint_extension( gWidgetData[m_nScreen].gNotebookWidget->style, pixmap, stateType, GTK_SHADOW_OUT, NULL, gWidgetData[m_nScreen].gNotebookWidget,
				(char *)"tab", (tabRect.Left() - pixmapRect.Left()), (tabRect.Top() - pixmapRect.Top()),
				tabRect.GetWidth(), tabRect.GetHeight(), GTK_POS_BOTTOM );

			if ( nState & CTRL_STATE_SELECTED )
			{
				gtk_paint_flat_box( gWidgetData[m_nScreen].gNotebookWidget->style, pixmap, stateType, GTK_SHADOW_NONE, NULL, m_pWindow,
					(char *)"base", 0, (pixmapRect.GetHeight() - 1), pixmapRect.GetWidth(), 1 );
			}
			break;

		default:
			break;
	}

	// Crux seems to think it can make the pane without a left edge
	if ( nType == CTRL_FIXEDBORDER )
		pixmapRect.Move( 1, 0 );

    // cache data
    if( nType == CTRL_TAB_ITEM )
        aCacheItems.Fill( nType, nState, pixmapRect, pixmap );
    else
        aCachePage.Fill( nType, nState, pixmapRect, pixmap );
        
	BOOL bSuccess = NWRenderPixmapToScreen(pixmap, pixmapRect);
	g_object_unref( pixmap );
	return bSuccess;
}

//-------------------------------------

BOOL GtkSalGraphics::NWPaintGTKListBox( GdkDrawable* gdkDrawable,
                                        ControlType nType, ControlPart nPart,
                                        const Rectangle& rControlRectangle,
                                        const clipList& rClipList,
                                        ControlState nState, 
                                        const ImplControlValue& aValue,
                                        SalControlHandle& rControlHandle, const OUString& rCaption )
{
	Rectangle		pixmapRect;
	Rectangle		widgetRect;
	Rectangle		aIndicatorRect;
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	gint			bInteriorFocus;
	gint			nFocusLineWidth;
	gint			nFocusPadding;
    gint			x,y;
    GdkRectangle    clipRect;

	NWEnsureGTKButton( m_nScreen );
	NWEnsureGTKOptionMenu( m_nScreen );
	NWEnsureGTKScrolledWindow( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

	// Find the overall bounding rect of the buttons's drawing area,
	// plus its actual draw rect excluding adornment
	pixmapRect = rControlRectangle;
	if ( nPart == PART_WINDOW )
	{
		// Make the widget a _bit_ bigger
		pixmapRect.SetPos( Point( pixmapRect.Left() - 1,
                                  pixmapRect.Top() - 1 ) );
		pixmapRect.SetSize( Size( pixmapRect.GetWidth() + 2,
		                          pixmapRect.GetHeight() + 2 ) );
	}

	widgetRect = pixmapRect;
    x = pixmapRect.Left();
    y = pixmapRect.Top();

    // set up references to correct drawable and cliprect
	NWSetWidgetState( gWidgetData[m_nScreen].gBtnWidget, nState, stateType );
	NWSetWidgetState( gWidgetData[m_nScreen].gOptionMenuWidget, nState, stateType );
	NWSetWidgetState( gWidgetData[m_nScreen].gScrolledWindowWidget, nState, stateType );

	if ( nPart != PART_WINDOW )
	{
		gtk_widget_style_get( gWidgetData[m_nScreen].gOptionMenuWidget,
			"interior_focus",	&bInteriorFocus,
			"focus_line_width",	&nFocusLineWidth,
			"focus_padding",	&nFocusPadding,
			(char *)NULL);
    }
    
    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
    
        if ( nPart != PART_WINDOW )
        {
            // Listboxes must paint opaque since some themes have alpha-channel enabled bodies
            gtk_paint_flat_box( gWidgetData[m_nScreen].gBtnWidget->style, gdkDrawable, GTK_STATE_NORMAL, GTK_SHADOW_NONE, 
                                &clipRect, m_pWindow, "base", x, y, 
                                pixmapRect.GetWidth(), pixmapRect.GetHeight() );
            gtk_paint_box( gWidgetData[m_nScreen].gOptionMenuWidget->style, gdkDrawable, stateType, shadowType, &clipRect,
                           gWidgetData[m_nScreen].gOptionMenuWidget, "optionmenu",
                           x+(widgetRect.Left() - pixmapRect.Left()), 
                           y+(widgetRect.Top() - pixmapRect.Top()),
                           widgetRect.GetWidth(), widgetRect.GetHeight() );
            aIndicatorRect = NWGetListBoxIndicatorRect( m_nScreen, nType, nPart, widgetRect, nState, 
                                                        aValue, rControlHandle, rCaption );
            gtk_paint_tab( gWidgetData[m_nScreen].gOptionMenuWidget->style, gdkDrawable, stateType, shadowType, &clipRect,
                           gWidgetData[m_nScreen].gOptionMenuWidget, "optionmenutab",
                           x+(aIndicatorRect.Left() - pixmapRect.Left()), 
                           y+(aIndicatorRect.Top() - pixmapRect.Top()),
                           aIndicatorRect.GetWidth(), aIndicatorRect.GetHeight() );
        }
        else
        {
            shadowType = GTK_SHADOW_IN;
        
            gtk_paint_shadow( gWidgetData[m_nScreen].gScrolledWindowWidget->style, gdkDrawable, GTK_STATE_NORMAL, shadowType,
                &clipRect, gWidgetData[m_nScreen].gScrolledWindowWidget, "scrolled_window",
                x+(widgetRect.Left() - pixmapRect.Left()), y+(widgetRect.Top() - pixmapRect.Top()),
                widgetRect.GetWidth(), widgetRect.GetHeight() );
        }
    }
    
	return( TRUE );
}

BOOL GtkSalGraphics::NWPaintGTKToolbar(
            GdkDrawable* gdkDrawable,
            ControlType, ControlPart nPart,
			const Rectangle& rControlRectangle,
            const clipList& rClipList,
            ControlState nState, const ImplControlValue& aValue,
			SalControlHandle&, const OUString& )
{
	GtkStateType	stateType;
	GtkShadowType	shadowType;
	gint			x, y, w, h;
    gint            g_x=0, g_y=0, g_w=10, g_h=10;
    bool            bPaintButton = true;
    GtkWidget*      pButtonWidget = gWidgetData[m_nScreen].gToolbarButtonWidget;
    const gchar*    pButtonDetail = "button";
    GdkRectangle	clipRect;
    
	NWEnsureGTKToolbar( m_nScreen );
    if( nPart == PART_BUTTON ) // toolbar buttons cannot focus in gtk
        nState &= ~CTRL_STATE_FOCUSED;
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );
    
	x = rControlRectangle.Left();
    y = rControlRectangle.Top();
	w = rControlRectangle.GetWidth();
	h = rControlRectangle.GetHeight();

    // handle toolbar
    if( nPart == PART_DRAW_BACKGROUND_HORZ || nPart == PART_DRAW_BACKGROUND_VERT )
    {
	    NWSetWidgetState( gWidgetData[m_nScreen].gToolbarWidget, nState, stateType );

        GTK_WIDGET_UNSET_FLAGS( gWidgetData[m_nScreen].gToolbarWidget, GTK_SENSITIVE );
        if ( nState & CTRL_STATE_ENABLED )
            GTK_WIDGET_SET_FLAGS( gWidgetData[m_nScreen].gToolbarWidget, GTK_SENSITIVE );

        if( nPart == PART_DRAW_BACKGROUND_HORZ )
            gtk_toolbar_set_orientation( GTK_TOOLBAR(gWidgetData[m_nScreen].gToolbarWidget), GTK_ORIENTATION_HORIZONTAL );
        else
            gtk_toolbar_set_orientation( GTK_TOOLBAR(gWidgetData[m_nScreen].gToolbarWidget), GTK_ORIENTATION_VERTICAL );
    }        
    // handle grip
    else if( nPart == PART_THUMB_HORZ || nPart == PART_THUMB_VERT )
    {
	    NWSetWidgetState( gWidgetData[m_nScreen].gHandleBoxWidget, nState, stateType );

        GTK_WIDGET_UNSET_FLAGS( gWidgetData[m_nScreen].gHandleBoxWidget, GTK_SENSITIVE );
        if ( nState & CTRL_STATE_ENABLED )
            GTK_WIDGET_SET_FLAGS( gWidgetData[m_nScreen].gHandleBoxWidget, GTK_SENSITIVE );

        gtk_handle_box_set_shadow_type( GTK_HANDLE_BOX(gWidgetData[m_nScreen].gHandleBoxWidget), shadowType );

        // evaluate grip rect
        ToolbarValue* pVal = (ToolbarValue*)aValue.getOptionalVal();
        if( pVal )
        {
            g_x = pVal->maGripRect.Left();
            g_y = pVal->maGripRect.Top();
            g_w = pVal->maGripRect.GetWidth();
            g_h = pVal->maGripRect.GetHeight();
        }
    }
    // handle button
    else if( nPart == PART_BUTTON )
    {
        bPaintButton =
            (GTK_BUTTON(pButtonWidget)->relief != GTK_RELIEF_NONE)
            || (nState & CTRL_STATE_PRESSED)
		    || (nState & CTRL_STATE_ROLLOVER); 
        if( aValue.getTristateVal() == BUTTONVALUE_ON )
        {
            pButtonWidget = gWidgetData[m_nScreen].gToolbarToggleWidget;
            shadowType = GTK_SHADOW_IN;
            // special case stateType value for depressed toggle buttons
            // cf. gtk+/gtk/gtktogglebutton.c (gtk_toggle_button_update_state)
            if( ! (nState & (CTRL_STATE_PRESSED|CTRL_STATE_ROLLOVER)) )
                stateType = GTK_STATE_ACTIVE;
            pButtonDetail = "togglebutton";
            bPaintButton = true;
        }
        
        NWSetWidgetState( pButtonWidget, nState, stateType );
        gtk_widget_ensure_style( pButtonWidget );            
    }        
    
    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        // draw toolbar
        if( nPart == PART_DRAW_BACKGROUND_HORZ || nPart == PART_DRAW_BACKGROUND_VERT )
        {
            gtk_paint_flat_box( gWidgetData[m_nScreen].gToolbarWidget->style,
                                gdkDrawable,
                                (GtkStateType)GTK_STATE_NORMAL,
                                GTK_SHADOW_NONE, 
                                &clipRect,
                                gWidgetData[m_nScreen].gToolbarWidget,
                                "base",
                                x, y, w, h );
            gtk_paint_box( gWidgetData[m_nScreen].gToolbarWidget->style,
                           gdkDrawable,
                           stateType,
                           shadowType,
                           &clipRect,
                           gWidgetData[m_nScreen].gToolbarWidget,
                           "toolbar",
                           x, y, w, h );
        }
        // draw grip
        else if( nPart == PART_THUMB_HORZ || nPart == PART_THUMB_VERT )
        {
            gtk_paint_handle( gWidgetData[m_nScreen].gHandleBoxWidget->style,
                              gdkDrawable,
                              GTK_STATE_NORMAL,
                              GTK_SHADOW_OUT,
                              &clipRect,
                              gWidgetData[m_nScreen].gHandleBoxWidget,
                              "handlebox",
                              g_x, g_y, g_w, g_h,
                              nPart == PART_THUMB_HORZ ?
                              GTK_ORIENTATION_HORIZONTAL :
                              GTK_ORIENTATION_VERTICAL
                              );
        }
        // draw button
        else if( nPart == PART_BUTTON )
        {
            if( bPaintButton )
            {
                gtk_paint_box( pButtonWidget->style, gdkDrawable,
                               stateType,
                               shadowType,
                               &clipRect,
                               pButtonWidget, pButtonDetail, x, y, w, h );
            }        
        }
    }
    
	return( TRUE );
}

//----

BOOL GtkSalGraphics::NWPaintGTKMenubar(
            GdkDrawable* gdkDrawable,
            ControlType, ControlPart nPart,
			const Rectangle& rControlRectangle,
            const clipList& rClipList,
            ControlState nState, const ImplControlValue&,
			SalControlHandle&, const OUString& )
{
	GtkStateType	stateType;
	GtkShadowType	shadowType;
    GtkShadowType   selected_shadow_type = GTK_SHADOW_OUT;
	gint			x, y, w, h;
    GdkRectangle	clipRect;
    
	NWEnsureGTKMenubar( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );
    
	x = rControlRectangle.Left();
    y = rControlRectangle.Top();
	w = rControlRectangle.GetWidth();
	h = rControlRectangle.GetHeight();

    if( nPart == PART_MENU_ITEM )
    {
        if( nState & (CTRL_STATE_SELECTED|CTRL_STATE_ROLLOVER) )
        {
            gtk_widget_style_get( gWidgetData[m_nScreen].gMenuItemMenubarWidget,
                                  "selected_shadow_type", &selected_shadow_type,
                                  (char *)NULL);
        }
    }
    
    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        // handle Menubar
        if( nPart == PART_ENTIRE_CONTROL )
        {
            NWSetWidgetState( gWidgetData[m_nScreen].gMenubarWidget, nState, stateType );
            
            GTK_WIDGET_UNSET_FLAGS( gWidgetData[m_nScreen].gMenubarWidget, GTK_SENSITIVE );
            if ( nState & CTRL_STATE_ENABLED )
                GTK_WIDGET_SET_FLAGS( gWidgetData[m_nScreen].gMenubarWidget, GTK_SENSITIVE );

            // #118704# for translucent menubar styles paint background first
            gtk_paint_flat_box( gWidgetData[m_nScreen].gMenubarWidget->style,
                                gdkDrawable,
                                GTK_STATE_NORMAL,
                                GTK_SHADOW_NONE, 
                                &clipRect,
                                GTK_WIDGET(m_pWindow),
                                "base", 
                                x, y, w, h );
            gtk_paint_box( gWidgetData[m_nScreen].gMenubarWidget->style,
                           gdkDrawable,
                           stateType,
                           shadowType,
                           &clipRect,
                           gWidgetData[m_nScreen].gMenubarWidget,
                           "menubar",
                           x, y, w, h );
        }
        else if( nPart == PART_MENU_ITEM )
        {
            if( nState & (CTRL_STATE_SELECTED|CTRL_STATE_ROLLOVER) )
            {
                gtk_paint_box( gWidgetData[m_nScreen].gMenuItemMenubarWidget->style,
                               gdkDrawable,
                               GTK_STATE_PRELIGHT,
                               selected_shadow_type,
                               &clipRect,
                               gWidgetData[m_nScreen].gMenuItemMenubarWidget,
                               "menuitem",
                               x, y, w, h);
            }
        }
    }
    
	return( TRUE );
}

BOOL GtkSalGraphics::NWPaintGTKPopupMenu(
            GdkDrawable* gdkDrawable,
            ControlType, ControlPart nPart,
			const Rectangle& rControlRectangle,
            const clipList& rClipList,
            ControlState nState, const ImplControlValue&,
			SalControlHandle&, const OUString& )
{
    // #i50745# gtk does not draw disabled menu entries (and crux theme
    // even crashes), draw them using vcl functionality.
    if( nPart == PART_MENU_ITEM && ! (nState & CTRL_STATE_ENABLED) )
        return FALSE;
    
	GtkStateType	stateType;
	GtkShadowType	shadowType;
    GtkShadowType   selected_shadow_type = GTK_SHADOW_OUT;
	gint			x, y, w, h;
    GdkRectangle	clipRect;
    
	NWEnsureGTKMenu( m_nScreen );
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );
    
	x = rControlRectangle.Left();
    y = rControlRectangle.Top();
	w = rControlRectangle.GetWidth();
	h = rControlRectangle.GetHeight();

    if( nPart == PART_MENU_ITEM &&
        ( nState & (CTRL_STATE_SELECTED|CTRL_STATE_ROLLOVER) ) )
    {
        gtk_widget_style_get( gWidgetData[m_nScreen].gMenuItemMenuWidget,
                              "selected_shadow_type", &selected_shadow_type,
                              (char *)NULL);
    }

    NWSetWidgetState( gWidgetData[m_nScreen].gMenuWidget, nState, stateType );

    GTK_WIDGET_UNSET_FLAGS( gWidgetData[m_nScreen].gMenuWidget, GTK_SENSITIVE );
    if ( nState & CTRL_STATE_ENABLED )
        GTK_WIDGET_SET_FLAGS( gWidgetData[m_nScreen].gMenuWidget, GTK_SENSITIVE );

    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        if( nPart == PART_ENTIRE_CONTROL )
        {
            // #118704# for translucent menubar styles paint background first
            gtk_paint_flat_box( gWidgetData[m_nScreen].gMenuWidget->style,
                                gdkDrawable,
                                GTK_STATE_NORMAL,
                                GTK_SHADOW_NONE, 
                                &clipRect,
                                GTK_WIDGET(m_pWindow),
                                "base", 
                                x, y, w, h );
            gtk_paint_box( gWidgetData[m_nScreen].gMenuWidget->style,
                           gdkDrawable,
                           GTK_STATE_NORMAL,
                           GTK_SHADOW_OUT,
                           &clipRect,
                           gWidgetData[m_nScreen].gMenuWidget,
                           "menu",
                           x, y, w, h );
        }
        else if( nPart == PART_MENU_ITEM )
        {
            if( nState & (CTRL_STATE_SELECTED|CTRL_STATE_ROLLOVER) )
            {
                if( nState & CTRL_STATE_ENABLED )
                gtk_paint_box( gWidgetData[m_nScreen].gMenuItemMenuWidget->style,
                               gdkDrawable,
                               GTK_STATE_PRELIGHT,
                               selected_shadow_type,
                               &clipRect,
                               gWidgetData[m_nScreen].gMenuItemMenuWidget,
                               "menuitem",
                               x, y, w, h);
            }
        }
        else if( nPart == PART_MENU_ITEM_CHECK_MARK || nPart == PART_MENU_ITEM_RADIO_MARK )
        {
            GtkWidget* pWidget = (nPart == PART_MENU_ITEM_CHECK_MARK) ?
                                 gWidgetData[m_nScreen].gMenuItemCheckMenuWidget :
                                 gWidgetData[m_nScreen].gMenuItemRadioMenuWidget;

            GtkStateType nStateType = GTK_STATE_NORMAL;
            GtkShadowType nShadowType;

            if ( nState & CTRL_STATE_SELECTED )
                nStateType = GTK_STATE_PRELIGHT;

            NWSetWidgetState( pWidget, nState, nStateType );

            if ( nState & CTRL_STATE_PRESSED )
                nShadowType = GTK_SHADOW_IN;
            else
                nShadowType = GTK_SHADOW_OUT;

            if ( nPart == PART_MENU_ITEM_CHECK_MARK )
            {
                gtk_paint_check( pWidget->style,
                                 gdkDrawable,
                                 nStateType,
                                 nShadowType,
                                 &clipRect,
                                 gWidgetData[m_nScreen].gMenuItemMenuWidget,
                                 "check",
                                 x, y, w, h );
            }
            else
            {
                gtk_paint_option( pWidget->style,
                                  gdkDrawable,
                                  nStateType,
                                  nShadowType,
                                  &clipRect,
                                  gWidgetData[m_nScreen].gMenuItemMenuWidget,
                                  "option",
                                  x, y, w, h );
            }
        }
    }
    
	return( TRUE );
}

BOOL GtkSalGraphics::NWPaintGTKTooltip(
            GdkDrawable* gdkDrawable,
            ControlType, ControlPart,
			const Rectangle& rControlRectangle,
            const clipList& rClipList,
            ControlState, const ImplControlValue&,
			SalControlHandle&, const OUString& )
{
    NWEnsureGTKTooltip( m_nScreen );
    
	gint			x, y, w, h;
    GdkRectangle	clipRect;
    
	x = rControlRectangle.Left();
    y = rControlRectangle.Top();
	w = rControlRectangle.GetWidth();
	h = rControlRectangle.GetHeight();

    for( clipList::const_iterator it = rClipList.begin(); it != rClipList.end(); ++it )
    {
        clipRect.x = it->Left();
        clipRect.y = it->Top();
        clipRect.width = it->GetWidth();
        clipRect.height = it->GetHeight();
        
        gtk_paint_flat_box( gWidgetData[m_nScreen].gTooltipPopup->style,
                            gdkDrawable,
                            GTK_STATE_NORMAL,
                            GTK_SHADOW_OUT,
                            &clipRect,
                            gWidgetData[m_nScreen].gTooltipPopup,
                            "tooltip", 
                            x, y, w, h );
    }
    
	return( TRUE );
}

BOOL GtkSalGraphics::NWPaintGTKListNode(
            GdkDrawable*,
            ControlType, ControlPart,
			const Rectangle& rControlRectangle,
            const clipList&,
            ControlState nState, const ImplControlValue& rValue,
			SalControlHandle&, const OUString& )
{
    NWEnsureGTKTreeView( m_nScreen );

    Rectangle aRect( rControlRectangle );
    aRect.Left() -= 2;
    aRect.Right() += 2;
    aRect.Top() -= 2;
    aRect.Bottom() += 2;
	gint			w, h;
	w = aRect.GetWidth();
	h = aRect.GetHeight();

	GtkStateType	stateType;
	GtkShadowType	shadowType;
	NWConvertVCLStateToGTKState( nState, &stateType, &shadowType );

    ButtonValue aButtonValue = rValue.getTristateVal();
    GtkExpanderStyle eStyle = GTK_EXPANDER_EXPANDED;

    switch( aButtonValue )
    {
        case BUTTONVALUE_ON: eStyle = GTK_EXPANDER_EXPANDED;break;
        case BUTTONVALUE_OFF: eStyle = GTK_EXPANDER_COLLAPSED; break;
        default:
            break;
    }
    
    GdkPixmap* pixmap = NWGetPixmapFromScreen( aRect );
    if( ! pixmap )
        return FALSE;
    
    GdkDrawable* const &pixDrawable = GDK_DRAWABLE( pixmap );
    gtk_paint_expander( gWidgetData[m_nScreen].gTreeView->style,
                        pixDrawable,
                        stateType,
                        NULL,
                        gWidgetData[m_nScreen].gTreeView,
                        "treeview",
                        w/2, h/2,
                        eStyle );

	BOOL bRet = NWRenderPixmapToScreen( pixmap, aRect );
    g_object_unref( pixmap );

	return bRet;
}

BOOL GtkSalGraphics::NWPaintGTKProgress(
            GdkDrawable*,
            ControlType, ControlPart,
			const Rectangle& rControlRectangle,
            const clipList&,
            ControlState, const ImplControlValue& rValue,
			SalControlHandle&, const OUString& )
{
    NWEnsureGTKProgressBar( m_nScreen );
    
	gint			w, h;
	w = rControlRectangle.GetWidth();
	h = rControlRectangle.GetHeight();

    long nProgressWidth = rValue.getNumericVal();
    
    GdkPixmap* pixmap = NWGetPixmapFromScreen( Rectangle( Point( 0, 0 ), Size( w, h ) ) );
    if( ! pixmap )
        return FALSE;
    
    GdkDrawable* const &pixDrawable = GDK_DRAWABLE( pixmap );

    // paint background
    gtk_paint_flat_box( gWidgetData[m_nScreen].gProgressBar->style,
                        pixDrawable,
                        GTK_STATE_NORMAL,
                        GTK_SHADOW_NONE,
                        NULL,
                        gWidgetData[m_nScreen].gProgressBar,
                        "trough", 
                        0, 0, w, h );
    if( nProgressWidth > 0 )
    {
        // paint progress
        if( Application::GetSettings().GetLayoutRTL() )
        {
            gtk_paint_box( gWidgetData[m_nScreen].gProgressBar->style,
                           pixDrawable,
                           GTK_STATE_PRELIGHT, GTK_SHADOW_OUT,
                           NULL,
                           gWidgetData[m_nScreen].gProgressBar,
                           "bar",
                           w-nProgressWidth, 0, nProgressWidth, h
                           );
        }
        else
        {
            gtk_paint_box( gWidgetData[m_nScreen].gProgressBar->style,
                           pixDrawable,
                           GTK_STATE_PRELIGHT, GTK_SHADOW_OUT,
                           NULL,
                           gWidgetData[m_nScreen].gProgressBar,
                           "bar",
                           0, 0, nProgressWidth, h
                           );
        }
    }

	BOOL bRet = NWRenderPixmapToScreen( pixmap, rControlRectangle );
    g_object_unref( pixmap );

	return bRet;
}

//----

static Rectangle NWGetListBoxButtonRect( int nScreen,
                                         ControlType,
                                         ControlPart    nPart,
                                         Rectangle      aAreaRect,
                                         ControlState,
                                         const ImplControlValue&,
                                         SalControlHandle&,
                                         const OUString& )
{
	Rectangle       aPartRect;
	GtkRequisition *pIndicatorSize = NULL;
	GtkBorder      *pIndicatorSpacing = NULL;
	gint            width = 13;	// GTK+ default
	gint            right = 5;	// GTK+ default
	gint            nButtonAreaWidth = 0;
    gint            xthickness = 0;

	NWEnsureGTKOptionMenu( nScreen );

	gtk_widget_style_get( gWidgetData[nScreen].gOptionMenuWidget,
			"indicator_size",	&pIndicatorSize,
			"indicator_spacing",&pIndicatorSpacing, (char *)NULL);
	
	if ( pIndicatorSize )
		width = pIndicatorSize->width;

	if ( pIndicatorSpacing )
		right = pIndicatorSpacing->right;

    Size aPartSize( 0, aAreaRect.GetHeight() );
    Point aPartPos ( 0, aAreaRect.Top() );

    xthickness = gWidgetData[nScreen].gOptionMenuWidget->style->xthickness;
	nButtonAreaWidth = width + right + (xthickness * 2);
	switch( nPart )
	{
		case PART_BUTTON_DOWN:
			aPartSize.Width() = nButtonAreaWidth;
			aPartPos.X() = aAreaRect.Left() + aAreaRect.GetWidth() - aPartSize.Width();
			break;

		case PART_SUB_EDIT:
			aPartSize.Width() = aAreaRect.GetWidth() - nButtonAreaWidth - xthickness;
			aPartPos.X() = aAreaRect.Left() + xthickness;
			break;

		default:
			aPartSize.Width() = aAreaRect.GetWidth();	
			aPartPos.X() = aAreaRect.Left();
			break;
	}
    aPartRect = Rectangle( aPartPos, aPartSize );

	if ( pIndicatorSize )
		gtk_requisition_free( pIndicatorSize );
	if ( pIndicatorSpacing )
		gtk_border_free( pIndicatorSpacing );

	return( aPartRect );
}

//----

static Rectangle NWGetListBoxIndicatorRect( int nScreen,
                                            ControlType,
                                            ControlPart,
                                            Rectangle				aAreaRect,
                                            ControlState,
                                            const ImplControlValue&,
                                            SalControlHandle&,
                                            const OUString& )
{
	Rectangle       aIndicatorRect;
	GtkRequisition *pIndicatorSize = NULL;
	GtkBorder      *pIndicatorSpacing = NULL;
	gint            width = 13;	// GTK+ default
	gint            height = 13;	// GTK+ default
	gint            right = 5;	// GTK+ default

	NWEnsureGTKOptionMenu( nScreen );

	gtk_widget_style_get( gWidgetData[nScreen].gOptionMenuWidget,
			"indicator_size",	&pIndicatorSize,
			"indicator_spacing",&pIndicatorSpacing, (char *)NULL);
	
	if ( pIndicatorSize )
    {
		width = pIndicatorSize->width;
		height = pIndicatorSize->height;
    }

	if ( pIndicatorSpacing )
		right = pIndicatorSpacing->right;

    aIndicatorRect.SetSize( Size( width, height ) );
	aIndicatorRect.SetPos( Point( aAreaRect.Left() + aAreaRect.GetWidth() - width - right - gWidgetData[nScreen].gOptionMenuWidget->style->xthickness,
                                  aAreaRect.Top() + ((aAreaRect.GetHeight() - height) / 2) ) );

	// If height is odd, move the indicator down 1 pixel
	if ( aIndicatorRect.GetHeight() % 2 )
		aIndicatorRect.Move( 0, 1 );

	if ( pIndicatorSize )
		gtk_requisition_free( pIndicatorSize );
	if ( pIndicatorSpacing )
		gtk_border_free( pIndicatorSpacing );

	return( aIndicatorRect );
}

static Rectangle NWGetToolbarRect(  int nScreen,
                                    ControlType,
									ControlPart				nPart,
									Rectangle				aAreaRect,
									ControlState,
									const ImplControlValue&,
									SalControlHandle&,
									const OUString& )
{
    Rectangle aRet;

    if( nPart == PART_DRAW_BACKGROUND_HORZ ||
        nPart == PART_DRAW_BACKGROUND_VERT )
        aRet = aAreaRect;
    else if( nPart == PART_THUMB_HORZ )
        aRet = Rectangle( Point( 0, 0 ), Size( aAreaRect.GetWidth(), 10 ) );
    else if( nPart == PART_THUMB_VERT )
        aRet = Rectangle( Point( 0, 0 ), Size( 10, aAreaRect.GetHeight() ) );
    else if( nPart == PART_BUTTON )
    {
        aRet = aAreaRect;
        
        NWEnsureGTKToolbar( nScreen );
        
        gint nMinWidth =
            2*gWidgetData[nScreen].gToolbarButtonWidget->style->xthickness
            + 1 // CHILD_SPACING constant, found in gtk_button.c
            + 3*gWidgetData[nScreen].gToolbarButtonWidget->style->xthickness; // Murphy factor
        gint nMinHeight =
            2*gWidgetData[nScreen].gToolbarButtonWidget->style->ythickness
            + 1 // CHILD_SPACING constant, found in gtk_button.c
            + 3*gWidgetData[nScreen].gToolbarButtonWidget->style->ythickness; // Murphy factor                          
        
        gtk_widget_ensure_style( gWidgetData[nScreen].gToolbarButtonWidget );
        if( aAreaRect.GetWidth() < nMinWidth )
            aRet.Right() = aRet.Left() + nMinWidth;
        if( aAreaRect.GetHeight() < nMinHeight  )
            aRet.Bottom() = aRet.Top() + nMinHeight;
    }

    return aRet;
}

/************************************************************************
 * helper for GtkSalFrame
 ************************************************************************/
static inline Color getColor( const GdkColor& rCol )
{
    return Color( rCol.red >> 8, rCol.green >> 8, rCol.blue >> 8 );
}

#if OSL_DEBUG_LEVEL > 1

void printColor( const char* name, const GdkColor& rCol )
{
    std::fprintf( stderr, "   %s = 0x%2x 0x%2x 0x%2x\n",
             name,
             rCol.red >> 8, rCol.green >> 8, rCol.blue >> 8 );
}

void printStyleColors( GtkStyle* pStyle )
{
    static const char* pStates[] = { "NORMAL", "ACTIVE", "PRELIGHT", "SELECTED", "INSENSITIVE" };

    for( int i = 0; i < 5; i++ )
    {
        std::fprintf( stderr, "state %s colors:\n", pStates[i] );
        printColor( "bg     ", pStyle->bg[i] ); 
        printColor( "fg     ", pStyle->fg[i] ); 
        printColor( "light  ", pStyle->light[i] ); 
        printColor( "dark   ", pStyle->dark[i] ); 
        printColor( "mid    ", pStyle->mid[i] ); 
        printColor( "text   ", pStyle->text[i] ); 
        printColor( "base   ", pStyle->base[i] ); 
        printColor( "text_aa", pStyle->text_aa[i] ); 
    }
}
#endif

void GtkSalGraphics::updateSettings( AllSettings& rSettings )
{
    // get the widgets in place
    NWEnsureGTKMenu( m_nScreen );
    NWEnsureGTKMenubar( m_nScreen );
    NWEnsureGTKScrollbars( m_nScreen );
    NWEnsureGTKEditBox( m_nScreen );
    NWEnsureGTKTooltip( m_nScreen );
    
    gtk_widget_ensure_style( m_pWindow );
    GtkStyle* pStyle = gtk_widget_get_style( m_pWindow );
    
    StyleSettings aStyleSet = rSettings.GetStyleSettings();

#if OSL_DEBUG_LEVEL > 2
    printStyleColors( pStyle );
#endif
    
    // text colors
    Color aTextColor = getColor( pStyle->text[GTK_STATE_NORMAL] );
    aStyleSet.SetDialogTextColor( aTextColor );
    aStyleSet.SetButtonTextColor( aTextColor );
    aStyleSet.SetRadioCheckTextColor( aTextColor );
    aStyleSet.SetGroupTextColor( aTextColor );
    aStyleSet.SetLabelTextColor( aTextColor );
    aStyleSet.SetInfoTextColor( aTextColor );
    aStyleSet.SetWindowTextColor( aTextColor );
    aStyleSet.SetFieldTextColor( aTextColor );

    // Tooltip colors
    GtkStyle* pTooltipStyle = gtk_widget_get_style( gWidgetData[m_nScreen].gTooltipPopup );
    aTextColor = getColor( pTooltipStyle->fg[ GTK_STATE_NORMAL ] );
    aStyleSet.SetHelpTextColor( aTextColor );
    
    // mouse over text colors
    aTextColor = getColor( pStyle->fg[ GTK_STATE_PRELIGHT ] );
    aStyleSet.SetButtonRolloverTextColor( aTextColor );
    aStyleSet.SetFieldRolloverTextColor( aTextColor );
    
    // background colors
    Color aBackColor = getColor( pStyle->bg[GTK_STATE_NORMAL] );
    Color aBackFieldColor = getColor( pStyle->base[ GTK_STATE_NORMAL ] );
    aStyleSet.Set3DColors( aBackColor );
    aStyleSet.SetFaceColor( aBackColor );
    aStyleSet.SetDialogColor( aBackColor );
    aStyleSet.SetWorkspaceColor( aBackColor );
    aStyleSet.SetFieldColor( aBackFieldColor );
    aStyleSet.SetWindowColor( aBackFieldColor );
//    aStyleSet.SetHelpColor( aBackColor );
    // ancient wisdom tells us a mystic algorithm how to set checked color
    if( aBackColor == COL_LIGHTGRAY )
        aStyleSet.SetCheckedColor( Color( 0xCC, 0xCC, 0xCC ) );
    else
    {
        Color aColor2 = aStyleSet.GetLightColor();
        Color aCheck( (BYTE)(((USHORT)aBackColor.GetRed()+(USHORT)aColor2.GetRed())/2),
                      (BYTE)(((USHORT)aBackColor.GetGreen()+(USHORT)aColor2.GetGreen())/2),
                      (BYTE)(((USHORT)aBackColor.GetBlue()+(USHORT)aColor2.GetBlue())/2)
                      );
        aStyleSet.SetCheckedColor( aCheck );
    }

    // highlighting colors
    Color aHighlightColor = getColor( pStyle->base[GTK_STATE_SELECTED] );
    Color aHighlightTextColor = getColor( pStyle->text[GTK_STATE_SELECTED] );
    aStyleSet.SetHighlightColor( aHighlightColor );
    aStyleSet.SetHighlightTextColor( aHighlightTextColor );

    if( ! gtk_check_version( 2, 10, 0 ) ) // link colors came in with 2.10, avoid an assertion
    {
        // hyperlink colors
        GdkColor *link_color = NULL;
        gtk_widget_style_get (m_pWindow, "link-color", &link_color, NULL);
        if (link_color)
        {
            aStyleSet.SetLinkColor(getColor(*link_color));
            gdk_color_free (link_color);
            link_color = NULL;
        }
        gtk_widget_style_get (m_pWindow, "visited-link-color", &link_color, NULL);
        if (link_color)
        {
            aStyleSet.SetVisitedLinkColor(getColor(*link_color));
            gdk_color_free (link_color);
        }
    }

    // Tab colors
    aStyleSet.SetActiveTabColor( aBackFieldColor ); // same as the window color.
    Color aSelectedBackColor = getColor( pStyle->bg[GTK_STATE_ACTIVE] );
    aStyleSet.SetInactiveTabColor( aSelectedBackColor );

    // menu disabled entries handling
    aStyleSet.SetSkipDisabledInMenus( TRUE );
    // menu colors
    GtkStyle* pMenuStyle = gtk_widget_get_style( gWidgetData[m_nScreen].gMenuWidget );
    GtkStyle* pMenuItemStyle = gtk_rc_get_style( gWidgetData[m_nScreen].gMenuItemMenuWidget );
    GtkStyle* pMenubarStyle = gtk_rc_get_style( gWidgetData[m_nScreen].gMenubarWidget );
    GtkStyle* pMenuTextStyle = gtk_rc_get_style( gtk_bin_get_child( GTK_BIN(gWidgetData[m_nScreen].gMenuItemMenuWidget) ) );
    
    aBackColor = getColor( pMenubarStyle->bg[GTK_STATE_NORMAL] );
    aStyleSet.SetMenuBarColor( aBackColor );
    aBackColor = getColor( pMenuStyle->bg[GTK_STATE_NORMAL] );
    aTextColor = getColor( pMenuTextStyle->text[GTK_STATE_NORMAL] );
    aStyleSet.SetMenuColor( aBackColor );
    aStyleSet.SetMenuTextColor( aTextColor );

    aTextColor = getColor( pMenubarStyle->text[GTK_STATE_NORMAL] );
    aStyleSet.SetMenuBarTextColor( aTextColor );
    
#if OSL_DEBUG_LEVEL > 1
    std::fprintf( stderr, "==\n" );
    std::fprintf( stderr, "MenuColor = %x (%d)\n", (int)aStyleSet.GetMenuColor().GetColor(), aStyleSet.GetMenuColor().GetLuminance() );
    std::fprintf( stderr, "MenuTextColor = %x (%d)\n", (int)aStyleSet.GetMenuTextColor().GetColor(), aStyleSet.GetMenuTextColor().GetLuminance() );
    std::fprintf( stderr, "MenuBarColor = %x (%d)\n", (int)aStyleSet.GetMenuBarColor().GetColor(), aStyleSet.GetMenuBarColor().GetLuminance() );
    std::fprintf( stderr, "MenuBarTextColor = %x (%d)\n", (int)aStyleSet.GetMenuBarTextColor().GetColor(), aStyleSet.GetMenuBarTextColor().GetLuminance() );
    std::fprintf( stderr, "LightColor = %x (%d)\n", (int)aStyleSet.GetLightColor().GetColor(), aStyleSet.GetLightColor().GetLuminance() );
    std::fprintf( stderr, "ShadowColor = %x (%d)\n", (int)aStyleSet.GetShadowColor().GetColor(), aStyleSet.GetShadowColor().GetLuminance() );
#endif

    // Awful hack for menu separators in the Sonar and similar themes.
    // If the menu color is not too dark, and the menu text color is lighter,
    // make the "light" color lighter than the menu color and the "shadow"
    // color darker than it.
    if ( aStyleSet.GetMenuColor().GetLuminance() >= 32 &&
	 aStyleSet.GetMenuColor().GetLuminance() <= aStyleSet.GetMenuTextColor().GetLuminance() )
    {
      Color temp = aStyleSet.GetMenuColor();
      temp.IncreaseLuminance( 8 );
      aStyleSet.SetLightColor( temp );
      temp = aStyleSet.GetMenuColor();
      temp.DecreaseLuminance( 16 );
      aStyleSet.SetShadowColor( temp );
    }
    
    aHighlightColor = getColor( pMenuItemStyle->bg[ GTK_STATE_SELECTED ] );
    aHighlightTextColor = getColor( pMenuTextStyle->fg[ GTK_STATE_PRELIGHT ] );
    if( aHighlightColor == aHighlightTextColor )
        aHighlightTextColor = (aHighlightColor.GetLuminance() < 128) ? Color( COL_WHITE ) : Color( COL_BLACK );
    aStyleSet.SetMenuHighlightColor( aHighlightColor );
    aStyleSet.SetMenuHighlightTextColor( aHighlightTextColor );
    
    // UI font
    OString	aFamily		= pango_font_description_get_family( pStyle->font_desc );
    int nPangoHeight	= pango_font_description_get_size( pStyle->font_desc );
    PangoStyle	eStyle	= pango_font_description_get_style( pStyle->font_desc );
    PangoWeight	eWeight	= pango_font_description_get_weight( pStyle->font_desc );
    PangoStretch eStretch = pango_font_description_get_stretch( pStyle->font_desc );
    
    psp::FastPrintFontInfo aInfo;
    // set family name
    aInfo.m_aFamilyName = OStringToOUString( aFamily, RTL_TEXTENCODING_UTF8 );
    // set italic
    switch( eStyle )
    {
        case PANGO_STYLE_NORMAL:	aInfo.m_eItalic = psp::italic::Upright;break;
        case PANGO_STYLE_ITALIC:	aInfo.m_eItalic = psp::italic::Italic;break;
        case PANGO_STYLE_OBLIQUE:	aInfo.m_eItalic = psp::italic::Oblique;break;
    }
    // set weight
    if( eWeight <= PANGO_WEIGHT_ULTRALIGHT )
        aInfo.m_eWeight = psp::weight::UltraLight;
    else if( eWeight <= PANGO_WEIGHT_LIGHT )
        aInfo.m_eWeight = psp::weight::Light;
    else if( eWeight <= PANGO_WEIGHT_NORMAL )
        aInfo.m_eWeight = psp::weight::Normal;
    else if( eWeight <= PANGO_WEIGHT_BOLD )
        aInfo.m_eWeight = psp::weight::Bold;
    else
        aInfo.m_eWeight = psp::weight::UltraBold;
    // set width
    switch( eStretch )
    {
        case PANGO_STRETCH_ULTRA_CONDENSED:	aInfo.m_eWidth = psp::width::UltraCondensed;break;
        case PANGO_STRETCH_EXTRA_CONDENSED:	aInfo.m_eWidth = psp::width::ExtraCondensed;break;
        case PANGO_STRETCH_CONDENSED:		aInfo.m_eWidth = psp::width::Condensed;break;
        case PANGO_STRETCH_SEMI_CONDENSED:	aInfo.m_eWidth = psp::width::SemiCondensed;break;
        case PANGO_STRETCH_NORMAL:			aInfo.m_eWidth = psp::width::Normal;break;
        case PANGO_STRETCH_SEMI_EXPANDED:	aInfo.m_eWidth = psp::width::SemiExpanded;break;
        case PANGO_STRETCH_EXPANDED:		aInfo.m_eWidth = psp::width::Expanded;break;
        case PANGO_STRETCH_EXTRA_EXPANDED:	aInfo.m_eWidth = psp::width::ExtraExpanded;break;
        case PANGO_STRETCH_ULTRA_EXPANDED:	aInfo.m_eWidth = psp::width::UltraExpanded;break;
    }

#if OSL_DEBUG_LEVEL > 1
    std::fprintf( stderr, "font name BEFORE system match: \"%s\"\n", aFamily.getStr() );
#endif

    // match font to e.g. resolve "Sans"
    psp::PrintFontManager::get().matchFont( aInfo, rSettings.GetUILocale() );

#if OSL_DEBUG_LEVEL > 1
    std::fprintf( stderr, "font match %s, name AFTER: \"%s\"\n",
             aInfo.m_nID != 0 ? "succeeded" : "failed",
             OUStringToOString( aInfo.m_aFamilyName, RTL_TEXTENCODING_ISO_8859_1 ).getStr() );
#endif
    
    sal_Int32 nDispDPIY = GetDisplay()->GetResolution().B();
    int nPointHeight = 0;
    static gboolean(*pAbso)(const PangoFontDescription*) =
        (gboolean(*)(const PangoFontDescription*))osl_getAsciiFunctionSymbol( GetSalData()->m_pPlugin, "pango_font_description_get_size_is_absolute" );

    if( pAbso && pAbso( pStyle->font_desc ) )
        nPointHeight = (nPangoHeight * 72 + nDispDPIY*PANGO_SCALE/2) / (nDispDPIY * PANGO_SCALE);
    else
        nPointHeight = nPangoHeight/PANGO_SCALE;
    
    Font aFont( aInfo.m_aFamilyName, Size( 0, nPointHeight ) );
    if( aInfo.m_eWeight != psp::weight::Unknown )
        aFont.SetWeight( PspGraphics::ToFontWeight( aInfo.m_eWeight ) );
    if( aInfo.m_eWidth != psp::width::Unknown )
        aFont.SetWidthType( PspGraphics::ToFontWidth( aInfo.m_eWidth ) );
    if( aInfo.m_eItalic != psp::italic::Unknown )
        aFont.SetItalic( PspGraphics::ToFontItalic( aInfo.m_eItalic ) );
    if( aInfo.m_ePitch != psp::pitch::Unknown )
        aFont.SetPitch( PspGraphics::ToFontPitch( aInfo.m_ePitch ) );

    aStyleSet.SetAppFont( aFont );
    aStyleSet.SetHelpFont( aFont );
    aStyleSet.SetTitleFont( aFont );
    aStyleSet.SetFloatTitleFont( aFont );
    aStyleSet.SetMenuFont( aFont );
    aStyleSet.SetToolFont( aFont );
    aStyleSet.SetLabelFont( aFont );
    aStyleSet.SetInfoFont( aFont );
    aStyleSet.SetRadioCheckFont( aFont );
    aStyleSet.SetPushButtonFont( aFont );
    aStyleSet.SetFieldFont( aFont );
    aStyleSet.SetIconFont( aFont );
    aStyleSet.SetGroupFont( aFont );

    // get cursor blink time
    GtkSettings *pSettings = gtk_widget_get_settings( gWidgetData[m_nScreen].gEditBoxWidget );
    gboolean blink = false;

    g_object_get( pSettings, "gtk-cursor-blink", &blink, (char *)NULL );
    if( blink )
    {
        gint blink_time = STYLE_CURSOR_NOBLINKTIME;
        g_object_get( pSettings, "gtk-cursor-blink-time", &blink_time, (char *)NULL );
        // set the blink_time if there is a setting and it is reasonable
        // else leave the default value
        if( blink_time > 100 && blink_time != gint(STYLE_CURSOR_NOBLINKTIME) )
            aStyleSet.SetCursorBlinkTime( blink_time/2 );
    }
    else
        aStyleSet.SetCursorBlinkTime( STYLE_CURSOR_NOBLINKTIME );

    gboolean showmenuicons = true;
    pSettings = gtk_widget_get_settings( gWidgetData[m_nScreen].gImageMenuItem );
    g_object_get( pSettings, "gtk-menu-images", &showmenuicons, (char *)NULL );
    aStyleSet.SetUseImagesInMenus( showmenuicons );

    // set scrollbar settings
    gint slider_width = 14;
    gint trough_border = 1;
    gint min_slider_length = 21;
    
    // Grab some button style attributes
    gtk_widget_style_get( gWidgetData[m_nScreen].gScrollHorizWidget,
                          "slider-width", &slider_width,
                          "trough-border", &trough_border,
                          "min-slider-length", &min_slider_length,
                          (char *)NULL );
    gint magic = trough_border ? 1 : 0;
    aStyleSet.SetScrollBarSize( slider_width + 2*trough_border );
    aStyleSet.SetMinThumbSize( min_slider_length - magic );

    // preferred icon style
    gchar* pIconThemeName = NULL;
    g_object_get( gtk_settings_get_default(), "gtk-icon-theme-name", &pIconThemeName, (char *)NULL );
    aStyleSet.SetPreferredSymbolsStyleName( OUString::createFromAscii(pIconThemeName) );
    g_free (pIconThemeName);

    //  FIXME: need some way of fetching toolbar icon size.
//	aStyleSet.SetToolbarIconSize( STYLE_TOOLBAR_ICONSIZE_SMALL );

    // finally update the collected settings
    rSettings.SetStyleSettings( aStyleSet );

    #if OSL_DEBUG_LEVEL > 1
    {    
        GtkSettings* pGtkSettings = gtk_settings_get_default();
        GValue aValue;
        memset( &aValue, 0, sizeof(GValue) );
        g_value_init( &aValue, G_TYPE_STRING );
        g_object_get_property( G_OBJECT(pGtkSettings), "gtk-theme-name", &aValue );
        const gchar* pThemeName = g_value_get_string( &aValue );
        std::fprintf( stderr, "Theme name is \"%s\"\n", pThemeName );
        g_value_unset( &aValue );
    }
    #endif
    GtkSettings* pGtkSettings = gtk_settings_get_default();
    GValue aValue;
    memset( &aValue, 0, sizeof(GValue) );
    g_value_init( &aValue, G_TYPE_STRING );
    g_object_get_property( G_OBJECT(pGtkSettings), "gtk-theme-name", &aValue );
    const gchar* pThemeName = g_value_get_string( &aValue );

    // default behaviour
    bNeedPixmapPaint = bGlobalNeedPixmapPaint;
    bToolbarGripWorkaround = false;
    bNeedButtonStyleAsEditBackgroundWorkaround = false;

    // setup some workarounds for "blueprint" theme
    if( pThemeName && strncasecmp( pThemeName, "blueprint", 9 ) == 0 )
    {
        bNeedButtonStyleAsEditBackgroundWorkaround = true;
        if( GetX11SalData()->GetDisplay()->GetServerVendor() == vendor_sun )
        {
            // #i52570#, #i61532# workaround a weird paint issue;
            // on a Sunray Xserver sometimes painting buttons and edits
            // won't work when using the blueprint theme
            // not reproducible with simpler programs or other themes
            if( pThemeName && strncasecmp( pThemeName, "blueprint", 9 ) == 0 )
            {
                bNeedPixmapPaint = true;
                bToolbarGripWorkaround = true;
            }
        }
    }
    // clean up
    g_value_unset( &aValue );
}


/************************************************************************
 * Create a GdkPixmap filled with the contents of an area of an Xlib window
 ************************************************************************/

GdkPixmap* GtkSalGraphics::NWGetPixmapFromScreen( Rectangle srcRect )
{
    // Create a new pixmap to hold the composite of the window background and the control
    GdkPixmap * pPixmap		= gdk_pixmap_new( GDK_DRAWABLE(GetGdkWindow()), srcRect.GetWidth(), srcRect.GetHeight(), -1 ); 
	GdkGC *	 pPixmapGC	= gdk_gc_new( pPixmap );

    if( !pPixmap || !pPixmapGC )
    {
        if ( pPixmap )
            g_object_unref( pPixmap );
        if ( pPixmapGC )
            g_object_unref( pPixmapGC );
        std::fprintf( stderr, "salnativewidgets-gtk.cxx: could not get valid pixmap from screen\n" );
        return( NULL );
    }

    // Copy the background of the screen into a composite pixmap
    CopyScreenArea( GetXDisplay(),
              GetDrawable(), GetScreenNumber(), GetVisual().GetDepth(),
              gdk_x11_drawable_get_xid(pPixmap),
              gdk_screen_get_number( gdk_drawable_get_screen( GDK_DRAWABLE(pPixmap) ) ),
              gdk_drawable_get_depth( GDK_DRAWABLE( pPixmap ) ),
              gdk_x11_gc_get_xgc(pPixmapGC),
              srcRect.Left(), srcRect.Top(), srcRect.GetWidth(), srcRect.GetHeight(), 0, 0 );

    g_object_unref( pPixmapGC );
    return( pPixmap );
}




/************************************************************************
 * Copy an alpha pixmap to screen using a gc with clipping
 ************************************************************************/

BOOL GtkSalGraphics::NWRenderPixmapToScreen( GdkPixmap* pPixmap, Rectangle dstRect )
{
    // The GC can't be null, otherwise we'd have no clip region
    if( SelectFont() == NULL )
    {
        std::fprintf(stderr, "salnativewidgets.cxx: no valid GC\n" );
        return( FALSE );
    }

    if ( !pPixmap )
        return( FALSE );

    // Copy the background of the screen into a composite pixmap
    CopyScreenArea( GetXDisplay(),
              GDK_DRAWABLE_XID(pPixmap),
              gdk_screen_get_number( gdk_drawable_get_screen( GDK_DRAWABLE(pPixmap) ) ),
              gdk_drawable_get_depth( GDK_DRAWABLE(pPixmap) ),
              GetDrawable(), m_nScreen, GetVisual().GetDepth(),
              SelectFont(),
              0, 0, dstRect.GetWidth(), dstRect.GetHeight(), dstRect.Left(), dstRect.Top() );

    return( TRUE );
}


/************************************************************************
 * State conversion
 ************************************************************************/
static void NWConvertVCLStateToGTKState( ControlState nVCLState,
			GtkStateType* nGTKState, GtkShadowType* nGTKShadow )
{
	*nGTKShadow = GTK_SHADOW_OUT;
	*nGTKState = GTK_STATE_INSENSITIVE;

	if ( nVCLState & CTRL_STATE_ENABLED )
	{
		if ( nVCLState & CTRL_STATE_PRESSED )
		{
			*nGTKState = GTK_STATE_ACTIVE;
			*nGTKShadow = GTK_SHADOW_IN;
		}
		else if ( nVCLState & CTRL_STATE_ROLLOVER )
		{
			*nGTKState = GTK_STATE_PRELIGHT;
			*nGTKShadow = GTK_SHADOW_OUT;
		}
		else
		{
			*nGTKState = GTK_STATE_NORMAL;
			*nGTKShadow = GTK_SHADOW_OUT;
		}
	}
}

/************************************************************************
 * Set widget flags
 ************************************************************************/
static void NWSetWidgetState( GtkWidget* widget, ControlState nState, GtkStateType nGtkState )
{
	// Set to default state, then build up from there
	GTK_WIDGET_UNSET_FLAGS( widget, GTK_HAS_DEFAULT );
	GTK_WIDGET_UNSET_FLAGS( widget, GTK_HAS_FOCUS );
	GTK_WIDGET_UNSET_FLAGS( widget, GTK_SENSITIVE );
	GTK_WIDGET_SET_FLAGS( widget, gWidgetDefaultFlags[(long)widget] );

	if ( nState & CTRL_STATE_DEFAULT )
		GTK_WIDGET_SET_FLAGS( widget, GTK_HAS_DEFAULT );
	if ( !GTK_IS_TOGGLE_BUTTON(widget) && (nState & CTRL_STATE_FOCUSED) )
		GTK_WIDGET_SET_FLAGS( widget, GTK_HAS_FOCUS );
	if ( nState & CTRL_STATE_ENABLED )
		GTK_WIDGET_SET_FLAGS( widget, GTK_SENSITIVE );
	gtk_widget_set_state( widget, nGtkState );
}

/************************************************************************
 * Widget ensure functions - make sure cached objects are valid
 ************************************************************************/

//-------------------------------------

static void NWAddWidgetToCacheWindow( GtkWidget* widget, int nScreen )
{
    NWFWidgetData& rData = gWidgetData[nScreen];
	if ( !rData.gCacheWindow || !rData.gDumbContainer )
	{
		if ( !rData.gCacheWindow )
        {
			rData.gCacheWindow = gtk_window_new( GTK_WINDOW_TOPLEVEL );
            GdkScreen* pScreen = gdk_display_get_screen( gdk_display_get_default(), nScreen );
            if( pScreen )
                gtk_window_set_screen( GTK_WINDOW(rData.gCacheWindow), pScreen );
        }
		if ( !rData.gDumbContainer )
			rData.gDumbContainer = gtk_fixed_new();
		gtk_container_add( GTK_CONTAINER(rData.gCacheWindow), rData.gDumbContainer );
		gtk_widget_realize( rData.gDumbContainer );
		gtk_widget_realize( rData.gCacheWindow );
	}

	gtk_container_add( GTK_CONTAINER(rData.gDumbContainer), widget );
	gtk_widget_realize( widget );
	gtk_widget_ensure_style( widget );

	// Store widget's default flags
	gWidgetDefaultFlags[ (long)widget ] = GTK_WIDGET_FLAGS( widget );
}

//-------------------------------------

static void NWEnsureGTKButton( int nScreen )
{
	if ( !gWidgetData[nScreen].gBtnWidget )
	{
		gWidgetData[nScreen].gBtnWidget = gtk_button_new_with_label( "" );
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gBtnWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKRadio( int nScreen )
{
	if ( !gWidgetData[nScreen].gRadioWidget || !gWidgetData[nScreen].gRadioWidgetSibling )
	{
		gWidgetData[nScreen].gRadioWidget = gtk_radio_button_new( NULL );
		gWidgetData[nScreen].gRadioWidgetSibling = gtk_radio_button_new_from_widget( GTK_RADIO_BUTTON(gWidgetData[nScreen].gRadioWidget) );
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gRadioWidget, nScreen );
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gRadioWidgetSibling, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKCheck( int nScreen )
{
	if ( !gWidgetData[nScreen].gCheckWidget )
	{
		gWidgetData[nScreen].gCheckWidget = gtk_check_button_new();
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gCheckWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKScrollbars( int nScreen )
{
	if ( !gWidgetData[nScreen].gScrollHorizWidget )
	{
		gWidgetData[nScreen].gScrollHorizWidget = gtk_hscrollbar_new( NULL );
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gScrollHorizWidget, nScreen );
	}

	if ( !gWidgetData[nScreen].gScrollVertWidget )
	{
		gWidgetData[nScreen].gScrollVertWidget = gtk_vscrollbar_new( NULL );
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gScrollVertWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKArrow( int nScreen )
{
	if ( !gWidgetData[nScreen].gArrowWidget || !gWidgetData[nScreen].gDropdownWidget )
	{
		gWidgetData[nScreen].gDropdownWidget = gtk_toggle_button_new();
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gDropdownWidget, nScreen );
		gWidgetData[nScreen].gArrowWidget = gtk_arrow_new( GTK_ARROW_DOWN, GTK_SHADOW_OUT );
		gtk_container_add( GTK_CONTAINER(gWidgetData[nScreen].gDropdownWidget), gWidgetData[nScreen].gArrowWidget );
		gtk_widget_set_rc_style( gWidgetData[nScreen].gArrowWidget );
		gtk_widget_realize( gWidgetData[nScreen].gArrowWidget );
	}
}

//-------------------------------------

static void NWEnsureGTKEditBox( int nScreen )
{
	if ( !gWidgetData[nScreen].gEditBoxWidget )
	{
		gWidgetData[nScreen].gEditBoxWidget = gtk_entry_new();
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gEditBoxWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKSpinButton( int nScreen )
{
	if ( !gWidgetData[nScreen].gSpinButtonWidget )
	{
		GtkAdjustment *adj = GTK_ADJUSTMENT( gtk_adjustment_new(0, 0, 1, 1, 1, 0) );
		gWidgetData[nScreen].gSpinButtonWidget = gtk_spin_button_new( adj, 1, 2 );

		//Setting non-editable means it doesn't blink, so there's no timeouts
		//running around to nobble us
		gtk_editable_set_editable(GTK_EDITABLE(gWidgetData[nScreen].gSpinButtonWidget), false);

		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gSpinButtonWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKNotebook( int nScreen )
{
	if ( !gWidgetData[nScreen].gNotebookWidget )
	{
		gWidgetData[nScreen].gNotebookWidget = gtk_notebook_new();
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gNotebookWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKOptionMenu( int nScreen )
{
	if ( !gWidgetData[nScreen].gOptionMenuWidget )
	{
		gWidgetData[nScreen].gOptionMenuWidget = gtk_option_menu_new();
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gOptionMenuWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKCombo( int nScreen )
{
	if ( !gWidgetData[nScreen].gComboWidget )
	{
		gWidgetData[nScreen].gComboWidget = gtk_combo_new();

		// #i59129# Setting non-editable means it doesn't blink, so
        // there are no timeouts running around to nobble us
		gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(gWidgetData[nScreen].gComboWidget)->entry), false);

		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gComboWidget, nScreen );
		// Must realize the ComboBox's children, since GTK
		// does not do this for us in GtkCombo::gtk_widget_realize()
		gtk_widget_realize( GTK_COMBO(gWidgetData[nScreen].gComboWidget)->button );
		gtk_widget_realize( GTK_COMBO(gWidgetData[nScreen].gComboWidget)->entry );
	}
}

//-------------------------------------

static void NWEnsureGTKScrolledWindow( int nScreen )
{
	if ( !gWidgetData[nScreen].gScrolledWindowWidget )
	{
		GtkAdjustment *hadj = GTK_ADJUSTMENT( gtk_adjustment_new(0, 0, 0, 0, 0, 0) );
		GtkAdjustment *vadj = GTK_ADJUSTMENT( gtk_adjustment_new(0, 0, 0, 0, 0, 0) );

		gWidgetData[nScreen].gScrolledWindowWidget = gtk_scrolled_window_new( hadj, vadj );
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gScrolledWindowWidget, nScreen );
	}
}

//-------------------------------------

static void NWEnsureGTKToolbar( int nScreen )
{
    if( !gWidgetData[nScreen].gToolbarWidget )
    {
        gWidgetData[nScreen].gToolbarWidget = gtk_toolbar_new();
        NWAddWidgetToCacheWindow( gWidgetData[nScreen].gToolbarWidget, nScreen );
        gWidgetData[nScreen].gToolbarButtonWidget = gtk_button_new();
        gWidgetData[nScreen].gToolbarToggleWidget = gtk_toggle_button_new();
        
        GtkReliefStyle aRelief = GTK_RELIEF_NORMAL;
        gtk_widget_ensure_style( gWidgetData[nScreen].gToolbarWidget );
        gtk_widget_style_get( gWidgetData[nScreen].gToolbarWidget,
			                  "button_relief", &aRelief,
                              (char *)NULL);
                              
        gtk_button_set_relief( GTK_BUTTON(gWidgetData[nScreen].gToolbarButtonWidget), aRelief );
        GTK_WIDGET_UNSET_FLAGS( gWidgetData[nScreen].gToolbarButtonWidget, GTK_CAN_FOCUS );
        GTK_WIDGET_UNSET_FLAGS( gWidgetData[nScreen].gToolbarButtonWidget, GTK_CAN_DEFAULT );
        NWAddWidgetToCacheWindow( gWidgetData[nScreen].gToolbarButtonWidget, nScreen );

        gtk_button_set_relief( GTK_BUTTON(gWidgetData[nScreen].gToolbarToggleWidget), aRelief );
        GTK_WIDGET_UNSET_FLAGS( gWidgetData[nScreen].gToolbarToggleWidget, GTK_CAN_FOCUS );
        GTK_WIDGET_UNSET_FLAGS( gWidgetData[nScreen].gToolbarToggleWidget, GTK_CAN_DEFAULT );
        NWAddWidgetToCacheWindow( gWidgetData[nScreen].gToolbarToggleWidget, nScreen );        
    }
    if( ! gWidgetData[nScreen].gHandleBoxWidget )
    {
        gWidgetData[nScreen].gHandleBoxWidget = gtk_handle_box_new();
        NWAddWidgetToCacheWindow( gWidgetData[nScreen].gHandleBoxWidget, nScreen );
    }
}

//-------------------------------------

static void NWEnsureGTKMenubar( int nScreen )
{
    if( !gWidgetData[nScreen].gMenubarWidget )
    {
        gWidgetData[nScreen].gMenubarWidget = gtk_menu_bar_new();
        gWidgetData[nScreen].gMenuItemMenubarWidget = gtk_menu_item_new_with_label( "b" );
        gtk_menu_shell_append( GTK_MENU_SHELL( gWidgetData[nScreen].gMenubarWidget ), gWidgetData[nScreen].gMenuItemMenubarWidget );
        gtk_widget_show( gWidgetData[nScreen].gMenuItemMenubarWidget );
        NWAddWidgetToCacheWindow( gWidgetData[nScreen].gMenubarWidget, nScreen );
        gtk_widget_show( gWidgetData[nScreen].gMenubarWidget );
        
        // do what NWAddWidgetToCacheWindow does except adding to def container
        gtk_widget_realize( gWidgetData[nScreen].gMenuItemMenubarWidget );
        gtk_widget_ensure_style( gWidgetData[nScreen].gMenuItemMenubarWidget );

        gWidgetDefaultFlags[ (long)gWidgetData[nScreen].gMenuItemMenubarWidget ] = GTK_WIDGET_FLAGS( gWidgetData[nScreen].gMenuItemMenubarWidget );
    }
}

static void NWEnsureGTKMenu( int nScreen )
{
    if( !gWidgetData[nScreen].gMenuWidget )
    {
        gWidgetData[nScreen].gMenuWidget              = gtk_menu_new();
        gWidgetData[nScreen].gMenuItemMenuWidget      = gtk_menu_item_new_with_label( "b" );
        gWidgetData[nScreen].gMenuItemCheckMenuWidget = gtk_check_menu_item_new_with_label( "b" );
        gWidgetData[nScreen].gMenuItemRadioMenuWidget = gtk_radio_menu_item_new_with_label( NULL, "b" );
        gWidgetData[nScreen].gImageMenuItem           = gtk_image_menu_item_new();
        
        gtk_menu_shell_append( GTK_MENU_SHELL( gWidgetData[nScreen].gMenuWidget ), gWidgetData[nScreen].gMenuItemMenuWidget );
        gtk_menu_shell_append( GTK_MENU_SHELL( gWidgetData[nScreen].gMenuWidget ), gWidgetData[nScreen].gMenuItemCheckMenuWidget );
        gtk_menu_shell_append( GTK_MENU_SHELL( gWidgetData[nScreen].gMenuWidget ), gWidgetData[nScreen].gMenuItemRadioMenuWidget );
        gtk_menu_shell_append( GTK_MENU_SHELL( gWidgetData[nScreen].gMenuWidget ), gWidgetData[nScreen].gImageMenuItem );
        
        // do what NWAddWidgetToCacheWindow does except adding to def container
        gtk_widget_realize( gWidgetData[nScreen].gMenuWidget );
        gtk_widget_ensure_style( gWidgetData[nScreen].gMenuWidget );

        gtk_widget_realize( gWidgetData[nScreen].gMenuItemMenuWidget );
        gtk_widget_ensure_style( gWidgetData[nScreen].gMenuItemMenuWidget );

        gtk_widget_realize( gWidgetData[nScreen].gMenuItemCheckMenuWidget );
        gtk_widget_ensure_style( gWidgetData[nScreen].gMenuItemCheckMenuWidget );

        gtk_widget_realize( gWidgetData[nScreen].gMenuItemRadioMenuWidget );
        gtk_widget_ensure_style( gWidgetData[nScreen].gMenuItemRadioMenuWidget );

        gtk_widget_realize( gWidgetData[nScreen].gImageMenuItem );
        gtk_widget_ensure_style( gWidgetData[nScreen].gImageMenuItem );

        gWidgetDefaultFlags[ (long)gWidgetData[nScreen].gMenuWidget ] = GTK_WIDGET_FLAGS( gWidgetData[nScreen].gMenuWidget );
        gWidgetDefaultFlags[ (long)gWidgetData[nScreen].gMenuItemMenuWidget ] = GTK_WIDGET_FLAGS( gWidgetData[nScreen].gMenuItemMenuWidget );
        gWidgetDefaultFlags[ (long)gWidgetData[nScreen].gMenuItemCheckMenuWidget ] = GTK_WIDGET_FLAGS( gWidgetData[nScreen].gMenuItemCheckMenuWidget );
        gWidgetDefaultFlags[ (long)gWidgetData[nScreen].gMenuItemRadioMenuWidget ] = GTK_WIDGET_FLAGS( gWidgetData[nScreen].gMenuItemRadioMenuWidget );
        gWidgetDefaultFlags[ (long)gWidgetData[nScreen].gImageMenuItem ] = GTK_WIDGET_FLAGS( gWidgetData[nScreen].gImageMenuItem );
    }
}

static void NWEnsureGTKTooltip( int nScreen )
{
    if( !gWidgetData[nScreen].gTooltipPopup )
    {
        gWidgetData[nScreen].gTooltipPopup = gtk_window_new (GTK_WINDOW_POPUP);
        GdkScreen* pScreen = gdk_display_get_screen( gdk_display_get_default(), nScreen );
        if( pScreen )
            gtk_window_set_screen( GTK_WINDOW(gWidgetData[nScreen].gTooltipPopup), pScreen );
        gtk_widget_set_name( gWidgetData[nScreen].gTooltipPopup, "gtk-tooltips");
        gtk_widget_realize( gWidgetData[nScreen].gTooltipPopup );
        gtk_widget_ensure_style( gWidgetData[nScreen].gTooltipPopup );
    }
}

static void NWEnsureGTKProgressBar( int nScreen )
{
    if( !gWidgetData[nScreen].gProgressBar )
    {
        gWidgetData[nScreen].gProgressBar = gtk_progress_bar_new ();
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gProgressBar, nScreen );
    }
}

static void NWEnsureGTKTreeView( int nScreen )
{
    if( !gWidgetData[nScreen].gTreeView )
    {
        gWidgetData[nScreen].gTreeView = gtk_tree_view_new ();
		NWAddWidgetToCacheWindow( gWidgetData[nScreen].gTreeView, nScreen );
    }
}
