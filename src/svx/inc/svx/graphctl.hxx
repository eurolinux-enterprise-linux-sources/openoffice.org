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
#ifndef _GRAPHCTL_HXX
#define _GRAPHCTL_HXX

// include ---------------------------------------------------------------

#include <vcl/ctrl.hxx>
#include <vcl/graph.hxx>
#include "svx/svxdllapi.h"

#include <svx/svdview.hxx>
#include <svx/svdobj.hxx>

/*************************************************************************
|*
|*
|*
\************************************************************************/

class GraphCtrlUserCall;
class SvxGraphCtrlAccessibleContext;

#define WB_SDRMODE		((WinBits)0x0080)
#define WB_ANIMATION	((WinBits)0x0100)

class SVX_DLLPUBLIC GraphCtrl : public Control
{
	friend class GraphCtrlView;
	friend class GraphCtrlUserCall;

	Graphic				aGraphic;
	Timer				aUpdateTimer;
	Link				aMousePosLink;
	Link				aGraphSizeLink;
	Link				aMarkObjLink;
	Link				aUpdateLink;
	MapMode				aMap100;
	Size				aGraphSize;
	Point				aMousePos;
	GraphCtrlUserCall*	pUserCall;
	WinBits				nWinStyle;
	SdrObjKind			eObjKind;
	USHORT				nPolyEdit;
	BOOL				bEditMode;
	BOOL				bSdrMode;
	BOOL				bAnim;

						DECL_LINK( UpdateHdl, Timer* );

	SvxGraphCtrlAccessibleContext* mpAccContext;

protected:

	SdrModel*			pModel;
	SdrView*			pView;

	virtual void		Paint( const Rectangle& rRect );
	virtual void		Resize();
	virtual void		KeyInput(const KeyEvent& rKEvt);
	virtual void		MouseButtonDown(const MouseEvent& rMEvt);
	virtual void		MouseButtonUp(const MouseEvent& rMEvt);
	virtual void		MouseMove(const MouseEvent& rMEvt);

	virtual void		InitSdrModel();

	virtual void		SdrObjCreated( const SdrObject& rObj );
	virtual void		SdrObjChanged( const SdrObject& rObj );
	virtual	void		MarkListHasChanged();

	SdrObjUserCall*		GetSdrUserCall() { return (SdrObjUserCall*) pUserCall; }

public:

						GraphCtrl( Window* pParent, const WinBits nWinBits = 0 );
						GraphCtrl( Window* pParent, const ResId& rResId );
						~GraphCtrl();

	void				SetWinStyle( WinBits nWinBits );
	WinBits				GetWinStyle() const { return nWinStyle; }

	void				SetGraphic( const Graphic& rGraphic, BOOL bNewModel = TRUE );
	const Graphic&		GetGraphic() const { return aGraphic; }
	const Size&			GetGraphicSize() const { return aGraphSize; }

	const Point&		GetMousePos() const { return aMousePos; }

	void				SetEditMode( const BOOL bEditMode );
	BOOL				IsEditMode() const { return bEditMode; }

	void				SetPolyEditMode( const USHORT nPolyEdit );
	USHORT				GetPolyEditMode() const { return nPolyEdit; }

	void				SetObjKind( const SdrObjKind eObjKind );
	SdrObjKind			GetObjKind() const { return eObjKind; }

	SdrModel*			GetSdrModel() const { return pModel; }
	SdrView*			GetSdrView() const { return pView; }
	SdrObject*			GetSelectedSdrObject() const;
	BOOL				IsChanged() const { return bSdrMode ? pModel->IsChanged() : FALSE; }

	void				SetMousePosLink( const Link& rLink ) { aMousePosLink = rLink; }
	const Link&			GetMousePosLink() const { return aMousePosLink; }

	void				SetGraphSizeLink( const Link& rLink ) { aGraphSizeLink = rLink; }
	const Link&			GetGraphSizeLink() const { return aGraphSizeLink; }

	void				SetMarkObjLink( const Link& rLink ) { aMarkObjLink = rLink; }
	const Link&			GetMarkObjLink() const { return aMarkObjLink; }

	void				SetUpdateLink( const Link& rLink ) { aUpdateLink = rLink; }
	const Link&			GetUpdateLink() const { return aUpdateLink; }

	static String		GetStringFromDouble( const double& rDouble );

    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > CreateAccessible();
};

//#if 0 // _SOLAR__PRIVATE

/*************************************************************************
|*
|*
|*
\************************************************************************/

class GraphCtrlUserCall : public SdrObjUserCall
{
	GraphCtrl&		rWin;

public:

					GraphCtrlUserCall( GraphCtrl& rGraphWin ) : rWin( rGraphWin ) {};
	virtual			~GraphCtrlUserCall() {};

	virtual void	Changed( const SdrObject& rObj, SdrUserCallType eType, const Rectangle& rOldBoundRect );
};

/*************************************************************************
|*
|*
|*
\************************************************************************/

class GraphCtrlView : public SdrView
{
	GraphCtrl&		rGraphCtrl;

protected:

	virtual void	MarkListHasChanged()
					{
						SdrView::MarkListHasChanged();
						rGraphCtrl.MarkListHasChanged();
					}

public:

					GraphCtrlView( SdrModel* pModel, GraphCtrl* pWindow) :
							SdrView		( pModel, pWindow ),
							rGraphCtrl	( *pWindow ) {};

	virtual			~GraphCtrlView() {};
};

//#endif // __PRIVATE

#endif // _GRAPHCTL_HXX

