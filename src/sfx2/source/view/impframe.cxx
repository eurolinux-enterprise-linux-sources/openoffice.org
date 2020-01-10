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
#include "precompiled_sfx2.hxx"

#ifndef GCC
#endif

#include "impframe.hxx"

#include <svtools/smplhint.hxx>

#include <sfx2/frame.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/viewfrm.hxx>

void SfxFrame_Impl::Notify( SfxBroadcaster& /*rBC*/, const SfxHint& rHint )
{
	SfxSimpleHint* pHint = PTR_CAST( SfxSimpleHint, &rHint );
    if( pHint && pHint->GetId() == SFX_HINT_CANCELLABLE && pCurrentViewFrame )
	{
		// vom Cancel-Manager
        SfxBindings &rBind = pCurrentViewFrame->GetBindings();
		rBind.Invalidate( SID_BROWSE_STOP );
		if ( !rBind.IsInRegistrations() )
			rBind.Update( SID_BROWSE_STOP );
		rBind.Invalidate( SID_BROWSE_STOP );
	}
}

