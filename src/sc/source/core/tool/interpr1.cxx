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

#include "scitems.hxx"
#include <svx/langitem.hxx>
#include <svx/algitem.hxx>
#include <unotools/textsearch.hxx>
#include <svtools/zforlist.hxx>
#include <svtools/zformat.hxx>
#include <tools/urlobj.hxx>
#include <unotools/charclass.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/printer.hxx>
#include <unotools/collatorwrapper.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <rtl/ustring.hxx>
#include <rtl/logfile.hxx>

#include "interpre.hxx"
#include "patattr.hxx"
#include "global.hxx"
#include "document.hxx"
#include "dociter.hxx"
#include "cell.hxx"
#include "scmatrix.hxx"
#include "docoptio.hxx"
#include "globstr.hrc"
#include "attrib.hxx"
#include "jumpmatrix.hxx"

#ifndef _COMPHELPER_PROCESSFACTORY_HXX_
#include <comphelper/processfactory.hxx>
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "cellkeytranslator.hxx"
#include "lookupcache.hxx"
#include "rangenam.hxx"
#include "compiler.hxx"
#include "externalrefmgr.hxx"

#define SC_DOUBLE_MAXVALUE  1.7e307

IMPL_FIXEDMEMPOOL_NEWDEL( ScTokenStack, 8, 4 )
IMPL_FIXEDMEMPOOL_NEWDEL( ScInterpreter, 32, 16 )

ScTokenStack* ScInterpreter::pGlobalStack = NULL;
BOOL ScInterpreter::bGlobalStackInUse = FALSE;

using namespace formula;
//-----------------------------------------------------------------------------
// Funktionen
//-----------------------------------------------------------------------------


void ScInterpreter::ScIfJump()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIfJump" );
    const short* pJump = pCur->GetJump();
    short nJumpCount = pJump[ 0 ];
    MatrixDoubleRefToMatrix();
    switch ( GetStackType() )
    {
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                PushIllegalParameter();
            else
            {
                FormulaTokenRef xNew;
                ScTokenMatrixMap::const_iterator aMapIter;
                // DoubleError handled by JumpMatrix
                pMat->SetErrorInterpreter( NULL);
                SCSIZE nCols, nRows;
                pMat->GetDimensions( nCols, nRows );
                if ( nCols == 0 || nRows == 0 )
                    PushIllegalArgument();
                else if (pTokenMatrixMap && ((aMapIter = pTokenMatrixMap->find(
                                    pCur)) != pTokenMatrixMap->end()))
                    xNew = (*aMapIter).second;
                else
                {
                    ScJumpMatrix* pJumpMat = new ScJumpMatrix( nCols, nRows );
                    for ( SCSIZE nC=0; nC < nCols; ++nC )
                    {
                        for ( SCSIZE nR=0; nR < nRows; ++nR )
                        {
                            double fVal;
                            bool bTrue;
                            ScMatValType nType = 0;
                            const ScMatrixValue* pMatVal = pMat->Get( nC, nR,
                                    nType);
                            bool bIsValue = ScMatrix::IsValueType( nType);
                            if ( bIsValue )
                            {
                                fVal = pMatVal->fVal;
                                bIsValue = ::rtl::math::isFinite( fVal );
                                bTrue = bIsValue && (fVal != 0.0);
                                if ( bTrue )
                                    fVal = 1.0;
                            }
                            else
                            {
                                // Treat empty and empty path as 0, but string
                                // as error.
                                bIsValue = !ScMatrix::IsRealStringType( nType);
                                bTrue = false;
                                fVal = (bIsValue ? 0.0 : CreateDoubleError( errNoValue));
                            }
                            if ( bTrue )
                            {   // TRUE
                                if( nJumpCount >= 2 )
                                {   // THEN path
                                    pJumpMat->SetJump( nC, nR, fVal,
                                            pJump[ 1 ],
                                            pJump[ nJumpCount ]);
                                }
                                else
                                {   // no parameter given for THEN
                                    pJumpMat->SetJump( nC, nR, fVal,
                                            pJump[ nJumpCount ],
                                            pJump[ nJumpCount ]);
                                }
                            }
                            else
                            {   // FALSE
                                if( nJumpCount == 3 && bIsValue )
                                {   // ELSE path
                                    pJumpMat->SetJump( nC, nR, fVal,
                                            pJump[ 2 ],
                                            pJump[ nJumpCount ]);
                                }
                                else
                                {   // no parameter given for ELSE,
                                    // or DoubleError
                                    pJumpMat->SetJump( nC, nR, fVal,
                                            pJump[ nJumpCount ],
                                            pJump[ nJumpCount ]);
                                }
                            }
                        }
                    }
                    xNew = new ScJumpMatrixToken( pJumpMat );
                    GetTokenMatrixMap().insert( ScTokenMatrixMap::value_type(pCur, xNew));
                }
                PushTempToken( xNew);
                // set endpoint of path for main code line
                aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
            }
        }
        break;
        default:
        {
            if ( GetBool() )
            {   // TRUE
                if( nJumpCount >= 2 )
                {   // THEN path
                    aCode.Jump( pJump[ 1 ], pJump[ nJumpCount ] );
                }
                else
                {   // no parameter given for THEN
                    nFuncFmtType = NUMBERFORMAT_LOGICAL;
                    PushInt(1);
                    aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
                }
            }
            else
            {   // FALSE
                if( nJumpCount == 3 )
                {   // ELSE path
                    aCode.Jump( pJump[ 2 ], pJump[ nJumpCount ] );
                }
                else
                {   // no parameter given for ELSE
                    nFuncFmtType = NUMBERFORMAT_LOGICAL;
                    PushInt(0);
                    aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
                }
            }
        }
    }
}


void ScInterpreter::ScChoseJump()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScChoseJump" );
    // We have to set a jump, if there was none chosen because of an error set
    // it to endpoint.
    bool bHaveJump = false;
    const short* pJump = pCur->GetJump();
    short nJumpCount = pJump[ 0 ];
    MatrixDoubleRefToMatrix();
    switch ( GetStackType() )
    {
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                PushIllegalParameter();
            else
            {
                FormulaTokenRef xNew;
                ScTokenMatrixMap::const_iterator aMapIter;
                // DoubleError handled by JumpMatrix
                pMat->SetErrorInterpreter( NULL);
                SCSIZE nCols, nRows;
                pMat->GetDimensions( nCols, nRows );
                if ( nCols == 0 || nRows == 0 )
                    PushIllegalParameter();
                else if (pTokenMatrixMap && ((aMapIter = pTokenMatrixMap->find(
                                    pCur)) != pTokenMatrixMap->end()))
                    xNew = (*aMapIter).second;
                else
                {
                    ScJumpMatrix* pJumpMat = new ScJumpMatrix( nCols, nRows );
                    for ( SCSIZE nC=0; nC < nCols; ++nC )
                    {
                        for ( SCSIZE nR=0; nR < nRows; ++nR )
                        {
                            double fVal;
                            ScMatValType nType;
                            const ScMatrixValue* pMatVal = pMat->Get( nC, nR,
                                    nType);
                            bool bIsValue = ScMatrix::IsValueType( nType);
                            if ( bIsValue )
                            {
                                fVal = pMatVal->fVal;
                                bIsValue = ::rtl::math::isFinite( fVal );
                                if ( bIsValue )
                                {
                                    fVal = ::rtl::math::approxFloor( fVal);
                                    if ( (fVal < 1) || (fVal >= nJumpCount))
                                    {
                                        bIsValue = FALSE;
                                        fVal = CreateDoubleError(
                                                errIllegalArgument);
                                    }
                                }
                            }
                            else
                            {
                                fVal = CreateDoubleError( errNoValue);
                            }
                            if ( bIsValue )
                            {
                                pJumpMat->SetJump( nC, nR, fVal,
                                        pJump[ (short)fVal ],
                                        pJump[ nJumpCount ]);
                            }
                            else
                            {
                                pJumpMat->SetJump( nC, nR, fVal,
                                        pJump[ nJumpCount ],
                                        pJump[ nJumpCount ]);
                            }
                        }
                    }
                    xNew = new ScJumpMatrixToken( pJumpMat );
                    GetTokenMatrixMap().insert( ScTokenMatrixMap::value_type(
                                pCur, xNew));
                }
                PushTempToken( xNew);
                // set endpoint of path for main code line
                aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
                bHaveJump = true;
            }
        }
        break;
        default:
        {
            double nJumpIndex = ::rtl::math::approxFloor( GetDouble() );
            if (!nGlobalError && (nJumpIndex >= 1) && (nJumpIndex < nJumpCount))
            {
                aCode.Jump( pJump[ (short) nJumpIndex ], pJump[ nJumpCount ] );
                bHaveJump = true;
            }
            else
                PushIllegalArgument();
        }
    }
    if (!bHaveJump)
        aCode.Jump( pJump[ nJumpCount ], pJump[ nJumpCount ] );
}

void lcl_AdjustJumpMatrix( ScJumpMatrix* pJumpM, ScMatrixRef& pResMat, SCSIZE nParmCols, SCSIZE nParmRows )
{
    SCSIZE nJumpCols, nJumpRows;
    SCSIZE nResCols, nResRows;
    SCSIZE nAdjustCols, nAdjustRows;
    pJumpM->GetDimensions( nJumpCols, nJumpRows );
    pJumpM->GetResMatDimensions( nResCols, nResRows );
    if (( nJumpCols == 1 && nParmCols > nResCols ) || 
        ( nJumpRows == 1 && nParmRows > nResRows ))
    {
        if ( nJumpCols == 1 && nJumpRows == 1 )
        {
            nAdjustCols = nParmCols > nResCols ? nParmCols : nResCols;
            nAdjustRows = nParmRows > nResRows ? nParmRows : nResRows;
        }
        else if ( nJumpCols == 1 )
        {
            nAdjustCols = nParmCols;
            nAdjustRows = nResRows;
        }
        else
        {
            nAdjustCols = nResCols;
            nAdjustRows = nParmRows;
        }
        pJumpM->SetNewResMat( nAdjustCols, nAdjustRows );
        pResMat = pJumpM->GetResultMatrix();
    }
}

bool ScInterpreter::JumpMatrix( short nStackLevel )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::JumpMatrix" );
    pJumpMatrix = static_cast<ScToken*>(pStack[sp-nStackLevel])->GetJumpMatrix();
    ScMatrixRef pResMat = pJumpMatrix->GetResultMatrix();
    SCSIZE nC, nR;
    if ( nStackLevel == 2 )
    {
        if ( aCode.HasStacked() )
            aCode.Pop();    // pop what Jump() pushed
        else
        {
            DBG_ERRORFILE( "ScInterpreter::JumpMatrix: pop goes the weasel" );
        }

        if ( !pResMat )
        {
            Pop();
            SetError( errUnknownStackVariable );
        }
        else
        {
            pJumpMatrix->GetPos( nC, nR );
            switch ( GetStackType() )
            {
                case svDouble:
                {
                    double fVal = GetDouble();
                    if ( nGlobalError )
                    {
                        fVal = CreateDoubleError( nGlobalError );
                        nGlobalError = 0;
                    }
                    pResMat->PutDouble( fVal, nC, nR );
                }
                break;
                case svString:
                {
                    const String& rStr = GetString();
                    if ( nGlobalError )
                    {
                        pResMat->PutDouble( CreateDoubleError( nGlobalError),
                                nC, nR);
                        nGlobalError = 0;
                    }
                    else
                        pResMat->PutString( rStr, nC, nR );
                }
                break;
                case svSingleRef:
                {
                    ScAddress aAdr;
                    PopSingleRef( aAdr );
                    if ( nGlobalError )
                    {
                        pResMat->PutDouble( CreateDoubleError( nGlobalError),
                                nC, nR);
                        nGlobalError = 0;
                    }
                    else
                    {
                        ScBaseCell* pCell = GetCell( aAdr );
                        if (HasCellEmptyData( pCell))
                            pResMat->PutEmpty( nC, nR );
                        else if (HasCellValueData( pCell))
                        {
                            double fVal = GetCellValue( aAdr, pCell);
                            if ( nGlobalError )
                            {
                                fVal = CreateDoubleError(
                                        nGlobalError);
                                nGlobalError = 0;
                            }
                            pResMat->PutDouble( fVal, nC, nR );
                        }
                        else
                        {
                            String aStr;
                            GetCellString( aStr, pCell );
                            if ( nGlobalError )
                            {
                                pResMat->PutDouble( CreateDoubleError(
                                            nGlobalError), nC, nR);
                                nGlobalError = 0;
                            }
                            else
                                pResMat->PutString( aStr, nC, nR);
                        }
                    }
                }
                break;
                case svDoubleRef:
                {   // upper left plus offset within matrix
                    double fVal;
                    ScRange aRange;
                    PopDoubleRef( aRange );
                    if ( nGlobalError )
                    {
                        fVal = CreateDoubleError( nGlobalError );
                        nGlobalError = 0;
                        pResMat->PutDouble( fVal, nC, nR );
                    }
                    else
                    {
                        // Do not modify the original range because we use it
                        // to adjust the size of the result matrix if necessary.
                        ScAddress aAdr( aRange.aStart);
                        ULONG nCol = (ULONG)aAdr.Col() + nC;
                        ULONG nRow = (ULONG)aAdr.Row() + nR;
                        if ((nCol > static_cast<ULONG>(aRange.aEnd.Col()) &&
                                    aRange.aEnd.Col() != aRange.aStart.Col())
                                || (nRow > static_cast<ULONG>(aRange.aEnd.Row()) &&
                                    aRange.aEnd.Row() != aRange.aStart.Row()))
                          {
                            fVal = CreateDoubleError( NOTAVAILABLE );
                            pResMat->PutDouble( fVal, nC, nR );
                          }
                          else
                          {
                            // Replicate column and/or row of a vector if it is
                            // one. Note that this could be a range reference
                            // that in fact consists of only one cell, e.g. A1:A1
                            if (aRange.aEnd.Col() == aRange.aStart.Col())
                                nCol = aRange.aStart.Col();
                            if (aRange.aEnd.Row() == aRange.aStart.Row())
                                nRow = aRange.aStart.Row();
                            aAdr.SetCol( static_cast<SCCOL>(nCol) );
                            aAdr.SetRow( static_cast<SCROW>(nRow) );
                            ScBaseCell* pCell = GetCell( aAdr );
                            if (HasCellEmptyData( pCell))
                                pResMat->PutEmpty( nC, nR );
                            else if (HasCellValueData( pCell))
                              {
                                double fCellVal = GetCellValue( aAdr, pCell);
                                if ( nGlobalError )
                                {
                                    fCellVal = CreateDoubleError(
                                            nGlobalError);
                                    nGlobalError = 0;
                                }
                                pResMat->PutDouble( fCellVal, nC, nR );
                              }
                              else
                            {
                                String aStr;
                                GetCellString( aStr, pCell );
                                if ( nGlobalError )
                                {
                                    pResMat->PutDouble( CreateDoubleError(
                                                nGlobalError), nC, nR);
                                    nGlobalError = 0;
                                }
                                else
                                    pResMat->PutString( aStr, nC, nR );
                            }
                          }
                        SCSIZE nParmCols = aRange.aEnd.Col() - aRange.aStart.Col() + 1;
                        SCSIZE nParmRows = aRange.aEnd.Row() - aRange.aStart.Row() + 1;
                        lcl_AdjustJumpMatrix( pJumpMatrix, pResMat, nParmCols, nParmRows );
                    }
                }
                break;
                case svMatrix:
                {   // match matrix offsets
                    double fVal;
                    ScMatrixRef pMat = PopMatrix();
                    if ( nGlobalError )
                    {
                        fVal = CreateDoubleError( nGlobalError );
                        nGlobalError = 0;
                        pResMat->PutDouble( fVal, nC, nR );
                    }
                    else if ( !pMat )
                    {
                        fVal = CreateDoubleError( errUnknownVariable );
                        pResMat->PutDouble( fVal, nC, nR );
                    }
                    else
                    {
                        SCSIZE nCols, nRows;
                        pMat->GetDimensions( nCols, nRows );
                        if ((nCols <= nC && nCols != 1) ||
                            (nRows <= nR && nRows != 1))
                        {
                            fVal = CreateDoubleError( NOTAVAILABLE );
                            pResMat->PutDouble( fVal, nC, nR );
                        }
                        else
                        {
                            if ( pMat->IsValue( nC, nR ) )
                            {
                                fVal = pMat->GetDouble( nC, nR );
                                pResMat->PutDouble( fVal, nC, nR );
                            }
                            else if ( pMat->IsEmpty( nC, nR ) )
                                pResMat->PutEmpty( nC, nR );
                            else
                            {
                                const String& rStr = pMat->GetString( nC, nR );
                                pResMat->PutString( rStr, nC, nR );
                            }
                        }
                        lcl_AdjustJumpMatrix( pJumpMatrix, pResMat, nCols, nRows );
                    }
                }
                break;
                case svError:
                {
                    PopError();
                    double fVal = CreateDoubleError( nGlobalError);
                    nGlobalError = 0;
                    pResMat->PutDouble( fVal, nC, nR );
                }
                break;
                default:
                {
                    Pop();
                    double fVal = CreateDoubleError( errIllegalArgument);
                    pResMat->PutDouble( fVal, nC, nR );
                }
            }
        }
    }
    bool bCont = pJumpMatrix->Next( nC, nR );
    if ( bCont )
    {
        double fBool;
        short nStart, nNext, nStop;
        pJumpMatrix->GetJump( nC, nR, fBool, nStart, nNext, nStop );
        while ( bCont && nStart == nNext )
        {   // push all results that have no jump path
            if ( pResMat )
            {
                // a FALSE without path results in an empty path value
                if ( fBool == 0.0 )
                    pResMat->PutEmptyPath( nC, nR );
                else
                    pResMat->PutDouble( fBool, nC, nR );
            }
            bCont = pJumpMatrix->Next( nC, nR );
            if ( bCont )
                pJumpMatrix->GetJump( nC, nR, fBool, nStart, nNext, nStop );
        }
        if ( bCont && nStart != nNext )
        {
            const ScTokenVec* pParams = pJumpMatrix->GetJumpParameters();
            if ( pParams )
            {
                for ( ScTokenVec::const_iterator i = pParams->begin();
                        i != pParams->end(); ++i )
                {
                    // This is not the current state of the interpreter, so
                    // push without error, and elements' errors are coded into
                    // double.
                    PushWithoutError( *(*i));
                }
            }
            aCode.Jump( nStart, nNext, nStop );
        }
    }
    if ( !bCont )
    {   // we're done with it, throw away jump matrix, keep result
        pJumpMatrix = NULL;
        Pop();
        PushMatrix( pResMat );
        // Remove jump matrix from map and remember result matrix in case it
        // could be reused in another path of the same condition.
        if (pTokenMatrixMap)
        {
            pTokenMatrixMap->erase( pCur);
            pTokenMatrixMap->insert( ScTokenMatrixMap::value_type( pCur,
                        pStack[sp-1]));
        }
        return true;
    }
    return false;
}


ScCompareOptions::ScCompareOptions( ScDocument* pDoc, const ScQueryEntry& rEntry, bool bReg ) :
    aQueryEntry(rEntry),
    bRegEx(bReg),
    bMatchWholeCell(pDoc->GetDocOptions().IsMatchWholeCell()),
    bIgnoreCase(true)
{
    bRegEx = (bRegEx && (aQueryEntry.eOp == SC_EQUAL || aQueryEntry.eOp == SC_NOT_EQUAL));
    // Interpreter functions usually are case insensitive, except the simple 
    // comparison operators, for which these options aren't used. Override in 
    // struct if needed.
}


double ScInterpreter::CompareFunc( const ScCompare& rComp, ScCompareOptions* pOptions )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::CompareFunc" );
    // Keep DoubleError if encountered
    // #i40539# if bEmpty is set, bVal/nVal are uninitialized
    if ( !rComp.bEmpty[0] && rComp.bVal[0] && !::rtl::math::isFinite( rComp.nVal[0]))
        return rComp.nVal[0];
    if ( !rComp.bEmpty[1] && rComp.bVal[1] && !::rtl::math::isFinite( rComp.nVal[1]))
        return rComp.nVal[1];

    double fRes = 0;
    if ( rComp.bEmpty[ 0 ] )
    {
        if ( rComp.bEmpty[ 1 ] )
            ;       // empty cell == empty cell, fRes 0
        else if( rComp.bVal[ 1 ] )
        {
            if ( !::rtl::math::approxEqual( rComp.nVal[ 1 ], 0.0 ) )
            {
                if ( rComp.nVal[ 1 ] < 0.0 )
                    fRes = 1;       // empty cell > -x
                else
                    fRes = -1;      // empty cell < x
            }
            // else: empty cell == 0.0
        }
        else
        {
            if ( rComp.pVal[ 1 ]->Len() )
                fRes = -1;      // empty cell < "..."
            // else: empty cell == ""
        }
    }
    else if ( rComp.bEmpty[ 1 ] )
    {
        if( rComp.bVal[ 0 ] )
        {
            if ( !::rtl::math::approxEqual( rComp.nVal[ 0 ], 0.0 ) )
            {
                if ( rComp.nVal[ 0 ] < 0.0 )
                    fRes = -1;      // -x < empty cell
                else
                    fRes = 1;       // x > empty cell
            }
            // else: empty cell == 0.0
        }
        else
        {
            if ( rComp.pVal[ 0 ]->Len() )
                fRes = 1;       // "..." > empty cell
            // else: "" == empty cell
        }
    }
    else if( rComp.bVal[ 0 ] )
    {
        if( rComp.bVal[ 1 ] )
        {
            if ( !::rtl::math::approxEqual( rComp.nVal[ 0 ], rComp.nVal[ 1 ] ) )
            {
                if( rComp.nVal[ 0 ] - rComp.nVal[ 1 ] < 0 )
                    fRes = -1;
                else
                    fRes = 1;
            }
        }
        else
            fRes = -1;  // number is less than string
    }
    else if( rComp.bVal[ 1 ] )
        fRes = 1;   // number is less than string
    else
    {
        // Both strings.
        if (pOptions)
        {
            // All similar to Sctable::ValidQuery(), *rComp.pVal[1] actually 
            // is/must be identical to *rEntry.pStr, which is essential for 
            // regex to work through GetSearchTextPtr().
            ScQueryEntry& rEntry = pOptions->aQueryEntry;
            DBG_ASSERT( *rComp.pVal[1] == *rEntry.pStr, "ScInterpreter::CompareFunc: broken options");
            if (pOptions->bRegEx)
            {
                xub_StrLen nStart = 0;
                xub_StrLen nStop  = rComp.pVal[0]->Len();
                bool bMatch = rEntry.GetSearchTextPtr( 
                        !pOptions->bIgnoreCase)->SearchFrwrd( *rComp.pVal[0], 
                            &nStart, &nStop);
                if (bMatch && pOptions->bMatchWholeCell && (nStart != 0 || nStop != rComp.pVal[0]->Len()))
                    bMatch = false;     // RegEx must match entire string.
                fRes = (bMatch ? 0 : 1);
            }
            else if (rEntry.eOp == SC_EQUAL || rEntry.eOp == SC_NOT_EQUAL)
            {
                ::utl::TransliterationWrapper* pTransliteration = 
                    (pOptions->bIgnoreCase ? ScGlobal::GetpTransliteration() : 
                     ScGlobal::GetCaseTransliteration());
                bool bMatch;
                if (pOptions->bMatchWholeCell)
                    bMatch = pTransliteration->isEqual( *rComp.pVal[0], *rComp.pVal[1]);
                else
                {
                    String aCell( pTransliteration->transliterate( 
                                *rComp.pVal[0], ScGlobal::eLnge, 0, 
                                rComp.pVal[0]->Len(), NULL));
                    String aQuer( pTransliteration->transliterate( 
                                *rComp.pVal[1], ScGlobal::eLnge, 0, 
                                rComp.pVal[1]->Len(), NULL));
                    bMatch = (aCell.Search( aQuer ) != STRING_NOTFOUND);
                }
                fRes = (bMatch ? 0 : 1);
            }
            else if (pOptions->bIgnoreCase)
                fRes = (double) ScGlobal::GetCollator()->compareString(
                        *rComp.pVal[ 0 ], *rComp.pVal[ 1 ] );
            else
                fRes = (double) ScGlobal::GetCaseCollator()->compareString(
                        *rComp.pVal[ 0 ], *rComp.pVal[ 1 ] );
        }
        else if (pDok->GetDocOptions().IsIgnoreCase())
            fRes = (double) ScGlobal::GetCollator()->compareString(
                *rComp.pVal[ 0 ], *rComp.pVal[ 1 ] );
        else
            fRes = (double) ScGlobal::GetCaseCollator()->compareString(
                *rComp.pVal[ 0 ], *rComp.pVal[ 1 ] );
    }
    return fRes;
}


double ScInterpreter::Compare()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::Compare" );
    String aVal1, aVal2;
    ScCompare aComp( &aVal1, &aVal2 );
    for( short i = 1; i >= 0; i-- )
    {
        switch ( GetRawStackType() )
        {
            case svEmptyCell:
                Pop();
                aComp.bEmpty[ i ] = TRUE;
                break;
            case svMissing:
            case svDouble:
                aComp.nVal[ i ] = GetDouble();
                aComp.bVal[ i ] = TRUE;
                break;
            case svString:
                *aComp.pVal[ i ] = GetString();
                aComp.bVal[ i ] = FALSE;
                break;
            case svDoubleRef :
            case svSingleRef :
            {
                ScAddress aAdr;
                if ( !PopDoubleRefOrSingleRef( aAdr ) )
                    break;
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellEmptyData( pCell))
                    aComp.bEmpty[ i ] = TRUE;
                else if (HasCellStringData( pCell))
                {
                    GetCellString( *aComp.pVal[ i ], pCell);
                    aComp.bVal[ i ] = FALSE;
                }
                else
                {
                    aComp.nVal[ i ] = GetCellValue( aAdr, pCell );
                    aComp.bVal[ i ] = TRUE;
                }
            }
            break;
            default:
                SetError( errIllegalParameter);
            break;
        }
    }
    if( nGlobalError )
        return 0;
    nCurFmtType = nFuncFmtType = NUMBERFORMAT_LOGICAL;
    return CompareFunc( aComp );
}


