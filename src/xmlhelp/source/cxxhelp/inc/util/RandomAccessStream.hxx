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
#ifndef _XMLEARCH_UTIL_RANDOMACCESSSTREAM_HXX_
#define _XMLEARCH_UTIL_RANDOMACCESSSTREAM_HXX_

#include <osl/file.hxx>

namespace xmlsearch {
	
	namespace util {
		
		
		class RandomAccessStream
		{
		public:

			virtual ~RandomAccessStream() { };
      
			// The calle is responsible for allocating the buffer
			virtual void seek( sal_Int32 ) = 0;
			virtual sal_Int32 readBytes( sal_Int8*,sal_Int32 ) = 0;
			virtual void writeBytes( sal_Int8*, sal_Int32 ) = 0;
			virtual sal_Int32 length() = 0;
			virtual void close() = 0;
      
      
		protected:
      
			enum OPENFLAG { Read = OpenFlag_Read,
							Write = OpenFlag_Write,
							Create = OpenFlag_Create };
      
		};

		
	}
}

#endif
