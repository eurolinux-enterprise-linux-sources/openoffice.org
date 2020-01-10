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
// INCLUDE ---------------------------------------------------------------

#include <rangelst.hxx>
#include <sfx2/app.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/objsh.hxx>
#include <basic/sbmeth.hxx>
#include <basic/sbmod.hxx>
#include <basic/sbstar.hxx>
#include <basic/sbx.hxx>
#include <svtools/zforlist.hxx>
#include <tools/urlobj.hxx>
#include <rtl/logfile.hxx>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <com/sun/star/table/XCellRange.hpp>

#include "interpre.hxx"
#include "global.hxx"
#include "dbcolect.hxx"
#include "cell.hxx"
#include "callform.hxx"
#include "addincol.hxx"
#include "document.hxx"
#include "dociter.hxx"
#include "docoptio.hxx"
#include "scmatrix.hxx"
#include "adiasync.hxx"
#include "sc.hrc"
#include "cellsuno.hxx"
#include "optuno.hxx"
#include "rangeseq.hxx"
#include "addinlis.hxx"
#include "jumpmatrix.hxx"
#include "parclass.hxx"
#include "externalrefmgr.hxx"

#include <math.h>
#include <float.h>
#include <map>
#include <algorithm>
#include <functional>

using namespace com::sun::star;
using namespace formula;

#define ADDIN_MAXSTRLEN 256

// Implementiert in ui\miscdlgs\teamdlg.cxx

extern void ShowTheTeam();

extern BOOL bOderSo; // in GLOBAL.CXX

//-----------------------------static data -----------------

#if SC_SPEW_ENABLED
ScSpew ScInterpreter::theSpew;
#endif

//-------------------------------------------------------------------------
// Funktionen fuer den Zugriff auf das Document
//-------------------------------------------------------------------------


void ScInterpreter::ReplaceCell( ScAddress& rPos )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ReplaceCell" );
    ScInterpreterTableOpParams* pTOp = pDok->aTableOpList.First();
    while (pTOp)
    {
        if ( rPos == pTOp->aOld1 )
        {
            rPos = pTOp->aNew1;
            return ;
        }
        else if ( rPos == pTOp->aOld2 )
        {
            rPos = pTOp->aNew2;
            return ;
        }
        else
            pTOp = pDok->aTableOpList.Next();
    }
}


void ScInterpreter::ReplaceCell( SCCOL& rCol, SCROW& rRow, SCTAB& rTab )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ReplaceCell" );
    ScAddress aCellPos( rCol, rRow, rTab );
    ScInterpreterTableOpParams* pTOp = pDok->aTableOpList.First();
    while (pTOp)
    {
        if ( aCellPos == pTOp->aOld1 )
        {
            rCol = pTOp->aNew1.Col();
            rRow = pTOp->aNew1.Row();
            rTab = pTOp->aNew1.Tab();
            return ;
        }
        else if ( aCellPos == pTOp->aOld2 )
        {
            rCol = pTOp->aNew2.Col();
            rRow = pTOp->aNew2.Row();
            rTab = pTOp->aNew2.Tab();
            return ;
        }
        else
            pTOp = pDok->aTableOpList.Next();
    }
}


BOOL ScInterpreter::IsTableOpInRange( const ScRange& rRange )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::IsTableOpInRange" );
    if ( rRange.aStart == rRange.aEnd )
        return FALSE;   // not considered to be a range in TableOp sense

    // we can't replace a single cell in a range
    ScInterpreterTableOpParams* pTOp = pDok->aTableOpList.First();
    while (pTOp)
    {
        if ( rRange.In( pTOp->aOld1 ) )
            return TRUE;
        if ( rRange.In( pTOp->aOld2 ) )
            return TRUE;
        pTOp = pDok->aTableOpList.Next();
    }
    return FALSE;
}


ULONG ScInterpreter::GetCellNumberFormat( const ScAddress& rPos, const ScBaseCell* pCell)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetCellNumberFormat" );
    ULONG nFormat;
    USHORT nErr;
    if ( pCell )
    {
        if ( pCell->GetCellType() == CELLTYPE_FORMULA )
            nErr = ((ScFormulaCell*)pCell)->GetErrCode();
        else
            nErr = 0;
        nFormat = pDok->GetNumberFormat( rPos );
        if ( pCell->GetCellType() == CELLTYPE_FORMULA
          && ((nFormat % SV_COUNTRY_LANGUAGE_OFFSET) == 0) )
            nFormat = ((ScFormulaCell*)pCell)->GetStandardFormat( *pFormatter,
                nFormat );
    }
    else
    {
        nFormat = pDok->GetNumberFormat( rPos );
        nErr = 0;
    }
    SetError(nErr);
    return nFormat;
}


/// Only ValueCell, formula cells already store the result rounded.
double ScInterpreter::GetValueCellValue( const ScAddress& rPos, const ScValueCell* pCell )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetValueCellValue" );
    double fVal = pCell->GetValue();
    if ( bCalcAsShown && fVal != 0.0 )
    {
        ULONG nFormat = pDok->GetNumberFormat( rPos );
        fVal = pDok->RoundValueAsShown( fVal, nFormat );
    }
    return fVal;
}


/** Convert string content to numeric value.
    
    Converted are only integer numbers including exponent, and ISO 8601 dates 
    and times in their extended formats with separators. Anything else, 
    especially fractional numeric values with decimal separators or dates other 
    than ISO 8601 would be locale dependent and is a no-no. Leading and 
    trailing blanks are ignored.

    The following ISO 8601 formats are converted:

    CCYY-MM-DD
    CCYY-MM-DDThh:mm
    CCYY-MM-DDThh:mm:ss
    CCYY-MM-DDThh:mm:ss,s
    CCYY-MM-DDThh:mm:ss.s
    hh:mm
    hh:mm:ss
    hh:mm:ss,s
    hh:mm:ss.s

    The century CC may not be omitted and the two-digit year setting is not 
    taken into account. Instead of the T date and time separator exactly one 
    blank may be used.

    If a date is given, it must be a valid Gregorian calendar date. In this 
    case the optional time must be in the range 00:00 to 23:59:59.99999...
    If only time is given, it may have any value for hours, taking elapsed time 
    into account; minutes and seconds are limited to the value 59 as well.
 */

double ScInterpreter::ConvertStringToValue( const String& rStr )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ConvertStringToValue" );
    double fValue = 0.0;
    ::rtl::OUString aStr( rStr);
    rtl_math_ConversionStatus eStatus;
    sal_Int32 nParseEnd;
    // Decimal and group separator 0 => only integer and possibly exponent, 
    // stops at first non-digit non-sign.
    fValue = ::rtl::math::stringToDouble( aStr, 0, 0, &eStatus, &nParseEnd);
    sal_Int32 nLen;
    if (eStatus == rtl_math_ConversionStatus_Ok && nParseEnd < (nLen = aStr.getLength()))
    {
        // Not at string end, check for trailing blanks or switch to date or 
        // time parsing or bail out.
        const sal_Unicode* const pStart = aStr.getStr();
        const sal_Unicode* p = pStart + nParseEnd;
        const sal_Unicode* const pStop = pStart + nLen;
        switch (*p++)
        {
            case ' ':
                while (p < pStop && *p == ' ')
                    ++p;
                if (p < pStop)
                    SetError( errNoValue);
                break;
            case '-':
            case ':':
                {
                    bool bDate = (*(p-1) == '-');
                    enum State { year = 0, month, day, hour, minute, second, fraction, done, blank, stop };
                    sal_Int32 nUnit[done] = {0,0,0,0,0,0,0};
                    const sal_Int32 nLimit[done] = {0,12,31,0,59,59,0};
                    State eState = (bDate ? month : minute);
                    nCurFmtType = (bDate ? NUMBERFORMAT_DATE : NUMBERFORMAT_TIME);
                    nUnit[eState-1] = aStr.copy( 0, nParseEnd).toInt32();
                    const sal_Unicode* pLastStart = p;
                    // Ensure there's no preceding sign. Negative dates 
                    // currently aren't handled correctly. Also discard 
                    // +CCYY-MM-DD
                    p = pStart;
                    while (p < pStop && *p == ' ')
                        ++p;
                    if (p < pStop && !CharClass::isAsciiDigit(*p))
                        SetError( errNoValue);
                    p = pLastStart;
                    while (p < pStop && !nGlobalError && eState < blank)
                    {
                        if (eState == minute)
                            nCurFmtType |= NUMBERFORMAT_TIME;
                        if (CharClass::isAsciiDigit(*p))
                        {
                            // Maximum 2 digits per unit, except fractions.
                            if (p - pLastStart >= 2 && eState != fraction)
                                SetError( errNoValue);
                        }
                        else if (p > pLastStart)
                        {
                            // We had at least one digit.
                            if (eState < done)
                            {
                                nUnit[eState] = aStr.copy( pLastStart - pStart, p - pLastStart).toInt32();
                                if (nLimit[eState] && nLimit[eState] < nUnit[eState])
                                    SetError( errNoValue);
                            }
                            pLastStart = p + 1;     // hypothetical next start
                            // Delimiters must match, a trailing delimiter 
                            // yields an invalid date/time.
                            switch (eState)
                            {
                                case month:
                                    // Month must be followed by separator and 
                                    // day, no trailing blanks.
                                    if (*p != '-' || (p+1 == pStop))
                                        SetError( errNoValue);
                                    break;
                                case day:
                                    if ((*p != 'T' || (p+1 == pStop)) && *p != ' ')
                                        SetError( errNoValue);
                                    // Take one blank as a valid delimiter 
                                    // between date and time.
                                    break;
                                case hour:
                                    // Hour must be followed by separator and 
                                    // minute, no trailing blanks.
                                    if (*p != ':' || (p+1 == pStop))
                                        SetError( errNoValue);
                                    break;
                                case minute:
                                    if ((*p != ':' || (p+1 == pStop)) && *p != ' ')
                                        SetError( errNoValue);
                                    if (*p == ' ')
                                        eState = done;
                                    break;
                                case second:
                                    if (((*p != ',' && *p != '.') || (p+1 == pStop)) && *p != ' ')
                                        SetError( errNoValue);
                                    if (*p == ' ')
                                        eState = done;
                                    break;
                                case fraction:
                                    eState = done;
                                    break;
                                case year:
                                case done:
                                case blank:
                                case stop:
                                    SetError( errNoValue);
                                    break;
                            }
                            eState = static_cast<State>(eState + 1);
                        }
                        else
                            SetError( errNoValue);
                        ++p;
                    }
                    if (eState == blank)
                    {
                        while (p < pStop && *p == ' ')
                            ++p;
                        if (p < pStop)
                            SetError( errNoValue);
                        eState = stop;
                    }

                    // Month without day, or hour without minute.
                    if (eState == month || (eState == day && p <= pLastStart) ||
                            eState == hour || (eState == minute && p <= pLastStart))
                        SetError( errNoValue);

                    if (!nGlobalError)
                    {
                        // Catch the very last unit at end of string.
                        if (p > pLastStart && eState < done)
                        {
                            nUnit[eState] = aStr.copy( pLastStart - pStart, p - pLastStart).toInt32();
                            if (nLimit[eState] && nLimit[eState] < nUnit[eState])
                                SetError( errNoValue);
                        }
                        if (bDate && nUnit[hour] > 23)
                            SetError( errNoValue);
                        if (!nGlobalError)
                        {
                            if (bDate && nUnit[day] == 0)
                                nUnit[day] = 1;
                            double fFraction = (nUnit[fraction] <= 0 ? 0.0 : 
                                    ::rtl::math::pow10Exp( nUnit[fraction],
                                        static_cast<int>( -ceil( log10( static_cast<double>( nUnit[fraction]))))));
                            fValue = (bDate ? GetDateSerial( 
                                        sal::static_int_cast<INT16>(nUnit[year]), 
                                        sal::static_int_cast<INT16>(nUnit[month]), 
                                        sal::static_int_cast<INT16>(nUnit[day]), 
                                        true) : 0.0);
                            fValue += ((nUnit[hour] * 3600) + (nUnit[minute] * 60) + nUnit[second] + fFraction) / 86400.0;
                        }
                    }
                }
                break;
            default:
                SetError( errNoValue);
        }
        if (nGlobalError)
            fValue = 0.0;
    }
    return fValue;
}


double ScInterpreter::GetCellValue( const ScAddress& rPos, const ScBaseCell* pCell )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetCellValue" );
    USHORT nErr = nGlobalError;
    nGlobalError = 0;
    double nVal = GetCellValueOrZero( rPos, pCell );
    if ( !nGlobalError || nGlobalError == errCellNoValue )
        nGlobalError = nErr;
    return nVal;
}


double ScInterpreter::GetCellValueOrZero( const ScAddress& rPos, const ScBaseCell* pCell )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetCellValueOrZero" );
    double fValue = 0.0;
    if (pCell)
    {
        CellType eType = pCell->GetCellType();
        switch ( eType )
        {
            case CELLTYPE_FORMULA:
            {
                ScFormulaCell* pFCell = (ScFormulaCell*) pCell;
                USHORT nErr = pFCell->GetErrCode();
                if( !nErr )
                {
                    if (pFCell->IsValue())
                    {
                        fValue = pFCell->GetValue();
                        pDok->GetNumberFormatInfo( nCurFmtType, nCurFmtIndex,
                            rPos, pFCell );
                    }
                    else
                    {
                        String aStr;
                        pFCell->GetString( aStr );
                        fValue = ConvertStringToValue( aStr );
                    }
                }
                else
                {
                    fValue = 0.0;
                    SetError(nErr);
                }
            }
            break;
            case CELLTYPE_VALUE:
            {
                fValue = ((ScValueCell*)pCell)->GetValue();
                nCurFmtIndex = pDok->GetNumberFormat( rPos );
                nCurFmtType = pFormatter->GetType( nCurFmtIndex );
                if ( bCalcAsShown && fValue != 0.0 )
                    fValue = pDok->RoundValueAsShown( fValue, nCurFmtIndex );
            }
            break;
            case  CELLTYPE_STRING:
            case  CELLTYPE_EDIT:
            {
                // SUM(A1:A2) differs from A1+A2. No good. But people insist on 
                // it ... #i5658#
                String aStr;
                if ( eType == CELLTYPE_STRING )
                    ((ScStringCell*)pCell)->GetString( aStr );
                else
                    ((ScEditCell*)pCell)->GetString( aStr );
                fValue = ConvertStringToValue( aStr );
            }
            break;
            case CELLTYPE_NONE:
            case CELLTYPE_NOTE:
                fValue = 0.0;       // empty or broadcaster cell
            break;
            case CELLTYPE_SYMBOLS:
#if DBG_UTIL
            case CELLTYPE_DESTROYED:
#endif
                SetError(errCellNoValue);
                fValue = 0.0;
            break;
        }
    }
    else
        fValue = 0.0;
    return fValue;
}


void ScInterpreter::GetCellString( String& rStr, const ScBaseCell* pCell )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetCellString" );
    USHORT nErr = 0;
    if (pCell)
    {
        switch (pCell->GetCellType())
        {
            case CELLTYPE_STRING:
                ((ScStringCell*) pCell)->GetString(rStr);
            break;
            case CELLTYPE_EDIT:
                ((ScEditCell*) pCell)->GetString(rStr);
            break;
            case CELLTYPE_FORMULA:
            {
                ScFormulaCell* pFCell = (ScFormulaCell*) pCell;
                nErr = pFCell->GetErrCode();
                if (pFCell->IsValue())
                {
                    double fVal = pFCell->GetValue();
                    ULONG nIndex = pFormatter->GetStandardFormat(
                                        NUMBERFORMAT_NUMBER,
                                        ScGlobal::eLnge);
                    pFormatter->GetInputLineString(fVal, nIndex, rStr);
                }
                else
                    pFCell->GetString(rStr);
            }
            break;
            case CELLTYPE_VALUE:
            {
                double fVal = ((ScValueCell*) pCell)->GetValue();
                ULONG nIndex = pFormatter->GetStandardFormat(
                                        NUMBERFORMAT_NUMBER,
                                        ScGlobal::eLnge);
                pFormatter->GetInputLineString(fVal, nIndex, rStr);
            }
            break;
            default:
                rStr = ScGlobal::GetEmptyString();
            break;
        }
    }
    else
        rStr = ScGlobal::GetEmptyString();
    SetError(nErr);
}


