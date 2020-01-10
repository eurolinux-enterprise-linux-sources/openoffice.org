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
#include "precompiled_desktop.hxx"

#include "dp_descriptioninfoset.hxx"

#include "dp_resource.h"
#include "sal/config.h"

#include "comphelper/sequence.hxx"
#include "comphelper/makesequence.hxx"
#include "boost/optional.hpp"
#include "com/sun/star/beans/Optional.hpp"
#include "com/sun/star/lang/XMultiComponentFactory.hpp"
#include "com/sun/star/lang/Locale.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/RuntimeException.hpp"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/XComponentContext.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/xml/dom/DOMException.hpp"
#include "com/sun/star/xml/dom/XNode.hpp"
#include "com/sun/star/xml/dom/XNodeList.hpp"
#include "com/sun/star/xml/xpath/XXPathAPI.hpp"
#include "cppuhelper/implbase1.hxx"
#include "cppuhelper/weak.hxx"
#include "rtl/ustring.h"
#include "rtl/ustring.hxx"
#include "sal/types.h"


namespace {

namespace css = ::com::sun::star;

class EmptyNodeList: public ::cppu::WeakImplHelper1< css::xml::dom::XNodeList >
{
public:
    EmptyNodeList();

    virtual ~EmptyNodeList();

    virtual ::sal_Int32 SAL_CALL getLength() throw (css::uno::RuntimeException);

    virtual css::uno::Reference< css::xml::dom::XNode > SAL_CALL
    item(::sal_Int32 index) throw (css::uno::RuntimeException);

private:
    EmptyNodeList(EmptyNodeList &); // not defined
    void operator =(EmptyNodeList &); // not defined
};

EmptyNodeList::EmptyNodeList() {}

EmptyNodeList::~EmptyNodeList() {}

::sal_Int32 EmptyNodeList::getLength() throw (css::uno::RuntimeException) {
    return 0;
}

css::uno::Reference< css::xml::dom::XNode > EmptyNodeList::item(::sal_Int32)
    throw (css::uno::RuntimeException)
{
    throw css::uno::RuntimeException(
        ::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM(
                "bad EmptyNodeList com.sun.star.xml.dom.XNodeList.item call")),
        static_cast< ::cppu::OWeakObject * >(this));
}

::rtl::OUString getNodeValue(
    css::uno::Reference< css::xml::dom::XNode > const & node)
{
    OSL_ASSERT(node.is());
    try {
        return node->getNodeValue();
    } catch (css::xml::dom::DOMException & e) {
        throw css::uno::RuntimeException(
            (::rtl::OUString(
                RTL_CONSTASCII_USTRINGPARAM(
                    "com.sun.star.xml.dom.DOMException: ")) +
             e.Message),
            css::uno::Reference< css::uno::XInterface >());
    }
}

}

namespace dp_misc {

DescriptionInfoset::DescriptionInfoset(
    css::uno::Reference< css::uno::XComponentContext > const & context,
    css::uno::Reference< css::xml::dom::XNode > const & element):
    m_element(element)
{
    css::uno::Reference< css::lang::XMultiComponentFactory > manager(
        context->getServiceManager(), css::uno::UNO_QUERY_THROW);
    if (m_element.is()) {
        m_xpath = css::uno::Reference< css::xml::xpath::XXPathAPI >(
            manager->createInstanceWithContext(
                ::rtl::OUString(
                    RTL_CONSTASCII_USTRINGPARAM(
                        "com.sun.star.xml.xpath.XPathAPI")),
                context),
            css::uno::UNO_QUERY_THROW);
        m_xpath->registerNS(
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("desc")),
            element->getNamespaceURI());
        m_xpath->registerNS(
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("xlink")),
            ::rtl::OUString(
                RTL_CONSTASCII_USTRINGPARAM("http://www.w3.org/1999/xlink")));
    }
}

DescriptionInfoset::~DescriptionInfoset() {}

