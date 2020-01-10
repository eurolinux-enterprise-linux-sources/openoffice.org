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

#ifndef FPICKER_WIN32_VISTA_TYPES_HXX
#define FPICKER_WIN32_VISTA_TYPES_HXX

//-----------------------------------------------------------------------------
// includes
//-----------------------------------------------------------------------------

#include "comptr.hxx"
#include <shobjidl.h>

//-----------------------------------------------------------------------------
// namespace
//-----------------------------------------------------------------------------

#ifdef css
    #error "Clash on using CSS as namespace define."
#else
    #define css ::com::sun::star
#endif

namespace fpicker{
namespace win32{
namespace vista{

//-----------------------------------------------------------------------------
// types, const etcpp.
//-----------------------------------------------------------------------------

typedef ComPtr< IFileDialog         , IID_IFileDialog                             > TFileDialog;
typedef ComPtr< IFileOpenDialog     , IID_IFileOpenDialog  , CLSID_FileOpenDialog > TFileOpenDialog;
typedef ComPtr< IFileSaveDialog     , IID_IFileSaveDialog  , CLSID_FileSaveDialog > TFileSaveDialog;
typedef ComPtr< IFileDialogEvents   , IID_IFileDialogEvents                       > TFileDialogEvents;
typedef ComPtr< IFileDialogCustomize, IID_IFileDialogCustomize                    > TFileDialogCustomize;

} // namespace vista
} // namespace win32
} // namespace fpicker

#undef css

#endif // FPICKER_WIN32_VISTA_TYPES_HXX