BOOL ScInterpreter::CreateDoubleArr(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                            SCCOL nCol2, SCROW nRow2, SCTAB nTab2, BYTE* pCellArr)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::CreateDoubleArr" );
#if SC_ROWLIMIT_MORE_THAN_64K
#error row limit 64k
#endif
    USHORT nCount = 0;
    USHORT* p = (USHORT*) pCellArr;
    *p++ = static_cast<USHORT>(nCol1);
    *p++ = static_cast<USHORT>(nRow1);
    *p++ = static_cast<USHORT>(nTab1);
    *p++ = static_cast<USHORT>(nCol2);
    *p++ = static_cast<USHORT>(nRow2);
    *p++ = static_cast<USHORT>(nTab2);
    USHORT* pCount = p;
    *p++ = 0;
    USHORT nPos = 14;
    SCTAB nTab = nTab1;
    ScAddress aAdr;
    while (nTab <= nTab2)
    {
        aAdr.SetTab( nTab );
        SCROW nRow = nRow1;
        while (nRow <= nRow2)
        {
            aAdr.SetRow( nRow );
            SCCOL nCol = nCol1;
            while (nCol <= nCol2)
            {
                aAdr.SetCol( nCol );
                ScBaseCell* pCell = pDok->GetCell( aAdr );
                if (pCell)
                {
                    USHORT  nErr = 0;
                    double  nVal = 0.0;
                    BOOL    bOk = TRUE;
                    switch ( pCell->GetCellType() )
                    {
                        case CELLTYPE_VALUE :
                            nVal = GetValueCellValue( aAdr, (ScValueCell*)pCell );
                            break;
                        case CELLTYPE_FORMULA :
                            if (((ScFormulaCell*)pCell)->IsValue())
                            {
                                nErr = ((ScFormulaCell*)pCell)->GetErrCode();
                                nVal = ((ScFormulaCell*)pCell)->GetValue();
                            }
                            else
                                bOk = FALSE;
                            break;
                        default :
                            bOk = FALSE;
                            break;
                    }
                    if (bOk)
                    {
                        if ((nPos + (4 * sizeof(USHORT)) + sizeof(double)) > MAXARRSIZE)
                            return FALSE;
                        *p++ = static_cast<USHORT>(nCol);
                        *p++ = static_cast<USHORT>(nRow);
                        *p++ = static_cast<USHORT>(nTab);
                        *p++ = nErr;
                        memcpy( p, &nVal, sizeof(double));
                        nPos += 8 + sizeof(double);
                        p = (USHORT*) ( pCellArr + nPos );
                        nCount++;
                    }
                }
                nCol++;
            }
            nRow++;
        }
        nTab++;
    }
    *pCount = nCount;
    return TRUE;
}


BOOL ScInterpreter::CreateStringArr(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                                    SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
                                    BYTE* pCellArr)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::CreateStringArr" );
#if SC_ROWLIMIT_MORE_THAN_64K
#error row limit 64k
#endif
    USHORT nCount = 0;
    USHORT* p = (USHORT*) pCellArr;
    *p++ = static_cast<USHORT>(nCol1);
    *p++ = static_cast<USHORT>(nRow1);
    *p++ = static_cast<USHORT>(nTab1);
    *p++ = static_cast<USHORT>(nCol2);
    *p++ = static_cast<USHORT>(nRow2);
    *p++ = static_cast<USHORT>(nTab2);
    USHORT* pCount = p;
    *p++ = 0;
    USHORT nPos = 14;
    SCTAB nTab = nTab1;
    while (nTab <= nTab2)
    {
        SCROW nRow = nRow1;
        while (nRow <= nRow2)
        {
            SCCOL nCol = nCol1;
            while (nCol <= nCol2)
            {
                ScBaseCell* pCell;
                pDok->GetCell(nCol, nRow, nTab, pCell);
                if (pCell)
                {
                    String  aStr;
                    USHORT  nErr = 0;
                    BOOL    bOk = TRUE;
                    switch ( pCell->GetCellType() )
                    {
                        case CELLTYPE_STRING :
                            ((ScStringCell*)pCell)->GetString(aStr);
                            break;
                        case CELLTYPE_EDIT :
                            ((ScEditCell*)pCell)->GetString(aStr);
                            break;
                        case CELLTYPE_FORMULA :
                            if (!((ScFormulaCell*)pCell)->IsValue())
                            {
                                nErr = ((ScFormulaCell*)pCell)->GetErrCode();
                                ((ScFormulaCell*)pCell)->GetString(aStr);
                            }
                            else
                                bOk = FALSE;
                            break;
                        default :
                            bOk = FALSE;
                            break;
                    }
                    if (bOk)
                    {
                        ByteString aTmp( aStr, osl_getThreadTextEncoding() );
                        // In case the xub_StrLen will be longer than USHORT
                        // one day, and room for pad byte check.
                        if ( aTmp.Len() > ((USHORT)(~0)) - 2 )
                            return FALSE;
                        // Append a 0-pad-byte if string length is not even
                        //! MUST be USHORT and not xub_StrLen
                        USHORT nStrLen = (USHORT) aTmp.Len();
                        USHORT nLen = ( nStrLen + 2 ) & ~1;

                        if (((ULONG)nPos + (5 * sizeof(USHORT)) + nLen) > MAXARRSIZE)
                            return FALSE;
                        *p++ = static_cast<USHORT>(nCol);
                        *p++ = static_cast<USHORT>(nRow);
                        *p++ = static_cast<USHORT>(nTab);
                        *p++ = nErr;
                        *p++ = nLen;
                        memcpy( p, aTmp.GetBuffer(), nStrLen + 1);
                        nPos += 10 + nStrLen + 1;
                        BYTE* q = ( pCellArr + nPos );
                        if( !nStrLen & 1 )
                            *q++ = 0, nPos++;
                        p = (USHORT*) ( pCellArr + nPos );
                        nCount++;
                    }
                }
                nCol++;
            }
            nRow++;
        }
        nTab++;
    }
    *pCount = nCount;
    return TRUE;
}


BOOL ScInterpreter::CreateCellArr(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                                  SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
                                  BYTE* pCellArr)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::CreateCellArr" );
#if SC_ROWLIMIT_MORE_THAN_64K
#error row limit 64k
#endif
    USHORT nCount = 0;
    USHORT* p = (USHORT*) pCellArr;
    *p++ = static_cast<USHORT>(nCol1);
    *p++ = static_cast<USHORT>(nRow1);
    *p++ = static_cast<USHORT>(nTab1);
    *p++ = static_cast<USHORT>(nCol2);
    *p++ = static_cast<USHORT>(nRow2);
    *p++ = static_cast<USHORT>(nTab2);
    USHORT* pCount = p;
    *p++ = 0;
    USHORT nPos = 14;
    SCTAB nTab = nTab1;
    ScAddress aAdr;
    while (nTab <= nTab2)
    {
        aAdr.SetTab( nTab );
        SCROW nRow = nRow1;
        while (nRow <= nRow2)
        {
            aAdr.SetRow( nRow );
            SCCOL nCol = nCol1;
            while (nCol <= nCol2)
            {
                aAdr.SetCol( nCol );
                ScBaseCell* pCell = pDok->GetCell( aAdr );
                if (pCell)
                {
                    USHORT  nErr = 0;
                    USHORT  nType = 0; // 0 = Zahl; 1 = String
                    double  nVal = 0.0;
                    String  aStr;
                    BOOL    bOk = TRUE;
                    switch ( pCell->GetCellType() )
                    {
                        case CELLTYPE_STRING :
                            ((ScStringCell*)pCell)->GetString(aStr);
                            nType = 1;
                            break;
                        case CELLTYPE_EDIT :
                            ((ScEditCell*)pCell)->GetString(aStr);
                            nType = 1;
                            break;
                        case CELLTYPE_VALUE :
                            nVal = GetValueCellValue( aAdr, (ScValueCell*)pCell );
                            break;
                        case CELLTYPE_FORMULA :
                            nErr = ((ScFormulaCell*)pCell)->GetErrCode();
                            if (((ScFormulaCell*)pCell)->IsValue())
                                nVal = ((ScFormulaCell*)pCell)->GetValue();
                            else
                                ((ScFormulaCell*)pCell)->GetString(aStr);
                            break;
                        default :
                            bOk = FALSE;
                            break;
                    }
                    if (bOk)
                    {
                        if ((nPos + (5 * sizeof(USHORT))) > MAXARRSIZE)
                            return FALSE;
                        *p++ = static_cast<USHORT>(nCol);
                        *p++ = static_cast<USHORT>(nRow);
                        *p++ = static_cast<USHORT>(nTab);
                        *p++ = nErr;
                        *p++ = nType;
                        nPos += 10;
                        if (nType == 0)
                        {
                            if ((nPos + sizeof(double)) > MAXARRSIZE)
                                return FALSE;
                            memcpy( p, &nVal, sizeof(double));
                            nPos += sizeof(double);
                        }
                        else
                        {
                            ByteString aTmp( aStr, osl_getThreadTextEncoding() );
                            // In case the xub_StrLen will be longer than USHORT
                            // one day, and room for pad byte check.
                            if ( aTmp.Len() > ((USHORT)(~0)) - 2 )
                                return FALSE;
                            // Append a 0-pad-byte if string length is not even
                            //! MUST be USHORT and not xub_StrLen
                            USHORT nStrLen = (USHORT) aTmp.Len();
                            USHORT nLen = ( nStrLen + 2 ) & ~1;
                            if ( ((ULONG)nPos + 2 + nLen) > MAXARRSIZE)
                                return FALSE;
                            *p++ = nLen;
                            memcpy( p, aTmp.GetBuffer(), nStrLen + 1);
                            nPos += 2 + nStrLen + 1;
                            BYTE* q = ( pCellArr + nPos );
                            if( !nStrLen & 1 )
                                *q++ = 0, nPos++;
                        }
                        nCount++;
                        p = (USHORT*) ( pCellArr + nPos );
                    }
                }
                nCol++;
            }
            nRow++;
        }
        nTab++;
    }
    *pCount = nCount;
    return TRUE;
}


//-----------------------------------------------------------------------------
// Stack operations
//-----------------------------------------------------------------------------


// Also releases a TempToken if appropriate.

void ScInterpreter::PushWithoutError( FormulaToken& r )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushWithoutError" );
    if ( sp >= MAXSTACK )
        SetError( errStackOverflow );
    else
    {
        nCurFmtType = NUMBERFORMAT_UNDEFINED;
        r.IncRef();
        if( sp >= maxsp )
            maxsp = sp + 1;
        else
            pStack[ sp ]->DecRef();
        pStack[ sp ] = (ScToken*) &r;
        ++sp;
    }
}

void ScInterpreter::Push( FormulaToken& r )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::Push" );
    if ( sp >= MAXSTACK )
        SetError( errStackOverflow );
    else
    {
        if (nGlobalError)
        {
            if (r.GetType() == svError)
            {
                r.SetError( nGlobalError);
                PushWithoutError( r);
            }
            else
                PushWithoutError( *(new FormulaErrorToken( nGlobalError)));
        }
        else
            PushWithoutError( r);
    }
}


void ScInterpreter::PushTempToken( FormulaToken* p )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushTempToken" );
    if ( sp >= MAXSTACK )
    {
        SetError( errStackOverflow );
        if (!p->GetRef())
            //! p is a dangling pointer hereafter!
            p->Delete();
    }
    else
    {
        if (nGlobalError)
        {
            if (p->GetType() == svError)
            {
                p->SetError( nGlobalError);
                PushTempTokenWithoutError( p);
            }
            else
            {
                if (!p->GetRef())
                    //! p is a dangling pointer hereafter!
                    p->Delete();
                PushTempTokenWithoutError( new FormulaErrorToken( nGlobalError));
            }
        }
        else
            PushTempTokenWithoutError( p);
    }
}


void ScInterpreter::PushTempTokenWithoutError( FormulaToken* p )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushTempTokenWithoutError" );
    p->IncRef();
    if ( sp >= MAXSTACK )
    {
        SetError( errStackOverflow );
        //! p may be a dangling pointer hereafter!
        p->DecRef();
    }
    else
    {
        if( sp >= maxsp )
            maxsp = sp + 1;
        else
            pStack[ sp ]->DecRef();
        pStack[ sp ] = p;
        ++sp;
    }
}


void ScInterpreter::PushTempToken( const FormulaToken& r )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushTempToken" );
    if (!IfErrorPushError())
        PushTempTokenWithoutError( r.Clone());
}


void ScInterpreter::PushCellResultToken( bool bDisplayEmptyAsString,
        const ScAddress & rAddress, short * pRetTypeExpr, ULONG * pRetIndexExpr )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushCellResultToken" );
    ScBaseCell* pCell = pDok->GetCell( rAddress);
    if (!pCell || pCell->HasEmptyData())
    {
        if (pRetTypeExpr && pRetIndexExpr)
            pDok->GetNumberFormatInfo( *pRetTypeExpr, *pRetIndexExpr, rAddress, pCell);
        bool bInherited = (GetCellType( pCell) == CELLTYPE_FORMULA);
        PushTempToken( new ScEmptyCellToken( bInherited, bDisplayEmptyAsString));
        return;
    }
    USHORT nErr;
    if ((nErr = pCell->GetErrorCode()) != 0)
    {
        PushError( nErr);
        if (pRetTypeExpr)
            *pRetTypeExpr = NUMBERFORMAT_UNDEFINED;
        if (pRetIndexExpr)
            *pRetIndexExpr = 0;
    }
    else if (pCell->HasStringData())
    {
        String aRes;
        GetCellString( aRes, pCell);
        PushString( aRes);
        if (pRetTypeExpr)
            *pRetTypeExpr = NUMBERFORMAT_TEXT;
        if (pRetIndexExpr)
            *pRetIndexExpr = 0;
    }
    else
    {
        double fVal = GetCellValue( rAddress, pCell);
        PushDouble( fVal);
        if (pRetTypeExpr)
            *pRetTypeExpr = nCurFmtType;
        if (pRetIndexExpr)
            *pRetIndexExpr = nCurFmtIndex;
    }
}


// Simply throw away TOS.

void ScInterpreter::Pop()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::Pop" );
    if( sp )
        sp--;
    else
        SetError(errUnknownStackVariable);
}


// Simply throw away TOS and set error code, used with ocIsError et al.

void ScInterpreter::PopError()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopError" );
    if( sp )
    {
        sp--;
        if (pStack[sp]->GetType() == svError)
            nGlobalError = pStack[sp]->GetError();
    }
    else
        SetError(errUnknownStackVariable);
}


FormulaTokenRef ScInterpreter::PopToken()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopToken" );
    if (sp)
    {
        sp--;
        FormulaToken* p = pStack[ sp ];
        if (p->GetType() == svError)
            nGlobalError = p->GetError();
        return p;
    }
    else
        SetError(errUnknownStackVariable);
    return NULL;
}


double ScInterpreter::PopDouble()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopDouble" );
    nCurFmtType = NUMBERFORMAT_NUMBER;
    nCurFmtIndex = 0;
    if( sp )
    {
        --sp;
        FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svDouble:
                return p->GetDouble();
            case svEmptyCell:
            case svMissing:
                return 0.0;
            default:
                SetError( errIllegalArgument);
        }
    }
    else
        SetError( errUnknownStackVariable);
    return 0.0;
}


const String& ScInterpreter::PopString()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopString" );
    nCurFmtType = NUMBERFORMAT_TEXT;
    nCurFmtIndex = 0;
    if( sp )
    {
        --sp;
        FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svString:
                return p->GetString();
            case svEmptyCell:
            case svMissing:
                return EMPTY_STRING;
            default:
                SetError( errIllegalArgument);
        }
    }
    else
        SetError( errUnknownStackVariable);
    return EMPTY_STRING;
}


void ScInterpreter::ValidateRef( const ScSingleRefData & rRef )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ValidateRef" );
    SCCOL nCol;
    SCROW nRow;
    SCTAB nTab;
    SingleRefToVars( rRef, nCol, nRow, nTab);
}


void ScInterpreter::ValidateRef( const ScComplexRefData & rRef )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ValidateRef" );
    ValidateRef( rRef.Ref1);
    ValidateRef( rRef.Ref2);
}


void ScInterpreter::ValidateRef( const ScRefList & rRefList )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ValidateRef" );
    ScRefList::const_iterator it( rRefList.begin());
    ScRefList::const_iterator end( rRefList.end());
    for ( ; it != end; ++it)
    {
        ValidateRef( *it);
    }
}


void ScInterpreter::SingleRefToVars( const ScSingleRefData & rRef,
        SCCOL & rCol, SCROW & rRow, SCTAB & rTab )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::SingleRefToVars" );
    if ( rRef.IsColRel() )
        rCol = aPos.Col() + rRef.nRelCol;
    else
        rCol = rRef.nCol;
    if ( rRef.IsRowRel() )
        rRow = aPos.Row() + rRef.nRelRow;
    else
        rRow = rRef.nRow;
    if ( rRef.IsTabRel() )
        rTab = aPos.Tab() + rRef.nRelTab;
    else
        rTab = rRef.nTab;
    if( !ValidCol( rCol) || rRef.IsColDeleted() )
        SetError( errNoRef ), rCol = 0;
    if( !ValidRow( rRow) || rRef.IsRowDeleted() )
        SetError( errNoRef ), rRow = 0;
    if( !ValidTab( rTab, pDok->GetTableCount() - 1) || rRef.IsTabDeleted() )
        SetError( errNoRef ), rTab = 0;
}


