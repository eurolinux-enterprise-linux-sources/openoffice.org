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
#include <com/sun/star/xml/sax/XAttributeList.hpp>
#include <xmloff/xmlimp.hxx>
#include "xmlnmspe.hxx"
#include <xmloff/nmspmap.hxx>

#ifndef _XMLOFF_ANIMIMP_HXX
#include <xmloff/formsimp.hxx>
#endif

using ::rtl::OUString;
using ::rtl::OUStringBuffer;


TYPEINIT1( XMLFormsContext, SvXMLImportContext );

XMLFormsContext::XMLFormsContext( SvXMLImport& rImport, sal_uInt16 nPrfx, const rtl::OUString& rLocalName )
: SvXMLImportContext(rImport, nPrfx, rLocalName)
{
}

XMLFormsContext::~XMLFormsContext()
{
}

SvXMLImportContext * XMLFormsContext::CreateChildContext( USHORT nPrefix, const ::rtl::OUString& rLocalName,
		const com::sun::star::uno::Reference< com::sun::star::xml::sax::XAttributeList>& xAttrList )
{
	return GetImport().GetFormImport()->createContext( nPrefix, rLocalName, xAttrList );
}
