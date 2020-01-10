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

#ifndef SC_PFILTDLG_HXX
#define SC_PFILTDLG_HXX

#ifndef _SV_HXX
#endif

#ifndef _MOREBTN_HXX //autogen
#include <vcl/morebtn.hxx>
#endif
#include <svtools/stdctrl.hxx>
#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif
#ifndef _DIALOG_HXX //autogen
#include <vcl/dialog.hxx>
#endif
#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif
#include "global.hxx" // -> ScQueryParam
#include "address.hxx"

//------------------------------------------------------------------

class ScViewData;
class ScDocument;
class ScQueryItem;
class TypedScStrCollection;

//==================================================================

class ScPivotFilterDlg : public ModalDialog
{
public:
					ScPivotFilterDlg( Window* pParent,
									  const SfxItemSet&	rArgSet, SCTAB nSourceTab );
					~ScPivotFilterDlg();

	const ScQueryItem&	GetOutputItem();

private:
    FixedLine       aFlCriteria;
	//----------------------------
	ListBox			aLbField1;
	ListBox			aLbCond1;
	ComboBox		aEdVal1;
	//----------------------------
	ListBox			aLbConnect1;
	ListBox			aLbField2;
	ListBox			aLbCond2;
	ComboBox		aEdVal2;
	//----------------------------
	ListBox			aLbConnect2;
	ListBox			aLbField3;
	ListBox			aLbCond3;
	ComboBox		aEdVal3;
	//----------------------------
	FixedText		aFtConnect;
	FixedText		aFtField;
	FixedText		aFtCond;
	FixedText		aFtVal;

    FixedLine       aFlOptions;
	CheckBox		aBtnCase;
	CheckBox		aBtnRegExp;
	CheckBox		aBtnUnique;
	FixedText		aFtDbAreaLabel;
	FixedInfo		aFtDbArea;
	OKButton		aBtnOk;
	CancelButton	aBtnCancel;
	HelpButton		aBtnHelp;
	MoreButton		aBtnMore;
	const String	aStrUndefined;
	const String	aStrNoName;
	const String	aStrNone;
	const String	aStrEmpty;
	const String	aStrNotEmpty;
	const String	aStrRow;
	const String	aStrColumn;

	const USHORT		nWhichQuery;
	const ScQueryParam	theQueryData;
	ScQueryItem*		pOutItem;
	ScViewData*			pViewData;
	ScDocument*			pDoc;
	SCTAB				nSrcTab;

	USHORT				nFieldCount;
	ComboBox*			aValueEdArr[3];
	ListBox*			aFieldLbArr[3];
	ListBox*			aCondLbArr[3];

	TypedScStrCollection*	pEntryLists[MAXCOLCOUNT];

#ifdef _PFILTDLG_CXX
private:
	void	Init			( const SfxItemSet&	rArgSet );
	void	FillFieldLists	();
	void	UpdateValueList	( USHORT nList );
	void	ClearValueList	( USHORT nList );
	USHORT	GetFieldSelPos	( SCCOL nField );

	// Handler:
	DECL_LINK( LbSelectHdl, ListBox* );
	DECL_LINK( ValModifyHdl, ComboBox* );
	DECL_LINK( CheckBoxHdl,	 CheckBox* );
#endif
};


#endif // SC_PFILTDLG_HXX