void ScInterpreter::PopSingleRef(SCCOL& rCol, SCROW &rRow, SCTAB& rTab)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopSingleRef" );
    if( sp )
    {
        --sp;
        FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svSingleRef:
                SingleRefToVars( static_cast<ScToken*>(p)->GetSingleRef(), rCol, rRow, rTab);
                if ( pDok->aTableOpList.Count() > 0 )
                    ReplaceCell( rCol, rRow, rTab );
                break;
            default:
                SetError( errIllegalParameter);
        }
    }
    else
        SetError( errUnknownStackVariable);
}


void ScInterpreter::PopSingleRef( ScAddress& rAdr )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopSingleRef" );
    if( sp )
    {
        --sp;
        FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svSingleRef:
                {
                    SCCOL nCol;
                    SCROW nRow;
                    SCTAB nTab;
                    SingleRefToVars( static_cast<ScToken*>(p)->GetSingleRef(), nCol, nRow, nTab);
                    rAdr.Set( nCol, nRow, nTab );
                    if ( pDok->aTableOpList.Count() > 0 )
                        ReplaceCell( rAdr );
                }
                break;
            default:
                SetError( errIllegalParameter);
        }
    }
    else
        SetError( errUnknownStackVariable);
}


void ScInterpreter::DoubleRefToVars( const ScToken* p,
        SCCOL& rCol1, SCROW &rRow1, SCTAB& rTab1,
        SCCOL& rCol2, SCROW &rRow2, SCTAB& rTab2,
        BOOL bDontCheckForTableOp )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::DoubleRefToVars" );
    const ScComplexRefData& rCRef = p->GetDoubleRef();
    SingleRefToVars( rCRef.Ref1, rCol1, rRow1, rTab1);
    SingleRefToVars( rCRef.Ref2, rCol2, rRow2, rTab2);
    if ( pDok->aTableOpList.Count() > 0 && !bDontCheckForTableOp )
    {
        ScRange aRange( rCol1, rRow1, rTab1, rCol2, rRow2, rTab2 );
        if ( IsTableOpInRange( aRange ) )
            SetError( errIllegalParameter );
    }
}


void ScInterpreter::PopDoubleRef(SCCOL& rCol1, SCROW &rRow1, SCTAB& rTab1,
                                 SCCOL& rCol2, SCROW &rRow2, SCTAB& rTab2,
                                 BOOL bDontCheckForTableOp )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopDoubleRef" );
    if( sp )
    {
        --sp;
        FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svDoubleRef:
                DoubleRefToVars( static_cast<ScToken*>(p), rCol1, rRow1, rTab1, rCol2, rRow2, rTab2,
                        bDontCheckForTableOp);
                break;
            default:
                SetError( errIllegalParameter);
        }
    }
    else
        SetError( errUnknownStackVariable);
}


void ScInterpreter::DoubleRefToRange( const ScComplexRefData & rCRef,
        ScRange & rRange, BOOL bDontCheckForTableOp )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::DoubleRefToRange" );
    SCCOL nCol;
    SCROW nRow;
    SCTAB nTab;
    SingleRefToVars( rCRef.Ref1, nCol, nRow, nTab);
    rRange.aStart.Set( nCol, nRow, nTab );
    SingleRefToVars( rCRef.Ref2, nCol, nRow, nTab);
    rRange.aEnd.Set( nCol, nRow, nTab );
    if ( pDok->aTableOpList.Count() > 0 && !bDontCheckForTableOp )
    {
        if ( IsTableOpInRange( rRange ) )
            SetError( errIllegalParameter );
    }
}


void ScInterpreter::PopDoubleRef( ScRange & rRange, short & rParam, size_t & rRefInList )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopDoubleRef" );
    if (sp)
    {
        formula::FormulaToken* pToken = pStack[ sp-1 ];
        ScToken* p = static_cast<ScToken*>(pToken);
        switch (pToken->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svDoubleRef:
                --sp;
                DoubleRefToRange( p->GetDoubleRef(), rRange);
                break;
            case svRefList:
                {
                    const ScRefList* pList = p->GetRefList();
                    if (rRefInList < pList->size())
                    {
                        DoubleRefToRange( (*pList)[rRefInList], rRange);
                        if (++rRefInList < pList->size())
                            ++rParam;
                        else
                        {
                            --sp;
                            rRefInList = 0;
                        }
                    }
                    else
                    {
                        --sp;
                        rRefInList = 0;
                        SetError( errIllegalParameter);
                    }
                }
                break;
            default:
                SetError( errIllegalParameter);
        }
    }
    else
        SetError( errUnknownStackVariable);
}


void ScInterpreter::PopDoubleRef( ScRange& rRange, BOOL bDontCheckForTableOp )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopDoubleRef" );
    if( sp )
    {
        --sp;
        FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svDoubleRef:
                DoubleRefToRange( static_cast<ScToken*>(p)->GetDoubleRef(), rRange, bDontCheckForTableOp);
                break;
            default:
                SetError( errIllegalParameter);
        }
    }
    else
        SetError( errUnknownStackVariable);
}


BOOL ScInterpreter::PopDoubleRefOrSingleRef( ScAddress& rAdr )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopDoubleRefOrSingleRef" );
    switch ( GetStackType() )
    {
        case svDoubleRef :
        {
            ScRange aRange;
            PopDoubleRef( aRange, TRUE );
            return DoubleRefToPosSingleRef( aRange, rAdr );
        }
        //break;
        case svSingleRef :
        {
            PopSingleRef( rAdr );
            return TRUE;
        }
        //break;
        default:
            PopError();
            SetError( errNoRef );
    }
    return FALSE;
}


void ScInterpreter::PopDoubleRefPushMatrix()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopDoubleRefPushMatrix" );
    if ( GetStackType() == svDoubleRef )
    {
        ScMatrixRef pMat = GetMatrix();
        if ( pMat )
            PushMatrix( pMat );
        else
            PushIllegalParameter();
    }
    else
        SetError( errNoRef );
}


ScTokenMatrixMap* ScInterpreter::CreateTokenMatrixMap()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::CreateTokenMatrixMap" );
    return new ScTokenMatrixMap;
}


bool ScInterpreter::ConvertMatrixParameters()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ConvertMatrixParameters" );
    USHORT nParams = pCur->GetParamCount();
    DBG_ASSERT( nParams <= sp, "ConvertMatrixParameters: stack/param count mismatch");
    SCSIZE nJumpCols = 0, nJumpRows = 0;
    for ( USHORT i=1; i <= nParams && i <= sp; ++i )
    {
        FormulaToken* p = pStack[ sp - i ];
        if ( p->GetOpCode() != ocPush )
        {
            DBG_ERRORFILE( "ConvertMatrixParameters: not a push");
        }
        else
        {
            switch ( p->GetType() )
            {
                case svDouble:
                case svString:
                case svSingleRef:
                case svMissing:
                case svError:
                case svEmptyCell:
                    // nothing to do
                break;
                case svMatrix:
                {
                    if ( ScParameterClassification::GetParameterType( pCur, nParams - i)
                            == ScParameterClassification::Value )
                    {   // only if single value expected
                        ScMatrixRef pMat = static_cast<ScToken*>(p)->GetMatrix();
                        if ( !pMat )
                            SetError( errUnknownVariable);
                        else
                        {
                            SCSIZE nCols, nRows;
                            pMat->GetDimensions( nCols, nRows);
                            if ( nJumpCols < nCols )
                                nJumpCols = nCols;
                            if ( nJumpRows < nRows )
                                nJumpRows = nRows;
                        }
                    }
                }
                break;
                case svDoubleRef:
                {
                    ScParameterClassification::Type eType =
                        ScParameterClassification::GetParameterType( pCur, nParams - i);
                    if ( eType != ScParameterClassification::Reference &&
                            eType != ScParameterClassification::ReferenceOrForceArray)
                    {
                        SCCOL nCol1, nCol2;
                        SCROW nRow1, nRow2;
                        SCTAB nTab1, nTab2;
                        DoubleRefToVars( static_cast<const ScToken*>( p), nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                        // Make sure the map exists, created if not.
                        GetTokenMatrixMap();
                        ScMatrixRef pMat = CreateMatrixFromDoubleRef( p,
                                nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                        if (pMat)
                        {
                            if ( eType == ScParameterClassification::Value )
                            {   // only if single value expected
                                if ( nJumpCols < static_cast<SCSIZE>(nCol2 - nCol1 + 1) )
                                    nJumpCols = static_cast<SCSIZE>(nCol2 - nCol1 + 1);
                                if ( nJumpRows < static_cast<SCSIZE>(nRow2 - nRow1 + 1) )
                                    nJumpRows = static_cast<SCSIZE>(nRow2 - nRow1 + 1);
                            }
                            ScToken* pNew = new ScMatrixToken( pMat);
                            pNew->IncRef();
                            pStack[ sp - i ] = pNew;
                            p->DecRef();    // p may be dead now!
                        }
                    }
                }
                break;
                case svRefList:
                {
                    ScParameterClassification::Type eType =
                        ScParameterClassification::GetParameterType( pCur, nParams - i);
                    if ( eType != ScParameterClassification::Reference &&
                            eType != ScParameterClassification::ReferenceOrForceArray)
                    {
                        // can't convert to matrix
                        SetError( errNoValue);
                    }
                }
                break;
                default:
                    DBG_ERRORFILE( "ConvertMatrixParameters: unknown parameter type");
            }
        }
    }
    if( nJumpCols && nJumpRows )
    {
        short nPC = aCode.GetPC();
        short nStart = nPC - 1;     // restart on current code (-1)
        short nNext = nPC;          // next instruction after subroutine
        short nStop = nPC + 1;      // stop subroutine before reaching that
        FormulaTokenRef xNew;
        ScTokenMatrixMap::const_iterator aMapIter;
        if (pTokenMatrixMap && ((aMapIter = pTokenMatrixMap->find( pCur)) !=
                    pTokenMatrixMap->end()))
            xNew = (*aMapIter).second;
        else
        {
            ScJumpMatrix* pJumpMat = new ScJumpMatrix( nJumpCols, nJumpRows);
            pJumpMat->SetAllJumps( 1.0, nStart, nNext, nStop);
            // pop parameters and store in ScJumpMatrix, push in JumpMatrix()
            ScTokenVec* pParams = new ScTokenVec( nParams);
            for ( USHORT i=1; i <= nParams && sp > 0; ++i )
            {
                FormulaToken* p = pStack[ --sp ];
                p->IncRef();
                // store in reverse order such that a push may simply iterate
                (*pParams)[ nParams - i ] = p;
            }
            pJumpMat->SetJumpParameters( pParams);
            xNew = new ScJumpMatrixToken( pJumpMat );
            GetTokenMatrixMap().insert( ScTokenMatrixMap::value_type( pCur,
                        xNew));
        }
        PushTempToken( xNew);
        // set continuation point of path for main code line
        aCode.Jump( nNext, nNext);
        return true;
    }
    return false;
}


ScMatrixRef ScInterpreter::PopMatrix()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PopMatrix" );
    if( sp )
    {
        --sp;
        FormulaToken* p = pStack[ sp ];
        switch (p->GetType())
        {
            case svError:
                nGlobalError = p->GetError();
                break;
            case svMatrix:
                {
                    ScMatrix* pMat = static_cast<ScToken*>(p)->GetMatrix();
                    if ( pMat )
                        pMat->SetErrorInterpreter( this);
                    else
                        SetError( errUnknownVariable);
                    return pMat;
                }
            default:
                SetError( errIllegalParameter);
        }
    }
    else
        SetError( errUnknownStackVariable);
    return NULL;
}


void ScInterpreter::PushDouble(double nVal)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushDouble" );
    TreatDoubleError( nVal );
    if (!IfErrorPushError())
        PushTempTokenWithoutError( new FormulaDoubleToken( nVal ) );
}


void ScInterpreter::PushInt(int nVal)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushInt" );
    if (!IfErrorPushError())
        PushTempTokenWithoutError( new FormulaDoubleToken( nVal ) );
}


void ScInterpreter::PushStringBuffer( const sal_Unicode* pString )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushStringBuffer" );
    if ( pString )
        PushString( String( pString ) );
    else
        PushString( EMPTY_STRING );
}


void ScInterpreter::PushString( const String& rString )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushString" );
    if (!IfErrorPushError())
        PushTempTokenWithoutError( new FormulaStringToken( rString ) );
}


void ScInterpreter::PushSingleRef(SCCOL nCol, SCROW nRow, SCTAB nTab)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushSingleRef" );
    if (!IfErrorPushError())
    {
        ScSingleRefData aRef;
        aRef.InitFlags();
        aRef.nCol = nCol;
        aRef.nRow = nRow;
        aRef.nTab = nTab;
        PushTempTokenWithoutError( new ScSingleRefToken( aRef ) );
    }
}


void ScInterpreter::PushDoubleRef(SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
                                  SCCOL nCol2, SCROW nRow2, SCTAB nTab2)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushDoubleRef" );
    if (!IfErrorPushError())
    {
        ScComplexRefData aRef;
        aRef.InitFlags();
        aRef.Ref1.nCol = nCol1;
        aRef.Ref1.nRow = nRow1;
        aRef.Ref1.nTab = nTab1;
        aRef.Ref2.nCol = nCol2;
        aRef.Ref2.nRow = nRow2;
        aRef.Ref2.nTab = nTab2;
        PushTempTokenWithoutError( new ScDoubleRefToken( aRef ) );
    }
}


void ScInterpreter::PushMatrix(ScMatrix* pMat)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushMatrix" );
    pMat->SetErrorInterpreter( NULL);
    // No   if (!IfErrorPushError())   because ScMatrix stores errors itself,
    // but with notifying ScInterpreter via nGlobalError, substituting it would
    // mean to inherit the error on all array elements in all following
    // operations.
    nGlobalError = 0;
    PushTempTokenWithoutError( new ScMatrixToken( pMat ) );
}


void ScInterpreter::PushError( USHORT nError )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushError" );
    SetError( nError );     // only sets error if not already set
    PushTempTokenWithoutError( new FormulaErrorToken( nGlobalError));
}

void ScInterpreter::PushParameterExpected()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushParameterExpected" );
    PushError( errParameterExpected);
}


void ScInterpreter::PushIllegalParameter()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushIllegalParameter" );
    PushError( errIllegalParameter);
}


void ScInterpreter::PushIllegalArgument()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushIllegalArgument" );
    PushError( errIllegalArgument);
}


void ScInterpreter::PushNA()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushNA" );
    PushError( NOTAVAILABLE);
}


void ScInterpreter::PushNoValue()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::PushNoValue" );
    PushError( errNoValue);
}


BOOL ScInterpreter::IsMissing()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::IsMissing" );
    return sp && pStack[sp - 1]->GetType() == svMissing;
}


StackVar ScInterpreter::GetRawStackType()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetRawStackType" );
    StackVar eRes;
    if( sp )
    {
        eRes = pStack[sp - 1]->GetType();
    }
    else
    {
        SetError(errUnknownStackVariable);
        eRes = svUnknown;
    }
    return eRes;
}


StackVar ScInterpreter::GetStackType()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetStackType" );
    StackVar eRes;
    if( sp )
    {
        eRes = pStack[sp - 1]->GetType();
        if( eRes == svMissing || eRes == svEmptyCell )
            eRes = svDouble;    // default!
    }
    else
    {
        SetError(errUnknownStackVariable);
        eRes = svUnknown;
    }
    return eRes;
}


StackVar ScInterpreter::GetStackType( BYTE nParam )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetStackType" );
    StackVar eRes;
    if( sp > nParam-1 )
    {
        eRes = pStack[sp - nParam]->GetType();
        if( eRes == svMissing || eRes == svEmptyCell )
            eRes = svDouble;    // default!
    }
    else
        eRes = svUnknown;
    return eRes;
}


