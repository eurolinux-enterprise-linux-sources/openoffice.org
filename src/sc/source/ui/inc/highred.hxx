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

#ifndef SC_HIGHRED_HXX
#define SC_HIGHRED_HXX

#ifndef _MOREBTN_HXX //autogen
#include <vcl/morebtn.hxx>
#endif
#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif
#ifndef _GROUP_HXX //autogen
#include <vcl/group.hxx>
#endif
#include <svtools/headbar.hxx>
#include <svtools/svtabbx.hxx>


#include "rangenam.hxx"
#include "anyrefdg.hxx"

#ifndef _MOREBTN_HXX //autogen
#include <vcl/morebtn.hxx>
#endif
#include <vcl/lstbox.hxx>

#ifndef _SVX_ACREDLIN_HXX
#include <svx/ctredlin.hxx>
#endif
#include <svx/simptabl.hxx>
#include "chgtrack.hxx"
#include "chgviset.hxx"

class ScViewData;
class ScDocument;

#ifndef	FLT_DATE_BEFORE
#define FLT_DATE_BEFORE		0
#define FLT_DATE_SINCE		1
#define FLT_DATE_EQUAL		2
#define FLT_DATE_NOTEQUAL	3
#define FLT_DATE_BETWEEN	4
#define FLT_DATE_SAVE		5
#endif

//==================================================================

class ScHighlightChgDlg : public ScAnyRefDlg
{
private:

	CheckBox				aHighlightBox;
    FixedLine               aFlFilter;
	SvxTPFilter			 	aFilterCtr;
	CheckBox				aCbAccept;
	CheckBox				aCbReject;

	OKButton				aOkButton;
	CancelButton			aCancelButton;
	HelpButton				aHelpButton;

	formula::RefEdit				aEdAssign;
	formula::RefButton				aRbAssign;

	ScViewData*				pViewData;
	ScDocument*				pDoc;
	ScRangeName				aLocalRangeName;
	Selection				theCurSel;
	Size					MinSize;
	ScRangeList				aRangeList;
	ScChangeViewSettings	aChangeViewSet;

	void					Init();

	DECL_LINK( RefHandle, SvxTPFilter* );
	DECL_LINK(HighLightHandle, CheckBox*);
	DECL_LINK(OKBtnHdl, PushButton*);


protected:

	virtual void	RefInputDone( BOOL bForced = FALSE );

public:
					ScHighlightChgDlg( SfxBindings* pB, SfxChildWindow* pCW, Window* pParent,
							   ScViewData*		ptrViewData);

					~ScHighlightChgDlg();

	virtual void	SetActive();
	virtual void	SetReference( const ScRange& rRef, ScDocument* pDoc );
	virtual BOOL	Close();
	virtual BOOL	IsRefInputMode() const;

};


#endif // SC_NAMEDLG_HXX

