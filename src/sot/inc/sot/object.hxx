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

#ifndef _SOT_OBJECT_HXX
#define _SOT_OBJECT_HXX

#include <sot/sotref.hxx>
#ifndef _SOT_SOTDATA_HXX
#include <sot/sotdata.hxx>
#endif
#ifndef _TOOLS_GLOBNAME_HXX
#include <tools/globname.hxx>
#endif
#include "sot/sotdllapi.h"

/*************************************************************************
*************************************************************************/

#define TEST_INVARIANT
#ifdef TEST_INVARIANT
#define SO2_DECL_INVARIANT()                                            \
        virtual void TestObjRef( BOOL bFree );                          \
        void         TestMemberObjRef( BOOL bFree );                    \
        virtual void TestInvariant( BOOL bPrint );                      \
        void         TestMemberInvariant( BOOL bPrint );

#define SO2_IMPL_INVARIANT(ClassName)                                   \
void __EXPORT ClassName::TestObjRef( BOOL bFree )                       \
{                                                                       \
    TestMemberObjRef( bFree );                                          \
}                                                                       \
void __EXPORT ClassName::TestInvariant( BOOL bPrint )                   \
{                                                                       \
    TestMemberInvariant( bPrint );                                      \
}

#define SO2_IMPL_INVARIANT1(ClassName,Super1)                           \
void __EXPORT ClassName::TestObjRef( BOOL bFree )                       \
{                                                                       \
    TestMemberObjRef( bFree );                                          \
    Super1::TestObjRef( bFree );                                        \
}                                                                       \
void __EXPORT ClassName::TestInvariant( BOOL bPrint )                   \
{                                                                       \
    TestMemberInvariant( bPrint );                                      \
    Super1::TestInvariant( bPrint );                                    \
}

#define SO2_IMPL_INVARIANT2(ClassName,Super1,Super2)                    \
void __EXPORT ClassName::TestObjRef( BOOL bFree )                       \
{                                                                       \
    TestMemberObjRef( bFree );                                          \
    Super1::TestObjRef( bFree );                                        \
    Super2::TestObjRef( bFree );                                        \
}                                                                       \
void __EXPORT ClassName::TestInvariant( BOOL bPrint )                   \
{                                                                       \
    TestMemberInvariant( bPrint );                                      \
    Super1::TestInvariant( bPrint );                                    \
    Super2::TestInvariant( bPrint );                                    \
}

#define SO2_IMPL_INVARIANT3(ClassName,Super1,Super2,Super3)             \
void __EXPORT ClassName::TestObjRef( BOOL bFree )                       \
{                                                                       \
    TestMemberObjRef( bFree );                                          \
    Super1::TestObjRef( bFree );                                        \
    Super2::TestObjRef( bFree );                                        \
    Super3::TestObjRef( bFree );                                        \
}                                                                       \
void __EXPORT ClassName::TestInvariant( BOOL bPrint )                   \
{                                                                       \
    TestMemberInvariant( bPrint );                                      \
    Super1::TestInvariant( bPrint );                                    \
    Super2::TestInvariant( bPrint );                                    \
    Super3::TestInvariant( bPrint );                                    \
}

#define SO2_IMPL_INVARIANT4(ClassName,Super1,Super2,Super3,Super4)      \
void __EXPORT ClassName::TestObjRef( BOOL bFree )                       \
{                                                                       \
    TestMemberObjRef( bFree );                                          \
    Super1::TestObjRef( bFree );                                        \
    Super2::TestObjRef( bFree );                                        \
    Super3::TestObjRef( bFree );                                        \
    Super4::TestObjRef( bFree );                                        \
}                                                                       \
void __EXPORT ClassName::TestInvariant( BOOL bPrint )                   \
{                                                                       \
    TestMemberInvariant( bPrint );                                      \
    Super1::TestInvariant( bPrint );                                    \
    Super2::TestInvariant( bPrint );                                    \
    Super3::TestInvariant( bPrint );                                    \
    Super4::TestInvariant( bPrint );                                    \
}

#ifdef DBG_UTIL
#define CALL_TEST_INVARIANT() SotFactory::TestInvariant()
#else
#define CALL_TEST_INVARIANT()
#endif  // DBG_UTIL

#else   // TEST_INVARIANT

#define SO2_DECL_INVARIANT()

#define SO2_IMPL_INVARIANT(ClassName)
#define SO2_IMPL_INVARIANT1(ClassName,Super1)
#define SO2_IMPL_INVARIANT2(ClassName,Super1,Super2)
#define SO2_IMPL_INVARIANT3(ClassName,Super1,Super2,Super3)
#define SO2_IMPL_INVARIANT4(ClassName,Super1,Super2,Super3,Super4)

#define CALL_TEST_INVARIANT()

#endif  // TEST_INVARIANT