::boost::optional< ::rtl::OUString > DescriptionInfoset::getIdentifier() const {
    return getOptionalValue(
        ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("desc:identifier/@value")));
}

::rtl::OUString DescriptionInfoset::getNodeValueFromExpression(::rtl::OUString const & expression) const
{
    css::uno::Reference< css::xml::dom::XNode > n;
    if (m_element.is()) {
        try {
            n = m_xpath->selectSingleNode(m_element, expression);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return n.is() ? getNodeValue(n) : ::rtl::OUString();
}


::rtl::OUString DescriptionInfoset::getVersion() const 
{
    return getNodeValueFromExpression( ::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM("desc:version/@value")));
}

css::uno::Sequence< ::rtl::OUString > DescriptionInfoset::getSupportedPlaforms() const
{
    //When there is no description.xml then we assume that we support all platforms
    if (! m_element.is())
    {
        return comphelper::makeSequence(
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("all")));
    }

    //Check if the <platform> element was provided. If not the default is "all" platforms
    css::uno::Reference< css::xml::dom::XNode > nodePlatform(
        m_xpath->selectSingleNode(m_element, ::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM("desc:platform"))));
    if (!nodePlatform.is())
    {
        return comphelper::makeSequence(
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("all")));
    }

    //There is a platform element.
    const ::rtl::OUString value = getNodeValueFromExpression(::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM("desc:platform/@value")));
    //parse the string, it can contained multiple strings separated by commas
    ::std::vector< ::rtl::OUString> vec;
    sal_Int32 nIndex = 0;
    do
    {
        ::rtl::OUString aToken = value.getToken( 0, ',', nIndex );
        aToken = aToken.trim();
        if (aToken.getLength())
            vec.push_back(aToken);
     
    }
    while (nIndex >= 0);

    return comphelper::containerToSequence(vec);
}

css::uno::Reference< css::xml::dom::XNodeList >
DescriptionInfoset::getDependencies() const {
    if (m_element.is()) {
        try {
            return m_xpath->selectNodeList(m_element, ::rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM("desc:dependencies/*")));
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return new EmptyNodeList;
}

css::uno::Sequence< ::rtl::OUString >
DescriptionInfoset::getUpdateInformationUrls() const {
    return getUrls(
        ::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM(
                "desc:update-information/desc:src/@xlink:href")));
}

css::uno::Sequence< ::rtl::OUString > 
DescriptionInfoset::getUpdateDownloadUrls() const
{
    return getUrls(
        ::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM(
                "desc:update-download/desc:src/@xlink:href")));
}

::rtl::OUString DescriptionInfoset::getIconURL( sal_Bool bHighContrast ) const
{
    css::uno::Sequence< ::rtl::OUString > aStrList = getUrls( ::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM( "desc:icon/desc:default/@xlink:href")));
    css::uno::Sequence< ::rtl::OUString > aStrListHC = getUrls( ::rtl::OUString(
            RTL_CONSTASCII_USTRINGPARAM( "desc:icon/desc:high-contrast/@xlink:href")));

    if ( bHighContrast && aStrListHC.hasElements() && aStrListHC[0].getLength() )
        return aStrListHC[0];
        
    if ( aStrList.hasElements() && aStrList[0].getLength() )
        return aStrList[0];
    
    return ::rtl::OUString();
}

::boost::optional< ::rtl::OUString > DescriptionInfoset::getLocalizedUpdateWebsiteURL()
    const
{
    bool bParentExists = false;
    const ::rtl::OUString sURL (getLocalizedHREFAttrFromChild(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
        "/desc:description/desc:update-website")), &bParentExists ));

    if (sURL.getLength() > 0)
        return ::boost::optional< ::rtl::OUString >(sURL);
    else
        return bParentExists ? ::boost::optional< ::rtl::OUString >(::rtl::OUString()) :
            ::boost::optional< ::rtl::OUString >();
}

