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
#ifndef _SVX_POSTATTR_HXX
#define _SVX_POSTATTR_HXX

// include ---------------------------------------------------------------

#include <svtools/stritem.hxx>
#include "svx/svxdllapi.h"

// class SvxPostItAuthorItem ---------------------------------------------



/*
[Beschreibung]
Dieses Item beschreibt das Autoren-Kuerzel eines Notizzettels.
*/

class SVX_DLLPUBLIC SvxPostItAuthorItem: public SfxStringItem
{
public:
	TYPEINFO();

    SvxPostItAuthorItem( USHORT nWhich  );

    SvxPostItAuthorItem( const String& rAuthor, USHORT nWhich  );
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	inline SvxPostItAuthorItem& operator=( const SvxPostItAuthorItem& rAuthor )
	{
		SetValue( rAuthor.GetValue() );
		return *this;
	}
};


// class SvxPostItDateItem -----------------------------------------------



/*
[Beschreibung]
Dieses Item beschreibt das Datum eines Notizzettels.
*/

class SVX_DLLPUBLIC SvxPostItDateItem: public SfxStringItem
{
public:
	TYPEINFO();

    SvxPostItDateItem( USHORT nWhich  );

    SvxPostItDateItem( const String& rDate, USHORT nWhich  );
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;
	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	inline SvxPostItDateItem& operator=( const SvxPostItDateItem& rDate )
	{
		SetValue( rDate.GetValue() );
		return *this;
	}
};


// class SvxPostItTextItem -----------------------------------------------



/*
[Beschreibung]
Dieses Item beschreibt den Text eines Notizzettels.
*/

class SVX_DLLPUBLIC SvxPostItTextItem: public SfxStringItem
{
public:
	TYPEINFO();

    SvxPostItTextItem( USHORT nWhich  );

    SvxPostItTextItem( const String& rText, USHORT nWhich  );
	// "pure virtual Methoden" vom SfxPoolItem
	virtual SfxItemPresentation GetPresentation( SfxItemPresentation ePres,
									SfxMapUnit eCoreMetric,
									SfxMapUnit ePresMetric,
                                    String &rText, const IntlWrapper * = 0 ) const;

	virtual SfxPoolItem*    Clone( SfxItemPool *pPool = 0 ) const;

	inline SvxPostItTextItem& operator=( const SvxPostItTextItem& rText )
	{
		SetValue( rText.GetValue() );
		return *this;
	}
};



#endif

