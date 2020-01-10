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

#ifndef _SOT_AGG_HXX
#define _SOT_AGG_HXX

#include <tools/ownlist.hxx>

/************** class SvAggregate ***************************************/
/************************************************************************/
class SotFactory;
class SotObject;
struct SvAggregate
{
	union
	{
		SotFactory * pFact;
		SotObject *	pObj;
	};
	BOOL	bFactory;
	BOOL	bMainObj; // TRUE, das Objekt, welches das casting steuert

	SvAggregate()
		: pFact( NULL )
        , bFactory( FALSE )
        , bMainObj( FALSE ) {}
	SvAggregate( SotObject * pObjP, BOOL bMainP )
		: pObj( pObjP )
        , bFactory( FALSE )
		, bMainObj( bMainP ) {}
	SvAggregate( SotFactory * pFactP )
		: pFact( pFactP )
        , bFactory( TRUE )
		, bMainObj( FALSE ) {}
};

/************** class SvAggregateMemberList *****************************/
/************************************************************************/
class SvAggregateMemberList
{
	 PRV_SV_DECL_OWNER_LIST(SvAggregateMemberList,SvAggregate)
};

#endif // _AGG_HXX
