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

#ifndef _PRGSBAR_HXX
#define _PRGSBAR_HXX

#include "svtools/svtdllapi.h"
#include <vcl/window.hxx>

/*************************************************************************

Beschreibung
============

class ProgressBar

Diese Klasse dient zur Anzeige einer Progress-Anzeige.

--------------------------------------------------------------------------

WinBits

WB_BORDER			Border um das Fenster
WB_3DLOOK			3D-Darstellung

--------------------------------------------------------------------------

Methoden

Mit SetValue() setzt man einen Prozent-Wert zwischen 0 und 100. Wenn Werte
groesser 100 gesetzt werden, faengt das letzte Rechteck an zu blinken.

*************************************************************************/

// -----------
// - WinBits -
// -----------

#define WB_STDPROGRESSBAR		WB_BORDER

// ---------------
// - ProgressBar -
// ---------------

class SVT_DLLPUBLIC ProgressBar : public Window
{
private:
	Point				maPos;
	long				mnPrgsWidth;
	long				mnPrgsHeight;
	USHORT				mnPercent;
	USHORT				mnPercentCount;
	BOOL				mbCalcNew;

#ifdef _SV_PRGSBAR_CXX
    using Window::ImplInit;
	SVT_DLLPRIVATE void				ImplInit();
	SVT_DLLPRIVATE void				ImplInitSettings( BOOL bFont, BOOL bForeground, BOOL bBackground );
	SVT_DLLPRIVATE void				ImplDrawProgress( USHORT nOldPerc, USHORT nNewPerc );
#endif

public:
						ProgressBar( Window* pParent, WinBits nWinBits = WB_STDPROGRESSBAR );
						ProgressBar( Window* pParent, const ResId& rResId );
						~ProgressBar();

	virtual void		Paint( const Rectangle& rRect );
	virtual void		Resize();
	virtual void		StateChanged( StateChangedType nStateChange );
	virtual void		DataChanged( const DataChangedEvent& rDCEvt );

	void				SetValue( USHORT nNewPercent );
	USHORT				GetValue() const { return mnPercent; }
};

#endif	// _PRGSBAR_HXX
