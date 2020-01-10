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

#ifndef RDFAEXPORTHELPER_HXX
#define RDFAEXPORTHELPER_HXX

#include <com/sun/star/uno/Reference.h>

#include <rtl/ustring.hxx>

#include <map>


namespace com { namespace sun { namespace star {
    namespace rdf { class XBlankNode; }
    namespace rdf { class XMetadatable; }
    namespace rdf { class XDocumentRepository; }
} } }

class SvXMLExport;

namespace xmloff {

class SAL_DLLPRIVATE RDFaExportHelper
{
private:
    SvXMLExport & m_rExport;

    ::com::sun::star::uno::Reference<
        ::com::sun::star::rdf::XDocumentRepository> m_xRepository;

    ::rtl::OUString m_RDFsLabel;

    typedef ::std::map< ::rtl::OUString, ::rtl::OUString >
        BlankNodeMap_t;

    BlankNodeMap_t m_BlankNodeMap;

    long m_Counter;

    ::rtl::OUString
    LookupBlankNode( ::com::sun::star::uno::Reference<
        ::com::sun::star::rdf::XBlankNode> const & i_xBlankNode);

public:
    RDFaExportHelper(SvXMLExport & i_rExport);

    void
    AddRDFa(::com::sun::star::uno::Reference<
        ::com::sun::star::rdf::XMetadatable> const & i_xMetadatable);
};

} // namespace xmloff

#endif // RDFAEXPORTHELPER_HXX