BOOL ScInterpreter::DoubleRefToPosSingleRef( const ScRange& rRange, ScAddress& rAdr )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::DoubleRefToPosSingleRef" );
    // Check for a singleton first - no implicit intersection for them.
    if( rRange.aStart == rRange.aEnd )
    {
        rAdr = rRange.aStart;
        return TRUE;
    }

    BOOL bOk = FALSE;

    if ( pJumpMatrix )
    {
        bOk = rRange.aStart.Tab() == rRange.aEnd.Tab();
        if ( !bOk )
            SetError( errIllegalArgument);
        else
        {
            SCSIZE nC, nR;
            pJumpMatrix->GetPos( nC, nR);
            rAdr.SetCol( sal::static_int_cast<SCCOL>( rRange.aStart.Col() + nC ) );
            rAdr.SetRow( sal::static_int_cast<SCROW>( rRange.aStart.Row() + nR ) );
            rAdr.SetTab( rRange.aStart.Tab());
            bOk = rRange.aStart.Col() <= rAdr.Col() && rAdr.Col() <=
                rRange.aEnd.Col() && rRange.aStart.Row() <= rAdr.Row() &&
                rAdr.Row() <= rRange.aEnd.Row();
            if ( !bOk )
                SetError( errNoValue);
        }
        return bOk;
    }

    SCCOL nMyCol = aPos.Col();
    SCROW nMyRow = aPos.Row();
    SCTAB nMyTab = aPos.Tab();
    SCCOL nCol = 0;
    SCROW nRow = 0;
    SCTAB nTab;
    nTab = rRange.aStart.Tab();
    if ( rRange.aStart.Col() <= nMyCol && nMyCol <= rRange.aEnd.Col() )
    {
        nRow = rRange.aStart.Row();
        if ( nRow == rRange.aEnd.Row() )
        {
            bOk = TRUE;
            nCol = nMyCol;
        }
        else if ( nTab != nMyTab && nTab == rRange.aEnd.Tab()
                && rRange.aStart.Row() <= nMyRow && nMyRow <= rRange.aEnd.Row() )
        {
            bOk = TRUE;
            nCol = nMyCol;
            nRow = nMyRow;
        }
    }
    else if ( rRange.aStart.Row() <= nMyRow && nMyRow <= rRange.aEnd.Row() )
    {
        nCol = rRange.aStart.Col();
        if ( nCol == rRange.aEnd.Col() )
        {
            bOk = TRUE;
            nRow = nMyRow;
        }
        else if ( nTab != nMyTab && nTab == rRange.aEnd.Tab()
                && rRange.aStart.Col() <= nMyCol && nMyCol <= rRange.aEnd.Col() )
        {
            bOk = TRUE;
            nCol = nMyCol;
            nRow = nMyRow;
        }
    }
    if ( bOk )
    {
        if ( nTab == rRange.aEnd.Tab() )
            ;   // all done
        else if ( nTab <= nMyTab && nMyTab <= rRange.aEnd.Tab() )
            nTab = nMyTab;
        else
            bOk = FALSE;
        if ( bOk )
            rAdr.Set( nCol, nRow, nTab );
    }
    if ( !bOk )
        SetError( errNoValue );
    return bOk;
}


double ScInterpreter::GetDouble()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetDouble" );
    double nVal;
    switch( GetRawStackType() )
    {
        case svDouble:
            nVal = PopDouble();
        break;
        case svString:
            nVal = ConvertStringToValue( PopString());
        break;
        case svSingleRef:
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            ScBaseCell* pCell = GetCell( aAdr );
            nVal = GetCellValue( aAdr, pCell );
        }
        break;
        case svDoubleRef:
        {   // generate position dependent SingleRef
            ScRange aRange;
            PopDoubleRef( aRange );
            ScAddress aAdr;
            if ( !nGlobalError && DoubleRefToPosSingleRef( aRange, aAdr ) )
            {
                ScBaseCell* pCell = GetCell( aAdr );
                nVal = GetCellValue( aAdr, pCell );
            }
            else
                nVal = 0.0;
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                nVal = 0.0;
            else if ( !pJumpMatrix )
                nVal = pMat->GetDouble( 0 );
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                    nVal = pMat->GetDouble( nC, nR);
                else
                {
                    SetError( errNoValue);
                    nVal = 0.0;
                }
            }
        }
        break;
        case svError:
            PopError();
            nVal = 0.0;
        break;
        case svEmptyCell:
        case svMissing:
            Pop();
            nVal = 0.0;
        break;
        default:
            PopError();
            SetError( errIllegalParameter);
            nVal = 0.0;
    }
    if ( nFuncFmtType == nCurFmtType )
        nFuncFmtIndex = nCurFmtIndex;
    return nVal;
}


double ScInterpreter::GetDoubleWithDefault(double nDefault)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetDoubleWithDefault" );
    bool bMissing = IsMissing();
    double nResultVal = GetDouble();
    if ( bMissing )
        nResultVal = nDefault;
    return nResultVal;
}


const String& ScInterpreter::GetString()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetString" );
    switch (GetRawStackType())
    {
        case svError:
            PopError();
            return EMPTY_STRING;
        //break;
        case svMissing:
        case svEmptyCell:
            Pop();
            return EMPTY_STRING;
        //break;
        case svDouble:
        {
            double fVal = PopDouble();
            ULONG nIndex = pFormatter->GetStandardFormat(
                                    NUMBERFORMAT_NUMBER,
                                    ScGlobal::eLnge);
            pFormatter->GetInputLineString(fVal, nIndex, aTempStr);
            return aTempStr;
        }
        //break;
        case svString:
            return PopString();
        //break;
        case svSingleRef:
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            if (nGlobalError == 0)
            {
                ScBaseCell* pCell = GetCell( aAdr );
                GetCellString( aTempStr, pCell );
                return aTempStr;
            }
            else
                return EMPTY_STRING;
        }
        //break;
        case svDoubleRef:
        {   // generate position dependent SingleRef
            ScRange aRange;
            PopDoubleRef( aRange );
            ScAddress aAdr;
            if ( !nGlobalError && DoubleRefToPosSingleRef( aRange, aAdr ) )
            {
                ScBaseCell* pCell = GetCell( aAdr );
                GetCellString( aTempStr, pCell );
                return aTempStr;
            }
            else
                return EMPTY_STRING;
        }
        //break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
            {
                aTempStr = pMat->GetString( *pFormatter, 0, 0);
                return aTempStr;
            }
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                {
                    aTempStr = pMat->GetString( *pFormatter, nC, nR);
                    return aTempStr;
                }
                else
                    SetError( errNoValue);
            }
        }
        break;
        default:
            PopError();
            SetError( errIllegalArgument);
    }
    return EMPTY_STRING;
}



ScMatValType ScInterpreter::GetDoubleOrStringFromMatrix( double& rDouble,
        String& rString )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetDoubleOrStringFromMatrix" );
    ScMatValType nMatValType = SC_MATVAL_EMPTY;
    switch ( GetStackType() )
    {
        case svMatrix:
            {
                const ScMatrixValue* pMatVal = 0;
                ScMatrixRef pMat = PopMatrix();
                if (!pMat)
                    ;   // nothing
                else if (!pJumpMatrix)
                    pMatVal = pMat->Get( 0, 0, nMatValType);
                else
                {
                    SCSIZE nCols, nRows, nC, nR;
                    pMat->GetDimensions( nCols, nRows);
                    pJumpMatrix->GetPos( nC, nR);
                    if ( nC < nCols && nR < nRows )
                        pMatVal = pMat->Get( nC, nR, nMatValType);
                    else
                        SetError( errNoValue);
                }
                if (!pMatVal)
                {
                    rDouble = 0.0;
                    rString.Erase();
                }
                else if (nMatValType == SC_MATVAL_VALUE)
                    rDouble = pMatVal->fVal;
                else if (nMatValType == SC_MATVAL_BOOLEAN)
                {
                    rDouble = pMatVal->fVal;
                    nMatValType = SC_MATVAL_VALUE;
                }
                else
                    rString = pMatVal->GetString();
            }
            break;
        default:
            PopError();
            rDouble = 0.0;
            rString.Erase();
            SetError( errIllegalParameter);
    }
    return nMatValType;
}


void ScInterpreter::ScDBGet()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBGet" );
    SCTAB nTab;
    ScQueryParam aQueryParam;
    BOOL bMissingField = FALSE;
    if (GetDBParams( nTab, aQueryParam, bMissingField))
    {
        ScBaseCell* pCell;
        ScQueryCellIterator aCellIter(pDok, nTab, aQueryParam);
        if ( (pCell = aCellIter.GetFirst()) != NULL )
        {
            if (aCellIter.GetNext())
                PushIllegalArgument();
            else
            {
                switch (pCell->GetCellType())
                {
                    case CELLTYPE_VALUE:
                    {
                        double rValue = ((ScValueCell*)pCell)->GetValue();
                        if ( bCalcAsShown )
                        {
                            ULONG nFormat;
                            nFormat = aCellIter.GetNumberFormat();
                            rValue = pDok->RoundValueAsShown( rValue, nFormat );
                        }
                        PushDouble(rValue);
                    }
                    break;
                    case CELLTYPE_STRING:
                    {
                        String rString;
                        ((ScStringCell*)pCell)->GetString(rString);
                        PushString(rString);
                    }
                    break;
                    case CELLTYPE_EDIT:
                    {
                        String rString;
                        ((ScEditCell*)pCell)->GetString(rString);
                        PushString(rString);
                    }
                    break;
                    case CELLTYPE_FORMULA:
                    {
                        USHORT rErr = ((ScFormulaCell*)pCell)->GetErrCode();
                        if (rErr)
                            PushError(rErr);
                        else if (((ScFormulaCell*)pCell)->IsValue())
                        {
                            double rValue = ((ScFormulaCell*)pCell)->GetValue();
                            PushDouble(rValue);
                        }
                        else
                        {
                            String rString;
                            ((ScFormulaCell*)pCell)->GetString(rString);
                            PushString(rString);
                        }
                    }
                    break;
                    case CELLTYPE_NONE:
                    case CELLTYPE_NOTE:
                    default:
                        PushIllegalArgument();
                    break;
                }
            }
        }
        else
            PushNoValue();
    }
    else
        PushIllegalParameter();
}


