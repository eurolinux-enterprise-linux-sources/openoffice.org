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

#include "saldata.hxx"
#include "salinst.h"
#include "DragSource.hxx"
#include "DropTarget.hxx"
#include "aqua_clipboard.hxx"
#include "osl/diagnose.h"

using namespace ::osl;
using namespace ::rtl;
using namespace ::com::sun::star::uno;
using namespace ::cppu;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::datatransfer::clipboard;


Reference< XInterface > AquaSalInstance::CreateClipboard( const Sequence< Any >& i_rArguments )
{
    SalData* pSalData = GetSalData();
    if( ! pSalData->mxClipboard.is() )
        pSalData->mxClipboard = Reference<XInterface>(static_cast< XClipboard* >(new AquaClipboard()), UNO_QUERY);
    return pSalData->mxClipboard;
}


Reference<XInterface> AquaSalInstance::CreateDragSource()
{
    return Reference<XInterface>(static_cast< XInitialization* >(new DragSource()), UNO_QUERY);
}

Reference<XInterface> AquaSalInstance::CreateDropTarget()
{
  return Reference<XInterface>(static_cast< XInitialization* >(new DropTarget()), UNO_QUERY);
}

