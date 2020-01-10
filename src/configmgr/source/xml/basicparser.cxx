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
#include "precompiled_configmgr.hxx"

#include "basicparser.hxx"
#include <com/sun/star/xml/sax/SAXException.hpp>
#include "valuetypeconverter.hxx"
// -----------------------------------------------------------------------------

namespace configmgr
{
// -----------------------------------------------------------------------------
    namespace xml
    {
// -----------------------------------------------------------------------------
	    namespace uno		= ::com::sun::star::uno;
	    namespace sax		= ::com::sun::star::xml::sax;
// -----------------------------------------------------------------------------

namespace
{
    static inline
    uno::Reference< script::XTypeConverter > createTCV(uno::Reference< uno::XComponentContext > const & _xContext)
    {
        OSL_ENSURE(_xContext.is(),"Cannot create Parser without a Context");

        static const rtl::OUString k_sTCVService(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.script.Converter"));

        uno::Reference< lang::XMultiComponentFactory > xSvcFactory = _xContext->getServiceManager();
        return uno::Reference< script::XTypeConverter >::query(xSvcFactory->createInstanceWithContext(k_sTCVService,_xContext));
    }
}
// -----------------------------------------------------------------------------

struct BasicParser::ValueData : ValueConverter
{
    rtl::OUString    content;
    rtl::OUString    locale;
    bool    isLocalized;

    ValueData(uno::Type const& _aType, uno::Reference< script::XTypeConverter > const & _xTCV)
    : ValueConverter(_aType, _xTCV)
    , content()
    , locale()
    , isLocalized(false)
    {
    }

    uno::Any convertToAny() const
    {
        return ValueConverter::convertToAny(this->content);
    }

    rtl::OUString toString() const
    {
        return this->content;
    }

    uno::Sequence<rtl::OUString> toStringList() const
    {
        return ValueConverter::splitStringList(this->content);
    }

    void setLocalized(rtl::OUString const & _aLocale)
    {
        isLocalized = true;
        locale = _aLocale;
    }
};
// -----------------------------------------------------------------------------

BasicParser::BasicParser(uno::Reference< uno::XComponentContext > const & _xContext)
: m_xTypeConverter( createTCV(_xContext) )
, m_xLocator(NULL)
, m_aDataParser(Logger(_xContext))
, m_aNodes()
, m_aValueType()
, m_pValueData(NULL)
, m_nSkipLevels(0)
, m_bEmpty(true)
, m_bInProperty(false)
{
    if (!m_xTypeConverter.is())
        throw uno::RuntimeException();

    OSL_DEBUG_ONLY( dbgUpdateLocation() );
}
// -----------------------------------------------------------------------------

BasicParser::~BasicParser()
{
    delete m_pValueData;
}
// -----------------------------------------------------------------------------

#if OSL_DEBUG_LEVEL > 0
void BasicParser::dbgUpdateLocation()
{
#ifndef DBG_UTIL
    rtl::OUString  dbgPublicId,    dbgSystemId;
    sal_Int32 dbgLineNo,      dbgColumnNo;
#endif // OSL_DEBUG_LEVEL

    if (m_xLocator.is())
    {
        dbgPublicId   = m_xLocator->getPublicId();
        dbgSystemId   = m_xLocator->getSystemId();
        dbgLineNo     = m_xLocator->getLineNumber();
        dbgColumnNo   = m_xLocator->getColumnNumber();
    }
    else
    {
        dbgPublicId = dbgSystemId = rtl::OUString::createFromAscii("<<<unknown>>>");
        dbgLineNo = dbgColumnNo = -1;
    }
}
#endif
// -----------------------------------------------------------------------------
void SAL_CALL BasicParser::startDocument(  )
        throw (sax::SAXException, uno::RuntimeException)
{
    m_aDataParser.reset();
    m_aValueType    = uno::Type();
    m_bInProperty   = false;
    m_nSkipLevels   = 0;

    delete m_pValueData, m_pValueData = NULL;

    while (!m_aNodes.empty()) m_aNodes.pop();

    m_bEmpty = true;

    OSL_DEBUG_ONLY( dbgUpdateLocation() );
}
// -----------------------------------------------------------------------------

void SAL_CALL BasicParser::endDocument(  ) throw (sax::SAXException, uno::RuntimeException)
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    if (!m_aNodes.empty() || isSkipping() || isInValueData())
        raiseParseException( "Configuration XML Parser - Invalid XML: Unexpected end of document" );

