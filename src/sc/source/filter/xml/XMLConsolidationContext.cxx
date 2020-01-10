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
#include "precompiled_sc.hxx"



//___________________________________________________________________
#include "XMLConsolidationContext.hxx"
#include "document.hxx"
#include "rangeutl.hxx"
#include "xmlimprt.hxx"
#include "XMLConverter.hxx"
#include <xmloff/nmspmap.hxx>
#include <xmloff/xmltoken.hxx>

using ::rtl::OUString;
using namespace ::com::sun::star;
using namespace xmloff::token;


//___________________________________________________________________

ScXMLConsolidationContext::ScXMLConsolidationContext(
		ScXMLImport& rImport,
		USHORT nPrfx,
		const OUString& rLName,
		const uno::Reference< xml::sax::XAttributeList >& xAttrList ) :
	SvXMLImportContext( rImport, nPrfx, rLName ),
	eFunction( SUBTOTAL_FUNC_NONE ),
	bLinkToSource( sal_False ),
	bTargetAddr(sal_False)
{
	rImport.LockSolarMutex();
	if( !xAttrList.is() ) return;

	sal_Int16				nAttrCount		= xAttrList->getLength();
	const SvXMLTokenMap&	rAttrTokenMap	= GetScImport().GetConsolidationAttrTokenMap();

	for( sal_Int16 nIndex = 0; nIndex < nAttrCount; ++nIndex )
	{
		const rtl::OUString& sAttrName	(xAttrList->getNameByIndex( nIndex ));
		const rtl::OUString& sValue		(xAttrList->getValueByIndex( nIndex ));
		OUString aLocalName;
		USHORT nPrefix		= GetScImport().GetNamespaceMap().GetKeyByAttrName( sAttrName, &aLocalName );

		switch( rAttrTokenMap.Get( nPrefix, aLocalName ) )
		{
			case XML_TOK_CONSOLIDATION_ATTR_FUNCTION:
				eFunction = ScXMLConverter::GetSubTotalFuncFromString( sValue );
			break;
			case XML_TOK_CONSOLIDATION_ATTR_SOURCE_RANGES:
				sSourceList = sValue;
			break;
			case XML_TOK_CONSOLIDATION_ATTR_TARGET_ADDRESS:
				{
					sal_Int32 nOffset(0);
					bTargetAddr = ScRangeStringConverter::GetAddressFromString(
						aTargetAddr, sValue, GetScImport().GetDocument(), ::formula::FormulaGrammar::CONV_OOO, nOffset );
				}
				break;
			case XML_TOK_CONSOLIDATION_ATTR_USE_LABEL:
				sUseLabel = sValue;
			break;
			case XML_TOK_CONSOLIDATION_ATTR_LINK_TO_SOURCE:
				bLinkToSource = IsXMLToken(sValue, XML_TRUE);
			break;
		}
	}
}

ScXMLConsolidationContext::~ScXMLConsolidationContext()
{
}

SvXMLImportContext *ScXMLConsolidationContext::CreateChildContext(
		USHORT nPrefix,
		const OUString& rLName,
        const uno::Reference< xml::sax::XAttributeList>& /* xAttrList */ )
{
	return new SvXMLImportContext( GetImport(), nPrefix, rLName );
}

void ScXMLConsolidationContext::EndElement()
{
}

