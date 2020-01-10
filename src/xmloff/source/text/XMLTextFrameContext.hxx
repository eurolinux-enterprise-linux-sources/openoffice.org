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

#ifndef _XMLTEXTFRAMECONTEXT_HXX
#define _XMLTEXTFRAMECONTEXT_HXX

#ifndef _COM_SUN_STAR_TEXT_TEXTCONTENTANCHORTYPE_HPP
#include <com/sun/star/text/TextContentAnchorType.hpp>
#endif
#include <xmloff/xmlictxt.hxx>

namespace com { namespace sun { namespace star {
	namespace text { class XTextCursor; class XTextContent; }
} } }

class SvXMLAttributeList;
class XMLTextFrameContextHyperlink_Impl;

class XMLTextFrameContext : public SvXMLImportContext
{
	::com::sun::star::uno::Reference<
		::com::sun::star::xml::sax::XAttributeList > m_xAttrList;

    SvXMLImportContextRef m_xImplContext;
    SvXMLImportContextRef m_xReplImplContext;
	SvXMLAttributeList *m_pAttrList;

	XMLTextFrameContextHyperlink_Impl	*m_pHyperlink;
    // --> OD 2009-07-22 #i73249#
    ::rtl::OUString m_sTitle;
    // <--
	::rtl::OUString	m_sDesc;

	::com::sun::star::text::TextContentAnchorType 	m_eDefaultAnchorType;

    // --> OD 2006-03-10 #i51726#
    // The <draw:name> can longer be used to distinguish Writer graphic/text box
    // objects and Draw graphic/text box objects.
    // The new distinguish attribute is the parent style of the automatic style
    // of the object. All Draw objects have an automatic style without a parent style.
    sal_Bool m_HasAutomaticStyleWithoutParentStyle;
    // <--
	sal_Bool m_bSupportsReplacement;

	sal_Bool CreateIfNotThere();
	sal_Bool CreateIfNotThere( ::com::sun::star::uno::Reference <
		::com::sun::star::beans::XPropertySet >& rPropSet );

public:

	TYPEINFO();

	XMLTextFrameContext( SvXMLImport& rImport,
			sal_uInt16 nPrfx,
			const ::rtl::OUString& rLName,
			const ::com::sun::star::uno::Reference<
				::com::sun::star::xml::sax::XAttributeList > & xAttrList,
			::com::sun::star::text::TextContentAnchorType eDfltAnchorType );
	virtual ~XMLTextFrameContext();

	virtual void EndElement();

	SvXMLImportContext *CreateChildContext( sal_uInt16 nPrefix,
				const ::rtl::OUString& rLocalName,
			 	const ::com::sun::star::uno::Reference<
					::com::sun::star::xml::sax::XAttributeList > & xAttrList );

	void SetHyperlink( const ::rtl::OUString& rHRef,
					   const ::rtl::OUString& rName,
					   const ::rtl::OUString& rTargetFrameName,
					   sal_Bool bMap );

	::com::sun::star::text::TextContentAnchorType GetAnchorType() const;

	::com::sun::star::uno::Reference <
		::com::sun::star::text::XTextContent > GetTextContent() const;
    // --> OD 2004-08-24 #i33242#
    ::com::sun::star::uno::Reference <
        ::com::sun::star::drawing::XShape > GetShape() const;
    // <--
};


#endif