void ScInterpreter::ScExternal()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScExternal" );
    USHORT nIndex;
    BYTE nParamCount = GetByte();
    String aUnoName;
    String aFuncName( ScGlobal::pCharClass->upper( pCur->GetExternal() ) );
    if (ScGlobal::GetFuncCollection()->SearchFunc(aFuncName, nIndex))
    {
        FuncData* pFuncData = (FuncData*)ScGlobal::GetFuncCollection()->At(nIndex);
        if (nParamCount == pFuncData->GetParamCount() - 1)
        {
            ParamType   eParamType[MAXFUNCPARAM];
            void*       ppParam[MAXFUNCPARAM];
            double      nVal[MAXFUNCPARAM];
            sal_Char*   pStr[MAXFUNCPARAM];
            BYTE*       pCellArr[MAXFUNCPARAM];
            short       i;

            for (i = 0; i < MAXFUNCPARAM; i++)
            {
                eParamType[i] = pFuncData->GetParamType(i);
                ppParam[i] = NULL;
                nVal[i] = 0.0;
                pStr[i] = NULL;
                pCellArr[i] = NULL;
            }

            for (i = nParamCount; (i > 0) && (nGlobalError == 0); i--)
            {
                switch (eParamType[i])
                {
                    case PTR_DOUBLE :
                        {
                            nVal[i-1] = GetDouble();
                            ppParam[i] = &nVal[i-1];
                        }
                        break;
                    case PTR_STRING :
                        {
                            ByteString aStr( GetString(), osl_getThreadTextEncoding() );
                            if ( aStr.Len() >= ADDIN_MAXSTRLEN )
                                SetError( errStringOverflow );
                            else
                            {
                                pStr[i-1] = new sal_Char[ADDIN_MAXSTRLEN];
                                strncpy( pStr[i-1], aStr.GetBuffer(), ADDIN_MAXSTRLEN );
                                pStr[i-1][ADDIN_MAXSTRLEN-1] = 0;
                                ppParam[i] = pStr[i-1];
                            }
                        }
                        break;
                    case PTR_DOUBLE_ARR :
                        {
                            SCCOL nCol1;
                            SCROW nRow1;
                            SCTAB nTab1;
                            SCCOL nCol2;
                            SCROW nRow2;
                            SCTAB nTab2;
                            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                            pCellArr[i-1] = new BYTE[MAXARRSIZE];
                            if (!CreateDoubleArr(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, pCellArr[i-1]))
                                SetError(errCodeOverflow);
                            else
                                ppParam[i] = pCellArr[i-1];
                        }
                        break;
                    case PTR_STRING_ARR :
                        {
                            SCCOL nCol1;
                            SCROW nRow1;
                            SCTAB nTab1;
                            SCCOL nCol2;
                            SCROW nRow2;
                            SCTAB nTab2;
                            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                            pCellArr[i-1] = new BYTE[MAXARRSIZE];
                            if (!CreateStringArr(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, pCellArr[i-1]))
                                SetError(errCodeOverflow);
                            else
                                ppParam[i] = pCellArr[i-1];
                        }
                        break;
                    case PTR_CELL_ARR :
                        {
                            SCCOL nCol1;
                            SCROW nRow1;
                            SCTAB nTab1;
                            SCCOL nCol2;
                            SCROW nRow2;
                            SCTAB nTab2;
                            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                            pCellArr[i-1] = new BYTE[MAXARRSIZE];
                            if (!CreateCellArr(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, pCellArr[i-1]))
                                SetError(errCodeOverflow);
                            else
                                ppParam[i] = pCellArr[i-1];
                        }
                        break;
                    default :
                        SetError(errIllegalParameter);
                        break;
                }
            }
            while ( i-- )
                Pop();      // im Fehlerfall (sonst ist i==0) Parameter wegpoppen

            if (nGlobalError == 0)
            {
                if ( pFuncData->GetAsyncType() == NONE )
                {
                    switch ( eParamType[0] )
                    {
                        case PTR_DOUBLE :
                        {
                            double nErg = 0.0;
                            ppParam[0] = &nErg;
                            pFuncData->Call(ppParam);
                            PushDouble(nErg);
                        }
                        break;
                        case PTR_STRING :
                        {
                            sal_Char* pcErg = new sal_Char[ADDIN_MAXSTRLEN];
                            ppParam[0] = pcErg;
                            pFuncData->Call(ppParam);
                            String aUni( pcErg, osl_getThreadTextEncoding() );
                            PushString( aUni );
                            delete[] pcErg;
                        }
                        break;
                        default:
                            PushError( errUnknownState );
                    }
                }
                else
                {
                    // nach dem Laden Asyncs wieder anwerfen
                    if ( pMyFormulaCell->GetCode()->IsRecalcModeNormal() )
                        pMyFormulaCell->GetCode()->SetRecalcModeOnLoad();
                    // garantiert identischer Handle bei identischem Aufruf?!?
                    // sonst schei*e ...
                    double nErg = 0.0;
                    ppParam[0] = &nErg;
                    pFuncData->Call(ppParam);
                    ULONG nHandle = ULONG( nErg );
                    if ( nHandle >= 65536 )
                    {
                        ScAddInAsync* pAs = ScAddInAsync::Get( nHandle );
                        if ( !pAs )
                        {
                            pAs = new ScAddInAsync( nHandle, nIndex, pDok );
                            pMyFormulaCell->StartListening( *pAs );
                        }
                        else
                        {
                            // falls per cut/copy/paste
                            pMyFormulaCell->StartListening( *pAs );
                            // in anderes Dokument?
                            if ( !pAs->HasDocument( pDok ) )
                                pAs->AddDocument( pDok );
                        }
                        if ( pAs->IsValid() )
                        {
                            switch ( pAs->GetType() )
                            {
                                case PTR_DOUBLE :
                                    PushDouble( pAs->GetValue() );
                                    break;
                                case PTR_STRING :
                                    PushString( pAs->GetString() );
                                    break;
                                default:
                                    PushError( errUnknownState );
                            }
                        }
                        else
                            PushNA();
                    }
                    else
                        PushNoValue();
                }
            }

            for (i = 0; i < MAXFUNCPARAM; i++)
            {
                delete[] pStr[i];
                delete[] pCellArr[i];
            }
        }
        else
        {
            while( nParamCount-- > 0)
                Pop();
            PushIllegalParameter();
        }
    }
    else if ( ( aUnoName = ScGlobal::GetAddInCollection()->FindFunction(aFuncName, FALSE) ).Len()  )
    {
        //  bLocalFirst=FALSE in FindFunction, cFunc should be the stored internal name

        ScUnoAddInCall aCall( *ScGlobal::GetAddInCollection(), aUnoName, nParamCount );

        if ( !aCall.ValidParamCount() )
            SetError( errIllegalParameter );

        if ( aCall.NeedsCaller() && !GetError() )
        {
            SfxObjectShell* pShell = pDok->GetDocumentShell();
            if (pShell)
                aCall.SetCallerFromObjectShell( pShell );
            else
            {
                // use temporary model object (without document) to supply options
                aCall.SetCaller( static_cast<beans::XPropertySet*>(
                                    new ScDocOptionsObj( pDok->GetDocOptions() ) ) );
            }
        }

        short nPar = nParamCount;
        while ( nPar > 0 && !GetError() )
        {
            --nPar;     // 0 .. (nParamCount-1)

            ScAddInArgumentType eType = aCall.GetArgType( nPar );
            BYTE nStackType = sal::static_int_cast<BYTE>( GetStackType() );

            uno::Any aParam;
            switch (eType)
            {
                case SC_ADDINARG_INTEGER:
                    {
                        double fVal = GetDouble();
                        double fInt = (fVal >= 0.0) ? ::rtl::math::approxFloor( fVal ) :
                                                      ::rtl::math::approxCeil( fVal );
                        if ( fInt >= LONG_MIN && fInt <= LONG_MAX )
                            aParam <<= (INT32)fInt;
                        else
                            SetError(errIllegalArgument);
                    }
                    break;

                case SC_ADDINARG_DOUBLE:
                    aParam <<= (double) GetDouble();
                    break;

                case SC_ADDINARG_STRING:
                    aParam <<= rtl::OUString( GetString() );
                    break;

                case SC_ADDINARG_INTEGER_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                double fVal = GetDouble();
                                double fInt = (fVal >= 0.0) ? ::rtl::math::approxFloor( fVal ) :
                                                              ::rtl::math::approxCeil( fVal );
                                if ( fInt >= LONG_MIN && fInt <= LONG_MAX )
                                {
                                    INT32 nIntVal = (long)fInt;
                                    uno::Sequence<INT32> aInner( &nIntVal, 1 );
                                    uno::Sequence< uno::Sequence<INT32> > aOuter( &aInner, 1 );
                                    aParam <<= aOuter;
                                }
                                else
                                    SetError(errIllegalArgument);
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillLongArray( aParam, pDok, aRange ))
                                    SetError(errIllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillLongArray( aParam, PopMatrix() ))
                                SetError(errIllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(errIllegalParameter);
                    }
                    break;

                case SC_ADDINARG_DOUBLE_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                double fVal = GetDouble();
                                uno::Sequence<double> aInner( &fVal, 1 );
                                uno::Sequence< uno::Sequence<double> > aOuter( &aInner, 1 );
                                aParam <<= aOuter;
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillDoubleArray( aParam, pDok, aRange ))
                                    SetError(errIllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillDoubleArray( aParam, PopMatrix() ))
                                SetError(errIllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(errIllegalParameter);
                    }
                    break;

                case SC_ADDINARG_STRING_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                rtl::OUString aString = rtl::OUString( GetString() );
                                uno::Sequence<rtl::OUString> aInner( &aString, 1 );
                                uno::Sequence< uno::Sequence<rtl::OUString> > aOuter( &aInner, 1 );
                                aParam <<= aOuter;
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillStringArray( aParam, pDok, aRange ))
                                    SetError(errIllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillStringArray( aParam, PopMatrix(), pFormatter ))
                                SetError(errIllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(errIllegalParameter);
                    }
                    break;

                case SC_ADDINARG_MIXED_ARRAY:
                    switch( nStackType )
                    {
                        case svDouble:
                        case svString:
                        case svSingleRef:
                            {
                                uno::Any aElem;
                                if ( nStackType == svDouble )
                                    aElem <<= (double) GetDouble();
                                else if ( nStackType == svString )
                                    aElem <<= rtl::OUString( GetString() );
                                else
                                {
                                    ScAddress aAdr;
                                    if ( PopDoubleRefOrSingleRef( aAdr ) )
                                    {
                                        ScBaseCell* pCell = GetCell( aAdr );
                                        if ( pCell && pCell->HasStringData() )
                                        {
                                            String aStr;
                                            GetCellString( aStr, pCell );
                                            aElem <<= rtl::OUString( aStr );
                                        }
                                        else
                                            aElem <<= (double) GetCellValue( aAdr, pCell );
                                    }
                                }
                                uno::Sequence<uno::Any> aInner( &aElem, 1 );
                                uno::Sequence< uno::Sequence<uno::Any> > aOuter( &aInner, 1 );
                                aParam <<= aOuter;
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillMixedArray( aParam, pDok, aRange ))
                                    SetError(errIllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillMixedArray( aParam, PopMatrix() ))
                                SetError(errIllegalParameter);
                            break;
                        default:
                            PopError();
                            SetError(errIllegalParameter);
                    }
                    break;

                case SC_ADDINARG_VALUE_OR_ARRAY:
                    if ( IsMissing() )
                        nStackType = svMissing;
                    switch( nStackType )
                    {
                        case svDouble:
                            aParam <<= (double) GetDouble();
                            break;
                        case svString:
                            aParam <<= rtl::OUString( GetString() );
                            break;
                        case svSingleRef:
                            {
                                ScAddress aAdr;
                                if ( PopDoubleRefOrSingleRef( aAdr ) )
                                {
                                    ScBaseCell* pCell = GetCell( aAdr );
                                    if ( pCell && pCell->HasStringData() )
                                    {
                                        String aStr;
                                        GetCellString( aStr, pCell );
                                        aParam <<= rtl::OUString( aStr );
                                    }
                                    else
                                        aParam <<= (double) GetCellValue( aAdr, pCell );
                                }
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                if (!ScRangeToSequence::FillMixedArray( aParam, pDok, aRange ))
                                    SetError(errIllegalParameter);
                            }
                            break;
                        case svMatrix:
                            if (!ScRangeToSequence::FillMixedArray( aParam, PopMatrix() ))
                                SetError(errIllegalParameter);
                            break;
                        case svMissing:
                            Pop();
                            aParam.clear();
                            break;
                        default:
                            PopError();
                            SetError(errIllegalParameter);
                    }
                    break;

                case SC_ADDINARG_CELLRANGE:
                    switch( nStackType )
                    {
                        case svSingleRef:
                            {
                                ScAddress aAdr;
                                PopSingleRef( aAdr );
                                ScRange aRange( aAdr );
                                uno::Reference<table::XCellRange> xObj =
                                        ScCellRangeObj::CreateRangeFromDoc( pDok, aRange );
                                if (xObj.is())
                                    aParam <<= xObj;
                                else
                                    SetError(errIllegalParameter);
                            }
                            break;
                        case svDoubleRef:
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                uno::Reference<table::XCellRange> xObj =
                                        ScCellRangeObj::CreateRangeFromDoc( pDok, aRange );
                                if (xObj.is())
                                    aParam <<= xObj;
                                else
                                    SetError(errIllegalParameter);
                            }
                            break;
                        default:
                            PopError();
                            SetError(errIllegalParameter);
                    }
                    break;

                default:
                    PopError();
                    SetError(errIllegalParameter);
            }
            aCall.SetParam( nPar, aParam );
        }

        while (nPar-- > 0)
            Pop();                  // in case of error, remove remaining args

        if ( !GetError() )
        {
            aCall.ExecuteCall();

            if ( aCall.HasVarRes() )                        // handle async functions
            {
                if ( pMyFormulaCell->GetCode()->IsRecalcModeNormal() )
                    pMyFormulaCell->GetCode()->SetRecalcModeOnLoad();

                uno::Reference<sheet::XVolatileResult> xRes = aCall.GetVarRes();
                ScAddInListener* pLis = ScAddInListener::Get( xRes );
                if ( !pLis )
                {
                    pLis = ScAddInListener::CreateListener( xRes, pDok );
                    pMyFormulaCell->StartListening( *pLis );
                }
                else
                {
                    pMyFormulaCell->StartListening( *pLis );
                    if ( !pLis->HasDocument( pDok ) )
                        pLis->AddDocument( pDok );
                }

                aCall.SetResult( pLis->GetResult() );       // use result from async
            }

            if ( aCall.GetErrCode() )
                PushError( aCall.GetErrCode() );
            else if ( aCall.HasMatrix() )
            {
                ScMatrixRef xMat = aCall.GetMatrix();
                PushMatrix( xMat );
            }
            else if ( aCall.HasString() )
                PushString( aCall.GetString() );
            else
                PushDouble( aCall.GetValue() );
        }
        else                // error...
            PushError( GetError());
    }
    else
    {
        while( nParamCount-- > 0)
            Pop();
        PushError( errNoAddin );
    }
}


void ScInterpreter::ScMissing()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScMissing" );
    PushTempToken( new FormulaMissingToken );
}


void ScInterpreter::ScMacro()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScMacro" );
    SbxBase::ResetError();

    BYTE nParamCount = GetByte();
    String aMacro( pCur->GetExternal() );

    SfxObjectShell* pDocSh = pDok->GetDocumentShell();
    if ( !pDocSh || !pDok->CheckMacroWarn() )
    {
        PushNoValue();      // ohne DocShell kein CallBasic
        return;
    }

    //  keine Sicherheitsabfrage mehr vorneweg (nur CheckMacroWarn), das passiert im CallBasic

    SfxApplication* pSfxApp = SFX_APP();
    pSfxApp->EnterBasicCall();              // Dok-Basic anlegen etc.

    //  Wenn das Dok waehrend eines Basic-Calls geladen wurde,
    //  ist das Sbx-Objekt evtl. nicht angelegt (?)
//  pDocSh->GetSbxObject();

    //  Funktion ueber den einfachen Namen suchen,
    //  dann aBasicStr, aMacroStr fuer SfxObjectShell::CallBasic zusammenbauen

    StarBASIC* pRoot = pDocSh->GetBasic();
    SbxVariable* pVar = pRoot->Find( aMacro, SbxCLASS_METHOD );
    if( !pVar || pVar->GetType() == SbxVOID || !pVar->ISA(SbMethod) )
    {
        PushError( errNoMacro );
        pSfxApp->LeaveBasicCall();
        return;
    }

    SbMethod* pMethod = (SbMethod*)pVar;
    SbModule* pModule = pMethod->GetModule();
    SbxObject* pObject = pModule->GetParent();
    DBG_ASSERT(pObject->IsA(TYPE(StarBASIC)), "Kein Basic gefunden!");
    String aMacroStr = pObject->GetName();
    aMacroStr += '.';
    aMacroStr += pModule->GetName();
    aMacroStr += '.';
    aMacroStr += pMethod->GetName();
    String aBasicStr;
    if (pObject->GetParent())
        aBasicStr = pObject->GetParent()->GetName();    // Dokumentenbasic
    else
        aBasicStr = SFX_APP()->GetName();               // Applikationsbasic

    //  Parameter-Array zusammenbauen

    SbxArrayRef refPar = new SbxArray;
    BOOL bOk = TRUE;
    for( short i = nParamCount; i && bOk ; i-- )
    {
        SbxVariable* pPar = refPar->Get( (USHORT) i );
        BYTE nStackType = sal::static_int_cast<BYTE>( GetStackType() );
        switch( nStackType )
        {
            case svDouble:
                pPar->PutDouble( GetDouble() );
            break;
            case svString:
                pPar->PutString( GetString() );
            break;
            case svSingleRef:
            {
                ScAddress aAdr;
                PopSingleRef( aAdr );
                bOk = SetSbxVariable( pPar, aAdr );
            }
            break;
            case svDoubleRef:
            {
                SCCOL nCol1;
                SCROW nRow1;
                SCTAB nTab1;
                SCCOL nCol2;
                SCROW nRow2;
                SCTAB nTab2;
                PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                if( nTab1 != nTab2 )
                {
                    SetError( errIllegalParameter );
                    bOk = FALSE;
                }
                else
                {
                    SbxDimArrayRef refArray = new SbxDimArray;
                    refArray->AddDim32( 1, nRow2 - nRow1 + 1 );
                    refArray->AddDim32( 1, nCol2 - nCol1 + 1 );
                    ScAddress aAdr( nCol1, nRow1, nTab1 );
                    for( SCROW nRow = nRow1; bOk && nRow <= nRow2; nRow++ )
                    {
                        aAdr.SetRow( nRow );
                        INT32 nIdx[ 2 ];
                        nIdx[ 0 ] = nRow-nRow1+1;
                        for( SCCOL nCol = nCol1; bOk && nCol <= nCol2; nCol++ )
                        {
                            aAdr.SetCol( nCol );
                            nIdx[ 1 ] = nCol-nCol1+1;
                            SbxVariable* p = refArray->Get32( nIdx );
                            bOk = SetSbxVariable( p, aAdr );
                        }
                    }
                    pPar->PutObject( refArray );
                }
            }
            break;
            case svMatrix:
            {
                ScMatrixRef pMat = PopMatrix();
                SCSIZE nC, nR;
                if (pMat)
                {
                    pMat->GetDimensions(nC, nR);
                    SbxDimArrayRef refArray = new SbxDimArray;
                    refArray->AddDim32( 1, static_cast<INT32>(nR) );
                    refArray->AddDim32( 1, static_cast<INT32>(nC) );
                    for( SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++ )
                    {
                        INT32 nIdx[ 2 ];
                        nIdx[ 0 ] = static_cast<INT32>(nMatRow+1);
                        for( SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++ )
                        {
                            nIdx[ 1 ] = static_cast<INT32>(nMatCol+1);
                            SbxVariable* p = refArray->Get32( nIdx );
                            if (pMat->IsString(nMatCol, nMatRow))
                                p->PutString( pMat->GetString(nMatCol, nMatRow) );
                            else
                                p->PutDouble( pMat->GetDouble(nMatCol, nMatRow));
                        }
                    }
                    pPar->PutObject( refArray );
                }
                else
                    SetError( errIllegalParameter );
            }
            break;
            default:
                SetError( errIllegalParameter );
                bOk = FALSE;
        }
    }
    if( bOk )
    {
        pDok->LockTable( aPos.Tab() );
        SbxVariableRef refRes = new SbxVariable;
        pDok->IncMacroInterpretLevel();
        ErrCode eRet = pDocSh->CallBasic( aMacroStr, aBasicStr, NULL, refPar, refRes );
        pDok->DecMacroInterpretLevel();
        pDok->UnlockTable( aPos.Tab() );

        SbxDataType eResType = refRes->GetType();
        if( pVar->GetError() )
            SetError( errNoValue);
        if ( eRet != ERRCODE_NONE )
            PushNoValue();
        else if( eResType >= SbxINTEGER && eResType <= SbxDOUBLE )
            PushDouble( refRes->GetDouble() );
        else if ( eResType & SbxARRAY )
        {
            SbxBase* pElemObj = refRes->GetObject();
            SbxDimArray* pDimArray = PTR_CAST(SbxDimArray,pElemObj);
            short nDim = pDimArray->GetDims();
            if ( 1 <= nDim && nDim <= 2 )
            {
                INT32 nCs, nCe, nRs, nRe;
                SCSIZE nC, nR;
                SCCOL nColIdx;
                SCROW nRowIdx;
                if ( nDim == 1 )
                {   // array( cols )  eine Zeile, mehrere Spalten
                    pDimArray->GetDim32( 1, nCs, nCe );
                    nC = static_cast<SCSIZE>(nCe - nCs + 1);
                    nRs = nRe = 0;
                    nR = 1;
                    nColIdx = 0;
                    nRowIdx = 1;
                }
                else
                {   // array( rows, cols )
                    pDimArray->GetDim32( 1, nRs, nRe );
                    nR = static_cast<SCSIZE>(nRe - nRs + 1);
                    pDimArray->GetDim32( 2, nCs, nCe );
                    nC = static_cast<SCSIZE>(nCe - nCs + 1);
                    nColIdx = 1;
                    nRowIdx = 0;
                }
                ScMatrixRef pMat = GetNewMat( nC, nR);
                if ( pMat )
                {
                    SbxVariable* pV;
                    SbxDataType eType;
                    for ( SCSIZE j=0; j < nR; j++ )
                    {
                        INT32 nIdx[ 2 ];
                        // bei eindimensionalem array( cols ) wird nIdx[1]
                        // von SbxDimArray::Get ignoriert
                        nIdx[ nRowIdx ] = nRs + static_cast<INT32>(j);
                        for ( SCSIZE i=0; i < nC; i++ )
                        {
                            nIdx[ nColIdx ] = nCs + static_cast<INT32>(i);
                            pV = pDimArray->Get32( nIdx );
                            eType = pV->GetType();
                            if ( eType >= SbxINTEGER && eType <= SbxDOUBLE )
                                pMat->PutDouble( pV->GetDouble(), i, j );
                            else
                                pMat->PutString( pV->GetString(), i, j );
                        }
                    }
                    PushMatrix( pMat );
                }
                else
                    PushIllegalArgument();
            }
            else
                PushNoValue();
        }
        else
            PushString( refRes->GetString() );
    }

    pSfxApp->LeaveBasicCall();
}


BOOL ScInterpreter::SetSbxVariable( SbxVariable* pVar, const ScAddress& rPos )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::SetSbxVariable" );
    BOOL bOk = TRUE;
    ScBaseCell* pCell = pDok->GetCell( rPos );
    if (pCell)
    {
        USHORT nErr;
        double nVal;
        switch( pCell->GetCellType() )
        {
            case CELLTYPE_VALUE :
                nVal = GetValueCellValue( rPos, (ScValueCell*)pCell );
                pVar->PutDouble( nVal );
                break;
            case CELLTYPE_STRING :
            {
                String aVal;
                ((ScStringCell*)pCell)->GetString( aVal );
                pVar->PutString( aVal );
                break;
            }
            case CELLTYPE_EDIT :
            {
                String aVal;
                ((ScEditCell*) pCell)->GetString( aVal );
                pVar->PutString( aVal );
                break;
            }
            case CELLTYPE_FORMULA :
                nErr = ((ScFormulaCell*)pCell)->GetErrCode();
                if( !nErr )
                {
                    if( ((ScFormulaCell*)pCell)->IsValue() )
                    {
                        nVal = ((ScFormulaCell*)pCell)->GetValue();
                        pVar->PutDouble( nVal );
                    }
                    else
                    {
                        String aVal;
                        ((ScFormulaCell*)pCell)->GetString( aVal );
                        pVar->PutString( aVal );
                    }
                }
                else
                    SetError( nErr ), bOk = FALSE;
                break;
            default :
                pVar->PutDouble( 0.0 );
        }
    }
    else
        pVar->PutDouble( 0.0 );
    return bOk;
}