    m_xLocator.clear();
}
// -----------------------------------------------------------------------------

void SAL_CALL BasicParser::characters( const rtl::OUString& aChars )
        throw (sax::SAXException, uno::RuntimeException)
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    if (isInValueData())
    {
        m_pValueData->content += aChars;
    }
#ifdef CONFIG_XMLPARSER_VALIDATE_WHITESPACE
    else
        OSL_ENSURE( isSkipping() || aChars.trim().getLength() == 0, "Unexpected text content in configuration XML");
#endif
}
// -----------------------------------------------------------------------------

void SAL_CALL BasicParser::ignorableWhitespace( const rtl::OUString& aWhitespaces )
        throw (sax::SAXException, uno::RuntimeException)
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );
    if (isInValueData())
    {
        OSL_ENSURE(false, "Configuration XML: Unexpected ignorable (!) whitespace instruction in value data");
        if (!m_pValueData->isNull())
            m_pValueData->content += aWhitespaces;
    }
#ifdef CONFIG_XMLPARSER_VALIDATE_WHITESPACE
    else
        OSL_ENSURE( aChars.trim().getLength() == 0, "Unexpected non-space content in ignorable whitespace");
#endif
}
// -----------------------------------------------------------------------------

void SAL_CALL BasicParser::processingInstruction( const rtl::OUString& /*aTarget*/, const rtl::OUString& /*aData*/ )
        throw (sax::SAXException, uno::RuntimeException)
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );
    OSL_ENSURE(false, "Unexpected processing instruction in Configuration XML");
}
// -----------------------------------------------------------------------------

void SAL_CALL BasicParser::setDocumentLocator( const uno::Reference< sax::XLocator >& xLocator )
        throw (sax::SAXException, uno::RuntimeException)
{
    m_xLocator = xLocator;
    OSL_DEBUG_ONLY( dbgUpdateLocation() );
}
// -----------------------------------------------------------------------------

void BasicParser::startNode( ElementInfo const & aInfo, const uno::Reference< sax::XAttributeList >& /*xAttribs*/ )
{
    { (void)aInfo; }
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    OSL_ENSURE( !isSkipping(), "While skipping, call startSkipping() instead of startNode()");
    OSL_ENSURE( aInfo.type != ElementType::property, "For properties, call startProperty() instead of startNode()");

    if (isInProperty())
        raiseParseException( "Configuration XML Parser - Invalid Data: Cannot have a node nested in a property" );

    m_aNodes.push(aInfo);
    m_bEmpty = (aInfo.flags != 0) || (aInfo.op > Operation::modify);

    OSL_POSTCOND( isInNode(), "Could not start a node ");
}
// -----------------------------------------------------------------------------

void BasicParser::endNode( )
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    OSL_ENSURE( !isSkipping(), "While skipping, honor wasSkipping() instead of calling endNode()");
    OSL_ENSURE( !isInProperty(), "For properties, call endProperty() instead of endNode()" );

    ensureInNode();

    m_aNodes.pop();
    m_bEmpty = false;
}
// -----------------------------------------------------------------------------

void BasicParser::ensureInNode( )
{
    if (!isInNode())
        raiseParseException("Unexpected endElement without matching startElement");
}
// -----------------------------------------------------------------------------

bool BasicParser::isInNode( )
{
    return ! m_aNodes.empty();
}
// -----------------------------------------------------------------------------

bool BasicParser::isEmptyNode( )
{
    return m_bEmpty;
}
// -----------------------------------------------------------------------------

