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
#ifndef _SVX_RELFLD_HXX
#define _SVX_RELFLD_HXX

// include ---------------------------------------------------------------

#ifndef _FIELD_HXX //autogen
#include <vcl/field.hxx>
#endif
#include "svx/svxdllapi.h"


// class SvxRelativeField ------------------------------------------------
/*
	[Beschreibung]

	"Ahnlich der Klasse FontSizeBox. Abgeleitet von der Klasse MetricField.
	Zus"atzliche Funktionalit"at: relative Angaben.
*/

class SVX_DLLPUBLIC SvxRelativeField : public MetricField
{
private:
	USHORT          nRelMin;
	USHORT          nRelMax;
	USHORT          nRelStep;
	BOOL            bRelativeMode;
	BOOL            bRelative;
    BOOL            bNegativeEnabled;

protected:
	void            Modify();

public:
	SvxRelativeField( Window* pParent, WinBits nWinStyle = 0 );
	SvxRelativeField( Window* pParent, const ResId& rResId );

	void            EnableRelativeMode( USHORT nMin = 50, USHORT nMax = 150,
										USHORT nStep = 5 );
	BOOL            IsRelativeMode() const { return bRelativeMode; }
	void            SetRelative( BOOL bRelative = FALSE );
	BOOL            IsRelative() const { return bRelative; }
    void            EnableNegativeMode() {bNegativeEnabled = TRUE;}
};


#endif