ScMatrixRef ScInterpreter::CompareMat( ScCompareOptions* pOptions )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::CompareMat" );
    String aVal1, aVal2;
    ScCompare aComp( &aVal1, &aVal2 );
    ScMatrixRef pMat[2];
    ScAddress aAdr;
    for( short i = 1; i >= 0; i-- )
    {
        switch (GetRawStackType())
        {
            case svEmptyCell:
                Pop();
                aComp.bEmpty[ i ] = TRUE;
                break;
            case svMissing:
            case svDouble:
                aComp.nVal[ i ] = GetDouble();
                aComp.bVal[ i ] = TRUE;
                break;
            case svString:
                *aComp.pVal[ i ] = GetString();
                aComp.bVal[ i ] = FALSE;
                break;
            case svSingleRef:
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellEmptyData( pCell))
                    aComp.bEmpty[ i ] = TRUE;
                else if (HasCellStringData( pCell))
                {
                    GetCellString( *aComp.pVal[ i ], pCell);
                    aComp.bVal[ i ] = FALSE;
                }
                else
                {
                    aComp.nVal[ i ] = GetCellValue( aAdr, pCell );
                    aComp.bVal[ i ] = TRUE;
                }
            }
            break;
            case svDoubleRef:
            case svMatrix:
                pMat[ i ] = GetMatrix();
                if ( !pMat[ i ] )
                    SetError( errIllegalParameter);
                else
                    pMat[i]->SetErrorInterpreter( NULL);
                    // errors are transported as DoubleError inside matrix
                break;
            default:
                SetError( errIllegalParameter);
            break;
        }
    }
    ScMatrixRef pResMat = NULL;
    if( !nGlobalError )
    {
        if ( pMat[0] && pMat[1] )
        {
            SCSIZE nC0, nC1;
            SCSIZE nR0, nR1;
            pMat[0]->GetDimensions( nC0, nR0 );
            pMat[1]->GetDimensions( nC1, nR1 );
            SCSIZE nC = Max( nC0, nC1 );
            SCSIZE nR = Max( nR0, nR1 );
            pResMat = GetNewMat( nC, nR);
            if ( !pResMat )
                return NULL;
            for ( SCSIZE j=0; j<nC; j++ )
            {
                for ( SCSIZE k=0; k<nR; k++ )
                {
                    SCSIZE nCol = j, nRow = k;
                    if (    pMat[0]->ValidColRowOrReplicated( nCol, nRow ) &&
                            pMat[1]->ValidColRowOrReplicated( nCol, nRow ))
                    {
                        for ( short i=1; i>=0; i-- )
                        {
                            if ( pMat[i]->IsString(j,k) )
                            {
                                aComp.bVal[i] = FALSE;
                                *aComp.pVal[i] = pMat[i]->GetString(j,k);
                                aComp.bEmpty[i] = pMat[i]->IsEmpty(j,k);
                            }
                            else
                            {
                                aComp.bVal[i] = TRUE;
                                aComp.nVal[i] = pMat[i]->GetDouble(j,k);
                                aComp.bEmpty[i] = FALSE;
                            }
                        }
                        pResMat->PutDouble( CompareFunc( aComp, pOptions ), j,k );
                    }
                    else
                        pResMat->PutString( ScGlobal::GetRscString(STR_NO_VALUE), j,k );
                }
            }
        }
        else if ( pMat[0] || pMat[1] )
        {
            short i = ( pMat[0] ? 0 : 1);
            SCSIZE nC, nR;
            pMat[i]->GetDimensions( nC, nR );
            pResMat = GetNewMat( nC, nR);
            if ( !pResMat )
                return NULL;
            SCSIZE n = nC * nR;
            for ( SCSIZE j=0; j<n; j++ )
            {
                if ( pMat[i]->IsValue(j) )
                {
                    aComp.bVal[i] = TRUE;
                    aComp.nVal[i] = pMat[i]->GetDouble(j);
                    aComp.bEmpty[i] = FALSE;
                }
                else
                {
                    aComp.bVal[i] = FALSE;
                    *aComp.pVal[i] = pMat[i]->GetString(j);
                    aComp.bEmpty[i] = pMat[i]->IsEmpty(j);
                }
                pResMat->PutDouble( CompareFunc( aComp, pOptions ), j );
            }
        }
    }
    nCurFmtType = nFuncFmtType = NUMBERFORMAT_LOGICAL;
    return pResMat;
}


ScMatrixRef ScInterpreter::QueryMat( ScMatrix* pMat, ScCompareOptions& rOptions )
{
    short nSaveCurFmtType = nCurFmtType;
    short nSaveFuncFmtType = nFuncFmtType;
    PushMatrix( pMat);
    if (rOptions.aQueryEntry.bQueryByString)
        PushString( *rOptions.aQueryEntry.pStr);
    else
        PushDouble( rOptions.aQueryEntry.nVal);
    ScMatrixRef pResultMatrix = CompareMat( &rOptions);
    nCurFmtType = nSaveCurFmtType;
    nFuncFmtType = nSaveFuncFmtType;
    if (nGlobalError || !pResultMatrix)
    {
        SetError( errIllegalParameter);
        return pResultMatrix;
    }

    switch (rOptions.aQueryEntry.eOp)
    {
        case SC_EQUAL:
            pResultMatrix->CompareEqual();
            break;
        case SC_LESS:
            pResultMatrix->CompareLess();
            break;
        case SC_GREATER:
            pResultMatrix->CompareGreater();
            break;
        case SC_LESS_EQUAL:
            pResultMatrix->CompareLessEqual();
            break;
        case SC_GREATER_EQUAL:
            pResultMatrix->CompareGreaterEqual();
            break;
        case SC_NOT_EQUAL:
            pResultMatrix->CompareNotEqual();
            break;
        default:
            SetError( errIllegalArgument);
            DBG_ERROR1( "ScInterpreter::QueryMat: unhandled comparison operator: %d", (int)rOptions.aQueryEntry.eOp);
    }
    return pResultMatrix;
}


void ScInterpreter::ScEqual()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScEqual" );
    if ( GetStackType(1) == svMatrix || GetStackType(2) == svMatrix )
    {
        ScMatrixRef pMat = CompareMat();
        if ( !pMat )
            PushIllegalParameter();
        else
        {
            pMat->CompareEqual();
            PushMatrix( pMat );
        }
    }
    else
        PushInt( Compare() == 0 );
}


void ScInterpreter::ScNotEqual()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScNotEqual" );
    if ( GetStackType(1) == svMatrix || GetStackType(2) == svMatrix )
    {
        ScMatrixRef pMat = CompareMat();
        if ( !pMat )
            PushIllegalParameter();
        else
        {
            pMat->CompareNotEqual();
            PushMatrix( pMat );
        }
    }
    else
        PushInt( Compare() != 0 );
}


void ScInterpreter::ScLess()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScLess" );
    if ( GetStackType(1) == svMatrix || GetStackType(2) == svMatrix )
    {
        ScMatrixRef pMat = CompareMat();
        if ( !pMat )
            PushIllegalParameter();
        else
        {
            pMat->CompareLess();
            PushMatrix( pMat );
        }
    }
    else
        PushInt( Compare() < 0 );
}


void ScInterpreter::ScGreater()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScGreater" );
    if ( GetStackType(1) == svMatrix || GetStackType(2) == svMatrix )
    {
        ScMatrixRef pMat = CompareMat();
        if ( !pMat )
            PushIllegalParameter();
        else
        {
            pMat->CompareGreater();
            PushMatrix( pMat );
        }
    }
    else
        PushInt( Compare() > 0 );
}


void ScInterpreter::ScLessEqual()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScLessEqual" );
    if ( GetStackType(1) == svMatrix || GetStackType(2) == svMatrix )
    {
        ScMatrixRef pMat = CompareMat();
        if ( !pMat )
            PushIllegalParameter();
        else
        {
            pMat->CompareLessEqual();
            PushMatrix( pMat );
        }
    }
    else
        PushInt( Compare() <= 0 );
}


void ScInterpreter::ScGreaterEqual()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScGreaterEqual" );
    if ( GetStackType(1) == svMatrix || GetStackType(2) == svMatrix )
    {
        ScMatrixRef pMat = CompareMat();
        if ( !pMat )
            PushIllegalParameter();
        else
        {
            pMat->CompareGreaterEqual();
            PushMatrix( pMat );
        }
    }
    else
        PushInt( Compare() >= 0 );
}


void ScInterpreter::ScAnd()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScAnd" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nParamCount = GetByte();
    if ( MustHaveParamCountMin( nParamCount, 1 ) )
    {
        BOOL bHaveValue = FALSE;
        short nRes = TRUE;
        size_t nRefInList = 0;
        while( nParamCount-- > 0)
        {
            if ( !nGlobalError )
            {
                switch ( GetStackType() )
                {
                    case svDouble :
                        bHaveValue = TRUE;
                        nRes &= ( PopDouble() != 0.0 );
                    break;
                    case svString :
                        Pop();
                        SetError( errNoValue );
                    break;
                    case svSingleRef :
                    {
                        ScAddress aAdr;
                        PopSingleRef( aAdr );
                        if ( !nGlobalError )
                        {
                            ScBaseCell* pCell = GetCell( aAdr );
                            if ( HasCellValueData( pCell ) )
                            {
                                bHaveValue = TRUE;
                                nRes &= ( GetCellValue( aAdr, pCell ) != 0.0 );
                            }
                            // else: Xcl setzt hier keinen Fehler
                        }
                    }
                    break;
                    case svDoubleRef:
                    case svRefList:
                    {
                        ScRange aRange;
                        PopDoubleRef( aRange, nParamCount, nRefInList);
                        if ( !nGlobalError )
                        {
                            double fVal;
                            USHORT nErr = 0;
                            ScValueIterator aValIter( pDok, aRange );
                            if ( aValIter.GetFirst( fVal, nErr ) )
                            {
                                bHaveValue = TRUE;
                                do
                                {
                                    nRes &= ( fVal != 0.0 );
                                } while ( (nErr == 0) &&
                                    aValIter.GetNext( fVal, nErr ) );
                            }
                            SetError( nErr );
                        }
                    }
                    break;
                    case svMatrix:
                    {
                        ScMatrixRef pMat = GetMatrix();
                        if ( pMat )
                        {
                            bHaveValue = TRUE;
                            double fVal = pMat->And();
                            USHORT nErr = GetDoubleErrorValue( fVal );
                            if ( nErr )
                            {
                                SetError( nErr );
                                nRes = FALSE;
                            }
                            else
                                nRes &= (fVal != 0.0);
                        }
                        // else: GetMatrix did set errIllegalParameter
                    }
                    break;
                    default:
                        Pop();
                        SetError( errIllegalParameter);
                }
            }
            else
                Pop();
        }
        if ( bHaveValue )
            PushInt( nRes );
        else
            PushNoValue();
    }
}


void ScInterpreter::ScOr()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScOr" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nParamCount = GetByte();
    if ( MustHaveParamCountMin( nParamCount, 1 ) )
    {
        BOOL bHaveValue = FALSE;
        short nRes = FALSE;
        size_t nRefInList = 0;
        while( nParamCount-- > 0)
        {
            if ( !nGlobalError )
            {
                switch ( GetStackType() )
                {
                    case svDouble :
                        bHaveValue = TRUE;
                        nRes |= ( PopDouble() != 0.0 );
                    break;
                    case svString :
                        Pop();
                        SetError( errNoValue );
                    break;
                    case svSingleRef :
                    {
                        ScAddress aAdr;
                        PopSingleRef( aAdr );
                        if ( !nGlobalError )
                        {
                            ScBaseCell* pCell = GetCell( aAdr );
                            if ( HasCellValueData( pCell ) )
                            {
                                bHaveValue = TRUE;
                                nRes |= ( GetCellValue( aAdr, pCell ) != 0.0 );
                            }
                            // else: Xcl setzt hier keinen Fehler
                        }
                    }
                    break;
                    case svDoubleRef:
                    case svRefList:
                    {
                        ScRange aRange;
                        PopDoubleRef( aRange, nParamCount, nRefInList);
                        if ( !nGlobalError )
                        {
                            double fVal;
                            USHORT nErr = 0;
                            ScValueIterator aValIter( pDok, aRange );
                            if ( aValIter.GetFirst( fVal, nErr ) )
                            {
                                bHaveValue = TRUE;
                                do
                                {
                                    nRes |= ( fVal != 0.0 );
                                } while ( (nErr == 0) &&
                                    aValIter.GetNext( fVal, nErr ) );
                            }
                            SetError( nErr );
                        }
                    }
                    break;
                    case svMatrix:
                    {
                        bHaveValue = TRUE;
                        ScMatrixRef pMat = GetMatrix();
                        if ( pMat )
                        {
                            bHaveValue = TRUE;
                            double fVal = pMat->Or();
                            USHORT nErr = GetDoubleErrorValue( fVal );
                            if ( nErr )
                            {
                                SetError( nErr );
                                nRes = FALSE;
                            }
                            else
                                nRes |= (fVal != 0.0);
                        }
                        // else: GetMatrix did set errIllegalParameter
                    }
                    break;
                    default:
                        Pop();
                        SetError( errIllegalParameter);
                }
            }
            else
                Pop();
        }
        if ( bHaveValue )
            PushInt( nRes );
        else
            PushNoValue();
    }
}


void ScInterpreter::ScNeg()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScNeg" );
    // Simple negation doesn't change current format type to number, keep
    // current type.
    nFuncFmtType = nCurFmtType;
    switch ( GetStackType() )
    {
        case svMatrix :
        {
            ScMatrixRef pMat = GetMatrix();
            if ( !pMat )
                PushIllegalParameter();
            else
            {
                SCSIZE nC, nR;
                pMat->GetDimensions( nC, nR );
                ScMatrixRef pResMat = GetNewMat( nC, nR);
                if ( !pResMat )
                    PushIllegalArgument();
                else
                {
                    SCSIZE nCount = nC * nR;
                    for ( SCSIZE j=0; j<nCount; ++j )
                    {
                        if ( pMat->IsValueOrEmpty(j) )
                            pResMat->PutDouble( -pMat->GetDouble(j), j );
                        else
                            pResMat->PutString(
                                ScGlobal::GetRscString( STR_NO_VALUE ), j );
                    }
                    PushMatrix( pResMat );
                }
            }
        }
        break;
        default:
            PushDouble( -GetDouble() );
    }
}


void ScInterpreter::ScPercentSign()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScPercentSign" );
    nFuncFmtType = NUMBERFORMAT_PERCENT;
    const FormulaToken* pSaveCur = pCur;
    BYTE nSavePar = cPar;
    PushInt( 100 );
    cPar = 2;
    FormulaByteToken aDivOp( ocDiv, cPar );
    pCur = &aDivOp;
    ScDiv();
    pCur = pSaveCur;
    cPar = nSavePar;
}


void ScInterpreter::ScNot()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScNot" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    switch ( GetStackType() )
    {
        case svMatrix :
        {
            ScMatrixRef pMat = GetMatrix();
            if ( !pMat )
                PushIllegalParameter();
            else
            {
                SCSIZE nC, nR;
                pMat->GetDimensions( nC, nR );
                ScMatrixRef pResMat = GetNewMat( nC, nR);
                if ( !pResMat )
                    PushIllegalArgument();
                else
                {
                    SCSIZE nCount = nC * nR;
                    for ( SCSIZE j=0; j<nCount; ++j )
                    {
                        if ( pMat->IsValueOrEmpty(j) )
                            pResMat->PutDouble( (pMat->GetDouble(j) == 0.0), j );
                        else
                            pResMat->PutString(
                                ScGlobal::GetRscString( STR_NO_VALUE ), j );
                    }
                    PushMatrix( pResMat );
                }
            }
        }
        break;
        default:
            PushInt( GetDouble() == 0.0 );
    }
}


void ScInterpreter::ScPi()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScPi" );
    PushDouble(F_PI);
}


void ScInterpreter::ScRandom()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScRandom" );
    PushDouble((double)rand() / ((double)RAND_MAX+1.0));
}


void ScInterpreter::ScTrue()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTrue" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    PushInt(1);
}


void ScInterpreter::ScFalse()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScFalse" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    PushInt(0);
}


void ScInterpreter::ScDeg()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDeg" );
    PushDouble((GetDouble() / F_PI) * 180.0);
}


void ScInterpreter::ScRad()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScRad" );
    PushDouble(GetDouble() * (F_PI / 180));
}


void ScInterpreter::ScSin()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSin" );
    PushDouble(::rtl::math::sin(GetDouble()));
}


void ScInterpreter::ScCos()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCos" );
    PushDouble(::rtl::math::cos(GetDouble()));
}


void ScInterpreter::ScTan()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTan" );
    PushDouble(::rtl::math::tan(GetDouble()));
}


void ScInterpreter::ScCot()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCot" );
    PushDouble(1.0 / ::rtl::math::tan(GetDouble()));
}


void ScInterpreter::ScArcSin()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcSin" );
    PushDouble(asin(GetDouble()));
}


void ScInterpreter::ScArcCos()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcCos" );
    PushDouble(acos(GetDouble()));
}


void ScInterpreter::ScArcTan()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcTan" );
    PushDouble(atan(GetDouble()));
}


void ScInterpreter::ScArcCot()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcCot" );
    PushDouble((F_PI2) - atan(GetDouble()));
}


void ScInterpreter::ScSinHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSinHyp" );
    PushDouble(sinh(GetDouble()));
}


void ScInterpreter::ScCosHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCosHyp" );
    PushDouble(cosh(GetDouble()));
}


void ScInterpreter::ScTanHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTanHyp" );
    PushDouble(tanh(GetDouble()));
}


void ScInterpreter::ScCotHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCotHyp" );
    PushDouble(1.0 / tanh(GetDouble()));
}


void ScInterpreter::ScArcSinHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcSinHyp" );
    PushDouble( ::rtl::math::asinh( GetDouble()));
}

void ScInterpreter::ScArcCosHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcCosHyp" );
    double fVal = GetDouble();
    if (fVal < 1.0)
        PushIllegalArgument();
    else
        PushDouble( ::rtl::math::acosh( fVal));
}

void ScInterpreter::ScArcTanHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcTanHyp" );
    double fVal = GetDouble();
    if (fabs(fVal) >= 1.0)
        PushIllegalArgument();
    else
        PushDouble( ::rtl::math::atanh( fVal));
}


void ScInterpreter::ScArcCotHyp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScArcCotHyp" );
    double nVal = GetDouble();
    if (fabs(nVal) <= 1.0)
        PushIllegalArgument();
    else
        PushDouble(0.5 * log((nVal + 1.0) / (nVal - 1.0)));
}


void ScInterpreter::ScExp()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScExp" );
    PushDouble(exp(GetDouble()));
}


void ScInterpreter::ScSqrt()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSqrt" );
    double fVal = GetDouble();
    if (fVal >= 0.0)
        PushDouble(sqrt(fVal));
    else
        PushIllegalArgument();
}


void ScInterpreter::ScIsEmpty()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsEmpty" );
    short nRes = 0;
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    switch ( GetRawStackType() )
    {
        case svEmptyCell:
        {
            FormulaTokenRef p = PopToken();
            if (!static_cast<const ScEmptyCellToken*>(p.get())->IsInherited())
                nRes = 1;
        }
        break;
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            // NOTE: this could test also on inherited emptiness, but then the
            // cell tested wouldn't be empty. Must correspond with
            // ScCountEmptyCells().
            // if (HasCellEmptyData( GetCell( aAdr)))
            CellType eCellType = GetCellType( GetCell( aAdr ) );
            if((eCellType == CELLTYPE_NONE) || (eCellType == CELLTYPE_NOTE))
                nRes = 1;
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
                nRes = pMat->IsEmpty( 0 );
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                    nRes = pMat->IsEmpty( nC, nR);
                // else: FALSE, not empty (which is what Xcl does)
            }
        }
        break;
        default:
            Pop();
    }
    nGlobalError = 0;
    PushInt( nRes );
}


short ScInterpreter::IsString()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::IsString" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    switch ( GetRawStackType() )
    {
        case svString:
            Pop();
            nRes = 1;
        break;
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            ScBaseCell* pCell = GetCell( aAdr );
            if (GetCellErrCode( pCell ) == 0)
            {
                switch ( GetCellType( pCell ) )
                {
                    case CELLTYPE_STRING :
                    case CELLTYPE_EDIT :
                        nRes = 1;
                        break;
                    case CELLTYPE_FORMULA :
                        nRes = !((ScFormulaCell*)pCell)->IsValue() &&
                            !((ScFormulaCell*)pCell)->IsEmpty();
                        break;
                    default:
                        ; // nothing
                }
            }
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
                nRes = pMat->IsString(0) && !pMat->IsEmpty(0);
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                    nRes = pMat->IsString( nC, nR) && !pMat->IsEmpty( nC, nR);
            }
        }
        break;
        default:
            Pop();
    }
    nGlobalError = 0;
    return nRes;
}


void ScInterpreter::ScIsString()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsString" );
    PushInt( IsString() );
}


void ScInterpreter::ScIsNonString()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsNonString" );
    PushInt( !IsString() );
}


void ScInterpreter::ScIsLogical()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsLogical" );
    short nRes = 0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            ScBaseCell* pCell = GetCell( aAdr );
            if (GetCellErrCode( pCell ) == 0)
            {
                if (HasCellValueData(pCell))
                {
                    ULONG nFormat = GetCellNumberFormat( aAdr, pCell );
                    nRes = ( pFormatter->GetType(nFormat)
                                                 == NUMBERFORMAT_LOGICAL);
                }
            }
        }
        break;
        case svMatrix:
            // TODO: we don't have type information for arrays except
            // numerical/string.
        // Fall thru
        default:
            PopError();
            if ( !nGlobalError )
                nRes = ( nCurFmtType == NUMBERFORMAT_LOGICAL );
    }
    nCurFmtType = nFuncFmtType = NUMBERFORMAT_LOGICAL;
    nGlobalError = 0;
    PushInt( nRes );
}


void ScInterpreter::ScType()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScType" );
    short nType = 0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            ScBaseCell* pCell = GetCell( aAdr );
            if (GetCellErrCode( pCell ) == 0)
            {
                switch ( GetCellType( pCell ) )
                {
                    // NOTE: this is Xcl nonsense!
                    case CELLTYPE_NOTE :
                        nType = 1;      // empty cell is value (0)
                        break;
                    case CELLTYPE_STRING :
                    case CELLTYPE_EDIT :
                        nType = 2;
                        break;
                    case CELLTYPE_VALUE :
                        {
                            ULONG nFormat = GetCellNumberFormat( aAdr, pCell );
                            if (pFormatter->GetType(nFormat)
                                                     == NUMBERFORMAT_LOGICAL)
                                nType = 4;
                            else
                                nType = 1;
                        }
                        break;
                    case CELLTYPE_FORMULA :
                        nType = 8;
                        break;
                    default:
                        PushIllegalArgument();
                }
            }
            else
                nType = 16;
        }
        break;
        case svString:
            PopError();
            if ( nGlobalError )
            {
                nType = 16;
                nGlobalError = 0;
            }
            else
                nType = 2;
        break;
        case svMatrix:
            PopMatrix();
            if ( nGlobalError )
            {
                nType = 16;
                nGlobalError = 0;
            }
            else
                nType = 64;
                // we could return the type of one element if in JumpMatrix or
                // ForceArray mode, but Xcl doesn't ...
        break;
        default:
            PopError();
            if ( nGlobalError )
            {
                nType = 16;
                nGlobalError = 0;
            }
            else
                nType = 1;
    }
    PushInt( nType );
}


inline BOOL lcl_FormatHasNegColor( const SvNumberformat* pFormat )
{
    return pFormat && pFormat->GetColor( 1 );
}


inline BOOL lcl_FormatHasOpenPar( const SvNumberformat* pFormat )
{
    return pFormat && (pFormat->GetFormatstring().Search( '(' ) != STRING_NOTFOUND);
}


