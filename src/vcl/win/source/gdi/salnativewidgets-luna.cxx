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

#define _SV_SALNATIVEWIDGETS_CXX

#include "svsys.h"
#include "salgdi.h"
#include "saldata.hxx"
#include "vcl/svapp.hxx"

#include "rtl/ustring.h"
#include "osl/module.h"

#include "uxtheme.h"
#include "tmschema.h"

#include <map>
#include <string>

using namespace rtl;
using namespace std;

typedef map< wstring, HTHEME > ThemeMap;
static ThemeMap aThemeMap;


/****************************************************
 wrap visual styles API to avoid linking against it
 it is not available on all Windows platforms
*****************************************************/

class VisualStylesAPI
{
private:
    typedef HTHEME  (WINAPI * OpenThemeData_Proc_T) ( HWND hwnd, LPCWSTR pszClassList );
    typedef HRESULT (WINAPI * CloseThemeData_Proc_T) ( HTHEME hTheme );
    typedef HRESULT (WINAPI * GetThemeBackgroundContentRect_Proc_T) ( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect );
    typedef HRESULT (WINAPI * DrawThemeBackground_Proc_T) ( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect );
    typedef HRESULT (WINAPI * DrawThemeText_Proc_T) ( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect );
    typedef HRESULT (WINAPI * GetThemePartSize_Proc_T) ( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, RECT *prc, THEMESIZE eSize, SIZE *psz );

    OpenThemeData_Proc_T                    lpfnOpenThemeData;
    CloseThemeData_Proc_T                   lpfnCloseThemeData;
    GetThemeBackgroundContentRect_Proc_T    lpfnGetThemeBackgroundContentRect;
    DrawThemeBackground_Proc_T              lpfnDrawThemeBackground;
    DrawThemeText_Proc_T                    lpfnDrawThemeText;
    GetThemePartSize_Proc_T                 lpfnGetThemePartSize;

    oslModule mhModule;

public:
    VisualStylesAPI();
    ~VisualStylesAPI();
    BOOL IsAvailable()  { return (mhModule != NULL); }

    HTHEME OpenThemeData( HWND hwnd, LPCWSTR pszClassList );
    HRESULT CloseThemeData( HTHEME hTheme );
    HRESULT GetThemeBackgroundContentRect( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect );
    HRESULT DrawThemeBackground( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect );
    HRESULT DrawThemeText( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect );
    HRESULT GetThemePartSize( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, RECT *prc, THEMESIZE eSize, SIZE *psz );
};

static VisualStylesAPI vsAPI;

VisualStylesAPI::VisualStylesAPI()
{
    OUString aLibraryName( RTL_CONSTASCII_USTRINGPARAM( "uxtheme.dll" ) );
    mhModule = osl_loadModule( aLibraryName.pData, SAL_LOADMODULE_DEFAULT );

    if ( mhModule )
    {
        lpfnOpenThemeData = (OpenThemeData_Proc_T)osl_getAsciiFunctionSymbol( mhModule, "OpenThemeData" );
        lpfnCloseThemeData = (CloseThemeData_Proc_T)osl_getAsciiFunctionSymbol( mhModule, "CloseThemeData" );
        lpfnGetThemeBackgroundContentRect = (GetThemeBackgroundContentRect_Proc_T)osl_getAsciiFunctionSymbol( mhModule, "GetThemeBackgroundContentRect" );
        lpfnDrawThemeBackground = (DrawThemeBackground_Proc_T)osl_getAsciiFunctionSymbol( mhModule, "DrawThemeBackground" );
        lpfnDrawThemeText = (DrawThemeText_Proc_T)osl_getAsciiFunctionSymbol( mhModule, "DrawThemeText" );
        lpfnGetThemePartSize = (GetThemePartSize_Proc_T)osl_getAsciiFunctionSymbol( mhModule, "GetThemePartSize" );
    }
    else
    {
        lpfnOpenThemeData = NULL;
        lpfnCloseThemeData = NULL;
        lpfnGetThemeBackgroundContentRect = NULL;
        lpfnDrawThemeBackground = NULL;
        lpfnDrawThemeText = NULL;
        lpfnGetThemePartSize = NULL;
    }
}
VisualStylesAPI::~VisualStylesAPI()
{
    if( mhModule )
        osl_unloadModule( mhModule );
}
HTHEME VisualStylesAPI::OpenThemeData( HWND hwnd, LPCWSTR pszClassList )
{
    if(lpfnOpenThemeData)
        return (*lpfnOpenThemeData) (hwnd, pszClassList);
    else
        return NULL;
}

