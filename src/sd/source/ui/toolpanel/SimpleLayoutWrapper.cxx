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
#include "precompiled_sd.hxx"

#include "SimpleLayoutWrapper.hxx"

#include <vcl/window.hxx>

namespace sd { namespace toolpanel {

SimpleLayoutWrapper::SimpleLayoutWrapper (
    ::std::auto_ptr< ::Window> pWindow,
    int nPreferredWidth,
    int nPreferredHeight,
    bool bIsResizable)
    : mpWindow (pWindow),
      mnPreferredWidth(nPreferredWidth),
      mnPreferredHeight(nPreferredHeight),
      mbIsResizable (bIsResizable)
{
}




SimpleLayoutWrapper::~SimpleLayoutWrapper (void)
{
}




Size SimpleLayoutWrapper::GetPreferredSize (void)
{
    return Size (mnPreferredWidth, mnPreferredHeight);
}




sal_Int32 SimpleLayoutWrapper::GetPreferredWidth (sal_Int32)
{
    return mnPreferredWidth;
}




sal_Int32 SimpleLayoutWrapper::GetPreferredHeight (sal_Int32)
{
    return mnPreferredHeight;
}




bool SimpleLayoutWrapper::IsResizable (void)
{
    return mbIsResizable;
}




::Window* SimpleLayoutWrapper::GetWindow (void)
{
    return mpWindow.get();
}


} } // end of namespace ::sd::toolpanel