void ScInterpreter::ScCell()
{   // ATTRIBUTE ; [REF]
    BYTE nParamCount = GetByte();
    if( MustHaveParamCount( nParamCount, 1, 2 ) )
    {
        ScAddress aCellPos( aPos );
        BOOL bError = FALSE;
        if( nParamCount == 2 )
            bError = !PopDoubleRefOrSingleRef( aCellPos );
        String aInfoType( GetString() );
        if( bError || nGlobalError )
            PushIllegalParameter();
        else
        {
            String          aFuncResult;
            ScBaseCell*     pCell = GetCell( aCellPos );

            ScCellKeywordTranslator::transKeyword(aInfoType, ScGlobal::GetLocale(), ocCell);

// *** ADDRESS INFO ***
            if( aInfoType.EqualsAscii( "COL" ) )
            {   // column number (1-based)
                PushInt( aCellPos.Col() + 1 );
            }
            else if( aInfoType.EqualsAscii( "ROW" ) )
            {   // row number (1-based)
                PushInt( aCellPos.Row() + 1 );
            }
            else if( aInfoType.EqualsAscii( "SHEET" ) )
            {   // table number (1-based)
                PushInt( aCellPos.Tab() + 1 );
            }
            else if( aInfoType.EqualsAscii( "ADDRESS" ) )
            {   // address formatted as [['FILENAME'#]$TABLE.]$COL$ROW
                USHORT nFlags = (aCellPos.Tab() == aPos.Tab()) ? (SCA_ABS) : (SCA_ABS_3D);
                aCellPos.Format( aFuncResult, nFlags, pDok, pDok->GetAddressConvention() );
                PushString( aFuncResult );
            }
            else if( aInfoType.EqualsAscii( "FILENAME" ) )
            {   // file name and table name: 'FILENAME'#$TABLE
                SCTAB nTab = aCellPos.Tab();
                if( nTab < pDok->GetTableCount() )
                {
                    if( pDok->GetLinkMode( nTab ) == SC_LINK_VALUE )
                        pDok->GetName( nTab, aFuncResult );
                    else
                    {
                        SfxObjectShell* pShell = pDok->GetDocumentShell();
                        if( pShell && pShell->GetMedium() )
                        {
                            aFuncResult = (sal_Unicode) '\'';
                            const INetURLObject& rURLObj = pShell->GetMedium()->GetURLObject();
                            aFuncResult += String( rURLObj.GetMainURL( INetURLObject::DECODE_UNAMBIGUOUS ) );
                            aFuncResult.AppendAscii( "'#$" );
                            String aTabName;
                            pDok->GetName( nTab, aTabName );
                            aFuncResult += aTabName;
                        }
                    }
                }
                PushString( aFuncResult );
            }
            else if( aInfoType.EqualsAscii( "COORD" ) )
            {   // address, lotus 1-2-3 formatted: $TABLE:$COL$ROW
                // Yes, passing tab as col is intentional!
                ScAddress( static_cast<SCCOL>(aCellPos.Tab()), 0, 0 ).Format( 
                    aFuncResult, (SCA_COL_ABSOLUTE|SCA_VALID_COL), NULL, pDok->GetAddressConvention() );
                aFuncResult += ':';
                String aCellStr;
                aCellPos.Format( aCellStr, (SCA_COL_ABSOLUTE|SCA_VALID_COL|SCA_ROW_ABSOLUTE|SCA_VALID_ROW),
                                 NULL, pDok->GetAddressConvention() );
                aFuncResult += aCellStr;
                PushString( aFuncResult );
            }

// *** CELL PROPERTIES ***
            else if( aInfoType.EqualsAscii( "CONTENTS" ) )
            {   // contents of the cell, no formatting
                if( pCell && pCell->HasStringData() )
                {
                    GetCellString( aFuncResult, pCell );
                    PushString( aFuncResult );
                }
                else
                    PushDouble( GetCellValue( aCellPos, pCell ) );
            }
            else if( aInfoType.EqualsAscii( "TYPE" ) )
            {   // b = blank; l = string (label); v = otherwise (value)
                if( HasCellStringData( pCell ) )
                    aFuncResult = 'l';
                else
                    aFuncResult = HasCellValueData( pCell ) ? 'v' : 'b';
                PushString( aFuncResult );
            }
            else if( aInfoType.EqualsAscii( "WIDTH" ) )
            {   // column width (rounded off as count of zero characters in standard font and size)
                Printer*    pPrinter = pDok->GetPrinter();
                MapMode     aOldMode( pPrinter->GetMapMode() );
                Font        aOldFont( pPrinter->GetFont() );
                Font        aDefFont;

                pPrinter->SetMapMode( MAP_TWIP );
                // font color doesn't matter here
                pDok->GetDefPattern()->GetFont( aDefFont, SC_AUTOCOL_BLACK, pPrinter );
                pPrinter->SetFont( aDefFont );
                long nZeroWidth = pPrinter->GetTextWidth( String( '0' ) );
                pPrinter->SetFont( aOldFont );
                pPrinter->SetMapMode( aOldMode );
                int nZeroCount = (int)(pDok->GetColWidth( aCellPos.Col(), aCellPos.Tab() ) / nZeroWidth);
                PushInt( nZeroCount );
            }
            else if( aInfoType.EqualsAscii( "PREFIX" ) )
            {   // ' = left; " = right; ^ = centered
                if( HasCellStringData( pCell ) )
                {
                    const SvxHorJustifyItem* pJustAttr = (const SvxHorJustifyItem*)
                        pDok->GetAttr( aCellPos.Col(), aCellPos.Row(), aCellPos.Tab(), ATTR_HOR_JUSTIFY );
                    switch( pJustAttr->GetValue() )
                    {
                        case SVX_HOR_JUSTIFY_STANDARD:
                        case SVX_HOR_JUSTIFY_LEFT:
                        case SVX_HOR_JUSTIFY_BLOCK:     aFuncResult = '\''; break;
                        case SVX_HOR_JUSTIFY_CENTER:    aFuncResult = '^';  break;
                        case SVX_HOR_JUSTIFY_RIGHT:     aFuncResult = '"';  break;
                        case SVX_HOR_JUSTIFY_REPEAT:    aFuncResult = '\\'; break;
                    }
                }
                PushString( aFuncResult );
            }
            else if( aInfoType.EqualsAscii( "PROTECT" ) )
            {   // 1 = cell locked
                const ScProtectionAttr* pProtAttr = (const ScProtectionAttr*)
                    pDok->GetAttr( aCellPos.Col(), aCellPos.Row(), aCellPos.Tab(), ATTR_PROTECTION );
                PushInt( pProtAttr->GetProtection() ? 1 : 0 );
            }

// *** FORMATTING ***
            else if( aInfoType.EqualsAscii( "FORMAT" ) )
            {   // specific format code for standard formats
                ULONG   nFormat = pDok->GetNumberFormat( aCellPos );
                BOOL    bAppendPrec = TRUE;
                USHORT  nPrec, nLeading;
                BOOL    bThousand, bIsRed;
                pFormatter->GetFormatSpecialInfo( nFormat, bThousand, bIsRed, nPrec, nLeading );

                switch( pFormatter->GetType( nFormat ) )
                {
                    case NUMBERFORMAT_NUMBER:       aFuncResult = (bThousand ? ',' : 'F');  break;
                    case NUMBERFORMAT_CURRENCY:     aFuncResult = 'C';                      break;
                    case NUMBERFORMAT_SCIENTIFIC:   aFuncResult = 'S';                      break;
                    case NUMBERFORMAT_PERCENT:      aFuncResult = 'P';                      break;
                    default:
                    {
                        bAppendPrec = FALSE;
                        switch( pFormatter->GetIndexTableOffset( nFormat ) )
                        {
                            case NF_DATE_SYSTEM_SHORT:
                            case NF_DATE_SYS_DMMMYY:
                            case NF_DATE_SYS_DDMMYY:
                            case NF_DATE_SYS_DDMMYYYY:
                            case NF_DATE_SYS_DMMMYYYY:
                            case NF_DATE_DIN_DMMMYYYY:
                            case NF_DATE_SYS_DMMMMYYYY:
                            case NF_DATE_DIN_DMMMMYYYY: aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D1" ) );  break;
                            case NF_DATE_SYS_DDMMM:     aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D2" ) );  break;
                            case NF_DATE_SYS_MMYY:      aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D3" ) );  break;
                            case NF_DATETIME_SYSTEM_SHORT_HHMM:
                            case NF_DATETIME_SYS_DDMMYYYY_HHMMSS:
                                                        aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D4" ) );  break;
                            case NF_DATE_DIN_MMDD:      aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D5" ) );  break;
                            case NF_TIME_HHMMSSAMPM:    aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D6" ) );  break;
                            case NF_TIME_HHMMAMPM:      aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D7" ) );  break;
                            case NF_TIME_HHMMSS:        aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D8" ) );  break;
                            case NF_TIME_HHMM:          aFuncResult.AssignAscii( RTL_CONSTASCII_STRINGPARAM( "D9" ) );  break;
                            default:                    aFuncResult = 'G';
                        }
                    }
                }
                if( bAppendPrec )
                    aFuncResult += String::CreateFromInt32( nPrec );
                const SvNumberformat* pFormat = pFormatter->GetEntry( nFormat );
                if( lcl_FormatHasNegColor( pFormat ) )
                    aFuncResult += '-';
                if( lcl_FormatHasOpenPar( pFormat ) )
                    aFuncResult.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "()" ) );
                PushString( aFuncResult );
            }
            else if( aInfoType.EqualsAscii( "COLOR" ) )
            {   // 1 = negative values are colored, otherwise 0
                const SvNumberformat* pFormat = pFormatter->GetEntry( pDok->GetNumberFormat( aCellPos ) );
                PushInt( lcl_FormatHasNegColor( pFormat ) ? 1 : 0 );
            }
            else if( aInfoType.EqualsAscii( "PARENTHESES" ) )
            {   // 1 = format string contains a '(' character, otherwise 0
                const SvNumberformat* pFormat = pFormatter->GetEntry( pDok->GetNumberFormat( aCellPos ) );
                PushInt( lcl_FormatHasOpenPar( pFormat ) ? 1 : 0 );
            }
            else
                PushIllegalArgument();
        }
    }
}


void ScInterpreter::ScIsRef()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCell" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    switch ( GetStackType() )
    {
        case svSingleRef :
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            if ( !nGlobalError )
                nRes = 1;
        }
        break;
        case svDoubleRef :
        {
            ScRange aRange;
            PopDoubleRef( aRange );
            if ( !nGlobalError )
                nRes = 1;
        }
        break;
        case svRefList :
        {
            FormulaTokenRef x = PopToken();
            if ( !nGlobalError )
                nRes = !static_cast<ScToken*>(x.get())->GetRefList()->empty();
        }
        break;
        default:
            Pop();
    }
    nGlobalError = 0;
    PushInt( nRes );
}


void ScInterpreter::ScIsValue()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsValue" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    switch ( GetRawStackType() )
    {
        case svDouble:
            Pop();
            nRes = 1;
        break;
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            ScBaseCell* pCell = GetCell( aAdr );
            if (GetCellErrCode( pCell ) == 0)
            {
                switch ( GetCellType( pCell ) )
                {
                    case CELLTYPE_VALUE :
                        nRes = 1;
                        break;
                    case CELLTYPE_FORMULA :
                        nRes = ((ScFormulaCell*)pCell)->IsValue() &&
                            !((ScFormulaCell*)pCell)->IsEmpty();
                        break;
                    default:
                        ; // nothing
                }
            }
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
            {
                if (pMat->GetErrorIfNotString( 0 ) == 0)
                    nRes = pMat->IsValue( 0 );
            }
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                    if (pMat->GetErrorIfNotString( nC, nR) == 0)
                        nRes = pMat->IsValue( nC, nR);
            }
        }
        break;
        default:
            Pop();
    }
    nGlobalError = 0;
    PushInt( nRes );
}


void ScInterpreter::ScIsFormula()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsFormula" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            nRes = (GetCellType( GetCell( aAdr ) ) == CELLTYPE_FORMULA);
        }
        break;
        default:
            Pop();
    }
    nGlobalError = 0;
    PushInt( nRes );
}


void ScInterpreter::ScFormula()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScFormula" );
    String aFormula;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            ScBaseCell* pCell = GetCell( aAdr );
            switch ( GetCellType( pCell ) )
            {
                case CELLTYPE_FORMULA :
                    ((ScFormulaCell*)pCell)->GetFormula( aFormula );
                break;
                default:
                    SetError( NOTAVAILABLE );
            }
        }
        break;
        default:
            Pop();
            SetError( NOTAVAILABLE );
    }
    PushString( aFormula );
}



void ScInterpreter::ScIsNV()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsNV" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            PopDoubleRefOrSingleRef( aAdr );
            if ( nGlobalError == NOTAVAILABLE )
                nRes = 1;
            else
            {
                ScBaseCell* pCell = GetCell( aAdr );
                USHORT nErr = GetCellErrCode( pCell );
                nRes = (nErr == NOTAVAILABLE);
            }
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
                nRes = (pMat->GetErrorIfNotString( 0 ) == NOTAVAILABLE);
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                    nRes = (pMat->GetErrorIfNotString( nC, nR) == NOTAVAILABLE);
            }
        }
        break;
        default:
            PopError();
            if ( nGlobalError == NOTAVAILABLE )
                nRes = 1;
    }
    nGlobalError = 0;
    PushInt( nRes );
}


void ScInterpreter::ScIsErr()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsErr" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            PopDoubleRefOrSingleRef( aAdr );
            if ( nGlobalError && nGlobalError != NOTAVAILABLE )
                nRes = 1;
            else
            {
                ScBaseCell* pCell = GetCell( aAdr );
                USHORT nErr = GetCellErrCode( pCell );
                nRes = (nErr && nErr != NOTAVAILABLE);
            }
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( nGlobalError || !pMat )
                nRes = ((nGlobalError && nGlobalError != NOTAVAILABLE) || !pMat);
            else if ( !pJumpMatrix )
            {
                USHORT nErr = pMat->GetErrorIfNotString( 0 );
                nRes = (nErr && nErr != NOTAVAILABLE);
            }
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                {
                    USHORT nErr = pMat->GetErrorIfNotString( nC, nR);
                    nRes = (nErr && nErr != NOTAVAILABLE);
                }
            }
        }
        break;
        default:
            PopError();
            if ( nGlobalError && nGlobalError != NOTAVAILABLE )
                nRes = 1;
    }
    nGlobalError = 0;
    PushInt( nRes );
}


void ScInterpreter::ScIsError()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsError" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
            {
                nRes = 1;
                break;
            }
            if ( nGlobalError )
                nRes = 1;
            else
            {
                ScBaseCell* pCell = GetCell( aAdr );
                nRes = (GetCellErrCode( pCell ) != 0);
            }
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( nGlobalError || !pMat )
                nRes = 1;
            else if ( !pJumpMatrix )
                nRes = (pMat->GetErrorIfNotString( 0 ) != 0);
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                    nRes = (pMat->GetErrorIfNotString( nC, nR) != 0);
            }
        }
        break;
        default:
            PopError();
            if ( nGlobalError )
                nRes = 1;
    }
    nGlobalError = 0;
    PushInt( nRes );
}


short ScInterpreter::IsEven()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::IsEven" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    short nRes = 0;
    double fVal = 0.0;
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
                break;
            ScBaseCell* pCell = GetCell( aAdr );
            USHORT nErr = GetCellErrCode( pCell );
            if (nErr != 0)
                SetError(nErr);
            else
            {
                switch ( GetCellType( pCell ) )
                {
                    case CELLTYPE_VALUE :
                        fVal = GetCellValue( aAdr, pCell );
                        nRes = 1;
                    break;
                    case CELLTYPE_FORMULA :
                        if( ((ScFormulaCell*)pCell)->IsValue() )
                        {
                            fVal = GetCellValue( aAdr, pCell );
                            nRes = 1;
                        }
                    break;
                    default:
                        ; // nothing
                }
            }
        }
        break;
        case svDouble:
        {
            fVal = PopDouble();
            nRes = 1;
        }
        break;
        case svMatrix:
        {
            ScMatrixRef pMat = PopMatrix();
            if ( !pMat )
                ;   // nothing
            else if ( !pJumpMatrix )
            {
                nRes = pMat->IsValue( 0 );
                if ( nRes )
                    fVal = pMat->GetDouble( 0 );
            }
            else
            {
                SCSIZE nCols, nRows, nC, nR;
                pMat->GetDimensions( nCols, nRows);
                pJumpMatrix->GetPos( nC, nR);
                if ( nC < nCols && nR < nRows )
                {
                    nRes = pMat->IsValue( nC, nR);
                    if ( nRes )
                        fVal = pMat->GetDouble( nC, nR);
                }
                else
                    SetError( errNoValue);
            }
        }
        break;
        default:
            ; // nothing
    }
    if ( !nRes )
        SetError( errIllegalParameter);
    else
        nRes = ( fmod( ::rtl::math::approxFloor( fabs( fVal ) ), 2.0 ) < 0.5 );
    return nRes;
}


void ScInterpreter::ScIsEven()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsEven" );
    PushInt( IsEven() );
}


void ScInterpreter::ScIsOdd()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIsOdd" );
    PushInt( !IsEven() );
}


void ScInterpreter::ScN()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScN" );
    USHORT nErr = nGlobalError;
    nGlobalError = 0;
    double fVal;
    if ( GetRawStackType() == svString )
    {
        fVal = 0.0;
        Pop();
    }
    else
        fVal = GetDouble();
    if ( nGlobalError == NOTAVAILABLE || nGlobalError == errIllegalArgument )
        nGlobalError = 0;       // N(#NA) and N("text") are ok
    if ( !nGlobalError && nErr != NOTAVAILABLE )
        nGlobalError = nErr;
    PushDouble( fVal );
}


void ScInterpreter::ScTrim()
{   // trimmt nicht nur sondern schnibbelt auch doppelte raus!
    String aVal( GetString() );
    aVal.EraseLeadingChars();
    aVal.EraseTrailingChars();
    String aStr;
    register const sal_Unicode* p = aVal.GetBuffer();
    register const sal_Unicode* const pEnd = p + aVal.Len();
    while ( p < pEnd )
    {
        if ( *p != ' ' || p[-1] != ' ' )    // erster kann kein ' ' sein, -1 ist also ok
            aStr += *p;
        p++;
    }
    PushString( aStr );
}


void ScInterpreter::ScUpper()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTrim" );
    String aString = GetString();
    ScGlobal::pCharClass->toUpper(aString);
    PushString(aString);
}


void ScInterpreter::ScPropper()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScPropper" );
//2do: what to do with I18N-CJK ?!?
    String aStr( GetString() );
    const xub_StrLen nLen = aStr.Len();
    // #i82487# don't try to write to empty string's BufferAccess
    // (would crash now that the empty string is const)
    if ( nLen > 0 )
    {
        String aUpr( ScGlobal::pCharClass->upper( aStr ) );
        String aLwr( ScGlobal::pCharClass->lower( aStr ) );
        register sal_Unicode* pStr = aStr.GetBufferAccess();
        const sal_Unicode* pUpr = aUpr.GetBuffer();
        const sal_Unicode* pLwr = aLwr.GetBuffer();
        *pStr = *pUpr;
        String aTmpStr( 'x' );
        xub_StrLen nPos = 1;
        while( nPos < nLen )
        {
            aTmpStr.SetChar( 0, pStr[nPos-1] );
            if ( !ScGlobal::pCharClass->isLetter( aTmpStr, 0 ) )
                pStr[nPos] = pUpr[nPos];
            else
                pStr[nPos] = pLwr[nPos];
            nPos++;
        }
        aStr.ReleaseBufferAccess( nLen );
    }
    PushString( aStr );
}


void ScInterpreter::ScLower()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScLower" );
    String aString( GetString() );
    ScGlobal::pCharClass->toLower(aString);
    PushString(aString);
}


void ScInterpreter::ScLen()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScLen" );
    String aStr( GetString() );
    PushDouble( aStr.Len() );
}


void ScInterpreter::ScT()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScT" );
    switch ( GetStackType() )
    {
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
            {
                PushInt(0);
                return ;
            }
            BOOL bValue = FALSE;
            ScBaseCell* pCell = GetCell( aAdr );
            if ( GetCellErrCode( pCell ) == 0 )
            {
                switch ( GetCellType( pCell ) )
                {
                    case CELLTYPE_VALUE :
                        bValue = TRUE;
                        break;
                    case CELLTYPE_FORMULA :
                        bValue = ((ScFormulaCell*)pCell)->IsValue();
                        break;
                    default:
                        ; // nothing
                }
            }
            if ( bValue )
                PushString( EMPTY_STRING );
            else
            {
                //  wie GetString()
                GetCellString( aTempStr, pCell );
                PushString( aTempStr );
            }
        }
        break;
        case svDouble :
        {
            PopError();
            PushString( EMPTY_STRING );
        }
        break;
        case svString :
            ;   // leave on stack
        break;
        default :
            PushError( errUnknownOpCode);
    }
}


void ScInterpreter::ScValue()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScValue" );
    String aInputString;
    double fVal;

    switch ( GetRawStackType() )
    {
        case svMissing:
        case svEmptyCell:
            Pop();
            PushInt(0);
            return;
        case svDouble:
            return;     // leave on stack
            //break;

        case svSingleRef:
        case svDoubleRef:
            {
                ScAddress aAdr;
                if ( !PopDoubleRefOrSingleRef( aAdr ) )
                {
                    PushInt(0);
                    return;
                }
                ScBaseCell* pCell = GetCell( aAdr );
                if ( pCell && pCell->HasStringData() )
                    GetCellString( aInputString, pCell );
                else if ( pCell && pCell->HasValueData() )
                {
                    PushDouble( GetCellValue(aAdr, pCell) );
                    return;
                }
                else
                {
                    PushDouble(0.0);
                    return;
                }
            }
            break;
        case svMatrix:
            {
                ScMatValType nType = GetDoubleOrStringFromMatrix( fVal,
                        aInputString);
                switch (nType)
                {
                    case SC_MATVAL_EMPTY:
                        fVal = 0.0;
                        // fallthru
                    case SC_MATVAL_VALUE:
                    case SC_MATVAL_BOOLEAN:
                        PushDouble( fVal);
                        return;
                        //break;
                    case SC_MATVAL_STRING:
                        // evaluated below
                        break;
                    default:
                        PushIllegalArgument();
                }
            }
            break;
        default:
            aInputString = GetString();
            break;
    }

    sal_uInt32 nFIndex = 0;     // 0 for default locale
    if (pFormatter->IsNumberFormat(aInputString, nFIndex, fVal))
        PushDouble(fVal);
    else
        PushIllegalArgument();
}


//2do: this should be a proper unicode string method
inline BOOL lcl_ScInterpreter_IsPrintable( sal_Unicode c )
{
    return 0x20 <= c && c != 0x7f;
}

void ScInterpreter::ScClean()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScClean" );
    String aStr( GetString() );
    for ( xub_StrLen i = 0; i < aStr.Len(); i++ )
    {
        if ( !lcl_ScInterpreter_IsPrintable( aStr.GetChar( i ) ) )
            aStr.Erase(i,1);
    }
    PushString(aStr);
}


void ScInterpreter::ScCode()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCode" );
//2do: make it full range unicode?
    const String& rStr = GetString();
    PushInt( (sal_uChar) ByteString::ConvertFromUnicode( rStr.GetChar(0), gsl_getSystemTextEncoding() ) );
}


void ScInterpreter::ScChar()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScChar" );
//2do: make it full range unicode?
    double fVal = GetDouble();
    if (fVal < 0.0 || fVal >= 256.0)
        PushIllegalArgument();
    else
    {
        String aStr( '0' );
        aStr.SetChar( 0, ByteString::ConvertToUnicode( (sal_Char) fVal, gsl_getSystemTextEncoding() ) );
        PushString( aStr );
    }
}


/* #i70213# fullwidth/halfwidth conversion provided by
 * Takashi Nakamoto <bluedwarf@ooo>
 * erAck: added Excel compatibility conversions as seen in issue's test case. */

static ::rtl::OUString lcl_convertIntoHalfWidth( const ::rtl::OUString & rStr )
{
    static bool bFirstASCCall = true;
    static utl::TransliterationWrapper aTrans( ::comphelper::getProcessServiceFactory(), 0 );

    if( bFirstASCCall )
    {
        aTrans.loadModuleByImplName( ::rtl::OUString::createFromAscii( "FULLWIDTH_HALFWIDTH_LIKE_ASC" ), LANGUAGE_SYSTEM );
        bFirstASCCall = false;
    }

    return aTrans.transliterate( rStr, 0, USHORT( rStr.getLength() ), NULL );
}


static ::rtl::OUString lcl_convertIntoFullWidth( const ::rtl::OUString & rStr )
{
    static bool bFirstJISCall = true;
    static utl::TransliterationWrapper aTrans( ::comphelper::getProcessServiceFactory(), 0 );

    if( bFirstJISCall )
    {
        aTrans.loadModuleByImplName( ::rtl::OUString::createFromAscii( "HALFWIDTH_FULLWIDTH_LIKE_JIS" ), LANGUAGE_SYSTEM );
        bFirstJISCall = false;
    }    

    return aTrans.transliterate( rStr, 0, USHORT( rStr.getLength() ), NULL );
}


/* ODFF:
 * Summary: Converts half-width to full-width ASCII and katakana characters.
 * Semantics: Conversion is done for half-width ASCII and katakana characters,
 * other characters are simply copied from T to the result. This is the
 * complementary function to ASC.
 * For references regarding halfwidth and fullwidth characters see
 * http://www.unicode.org/reports/tr11/
 * http://www.unicode.org/charts/charindex2.html#H
 * http://www.unicode.org/charts/charindex2.html#F
 */
void ScInterpreter::ScJis()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScJis" );
    if (MustHaveParamCount( GetByte(), 1))
        PushString( lcl_convertIntoFullWidth( GetString()));
}


/* ODFF:
 * Summary: Converts full-width to half-width ASCII and katakana characters.
 * Semantics: Conversion is done for full-width ASCII and katakana characters,
 * other characters are simply copied from T to the result. This is the
 * complementary function to JIS.
 */
void ScInterpreter::ScAsc()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScAsc" );
    if (MustHaveParamCount( GetByte(), 1))
        PushString( lcl_convertIntoHalfWidth( GetString()));
}

void ScInterpreter::ScUnicode()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScUnicode" );
    if ( MustHaveParamCount( GetByte(), 1 ) )
    {
        const rtl::OUString& rStr = GetString();
        if (rStr.getLength() <= 0)
            PushIllegalParameter();
        else
        {
            sal_Int32 i = 0;
            PushDouble( rStr.iterateCodePoints(&i) );
        }
    }
}

void ScInterpreter::ScUnichar()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScUnichar" );
    if ( MustHaveParamCount( GetByte(), 1 ) )
    {
        double dVal = ::rtl::math::approxFloor( GetDouble() );
        if ((dVal < 0x000000) || (dVal > 0x10FFFF)) 
            PushIllegalArgument();
        else
        {
            sal_uInt32 nCodePoint = static_cast<sal_uInt32>( dVal );
            rtl::OUString aStr( &nCodePoint, 1 );
            PushString( aStr );
        }
    }
}


