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

#include <stdio.h>
#include <xmloff/formlayerexport.hxx>
#include "strings.hxx"
#include "elementexport.hxx"
#include "xmlnmspe.hxx"
#include <xmloff/xmlexp.hxx>
#include "layerexport.hxx"
#include "propertyexport.hxx"
#include <osl/diagnose.h>
#include <comphelper/extract.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <comphelper/stl_types.hxx>
#include "officeforms.hxx"


//.........................................................................
namespace xmloff
{
//.........................................................................

	using namespace ::com::sun::star::uno;
	using namespace ::com::sun::star::lang;
	using namespace ::com::sun::star::awt;
	using namespace ::com::sun::star::beans;
	using namespace ::com::sun::star::container;
	using namespace ::com::sun::star::drawing;
	using namespace ::com::sun::star::frame;

	//=====================================================================
	//= OFormLayerXMLExport
	//=====================================================================

	//---------------------------------------------------------------------
	OFormLayerXMLExport::OFormLayerXMLExport(SvXMLExport& _rContext)
		:m_rContext(_rContext)
		,m_pImpl(new OFormLayerXMLExport_Impl(_rContext))
	{
	}

	//---------------------------------------------------------------------
	OFormLayerXMLExport::~OFormLayerXMLExport()
	{
		delete m_pImpl;
		m_pImpl = NULL;
	}

	//---------------------------------------------------------------------
	sal_Bool OFormLayerXMLExport::seekPage(const Reference< XDrawPage >& _rxDrawPage)
	{
		return m_pImpl->seekPage(_rxDrawPage);
	}

	//---------------------------------------------------------------------
	::rtl::OUString OFormLayerXMLExport::getControlId(const Reference< XPropertySet >& _rxControl)
	{
		return m_pImpl->getControlId(_rxControl);
	}

	//---------------------------------------------------------------------
	::rtl::OUString OFormLayerXMLExport::getControlNumberStyle( const Reference< XPropertySet >& _rxControl )
	{
		return m_pImpl->getControlNumberStyle(_rxControl);
	}

	//---------------------------------------------------------------------
	::vos::ORef< SvXMLExportPropertyMapper > OFormLayerXMLExport::getStylePropertyMapper()
	{
		return m_pImpl->getStylePropertyMapper();
	}

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::initialize()
	{
		m_pImpl->clear();
	}

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::examineForms(const Reference< XDrawPage >& _rxDrawPage)
	{
		try
		{
			m_pImpl->examineForms(_rxDrawPage);
		}
		catch(Exception&)
		{
			OSL_ENSURE(sal_False, "OFormLayerXMLExport::examine: could not examine the draw page!");
		}
	}

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::exportForms(const Reference< XDrawPage >& _rxDrawPage)
	{
		m_pImpl->exportForms(_rxDrawPage);
	}

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::exportXForms() const
    {
		m_pImpl->exportXForms();
    }

	//---------------------------------------------------------------------
	bool OFormLayerXMLExport::pageContainsForms( const Reference< XDrawPage >& _rxDrawPage ) const
    {
		return m_pImpl->pageContainsForms( _rxDrawPage );
    }

	//---------------------------------------------------------------------
	bool OFormLayerXMLExport::documentContainsXForms() const
    {
		return m_pImpl->documentContainsXForms();
    }

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::exportControlNumberStyles()
	{
		m_pImpl->exportControlNumberStyles();
	}

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::exportAutoControlNumberStyles()
	{
		m_pImpl->exportAutoControlNumberStyles();
	}

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::exportAutoStyles()
	{
		m_pImpl->exportAutoStyles();
	}

	//---------------------------------------------------------------------
	void OFormLayerXMLExport::excludeFromExport( const Reference< XControlModel > _rxControl )
	{
		m_pImpl->excludeFromExport( _rxControl );
	}

	//=========================================================================
	//= OOfficeFormsExport
	//=========================================================================
	//-------------------------------------------------------------------------
	OOfficeFormsExport::OOfficeFormsExport( SvXMLExport& _rExp )
		:m_pImpl(NULL)
	{
		m_pImpl = new OFormsRootExport(_rExp);
	}

	//-------------------------------------------------------------------------
	OOfficeFormsExport::~OOfficeFormsExport()
	{
		delete m_pImpl;
	}

//.........................................................................
}	// namespace xmloff
//.........................................................................

