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


#include "reftokenhelper.hxx"
#include "document.hxx"
#include "rangeutl.hxx"
#include "compiler.hxx"
#include "tokenarray.hxx"

#include "rtl/ustring.hxx"
#include "formula/grammar.hxx"
#include "formula/token.hxx"

using namespace formula;

using ::std::vector;
using ::std::auto_ptr;
using ::rtl::OUString;

void ScRefTokenHelper::compileRangeRepresentation(
    vector<ScSharedTokenRef>& rRefTokens, const OUString& rRangeStr, ScDocument* pDoc, FormulaGrammar::Grammar eGrammar)
{
    const sal_Unicode cSep = GetScCompilerNativeSymbol(ocSep).GetChar(0);
    const sal_Unicode cQuote = '\'';

    // #i107275# ignore parentheses
    OUString aRangeStr = rRangeStr;
    while( (aRangeStr.getLength() >= 2) && (aRangeStr[ 0 ] == '(') && (aRangeStr[ aRangeStr.getLength() - 1 ] == ')') )
        aRangeStr = aRangeStr.copy( 1, aRangeStr.getLength() - 2 );

    bool bFailure = false;
    sal_Int32 nOffset = 0;
    while (nOffset >= 0 && !bFailure)
    {
        OUString aToken;
        ScRangeStringConverter::GetTokenByOffset(aToken, aRangeStr, nOffset, cSep, cQuote);
        if (nOffset < 0)
            break;

        ScCompiler aCompiler(pDoc, ScAddress(0,0,0));
        aCompiler.SetGrammar(eGrammar);
        auto_ptr<ScTokenArray> pArray(aCompiler.CompileString(aToken));

        // There MUST be exactly one reference per range token and nothing
        // else, and it MUST be a valid reference, not some #REF!
        USHORT nLen = pArray->GetLen();
        if (!nLen)
            continue;   // Should a missing range really be allowed?
        if (nLen != 1)
            bFailure = true;
        else
        {
            pArray->Reset();
            const FormulaToken* p = pArray->GetNextReference();
            if (!p)
                bFailure = true;
            else
            {
                const ScToken* pT = static_cast<const ScToken*>(p);
                switch (pT->GetType())
                {
                    case svSingleRef:
                        if (!pT->GetSingleRef().Valid())
                            bFailure = true;
                        break;
                    case svDoubleRef:
                        if (!pT->GetDoubleRef().Valid())
                            bFailure = true;
                        break;
                    case svExternalSingleRef:
                        if (!pT->GetSingleRef().ValidExternal())
                            bFailure = true;
                        break;
                    case svExternalDoubleRef:
                        if (!pT->GetDoubleRef().ValidExternal())
                            bFailure = true;
                        break;
                    default:
                        ;
                }
                if (!bFailure)
                    rRefTokens.push_back(
                            ScSharedTokenRef(static_cast<ScToken*>(p->Clone())));
            }
        }

#if 0
        switch (p->GetType())
        {
            case svSingleRef:
                fprintf(stdout, "ScChart2DataProvider::compileRangeRepresentation:   single ref\n");
            break;
            case svDoubleRef:
                fprintf(stdout, "ScChart2DataProvider::compileRangeRepresentation:   double ref\n");
            break;
            case svExternalSingleRef:
                fprintf(stdout, "ScChart2DataProvider::compileRangeRepresentation:   external single ref\n");
            break;
            case svExternalDoubleRef:
                fprintf(stdout, "ScChart2DataProvider::compileRangeRepresentation:   external double ref\n");
            break;
            default:
                ;
        }
#endif

    }
    if (bFailure)
        rRefTokens.clear();
}

bool ScRefTokenHelper::getRangeFromToken(ScRange& rRange, const ScSharedTokenRef& pToken, bool bExternal)
{
    StackVar eType = pToken->GetType();
    switch (pToken->GetType())
    {
        case svSingleRef:
        case svExternalSingleRef:
        {
            if ((eType == svExternalSingleRef && !bExternal) ||
                (eType == svSingleRef && bExternal))
                return false;

            const ScSingleRefData& rRefData = pToken->GetSingleRef();
            rRange.aStart.SetCol(rRefData.nCol);
            rRange.aStart.SetRow(rRefData.nRow);
            rRange.aStart.SetTab(rRefData.nTab);
            rRange.aEnd = rRange.aStart;
            return true;
        }
        case svDoubleRef:
        case svExternalDoubleRef:
        {
            if ((eType == svExternalDoubleRef && !bExternal) ||
                (eType == svDoubleRef && bExternal))
                return false;

            const ScComplexRefData& rRefData = pToken->GetDoubleRef();
            rRange.aStart.SetCol(rRefData.Ref1.nCol);
            rRange.aStart.SetRow(rRefData.Ref1.nRow);
            rRange.aStart.SetTab(rRefData.Ref1.nTab);
            rRange.aEnd.SetCol(rRefData.Ref2.nCol);
            rRange.aEnd.SetRow(rRefData.Ref2.nRow);
            rRange.aEnd.SetTab(rRefData.Ref2.nTab);
            return true;
        }
        default:
            ; // do nothing
    }
    return false;
}