void ScInterpreter::ScMin( BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScMin" );
    short nParamCount = GetByte();
    if (!MustHaveParamCountMin( nParamCount, 1))
        return;
    double nMin = ::std::numeric_limits<double>::max();
    double nVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
            {
                nVal = GetDouble();
                if (nMin > nVal) nMin = nVal;
                nFuncFmtType = NUMBERFORMAT_NUMBER;
            }
            break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    nVal = GetCellValue( aAdr, pCell );
                    CurFmtToFuncFmt();
                    if (nMin > nVal) nMin = nVal;
                }
                else if ( bTextAsZero && HasCellStringData( pCell ) )
                {
                    if ( nMin > 0.0 )
                        nMin = 0.0;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                ScValueIterator aValIter( pDok, aRange, glSubTotal, bTextAsZero );
                if (aValIter.GetFirst(nVal, nErr))
                {
                    if (nMin > nVal)
                        nMin = nVal;
                    aValIter.GetCurNumFmtInfo( nFuncFmtType, nFuncFmtIndex );
                    while ((nErr == 0) && aValIter.GetNext(nVal, nErr))
                    {
                        if (nMin > nVal)
                            nMin = nVal;
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nC, nR;
                    nFuncFmtType = NUMBERFORMAT_NUMBER;
                    pMat->GetDimensions(nC, nR);
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++)
                            for (SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++)
                            {
                                nVal = pMat->GetDouble(nMatCol,nMatRow);
                                if (nMin > nVal) nMin = nVal;
                            }
                    }
                    else
                    {
                        for (SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++)
                        {
                            for (SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++)
                            {
                                if (!pMat->IsString(nMatCol,nMatRow))
                                {
                                    nVal = pMat->GetDouble(nMatCol,nMatRow);
                                    if (nMin > nVal) nMin = nVal;
                                }
                                else if ( bTextAsZero )
                                {
                                    if ( nMin > 0.0 )
                                        nMin = 0.0;
                                }
                            }
                         }
                    }
                }
            }
            break;
            case svString :
            {
                Pop();
                if ( bTextAsZero )
                {
                    if ( nMin > 0.0 )
                        nMin = 0.0;
                }
                else
                    SetError(errIllegalParameter);
            }
            break;
            default :
                Pop();
                SetError(errIllegalParameter);
        }
    }
    if ( nVal < nMin  )
        PushDouble(0.0);
    else
        PushDouble(nMin);
}

#if defined(WIN) && defined(MSC)
#pragma optimize("",off)
#endif

void ScInterpreter::ScMax( BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScMax" );
    short nParamCount = GetByte();
    if (!MustHaveParamCountMin( nParamCount, 1))
        return;
    double nMax = -(::std::numeric_limits<double>::max());
    double nVal = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
            {
                nVal = GetDouble();
                if (nMax < nVal) nMax = nVal;
                nFuncFmtType = NUMBERFORMAT_NUMBER;
            }
            break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    nVal = GetCellValue( aAdr, pCell );
                    CurFmtToFuncFmt();
                    if (nMax < nVal) nMax = nVal;
                }
                else if ( bTextAsZero && HasCellStringData( pCell ) )
                {
                    if ( nMax < 0.0 )
                        nMax = 0.0;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                ScValueIterator aValIter( pDok, aRange, glSubTotal, bTextAsZero );
                if (aValIter.GetFirst(nVal, nErr))
                {
                    if (nMax < nVal)
                        nMax = nVal;
                    aValIter.GetCurNumFmtInfo( nFuncFmtType, nFuncFmtIndex );
                    while ((nErr == 0) && aValIter.GetNext(nVal, nErr))
                    {
                        if (nMax < nVal)
                            nMax = nVal;
                    }
                    SetError(nErr);
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    nFuncFmtType = NUMBERFORMAT_NUMBER;
                    SCSIZE nC, nR;
                    pMat->GetDimensions(nC, nR);
                    if (pMat->IsNumeric())
                    {
                        for (SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++)
                            for (SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++)
                            {
                                nVal = pMat->GetDouble(nMatCol,nMatRow);
                                if (nMax < nVal) nMax = nVal;
                            }
                    }
                    else
                    {
                        for (SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++)
                        {
                            for (SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++)
                            {
                                if (!pMat->IsString(nMatCol,nMatRow))
                                {
                                    nVal = pMat->GetDouble(nMatCol,nMatRow);
                                    if (nMax < nVal) nMax = nVal;
                                }
                                else if ( bTextAsZero )
                                {
                                    if ( nMax < 0.0 )
                                        nMax = 0.0;
                                }
                            }
                        }
                    }
                }
            }
            break;
            case svString :
            {
                Pop();
                if ( bTextAsZero )
                {
                    if ( nMax < 0.0 )
                        nMax = 0.0;
                }
                else
                    SetError(errIllegalParameter);
            }
            break;
            default :
                Pop();
                SetError(errIllegalParameter);
        }
    }
    if ( nVal > nMax  )
        PushDouble(0.0);
    else
        PushDouble(nMax);
}
#if defined(WIN) && defined(MSC)
#pragma optimize("",on)
#endif


double ScInterpreter::IterateParameters( ScIterFunc eFunc, BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::IterateParameters" );
    short nParamCount = GetByte();
    double fRes = ( eFunc == ifPRODUCT ) ? 1.0 : 0.0;
    double fVal = 0.0;
    double fMem = 0.0;
    BOOL bNull = TRUE;
    ULONG nCount = 0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    if ( nGlobalError && ( eFunc == ifCOUNT2 || eFunc == ifCOUNT ) )
        nGlobalError = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {

            case svString:
            {
                if( eFunc == ifCOUNT )
                {
                    String aStr( PopString() );
                    sal_uInt32 nFIndex = 0;                 // damit default Land/Spr.
                    if ( bTextAsZero || pFormatter->IsNumberFormat(aStr, nFIndex, fVal))
                        nCount++;
                }
                else
                {
                    switch ( eFunc )
                    {
                        case ifAVERAGE:
                        case ifSUM:
                        case ifSUMSQ:
                        case ifPRODUCT:
                        {
                            if ( bTextAsZero )
                            {
                                Pop();
                                nCount++;
                                if ( eFunc == ifPRODUCT )
                                    fRes = 0.0;
                            }
                            else
                            {
                                while (nParamCount-- > 0)
                                    Pop();
                                SetError( errNoValue );
                            }
                        }
                        break;
                        default:
                            Pop();
                            nCount++;
                    }
                }
            }
            break;
            case svDouble    :
                fVal = GetDouble();
                nCount++;
                switch( eFunc )
                {
                    case ifAVERAGE:
                    case ifSUM:
                        if ( bNull && fVal != 0.0 )
                        {
                            bNull = FALSE;
                            fMem = fVal;
                        }
                        else
                            fRes += fVal;
                        break;
                    case ifSUMSQ:   fRes += fVal * fVal; break;
                    case ifPRODUCT: fRes *= fVal; break;
                    default: ; // nothing
                }
                nFuncFmtType = NUMBERFORMAT_NUMBER;
                break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                if ( nGlobalError && ( eFunc == ifCOUNT2 || eFunc == ifCOUNT ) )
                {
                    nGlobalError = 0;
                    if ( eFunc == ifCOUNT2 )
                        ++nCount;
                    break;
                }
                ScBaseCell* pCell = GetCell( aAdr );
                if ( pCell )
                {
                    if( eFunc == ifCOUNT2 )
                    {
                        CellType eCellType = pCell->GetCellType();
                        if (eCellType != CELLTYPE_NONE && eCellType != CELLTYPE_NOTE)
                            nCount++;
                        if ( nGlobalError )
                            nGlobalError = 0;
                    }
                    else if ( pCell->HasValueData() )
                    {
                        nCount++;
                        fVal = GetCellValue( aAdr, pCell );
                        CurFmtToFuncFmt();
                        switch( eFunc )
                        {
                            case ifAVERAGE:
                            case ifSUM:
                                if ( bNull && fVal != 0.0 )
                                {
                                    bNull = FALSE;
                                    fMem = fVal;
                                }
                                else
                                    fRes += fVal;
                                break;
                            case ifSUMSQ:   fRes += fVal * fVal; break;
                            case ifPRODUCT: fRes *= fVal; break;
                            case ifCOUNT:
                                if ( nGlobalError )
                                {    
                                    nGlobalError = 0;
                                    nCount--;
                                }
                                break;
                            default: ; // nothing
                        }
                    }
                    else if ( bTextAsZero && pCell->HasStringData() )
                    {
                        nCount++;
                        if ( eFunc == ifPRODUCT )
                            fRes = 0.0;
                    }
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                if ( nGlobalError && ( eFunc == ifCOUNT2 || eFunc == ifCOUNT ) )
                {
                    nGlobalError = 0;
                    if ( eFunc == ifCOUNT2 )
                        ++nCount;
                    break;
                }
                if( eFunc == ifCOUNT2 )
                {
                    ScBaseCell* pCell;
                    ScCellIterator aIter( pDok, aRange, glSubTotal );
                    if ( (pCell = aIter.GetFirst()) != NULL )
                    {
                        do
                        {
                            CellType eType = pCell->GetCellType();
                            if( eType != CELLTYPE_NONE && eType != CELLTYPE_NOTE )
                                nCount++;
                        }
                        while ( (pCell = aIter.GetNext()) != NULL );
                    }
                    if ( nGlobalError )
                        nGlobalError = 0;
                }
                else
                {
                    ScValueIterator aValIter( pDok, aRange, glSubTotal, bTextAsZero );
                    if (aValIter.GetFirst(fVal, nErr))
                    {
                        //  Schleife aus Performance-Gruenden nach innen verlegt:
                        aValIter.GetCurNumFmtInfo( nFuncFmtType, nFuncFmtIndex );
                        switch( eFunc )
                        {
                            case ifAVERAGE:
                            case ifSUM:
                                    do
                                    {
                                        SetError(nErr);
                                        if ( bNull && fVal != 0.0 )
                                        {
                                            bNull = FALSE;
                                            fMem = fVal;
                                        }
                                        else
                                            fRes += fVal;
                                        nCount++;
                                    }
                                    while (aValIter.GetNext(fVal, nErr));
                                    break;
                            case ifSUMSQ:
                                    do
                                    {
                                        SetError(nErr);
                                        fRes += fVal * fVal;
                                        nCount++;
                                    }
                                    while (aValIter.GetNext(fVal, nErr));
                                    break;
                            case ifPRODUCT:
                                    do
                                    {
                                        SetError(nErr);
                                        fRes *= fVal;
                                        nCount++;
                                    }
                                    while (aValIter.GetNext(fVal, nErr));
                                    break;
                            case ifCOUNT:
                                    do
                                    {
                                        if ( !nErr )
                                            nCount++;
                                    }
                                    while (aValIter.GetNext(fVal, nErr));
                                    break;
                            default: ;  // nothing
                        }
                        SetError( nErr );
                    }
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nC, nR;
                    nFuncFmtType = NUMBERFORMAT_NUMBER;
                    pMat->GetDimensions(nC, nR);
                    if( eFunc == ifCOUNT2 )
                        nCount += (ULONG) nC * nR;
                    else
                    {
                        for (SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++)
                        {
                            for (SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++)
                            {
                                if (!pMat->IsString(nMatCol,nMatRow))
                                {
                                    nCount++;
                                    fVal = pMat->GetDouble(nMatCol,nMatRow);
                                    switch( eFunc )
                                    {
                                        case ifAVERAGE:
                                        case ifSUM:
                                            if ( bNull && fVal != 0.0 )
                                            {
                                                bNull = FALSE;
                                                fMem = fVal;
                                            }
                                            else
                                                fRes += fVal;
                                            break;
                                        case ifSUMSQ:   fRes += fVal * fVal; break;
                                        case ifPRODUCT: fRes *= fVal; break;
                                        default: ; // nothing
                                    }
                                }
                                else if ( bTextAsZero )
                                {
                                    nCount++;
                                    if ( eFunc == ifPRODUCT )
                                        fRes = 0.0;
                                }
                            }
                        }
                    }
                }
            }
            break;
            case svError:
            {
                Pop();
                if ( eFunc == ifCOUNT )
                {
                    nGlobalError = 0;
                }
                else if ( eFunc == ifCOUNT2 )
                {
                    nCount++;
                    nGlobalError = 0;
                }
            }
            break;
            default :
                while (nParamCount-- > 0)
                    PopError();
                SetError(errIllegalParameter);
        }
    }
    switch( eFunc )
    {
        case ifSUM:     fRes = ::rtl::math::approxAdd( fRes, fMem ); break;
        case ifAVERAGE: fRes = div(::rtl::math::approxAdd( fRes, fMem ), nCount); break;
        case ifCOUNT2:
        case ifCOUNT:   fRes  = nCount; break;
        case ifPRODUCT: if ( !nCount ) fRes = 0.0; break;
        default: ; // nothing
    }
    // Bei Summen etc. macht ein BOOL-Ergebnis keinen Sinn
    // und Anzahl ist immer Number (#38345#)
    if( eFunc == ifCOUNT || nFuncFmtType == NUMBERFORMAT_LOGICAL )
        nFuncFmtType = NUMBERFORMAT_NUMBER;
    return fRes;
}


void ScInterpreter::ScSumSQ()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSumSQ" );
    PushDouble( IterateParameters( ifSUMSQ ) );
}


void ScInterpreter::ScSum()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSum" );
    PushDouble( IterateParameters( ifSUM ) );
}


void ScInterpreter::ScProduct()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScProduct" );
    PushDouble( IterateParameters( ifPRODUCT ) );
}


void ScInterpreter::ScAverage( BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScAverage" );
    PushDouble( IterateParameters( ifAVERAGE, bTextAsZero ) );
}


void ScInterpreter::ScCount()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCount" );
    PushDouble( IterateParameters( ifCOUNT ) );
}


void ScInterpreter::ScCount2()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCount2" );
    PushDouble( IterateParameters( ifCOUNT2 ) );
}


void ScInterpreter::GetStVarParams( double& rVal, double& rValCount,
                BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetStVarParams" );
    short nParamCount = GetByte();

    std::vector<double> values;
    double fSum    = 0.0;
    double vSum    = 0.0;
    double vMean    = 0.0;
    double fVal = 0.0;
    rValCount = 0.0;
    ScAddress aAdr;
    ScRange aRange;
    size_t nRefInList = 0;
    while (nParamCount-- > 0)
    {
        switch (GetStackType())
        {
            case svDouble :
            {
                fVal = GetDouble();
                values.push_back(fVal);
                fSum    += fVal;
                rValCount++;
            }
            break;
            case svSingleRef :
            {
                PopSingleRef( aAdr );
                ScBaseCell* pCell = GetCell( aAdr );
                if (HasCellValueData(pCell))
                {
                    fVal = GetCellValue( aAdr, pCell );
                    values.push_back(fVal);
                    fSum += fVal;
                    rValCount++;
                }
                else if ( bTextAsZero && HasCellStringData( pCell ) )
                {
                    values.push_back(0.0);
                    rValCount++;
                }
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                USHORT nErr = 0;
                PopDoubleRef( aRange, nParamCount, nRefInList);
                ScValueIterator aValIter( pDok, aRange, glSubTotal, bTextAsZero );
                if (aValIter.GetFirst(fVal, nErr))
                {
                    do
                    {
                        values.push_back(fVal);
                        fSum += fVal;
                        rValCount++;
                    }
                    while ((nErr == 0) && aValIter.GetNext(fVal, nErr));
                }
            }
            break;
            case svMatrix :
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nC, nR;
                    pMat->GetDimensions(nC, nR);
                    for (SCSIZE nMatCol = 0; nMatCol < nC; nMatCol++)
                    {
                        for (SCSIZE nMatRow = 0; nMatRow < nR; nMatRow++)
                        {
                            if (!pMat->IsString(nMatCol,nMatRow))
                            {
                                fVal= pMat->GetDouble(nMatCol,nMatRow);
                                values.push_back(fVal);
                                fSum += fVal;
                                rValCount++;
                            }
                            else if ( bTextAsZero )
                            {
                                values.push_back(0.0);
                                rValCount++;
                            }
                        }
                    }
                }
            }
            break;
            case svString :
            {
                Pop();
                if ( bTextAsZero )
                {
                    values.push_back(0.0);
                    rValCount++;
                }
                else
                    SetError(errIllegalParameter);
            }
            break;
            default :
                Pop();
                SetError(errIllegalParameter);
        }
    }

    ::std::vector<double>::size_type n = values.size();
    vMean = fSum / n;
    for (::std::vector<double>::size_type i = 0; i < n; i++)
        vSum += ::rtl::math::approxSub( values[i], vMean) * ::rtl::math::approxSub( values[i], vMean);
    rVal = vSum;
}


void ScInterpreter::ScVar( BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScVar" );
    double nVal;
    double nValCount;
    GetStVarParams( nVal, nValCount, bTextAsZero );

    if (nValCount <= 1.0)
        PushError( errDivisionByZero );
    else
        PushDouble( nVal / (nValCount - 1.0));
}


void ScInterpreter::ScVarP( BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScVarP" );
    double nVal;
    double nValCount;
    GetStVarParams( nVal, nValCount, bTextAsZero );

    PushDouble( div( nVal, nValCount));
}


void ScInterpreter::ScStDev( BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScStDev" );
    double nVal;
    double nValCount;
    GetStVarParams( nVal, nValCount, bTextAsZero );
    if (nValCount <= 1.0)
        PushError( errDivisionByZero );
    else
        PushDouble( sqrt( nVal / (nValCount - 1.0)));
}


void ScInterpreter::ScStDevP( BOOL bTextAsZero )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScStDevP" );
    double nVal;
    double nValCount;
    GetStVarParams( nVal, nValCount, bTextAsZero );
    if (nValCount == 0.0)
        PushError( errDivisionByZero );
    else
        PushDouble( sqrt( nVal / nValCount));

    /* this was: PushDouble( sqrt( div( nVal, nValCount)));
     *
     * Besides that the special NAN gets lost in the call through sqrt(),
     * unxlngi6.pro then looped back and forth somewhere between div() and
     * ::rtl::math::setNan(). Tests showed that
     *
     *      sqrt( div( 1, 0));
     *
     * produced a loop, but
     *
     *      double f1 = div( 1, 0);
     *      sqrt( f1 );
     *
     * was fine. There seems to be some compiler optimization problem. It does
     * not occur when compiled with debug=t.
     */
}


void ScInterpreter::ScColumns()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScColumns" );
    BYTE nParamCount = GetByte();
    ULONG nVal = 0;
    SCCOL nCol1;
    SCROW nRow1;
    SCTAB nTab1;
    SCCOL nCol2;
    SCROW nRow2;
    SCTAB nTab2;
    while (nParamCount-- > 0)
    {
        switch ( GetStackType() )
        {
            case svSingleRef:
                PopError();
                nVal++;
                break;
            case svDoubleRef:
                PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                nVal += static_cast<ULONG>(nTab2 - nTab1 + 1) *
                    static_cast<ULONG>(nCol2 - nCol1 + 1);
                break;
            case svMatrix:
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nC, nR;
                    pMat->GetDimensions(nC, nR);
                    nVal += nC;
                }
            }
            break;
            default:
                PopError();
                SetError(errIllegalParameter);
        }
    }
    PushDouble((double)nVal);
}


void ScInterpreter::ScRows()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScRows" );
    BYTE nParamCount = GetByte();
    ULONG nVal = 0;
    SCCOL nCol1;
    SCROW nRow1;
    SCTAB nTab1;
    SCCOL nCol2;
    SCROW nRow2;
    SCTAB nTab2;
    while (nParamCount-- > 0)
    {
        switch ( GetStackType() )
        {
            case svSingleRef:
                PopError();
                nVal++;
                break;
            case svDoubleRef:
                PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                nVal += static_cast<ULONG>(nTab2 - nTab1 + 1) *
                    static_cast<ULONG>(nRow2 - nRow1 + 1);
                break;
            case svMatrix:
            {
                ScMatrixRef pMat = PopMatrix();
                if (pMat)
                {
                    SCSIZE nC, nR;
                    pMat->GetDimensions(nC, nR);
                    nVal += nR;
                }
            }
            break;
            default:
                PopError();
                SetError(errIllegalParameter);
        }
    }
    PushDouble((double)nVal);
}

void ScInterpreter::ScTables()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTables" );
    BYTE nParamCount = GetByte();
    ULONG nVal;
    if ( nParamCount == 0 )
        nVal = pDok->GetTableCount();
    else
    {
        nVal = 0;
        SCCOL nCol1;
        SCROW nRow1;
        SCTAB nTab1;
        SCCOL nCol2;
        SCROW nRow2;
        SCTAB nTab2;
        while (nParamCount-- > 0)
        {
            switch ( GetStackType() )
            {
                case svSingleRef:
                    PopError();
                    nVal++;
                break;
                case svDoubleRef:
                    PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                    nVal += static_cast<ULONG>(nTab2 - nTab1 + 1);
                break;
                case svMatrix:
                    PopError();
                    nVal++;
                break;
                default:
                    PopError();
                    SetError( errIllegalParameter );
            }
        }
    }
    PushDouble( (double) nVal );
}


void ScInterpreter::ScColumn()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScColumn" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 0, 1 ) )
    {
        double nVal = 0;
        if (nParamCount == 0)
        {
            nVal = aPos.Col() + 1;
            if (bMatrixFormula)
            {
                SCCOL nCols;
                SCROW nRows;
                pMyFormulaCell->GetMatColsRows( nCols, nRows);
                ScMatrixRef pResMat = GetNewMat( static_cast<SCSIZE>(nCols), 1);
                if (pResMat)
                {
                    for (SCCOL i=0; i < nCols; ++i)
                        pResMat->PutDouble( nVal + i, static_cast<SCSIZE>(i), 0);
                    PushMatrix( pResMat);
                    return;
                }
            }
        }
        else
        {
            switch ( GetStackType() )
            {
                case svSingleRef :
                {
                    SCCOL nCol1;
                    SCROW nRow1;
                    SCTAB nTab1;
                    PopSingleRef( nCol1, nRow1, nTab1 );
                    nVal = (double) (nCol1 + 1);
                }
                break;
                case svDoubleRef :
                {
                    SCCOL nCol1;
                    SCROW nRow1;
                    SCTAB nTab1;
                    SCCOL nCol2;
                    SCROW nRow2;
                    SCTAB nTab2;
                    PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                    if (nCol2 > nCol1)
                    {
                        ScMatrixRef pResMat = GetNewMat(
                                static_cast<SCSIZE>(nCol2-nCol1+1), 1);
                        if (pResMat)
                        {
                            for (SCCOL i = nCol1; i <= nCol2; i++)
                                pResMat->PutDouble((double)(i+1),
                                        static_cast<SCSIZE>(i-nCol1), 0);
                            PushMatrix(pResMat);
                            return;
                        }
                        else
                            nVal = 0.0;
                    }
                    else
                        nVal = (double) (nCol1 + 1);
                }
                break;
                default:
                    SetError( errIllegalParameter );
                    nVal = 0.0;
            }
        }
        PushDouble( nVal );
    }
}


void ScInterpreter::ScRow()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScRow" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 0, 1 ) )
    {
        double nVal = 0;
        if (nParamCount == 0)
        {
            nVal = aPos.Row() + 1;
            if (bMatrixFormula)
            {
                SCCOL nCols;
                SCROW nRows;
                pMyFormulaCell->GetMatColsRows( nCols, nRows);
                ScMatrixRef pResMat = GetNewMat( 1, static_cast<SCSIZE>(nRows));
                if (pResMat)
                {
                    for (SCROW i=0; i < nRows; i++)
                        pResMat->PutDouble( nVal + i, 0, static_cast<SCSIZE>(i));
                    PushMatrix( pResMat);
                    return;
                }
            }
        }
        else
        {
            switch ( GetStackType() )
            {
                case svSingleRef :
                {
                    SCCOL nCol1;
                    SCROW nRow1;
                    SCTAB nTab1;
                    PopSingleRef( nCol1, nRow1, nTab1 );
                    nVal = (double) (nRow1 + 1);
                }
                break;
                case svDoubleRef :
                {
                    SCCOL nCol1;
                    SCROW nRow1;
                    SCTAB nTab1;
                    SCCOL nCol2;
                    SCROW nRow2;
                    SCTAB nTab2;
                    PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                    if (nRow2 > nRow1)
                    {
                        ScMatrixRef pResMat = GetNewMat( 1,
                                static_cast<SCSIZE>(nRow2-nRow1+1));
                        if (pResMat)
                        {
                            for (SCROW i = nRow1; i <= nRow2; i++)
                                pResMat->PutDouble((double)(i+1), 0,
                                        static_cast<SCSIZE>(i-nRow1));
                            PushMatrix(pResMat);
                            return;
                        }
                        else
                            nVal = 0.0;
                    }
                    else
                        nVal = (double) (nRow1 + 1);
                }
                break;
                default:
                    SetError( errIllegalParameter );
                    nVal = 0.0;
            }
        }
        PushDouble( nVal );
    }
}

void ScInterpreter::ScTable()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScTable" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 0, 1 ) )
    {
        SCTAB nVal = 0;
        if ( nParamCount == 0 )
            nVal = aPos.Tab() + 1;
        else
        {
            switch ( GetStackType() )
            {
                case svString :
                {
                    String aStr( PopString() );
                    if ( pDok->GetTable( aStr, nVal ) )
                        ++nVal;
                    else
                        SetError( errIllegalArgument );
                }
                break;
                case svSingleRef :
                {
                    SCCOL nCol1;
                    SCROW nRow1;
                    SCTAB nTab1;
                    PopSingleRef( nCol1, nRow1, nTab1 );
                    nVal = nTab1 + 1;
                }
                break;
                case svDoubleRef :
                {
                    SCCOL nCol1;
                    SCROW nRow1;
                    SCTAB nTab1;
                    SCCOL nCol2;
                    SCROW nRow2;
                    SCTAB nTab2;
                    PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                    nVal = nTab1 + 1;
                }
                break;
                default:
                    SetError( errIllegalParameter );
            }
            if ( nGlobalError )
                nVal = 0;
        }
        PushDouble( (double) nVal );
    }
}

/** returns -1 when the matrix value is smaller than the query value, 0 when
    they are equal, and 1 when the matrix value is larger than the query
    value. */