void ScInterpreter::ScTableOp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTableOp" );
    BYTE nParamCount = GetByte();
    if (nParamCount != 3 && nParamCount != 5)
    {
        PushIllegalParameter();
        return;
    }
    ScInterpreterTableOpParams* pTableOp = new ScInterpreterTableOpParams;
    if (nParamCount == 5)
    {
        PopSingleRef( pTableOp->aNew2 );
        PopSingleRef( pTableOp->aOld2 );
    }
    PopSingleRef( pTableOp->aNew1 );
    PopSingleRef( pTableOp->aOld1 );
    PopSingleRef( pTableOp->aFormulaPos );

    pTableOp->bValid = TRUE;
    pDok->aTableOpList.Insert( pTableOp );
    pDok->IncInterpreterTableOpLevel();

    BOOL bReuseLastParams = (pDok->aLastTableOpParams == *pTableOp);
    if ( bReuseLastParams )
    {
        pTableOp->aNotifiedFormulaPos = pDok->aLastTableOpParams.aNotifiedFormulaPos;
        pTableOp->bRefresh = TRUE;
        for ( ::std::vector< ScAddress >::const_iterator iBroadcast(
                    pTableOp->aNotifiedFormulaPos.begin() );
                iBroadcast != pTableOp->aNotifiedFormulaPos.end();
                ++iBroadcast )
        {   // emulate broadcast and indirectly collect cell pointers
            ScBaseCell* pCell = pDok->GetCell( *iBroadcast );
            if ( pCell && pCell->GetCellType() == CELLTYPE_FORMULA )
                ((ScFormulaCell*)pCell)->SetTableOpDirty();
        }
    }
    else
    {   // broadcast and indirectly collect cell pointers and positions
        pDok->SetTableOpDirty( pTableOp->aOld1 );
        if ( nParamCount == 5 )
            pDok->SetTableOpDirty( pTableOp->aOld2 );
    }
    pTableOp->bCollectNotifications = FALSE;

    ScBaseCell* pFCell = pDok->GetCell( pTableOp->aFormulaPos );
    if ( pFCell && pFCell->GetCellType() == CELLTYPE_FORMULA )
        ((ScFormulaCell*)pFCell)->SetDirtyVar();
    if ( HasCellValueData( pFCell ) )
        PushDouble( GetCellValue( pTableOp->aFormulaPos, pFCell ));
    else
    {
        String aCellString;
        GetCellString( aCellString, pFCell );
        PushString( aCellString );
    }

    pDok->aTableOpList.Remove( pTableOp );
    // set dirty again once more to be able to recalculate original
    for ( ::std::vector< ScFormulaCell* >::const_iterator iBroadcast(
                pTableOp->aNotifiedFormulaCells.begin() );
            iBroadcast != pTableOp->aNotifiedFormulaCells.end();
            ++iBroadcast )
    {
        (*iBroadcast)->SetTableOpDirty();
    }

    // save these params for next incarnation
    if ( !bReuseLastParams )
        pDok->aLastTableOpParams = *pTableOp;

    if ( pFCell && pFCell->GetCellType() == CELLTYPE_FORMULA )
    {
        ((ScFormulaCell*)pFCell)->SetDirtyVar();
        ((ScFormulaCell*)pFCell)->GetErrCode();     // recalculate original
    }

    // Reset all dirty flags so next incarnation does really collect all cell
    // pointers during notifications and not just non-dirty ones, which may
    // happen if a formula cell is used by more than one TableOp block.
    for ( ::std::vector< ScFormulaCell* >::const_iterator iBroadcast2(
                pTableOp->aNotifiedFormulaCells.begin() );
            iBroadcast2 != pTableOp->aNotifiedFormulaCells.end();
            ++iBroadcast2 )
    {
        (*iBroadcast2)->ResetTableOpDirtyVar();
    }
    delete pTableOp;

    pDok->DecInterpreterTableOpLevel();
}


/*

void ScInterpreter::ScErrCell()
{
RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScErrCell" );
    double fErrNum = GetDouble();
    PushError((USHORT) fErrNum);
}
*/

void ScInterpreter::ScDBArea()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBArea" );
    ScDBData* pDBData = pDok->GetDBCollection()->FindIndex( pCur->GetIndex());
    if (pDBData)
    {
        ScComplexRefData aRefData;
        aRefData.InitFlags();
        pDBData->GetArea( (SCTAB&) aRefData.Ref1.nTab,
                          (SCCOL&) aRefData.Ref1.nCol,
                          (SCROW&) aRefData.Ref1.nRow,
                          (SCCOL&) aRefData.Ref2.nCol,
                          (SCROW&) aRefData.Ref2.nRow);
        aRefData.Ref2.nTab    = aRefData.Ref1.nTab;
        aRefData.CalcRelFromAbs( aPos );
        PushTempToken( new ScDoubleRefToken( aRefData ) );
    }
    else
        PushError( errNoName);
}


void ScInterpreter::ScColRowNameAuto()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScColRowNameAuto" );
    ScComplexRefData aRefData( static_cast<const ScToken*>(pCur)->GetDoubleRef() );
    aRefData.CalcAbsIfRel( aPos );
    if ( aRefData.Valid() )
    {
        SCsCOL nStartCol;
        SCsROW nStartRow;
        SCsCOL nCol2;
        SCsROW nRow2;
        // evtl. Begrenzung durch definierte ColRowNameRanges merken
        nCol2 = aRefData.Ref2.nCol;
        nRow2 = aRefData.Ref2.nRow;
        // DataArea der ersten Zelle
        nStartCol = aRefData.Ref2.nCol = aRefData.Ref1.nCol;
        nStartRow = aRefData.Ref2.nRow = aRefData.Ref1.nRow;
        aRefData.Ref2.nTab = aRefData.Ref1.nTab;
        pDok->GetDataArea(  (SCTAB&) aRefData.Ref1.nTab,
                            (SCCOL&) aRefData.Ref1.nCol,
                            (SCROW&) aRefData.Ref1.nRow,
                            (SCCOL&) aRefData.Ref2.nCol,
                            (SCROW&) aRefData.Ref2.nRow,
                            TRUE );
        // DataArea im Ursprung begrenzen
        aRefData.Ref1.nCol = nStartCol;
        aRefData.Ref1.nRow = nStartRow;

        //! korrespondiert mit ScCompiler::GetToken
        if ( aRefData.Ref1.IsColRel() )
        {   // ColName
            aRefData.Ref2.nCol = nStartCol;
            // evtl. vorherige Begrenzung durch definierte ColRowNameRanges erhalten
            if ( aRefData.Ref2.nRow > nRow2 )
                aRefData.Ref2.nRow = nRow2;
            SCROW nMyRow;
            if ( aPos.Col() == nStartCol
              && nStartRow <= (nMyRow = aPos.Row()) && nMyRow <= aRefData.Ref2.nRow )
            {   // Formel in gleicher Spalte und innerhalb des Range
                if ( nMyRow == nStartRow )
                {   // direkt unter dem Namen den Rest nehmen
                    nStartRow++;
                    if ( nStartRow > MAXROW )
                        nStartRow = MAXROW;
                    aRefData.Ref1.nRow = nStartRow;
                }
                else
                {   // weiter unten vom Namen bis zur Formelzelle
                    aRefData.Ref2.nRow = nMyRow - 1;
                }
            }
        }
        else
        {   // RowName
            aRefData.Ref2.nRow = nStartRow;
            // evtl. vorherige Begrenzung durch definierte ColRowNameRanges erhalten
            if ( aRefData.Ref2.nCol > nCol2 )
                aRefData.Ref2.nCol = nCol2;
            SCCOL nMyCol;
            if ( aPos.Row() == nStartRow
              && nStartCol <= (nMyCol = aPos.Col()) && nMyCol <= aRefData.Ref2.nCol )
            {   // Formel in gleicher Zeile und innerhalb des Range
                if ( nMyCol == nStartCol )
                {   // direkt neben dem Namen den Rest nehmen
                    nStartCol++;
                    if ( nStartCol > MAXCOL )
                        nStartCol = MAXCOL;
                    aRefData.Ref1.nCol = nStartCol;
                }
                else
                {   // weiter rechts vom Namen bis zur Formelzelle
                    aRefData.Ref2.nCol = nMyCol - 1;
                }
            }
        }
        aRefData.CalcRelFromAbs( aPos );
        PushTempToken( new ScDoubleRefToken( aRefData ) );
    }
    else
        PushError( errNoRef );
}

void ScInterpreter::ScExternalRef()
{
    ScExternalRefManager* pRefMgr = pDok->GetExternalRefManager();
    const String* pFile = pRefMgr->getExternalFileName(pCur->GetIndex());
    if (!pFile)
        PushError(errNoName);

    switch (pCur->GetType())
    {
        case svExternalSingleRef:
        {
            ScSingleRefData aData(static_cast<const ScToken*>(pCur)->GetSingleRef());
            if (aData.IsTabRel())
            {
                DBG_ERROR("ScCompiler::GetToken: external single reference must have an absolute table reference!");
                break;
            }

            aData.CalcAbsIfRel(aPos);
            ScAddress aAddr(aData.nCol, aData.nRow, aData.nTab);
            ScExternalRefCache::CellFormat aFmt;
            ScExternalRefCache::TokenRef xNew = pRefMgr->getSingleRefToken(
                pCur->GetIndex(), pCur->GetString(), aAddr, &aPos, NULL, &aFmt);

            if (!xNew)
                break;

            PushTempToken( *xNew);      // push a clone

            if (aFmt.mbIsSet)
            {
                nFuncFmtType = aFmt.mnType;
                nFuncFmtIndex = aFmt.mnIndex;
            }
            return;
        }
        //break;    // unreachable, prevent compiler warning
        case svExternalDoubleRef:
        {
            ScComplexRefData aData(static_cast<const ScToken*>(pCur)->GetDoubleRef());
            if (aData.Ref1.IsTabRel() || aData.Ref2.IsTabRel())
            {
                DBG_ERROR("ScCompiler::GetToken: external double reference must have an absolute table reference!");
                break;
            }

            aData.CalcAbsIfRel(aPos);
            ScRange aRange(aData.Ref1.nCol, aData.Ref1.nRow, aData.Ref1.nTab,
                           aData.Ref2.nCol, aData.Ref2.nRow, aData.Ref2.nTab);
            ScExternalRefCache::TokenArrayRef xNew = pRefMgr->getDoubleRefTokens(
                pCur->GetIndex(), pCur->GetString(), aRange, &aPos);

            if (!xNew)
                break;

            ScToken* p = static_cast<ScToken*>(xNew->First());
            if (p->GetType() != svMatrix)
                break;

            if (xNew->Next())
            {
                // Can't handle more than one matrix per parameter.
                SetError( errIllegalArgument);
                break;
            }

            PushMatrix(p->GetMatrix());
            return;
        }
        //break;    // unreachable, prevent compiler warning
        default:
            ;
    }
    PushError(errNoRef);
}

// --- internals ------------------------------------------------------------


void ScInterpreter::ScAnswer()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScAnswer" );
    String aStr( GetString() );
    if( aStr.EqualsIgnoreCaseAscii( "Das Leben, das Universum und der ganze Rest" ) )
    {
        PushInt( 42 );
        bOderSo = TRUE;
    }
    else
        PushNoValue();
}


void ScInterpreter::ScCalcTeam()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCalcTeam" );
    static BOOL bShown = FALSE;
    if( !bShown )
    {
        ShowTheTeam();
        String aTeam( RTL_CONSTASCII_USTRINGPARAM( "Nebel, Benisch, Rentz, Rathke" ) );
        if ( (GetByte() == 1) && ::rtl::math::approxEqual( GetDouble(), 1996) )
            aTeam.AppendAscii( "   (a word with 'B': -Olk, -Nietsch, -Daeumling)" );
        PushString( aTeam );
        bShown = TRUE;
    }
    else
        PushInt( 42 );
}


void ScInterpreter::ScSpewFunc()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSpewFunc" );
    BOOL bRefresh = FALSE;
    BOOL bClear = FALSE;
    // Stack aufraeumen
    BYTE nParamCount = GetByte();
    while ( nParamCount-- > 0)
    {
        switch ( GetStackType() )
        {
            case svString:
            case svSingleRef:
            case svDoubleRef:
            {
                const sal_Unicode ch = GetString().GetChar(0);
                if ( !bRefresh && ch < 256 )
                    bRefresh = (tolower( (sal_uChar) ch ) == 'r');
                if ( !bClear && ch < 256 )
                    bClear = (tolower( (sal_uChar) ch ) == 'c');
            }
            break;
            default:
                PopError();
        }
    }
    String aStr;
#if SC_SPEW_ENABLED
    if ( bRefresh )
        theSpew.Clear();        // GetSpew liest SpewRulesFile neu
    theSpew.GetSpew( aStr );
    if ( bClear )
        theSpew.Clear();        // release Memory
    xub_StrLen nPos = 0;
    while ( (nPos = aStr.SearchAndReplace( '\n', ' ', nPos )) != STRING_NOTFOUND )
        nPos++;
#else
    aStr.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "spitted out all spew :-(" ) );
#endif
    PushString( aStr );
}


#include "sctictac.hxx"
#include "scmod.hxx"

extern "C" { static void SAL_CALL thisModule() {} }

void ScInterpreter::ScGame()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScGame" );
    enum GameType {
        SC_GAME_NONE,
        SC_GAME_ONCE,
        SC_GAME_START,
        SC_GAME_TICTACTOE = SC_GAME_START,
        SC_GAME_STARWARS,
        SC_GAME_FROGGER,
        SC_GAME_COUNT
    };
    // ein grep im binary laeuft ins leere
    static sal_Char sGameNone[]         = "\14\36\6\137\10\27\36\13\100";
    static sal_Char sGameOnce[]         = "\20\27\137\21\20\123\137\21\20\13\137\36\30\36\26\21\136";
    static sal_Char sGameTicTacToe[]    = "\53\26\34\53\36\34\53\20\32";
    static sal_Char sGameStarWars[]     = "\54\13\36\15\50\36\15\14";
    static sal_Char sGameFrogger[]      = "\71\15\20\30\30\26\32";
    sal_Char* const pGames[SC_GAME_COUNT] = {
        sGameNone,
        sGameOnce,
        sGameTicTacToe,
        sGameStarWars,
        sGameFrogger
    };