css::uno::Reference< css::xml::xpath::XXPathAPI > DescriptionInfoset::getXpath()
    const
{
    return m_xpath;
}

::boost::optional< ::rtl::OUString > DescriptionInfoset::getOptionalValue(
    ::rtl::OUString const & expression) const
{
    css::uno::Reference< css::xml::dom::XNode > n;
    if (m_element.is()) {
        try {
            n = m_xpath->selectSingleNode(m_element, expression);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return n.is()
        ? ::boost::optional< ::rtl::OUString >(getNodeValue(n))
        : ::boost::optional< ::rtl::OUString >();
}

css::uno::Sequence< ::rtl::OUString > DescriptionInfoset::getUrls(
    ::rtl::OUString const & expression) const
{
    css::uno::Reference< css::xml::dom::XNodeList > ns;
    if (m_element.is()) {
        try {
            ns = m_xpath->selectNodeList(m_element, expression);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    css::uno::Sequence< ::rtl::OUString > urls(ns.is() ? ns->getLength() : 0);
    for (::sal_Int32 i = 0; i < urls.getLength(); ++i) {
        urls[i] = getNodeValue(ns->item(i));
    }
    return urls;
}

::std::pair< ::rtl::OUString, ::rtl::OUString > DescriptionInfoset::getLocalizedPublisherNameAndURL() const
{
    css::uno::Reference< css::xml::dom::XNode > node = 
        getLocalizedChild(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("desc:publisher")));

    ::rtl::OUString sPublisherName;
    ::rtl::OUString sURL;
    if (node.is())
    {
        const ::rtl::OUString exp1(RTL_CONSTASCII_USTRINGPARAM("text()"));
        css::uno::Reference< css::xml::dom::XNode > xPathName;
        try {
            xPathName = m_xpath->selectSingleNode(node, exp1);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
        OSL_ASSERT(xPathName.is());
        if (xPathName.is())
            sPublisherName = xPathName->getNodeValue();

        const ::rtl::OUString exp2(RTL_CONSTASCII_USTRINGPARAM("@xlink:href"));
        css::uno::Reference< css::xml::dom::XNode > xURL;
        try {
            xURL = m_xpath->selectSingleNode(node, exp2);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
        OSL_ASSERT(xURL.is());
        if (xURL.is())
           sURL = xURL->getNodeValue();
    }
    return ::std::make_pair(sPublisherName, sURL);
}

::rtl::OUString DescriptionInfoset::getLocalizedReleaseNotesURL() const
{
    return getLocalizedHREFAttrFromChild(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
        "/desc:description/desc:release-notes")), NULL);
}

::rtl::OUString DescriptionInfoset::getLocalizedDisplayName() const
{
    css::uno::Reference< css::xml::dom::XNode > node = 
        getLocalizedChild(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("desc:display-name")));
    if (node.is())
    {
        const ::rtl::OUString exp(RTL_CONSTASCII_USTRINGPARAM("text()"));
        css::uno::Reference< css::xml::dom::XNode > xtext;
        try {
            xtext = m_xpath->selectSingleNode(node, exp);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
        if (xtext.is())
            return xtext->getNodeValue();
    }
    return ::rtl::OUString();
}

::rtl::OUString DescriptionInfoset::getLocalizedLicenseURL() const
{
    return getLocalizedHREFAttrFromChild(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
        "/desc:description/desc:registration/desc:simple-license")), NULL);

}

::boost::optional<SimpleLicenseAttributes> 
DescriptionInfoset::getSimpleLicenseAttributes() const
{
    //Check if the node exist
    css::uno::Reference< css::xml::dom::XNode > n;
    if (m_element.is()) {
        try {
            n = m_xpath->selectSingleNode(m_element, 
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
                "/desc:description/desc:registration/desc:simple-license/@accept-by")));
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
        if (n.is())
        {
            SimpleLicenseAttributes attributes;
            attributes.acceptBy = 
                getNodeValueFromExpression(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
                "/desc:description/desc:registration/desc:simple-license/@accept-by")));

            ::boost::optional< ::rtl::OUString > suppressOnUpdate = getOptionalValue(
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
                "/desc:description/desc:registration/desc:simple-license/@suppress-on-update")));
            if (suppressOnUpdate)
                attributes.suppressOnUpdate = (*suppressOnUpdate).trim().equalsIgnoreAsciiCase(
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("true")));
            else
                attributes.suppressOnUpdate = false;

            ::boost::optional< ::rtl::OUString > suppressIfRequired = getOptionalValue(
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
                "/desc:description/desc:registration/desc:simple-license/@suppress-if-required")));
            if (suppressIfRequired)
                attributes.suppressIfRequired = (*suppressIfRequired).trim().equalsIgnoreAsciiCase(
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("true")));
            else
                attributes.suppressIfRequired = false;

            return ::boost::optional<SimpleLicenseAttributes>(attributes);
        }
    }
    return ::boost::optional<SimpleLicenseAttributes>();
}

