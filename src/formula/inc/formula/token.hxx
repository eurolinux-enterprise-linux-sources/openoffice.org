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

#ifndef FORMULA_TOKEN_HXX
#define FORMULA_TOKEN_HXX

#include <memory>
#include <string.h>
#include <vector>
#include "formula/opcode.hxx"
//#include "refdata.hxx"
//#include "scmatrix.hxx"
#include "formula/intruref.hxx"
#include <tools/mempool.hxx>
#include "formula/IFunctionDescription.hxx"
#include "formula/formuladllapi.h"

namespace formula
{

enum StackVarEnum
{
    svByte,
    svDouble,
    svString,
    svSingleRef,
    svDoubleRef,
    svMatrix,
    svIndex,
    svJump,
    svExternal,                         // Byte + String
    svFAP,                              // FormulaAutoPilot only, ever exported
    svJumpMatrix,                       // 2003-07-02
    svRefList,                          // ocUnion result
    svEmptyCell,                        // Result is an empty cell, e.g. in LOOKUP()

    svMatrixCell,                       // Result is a matrix with bells and
                                        // whistles as needed for _the_ matrix
                                        // formula result.

    svHybridCell,                       // A temporary condition of a formula
                                        // cell during import, having a double
                                        // and/or string result and a formula
                                        // string to be compiled.

    svExternalSingleRef,
    svExternalDoubleRef,
    svExternalName,
    svError,                            // error token
    svMissing = 0x70,                   // 0 or ""
    svSep,                              // separator, ocSep, ocOpen, ocClose
    svUnknown                           // unknown StackType
};

#ifdef PRODUCT
// save memory since compilers tend to int an enum
typedef BYTE StackVar;
#else
// have enum names in debugger
typedef StackVarEnum StackVar;
#endif


class FormulaToken;
typedef SimpleIntrusiveReference< class FormulaToken > FormulaTokenRef;
typedef SimpleIntrusiveReference< const class FormulaToken > FormulaConstTokenRef;


class FORMULA_DLLPUBLIC FormulaToken : public IFormulaToken
{
    OpCode                      eOp;
    // not implemented, prevent usage
            FormulaToken();
            FormulaToken&            operator=( const FormulaToken& );
protected:

            const StackVar      eType;          // type of data
            mutable USHORT      nRefCnt;        // reference count

public:
                                FormulaToken( StackVar eTypeP,OpCode e = ocPush ) :
                                    eOp(e), eType( eTypeP ), nRefCnt(0) {}
                                FormulaToken( const FormulaToken& r ) : IFormulaToken(),
                                    eOp(r.eOp), eType( r.eType ), nRefCnt(0) {}

    virtual                     ~FormulaToken();

    inline  void                Delete()                { delete this; }
    inline  StackVar      		GetType() const         { return eType; }
            BOOL                IsFunction() const; // pure functions, no operators
            BOOL                IsMatrixFunction() const;   // if a function _always_ returns a Matrix
            BYTE                GetParamCount() const;
    inline  void                IncRef() const          { nRefCnt++; }
    inline  void                DecRef() const
                                    {
                                        if (!--nRefCnt)
                                            const_cast<FormulaToken*>(this)->Delete();
                                    }
    inline  USHORT              GetRef() const          { return nRefCnt; }
    inline OpCode               GetOpCode() const       { return eOp; }

    /**
        Dummy methods to avoid switches and casts where possible,
        the real token classes have to overload the appropriate method[s].
        The only methods valid anytime if not overloaded are:

        - GetByte() since this represents the count of parameters to a function
          which of course is 0 on non-functions. FormulaByteToken and ScExternal do
          overload it.

        - HasForceArray() since also this is only used for operators and
          functions and is 0 for other tokens.

        Any other non-overloaded method pops up an assertion.
     */

    virtual BYTE                GetByte() const;
    virtual void                SetByte( BYTE n );
    virtual bool                HasForceArray() const;
    virtual void                SetForceArray( bool b );
    virtual double              GetDouble() const;
    virtual double&             GetDoubleAsReference();
    virtual const String&       GetString() const;
    virtual USHORT              GetIndex() const;
    virtual void                SetIndex( USHORT n );
    virtual short*              GetJump() const;
    virtual const String&       GetExternal() const;
    virtual FormulaToken*       GetFAPOrigToken() const;
    virtual USHORT              GetError() const;
    virtual void                SetError( USHORT );

    virtual FormulaToken*       Clone() const { return new FormulaToken(*this); }

    virtual BOOL                Is3DRef() const;    // reference with 3D flag set
    virtual BOOL                TextEqual( const formula::FormulaToken& rToken ) const;
    virtual BOOL                operator==( const FormulaToken& rToken ) const;

    virtual bool isFunction() const 
    {
        return IsFunction();
    }

