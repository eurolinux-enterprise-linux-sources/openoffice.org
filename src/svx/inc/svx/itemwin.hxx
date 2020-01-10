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
#ifndef _SVX_ITEMWIN_HXX
#define _SVX_ITEMWIN_HXX

#include <vcl/bitmap.hxx>

#include <svx/dlgctrl.hxx>
#include "svx/svxdllapi.h"

// forward ---------------------------------------------------------------

class XLineColorItem;
class XLineWidthItem;
class SfxObjectShell;

// class SvxLineBox ------------------------------------------------------

class SvxLineBox : public LineLB
{
	BmpColorMode	meBmpMode;
	USHORT			nCurPos;
	Timer			aDelayTimer;
    Size            aLogicalSize;
    BOOL            bRelease;
	SfxObjectShell* mpSh;
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > mxFrame;

#ifdef _SVX_ITEMWIN_CXX
					DECL_LINK( DelayHdl_Impl, Timer * );

	void			ReleaseFocus_Impl();
#endif
public:
	SvxLineBox( Window* pParent, 
                const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
				WinBits nBits = WB_BORDER | WB_DROPDOWN | WB_AUTOHSCROLL );
	~SvxLineBox();

	void FillControl();

protected:
	virtual void 	Select();
	virtual long	PreNotify( NotifyEvent& rNEvt );
	virtual long	Notify( NotifyEvent& rNEvt );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );

};

// class SvxColorBox -----------------------------------------------------

class SvxColorBox : public ColorLB
{
	using Window::Update;

	USHORT			nCurPos;
	USHORT			nId;
	Timer			aDelayTimer;
    Size            aLogicalSize;
    BOOL            bRelease;
    ::rtl::OUString maCommand;
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > mxFrame;

#ifdef _SVX_ITEMWIN_CXX
					DECL_LINK( DelayHdl_Impl, Timer * );

	void			ReleaseFocus_Impl();
#endif

public:
	SvxColorBox( Window* pParent, 
                 const rtl::OUString& rCommand,
                 const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
				 WinBits nBits = WB_BORDER | WB_DROPDOWN | WB_AUTOHSCROLL );
	~SvxColorBox();

	void			Update( const XLineColorItem* pItem );

protected:
	virtual void 	Select();
	virtual long	PreNotify( NotifyEvent& rNEvt );
	virtual long	Notify( NotifyEvent& rNEvt );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );

};

// class SvxMetricField --------------------------------------------------

class SVX_DLLPUBLIC SvxMetricField : public MetricField
{
	using Window::Update;

	String			aCurTxt;
	SfxMapUnit		ePoolUnit;
	FieldUnit		eDlgUnit;
    Size            aLogicalSize;
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > mxFrame;

#ifdef _SVX_ITEMWIN_CXX
	void			ReleaseFocus_Impl();
#endif

protected:
	virtual void 	Modify();
	virtual void    Down();
	virtual void    Up();		// Nur zur Sicherheit

	virtual long	PreNotify( NotifyEvent& rNEvt );
	virtual long	Notify( NotifyEvent& rNEvt );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );

public:
	SvxMetricField( Window* pParent, 
                    const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
					WinBits nBits = WB_BORDER | WB_SPIN | WB_REPEAT );
	~SvxMetricField();

	void			Update( const XLineWidthItem* pItem );
	void			SetCoreUnit( SfxMapUnit eUnit );
	void			RefreshDlgUnit();
};

// class SvxFillTypeBox --------------------------------------------------

class SvxFillTypeBox : public FillTypeLB
{
public:
	SvxFillTypeBox( Window* pParent, WinBits nBits = WB_BORDER | WB_DROPDOWN | WB_AUTOHSCROLL );
	~SvxFillTypeBox();

	void			Selected() { bSelect = TRUE; }
    BOOL            IsRelease() { return bRelease;}

protected:
	virtual long	PreNotify( NotifyEvent& rNEvt );
	virtual long	Notify( NotifyEvent& rNEvt );

private:
	USHORT			nCurPos;
	BOOL			bSelect;
    BOOL            bRelease;

#ifdef _SVX_ITEMWIN_CXX
	void			ReleaseFocus_Impl();
#endif
};

// class SvxFillAttrBox --------------------------------------------------

class SvxFillAttrBox : public FillAttrLB
{
public:
	SvxFillAttrBox( Window* pParent, WinBits nBits = WB_BORDER | WB_DROPDOWN | WB_AUTOHSCROLL );
	~SvxFillAttrBox();

    BOOL            IsRelease() { return bRelease;}

protected:
	virtual long	PreNotify( NotifyEvent& rNEvt );
	virtual long	Notify( NotifyEvent& rNEvt );
	virtual void	Select();

private:
	USHORT			nCurPos;
    BOOL            bRelease;

#ifdef _SVX_ITEMWIN_CXX
	void			ReleaseFocus_Impl();
#endif
};

#endif // #ifndef _SVX_ITEMWIN_HXX

