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
#include "precompiled_svtools.hxx"

#define _TASKBAR_CXX

#ifndef _TOOLS_LIST_HXX
#include <tools/list.hxx>
#endif
#include <tools/debug.hxx>
#include <vcl/help.hxx>

#include <taskbar.hxx>

// =======================================================================

TaskButtonBar::TaskButtonBar( Window* pParent, WinBits nWinStyle ) :
	ToolBox( pParent, nWinStyle | WB_3DLOOK )
{
	SetAlign( WINDOWALIGN_BOTTOM );
	SetButtonType( BUTTON_SYMBOLTEXT );
}

// -----------------------------------------------------------------------

TaskButtonBar::~TaskButtonBar()
{
}

// -----------------------------------------------------------------------

void TaskButtonBar::RequestHelp( const HelpEvent& rHEvt )
{
	ToolBox::RequestHelp( rHEvt );
}

// =======================================================================

WindowArrange::WindowArrange()
{
	mpWinList = new List;
}

// -----------------------------------------------------------------------

WindowArrange::~WindowArrange()
{
	delete mpWinList;
}

// -----------------------------------------------------------------------

static USHORT ImplCeilSqareRoot( USHORT nVal )
{
	USHORT i;

	// Ueberlauf verhindern
	if ( nVal > 0xFE * 0xFE )
		return 0xFE;

	for ( i=0; i*i < nVal; i++ )
		{}

	return i;
}

// -----------------------------------------------------------------------

static void ImplPosSizeWindow( Window* pWindow,
							   long nX, long nY, long nWidth, long nHeight )
{
	if ( nWidth < 32 )
		nWidth = 32;
	if ( nHeight < 24 )
		nHeight = 24;
	pWindow->SetPosSizePixel( nX, nY, nWidth, nHeight );
}

// -----------------------------------------------------------------------

void WindowArrange::ImplTile( const Rectangle& rRect )
{
	USHORT nCount = (USHORT)mpWinList->Count();
	if ( nCount < 3 )
	{
		ImplVert( rRect );
		return;
	}

	USHORT		i;
	USHORT		j;
	USHORT		nCols;
	USHORT		nRows;
	USHORT		nActRows;
	USHORT		nOffset;
	long		nOverWidth;
	long		nOverHeight;
	Window* 	pWindow;
	long		nX = rRect.Left();
	long		nY = rRect.Top();
	long		nWidth = rRect.GetWidth();
	long		nHeight = rRect.GetHeight();
	long		nRectY = nY;
	long		nRectWidth = nWidth;
	long		nRectHeight = nHeight;
	long		nTempWidth;
	long		nTempHeight;

	nCols	= ImplCeilSqareRoot( nCount );
	nOffset = (nCols*nCols) - nCount;
	if ( nOffset >= nCols )
	{
		nRows	 = nCols -1;
		nOffset = nOffset - nCols;
	}
	else
		nRows = nCols;

	nWidth /= nCols;
	if ( nWidth < 1 )
		nWidth = 1;
	nOverWidth = nRectWidth-(nWidth*nCols);

	pWindow = (Window*)mpWinList->First();
	for ( i = 0; i < nCols; i++ )
	{
		if ( i < nOffset )
			nActRows = nRows - 1;
		else
			nActRows = nRows;

		nTempWidth = nWidth;
		if ( nOverWidth > 0 )
		{
			nTempWidth++;
			nOverWidth--;
		}

		nHeight = nRectHeight / nActRows;
		if ( nHeight < 1 )
			nHeight = 1;
		nOverHeight = nRectHeight-(nHeight*nActRows);
		for ( j = 0; j < nActRows; j++ )
		{
			// Ueberhang verteilen
			nTempHeight = nHeight;
			if ( nOverHeight > 0 )
			{
				nTempHeight++;
				nOverHeight--;
			}
			ImplPosSizeWindow( pWindow, nX, nY, nTempWidth, nTempHeight );
			nY += nTempHeight;

			pWindow = (Window*)mpWinList->Next();
			if ( !pWindow )
				break;
		}

		nX += nWidth;
		nY = nRectY;

		if ( !pWindow )
			break;
	}
}

// -----------------------------------------------------------------------

void WindowArrange::ImplHorz( const Rectangle& rRect )
{
	long		nCount = (long)mpWinList->Count();
	long		nX = rRect.Left();
	long		nY = rRect.Top();
	long		nWidth = rRect.GetWidth();
	long		nHeight = rRect.GetHeight();
	long		nRectHeight = nHeight;
	long		nOver;
	long		nTempHeight;
	Window* 	pWindow;

	nHeight /= nCount;
	if ( nHeight < 1 )
		nHeight = 1;
	nOver = nRectHeight - (nCount*nHeight);
	pWindow = (Window*)mpWinList->First();
	while ( pWindow )
	{
		nTempHeight = nHeight;
		if ( nOver > 0 )
		{
			nTempHeight++;
			nOver--;
		}
		ImplPosSizeWindow( pWindow, nX, nY, nWidth, nTempHeight );
		nY += nTempHeight;

		pWindow = (Window*)mpWinList->Next();
	}
}

