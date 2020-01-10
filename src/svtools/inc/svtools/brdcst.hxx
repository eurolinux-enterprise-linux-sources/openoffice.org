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
#ifndef _SFXBRDCST_HXX
#define _SFXBRDCST_HXX

#include "svtools/svldllapi.h"
#include <tools/rtti.hxx>
#include <svtools/svarray.hxx>

class SfxListener;
class SfxHint;

#ifndef _SFX_BRDCST_CXX
typedef SvPtrarr SfxListenerArr_Impl;
#endif

//-------------------------------------------------------------------------

class SVL_DLLPUBLIC SfxBroadcaster
{
friend class SfxListener;

	SfxListenerArr_Impl 	aListeners;

private:
	BOOL		 AddListener( SfxListener& rListener );
	void					RemoveListener( SfxListener& rListener );
	const SfxBroadcaster&	operator=(const SfxBroadcaster &); // verboten

protected:
	void					Forward(SfxBroadcaster& rBC, const SfxHint& rHint);
	virtual void			ListenersGone();

public:
							TYPEINFO();

							SfxBroadcaster();
							SfxBroadcaster( const SfxBroadcaster &rBC );
	virtual 				~SfxBroadcaster();

	void					Broadcast( const SfxHint &rHint );
	void					BroadcastDelayed( const SfxHint& rHint );
	void					BroadcastInIdle( const SfxHint& rHint );

	BOOL					HasListeners() const;
	USHORT					GetListenerCount() const { return aListeners.Count(); }
	SfxListener*			GetListener( USHORT nNo ) const
							{ return (SfxListener*) aListeners[nNo]; }
};

#endif
