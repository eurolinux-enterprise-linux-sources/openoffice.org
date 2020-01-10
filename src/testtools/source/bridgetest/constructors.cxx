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
#include "precompiled_testtools.hxx"

#include "sal/config.h"

#include "com/sun/star/lang/XInitialization.hpp"
#include "com/sun/star/lang/XComponent.hpp"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Exception.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "com/sun/star/uno/XComponentContext.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "cppu/unotype.hxx"
#include "cppuhelper/factory.hxx"
#include "cppuhelper/implbase1.hxx"
#include "cppuhelper/implbase2.hxx"
#include "cppuhelper/implementationentry.hxx"
#include "cppuhelper/weak.hxx"
#include "rtl/string.h"
#include "rtl/ustring.h"
#include "rtl/ustring.hxx"
#include "sal/types.h"
#include "test/testtools/bridgetest/BadConstructorArguments.hpp"
#include "test/testtools/bridgetest/TestEnum.hpp"
#include "test/testtools/bridgetest/TestStruct.hpp"
#include "test/testtools/bridgetest/TestPolyStruct.hpp"
#include "test/testtools/bridgetest/TestPolyStruct2.hpp"
#include "test/testtools/bridgetest/XMultiBase1.hpp"
#include "uno/lbnames.h"

namespace {

namespace css = ::com::sun::star;
namespace ttb = ::test::testtools::bridgetest;
class Impl: public ::cppu::WeakImplHelper1< css::lang::XInitialization > {
public:
    Impl() {}

private:
    Impl(Impl &); // not defined
    void operator =(Impl &); // not defined

    virtual ~Impl() {}