// -----------------------------------------------------------------------

void WindowArrange::ImplVert( const Rectangle& rRect )
{
	long		nCount = (long)mpWinList->Count();
	long		nX = rRect.Left();
	long		nY = rRect.Top();
	long		nWidth = rRect.GetWidth();
	long		nHeight = rRect.GetHeight();
	long		nRectWidth = nWidth;
	long		nOver;
	long		nTempWidth;
	Window* 	pWindow;

	nWidth /= nCount;
	if ( nWidth < 1 )
		nWidth = 1;
	nOver = nRectWidth - (nCount*nWidth);
	pWindow = (Window*)mpWinList->First();
	while ( pWindow )
	{
		nTempWidth = nWidth;
		if ( nOver > 0 )
		{
			nTempWidth++;
			nOver--;
		}
		ImplPosSizeWindow( pWindow, nX, nY, nTempWidth, nHeight );
		nX += nTempWidth;

		pWindow = (Window*)mpWinList->Next();
	}
}

// -----------------------------------------------------------------------

void WindowArrange::ImplCascade( const Rectangle& rRect )
{
	long		nX = rRect.Left();
	long		nY = rRect.Top();
	long		nWidth = rRect.GetWidth();
	long		nHeight = rRect.GetHeight();
	long		nRectWidth = nWidth;
	long		nRectHeight = nHeight;
	long		nOff;
	long		nCascadeWins;
	sal_Int32	nLeftBorder;
	sal_Int32	nTopBorder;
	sal_Int32	nRightBorder;
	sal_Int32	nBottomBorder;
	long		nStartOverWidth;
	long		nStartOverHeight;
	long		nOverWidth = 0;
	long		nOverHeight = 0;
	long		nTempX;
	long		nTempY;
	long		nTempWidth;
	long		nTempHeight;
	long		i;
	Window* 	pWindow;
	Window* 	pTempWindow;

	// Border-Fenster suchen um den Versatz zu ermitteln
	pTempWindow = (Window*)mpWinList->First();
	pTempWindow->GetBorder( nLeftBorder, nTopBorder, nRightBorder, nBottomBorder );
	while ( !nTopBorder )
	{
		Window* pBrdWin = pTempWindow->GetWindow( WINDOW_REALPARENT );
		if ( !pBrdWin || (pBrdWin->GetWindow( WINDOW_CLIENT ) != pTempWindow) )
			break;
		pTempWindow = pBrdWin;
		pTempWindow->GetBorder( nLeftBorder, nTopBorder, nRightBorder, nBottomBorder );
	}
	if ( !nTopBorder )
		nTopBorder = 22;
	nOff = nTopBorder;

	nCascadeWins = nRectHeight / 3 / nOff;
	if ( !nCascadeWins )
		nCascadeWins = 1;
	nWidth	 -= nCascadeWins*nOff;
	nHeight  -= nCascadeWins*nOff;
	if ( nWidth < 1 )
		nWidth = 1;
	if ( nHeight < 1 )
		nHeight = 1;

	nStartOverWidth = nRectWidth-(nWidth+(nCascadeWins*nOff));
	nStartOverHeight = nRectHeight-(nHeight+(nCascadeWins*nOff));

	i = 0;
	pWindow = (Window*)mpWinList->First();
	while ( pWindow )
	{
		if ( !i )
		{
			nOverWidth = nStartOverWidth;
			nOverHeight = nStartOverHeight;
		}

		// Position
		nTempX = nX + (i*nOff);
		nTempY = nY + (i*nOff);

		// Ueberhang verteilen
		nTempWidth = nWidth;
		if ( nOverWidth > 0 )
		{
			nTempWidth++;
			nOverWidth--;
		}
		nTempHeight = nHeight;
		if ( nOverHeight > 0 )
		{
			nTempHeight++;
			nOverHeight--;
		}

		ImplPosSizeWindow( pWindow, nTempX, nTempY, nTempWidth, nTempHeight );

		if ( i < nCascadeWins )
			i++;
		else
			i = 0;

		pWindow = (Window*)mpWinList->Next();
	}
}

// -----------------------------------------------------------------------

void WindowArrange::Arrange( USHORT nType, const Rectangle& rRect )
{
	if ( !mpWinList->Count() )
		return;

	switch ( nType )
	{
		case WINDOWARRANGE_TILE:
			ImplTile( rRect );
			break;
		case WINDOWARRANGE_HORZ:
			ImplHorz( rRect );
			break;
		case WINDOWARRANGE_VERT:
			ImplVert( rRect );
			break;
		case WINDOWARRANGE_CASCADE:
			ImplCascade( rRect );
			break;
	}
}

