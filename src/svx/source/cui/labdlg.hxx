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
#ifndef _SVX_LABDLG_HXX
#define _SVX_LABDLG_HXX

// include ---------------------------------------------------------------


#ifndef _FIELD_HXX //autogen
#include <vcl/field.hxx>
#endif
#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif
#include <svtools/valueset.hxx>
#include <sfx2/tabdlg.hxx>
class SdrView;

// class SvxCaptionTabDialog ---------------------------------------------

class SvxCaptionTabDialog : public SfxTabDialog
{
private:
//	const SfxItemSet&	rOutAttrs;
	const SdrView*		pView;
	USHORT				nAnchorCtrls;

    Link                aValidateLink;
    
    virtual void        PageCreated( USHORT nId, SfxTabPage &rPage );

public:

			SvxCaptionTabDialog(Window* pParent, const SdrView* pView,
									USHORT nAnchorTypes = 0 );

			~SvxCaptionTabDialog();
            
            //link for the Writer to validate positions 
            void SetValidateFramePosLink( const Link& rLink );
};


#endif //_SVX_LABDLG_HXX