/**************************************************************************
**************************************************************************/
#define SO2_DECL_BASIC_CLASS_DLL(ClassName,FacName)                       \
private:                                                                  \
    static SotFactory **       GetFactoryAdress()                          \
                              { return &(FacName->p##ClassName##Factory); } \
public:                                                                   \
    static void *             CreateInstance( SotObject ** = NULL );       \
    static SotFactory *        ClassFactory();                             \
    virtual const SotFactory * GetSvFactory() const;                       \
    virtual void *            Cast( const SotFactory * );

#define SO2_DECL_BASIC_CLASS(ClassName)                                   \
private:                                                                  \
    static SotFactory *        pFactory;                                   \
    static SotFactory **       GetFactoryAdress() { return &pFactory; }    \
public:                                                                   \
    static void *             CreateInstance( SotObject ** = NULL );       \
    static SotFactory *        ClassFactory();                             \
    virtual const SotFactory * GetSvFactory() const;                       \
    virtual void *            Cast( const SotFactory * );

/**************************************************************************
**************************************************************************/
#define SO2_IMPL_BASIC_CLASS_DLL(ClassName,FactoryName,GlobalName)        \
SotFactory * ClassName::ClassFactory()                                     \
{                                                                         \
    SotFactory **ppFactory = GetFactoryAdress();                           \
    if( !*ppFactory )                                                     \
    {                                                                     \
        *ppFactory = new FactoryName( GlobalName,                         \
			String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( #ClassName ) ), \
								 ClassName::CreateInstance );			  \
    }                                                                     \
    return *ppFactory;                                                    \
}                                                                         \
void * __EXPORT ClassName::CreateInstance( SotObject ** ppObj )            \
{                                                                         \
    ClassName * p = new ClassName();                                      \
    if( ppObj )                                                           \
        *ppObj = p;                                                       \
    return p;                                                             \
}                                                                         \
const SotFactory * __EXPORT ClassName::GetSvFactory() const                         \
{                                                                         \
    return ClassFactory();                                                \
}                                                                         \
void * __EXPORT ClassName::Cast( const SotFactory * pFact )                         \
{                                                                         \
    void * pRet = NULL;                                                   \
    if( !pFact || pFact == ClassFactory() )                               \
        pRet = this;                                                      \
    return pRet;                                                          \
}

#define SO2_IMPL_BASIC_CLASS(ClassName,FactoryName,GlobalName)                        \
SotFactory * ClassName::pFactory = NULL;                                   \
    SO2_IMPL_BASIC_CLASS_DLL(ClassName,FactoryName,GlobalName)

/**************************************************************************
**************************************************************************/
#define SO2_IMPL_BASIC_CLASS1_DLL(ClassName,FactoryName,Super1,GlobalName)\
SotFactory * ClassName::ClassFactory()                                     \
{                                                                         \
    SotFactory **ppFactory = GetFactoryAdress();                           \
    if( !*ppFactory )                                                     \
    {                                                                     \
        *ppFactory = new FactoryName( GlobalName,                         \
			String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( #ClassName ) ), \
								ClassName::CreateInstance );				\
        (*ppFactory)->PutSuperClass( Super1::ClassFactory() );            \
    }                                                                     \
    return *ppFactory;                                                    \
}                                                                         \
void * __EXPORT ClassName::CreateInstance( SotObject ** ppObj )            \
{                                                                         \
    ClassName * p = new ClassName();                                      \
    Super1* pSuper1 = p;                                                  \
    SotObject* pBasicObj = pSuper1;                                        \
    if( ppObj )                                                           \
        *ppObj = pBasicObj;                                               \
    return p;                                                             \
}                                                                         \
const SotFactory * __EXPORT ClassName::GetSvFactory() const                \
{                                                                         \
    return ClassFactory();                                                \
}                                                                         \
void * __EXPORT ClassName::Cast( const SotFactory * pFact )                \
{                                                                         \
    void * pRet = NULL;                                                   \
    if( !pFact || pFact == ClassFactory() )                               \
        pRet = this;                                                      \
    if( !pRet )                                                           \
        pRet = Super1::Cast( pFact );                                     \
    return pRet;                                                          \
}

#define SO2_IMPL_BASIC_CLASS1(ClassName,FactoryName,Super1,GlobalName)    \
SotFactory * ClassName::pFactory = NULL;                                   \
    SO2_IMPL_BASIC_CLASS1_DLL(ClassName,FactoryName,Super1,GlobalName)

/**************************************************************************
**************************************************************************/
#define SO2_IMPL_BASIC_CLASS2_DLL(ClassName,FactoryName,Super1,Super2,GlobalName)  \
SotFactory * ClassName::ClassFactory()                                     \
{                                                                         \
    SotFactory **ppFactory = GetFactoryAdress();                           \
    if( !*ppFactory )                                                     \
    {                                                                     \
        *ppFactory = new FactoryName( GlobalName,                         \
			String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( #ClassName ) ), \
								 ClassName::CreateInstance );			  \
        (*ppFactory)->PutSuperClass( Super1::ClassFactory() );            \
        (*ppFactory)->PutSuperClass( Super2::ClassFactory() );            \
    }                                                                     \
    return *ppFactory;                                                    \
}                                                                         \
void * __EXPORT ClassName::CreateInstance( SotObject ** ppObj )            \
{                                                                         \
    ClassName * p = new ClassName();                                      \
    if( ppObj )                                                           \
        *ppObj = p;                                                       \
    return p;                                                             \
}                                                                         \
const SotFactory * __EXPORT ClassName::GetSvFactory() const                \
{                                                                         \
    return ClassFactory();                                                \
}                                                                         \
void * __EXPORT ClassName::Cast( const SotFactory * pFact )                \
{                                                                         \
    void * pRet = NULL;                                                   \
    if( !pFact || pFact == ClassFactory() )                               \
        pRet = this;                                                      \
    if( !pRet )                                                           \
        pRet = Super1::Cast( pFact );                                     \
    if( !pRet )                                                           \
        pRet = Super2::Cast( pFact );                                     \
    return pRet;                                                          \
}
#define SO2_IMPL_BASIC_CLASS2(ClassName,FactoryName,Super1,Super2,GlobalName)  \
SotFactory * ClassName::pFactory = NULL;                                   \
    SO2_IMPL_BASIC_CLASS2_DLL(ClassName,FactoryName,Super1,Super2,GlobalName)

/**************************************************************************
**************************************************************************/
#define SO2_IMPL_BASIC_CLASS3_DLL(ClassName,FactoryName,Super1,Super2,Super3,GlobalName)  \
SotFactory * ClassName::ClassFactory()                                     \
{                                                                         \
    SotFactory **ppFactory = GetFactoryAdress();                           \
    if( !*ppFactory )                                                     \
    {                                                                     \
        *ppFactory = new FactoryName( GlobalName,                         \
			String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( #ClassName ) ), \
								 ClassName::CreateInstance );\
        (*ppFactory)->PutSuperClass( Super1::ClassFactory() );            \
        (*ppFactory)->PutSuperClass( Super2::ClassFactory() );            \
        (*ppFactory)->PutSuperClass( Super3::ClassFactory() );            \
    }                                                                     \
    return *pFactory;                                                     \
}                                                                         \
void * __EXPORT ClassName::CreateInstance( SotObject ** ppObj )            \
{                                                                         \
    ClassName * p = new ClassName();                                      \
    if( ppObj )                                                           \
        *ppObj = p;                                                       \
    return p;                                                             \
}                                                                         \
const SotFactory * __EXPORT ClassName::GetSvFactory() const                \
{                                                                         \
    return ClassFactory();                                                \
}                                                                         \
void * __EXPORT ClassName::Cast( const SotFactory * pFact )                \
{                                                                         \
    void * pRet = NULL;                                                   \
    if( !pFact || pFact == ClassFactory() )                               \
        pRet = this;                                                      \
    if( !pRet )                                                           \
        pRet = Super1::Cast( pFact );                                     \
    if( !pRet )                                                           \
        pRet = Super2::Cast( pFact );                                     \
    if( !pRet )                                                           \
        pRet = Super3::Cast( pFact );                                     \
    return pRet;                                                          \
}

#define SO2_IMPL_BASIC_CLASS3(ClassName,FactoryName,Super1,Super2,Super3,GlobalName)  \
SotFactory * ClassName::pFactory = NULL;                                   \
    SO2_IMPL_BASIC_CLASS3_DLL(ClassName,FactoryName,Super1,Super2,Super3,GlobalName)

/**************************************************************************
**************************************************************************/
#define SO2_IMPL_BASIC_CLASS4_DLL(ClassName,FactoryName,Super1,Super2,Super3,Super4,GlobalName)  \
SotFactory * ClassName::ClassFactory()                                     \
{                                                                         \
    SotFactory **ppFactory = GetFactoryAdress();                           \
    if( !*ppFactory )                                                     \
    {                                                                     \
        *ppFactory = new SotFactory( GlobalName,                           \
			String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( #ClassName ) ), \
								 ClassName::CreateInstance );\
        (*ppFactory)->PutSuperClass( Super1::ClassFactory() );            \
        (*ppFactory)->PutSuperClass( Super2::ClassFactory() );            \
        (*ppFactory)->PutSuperClass( Super3::ClassFactory() );            \
        (*ppFactory)->PutSuperClass( Super4::ClassFactory() );            \
    }                                                                     \
    return *ppFactory;                                                    \
}                                                                         \
void * __EXPORT ClassName::CreateInstance( SotObject ** ppObj )            \
{                                                                         \
    ClassName * p = new ClassName();                                      \
    if( ppObj )                                                           \
        *ppObj = p;                                                       \
    return p;                                                             \
}                                                                         \
const SotFactory * __EXPORT ClassName::GetSvFactory() const                \
{                                                                         \
    return ClassFactory();                                                \
}                                                                         \
void * __EXPORT ClassName::Cast( const SotFactory * pFact )                \
{                                                                         \
    void * pRet = NULL;                                                   \
    if( !pFact || pFact == ClassFactory() )                               \
        pRet = this;                                                      \
    if( !pRet )                                                           \
        pRet = Super1::Cast( pFact );                                     \
    if( !pRet )                                                           \
        pRet = Super2::Cast( pFact );                                     \
    if( !pRet )                                                           \
        pRet = Super3::Cast( pFact );                                     \
    if( !pRet )                                                           \
        pRet = Super4::Cast( pFact );                                     \
    return pRet;                                                          \
}

#define SO2_IMPL_BASIC_CLASS4(ClassName,FactoryName,Super1,Super2,Super3,Super4,GlobalName)  \
SotFactory * ClassName::pFactory = NULL;                                   \
    SO2_IMPL_BASIC_CLASS4_DLL(ClassName,FactoryName,Super1,Super2,Super3,Super4,GlobalName)

//==================class SotObject========================================
#ifdef _MSC_VER
#pragma warning(disable: 4250)
#endif

class SvAggregateMemberList;
struct IUnknown;
class SOT_DLLPUBLIC SotObject : virtual public SvRefBase
{
friend class SotFactory;
friend class SvObject;
    SvAggregateMemberList * pAggList; // fuer Aggregation, erstes ist das MainObj
    USHORT      nStrongLockCount;
    USHORT      nOwnerLockCount;
    BOOL        bOwner:1,
                bSVObject:1,        // Ist Proxy, dann TRUE wenn andere Seite SV ist
                bInClose:1;         // TRUE, im DoClose

    void *      DownAggCast( const SotFactory * pFact );
    void        RemoveInterface( ULONG );
    void        RemoveInterface( SotObject * );
#if defined (GCC) && (defined (C281) || defined (C290) || defined (C291))
public:
#else
protected:
#endif
    virtual             ~SotObject();
    void                SetExtern() { bOwner = FALSE; }
    virtual BOOL        Close();
public:
                        SotObject();
                        SO2_DECL_BASIC_CLASS_DLL(SotObject,SOTDATA())
						SO2_DECL_INVARIANT()

						// Nur damit die Makros in So3 nicht ganz ausufern
    virtual IUnknown *  GetInterface( const SvGlobalName & );

    BOOL                Owner() const { return bOwner; }
    BOOL                IsSvObject() const;

    // Methoden fuer die Aggregation (siehe OLE2-Spec)
    BOOL                ShouldDelete();
    virtual void        QueryDelete();
    SvAggregateMemberList & GetAggList();
    void                AddInterface( SotObject * );
    void                AddInterface( SotFactory * );
    virtual SotObjectRef CreateAggObj( const SotFactory * );
    void *              AggCast( const SotFactory * pFact );
    void *              CastAndAddRef( const SotFactory * pFact );
    SotObject *         GetMainObj() const;

                        // !!! Read the Manual !!!
    virtual USHORT      FuzzyLock( BOOL bLock, BOOL bIntern, BOOL bClose );
    void                Lock( BOOL bLock )
                        {
                            FuzzyLock( bLock, TRUE, TRUE );
                        }
    USHORT              GetOwnerLockCount() const { return nOwnerLockCount; }
    USHORT              GetStrongLockCount() const { return nStrongLockCount; }

    void                OwnerLock( BOOL bLock );
	void                RemoveOwnerLock();
    BOOL                DoClose();
    BOOL                IsInClose() const { return bInClose; }

private:
    // Kopieren und Zuweisen dieses Objekttyps ist nicht erlaubt
    SOT_DLLPRIVATE SotObject & operator = ( const SotObject & );
    SOT_DLLPRIVATE SotObject( const SotObject & );
};

//==================class SotObjectRef======================================
SV_IMPL_REF(SotObject)

inline SotObjectRef::SotObjectRef( SotObject * pObjP, SvCastEnum )
{
    if( pObjP )
    {
        pObj = (SotObject *)pObjP->AggCast( SotObject::ClassFactory() );
        if( pObj )
            pObj->AddRef();
    }
    else
        pObj = NULL;
}

//==================class SotObject*List====================================
SV_DECL_REF_LIST(SotObject,SotObject*)
SV_IMPL_REF_LIST(SotObject,SotObject*)

#endif // _IFACE_HXX