void ScRefTokenHelper::getRangeListFromTokens(ScRangeList& rRangeList, const vector<ScSharedTokenRef>& rTokens)
{
    vector<ScSharedTokenRef>::const_iterator itr = rTokens.begin(), itrEnd = rTokens.end();
    for (; itr != itrEnd; ++itr)
    {
        ScRange aRange;
        getRangeFromToken(aRange, *itr);
        rRangeList.Append(aRange);
    }
}

void ScRefTokenHelper::getTokenFromRange(ScSharedTokenRef& pToken, const ScRange& rRange)
{
    ScComplexRefData aData;
    aData.InitFlags();
    aData.Ref1.nCol = rRange.aStart.Col();
    aData.Ref1.nRow = rRange.aStart.Row();
    aData.Ref1.nTab = rRange.aStart.Tab();
    aData.Ref1.SetColRel(false);
    aData.Ref1.SetRowRel(false);
    aData.Ref1.SetTabRel(false);
    aData.Ref1.SetFlag3D(true);

    aData.Ref2.nCol = rRange.aEnd.Col();
    aData.Ref2.nRow = rRange.aEnd.Row();
    aData.Ref2.nTab = rRange.aEnd.Tab();
    aData.Ref2.SetColRel(false);
    aData.Ref2.SetRowRel(false);
    aData.Ref2.SetTabRel(false);
    // Display sheet name on 2nd reference only when the 1st and 2nd refs are on
    // different sheets.
    aData.Ref2.SetFlag3D(aData.Ref1.nTab != aData.Ref2.nTab);

    pToken.reset(new ScDoubleRefToken(aData));
}

void ScRefTokenHelper::getTokensFromRangeList(vector<ScSharedTokenRef>& pTokens, const ScRangeList& rRanges)
{
    vector<ScSharedTokenRef> aTokens;
    sal_uInt32 nCount = rRanges.Count();
    aTokens.reserve(nCount);
    for (sal_uInt32 i = 0; i < nCount; ++i)
    {
        ScRange* pRange = static_cast<ScRange*>(rRanges.GetObject(i));
        if (!pRange)
            // failed.
            return;

        ScSharedTokenRef pToken;
        ScRefTokenHelper::getTokenFromRange(pToken,* pRange);
        aTokens.push_back(pToken);
    }
    pTokens.swap(aTokens);
}

bool ScRefTokenHelper::isRef(const ScSharedTokenRef& pToken)
{
    switch (pToken->GetType())
    {
        case svSingleRef:
        case svDoubleRef:
        case svExternalSingleRef:
        case svExternalDoubleRef:
            return true;
        default:
            ;
    }
    return false;
}

bool ScRefTokenHelper::isExternalRef(const ScSharedTokenRef& pToken)
{
    switch (pToken->GetType())
    {
        case svExternalSingleRef:
        case svExternalDoubleRef:
            return true;
        default:
            ;
    }
    return false;
}

bool ScRefTokenHelper::intersects(const vector<ScSharedTokenRef>& rTokens, const ScSharedTokenRef& pToken)
{
    if (!isRef(pToken))
        return false;

    bool bExternal = isExternalRef(pToken);
    sal_uInt16 nFileId = bExternal ? pToken->GetIndex() : 0;

    ScRange aRange;
    getRangeFromToken(aRange, pToken, bExternal);

    vector<ScSharedTokenRef>::const_iterator itr = rTokens.begin(), itrEnd = rTokens.end();
    for (; itr != itrEnd; ++itr)
    {
        const ScSharedTokenRef& p = *itr;
        if (!isRef(p))
            continue;

        if (bExternal != isExternalRef(p))
            continue;

        ScRange aRange2;
        getRangeFromToken(aRange2, p, bExternal);

        if (bExternal && nFileId != p->GetIndex())
            // different external file
            continue;

        if (aRange.Intersects(aRange2))
            return true;
    }
    return false;
}

namespace {

class JoinRefTokenRanges
{
public:
    /**
     * Insert a new reference token into the existing list of reference tokens,
     * but in that process, try to join as many adjacent ranges as possible.
     *
     * @param rTokens existing list of reference tokens
     * @param rToken new token
     */
    void operator() (vector<ScSharedTokenRef>& rTokens, const ScSharedTokenRef& pToken)
    {
        join(rTokens, pToken);
    }

private:

    /**
     * Check two 1-dimensional ranges to see if they overlap each other.
     *
     * @param nMin1 min value of range 1
     * @param nMax1 max value of range 1
     * @param nMin2 min value of range 2
     * @param nMax2 max value of range 2
     * @param rNewMin min value of new range in case they overlap
     * @param rNewMax max value of new range in case they overlap
     */
    template<typename T>
    static bool overlaps(T nMin1, T nMax1, T nMin2, T nMax2, T& rNewMin, T& rNewMax)
    {
        bool bDisjoint1 = (nMin1 > nMax2) && (nMin1 - nMax2 > 1);
        bool bDisjoint2  = (nMin2 > nMax1) && (nMin2 - nMax1 > 1);
        if (bDisjoint1 || bDisjoint2)
            // These two ranges cannot be joined.  Move on.
            return false;

        T nMin = nMin1 < nMin2 ? nMin1 : nMin2;
        T nMax = nMax1 > nMax2 ? nMax1 : nMax2;

        rNewMin = nMin;
        rNewMax = nMax;

        return true;
    }

