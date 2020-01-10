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

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"

#include "xipivot.hxx"

#include <com/sun/star/sheet/DataPilotFieldSortInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldAutoShowInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldLayoutInfo.hpp>
#include <com/sun/star/sheet/DataPilotFieldReference.hpp>

#include <tools/datetime.hxx>
#include <svtools/zformat.hxx>
#include <svtools/intitem.hxx>

#include "document.hxx"
#include "cell.hxx"
#include "dpsave.hxx"
#include "dpdimsave.hxx"
#include "dpobject.hxx"
#include "dpshttab.hxx"
#include "scitems.hxx"

#include "xltracer.hxx"
#include "xistream.hxx"
#include "xihelper.hxx"
#include "xilink.hxx"
#include "xiescher.hxx"

//! TODO ExcelToSc usage
#include "excform.hxx"
#include "xltable.hxx"

using ::rtl::OUString;
using ::com::sun::star::sheet::DataPilotFieldOrientation;
using ::com::sun::star::sheet::DataPilotFieldOrientation_DATA;
using ::com::sun::star::sheet::DataPilotFieldSortInfo;
using ::com::sun::star::sheet::DataPilotFieldAutoShowInfo;
using ::com::sun::star::sheet::DataPilotFieldLayoutInfo;
using ::com::sun::star::sheet::DataPilotFieldReference;

// ============================================================================
// Pivot cache
// ============================================================================

XclImpPCItem::XclImpPCItem( XclImpStream& rStrm )
{
    switch( rStrm.GetRecId() )
    {
        case EXC_ID_SXDOUBLE:   ReadSxdouble( rStrm );      break;
        case EXC_ID_SXBOOLEAN:  ReadSxboolean( rStrm );     break;
        case EXC_ID_SXERROR:    ReadSxerror( rStrm );       break;
        case EXC_ID_SXINTEGER:  ReadSxinteger( rStrm );     break;
        case EXC_ID_SXSTRING:   ReadSxstring( rStrm );      break;
        case EXC_ID_SXDATETIME: ReadSxdatetime( rStrm );    break;
        case EXC_ID_SXEMPTY:    ReadSxempty( rStrm );       break;
        default:    DBG_ERRORFILE( "XclImpPCItem::XclImpPCItem - unknown record id" );
    }
}

namespace {

void lclSetValue( const XclImpRoot& rRoot, const ScAddress& rScPos, double fValue, short nFormatType )
{
    ScDocument& rDoc = rRoot.GetDoc();
    rDoc.SetValue( rScPos.Col(), rScPos.Row(), rScPos.Tab(), fValue );
    sal_uInt32 nScNumFmt = rRoot.GetFormatter().GetStandardFormat( nFormatType, rRoot.GetDocLanguage() );
    rDoc.ApplyAttr( rScPos.Col(), rScPos.Row(), rScPos.Tab(), SfxUInt32Item( ATTR_VALUE_FORMAT, nScNumFmt ) );
}

} // namespace

void XclImpPCItem::WriteToSource( const XclImpRoot& rRoot, const ScAddress& rScPos ) const
{
    ScDocument& rDoc = rRoot.GetDoc();
    if( const String* pText = GetText() )
        rDoc.SetString( rScPos.Col(), rScPos.Row(), rScPos.Tab(), *pText );
    else if( const double* pfValue = GetDouble() )
        rDoc.SetValue( rScPos.Col(), rScPos.Row(), rScPos.Tab(), *pfValue );
    else if( const sal_Int16* pnValue = GetInteger() )
        rDoc.SetValue( rScPos.Col(), rScPos.Row(), rScPos.Tab(), *pnValue );
    else if( const bool* pbValue = GetBool() )
        lclSetValue( rRoot, rScPos, *pbValue ? 1.0 : 0.0, NUMBERFORMAT_LOGICAL );
    else if( const DateTime* pDateTime = GetDateTime() )
    {
        // set number format date, time, or date/time, depending on the value
        double fValue = rRoot.GetDoubleFromDateTime( *pDateTime );
        double fInt = 0.0;
        double fFrac = modf( fValue, &fInt );
        short nFormatType = ((fFrac == 0.0) && (fInt != 0.0)) ? NUMBERFORMAT_DATE :
            ((fInt == 0.0) ? NUMBERFORMAT_TIME : NUMBERFORMAT_DATETIME);
        lclSetValue( rRoot, rScPos, fValue, nFormatType );
    }
    else if( const sal_uInt16* pnError = GetError() )
    {
        double fValue;
        sal_uInt8 nErrCode = static_cast< sal_uInt8 >( *pnError );
        const ScTokenArray* pScTokArr = rRoot.GetOldFmlaConverter().GetBoolErr(
            XclTools::ErrorToEnum( fValue, EXC_BOOLERR_ERROR, nErrCode ) );
        ScFormulaCell* pCell = new ScFormulaCell( &rDoc, rScPos, pScTokArr );
        pCell->SetHybridDouble( fValue );
        rDoc.PutCell( rScPos, pCell );
    }
}

void XclImpPCItem::ReadSxdouble( XclImpStream& rStrm )
{
    DBG_ASSERT( rStrm.GetRecSize() == 8, "XclImpPCItem::ReadSxdouble - wrong record size" );
    SetDouble( rStrm.ReadDouble() );
}

void XclImpPCItem::ReadSxboolean( XclImpStream& rStrm )
{
    DBG_ASSERT( rStrm.GetRecSize() == 2, "XclImpPCItem::ReadSxboolean - wrong record size" );
    SetBool( rStrm.ReaduInt16() != 0 );
}

void XclImpPCItem::ReadSxerror( XclImpStream& rStrm )
{
    DBG_ASSERT( rStrm.GetRecSize() == 2, "XclImpPCItem::ReadSxerror - wrong record size" );
    SetError( rStrm.ReaduInt16() );
}

void XclImpPCItem::ReadSxinteger( XclImpStream& rStrm )
{
    DBG_ASSERT( rStrm.GetRecSize() == 2, "XclImpPCItem::ReadSxinteger - wrong record size" );
    SetInteger( rStrm.ReadInt16() );
}

void XclImpPCItem::ReadSxstring( XclImpStream& rStrm )
{
    DBG_ASSERT( rStrm.GetRecSize() >= 3, "XclImpPCItem::ReadSxstring - wrong record size" );
    SetText( rStrm.ReadUniString() );
}

void XclImpPCItem::ReadSxdatetime( XclImpStream& rStrm )
{
    DBG_ASSERT( rStrm.GetRecSize() == 8, "XclImpPCItem::ReadSxdatetime - wrong record size" );
    sal_uInt16 nYear, nMonth;
    sal_uInt8 nDay, nHour, nMin, nSec;
    rStrm >> nYear >> nMonth >> nDay >> nHour >> nMin >> nSec;
    SetDateTime( DateTime( Date( nDay, nMonth, nYear ), Time( nHour, nMin, nSec ) ) );
}

void XclImpPCItem::ReadSxempty( XclImpStream& rStrm )
{
    (void)rStrm;    // avoid compiler warning
    DBG_ASSERT( rStrm.GetRecSize() == 0, "XclImpPCItem::ReadSxempty - wrong record size" );
    SetEmpty();
}

// ============================================================================

XclImpPCField::XclImpPCField( const XclImpRoot& rRoot, XclImpPivotCache& rPCache, sal_uInt16 nFieldIdx ) :
    XclPCField( EXC_PCFIELD_UNKNOWN, nFieldIdx ),
    XclImpRoot( rRoot ),
    mrPCache( rPCache ),
    mnSourceScCol( -1 ),
    mbNumGroupInfoRead( false )
{
}

