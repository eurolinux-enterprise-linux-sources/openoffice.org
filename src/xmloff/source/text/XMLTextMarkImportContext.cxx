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


#include "XMLTextMarkImportContext.hxx"


#include <rtl/ustring.hxx>
#include <tools/debug.hxx>
#include <xmloff/xmluconv.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmlimp.hxx>
#include <xmloff/nmspmap.hxx>
#include "xmlnmspe.hxx"
#include <com/sun/star/xml/sax/XAttributeList.hpp>
#include <com/sun/star/text/XTextContent.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/rdf/XMetadatable.hpp>

#include <com/sun/star/text/XFormField.hpp>


using ::rtl::OUString;
using ::rtl::OUStringBuffer;

using namespace ::com::sun::star::text;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::xml::sax;
using namespace ::xmloff::token;


XMLFieldParamImportContext::XMLFieldParamImportContext(
	SvXMLImport& rImport, 
	XMLTextImportHelper& rHlp,
	sal_uInt16 nPrefix,
	const OUString& rLocalName ) :
		SvXMLImportContext(rImport, nPrefix, rLocalName),
		rHelper(rHlp)
{
}


void XMLFieldParamImportContext::StartElement(const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XAttributeList> & xAttrList)
{
	SvXMLImport& rImport = GetImport();
	::rtl::OUString sName;
	::rtl::OUString sValue;

	sal_Int16 nLength = xAttrList->getLength();
	for(sal_Int16 nAttr = 0; nAttr < nLength; nAttr++)
	{
		OUString sLocalName;
		sal_uInt16 nPrefix = rImport.GetNamespaceMap().
			GetKeyByAttrName( xAttrList->getNameByIndex(nAttr), 
							  &sLocalName );

		if ( (XML_NAMESPACE_FIELD == nPrefix) &&
			 IsXMLToken(sLocalName, XML_NAME)   )
		{
			sName = xAttrList->getValueByIndex(nAttr);
		}
		if ( (XML_NAMESPACE_FIELD == nPrefix) &&
			 IsXMLToken(sLocalName, XML_VALUE)   )
		{
			sValue = xAttrList->getValueByIndex(nAttr);
		}
	}
	if (rHelper.hasCurrentFieldCtx() && sName.getLength()>0) {
		rHelper.addFieldParam(sName, sValue);
	}
}


TYPEINIT1( XMLTextMarkImportContext, SvXMLImportContext);

XMLTextMarkImportContext::XMLTextMarkImportContext(
	SvXMLImport& rImport, 
	XMLTextImportHelper& rHlp,
	sal_uInt16 nPrefix,
    const OUString& rLocalName )
    : SvXMLImportContext(rImport, nPrefix, rLocalName)
    , m_rHelper(rHlp)
    , m_bHaveAbout(false)
{
}

enum lcl_MarkType { TypeReference, TypeReferenceStart, TypeReferenceEnd,
					TypeBookmark, TypeBookmarkStart, TypeBookmarkEnd,
					TypeFieldmark, TypeFieldmarkStart, TypeFieldmarkEnd
				  };

static SvXMLEnumMapEntry __READONLY_DATA lcl_aMarkTypeMap[] =
{
	{ XML_REFERENCE_MARK,			TypeReference },
	{ XML_REFERENCE_MARK_START,	    TypeReferenceStart },
	{ XML_REFERENCE_MARK_END,		TypeReferenceEnd },
	{ XML_BOOKMARK,				    TypeBookmark },
	{ XML_BOOKMARK_START,			TypeBookmarkStart },
	{ XML_BOOKMARK_END,			    TypeBookmarkEnd },
	{ XML_FIELDMARK,				TypeFieldmark },
	{ XML_FIELDMARK_START,			TypeFieldmarkStart },
	{ XML_FIELDMARK_END,			TypeFieldmarkEnd },
	{ XML_TOKEN_INVALID,    		0 },
};

void XMLTextMarkImportContext::StartElement(
	const Reference<XAttributeList> & xAttrList)
{
    if (!FindName(GetImport(), xAttrList))
    {
        m_sBookmarkName = OUString();
    }

	if (IsXMLToken(GetLocalName(), XML_FIELDMARK_END))
    {
        m_sBookmarkName = m_rHelper.FindActiveBookmarkName();
    }

    if (IsXMLToken(GetLocalName(), XML_FIELDMARK_START) || IsXMLToken(GetLocalName(), XML_FIELDMARK))
    {
        if (m_sBookmarkName.getLength() == 0) 
        {
            m_sBookmarkName = ::rtl::OUString::createFromAscii("Unknown");
        }
        m_rHelper.pushFieldCtx( m_sBookmarkName, m_sFieldName );
	}
}