static sal_Int32 lcl_CompareMatrix2Query( SCSIZE i, const ScMatrix& rMat,
        const ScQueryEntry& rEntry)
{
    if (rMat.IsEmpty(i))
    {
        /* TODO: in case we introduced query for real empty this would have to
         * be changed! */
        return -1;      // empty always less than anything else
    }

    /* FIXME: what is an empty path (result of IF(false;true_path) in
     * comparisons? */

    if (rMat.IsValue(i))
    {
        if (rEntry.bQueryByString)
            return -1;  // numeric always less than string

        const double nVal1 = rMat.GetDouble(i);
        const double nVal2 = rEntry.nVal;
        if (nVal1 == nVal2)
            return 0;

        return nVal1 < nVal2 ? -1 : 1;
    }

    if (!rEntry.bQueryByString)
        return 1;       // string always greater than numeric

    if (!rEntry.pStr)
        // this should not happen!
        return 1;

    const String& rStr1 = rMat.GetString(i);
    const String& rStr2 = *rEntry.pStr;

    return ScGlobal::GetCollator()->compareString( rStr1, rStr2); // case-insensitive
}

/** returns the last item with the identical value as the original item
    value. */
static void lcl_GetLastMatch( SCSIZE& rIndex, const ScMatrix& rMat,
        SCSIZE nMatCount, bool bReverse)
{
    if (rMat.IsValue(rIndex))
    {
        double nVal = rMat.GetDouble(rIndex);
        if (bReverse)
            while (rIndex > 0 && rMat.IsValue(rIndex-1) &&
                    nVal == rMat.GetDouble(rIndex-1))
                --rIndex;
        else
            while (rIndex < nMatCount-1 && rMat.IsValue(rIndex+1) &&
                    nVal == rMat.GetDouble(rIndex+1))
                ++rIndex;
    }
    //! Order of IsEmptyPath, IsEmpty, IsString is significant!
    else if (rMat.IsEmptyPath(rIndex))
    {
        if (bReverse)
            while (rIndex > 0 && rMat.IsEmptyPath(rIndex-1))
                --rIndex;
        else
            while (rIndex < nMatCount-1 && rMat.IsEmptyPath(rIndex+1))
                ++rIndex;
    }
    else if (rMat.IsEmpty(rIndex))
    {
        if (bReverse)
            while (rIndex > 0 && rMat.IsEmpty(rIndex-1))
                --rIndex;
        else
            while (rIndex < nMatCount-1 && rMat.IsEmpty(rIndex+1))
                ++rIndex;
    }
    else if (rMat.IsString(rIndex))
    {
        String aStr( rMat.GetString(rIndex));
        if (bReverse)
            while (rIndex > 0 && rMat.IsString(rIndex-1) &&
                    aStr == rMat.GetString(rIndex-1))
                --rIndex;
        else
            while (rIndex < nMatCount-1 && rMat.IsString(rIndex+1) &&
                    aStr == rMat.GetString(rIndex+1))
                ++rIndex;
    }
    else
    {
        DBG_ERRORFILE("lcl_GetLastMatch: unhandled matrix type");
    }
}

void ScInterpreter::ScMatch()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScMatch" );
    ScMatrixRef pMatSrc = NULL;

    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 2, 3 ) )
    {
        double fTyp;
        if (nParamCount == 3)
            fTyp = GetDouble();
        else
            fTyp = 1.0;
        SCCOL nCol1 = 0;
        SCROW nRow1 = 0;
        SCTAB nTab1 = 0;
        SCCOL nCol2 = 0;
        SCROW nRow2 = 0;
        SCTAB nTab2 = 0;
        if (GetStackType() == svDoubleRef)
        {
            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
            if (nTab1 != nTab2 || (nCol1 != nCol2 && nRow1 != nRow2))
            {
                PushIllegalParameter();
                return;
            }
        }
        else if (GetStackType() == svMatrix)
        {
            pMatSrc = PopMatrix();
            if (!pMatSrc)
            {
                PushIllegalParameter();
                return;
            }
        }
        else
        {
            PushIllegalParameter();
            return;
        }
        if (nGlobalError == 0)
        {
            double fVal;
            String sStr;
            ScQueryParam rParam;
            rParam.nCol1       = nCol1;
            rParam.nRow1       = nRow1;
            rParam.nCol2       = nCol2;
            rParam.nTab        = nTab1;
            rParam.bMixedComparison = TRUE;

            ScQueryEntry& rEntry = rParam.GetEntry(0);
            rEntry.bDoQuery = TRUE;
            if (fTyp < 0.0)
                rEntry.eOp = SC_GREATER_EQUAL;
            else if (fTyp > 0.0)
                rEntry.eOp = SC_LESS_EQUAL;
            switch ( GetStackType() )
            {
                case svDouble:
                {
                    fVal = GetDouble();
                    rEntry.bQueryByString = FALSE;
                    rEntry.nVal = fVal;
                }
                break;
                case svString:
                {
                    sStr = GetString();
                    rEntry.bQueryByString = TRUE;
                    *rEntry.pStr = sStr;
                }
                break;
                case svDoubleRef :
                case svSingleRef :
                {
                    ScAddress aAdr;
                    if ( !PopDoubleRefOrSingleRef( aAdr ) )
                    {
                        PushInt(0);
                        return ;
                    }
                    ScBaseCell* pCell = GetCell( aAdr );
                    if (HasCellValueData(pCell))
                    {
                        fVal = GetCellValue( aAdr, pCell );
                        rEntry.bQueryByString = FALSE;
                        rEntry.nVal = fVal;
                    }
                    else
                    {
                        GetCellString(sStr, pCell);
                        rEntry.bQueryByString = TRUE;
                        *rEntry.pStr = sStr;
                    }
                }
                break;
                case svMatrix :
                {
                    ScMatValType nType = GetDoubleOrStringFromMatrix(
                            rEntry.nVal, *rEntry.pStr);
                    rEntry.bQueryByString = ScMatrix::IsNonValueType( nType);
                }
                break;
                default:
                {
                    PushIllegalParameter();
                    return;
                }
            }
            if ( rEntry.bQueryByString )
                rParam.bRegExp = MayBeRegExp( *rEntry.pStr, pDok );

            if (pMatSrc) // The source data is matrix array.
            {
                SCSIZE nC, nR;
                pMatSrc->GetDimensions( nC, nR);
                if (nC > 1 && nR > 1)
                {
                    // The source matrix must be a vector.
                    PushIllegalParameter();
                    return;
                }
                SCSIZE nMatCount = (nC == 1) ? nR : nC;

                // simple serial search for equality mode (source data doesn't
                // need to be sorted).

                if (rEntry.eOp == SC_EQUAL)
                {
                    for (SCSIZE i = 0; i < nMatCount; ++i)
                    {
                        if (lcl_CompareMatrix2Query( i, *pMatSrc, rEntry) == 0)
                        {
                            PushDouble(i+1); // found !
                            return;
                        }
                    }
                    PushNA(); // not found
                    return;
                }

                // binary search for non-equality mode (the source data is
                // assumed to be sorted).

                bool bAscOrder = (rEntry.eOp == SC_LESS_EQUAL);
                SCSIZE nFirst = 0, nLast = nMatCount-1, nHitIndex = 0;
                for (SCSIZE nLen = nLast-nFirst; nLen > 0; nLen = nLast-nFirst)
                {
                    SCSIZE nMid = nFirst + nLen/2;
                    sal_Int32 nCmp = lcl_CompareMatrix2Query( nMid, *pMatSrc, rEntry);
                    if (nCmp == 0)
                    {
                        // exact match.  find the last item with the same value.
                        lcl_GetLastMatch( nMid, *pMatSrc, nMatCount, !bAscOrder);
                        PushDouble( nMid+1);
                        return;
                    }

                    if (nLen == 1) // first and last items are next to each other.
                    {
                        if (nCmp < 0)
                            nHitIndex = bAscOrder ? nLast : nFirst;
                        else
                            nHitIndex = bAscOrder ? nFirst : nLast;
                        break;
                    }

                    if (nCmp < 0)
                    {
                        if (bAscOrder)
                            nFirst = nMid;
                        else
                            nLast = nMid;
                    }
                    else
                    {
                        if (bAscOrder)
                            nLast = nMid;
                        else
                            nFirst = nMid;
                    }
                }

                if (nHitIndex == nMatCount-1) // last item
                {
                    sal_Int32 nCmp = lcl_CompareMatrix2Query( nHitIndex, *pMatSrc, rEntry);
                    if ((bAscOrder && nCmp <= 0) || (!bAscOrder && nCmp >= 0))
                    {
                        // either the last item is an exact match or the real
                        // hit is beyond the last item.
                        PushDouble( nHitIndex+1);
                        return;
                    }
                }

                if (nHitIndex > 0) // valid hit must be 2nd item or higher
                {
                    PushDouble( nHitIndex); // non-exact match
                    return;
                }

                PushNA();
                return;
            }

            SCCOLROW nDelta = 0;
            if (nCol1 == nCol2)
            {                                           // search row in column
                rParam.nRow2 = nRow2;
                rEntry.nField = nCol1;
                ScAddress aResultPos( nCol1, nRow1, nTab1);
                if (!LookupQueryWithCache( aResultPos, rParam))
                {
                    PushNA();
                    return;
                }
                nDelta = aResultPos.Row() - nRow1;
            }
            else
            {                                           // search column in row
                SCCOL nC;
                rParam.bByRow = FALSE;
                rParam.nRow2 = nRow1;
                rEntry.nField = nCol1;
                ScQueryCellIterator aCellIter(pDok, nTab1, rParam, FALSE);
                // Advance Entry.nField in Iterator if column changed
                aCellIter.SetAdvanceQueryParamEntryField( TRUE );
                if (fTyp == 0.0)
                {                                       // EQUAL
                    if ( aCellIter.GetFirst() )
                        nC = aCellIter.GetCol();
                    else
                    {
                        PushNA();
                        return;
                    }
                }
                else
                {                                       // <= or >=
                    SCROW nR;
                    if ( !aCellIter.FindEqualOrSortedLastInRange( nC, nR ) )
                    {
                        PushNA();
                        return;
                    }
                }
                nDelta = nC - nCol1;
            }
            PushDouble((double) (nDelta + 1));
        }
        else
            PushIllegalParameter();
    }
}


void ScInterpreter::ScCountEmptyCells()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCountEmptyCells" );
    if ( MustHaveParamCount( GetByte(), 1 ) )
    {
        ULONG nMaxCount = 0, nCount = 0;
        CellType eCellType;
        switch (GetStackType())
        {
            case svSingleRef :
            {
                nMaxCount = 1;
                ScAddress aAdr;
                PopSingleRef( aAdr );
                eCellType = GetCellType( GetCell( aAdr ) );
                if (eCellType != CELLTYPE_NONE && eCellType != CELLTYPE_NOTE)
                    nCount = 1;
            }
            break;
            case svDoubleRef :
            case svRefList :
            {
                ScRange aRange;
                short nParam = 1;
                size_t nRefInList = 0;
                while (nParam-- > 0)
                {
                    PopDoubleRef( aRange, nParam, nRefInList);
                    nMaxCount +=
                        static_cast<ULONG>(aRange.aEnd.Row() - aRange.aStart.Row() + 1) *
                        static_cast<ULONG>(aRange.aEnd.Col() - aRange.aStart.Col() + 1) *
                        static_cast<ULONG>(aRange.aEnd.Tab() - aRange.aStart.Tab() + 1);
                    ScBaseCell* pCell;
                    ScCellIterator aDocIter( pDok, aRange, glSubTotal);
                    if ( (pCell = aDocIter.GetFirst()) != NULL )
                    {
                        do
                        {
                            if ((eCellType = pCell->GetCellType()) != CELLTYPE_NONE
                                    && eCellType != CELLTYPE_NOTE)
                                nCount++;
                        } while ( (pCell = aDocIter.GetNext()) != NULL );
                    }
                }
            }
            break;
            default : SetError(errIllegalParameter); break;
        }
        PushDouble(nMaxCount - nCount);
    }
}


void ScInterpreter::ScCountIf()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCountIf" );
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        String rString;
        double fVal = 0.0;
        BOOL bIsString = TRUE;
        switch ( GetStackType() )
        {
            case svDoubleRef :
            case svSingleRef :
            {
                ScAddress aAdr;
                if ( !PopDoubleRefOrSingleRef( aAdr ) )
                {
                    PushInt(0);
                    return ;
                }
                ScBaseCell* pCell = GetCell( aAdr );
                switch ( GetCellType( pCell ) )
                {
                    case CELLTYPE_VALUE :
                        fVal = GetCellValue( aAdr, pCell );
                        bIsString = FALSE;
                        break;
                    case CELLTYPE_FORMULA :
                        if( ((ScFormulaCell*)pCell)->IsValue() )
                        {
                            fVal = GetCellValue( aAdr, pCell );
                            bIsString = FALSE;
                        }
                        else
                            GetCellString(rString, pCell);
                        break;
                    case CELLTYPE_STRING :
                    case CELLTYPE_EDIT :
                        GetCellString(rString, pCell);
                        break;
                    default:
                        fVal = 0.0;
                        bIsString = FALSE;
                }
            }
            break;
            case svMatrix :
            {
                ScMatValType nType = GetDoubleOrStringFromMatrix( fVal,
                        rString);
                bIsString = ScMatrix::IsNonValueType( nType);
            }
            break;
            case svString:
                rString = GetString();
            break;
            default:
            {
                fVal = GetDouble();
                bIsString = FALSE;
            }
        }
        double fSum = 0.0;
        short nParam = 1;
        size_t nRefInList = 0;
        while (nParam-- > 0)
        {
            SCCOL nCol1;
            SCROW nRow1;
            SCTAB nTab1;
            SCCOL nCol2;
            SCROW nRow2;
            SCTAB nTab2;
            ScMatrixRef pQueryMatrix;
            switch ( GetStackType() )
            {
                case svDoubleRef :
                case svRefList :
                    {
                        ScRange aRange;
                        PopDoubleRef( aRange, nParam, nRefInList);
                        aRange.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                    }
                    break;
                case svSingleRef :
                    PopSingleRef( nCol1, nRow1, nTab1 );
                    nCol2 = nCol1;
                    nRow2 = nRow1;
                    nTab2 = nTab1;
                    break;
                case svMatrix:
                    {
                        pQueryMatrix = PopMatrix();
                        if (!pQueryMatrix)
                        {
                            PushIllegalParameter();
                            return;
                        }
                        nCol1 = 0;
                        nRow1 = 0;
                        nTab1 = 0;
                        SCSIZE nC, nR;
                        pQueryMatrix->GetDimensions( nC, nR);
                        nCol2 = static_cast<SCCOL>(nC - 1);
                        nRow2 = static_cast<SCROW>(nR - 1);
                        nTab2 = 0;
                    }
                    break;
                default:
                    PushIllegalParameter();
                    return ;
            }
            if ( nTab1 != nTab2 )
            {
                PushIllegalParameter();
                return;
            }
            if (nCol1 > nCol2)
            {
                PushIllegalParameter();
                return;
            }
            if (nGlobalError == 0)
            {
                ScQueryParam rParam;
                rParam.nRow1       = nRow1;
                rParam.nRow2       = nRow2;

                ScQueryEntry& rEntry = rParam.GetEntry(0);
                rEntry.bDoQuery = TRUE;
                if (!bIsString)
                {
                    rEntry.bQueryByString = FALSE;
                    rEntry.nVal = fVal;
                    rEntry.eOp = SC_EQUAL;
                }
                else
                {
                    rParam.FillInExcelSyntax(rString, 0);
                    sal_uInt32 nIndex = 0;
                    rEntry.bQueryByString =
                        !(pFormatter->IsNumberFormat(
                                    *rEntry.pStr, nIndex, rEntry.nVal));
                    if ( rEntry.bQueryByString )
                        rParam.bRegExp = MayBeRegExp( *rEntry.pStr, pDok );
                }
                rParam.nCol1  = nCol1;
                rParam.nCol2  = nCol2;
                rEntry.nField = nCol1;
                if (pQueryMatrix)
                {
                    // Never case-sensitive.
                    ScCompareOptions aOptions( pDok, rEntry, rParam.bRegExp);
                    ScMatrixRef pResultMatrix = QueryMat( pQueryMatrix, aOptions);
                    if (nGlobalError || !pResultMatrix)
                    {
                        PushIllegalParameter();
                        return;
                    }

                    SCSIZE nSize = pResultMatrix->GetElementCount();
                    for (SCSIZE nIndex = 0; nIndex < nSize; ++nIndex)
                    {
                        if (pResultMatrix->IsValue( nIndex) && 
                                pResultMatrix->GetDouble( nIndex))
                            ++fSum;
                    }
                }
                else
                {
                    ScQueryCellIterator aCellIter(pDok, nTab1, rParam, FALSE);
                    // Entry.nField im Iterator bei Spaltenwechsel weiterschalten
                    aCellIter.SetAdvanceQueryParamEntryField( TRUE );
                    if ( aCellIter.GetFirst() )
                    {
                        do
                        {
                            fSum++;
                        } while ( aCellIter.GetNext() );
                    }
                }
            }
            else
            {
                PushIllegalParameter();
                return;
            }
        }
        PushDouble(fSum);
    }
}


