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

#ifndef SC_DELCLDLG_HXX
#define SC_DELCLDLG_HXX


#include <vcl/dialog.hxx>
#include <vcl/imagebtn.hxx>
#include <vcl/fixed.hxx>


#include "global.hxx"

//------------------------------------------------------------------------

class ScDeleteCellDlg : public ModalDialog
{
private:
    FixedLine       aFlFrame;
	RadioButton		aBtnCellsUp;
	RadioButton		aBtnCellsLeft;
	RadioButton		aBtnDelRows;
	RadioButton		aBtnDelCols;
	OKButton		aBtnOk;
	CancelButton	aBtnCancel;
	HelpButton		aBtnHelp;


public:
			ScDeleteCellDlg( Window* pParent, BOOL bDisallowCellMove = FALSE );
			~ScDeleteCellDlg();

	DelCellCmd GetDelCellCmd() const;
};


#endif // SC_DELCLDLG_HXX


