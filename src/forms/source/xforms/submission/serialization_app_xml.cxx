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
#include "precompiled_forms.hxx"

#include "serialization.hxx"
#include "serialization_app_xml.hxx"

#include <unotools/processfactory.hxx>
#include <com/sun/star/xml/dom/XNode.hpp>
#include <com/sun/star/xml/dom/XDocument.hpp>
#include <com/sun/star/xml/dom/XNodeList.hpp>
#include <com/sun/star/xml/dom/NodeType.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/xml/xpath/XPathObjectType.hpp>

#include <libxml/tree.h>

CSerializationAppXML::CSerializationAppXML()
    : m_aFactory(utl::getProcessServiceFactory())
    , m_aPipe(CSS::uno::Reference< CSS::io::XOutputStream > (m_aFactory->createInstance( 
        rtl::OUString::createFromAscii("com.sun.star.io.Pipe")), CSS::uno::UNO_QUERY))
{
    OSL_ENSURE(m_aPipe.is(), "cannot create Pipe");
}

CSS::uno::Reference< CSS::io::XInputStream > 
CSerializationAppXML::getInputStream()
{
    // The pipes output is provided through it's
    // XOutputStream interface aspect
    return CSS::uno::Reference< CSS::io::XInputStream >(m_aPipe, CSS::uno::UNO_QUERY);
}

void 
CSerializationAppXML::serialize_node(const CSS::uno::Reference< CSS::xml::dom::XNode >& rNode)
{
    CSS::uno::Reference< CSS::xml::dom::XNode > aNode = rNode;
    if (aNode->getNodeType() == CSS::xml::dom::NodeType_DOCUMENT_NODE)
    {
        CSS::uno::Reference< CSS::xml::dom::XDocument > aDoc(rNode, CSS::uno::UNO_QUERY_THROW);
        aNode = CSS::uno::Reference< CSS::xml::dom::XNode >(aDoc->getDocumentElement(), CSS::uno::UNO_QUERY_THROW);
    } 
    if (aNode->getNodeType() != CSS::xml::dom::NodeType_ELEMENT_NODE)
        return;

    // clone the node to a new document and serialize that document
    CSS::uno::Reference< CSS::lang::XUnoTunnel > aTunnel(aNode, CSS::uno::UNO_QUERY);
    if (aTunnel.is()) 
    {
        xmlNodePtr aNodePtr = reinterpret_cast< xmlNodePtr >( aTunnel->getSomething(CSS::uno::Sequence< sal_Int8 >()) );
        xmlDocPtr aDocPtr = xmlNewDoc((xmlChar*)"1.0");
        xmlNodePtr aDocNodePtr = xmlDocCopyNode(aNodePtr, aDocPtr, 1);
        if (aDocNodePtr != NULL) {
            xmlAddChild((xmlNodePtr)aDocPtr, aDocNodePtr);
            xmlChar *buffer = NULL;
            sal_Int32 size = 0;
            xmlDocDumpMemory(aDocPtr, &buffer, (int*)&size);
        
            // write the xml into the pipe through it's XOutputStream interface
            m_aPipe->writeBytes(CSS::uno::Sequence< sal_Int8 >((sal_Int8*)buffer, size));            
            xmlFree(buffer);
        }

    } else {
        // can't get tunnel to native backend
        // logic for generic implementation could be implemented here...
        OSL_ENSURE(sal_False, "unkown dom implementation, cannot serialize");
        return;
    }
}

/*
void
CSerializationAppXML::serialize_nodeset()
{
    CSS::uno::Reference< CSS::xml::dom::XNodeList > aNodeList = m_aXPathObject->getNodeList();
    for (sal_Int32 i=0; i<aNodeList->getLength(); i++)
        serialize_node(aNodeList->item(i));
    m_aPipe->closeOutput();
}
*/

void
CSerializationAppXML::serialize()
{    
    if (!m_aFragment.is()) return;

    CSS::uno::Reference< CSS::xml::dom::XNode > cur = m_aFragment->getFirstChild();
    while (cur.is())
    {
        serialize_node(cur);
        cur = cur->getNextSibling();
    }
    m_aPipe->closeOutput();
}
