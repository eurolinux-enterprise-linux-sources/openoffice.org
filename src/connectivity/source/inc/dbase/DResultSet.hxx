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
#ifndef _CONNECTIVITY_DBASE_DRESULTSET_HXX_
#define _CONNECTIVITY_DBASE_DRESULTSET_HXX_

#include "file/FResultSet.hxx"
#include <com/sun/star/sdbcx/XRowLocate.hpp>
#include <com/sun/star/sdbcx/XDeleteRows.hpp>
#include <cppuhelper/implbase2.hxx>

namespace connectivity
{
	namespace dbase
	{
		class ODbaseResultSet;
		// these typedef's are only necessary for the compiler
		typedef ::cppu::ImplHelper2<  ::com::sun::star::sdbcx::XRowLocate,
                                      ::com::sun::star::sdbcx::XDeleteRows> ODbaseResultSet_BASE;
		typedef file::OResultSet											ODbaseResultSet_BASE2;
		typedef ::comphelper::OPropertyArrayUsageHelper<ODbaseResultSet>	ODbaseResultSet_BASE3;


		class ODbaseResultSet : public ODbaseResultSet_BASE2,
								public ODbaseResultSet_BASE,
								public ODbaseResultSet_BASE3
		{
			sal_Bool m_bBookmarkable;
		protected:
			// OPropertyArrayUsageHelper
			virtual ::cppu::IPropertyArrayHelper* createArrayHelper() const;
			// OPropertySetHelper
			virtual ::cppu::IPropertyArrayHelper & SAL_CALL getInfoHelper();
			virtual sal_Bool fillIndexValues(const ::com::sun::star::uno::Reference< ::com::sun::star::sdbcx::XColumnsSupplier> &_xIndex);
			virtual file::OSQLAnalyzer* createAnalyzer();
		public:
			DECLARE_SERVICE_INFO();

			ODbaseResultSet( file::OStatement_Base* pStmt,connectivity::OSQLParseTreeIterator&	_aSQLIterator);

			// XInterface
            virtual ::com::sun::star::uno::Any SAL_CALL queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException);
			virtual void SAL_CALL acquire() throw();
            virtual void SAL_CALL release() throw();
			//XTypeProvider
            virtual ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Type > SAL_CALL getTypes(  ) throw(::com::sun::star::uno::RuntimeException);
			// XPropertySet
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) throw(::com::sun::star::uno::RuntimeException);

			// XRowLocate
            virtual ::com::sun::star::uno::Any SAL_CALL getBookmark(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL moveToBookmark( const ::com::sun::star::uno::Any& bookmark ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL moveRelativeToBookmark( const ::com::sun::star::uno::Any& bookmark, sal_Int32 rows ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Int32 SAL_CALL compareBookmarks( const ::com::sun::star::uno::Any& first, const ::com::sun::star::uno::Any& second ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Bool SAL_CALL hasOrderedBookmarks(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
            virtual sal_Int32 SAL_CALL hashBookmark( const ::com::sun::star::uno::Any& bookmark ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);
			// XDeleteRows
            virtual ::com::sun::star::uno::Sequence< sal_Int32 > SAL_CALL deleteRows( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& rows ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException);

			// own methods
			sal_Int32 getCurrentFilePos() const;

		};
	}
}
#endif //_CONNECTIVITY_DBASE_DRESULTSET_HXX_

