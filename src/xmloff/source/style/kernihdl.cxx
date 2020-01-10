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
#include "kernihdl.hxx"
#include <com/sun/star/uno/Any.hxx>
#include <rtl/ustrbuf.hxx>

// --
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmluconv.hxx>

using ::rtl::OUString;
using ::rtl::OUStringBuffer;

using namespace ::com::sun::star::uno;
using namespace ::xmloff::token;

///////////////////////////////////////////////////////////////////////////////
//
// class XMLKerningPropHdl
//

XMLKerningPropHdl::~XMLKerningPropHdl()
{
	// nothing to do
}

sal_Bool XMLKerningPropHdl::importXML( const OUString& rStrImpValue, Any& rValue, const SvXMLUnitConverter& rUnitConverter ) const
{ 
	sal_Bool bRet = sal_True;
	sal_Int32 nKerning = 0;

	if( ! IsXMLToken( rStrImpValue, XML_KERNING_NORMAL ) )
	{
		bRet = rUnitConverter.convertMeasure( nKerning, rStrImpValue );
	}

	rValue <<= (sal_Int16)nKerning;

	return bRet; 
}

sal_Bool XMLKerningPropHdl::exportXML( OUString& rStrExpValue, const Any& rValue, const SvXMLUnitConverter& rUnitConverter ) const
{ 
	sal_Bool bRet = sal_False;
	sal_Int16 nValue = sal_Int16();

	if( rValue >>= nValue )
	{
		OUStringBuffer aOut;

		if( nValue == 0 )
			aOut.append( GetXMLToken(XML_KERNING_NORMAL) );
		else
			rUnitConverter.convertMeasure( aOut, nValue );

		rStrExpValue = aOut.makeStringAndClear();

		bRet = sal_True;
	}

	return bRet;
}
