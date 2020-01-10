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

#ifndef _SD_CUSTOMANIMATION_HXX_
#define _SD_CUSTOMANIMATION_HXX_

#ifndef _COM_SUN_STAR_FRAME_XModel_HPP_
#include <com/sun/star/frame/XModel.hpp>
#endif

class Window;

namespace sd {

//	extern void showCustomAnimationPane( const ::com::sun::star::uno::Reference< ::com::sun::star::drawing::XDrawView >& xView );
	extern ::Window* createCustomAnimationPane( ::Window* pParent, const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModel >& xModel );
}

#endif // _SD_CUSTOMANIMATION_HXX_
