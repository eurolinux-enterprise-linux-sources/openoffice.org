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

#ifndef _XMLOFF_METATCONTEXT_HXX
#define _XMLOFF_METATCONTEXT_HXX

#include <rtl/ref.hxx>
#include "functional.hxx"

#include <map>

#ifndef _XMLOFF_FLATTRCONTEXT_HXX
#include "FlatTContext.hxx"
#endif

typedef ::std::multimap< ::rtl::OUString, 
					::rtl::Reference< XMLPersTextContentTContext >,
					less_functor > XMLMetaContexts_Impl;


class XMLMetaTransformerContext : public XMLTransformerContext 
{
	XMLMetaContexts_Impl m_aContexts;	

public:
	TYPEINFO();

	// A contexts constructor does anything that is required if an element
	// starts. Namespace processing has been done already.
	// Note that virtual methods cannot be used inside constructors. Use
	// StartElement instead if this is required.
	XMLMetaTransformerContext( XMLTransformerBase& rTransformer, 
						   const ::rtl::OUString& rQName );

	// A contexts destructor does anything that is required if an element
	// ends. By default, nothing is done.
	// Note that virtual methods cannot be used inside destructors. Use
	// EndElement instead if this is required.
	virtual ~XMLMetaTransformerContext();

	// Create a childs element context. By default, the import's
	// CreateContext method is called to create a new default context.
	virtual XMLTransformerContext *CreateChildContext( sal_uInt16 nPrefix,
								   const ::rtl::OUString& rLocalName,
								   const ::rtl::OUString& rQName,
								   const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XAttributeList >& xAttrList );

	// EndElement is called before a context will be destructed, but
	// after a elements context has been parsed. It may be used for actions
	// that require virtual methods. The default is to do nothing.
	virtual void EndElement();

	// This method is called for all characters that are contained in the
	// current element. The default is to ignore them.
	virtual void Characters( const ::rtl::OUString& rChars );
};

#endif	//  _XMLOFF_METATCONTEXT_HXX

