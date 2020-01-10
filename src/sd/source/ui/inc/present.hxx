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


#ifndef _SD_PRESENT_HXX_
#define _SD_PRESENT_HXX_

#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#include <vcl/fixed.hxx>
#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#ifndef _DIALOG_HXX //autogen
#include <vcl/dialog.hxx>
#endif
#ifndef _FIELD_HXX //autogen
#include <vcl/field.hxx>
#endif

class SfxItemSet;
class List;

/*************************************************************************
|* Dialog zum Festlegen von Optionen und Starten der Praesentation
\************************************************************************/
class SdStartPresentationDlg : public ModalDialog
{
private:

	FixedLine			aGrpRange;
	RadioButton			aRbtAll;
	RadioButton			aRbtAtDia;
	RadioButton			aRbtCustomshow;
	ListBox				aLbDias;
	ListBox				aLbCustomshow;

	FixedLine			aGrpKind;
	RadioButton			aRbtStandard;
	RadioButton			aRbtWindow;
	RadioButton			aRbtAuto;
	TimeField			aTmfPause;
	CheckBox			aCbxAutoLogo;

	FixedLine			aGrpOptions;
	CheckBox			aCbxManuel;
	CheckBox			aCbxMousepointer;
	CheckBox			aCbxPen;
	CheckBox			aCbxNavigator;
	CheckBox            aCbxAnimationAllowed;
	CheckBox            aCbxChangePage;
	CheckBox			aCbxAlwaysOnTop;

	FixedLine			maGrpMonitor;
	FixedText			maFtMonitor;
	ListBox				maLBMonitor;

	OKButton			aBtnOK;
	CancelButton		aBtnCancel;
	HelpButton			aBtnHelp;

	List*				pCustomShowList;
	const SfxItemSet&	rOutAttrs;
	sal_Int32			mnMonitors;

	String				msPrimaryMonitor;
	String				msMonitor;
	String				msAllMonitors;

						DECL_LINK( ChangeRangeHdl, void * );
						DECL_LINK( ClickWindowPresentationHdl, void * );
						DECL_LINK( ChangePauseHdl, void * );

	void				InitMonitorSettings();

public:
						SdStartPresentationDlg( Window* pWindow,
								const SfxItemSet& rInAttrs,
								List& rPageNames,
								List* pCSList );

	void				GetAttr( SfxItemSet& rOutAttrs );
};

#endif // _SD_PRESENT_HXX_