ElementInfo const & BasicParser::getActiveNodeInfo( )
{
    ensureInNode();

    return m_aNodes.top();
}
// -----------------------------------------------------------------------------

void BasicParser::startProperty( ElementInfo const & aInfo, const uno::Reference< sax::XAttributeList >& xAttribs )
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    OSL_ENSURE( !isSkipping(), "While skipping, call startSkipping() instead of startProperty()");
    OSL_ENSURE( aInfo.type == ElementType::property, "For non-property nodes, call startNode() instead of startProperty()");

    if (isInProperty())
        raiseParseException( "Configuration XML Parser - Invalid Data: Properties may not nest" );

    try 
    { 
        m_aValueType = getDataParser().getPropertyValueType(xAttribs); 
    }
    catch (ElementParser::BadValueType & error) 
    {
        raiseParseException(error.message());
    }

    m_bInProperty = true;

    m_aNodes.push(aInfo);
    m_bEmpty = true;

    OSL_POSTCOND( isInProperty(), "Could not get data to start a property" );
    OSL_POSTCOND( isInUnhandledProperty(), "Could not mark property as unhandled");
}
// -----------------------------------------------------------------------------

void BasicParser::endProperty( )
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    OSL_ENSURE( !isSkipping(), "While skipping, honor wasSkipping() instead of calling endProperty()");
    OSL_ENSURE( isInProperty(), "For non-property nodes, call endNode() instead of endProperty()" );

    ensureInNode();

    m_aNodes.pop();
    m_bEmpty = false;

    m_aValueType = uno::Type();
    m_bInProperty = false;

    OSL_POSTCOND( !isInProperty(), "Could not get mark end of property" );
}
// -----------------------------------------------------------------------------

uno::Type BasicParser::getActivePropertyType()
{
    return m_aValueType;
}
// -----------------------------------------------------------------------------

bool BasicParser::isInProperty()
{
    return m_bInProperty;
}
// -----------------------------------------------------------------------------

bool BasicParser::isInUnhandledProperty()
{
    return m_bEmpty && m_bInProperty;
}
// -----------------------------------------------------------------------------

void BasicParser::startValueData(const uno::Reference< sax::XAttributeList >& xAttribs)
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    if (!isInProperty())
        raiseParseException( "Configuration XML Parser - Invalid Data: A value may occur only within a property" );

    if (m_aValueType.getTypeClass() == uno::TypeClass_ANY)
        raiseParseException( "Configuration XML Parser - Invalid Data: Cannot have values for properties of type 'Any'" );

    if (isInValueData())
        raiseParseException( "Configuration XML Parser - Invalid Data: Unexpected element while parsing value data" );

    m_pValueData = new ValueData(m_aValueType, m_xTypeConverter);

    m_pValueData->setIsNull( getDataParser().isNull(xAttribs) );

    m_pValueData->setSeparator( getDataParser().getSeparator(xAttribs) );

    OSL_ENSURE( !m_pValueData->hasSeparator() ||
                !m_pValueData->isTypeSet() ||
                m_pValueData->isList(),
                "Warning: Spurious oor:separator on value that is not a list");
    OSL_ENSURE( !m_pValueData->hasSeparator() ||
                !m_pValueData->isNull(),
                "Warning: Spurious oor:separator on value that is not a list");

    rtl::OUString aLocale;
    if ( getDataParser().getLanguage(xAttribs,aLocale) )
        m_pValueData->setLocalized( aLocale );
}
// -----------------------------------------------------------------------------

bool BasicParser::isInValueData()
{
    return m_pValueData != NULL;
}
// -----------------------------------------------------------------------------

bool BasicParser::isValueDataLocalized()
{
    OSL_ENSURE(isInValueData(), "There is no value data that could be localized");

    return m_pValueData && m_pValueData->isLocalized;
}
// -----------------------------------------------------------------------------

rtl::OUString BasicParser::getValueDataLocale()
{
    OSL_ENSURE(isValueDataLocalized(), "There is no value data or it is not localized");

    return m_pValueData->locale;
}
// -----------------------------------------------------------------------------

