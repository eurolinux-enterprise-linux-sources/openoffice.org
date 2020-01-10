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
#include "precompiled_xmloff.hxx"
#include <postuhdl.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmluconv.hxx>
#include <rtl/ustrbuf.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/awt/FontSlant.hpp>
#include <vcl/vclenum.hxx>

#ifndef _XMLOFF_XMLEMENT_HXX
#include <xmloff/xmlelement.hxx>
#endif

using ::rtl::OUString;
using ::rtl::OUStringBuffer;

using namespace ::com::sun::star;
using namespace ::xmloff::token;

SvXMLEnumMapEntry __READONLY_DATA aPostureGenericMapping[] =
{
	{ XML_POSTURE_NORMAL,		ITALIC_NONE		},
	{ XML_POSTURE_ITALIC,		ITALIC_NORMAL	},
	{ XML_POSTURE_OBLIQUE,		ITALIC_OBLIQUE	},
	{ XML_TOKEN_INVALID,		0 				}
};

///////////////////////////////////////////////////////////////////////////////
//
// class XMLPosturePropHdl
//

XMLPosturePropHdl::~XMLPosturePropHdl()
{
	// nothing to do
}

sal_Bool XMLPosturePropHdl::importXML( const OUString& rStrImpValue, uno::Any& rValue, const SvXMLUnitConverter& ) const
{ 
	sal_uInt16 ePosture;
	sal_Bool bRet = SvXMLUnitConverter::convertEnum( ePosture, rStrImpValue, aPostureGenericMapping );
	if( bRet )
		rValue <<= (awt::FontSlant)ePosture;

	return bRet; 
}

sal_Bool XMLPosturePropHdl::exportXML( OUString& rStrExpValue, const uno::Any& rValue, const SvXMLUnitConverter& ) const
{ 
	awt::FontSlant eSlant;

	if( !( rValue >>= eSlant ) )
	{
		sal_Int32 nValue = 0;
		
		if( !( rValue >>= nValue ) )
			return sal_False;

		eSlant = (awt::FontSlant)nValue;
	}

	OUStringBuffer aOut;
	sal_Bool bRet = SvXMLUnitConverter::convertEnum( aOut, (sal_Int32)eSlant, aPostureGenericMapping );
	if( bRet )
		rStrExpValue = aOut.makeStringAndClear();

	return bRet;
}