XclImpPCField::~XclImpPCField()
{
}

// general field/item access --------------------------------------------------

const String& XclImpPCField::GetFieldName( const ScfStringVec& rVisNames ) const
{
    if( IsGroupChildField() && (mnFieldIdx < rVisNames.size()) )
    {
        const String& rVisName = rVisNames[ mnFieldIdx ];
        if( rVisName.Len() > 0 )
            return rVisName;
    }
    return maFieldInfo.maName;
}

const XclImpPCField* XclImpPCField::GetGroupBaseField() const
{
    DBG_ASSERT( IsGroupChildField(), "XclImpPCField::GetGroupBaseField - this field type does not have a base field" );
    return IsGroupChildField() ? mrPCache.GetField( maFieldInfo.mnGroupBase ) : 0;
}

sal_uInt16 XclImpPCField::GetItemCount() const
{
    return static_cast< sal_uInt16 >( maItems.size() );
}

const XclImpPCItem* XclImpPCField::GetItem( sal_uInt16 nItemIdx ) const
{
    return (nItemIdx < maItems.size()) ? maItems[ nItemIdx ].get() : 0;
}

const XclImpPCItem* XclImpPCField::GetLimitItem( sal_uInt16 nItemIdx ) const
{
    DBG_ASSERT( nItemIdx < 3, "XclImpPCField::GetLimitItem - invalid item index" );
    DBG_ASSERT( nItemIdx < maNumGroupItems.size(), "XclImpPCField::GetLimitItem - no item found" );
    return (nItemIdx < maNumGroupItems.size()) ? maNumGroupItems[ nItemIdx ].get() : 0;
}

void XclImpPCField::WriteFieldNameToSource( SCCOL nScCol, SCTAB nScTab ) const
{
    DBG_ASSERT( HasOrigItems(), "XclImpPCField::WriteFieldNameToSource - only for standard fields" );
    GetDoc().SetString( nScCol, 0, nScTab, maFieldInfo.maName );
    mnSourceScCol = nScCol;
}

void XclImpPCField::WriteOrigItemToSource( SCROW nScRow, SCTAB nScTab, sal_uInt16 nItemIdx ) const
{
    if( nItemIdx < maOrigItems.size() )
        maOrigItems[ nItemIdx ]->WriteToSource( GetRoot(), ScAddress( mnSourceScCol, nScRow, nScTab ) );
}

void XclImpPCField::WriteLastOrigItemToSource( SCROW nScRow, SCTAB nScTab ) const
{
    if( !maOrigItems.empty() )
        maOrigItems.back()->WriteToSource( GetRoot(), ScAddress( mnSourceScCol, nScRow, nScTab ) );
}

// records --------------------------------------------------------------------

void XclImpPCField::ReadSxfield( XclImpStream& rStrm )
{
    rStrm >> maFieldInfo;

    /*  Detect the type of this field. This is done very restrictive to detect
        any unexpected state. */
    meFieldType = EXC_PCFIELD_UNKNOWN;

    bool bItems  = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_HASITEMS );
    bool bPostp  = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_POSTPONE );
    bool bCalced = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_CALCED );
    bool bChild  = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_HASCHILD );
    bool bNum    = ::get_flag( maFieldInfo.mnFlags, EXC_SXFIELD_NUMGROUP );

    sal_uInt16 nVisC   = maFieldInfo.mnVisItems;
    sal_uInt16 nGroupC = maFieldInfo.mnGroupItems;
    sal_uInt16 nBaseC  = maFieldInfo.mnBaseItems;
    sal_uInt16 nOrigC  = maFieldInfo.mnOrigItems;
    DBG_ASSERT( nVisC > 0, "XclImpPCField::ReadSxfield - field without visible items" );

    sal_uInt16 nType = maFieldInfo.mnFlags & EXC_SXFIELD_DATA_MASK;
    bool bType =
        (nType == EXC_SXFIELD_DATA_STR) ||
        (nType == EXC_SXFIELD_DATA_INT) ||
        (nType == EXC_SXFIELD_DATA_DBL) ||
        (nType == EXC_SXFIELD_DATA_STR_INT) ||
        (nType == EXC_SXFIELD_DATA_STR_DBL) ||
        (nType == EXC_SXFIELD_DATA_DATE) ||
        (nType == EXC_SXFIELD_DATA_DATE_EMP) ||
        (nType == EXC_SXFIELD_DATA_DATE_NUM) ||
        (nType == EXC_SXFIELD_DATA_DATE_STR);
    bool bTypeNone =
        (nType == EXC_SXFIELD_DATA_NONE);
    // for now, ignore data type of calculated fields
    DBG_ASSERT( bCalced || bType || bTypeNone, "XclImpPCField::ReadSxfield - unknown item data type" );

    if( nVisC > 0 || bPostp )
    {
        if( bItems && !bPostp )
        {
            if( !bCalced )
            {
                // 1) standard fields and standard grouping fields
                if( !bNum )
                {
                    // 1a) standard field without grouping
                    if( bType && (nGroupC == 0) && (nBaseC == 0) && (nOrigC == nVisC) )
                        meFieldType = EXC_PCFIELD_STANDARD;

                    // 1b) standard grouping field
                    else if( bTypeNone && (nGroupC == nVisC) && (nBaseC > 0) && (nOrigC == 0) )
                        meFieldType = EXC_PCFIELD_STDGROUP;
                }
                // 2) numerical grouping fields
                else if( (nGroupC == nVisC) && (nBaseC == 0) )
                {
                    // 2a) single num/date grouping field without child grouping field
                    if( !bChild && bType && (nOrigC > 0) )
                    {
                        switch( nType )
                        {
                            case EXC_SXFIELD_DATA_INT:
                            case EXC_SXFIELD_DATA_DBL:  meFieldType = EXC_PCFIELD_NUMGROUP;     break;
                            case EXC_SXFIELD_DATA_DATE: meFieldType = EXC_PCFIELD_DATEGROUP;    break;
                            default:    DBG_ERRORFILE( "XclImpPCField::ReadSxfield - numeric group with wrong data type" );
                        }
                    }

                    // 2b) first date grouping field with child grouping field
                    else if( bChild && (nType == EXC_SXFIELD_DATA_DATE) && (nOrigC > 0) )
                        meFieldType = EXC_PCFIELD_DATEGROUP;

                    // 2c) additional date grouping field
                    else if( bTypeNone && (nOrigC == 0) )
                        meFieldType = EXC_PCFIELD_DATECHILD;
                }
                DBG_ASSERT( meFieldType != EXC_PCFIELD_UNKNOWN, "XclImpPCField::ReadSxfield - invalid standard or grouped field" );
            }

            // 3) calculated field
            else
            {
                if( !bChild && !bNum && (nGroupC == 0) && (nBaseC == 0) && (nOrigC == 0) )
                    meFieldType = EXC_PCFIELD_CALCED;
                DBG_ASSERT( meFieldType == EXC_PCFIELD_CALCED, "XclImpPCField::ReadSxfield - invalid calculated field" );
            }
        }

        else if( !bItems && bPostp )
        {
            // 4) standard field with postponed items
            if( !bCalced && !bChild && !bNum && bType && (nGroupC == 0) && (nBaseC == 0) && (nOrigC == 0) )
                meFieldType = EXC_PCFIELD_STANDARD;
            DBG_ASSERT( meFieldType == EXC_PCFIELD_STANDARD, "XclImpPCField::ReadSxfield - invalid postponed field" );
        }
    }
}

