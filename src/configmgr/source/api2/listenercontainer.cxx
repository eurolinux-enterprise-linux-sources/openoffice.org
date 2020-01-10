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
#include "precompiled_configmgr.hxx"

#include "listenercontainer.hxx"
#include <com/sun/star/lang/XEventListener.hpp>

#include <osl/diagnose.h>

namespace configmgr
{
	namespace configapi
	{
/////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//		class DisposeNotifier
//-----------------------------------------------------------------------------

void DisposeNotifier::appendAndClearContainer(cppu::OInterfaceContainerHelper* pContainer)
{
	if (pContainer)
	{
		{
			cppu::OInterfaceIteratorHelper aIterator(*pContainer);
			while (aIterator.hasMoreElements())
			{
				aListeners.push_back(uno::Reference< lang::XEventListener >::query(aIterator.next()));
			}
		}
		pContainer->clear();
	}
}
//-----------------------------------------------------------------------------
void DisposeNotifier::notify()
{
	for(std::vector< uno::Reference< lang::XEventListener > >::iterator it = aListeners.begin(); it != aListeners.end(); ++it)
	{
		if (it->is())
		{
            try { (*it)->disposing(aEvent); } catch (uno::Exception & ) {}
			it->clear();
		}
	}
	aListeners.clear();
}

//-----------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////////////
	}
}


