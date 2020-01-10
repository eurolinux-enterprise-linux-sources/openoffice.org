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

#ifndef _STDCTRL_HXX
#define _STDCTRL_HXX

#include "svtools/svtdllapi.h"

#ifndef _EDIT_HXX
#include <vcl/edit.hxx>
#endif
#ifndef _FIXED_HXX
#include <vcl/fixed.hxx>
#endif

// -------------
// - FixedInfo -
// -------------

class SVT_DLLPUBLIC FixedInfo : public FixedText
{
public:
    FixedInfo( Window* pParent, WinBits nWinStyle = WB_LEFT );
    FixedInfo( Window* pParent, const ResId& rResId );
};

namespace svt
{
    // ----------------------------
    // - svt::SelectableFixedText -
    // ----------------------------

    class SVT_DLLPUBLIC SelectableFixedText : public Edit
    {
    private:
        void    Init();

    public:
                SelectableFixedText( Window* pParent, WinBits nWinStyle );
                SelectableFixedText( Window* pParent, const ResId& rResId );
        virtual ~SelectableFixedText();

        virtual void    LoseFocus();
    };

} // namespace svt

#endif  // _STDCTRL_HXX