void XclImpPCField::ReadItem( XclImpStream& rStrm )
{
    DBG_ASSERT( HasInlineItems() || HasPostponedItems(), "XclImpPCField::ReadItem - field does not expect items" );

    // read the item
    XclImpPCItemRef xItem( new XclImpPCItem( rStrm ) );

    // try to insert into an item list
    if( mbNumGroupInfoRead )
    {
        // there are 3 items after SXNUMGROUP that contain grouping limits and step count
        if( maNumGroupItems.size() < 3 )
            maNumGroupItems.push_back( xItem );
        else
            maOrigItems.push_back( xItem );
    }
    else if( HasInlineItems() || HasPostponedItems() )
    {
        maItems.push_back( xItem );
        // visible item is original item in standard fields
        if( IsStandardField() )
            maOrigItems.push_back( xItem );
    }
}

void XclImpPCField::ReadSxnumgroup( XclImpStream& rStrm )
{
    DBG_ASSERT( IsNumGroupField() || IsDateGroupField(), "XclImpPCField::ReadSxnumgroup - SXNUMGROUP outside numeric grouping field" );
    DBG_ASSERT( !mbNumGroupInfoRead, "XclImpPCField::ReadSxnumgroup - multiple SXNUMGROUP records" );
    DBG_ASSERT( maItems.size() == maFieldInfo.mnGroupItems, "XclImpPCField::ReadSxnumgroup - SXNUMGROUP out of record order" );
    rStrm >> maNumGroupInfo;
    mbNumGroupInfoRead = IsNumGroupField() || IsDateGroupField();
}

void XclImpPCField::ReadSxgroupinfo( XclImpStream& rStrm )
{
    DBG_ASSERT( IsStdGroupField(), "XclImpPCField::ReadSxgroupinfo - SXGROUPINFO outside grouping field" );
    DBG_ASSERT( maGroupOrder.empty(), "XclImpPCField::ReadSxgroupinfo - multiple SXGROUPINFO records" );
    DBG_ASSERT( maItems.size() == maFieldInfo.mnGroupItems, "XclImpPCField::ReadSxgroupinfo - SXGROUPINFO out of record order" );
    DBG_ASSERT( (rStrm.GetRecLeft() / 2) == maFieldInfo.mnBaseItems, "XclImpPCField::ReadSxgroupinfo - wrong SXGROUPINFO size" );
    maGroupOrder.clear();
    size_t nSize = rStrm.GetRecLeft() / 2;
    maGroupOrder.resize( nSize, 0 );
    for( size_t nIdx = 0; nIdx < nSize; ++nIdx )
        rStrm >> maGroupOrder[ nIdx ];
}

// grouping -------------------------------------------------------------------

void XclImpPCField::ConvertGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    if( GetFieldName( rVisNames ).Len() > 0 )
    {
        if( IsStdGroupField() )
            ConvertStdGroupField( rSaveData, rVisNames );
        else if( IsNumGroupField() )
            ConvertNumGroupField( rSaveData, rVisNames );
        else if( IsDateGroupField() )
            ConvertDateGroupField( rSaveData, rVisNames );
    }
}

// private --------------------------------------------------------------------

void XclImpPCField::ConvertStdGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    if( const XclImpPCField* pBaseField = GetGroupBaseField() )
    {
        const String& rBaseFieldName = pBaseField->GetFieldName( rVisNames );
        if( rBaseFieldName.Len() > 0 )
        {
            // *** create a ScDPSaveGroupItem for each own item, they collect base item names ***
            typedef ::std::vector< ScDPSaveGroupItem > ScDPSaveGroupItemVec;
            ScDPSaveGroupItemVec aGroupItems;
            aGroupItems.reserve( maItems.size() );
            // initialize with own item names
            for( XclImpPCItemVec::const_iterator aIt = maItems.begin(), aEnd = maItems.end(); aIt != aEnd; ++aIt )
                aGroupItems.push_back( ScDPSaveGroupItem( (*aIt)->ConvertToText() ) );

            // *** iterate over all base items, set their names at corresponding own items ***
            for( sal_uInt16 nItemIdx = 0, nItemCount = static_cast< sal_uInt16 >( maGroupOrder.size() ); nItemIdx < nItemCount; ++nItemIdx )
                if( maGroupOrder[ nItemIdx ] < aGroupItems.size() )
                    if( const XclImpPCItem* pBaseItem = pBaseField->GetItem( nItemIdx ) )
                        if( const XclImpPCItem* pGroupItem = GetItem( maGroupOrder[ nItemIdx ] ) )
                            if( *pBaseItem != *pGroupItem )
                                aGroupItems[ maGroupOrder[ nItemIdx ] ].AddElement( pBaseItem->ConvertToText() );

            // *** create the ScDPSaveGroupDimension object, fill with grouping info ***
            ScDPSaveGroupDimension aGroupDim( rBaseFieldName, GetFieldName( rVisNames ) );
            for( ScDPSaveGroupItemVec::const_iterator aIt = aGroupItems.begin(), aEnd = aGroupItems.end(); aIt != aEnd; ++aIt )
                if( !aIt->IsEmpty() )
                    aGroupDim.AddGroupItem( *aIt );
            rSaveData.GetDimensionData()->AddGroupDimension( aGroupDim );
        }
    }
}

void XclImpPCField::ConvertNumGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    ScDPNumGroupInfo aNumInfo( GetScNumGroupInfo() );
    ScDPSaveNumGroupDimension aNumGroupDim( GetFieldName( rVisNames ), aNumInfo );
    rSaveData.GetDimensionData()->AddNumGroupDimension( aNumGroupDim );
}

void XclImpPCField::ConvertDateGroupField( ScDPSaveData& rSaveData, const ScfStringVec& rVisNames ) const
{
    ScDPNumGroupInfo aDateInfo( GetScDateGroupInfo() );
    sal_Int32 nScDateType = maNumGroupInfo.GetScDateType();

    switch( meFieldType )
    {
        case EXC_PCFIELD_DATEGROUP:
        {
            if( aDateInfo.DateValues )
            {
                // special case for days only with step value - create numeric grouping
                ScDPSaveNumGroupDimension aNumGroupDim( GetFieldName( rVisNames ), aDateInfo );
                rSaveData.GetDimensionData()->AddNumGroupDimension( aNumGroupDim );
            }
            else
            {
                ScDPSaveNumGroupDimension aNumGroupDim( GetFieldName( rVisNames ), ScDPNumGroupInfo() );
                aNumGroupDim.SetDateInfo( aDateInfo, nScDateType );
                rSaveData.GetDimensionData()->AddNumGroupDimension( aNumGroupDim );
            }
        }
        break;

        case EXC_PCFIELD_DATECHILD:
        {
            if( const XclImpPCField* pBaseField = GetGroupBaseField() )
            {
                const String& rBaseFieldName = pBaseField->GetFieldName( rVisNames );
                if( rBaseFieldName.Len() > 0 )
                {
                    ScDPSaveGroupDimension aGroupDim( rBaseFieldName, GetFieldName( rVisNames ) );
                    aGroupDim.SetDateInfo( aDateInfo, nScDateType );
                    rSaveData.GetDimensionData()->AddGroupDimension( aGroupDim );
                }
            }
        }
        break;

        default:
            DBG_ERRORFILE( "XclImpPCField::ConvertDateGroupField - unknown date field type" );
    }
}

