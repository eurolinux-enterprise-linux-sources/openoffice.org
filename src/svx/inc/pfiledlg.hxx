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
#ifndef _SVX_PFILEDLG_HXX
#define _SVX_PFILEDLG_HXX

// include ---------------------------------------------------------------

#include <sfx2/filedlghelper.hxx>
#include "svx/svxdllapi.h"

/*************************************************************************
|*
|* Filedialog to insert Plugin-Fileformats
|*
\************************************************************************/

class SVX_DLLPUBLIC SvxPluginFileDlg
{
private:
	sfx2::FileDialogHelper			maFileDlg;

public:
	// with nKind = SID_INSERT_SOUND    or
	//      nKind = SID_INSERT_VIDEO
	SvxPluginFileDlg (Window *pParent, USHORT nKind );
	~SvxPluginFileDlg ();

	ErrCode					 Execute();
	String					 GetPath() const;

	static bool IsAvailable (USHORT nKind);

	// setting HelpId and/or context of FileDialogHelper
	void					SetDialogHelpId( const sal_Int32 nHelpId );
	void					SetContext( sfx2::FileDialogHelper::Context eNewContext );
};

#endif // _SVX_PFILEDLG_HXX


