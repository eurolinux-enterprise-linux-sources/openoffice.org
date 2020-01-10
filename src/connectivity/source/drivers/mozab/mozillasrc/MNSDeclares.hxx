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

#ifndef _CONNECTIVITY_MAB_NS_DECLARES_HXX_
#define _CONNECTIVITY_MAB_NS_DECLARES_HXX_ 

#include <sal/types.h>


const	sal_Int32 RowStates_Normal = 0;
const	sal_Int32 RowStates_Inserted = 1;
const	sal_Int32 RowStates_Updated = 2;
const	sal_Int32 RowStates_Deleted  = 4;
const	sal_Int32 RowStates_Error  = 32;

namespace connectivity{
	namespace mozab{
        class OConnection;
	}
}
sal_Bool isProfileLocked(connectivity::mozab::OConnection* _pCon);

class nsIAbDirectory;
sal_Int32 getDirectoryType(const nsIAbDirectory*  directory);


#endif // _CONNECTIVITY_MAB_NS_DECLARES_HXX_ 1

