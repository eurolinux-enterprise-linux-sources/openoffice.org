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
#include "precompiled_linguistic.hxx"

#include <string.h>

#include "iprcache.hxx"
#include "misc.hxx"

#include <com/sun/star/linguistic2/DictionaryListEventFlags.hpp>
#include <tools/debug.hxx>
#include <osl/mutex.hxx>

//#define IPR_DEF_CACHE_SIZE		503
#define IPR_DEF_CACHE_MAX		375
#define IPR_DEF_CACHE_MAXINPUT	200

#ifdef DBG_STATISTIC
#include <tools/stream.hxx>

//#define IPR_CACHE_SIZE 		nTblSize
#define IPR_CACHE_MAX 		nMax
#define IPR_CACHE_MAXINPUT 	nMaxInput

#else

//#define IPR_CACHE_SIZE		IPR_DEF_CACHE_SIZE
#define IPR_CACHE_MAX		IPR_DEF_CACHE_MAX
#define IPR_CACHE_MAXINPUT	IPR_DEF_CACHE_MAXINPUT

#endif
#include <unotools/processfactory.hxx>

#include <lngprops.hxx>

using namespace utl;
using namespace osl;
using namespace rtl;
using namespace com::sun::star;
using namespace com::sun::star::beans;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace com::sun::star::linguistic2;


namespace linguistic
{

///////////////////////////////////////////////////////////////////////////

#define NUM_FLUSH_PROPS		6

static const struct
{
	const char *pPropName;
	INT32		nPropHdl;
} aFlushProperties[ NUM_FLUSH_PROPS ] = 
{
    { UPN_IS_USE_DICTIONARY_LIST,         UPH_IS_USE_DICTIONARY_LIST },
    { UPN_IS_IGNORE_CONTROL_CHARACTERS,   UPH_IS_IGNORE_CONTROL_CHARACTERS },
    { UPN_IS_SPELL_UPPER_CASE,            UPH_IS_SPELL_UPPER_CASE },
    { UPN_IS_SPELL_WITH_DIGITS,           UPH_IS_SPELL_WITH_DIGITS },
    { UPN_IS_SPELL_CAPITALIZATION,        UPH_IS_SPELL_CAPITALIZATION }
};


static void lcl_AddAsPropertyChangeListener( 
		Reference< XPropertyChangeListener > xListener,
		Reference< XPropertySet > &rPropSet )
{
	if (xListener.is() && rPropSet.is())
	{
		for (int i = 0;  i < NUM_FLUSH_PROPS;  ++i)
		{
			rPropSet->addPropertyChangeListener( 
					A2OU(aFlushProperties[i].pPropName), xListener );
		}
	}
}


static void lcl_RemoveAsPropertyChangeListener( 
		Reference< XPropertyChangeListener > xListener,
		Reference< XPropertySet > &rPropSet )
{
	if (xListener.is() && rPropSet.is())
	{
		for (int i = 0;  i < NUM_FLUSH_PROPS;  ++i)
		{
			rPropSet->removePropertyChangeListener( 
					A2OU(aFlushProperties[i].pPropName), xListener );
		}
	}
}


static BOOL lcl_IsFlushProperty( INT32 nHandle )
{
	int i;
	for (i = 0;  i < NUM_FLUSH_PROPS;  ++i)
	{
		if (nHandle == aFlushProperties[i].nPropHdl)
			break;
	}
	return i < NUM_FLUSH_PROPS;
}


FlushListener::FlushListener( Flushable *pFO )
{
	SetFlushObj( pFO );
}


FlushListener::~FlushListener()
{
}


void FlushListener::SetDicList(	Reference<XDictionaryList> &rDL )
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (xDicList != rDL)
	{
		if (xDicList.is())
			xDicList->removeDictionaryListEventListener( this );

		xDicList = rDL;
		if (xDicList.is())
			xDicList->addDictionaryListEventListener( this, FALSE );
	}
}
	