HRESULT VisualStylesAPI::CloseThemeData( HTHEME hTheme )
{
    if(lpfnCloseThemeData)
        return (*lpfnCloseThemeData) (hTheme);
    else
        return S_FALSE;
}
HRESULT VisualStylesAPI::GetThemeBackgroundContentRect( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect )
{
    if(lpfnGetThemeBackgroundContentRect)
        return (*lpfnGetThemeBackgroundContentRect) ( hTheme, hdc, iPartId, iStateId, pBoundingRect, pContentRect );
    else
        return S_FALSE;
}
HRESULT VisualStylesAPI::DrawThemeBackground( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect )
{
    if(lpfnDrawThemeBackground)
        return (*lpfnDrawThemeBackground) (hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
    else
        return S_FALSE;
}
HRESULT VisualStylesAPI::DrawThemeText( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect )
{
    if(lpfnDrawThemeText)
        return (*lpfnDrawThemeText) (hTheme, hdc, iPartId, iStateId, pszText, iCharCount, dwTextFlags, dwTextFlags2, pRect);
    else
        return S_FALSE;
}
HRESULT VisualStylesAPI::GetThemePartSize( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, RECT *prc, THEMESIZE eSize, SIZE *psz )
{
    if(lpfnGetThemePartSize)
        return (*lpfnGetThemePartSize) (hTheme, hdc, iPartId, iStateId, prc, eSize, psz);
    else
        return S_FALSE;
}


/*********************************************************
 * Initialize XP theming and local stuff                         
 *********************************************************/
void SalData::initNWF( void )
{
    ImplSVData* pSVData = ImplGetSVData();

    // the menu bar and the top docking area should have a common background (gradient)
    pSVData->maNWFData.mbMenuBarDockingAreaCommonBG = true;
}


// *********************************************************
// * Release theming handles
// ********************************************************
void SalData::deInitNWF( void )
{
    ThemeMap::iterator iter = aThemeMap.begin();
    while( iter != aThemeMap.end() )
    {
        vsAPI.CloseThemeData(iter->second);
        iter++;
    }
    aThemeMap.clear();
}

static HTHEME getThemeHandle( HWND hWnd, LPCWSTR name )
{
    if( GetSalData()->mbThemeChanged )
    {
        // throw away invalid theme handles
        GetSalData()->deInitNWF();
        GetSalData()->mbThemeChanged = FALSE;
    }

    ThemeMap::iterator iter;
    if( (iter = aThemeMap.find( name )) != aThemeMap.end() )
        return iter->second;
    // theme not found -> add it to map
    HTHEME hTheme = vsAPI.OpenThemeData( hWnd, name );
    if( hTheme != NULL )
        aThemeMap[name] = hTheme;
    return hTheme;
}

/*
 * IsNativeControlSupported()
 *
 *  Returns TRUE if the platform supports native
 *  drawing of the control defined by nPart
 */
BOOL WinSalGraphics::IsNativeControlSupported( ControlType nType, ControlPart nPart )
{
    HTHEME hTheme = NULL;

    switch( nType )
    {
        case CTRL_PUSHBUTTON:
        case CTRL_RADIOBUTTON:
        case CTRL_CHECKBOX:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Button");
            break;
        case CTRL_SCROLLBAR:
            if( nPart == PART_DRAW_BACKGROUND_HORZ || nPart == PART_DRAW_BACKGROUND_VERT )
                return FALSE;   // no background painting needed
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Scrollbar");
            break;
        case CTRL_COMBOBOX:
            if( nPart == HAS_BACKGROUND_TEXTURE )
                return FALSE;   // we do not paint the inner part (ie the selection background/focus indication)
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Edit");
            else if( nPart == PART_BUTTON_DOWN )
                hTheme = getThemeHandle( mhWnd, L"Combobox");
            break;
        case CTRL_SPINBOX:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Edit");
            else if( nPart == PART_ALL_BUTTONS || 
                nPart == PART_BUTTON_UP || nPart == PART_BUTTON_DOWN || 
                nPart == PART_BUTTON_LEFT|| nPart == PART_BUTTON_RIGHT )
                hTheme = getThemeHandle( mhWnd, L"Spin");
            break;
        case CTRL_SPINBUTTONS:
            if( nPart == PART_ENTIRE_CONTROL || nPart == PART_ALL_BUTTONS )
                hTheme = getThemeHandle( mhWnd, L"Spin");
            break;
        case CTRL_EDITBOX:
        case CTRL_MULTILINE_EDITBOX:
            if( nPart == HAS_BACKGROUND_TEXTURE )
                return FALSE;   // we do not paint the inner part (ie the selection background/focus indication)
                //return TRUE;
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Edit");
            break;
        case CTRL_LISTBOX:
            if( nPart == HAS_BACKGROUND_TEXTURE )
                return FALSE;   // we do not paint the inner part (ie the selection background/focus indication)
            if( nPart == PART_ENTIRE_CONTROL || nPart == PART_WINDOW )
                hTheme = getThemeHandle( mhWnd, L"Listview");
            else if( nPart == PART_BUTTON_DOWN )
                hTheme = getThemeHandle( mhWnd, L"Combobox");
            break;
        case CTRL_TAB_PANE:
        case CTRL_TAB_BODY:
        case CTRL_TAB_ITEM:
        case CTRL_FIXEDBORDER:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Tab");
            break;
        case CTRL_TOOLBAR:
            if( nPart == PART_ENTIRE_CONTROL || nPart == PART_BUTTON )
                hTheme = getThemeHandle( mhWnd, L"Toolbar");
            else
                // use rebar theme for grip and background
                hTheme = getThemeHandle( mhWnd, L"Rebar");
            break;
        case CTRL_MENUBAR:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Rebar");
            break;
        case CTRL_PROGRESS:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Progress");
            break;
        default:
            hTheme = NULL;
            break;
    }

    return (hTheme != NULL);
}


/*
 * HitTestNativeControl()
 *
 *  If the return value is TRUE, bIsInside contains information whether
 *  aPos was or was not inside the native widget specified by the
 *  nType/nPart combination.
 */
BOOL WinSalGraphics::hitTestNativeControl( ControlType,
							  ControlPart,
							  const Region&,
							  const Point&,
							  SalControlHandle&,
							  BOOL& )
{
    return FALSE;
}

BOOL ImplDrawTheme( HTHEME hTheme, HDC hDC, int iPart, int iState, RECT rc, const OUString& aStr)
{
    HRESULT hr = vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);

    if( aStr.getLength() )
    {
        RECT rcContent;
        hr = vsAPI.GetThemeBackgroundContentRect( hTheme, hDC, iPart, iState, &rc, &rcContent);
        hr = vsAPI.DrawThemeText( hTheme, hDC, iPart, iState,
            reinterpret_cast<LPCWSTR>(aStr.getStr()), -1,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE,
            0, &rcContent);
    }
    return (hr == S_OK);
}


