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

#ifndef _SV_FILECTRL_HXX
#define _SV_FILECTRL_HXX

#include "svtools/svtdllapi.h"
#include <vcl/window.hxx>
#include <vcl/edit.hxx>
#ifndef _SV_BUTTON_HXX
#include <vcl/button.hxx>
#endif


#define STR_FILECTRL_BUTTONTEXT		333		// ID-Range?!

// Flags for FileControl
typedef USHORT FileControlMode;
#define FILECTRL_RESIZEBUTTONBYPATHLEN	((USHORT)0x0001)//if this is set, the button will become small as soon as the Text in the Edit Field is to long to be shown completely


// Flags for internal use of FileControl
typedef USHORT FileControlMode_Internal;
#define FILECTRL_INRESIZE				((USHORT)0x0001)
#define FILECTRL_ORIGINALBUTTONTEXT		((USHORT)0x0002)


class SVT_DLLPUBLIC FileControl : public Window
{
private:
	Edit			maEdit;
	PushButton		maButton;

	String			maButtonText;
	BOOL			mbOpenDlg;

	Link			maDialogCreatedHdl;

	FileControlMode				mnFlags;
	FileControlMode_Internal	mnInternalFlags;

private:
	SVT_DLLPRIVATE void		ImplBrowseFile( );

protected:
	SVT_DLLPRIVATE void		Resize();
	SVT_DLLPRIVATE void		GetFocus();
	SVT_DLLPRIVATE void     StateChanged( StateChangedType nType );
	SVT_DLLPRIVATE WinBits  ImplInitStyle( WinBits nStyle );
	DECL_DLLPRIVATE_LINK( ButtonHdl, PushButton* );

public:
					FileControl( Window* pParent, WinBits nStyle, FileControlMode = 0 );
					~FileControl();

	Edit&			GetEdit() { return maEdit; }
	PushButton&		GetButton() { return maButton; }

	void			Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, ULONG nFlags );

	void			SetOpenDialog( BOOL bOpen )		{ mbOpenDlg = bOpen; }
	BOOL			IsOpenDialog() const			{ return mbOpenDlg; }

	void			SetText( const XubString& rStr );
	XubString		GetText() const;
	UniString			GetSelectedText() const			{ return maEdit.GetSelected(); }

	void 			SetSelection( const Selection& rSelection ) { maEdit.SetSelection( rSelection ); }
	Selection 		GetSelection() const 						{ return maEdit.GetSelection(); }

	void			SetReadOnly( BOOL bReadOnly = TRUE ) 	{ maEdit.SetReadOnly( bReadOnly ); }
	BOOL			IsReadOnly() const 						{ return maEdit.IsReadOnly(); }

	//------
	//manipulate the Button-Text:
	XubString		GetButtonText() const { return maButtonText; }
	void			SetButtonText( const XubString& rStr );
	void			ResetButtonText();
						  
	//------
	//use this to manipulate the dialog bevore executing it:
	void			SetDialogCreatedHdl( const Link& rLink ) { maDialogCreatedHdl = rLink; }
	const Link& 	GetDialogCreatedHdl() const { return maDialogCreatedHdl; }
};

#endif

