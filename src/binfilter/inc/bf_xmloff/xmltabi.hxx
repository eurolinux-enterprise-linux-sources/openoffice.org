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

#ifndef _XMLOFF_XMLTABI_HXX
#define _XMLOFF_XMLTABI_HXX

#ifndef _XMLOFF_XMLELEMENTPROPERTYCONTEXT_HXX
#include "XMLElementPropertyContext.hxx"
#endif
namespace rtl
{
	class OUString;
}
namespace binfilter {

class SvxXMLTabStopArray_Impl;
class SvXMLImport;



class SvxXMLTabStopImportContext : public XMLElementPropertyContext
{
private:
	SvxXMLTabStopArray_Impl*	mpTabStops;

public:
	TYPEINFO();

	SvxXMLTabStopImportContext( SvXMLImport& rImport, sal_uInt16 nPrfx,
								const ::rtl::OUString& rLName,
								const XMLPropertyState& rProp,
				 				::std::vector< XMLPropertyState > &rProps );

	virtual ~SvxXMLTabStopImportContext();

	virtual SvXMLImportContext *CreateChildContext( sal_uInt16 nPrefix,
								   const ::rtl::OUString& rLocalName,
								   const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XAttributeList > & xAttrList );

	virtual void EndElement();
};

}//end of namespace binfilter
#endif	//  _XMLOFF_XMLTABI_HXX