ScDPNumGroupInfo XclImpPCField::GetScNumGroupInfo() const
{
    ScDPNumGroupInfo aNumInfo;
    aNumInfo.Enable = sal_True;
    aNumInfo.DateValues = sal_False;
    aNumInfo.AutoStart = sal_True;
    aNumInfo.AutoEnd = sal_True;

    if( const double* pfMinValue = GetNumGroupLimit( EXC_SXFIELD_INDEX_MIN ) )
    {
        aNumInfo.Start = *pfMinValue;
        aNumInfo.AutoStart = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMIN );
    }
    if( const double* pfMaxValue = GetNumGroupLimit( EXC_SXFIELD_INDEX_MAX ) )
    {
        aNumInfo.End = *pfMaxValue;
        aNumInfo.AutoEnd = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMAX );
    }
    if( const double* pfStepValue = GetNumGroupLimit( EXC_SXFIELD_INDEX_STEP ) )
        aNumInfo.Step = *pfStepValue;

    return aNumInfo;
}

ScDPNumGroupInfo XclImpPCField::GetScDateGroupInfo() const
{
    ScDPNumGroupInfo aDateInfo;
    aDateInfo.Enable = sal_True;
    aDateInfo.DateValues = sal_False;
    aDateInfo.AutoStart = sal_True;
    aDateInfo.AutoEnd = sal_True;

    if( const DateTime* pMinDate = GetDateGroupLimit( EXC_SXFIELD_INDEX_MIN ) )
    {
        aDateInfo.Start = GetDoubleFromDateTime( *pMinDate );
        aDateInfo.AutoStart = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMIN );
    }
    if( const DateTime* pMaxDate = GetDateGroupLimit( EXC_SXFIELD_INDEX_MAX ) )
    {
        aDateInfo.End = GetDoubleFromDateTime( *pMaxDate );
        aDateInfo.AutoEnd = ::get_flag( maNumGroupInfo.mnFlags, EXC_SXNUMGROUP_AUTOMAX );
    }
    // GetDateGroupStep() returns a value for date type "day" in single date groups only
    if( const sal_Int16* pnStepValue = GetDateGroupStep() )
    {
        aDateInfo.Step = *pnStepValue;
        aDateInfo.DateValues = sal_True;
    }

    return aDateInfo;
}

const double* XclImpPCField::GetNumGroupLimit( sal_uInt16 nLimitIdx ) const
{
    DBG_ASSERT( IsNumGroupField(), "XclImpPCField::GetNumGroupLimit - only for numeric grouping fields" );
    if( const XclImpPCItem* pItem = GetLimitItem( nLimitIdx ) )
    {
        DBG_ASSERT( pItem->GetDouble(), "XclImpPCField::GetNumGroupLimit - SXDOUBLE item expected" );
        return pItem->GetDouble();
    }
    return 0;
}

const DateTime* XclImpPCField::GetDateGroupLimit( sal_uInt16 nLimitIdx ) const
{
    DBG_ASSERT( IsDateGroupField(), "XclImpPCField::GetDateGroupLimit - only for date grouping fields" );
    if( const XclImpPCItem* pItem = GetLimitItem( nLimitIdx ) )
    {
        DBG_ASSERT( pItem->GetDateTime(), "XclImpPCField::GetDateGroupLimit - SXDATETIME item expected" );
        return pItem->GetDateTime();
    }
    return 0;
}

const sal_Int16* XclImpPCField::GetDateGroupStep() const
{
    // only for single date grouping fields, not for grouping chains
    if( !IsGroupBaseField() && !IsGroupChildField() )
    {
        // only days may have a step value, return 0 for all other date types
        if( maNumGroupInfo.GetXclDataType() == EXC_SXNUMGROUP_TYPE_DAY )
        {
            if( const XclImpPCItem* pItem = GetLimitItem( EXC_SXFIELD_INDEX_STEP ) )
            {
                DBG_ASSERT( pItem->GetInteger(), "XclImpPCField::GetDateGroupStep - SXINTEGER item expected" );
                if( const sal_Int16* pnStep = pItem->GetInteger() )
                {
                    DBG_ASSERT( *pnStep > 0, "XclImpPCField::GetDateGroupStep - invalid step count" );
                    // return nothing for step count 1 - this is also a standard date group in Excel
                    return (*pnStep > 1) ? pnStep : 0;
                }
            }
        }
    }
    return 0;
}

// ============================================================================

XclImpPivotCache::XclImpPivotCache( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot ),
    maSrcRange( ScAddress::INITIALIZE_INVALID ),
    mnStrmId( 0 ),
    mnSrcType( EXC_SXVS_UNKNOWN ),
    mbSelfRef( false )
{
}

XclImpPivotCache::~XclImpPivotCache()
{
}

// data access ----------------------------------------------------------------

sal_uInt16 XclImpPivotCache::GetFieldCount() const
{
    return static_cast< sal_uInt16 >( maFields.size() );
}

const XclImpPCField* XclImpPivotCache::GetField( sal_uInt16 nFieldIdx ) const
{
    return (nFieldIdx < maFields.size()) ? maFields[ nFieldIdx ].get() : 0;
}

// records --------------------------------------------------------------------

void XclImpPivotCache::ReadSxidstm( XclImpStream& rStrm )
{
    rStrm >> mnStrmId;
}

void XclImpPivotCache::ReadSxvs( XclImpStream& rStrm )
{
    rStrm >> mnSrcType;
    GetTracer().TracePivotDataSource( mnSrcType != EXC_SXVS_SHEET );
}

void XclImpPivotCache::ReadDconref( XclImpStream& rStrm )
{
    /*  Read DCONREF only once (by checking maTabName), there may be other
        DCONREF records in another context. Read reference only if a leading
        SXVS record is present (by checking mnSrcType). */
    if( (maTabName.Len() > 0) || (mnSrcType != EXC_SXVS_SHEET) )
        return;

    XclRange aXclRange( ScAddress::UNINITIALIZED );
    aXclRange.Read( rStrm, false );
    String aEncUrl = rStrm.ReadUniString();

    XclImpUrlHelper::DecodeUrl( maUrl, maTabName, mbSelfRef, GetRoot(), aEncUrl );

    /*  Do not convert maTabName to Calc sheet name -> original name is used to
        find the sheet in the document. Sheet index of source range will be
        found later in XclImpPivotCache::ReadPivotCacheStream(), because sheet
        may not exist yet. */
    if( mbSelfRef )
        GetAddressConverter().ConvertRange( maSrcRange, aXclRange, 0, 0, true );
}

