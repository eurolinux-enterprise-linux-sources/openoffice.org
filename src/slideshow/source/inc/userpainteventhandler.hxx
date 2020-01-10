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

#ifndef INCLUDED_SLIDESHOW_USERPAINTEVENTHANDLER_HXX
#define INCLUDED_SLIDESHOW_USERPAINTEVENTHANDLER_HXX

#include <boost/shared_ptr.hpp>

/* Definition of UserPaintEventHandler interface */

namespace slideshow
{
    namespace internal
    {
        class RGBColor;
        
        /** Interface for handling user paint state changes.

        	Classes implementing this interface can be added to an
        	EventMultiplexer object, and are called from there to
        	handle user paint events.
         */
        class UserPaintEventHandler
        {
        public:
            virtual ~UserPaintEventHandler() {}

            virtual bool colorChanged( RGBColor const& rUserColor ) = 0;
            virtual bool disable() = 0;
        };

        typedef ::boost::shared_ptr< UserPaintEventHandler > UserPaintEventHandlerSharedPtr;

    }
}

#endif /* INCLUDED_SLIDESHOW_USERPAINTEVENTHANDLER_HXX */
