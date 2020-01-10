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

#include <vcl/window.hxx>

#define MARKER_NOMARKER	0xFFFF


class SbModule;
class BreakpointListe;
struct Breakpoint;
class ImageList;

DECLARE_LIST( BreakpointList, Breakpoint* )

class BreakpointWindow : public Window, public BreakpointList
{
using Window::Scroll;

public:
	BreakpointWindow( Window *pParent );
//	~BreakpointWindow();

	void		Reset();

	void		SetModule( SbModule *pMod );
	void		SetBPsInModule();

	void		InsertBreakpoint( USHORT nLine );
	void		ToggleBreakpoint( USHORT nLine );
	void		AdjustBreakpoints( ULONG nLine, BOOL bInserted );

	void		LoadBreakpoints( String aFilename );
	void		SaveBreakpoints( String aFilename );

protected:
	Breakpoint*	FindBreakpoint( ULONG nLine );

private:
	long			nCurYOffset;
	USHORT			nMarkerPos;
	SbModule*		pModule;
	BOOL			bErrorMarker;
	static ImageList *pImages;

protected:
	virtual void	Paint( const Rectangle& );
	Breakpoint* 	FindBreakpoint( const Point& rMousePos );
	void			ShowMarker( BOOL bShow );
	virtual void	MouseButtonDown( const MouseEvent& rMEvt );

public:

//	void			SetModulWindow( ModulWindow* pWin )
//						{ pModulWindow = pWin; }

	void			SetMarkerPos( USHORT nLine, BOOL bErrorMarker = FALSE );

    virtual void        Scroll( long nHorzScroll, long nVertScroll,
                                USHORT nFlags = 0 );
	long&			GetCurYOffset() 		{ return nCurYOffset; }
};








