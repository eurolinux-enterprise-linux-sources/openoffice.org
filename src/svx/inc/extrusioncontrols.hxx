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
#ifndef _SVX_EXTRUSION_CONTROLS_HXX
#define _SVX_EXTRUSION_CONTROLS_HXX

#include <svtools/valueset.hxx>
#include <svtools/lstner.hxx>
#include <sfx2/tbxctrl.hxx>
#include <svtools/svtreebx.hxx>
#ifndef _SV_BUTTON_HXX
#include <vcl/button.hxx>
#endif
#include <vcl/dialog.hxx>
#include <vcl/field.hxx>
#include <com/sun/star/frame/XFrame.hpp>
#include "svx/svxdllapi.h"

class SfxBindings;
class ToolbarMenu;
class SfxStatusForwarder;

//========================================================================

namespace svx
{
class ToolboxButtonColorUpdater;

class ExtrusionDirectionWindow : public SfxPopupWindow
{
	using FloatingWindow::StateChanged;

private:
	ToolbarMenu*		mpMenu;
	ValueSet*			mpDirectionSet;
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > mxFrame;

	SfxStatusForwarder* mpDirectionForewarder;
	SfxStatusForwarder* mpProjectionForewarder;

	Image		maImgDirection[9];
	Image		maImgDirectionH[9];

	Image		maImgPerspective;
	Image		maImgPerspectiveH;
	Image		maImgParallel;
	Image		maImgParallelH;

	bool		mbPopupMode;

	DECL_LINK( SelectHdl, void * );
	void			FillValueSet();

	void implSetDirection( sal_Int32 nSkew, bool bEnabled = true );
	void implSetProjection( sal_Int32 nProjection, bool bEnabled = true );
    void implInit();

protected:
	virtual BOOL	Close();
	virtual void	PopupModeEnd();

    /** This function is called when the window gets the focus.  It grabs
        the focus to the line ends value set so that it can be controlled with
        the keyboard.
    */
	virtual void GetFocus (void);

public:
	ExtrusionDirectionWindow( USHORT nId, 
							  const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame );
    ExtrusionDirectionWindow( USHORT nId,
                              const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
                              Window* pParentWindow );
    ~ExtrusionDirectionWindow();

	void            StartSelection();

	virtual SfxPopupWindow* Clone() const;

	virtual void StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );
	virtual void DataChanged( const DataChangedEvent& rDCEvt );
};

//========================================================================

class SVX_DLLPUBLIC ExtrusionDirectionControl : public SfxToolBoxControl
{
public:
	SFX_DECL_TOOLBOX_CONTROL();
	ExtrusionDirectionControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
	~ExtrusionDirectionControl();

	virtual void				StateChanged( USHORT nSID, SfxItemState eState,
											  const SfxPoolItem* pState );
	virtual SfxPopupWindowType	GetPopupWindowType() const;
	virtual SfxPopupWindow*		CreatePopupWindow();
};

//========================================================================

class ExtrusionDepthWindow : public SfxPopupWindow
{
	using FloatingWindow::StateChanged;

private:
	ToolbarMenu* mpMenu;

	Image maImgDepth0;
	Image maImgDepth1;
	Image maImgDepth2;
	Image maImgDepth3;
	Image maImgDepth4;
	Image maImgDepthInfinity;

	Image maImgDepth0h;
	Image maImgDepth1h;
	Image maImgDepth2h;
	Image maImgDepth3h;
	Image maImgDepth4h;
	Image maImgDepthInfinityh;

	SfxStatusForwarder* mpDepthForewarder;
	SfxStatusForwarder* mpMetricForewarder;
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > mxFrame;

	bool		mbPopupMode;
	FieldUnit	meUnit;
	double		mfDepth;
	bool		mbEnabled;

	DECL_LINK( SelectHdl, void * );

	void	implFillStrings( FieldUnit eUnit );
	void	implSetDepth( double fDepth, bool bEnabled );
    void    implInit();

protected:
	virtual BOOL	Close();
	virtual void	PopupModeEnd();

    /** This function is called when the window gets the focus.  It grabs
        the focus to the line ends value set so that it can be controlled with
        the keyboard.
    */
	virtual void GetFocus (void);

public:
	ExtrusionDepthWindow( USHORT nId,
	                      const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame );
	ExtrusionDepthWindow( USHORT nId,
	                      const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
                          Window* pParentWindow );
	~ExtrusionDepthWindow();

	void            StartSelection();

	virtual SfxPopupWindow* Clone() const;

	virtual void StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );
	virtual void DataChanged( const DataChangedEvent& rDCEvt );
};

//========================================================================

class SVX_DLLPUBLIC ExtrusionDepthControl : public SfxToolBoxControl
{
public:
	SFX_DECL_TOOLBOX_CONTROL();
	ExtrusionDepthControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
	~ExtrusionDepthControl();

	virtual void				StateChanged( USHORT nSID, SfxItemState eState,
											  const SfxPoolItem* pState );
	virtual SfxPopupWindowType	GetPopupWindowType() const;
	virtual SfxPopupWindow*		CreatePopupWindow();
};

//========================================================================

class ExtrusionLightingWindow : public SfxPopupWindow
{
	using FloatingWindow::StateChanged;

private:
	ToolbarMenu* mpMenu;
	ValueSet*	 mpLightingSet;

