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

#ifndef _SV_SALFRAME_H
#define _SV_SALFRAME_H


#include "vcl/sv.h"
#include "vcl/salframe.hxx"
#include "vcl/sysdata.hxx"

#include "salmenu.h"
#include "saldata.hxx"
#include "aquavcltypes.h"

#include <vector>
#include <utility>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

class AquaSalGraphics;
class AquaSalFrame;
class AquaSalTimer;
class AquaSalInstance;
class AquaSalMenu;
class AquaBlinker;

typedef struct SalFrame::SalPointerState SalPointerState;

// ----------------
// - AquaSalFrame -
// ----------------

class AquaSalFrame : public SalFrame
{
public:
    NSWindow*                       mpWindow;               // Cocoa window
	NSView*                         mpView;				    // Cocoa view (actually a custom view, see below
    NSMenuItem*                     mpDockMenuEntry;        // entry in the dynamic dock menu
    NSRect                          maScreenRect;           // for mirroring purposes
	AquaSalGraphics*		        mpGraphics; 			// current frame graphics
	AquaSalFrame*			        mpParent;				// pointer to parent frame
 	SystemEnvData			        maSysData;				// system data
	int                             mnMinWidth;             // min. client width in pixels
	int                             mnMinHeight;            // min. client height in pixels
	int                             mnMaxWidth;             // max. client width in pixels
	int                             mnMaxHeight;            // max. client height in pixels
	NSRect                          maFullScreenRect;       // old window size when in FullScreen
	bool					        mbGraphics:1; 			// is Graphics used?
	bool                            mbFullScreen:1;         // is Window in FullScreen?
    bool                            mbShown:1;
    bool                            mbInitShow:1;
    bool                            mbPositioned:1;
    bool                            mbSized:1;
    bool                            mbPresentation:1;
    
    ULONG                           mnStyle;
    unsigned int                    mnStyleMask;            // our style mask from NSWindow creation
    
    ULONG                           mnLastEventTime;
    unsigned int                    mnLastModifierFlags;
    AquaSalMenu*                    mpMenu;
    
    SalExtStyle                     mnExtStyle;             // currently document frames are marked this way
    
    PointerStyle                    mePointerStyle;         // currently active pointer style
    
    NSTrackingRectTag               mnTrackingRectTag;      // used to get enter/leave messages

    CGMutablePathRef                mrClippingPath;         // used for "shaping"
    std::vector< CGRect >           maClippingRects;
    
    std::list<AquaBlinker*>         maBlinkers;
    
    Rectangle                       maInvalidRect;
    
    ULONG                           mnICOptions;
    
    boost::shared_ptr< Timer >      mpActivityTimer; // Timer to prevent system sleep during presentation
public:
    /** Constructor

        Creates a system window and connects this frame with it.

        @throws std::runtime_error in case window creation fails
    */
    AquaSalFrame( SalFrame* pParent, ULONG salFrameStyle );

    virtual ~AquaSalFrame();