    virtual void SAL_CALL initialize(
        css::uno::Sequence< css::uno::Any > const & arguments)
        throw (css::uno::Exception);
};

void Impl::initialize(css::uno::Sequence< css::uno::Any > const & arguments)
    throw (css::uno::Exception)
{
    ::sal_Bool arg0 = sal_Bool();
    ::sal_Int8 arg1 = sal_Int8();
    ::sal_Int16 arg2 = sal_Int16();
    ::sal_uInt16 arg3 = sal_uInt16();
    ::sal_Int32 arg4 = sal_Int32();
    ::sal_uInt32 arg5 = sal_uInt32();
    ::sal_Int64 arg6 = sal_Int64();
    ::sal_uInt64 arg7 = sal_uInt64();
    float arg8 = float();
    double arg9 = double();
    ::rtl::OUString arg11;
    css::uno::Type arg12;
    ::sal_Bool arg13 = sal_Bool();
    css::uno::Sequence< ::sal_Bool > arg14;
    css::uno::Sequence< ::sal_Int8 > arg15;
    css::uno::Sequence< ::sal_Int16 > arg16;
    css::uno::Sequence< ::sal_uInt16 > arg17;
    css::uno::Sequence< ::sal_Int32 > arg18;
    css::uno::Sequence< ::sal_uInt32 > arg19;
    css::uno::Sequence< ::sal_Int64 > arg20;
    css::uno::Sequence< ::sal_uInt64 > arg21;
    css::uno::Sequence< float > arg22;
    css::uno::Sequence< double > arg23;
    css::uno::Sequence< ::rtl::OUString > arg25;
    css::uno::Sequence< css::uno::Type > arg26;
    css::uno::Sequence< css::uno::Any > arg27;
    ::sal_Bool arg27b = sal_Bool();
    css::uno::Sequence< css::uno::Sequence< ::sal_Bool > > arg28;
    css::uno::Sequence< css::uno::Sequence< css::uno::Any > > arg29;
    ::sal_Bool arg29b = sal_Bool();
    css::uno::Sequence< ::test::testtools::bridgetest::TestEnum > arg30;
    css::uno::Sequence< ::test::testtools::bridgetest::TestStruct > arg31;
    css::uno::Sequence<
        ::test::testtools::bridgetest::TestPolyStruct< sal_Bool > > arg32;
    css::uno::Sequence<
        ::test::testtools::bridgetest::TestPolyStruct< css::uno::Any > > arg33;
    ::sal_Bool arg33b = sal_Bool();
    css::uno::Sequence< css::uno::Reference< css::uno::XInterface > > arg34;
    ::test::testtools::bridgetest::TestEnum arg35;
    ::test::testtools::bridgetest::TestStruct arg36;
    ::test::testtools::bridgetest::TestPolyStruct< ::sal_Bool > arg37;
    ::test::testtools::bridgetest::TestPolyStruct< css::uno::Any > arg38;
    ::sal_Bool arg38b = sal_Bool();
    css::uno::Reference< css::uno::XInterface > arg39;
    if (!(arguments.getLength() == 40
          && (arguments[0] >>= arg0) && arg0
          && (arguments[1] >>= arg1) && arg1 == SAL_MIN_INT8
          && (arguments[2] >>= arg2) && arg2 == SAL_MIN_INT16
          && (arguments[3].getValueType()
              == ::cppu::UnoType< ::cppu::UnoUnsignedShortType >::get())
          && (arguments[3] >>= arg3) && arg3 == SAL_MAX_UINT16
          && (arguments[4] >>= arg4) && arg4 == SAL_MIN_INT32
          && (arguments[5] >>= arg5) && arg5 == SAL_MAX_UINT32
          && (arguments[6] >>= arg6) && arg6 == SAL_MIN_INT64
          && (arguments[7] >>= arg7) && arg7 == SAL_MAX_UINT64
          && (arguments[8] >>= arg8) && arg8 == 0.123f
          && (arguments[9] >>= arg9) && arg9 == 0.456
          && (arguments[10].getValueType()
              == ::cppu::UnoType< ::cppu::UnoCharType >::get())
          && (*static_cast< ::sal_Unicode const * >(arguments[10].getValue())
              == 'X')
          && (arguments[11] >>= arg11)
          && arg11.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && (arguments[12] >>= arg12)
          && arg12 == ::cppu::UnoType< css::uno::Any >::get()
          && (arguments[13] >>= arg13) && arg13
          && (arguments[14] >>= arg14) && arg14.getLength() == 1 && arg14[0]
          && (arguments[15] >>= arg15) && arg15.getLength() == 1
          && arg15[0] == SAL_MIN_INT8
          && (arguments[16] >>= arg16) && arg16.getLength() == 1
          && arg16[0] == SAL_MIN_INT16
          && (arguments[17].getValueType()
              == ::cppu::UnoType<
                  ::cppu::UnoSequenceType <
                      ::cppu::UnoUnsignedShortType > >::get())
          && (arguments[17] >>= arg17) && arg17.getLength() == 1
          && arg17[0] == SAL_MAX_UINT16
          && (arguments[18] >>= arg18) && arg18.getLength() == 1
          && arg18[0] == SAL_MIN_INT32
          && (arguments[19] >>= arg19) && arg19.getLength() == 1
          && arg19[0] == SAL_MAX_UINT32
          && (arguments[20] >>= arg20) && arg20.getLength() == 1
          && arg20[0] == SAL_MIN_INT64
          && (arguments[21] >>= arg21) && arg21.getLength() == 1
          && arg21[0] == SAL_MAX_UINT64
          && (arguments[22] >>= arg22) && arg22.getLength() == 1
          && arg22[0] == 0.123f
          && (arguments[23] >>= arg23) && arg23.getLength() == 1
          && arg23[0] == 0.456
          && (arguments[24].getValueType()
              == ::cppu::UnoType<
                  ::cppu::UnoSequenceType< ::cppu::UnoCharType > >::get())
          && (static_cast< css::uno::Sequence< ::sal_Unicode > const * >(
                  arguments[24].getValue())->getLength()
              == 1)
          && ((*static_cast< css::uno::Sequence< ::sal_Unicode > const * >(
                   arguments[24].getValue()))[0]
              == 'X')
          && (arguments[25] >>= arg25) && arg25.getLength() == 1
          && arg25[0].equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && (arguments[26] >>= arg26) && arg26.getLength() == 1
          && arg26[0] == ::cppu::UnoType< css::uno::Any >::get()
          && (arguments[27] >>= arg27) && arg27.getLength() == 1
          && (arg27[0] >>= arg27b) && arg27b
          && (arguments[28] >>= arg28) && arg28.getLength() == 1
          && arg28[0].getLength() == 1 && arg28[0][0]
          && (arguments[29] >>= arg29) && arg29.getLength() == 1
          && arg29[0].getLength() == 1 && (arg29[0][0] >>= arg29b) && arg29b
          && (arguments[30] >>= arg30) && arg30.getLength() == 1
          && arg30[0] == ::test::testtools::bridgetest::TestEnum_TWO
          && (arguments[31] >>= arg31) && arg31.getLength() == 1
          && arg31[0].member == 10
          && (arguments[32] >>= arg32) && arg32.getLength() == 1
          && arg32[0].member
          && (arguments[33] >>= arg33) && arg33.getLength() == 1
          && (arg33[0].member >>= arg33b) && arg33b
          && (arguments[34] >>= arg34) && arg34.getLength() == 1
          && !arg34[0].is()
          && (arguments[35] >>= arg35)
          && arg35 == ::test::testtools::bridgetest::TestEnum_TWO
          && (arguments[36] >>= arg36) && arg36.member == 10
          && (arguments[37] >>= arg37) && arg37.member
          && (arguments[38] >>= arg38) && (arg38.member >>= arg38b) && arg38b
          && (arguments[39] >>= arg39) && !arg39.is()))
    {
        throw ::test::testtools::bridgetest::BadConstructorArguments();
    }
}


class Impl2: public ::cppu::WeakImplHelper2<
    css::lang::XInitialization, ttb::XMultiBase1 >
{
public:
    Impl2(): m_attr1(0.0) {}

private:
    Impl2(Impl &); // not defined
    void operator =(Impl &); // not defined

    virtual ~Impl2() {}

    virtual void SAL_CALL initialize(
        css::uno::Sequence< css::uno::Any > const & arguments)
        throw (css::uno::Exception);

    //XMultiBase1
    virtual double SAL_CALL getatt1()
        throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setatt1( double _att1 )
        throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Int32 SAL_CALL fn11( ::sal_Int32 arg )
        throw (::com::sun::star::uno::RuntimeException);
    virtual ::rtl::OUString SAL_CALL fn12( const ::rtl::OUString& arg )
        throw (::com::sun::star::uno::RuntimeException);


    double m_attr1;
};

void Impl2::initialize(css::uno::Sequence< css::uno::Any > const & arguments)
    throw (css::uno::Exception)
{
    ttb::TestPolyStruct< css::uno::Type > arg0;
    ttb::TestPolyStruct< css::uno::Any > arg1;
    css::uno::Any arg1b(sal_True);
    ttb::TestPolyStruct< ::sal_Bool > arg2;
    ttb::TestPolyStruct< ::sal_Int8 > arg3;
    ttb::TestPolyStruct< ::sal_Int16 > arg4;
    ttb::TestPolyStruct< ::sal_Int32 > arg5;
    ttb::TestPolyStruct< ::sal_Int64 > arg6;
    ttb::TestPolyStruct< ::sal_Unicode > arg7;
    ttb::TestPolyStruct< ::rtl::OUString > arg8;
    ttb::TestPolyStruct< float > arg9;
    ttb::TestPolyStruct< double > arg10;
    ttb::TestPolyStruct<css::uno::Reference<css::uno::XInterface> > arg11;
    ttb::TestPolyStruct<css::uno::Reference< css::lang::XComponent> > arg12;
    ttb::TestPolyStruct<ttb::TestEnum>  arg13;
    ttb::TestPolyStruct<
          ttb::TestPolyStruct2<sal_Unicode, css::uno::Any> > arg14;
    ttb::TestPolyStruct< ttb::TestPolyStruct2 <
        ttb::TestPolyStruct2< sal_Unicode, css::uno::Any >,
        ::rtl::OUString > > arg15;
    ttb::TestPolyStruct2< ::rtl::OUString,ttb::TestPolyStruct2 <
        ::sal_Unicode, ttb::TestPolyStruct < css::uno::Any > > > arg16;
    ttb::TestPolyStruct2< ttb::TestPolyStruct2<sal_Unicode, css::uno::Any >,
        ttb::TestPolyStruct<sal_Unicode> > arg17;
    ttb::TestPolyStruct<css::uno::Sequence< css::uno::Type > > arg18;
    ttb::TestPolyStruct<css::uno::Sequence<css::uno::Any> > arg19;
    ttb::TestPolyStruct<css::uno::Sequence< ::sal_Bool > > arg20;
    ttb::TestPolyStruct<css::uno::Sequence< ::sal_Int8 > > arg21;
    ttb::TestPolyStruct<css::uno::Sequence< ::sal_Int16 > > arg22;
    ttb::TestPolyStruct<css::uno::Sequence< ::sal_Int32 > > arg23;
    ttb::TestPolyStruct<css::uno::Sequence< ::sal_Int64 > > arg24;
//    ttb::TestPolyStruct<css::uno::Sequence<::sal_Unicode> > arg25;
    ttb::TestPolyStruct<css::uno::Sequence< ::rtl::OUString > > arg26;
    ttb::TestPolyStruct<css::uno::Sequence<float> > arg27;
    ttb::TestPolyStruct<css::uno::Sequence<double> > arg28;
    ttb::TestPolyStruct<css::uno::Sequence<css::uno::Reference<css::uno::XInterface> > > arg29;
    ttb::TestPolyStruct<css::uno::Sequence<css::uno::Reference<css::lang::XComponent> > > arg30;
    ttb::TestPolyStruct<css::uno::Sequence<ttb::TestEnum> >  arg31;
    ttb::TestPolyStruct<css::uno::Sequence<ttb::TestPolyStruct2<
        ::sal_Unicode, css::uno::Sequence<css::uno::Any> > > > arg32;
    bool arg32a = false;
    ttb::TestPolyStruct<css::uno::Sequence<ttb::TestPolyStruct2<
        ttb::TestPolyStruct< ::sal_Unicode>, css::uno::Sequence<css::uno::Any> > > > arg33;
    ttb::TestPolyStruct<css::uno::Sequence<css::uno::Sequence< ::sal_Int32> > > arg34;
    css::uno::Sequence<ttb::TestPolyStruct< ::sal_Int32> > arg35;
    css::uno::Sequence<ttb::TestPolyStruct<ttb::TestPolyStruct2< ::sal_Unicode, css::uno::Any> > > arg36;
    css::uno::Sequence<ttb::TestPolyStruct<ttb::TestPolyStruct2<
        ttb::TestPolyStruct2< ::sal_Unicode,css::uno::Any >, ::rtl::OUString> > > arg37;        
    css::uno::Sequence<ttb::TestPolyStruct2< ::rtl::OUString, ttb::TestPolyStruct2<
        ::sal_Unicode, ttb::TestPolyStruct<css::uno::Any> > > > arg38;
    css::uno::Sequence<ttb::TestPolyStruct2<ttb::TestPolyStruct2<
        ::sal_Unicode, css::uno::Any>, ttb::TestPolyStruct< ::sal_Unicode> > > arg39;
    css::uno::Sequence<css::uno::Sequence<ttb::TestPolyStruct< ::sal_Unicode> > > arg40;
    css::uno::Sequence<css::uno::Sequence<ttb::TestPolyStruct<ttb::TestPolyStruct2< ::sal_Unicode, css::uno::Any> > > >arg41;                
    css::uno::Sequence<css::uno::Sequence<ttb::TestPolyStruct<
        ttb::TestPolyStruct2<ttb::TestPolyStruct2<sal_Unicode, css::uno::Any>, ::rtl::OUString> > > > arg42;
    css::uno::Sequence<css::uno::Sequence<ttb::TestPolyStruct2<
        ::rtl::OUString, ttb::TestPolyStruct2< ::sal_Unicode, ttb::TestPolyStruct<css::uno::Any> > > > > arg43;
    css::uno::Sequence<css::uno::Sequence<ttb::TestPolyStruct2<
        ttb::TestPolyStruct2< ::sal_Unicode, css::uno::Any>, ttb::TestPolyStruct<
        ::sal_Unicode> > > > arg44;

    if (!(arguments.getLength() == 45
          && (arguments[0] >>= arg0) && arg0.member == cppu::UnoType<sal_Int32>::get()
          && (arguments[1] >>= arg1) && arg1.member == arg1b 
          && (arguments[2] >>= arg2) && arg2.member == sal_True
          && (arguments[3] >>= arg3) && arg3.member == SAL_MIN_INT8
          && (arguments[4] >>= arg4) && arg4.member == SAL_MIN_INT16
          && (arguments[5] >>= arg5) && arg5.member == SAL_MIN_INT32
          && (arguments[6] >>= arg6) && arg6.member == SAL_MIN_INT64
          && (arguments[7] >>= arg7) && arg7.member == 'X'
          && (arguments[8] >>= arg8) && arg8.member.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && (arguments[9] >>= arg9) && arg9.member == 0.123f
          && (arguments[10] >>= arg10) && arg10.member == 0.456
          && (arguments[11] >>= arg11)
          && (arguments[12] >>= arg12)
          && (arguments[13] >>= arg13) && arg13.member == ttb::TestEnum_TWO
          && (arguments[14] >>= arg14) && arg14.member.member1 == 'X' && arg14.member.member2 == arg1b
          && (arguments[15] >>= arg15) && arg15.member.member1.member1 == 'X'
          && arg15.member.member1.member2 == arg1b
          && arg15.member.member2.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && (arguments[16] >>= arg16) && arg16.member1.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && arg16.member2.member1 == 'X'
          && arg16.member2.member2.member == arg1b
          && (arguments[17] >>= arg17) && arg17.member1.member1 == 'X'
          && arg17.member1.member2 == arg1b
          && arg17.member2.member == 'X'
          && (arguments[18] >>= arg18)  && arg18.member.getLength() == 1
          && arg18.member[0] == ::cppu::UnoType<sal_Int32>::get()
          && (arguments[19] >>= arg19) && arg19.member.getLength() == 1
          && arg19.member[0] == arg1b
          && (arguments[20] >>= arg20) && arg20.member.getLength() == 1
          && arg20.member[0] == sal_True
          && (arguments[21] >>= arg21) && arg21.member.getLength() == 1
          && arg21.member[0] == SAL_MIN_INT8
          && (arguments[22] >>= arg22) && arg22.member.getLength() == 1
          && arg22.member[0] == SAL_MIN_INT16
          && (arguments[23] >>= arg23) && arg23.member.getLength() == 1
          && arg23.member[0] == SAL_MIN_INT32
          && (arguments[24] >>= arg24) && arg24.member.getLength() == 1
          && arg24.member[0] == SAL_MIN_INT64
          && (arguments[25].getValueType()
               == ::cppu::UnoType< ttb::TestPolyStruct< ::cppu::UnoSequenceType<
              ::cppu::UnoCharType> > >::get())
          && (static_cast< ttb::TestPolyStruct<css::uno::Sequence<
              ::sal_Unicode> > const *>(arguments[25].getValue())->member[0]) == 'X'
          && (arguments[26] >>= arg26) && arg26.member.getLength() == 1
          && arg26.member[0].equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && (arguments[27] >>= arg27) && arg27.member.getLength() == 1
          && arg27.member[0] == 0.123f
          && (arguments[28] >>= arg28) && arg28.member.getLength() == 1
          && arg28.member[0] == 0.456
          && (arguments[29] >>= arg29) && arg29.member.getLength() == 1
          && arg29.member[0] != NULL
          && (arguments[30] >>= arg30) && arg30.member.getLength() == 1
          && arg30.member[0] != NULL
          && (arguments[31] >>= arg31) && arg31.member.getLength() == 1
          && arg31.member[0] == ttb::TestEnum_TWO
          && (arguments[32] >>= arg32) && arg32.member.getLength() == 1
          && arg32.member[0].member1 == 'X'
          && arg32.member[0].member2.getLength() == 1
          && (arg32.member[0].member2[0] >>= arg32a) && arg32a == true
          && (arguments[33] >>= arg33) && arg33.member.getLength() == 1
          && arg33.member[0].member1.member == 'X'
          && arg33.member[0].member2.getLength() == 1
          && arg33.member[0].member2[0] == arg1b
          && (arguments[34] >>= arg34) && arg34.member.getLength() == 1
          && arg34.member[0].getLength() == 1 && arg34.member[0][0] == SAL_MIN_INT32
          && (arguments[35] >>= arg35) && arg35.getLength() == 1
          && arg35[0].member == SAL_MIN_INT32
          && (arguments[36] >>= arg36) && arg36.getLength() == 1
          && arg36[0].member.member1 == 'X'
          && arg36[0].member.member2 == arg1b
          && (arguments[37] >>= arg37) && arg37.getLength() == 1
          && arg37[0].member.member1.member1 == 'X'
          && arg37[0].member.member1.member2 == arg1b
          && arg37[0].member.member2.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && (arguments[38] >>= arg38) && arg38.getLength() == 1
          && arg38[0].member1.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && arg38[0].member2.member1 == 'X'
          && arg38[0].member2.member2.member == arg1b
          && (arguments[39] >>= arg39) && arg39.getLength() == 1
          && arg39[0].member1.member1 == 'X'
          && arg39[0].member1.member2 == arg1b
          && arg39[0].member2.member == 'X'
          && (arguments[40] >>= arg40) && arg40.getLength() == 1
          && arg40[0].getLength() == 1
          && arg40[0][0].member == 'X'
          && (arguments[41] >>= arg41) && arg41.getLength() == 1
          && arg41[0].getLength() == 1
          && arg41[0][0].member.member1 == 'X'
          && arg41[0][0].member.member2 == arg1b
          && (arguments[42] >>= arg42) && arg42.getLength() == 1
          && arg42[0].getLength() == 1
          && arg42[0][0].member.member1.member1 == 'X'
          && arg42[0][0].member.member1.member2 == arg1b
          && arg42[0][0].member.member2.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && (arguments[43] >>= arg43) && arg43.getLength() == 1
          && arg43[0].getLength() == 1
          && arg43[0][0].member1.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM("test"))
          && arg43[0][0].member2.member1 == 'X'
          && arg43[0][0].member2.member2.member == arg1b
          && (arguments[44] >>= arg44) && arg44.getLength() == 1
          && arg44[0].getLength() == 1
          && arg44[0][0].member1.member1 == 'X'
          && arg44[0][0].member1.member2 == arg1b
          && arg44[0][0].member2.member == 'X'
            ))
    {
        throw ::test::testtools::bridgetest::BadConstructorArguments();
    }

