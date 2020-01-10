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

#include "precompiled_xmloff.hxx"

#include "RDFaExportHelper.hxx"

#include "xmlnmspe.hxx"

#include <xmloff/xmlexp.hxx>
#include <xmloff/xmltoken.hxx>

#include <comphelper/stlunosequence.hxx>
#include <comphelper/stl_types.hxx>

#include <com/sun/star/rdf/Statement.hpp>
#include <com/sun/star/rdf/URIs.hpp>
#include <com/sun/star/rdf/URI.hpp>
#include <com/sun/star/rdf/XLiteral.hpp>
#include <com/sun/star/rdf/XRepositorySupplier.hpp>
#include <com/sun/star/rdf/XDocumentRepository.hpp>

#include <rtl/ustrbuf.hxx>

#include <boost/bind.hpp>
#include <boost/iterator_adaptors.hpp>
#ifndef BOOST_ITERATOR_ADAPTOR_DWA053000_HPP_ // from iterator_adaptors.hpp
// N.B.: the check for the header guard _of a specific version of boost_
//       is here so this may work on different versions of boost,
//       which sadly put the goods in different header files
#include <boost/iterator/transform_iterator.hpp>
#endif

#include <functional>
#include <algorithm>


using namespace ::com::sun::star;

namespace xmloff {

static const char s_prefix [] = "_:b";

static ::rtl::OUString
makeCURIE(SvXMLExport * i_pExport,
    uno::Reference<rdf::XURI> const & i_xURI)
{
    OSL_ENSURE(i_xURI.is(), "makeCURIE: null URI");
    if (!i_xURI.is()) throw uno::RuntimeException();

    const ::rtl::OUString Namespace( i_xURI->getNamespace() );
    OSL_ENSURE(Namespace.getLength(), "makeCURIE: no namespace");
    if (!Namespace.getLength()) throw uno::RuntimeException();

    ::rtl::OUStringBuffer buf;
    buf.append( i_pExport->EnsureNamespace(Namespace) );
    buf.append( static_cast<sal_Unicode>(':') );
    // N.B.: empty LocalName is valid!
    buf.append( i_xURI->getLocalName() );

    return buf.makeStringAndClear();
}

////////////////////////////////////////////////////////////////////////////

RDFaExportHelper::RDFaExportHelper(SvXMLExport & i_rExport)
    : m_rExport(i_rExport), m_xRepository(0), m_Counter(0)
{
    const uno::Reference<rdf::XRepositorySupplier> xRS( m_rExport.GetModel(),
            uno::UNO_QUERY);
    OSL_ENSURE(xRS.is(), "AddRDFa: model is no rdf::XRepositorySupplier");
    if (!xRS.is()) throw uno::RuntimeException();
    m_xRepository.set(xRS->getRDFRepository(), uno::UNO_QUERY_THROW);

    const uno::Reference<rdf::XURI> xLabel(
        rdf::URI::createKnown(m_rExport.GetComponentContext(),
            rdf::URIs::RDFS_LABEL));
    m_RDFsLabel = xLabel->getStringValue();
}

::rtl::OUString
RDFaExportHelper::LookupBlankNode(
    uno::Reference<rdf::XBlankNode> const & i_xBlankNode)
{
    OSL_ENSURE(i_xBlankNode.is(), "null BlankNode?");
    if (!i_xBlankNode.is()) throw uno::RuntimeException();
    ::rtl::OUString & rEntry(
        m_BlankNodeMap[ i_xBlankNode->getStringValue() ] );
    if (!rEntry.getLength())
    {
        ::rtl::OUStringBuffer buf;
        buf.appendAscii(s_prefix);
        buf.append(++m_Counter);
        rEntry = buf.makeStringAndClear();
    }
    return rEntry;
}

////////////////////////////////////////////////////////////////////////////

void
RDFaExportHelper::AddRDFa(
    uno::Reference<rdf::XMetadatable> const & i_xMetadatable)
{
    try
    {
        uno::Sequence<rdf::Statement> stmts(
            m_xRepository->getStatementRDFa(i_xMetadatable) );

        if (0 == stmts.getLength())
        {
            return; // no RDFa
        }

        // all stmts have the same subject, so we only handle first one
        const uno::Reference<rdf::XURI> xSubjectURI(stmts[0].Subject,
            uno::UNO_QUERY);
        const uno::Reference<rdf::XBlankNode> xSubjectBNode(stmts[0].Subject,
            uno::UNO_QUERY);
        if (!xSubjectURI.is() && !xSubjectBNode.is())
        {
            throw uno::RuntimeException();
        }
        static const sal_Unicode s_OpenBracket ('[');
        static const sal_Unicode s_CloseBracket(']');
        const ::rtl::OUString about( xSubjectURI.is()
            ?   m_rExport.GetRelativeReference(xSubjectURI->getStringValue())
            :   ::rtl::OUStringBuffer().append(s_OpenBracket).append(
                        LookupBlankNode(xSubjectBNode)).append(s_CloseBracket)
                    .makeStringAndClear()
            );

        rdf::Statement* const iter
            ( ::std::partition( ::comphelper::stl_begin(stmts),
                ::comphelper::stl_end(stmts),
                ::boost::bind(&::rtl::OUString::equals, m_RDFsLabel,
                    ::boost::bind(&rdf::XNode::getStringValue,
                        ::boost::bind(&rdf::Statement::Predicate, _1))) ) );

        if (iter != ::comphelper::stl_end(stmts))
        {
            // from iter to end, all stmts should have same object
            const uno::Reference<rdf::XLiteral> xContent(
                (*iter).Object, uno::UNO_QUERY_THROW );
            const uno::Reference<rdf::XURI> xDatatype(xContent->getDatatype());
            if (xDatatype.is())
            {
                const ::rtl::OUString datatype(
                    makeCURIE(&m_rExport, xDatatype) );
                m_rExport.AddAttribute(XML_NAMESPACE_XHTML,
                    token::XML_DATATYPE, datatype);
            }
            if (iter != ::comphelper::stl_begin(stmts)) // there is rdfs:label
            {
                m_rExport.AddAttribute(XML_NAMESPACE_XHTML, token::XML_CONTENT,
                    xContent->getValue());
            }
        }
        else
        {
            OSL_ENSURE(false,"invalid RDFa: every property is rdfs:label");
            return;
        }

        ::rtl::OUStringBuffer property;
        ::comphelper::intersperse(
            ::boost::make_transform_iterator(
                iter, // omit RDFsLabel predicates!
                ::boost::bind(&makeCURIE, &m_rExport,
                    ::boost::bind(&rdf::Statement::Predicate, _1))),
            // argh, this must be the same type :(
            ::boost::make_transform_iterator(
                ::comphelper::stl_end(stmts),
                ::boost::bind(&makeCURIE, &m_rExport,
                    ::boost::bind(&rdf::Statement::Predicate, _1))),
            ::comphelper::OUStringBufferAppender(property),
            ::rtl::OUString::createFromAscii(" "));

        m_rExport.AddAttribute(XML_NAMESPACE_XHTML, token::XML_PROPERTY,
            property.makeStringAndClear());

        m_rExport.AddAttribute(XML_NAMESPACE_XHTML, token::XML_ABOUT, about);
    }
    catch (uno::Exception &)
    {
        OSL_ENSURE(false, "AddRDFa: exception");
    }
}

} // namespace xmloff

