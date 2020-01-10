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
#include "precompiled_canvas.hxx"

#include "cairo_cairo.hxx"

#ifdef WNT
# include <tools/prewin.h>
# include <windows.h> 
# include <tools/postwin.h>
#endif

#include <vcl/sysdata.hxx>
#include <vcl/syschild.hxx>

namespace cairo
{
/****************************************************************************************
 * Platform independent part of surface backends for OpenOffice.org Cairo Canvas        *
 * For the rest of the functions (and the platform-specific derived                     *  
 *  Surface classes), see platform specific cairo_<platform>_cairo.cxx                  *
 ****************************************************************************************/

    const SystemEnvData* GetSysData(const Window *pOutputWindow)
    {
        const SystemEnvData* pSysData = NULL;
        // check whether we're a SysChild: have to fetch system data
        // directly from SystemChildWindow, because the GetSystemData
        // method is unfortunately not virtual
        const SystemChildWindow* pSysChild = dynamic_cast< const SystemChildWindow* >( pOutputWindow );
        if( pSysChild )
            pSysData = pSysChild->GetSystemData();
        else
            pSysData = pOutputWindow->GetSystemData();
        return pSysData;
    }
}