#if 0
say what?
oh no, not again!
TicTacToe
StarWars
Froggie
// Routine um Datenblock zu erzeugen:
#include <stdio.h>
int main()
{
    int b = 1;
    int c;
    while ( (c = getchar()) != EOF )
    {
        if ( b == 1 )
        {
            printf( "\"" );
            b = 0;
        }
        if ( c != 10 )
        {
            c ^= 0x7F;
            printf( "\\%o", c );

        }
        else
        {
            printf( "\";\n" );
            b = 1;
        }
    }
    return 0;
}
#endif
    static BOOL bRun[SC_GAME_COUNT] = { FALSE };
    static BOOL bFirst = TRUE;
    if ( bFirst )
    {
        bFirst = FALSE;
        for ( int j = SC_GAME_NONE; j < SC_GAME_COUNT; j++ )
        {
            sal_Char* p = pGames[j];
            while ( *p )
                *p++ ^= 0x7F;
        }
    }
    String aFuncResult;
    GameType eGame = SC_GAME_NONE;
    BYTE nParamCount = GetByte();
    if ( nParamCount >= 1 )
    {
        String aStr( GetString() );
        nParamCount--;
        for ( int j = SC_GAME_START; j < SC_GAME_COUNT; j++ )
        {
            if ( aStr.EqualsAscii( pGames[j] ) )
            {
                eGame = (GameType) j;
                break;  // for
            }
        }
        if ( eGame != SC_GAME_NONE )
        {
            // jedes Game nur ein einziges Mal starten, um nicht durch Recalc
            // o.ae. mehrere Instanzen zu haben, ideal waere eine Abfrage an den
            // Games, ob sie bereits laufen ...
            if ( bRun[ eGame ] && eGame != SC_GAME_TICTACTOE )
                eGame = SC_GAME_ONCE;
            else
            {
                bRun[ eGame ] = TRUE;
                switch ( eGame )
                {
                    case SC_GAME_TICTACTOE :
                    {
                        static ScTicTacToe* pTicTacToe = NULL;
                        static ScRange aTTTrange;
                        static BOOL bHumanFirst = FALSE;
                        if ( nParamCount >= 1 )
                        {
                            if ( GetStackType() == svDoubleRef )
                            {
                                ScRange aRange;
                                PopDoubleRef( aRange );
                                nParamCount--;
                                if ( aRange.aEnd.Col() - aRange.aStart.Col() == 2
                                  && aRange.aEnd.Row() - aRange.aStart.Row() == 2 )
                                {
                                    BOOL bOk;
                                    if ( pTicTacToe )
                                        bOk = (aRange == aTTTrange);
                                    else
                                    {
                                        bOk =TRUE;
                                        aTTTrange = aRange;
                                        pTicTacToe = new ScTicTacToe( pDok,
                                            aRange.aStart );
                                        pTicTacToe->Initialize( bHumanFirst );
                                    }
                                    // nur einmal und das auf dem gleichen Range
                                    if ( !bOk )
                                        eGame = SC_GAME_ONCE;
                                    else
                                    {
                                        Square_Type aWinner = pTicTacToe->CalcMove();
                                        pTicTacToe->GetOutput( aFuncResult );
                                        if ( aWinner != pTicTacToe->GetEmpty() )
                                        {
                                            delete pTicTacToe;
                                            pTicTacToe = NULL;
                                            bRun[ eGame ] = FALSE;
                                            bHumanFirst = !bHumanFirst;
                                        }
                                        pDok->GetDocumentShell()->Broadcast(
                                            SfxSimpleHint( FID_DATACHANGED ) );
                                        pDok->ResetChanged( aRange );
                                    }
                                }
                                else
                                    SetError( errIllegalArgument );
                            }
                            else
                                SetError( errIllegalParameter );
                        }
                        else
                            SetError( errIllegalParameter );
                    }
                    break;
                    case SC_GAME_STARWARS :
                    {
                        oslModule m_tfu = osl_loadModuleRelative(&thisModule, rtl::OUString::createFromAscii( SVLIBRARY( "tfu" ) ).pData, SAL_LOADMODULE_NOW);
                        typedef void StartInvader_Type (Window*, ResMgr*);

                        StartInvader_Type *StartInvader = (StartInvader_Type *) osl_getFunctionSymbol( m_tfu, rtl::OUString::createFromAscii("StartInvader").pData );
                        if ( StartInvader )
                            StartInvader( Application::GetDefDialogParent(), ResMgr::CreateResMgr( "tfu" ));
                    }
                    break;
                    case SC_GAME_FROGGER :
                        //Game();
                    break;
                    default:
                    {
                        // added to avoid warnings
                    }
                }
            }
        }
    }
    // Stack aufraeumen
    while ( nParamCount-- > 0)
        Pop();
    if ( !aFuncResult.Len() )
        PushString( String( pGames[ eGame ], RTL_TEXTENCODING_ASCII_US ) );
    else
        PushString( aFuncResult );
}

void ScInterpreter::ScTTT()
{   // Temporaerer Test-Tanz, zum auspropieren von Funktionen etc.
    BOOL bOk = TRUE;
    BYTE nParamCount = GetByte();
    // do something, nParamCount bei Pops runterzaehlen!

    if ( bOk && nParamCount )
    {
        bOk = GetBool();
        --nParamCount;
    }
    // Stack aufraeumen
    while ( nParamCount-- > 0)
        Pop();
    static const sal_Unicode __FAR_DATA sEyes[]     = { ',',';',':','|','8','B', 0 };
    static const sal_Unicode __FAR_DATA sGoods[]    = { ')',']','}', 0 };
    static const sal_Unicode __FAR_DATA sBads[]     = { '(','[','{','/', 0 };
    sal_Unicode aFace[4];
    if ( bOk )
    {
        aFace[0] = sEyes[ rand() % ((sizeof( sEyes )/sizeof(sal_Unicode)) - 1) ];
        aFace[1] = '-';
        aFace[2] = sGoods[ rand() % ((sizeof( sGoods )/sizeof(sal_Unicode)) - 1) ];
    }
    else
    {
        aFace[0] = ':';
        aFace[1] = '-';
        aFace[2] = sBads[ rand() % ((sizeof( sBads )/sizeof(sal_Unicode)) - 1) ];
    }
    aFace[3] = 0;
    PushStringBuffer( aFace );
}

// -------------------------------------------------------------------------


ScInterpreter::ScInterpreter( ScFormulaCell* pCell, ScDocument* pDoc,
        const ScAddress& rPos, ScTokenArray& r ) :
    aCode( r ),
    aPos( rPos ),
    rArr( r ),
    pDok( pDoc ),
    pTokenMatrixMap( NULL ),
    pMyFormulaCell( pCell ),
    pFormatter( pDoc->GetFormatTable() ),
    bCalcAsShown( pDoc->GetDocOptions().IsCalcAsShown() )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTTT" );
//  pStack = new ScToken*[ MAXSTACK ];

    BYTE cMatFlag = pMyFormulaCell->GetMatrixFlag();
    bMatrixFormula = ( cMatFlag == MM_FORMULA || cMatFlag == MM_FAKE );
    if (!bGlobalStackInUse)
    {
        bGlobalStackInUse = TRUE;
        if (!pGlobalStack)
            pGlobalStack = new ScTokenStack;
        pStackObj = pGlobalStack;
    }
    else
    {
        pStackObj = new ScTokenStack;
    }
    pStack = pStackObj->pPointer;
}

ScInterpreter::~ScInterpreter()
{
//  delete pStack;

    if ( pStackObj == pGlobalStack )
        bGlobalStackInUse = FALSE;
    else
        delete pStackObj;
    if (pTokenMatrixMap)
        delete pTokenMatrixMap;
}


void ScInterpreter::GlobalExit()        // static
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GlobalExit" );
    DBG_ASSERT(!bGlobalStackInUse, "wer benutzt noch den TokenStack?");
    DELETEZ(pGlobalStack);
}


