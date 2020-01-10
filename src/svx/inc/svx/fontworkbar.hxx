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

#ifndef _SVX_FONTWORK_BAR_HXX
#define _SVX_FONTWORK_BAR_HXX

#include <sfx2/shell.hxx>
#include <sfx2/module.hxx>
#include <svx/ifaceids.hxx>
#include "svx/svxdllapi.h"

class SfxViewShell;
class SdrView;

/************************************************************************/

namespace svx
{

bool SVX_DLLPUBLIC checkForSelectedFontWork( SdrView* pSdrView, sal_uInt32& nCheckStatus );

class SVX_DLLPUBLIC FontworkBar : public SfxShell
{
public:
	TYPEINFO();
	SFX_DECL_INTERFACE(SVX_INTERFACE_FONTWORK_BAR)

	 FontworkBar(SfxViewShell* pViewShell );
	~FontworkBar();

	static void execute( SdrView* pSdrView, SfxRequest& rReq, SfxBindings& rBindings );
	static void getState( SdrView* pSdrView, SfxItemSet& rSet );
};

}

#endif			// _SVX_FONTWORK_BAR_HXX
