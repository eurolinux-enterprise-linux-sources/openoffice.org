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

#ifndef _XMLOFF_SCHEMASIMPLETYPECONTEXT_HXX
#define _XMLOFF_SCHEMASIMPLETYPECONTEXT_HXX


//
// include for parent class and members
//

#include "TokenContext.hxx"
#include <com/sun/star/uno/Reference.hxx>


//
// forward declarations
//

namespace com { namespace sun { namespace star {
    namespace xml { namespace sax { class XAttributeList; } }
    namespace beans { class XPropertySet; }
    namespace xforms { class XDataTypeRepository; }
} } }
namespace rtl { class OUString; }
class SvXMLImport;
class SvXMLImportContext;

/** import the xsd:simpleType element */
class SchemaSimpleTypeContext : public TokenContext
{
    com::sun::star::uno::Reference<com::sun::star::xforms::XDataTypeRepository> mxRepository;
    rtl::OUString msTypeName;

public:

    SchemaSimpleTypeContext( SvXMLImport& rImport, 
                             USHORT nPrfx,
                             const ::rtl::OUString& rLName,
                             const com::sun::star::uno::Reference<com::sun::star::xforms::XDataTypeRepository>& rRepository );

    virtual ~SchemaSimpleTypeContext();


    //
    // implement TokenContext methods:
    //

protected:

    virtual void HandleAttribute( 
        sal_uInt16 nToken, 
        const rtl::OUString& rValue );

    virtual SvXMLImportContext* HandleChild( 
        sal_uInt16 nToken,
        sal_uInt16 nPrefix,
        const rtl::OUString& rLocalName,
        const com::sun::star::uno::Reference<com::sun::star::xml::sax::XAttributeList>& xAttrList );
};

#endif
