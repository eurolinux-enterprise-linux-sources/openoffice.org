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

#ifndef _DLGEPS_HXX_
#define _DLGEPS_HXX_
#include <svtools/fltcall.hxx>
#include <vcl/dialog.hxx>
#include <vcl/button.hxx>
#include <vcl/fixed.hxx>
#include <vcl/field.hxx>
#include <vcl/lstbox.hxx>
#include <svtools/stdctrl.hxx>


/*************************************************************************
|*
|* Dialog zum Einstellen von Filteroptionen
|*
\************************************************************************/

class FilterConfigItem;
class ResMgr;

class DlgExportEPS : public ModalDialog
{
private:

	FltCallDialogParameter& rFltCallPara;

	FixedLine			aGrpPreview;
	CheckBox			aCBPreviewTiff;
	CheckBox			aCBPreviewEPSI;
	FixedLine			aGrpVersion;
	RadioButton 		aRBLevel1;
	RadioButton 		aRBLevel2;
	FixedLine 			aGrpColor;
	RadioButton 		aRBColor;
	RadioButton 		aRBGrayscale;
	FixedLine 			aGrpCompression;
	RadioButton 		aRBCompressionLZW;
	RadioButton 		aRBCompressionNone;
	OKButton			aBtnOK;
	CancelButton		aBtnCancel;
	HelpButton			aBtnHelp;

	FilterConfigItem* 	pConfigItem;
	ResMgr* 			pMgr;

	DECL_LINK( OK, void * );
	DECL_LINK( LEVEL1, void* );
	DECL_LINK( LEVEL2, void* );

public:
			DlgExportEPS( FltCallDialogParameter& rPara );
			~DlgExportEPS();
};

#endif // _DLGEPS_HXX_
