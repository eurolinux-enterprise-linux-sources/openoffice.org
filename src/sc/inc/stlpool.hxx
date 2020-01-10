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

#ifndef SC_STLPOOL_HXX
#define SC_STLPOOL_HXX

#include <svtools/style.hxx>


class ScStyleSheet;
class ScDocument;

class ScStyleSheetPool : public SfxStyleSheetPool
{
public:
						ScStyleSheetPool( SfxItemPool&	rPool,
										  ScDocument*	pDocument );

	void				SetDocument( ScDocument* pDocument );
	ScDocument*			GetDocument() const { return pDoc; }

	virtual void		Remove( SfxStyleSheetBase* pStyle );

	void				SetActualStyleSheet ( SfxStyleSheetBase* pActStyleSheet )
								{ pActualStyleSheet = pActStyleSheet; }

	SfxStyleSheetBase*	GetActualStyleSheet ()
								{ return pActualStyleSheet; }

	void				CreateStandardStyles();
	void				CopyStdStylesFrom( ScStyleSheetPool* pSrcPool );
//UNUSED2008-05  void				UpdateStdNames();

	void				CopyStyleFrom( ScStyleSheetPool* pSrcPool,
										const String& rName, SfxStyleFamily eFamily );

	ScStyleSheet*		FindCaseIns( const String& rName, SfxStyleFamily eFam );

//UNUSED2009-05 void				SetForceStdName( const String* pSet );
	const String*		GetForceStdName() const	{ return pForceStdName; }

	virtual SfxStyleSheetBase& Make( const String&, SfxStyleFamily eFam,
									 USHORT nMask = 0xffff, USHORT nPos = 0xffff );

protected:
	virtual 			~ScStyleSheetPool();

    using SfxStyleSheetPool::Create;    // calcwarnings: Create(const SfxStyleSheet&) - ever used?

	virtual SfxStyleSheetBase* Create( const String&	rName,
									   SfxStyleFamily	eFamily,
									   USHORT			nMask);
	virtual SfxStyleSheetBase* Create( const SfxStyleSheetBase& rStyle );

private:
	SfxStyleSheetBase*	pActualStyleSheet;
	ScDocument*			pDoc;
	const String*		pForceStdName;
};

#endif	   // SC_STLPOOL_HXX