void XMLTextMarkImportContext::EndElement()
{
	SvXMLImportContext::EndElement();

	const OUString sAPI_reference_mark(
		RTL_CONSTASCII_USTRINGPARAM("com.sun.star.text.ReferenceMark"));
	const OUString sAPI_bookmark(
		RTL_CONSTASCII_USTRINGPARAM("com.sun.star.text.Bookmark"));
	const OUString sAPI_fieldmark(
		RTL_CONSTASCII_USTRINGPARAM("com.sun.star.text.Fieldmark"));
	const OUString sAPI_formfieldmark(
		RTL_CONSTASCII_USTRINGPARAM("com.sun.star.text.FormFieldmark"));

    if (m_sBookmarkName.getLength() > 0)
	{
		sal_uInt16 nTmp;
		if (SvXMLUnitConverter::convertEnum(nTmp, GetLocalName(), 
											lcl_aMarkTypeMap))
		{
			switch ((lcl_MarkType)nTmp)
			{
				case TypeReference:
					// export point reference mark
					CreateAndInsertMark(GetImport(),
                        sAPI_reference_mark,
                        m_sBookmarkName,
                        m_rHelper.GetCursorAsRange()->getStart(),
                        ::rtl::OUString());
					break;

				case TypeFieldmark:
				case TypeBookmark:
					{
						bool bImportAsField=((lcl_MarkType)nTmp==TypeFieldmark && m_sFieldName.compareToAscii("msoffice.field.FORMCHECKBOX")==0); // for now only import FORMCHECKBOX boxes
						// export point bookmark
                        const Reference<XInterface> xContent(
                            CreateAndInsertMark(GetImport(),
										(bImportAsField?sAPI_formfieldmark:sAPI_bookmark),
                                m_sBookmarkName,
                                m_rHelper.GetCursorAsRange()->getStart(),
                                m_sXmlId) );
                        if (m_bHaveAbout)
                        {
                            const Reference<com::sun::star::rdf::XMetadatable>
                                xMeta( xContent, UNO_QUERY);
                            GetImport().AddRDFa(xMeta,
                                m_sAbout, m_sProperty, m_sContent, m_sDatatype);
                        }
						if ((lcl_MarkType)nTmp==TypeFieldmark) {
                            if (xContent.is() && bImportAsField) {
								// setup fieldmark...
                                Reference< ::com::sun::star::text::XFormField> xFormField(xContent, UNO_QUERY);
								xFormField->setType(1); // Checkbox...
                                if (xFormField.is() && m_rHelper.hasCurrentFieldCtx()) {
//									xFormField->setDescription(::rtl::OUString::createFromAscii("HELLO CHECKBOX"));
//									xFormField->setRes(1); 
                                    m_rHelper.setCurrentFieldParamsTo(xFormField);
								}
							}
                            m_rHelper.popFieldCtx();
						}
					}
					break;

				case TypeFieldmarkStart:
				case TypeBookmarkStart:
					// save XTextRange for later construction of bookmark
                    m_rHelper.InsertBookmarkStartRange(
                        m_sBookmarkName, m_rHelper.GetCursorAsRange()->getStart(),
                        m_sXmlId);
					break;

				case TypeFieldmarkEnd:
				case TypeBookmarkEnd:
				{
					// get old range, and construct
					Reference<XTextRange> xStartRange;
                    if (m_rHelper.FindAndRemoveBookmarkStartRange(m_sBookmarkName,
                            xStartRange, m_sXmlId))
					{
						Reference<XTextRange> xEndRange(
                            m_rHelper.GetCursorAsRange()->getStart());

						// check if beginning and end are in same XText
						if (xStartRange->getText() == xEndRange->getText())
						{
							// create range for insertion
							Reference<XTextCursor> xInsertionCursor =
                                m_rHelper.GetText()->createTextCursorByRange(
									xEndRange);
							xInsertionCursor->gotoRange(xStartRange, sal_True);

							//DBG_ASSERT(! xInsertionCursor->isCollapsed(), 
							// 				"we want no point mark");
							// can't assert, because someone could
							// create a file with subsequence
							// start/end elements

							Reference<XTextRange> xInsertionRange(
								xInsertionCursor, UNO_QUERY);

                            bool bImportAsField=((lcl_MarkType)nTmp==TypeFieldmarkEnd && m_rHelper.hasCurrentFieldCtx());
							if (bImportAsField) {
								::rtl::OUString currentFieldType =
                                    m_rHelper.getCurrentFieldType();
								bImportAsField=currentFieldType.compareToAscii("msoffice.field.FORMTEXT")==0; // for now only import FORMTEXT boxes
							}

							// insert reference
                            const Reference<XInterface> xContent(
                                CreateAndInsertMark(GetImport(),
												(bImportAsField?sAPI_fieldmark:sAPI_bookmark),
                                    m_sBookmarkName,
                                    xInsertionRange,
                                    m_sXmlId) );
                            if (m_bHaveAbout)
                            {
                                const Reference<com::sun::star::rdf::XMetadatable>
                                    xMeta( xContent, UNO_QUERY);
                                GetImport().AddRDFa(xMeta,
                                    m_sAbout, m_sProperty, m_sContent, m_sDatatype);
                            }

							if ((lcl_MarkType)nTmp==TypeFieldmarkEnd) {
                                if (xContent.is() && bImportAsField) {
									// setup fieldmark...
                                    Reference< ::com::sun::star::text::XFormField> xFormField(xContent, UNO_QUERY);
									xFormField->setType(0); // Text
                                    if (xFormField.is() && m_rHelper.hasCurrentFieldCtx()) {
                                        m_rHelper.setCurrentFieldParamsTo(xFormField);
//									xFormField->setDescription(::rtl::OUString::createFromAscii("HELLO"));
									}
								}
								m_rHelper.popFieldCtx();
							}
						}
						// else: beginning/end in different XText -> ignore!
					}
					// else: no start found -> ignore!
					break;
				}

				case TypeReferenceStart:
				case TypeReferenceEnd:	
					DBG_ERROR("reference start/end are handled in txtparai !");
					break;

				default:
					DBG_ERROR("unknown mark type");
					break;
			}
		}
	}
}