void XclImpPivotCache::ReadPivotCacheStream( XclImpStream& rStrm )
{
    if( (mnSrcType != EXC_SXVS_SHEET) && (mnSrcType != EXC_SXVS_EXTERN) )
        return;

    ScDocument& rDoc = GetDoc();
    SCCOL nFieldScCol = 0;              // column index of source data for next field
    SCROW nItemScRow = 0;               // row index of source data for current items
    SCTAB nScTab = 0;                   // sheet index of source data
    bool bGenerateSource = false;       // true = write source data from cache to dummy table

    if( mbSelfRef )
    {
        // try to find internal sheet containing the source data
        nScTab = GetTabInfo().GetScTabFromXclName( maTabName );
        if( rDoc.HasTable( nScTab ) )
        {
            // set sheet index to source range
            maSrcRange.aStart.SetTab( nScTab );
            maSrcRange.aEnd.SetTab( nScTab );
        }
        else
        {
            // create dummy sheet for deleted internal sheet
            bGenerateSource = true;
        }
    }
    else
    {
        // create dummy sheet for external sheet
        bGenerateSource = true;
    }

    // create dummy sheet for source data from external or deleted sheet
    if( bGenerateSource )
    {
        if( rDoc.GetTableCount() >= MAXTABCOUNT )
            // cannot create more sheets -> exit
            return;

        nScTab = rDoc.GetTableCount();
        rDoc.MakeTable( nScTab );
        String aDummyName = CREATE_STRING( "DPCache" );
        if( maTabName.Len() > 0 )
            aDummyName.Append( '_' ).Append( maTabName );
        rDoc.CreateValidTabName( aDummyName );
        rDoc.RenameTab( nScTab, aDummyName );
        // set sheet index to source range
        maSrcRange.aStart.SetTab( nScTab );
        maSrcRange.aEnd.SetTab( nScTab );
    }

    // open pivot cache storage stream
    SotStorageRef xSvStrg = OpenStorage( EXC_STORAGE_PTCACHE );
    SotStorageStreamRef xSvStrm = OpenStream( xSvStrg, ScfTools::GetHexStr( mnStrmId ) );
    if( !xSvStrm.Is() )
        return;

    // create Excel record stream object
    XclImpStream aPCStrm( *xSvStrm, GetRoot() );
    aPCStrm.CopyDecrypterFrom( rStrm );     // pivot cache streams are encrypted

    XclImpPCFieldRef xCurrField;    // current field for new items
    XclImpPCFieldVec aOrigFields;   // all standard fields with inline original  items
    XclImpPCFieldVec aPostpFields;  // all standard fields with postponed original items
    size_t nPostpIdx = 0;           // index to current field with postponed items
    bool bLoop = true;              // true = continue loop

    while( bLoop && aPCStrm.StartNextRecord() )
    {
        switch( aPCStrm.GetRecId() )
        {
            case EXC_ID_EOF:
                bLoop = false;
            break;

            case EXC_ID_SXDB:
                aPCStrm >> maPCInfo;
            break;

            case EXC_ID_SXFIELD:
            {
                xCurrField.reset();
                sal_uInt16 nNewFieldIdx = GetFieldCount();
                if( nNewFieldIdx < EXC_PC_MAXFIELDCOUNT )
                {
                    xCurrField.reset( new XclImpPCField( GetRoot(), *this, nNewFieldIdx ) );
                    maFields.push_back( xCurrField );
                    xCurrField->ReadSxfield( aPCStrm );
                    if( xCurrField->HasOrigItems() )
                    {
                        if( xCurrField->HasPostponedItems() )
                            aPostpFields.push_back( xCurrField );
                        else
                            aOrigFields.push_back( xCurrField );
                        // insert field name into generated source data, field remembers its column index
                        if( bGenerateSource && (nFieldScCol <= MAXCOL) )
                            xCurrField->WriteFieldNameToSource( nFieldScCol++, nScTab );
                    }
                    // do not read items into invalid/postponed fields
                    if( !xCurrField->HasInlineItems() )
                        xCurrField.reset();
                }
            }
            break;

            case EXC_ID_SXINDEXLIST:
                // read index list and insert all items into generated source data
                if( bGenerateSource && (nItemScRow <= MAXROW) && (++nItemScRow <= MAXROW) )
                {
                    for( XclImpPCFieldVec::const_iterator aIt = aOrigFields.begin(), aEnd = aOrigFields.end(); aIt != aEnd; ++aIt )
                    {
                        sal_uInt16 nItemIdx = (*aIt)->Has16BitIndexes() ? aPCStrm.ReaduInt16() : aPCStrm.ReaduInt8();
                        (*aIt)->WriteOrigItemToSource( nItemScRow, nScTab, nItemIdx );
                    }
                }
                xCurrField.reset();
            break;

            case EXC_ID_SXDOUBLE:
            case EXC_ID_SXBOOLEAN:
            case EXC_ID_SXERROR:
            case EXC_ID_SXINTEGER:
            case EXC_ID_SXSTRING:
            case EXC_ID_SXDATETIME:
            case EXC_ID_SXEMPTY:
                if( xCurrField.is() )                   // inline items
                {
                    xCurrField->ReadItem( aPCStrm );
                }
                else if( !aPostpFields.empty() )        // postponed items
                {
                    // read postponed item
                    aPostpFields[ nPostpIdx ]->ReadItem( aPCStrm );
                    // write item to source
                    if( bGenerateSource && (nItemScRow <= MAXROW) )
                    {
                        // start new row, if there are only postponed fields
                        if( aOrigFields.empty() && (nPostpIdx == 0) )
                            ++nItemScRow;
                        if( nItemScRow <= MAXROW )
                            aPostpFields[ nPostpIdx ]->WriteLastOrigItemToSource( nItemScRow, nScTab );
                    }
                    // get index of next postponed field
                    ++nPostpIdx;
                    if( nPostpIdx >= aPostpFields.size() )
                        nPostpIdx = 0;
                }
            break;

            case EXC_ID_SXNUMGROUP:
                if( xCurrField.is() )
                    xCurrField->ReadSxnumgroup( aPCStrm );
            break;

            case EXC_ID_SXGROUPINFO:
                if( xCurrField.is() )
                    xCurrField->ReadSxgroupinfo( aPCStrm );
            break;

            // known but ignored records
            case EXC_ID_SXRULE:
            case EXC_ID_SXFILT:
            case EXC_ID_00F5:
            case EXC_ID_SXNAME:
            case EXC_ID_SXPAIR:
            case EXC_ID_SXFMLA:
            case EXC_ID_SXFORMULA:
            case EXC_ID_SXDBEX:
            case EXC_ID_SXFDBTYPE:
            break;

            default:
                DBG_ERROR1( "XclImpPivotCache::ReadPivotCacheStream - unknown record 0x%04hX", aPCStrm.GetRecId() );
        }
    }

    DBG_ASSERT( maPCInfo.mnTotalFields == maFields.size(),
        "XclImpPivotCache::ReadPivotCacheStream - field count mismatch" );

    // set source range for external source data
    if( bGenerateSource && (nFieldScCol > 0) )
    {
        maSrcRange.aStart.SetCol( 0 );
        maSrcRange.aStart.SetRow( 0 );
        // nFieldScCol points to first unused column
        maSrcRange.aEnd.SetCol( nFieldScCol - 1 );
        // nItemScRow points to last used row
        maSrcRange.aEnd.SetRow( nItemScRow );
    }
}

// ============================================================================
// Pivot table
// ============================================================================

XclImpPTItem::XclImpPTItem( const XclImpPCField* pCacheField ) :
    mpCacheField( pCacheField )
{
}

const String* XclImpPTItem::GetItemName() const
{
    if( mpCacheField )
        if( const XclImpPCItem* pCacheItem = mpCacheField->GetItem( maItemInfo.mnCacheIdx ) )
            //! TODO: use XclImpPCItem::ConvertToText(), if all conversions are available
            return pCacheItem->IsEmpty() ? &String::EmptyString() : pCacheItem->GetText();
    return 0;
}

const String* XclImpPTItem::GetVisItemName() const
{
    return maItemInfo.HasVisName() ? maItemInfo.GetVisName() : GetItemName();
}

void XclImpPTItem::ReadSxvi( XclImpStream& rStrm )
{
    rStrm >> maItemInfo;
}

void XclImpPTItem::ConvertItem( ScDPSaveDimension& rSaveDim ) const
{
    if( const String* pItemName = GetItemName() )
    {
        ScDPSaveMember& rMember = *rSaveDim.GetMemberByName( *pItemName );
        rMember.SetIsVisible( !::get_flag( maItemInfo.mnFlags, EXC_SXVI_HIDDEN ) );
        rMember.SetShowDetails( !::get_flag( maItemInfo.mnFlags, EXC_SXVI_HIDEDETAIL ) );
    }
}

// ============================================================================

