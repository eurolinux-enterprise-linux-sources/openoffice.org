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
#ifndef _SFXDOCKWIN_HXX
#define _SFXDOCKWIN_HXX

#include "sal/config.h"
#include "sal/types.h"
#include <vcl/dockwin.hxx>

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/frame/XFrame.hpp>

#include "sfx2/dllapi.h"
#include <sfx2/childwin.hxx>

class SfxSplitWindow;
class SfxDockingWindow_Impl;

void SFX2_DLLPUBLIC SAL_CALL SfxDockingWindowFactory( const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame, const rtl::OUString& rDockingWindowName );
bool SFX2_DLLPUBLIC SAL_CALL IsDockingWindowVisible( const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame, const rtl::OUString& rDockingWindowName );

class SFX2_DLLPUBLIC SfxDockingWindow : public DockingWindow
{
private:
	Rectangle               aInnerRect;
	Rectangle               aOuterRect;
	SfxBindings*            pBindings;
	Size                    aFloatSize;
	SfxChildWindow*         pMgr;
	SfxDockingWindow_Impl*  pImp;

    SAL_DLLPRIVATE SfxDockingWindow(SfxDockingWindow &); // not defined
    SAL_DLLPRIVATE void operator =(SfxDockingWindow &); // not defined

protected:
	SfxChildAlignment   CalcAlignment(const Point& rPos, Rectangle& rRect );
	void                CalcSplitPosition(const Point rPos, Rectangle& rRect,
							SfxChildAlignment eAlign);
	virtual Size        CalcDockingSize(SfxChildAlignment);
	virtual SfxChildAlignment
						CheckAlignment(SfxChildAlignment,SfxChildAlignment);

	virtual void        Resize();
	virtual BOOL        PrepareToggleFloatingMode();
	virtual void        ToggleFloatingMode();
	virtual void        StartDocking();
	virtual BOOL        Docking( const Point& rPos, Rectangle& rRect );
	virtual void        EndDocking( const Rectangle& rRect, BOOL bFloatMode );
	virtual void        Resizing( Size& rSize );
	virtual void        Paint( const Rectangle& rRect );
	virtual BOOL        Close();
    virtual void        Move();

//#if 0 // _SOLAR__PRIVATE
	SAL_DLLPRIVATE SfxChildWindow* GetChildWindow_Impl()	{ return pMgr; }
//#endif

public:
						SfxDockingWindow( SfxBindings *pBindings,
										  SfxChildWindow *pCW,
										  Window* pParent,
										  WinBits nWinBits=0);
						SfxDockingWindow( SfxBindings *pBindings,
										  SfxChildWindow *pCW,
										  Window* pParent,
										  const ResId& rResId);
						~SfxDockingWindow();

	void                Initialize (SfxChildWinInfo* pInfo);
	virtual void        FillInfo(SfxChildWinInfo&) const;
	virtual void		StateChanged( StateChangedType nStateChange );

	void                SetDockingRects(const Rectangle& rOuter, const Rectangle& rInner)
							{ aInnerRect = rInner; aOuterRect = rOuter; }
	const Rectangle&    GetInnerRect() const					{ return aInnerRect; }
	const Rectangle&    GetOuterRect() const					{ return aOuterRect; }
	SfxBindings&        GetBindings() const						{ return *pBindings; }
	USHORT              GetType() const							{ return pMgr->GetType(); }
	SfxChildAlignment   GetAlignment() const					{ return pMgr->GetAlignment(); }
	void                SetAlignment(SfxChildAlignment eAlign)	{ pMgr->SetAlignment(eAlign); }
	Size                GetFloatingSize() const					{ return aFloatSize; }
	void                SetFloatingSize(const Size& rSize)		{ aFloatSize=rSize; }

	void                SetMinOutputSizePixel( const Size& rSize );
	Size                GetMinOutputSizePixel() const;
	virtual long		Notify( NotifyEvent& rNEvt );
	virtual void        FadeIn( BOOL );
    void                AutoShow( BOOL bShow = TRUE );
	DECL_LINK( TimerHdl, Timer* );

//#if 0 // _SOLAR__PRIVATE
	SAL_DLLPRIVATE void Initialize_Impl();
	SAL_DLLPRIVATE USHORT GetWinBits_Impl() const;
	SAL_DLLPRIVATE void SetItemSize_Impl( const Size& rSize );
	SAL_DLLPRIVATE void Disappear_Impl();
	SAL_DLLPRIVATE void Reappear_Impl();
	SAL_DLLPRIVATE BOOL IsAutoHide_Impl() const;
	SAL_DLLPRIVATE BOOL IsPinned_Impl() const;
	SAL_DLLPRIVATE void AutoShow_Impl( BOOL bShow = TRUE );
	SAL_DLLPRIVATE void Pin_Impl( BOOL bPinned );
	SAL_DLLPRIVATE SfxSplitWindow* GetSplitWindow_Impl() const;
    SAL_DLLPRIVATE void ReleaseChildWindow_Impl();
//#endif
};

class SfxDockingWrapper : public SfxChildWindow
{
	public:
		SfxDockingWrapper( Window* pParent ,
						   USHORT nId ,
						   SfxBindings* pBindings ,
						   SfxChildWinInfo* pInfo );

		SFX_DECL_CHILDWINDOW(SfxDockingWrapper);
};

#endif // #ifndef _SFXDOCKWIN_HXX