    virtual SalGraphics*		GetGraphics();
    virtual void				ReleaseGraphics( SalGraphics* pGraphics );
    virtual BOOL				PostEvent( void* pData );
    virtual void				SetTitle( const XubString& rTitle );
    virtual void				SetIcon( USHORT nIcon );
    virtual void                SetMenu( SalMenu* pSalMenu );
    virtual void                DrawMenuBar();
    virtual void				Show( BOOL bVisible, BOOL bNoActivate = FALSE );
    virtual void				Enable( BOOL bEnable );
    virtual void                SetMinClientSize( long nWidth, long nHeight );
    virtual void                SetMaxClientSize( long nWidth, long nHeight );
    virtual void				SetPosSize( long nX, long nY, long nWidth, long nHeight, USHORT nFlags );
    virtual void				GetClientSize( long& rWidth, long& rHeight );
    virtual void				GetWorkArea( Rectangle& rRect );
    virtual SalFrame*			GetParent() const;
    virtual void				SetWindowState( const SalFrameState* pState );
    virtual BOOL				GetWindowState( SalFrameState* pState );
    virtual void				ShowFullScreen( BOOL bFullScreen, sal_Int32 nDisplay );
    virtual void				StartPresentation( BOOL bStart );
    virtual void				SetAlwaysOnTop( BOOL bOnTop );
    virtual void				ToTop( USHORT nFlags );
    virtual void				SetPointer( PointerStyle ePointerStyle );
    virtual void				CaptureMouse( BOOL bMouse );
    virtual void				SetPointerPos( long nX, long nY );
    virtual void				Flush( void);
    virtual void				Flush( const Rectangle& );
    virtual void				Sync();
    virtual void				SetInputContext( SalInputContext* pContext );
    virtual void				EndExtTextInput( USHORT nFlags );
    virtual String				GetKeyName( USHORT nKeyCode );
    virtual String				GetSymbolKeyName( const XubString& rFontName, USHORT nKeyCode );
    virtual BOOL                MapUnicodeToKeyCode( sal_Unicode aUnicode, LanguageType aLangType, KeyCode& rKeyCode );
    virtual LanguageType		GetInputLanguage();
    virtual SalBitmap*			SnapShot();
    virtual void				UpdateSettings( AllSettings& rSettings );
    virtual void				Beep( SoundType eSoundType );
    virtual const SystemEnvData*	GetSystemData() const;
    virtual SalPointerState		GetPointerState();
    virtual void				SetParent( SalFrame* pNewParent );
    virtual bool				SetPluginParent( SystemParentData* pNewParent );
    virtual void                SetExtendedFrameStyle( SalExtStyle );
    virtual void                SetBackgroundBitmap( SalBitmap* );
    virtual void                SetScreenNumber(unsigned int);

    // shaped system windows
    // set clip region to none (-> rectangular windows, normal state)
    virtual void ResetClipRegion();
    // start setting the clipregion consisting of nRects rectangles
    virtual void BeginSetClipRegion( ULONG nRects );
    // add a rectangle to the clip region
    virtual void UnionClipRegion( long nX, long nY, long nWidth, long nHeight );
    // done setting up the clipregion
    virtual void EndSetClipRegion();

    virtual void SetClientSize( long nWidth, long nHeight );

    void UpdateFrameGeometry();
    
    // trigger painting of the window
    void SendPaintEvent( const Rectangle* pRect = NULL );

    static bool isAlive( const AquaSalFrame* pFrame )
    { return GetSalData()->maFrameCheck.find( pFrame ) != GetSalData()->maFrameCheck.end(); }
    
    static AquaSalFrame* GetCaptureFrame() { return s_pCaptureFrame; }
    
    NSWindow* getWindow() const { return mpWindow; }
    NSView* getView() const { return mpView; }
    unsigned int getStyleMask() const { return mnStyleMask; }
    
    // actually the follwing methods do the same thing: flipping y coordinates
    // but having two of them makes clearer what the coordinate system
    // is supposed to be before and after
    void VCLToCocoa( NSRect& io_rRect, bool bRelativeToScreen = true );
    void CocoaToVCL( NSRect& io_rRect, bool bRelativeToScreen = true );

    void VCLToCocoa( NSPoint& io_rPoint, bool bRelativeToScreen = true );
    void CocoaToVCL( NSPoint& io_Point, bool bRelativeToScreen = true );
    
    NSCursor* getCurrentCursor() const;
    
    CGMutablePathRef getClipPath() const { return mrClippingPath; }
    
    // called by VCL_NSApplication to indicate screen settings have changed
    void screenParametersChanged();

 private: // methods
    /** do things on initial show (like centering on parent or on screen)
    */
    void initShow();
    
    void initWindowAndView();

 private: // data    
    static AquaSalFrame*                   s_pCaptureFrame;
    
    // make AquaSalFrame non copyable
    AquaSalFrame( const AquaSalFrame& );
    AquaSalFrame& operator=(const AquaSalFrame&);
};

#endif // _SV_SALFRAME_H