uno::Any BasicParser::getCurrentValue()
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    OSL_ASSERT( isInValueData() );

    uno::Any aResult;

    if (m_pValueData->isTypeSet())
        try
        {
            aResult = m_pValueData->convertToAny();
        }
        catch (script::CannotConvertException & e)
        {
            this->raiseParseException(uno::makeAny(e),"Configuration XML Parser - Invalid Data: Cannot convert value to type of property" );
        }
    else if (m_pValueData->isNull())
    {
        // nothing to do
    }
    else if (m_pValueData->hasSeparator() || m_pValueData->isList())
    {
        aResult <<= m_pValueData->toStringList();
    }
    else
    {
        aResult <<= m_pValueData->toString();
    }
    return aResult;
}
// -----------------------------------------------------------------------------

/// end collecting data for a value
void BasicParser::endValueData()
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    OSL_ASSERT( isInValueData() );

    delete m_pValueData, m_pValueData = NULL;
    m_bEmpty = false;

    OSL_POSTCOND( !isInValueData(), "Could not end value data tag" );
    OSL_POSTCOND( !isInUnhandledProperty(), "Could not mark property as handled" );
}
// -----------------------------------------------------------------------------

void BasicParser::startSkipping( const rtl::OUString& aName, const uno::Reference< sax::XAttributeList >& /*xAttribs*/ )
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    m_aNodes.push( ElementInfo(aName) );
    ++m_nSkipLevels;
}
// -----------------------------------------------------------------------------

bool BasicParser::wasSkipping( const rtl::OUString& aName )
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    if (m_nSkipLevels == 0) return false;

    if (m_aNodes.empty())
        raiseParseException( "Configuration XML Parser - Invalid XML: Unexpected end of element (while skipping data)" );

    if (aName != m_aNodes.top().name)
        raiseParseException( "Configuration XML Parser - Invalid XML: End tag does not match start tag (while skipping data)" );

    --m_nSkipLevels;
    m_aNodes.pop();

    return true;
}
// -----------------------------------------------------------------------------

bool BasicParser::isSkipping( )
{
    return m_nSkipLevels != 0;
}
// -----------------------------------------------------------------------------

void BasicParser::raiseParseException( uno::Any const & _aTargetException, sal_Char const * _pMsg )
    SAL_THROW((sax::SAXException, uno::RuntimeException))
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    if (_pMsg == 0) _pMsg = "Configuration XML Parser: Invalid Data: ";

    rtl::OUString sMessage = rtl::OUString::createFromAscii(_pMsg);

    uno::Exception aEx;
    if (_aTargetException >>= aEx)
        sMessage += aEx.Message;

    getLogger().error(sMessage,"parse","configuration::xml::BasicParser");
    throw sax::SAXException( sMessage, *this, _aTargetException );
}
// -----------------------------------------------------------------------------

void BasicParser::raiseParseException( sal_Char const * _pMsg )
    SAL_THROW((sax::SAXException, uno::RuntimeException))
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    if (_pMsg == 0) _pMsg = "Configuration XML Parser: Invalid XML";

    rtl::OUString const sMessage = rtl::OUString::createFromAscii(_pMsg);

    getLogger().error(sMessage,"parse","configuration::xml::BasicParser");
    throw sax::SAXException( sMessage, *this, uno::Any() );
}
// -----------------------------------------------------------------------------

void BasicParser::raiseParseException( rtl::OUString const & sMessage )
    SAL_THROW((sax::SAXException, uno::RuntimeException))
{
    OSL_DEBUG_ONLY( dbgUpdateLocation() );

    if (sMessage.getLength() == 0) raiseParseException(NULL);

    getLogger().error(sMessage,"parse","configuration::xml::BasicParser");
    throw sax::SAXException( sMessage, *this, uno::Any() );
}
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
    } // namespace

// -----------------------------------------------------------------------------
} // namespace