XclImpPTField::XclImpPTField( const XclImpPivotTable& rPTable, sal_uInt16 nCacheIdx ) :
    mrPTable( rPTable )
{
    maFieldInfo.mnCacheIdx = nCacheIdx;
}

// general field/item access --------------------------------------------------

const XclImpPCField* XclImpPTField::GetCacheField() const
{
    XclImpPivotCacheRef xPCache = mrPTable.GetPivotCache();
    return xPCache.is() ? xPCache->GetField( maFieldInfo.mnCacheIdx ) : 0;
}

const String& XclImpPTField::GetFieldName() const
{
    const XclImpPCField* pField = GetCacheField();
    return pField ? pField->GetFieldName( mrPTable.GetVisFieldNames() ) : String::EmptyString();
}

const String& XclImpPTField::GetVisFieldName() const
{
    const String* pVisName = maFieldInfo.GetVisName();
    return pVisName ? *pVisName : String::EmptyString();
}

const XclImpPTItem* XclImpPTField::GetItem( sal_uInt16 nItemIdx ) const
{
    return (nItemIdx < maItems.size()) ? maItems[ nItemIdx ].get() : 0;
}

const String* XclImpPTField::GetItemName( sal_uInt16 nItemIdx ) const
{
    const XclImpPTItem* pItem = GetItem( nItemIdx );
    return pItem ? pItem->GetItemName() : 0;
}

const String* XclImpPTField::GetVisItemName( sal_uInt16 nItemIdx ) const
{
    const XclImpPTItem* pItem = GetItem( nItemIdx );
    return pItem ? pItem->GetVisItemName() : 0;
}

// records --------------------------------------------------------------------

void XclImpPTField::ReadSxvd( XclImpStream& rStrm )
{
    rStrm >> maFieldInfo;
}

void XclImpPTField::ReadSxvdex( XclImpStream& rStrm )
{
    rStrm >> maFieldExtInfo;
}

void XclImpPTField::ReadSxvi( XclImpStream& rStrm )
{
    XclImpPTItemRef xItem( new XclImpPTItem( GetCacheField() ) );
    maItems.push_back( xItem );
    xItem->ReadSxvi( rStrm );
}

// row/column fields ----------------------------------------------------------

void XclImpPTField::ConvertRowColField( ScDPSaveData& rSaveData ) const
{
    DBG_ASSERT( maFieldInfo.mnAxes & EXC_SXVD_AXIS_ROWCOL, "XclImpPTField::ConvertRowColField - no row/column field" );
    // special data orientation field?
    if( maFieldInfo.mnCacheIdx == EXC_SXIVD_DATA )
        rSaveData.GetDataLayoutDimension()->SetOrientation( static_cast< USHORT >( maFieldInfo.GetApiOrient( EXC_SXVD_AXIS_ROWCOL ) ) );
    else
        ConvertRCPField( rSaveData );
}

// page fields ----------------------------------------------------------------

void XclImpPTField::SetPageFieldInfo( const XclPTPageFieldInfo& rPageInfo )
{
    maPageInfo = rPageInfo;
}

void XclImpPTField::ConvertPageField( ScDPSaveData& rSaveData ) const
{
    DBG_ASSERT( maFieldInfo.mnAxes & EXC_SXVD_AXIS_PAGE, "XclImpPTField::ConvertPageField - no page field" );
    if( ScDPSaveDimension* pSaveDim = ConvertRCPField( rSaveData ) )
        pSaveDim->SetCurrentPage( GetItemName( maPageInfo.mnSelItem ) );
}

// hidden fields --------------------------------------------------------------

void XclImpPTField::ConvertHiddenField( ScDPSaveData& rSaveData ) const
{
    DBG_ASSERT( (maFieldInfo.mnAxes & EXC_SXVD_AXIS_ROWCOLPAGE) == 0, "XclImpPTField::ConvertHiddenField - field not hidden" );
    ConvertRCPField( rSaveData );
}

// data fields ----------------------------------------------------------------

bool XclImpPTField::HasDataFieldInfo() const
{
    return !maDataInfoList.empty();
}

void XclImpPTField::AddDataFieldInfo( const XclPTDataFieldInfo& rDataInfo )
{
    DBG_ASSERT( maFieldInfo.mnAxes & EXC_SXVD_AXIS_DATA, "XclImpPTField::AddDataFieldInfo - no data field" );
    maDataInfoList.push_back( rDataInfo );
}

void XclImpPTField::ConvertDataField( ScDPSaveData& rSaveData ) const
{
    DBG_ASSERT( maFieldInfo.mnAxes & EXC_SXVD_AXIS_DATA, "XclImpPTField::ConvertDataField - no data field" );
    DBG_ASSERT( !maDataInfoList.empty(), "XclImpPTField::ConvertDataField - no data field info" );
    if( !maDataInfoList.empty() )
    {
        const String& rFieldName = GetFieldName();
        if( rFieldName.Len() > 0 )
        {
            XclPTDataFieldInfoList::const_iterator aIt = maDataInfoList.begin(), aEnd = maDataInfoList.end();

            ScDPSaveDimension& rSaveDim = *rSaveData.GetNewDimensionByName( rFieldName );
            ConvertDataField( rSaveDim, *aIt );

            // multiple data fields -> clone dimension
            for( ++aIt; aIt != aEnd; ++aIt )
            {
                ScDPSaveDimension& rDupDim = rSaveData.DuplicateDimension( rSaveDim );
                ConvertDataFieldInfo( rDupDim, *aIt );
            }
        }
    }
}

// private --------------------------------------------------------------------

ScDPSaveDimension* XclImpPTField::ConvertRCPField( ScDPSaveData& rSaveData ) const
{
    const String& rFieldName = GetFieldName();
    if( rFieldName.Len() == 0 )
        return 0;

    const XclImpPCField* pCacheField = GetCacheField();
    if( !pCacheField || !pCacheField->IsSupportedField() )
        return 0;

    ScDPSaveDimension& rSaveDim = *rSaveData.GetNewDimensionByName( rFieldName );

    // orientation
    rSaveDim.SetOrientation( static_cast< USHORT >( maFieldInfo.GetApiOrient( EXC_SXVD_AXIS_ROWCOLPAGE ) ) );

    // general field info
    ConvertFieldInfo( rSaveDim );

    // visible name
    if( const String* pVisName = maFieldInfo.GetVisName() )
        if( pVisName->Len() > 0 )
            rSaveDim.SetLayoutName( pVisName );

    // subtotal function(s)
    XclPTSubtotalVec aSubtotalVec;
    maFieldInfo.GetSubtotals( aSubtotalVec );
    if( !aSubtotalVec.empty() )
        rSaveDim.SetSubTotals( static_cast< long >( aSubtotalVec.size() ), &aSubtotalVec[ 0 ] );

    // sorting
    DataPilotFieldSortInfo aSortInfo;
    aSortInfo.Field = mrPTable.GetDataFieldName( maFieldExtInfo.mnSortField );
    aSortInfo.IsAscending = ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_SORT_ASC );
    aSortInfo.Mode = maFieldExtInfo.GetApiSortMode();
    rSaveDim.SetSortInfo( &aSortInfo );

    // auto show
    DataPilotFieldAutoShowInfo aShowInfo;
    aShowInfo.IsEnabled = ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_AUTOSHOW );
    aShowInfo.ShowItemsMode = maFieldExtInfo.GetApiAutoShowMode();
    aShowInfo.ItemCount = maFieldExtInfo.GetApiAutoShowCount();
    aShowInfo.DataField = mrPTable.GetDataFieldName( maFieldExtInfo.mnShowField );
    rSaveDim.SetAutoShowInfo( &aShowInfo );

    // layout
    DataPilotFieldLayoutInfo aLayoutInfo;
    aLayoutInfo.LayoutMode = maFieldExtInfo.GetApiLayoutMode();
    aLayoutInfo.AddEmptyLines = ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_LAYOUT_BLANK );
    rSaveDim.SetLayoutInfo( &aLayoutInfo );

    // grouping info
    pCacheField->ConvertGroupField( rSaveData, mrPTable.GetVisFieldNames() );

    return &rSaveDim;
}

