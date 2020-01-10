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

#ifndef _SVEDIT_HXX
#define _SVEDIT_HXX

#include <vcl/wintypes.hxx>
#include <vcl/edit.hxx>

#include <svtools/syntaxhighlight.hxx>
#include <svtools/svtdllapi.h>
#include <svtools/colorcfg.hxx>

class ImpSvMEdit;
class Timer;
class ExtTextEngine;
class ExtTextView;

class SVT_DLLPUBLIC MultiLineEdit : public Edit
{
private:
	ImpSvMEdit*		pImpSvMEdit;

	XubString		aSaveValue;
	Link			aModifyHdlLink;

	Timer*			pUpdateDataTimer;
	Link			aUpdateDataHdlLink;

protected:

	DECL_LINK( 		ImpUpdateDataHdl, Timer* );
	void 			StateChanged( StateChangedType nType );
	void 			DataChanged( const DataChangedEvent& rDCEvt );
	virtual long 	PreNotify( NotifyEvent& rNEvt );
	long 			Notify( NotifyEvent& rNEvt );
	void 			ImplInitSettings( BOOL bFont, BOOL bForeground, BOOL bBackground );
	WinBits 		ImplInitStyle( WinBits nStyle );

	ExtTextEngine*	GetTextEngine() const;
	ExtTextView*	GetTextView() const;
	ScrollBar*		GetHScrollBar() const;
	ScrollBar*		GetVScrollBar() const;

public:
					MultiLineEdit( Window* pParent, WinBits nWinStyle = WB_LEFT | WB_BORDER );
					MultiLineEdit( Window* pParent, const ResId& rResId );
					~MultiLineEdit();


	virtual void	Modify();
	virtual void	UpdateData();

	virtual void	SetModifyFlag();
	virtual void	ClearModifyFlag();
	virtual BOOL	IsModified() const;

	virtual void	EnableUpdateData( ULONG nTimeout = EDIT_UPDATEDATA_TIMEOUT );
	virtual void	DisableUpdateData() { delete pUpdateDataTimer; pUpdateDataTimer = NULL; }
	virtual ULONG	IsUpdateDataEnabled() const;

	virtual void	SetReadOnly( BOOL bReadOnly = TRUE );
	virtual BOOL	IsReadOnly() const;

	void			EnableFocusSelectionHide( BOOL bHide );
	BOOL			IsFocusSelectionHideEnabled() const;

	virtual void	SetMaxTextLen( xub_StrLen nMaxLen = 0 );
	virtual xub_StrLen GetMaxTextLen() const;

	virtual void	SetSelection( const Selection& rSelection );
	virtual const Selection& GetSelection() const;

	virtual void	    ReplaceSelected( const XubString& rStr );
	virtual void	    DeleteSelected();
	virtual XubString	GetSelected() const;
	virtual XubString	GetSelected( LineEnd aSeparator ) const;

	virtual void	Cut();
	virtual void	Copy();
	virtual void	Paste();

	virtual void    SetText( const XubString& rStr );
	virtual void    SetText( const XubString& rStr, const Selection& rNewSelection )
                    { SetText( rStr ); SetSelection( rNewSelection ); }
	String			GetText() const;
	String			GetText( LineEnd aSeparator ) const;
	String			GetTextLines() const;
	String			GetTextLines( LineEnd aSeparator ) const;

    void            SetRightToLeft( BOOL bRightToLeft );
    BOOL            IsRightToLeft() const;

	void			SaveValue() 						{ aSaveValue = GetText(); }
	const XubString&	GetSavedValue() const 				{ return aSaveValue; }

	void			SetModifyHdl( const Link& rLink ) 	{ aModifyHdlLink = rLink; }
	const Link&		GetModifyHdl() const 				{ return aModifyHdlLink; }

	void			SetUpdateDataHdl( const Link& rLink ) { aUpdateDataHdlLink = rLink; }
	const Link&		GetUpdateDataHdl() const { return aUpdateDataHdlLink; }

	virtual void	Resize();
	virtual void	GetFocus();

	Size			CalcMinimumSize() const;
	Size			CalcAdjustedSize( const Size& rPrefSize ) const;
    using Edit::CalcSize;
	Size			CalcSize( USHORT nColumns, USHORT nLines ) const;
	void			GetMaxVisColumnsAndLines( USHORT& rnCols, USHORT& rnLines ) const;

	void 			Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, ULONG nFlags );

   	void			SetLeftMargin( USHORT n );
	USHORT			GetLeftMargin() const;

    virtual
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XWindowPeer >
    GetComponentInterface(BOOL bCreate = TRUE);

    void            DisableSelectionOnFocus();
};

inline ULONG MultiLineEdit::IsUpdateDataEnabled() const
{
	return pUpdateDataTimer ? pUpdateDataTimer->GetTimeout() : 0;
}

#endif
