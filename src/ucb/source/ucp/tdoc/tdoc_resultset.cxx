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
#include "precompiled_ucb.hxx"

/**************************************************************************
								TODO
 **************************************************************************

 - This implementation is not a dynamic result set!!! It only implements
   the necessary interfaces, but never recognizes/notifies changes!!!

 *************************************************************************/

#include "ucbhelper/resultset.hxx"

#include "tdoc_datasupplier.hxx"
#include "tdoc_resultset.hxx"
#include "tdoc_content.hxx"

using namespace com::sun::star;
using namespace tdoc_ucp;

//=========================================================================
//=========================================================================
//
// DynamicResultSet Implementation.
//
//=========================================================================
//=========================================================================

DynamicResultSet::DynamicResultSet(
            const uno::Reference< lang::XMultiServiceFactory >& rxSMgr,
            const rtl::Reference< Content >& rxContent,
            const ucb::OpenCommandArgument2& rCommand )
: ResultSetImplHelper( rxSMgr, rCommand ),
  m_xContent( rxContent )
{
}

//=========================================================================
//
// Non-interface methods.
//
//=========================================================================

void DynamicResultSet::initStatic()
{
	m_xResultSet1
		= new ::ucbhelper::ResultSet(
            m_xSMgr,
            m_aCommand.Properties,
            new ResultSetDataSupplier( m_xSMgr,
                                       m_xContent,
                                       m_aCommand.Mode ) );
}

//=========================================================================
void DynamicResultSet::initDynamic()
{
	m_xResultSet1
		= new ::ucbhelper::ResultSet(
            m_xSMgr,
            m_aCommand.Properties,
            new ResultSetDataSupplier( m_xSMgr,
                                       m_xContent,
                                       m_aCommand.Mode ) );
	m_xResultSet2 = m_xResultSet1;
}

