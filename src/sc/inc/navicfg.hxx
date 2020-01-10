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

#ifndef SC_NAVICFG_HXX
#define SC_NAVICFG_HXX

#include <tools/solar.h>


//==================================================================
// CfgItem fuer Navigator-Zustand
//==================================================================

class ScNavipiCfg
{
private:
	USHORT 	nListMode;
	USHORT	nDragMode;
	USHORT	nRootType;

public:
			ScNavipiCfg();

	void	SetListMode(USHORT nNew);
	USHORT	GetListMode() const			{ return nListMode; }
	void	SetDragMode(USHORT nNew);
	USHORT	GetDragMode() const			{ return nDragMode; }
	void	SetRootType(USHORT nNew);
	USHORT	GetRootType() const			{ return nRootType; }
};


#endif

