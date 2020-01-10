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
#include "precompiled_svtools.hxx"

#ifndef INCLUDE_CONFIGITEMS_ACCESSIBILITYOPTIONS_CONST_HXX
#define INCLUDE_CONFIGITEMS_ACCESSIBILITYOPTIONS_CONST_HXX

#include <rtl/ustring.hxx>

namespace
{
	static const ::rtl::OUString s_sAccessibility           = ::rtl::OUString::createFromAscii("org.openoffice.Office.Common/Accessibility");
	static const ::rtl::OUString s_sAutoDetectSystemHC      = ::rtl::OUString::createFromAscii("AutoDetectSystemHC");
	static const ::rtl::OUString s_sIsForPagePreviews       = ::rtl::OUString::createFromAscii("IsForPagePreviews");
	static const ::rtl::OUString s_sIsHelpTipsDisappear     = ::rtl::OUString::createFromAscii("IsHelpTipsDisappear");
	static const ::rtl::OUString s_sHelpTipSeconds          = ::rtl::OUString::createFromAscii("HelpTipSeconds");
	static const ::rtl::OUString s_sIsAllowAnimatedGraphics = ::rtl::OUString::createFromAscii("IsAllowAnimatedGraphics");
	static const ::rtl::OUString s_sIsAllowAnimatedText     = ::rtl::OUString::createFromAscii("IsAllowAnimatedText");
	static const ::rtl::OUString s_sIsAutomaticFontColor    = ::rtl::OUString::createFromAscii("IsAutomaticFontColor");
	static const ::rtl::OUString s_sIsSystemFont            = ::rtl::OUString::createFromAscii("IsSystemFont");
	static const ::rtl::OUString s_sIsSelectionInReadonly   = ::rtl::OUString::createFromAscii("IsSelectionInReadonly");
}

#endif //  INCLUDE_CONFIGITEMS_ACCESSIBILITYOPTIONS_CONST_HXX