void XclImpPTField::ConvertFieldInfo( ScDPSaveDimension& rSaveDim ) const
{
    rSaveDim.SetShowEmpty( ::get_flag( maFieldExtInfo.mnFlags, EXC_SXVDEX_SHOWALL ) );
    ConvertItems( rSaveDim );
}

void XclImpPTField::ConvertDataField( ScDPSaveDimension& rSaveDim, const XclPTDataFieldInfo& rDataInfo ) const
{
    // orientation
    rSaveDim.SetOrientation( DataPilotFieldOrientation_DATA );
    // general field info
    ConvertFieldInfo( rSaveDim );
    // extended data field info
    ConvertDataFieldInfo( rSaveDim, rDataInfo );
}

void XclImpPTField::ConvertDataFieldInfo( ScDPSaveDimension& rSaveDim, const XclPTDataFieldInfo& rDataInfo ) const
{
    // visible name
    if( const String* pVisName = rDataInfo.GetVisName() )
        if( pVisName->Len() > 0 )
            rSaveDim.SetLayoutName( pVisName );

    // aggregation function
    rSaveDim.SetFunction( static_cast< USHORT >( rDataInfo.GetApiAggFunc() ) );

    // result field reference
    sal_Int32 nRefType = rDataInfo.GetApiRefType();
    if( nRefType != ::com::sun::star::sheet::DataPilotFieldReferenceType::NONE )
    {
        DataPilotFieldReference aFieldRef;
        aFieldRef.ReferenceType = nRefType;

        if( const XclImpPTField* pRefField = mrPTable.GetField( rDataInfo.mnRefField ) )
        {
            aFieldRef.ReferenceField = pRefField->GetFieldName();
            aFieldRef.ReferenceItemType = rDataInfo.GetApiRefItemType();
            if( aFieldRef.ReferenceItemType == ::com::sun::star::sheet::DataPilotFieldReferenceItemType::NAMED )
                if( const String* pRefItemName = pRefField->GetItemName( rDataInfo.mnRefItem ) )
                    aFieldRef.ReferenceItemName = *pRefItemName;
        }

        rSaveDim.SetReferenceValue( &aFieldRef );
    }
}

void XclImpPTField::ConvertItems( ScDPSaveDimension& rSaveDim ) const
{
    for( XclImpPTItemVec::const_iterator aIt = maItems.begin(), aEnd = maItems.end(); aIt != aEnd; ++aIt )
        (*aIt)->ConvertItem( rSaveDim );
}

// ============================================================================

XclImpPivotTable::XclImpPivotTable( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot ),
    maDataOrientField( *this, EXC_SXIVD_DATA )
{
}

XclImpPivotTable::~XclImpPivotTable()
{
}

// cache/field access, misc. --------------------------------------------------

sal_uInt16 XclImpPivotTable::GetFieldCount() const
{
    return static_cast< sal_uInt16 >( maFields.size() );
}

const XclImpPTField* XclImpPivotTable::GetField( sal_uInt16 nFieldIdx ) const
{
    return (nFieldIdx == EXC_SXIVD_DATA) ? &maDataOrientField :
        ((nFieldIdx < maFields.size()) ? maFields[ nFieldIdx ].get() : 0);
}

XclImpPTField* XclImpPivotTable::GetFieldAcc( sal_uInt16 nFieldIdx )
{
    // do not return maDataOrientField
    return (nFieldIdx < maFields.size()) ? maFields[ nFieldIdx ].get() : 0;
}

const String& XclImpPivotTable::GetFieldName( sal_uInt16 nFieldIdx ) const
{
    if( const XclImpPTField* pField = GetField( nFieldIdx ) )
        return pField->GetFieldName();
    return EMPTY_STRING;
}

const XclImpPTField* XclImpPivotTable::GetDataField( sal_uInt16 nDataFieldIdx ) const
{
    if( nDataFieldIdx < maOrigDataFields.size() )
        return GetField( maOrigDataFields[ nDataFieldIdx ] );
    return 0;
}

const String& XclImpPivotTable::GetDataFieldName( sal_uInt16 nDataFieldIdx ) const
{
    if( const XclImpPTField* pField = GetDataField( nDataFieldIdx ) )
        return pField->GetFieldName();
    return EMPTY_STRING;
}

// records --------------------------------------------------------------------

void XclImpPivotTable::ReadSxview( XclImpStream& rStrm )
{
    rStrm >> maPTInfo;

    GetAddressConverter().ConvertRange(
        maOutScRange, maPTInfo.maOutXclRange, GetCurrScTab(), GetCurrScTab(), true );

    mxPCache = GetPivotTableManager().GetPivotCache( maPTInfo.mnCacheIdx );
    mxCurrField.reset();
}

void XclImpPivotTable::ReadSxvd( XclImpStream& rStrm )
{
    sal_uInt16 nFieldCount = GetFieldCount();
    if( nFieldCount < EXC_PT_MAXFIELDCOUNT )
    {
        // cache index for the field is equal to the SXVD record index
        mxCurrField.reset( new XclImpPTField( *this, nFieldCount ) );
        maFields.push_back( mxCurrField );
        mxCurrField->ReadSxvd( rStrm );
        // add visible name of new field to list of visible names
        maVisFieldNames.push_back( mxCurrField->GetVisFieldName() );
        DBG_ASSERT( maFields.size() == maVisFieldNames.size(),
            "XclImpPivotTable::ReadSxvd - wrong size of visible name array" );
    }
    else
        mxCurrField.reset();
}

void XclImpPivotTable::ReadSxvi( XclImpStream& rStrm )
{
    if( mxCurrField.is() )
        mxCurrField->ReadSxvi( rStrm );
}

void XclImpPivotTable::ReadSxvdex( XclImpStream& rStrm )
{
    if( mxCurrField.is() )
        mxCurrField->ReadSxvdex( rStrm );
}

void XclImpPivotTable::ReadSxivd( XclImpStream& rStrm )
{
    mxCurrField.reset();

    // find the index vector to fill (row SXIVD doesn't exist without row fields)
    ScfUInt16Vec* pFieldVec = 0;
    if( maRowFields.empty() && (maPTInfo.mnRowFields > 0) )
        pFieldVec = &maRowFields;
    else if( maColFields.empty() && (maPTInfo.mnColFields > 0) )
        pFieldVec = &maColFields;

    // fill the vector from record data
    if( pFieldVec )
    {
        sal_uInt16 nSize = ulimit_cast< sal_uInt16 >( rStrm.GetRecSize() / 2, EXC_PT_MAXROWCOLCOUNT );
        pFieldVec->reserve( nSize );
        for( sal_uInt16 nIdx = 0; nIdx < nSize; ++nIdx )
        {
            sal_uInt16 nFieldIdx;
            rStrm >> nFieldIdx;
            pFieldVec->push_back( nFieldIdx );

            // set orientation at special data orientation field
            if( nFieldIdx == EXC_SXIVD_DATA )
            {
                sal_uInt16 nAxis = (pFieldVec == &maRowFields) ? EXC_SXVD_AXIS_ROW : EXC_SXVD_AXIS_COL;
                maDataOrientField.SetAxes( nAxis );
            }
        }
    }
}

