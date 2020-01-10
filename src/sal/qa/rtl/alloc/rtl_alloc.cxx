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
#include "precompiled_sal.hxx"
// autogenerated file with codegen.pl

#include <rtl/alloc.h>
#include <cppunit/simpleheader.hxx>

namespace rtl_alloc
{

    // small memory check routine, which return false, if there is a problem

    bool checkMemory(char* _pMemory, sal_uInt32 _nSize, char _n)
    {
        bool bOk = true;
        
        for (sal_uInt32 i=0;i<_nSize;i++)
        {
            if (_pMemory[i] != _n)
            {
                bOk = false;
            }
        }
        return bOk;
    }
    
class Memory : public CppUnit::TestFixture
{
    // for normal alloc functions
    char       *m_pMemory;
    sal_uInt32  m_nSizeOfMemory;

public:
    Memory()
            :m_pMemory(NULL),
             m_nSizeOfMemory(50 * 1024 * 1024)
        {
        }
    
    // initialise your test code values here.
    void setUp()
    {
            t_print("allocate memory\n");
            m_pMemory = (char*) rtl_allocateMemory( m_nSizeOfMemory );
    }

    void tearDown()
    {
            t_print("free memory\n");
            rtl_freeMemory(m_pMemory);
            m_pMemory = NULL;
    }

    // insert your test code here.
    void rtl_allocateMemory_001()
    {
        // this is demonstration code
        // CPPUNIT_ASSERT_MESSAGE("a message", 1 == 1);
        
        CPPUNIT_ASSERT_MESSAGE( "Can get zero memory.", m_pMemory != NULL);
        memset(m_pMemory, 1, m_nSizeOfMemory);
        CPPUNIT_ASSERT_MESSAGE( "memory contains wrong value.", checkMemory(m_pMemory, m_nSizeOfMemory, 1) == true);
    }

    void rtl_reallocateMemory_001()
    {
        t_print("reallocate memory\n");
        sal_uInt32 nSize = 10 * 1024 * 1024;
        m_pMemory = (char*)rtl_reallocateMemory(m_pMemory, nSize);
        
        CPPUNIT_ASSERT_MESSAGE( "Can reallocate memory.", m_pMemory != NULL);
        memset(m_pMemory, 2, nSize);
        CPPUNIT_ASSERT_MESSAGE( "memory contains wrong value.", checkMemory(m_pMemory, nSize, 2) == true);
    }

    // void rtl_freeMemory_001()
    //     {
    //         // CPPUNIT_ASSERT_STUB();
    //     }

    // Change the following lines only, if you add, remove or rename 
    // member functions of the current class, 
    // because these macros are need by auto register mechanism.

    CPPUNIT_TEST_SUITE(Memory);
    CPPUNIT_TEST(rtl_allocateMemory_001);
    CPPUNIT_TEST(rtl_reallocateMemory_001);
    // CPPUNIT_TEST(rtl_freeMemory_001);
    CPPUNIT_TEST_SUITE_END();
}; // class test

class ZeroMemory : public CppUnit::TestFixture
{
    // for zero functions
    char       *m_pZeroMemory;
    sal_uInt32  m_nSizeOfZeroMemory;
    
public:
    ZeroMemory()
            :m_pZeroMemory(NULL),
             m_nSizeOfZeroMemory( 50 * 1024 * 1024 )
        {
        }
    
    // initialise your test code values here.
    void setUp()
        {
            t_print("allocate zero memory\n");
            m_pZeroMemory = (char*) rtl_allocateZeroMemory( m_nSizeOfZeroMemory );
        }

    void tearDown()
    {
            t_print("free zero memory\n");
            rtl_freeZeroMemory(m_pZeroMemory, m_nSizeOfZeroMemory);
            // LLA: no check possible, may GPF if there is something wrong.
            // CPPUNIT_ASSERT_MESSAGE( "Can get zero memory.", pZeroMemory != NULL);
    }

    // insert your test code here.

    void rtl_allocateZeroMemory_001()
    {
            CPPUNIT_ASSERT_MESSAGE( "Can get zero memory.", m_pZeroMemory != NULL);
            CPPUNIT_ASSERT_MESSAGE( "memory contains wrong value.", checkMemory(m_pZeroMemory, m_nSizeOfZeroMemory, 0) == true);

            memset(m_pZeroMemory, 3, m_nSizeOfZeroMemory);
            CPPUNIT_ASSERT_MESSAGE( "memory contains wrong value.", checkMemory(m_pZeroMemory, m_nSizeOfZeroMemory, 3) == true);
    }

    // Change the following lines only, if you add, remove or rename 
    // member functions of the current class, 
    // because these macros are need by auto register mechanism.

    CPPUNIT_TEST_SUITE(ZeroMemory);
    CPPUNIT_TEST(rtl_allocateZeroMemory_001);
    CPPUNIT_TEST_SUITE_END();
}; // class test

// -----------------------------------------------------------------------------
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(rtl_alloc::Memory, "rtl_alloc");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(rtl_alloc::ZeroMemory, "rtl_alloc");
} // namespace rtl_alloc


// -----------------------------------------------------------------------------

// this macro creates an empty function, which will called by the RegisterAllFunctions()
// to let the user the possibility to also register some functions by hand.
NOADDITIONAL;

