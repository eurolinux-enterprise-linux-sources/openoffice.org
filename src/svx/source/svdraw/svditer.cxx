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
#include "precompiled_svx.hxx"
#include "svditer.hxx"
#include <svx/svdpage.hxx>
#include <svx/svdogrp.hxx>
#include <svx/svdobj.hxx>
#include <svx/svdmark.hxx>

// #99190#
#include <svx/scene3d.hxx>

SdrObjListIter::SdrObjListIter(const SdrObjList& rObjList, SdrIterMode eMode, BOOL bReverse)
:	maObjList(1024, 64, 64),
	mnIndex(0L),
	mbReverse(bReverse)
{
	ImpProcessObjectList(rObjList, eMode, TRUE);
	Reset();
}

SdrObjListIter::SdrObjListIter(const SdrObjList& rObjList, BOOL bUseZOrder, SdrIterMode eMode, BOOL bReverse)
:	maObjList(1024, 64, 64),
	mnIndex(0L),
	mbReverse(bReverse)
{
	ImpProcessObjectList(rObjList, eMode, bUseZOrder);
	Reset();
}

SdrObjListIter::SdrObjListIter( const SdrObject& rObj, SdrIterMode eMode, BOOL bReverse )
:	maObjList(1024, 64, 64),
	mnIndex(0L),
	mbReverse(bReverse)
{
	if ( rObj.ISA( SdrObjGroup ) )
		ImpProcessObjectList(*rObj.GetSubList(), eMode, TRUE);
	else
		maObjList.Insert( (void*)&rObj, LIST_APPEND );
	Reset();
}

SdrObjListIter::SdrObjListIter( const SdrMarkList& rMarkList, SdrIterMode eMode, BOOL bReverse )
:	maObjList(1024, 64, 64),
	mnIndex(0L),
	mbReverse(bReverse)
{
	ImpProcessMarkList(rMarkList, eMode);
	Reset();
}

void SdrObjListIter::ImpProcessObjectList(const SdrObjList& rObjList, SdrIterMode eMode, BOOL bUseZOrder)
{
	for( ULONG nIdx = 0, nCount = rObjList.GetObjCount(); nIdx < nCount; ++nIdx )
	{
		SdrObject* pObj = bUseZOrder ?
            rObjList.GetObj( nIdx ) : rObjList.GetObjectForNavigationPosition( nIdx );
        OSL_ASSERT( pObj != 0 );
        if( pObj )
            ImpProcessObj( pObj, eMode, bUseZOrder );
	}
}

void SdrObjListIter::ImpProcessMarkList( const SdrMarkList& rMarkList, SdrIterMode eMode )
{
    for( ULONG nIdx = 0, nCount = rMarkList.GetMarkCount(); nIdx < nCount; ++nIdx )
        if( SdrObject* pObj = rMarkList.GetMark( nIdx )->GetMarkedSdrObj() )
            ImpProcessObj( pObj, eMode, FALSE );
}

void SdrObjListIter::ImpProcessObj(SdrObject* pObj, SdrIterMode eMode, BOOL bUseZOrder)
{
    bool bIsGroup = pObj->IsGroupObject();
    // #99190# 3D objects are no group objects, IsGroupObject()
    // only tests if pSub is not null ptr :-(
    if( bIsGroup && pObj->ISA( E3dObject ) && !pObj->ISA( E3dScene ) )
        bIsGroup = false;

    if( !bIsGroup || (eMode != IM_DEEPNOGROUPS) )
        maObjList.Insert( pObj, LIST_APPEND );

    if( bIsGroup && (eMode != IM_FLAT) )
        ImpProcessObjectList( *pObj->GetSubList(), eMode, bUseZOrder );
}
