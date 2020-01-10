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

#ifndef _BASICPRN_HXX
#define _BASICPRN_HXX

#ifndef _PRINT_HXX //autogen
#include <vcl/print.hxx>
#endif

class PrintingDialog;

class BasicPrinter : public Printer {
	short nLine;						// aktuelle Zeile
	short nPage;						// aktuelle Seite
	short nLines;						// Zeilen pro Seite
	short nYoff;						// Zeilenabstand in Points
	String aFile;						// Dateiname
	PrintingDialog* pDlg;				// Druck-Dialog
	void Header();						// Seitenkopf drucken
public:
	BasicPrinter();
	void Print( const String& rFile, const String& rText, BasicFrame *pFrame );
	DECL_LINK( Abort, void * );
	DECL_LINK( StartPrintHdl, Printer * );
	DECL_LINK( EndPrintHdl, Printer * );
	DECL_LINK( PrintPageHdl, Printer * );
#if defined( PM20 )
	// StarView-Bug, bald wieder zu entfernen:
	virtual void SetPageQueueSize( USHORT ) {}
#endif
};

#endif
