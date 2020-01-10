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

#ifndef FORMULA_TOKENARRAY_HXX
#define FORMULA_TOKENARRAY_HXX

#include "formula/token.hxx"
#include "formula/ExternalReferenceHelper.hxx"
#include <tools/solar.h>
#include <com/sun/star/sheet/FormulaToken.hpp>

#include <limits.h>

namespace formula
{

// RecalcMode access only via TokenArray SetRecalcMode / IsRecalcMode...

typedef BYTE ScRecalcMode;
// Only one of the exclusive bits can be set,
// handled by TokenArray SetRecalcMode... methods
#define RECALCMODE_NORMAL       0x01    // exclusive
#define RECALCMODE_ALWAYS       0x02    // exclusive, always
#define RECALCMODE_ONLOAD       0x04    // exclusive, always after load
#define RECALCMODE_ONLOAD_ONCE  0x08    // exclusive, once after load
#define RECALCMODE_FORCED       0x10    // combined, also if cell isn't visible
#define RECALCMODE_ONREFMOVE    0x20    // combined, if reference was moved
#define RECALCMODE_EMASK        0x0F    // mask of exclusive bits
// If new bits are to be defined, AddRecalcMode has to be adjusted!

class FormulaMissingContext;

class FORMULA_DLLPUBLIC MissingConvention
{
    bool    mbODFF;     /// TRUE: ODFF, FALSE: PODF
public:
    explicit    MissingConvention( bool bODFF ) : mbODFF(bODFF) {}
    // Implementation and usage only in token.cxx
    inline  bool    isRewriteNeeded( OpCode eOp ) const;
    inline  bool    isODFF() const { return mbODFF; }
};

class FORMULA_DLLPUBLIC FormulaTokenArray
{
    friend class FormulaCompiler;
    friend class FormulaTokenIterator;
    friend class FormulaMissingContext;

protected:
    FormulaToken**  pCode;                  // Token code array
    FormulaToken**  pRPN;                   // RPN array
    USHORT          nLen;                   // Length of token array
    USHORT          nRPN;                   // Length of RPN array
    USHORT          nIndex;                 // Current step index
    USHORT          nError;                 // Error code
    short           nRefs;                  // Count of cell references
    ScRecalcMode    nMode;                  // Flags to indicate when to recalc this code
    BOOL            bHyperLink;             // If HYPERLINK() occurs in the formula.

protected:
    void                    Assign( const FormulaTokenArray& );

    /// Also used by the compiler. The token MUST had been allocated with new!
    FormulaToken*           Add( FormulaToken* );
    inline  void            SetCombinedBitsRecalcMode( ScRecalcMode nBits )
                                { nMode |= (nBits & ~RECALCMODE_EMASK); }
    inline  ScRecalcMode    GetCombinedBitsRecalcMode() const
                                { return nMode & ~RECALCMODE_EMASK; }
                            /** Exclusive bits already set in nMode are
                                zero'ed, nVal may contain combined bits, but
                                only one exclusive bit may be set! */
    inline  void            SetMaskedRecalcMode( ScRecalcMode nBits )
                                { nMode = GetCombinedBitsRecalcMode() | nBits; }

public:
    FormulaTokenArray();
    /// Assignment with references to FormulaToken entries (not copied!)
    FormulaTokenArray( const FormulaTokenArray& );
    virtual ~FormulaTokenArray();
    FormulaTokenArray* Clone() const;    /// True copy!
    void Clear();
    void DelRPN();
    FormulaToken* First() { nIndex = 0; return Next(); }
    FormulaToken* Next();
    FormulaToken* FirstNoSpaces() { nIndex = 0; return NextNoSpaces(); }
    FormulaToken* NextNoSpaces();
    FormulaToken* GetNextName();
    FormulaToken* GetNextDBArea();
    FormulaToken* GetNextReference();
    FormulaToken* GetNextReferenceRPN();
    FormulaToken* GetNextReferenceOrName();
    FormulaToken* GetNextColRowName();
    FormulaToken* GetNextOpCodeRPN( OpCode );
    /// Peek at nIdx-1 if not out of bounds, decrements nIdx if successful. Returns NULL if not.
    FormulaToken* PeekPrev( USHORT & nIdx );
    FormulaToken* PeekNext();
    FormulaToken* PeekPrevNoSpaces();    /// Only after Reset/First/Next/Last/Prev!
    FormulaToken* PeekNextNoSpaces();    /// Only after Reset/First/Next/Last/Prev!
    FormulaToken* FirstRPN() { nIndex = 0; return NextRPN(); }
    FormulaToken* NextRPN();
    FormulaToken* LastRPN() { nIndex = nRPN; return PrevRPN(); }
    FormulaToken* PrevRPN();