	Image maImgLightingOff[9];
	Image maImgLightingOn[9];
	Image maImgLightingPreview[9];

	Image maImgLightingOffh[9];
	Image maImgLightingOnh[9];
	Image maImgLightingPreviewh[9];

	Image maImgBright;
	Image maImgNormal;
	Image maImgDim;
	Image maImgBrighth;
	Image maImgNormalh;
	Image maImgDimh;

	SfxStatusForwarder* mpLightingDirectionForewarder;
	SfxStatusForwarder* mpLightingIntensityForewarder;
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > mxFrame;

	bool	mbPopupMode;
	int		mnLevel;
	bool	mbLevelEnabled;
	int		mnDirection;
	bool	mbDirectionEnabled;

	void	implSetIntensity( int nLevel, bool bEnabled );
	void	implSetDirection( int nDirection, bool bEnabled );
    void    implInit();

	DECL_LINK( SelectHdl, void * );

protected:
	virtual BOOL	Close();
	virtual void	PopupModeEnd();

    /** This function is called when the window gets the focus.  It grabs
        the focus to the line ends value set so that it can be controlled with
        the keyboard.
    */
	virtual void GetFocus (void);

public:
	ExtrusionLightingWindow( USHORT nId, 
							 const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame );
	ExtrusionLightingWindow( USHORT nId, 
							 const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
                             Window* pParentWindow );
	~ExtrusionLightingWindow();

	void            StartSelection();

	virtual SfxPopupWindow* Clone() const;

	virtual void StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );
	virtual void DataChanged( const DataChangedEvent& rDCEvt );
};

//========================================================================

class SVX_DLLPUBLIC ExtrusionLightingControl : public SfxToolBoxControl
{
public:
	SFX_DECL_TOOLBOX_CONTROL();
	ExtrusionLightingControl( USHORT nSlotid, USHORT nId, ToolBox& rTbx );
	~ExtrusionLightingControl();

	virtual void				StateChanged( USHORT nSID, SfxItemState eState,
											  const SfxPoolItem* pState );
	virtual SfxPopupWindowType	GetPopupWindowType() const;
	virtual SfxPopupWindow*		CreatePopupWindow();
};

//========================================================================

class ExtrusionSurfaceWindow : public SfxPopupWindow
{
	using FloatingWindow::StateChanged;
private:
	ToolbarMenu* mpMenu;
	
	Image maImgSurface1;
	Image maImgSurface2;
	Image maImgSurface3;
	Image maImgSurface4;
	Image maImgSurface1h;
	Image maImgSurface2h;
	Image maImgSurface3h;
	Image maImgSurface4h;

	SfxStatusForwarder* mpSurfaceForewarder;
	::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > mxFrame;

	bool		mbPopupMode;

	DECL_LINK( SelectHdl, void * );

	void	implSetSurface( int nSurface, bool bEnabled );
    void    implInit();

protected:
	virtual BOOL	Close();
	virtual void	PopupModeEnd();

    /** This function is called when the window gets the focus.  It grabs
        the focus to the line ends value set so that it can be controlled with
        the keyboard.
    */
	virtual void GetFocus (void);

public:
	ExtrusionSurfaceWindow( USHORT nId,
							const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame );
	ExtrusionSurfaceWindow( USHORT nId,
							const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& rFrame,
                            Window* pParentWindow );
	~ExtrusionSurfaceWindow();

	void            StartSelection();

	virtual SfxPopupWindow* Clone() const;

	virtual void StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState );
	virtual void DataChanged( const DataChangedEvent& rDCEvt );
};

//========================================================================

class SVX_DLLPUBLIC ExtrusionSurfaceControl : public SfxToolBoxControl
{
public:
	SFX_DECL_TOOLBOX_CONTROL();
	ExtrusionSurfaceControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
	~ExtrusionSurfaceControl();

	virtual void				StateChanged( USHORT nSID, SfxItemState eState,
											  const SfxPoolItem* pState );
	virtual SfxPopupWindowType	GetPopupWindowType() const;
	virtual SfxPopupWindow*		CreatePopupWindow();
};

//========================================================================

class SVX_DLLPUBLIC ExtrusionColorControl : public SfxToolBoxControl
{
private:
	ToolboxButtonColorUpdater*	mpBtnUpdater;

public:
	SFX_DECL_TOOLBOX_CONTROL();
	ExtrusionColorControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx );
	~ExtrusionColorControl();

	virtual void				StateChanged( USHORT nSID, SfxItemState eState,
											  const SfxPoolItem* pState );
	virtual SfxPopupWindowType	GetPopupWindowType() const;
	virtual SfxPopupWindow*		CreatePopupWindow();
};

//========================================================================

class ExtrusionDepthDialog : public ModalDialog
{
	FixedText			maFLDepth;
	MetricField			maMtrDepth;
	OKButton			maOKButton;
	CancelButton		maCancelButton;
	HelpButton			maHelpButton;

	FieldUnit			meDefaultUnit;

public:
	ExtrusionDepthDialog( Window* pParent, double fDepth, FieldUnit eDefaultUnit );
	~ExtrusionDepthDialog();

	double getDepth() const;
};

}
#endif

