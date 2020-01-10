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

#ifndef SVX_WRAPFIELD_HXX
#define SVX_WRAPFIELD_HXX

#include <vcl/field.hxx>
#include "svx/svxdllapi.h"

namespace svx {

// ============================================================================

/** A numeric spin field that wraps around the value on limits.
    @descr  Note: Use type "NumericField" in resources. */
class SVX_DLLPUBLIC WrapField : public NumericField
{
public:
    explicit            WrapField( Window* pParent, WinBits nWinStyle );
    explicit            WrapField( Window* pParent, const ResId& rResId );

protected:
    /** Up event with wrap-around functionality. */
    virtual void        Up();
    /** Down event with wrap-around functionality. */
    virtual void        Down();
};

// ============================================================================

} // namespace svx

#endif

