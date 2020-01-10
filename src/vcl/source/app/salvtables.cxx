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
#include "precompiled_vcl.hxx"

#include <vcl/salframe.hxx>
#include <vcl/salinst.hxx>
#include <vcl/salvd.hxx>
#include <vcl/salprn.hxx>
#include <vcl/saltimer.hxx>
#include <vcl/salimestatus.hxx>
#include <vcl/salsys.hxx>
#include <vcl/salbmp.hxx>
#include <vcl/salobj.hxx>
#include <vcl/salmenu.hxx>
#include <vcl/salctrlhandle.hxx>

// this file contains the virtual destructors of the sal interface
// compilers ususally put their vtables where the destructor is

SalFrame::~SalFrame()
{
}

// -----------------------------------------------------------------------

// default to full-frame flushes
// on ports where partial-flushes are much cheaper this method should be overridden
void SalFrame::Flush( const Rectangle& )
{
    Flush();
}

// -----------------------------------------------------------------------

SalInstance::~SalInstance()
{
}

void SalInstance::FillFontPathList( std::list< rtl::OString >& )
{
    // do nothing
}

SalTimer::~SalTimer()
{
}

SalBitmap::~SalBitmap()
{
}

SalI18NImeStatus::~SalI18NImeStatus()
{
}

SalSystem::~SalSystem()
{
}

SalPrinter::~SalPrinter()
{
}

BOOL SalPrinter::StartJob( const String*, const String&,
                           ImplJobSetup*, ImplQPrinter* )
{
    return FALSE;
}

SalInfoPrinter::~SalInfoPrinter()
{
}

SalVirtualDevice::~SalVirtualDevice()
{
}

SalObject::~SalObject()
{
}

SalMenu::~SalMenu()
{
}

bool SalMenu::ShowNativePopupMenu(FloatingWindow *, const Rectangle&, ULONG )
{
    return false;
}

bool SalMenu::AddMenuBarButton( const SalMenuButtonItem& )
{
    return false;
}

void SalMenu::RemoveMenuBarButton( USHORT )
{
}

Rectangle SalMenu::GetMenuBarButtonRectPixel( USHORT, SalFrame* )
{
    return Rectangle();
}

SalMenuItem::~SalMenuItem()
{
}
SalControlHandle::~SalControlHandle()
{
}