void XclImpPivotTable::ReadSxpi( XclImpStream& rStrm )
{
    mxCurrField.reset();

    sal_uInt16 nSize = ulimit_cast< sal_uInt16 >( rStrm.GetRecSize() / 6 );
    for( sal_uInt16 nEntry = 0; nEntry < nSize; ++nEntry )
    {
        XclPTPageFieldInfo aPageInfo;
        rStrm >> aPageInfo;
        if( XclImpPTField* pField = GetFieldAcc( aPageInfo.mnField ) )
        {
            maPageFields.push_back( aPageInfo.mnField );
            pField->SetPageFieldInfo( aPageInfo );
        }
        GetObjectManager().SetSkipObj( GetCurrScTab(), aPageInfo.mnObjId );
    }
}

void XclImpPivotTable::ReadSxdi( XclImpStream& rStrm )
{
    mxCurrField.reset();

    XclPTDataFieldInfo aDataInfo;
    rStrm >> aDataInfo;
    if( XclImpPTField* pField = GetFieldAcc( aDataInfo.mnField ) )
    {
        maOrigDataFields.push_back( aDataInfo.mnField );
        // DataPilot does not support double data fields -> add first appearence to index list only
        if( !pField->HasDataFieldInfo() )
            maFiltDataFields.push_back( aDataInfo.mnField );
        pField->AddDataFieldInfo( aDataInfo );
    }
}

void XclImpPivotTable::ReadSxex( XclImpStream& rStrm )
{
    rStrm >> maPTExtInfo;
}

// ----------------------------------------------------------------------------

void XclImpPivotTable::Convert()
{
    if( !mxPCache || !mxPCache->GetSourceRange().IsValid() )
		return;

	ScDPSaveData aSaveData;

    // *** global settings ***

    aSaveData.SetRowGrand( ::get_flag( maPTInfo.mnFlags, EXC_SXVIEW_ROWGRAND ) );
    aSaveData.SetColumnGrand( ::get_flag( maPTInfo.mnFlags, EXC_SXVIEW_COLGRAND ) );
    aSaveData.SetFilterButton( FALSE );
    aSaveData.SetDrillDown( ::get_flag( maPTExtInfo.mnFlags, EXC_SXEX_DRILLDOWN ) );

    // *** fields ***

    ScfUInt16Vec::const_iterator aIt, aEnd;

    // row fields
    for( aIt = maRowFields.begin(), aEnd = maRowFields.end(); aIt != aEnd; ++aIt )
        if( const XclImpPTField* pField = GetField( *aIt ) )
            pField->ConvertRowColField( aSaveData );

    // column fields
    for( aIt = maColFields.begin(), aEnd = maColFields.end(); aIt != aEnd; ++aIt )
        if( const XclImpPTField* pField = GetField( *aIt ) )
            pField->ConvertRowColField( aSaveData );

    // page fields
    for( aIt = maPageFields.begin(), aEnd = maPageFields.end(); aIt != aEnd; ++aIt )
        if( const XclImpPTField* pField = GetField( *aIt ) )
            pField->ConvertPageField( aSaveData );

    // hidden fields
    for( sal_uInt16 nField = 0, nCount = GetFieldCount(); nField < nCount; ++nField )
        if( const XclImpPTField* pField = GetField( nField ) )
            if( (pField->GetAxes() & EXC_SXVD_AXIS_ROWCOLPAGE) == 0 )
                pField->ConvertHiddenField( aSaveData );

    // data fields
    for( aIt = maFiltDataFields.begin(), aEnd = maFiltDataFields.end(); aIt != aEnd; ++aIt )
        if( const XclImpPTField* pField = GetField( *aIt ) )
            pField->ConvertDataField( aSaveData );

    // *** insert into Calc document ***

    // create source descriptor
    ScSheetSourceDesc aDesc;
    aDesc.aSourceRange = mxPCache->GetSourceRange();

    // adjust output range to include the page fields
    ScRange aOutRange( maOutScRange );
    if( !maPageFields.empty() )
    {
        SCsROW nDecRows = ::std::min< SCsROW >( aOutRange.aStart.Row(), maPageFields.size() + 1 );
        aOutRange.aStart.IncRow( -nDecRows );
    }

    // create the DataPilot
    ScDPObject* pDPObj = new ScDPObject( GetDocPtr() );
    pDPObj->SetName( maPTInfo.maTableName );
    pDPObj->SetSaveData( aSaveData );
    pDPObj->SetSheetDesc( aDesc );
    pDPObj->SetOutRange( aOutRange );
    pDPObj->SetAlive( TRUE );
    GetDoc().GetDPCollection()->Insert( pDPObj );
}

// ============================================================================
// ============================================================================

XclImpPivotTableManager::XclImpPivotTableManager( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot )
{
}

XclImpPivotTableManager::~XclImpPivotTableManager()
{
}

// pivot cache records --------------------------------------------------------

XclImpPivotCacheRef XclImpPivotTableManager::GetPivotCache( sal_uInt16 nCacheIdx )
{
    XclImpPivotCacheRef xPCache;
    if( nCacheIdx < maPCaches.size() )
        xPCache = maPCaches[ nCacheIdx ];
    return xPCache;
}

void XclImpPivotTableManager::ReadSxidstm( XclImpStream& rStrm )
{
    XclImpPivotCacheRef xPCache( new XclImpPivotCache( GetRoot() ) );
    maPCaches.push_back( xPCache );
    xPCache->ReadSxidstm( rStrm );
}

void XclImpPivotTableManager::ReadSxvs( XclImpStream& rStrm )
{
    if( !maPCaches.empty() )
        maPCaches.back()->ReadSxvs( rStrm );
}

void XclImpPivotTableManager::ReadDconref( XclImpStream& rStrm )
{
    if( !maPCaches.empty() )
        maPCaches.back()->ReadDconref( rStrm );
}

// pivot table records --------------------------------------------------------

void XclImpPivotTableManager::ReadSxview( XclImpStream& rStrm )
{
    XclImpPivotTableRef xPTable( new XclImpPivotTable( GetRoot() ) );
    maPTables.push_back( xPTable );
    xPTable->ReadSxview( rStrm );
}

void XclImpPivotTableManager::ReadSxvd( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxvd( rStrm );
}

void XclImpPivotTableManager::ReadSxvdex( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxvdex( rStrm );
}

void XclImpPivotTableManager::ReadSxivd( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxivd( rStrm );
}

void XclImpPivotTableManager::ReadSxpi( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxpi( rStrm );
}

void XclImpPivotTableManager::ReadSxdi( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxdi( rStrm );
}

void XclImpPivotTableManager::ReadSxvi( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxvi( rStrm );
}

void XclImpPivotTableManager::ReadSxex( XclImpStream& rStrm )
{
    if( !maPTables.empty() )
        maPTables.back()->ReadSxex( rStrm );
}

// ----------------------------------------------------------------------------

void XclImpPivotTableManager::ReadPivotCaches( XclImpStream& rStrm )
{
    for( XclImpPivotCacheVec::iterator aIt = maPCaches.begin(), aEnd = maPCaches.end(); aIt != aEnd; ++aIt )
        (*aIt)->ReadPivotCacheStream( rStrm );
}

void XclImpPivotTableManager::ConvertPivotTables()
{
    for( XclImpPivotTableVec::iterator aIt = maPTables.begin(), aEnd = maPTables.end(); aIt != aEnd; ++aIt )
        (*aIt)->Convert();
}

// ============================================================================