void ScInterpreter::ScSumIf()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSumIf" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 2, 3 ) )
    {
        SCCOL nCol3 = 0;
        SCROW nRow3 = 0;
        SCTAB nTab3 = 0;

        ScMatrixRef pSumExtraMatrix;
        bool bSumExtraRange = (nParamCount == 3);
        if (bSumExtraRange)
        {
            // Save only the upperleft cell in case of cell range.  The geometry
            // of the 3rd parameter is taken from the 1st parameter.

            switch ( GetStackType() )
            {
                case svDoubleRef :
                {
                    SCCOL nColJunk = 0;
                    SCROW nRowJunk = 0;
                    SCTAB nTabJunk = 0;
                    PopDoubleRef( nCol3, nRow3, nTab3, nColJunk, nRowJunk, nTabJunk );
                    if ( nTabJunk != nTab3 )
                    {
                        PushIllegalParameter();
                        return;
                    }
                }
                break;
                case svSingleRef :
                    PopSingleRef( nCol3, nRow3, nTab3 );
                break;
                case svMatrix:
                    pSumExtraMatrix = PopMatrix();
                    //! nCol3, nRow3, nTab3 remain 0
                break;
                default:
                    PushIllegalParameter();
                    return ;
            }
        }
        String rString;
        double fVal = 0.0;
        BOOL bIsString = TRUE;
        switch ( GetStackType() )
        {
            case svDoubleRef :
            case svSingleRef :
            {
                ScAddress aAdr;
                if ( !PopDoubleRefOrSingleRef( aAdr ) )
                {
                    PushInt(0);
                    return ;
                }
                ScBaseCell* pCell = GetCell( aAdr );
                switch ( GetCellType( pCell ) )
                {
                    case CELLTYPE_VALUE :
                        fVal = GetCellValue( aAdr, pCell );
                        bIsString = FALSE;
                        break;
                    case CELLTYPE_FORMULA :
                        if( ((ScFormulaCell*)pCell)->IsValue() )
                        {
                            fVal = GetCellValue( aAdr, pCell );
                            bIsString = FALSE;
                        }
                        else
                            GetCellString(rString, pCell);
                        break;
                    case CELLTYPE_STRING :
                    case CELLTYPE_EDIT :
                        GetCellString(rString, pCell);
                        break;
                    default:
                        fVal = 0.0;
                        bIsString = FALSE;
                }
            }
            break;
            case svString:
                rString = GetString();
            break;
            case svMatrix :
            {
                ScMatValType nType = GetDoubleOrStringFromMatrix( fVal,
                        rString);
                bIsString = ScMatrix::IsNonValueType( nType);
            }
            break;
            default:
            {
                fVal = GetDouble();
                bIsString = FALSE;
            }
        }

        double fSum = 0.0;
        double fMem = 0.0;
        BOOL bNull = TRUE;
        short nParam = 1;
        size_t nRefInList = 0;
        while (nParam-- > 0)
        {
            SCCOL nCol1;
            SCROW nRow1;
            SCTAB nTab1;
            SCCOL nCol2;
            SCROW nRow2;
            SCTAB nTab2;
            ScMatrixRef pQueryMatrix;
            switch ( GetStackType() )
            {
                case svRefList :
                    if (bSumExtraRange)
                    {
                        PushIllegalParameter();
                        return;
                    }
                    else
                    {
                        ScRange aRange;
                        PopDoubleRef( aRange, nParam, nRefInList);
                        aRange.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                    }
                    break;
                case svDoubleRef :
                    PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
                    break;
                case svSingleRef :
                    PopSingleRef( nCol1, nRow1, nTab1 );
                    nCol2 = nCol1;
                    nRow2 = nRow1;
                    nTab2 = nTab1;
                    break;
                case svMatrix:
                    {
                        pQueryMatrix = PopMatrix();
                        if (!pQueryMatrix)
                        {
                            PushIllegalParameter();
                            return;
                        }
                        nCol1 = 0;
                        nRow1 = 0;
                        nTab1 = 0;
                        SCSIZE nC, nR;
                        pQueryMatrix->GetDimensions( nC, nR);
                        nCol2 = static_cast<SCCOL>(nC - 1);
                        nRow2 = static_cast<SCROW>(nR - 1);
                        nTab2 = 0;
                    }
                    break;
                default:
                    PushIllegalParameter();
                    return ;
            }
            if ( nTab1 != nTab2 )
            {
                PushIllegalArgument();
                return;
            }

            if (bSumExtraRange)
            {
                // Take the range geometry of the 1st parameter and apply it to
                // the 3rd. If parts of the resulting range would point outside
                // the sheet, don't complain but silently ignore and simply cut
                // them away, this is what Xcl does :-/

                // For the cut-away part we also don't need to determine the
                // criteria match, so shrink the source range accordingly,
                // instead of the result range.
                SCCOL nColDelta = nCol2 - nCol1;
                SCROW nRowDelta = nRow2 - nRow1;
                SCCOL nMaxCol;
                SCROW nMaxRow;
                if (pSumExtraMatrix)
                {
                    SCSIZE nC, nR;
                    pSumExtraMatrix->GetDimensions( nC, nR);
                    nMaxCol = static_cast<SCCOL>(nC - 1);
                    nMaxRow = static_cast<SCROW>(nR - 1);
                }
                else
                {
                    nMaxCol = MAXCOL;
                    nMaxRow = MAXROW;
                }
                if (nCol3 + nColDelta > nMaxCol)
                {
                    SCCOL nNewDelta = nMaxCol - nCol3;
                    nCol2 = nCol1 + nNewDelta;
                }

                if (nRow3 + nRowDelta > nMaxRow)
                {
                    SCROW nNewDelta = nMaxRow - nRow3;
                    nRow2 = nRow1 + nNewDelta;
                }
            }
            else
            {
                nCol3 = nCol1;
                nRow3 = nRow1;
                nTab3 = nTab1;
            }

            if (nGlobalError == 0)
            {
                ScQueryParam rParam;
                rParam.nRow1       = nRow1;
                rParam.nRow2       = nRow2;

                ScQueryEntry& rEntry = rParam.GetEntry(0);
                rEntry.bDoQuery = TRUE;
                if (!bIsString)
                {
                    rEntry.bQueryByString = FALSE;
                    rEntry.nVal = fVal;
                    rEntry.eOp = SC_EQUAL;
                }
                else
                {
                    rParam.FillInExcelSyntax(rString, 0);
                    sal_uInt32 nIndex = 0;
                    rEntry.bQueryByString =
                        !(pFormatter->IsNumberFormat(
                                    *rEntry.pStr, nIndex, rEntry.nVal));
                    if ( rEntry.bQueryByString )
                        rParam.bRegExp = MayBeRegExp( *rEntry.pStr, pDok );
                }
                ScAddress aAdr;
                aAdr.SetTab( nTab3 );
                rParam.nCol1  = nCol1;
                rParam.nCol2  = nCol2;
                rEntry.nField = nCol1;
                SCsCOL nColDiff = nCol3 - nCol1;
                SCsROW nRowDiff = nRow3 - nRow1;
                if (pQueryMatrix)
                {
                    // Never case-sensitive.
                    ScCompareOptions aOptions( pDok, rEntry, rParam.bRegExp);
                    ScMatrixRef pResultMatrix = QueryMat( pQueryMatrix, aOptions);
                    if (nGlobalError || !pResultMatrix)
                    {
                        PushIllegalParameter();
                        return;
                    }

                    if (pSumExtraMatrix)
                    {
                        for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
                        {
                            for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
                            {
                                if (pResultMatrix->IsValue( nCol, nRow) && 
                                        pResultMatrix->GetDouble( nCol, nRow))
                                {
                                    SCSIZE nC = nCol + nColDiff;
                                    SCSIZE nR = nRow + nRowDiff;
                                    if (pSumExtraMatrix->IsValue( nC, nR))
                                    {
                                        fVal = pSumExtraMatrix->GetDouble( nC, nR);
                                        if ( bNull && fVal != 0.0 )
                                        {
                                            bNull = FALSE;
                                            fMem = fVal;
                                        }
                                        else
                                            fSum += fVal;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        for (SCCOL nCol = nCol1; nCol <= nCol2; ++nCol)
                        {
                            for (SCROW nRow = nRow1; nRow <= nRow2; ++nRow)
                            {
                                if (pResultMatrix->GetDouble( nCol, nRow))
                                {
                                    aAdr.SetCol( nCol + nColDiff);
                                    aAdr.SetRow( nRow + nRowDiff);
                                    ScBaseCell* pCell = GetCell( aAdr );
                                    if ( HasCellValueData(pCell) )
                                    {
                                        fVal = GetCellValue( aAdr, pCell );
                                        if ( bNull && fVal != 0.0 )
                                        {
                                            bNull = FALSE;
                                            fMem = fVal;
                                        }
                                        else
                                            fSum += fVal;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    ScQueryCellIterator aCellIter(pDok, nTab1, rParam, FALSE);
                    // Increment Entry.nField in iterator when switching to next column.
                    aCellIter.SetAdvanceQueryParamEntryField( TRUE );
                    if ( aCellIter.GetFirst() )
                    {
                        if (pSumExtraMatrix)
                        {
                            do
                            {
                                SCSIZE nC = aCellIter.GetCol() + nColDiff;
                                SCSIZE nR = aCellIter.GetRow() + nRowDiff;
                                if (pSumExtraMatrix->IsValue( nC, nR))
                                {
                                    fVal = pSumExtraMatrix->GetDouble( nC, nR);
                                    if ( bNull && fVal != 0.0 )
                                    {
                                        bNull = FALSE;
                                        fMem = fVal;
                                    }
                                    else
                                        fSum += fVal;
                                }
                            } while ( aCellIter.GetNext() );
                        }
                        else
                        {
                            do
                            {
                                aAdr.SetCol( aCellIter.GetCol() + nColDiff);
                                aAdr.SetRow( aCellIter.GetRow() + nRowDiff);
                                ScBaseCell* pCell = GetCell( aAdr );
                                if ( HasCellValueData(pCell) )
                                {
                                    fVal = GetCellValue( aAdr, pCell );
                                    if ( bNull && fVal != 0.0 )
                                    {
                                        bNull = FALSE;
                                        fMem = fVal;
                                    }
                                    else
                                        fSum += fVal;
                                }
                            } while ( aCellIter.GetNext() );
                        }
                    }
                }
            }
            else
            {
                PushIllegalParameter();
                return;
            }
        }
        PushDouble( ::rtl::math::approxAdd( fSum, fMem ) );
    }
}


void ScInterpreter::ScLookup()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScLookup" );
    BYTE nParamCount = GetByte();
    if ( !MustHaveParamCount( nParamCount, 2, 3 ) )
        return ;

    ScMatrixRef pDataMat = NULL, pResMat = NULL;
    SCCOL nCol1 = 0, nCol2 = 0, nResCol1 = 0, nResCol2 = 0;
    SCROW nRow1 = 0, nRow2 = 0, nResRow1 = 0, nResRow2 = 0;
    SCTAB nTab1 = 0, nResTab = 0;
    SCSIZE nLenMajor = 0;   // length of major direction
    bool bVertical = true;  // whether to lookup vertically or horizontally

    // The third parameter, result array, for double, string and single reference.
    double fResVal = 0.0;
    String aResStr;
    ScAddress aResAdr;
    StackVar eResArrayType = svUnknown;

    if (nParamCount == 3)
    {
        eResArrayType = GetStackType();
        switch (eResArrayType)
        {
            case svDoubleRef:
            {
                SCTAB nTabJunk;
                PopDoubleRef(nResCol1, nResRow1, nResTab, 
                             nResCol2, nResRow2, nTabJunk);
                if (nResTab != nTabJunk || 
                    ((nResRow2 - nResRow1) > 0 && (nResCol2 - nResCol1) > 0))
                {
                    // The result array must be a vector.
                    PushIllegalParameter();
                    return;
                }
            }
            break;
            case svMatrix:
            {
                pResMat = PopMatrix();
                if (!pResMat)
                {
                    PushIllegalParameter();
                    return;
                }
                SCSIZE nC, nR;
                pResMat->GetDimensions(nC, nR);
                if (nC != 1 && nR != 1)
                {
                    // Result matrix must be a vector.
                    PushIllegalParameter();
                    return;
                }
            }
            break;
            case svDouble:
                fResVal = GetDouble();
            break;
            case svString:
                aResStr = GetString();
            break;
            case svSingleRef:
                PopSingleRef( aResAdr );
            break;
            default:
                PushIllegalParameter();
                return;
        }
    }

    // For double, string and single reference.
    double fDataVal = 0.0;
    String aDataStr;
    ScAddress aDataAdr;
    bool bValueData = false;

    // Get the data-result range and also determine whether this is vertical
    // lookup or horizontal lookup.

    StackVar eDataArrayType = GetStackType();
    switch (eDataArrayType)
    {
        case svDoubleRef:
        {
            SCTAB nTabJunk;
            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTabJunk);
            if (nTab1 != nTabJunk)
            {
                PushIllegalParameter();
                return;
            }
            bVertical = (nRow2 - nRow1) >= (nCol2 - nCol1);
            nLenMajor = bVertical ? nRow2 - nRow1 + 1 : nCol2 - nCol1 + 1;
        }
        break;
        case svMatrix:
        {
            pDataMat = PopMatrix();
            if (!pDataMat)
            {
                PushIllegalParameter();
                return;
            }

            SCSIZE nC, nR;
            pDataMat->GetDimensions(nC, nR);
            bVertical = (nR >= nC);
            nLenMajor = bVertical ? nR : nC;
        }
        break;
        case svDouble:
        {
            fDataVal = GetDouble();
            bValueData = true;
        }
        break;
        case svString:
        {
            aDataStr = GetString();
        }
        break;
        case svSingleRef:
        {
            PopSingleRef( aDataAdr );
            const ScBaseCell* pDataCell = GetCell( aDataAdr );
            if (HasCellEmptyData( pDataCell))
            {
                // Empty cells aren't found anywhere, bail out early.
                SetError( NOTAVAILABLE);
            }
            else if (HasCellValueData( pDataCell))
            {
                fDataVal = GetCellValue( aDataAdr, pDataCell );
                bValueData = true;
            }
            else
                GetCellString( aDataStr, pDataCell );
        }
        break;
        default:
            SetError( errIllegalParameter);
    }


    if (nGlobalError)
    {
        PushError( nGlobalError);
        return;
    }

    // Get the lookup value.

    ScQueryParam aParam;
    ScQueryEntry& rEntry = aParam.GetEntry(0);
    if ( !FillEntry(rEntry) )
        return;

    if ( eDataArrayType == svDouble || eDataArrayType == svString ||
            eDataArrayType == svSingleRef )
    {
        // Delta position for a single value is always 0.

        // Found if data <= query, but not if query is string and found data is 
        // numeric or vice versa. This is how Excel does it but doesn't
        // document it.

        bool bFound = false;
        if ( bValueData )
        {
            if ( rEntry.bQueryByString )
                bFound = false;
            else
                bFound = (fDataVal <= rEntry.nVal);
        }
        else
        {
            if ( !rEntry.bQueryByString )
                bFound = false;
            else
                bFound = (ScGlobal::GetCollator()->compareString( aDataStr, *rEntry.pStr) <= 0);
        }

        if (!bFound)
        {
            PushNA();
            return;
        }

        if (pResMat)
        {
            if (pResMat->IsValue( 0 ))
                PushDouble(pResMat->GetDouble( 0 ));
            else
                PushString(pResMat->GetString( 0 ));
        }
        else if (nParamCount == 3)
        {
            switch (eResArrayType)
            {
                case svDouble:
                    PushDouble( fResVal );
                    break;
                case svString:
                    PushString( aResStr );
                    break;
                case svDoubleRef:
                    aResAdr.Set( nResCol1, nResRow1, nResTab);
                    // fallthru
                case svSingleRef:
                    PushCellResultToken( true, aResAdr, NULL, NULL);
                    break;
                default:
                    DBG_ERRORFILE( "ScInterpreter::ScLookup: unhandled eResArrayType, single value data");
            }
        }
        else
        {
            switch (eDataArrayType)
            {
                case svDouble:
                    PushDouble( fDataVal );
                    break;
                case svString:
                    PushString( aDataStr );
                    break;
                case svSingleRef:
                    PushCellResultToken( true, aDataAdr, NULL, NULL);
                    break;
                default:
                    DBG_ERRORFILE( "ScInterpreter::ScLookup: unhandled eDataArrayType, single value data");
            }
        }
        return;
    }

    // Now, perform the search to compute the delta position (nDelta).

    if (pDataMat)
    {
        // Data array is given as a matrix.
        rEntry.bDoQuery = true;
        rEntry.eOp = SC_LESS_EQUAL;
        bool bFound = false;

        SCSIZE nC, nR;
        pDataMat->GetDimensions(nC, nR);

        // In case of non-vector matrix, only search the first row or column.
        ScMatrixRef pDataMat2;
        if (bVertical)
        {
            ScMatrixRef pTempMat(new ScMatrix(1, nR));
            for (SCSIZE i = 0; i < nR; ++i)
                if (pDataMat->IsValue(0, i))
                    pTempMat->PutDouble(pDataMat->GetDouble(0, i), 0, i);
                else
                    pTempMat->PutString(pDataMat->GetString(0, i), 0, i);
            pDataMat2 = pTempMat;
        }
        else
        {
            ScMatrixRef pTempMat(new ScMatrix(nC, 1));
            for (SCSIZE i = 0; i < nC; ++i)
                if (pDataMat->IsValue(i, 0))
                    pTempMat->PutDouble(pDataMat->GetDouble(i, 0), i, 0);
                else
                    pTempMat->PutString(pDataMat->GetString(i, 0), i, 0);
            pDataMat2 = pTempMat;
        }

        // binary search for non-equality mode (the source data is
        // assumed to be sorted in ascending order).

        SCCOLROW nDelta = -1;

        SCSIZE nFirst = 0, nLast = nLenMajor-1; //, nHitIndex = 0;
        for (SCSIZE nLen = nLast-nFirst; nLen > 0; nLen = nLast-nFirst)
        {
            SCSIZE nMid = nFirst + nLen/2;
            sal_Int32 nCmp = lcl_CompareMatrix2Query( nMid, *pDataMat2, rEntry);
            if (nCmp == 0)
            {
                // exact match.  find the last item with the same value.
                lcl_GetLastMatch( nMid, *pDataMat2, nLenMajor, false);
                nDelta = nMid;
                bFound = true;
                break;
            }

            if (nLen == 1) // first and last items are next to each other.
            {
                nDelta = nCmp < 0 ? nLast - 1 : nFirst - 1;
                // If already the 1st item is greater there's nothing found.
                bFound = (nDelta >= 0);
                break;
            }

            if (nCmp < 0)
                nFirst = nMid;
            else
                nLast = nMid;
        }

        if (nDelta == static_cast<SCCOLROW>(nLenMajor-2)) // last item
        {
            sal_Int32 nCmp = lcl_CompareMatrix2Query(nDelta+1, *pDataMat2, rEntry);
            if (nCmp <= 0)
            {
                // either the last item is an exact match or the real
                // hit is beyond the last item.
                nDelta += 1;
                bFound = true;
            }
        }
        else if (nDelta > 0) // valid hit must be 2nd item or higher
        {
            // non-exact match
            bFound = true;
        }

        // With 0-9 < A-Z, if query is numeric and data found is string, or
        // vice versa, the (yet another undocumented) Excel behavior is to 
        // return #N/A instead.

        if (bFound)
        {
            SCCOLROW i = nDelta;
            SCSIZE n = pDataMat->GetElementCount();
            if (static_cast<SCSIZE>(i) >= n)
                i = static_cast<SCCOLROW>(n);
            if (bool(rEntry.bQueryByString) == bool(pDataMat->IsValue(i)))
                bFound = false;
        }

        if (!bFound)
        {
            PushNA();
            return;
        }

        // Now that we've found the delta, push the result back to the cell.

        if (pResMat)
        {
            // result array is matrix.
            if (static_cast<SCSIZE>(nDelta) >= pResMat->GetElementCount())
            {
                PushNA();
                return;
            }
            if (pResMat->IsValue(nDelta))
                PushDouble(pResMat->GetDouble(nDelta));
            else
                PushString(pResMat->GetString(nDelta));
        }
        else if (nParamCount == 3)
        {
            // result array is cell range.
            ScAddress aAdr;
            aAdr.SetTab(nResTab);
            bool bResVertical = (nResRow2 - nResRow1) > 0;
            if (bResVertical)
            {
                SCROW nTempRow = static_cast<SCROW>(nResRow1 + nDelta);
                if (nTempRow > MAXROW)
                {
                    PushDouble(0);
                    return;
                }
                aAdr.SetCol(nResCol1);
                aAdr.SetRow(nTempRow);
            }
            else
            {
                SCCOL nTempCol = static_cast<SCCOL>(nResCol1 + nDelta);
                if (nTempCol > MAXCOL)
                {
                    PushDouble(0);
                    return;
                }
                aAdr.SetCol(nTempCol);
                aAdr.SetRow(nResRow1);
            }
            PushCellResultToken(true, aAdr, NULL, NULL);
        }
        else
        {
            // no result array.  Use the data array to get the final value from.
            if (bVertical)
            {
                if (pDataMat->IsValue(nC-1, nDelta))
                    PushDouble(pDataMat->GetDouble(nC-1, nDelta));
                else
                    PushString(pDataMat->GetString(nC-1, nDelta));
            }
            else
            {
                if (pDataMat->IsValue(nDelta, nR-1))
                    PushDouble(pDataMat->GetDouble(nDelta, nR-1));
                else
                    PushString(pDataMat->GetString(nDelta, nR-1));
            }
        }

        return;
    }

    // Perform cell range search.

    aParam.nCol1            = nCol1;
    aParam.nRow1            = nRow1;
    aParam.nCol2            = bVertical ? nCol1 : nCol2;
    aParam.nRow2            = bVertical ? nRow2 : nRow1;
    aParam.bByRow           = bVertical;
    aParam.bMixedComparison = true;

    rEntry.bDoQuery = TRUE;
    rEntry.eOp = SC_LESS_EQUAL;
    rEntry.nField = nCol1;
    if ( rEntry.bQueryByString )
        aParam.bRegExp = MayBeRegExp( *rEntry.pStr, pDok );

    ScQueryCellIterator aCellIter(pDok, nTab1, aParam, FALSE);
    SCCOL nC;
    SCROW nR;
    // Advance Entry.nField in iterator upon switching columns if
    // lookup in row.
    aCellIter.SetAdvanceQueryParamEntryField(!bVertical);
    if ( !aCellIter.FindEqualOrSortedLastInRange(nC, nR) )
    {
        PushNA();
        return;
    }

    SCCOLROW nDelta = bVertical ? static_cast<SCSIZE>(nR-nRow1) : static_cast<SCSIZE>(nC-nCol1);

    if (pResMat)
    {
        // Use the matrix result array.
        if (pResMat->IsValue(nDelta))
            PushDouble(pResMat->GetDouble(nDelta));
        else
            PushString(pResMat->GetString(nDelta));
    }
    else if (nParamCount == 3)
    {
        switch (eResArrayType)
        {
            case svDoubleRef:
            {
                // Use the result array vector.  Note that the result array is assumed
                // to be a vector (i.e. 1-dimensinoal array).

                ScAddress aAdr;
                aAdr.SetTab(nResTab);
                bool bResVertical = (nResRow2 - nResRow1) > 0;
                if (bResVertical)
                {
                    SCROW nTempRow = static_cast<SCROW>(nResRow1 + nDelta);
                    if (nTempRow > MAXROW)
                    {
                        PushDouble(0);
                        return;
                    }
                    aAdr.SetCol(nResCol1);
                    aAdr.SetRow(nTempRow);
                }
                else
                {
                    SCCOL nTempCol = static_cast<SCCOL>(nResCol1 + nDelta);
                    if (nTempCol > MAXCOL)
                    {
                        PushDouble(0);
                        return;
                    }
                    aAdr.SetCol(nTempCol);
                    aAdr.SetRow(nResRow1);
                }
                PushCellResultToken( true, aAdr, NULL, NULL);
            }
            break;
            case svDouble:
            case svString:
            case svSingleRef:
            {
                if (nDelta != 0)
                    PushNA();
                else
                {
                    switch (eResArrayType)
                    {
                        case svDouble:
                            PushDouble( fResVal );
                            break;
                        case svString:
                            PushString( aResStr );
                            break;
                        case svSingleRef:
                            PushCellResultToken( true, aResAdr, NULL, NULL);
                            break;
                        default:
                            ;   // nothing
                    }
                }
            }
            break;
            default:
                DBG_ERRORFILE( "ScInterpreter::ScLookup: unhandled eResArrayType, range search");
        }
    }
    else
    {
        // Regardless of whether or not the result array exists, the last
        // array is always used as the "result" array.

        ScAddress aAdr;
        aAdr.SetTab(nTab1);
        if (bVertical)
        {
            SCROW nTempRow = static_cast<SCROW>(nRow1 + nDelta);
            if (nTempRow > MAXROW)
            {
                PushDouble(0);
                return;
            }
            aAdr.SetCol(nCol2);
            aAdr.SetRow(nTempRow);
        }
        else
        {
            SCCOL nTempCol = static_cast<SCCOL>(nCol1 + nDelta);
            if (nTempCol > MAXCOL)
            {
                PushDouble(0);
                return;
            }
            aAdr.SetCol(nTempCol);
            aAdr.SetRow(nRow2);
        }
        PushCellResultToken(true, aAdr, NULL, NULL);
    }
}


void ScInterpreter::ScHLookup()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScHLookup" );
    CalculateLookup(TRUE);
}
void ScInterpreter::CalculateLookup(BOOL HLookup)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::CalculateLookup" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 3, 4 ) )
    {
        BOOL bSorted;
        if (nParamCount == 4)
            bSorted = GetBool();
        else
            bSorted = TRUE;
        double fIndex = ::rtl::math::approxFloor( GetDouble() ) - 1.0;
        ScMatrixRef pMat = NULL;
        SCSIZE nC = 0, nR = 0;
        SCCOL nCol1 = 0;
        SCROW nRow1 = 0;
        SCTAB nTab1 = 0;
        SCCOL nCol2 = 0;
        SCROW nRow2 = 0;
        SCTAB nTab2;
        if (GetStackType() == svDoubleRef)
        {
            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
            if (nTab1 != nTab2)
            {
                PushIllegalParameter();
                return;
            }
        }
        else if (GetStackType() == svMatrix)
        {
            pMat = PopMatrix();
            if (pMat)
                pMat->GetDimensions(nC, nR);
            else
            {
                PushIllegalParameter();
                return;
            }
        }
        else
        {
            PushIllegalParameter();
            return;
        }
        if ( fIndex < 0.0 || (HLookup ? (pMat ? (fIndex >= nR) : (fIndex+nRow1 > nRow2)) : (pMat ? (fIndex >= nC) : (fIndex+nCol1 > nCol2)) ) )
        {
            PushIllegalArgument();
            return;
        }
        SCROW nZIndex = static_cast<SCROW>(fIndex);
        SCCOL nSpIndex = static_cast<SCCOL>(fIndex);

        if (!pMat)
        {
            nZIndex += nRow1;                       // Wertzeile
            nSpIndex = sal::static_int_cast<SCCOL>( nSpIndex + nCol1 );     // value column
        }

        if (nGlobalError == 0)
        {
            ScQueryParam rParam;
            rParam.nCol1       = nCol1;
            rParam.nRow1       = nRow1;
            if ( HLookup )
            {
                rParam.nCol2       = nCol2;
                rParam.nRow2       = nRow1;     // nur in der ersten Zeile suchen
                rParam.bByRow      = FALSE;
            } // if ( HLookup )
            else
            {
                rParam.nCol2       = nCol1;     // nur in der ersten Spalte suchen
                rParam.nRow2       = nRow2;
                rParam.nTab        = nTab1;
            }
            rParam.bMixedComparison = TRUE;

            ScQueryEntry& rEntry = rParam.GetEntry(0);
            rEntry.bDoQuery = TRUE;
            if ( bSorted )
                rEntry.eOp = SC_LESS_EQUAL;
            if ( !FillEntry(rEntry) )
                return;
            if ( rEntry.bQueryByString )
                rParam.bRegExp = MayBeRegExp( *rEntry.pStr, pDok );
            if (pMat)
            {
                SCSIZE nMatCount = HLookup ? nC : nR;
                SCSIZE nDelta = SCSIZE_MAX;
                if (rEntry.bQueryByString)
                {
        //!!!!!!!
        //! TODO: enable regex on matrix strings
        //!!!!!!!
                    String aParamStr = *rEntry.pStr;
                    if ( bSorted )
                    {
                        static CollatorWrapper* pCollator = ScGlobal::GetCollator();
                        for (SCSIZE i = 0; i < nMatCount; i++)
                        {
                            if (HLookup ? pMat->IsString(i, 0) : pMat->IsString(0, i))
                            {
                                sal_Int32 nRes =
                                    pCollator->compareString( HLookup ? pMat->GetString(i,0) : pMat->GetString(0,i), aParamStr);
                                if (nRes <= 0)
                                    nDelta = i;
                                else if (i>0)   // #i2168# ignore first mismatch
                                    i = nMatCount+1;
                            }
                            else
                                nDelta = i;
                        }
                    }
                    else
                    {
                        for (SCSIZE i = 0; i < nMatCount; i++)
                        {
                            if (HLookup ? pMat->IsString(i, 0) : pMat->IsString(0, i))
                            {
                                if ( ScGlobal::GetpTransliteration()->isEqual(
                                    HLookup ? pMat->GetString(i,0) : pMat->GetString(0,i), aParamStr ) )
                                {
                                    nDelta = i;
                                    i = nMatCount + 1;
                                }
                            }
                        }
                    }
                }
                else
                {
                    if ( bSorted )
                    {
                        // #i2168# ignore strings
                        for (SCSIZE i = 0; i < nMatCount; i++)
                        {
                            if (!(HLookup ? pMat->IsString(i, 0) : pMat->IsString(0, i)))
                            {
                                if ((HLookup ? pMat->GetDouble(i,0) : pMat->GetDouble(0,i)) <= rEntry.nVal)
                                    nDelta = i;
                                else
                                    i = nMatCount+1;
                            }
                        }
                    }
                    else
                    {
                        for (SCSIZE i = 0; i < nMatCount; i++)
                        {
                            if (!(HLookup ? pMat->IsString(i, 0) : pMat->IsString(0, i)))
                            {
                                if ((HLookup ? pMat->GetDouble(i,0) : pMat->GetDouble(0,i)) == rEntry.nVal)
                                {
                                    nDelta = i;
                                    i = nMatCount + 1;
                                }
                            }
                        }
                    }
                }
                if ( nDelta != SCSIZE_MAX )
                {
                    SCSIZE nX = static_cast<SCSIZE>(nSpIndex);
                    SCSIZE nY = nDelta;
                    if ( HLookup )
                    {
                        nX = nDelta;
                        nY = static_cast<SCSIZE>(nZIndex);
                    }
                    if ( pMat->IsString( nX, nY) )
                        PushString(pMat->GetString( nX,nY));
                    else
                        PushDouble(pMat->GetDouble( nX,nY));
                }
                else
                    PushNA();
            }
            else
            {
                rEntry.nField = nCol1;
                BOOL bFound = FALSE;
                SCCOL nCol = 0;
                SCROW nRow = 0;
                if ( bSorted )
                    rEntry.eOp = SC_LESS_EQUAL;
                if ( HLookup )
                {
                    ScQueryCellIterator aCellIter(pDok, nTab1, rParam, FALSE);
                    // advance Entry.nField in Iterator upon switching columns
                    aCellIter.SetAdvanceQueryParamEntryField( TRUE );
                    if ( bSorted )
                    {
                        SCROW nRow1_temp;
                        bFound = aCellIter.FindEqualOrSortedLastInRange( nCol, nRow1_temp );
                    }
                    else if ( aCellIter.GetFirst() )
                    {
                        bFound = TRUE;
                        nCol = aCellIter.GetCol();
                    }
                    nRow = nZIndex;
                } // if ( HLookup )
                else
                {
                    ScAddress aResultPos( nCol1, nRow1, nTab1);
                    bFound = LookupQueryWithCache( aResultPos, rParam);
                    nRow = aResultPos.Row();
                    nCol = nSpIndex;
                }
                if ( bFound )
                {
                    ScAddress aAdr( nCol, nRow, nTab1 );
                    PushCellResultToken( true, aAdr, NULL, NULL);
                }
                else
                    PushNA();
            }
        }
        else
            PushIllegalParameter();
    }
}

bool ScInterpreter::FillEntry(ScQueryEntry& rEntry)
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::FillEntry" );
    switch ( GetStackType() )
    {
        case svDouble:
        {
            rEntry.bQueryByString = FALSE;
            rEntry.nVal = GetDouble();
        }
        break;
        case svString:
        {
            const String sStr = GetString();
            rEntry.bQueryByString = TRUE;
            *rEntry.pStr = sStr;
        }
        break;
        case svDoubleRef :
        case svSingleRef :
        {
            ScAddress aAdr;
            if ( !PopDoubleRefOrSingleRef( aAdr ) )
            {
                PushInt(0);
                return false;
            }
            ScBaseCell* pCell = GetCell( aAdr );
            if (HasCellValueData(pCell))
            {
                rEntry.bQueryByString = FALSE;
                rEntry.nVal = GetCellValue( aAdr, pCell );
            }
            else
            {
                if ( GetCellType( pCell ) == CELLTYPE_NOTE )
                {
                    rEntry.bQueryByString = FALSE;
                    rEntry.nVal = 0.0;
                }
                else
                {
                    String sStr;
                    GetCellString(sStr, pCell);
                    rEntry.bQueryByString = TRUE;
                    *rEntry.pStr = sStr;
                }
            }
        }
        break;
        case svMatrix :
        {
            const ScMatValType nType = GetDoubleOrStringFromMatrix(rEntry.nVal, *rEntry.pStr);
            rEntry.bQueryByString = ScMatrix::IsNonValueType( nType);
        }
        break;
        default:
        {
            PushIllegalParameter();
            return false;
        }
    } // switch ( GetStackType() )
    return true;
}
void ScInterpreter::ScVLookup()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScVLookup" );
    CalculateLookup(FALSE);
}

#if defined(WIN) && defined(MSC)
#pragma optimize("",off)
#endif

void ScInterpreter::ScSubTotal()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSubTotal" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCountMin( nParamCount, 2 ) )
    {
        // We must fish the 1st parameter deep from the stack! And push it on top.
        const FormulaToken* p = pStack[ sp - nParamCount ];
        PushTempToken( *p );
        int nFunc = (int) ::rtl::math::approxFloor( GetDouble() );
        if( nFunc < 1 || nFunc > 11 )
            PushIllegalArgument();  // simulate return on stack, not SetError(...)
        else
        {
            cPar = nParamCount - 1;
            glSubTotal = TRUE;
            switch( nFunc )
            {
                case SUBTOTAL_FUNC_AVE  : ScAverage(); break;
                case SUBTOTAL_FUNC_CNT  : ScCount();   break;
                case SUBTOTAL_FUNC_CNT2 : ScCount2();  break;
                case SUBTOTAL_FUNC_MAX  : ScMax();     break;
                case SUBTOTAL_FUNC_MIN  : ScMin();     break;
                case SUBTOTAL_FUNC_PROD : ScProduct(); break;
                case SUBTOTAL_FUNC_STD  : ScStDev();   break;
                case SUBTOTAL_FUNC_STDP : ScStDevP();  break;
                case SUBTOTAL_FUNC_SUM  : ScSum();     break;
                case SUBTOTAL_FUNC_VAR  : ScVar();     break;
                case SUBTOTAL_FUNC_VARP : ScVarP();    break;
                default : PushIllegalArgument();       break;
            }
            glSubTotal = FALSE;
        }
        // Get rid of the 1st (fished) parameter.
        double nVal = GetDouble();
        Pop();
        PushDouble( nVal );
    }
}
#if defined(WIN) && defined(MSC)
#pragma optimize("",on)
#endif


BOOL ScInterpreter::GetDBParams(SCTAB& rTab, ScQueryParam& rParam,
        BOOL& rMissingField )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetDBParams" );
    BOOL bRet = FALSE;
    BOOL bAllowMissingField = FALSE;
    if ( rMissingField )
    {
        bAllowMissingField = TRUE;
        rMissingField = FALSE;
    }
    if ( GetByte() == 3 )
    {

        SCCOL nQCol1;
        SCROW nQRow1;
        SCTAB nQTab1;
        SCCOL nQCol2;
        SCROW nQRow2;
        SCTAB nQTab2;
        PopDoubleRef(nQCol1, nQRow1, nQTab1, nQCol2, nQRow2, nQTab2);

        BOOL    bByVal = TRUE;
        double  nVal = 0.0;
        String  aStr;
        ScRange aMissingRange;
        BOOL bRangeFake = FALSE;
        switch (GetStackType())
        {
            case svDouble :
                nVal = ::rtl::math::approxFloor( GetDouble() );
                if ( bAllowMissingField && nVal == 0.0 )
                    rMissingField = TRUE;   // fake missing parameter
                break;
            case svString :
                bByVal = FALSE;
                aStr = GetString();
                break;
            case svSingleRef :
                {
                    ScAddress aAdr;
                    PopSingleRef( aAdr );
                    ScBaseCell* pCell = GetCell( aAdr );
                    if (HasCellValueData(pCell))
                        nVal = GetCellValue( aAdr, pCell );
                    else
                    {
                        bByVal = FALSE;
                        GetCellString(aStr, pCell);
                    }
                }
                break;
            case svDoubleRef :
                if ( bAllowMissingField )
                {   // fake missing parameter for old SO compatibility
                    bRangeFake = TRUE;
                    PopDoubleRef( aMissingRange );
                }
                else
                {
                    PopError();
                    SetError( errIllegalParameter );
                }
                break;
            case svMissing :
                PopError();
                if ( bAllowMissingField )
                    rMissingField = TRUE;
                else
                    SetError( errIllegalParameter );
                break;
            default:
                PopError();
                SetError( errIllegalParameter );
        }

        SCCOL nDBCol1;
        SCROW nDBRow1;
        SCTAB nDBTab1;
        SCCOL nDBCol2;
        SCROW nDBRow2;
        SCTAB nDBTab2;
        PopDoubleRef(nDBCol1, nDBRow1, nDBTab1, nDBCol2, nDBRow2, nDBTab2);

        if ( nGlobalError == 0 && bRangeFake )
        {
            // range parameter must match entire database range
            if ( aMissingRange == ScRange( nDBCol1, nDBRow1, nDBTab1, nDBCol2,
                        nDBRow2, nDBTab2) )
                rMissingField = TRUE;
            else
                SetError( errIllegalParameter );
        }
        
        if (nGlobalError == 0)
        {
            SCCOL   nField = nDBCol1;
            BOOL    bFound = TRUE;
            if ( rMissingField )
                ;   // special case
            else if ( bByVal )
            {
                if ( nVal <= 0 || nVal > (nDBCol2 - nDBCol1 + 1) )
                    bFound = FALSE;
                else
                    nField = Min(nDBCol2, (SCCOL)(nDBCol1 + (SCCOL)nVal - 1));
            }
            else
            {
                bFound = FALSE;
                String aCellStr;
                ScAddress aLook( nDBCol1, nDBRow1, nDBTab1 );
                while (!bFound && (aLook.Col() <= nDBCol2))
                {
                    ScBaseCell* pCell = GetCell( aLook );
                    GetCellString( aCellStr, pCell );
                    bFound = ScGlobal::GetpTransliteration()->isEqual( aCellStr, aStr );
                    if (!bFound)
                        aLook.IncCol();
                }
                nField = aLook.Col();
            }
            if (bFound)
            {
                rParam.nCol1 = nDBCol1;
                rParam.nRow1 = nDBRow1;
                rParam.nCol2 = nDBCol2;
                rParam.nRow2 = nDBRow2;
                rParam.nTab  = nDBTab1;
                rParam.bHasHeader = TRUE;
                rParam.bByRow = TRUE;
                rParam.bInplace = TRUE;
                rParam.bCaseSens = FALSE;
                rParam.bRegExp = FALSE;
                rParam.bDuplicate = TRUE;
                if (pDok->CreateQueryParam(nQCol1, nQRow1, nQCol2, nQRow2, nQTab1, rParam))
                {
                    // An allowed missing field parameter sets the result field
                    // to any of the query fields, just to be able to return
                    // some cell from the iterator.
                    if ( rMissingField )
                        nField = static_cast<SCCOL>(rParam.GetEntry(0).nField);

                    rParam.nCol1 = nField;
                    rParam.nCol2 = nField;
                    rTab = nDBTab1;
                    bRet = TRUE;
                    SCSIZE nCount = rParam.GetEntryCount();
                    for ( SCSIZE i=0; i < nCount; i++ )
                    {
                        ScQueryEntry& rEntry = rParam.GetEntry(i);
                        if ( rEntry.bDoQuery )
                        {
                            sal_uInt32 nIndex = 0;
                            rEntry.bQueryByString = !pFormatter->IsNumberFormat(
                                *rEntry.pStr, nIndex, rEntry.nVal );
                            if ( rEntry.bQueryByString && !rParam.bRegExp )
                                rParam.bRegExp = MayBeRegExp( *rEntry.pStr, pDok );
                        }
                        else
                            break;  // for
                    }
                }
            }
        }
    }
    return bRet;
}


