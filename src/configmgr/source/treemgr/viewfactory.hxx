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

#ifndef CONFIGMGR_VIEWBEHAVIORFACTORY_HXX_
#define CONFIGMGR_VIEWBEHAVIORFACTORY_HXX_

namespace configmgr
{
//-----------------------------------------------------------------------------
    namespace data { class TreeSegment; }

	namespace view 
	{ 
	// Different standard (static) strategies
	//---------------------------------------------------------------------
		/// provides a factory for read-only node implementations
		rtl::Reference<ViewStrategy> createReadOnlyStrategy();
		/// provides a factory for nodes that cache changes temporarily
		rtl::Reference<ViewStrategy> createDeferredChangeStrategy();
		/// provides a factory for immediately commiting node implementations
        rtl::Reference<ViewStrategy> createDirectAccessStrategy(rtl::Reference< data::TreeSegment > const & _aTreeSegment);
	//---------------------------------------------------------------------
	}

//-----------------------------------------------------------------------------

}

#endif // CONFIGMGR_VIEWBEHAVIORFACTORY_HXX_