    //check if interfaces work
    arg11.member->acquire();
    arg11.member->release();
    arg12.member->acquire();
    arg12.member->release();
    arg29.member[0]->acquire();
    arg29.member[0]->release();

}

//XMultiBase1
double Impl2::getatt1()
        throw (::com::sun::star::uno::RuntimeException)
{
    return m_attr1;
}

void Impl2::setatt1( double _att1 )throw (::com::sun::star::uno::RuntimeException)
{
    m_attr1 = _att1;
}
 
::sal_Int32 Impl2::fn11( ::sal_Int32 arg )
        throw (::com::sun::star::uno::RuntimeException)
{
    return 11 * arg;
}

::rtl::OUString Impl2::fn12( const ::rtl::OUString& arg )
        throw (::com::sun::star::uno::RuntimeException)
{
    return ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("12")) + arg;
}

css::uno::Reference< css::uno::XInterface > SAL_CALL create(
    css::uno::Reference< css::uno::XComponentContext > const &)
    SAL_THROW((css::uno::Exception))
{
    return static_cast< ::cppu::OWeakObject * >(new Impl);
}

::rtl::OUString SAL_CALL getImplementationName() {
    return ::rtl::OUString(
        RTL_CONSTASCII_USTRINGPARAM(
            "comp.test.testtools.bridgetest.Constructors"));
}

