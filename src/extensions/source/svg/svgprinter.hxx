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

#ifndef _SVGPRINTER_HXX
#define _SVGPRINTER_HXX

#include "svgcom.hxx"

// -------------
// - SVGWriter -
// -------------

class SVGPrinterExport;

class SVGPrinter : public NMSP_CPPU::OWeakObject, NMSP_SVG::XSVGPrinter
{	
private:

	REF( NMSP_LANG::XMultiServiceFactory )	mxFact;
	SVGPrinterExport*						mpWriter;
											
											SVGPrinter();
											
public:										
											
											SVGPrinter( const REF( NMSP_LANG::XMultiServiceFactory )& rxMgr );
	virtual 								~SVGPrinter();
											
	// XInterface							
	virtual ANY SAL_CALL					queryInterface( const NMSP_UNO::Type & rType ) throw( NMSP_UNO::RuntimeException );
    virtual void SAL_CALL					acquire() throw();
    virtual void SAL_CALL					release() throw();
											
	// XSVGPrinter
    virtual sal_Bool SAL_CALL				startJob( const REF( NMSP_SAX::XDocumentHandler )& rxHandler, 
													  const SEQ( sal_Int8 )& rJobSetup, 
													  const NMSP_RTL::OUString& rJobName, 
													  sal_uInt32 nCopies, sal_Bool bCollate ) throw( NMSP_UNO::RuntimeException );
	virtual void SAL_CALL					printPage( const SEQ( sal_Int8 )& rPrintPage ) throw( NMSP_UNO::RuntimeException );
	virtual void SAL_CALL					endJob() throw( NMSP_UNO::RuntimeException );
};

#endif
