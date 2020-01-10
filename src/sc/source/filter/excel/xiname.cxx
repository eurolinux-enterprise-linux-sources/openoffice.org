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
#include "xiname.hxx"
#include "rangenam.hxx"
#include "xistream.hxx"

// for formula compiler
#include "excform.hxx"
// for filter manager
#include "excimp8.hxx"

// ============================================================================
// *** Implementation ***
// ============================================================================

XclImpName::XclImpName( XclImpStream& rStrm, sal_uInt16 nXclNameIdx ) :
    XclImpRoot( rStrm.GetRoot() ),
    mpScData( 0 ),
    mcBuiltIn( EXC_BUILTIN_UNKNOWN ),
    mnScTab( SCTAB_MAX ),
    mbVBName( false )
{
    ExcelToSc& rFmlaConv = GetOldFmlaConverter();
    ScRangeName& rRangeNames = GetNamedRanges();

    // 1) *** read data from stream *** ---------------------------------------

    sal_uInt16 nFlags = 0, nFmlaSize = 0, nExtSheet = EXC_NAME_GLOBAL, nXclTab = EXC_NAME_GLOBAL;
    sal_uInt8 nNameLen = 0, nShortCut;

    switch( GetBiff() )
    {
        case EXC_BIFF2:
        {
            sal_uInt8 nFlagsBiff2;
            rStrm >> nFlagsBiff2;
            rStrm.Ignore( 1 );
            rStrm >> nShortCut >> nNameLen;
            nFmlaSize = rStrm.ReaduInt8();
            ::set_flag( nFlags, EXC_NAME_FUNC, ::get_flag( nFlagsBiff2, EXC_NAME2_FUNC ) );
        }
        break;

        case EXC_BIFF3:
        case EXC_BIFF4:
        {
            rStrm >> nFlags >> nShortCut >> nNameLen >> nFmlaSize;
        }
        break;

        case EXC_BIFF5:
        case EXC_BIFF8:
        {
            rStrm >> nFlags >> nShortCut >> nNameLen >> nFmlaSize >> nExtSheet >> nXclTab;
            rStrm.Ignore( 4 );
        }
        break;

        default: DBG_ERROR_BIFF();
    }

    if( GetBiff() <= EXC_BIFF5 )
        maXclName = rStrm.ReadRawByteString( nNameLen );
    else
        maXclName = rStrm.ReadUniString( nNameLen );

    // 2) *** convert sheet index and name *** --------------------------------

    // Visual Basic procedure
    mbVBName = ::get_flag( nFlags, EXC_NAME_VB );

    // get built-in name, or convert characters invalid in Calc
    bool bBuiltIn = ::get_flag( nFlags, EXC_NAME_BUILTIN );

    // special case for BIFF5 filter range - name appears as plain text without built-in flag
    if( (GetBiff() == EXC_BIFF5) && (maXclName == XclTools::GetXclBuiltInDefName( EXC_BUILTIN_FILTERDATABASE )) )
    {
        bBuiltIn = true;
        maXclName.Assign( EXC_BUILTIN_FILTERDATABASE );
    }

    // convert Excel name to Calc name
    if( mbVBName )
    {
        // VB macro name
        maScName = maXclName;
    }
    else if( bBuiltIn )
    {
        // built-in name
        if( maXclName.Len() )
            mcBuiltIn = maXclName.GetChar( 0 );
        if( mcBuiltIn == '?' )      // NUL character is imported as '?'
            mcBuiltIn = '\0';
        maScName = XclTools::GetBuiltInDefName( mcBuiltIn );
    }
    else
    {
        // any other name
        maScName = maXclName;
        ScfTools::ConvertToScDefinedName( maScName );
    }

    // add index for local names
    if( nXclTab != EXC_NAME_GLOBAL )
    {
        sal_uInt16 nUsedTab = (GetBiff() == EXC_BIFF8) ? nXclTab : nExtSheet;
        maScName.Append( '_' ).Append( String::CreateFromInt32( nUsedTab ) );
        // TODO: may not work for BIFF5, handle skipped sheets (all BIFF)
        mnScTab = static_cast< SCTAB >( nUsedTab - 1 );
    }

    // find an unused name
    String aOrigName( maScName );
    sal_Int32 nCounter = 0;
    USHORT nDummy;
    while( rRangeNames.SearchName( maScName, nDummy ) )
        maScName.Assign( aOrigName ).Append( ' ' ).Append( String::CreateFromInt32( ++nCounter ) );

    // 3) *** convert the name definition formula *** -------------------------

    rFmlaConv.Reset();
    const ScTokenArray* pTokArr = 0; // pointer to token array, owned by rFmlaConv
    RangeType nNameType = RT_NAME;

    if( ::get_flag( nFlags, EXC_NAME_BIG ) )
    {
        // special, unsupported name
        rFmlaConv.GetDummy( pTokArr );
    }
    else if( bBuiltIn )
    {
        // --- print ranges or title ranges ---
        rStrm.PushPosition();
        switch( mcBuiltIn )
        {
            case EXC_BUILTIN_PRINTAREA:
                if( rFmlaConv.Convert( GetPrintAreaBuffer(), rStrm, nFmlaSize, FT_RangeName ) == ConvOK )
                    nNameType |= RT_PRINTAREA;
            break;
            case EXC_BUILTIN_PRINTTITLES:
                if( rFmlaConv.Convert( GetTitleAreaBuffer(), rStrm, nFmlaSize, FT_RangeName ) == ConvOK )
                    nNameType |= RT_COLHEADER | RT_ROWHEADER;
            break;
        }
        rStrm.PopPosition();

        // --- name formula ---
        // JEG : double check this.  It is clearly false for normal names
        //  but some of the builtins (sheettitle?) might be able to handle arrays
        rFmlaConv.Convert( pTokArr, rStrm, nFmlaSize, false, FT_RangeName );

        // --- auto or advanced filter ---
        if( (GetBiff() == EXC_BIFF8) && pTokArr && bBuiltIn )
        {
            ScRange aRange;
            if( pTokArr->IsReference( aRange ) )
            {
                switch( mcBuiltIn )
                {
                    case EXC_BUILTIN_FILTERDATABASE:
                        GetFilterManager().Insert( &GetOldRoot(), aRange, maScName );
                    break;
                    case EXC_BUILTIN_CRITERIA:
                        GetFilterManager().AddAdvancedRange( aRange );
                        nNameType |= RT_CRITERIA;
                    break;
                    case EXC_BUILTIN_EXTRACT:
                        if( pTokArr->IsValidReference( aRange ) )
                            GetFilterManager().AddExtractPos( aRange );
                    break;
                }
            }
        }
    }
    else if( nFmlaSize > 0 )
    {
        // regular defined name
        rFmlaConv.Convert( pTokArr, rStrm, nFmlaSize, true, FT_RangeName );
    }

    // 4) *** create a defined name in the Calc document *** ------------------

    if( pTokArr && (bBuiltIn || !::get_flag( nFlags, EXC_NAME_HIDDEN )) && !mbVBName )
    {
        // create the Calc name data
        ScRangeData* pData = new ScRangeData( GetDocPtr(), maScName, *pTokArr, ScAddress(), nNameType );
        pData->GuessPosition();             // calculate base position for relative refs
        pData->SetIndex( nXclNameIdx );     // used as unique identifier in formulas
        rRangeNames.Insert( pData );        // takes ownership of pData
        mpScData = pData;                   // cache for later use
    }
}