void FlushListener::SetPropSet( Reference< XPropertySet > &rPS )
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (xPropSet != rPS)
	{
		if (xPropSet.is())
			lcl_RemoveAsPropertyChangeListener( this, xPropSet );

		xPropSet = rPS;
		if (xPropSet.is())
			lcl_AddAsPropertyChangeListener( this, xPropSet );
	}
}


void SAL_CALL FlushListener::disposing( const EventObject& rSource )
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (xDicList.is()  &&  rSource.Source == xDicList)
	{
		xDicList->removeDictionaryListEventListener( this );
		xDicList = NULL;	//! release reference
	}
	if (xPropSet.is()  &&  rSource.Source == xPropSet)
	{
		lcl_RemoveAsPropertyChangeListener( this, xPropSet );
		xPropSet = NULL;	//! release reference
	}
}


void SAL_CALL FlushListener::processDictionaryListEvent(
			const DictionaryListEvent& rDicListEvent )
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (rDicListEvent.Source == xDicList)
	{
		INT16 nEvt = rDicListEvent.nCondensedEvent;
		INT16 nFlushFlags = 
				DictionaryListEventFlags::ADD_NEG_ENTRY		|
				DictionaryListEventFlags::DEL_POS_ENTRY		|
				DictionaryListEventFlags::ACTIVATE_NEG_DIC	|
				DictionaryListEventFlags::DEACTIVATE_POS_DIC;
		BOOL bFlush	= 0 != (nEvt & nFlushFlags);

		DBG_ASSERT( pFlushObj, "missing object (NULL pointer)" );
		if (bFlush && pFlushObj != NULL)
			pFlushObj->Flush();
	}
}
    

void SAL_CALL FlushListener::propertyChange( 
			const PropertyChangeEvent& rEvt ) 
		throw(RuntimeException)
{
	MutexGuard	aGuard( GetLinguMutex() );

	if (rEvt.Source == xPropSet)
	{
		BOOL bFlush	= lcl_IsFlushProperty( rEvt.PropertyHandle );

		DBG_ASSERT( pFlushObj, "missing object (NULL pointer)" );
		if (bFlush && pFlushObj != NULL)
			pFlushObj->Flush();
	}
}


///////////////////////////////////////////////////////////////////////////

SpellCache::SpellCache()
{
	pFlushLstnr = new FlushListener( this );
	xFlushLstnr = pFlushLstnr;
    Reference<XDictionaryList> aDictionaryList(GetDictionaryList());
	pFlushLstnr->SetDicList( aDictionaryList );	//! after reference is established
    Reference<XPropertySet> aPropertySet(GetLinguProperties());
	pFlushLstnr->SetPropSet( aPropertySet );	//! after reference is established
}

SpellCache::~SpellCache()
{
    Reference<XDictionaryList>  aEmptyList;
    Reference<XPropertySet>     aEmptySet;
    pFlushLstnr->SetDicList( aEmptyList );
    pFlushLstnr->SetPropSet( aEmptySet );
}

void SpellCache::Flush()
{
    MutexGuard	aGuard( GetLinguMutex() );
    // clear word list
    LangWordList_t aEmpty;
    aWordLists.swap( aEmpty );
}

bool SpellCache::CheckWord( const OUString& rWord, LanguageType nLang )
{
    MutexGuard	aGuard( GetLinguMutex() );
    WordList_t &rList = aWordLists[ nLang ];
    const WordList_t::const_iterator aIt = rList.find( rWord );
    return aIt != rList.end();
}

void SpellCache::AddWord( const OUString& rWord, LanguageType nLang )
{
    MutexGuard	aGuard( GetLinguMutex() );
    WordList_t & rList = aWordLists[ nLang ];
    // occasional clean-up...
    if (rList.size() > 500)
        rList.clear();
    rList.insert( rWord );
}
///////////////////////////////////////////////////////////////////////////

}	// namespace linguistic

