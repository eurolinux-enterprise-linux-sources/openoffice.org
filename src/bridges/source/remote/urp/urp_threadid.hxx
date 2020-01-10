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
#ifndef _URP_THREADID_HXX_
#define _URP_THREADID_HXX_

#include <sal/types.h>
#include <rtl/byteseq.hxx>
#include <rtl/string.hxx>

namespace bridges_urp
{

	struct EqualThreadId
	{
		sal_Int32 operator () ( const ::rtl::ByteSequence &a , const ::rtl::ByteSequence &b ) const
			{
				return a == b;
			}
	};

	struct HashThreadId
	{
		sal_Int32 operator () ( const ::rtl::ByteSequence &a  )  const 
			{
				if( a.getLength() >= 4 )
				{
					return *(sal_Int32*) a.getConstArray();
				}
				return 0;
			}
	};

	rtl::OString byteSequence2HumanReadableString( const rtl::ByteSequence &a );
}
#endif