Rectangle ImplGetThemeRect( HTHEME hTheme, HDC hDC, int iPart, int iState, const Rectangle& aRect )
{
    SIZE aSz;
    RECT rc;
    rc.left = aRect.nLeft;
    rc.right = aRect.nRight;
    rc.top = aRect.nTop;
    rc.bottom = aRect.nBottom;
    HRESULT hr = vsAPI.GetThemePartSize( hTheme, hDC, iPart, iState, NULL, TS_TRUE, &aSz ); // TS_TRUE returns optimal size
    if( hr == S_OK )
        return Rectangle( 0, 0, aSz.cx, aSz.cy );
    else
        return Rectangle();
}

// Helper functions
// ----

void ImplConvertSpinbuttonValues( int nControlPart, const ControlState& rState, const Rectangle& rRect, 
                                 int* pLunaPart, int *pLunaState, RECT *pRect )
{
    if( nControlPart == PART_BUTTON_DOWN )
    {
        *pLunaPart = SPNP_DOWN;
        if( rState & CTRL_STATE_PRESSED )
            *pLunaState = DNS_PRESSED;
        else if( !(rState & CTRL_STATE_ENABLED) )
            *pLunaState = DNS_DISABLED;
        else if( rState & CTRL_STATE_ROLLOVER )
            *pLunaState = DNS_HOT;
        else
            *pLunaState = DNS_NORMAL;
    }
    if( nControlPart == PART_BUTTON_UP )
    {
        *pLunaPart = SPNP_UP;
        if( rState & CTRL_STATE_PRESSED )
            *pLunaState = UPS_PRESSED;
        else if( !(rState & CTRL_STATE_ENABLED) )
            *pLunaState = UPS_DISABLED;
        else if( rState & CTRL_STATE_ROLLOVER )
            *pLunaState = UPS_HOT;
        else
            *pLunaState = UPS_NORMAL;
    }
    if( nControlPart == PART_BUTTON_RIGHT )
    {
        *pLunaPart = SPNP_UPHORZ;
        if( rState & CTRL_STATE_PRESSED )
            *pLunaState = DNHZS_PRESSED;
        else if( !(rState & CTRL_STATE_ENABLED) )
            *pLunaState = DNHZS_DISABLED;
        else if( rState & CTRL_STATE_ROLLOVER )
            *pLunaState = DNHZS_HOT;
        else
            *pLunaState = DNHZS_NORMAL;
    }
    if( nControlPart == PART_BUTTON_LEFT )
    {
        *pLunaPart = SPNP_DOWNHORZ;
        if( rState & CTRL_STATE_PRESSED )
            *pLunaState = UPHZS_PRESSED;
        else if( !(rState & CTRL_STATE_ENABLED) )
            *pLunaState = UPHZS_DISABLED;
        else if( rState & CTRL_STATE_ROLLOVER )
            *pLunaState = UPHZS_HOT;
        else
            *pLunaState = UPHZS_NORMAL;
    }

    pRect->left   = rRect.Left();
    pRect->right  = rRect.Right()+1;
    pRect->top    = rRect.Top();
    pRect->bottom = rRect.Bottom()+1;
}

// ----

