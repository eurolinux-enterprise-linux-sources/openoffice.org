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

#ifndef _MACROPG_IMPL_HXX
#define _MACROPG_IMPL_HXX

class _SvxMacroTabPage_Impl
{
public:
									_SvxMacroTabPage_Impl( const SfxItemSet& rAttrSet );
									~_SvxMacroTabPage_Impl();

	FixedText*						pAssignFT;
	PushButton*						pAssignPB;
	PushButton*						pAssignComponentPB;
	PushButton*						pDeletePB;
	Image*							pMacroImg;
	Image*							pComponentImg;
	Image*							pMacroImg_h;
	Image*							pComponentImg_h;
	String*							pStrEvent;
	String*							pAssignedMacro;
	_HeaderTabListBox*				pEventLB;
	BOOL							bReadOnly;
	BOOL							bIDEDialogMode;
};

class AssignComponentDialog : public ModalDialog
{
private:
	FixedText		maMethodLabel;
	Edit			maMethodEdit;
	OKButton		maOKButton;
	CancelButton	maCancelButton;
	HelpButton		maHelpButton;

	::rtl::OUString maURL;

    DECL_LINK(ButtonHandler, Button *);

public:
	AssignComponentDialog( Window * pParent, const ::rtl::OUString& rURL );
	~AssignComponentDialog();

	::rtl::OUString getURL( void ) const
		{ return maURL; }
};

#endif