::rtl::OUString DescriptionInfoset::getLocalizedDescriptionURL() const
{
    return getLocalizedHREFAttrFromChild(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(
        "/desc:description/desc:extension-description")), NULL);
}

css::uno::Reference< css::xml::dom::XNode > 
DescriptionInfoset::getLocalizedChild( const ::rtl::OUString & sParent) const
{
    if ( ! m_element.is() || !sParent.getLength())
        return css::uno::Reference< css::xml::dom::XNode > ();

    css::uno::Reference< css::xml::dom::XNode > xParent;
    try {
        xParent = m_xpath->selectSingleNode(m_element, sParent);
    } catch (css::xml::xpath::XPathException &) {
        // ignore
    }
    css::uno::Reference<css::xml::dom::XNode> nodeMatch;
    if (xParent.is())
    {
        const ::rtl::OUString sLocale = getOfficeLocaleString();
        nodeMatch = matchFullLocale(xParent, sLocale);

        //office: en-DE, en, en-DE-altmark
        if (! nodeMatch.is())
        {
            const css::lang::Locale officeLocale = getOfficeLocale();
            nodeMatch = matchCountryAndLanguage(xParent, officeLocale); 
            if ( ! nodeMatch.is())
            {
                nodeMatch = matchLanguage(xParent, officeLocale);
                if (! nodeMatch.is())
                    nodeMatch = getChildWithDefaultLocale(xParent);
            }
        }
    }

    return nodeMatch;
}

css::uno::Reference<css::xml::dom::XNode> 
DescriptionInfoset::matchFullLocale(css::uno::Reference< css::xml::dom::XNode > 
                                    const & xParent, ::rtl::OUString const & sLocale) const
{
    OSL_ASSERT(xParent.is());   
    const ::rtl::OUString exp1(
        ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("*[@lang=\""))
        + sLocale + 
        ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("\"]")));
    try {
        return m_xpath->selectSingleNode(xParent, exp1);
    } catch (css::xml::xpath::XPathException &) {
        // ignore
        return 0;
    }
}

css::uno::Reference<css::xml::dom::XNode> 
DescriptionInfoset::matchCountryAndLanguage(
    css::uno::Reference< css::xml::dom::XNode > const & xParent, css::lang::Locale const & officeLocale) const
{
    OSL_ASSERT(xParent.is());
    css::uno::Reference<css::xml::dom::XNode> nodeMatch;

    if (officeLocale.Country.getLength())
    {
        const ::rtl::OUString sLangCountry(officeLocale.Language + 
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("-")) +
            officeLocale.Country);
        //first try exact match for lang-country
        const ::rtl::OUString exp1(
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("*[@lang=\""))
            + sLangCountry + 
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("\"]")));
        try {
            nodeMatch = m_xpath->selectSingleNode(xParent, exp1);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }

        //try to match in strings that also have a variant, for example en-US matches in
        //en-US-montana
        if (!nodeMatch.is())
        {
            const ::rtl::OUString exp2(
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("*[starts-with(@lang,\""))
                + sLangCountry + 
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("-\")]")));
            try {
                nodeMatch = m_xpath->selectSingleNode(xParent, exp2);
            } catch (css::xml::xpath::XPathException &) {
                // ignore
            }
        }
    }

    return nodeMatch;
}


