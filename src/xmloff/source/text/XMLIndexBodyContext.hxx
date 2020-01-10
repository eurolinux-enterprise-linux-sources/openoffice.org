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

#ifndef _XMLOFF_XMLINDEXBODYCONTEXT_HXX_
#define _XMLOFF_XMLINDEXBODYCONTEXT_HXX_

#include <xmloff/xmlictxt.hxx>
#include <com/sun/star/uno/Reference.h>


namespace com { namespace sun { namespace star {
	namespace xml { namespace sax { class XAttributeList; } }
	namespace beans { class XPropertySet; }
} } }
namespace rtl {	class OUString; }

/**
 * Import index body.
 *
 * This class basically delegates all CreateChildContext() calls to
 * the text import and doesn't do much else.
 */
class XMLIndexBodyContext : public SvXMLImportContext
{
    sal_Bool bHasContent;

public:

	TYPEINFO();

	XMLIndexBodyContext(
		SvXMLImport& rImport, 
		sal_uInt16 nPrfx,
		const ::rtl::OUString& rLocalName );

	~XMLIndexBodyContext();

    /// return whether any content elements were encountered
    inline sal_Bool HasContent();

protected:

	virtual SvXMLImportContext *CreateChildContext( 
		sal_uInt16 nPrefix,
		const ::rtl::OUString& rLocalName,
		const ::com::sun::star::uno::Reference< 
			::com::sun::star::xml::sax::XAttributeList> & xAttrList );
};

inline sal_Bool XMLIndexBodyContext::HasContent()
{
    return bHasContent;
}

#endif