    bool isContained(const ScComplexRefData& aOldData, const ScComplexRefData& aData) const
    {
        // Check for containment.
        bool bRowsContained = (aOldData.Ref1.nRow <= aData.Ref1.nRow) && (aData.Ref2.nRow <= aOldData.Ref2.nRow);
        bool bColsContained = (aOldData.Ref1.nCol <= aData.Ref1.nCol) && (aData.Ref2.nCol <= aOldData.Ref2.nCol);
        return (bRowsContained && bColsContained);
    }

    void join(vector<ScSharedTokenRef>& rTokens, const ScSharedTokenRef& pToken)
    {
        // Normalize the token to a double reference.
        ScComplexRefData aData;
        if (!ScRefTokenHelper::getDoubleRefDataFromToken(aData, pToken))
            return;

        // Get the information of the new token.
        bool bExternal = ScRefTokenHelper::isExternalRef(pToken);
        sal_uInt16 nFileId = bExternal ? pToken->GetIndex() : 0;
        String aTabName = bExternal ? pToken->GetString() : String();

        bool bJoined = false;
        vector<ScSharedTokenRef>::iterator itr = rTokens.begin(), itrEnd = rTokens.end();
        for (; itr != itrEnd; ++itr)
        {
            ScSharedTokenRef& pOldToken = *itr;

            if (!ScRefTokenHelper::isRef(pOldToken))
                // A non-ref token should not have been added here in the first
                // place!
                continue;

            if (bExternal != ScRefTokenHelper::isExternalRef(pOldToken))
                // External and internal refs don't mix.
                continue;

            if (bExternal)
            {
                if (nFileId != pOldToken->GetIndex())
                    // Different external files.
                    continue;

                if (aTabName != pOldToken->GetString())
                    // Different table names.
                    continue;
            }

            ScComplexRefData aOldData;
            if (!ScRefTokenHelper::getDoubleRefDataFromToken(aOldData, pOldToken))
                continue;

            if (aData.Ref1.nTab != aOldData.Ref1.nTab || aData.Ref2.nTab != aOldData.Ref2.nTab)
                // Sheet ranges differ.
                continue;

            if (isContained(aOldData, aData))
                // This new range is part of an existing range.  Skip it.
                return;

            bool bSameRows = (aData.Ref1.nRow == aOldData.Ref1.nRow) && (aData.Ref2.nRow == aOldData.Ref2.nRow);
            bool bSameCols = (aData.Ref1.nCol == aOldData.Ref1.nCol) && (aData.Ref2.nCol == aOldData.Ref2.nCol);
            ScComplexRefData aNewData = aOldData;
            bool bJoinRanges = false;
            if (bSameRows)
            {
                bJoinRanges = overlaps(
                    aData.Ref1.nCol, aData.Ref2.nCol, aOldData.Ref1.nCol, aOldData.Ref2.nCol,
                    aNewData.Ref1.nCol, aNewData.Ref2.nCol);
            }
            else if (bSameCols)
            {
                bJoinRanges = overlaps(
                    aData.Ref1.nRow, aData.Ref2.nRow, aOldData.Ref1.nRow, aOldData.Ref2.nRow,
                    aNewData.Ref1.nRow, aNewData.Ref2.nRow);
            }

            if (bJoinRanges)
            {
                if (bExternal)
                    pOldToken.reset(new ScExternalDoubleRefToken(nFileId, aTabName, aNewData));
                else
                    pOldToken.reset(new ScDoubleRefToken(aNewData));

                bJoined = true;
                break;
            }
        }

        if (bJoined)
        {
            if (rTokens.size() == 1)
                // There is only one left.  No need to do more joining.
                return;

            // Pop the last token from the list, and keep joining recursively.
            ScSharedTokenRef p = rTokens.back();
            rTokens.pop_back();
            join(rTokens, p);
        }
        else
            rTokens.push_back(pToken);
    }
};

}

void ScRefTokenHelper::join(vector<ScSharedTokenRef>& rTokens, const ScSharedTokenRef& pToken)
{
    JoinRefTokenRanges join;
    join(rTokens, pToken);
}

bool ScRefTokenHelper::getDoubleRefDataFromToken(ScComplexRefData& rData, const ScSharedTokenRef& pToken)
{
    switch (pToken->GetType())
    {
        case svSingleRef:
        case svExternalSingleRef:
        {
            const ScSingleRefData& r = pToken->GetSingleRef();
            rData.Ref1 = r;
            rData.Ref1.SetFlag3D(true);
            rData.Ref2 = r;
            rData.Ref2.SetFlag3D(false); // Don't display sheet name on second reference.
        }
        break;
        case svDoubleRef:
        case svExternalDoubleRef:
            rData = pToken->GetDoubleRef();
        break;
        default:
            // Not a reference token.  Bail out.
            return false;
    }
    return true;
}