    BOOL    HasOpCode( OpCode ) const;
    BOOL    HasOpCodeRPN( OpCode ) const;
    /// Token of type svIndex or opcode ocColRowName
    BOOL    HasNameOrColRowName() const;

    FormulaToken** GetArray() const  { return pCode; }
    FormulaToken** GetCode()  const  { return pRPN; }
    USHORT    GetLen() const     { return nLen; }
    USHORT    GetCodeLen() const { return nRPN; }
    void      Reset()            { nIndex = 0; }
    USHORT    GetCodeError() const      { return nError; }
    void      SetCodeError( USHORT n )  { nError = n; }
    short     GetRefs()  const { return nRefs;  }
    void      IncrementRefs() { ++nRefs; }
    void      SetHyperLink( BOOL bVal ) { bHyperLink = bVal; }
    BOOL      IsHyperLink() const       { return bHyperLink; }

    inline  ScRecalcMode    GetRecalcMode() const { return nMode; }
                            /** Bits aren't set directly but validated and
                                maybe handled according to priority if more
                                than one exclusive bit was set. */
            void            AddRecalcMode( ScRecalcMode nBits );

    inline  void            ClearRecalcMode() { nMode = RECALCMODE_NORMAL; }
    inline  void            SetRecalcModeNormal()
                                { SetMaskedRecalcMode( RECALCMODE_NORMAL ); }
    inline  void            SetRecalcModeAlways()
                                { SetMaskedRecalcMode( RECALCMODE_ALWAYS ); }
    inline  void            SetRecalcModeOnLoad()
                                { SetMaskedRecalcMode( RECALCMODE_ONLOAD ); }
    inline  void            SetRecalcModeOnLoadOnce()
                                { SetMaskedRecalcMode( RECALCMODE_ONLOAD_ONCE ); }
    inline  void            SetRecalcModeForced()
                                { nMode |= RECALCMODE_FORCED; }
    inline  void            ClearRecalcModeForced()
                                { nMode &= ~RECALCMODE_FORCED; }
    inline  void            SetRecalcModeOnRefMove()
                                { nMode |= RECALCMODE_ONREFMOVE; }
    inline  void            ClearRecalcModeOnRefMove()
                                { nMode &= ~RECALCMODE_ONREFMOVE; }
    inline  BOOL            IsRecalcModeNormal() const
                                { return (nMode & RECALCMODE_NORMAL) != 0; }
    inline  BOOL            IsRecalcModeAlways() const
                                { return (nMode & RECALCMODE_ALWAYS) != 0; }
    inline  BOOL            IsRecalcModeOnLoad() const
                                { return (nMode & RECALCMODE_ONLOAD) != 0; }
    inline  BOOL            IsRecalcModeOnLoadOnce() const
                                { return (nMode & RECALCMODE_ONLOAD_ONCE) != 0; }
    inline  BOOL            IsRecalcModeForced() const
                                { return (nMode & RECALCMODE_FORCED) != 0; }
    inline  BOOL            IsRecalcModeOnRefMove() const
                                { return (nMode & RECALCMODE_ONREFMOVE) != 0; }

                            /** Get OpCode of the most outer function */
    inline OpCode           GetOuterFuncOpCode();

