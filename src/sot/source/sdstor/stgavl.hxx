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

#ifndef _STGAVL_HXX
#define _STGAVL_HXX

#ifndef _TOOLS_SOLAR_H
#include <tools/solar.h>
#endif

// This class must be overloaded to define real, living nodes.
// Especially, the compare function must be implemented.

class StgAvlNode
{
	friend class StgAvlIterator;
private:
	short Locate( StgAvlNode*, StgAvlNode**, StgAvlNode**, StgAvlNode** );
	short Adjust( StgAvlNode**, StgAvlNode* );
	StgAvlNode* RotLL();
	StgAvlNode* RotLR();
	StgAvlNode* RotRR();
	StgAvlNode* RotRL();
	void   StgEnum( short& );
	static StgAvlNode* Rem( StgAvlNode**, StgAvlNode*, BOOL );
protected:
	short nId;						  	// iterator ID
	short nBalance;						// indicates tree balance
	StgAvlNode* pLeft, *pRight; 		// leaves
	StgAvlNode();
public:
	virtual ~StgAvlNode();
	StgAvlNode* Find( StgAvlNode* );
	static BOOL Insert( StgAvlNode**, StgAvlNode* );
	static BOOL Remove( StgAvlNode**, StgAvlNode*, BOOL bDel = TRUE );
	static BOOL Move( StgAvlNode**, StgAvlNode**, StgAvlNode* );
	virtual short Compare( const StgAvlNode* ) const = 0;
};

// The iterator class provides single stepping through an AVL tree.

class StgAvlIterator {
	StgAvlNode* pRoot;					// root entry (parent)
	short 	    nCount;					// tree size
	short		nCur;					// current element
	StgAvlNode* Find( short );
public:
	StgAvlIterator( StgAvlNode* );
	StgAvlNode* First();
	StgAvlNode* Last();
	StgAvlNode* Next();
	StgAvlNode* Prev();
};

#endif