    virtual sal_uInt32 getArgumentCount() const 
    {
        return GetParamCount();
    }

    /** This is dirty and only the compiler should use it! */
    struct PrivateAccess { friend class FormulaCompiler; private: PrivateAccess() { }  };
    inline  void                NewOpCode( OpCode e, const PrivateAccess&  ) { eOp = e; }

    static  size_t              GetStrLenBytes( xub_StrLen nLen )
                                    { return nLen * sizeof(sal_Unicode); }
    static  size_t              GetStrLenBytes( const String& rStr )
                                    { return GetStrLenBytes( rStr.Len() ); }
};

class FORMULA_DLLPUBLIC FormulaByteToken : public FormulaToken
{
private:
            BYTE                nByte;
            bool                bHasForceArray;
protected:
                                FormulaByteToken( OpCode e, BYTE n, StackVar v, bool b ) :
                                    FormulaToken( v,e ), nByte( n ),
                                    bHasForceArray( b ) {}
public:
                                FormulaByteToken( OpCode e, BYTE n, bool b ) :
                                    FormulaToken( svByte,e ), nByte( n ),
                                    bHasForceArray( b ) {}
                                FormulaByteToken( OpCode e, BYTE n ) :
                                    FormulaToken( svByte,e ), nByte( n ),
                                    bHasForceArray( false ) {}
                                FormulaByteToken( OpCode e ) :
                                    FormulaToken( svByte,e ), nByte( 0 ),
                                    bHasForceArray( false ) {}
                                FormulaByteToken( const FormulaByteToken& r ) :
                                    FormulaToken( r ), nByte( r.nByte ),
                                    bHasForceArray( r.bHasForceArray ) {}

    virtual FormulaToken*       Clone() const { return new FormulaByteToken(*this); }
    virtual BYTE                GetByte() const;
    virtual void                SetByte( BYTE n );
    virtual bool                HasForceArray() const;
    virtual void                SetForceArray( bool b );
    virtual BOOL                operator==( const FormulaToken& rToken ) const;

    DECL_FIXEDMEMPOOL_NEWDEL_DLL( FormulaByteToken )
};


// A special token for the FormulaAutoPilot only. Keeps a reference pointer of
// the token of which it was created for comparison.
class FORMULA_DLLPUBLIC FormulaFAPToken : public FormulaByteToken
{
private:
            FormulaTokenRef          pOrigToken;
public:
                                FormulaFAPToken( OpCode e, BYTE n, FormulaToken* p ) :
                                    FormulaByteToken( e, n, svFAP, false ),
                                    pOrigToken( p ) {}
                                FormulaFAPToken( const FormulaFAPToken& r ) :
                                    FormulaByteToken( r ), pOrigToken( r.pOrigToken ) {}

    virtual FormulaToken*       Clone() const { return new FormulaFAPToken(*this); }
    virtual FormulaToken*       GetFAPOrigToken() const;
    virtual BOOL                operator==( const FormulaToken& rToken ) const;
};

class FORMULA_DLLPUBLIC FormulaDoubleToken : public FormulaToken
{
private:
            double              fDouble;
public:
                                FormulaDoubleToken( double f ) :
                                    FormulaToken( svDouble ), fDouble( f ) {}
                                FormulaDoubleToken( const FormulaDoubleToken& r ) :
                                    FormulaToken( r ), fDouble( r.fDouble ) {}

    virtual FormulaToken*       Clone() const { return new FormulaDoubleToken(*this); }
    virtual double              GetDouble() const;
    virtual double&             GetDoubleAsReference();
    virtual BOOL                operator==( const FormulaToken& rToken ) const;

    DECL_FIXEDMEMPOOL_NEWDEL_DLL( FormulaDoubleToken )
};


class FORMULA_DLLPUBLIC FormulaStringToken : public FormulaToken
{
private:
            String              aString;
public:
                                FormulaStringToken( const String& r ) :
                                    FormulaToken( svString ), aString( r ) {}
                                FormulaStringToken( const FormulaStringToken& r ) :
                                    FormulaToken( r ), aString( r.aString ) {}

    virtual FormulaToken*       Clone() const { return new FormulaStringToken(*this); }
    virtual const String&       GetString() const;
    virtual BOOL                operator==( const FormulaToken& rToken ) const;

    DECL_FIXEDMEMPOOL_NEWDEL_DLL( FormulaStringToken )
};


/** Identical to FormulaStringToken, but with explicit OpCode instead of implicit
    ocPush, and an optional BYTE for ocBad tokens. */
class FORMULA_DLLPUBLIC FormulaStringOpToken : public FormulaByteToken
{
private:
            String              aString;
public:
                                FormulaStringOpToken( OpCode e, const String& r ) :
                                    FormulaByteToken( e, 0, svString, false ), aString( r ) {}
                                FormulaStringOpToken( const FormulaStringOpToken& r ) :
                                    FormulaByteToken( r ), aString( r.aString ) {}