SvXMLImportContext *XMLTextMarkImportContext::CreateChildContext( USHORT nPrefix,
                                        const ::rtl::OUString& rLocalName,
                                        const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XAttributeList >&  )
{
    return new XMLFieldParamImportContext(GetImport(), m_rHelper,
                nPrefix, rLocalName);
}


Reference<XTextContent> XMLTextMarkImportContext::CreateAndInsertMark(
    SvXMLImport& rImport,
    const OUString& sServiceName,
    const OUString& sMarkName,
    const Reference<XTextRange> & rRange,
    const OUString& i_rXmlId)
{
    // create mark
    const Reference<XMultiServiceFactory> xFactory(rImport.GetModel(),
        UNO_QUERY);
    Reference<XInterface> xIfc;

    if (xFactory.is())
    {
        xIfc = xFactory->createInstance(sServiceName);

        if (!xIfc.is())
        {
            OSL_ENSURE(false, "CreateAndInsertMark: cannot create service?");
            return 0;
        }

        // set name (unless there is no name (text:meta))
        const Reference<XNamed> xNamed(xIfc, UNO_QUERY);
        if (xNamed.is())
        {
            xNamed->setName(sMarkName);
        }
        else
        {
            if (sMarkName.getLength())
            {
                OSL_ENSURE(false, "name given, but XNamed not supported?");
                return 0;
            }
        }

        // cast to XTextContent and attach to document
        const Reference<XTextContent> xTextContent(xIfc, UNO_QUERY);
        if (xTextContent.is())
        {
            try
            {
                // if inserting marks, bAbsorb==sal_False will cause
                // collapsing of the given XTextRange.
                rImport.GetTextImport()->GetText()->insertTextContent(rRange,
                    xTextContent, sal_True);

                // xml:id for RDF metadata -- after insertion!
                rImport.SetXmlId(xIfc, i_rXmlId);

                return xTextContent;
            }
            catch (com::sun::star::lang::IllegalArgumentException &)
            {
                OSL_ENSURE(false, "CreateAndInsertMark: cannot insert?");
                return 0;
            }
        }
    }
    return 0;
}

sal_Bool XMLTextMarkImportContext::FindName(
	SvXMLImport& rImport,
    const Reference<XAttributeList> & xAttrList)
{
	sal_Bool bNameOK = sal_False;

	// find name attribute first
    const sal_Int16 nLength = xAttrList->getLength();
	for(sal_Int16 nAttr = 0; nAttr < nLength; nAttr++)
	{
		OUString sLocalName;
        const sal_uInt16 nPrefix = rImport.GetNamespaceMap().
			GetKeyByAttrName( xAttrList->getNameByIndex(nAttr), 
							  &sLocalName );

		if ( (XML_NAMESPACE_TEXT == nPrefix) &&
			 IsXMLToken(sLocalName, XML_NAME)   )
		{
            m_sBookmarkName = xAttrList->getValueByIndex(nAttr);
			bNameOK = sal_True;
		}
		else if ( (XML_NAMESPACE_XML == nPrefix) &&
			 IsXMLToken(sLocalName, XML_ID)   )
		{
            m_sXmlId = xAttrList->getValueByIndex(nAttr);
		}
        else if ( XML_NAMESPACE_XHTML == nPrefix )
        {
            // RDFa
            if ( IsXMLToken( sLocalName, XML_ABOUT) )
            {
                m_sAbout = xAttrList->getValueByIndex(nAttr);
                m_bHaveAbout = true;
            }
            else if ( IsXMLToken( sLocalName, XML_PROPERTY) )
            {
                m_sProperty = xAttrList->getValueByIndex(nAttr);
            }
            else if ( IsXMLToken( sLocalName, XML_CONTENT) )
            {
                m_sContent = xAttrList->getValueByIndex(nAttr);
            }
            else if ( IsXMLToken( sLocalName, XML_DATATYPE) )
            {
                m_sDatatype = xAttrList->getValueByIndex(nAttr);
            }
        }
        else if ( (XML_NAMESPACE_FIELD == nPrefix) &&
			 IsXMLToken(sLocalName, XML_TYPE)   )
		{
            m_sFieldName = xAttrList->getValueByIndex(nAttr);
		}
	}

	return bNameOK;
}