void ScInterpreter::DBIterator( ScIterFunc eFunc )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::DBIterator" );
    SCTAB nTab1;
    double nErg = 0.0;
    double fMem = 0.0;
    BOOL bNull = TRUE;
    ULONG nCount = 0;
    ScQueryParam aQueryParam;
    BOOL bMissingField = FALSE;
    if ( GetDBParams( nTab1, aQueryParam, bMissingField) )
    {
        double nVal;
        USHORT nErr;
        ScQueryValueIterator aValIter(pDok, nTab1, aQueryParam);
        if ( aValIter.GetFirst(nVal, nErr) && !nErr )
        {
            switch( eFunc )
            {
                case ifPRODUCT: nErg = 1; break;
                case ifMAX:     nErg = -MAXDOUBLE; break;
                case ifMIN:     nErg = MAXDOUBLE; break;
                default: ; // nothing
            }
            do
            {
                nCount++;
                switch( eFunc )
                {
                    case ifAVERAGE:
                    case ifSUM:
                        if ( bNull && nVal != 0.0 )
                        {
                            bNull = FALSE;
                            fMem = nVal;
                        }
                        else
                            nErg += nVal;
                        break;
                    case ifSUMSQ:   nErg += nVal * nVal; break;
                    case ifPRODUCT: nErg *= nVal; break;
                    case ifMAX:     if( nVal > nErg ) nErg = nVal; break;
                    case ifMIN:     if( nVal < nErg ) nErg = nVal; break;
                    default: ; // nothing
                }
            }
            while ( aValIter.GetNext(nVal, nErr) && !nErr );
        }
        SetError(nErr);
    }
    else
        SetError( errIllegalParameter);
    switch( eFunc )
    {
        case ifCOUNT:   nErg = nCount; break;
        case ifSUM:     nErg = ::rtl::math::approxAdd( nErg, fMem ); break;
        case ifAVERAGE: nErg = ::rtl::math::approxAdd( nErg, fMem ) / nCount; break;
        default: ; // nothing
    }
    PushDouble( nErg );
}


void ScInterpreter::ScDBSum()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBSum" );
    DBIterator( ifSUM );
}


void ScInterpreter::ScDBCount()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBCount" );
    SCTAB nTab;
    ScQueryParam aQueryParam;
    BOOL bMissingField = TRUE;
    if ( GetDBParams( nTab, aQueryParam, bMissingField) )
    {
        ULONG nCount = 0;
        if ( bMissingField )
        {   // count all matching records
            // TODO: currently the QueryIterators only return cell pointers of
            // existing cells, so if a query matches an empty cell there's
            // nothing returned, and therefor not counted!
            // Since this has ever been the case and this code here only came
            // into existance to fix #i6899 and it never worked before we'll
            // have to live with it until we reimplement the iterators to also
            // return empty cells, which would mean to adapt all callers of
            // iterators.
            ScQueryCellIterator aCellIter( pDok, nTab, aQueryParam);
            if ( aCellIter.GetFirst() )
            {
                do
                {
                    nCount++;
                } while ( aCellIter.GetNext() );
            }
        }
        else
        {   // count only matching records with a value in the "result" field
            double nVal;
            USHORT nErr = 0;
            ScQueryValueIterator aValIter( pDok, nTab, aQueryParam);
            if ( aValIter.GetFirst( nVal, nErr) && !nErr )
            {
                do
                {
                    nCount++;
                } while ( aValIter.GetNext( nVal, nErr) && !nErr );
            }
            SetError( nErr );
        }
        PushDouble( nCount );
    }
    else
        PushIllegalParameter();
}


void ScInterpreter::ScDBCount2()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBCount2" );
    SCTAB nTab;
    ScQueryParam aQueryParam;
    BOOL bMissingField = TRUE;
    if (GetDBParams( nTab, aQueryParam, bMissingField))
    {
        ULONG nCount = 0;
        ScQueryCellIterator aCellIter(pDok, nTab, aQueryParam);
        if ( aCellIter.GetFirst() )
        {
            do
            {
                nCount++;
            } while ( aCellIter.GetNext() );
        }
        PushDouble(nCount);
    }
    else
        PushIllegalParameter();
}


void ScInterpreter::ScDBAverage()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBAverage" );
    DBIterator( ifAVERAGE );
}


void ScInterpreter::ScDBMax()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBMax" );
    DBIterator( ifMAX );
}


void ScInterpreter::ScDBMin()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBMin" );
    DBIterator( ifMIN );
}


void ScInterpreter::ScDBProduct()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBProduct" );
    DBIterator( ifPRODUCT );
}


void ScInterpreter::GetDBStVarParams( double& rVal, double& rValCount )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::GetDBStVarParams" );
    std::vector<double> values;
    double vSum    = 0.0;
    double vMean    = 0.0;

    rValCount = 0.0;
    double fSum    = 0.0;
    SCTAB nTab;
    ScQueryParam aQueryParam;
    BOOL bMissingField = FALSE;
    if (GetDBParams( nTab, aQueryParam, bMissingField))
    {
        double fVal;
        USHORT nErr;
        ScQueryValueIterator aValIter(pDok, nTab, aQueryParam);
        if (aValIter.GetFirst(fVal, nErr) && !nErr)
        {
            do
            {
                rValCount++;
                values.push_back(fVal);
                fSum += fVal;
            }
            while ((nErr == 0) && aValIter.GetNext(fVal, nErr));
        }
        SetError(nErr);
    }
    else
        SetError( errIllegalParameter);

    vMean = fSum / values.size();
    
    for (size_t i = 0; i < values.size(); i++)
        vSum += (values[i] - vMean) * (values[i] - vMean);

    rVal = vSum;
}


void ScInterpreter::ScDBStdDev()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBStdDev" );
    double fVal, fCount;
    GetDBStVarParams( fVal, fCount );
    PushDouble( sqrt(fVal/(fCount-1)));
}


void ScInterpreter::ScDBStdDevP()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBStdDevP" );
    double fVal, fCount;
    GetDBStVarParams( fVal, fCount );
    PushDouble( sqrt(fVal/fCount));
}


void ScInterpreter::ScDBVar()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBVar" );
    double fVal, fCount;
    GetDBStVarParams( fVal, fCount );
    PushDouble(fVal/(fCount-1));
}


void ScInterpreter::ScDBVarP()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScDBVarP" );
    double fVal, fCount;
    GetDBStVarParams( fVal, fCount );
    PushDouble(fVal/fCount);
}


ScTokenArray* lcl_CreateExternalRefTokenArray( const ScAddress& rPos, ScDocument* pDoc, 
        const ScAddress::ExternalInfo& rExtInfo, const ScRefAddress& rRefAd1, 
        const ScRefAddress* pRefAd2 )
{
    ScExternalRefManager* pRefMgr = pDoc->GetExternalRefManager();
    size_t nSheets = 1;
    const String* pRealTab = pRefMgr->getRealTableName( rExtInfo.mnFileId, rExtInfo.maTabName);
    ScTokenArray* pTokenArray = new ScTokenArray;
    if (pRefAd2)
    {
        ScComplexRefData aRef;
        aRef.InitRangeRel( ScRange( rRefAd1.GetAddress(), pRefAd2->GetAddress()), rPos);
        aRef.Ref1.SetColRel( rRefAd1.IsRelCol());
        aRef.Ref1.SetRowRel( rRefAd1.IsRelRow());
        aRef.Ref1.SetTabRel( rRefAd1.IsRelTab());
        aRef.Ref1.SetFlag3D( true);
        aRef.Ref2.SetColRel( pRefAd2->IsRelCol());
        aRef.Ref2.SetRowRel( pRefAd2->IsRelRow());
        aRef.Ref2.SetTabRel( pRefAd2->IsRelTab());
        nSheets = aRef.Ref2.nTab - aRef.Ref1.nTab + 1;
        aRef.Ref2.SetFlag3D( nSheets > 1 );
        pTokenArray->AddExternalDoubleReference( rExtInfo.mnFileId,
                (pRealTab ? *pRealTab : rExtInfo.maTabName), aRef);
    }
    else
    {
        ScSingleRefData aRef;
        aRef.InitAddressRel( rRefAd1.GetAddress(), rPos);
        aRef.SetColRel( rRefAd1.IsRelCol());
        aRef.SetRowRel( rRefAd1.IsRelRow());
        aRef.SetTabRel( rRefAd1.IsRelTab());
        aRef.SetFlag3D( true);
        pTokenArray->AddExternalSingleReference( rExtInfo.mnFileId,
                (pRealTab ? *pRealTab : rExtInfo.maTabName), aRef);
    }
    // The indirect usage of the external table can't be detected during the 
    // store-to-file cycle, mark it as permanently referenced so it gets stored 
    // even if not directly referenced anywhere.
    pRefMgr->setCacheTableReferencedPermanently( rExtInfo.mnFileId, 
            rExtInfo.maTabName, nSheets);
    ScCompiler aComp( pDoc, rPos, *pTokenArray);
    aComp.CompileTokenArray();
    return pTokenArray;
}


void ScInterpreter::ScIndirect()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIndirect" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 1, 2 )  )
    {
        bool bTryXlA1 = true;   // whether to try XL_A1 style as well.
        FormulaGrammar::AddressConvention eConv = FormulaGrammar::CONV_OOO;
        if (nParamCount == 2 && 0.0 == ::rtl::math::approxFloor( GetDouble()))
        {
            eConv = FormulaGrammar::CONV_XL_R1C1;
            bTryXlA1 = false;
        }
        const ScAddress::Details aDetails( eConv, aPos );
        const ScAddress::Details aDetailsXlA1( FormulaGrammar::CONV_XL_A1, aPos );
        SCTAB nTab = aPos.Tab();
        String sRefStr( GetString() );
        ScRefAddress aRefAd, aRefAd2;
        ScAddress::ExternalInfo aExtInfo;
        if ( ConvertDoubleRef( pDok, sRefStr, nTab, aRefAd, aRefAd2, aDetails, &aExtInfo) ||
                (bTryXlA1 && ConvertDoubleRef( pDok, sRefStr, nTab, aRefAd,
                                               aRefAd2, aDetailsXlA1, &aExtInfo)))
        {
            if (aExtInfo.mbExternal)
            {
                /* TODO: future versions should implement a proper subroutine 
                 * token. This procedure here is a minimally invasive fix for 
                 * #i101645# in OOo3.1.1 */
                // Push a subroutine on the instruction code stack that 
                // resolves the external reference as the next instruction.
                aCode.Push( lcl_CreateExternalRefTokenArray( aPos, pDok, 
                            aExtInfo, aRefAd, &aRefAd2));
                // Signal subroutine call to interpreter.
                PushTempToken( new FormulaUnknownToken( ocCall));
            }
            else
                PushDoubleRef( aRefAd.Col(), aRefAd.Row(), aRefAd.Tab(),
                        aRefAd2.Col(), aRefAd2.Row(), aRefAd2.Tab() );
        }
        else if ( ConvertSingleRef ( pDok, sRefStr, nTab, aRefAd, aDetails, &aExtInfo) ||
                (bTryXlA1 && ConvertSingleRef ( pDok, sRefStr, nTab, aRefAd,
                                                aDetailsXlA1, &aExtInfo)))
        {
            if (aExtInfo.mbExternal)
            {
                /* TODO: future versions should implement a proper subroutine 
                 * token. This procedure here is a minimally invasive fix for 
                 * #i101645# in OOo3.1.1 */
                // Push a subroutine on the instruction code stack that 
                // resolves the external reference as the next instruction.
                aCode.Push( lcl_CreateExternalRefTokenArray( aPos, pDok, 
                            aExtInfo, aRefAd, NULL));
                // Signal subroutine call to interpreter.
                PushTempToken( new FormulaUnknownToken( ocCall));
            }
            else
                PushSingleRef( aRefAd.Col(), aRefAd.Row(), aRefAd.Tab() );
        }
        else
        {
            do
            {
                ScRangeName* pNames = pDok->GetRangeName();
                if (!pNames)
                    break;

                USHORT nPos = 0;
                if (!pNames->SearchName( sRefStr, nPos)) 
                    break;

                ScRangeData* rData = (*pNames)[nPos];
                if (!rData)
                    break;

                // We need this in order to obtain a good range.
                rData->ValidateTabRefs();

                ScRange aRange;
#if 0
                // This is some really odd Excel behavior and renders named
                // ranges containing relative references totally useless.
                if (!rData->IsReference(aRange, ScAddress( aPos.Tab(), 0, 0))) 
                    break;
#else
                // This is the usual way to treat named ranges containing
                // relative references.
                if (!rData->IsReference( aRange, aPos)) 
                    break;
#endif

                if (aRange.aStart == aRange.aEnd)
                    PushSingleRef( aRange.aStart.Col(), aRange.aStart.Row(),
                            aRange.aStart.Tab());
                else
                    PushDoubleRef( aRange.aStart.Col(), aRange.aStart.Row(),
                            aRange.aStart.Tab(), aRange.aEnd.Col(),
                            aRange.aEnd.Row(), aRange.aEnd.Tab());

                // success!
                return;
            }
            while (false);

            PushIllegalArgument();
        }
    }
}


void ScInterpreter::ScAddressFunc()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScAddressFunc" );
    String  sTabStr;

    BYTE    nParamCount = GetByte();
    if( !MustHaveParamCount( nParamCount, 2, 5 ) )
        return;

    if( nParamCount >= 5 )
        sTabStr = GetString();

    FormulaGrammar::AddressConvention eConv = FormulaGrammar::CONV_OOO;      // default
    if( nParamCount >= 4 && 0.0 == ::rtl::math::approxFloor( GetDoubleWithDefault( 1.0)))
        eConv = FormulaGrammar::CONV_XL_R1C1;

    USHORT  nFlags = SCA_COL_ABSOLUTE | SCA_ROW_ABSOLUTE;   // default
    if( nParamCount >= 3 )
    {
        USHORT n = (USHORT) ::rtl::math::approxFloor( GetDoubleWithDefault( 1.0));
        switch ( n )
        {
            default :
                PushNoValue();
                return;

            case 5:
            case 1 : break; // default
            case 6:
            case 2 : nFlags = SCA_ROW_ABSOLUTE; break;
            case 7:
            case 3 : nFlags = SCA_COL_ABSOLUTE; break;
            case 8:
            case 4 : nFlags = 0; break; // both relative
        }
    }
    nFlags |= SCA_VALID | SCA_VALID_ROW | SCA_VALID_COL;

    SCCOL nCol = (SCCOL) ::rtl::math::approxFloor(GetDouble());
    SCROW nRow = (SCROW) ::rtl::math::approxFloor(GetDouble());
    if( eConv == FormulaGrammar::CONV_XL_R1C1 )
    {
        // YUCK!  The XL interface actually treats rel R1C1 refs differently
        // than A1
        if( !(nFlags & SCA_COL_ABSOLUTE) )
            nCol += aPos.Col() + 1;
        if( !(nFlags & SCA_ROW_ABSOLUTE) )
            nRow += aPos.Row() + 1;
    }

    --nCol;
    --nRow;
    if(!ValidCol( nCol) || !ValidRow( nRow))
    {
        PushIllegalArgument();
        return;
    }

    String aRefStr;
    const ScAddress::Details aDetails( eConv, aPos );
    const ScAddress aAdr( nCol, nRow, 0);
    aAdr.Format( aRefStr, nFlags, pDok, aDetails );

    if( nParamCount >= 5 )
    {
        ScCompiler::CheckTabQuotes( sTabStr, eConv);
        sTabStr += static_cast<sal_Unicode>(eConv == FormulaGrammar::CONV_XL_R1C1 ? '!' : '.');
        sTabStr += aRefStr;
        PushString( sTabStr );
    }
    else
        PushString( aRefStr );
}


void ScInterpreter::ScOffset()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScOffset" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 3, 5 ) )
    {
        long nColNew = -1, nRowNew = -1, nColPlus, nRowPlus;
        if (nParamCount == 5)
            nColNew = (long) ::rtl::math::approxFloor(GetDouble());
        if (nParamCount >= 4)
            nRowNew = (long) ::rtl::math::approxFloor(GetDoubleWithDefault( -1.0 ));
        nColPlus = (long) ::rtl::math::approxFloor(GetDouble());
        nRowPlus = (long) ::rtl::math::approxFloor(GetDouble());
        SCCOL nCol1;
        SCROW nRow1;
        SCTAB nTab1;
        SCCOL nCol2;
        SCROW nRow2;
        SCTAB nTab2;
        if (nColNew == 0 || nRowNew == 0)
        {
            PushIllegalArgument();
            return;
        }
        if (GetStackType() == svSingleRef)
        {
            PopSingleRef(nCol1, nRow1, nTab1);
            if (nParamCount == 3 || (nColNew < 0 && nRowNew < 0))
            {
                nCol1 = (SCCOL)((long) nCol1 + nColPlus);
                nRow1 = (SCROW)((long) nRow1 + nRowPlus);
                if (!ValidCol(nCol1) || !ValidRow(nRow1))
                    PushIllegalArgument();
                else
                    PushSingleRef(nCol1, nRow1, nTab1);
            }
            else
            {
                if (nColNew < 0)
                    nColNew = 1;
                if (nRowNew < 0)
                    nRowNew = 1;
                nCol1 = (SCCOL)((long)nCol1+nColPlus);      // ! nCol1 wird veraendert!
                nRow1 = (SCROW)((long)nRow1+nRowPlus);
                nCol2 = (SCCOL)((long)nCol1+nColNew-1);
                nRow2 = (SCROW)((long)nRow1+nRowNew-1);
                if (!ValidCol(nCol1) || !ValidRow(nRow1) ||
                    !ValidCol(nCol2) || !ValidRow(nRow2))
                    PushIllegalArgument();
                else
                    PushDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab1);
            }
        }
        else if (GetStackType() == svDoubleRef)
        {
            PopDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
            if (nColNew < 0)
                nColNew = nCol2 - nCol1 + 1;
            if (nRowNew < 0)
                nRowNew = nRow2 - nRow1 + 1;
            nCol1 = (SCCOL)((long)nCol1+nColPlus);
            nRow1 = (SCROW)((long)nRow1+nRowPlus);
            nCol2 = (SCCOL)((long)nCol1+nColNew-1);
            nRow2 = (SCROW)((long)nRow1+nRowNew-1);
            if (!ValidCol(nCol1) || !ValidRow(nRow1) ||
                !ValidCol(nCol2) || !ValidRow(nRow2) || nTab1 != nTab2)
                PushIllegalArgument();
            else
                PushDoubleRef(nCol1, nRow1, nTab1, nCol2, nRow2, nTab1);
        }
        else
            PushIllegalParameter();
    }
}