                            /** Operators +,-,*,/,^,&,=,<>,<,>,<=,>=
                                with DoubleRef in Formula? */
    BOOL                    HasMatrixDoubleRefOps();

    virtual FormulaToken* AddOpCode(OpCode e);

    /** Adds the single token to array.
        Derived classes must overload it when they want to support derived classes from FormulaToken.
        @return true        when an error occurs
    */
    virtual bool AddFormulaToken(const com::sun::star::sheet::FormulaToken& _aToken, ExternalReferenceHelper* _pRef = NULL);

    /** fill the array with the tokens from the sequence.
        It calls AddFormulaToken for each token in the list.
        @param  _aSequence  the token to add
        @return true        when an error occurs
    */
    bool Fill(const com::sun::star::uno::Sequence< com::sun::star::sheet::FormulaToken >& _aSequence, ExternalReferenceHelper* _pRef = NULL);

    FormulaToken* AddToken( const FormulaToken& );
    FormulaToken* AddString( const sal_Unicode* pStr );
    FormulaToken* AddString( const String& rStr );
    FormulaToken* AddDouble( double fVal );
    FormulaToken* AddName( USHORT n );
    FormulaToken* AddExternal( const sal_Unicode* pStr );
    /** Xcl import may play dirty tricks with OpCode!=ocExternal.
        Others don't use! */
    FormulaToken* AddExternal( const String& rStr, OpCode eOp = ocExternal );
    FormulaToken* AddBad( const sal_Unicode* pStr );     /// ocBad with String
    FormulaToken* AddBad( const String& rStr );          /// ocBad with String

    virtual FormulaToken* MergeArray( );

    /// Assignment with references to FormulaToken entries (not copied!)
    FormulaTokenArray& operator=( const FormulaTokenArray& );

    /** Determines if this formula needs any changes to convert it to something
        previous versions of OOo could consume (Plain Old Formula). */
            bool                NeedsPofRewrite(const MissingConvention & rConv);

    /** Rewrites to Plain Old Formula, substituting missing parameters. The
        FormulaTokenArray* returned is new'ed. */
            FormulaTokenArray*  RewriteMissingToPof(const MissingConvention & rConv);

    /** Determines if this formula may be followed by a reference. */
            bool                MayReferenceFollow();
};

inline OpCode FormulaTokenArray::GetOuterFuncOpCode()
{
    if ( pRPN && nRPN )
        return pRPN[nRPN-1]->GetOpCode();
    return ocNone;
}

struct ImpTokenIterator
{
    ImpTokenIterator* pNext;
    const FormulaTokenArray* pArr;
    short nPC;
    short nStop;

    DECL_FIXEDMEMPOOL_NEWDEL( ImpTokenIterator );
};

class FORMULA_DLLPUBLIC FormulaTokenIterator
{
    ImpTokenIterator* pCur;
    
public:
    FormulaTokenIterator( const FormulaTokenArray& );
   ~FormulaTokenIterator();
    void    Reset();
    const   FormulaToken* First();
    const   FormulaToken* Next();
    bool    IsEndOfPath() const;    /// if a jump or subroutine path is done
    bool    HasStacked() const { return pCur->pNext != 0; }
    short   GetPC() const { return pCur->nPC; }

    /** Jump or subroutine call.
        Program counter values will be incremented before code is executed =>
        positions are to be passed with -1 offset.
        @param nStart
            Start on code at position nStart+1 (yes, pass with offset -1)
        @param nNext
            After subroutine continue with instruction at position nNext+1
        @param nStop
            Stop before reaching code at position nStop. If not specified the
            default is to either run the entire code, or to stop if an ocSep or
            ocClose is encountered, which are only present in ocIf or ocChose
            jumps.
      */
    void Jump( short nStart, short nNext, short nStop = SHRT_MAX );
    void Push( const FormulaTokenArray* );
    void Pop();
};
// =============================================================================
} // formula
// =============================================================================


#endif // FORMULA_TOKENARRAY_HXX

