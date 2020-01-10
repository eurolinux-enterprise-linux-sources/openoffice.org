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


#ifndef _IMAPDLG_HXX_
#define _IMAPDLG_HXX_

#include <svtools/inettbc.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/ctrlitem.hxx>
#include <sfx2/basedlgs.hxx>

#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif

#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif

#ifndef _EDIT_HXX //autogen
#include <vcl/edit.hxx>
#endif

#ifndef _TOOLBOX_HXX //autogen
#include <vcl/toolbox.hxx>
#endif

#ifndef _STATUS_HXX //autogen
#include <vcl/status.hxx>
#endif
#include "svx/svxdllapi.h"


#ifndef _GOMISC_HXX
class ImageMap;
#endif


/*************************************************************************
|*
|* Ableitung vom SfxChildWindow als "Behaelter" fuer Float
|*
\************************************************************************/

class Graphic;
class TargetList;

class SVX_DLLPUBLIC SvxIMapDlgChildWindow : public SfxChildWindow
{
 public:

	SvxIMapDlgChildWindow( Window*, USHORT, SfxBindings*, SfxChildWinInfo* );

	SFX_DECL_CHILDWINDOW( SvxIMapDlgChildWindow );

	static void UpdateIMapDlg( const Graphic& rGraphic, const ImageMap* pImageMap = NULL,
							   const TargetList* pTargetList = NULL, void* pEditingObj = NULL );
};


#ifndef _REDUCED_IMAPDLG_HXX_
#define _REDUCED_IMAPDLG_HXX_

/*************************************************************************
|*
|*
|*
\************************************************************************/

class SvxIMapDlg;

class SvxIMapDlgItem : public SfxControllerItem
{
	SvxIMapDlg&	rIMap;


protected:

	virtual void StateChanged( USHORT nSID, SfxItemState eState,
							   const SfxPoolItem* pState );


public:

	SvxIMapDlgItem( USHORT nId, SvxIMapDlg& rIMapDlg, SfxBindings& rBindings );
};


/*************************************************************************
|*
|*
|*
\************************************************************************/

class IMapOwnData;
class IMapWindow;

class SVX_DLLPUBLIC SvxIMapDlg : public SfxModelessDialog // SfxFloatingWindow
{
	friend class IMapOwnData;
	friend class IMapWindow;
	using Window::Update;

	ToolBox				aTbxIMapDlg1;
	FixedText			aFtURL;
	SvtURLBox			maURLBox;
	FixedText			aFtText;
	Edit				aEdtText;
	FixedText			maFtTarget;
	ComboBox			maCbbTarget;
	StatusBar			aStbStatus;
	ImageList			maImageList;
	ImageList			maImageListH;

	Size				aLastSize;
	IMapWindow*			pIMapWnd;
	IMapOwnData*		pOwnData;
	void*				pCheckObj;
	SvxIMapDlgItem		aIMapItem;

	virtual	void		Resize();
	virtual BOOL 		Close();

#ifdef _IMAPDLG_PRIVATE

						DECL_LINK( TbxClickHdl, ToolBox* );
						DECL_LINK( InfoHdl, IMapWindow* );
						DECL_LINK( MousePosHdl, IMapWindow* );
						DECL_LINK( GraphSizeHdl, IMapWindow* );
						DECL_LINK( URLModifyHdl, void* );
						DECL_LINK( URLLoseFocusHdl, void* );
						DECL_LINK( UpdateHdl, Timer* );
						DECL_LINK( TbxUpdateHdl, Timer* );
						DECL_LINK( StateHdl, IMapWindow* );
						DECL_LINK( MiscHdl, void* );

	void				DoOpen();
	BOOL				DoSave();

#endif


public:

						SvxIMapDlg( SfxBindings *pBindings, SfxChildWindow *pCW,
									Window* pParent, const ResId& rResId );
						~SvxIMapDlg();

	void				SetExecState( BOOL bEnable );

	void				SetGraphic( const Graphic& rGraphic );

	void				SetEditingObject( void* pObj ) { pCheckObj = pObj; }
	const void*			GetEditingObject() const { return pCheckObj; }

	void				SetImageMap( const ImageMap& rImageMap );
	const ImageMap&		GetImageMap() const;

	void				SetTargetList( const TargetList& rTargetList );
	const TargetList&	GetTargetList() const;

	void				Update( const Graphic& rGraphic, const ImageMap* pImageMap = NULL,
								const TargetList* pTargetList = NULL, void* pEditingObj = NULL );

	virtual void		KeyInput( const KeyEvent& rKEvt );

	virtual void DataChanged( const DataChangedEvent& rDCEvt );
	void ApplyImageList();
};


/*************************************************************************
|*
|* Defines
|*
\************************************************************************/

#define SVXIMAPDLG() ( (SvxIMapDlg*) ( SfxViewFrame::Current()->GetChildWindow( 		\
						SvxIMapDlgChildWindow::GetChildWindowId() )-> 	\
						GetWindow() ) )


#endif // _REDUCED_IMAPDLG_HXX_
#endif // _IMAPDLG_HXX_