    virtual FormulaToken*       Clone() const { return new FormulaStringOpToken(*this); }
    virtual const String&       GetString() const;
    virtual BOOL                operator==( const FormulaToken& rToken ) const;
};

class FORMULA_DLLPUBLIC FormulaIndexToken : public FormulaToken
{
private:
            USHORT              nIndex;
public:
                                FormulaIndexToken( OpCode e, USHORT n ) :
                                    FormulaToken(  svIndex, e ), nIndex( n ) {}
                                FormulaIndexToken( const FormulaIndexToken& r ) :
                                    FormulaToken( r ), nIndex( r.nIndex ) {}

    virtual FormulaToken*       Clone() const { return new FormulaIndexToken(*this); }
    virtual USHORT              GetIndex() const;
    virtual void                SetIndex( USHORT n );
    virtual BOOL                operator==( const FormulaToken& rToken ) const;
};


class FORMULA_DLLPUBLIC FormulaExternalToken : public FormulaToken
{
private:
            String              aExternal;
            BYTE                nByte;
public:
                                FormulaExternalToken( OpCode e, BYTE n, const String& r ) :
                                    FormulaToken( svExternal, e ), aExternal( r ),
                                    nByte( n ) {}
                                FormulaExternalToken( OpCode e, const String& r ) :
                                    FormulaToken(svExternal, e ), aExternal( r ),
                                    nByte( 0 ) {}
                                FormulaExternalToken( const FormulaExternalToken& r ) :
                                    FormulaToken( r ), aExternal( r.aExternal ),
                                    nByte( r.nByte ) {}

    virtual FormulaToken*       Clone() const { return new FormulaExternalToken(*this); }
    virtual const String&       GetExternal() const;
    virtual BYTE                GetByte() const;
    virtual void                SetByte( BYTE n );
    virtual BOOL                operator==( const FormulaToken& rToken ) const;
};


class FORMULA_DLLPUBLIC FormulaMissingToken : public FormulaToken
{
public:
                                FormulaMissingToken() :
                                    FormulaToken( svMissing,ocMissing ) {}
                                FormulaMissingToken( const FormulaMissingToken& r ) :
                                    FormulaToken( r ) {}

    virtual FormulaToken*       Clone() const { return new FormulaMissingToken(*this); }
    virtual double              GetDouble() const;
    virtual const String&       GetString() const;
    virtual BOOL                operator==( const FormulaToken& rToken ) const;
};

class FORMULA_DLLPUBLIC FormulaJumpToken : public FormulaToken
{
private:
            short*              pJump;
public:
                                FormulaJumpToken( OpCode e, short* p ) :
                                    FormulaToken( formula::svJump , e)
                                {
                                    pJump = new short[ p[0] + 1 ];
                                    memcpy( pJump, p, (p[0] + 1) * sizeof(short) );
                                }
                                FormulaJumpToken( const FormulaJumpToken& r ) :
                                    FormulaToken( r )
                                {
                                    pJump = new short[ r.pJump[0] + 1 ];
                                    memcpy( pJump, r.pJump, (r.pJump[0] + 1) * sizeof(short) );
                                }
    virtual                     ~FormulaJumpToken();
    virtual short*              GetJump() const;
    virtual BOOL                operator==( const formula::FormulaToken& rToken ) const;
    virtual FormulaToken*       Clone() const { return new FormulaJumpToken(*this); }
};


class FORMULA_DLLPUBLIC FormulaUnknownToken : public FormulaToken
{
public:
                                FormulaUnknownToken( OpCode e ) :
                                    FormulaToken( svUnknown, e ) {}
                                FormulaUnknownToken( const FormulaUnknownToken& r ) :
                                    FormulaToken( r ) {}

    virtual FormulaToken*       Clone() const { return new FormulaUnknownToken(*this); }
    virtual BOOL                operator==( const FormulaToken& rToken ) const;
};


class FORMULA_DLLPUBLIC FormulaErrorToken : public FormulaToken
{
            USHORT              nError;
public:
                                FormulaErrorToken( USHORT nErr ) :
                                    FormulaToken( svError ), nError( nErr) {}
                                FormulaErrorToken( const FormulaErrorToken& r ) :
                                    FormulaToken( r ), nError( r.nError) {}

    virtual FormulaToken*       Clone() const { return new FormulaErrorToken(*this); }
    virtual USHORT              GetError() const;
    virtual void                SetError( USHORT nErr );
    virtual BOOL                operator==( const FormulaToken& rToken ) const;
};

// =============================================================================
} // formula
// =============================================================================

#endif