// ----------------------------------------------------------------------------

XclImpNameManager::XclImpNameManager( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot )
{
}

void XclImpNameManager::ReadName( XclImpStream& rStrm )
{
    ULONG nCount = maNameList.Count();
    if( nCount < 0xFFFF )
        maNameList.Append( new XclImpName( rStrm, static_cast< sal_uInt16 >( nCount + 1 ) ) );
}

const XclImpName* XclImpNameManager::FindName( const String& rXclName, SCTAB nScTab ) const
{
    const XclImpName* pGlobalName = 0;   // a found global name
    const XclImpName* pLocalName = 0;    // a found local name
    for( const XclImpName* pName = maNameList.First(); pName && !pLocalName; pName = maNameList.Next() )
    {
        if( pName->GetXclName() == rXclName )
        {
            if( pName->GetScTab() == nScTab )
                pLocalName = pName;
            else if( pName->IsGlobal() )
                pGlobalName = pName;
        }
    }
    return pLocalName ? pLocalName : pGlobalName;
}

const XclImpName* XclImpNameManager::GetName( sal_uInt16 nXclNameIdx ) const
{
    DBG_ASSERT( nXclNameIdx > 0, "XclImpNameManager::GetName - index must be >0" );
    return maNameList.GetObject( nXclNameIdx - 1 );
}

// ============================================================================

