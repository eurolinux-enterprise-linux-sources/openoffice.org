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

#ifndef SC_PVFUNDLG_HXX
#define SC_PVFUNDLG_HXX

#include <com/sun/star/sheet/DataPilotFieldReference.hpp>
#include <com/sun/star/sheet/DataPilotFieldSortInfo.hpp>

#ifndef _FIXED_HXX
#include <vcl/fixed.hxx>
#endif
#ifndef _LSTBOX_HXX
#include <vcl/lstbox.hxx>
#endif
#ifndef _DIALOG_HXX
#include <vcl/dialog.hxx>
#endif
#ifndef _BUTTON_HXX
#include <vcl/button.hxx>
#endif
#ifndef _MOREBTN_HXX
#include <vcl/morebtn.hxx>
#endif
#include <vcl/field.hxx>
#include <svtools/stdctrl.hxx>
#include <svx/checklbx.hxx>
#include <sfx2/itemconnect.hxx>
#include "pivot.hxx"

// ============================================================================

typedef sfx::ListBoxWrapper< sal_Int32 > ScDPListBoxWrapper;

class ScDPObject;

// ============================================================================

class ScDPFunctionListBox : public MultiListBox
{
public:
    explicit            ScDPFunctionListBox( Window* pParent, const ResId& rResId );

    void                SetSelection( USHORT nFuncMask );
    USHORT              GetSelection() const;

private:
    void                FillFunctionNames();
};

// ============================================================================

class ScDPFunctionDlg : public ModalDialog
{
public:
    explicit            ScDPFunctionDlg( Window* pParent, const ScDPLabelDataVec& rLabelVec,
                            const ScDPLabelData& rLabelData, const ScDPFuncData& rFuncData );

    USHORT              GetFuncMask() const;
    ::com::sun::star::sheet::DataPilotFieldReference GetFieldRef() const;

private:
    void                Init( const ScDPLabelData& rLabelData, const ScDPFuncData& rFuncData );

    DECL_LINK( SelectHdl, ListBox* );
    DECL_LINK( DblClickHdl, MultiListBox* );

private:
    FixedLine           maFlFunc;
    ScDPFunctionListBox maLbFunc;
    FixedText           maFtNameLabel;
    FixedInfo           maFtName;
    FixedLine           maFlDisplay;
    FixedText           maFtType;
    ListBox             maLbType;
    FixedText           maFtBaseField;
    ListBox             maLbBaseField;
    FixedText           maFtBaseItem;
    ListBox             maLbBaseItem;
    OKButton            maBtnOk;
    CancelButton        maBtnCancel;
    HelpButton          maBtnHelp;
    MoreButton          maBtnMore;

    ScDPListBoxWrapper  maLbTypeWrp;        /// Wrapper for direct usage of API constants.

    const ScDPLabelDataVec& mrLabelVec;     /// Data of all labels.
    bool                mbEmptyItem;        /// true = Empty base item in listbox.
};

// ============================================================================

class ScDPSubtotalDlg : public ModalDialog
{
public:
    explicit            ScDPSubtotalDlg( Window* pParent, ScDPObject& rDPObj,
                            const ScDPLabelData& rLabelData, const ScDPFuncData& rFuncData,
                            const ScDPNameVec& rDataFields, bool bEnableLayout );

    USHORT              GetFuncMask() const;

    void                FillLabelData( ScDPLabelData& rLabelData ) const;

private:
    void                Init( const ScDPLabelData& rLabelData, const ScDPFuncData& rFuncData );

    DECL_LINK( DblClickHdl, MultiListBox* );
    DECL_LINK( RadioClickHdl, RadioButton* );
    DECL_LINK( ClickHdl, PushButton* );

private:
    FixedLine           maFlSubt;
    RadioButton         maRbNone;
    RadioButton         maRbAuto;
    RadioButton         maRbUser;
    ScDPFunctionListBox maLbFunc;
    FixedText           maFtNameLabel;
    FixedInfo           maFtName;
    CheckBox            maCbShowAll;
    OKButton            maBtnOk;
    CancelButton        maBtnCancel;
    HelpButton          maBtnHelp;
    PushButton          maBtnOptions;

    ScDPObject&         mrDPObj;            /// The DataPilot object (for member names).
    const ScDPNameVec&  mrDataFields;       /// The list of all data field names.

    ScDPLabelData       maLabelData;        /// Cache for sub dialog.
    bool                mbEnableLayout;     /// true = Enable Layout mode controls.
};

// ============================================================================

class ScDPSubtotalOptDlg : public ModalDialog
{
public:
    explicit            ScDPSubtotalOptDlg( Window* pParent, ScDPObject& rDPObj,
                            const ScDPLabelData& rLabelData, const ScDPNameVec& rDataFields,
                            bool bEnableLayout );

    void                FillLabelData( ScDPLabelData& rLabelData ) const;

private:
    void                Init( const ScDPNameVec& rDataFields, bool bEnableLayout );
    void                InitHideListBox();

    DECL_LINK( RadioClickHdl, RadioButton* );
    DECL_LINK( CheckHdl, CheckBox* );
    DECL_LINK( SelectHdl, ListBox* );

private:
    FixedLine           maFlSortBy;
    ListBox             maLbSortBy;
    RadioButton         maRbSortAsc;
    RadioButton         maRbSortDesc;
    RadioButton         maRbSortMan;
    FixedLine           maFlLayout;
    FixedText           maFtLayout;
    ListBox             maLbLayout;
    CheckBox            maCbLayoutEmpty;
    FixedLine           maFlAutoShow;
    CheckBox            maCbShow;
    NumericField        maNfShow;
    FixedText           maFtShow;
    FixedText           maFtShowFrom;
    ListBox             maLbShowFrom;
    FixedText           maFtShowUsing;
    ListBox             maLbShowUsing;
    FixedLine           maFlHide;
    SvxCheckListBox     maLbHide;
    FixedText           maFtHierarchy;
    ListBox             maLbHierarchy;
    OKButton            maBtnOk;
    CancelButton        maBtnCancel;
    HelpButton          maBtnHelp;

    ScDPListBoxWrapper  maLbLayoutWrp;      /// Wrapper for direct usage of API constants.
    ScDPListBoxWrapper  maLbShowFromWrp;    /// Wrapper for direct usage of API constants.

    ScDPObject&         mrDPObj;            /// The DataPilot object (for member names).
    ScDPLabelData       maLabelData;        /// Cache for members data.
};

// ============================================================================

class ScDPShowDetailDlg : public ModalDialog
{
public:
    explicit            ScDPShowDetailDlg( Window* pParent, ScDPObject& rDPObj, USHORT nOrient );

    virtual short       Execute();

    String              GetDimensionName() const;

private:
    DECL_LINK( DblClickHdl, ListBox* );

private:
    FixedText           maFtDims;
    ListBox             maLbDims;
    OKButton            maBtnOk;
    CancelButton        maBtnCancel;
    HelpButton          maBtnHelp;
};

// ============================================================================

#endif

