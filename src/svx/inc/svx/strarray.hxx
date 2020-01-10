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
#ifndef _SVX_STRARRAY_HXX
#define _SVX_STRARRAY_HXX

// include ---------------------------------------------------------------

#ifndef _RESARY_HXX //autogen
#include <tools/resary.hxx>
#endif
#include "svx/svxdllapi.h"

//	class SvxStringArray -------------------------------------------------

class SVX_DLLPUBLIC SvxStringArray : public ResStringArray
{
public:
	SvxStringArray( sal_uInt32 nResId );
	SvxStringArray( const ResId& rResId );
	~SvxStringArray();

	const String&	GetStringByPos( sal_uInt32 nPos ) const;
	const String&	GetStringByType( long nType ) const;
	long			GetValueByStr( const String& rStr ) const;
};


#endif