BOOL ImplDrawNativeControl(	HDC hDC, HTHEME hTheme, RECT rc,
                            ControlType nType,
							ControlPart nPart,
							ControlState nState,
							const ImplControlValue& aValue,
							SalControlHandle&,
							OUString aCaption )
{
    // a listbox dropdown is actually a combobox dropdown
    if( nType == CTRL_LISTBOX )
        if( nPart == PART_BUTTON_DOWN )
            nType = CTRL_COMBOBOX;

    // draw entire combobox as a large edit box
    if( nType == CTRL_COMBOBOX )
        if( nPart == PART_ENTIRE_CONTROL )
            nType = CTRL_EDITBOX;

    // draw entire spinbox as a large edit box
    if( nType == CTRL_SPINBOX )
        if( nPart == PART_ENTIRE_CONTROL )
            nType = CTRL_EDITBOX;

    int iPart(0), iState(0);
    if( nType == CTRL_SCROLLBAR )
    {
        HRESULT hr;
        if( nPart == PART_BUTTON_UP )
        {
            iPart = SBP_ARROWBTN;
            if( nState & CTRL_STATE_PRESSED )
                iState = ABS_UPPRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = ABS_UPDISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = ABS_UPHOT;
            else
                iState = ABS_UPNORMAL;
            hr = vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);
            return (hr == S_OK);
        }
        if( nPart == PART_BUTTON_DOWN )
        {
            iPart = SBP_ARROWBTN;
            if( nState & CTRL_STATE_PRESSED )
                iState = ABS_DOWNPRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = ABS_DOWNDISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = ABS_DOWNHOT;
            else
                iState = ABS_DOWNNORMAL;
            hr = vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);
            return (hr == S_OK);
        }
        if( nPart == PART_BUTTON_LEFT )
        {
            iPart = SBP_ARROWBTN;
            if( nState & CTRL_STATE_PRESSED )
                iState = ABS_LEFTPRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = ABS_LEFTDISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = ABS_LEFTHOT;
            else
                iState = ABS_LEFTNORMAL;
            hr = vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);
            return (hr == S_OK);
        }
        if( nPart == PART_BUTTON_RIGHT )
        {
            iPart = SBP_ARROWBTN;
            if( nState & CTRL_STATE_PRESSED )
                iState = ABS_RIGHTPRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = ABS_RIGHTDISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = ABS_RIGHTHOT;
            else
                iState = ABS_RIGHTNORMAL;
            hr = vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);
            return (hr == S_OK);
        }
        if( nPart == PART_THUMB_HORZ || nPart == PART_THUMB_VERT )
        {
            iPart = (nPart == PART_THUMB_HORZ) ? SBP_THUMBBTNHORZ : SBP_THUMBBTNVERT;
            if( nState & CTRL_STATE_PRESSED )
                iState = SCRBS_PRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = SCRBS_DISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = SCRBS_HOT;
            else
                iState = SCRBS_NORMAL;

            SIZE sz;
            vsAPI.GetThemePartSize(hTheme, hDC, iPart, iState, NULL, TS_MIN, &sz);
            vsAPI.GetThemePartSize(hTheme, hDC, iPart, iState, NULL, TS_TRUE, &sz);
            vsAPI.GetThemePartSize(hTheme, hDC, iPart, iState, NULL, TS_DRAW, &sz);

            hr = vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);
            // paint gripper on thumb if enough space
            if( ( (nPart == PART_THUMB_VERT) && (rc.bottom-rc.top > 12) ) ||
                ( (nPart == PART_THUMB_HORZ) && (rc.right-rc.left > 12) ) )
            {
                iPart = (nPart == PART_THUMB_HORZ) ? SBP_GRIPPERHORZ : SBP_GRIPPERVERT;
                iState = 0;
                vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);
            }
            return (hr == S_OK);
        }
        if( nPart == PART_TRACK_HORZ_LEFT || nPart == PART_TRACK_HORZ_RIGHT || nPart == PART_TRACK_VERT_UPPER || nPart == PART_TRACK_VERT_LOWER )
        {
            switch( nPart )
            {
                case PART_TRACK_HORZ_LEFT:  iPart = SBP_UPPERTRACKHORZ; break;
                case PART_TRACK_HORZ_RIGHT: iPart = SBP_LOWERTRACKHORZ; break;
                case PART_TRACK_VERT_UPPER: iPart = SBP_UPPERTRACKVERT; break;
                case PART_TRACK_VERT_LOWER: iPart = SBP_LOWERTRACKVERT; break;
            }

            if( nState & CTRL_STATE_PRESSED )
                iState = SCRBS_PRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = SCRBS_DISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = SCRBS_HOT;
            else
                iState = SCRBS_NORMAL;
            hr = vsAPI.DrawThemeBackground( hTheme, hDC, iPart, iState, &rc, 0);
            return (hr == S_OK);
        }
    }
    if( nType == CTRL_SPINBUTTONS && nPart == PART_ALL_BUTTONS )
    {
        SpinbuttonValue *pValue = (SpinbuttonValue*) aValue.getOptionalVal();
        if( pValue )
        {
            BOOL bOk = FALSE;

            RECT rect;
            ImplConvertSpinbuttonValues( pValue->mnUpperPart, pValue->mnUpperState, pValue->maUpperRect, &iPart, &iState, &rect );
            bOk = ImplDrawTheme( hTheme, hDC, iPart, iState, rect, aCaption);

            if( bOk )
            {
                ImplConvertSpinbuttonValues( pValue->mnLowerPart, pValue->mnLowerState, pValue->maLowerRect, &iPart, &iState, &rect );
                bOk = ImplDrawTheme( hTheme, hDC, iPart, iState, rect, aCaption);
            }

            return bOk;
        }
    }
    if( nType == CTRL_SPINBOX )
    {
        // decrease spinbutton rects a little
        //rc.right--;
        //rc.bottom--;
        if( nPart == PART_ALL_BUTTONS )
        {
            SpinbuttonValue *pValue = (SpinbuttonValue*) aValue.getOptionalVal();
            if( pValue )
            {
                BOOL bOk = FALSE;

                RECT rect;
                ImplConvertSpinbuttonValues( pValue->mnUpperPart, pValue->mnUpperState, pValue->maUpperRect, &iPart, &iState, &rect );
                bOk = ImplDrawTheme( hTheme, hDC, iPart, iState, rect, aCaption);

                if( bOk )
                {
                    ImplConvertSpinbuttonValues( pValue->mnLowerPart, pValue->mnLowerState, pValue->maLowerRect, &iPart, &iState, &rect );
                    bOk = ImplDrawTheme( hTheme, hDC, iPart, iState, rect, aCaption);
                }

                return bOk;
            }
        }

        if( nPart == PART_BUTTON_DOWN )
        {
            iPart = SPNP_DOWN;
            if( nState & CTRL_STATE_PRESSED )
                iState = DNS_PRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = DNS_DISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = DNS_HOT;
            else
                iState = DNS_NORMAL;
        }
        if( nPart == PART_BUTTON_UP )
        {
            iPart = SPNP_UP;
            if( nState & CTRL_STATE_PRESSED )
                iState = UPS_PRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = UPS_DISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = UPS_HOT;
            else
                iState = UPS_NORMAL;
        }
        if( nPart == PART_BUTTON_RIGHT )
        {
            iPart = SPNP_DOWNHORZ;
            if( nState & CTRL_STATE_PRESSED )
                iState = DNHZS_PRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = DNHZS_DISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = DNHZS_HOT;
            else
                iState = DNHZS_NORMAL;
        }
        if( nPart == PART_BUTTON_LEFT )
        {
            iPart = SPNP_UPHORZ;
            if( nState & CTRL_STATE_PRESSED )
                iState = UPHZS_PRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = UPHZS_DISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = UPHZS_HOT;
            else
                iState = UPHZS_NORMAL;
        }
        if( nPart == PART_BUTTON_LEFT || nPart == PART_BUTTON_RIGHT || nPart == PART_BUTTON_UP || nPart == PART_BUTTON_DOWN )
            return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }
    if( nType == CTRL_COMBOBOX )
    {
        if( nPart == PART_BUTTON_DOWN )
        {
            iPart = CP_DROPDOWNBUTTON;
            if( nState & CTRL_STATE_PRESSED )
                iState = CBXS_PRESSED;
            else if( !(nState & CTRL_STATE_ENABLED) )
                iState = CBXS_DISABLED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = CBXS_HOT;
            else
                iState = CBXS_NORMAL;
            return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
        }
    }
    if( nType == CTRL_PUSHBUTTON )
    {
        iPart = BP_PUSHBUTTON;
        if( nState & CTRL_STATE_PRESSED )
            iState = PBS_PRESSED;
        else if( !(nState & CTRL_STATE_ENABLED) )
            iState = PBS_DISABLED;
        else if( nState & CTRL_STATE_ROLLOVER )
            iState = PBS_HOT;
        else if( nState & CTRL_STATE_DEFAULT )
            iState = PBS_DEFAULTED;
        //else if( nState & CTRL_STATE_FOCUSED )
        //    iState = PBS_DEFAULTED;    // may need to draw focus rect 
        else
            iState = PBS_NORMAL;

        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( nType == CTRL_RADIOBUTTON )
    {
        iPart = BP_RADIOBUTTON;
        BOOL bChecked = ( aValue.getTristateVal() == BUTTONVALUE_ON );

        if( nState & CTRL_STATE_PRESSED )
            iState = bChecked ? RBS_CHECKEDPRESSED : RBS_UNCHECKEDPRESSED;
        else if( !(nState & CTRL_STATE_ENABLED) )
            iState = bChecked ? RBS_CHECKEDDISABLED : RBS_UNCHECKEDDISABLED;
        else if( nState & CTRL_STATE_ROLLOVER )
            iState = bChecked ? RBS_CHECKEDHOT : RBS_UNCHECKEDHOT;
        else
            iState = bChecked ? RBS_CHECKEDNORMAL : RBS_UNCHECKEDNORMAL;

        //if( nState & CTRL_STATE_FOCUSED )
        //    iState |= PBS_DEFAULTED;    // may need to draw focus rect 

        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( nType == CTRL_CHECKBOX )
    {
        iPart = BP_CHECKBOX;
        ButtonValue v = aValue.getTristateVal();

        if( nState & CTRL_STATE_PRESSED )
            iState = (v == BUTTONVALUE_ON)  ? CBS_CHECKEDPRESSED : 
                    ( (v == BUTTONVALUE_OFF) ? CBS_UNCHECKEDPRESSED : CBS_MIXEDPRESSED );
        else if( !(nState & CTRL_STATE_ENABLED) )
            iState = (v == BUTTONVALUE_ON)  ? CBS_CHECKEDDISABLED : 
                    ( (v == BUTTONVALUE_OFF) ? CBS_UNCHECKEDDISABLED : CBS_MIXEDDISABLED );
        else if( nState & CTRL_STATE_ROLLOVER )
            iState = (v == BUTTONVALUE_ON)  ? CBS_CHECKEDHOT : 
                    ( (v == BUTTONVALUE_OFF) ? CBS_UNCHECKEDHOT : CBS_MIXEDHOT );
        else
            iState = (v == BUTTONVALUE_ON)  ? CBS_CHECKEDNORMAL : 
                    ( (v == BUTTONVALUE_OFF) ? CBS_UNCHECKEDNORMAL : CBS_MIXEDNORMAL );

        //if( nState & CTRL_STATE_FOCUSED )
        //    iState |= PBS_DEFAULTED;    // may need to draw focus rect 

        //SIZE sz;
        //THEMESIZE eSize = TS_DRAW; // TS_MIN, TS_TRUE, TS_DRAW
        //vsAPI.GetThemePartSize( hTheme, hDC, iPart, iState, &rc, eSize, &sz);

        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( ( nType == CTRL_EDITBOX ) || ( nType == CTRL_MULTILINE_EDITBOX ) )
    {
        iPart = EP_EDITTEXT;
        if( !(nState & CTRL_STATE_ENABLED) )
            iState = ETS_DISABLED;
        else if( nState & CTRL_STATE_FOCUSED )
            iState = ETS_FOCUSED;
        else if( nState & CTRL_STATE_ROLLOVER )
            iState = ETS_HOT;
        else
            iState = ETS_NORMAL;

        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( nType == CTRL_LISTBOX )
    {
        if( nPart == PART_ENTIRE_CONTROL || nPart == PART_WINDOW )
        {
            iPart = LVP_EMPTYTEXT; // ??? no idea which part to choose here
            return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
        }
    }

    if( nType == CTRL_TAB_PANE )
    {
        iPart = TABP_PANE;
        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( nType == CTRL_FIXEDBORDER )
    {
        /*
        iPart = BP_GROUPBOX;
        if( !(nState & CTRL_STATE_ENABLED) )
            iState = GBS_DISABLED;
        else
            iState = GBS_NORMAL;
            */
        // The fixed border is only used around the tools->options tabpage where
        // TABP_PANE fits best
        iPart = TABP_PANE;
        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( nType == CTRL_TAB_BODY )
    {
        iPart = TABP_BODY;
        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( nType == CTRL_TAB_ITEM )
    {
        iPart = TABP_TABITEMLEFTEDGE;
        rc.bottom--;

        TabitemValue *pValue = (TabitemValue*) aValue.getOptionalVal();
        if( pValue )
        {
            if( pValue->isBothAligned() )
            {
                iPart = TABP_TABITEMLEFTEDGE;
                rc.right--;
            }
            else if( pValue->isLeftAligned() )
                iPart = TABP_TABITEMLEFTEDGE;
            else if( pValue->isRightAligned() )
                iPart = TABP_TABITEMRIGHTEDGE;
            else iPart = TABP_TABITEM;
        }
 
        if( !(nState & CTRL_STATE_ENABLED) )
            iState = TILES_DISABLED;
        else if( nState & CTRL_STATE_SELECTED )
        {
            iState = TILES_SELECTED;
            // increase the selected tab
            rc.left-=2;
            if( pValue && !pValue->isBothAligned() )
            {
                if( pValue->isLeftAligned() || pValue->isNotAligned() )
                    rc.right+=2;
                if( pValue->isRightAligned() )
                    rc.right+=1;
            }
            rc.top-=2;
            rc.bottom+=2;
        }
        else if( nState & CTRL_STATE_ROLLOVER )
            iState = TILES_HOT;
        else if( nState & CTRL_STATE_FOCUSED )
            iState = TILES_FOCUSED;    // may need to draw focus rect 
        else
            iState = TILES_NORMAL;
        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }

    if( nType == CTRL_TOOLBAR )
    {
        if( nPart == PART_BUTTON )
        {
            iPart = TP_BUTTON;
            BOOL bChecked = ( aValue.getTristateVal() == BUTTONVALUE_ON );
            if( !(nState & CTRL_STATE_ENABLED) )
                //iState = TS_DISABLED;
                // disabled buttons are typically not painted at all but we need visual
                // feedback when travelling by keyboard over disabled entries
                iState = TS_HOT;    
            else if( nState & CTRL_STATE_PRESSED )
                iState = TS_PRESSED;
            else if( nState & CTRL_STATE_ROLLOVER )
                iState = bChecked ? TS_HOTCHECKED : TS_HOT;
            else 
                iState = bChecked ? TS_CHECKED : TS_NORMAL;
            return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
        }
        else if( nPart == PART_THUMB_HORZ || nPart == PART_THUMB_VERT )
        {
            // the vertical gripper is not supported in most themes and it makes no 
            // sense to only support horizontal gripper
            //iPart = (nPart == PART_THUMB_HORZ) ? RP_GRIPPERVERT : RP_GRIPPER;
            //return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
        }
        else if( nPart == PART_DRAW_BACKGROUND_HORZ || nPart == PART_DRAW_BACKGROUND_VERT )
        {
            ToolbarValue *pValue = (ToolbarValue*) aValue.getOptionalVal();
            if( pValue && pValue->mbIsTopDockingArea )
                rc.top = 0; // extend potential gradient to cover menu bar as well
            return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
        }
    }

    if( nType == CTRL_MENUBAR )
    {
        if( nPart != PART_ENTIRE_CONTROL )
            return FALSE;

        MenubarValue *pValue = (MenubarValue*) aValue.getOptionalVal();
        if( pValue )
            rc.bottom += pValue->maTopDockingAreaHeight;    // extend potential gradient to cover docking area as well
        return ImplDrawTheme( hTheme, hDC, iPart, iState, rc, aCaption);
    }
    
    if( nType == CTRL_PROGRESS )
    {
        if( nPart != PART_ENTIRE_CONTROL )
            return FALSE;

        if( ! ImplDrawTheme( hTheme, hDC, PP_BAR, iState, rc, aCaption) )
            return false;
        RECT aProgressRect = rc;
        if( vsAPI.GetThemeBackgroundContentRect( hTheme, hDC, PP_BAR, iState, &rc, &aProgressRect) != S_OK )
            return false;
        
        long nProgressWidth = aValue.getNumericVal();
        nProgressWidth *= (aProgressRect.right - aProgressRect.left);
        nProgressWidth /= (rc.right - rc.left);
        if( Application::GetSettings().GetLayoutRTL() )
            aProgressRect.left = aProgressRect.right - nProgressWidth;
        else
            aProgressRect.right = aProgressRect.left + nProgressWidth;

        return ImplDrawTheme( hTheme, hDC, PP_CHUNK, iState, aProgressRect, aCaption );
    }

    return false;
}

/*
 * DrawNativeControl()
 *
 *  Draws the requested control described by nPart/nState.
 *
 *  rControlRegion:	The bounding region of the complete control in VCL frame coordinates.
 *  aValue:  		An optional value (tristate/numerical/string)
 *  rControlHandle:	Carries platform dependent data and is maintained by the WinSalGraphics implementation.
 *  aCaption:  	A caption or title string (like button text etc)
 */
BOOL WinSalGraphics::drawNativeControl(	ControlType nType,
							ControlPart nPart,
							const Region& rControlRegion,
							ControlState nState,
							const ImplControlValue& aValue,
							SalControlHandle& rControlHandle,
							const OUString& aCaption )
{
    BOOL bOk = false;
    HTHEME hTheme = NULL;

    switch( nType )
    {
        case CTRL_PUSHBUTTON:
        case CTRL_RADIOBUTTON:
        case CTRL_CHECKBOX:
            hTheme = getThemeHandle( mhWnd, L"Button");
            break;
        case CTRL_SCROLLBAR:
            hTheme = getThemeHandle( mhWnd, L"Scrollbar");
            break;
        case CTRL_COMBOBOX:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Edit");
            else if( nPart == PART_BUTTON_DOWN )
                hTheme = getThemeHandle( mhWnd, L"Combobox");
            break;
        case CTRL_SPINBOX:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Edit");
            else
                hTheme = getThemeHandle( mhWnd, L"Spin");
            break;
        case CTRL_SPINBUTTONS:
            hTheme = getThemeHandle( mhWnd, L"Spin");
            break;
        case CTRL_EDITBOX:
        case CTRL_MULTILINE_EDITBOX:
            hTheme = getThemeHandle( mhWnd, L"Edit");
            break;
        case CTRL_LISTBOX:
            if( nPart == PART_ENTIRE_CONTROL || nPart == PART_WINDOW )
                hTheme = getThemeHandle( mhWnd, L"Listview");
            else if( nPart == PART_BUTTON_DOWN )
                hTheme = getThemeHandle( mhWnd, L"Combobox");
            break;
        case CTRL_TAB_PANE:
        case CTRL_TAB_BODY:
        case CTRL_TAB_ITEM:
        case CTRL_FIXEDBORDER:
            hTheme = getThemeHandle( mhWnd, L"Tab");
            break;
        case CTRL_TOOLBAR:
            if( nPart == PART_ENTIRE_CONTROL || nPart == PART_BUTTON )
                hTheme = getThemeHandle( mhWnd, L"Toolbar");
            else
                // use rebar for grip and background
                hTheme = getThemeHandle( mhWnd, L"Rebar");
            break;
        case CTRL_MENUBAR:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Rebar");
            break;
        case CTRL_PROGRESS:
            if( nPart == PART_ENTIRE_CONTROL )
                hTheme = getThemeHandle( mhWnd, L"Progress");
            break;
        default:
            hTheme = NULL;
            break;
    }

    if( !hTheme )
        return false;

	Rectangle buttonRect = rControlRegion.GetBoundRect();
    RECT rc;
    rc.left   = buttonRect.Left();
    rc.right  = buttonRect.Right()+1;
    rc.top    = buttonRect.Top();
    rc.bottom = buttonRect.Bottom()+1;

    // set default text alignment
    int ta = SetTextAlign( mhDC, TA_LEFT|TA_TOP|TA_NOUPDATECP );

    OUString aCaptionStr( aCaption.replace('~', '&') ); // translate mnemonics
    bOk = ImplDrawNativeControl(mhDC, hTheme, rc,
                            nType, nPart, nState, aValue,
							rControlHandle, aCaptionStr );

    // restore alignment
    SetTextAlign( mhDC, ta );
   

    //GdiFlush();

	return bOk;
}


/*
 * DrawNativeControlText()
 *
 *  OPTIONAL.  Draws the requested text for the control described by nPart/nState.
 *     Used if text not drawn by DrawNativeControl().
 *
 *  rControlRegion:	The bounding region of the complete control in VCL frame coordinates.
 *  aValue:  		An optional value (tristate/numerical/string)
 *  rControlHandle:	Carries platform dependent data and is maintained by the WinSalGraphics implementation.
 *  aCaption:  	A caption or title string (like button text etc)
 */
BOOL WinSalGraphics::drawNativeControlText(	ControlType,
								ControlPart,
								const Region&,
								ControlState,
								const ImplControlValue&,
								SalControlHandle&,
								const OUString& )
{
	return( false );
}


/*
 * GetNativeControlRegion()
 *
 *  If the return value is TRUE, rNativeBoundingRegion
 *  contains the true bounding region covered by the control
 *  including any adornment, while rNativeContentRegion contains the area
 *  within the control that can be safely drawn into without drawing over
 *  the borders of the control.
 *
 *  rControlRegion:	The bounding region of the control in VCL frame coordinates.
 *  aValue:		An optional value (tristate/numerical/string)
 *  rControlHandle:	Carries platform dependent data and is maintained by the WinSalGraphics implementation.
 *  aCaption:		A caption or title string (like button text etc)
 */
BOOL WinSalGraphics::getNativeControlRegion(  ControlType nType,
								ControlPart nPart,
								const Region& rControlRegion,
								ControlState,
								const ImplControlValue&,
								SalControlHandle&,
								const OUString&,
								Region &rNativeBoundingRegion,
								Region &rNativeContentRegion )
{
    BOOL bRet = FALSE;

    HDC hDC = GetDC( mhWnd );
    if( nType == CTRL_TOOLBAR )
    {
        if( nPart == PART_THUMB_HORZ || nPart == PART_THUMB_VERT )
        {
            /*
            // the vertical gripper is not supported in most themes and it makes no 
            // sense to only support horizontal gripper

            HTHEME hTheme = getThemeHandle( mhWnd, L"Rebar");
            if( hTheme )
            {
                Rectangle aRect( ImplGetThemeRect( hTheme, hDC, nPart == PART_THUMB_HORZ ? RP_GRIPPERVERT : RP_GRIPPER, 
                    0, rControlRegion.GetBoundRect() ) );
                if( nPart == PART_THUMB_HORZ && !aRect.IsEmpty() )
                {
                    Rectangle aVertRect( 0, 0, aRect.getHeight(), aRect.getWidth() );
                    rNativeContentRegion = aVertRect;
                }
                else
                    rNativeContentRegion = aRect;
                rNativeBoundingRegion = rNativeContentRegion;
                if( !rNativeContentRegion.IsEmpty() )
                    bRet = TRUE;
            }
            */
        }
        if( nPart == PART_BUTTON )
        {
            HTHEME hTheme = getThemeHandle( mhWnd, L"Toolbar");
            if( hTheme )
            {
                Rectangle aRect( ImplGetThemeRect( hTheme, hDC, TP_SPLITBUTTONDROPDOWN, 
                    TS_HOT, rControlRegion.GetBoundRect() ) );
                rNativeContentRegion = aRect;
                rNativeBoundingRegion = rNativeContentRegion;
                if( !rNativeContentRegion.IsEmpty() )
                    bRet = TRUE;
            }
        }
    }
    if( nType == CTRL_PROGRESS && nPart == PART_ENTIRE_CONTROL )
    {
        HTHEME hTheme = getThemeHandle( mhWnd, L"Progress");
        if( hTheme )
        {
            Rectangle aRect( ImplGetThemeRect( hTheme, hDC, PP_BAR, 
                0, rControlRegion.GetBoundRect() ) );
            rNativeContentRegion = aRect;
            rNativeBoundingRegion = rNativeContentRegion;
            if( !rNativeContentRegion.IsEmpty() )
                bRet = TRUE;
        }
    }
    ReleaseDC( mhWnd, hDC );
	return( bRet );
}