StackVar ScInterpreter::Interpret()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::Interpret" );
    short nRetTypeExpr = NUMBERFORMAT_UNDEFINED;
    ULONG nRetIndexExpr = 0;
    USHORT nErrorFunction = 0;
    USHORT nErrorFunctionCount = 0;
    USHORT nStackBase;

    nGlobalError = 0;
    nStackBase = sp = maxsp = 0;
    nRetFmtType = NUMBERFORMAT_UNDEFINED;
    nFuncFmtType = NUMBERFORMAT_UNDEFINED;
    nFuncFmtIndex = nCurFmtIndex = nRetFmtIndex = 0;
    xResult = NULL;
    pJumpMatrix = NULL;
    glSubTotal = FALSE;
    ScTokenMatrixMap::const_iterator aTokenMatrixMapIter;

    // Once upon a time we used to have FP exceptions on, and there was a
    // Windows printer driver that kept switching off exceptions, so we had to
    // switch them back on again every time. Who knows if there isn't a driver
    // that keeps switching exceptions on, now that we run with exceptions off,
    // so reassure exceptions are really off.
    SAL_MATH_FPEXCEPTIONS_OFF();

    aCode.Reset();
    while( ( pCur = aCode.Next() ) != NULL
            && (!nGlobalError || nErrorFunction <= nErrorFunctionCount) )
    {
        OpCode eOp = pCur->GetOpCode();
        cPar = pCur->GetByte();
        if ( eOp == ocPush )
        {
            // RPN code push without error
            PushWithoutError( (FormulaToken&) *pCur );
        }
        else if (pTokenMatrixMap && !(eOp == ocIf || eOp == ocChose) &&
                ((aTokenMatrixMapIter = pTokenMatrixMap->find( pCur)) !=
                 pTokenMatrixMap->end()) &&
                (*aTokenMatrixMapIter).second->GetType() != svJumpMatrix)
        {
            // Path already calculated, reuse result.
            nStackBase = sp - pCur->GetParamCount();
            if ( nStackBase > sp )
                nStackBase = sp;        // underflow?!?
            sp = nStackBase;
            PushTempToken( (*aTokenMatrixMapIter).second);
        }
        else
        {
            // previous expression determines the current number format
            nCurFmtType = nRetTypeExpr;
            nCurFmtIndex = nRetIndexExpr;
            // default function's format, others are set if needed
            nFuncFmtType = NUMBERFORMAT_NUMBER;
            nFuncFmtIndex = 0;

            if ( eOp == ocIf || eOp == ocChose )
                nStackBase = sp;        // don't mess around with the jumps
            else
            {
                // Convert parameters to matrix if in array/matrix formula and
                // parameters of function indicate doing so. Create JumpMatrix
                // if necessary.
                if ( MatrixParameterConversion() )
                {
                    eOp = ocNone;       // JumpMatrix created
                    nStackBase = sp;
                }
                else
                    nStackBase = sp - pCur->GetParamCount();
            }
            if ( nStackBase > sp )
                nStackBase = sp;        // underflow?!?

            switch( eOp )
            {
                case ocSep:
                case ocClose:           // pushed by the compiler
                case ocMissing          : ScMissing();                  break;
                case ocMacro            : ScMacro();                    break;
                case ocDBArea           : ScDBArea();                   break;
                case ocColRowNameAuto   : ScColRowNameAuto();           break;
// separated    case ocPush             : Push( (ScToken&) *pCur );     break;
                case ocExternalRef      : ScExternalRef();              break;
                case ocIf               : ScIfJump();                   break;
                case ocChose            : ScChoseJump();                break;
                case ocAdd              : ScAdd();                      break;
                case ocSub              : ScSub();                      break;
                case ocMul              : ScMul();                      break;
                case ocDiv              : ScDiv();                      break;
                case ocAmpersand        : ScAmpersand();                break;
                case ocPow              : ScPow();                      break;
                case ocEqual            : ScEqual();                    break;
                case ocNotEqual         : ScNotEqual();                 break;
                case ocLess             : ScLess();                     break;
                case ocGreater          : ScGreater();                  break;
                case ocLessEqual        : ScLessEqual();                break;
                case ocGreaterEqual     : ScGreaterEqual();             break;
                case ocAnd              : ScAnd();                      break;
                case ocOr               : ScOr();                       break;
                case ocIntersect        : ScIntersect();                break;
                case ocRange            : ScRangeFunc();                break;
                case ocUnion            : ScUnionFunc();                break;
                case ocNot              : ScNot();                      break;
                case ocNegSub           :
                case ocNeg              : ScNeg();                      break;
                case ocPercentSign      : ScPercentSign();              break;
                case ocPi               : ScPi();                       break;
//              case ocDefPar           : ScDefPar();                   break;
                case ocRandom           : ScRandom();                   break;
                case ocTrue             : ScTrue();                     break;
                case ocFalse            : ScFalse();                    break;
                case ocGetActDate       : ScGetActDate();               break;
                case ocGetActTime       : ScGetActTime();               break;
                case ocNotAvail         : PushError( NOTAVAILABLE);     break;
                case ocDeg              : ScDeg();                      break;
                case ocRad              : ScRad();                      break;
                case ocSin              : ScSin();                      break;
                case ocCos              : ScCos();                      break;
                case ocTan              : ScTan();                      break;
                case ocCot              : ScCot();                      break;
                case ocArcSin           : ScArcSin();                   break;
                case ocArcCos           : ScArcCos();                   break;
                case ocArcTan           : ScArcTan();                   break;
                case ocArcCot           : ScArcCot();                   break;
                case ocSinHyp           : ScSinHyp();                   break;
                case ocCosHyp           : ScCosHyp();                   break;
                case ocTanHyp           : ScTanHyp();                   break;
                case ocCotHyp           : ScCotHyp();                   break;
                case ocArcSinHyp        : ScArcSinHyp();                break;
                case ocArcCosHyp        : ScArcCosHyp();                break;
                case ocArcTanHyp        : ScArcTanHyp();                break;
                case ocArcCotHyp        : ScArcCotHyp();                break;
                case ocExp              : ScExp();                      break;
                case ocLn               : ScLn();                       break;
                case ocLog10            : ScLog10();                    break;
                case ocSqrt             : ScSqrt();                     break;
                case ocFact             : ScFact();                     break;
                case ocGetYear          : ScGetYear();                  break;
                case ocGetMonth         : ScGetMonth();                 break;
                case ocGetDay           : ScGetDay();                   break;
                case ocGetDayOfWeek     : ScGetDayOfWeek();             break;
                case ocWeek             : ScGetWeekOfYear();            break;
                case ocEasterSunday     : ScEasterSunday();             break;
                case ocGetHour          : ScGetHour();                  break;
                case ocGetMin           : ScGetMin();                   break;
                case ocGetSec           : ScGetSec();                   break;
                case ocPlusMinus        : ScPlusMinus();                break;
                case ocAbs              : ScAbs();                      break;
                case ocInt              : ScInt();                      break;
                case ocEven             : ScEven();                     break;
                case ocOdd              : ScOdd();                      break;
                case ocPhi              : ScPhi();                      break;
                case ocGauss            : ScGauss();                    break;
                case ocStdNormDist      : ScStdNormDist();              break;
                case ocFisher           : ScFisher();                   break;
                case ocFisherInv        : ScFisherInv();                break;
                case ocIsEmpty          : ScIsEmpty();                  break;
                case ocIsString         : ScIsString();                 break;
                case ocIsNonString      : ScIsNonString();              break;
                case ocIsLogical        : ScIsLogical();                break;
                case ocType             : ScType();                     break;
                case ocCell             : ScCell();                     break;
                case ocIsRef            : ScIsRef();                    break;
                case ocIsValue          : ScIsValue();                  break;
                case ocIsFormula        : ScIsFormula();                break;
                case ocFormula          : ScFormula();                  break;
                case ocIsNA             : ScIsNV();                     break;
                case ocIsErr            : ScIsErr();                    break;
                case ocIsError          : ScIsError();                  break;
                case ocIsEven           : ScIsEven();                   break;
                case ocIsOdd            : ScIsOdd();                    break;
                case ocN                : ScN();                        break;
                case ocGetDateValue     : ScGetDateValue();             break;
                case ocGetTimeValue     : ScGetTimeValue();             break;
                case ocCode             : ScCode();                     break;
                case ocTrim             : ScTrim();                     break;
                case ocUpper            : ScUpper();                    break;
                case ocPropper          : ScPropper();                  break;
                case ocLower            : ScLower();                    break;
                case ocLen              : ScLen();                      break;
                case ocT                : ScT();                        break;
                case ocClean            : ScClean();                    break;
                case ocValue            : ScValue();                    break;
                case ocChar             : ScChar();                     break;
                case ocArcTan2          : ScArcTan2();                  break;
                case ocMod              : ScMod();                      break;
                case ocPower            : ScPower();                    break;
                case ocRound            : ScRound();                    break;
                case ocRoundUp          : ScRoundUp();                  break;
                case ocTrunc            :
                case ocRoundDown        : ScRoundDown();                break;
                case ocCeil             : ScCeil();                     break;
                case ocFloor            : ScFloor();                    break;
                case ocSumProduct       : ScSumProduct();               break;
                case ocSumSQ            : ScSumSQ();                    break;
                case ocSumX2MY2         : ScSumX2MY2();                 break;
                case ocSumX2DY2         : ScSumX2DY2();                 break;
                case ocSumXMY2          : ScSumXMY2();                  break;
                case ocLog              : ScLog();                      break;
                case ocGCD              : ScGCD();                      break;
                case ocLCM              : ScLCM();                      break;
                case ocGetDate          : ScGetDate();                  break;
                case ocGetTime          : ScGetTime();                  break;
                case ocGetDiffDate      : ScGetDiffDate();              break;
                case ocGetDiffDate360   : ScGetDiffDate360();           break;
                case ocMin              : ScMin( FALSE );               break;
                case ocMinA             : ScMin( TRUE );                break;
                case ocMax              : ScMax( FALSE );               break;
                case ocMaxA             : ScMax( TRUE );                break;
                case ocSum              : ScSum();                      break;
                case ocProduct          : ScProduct();                  break;
                case ocNPV              : ScNPV();                      break;
                case ocIRR              : ScIRR();                      break;
                case ocMIRR             : ScMIRR();                     break;
                case ocISPMT            : ScISPMT();                    break;
                case ocAverage          : ScAverage( FALSE );           break;
                case ocAverageA         : ScAverage( TRUE );            break;
                case ocCount            : ScCount();                    break;
                case ocCount2           : ScCount2();                   break;
                case ocVar              : ScVar( FALSE );               break;
                case ocVarA             : ScVar( TRUE );                break;
                case ocVarP             : ScVarP( FALSE );              break;
                case ocVarPA            : ScVarP( TRUE );               break;
                case ocStDev            : ScStDev( FALSE );             break;
                case ocStDevA           : ScStDev( TRUE );              break;
                case ocStDevP           : ScStDevP( FALSE );            break;
                case ocStDevPA          : ScStDevP( TRUE );             break;
                case ocBW               : ScBW();                       break;
                case ocDIA              : ScDIA();                      break;
                case ocGDA              : ScGDA();                      break;
                case ocGDA2             : ScGDA2();                     break;
                case ocVBD              : ScVDB();                      break;
                case ocLaufz            : ScLaufz();                    break;
                case ocLIA              : ScLIA();                      break;
                case ocRMZ              : ScRMZ();                      break;
                case ocColumns          : ScColumns();                  break;
                case ocRows             : ScRows();                     break;
                case ocTables           : ScTables();                   break;
                case ocColumn           : ScColumn();                   break;
                case ocRow              : ScRow();                      break;
                case ocTable            : ScTable();                    break;
                case ocZGZ              : ScZGZ();                      break;
                case ocZW               : ScZW();                       break;
                case ocZZR              : ScZZR();                      break;
                case ocZins             : ScZins();                     break;
                case ocZinsZ            : ScZinsZ();                    break;
                case ocKapz             : ScKapz();                     break;
                case ocKumZinsZ         : ScKumZinsZ();                 break;
                case ocKumKapZ          : ScKumKapZ();                  break;
                case ocEffektiv         : ScEffektiv();                 break;
                case ocNominal          : ScNominal();                  break;
                case ocSubTotal         : ScSubTotal();                 break;
                case ocDBSum            : ScDBSum();                    break;
                case ocDBCount          : ScDBCount();                  break;
                case ocDBCount2         : ScDBCount2();                 break;
                case ocDBAverage        : ScDBAverage();                break;
                case ocDBGet            : ScDBGet();                    break;
                case ocDBMax            : ScDBMax();                    break;
                case ocDBMin            : ScDBMin();                    break;
                case ocDBProduct        : ScDBProduct();                break;
                case ocDBStdDev         : ScDBStdDev();                 break;
                case ocDBStdDevP        : ScDBStdDevP();                break;
                case ocDBVar            : ScDBVar();                    break;
                case ocDBVarP           : ScDBVarP();                   break;
                case ocIndirect         : ScIndirect();                 break;
                case ocAddress          : ScAddressFunc();              break;
                case ocMatch            : ScMatch();                    break;
                case ocCountEmptyCells  : ScCountEmptyCells();          break;
                case ocCountIf          : ScCountIf();                  break;
                case ocSumIf            : ScSumIf();                    break;
                case ocLookup           : ScLookup();                   break;
                case ocVLookup          : ScVLookup();                  break;
                case ocHLookup          : ScHLookup();                  break;
                case ocIndex            : ScIndex();                    break;
                case ocMultiArea        : ScMultiArea();                break;
                case ocOffset           : ScOffset();                   break;
                case ocAreas            : ScAreas();                    break;
                case ocCurrency         : ScCurrency();                 break;
                case ocReplace          : ScReplace();                  break;
                case ocFixed            : ScFixed();                    break;
                case ocFind             : ScFind();                     break;
                case ocExact            : ScExact();                    break;
                case ocLeft             : ScLeft();                     break;
                case ocRight            : ScRight();                    break;
                case ocSearch           : ScSearch();                   break;
                case ocMid              : ScMid();                      break;
                case ocText             : ScText();                     break;
                case ocSubstitute       : ScSubstitute();               break;
                case ocRept             : ScRept();                     break;
                case ocConcat           : ScConcat();                   break;
                case ocMatValue         : ScMatValue();                 break;
                case ocMatrixUnit       : ScEMat();                     break;
                case ocMatDet           : ScMatDet();                   break;
                case ocMatInv           : ScMatInv();                   break;
                case ocMatMult          : ScMatMult();                  break;
                case ocMatTrans         : ScMatTrans();                 break;
                case ocMatRef           : ScMatRef();                   break;
                case ocBackSolver       : ScBackSolver();               break;
                case ocB                : ScB();                        break;
                case ocNormDist         : ScNormDist();                 break;
                case ocExpDist          : ScExpDist();                  break;
                case ocBinomDist        : ScBinomDist();                break;
                case ocPoissonDist      : ScPoissonDist();              break;
                case ocKombin           : ScKombin();                   break;
                case ocKombin2          : ScKombin2();                  break;
                case ocVariationen      : ScVariationen();              break;
                case ocVariationen2     : ScVariationen2();             break;
                case ocHypGeomDist      : ScHypGeomDist();              break;
                case ocLogNormDist      : ScLogNormDist();              break;
                case ocTDist            : ScTDist();                    break;
                case ocFDist            : ScFDist();                    break;
                case ocChiDist          : ScChiDist();                  break;
                case ocChiSqDist        : ScChiSqDist();                break;
                case ocStandard         : ScStandard();                 break;
                case ocAveDev           : ScAveDev();                   break;
                case ocDevSq            : ScDevSq();                    break;
                case ocKurt             : ScKurt();                     break;
                case ocSchiefe          : ScSkew();                     break;
                case ocModalValue       : ScModalValue();               break;
                case ocMedian           : ScMedian();                   break;
                case ocGeoMean          : ScGeoMean();                  break;
                case ocHarMean          : ScHarMean();                  break;
                case ocWeibull          : ScWeibull();                  break;
                case ocKritBinom        : ScCritBinom();                break;
                case ocNegBinomVert     : ScNegBinomDist();             break;
                case ocNoName           : ScNoName();                   break;
                case ocBad              : ScBadName();                  break;
                case ocZTest            : ScZTest();                    break;
                case ocTTest            : ScTTest();                    break;
                case ocFTest            : ScFTest();                    break;
                case ocRank             : ScRank();                     break;
                case ocPercentile       : ScPercentile();               break;
                case ocPercentrank      : ScPercentrank();              break;
                case ocLarge            : ScLarge();                    break;
                case ocSmall            : ScSmall();                    break;
                case ocFrequency        : ScFrequency();                break;
                case ocQuartile         : ScQuartile();                 break;
                case ocNormInv          : ScNormInv();                  break;
                case ocSNormInv         : ScSNormInv();                 break;
                case ocConfidence       : ScConfidence();               break;
                case ocTrimMean         : ScTrimMean();                 break;
                case ocProb             : ScProbability();              break;
                case ocCorrel           : ScCorrel();                   break;
                case ocCovar            : ScCovar();                    break;
                case ocPearson          : ScPearson();                  break;
                case ocRSQ              : ScRSQ();                      break;
                case ocSTEYX            : ScSTEXY();                    break;
                case ocSlope            : ScSlope();                    break;
                case ocIntercept        : ScIntercept();                break;
                case ocTrend            : ScTrend();                    break;
                case ocGrowth           : ScGrowth();                   break;
                case ocRGP              : ScRGP();                      break;
                case ocRKP              : ScRKP();                      break;
                case ocForecast         : ScForecast();                 break;
                case ocGammaLn          : ScLogGamma();                 break;
                case ocGamma            : ScGamma();                    break;
                case ocGammaDist        : ScGammaDist();                break;
                case ocGammaInv         : ScGammaInv();                 break;
                case ocChiTest          : ScChiTest();                  break;
                case ocChiInv           : ScChiInv();                   break;
                case ocChiSqInv         : ScChiSqInv();                 break;
                case ocTInv             : ScTInv();                     break;
                case ocFInv             : ScFInv();                     break;
                case ocLogInv           : ScLogNormInv();               break;
                case ocBetaDist         : ScBetaDist();                 break;
                case ocBetaInv          : ScBetaInv();                  break;
                case ocExternal         : ScExternal();                 break;
                case ocTableOp          : ScTableOp();                  break;
//              case ocErrCell          : ScErrCell();                  break;
                case ocStop :                                           break;
                case ocErrorType        : ScErrorType();                break;
                case ocCurrent          : ScCurrent();                  break;
                case ocStyle            : ScStyle();                    break;
                case ocDde              : ScDde();                      break;
                case ocBase             : ScBase();                     break;
                case ocDecimal          : ScDecimal();                  break;
                case ocConvert          : ScConvert();                  break;
                case ocEuroConvert      : ScEuroConvert();              break;
                case ocRoman            : ScRoman();                    break;
                case ocArabic           : ScArabic();                   break;
                case ocInfo             : ScInfo();                     break;
                case ocHyperLink        : ScHyperLink();                break;
                case ocBahtText         : ScBahtText();                 break;
                case ocGetPivotData     : ScGetPivotData();             break;
                case ocJis              : ScJis();                      break;
                case ocAsc              : ScAsc();                      break;
                case ocUnicode          : ScUnicode();                  break;
                case ocUnichar          : ScUnichar();                  break;
                case ocAnswer           : ScAnswer();                   break;
                case ocTeam             : ScCalcTeam();                 break;
                case ocTTT              : ScTTT();                      break;
                case ocSpew             : ScSpewFunc();                 break;
                case ocGame             : ScGame();                     break;
                case ocNone : nFuncFmtType = NUMBERFORMAT_UNDEFINED;    break;
                default : PushError( errUnknownOpCode);                 break;
            }

            // If the function signalled that it pushed a subroutine on the 
            // instruction code stack instead of a result, continue with  
            // execution of the subroutine.
            if (sp > nStackBase && pStack[sp-1]->GetOpCode() == ocCall)
            {
                Pop();
                continue;   // while( ( pCur = aCode.Next() ) != NULL  ...
            }

            // Remember result matrix in case it could be reused.
            if (pTokenMatrixMap && sp && GetStackType() == svMatrix)
                pTokenMatrixMap->insert( ScTokenMatrixMap::value_type( pCur,
                            pStack[sp-1]));

            // outer function determines format of an expression
            if ( nFuncFmtType != NUMBERFORMAT_UNDEFINED )
            {
                nRetTypeExpr = nFuncFmtType;
                // inherit the format index only for currency formats
                nRetIndexExpr = ( nFuncFmtType == NUMBERFORMAT_CURRENCY ?
                    nFuncFmtIndex : 0 );
            }
        }

        // Need a clean stack environment for the JumpMatrix to work.
        if (nGlobalError && eOp != ocPush && sp > nStackBase + 1)
        {
            // Not all functions pop all parameters in case an error is
            // generated. Clean up stack. Assumes that every function pushes a
            // result, may be arbitrary in case of error.
            const FormulaToken* pLocalResult = pStack[ sp - 1 ];
            while (sp > nStackBase)
                Pop();
            PushTempToken( *pLocalResult );
        }

        bool bGotResult;
        do
        {
            bGotResult = false;
            BYTE nLevel = 0;
            if ( GetStackType( ++nLevel ) == svJumpMatrix )
                ;   // nothing
            else if ( GetStackType( ++nLevel ) == svJumpMatrix )
                ;   // nothing
            else
                nLevel = 0;
            if ( nLevel == 1 || (nLevel == 2 && aCode.IsEndOfPath()) )
                bGotResult = JumpMatrix( nLevel );
            else
                pJumpMatrix = NULL;
        } while ( bGotResult );


// Functions that evaluate an error code and directly set nGlobalError to 0,
// usage: switch( OpCode ) { CASE_OCERRFUNC statements; }
#define CASE_OCERRFUNC \
    case ocCount : \
    case ocCount2 : \
    case ocErrorType : \
    case ocIsEmpty : \
    case ocIsErr : \
    case ocIsError : \
    case ocIsFormula : \
    case ocIsLogical : \
    case ocIsNA : \
    case ocIsNonString : \
    case ocIsRef : \
    case ocIsString : \
    case ocIsValue : \
    case ocN : \
    case ocType :

        switch ( eOp )
        {
            CASE_OCERRFUNC
                 ++ nErrorFunction;
            default:
                ;   // nothing
        }
        if ( nGlobalError )
        {
            if ( !nErrorFunctionCount )
            {   // count of errorcode functions in formula
                for ( FormulaToken* t = rArr.FirstRPN(); t; t = rArr.NextRPN() )
                {
                    switch ( t->GetOpCode() )
                    {
                        CASE_OCERRFUNC
                             ++nErrorFunctionCount;
                        default:
                            ;   // nothing
                    }
                }
            }
            if ( nErrorFunction >= nErrorFunctionCount )
                ++nErrorFunction;   // that's it, error => terminate
        }
    }

    // End: obtain result

    if( sp )
    {
        pCur = pStack[ sp-1 ];
        if( pCur->GetOpCode() == ocPush )
        {
            switch( pCur->GetType() )
            {
                case svEmptyCell:
                    ;   // nothing
                break;
                case svError:
                    nGlobalError = pCur->GetError();
                break;
                case svDouble :
                    if ( nFuncFmtType == NUMBERFORMAT_UNDEFINED )
                    {
                        nRetTypeExpr = NUMBERFORMAT_NUMBER;
                        nRetIndexExpr = 0;
                    }
                break;
                case svString :
                    nRetTypeExpr = NUMBERFORMAT_TEXT;
                    nRetIndexExpr = 0;
                break;
                case svSingleRef :
                {
                    ScAddress aAdr;
                    PopSingleRef( aAdr );
                    if( !nGlobalError )
                        PushCellResultToken( false, aAdr,
                                &nRetTypeExpr, &nRetIndexExpr);
                }
                break;
                case svRefList :
                    PopError();     // maybe #REF! takes precedence over #VALUE!
                    PushError( errNoValue);
                break;
                case svDoubleRef :
                {
                    if ( bMatrixFormula )
                    {   // create matrix for {=A1:A5}
                        PopDoubleRefPushMatrix();
                        // no break, continue with svMatrix
                    }
                    else
                    {
                        ScRange aRange;
                        PopDoubleRef( aRange );
                        ScAddress aAdr;
                        if ( !nGlobalError && DoubleRefToPosSingleRef( aRange, aAdr))
                            PushCellResultToken( false, aAdr,
                                    &nRetTypeExpr, &nRetIndexExpr);
                        break;
                    }
                }
                // no break
                case svMatrix :
                {
                    ScMatrixRef xMat = PopMatrix();
                    if (xMat)
                    {
                        ScMatValType nMatValType;
                        const ScMatrixValue* pMatVal = xMat->Get(0, 0, nMatValType);
                        if ( pMatVal )
                        {
                            if (ScMatrix::IsNonValueType( nMatValType))
                            {
                                if ( xMat->IsEmptyPath( 0, 0))
                                {   // result of empty FALSE jump path
                                    FormulaTokenRef xRes = new FormulaDoubleToken( 0.0);
                                    PushTempToken( new ScMatrixCellResultToken( xMat, xRes));
                                    nRetTypeExpr = NUMBERFORMAT_LOGICAL;
                                }
                                else
                                {
                                    String aStr( pMatVal->GetString());
                                    FormulaTokenRef xRes = new FormulaStringToken( aStr);
                                    PushTempToken( new ScMatrixCellResultToken( xMat, xRes));
                                    nRetTypeExpr = NUMBERFORMAT_TEXT;
                                }
                            }
                            else
                            {
                                USHORT nErr = GetDoubleErrorValue( pMatVal->fVal);
                                FormulaTokenRef xRes;
                                if (nErr)
                                    xRes = new FormulaErrorToken( nErr);
                                else
                                    xRes = new FormulaDoubleToken( pMatVal->fVal);
                                PushTempToken( new ScMatrixCellResultToken( xMat, xRes));
                                if ( nRetTypeExpr != NUMBERFORMAT_LOGICAL )
                                    nRetTypeExpr = NUMBERFORMAT_NUMBER;
                            }
                            nRetIndexExpr = 0;
                        }
                        else
                            SetError( errUnknownStackVariable);
                        xMat->SetErrorInterpreter( NULL);
                    }
                    else
                        SetError( errUnknownStackVariable);
                }
                break;
                default :
                    SetError( errUnknownStackVariable);
            }
        }
        else
            SetError( errUnknownStackVariable);
    }
    else
        SetError( errNoCode);

    if( nRetTypeExpr != NUMBERFORMAT_UNDEFINED )
    {
        nRetFmtType = nRetTypeExpr;
        nRetFmtIndex = nRetIndexExpr;
    }
    else if( nFuncFmtType != NUMBERFORMAT_UNDEFINED )
    {
        nRetFmtType = nFuncFmtType;
        nRetFmtIndex = nFuncFmtIndex;
    }
    else
        nRetFmtType = NUMBERFORMAT_NUMBER;
    // inherit the format index only for currency formats
    if ( nRetFmtType != NUMBERFORMAT_CURRENCY )
        nRetFmtIndex = 0;

    if (nGlobalError && GetStackType() != svError )
        PushError( nGlobalError);

    // THE final result.
    xResult = PopToken();
    if (!xResult)
        xResult = new FormulaErrorToken( errUnknownStackVariable);

    // release tokens in expression stack
    FormulaToken** p = pStack;
    while( maxsp-- )
        (*p++)->DecRef();

    return xResult->GetType();
}