css::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames() {
    css::uno::Sequence< ::rtl::OUString > s(1);
    s[0] = ::rtl::OUString(
        RTL_CONSTASCII_USTRINGPARAM("test.testtools.bridgetest.Constructors"));
    return s;
}

css::uno::Reference< css::uno::XInterface > SAL_CALL create2(
    css::uno::Reference< css::uno::XComponentContext > const &)
    SAL_THROW((css::uno::Exception))
{
    return static_cast< ::cppu::OWeakObject * >(new Impl2);
}

::rtl::OUString SAL_CALL getImplementationName2() {
    return ::rtl::OUString(
        RTL_CONSTASCII_USTRINGPARAM(
            "comp.test.testtools.bridgetest.Constructors2"));
}

css::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames2() {
    css::uno::Sequence< ::rtl::OUString > s(1);
    s[0] = ::rtl::OUString(
        RTL_CONSTASCII_USTRINGPARAM("test.testtools.bridgetest.Constructors2"));
    return s;
}

::cppu::ImplementationEntry entries[] = {
    { &create, &getImplementationName, &getSupportedServiceNames,
      &::cppu::createSingleComponentFactory, 0, 0 },
    { &create2, &getImplementationName2, &getSupportedServiceNames2,
      &::cppu::createSingleComponentFactory, 0, 0 },
    { 0, 0, 0, 0, 0, 0 } };
}

extern "C" void * SAL_CALL component_getFactory(
    char const * implName, void * serviceManager, void * registryKey)
{
    return ::cppu::component_getFactoryHelper(
        implName, serviceManager, registryKey, entries);
}

extern "C" void SAL_CALL component_getImplementationEnvironment(
    char const ** envTypeName, uno_Environment **)
{
    *envTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

extern "C" ::sal_Bool SAL_CALL component_writeInfo(
    void * serviceManager, void * registryKey)
{
    return ::cppu::component_writeInfoHelper(
        serviceManager, registryKey, entries);
}
