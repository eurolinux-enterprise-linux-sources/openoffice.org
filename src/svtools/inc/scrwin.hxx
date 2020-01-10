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

#ifndef _SCRWIN_HXX
#define _SCRWIN_HXX

#include "svtools/svtdllapi.h"

#ifndef _SCRBAR_HXX //autogen
#include <vcl/scrbar.hxx>
#endif

class DataChangedEvent;

// -------------------------
// - ScrollableWindow-Type -
// -------------------------

typedef USHORT ScrollableWindowFlags;

#define SCRWIN_THUMBDRAGGING 1
#define SCRWIN_VCENTER		 2
#define SCRWIN_HCENTER		 4
#define SCRWIN_DEFAULT		 (SCRWIN_THUMBDRAGGING | SCRWIN_VCENTER | SCRWIN_HCENTER)

// --------------------
// - ScrollableWindow -
// --------------------

class SVT_DLLPUBLIC ScrollableWindow: public Window
{
private:
	Point			aPixOffset; 		// offset to virtual window (pixel)
	Size			aTotPixSz;			// total size of virtual window (pixel)
	long			nLinePixH;			// size of a line/column (pixel)
	long			nColumnPixW;

	ScrollBar		aVScroll;			// the scrollbars
	ScrollBar		aHScroll;
	ScrollBarBox	aCornerWin; 		// window in the bottom right corner
	BOOL			bScrolling:1,		// user controlled scrolling
					bHandleDragging:1,	// scroll window while dragging
					bHCenter:1,
					bVCenter:1;

#ifdef _SVT_SCRWIN_CXX
	SVT_DLLPRIVATE void			ImpInitialize( ScrollableWindowFlags nFlags );
	DECL_DLLPRIVATE_LINK( ScrollHdl, ScrollBar * );
	DECL_DLLPRIVATE_LINK( EndScrollHdl, ScrollBar * );
#endif

public:
					ScrollableWindow( Window* pParent, WinBits nBits = 0,
									  ScrollableWindowFlags = SCRWIN_DEFAULT );
					ScrollableWindow( Window* pParent, const ResId& rId,
									  ScrollableWindowFlags = SCRWIN_DEFAULT );

	virtual void	Resize();
	virtual void	Command( const CommandEvent& rCEvt );
	virtual void	DataChanged( const DataChangedEvent& rDEvt );

	virtual void	StartScroll();
	virtual void	EndScroll( long nDeltaX, long nDeltaY );

    using OutputDevice::SetMapMode;
	virtual void	SetMapMode( const MapMode& rNewMapMode );
	virtual MapMode	GetMapMode() const;

	void			SetTotalSize( const Size& rNewSize );
	Size			GetTotalSize() { return PixelToLogic( aTotPixSz ); }

	void			SetVisibleSize( const Size& rNewSize );
	BOOL			MakeVisible( const Rectangle& rTarget, BOOL bSloppy = FALSE );
	Rectangle		GetVisibleArea() const;

	void			SetLineSize( ULONG nHorz, ULONG nVert );
    using Window::Scroll;
	virtual void	Scroll( long nDeltaX, long nDeltaY, USHORT nFlags = 0 );
	void			ScrollLines( long nLinesX, long nLinesY );
	void			ScrollPages( long nPagesX, ULONG nOverlapX,
								 long nPagesY, ULONG nOverlapY );

private:
	SVT_DLLPRIVATE Size			GetOutputSizePixel() const;
	SVT_DLLPRIVATE Size			GetOutputSize() const;
};

#endif
