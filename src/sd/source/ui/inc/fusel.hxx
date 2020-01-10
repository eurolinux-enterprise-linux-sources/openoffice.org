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

#ifndef SD_FU_SELECTION_HXX
#define SD_FU_SELECTION_HXX

#include "fudraw.hxx"

#include <com/sun/star/media/XPlayer.hpp>

class SdrHdl;
class SdrObject;
class Sound;


namespace sd {

class FuSelection 
    : public FuDraw
{
public:
	TYPEINFO();

	static FunctionReference Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq );
	virtual void DoExecute( SfxRequest& rReq );

									   // Mouse- & Key-Events
	virtual BOOL KeyInput(const KeyEvent& rKEvt);
	virtual BOOL MouseMove(const MouseEvent& rMEvt);
	virtual BOOL MouseButtonUp(const MouseEvent& rMEvt);
	virtual BOOL MouseButtonDown(const MouseEvent& rMEvt);

	virtual void Activate();		   // Function aktivieren
	virtual void Deactivate();		   // Function deaktivieren

	virtual void SelectionHasChanged();

	void    SetEditMode(USHORT nMode);
	USHORT  GetEditMode() { return nEditMode; }

	BOOL	AnimateObj(SdrObject* pObj, const Point& rPos);

	/** is called when the currenct function should be aborted. <p>
		This is used when a function gets a KEY_ESCAPE but can also
		be called directly.

		@returns true if a active function was aborted
	*/
	virtual bool cancel();

protected:
	FuSelection (ViewShell* pViewSh, 
        ::sd::Window* pWin, 
        ::sd::View* pView,
        SdDrawDocument* pDoc, 
        SfxRequest& rReq);

	virtual ~FuSelection();

	BOOL			bTempRotation;
	BOOL            bSelectionChanged;
	BOOL            bHideAndAnimate;
	SdrHdl*         pHdl;
	BOOL            bSuppressChangesOfSelection;
	BOOL            bMirrorSide0;
	USHORT          nEditMode;
        ::com::sun::star::uno::Reference< ::com::sun::star::media::XPlayer > mxPlayer;

private:
    /** This pointer stores a canidate for assigning a style in the water
        can mode between mouse button down and mouse button up.
    */
    SdrObject* pWaterCanCandidate;

    /** Find the object under the given test point without selecting it.
        @param rTestPoint
            The coordinates at which to search for a shape.
        @return
            The shape at the test point.  When there is no shape at this
            position then NULL is returned.
    */
    SdrObject* pickObject (const Point& rTestPoint);
};

} // end of namespace sd

#endif		// _SD_FUSEL_HXX

