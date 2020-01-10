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

#ifndef SD_FU_CONSTRUCT_UNO_CONTROL_HXX
#define SD_FU_CONSTRUCT_UNO_CONTROL_HXX

#include <svtools/itemset.hxx>
#include "fuconstr.hxx"

namespace sd {

/*************************************************************************
|*
|* Control zeichnen
|*
\************************************************************************/

class FuConstructUnoControl 
    : public FuConstruct
{
public:
	TYPEINFO();

	static FunctionReference Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq, bool bPermanent );
	virtual void DoExecute( SfxRequest& rReq );

    // Mouse- & Key-Events
	virtual BOOL KeyInput(const KeyEvent& rKEvt);
	virtual BOOL MouseMove(const MouseEvent& rMEvt);
	virtual BOOL MouseButtonUp(const MouseEvent& rMEvt);
	virtual BOOL MouseButtonDown(const MouseEvent& rMEvt);

	virtual void Activate();		   // Function aktivieren
	virtual void Deactivate();		   // Function deaktivieren

	// #97016#
	virtual SdrObject* CreateDefaultObject(const sal_uInt16 nID, const Rectangle& rRectangle);

protected:
	FuConstructUnoControl(
        ViewShell* pViewSh, 
        ::sd::Window* pWin,
        ::sd::View* pView, 
        SdDrawDocument* pDoc,
        SfxRequest& rReq);

	String	aOldLayer;

private:
	UINT32 nInventor;
	UINT16 nIdentifier;
};

} // end of namespace sd

#endif