void ScInterpreter::ScIndex()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScIndex" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 1, 4 ) )
    {
        long nArea;
        size_t nAreaCount;
        SCCOL nCol;
        SCROW nRow;
        if (nParamCount == 4)
            nArea = (long) ::rtl::math::approxFloor(GetDouble());
        else
            nArea = 1;
        if (nParamCount >= 3)
            nCol = (SCCOL) ::rtl::math::approxFloor(GetDouble());
        else
            nCol = 0;
        if (nParamCount >= 2)
            nRow = (SCROW) ::rtl::math::approxFloor(GetDouble());
        else
            nRow = 0;
        if (GetStackType() == svRefList)
            nAreaCount = (sp ? static_cast<ScToken*>(pStack[sp-1])->GetRefList()->size() : 0);
        else
            nAreaCount = 1;     // one reference or array or whatever
        if (nAreaCount == 0 || (size_t)nArea > nAreaCount)
        {
            PushError( errNoRef);
            return;
        }
        else if (nArea < 1 || nCol < 0 || nRow < 0)
        {
            PushIllegalArgument();
            return;
        }
        switch (GetStackType())
        {
            case svMatrix:
                {
                    if (nArea != 1)
                        SetError(errIllegalArgument);
                    USHORT nOldSp = sp;
                    ScMatrixRef pMat = GetMatrix();
                    if (pMat)
                    {
                        SCSIZE nC, nR;
                        pMat->GetDimensions(nC, nR);
                        // Access one element of a vector independent of col/row
                        // orientation?
                        bool bVector = ((nCol == 0 || nRow == 0) && (nC == 1 || nR == 1));
                        SCSIZE nElement = ::std::max( static_cast<SCSIZE>(nCol),
                                static_cast<SCSIZE>(nRow));
                        if (nC == 0 || nR == 0 ||
                                (!bVector && (static_cast<SCSIZE>(nCol) > nC ||
                                              static_cast<SCSIZE>(nRow) > nR)) ||
                                (bVector && nElement > nC * nR))
                            PushIllegalArgument();
                        else if (nCol == 0 && nRow == 0)
                            sp = nOldSp;
                        else if (bVector)
                        {
                            --nElement;
                            if (pMat->IsString( nElement))
                                PushString( pMat->GetString( nElement));
                            else
                                PushDouble( pMat->GetDouble( nElement));
                        }
                        else if (nCol == 0)
                        {
                            ScMatrixRef pResMat = GetNewMat(nC, 1);
                            if (pResMat)
                            {
                                SCSIZE nRowMinus1 = static_cast<SCSIZE>(nRow - 1);
                                for (SCSIZE i = 0; i < nC; i++)
                                    if (!pMat->IsString(i, nRowMinus1))
                                        pResMat->PutDouble(pMat->GetDouble(i,
                                                    nRowMinus1), i, 0);
                                    else
                                        pResMat->PutString(pMat->GetString(i,
                                                    nRowMinus1), i, 0);
                                PushMatrix(pResMat);
                            }
                            else
                                PushIllegalArgument();
                        }
                        else if (nRow == 0)
                        {
                            ScMatrixRef pResMat = GetNewMat(1, nR);
                            if (pResMat)
                            {
                                SCSIZE nColMinus1 = static_cast<SCSIZE>(nCol - 1);
                                for (SCSIZE i = 0; i < nR; i++)
                                    if (!pMat->IsString(nColMinus1, i))
                                        pResMat->PutDouble(pMat->GetDouble(nColMinus1,
                                                    i), i);
                                    else
                                        pResMat->PutString(pMat->GetString(nColMinus1,
                                                    i), i);
                                PushMatrix(pResMat);
                            }
                            else
                                PushIllegalArgument();
                        }
                        else
                        {
                            if (!pMat->IsString( static_cast<SCSIZE>(nCol-1),
                                        static_cast<SCSIZE>(nRow-1)))
                                PushDouble( pMat->GetDouble(
                                            static_cast<SCSIZE>(nCol-1),
                                            static_cast<SCSIZE>(nRow-1)));
                            else
                                PushString( pMat->GetString(
                                            static_cast<SCSIZE>(nCol-1),
                                            static_cast<SCSIZE>(nRow-1)));
                        }
                    }
                }
                break;
            case svSingleRef:
                {
                    SCCOL nCol1 = 0;
                    SCROW nRow1 = 0;
                    SCTAB nTab1 = 0;
                    PopSingleRef( nCol1, nRow1, nTab1);
                    if (nCol > 1 || nRow > 1)
                        PushIllegalArgument();
                    else
                        PushSingleRef( nCol1, nRow1, nTab1);
                }
                break;
            case svDoubleRef:
            case svRefList:
                {
                    SCCOL nCol1 = 0;
                    SCROW nRow1 = 0;
                    SCTAB nTab1 = 0;
                    SCCOL nCol2 = 0;
                    SCROW nRow2 = 0;
                    SCTAB nTab2 = 0;
                    BOOL bRowArray = FALSE;
                    if (GetStackType() == svRefList)
                    {
                        FormulaTokenRef xRef = PopToken();
                        if (nGlobalError || !xRef)
                        {
                            PushIllegalParameter();
                            return;
                        }
                        ScRange aRange( ScAddress::UNINITIALIZED);
                        DoubleRefToRange( (*(static_cast<ScToken*>(xRef.get())->GetRefList()))[nArea-1], aRange);
                        aRange.GetVars( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                        if ( nParamCount == 2 && nRow1 == nRow2 )
                            bRowArray = TRUE;
                    }
                    else
                    {
                        PopDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2);
                        if ( nParamCount == 2 && nRow1 == nRow2 )
                            bRowArray = TRUE;
                    }
                    if ( nTab1 != nTab2 ||
                            (nCol > 0 && nCol1+nCol-1 > nCol2) ||
                            (nRow > 0 && nRow1+nRow-1 > nRow2 && !bRowArray ) ||
                            ( nRow > nCol2 - nCol1 + 1 && bRowArray ))
                        PushIllegalArgument();
                    else if (nCol == 0 && nRow == 0)
                    {
                        if ( nCol1 == nCol2 && nRow1 == nRow2 )
                            PushSingleRef( nCol1, nRow1, nTab1 );
                        else
                            PushDoubleRef( nCol1, nRow1, nTab1, nCol2, nRow2, nTab1 );
                    }
                    else if (nRow == 0)
                    {
                        if ( nRow1 == nRow2 )
                            PushSingleRef( nCol1+nCol-1, nRow1, nTab1 );
                        else
                            PushDoubleRef( nCol1+nCol-1, nRow1, nTab1,
                                    nCol1+nCol-1, nRow2, nTab1 );
                    }
                    else if (nCol == 0)
                    {
                        if ( nCol1 == nCol2 )
                            PushSingleRef( nCol1, nRow1+nRow-1, nTab1 );
                        else if ( bRowArray )
                        {
                            nCol =(SCCOL) nRow;
                            nRow = 1;
                            PushSingleRef( nCol1+nCol-1, nRow1+nRow-1, nTab1);
                        }
                        else
                            PushDoubleRef( nCol1, nRow1+nRow-1, nTab1,
                                    nCol2, nRow1+nRow-1, nTab1);
                    }
                    else
                        PushSingleRef( nCol1+nCol-1, nRow1+nRow-1, nTab1);
                }
                break;
            default:
                PushIllegalParameter();
        }
    }
}


void ScInterpreter::ScMultiArea()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScMultiArea" );
    // Legacy support, convert to RefList
    BYTE nParamCount = GetByte();
    if (MustHaveParamCountMin( nParamCount, 1))
    {
        while (!nGlobalError && nParamCount-- > 1)
        {
            ScUnionFunc();
        }
    }
}


void ScInterpreter::ScAreas()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScAreas" );
    BYTE nParamCount = GetByte();
    if (MustHaveParamCount( nParamCount, 1))
    {
        size_t nCount = 0;
        switch (GetStackType())
        {
            case svSingleRef:
                {
                    FormulaTokenRef xT = PopToken();
                    ValidateRef( static_cast<ScToken*>(xT.get())->GetSingleRef());
                    ++nCount;
                }
                break;
            case svDoubleRef:
                {
                    FormulaTokenRef xT = PopToken();
                    ValidateRef( static_cast<ScToken*>(xT.get())->GetDoubleRef());
                    ++nCount;
                }
                break;
            case svRefList:
                {
                    FormulaTokenRef xT = PopToken();
                    ValidateRef( *(static_cast<ScToken*>(xT.get())->GetRefList()));
                    nCount += static_cast<ScToken*>(xT.get())->GetRefList()->size();
                }
                break;
            default:
                SetError( errIllegalParameter);
        }
        PushDouble( double(nCount));
    }
}


void ScInterpreter::ScCurrency()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScCurrency" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 1, 2 ) )
    {
        String aStr;
        double fDec;
        if (nParamCount == 2)
        {
            fDec = ::rtl::math::approxFloor(GetDouble());
            if (fDec < -15.0 || fDec > 15.0)
            {
                PushIllegalArgument();
                return;
            }
        }
        else
            fDec = 2.0;
        double fVal = GetDouble();
        double fFac;
        if ( fDec != 0.0 )
            fFac = pow( (double)10, fDec );
        else
            fFac = 1.0;
        if (fVal < 0.0)
            fVal = ceil(fVal*fFac-0.5)/fFac;
        else
            fVal = floor(fVal*fFac+0.5)/fFac;
        Color* pColor = NULL;
        if ( fDec < 0.0 )
            fDec = 0.0;
        ULONG nIndex = pFormatter->GetStandardFormat(
                                        NUMBERFORMAT_CURRENCY,
                                        ScGlobal::eLnge);
        if ( (USHORT) fDec != pFormatter->GetFormatPrecision( nIndex ) )
        {
            String sFormatString;
            pFormatter->GenerateFormat(sFormatString,
                                                   nIndex,
                                                   ScGlobal::eLnge,
                                                   TRUE,        // mit Tausenderpunkt
                                                   FALSE,       // nicht rot
                                                  (USHORT) fDec,// Nachkommastellen
                                                   1);          // 1 Vorkommanull
            if (!pFormatter->GetPreviewString(sFormatString,
                                                  fVal,
                                                  aStr,
                                                  &pColor,
                                                  ScGlobal::eLnge))
                SetError(errIllegalArgument);
        }
        else
        {
            pFormatter->GetOutputString(fVal, nIndex, aStr, &pColor);
        }
        PushString(aStr);
    }
}


void ScInterpreter::ScReplace()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScReplace" );
    if ( MustHaveParamCount( GetByte(), 4 ) )
    {
        String aNewStr( GetString() );
        double fCount = ::rtl::math::approxFloor( GetDouble());
        double fPos   = ::rtl::math::approxFloor( GetDouble());
        String aOldStr( GetString() );
        if (fPos < 1.0 || fPos > static_cast<double>(STRING_MAXLEN)
                || fCount < 0.0 || fCount > static_cast<double>(STRING_MAXLEN))
            PushIllegalArgument();
        else
        {
            xub_StrLen nCount = static_cast<xub_StrLen>(fCount);
            xub_StrLen nPos   = static_cast<xub_StrLen>(fPos);
            xub_StrLen nLen   = aOldStr.Len();
            if (nPos > nLen + 1)
                nPos = nLen + 1;
            if (nCount > nLen - nPos + 1)
                nCount = nLen - nPos + 1;
            aOldStr.Erase( nPos-1, nCount );
            if ( CheckStringResultLen( aOldStr, aNewStr ) )
                aOldStr.Insert( aNewStr, nPos-1 );
            PushString( aOldStr );
        }
    }
}


void ScInterpreter::ScFixed()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScFixed" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 1, 3 ) )
    {
        String aStr;
        double fDec;
        BOOL bThousand;
        if (nParamCount == 3)
            bThousand = !GetBool();     // Param TRUE: keine Tausenderpunkte
        else
            bThousand = TRUE;
        if (nParamCount >= 2)
        {
            fDec = ::rtl::math::approxFloor(GetDoubleWithDefault( 2.0 ));
            if (fDec < -15.0 || fDec > 15.0)
            {
                PushIllegalArgument();
                return;
            }
        }
        else
            fDec = 2.0;
        double fVal = GetDouble();
        double fFac;
        if ( fDec != 0.0 )
            fFac = pow( (double)10, fDec );
        else
            fFac = 1.0;
        if (fVal < 0.0)
            fVal = ceil(fVal*fFac-0.5)/fFac;
        else
            fVal = floor(fVal*fFac+0.5)/fFac;
        Color* pColor = NULL;
        String sFormatString;
        if (fDec < 0.0)
            fDec = 0.0;
        ULONG nIndex = pFormatter->GetStandardFormat(
                                            NUMBERFORMAT_NUMBER,
                                            ScGlobal::eLnge);
        pFormatter->GenerateFormat(sFormatString,
                                               nIndex,
                                               ScGlobal::eLnge,
                                               bThousand,   // mit Tausenderpunkt
                                               FALSE,       // nicht rot
                                               (USHORT) fDec,// Nachkommastellen
                                               1);          // 1 Vorkommanull
        if (!pFormatter->GetPreviewString(sFormatString,
                                                  fVal,
                                                  aStr,
                                                  &pColor,
                                                  ScGlobal::eLnge))
            PushIllegalArgument();
        else
            PushString(aStr);
    }
}


void ScInterpreter::ScFind()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScFind" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 2, 3 ) )
    {
        double fAnz;
        if (nParamCount == 3)
            fAnz = GetDouble();
        else
            fAnz = 1.0;
        String sStr = GetString();
        if( fAnz < 1.0 || fAnz > (double) sStr.Len() )
            PushNoValue();
        else
        {
            xub_StrLen nPos = sStr.Search( GetString(), (xub_StrLen) fAnz - 1 );
            if (nPos == STRING_NOTFOUND)
                PushNoValue();
            else
                PushDouble((double)(nPos + 1));
        }
    }
}


void ScInterpreter::ScExact()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScExact" );
    nFuncFmtType = NUMBERFORMAT_LOGICAL;
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        String s1( GetString() );
        String s2( GetString() );
        PushInt( s1 == s2 );
    }
}


void ScInterpreter::ScLeft()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScLeft" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 1, 2 ) )
    {
        xub_StrLen n;
        if (nParamCount == 2)
        {
            double nVal = ::rtl::math::approxFloor(GetDouble());
            if ( nVal < 0.0 || nVal > STRING_MAXLEN )
            {
                PushIllegalArgument();
                return ;
            }
            else
                n = (xub_StrLen) nVal;
        }
        else
            n = 1;
        String aStr( GetString() );
        aStr.Erase( n );
        PushString( aStr );
    }
}


void ScInterpreter::ScRight()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScRight" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 1, 2 ) )
    {
        xub_StrLen n;
        if (nParamCount == 2)
        {
            double nVal = ::rtl::math::approxFloor(GetDouble());
            if ( nVal < 0.0 || nVal > STRING_MAXLEN )
            {
                PushIllegalArgument();
                return ;
            }
            else
                n = (xub_StrLen) nVal;
        }
        else
            n = 1;
        String aStr( GetString() );
        if( n < aStr.Len() )
            aStr.Erase( 0, aStr.Len() - n );
        PushString( aStr );
    }
}


void ScInterpreter::ScSearch()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSearch" );
    double fAnz;
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 2, 3 ) )
    {
        if (nParamCount == 3)
        {
            fAnz = ::rtl::math::approxFloor(GetDouble());
            if (fAnz > double(STRING_MAXLEN))
            {
                PushIllegalArgument();
                return;
            }
        }
        else
            fAnz = 1.0;
        String sStr = GetString();
        String SearchStr = GetString();
        xub_StrLen nPos = (xub_StrLen) fAnz - 1;
        xub_StrLen nEndPos = sStr.Len();
        if( nPos >= nEndPos )
            PushNoValue();
        else
        {
            utl::SearchParam::SearchType eSearchType =
                (MayBeRegExp( SearchStr, pDok ) ?
                utl::SearchParam::SRCH_REGEXP : utl::SearchParam::SRCH_NORMAL);
            utl::SearchParam sPar(SearchStr, eSearchType, FALSE, FALSE, FALSE);
            utl::TextSearch sT( sPar, *ScGlobal::pCharClass );
            int nBool = sT.SearchFrwrd(sStr, &nPos, &nEndPos);
            if (!nBool)
                PushNoValue();
            else
                PushDouble((double)(nPos) + 1);
        }
    }
}


void ScInterpreter::ScMid()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScMid" );
    if ( MustHaveParamCount( GetByte(), 3 ) )
    {
        double fAnz    = ::rtl::math::approxFloor(GetDouble());
        double fAnfang = ::rtl::math::approxFloor(GetDouble());
        const String& rStr = GetString();
        if (fAnfang < 1.0 || fAnz < 0.0 || fAnfang > double(STRING_MAXLEN) || fAnz > double(STRING_MAXLEN))
            PushIllegalArgument();
        else
            PushString(rStr.Copy( (xub_StrLen) fAnfang - 1, (xub_StrLen) fAnz ));
    }
}


void ScInterpreter::ScText()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScText" );
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        String sFormatString = GetString();
        double fVal = GetDouble();
        String aStr;
        Color* pColor = NULL;
        LanguageType eCellLang;
        const ScPatternAttr* pPattern = pDok->GetPattern(
            aPos.Col(), aPos.Row(), aPos.Tab() );
        if ( pPattern )
            eCellLang = ((const SvxLanguageItem&)
                pPattern->GetItem( ATTR_LANGUAGE_FORMAT )).GetValue();
        else
            eCellLang = ScGlobal::eLnge;
        if ( !pFormatter->GetPreviewStringGuess( sFormatString, fVal, aStr,
                &pColor, eCellLang ) )
            PushIllegalArgument();
        else
            PushString(aStr);
    }
}


void ScInterpreter::ScSubstitute()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScSubstitute" );
    BYTE nParamCount = GetByte();
    if ( MustHaveParamCount( nParamCount, 3, 4 ) )
    {
        xub_StrLen nAnz;
        if (nParamCount == 4)
        {
            double fAnz = ::rtl::math::approxFloor(GetDouble());
            if( fAnz < 1 || fAnz > STRING_MAXLEN )
            {
                PushIllegalArgument();
                return;
            }
            else
                nAnz = (xub_StrLen) fAnz;
        }
        else
            nAnz = 0;
        String sNewStr = GetString();
        String sOldStr = GetString();
        String sStr    = GetString();
        xub_StrLen nPos = 0;
        xub_StrLen nCount = 0;
        xub_StrLen nNewLen = sNewStr.Len();
        xub_StrLen nOldLen = sOldStr.Len();
        while( TRUE )
        {
            nPos = sStr.Search( sOldStr, nPos );
            if (nPos != STRING_NOTFOUND)
            {
                nCount++;
                if( !nAnz || nCount == nAnz )
                {
                    sStr.Erase(nPos,nOldLen);
                    if ( CheckStringResultLen( sStr, sNewStr ) )
                    {
                        sStr.Insert(sNewStr,nPos);
                        nPos = sal::static_int_cast<xub_StrLen>( nPos + nNewLen );
                    }
                    else
                        break;
                }
                else
                    nPos++;
            }
            else
                break;
        }
        PushString( sStr );
    }
}


void ScInterpreter::ScRept()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScRept" );
    if ( MustHaveParamCount( GetByte(), 2 ) )
    {
        double fAnz = ::rtl::math::approxFloor(GetDouble());
        String aStr( GetString() );
        if ( fAnz < 0.0 )
            PushIllegalArgument();
        else if ( fAnz * aStr.Len() > STRING_MAXLEN )
        {
            PushError( errStringOverflow );
        }
        else if ( fAnz == 0.0 )
            PushString( EMPTY_STRING );
        else
        {
            xub_StrLen n = (xub_StrLen) fAnz;
            const xub_StrLen nLen = aStr.Len();
            String aRes;
            const sal_Unicode* const pSrc = aStr.GetBuffer();
            sal_Unicode* pDst = aRes.AllocBuffer( n * nLen );
            while( n-- )
            {
                memcpy( pDst, pSrc, nLen * sizeof(sal_Unicode) );
                pDst += nLen;
            }
            PushString( aRes );
        }
    }
}


void ScInterpreter::ScConcat()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScConcat" );
    BYTE nParamCount = GetByte();
    String aRes;
    while( nParamCount-- > 0)
    {
        const String& rStr = GetString();
        aRes.Insert( rStr, 0 );
    }
    PushString( aRes );
}


void ScInterpreter::ScErrorType()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::ScErrorType" );
    USHORT nErr;
    USHORT nOldError = nGlobalError;
    nGlobalError = 0;
    switch ( GetStackType() )
    {
        case svRefList :
        {
            FormulaTokenRef x = PopToken();
            if (nGlobalError)
                nErr = nGlobalError;
            else
            {
                const ScRefList* pRefList = static_cast<ScToken*>(x.get())->GetRefList();
                size_t n = pRefList->size();
                if (!n)
                    nErr = errNoRef;
                else if (n > 1)
                    nErr = errNoValue;
                else
                {
                    ScRange aRange;
                    DoubleRefToRange( (*pRefList)[0], aRange);
                    if (nGlobalError)
                        nErr = nGlobalError;
                    else
                    {
                        ScAddress aAdr;
                        if ( DoubleRefToPosSingleRef( aRange, aAdr ) )
                            nErr = pDok->GetErrCode( aAdr );
                        else
                            nErr = nGlobalError;
                    }
                }
            }
        }
        break;
        case svDoubleRef :
        {
            ScRange aRange;
            PopDoubleRef( aRange );
            if ( nGlobalError )
                nErr = nGlobalError;
            else
            {
                ScAddress aAdr;
                if ( DoubleRefToPosSingleRef( aRange, aAdr ) )
                    nErr = pDok->GetErrCode( aAdr );
                else
                    nErr = nGlobalError;
            }
        }
        break;
        case svSingleRef :
        {
            ScAddress aAdr;
            PopSingleRef( aAdr );
            if ( nGlobalError )
                nErr = nGlobalError;
            else
                nErr = pDok->GetErrCode( aAdr );
        }
        break;
        default:
            PopError();
            nErr = nGlobalError;
    }
    if ( nErr )
    {
        nGlobalError = 0;
        PushDouble( nErr );
    }
    else
    {
        nGlobalError = nOldError;
        PushNA();
    }
}


BOOL ScInterpreter::MayBeRegExp( const String& rStr, const ScDocument* pDoc  )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::MayBeRegExp" );
    if ( pDoc && !pDoc->GetDocOptions().IsFormulaRegexEnabled() )
        return FALSE;
    if ( !rStr.Len() || (rStr.Len() == 1 && rStr.GetChar(0) != '.') )
        return FALSE;   // einzelnes Metazeichen kann keine RegExp sein
    static const sal_Unicode cre[] = { '.','*','+','?','[',']','^','$','\\','<','>','(',')','|', 0 };
    const sal_Unicode* p1 = rStr.GetBuffer();
    sal_Unicode c1;
    while ( ( c1 = *p1++ ) != 0 )
    {
        const sal_Unicode* p2 = cre;
        while ( *p2 )
        {
            if ( c1 == *p2++ )
                return TRUE;
        }
    }
    return FALSE;
}

static bool lcl_LookupQuery( ScAddress & o_rResultPos, ScDocument * pDoc,
        const ScQueryParam & rParam, const ScQueryEntry & rEntry )
{
    bool bFound = false;
    ScQueryCellIterator aCellIter( pDoc, rParam.nTab, rParam, FALSE);
    if (rEntry.eOp != SC_EQUAL)
    {
        // range lookup <= or >=
        SCCOL nCol;
        SCROW nRow;
        bFound = aCellIter.FindEqualOrSortedLastInRange( nCol, nRow);
        if (bFound)
        {
            o_rResultPos.SetCol( nCol);
            o_rResultPos.SetRow( nRow);
        }
    }
    else if (aCellIter.GetFirst())
    {
        // EQUAL
        bFound = true;
        o_rResultPos.SetCol( aCellIter.GetCol());
        o_rResultPos.SetRow( aCellIter.GetRow());
    }
    return bFound;
}

#define erDEBUG_LOOKUPCACHE 0
#if erDEBUG_LOOKUPCACHE
#include <cstdio>
using ::std::fprintf;
using ::std::fflush;
static struct LookupCacheDebugCounter
{
    unsigned long nMiss;
    unsigned long nHit;
    LookupCacheDebugCounter() : nMiss(0), nHit(0) {}
    ~LookupCacheDebugCounter()
    {
        fprintf( stderr, "\nmiss: %lu, hit: %lu, total: %lu, hit/miss: %lu, hit/total %lu%\n",
                nMiss, nHit, nHit+nMiss, (nMiss>0 ? nHit/nMiss : 0),
                ((nHit+nMiss)>0 ? (100*nHit)/(nHit+nMiss) : 0));
        fflush( stderr);
    }
} aLookupCacheDebugCounter;
#endif

bool ScInterpreter::LookupQueryWithCache( ScAddress & o_rResultPos,
        const ScQueryParam & rParam ) const
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLogger, "sc", "er", "ScInterpreter::LookupQueryWithCache" );
    bool bFound = false;
    const ScQueryEntry& rEntry = rParam.GetEntry(0);
    bool bColumnsMatch = (rParam.nCol1 == rEntry.nField);
    DBG_ASSERT( bColumnsMatch, "ScInterpreter::LookupQueryWithCache: columns don't match");
    if (!bColumnsMatch)
        bFound = lcl_LookupQuery( o_rResultPos, pDok, rParam, rEntry);
    else
    {
        ScRange aLookupRange( rParam.nCol1, rParam.nRow1, rParam.nTab,
                rParam.nCol2, rParam.nRow2, rParam.nTab);
        ScLookupCache& rCache = pDok->GetLookupCache( aLookupRange);
        ScLookupCache::QueryCriteria aCriteria( rEntry);
        ScLookupCache::Result eCacheResult = rCache.lookup( o_rResultPos,
                aCriteria, aPos);
        switch (eCacheResult)
        {
            case ScLookupCache::NOT_CACHED :
            case ScLookupCache::CRITERIA_DIFFERENT :
#if erDEBUG_LOOKUPCACHE
                ++aLookupCacheDebugCounter.nMiss;
#if erDEBUG_LOOKUPCACHE > 1
                fprintf( stderr, "miss %d,%d,%d\n", (int)aPos.Col(), (int)aPos.Row(), (int)aPos.Tab());
#endif
#endif
                bFound = lcl_LookupQuery( o_rResultPos, pDok, rParam, rEntry);
                if (eCacheResult == ScLookupCache::NOT_CACHED)
                    rCache.insert( o_rResultPos, aCriteria, aPos, bFound);
                break;
            case ScLookupCache::FOUND :
#if erDEBUG_LOOKUPCACHE
                ++aLookupCacheDebugCounter.nHit;
#if erDEBUG_LOOKUPCACHE > 1
                fprintf( stderr, "hit  %d,%d,%d\n", (int)aPos.Col(), (int)aPos.Row(), (int)aPos.Tab());
#endif
#endif
                bFound = true;
                break;
            case ScLookupCache::NOT_AVAILABLE :
                ;   // nothing, bFound remains FALSE
                break;
        }
    }
    return bFound;
}
