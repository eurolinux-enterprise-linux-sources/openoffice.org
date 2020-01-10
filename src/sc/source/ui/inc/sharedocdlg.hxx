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

#ifndef SC_SHAREDOCDLG_HXX
#define SC_SHAREDOCDLG_HXX

#include <vcl/button.hxx>
#include <vcl/dialog.hxx>
#include <vcl/fixed.hxx>
#include <svx/simptabl.hxx>

class ScViewData;
class ScDocShell;


//=============================================================================
// class ScShareDocumentDlg
//=============================================================================

class ScShareDocumentDlg : public ModalDialog
{
private:
    CheckBox            maCbShare;
    FixedText           maFtWarning;
    FixedLine           maFlUsers;
    FixedText           maFtUsers;
    SvxSimpleTable      maLbUsers;
    FixedLine           maFlEnd;
    HelpButton          maBtnHelp;
    OKButton            maBtnOK;
    CancelButton        maBtnCancel;

    String              maStrTitleName;
    String              maStrTitleAccessed;
    String              maStrNoUserData;
    String              maStrUnkownUser;
    String              maStrExclusiveAccess;

    ScViewData*         mpViewData;
    ScDocShell*         mpDocShell;

    DECL_LINK( ToggleHandle, void* );

public:
                        ScShareDocumentDlg( Window* pParent, ScViewData* pViewData );
                        ~ScShareDocumentDlg();

    bool                IsShareDocumentChecked() const;
    void                UpdateView();
};

#endif
