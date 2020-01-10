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

#ifndef SD_FU_BULLET_HXX
#define SD_FU_BULLET_HXX

#include "fupoor.hxx"

class SfxItemSet;
class SfxViewFrame;

namespace sd {

class ViewShell;

class FuBullet : public FuPoor
{
public:
	TYPEINFO();

	static FunctionReference Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq );
	virtual void DoExecute( SfxRequest& rReq );

	static void GetSlotState( SfxItemSet& rSet, ViewShell* pViewShell, SfxViewFrame* pViewFrame );

private:
	FuBullet (
        ViewShell* pViewSh, 
        ::sd::Window* pWin, 
        ::sd::View* pView,
        SdDrawDocument* pDoc,
        SfxRequest& rReq);

	void InsertSpecialCharacter( SfxRequest& rReq );
	void InsertFormattingMark( sal_Unicode cMark );
};

} // end of namespace sd

#endif