css::uno::Reference<css::xml::dom::XNode> 
DescriptionInfoset::matchLanguage(
    css::uno::Reference< css::xml::dom::XNode > const & xParent, css::lang::Locale const & officeLocale) const
{
    OSL_ASSERT(xParent.is());
    css::uno::Reference<css::xml::dom::XNode> nodeMatch;

    //first try exact match for lang
    const ::rtl::OUString exp1(
        ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("*[@lang=\"")) 
        + officeLocale.Language 
        + ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("\"]")));
    try {
        nodeMatch = m_xpath->selectSingleNode(xParent, exp1);
    } catch (css::xml::xpath::XPathException &) {
        // ignore
    }

    //try to match in strings that also have a country and/orvariant, for example en  matches in
    //en-US-montana, en-US, en-montana
    if (!nodeMatch.is())
    {
        const ::rtl::OUString exp2(
            ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("*[starts-with(@lang,\""))
            + officeLocale.Language 
            + ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("-\")]")));
        try {
            nodeMatch = m_xpath->selectSingleNode(xParent, exp2);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
    }
    return nodeMatch;
}

css::uno::Reference<css::xml::dom::XNode> 
DescriptionInfoset::getChildWithDefaultLocale(css::uno::Reference< css::xml::dom::XNode > 
                                    const & xParent) const
{
    OSL_ASSERT(xParent.is());
    if (xParent->getNodeName().equals(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("simple-license"))))
    {
        css::uno::Reference<css::xml::dom::XNode> nodeDefault;
        try {
            nodeDefault = m_xpath->selectSingleNode(xParent, ::rtl::OUString(
                RTL_CONSTASCII_USTRINGPARAM("@default-license-id")));
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
        if (nodeDefault.is())
        {
            //The old way
            const ::rtl::OUString exp1(
                ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("desc:license-text[@license-id = \"")) 
                + nodeDefault->getNodeValue() 
                + ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("\"]")));
            try {
                return m_xpath->selectSingleNode(xParent, exp1);
            } catch (css::xml::xpath::XPathException &) {
                // ignore
            }
        }
    }

    const ::rtl::OUString exp2(RTL_CONSTASCII_USTRINGPARAM("*[1]"));
    try {
        return m_xpath->selectSingleNode(xParent, exp2);
    } catch (css::xml::xpath::XPathException &) {
        // ignore
        return 0;
    }
}

::rtl::OUString DescriptionInfoset::getLocalizedHREFAttrFromChild(
    ::rtl::OUString const & sXPathParent, bool * out_bParentExists)
    const
{
    css::uno::Reference< css::xml::dom::XNode > node = 
        getLocalizedChild(sXPathParent);

    ::rtl::OUString sURL;
    if (node.is())
    {
        if (out_bParentExists)
            *out_bParentExists = true;
        const ::rtl::OUString exp(RTL_CONSTASCII_USTRINGPARAM("@xlink:href"));
        css::uno::Reference< css::xml::dom::XNode > xURL;
        try {
            xURL = m_xpath->selectSingleNode(node, exp);
        } catch (css::xml::xpath::XPathException &) {
            // ignore
        }
        OSL_ASSERT(xURL.is());
        if (xURL.is())
            sURL = xURL->getNodeValue();
    }
    else
    {
        if (out_bParentExists)
            *out_bParentExists = false;
    }
    return sURL;
}

}
