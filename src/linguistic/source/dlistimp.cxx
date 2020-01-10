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
#include "dlistimp.hxx"
#include "dicimp.hxx"
#include "lngopt.hxx"

#include <osl/file.hxx>
#include <tools/fsys.hxx>
#include <tools/stream.hxx>
#include <tools/urlobj.hxx>
#include <i18npool/mslangid.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/useroptions.hxx>
#include <sfx2/docfile.hxx>
#include <vcl/svapp.hxx>
#include <cppuhelper/factory.hxx>	// helper for factories
#include <unotools/localfilehelper.hxx>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/linguistic2/DictionaryEventFlags.hpp>
#include <com/sun/star/linguistic2/DictionaryListEventFlags.hpp>
#include <com/sun/star/registry/XRegistryKey.hpp>

//using namespace utl;
using namespace osl;
using namespace rtl;
using namespace com::sun::star;
using namespace com::sun::star::lang;
using namespace com::sun::star::uno;
using namespace com::sun::star::linguistic2;
using namespace linguistic;

///////////////////////////////////////////////////////////////////////////

static BOOL IsVers2OrNewer( const String& rFileURL, USHORT& nLng, BOOL& bNeg );

static void AddInternal( const uno::Reference< XDictionary > &rDic,
                         const rtl::OUString& rNew );
static void AddUserData( const uno::Reference< XDictionary > &rDic );

///////////////////////////////////////////////////////////////////////////

class DicEvtListenerHelper :
	public cppu::WeakImplHelper1
	<
		XDictionaryEventListener
	>
{
    cppu::OInterfaceContainerHelper         aDicListEvtListeners;
    uno::Sequence< DictionaryEvent >        aCollectDicEvt;
	uno::Reference< XDictionaryList >		xMyDicList;

	INT16								nCondensedEvt;
	INT16								nNumCollectEvtListeners,
		 								nNumVerboseListeners;

public:
	DicEvtListenerHelper( const uno::Reference< XDictionaryList > &rxDicList );
	virtual ~DicEvtListenerHelper();

	// XEventListener
	virtual void SAL_CALL
		disposing( const EventObject& rSource )
			throw(RuntimeException);

	// XDictionaryEventListener
    virtual void SAL_CALL
		processDictionaryEvent( const DictionaryEvent& rDicEvent )
			throw(RuntimeException);

	// non-UNO functions
	void 	DisposeAndClear( const EventObject &rEvtObj );

    BOOL	AddDicListEvtListener(
				const uno::Reference< XDictionaryListEventListener >& rxListener,
				BOOL bReceiveVerbose );
    BOOL	RemoveDicListEvtListener(
				const uno::Reference< XDictionaryListEventListener >& rxListener );
    INT16	BeginCollectEvents();
    INT16	EndCollectEvents();
    INT16	FlushEvents();
    void    ClearEvents()   { nCondensedEvt = 0; }
};


DicEvtListenerHelper::DicEvtListenerHelper(
		const uno::Reference< XDictionaryList > &rxDicList ) :
	aDicListEvtListeners	( GetLinguMutex() ),
	xMyDicList				( rxDicList )
{
	nCondensedEvt	= 0;
	nNumCollectEvtListeners = nNumVerboseListeners	= 0;
}


DicEvtListenerHelper::~DicEvtListenerHelper()
{
	DBG_ASSERT(aDicListEvtListeners.getLength() == 0,
		"lng : event listeners are still existing");
}


void DicEvtListenerHelper::DisposeAndClear( const EventObject &rEvtObj )
{
	aDicListEvtListeners.disposeAndClear( rEvtObj );
}


void SAL_CALL DicEvtListenerHelper::disposing( const EventObject& rSource )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	uno::Reference< XInterface > xSrc( rSource.Source );

	// remove event object from EventListener list
	if (xSrc.is())
		aDicListEvtListeners.removeInterface( xSrc );

	// if object is a dictionary then remove it from the dictionary list
	// Note: this will probably happen only if someone makes a XDictionary
	// implementation of his own that is also a XComponent.
	uno::Reference< XDictionary > xDic( xSrc, UNO_QUERY );
	if (xDic.is())
	{
		xMyDicList->removeDictionary( xDic );
	}
}


void SAL_CALL DicEvtListenerHelper::processDictionaryEvent(
			const DictionaryEvent& rDicEvent )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	uno::Reference< XDictionary > xDic( rDicEvent.Source, UNO_QUERY );
	DBG_ASSERT(xDic.is(), "lng : missing event source");

	// assert that there is a corresponding dictionary entry if one was
	// added or deleted
	uno::Reference< XDictionaryEntry > xDicEntry( rDicEvent.xDictionaryEntry, UNO_QUERY );
	DBG_ASSERT( !(rDicEvent.nEvent &
					(DictionaryEventFlags::ADD_ENTRY | DictionaryEventFlags::DEL_ENTRY))
				|| xDicEntry.is(),
				"lng : missing dictionary entry" );

    /*BOOL bActiveDicsModified = FALSE;*/
	//
	// evaluate DictionaryEvents and update data for next DictionaryListEvent
	//
	DictionaryType eDicType = xDic->getDictionaryType();
	DBG_ASSERT(eDicType != DictionaryType_MIXED,
		"lng : unexpected dictionary type");
	if ((rDicEvent.nEvent & DictionaryEventFlags::ADD_ENTRY) && xDic->isActive())
		nCondensedEvt |= xDicEntry->isNegative() ?
			DictionaryListEventFlags::ADD_NEG_ENTRY :
			DictionaryListEventFlags::ADD_POS_ENTRY;
	if ((rDicEvent.nEvent & DictionaryEventFlags::DEL_ENTRY) && xDic->isActive())
		nCondensedEvt |= xDicEntry->isNegative() ?
			DictionaryListEventFlags::DEL_NEG_ENTRY :
			DictionaryListEventFlags::DEL_POS_ENTRY;
	if ((rDicEvent.nEvent & DictionaryEventFlags::ENTRIES_CLEARED) && xDic->isActive())
		nCondensedEvt |= eDicType == DictionaryType_NEGATIVE ?
			DictionaryListEventFlags::DEL_NEG_ENTRY :
			DictionaryListEventFlags::DEL_POS_ENTRY;
	if ((rDicEvent.nEvent & DictionaryEventFlags::CHG_LANGUAGE) && xDic->isActive())
		nCondensedEvt |= eDicType == DictionaryType_NEGATIVE ?
			DictionaryListEventFlags::DEACTIVATE_NEG_DIC
				| DictionaryListEventFlags::ACTIVATE_NEG_DIC :
			DictionaryListEventFlags::DEACTIVATE_POS_DIC
				| DictionaryListEventFlags::ACTIVATE_POS_DIC;
	if ((rDicEvent.nEvent & DictionaryEventFlags::ACTIVATE_DIC))
		nCondensedEvt |= eDicType == DictionaryType_NEGATIVE ?
			DictionaryListEventFlags::ACTIVATE_NEG_DIC :
			DictionaryListEventFlags::ACTIVATE_POS_DIC;
	if ((rDicEvent.nEvent & DictionaryEventFlags::DEACTIVATE_DIC))
		nCondensedEvt |= eDicType == DictionaryType_NEGATIVE ?
			DictionaryListEventFlags::DEACTIVATE_NEG_DIC :
			DictionaryListEventFlags::DEACTIVATE_POS_DIC;

	// update list of collected events if needs to be
	if (nNumVerboseListeners > 0)
	{
		INT32 nColEvts = aCollectDicEvt.getLength();
		aCollectDicEvt.realloc( nColEvts + 1 );
		aCollectDicEvt.getArray()[ nColEvts ] = rDicEvent;
	}

	if (nNumCollectEvtListeners == 0 && nCondensedEvt != 0)
		FlushEvents();
}


BOOL DicEvtListenerHelper::AddDicListEvtListener(
			const uno::Reference< XDictionaryListEventListener >& xListener,
            BOOL /*bReceiveVerbose*/ )
{
	DBG_ASSERT( xListener.is(), "empty reference" );
	INT32	nCount = aDicListEvtListeners.getLength();
	return aDicListEvtListeners.addInterface( xListener ) != nCount;
}


BOOL DicEvtListenerHelper::RemoveDicListEvtListener(
			const uno::Reference< XDictionaryListEventListener >& xListener )
{
	DBG_ASSERT( xListener.is(), "empty reference" );
	INT32	nCount = aDicListEvtListeners.getLength();
	return aDicListEvtListeners.removeInterface( xListener ) != nCount;
}


INT16 DicEvtListenerHelper::BeginCollectEvents()
{
	return ++nNumCollectEvtListeners;
}


INT16 DicEvtListenerHelper::EndCollectEvents()
{
	DBG_ASSERT(nNumCollectEvtListeners > 0, "lng: mismatched function call");
	if (nNumCollectEvtListeners > 0)
	{
		FlushEvents();
		nNumCollectEvtListeners--;
	}

	return nNumCollectEvtListeners;
}


INT16 DicEvtListenerHelper::FlushEvents()
{
	if (0 != nCondensedEvt)
	{
		// build DictionaryListEvent to pass on to listeners
		uno::Sequence< DictionaryEvent > aDicEvents;
		if (nNumVerboseListeners > 0)
			aDicEvents = aCollectDicEvt;
		DictionaryListEvent aEvent( xMyDicList, nCondensedEvt, aDicEvents );

		// pass on event
		cppu::OInterfaceIteratorHelper aIt( aDicListEvtListeners );
		while (aIt.hasMoreElements())
		{
			uno::Reference< XDictionaryListEventListener > xRef( aIt.next(), UNO_QUERY );
			if (xRef.is())
				xRef->processDictionaryListEvent( aEvent );
		}

		// clear "list" of events
		nCondensedEvt = 0;
		aCollectDicEvt.realloc( 0 );
	}

	return nNumCollectEvtListeners;
}


///////////////////////////////////////////////////////////////////////////


void DicList::MyAppExitListener::AtExit()
{
    rMyDicList.SaveDics();
}


DicList::DicList() :
    aEvtListeners   ( GetLinguMutex() )
{
	pDicEvtLstnrHelper	= new DicEvtListenerHelper( this );
	xDicEvtLstnrHelper	= pDicEvtLstnrHelper;
	bDisposing = FALSE;
    bInCreation = FALSE;

	pExitListener = new MyAppExitListener( *this );
	xExitListener = pExitListener;
	pExitListener->Activate();
}

DicList::~DicList()
{
	pExitListener->Deactivate();
}


void DicList::SearchForDictionaries( 
    DictionaryVec_t&rDicList,
    const String &rDicDirURL, 
    BOOL bIsWriteablePath )
{
    osl::MutexGuard aGuard( GetLinguMutex() );

    const uno::Sequence< rtl::OUString > aDirCnt( utl::LocalFileHelper::
                                        GetFolderContents( rDicDirURL, FALSE ) );
    const rtl::OUString *pDirCnt = aDirCnt.getConstArray();
	INT32 nEntries = aDirCnt.getLength();

	String aDCN( String::CreateFromAscii( "dcn" ) );
	String aDCP( String::CreateFromAscii( "dcp" ) );
	for (INT32 i = 0;  i < nEntries;  ++i)
	{
        String  aURL( pDirCnt[i] );
		USHORT	nLang = LANGUAGE_NONE;
		BOOL	bNeg  = FALSE;

        if(!::IsVers2OrNewer( aURL, nLang, bNeg ))
		{
			// Wenn kein
            xub_StrLen nPos  = aURL.Search('.');
            String aExt(aURL.Copy(nPos + 1));
			aExt.ToLowerAscii();

			if(aExt == aDCN)       // negativ
				bNeg = TRUE;
			else if(aExt == aDCP)  // positiv
				bNeg = FALSE;
			else
				continue;          // andere Files
		}

		// Aufnehmen in die Liste der Dictionaries
		// Wenn existent nicht aufnehmen
		//
		INT16 nSystemLanguage = MsLangId::getSystemLanguage();
        String aTmp1 = ToLower( aURL, nSystemLanguage );
		xub_StrLen nPos = aTmp1.SearchBackward( '/' );
		if (STRING_NOTFOUND != nPos)
			aTmp1 = aTmp1.Copy( nPos + 1 );
		String aTmp2;
        size_t j;
        size_t nCount = rDicList.size();
		for(j = 0;  j < nCount;  j++)
		{
            aTmp2 = rDicList[j]->getName().getStr();
			aTmp2 = ToLower( aTmp2, nSystemLanguage );
			if(aTmp1 == aTmp2)
				break;
		}
		if(j >= nCount)		// dictionary not yet in DicList
		{
            // get decoded dictionary file name
            INetURLObject aURLObj( aURL );
            String aDicName = aURLObj.getName( INetURLObject::LAST_SEGMENT, 
                        true, INetURLObject::DECODE_WITH_CHARSET, 
                        RTL_TEXTENCODING_UTF8 );
            
            DictionaryType eType = bNeg ? DictionaryType_NEGATIVE : DictionaryType_POSITIVE;
			uno::Reference< XDictionary > xDic =
                        new DictionaryNeo( aDicName, nLang, eType, aURL, bIsWriteablePath );

			addDictionary( xDic );
			nCount++;
		}
	}
}


INT32 DicList::GetDicPos(const uno::Reference< XDictionary > &xDic)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	INT32 nPos = -1;
    DictionaryVec_t& rDicList = GetOrCreateDicList();
    size_t n = rDicList.size();
    for (size_t i = 0;  i < n;  i++)
	{
        if ( rDicList[i] == xDic )
			return i;
	}
	return nPos;
}


uno::Reference< XInterface > SAL_CALL
    DicList_CreateInstance( const uno::Reference< XMultiServiceFactory > & /*rSMgr*/ )
			throw(Exception)
{
	uno::Reference< XInterface > xService = (cppu::OWeakObject *) new DicList;
	return xService;
}

sal_Int16 SAL_CALL DicList::getCount() throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );
    return static_cast< sal_Int16 >(GetOrCreateDicList().size());
}

uno::Sequence< uno::Reference< XDictionary > > SAL_CALL
		DicList::getDictionaries()
			throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

    DictionaryVec_t& rDicList = GetOrCreateDicList();

    uno::Sequence< uno::Reference< XDictionary > > aDics( rDicList.size() );
	uno::Reference< XDictionary > *pDic = aDics.getArray();

    INT32 n = (USHORT) aDics.getLength();
    for (INT32 i = 0;  i < n;  i++)
        pDic[i] = rDicList[i];

	return aDics;
}

uno::Reference< XDictionary > SAL_CALL
        DicList::getDictionaryByName( const rtl::OUString& aDictionaryName )
			throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	uno::Reference< XDictionary > xDic;
    DictionaryVec_t& rDicList = GetOrCreateDicList();
    size_t nCount = rDicList.size();
    for (size_t i = 0;  i < nCount;  i++)
	{
        const uno::Reference< XDictionary > &rDic = rDicList[i];
		if (rDic.is()  &&  rDic->getName() == aDictionaryName)
		{
			xDic = rDic;
			break;
		}
	}

	return xDic;
}

sal_Bool SAL_CALL DicList::addDictionary(
			const uno::Reference< XDictionary >& xDictionary )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	if (bDisposing)
		return FALSE;

	BOOL bRes = FALSE;
	if (xDictionary.is())
	{
        DictionaryVec_t& rDicList = GetOrCreateDicList();
        rDicList.push_back( xDictionary );
		bRes = TRUE;

		// add listener helper to the dictionaries listener lists
		xDictionary->addDictionaryEventListener( xDicEvtLstnrHelper );
	}
	return bRes;
}

sal_Bool SAL_CALL
	DicList::removeDictionary( const uno::Reference< XDictionary >& xDictionary )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	if (bDisposing)
		return FALSE;

	BOOL  bRes = FALSE;
    INT32 nPos = GetDicPos( xDictionary );
	if (nPos >= 0)
	{
		// remove dictionary list from the dictionaries listener lists
        DictionaryVec_t& rDicList = GetOrCreateDicList();
        uno::Reference< XDictionary > xDic( rDicList[ nPos ] );
		DBG_ASSERT(xDic.is(), "lng : empty reference");
		if (xDic.is())
		{
			// deactivate dictionary if not already done
			xDic->setActive( FALSE );

			xDic->removeDictionaryEventListener( xDicEvtLstnrHelper );
		}

        // remove element at nPos
        rDicList.erase( rDicList.begin() + nPos );
		bRes = TRUE;
	}
	return bRes;
}

sal_Bool SAL_CALL DicList::addDictionaryListEventListener(
			const uno::Reference< XDictionaryListEventListener >& xListener,
			sal_Bool bReceiveVerbose )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	if (bDisposing)
		return FALSE;

	DBG_ASSERT(!bReceiveVerbose, "lng : not yet supported");

	BOOL bRes = FALSE;
	if (xListener.is())	//! don't add empty references
	{
		bRes = pDicEvtLstnrHelper->
						AddDicListEvtListener( xListener, bReceiveVerbose );
	}
	return bRes;
}

sal_Bool SAL_CALL DicList::removeDictionaryListEventListener(
			const uno::Reference< XDictionaryListEventListener >& xListener )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	if (bDisposing)
		return FALSE;

	BOOL bRes = FALSE;
	if(xListener.is())
	{
		bRes = pDicEvtLstnrHelper->RemoveDicListEvtListener( xListener );
	}
	return bRes;
}

sal_Int16 SAL_CALL DicList::beginCollectEvents() throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );
	return pDicEvtLstnrHelper->BeginCollectEvents();
}

sal_Int16 SAL_CALL DicList::endCollectEvents() throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );
	return pDicEvtLstnrHelper->EndCollectEvents();
}

sal_Int16 SAL_CALL DicList::flushEvents() throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );
	return pDicEvtLstnrHelper->FlushEvents();
}

uno::Reference< XDictionary > SAL_CALL
    DicList::createDictionary( const rtl::OUString& rName, const Locale& rLocale,
            DictionaryType eDicType, const rtl::OUString& rURL )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	INT16 nLanguage = LocaleToLanguage( rLocale );
    bool bIsWriteablePath = rURL.match( GetDictionaryWriteablePath(), 0 );
    return new DictionaryNeo( rName, nLanguage, eDicType, rURL, bIsWriteablePath );
}


uno::Reference< XDictionaryEntry > SAL_CALL
    DicList::queryDictionaryEntry( const rtl::OUString& rWord, const Locale& rLocale,
			sal_Bool bSearchPosDics, sal_Bool bSearchSpellEntry )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );
	return SearchDicList( this, rWord, LocaleToLanguage( rLocale ),
							bSearchPosDics, bSearchSpellEntry );
}


void SAL_CALL
	DicList::dispose()
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	if (!bDisposing)
	{
		bDisposing = TRUE;
		EventObject	aEvtObj( (XDictionaryList *) this );

		aEvtListeners.disposeAndClear( aEvtObj );
		if (pDicEvtLstnrHelper)
			pDicEvtLstnrHelper->DisposeAndClear( aEvtObj );

        //! avoid creation of dictionaries if not already done
        if (aDicList.size() > 0)
        {
            DictionaryVec_t& rDicList = GetOrCreateDicList();
            size_t nCount = rDicList.size();
            for (size_t i = 0;  i < nCount;  i++)
            {
                uno::Reference< XDictionary > xDic( rDicList[i], UNO_QUERY );

                // save (modified) dictionaries
                uno::Reference< frame::XStorable >  xStor( xDic , UNO_QUERY );
                if (xStor.is())
                {
                    try
                    {
                        if (!xStor->isReadonly() && xStor->hasLocation())
                            xStor->store();
                    }
                    catch(Exception &)
                    {
                    }
                }

                // release references to (members of) this object hold by
                // dictionaries
                if (xDic.is())
                    xDic->removeDictionaryEventListener( xDicEvtLstnrHelper );
            }
        }
	}
}

void SAL_CALL
	DicList::addEventListener( const uno::Reference< XEventListener >& rxListener )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	if (!bDisposing && rxListener.is())
		aEvtListeners.addInterface( rxListener );
}

void SAL_CALL
	DicList::removeEventListener( const uno::Reference< XEventListener >& rxListener )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

	if (!bDisposing && rxListener.is())
		aEvtListeners.removeInterface( rxListener );
}

void DicList::_CreateDicList()
{
	bInCreation = TRUE;

	// look for dictionaries
    const rtl::OUString aWriteablePath( GetDictionaryWriteablePath() );
    uno::Sequence< rtl::OUString > aPaths( GetDictionaryPaths() );
    const rtl::OUString *pPaths = aPaths.getConstArray();
    for (sal_Int32 i = 0;  i < aPaths.getLength();  ++i)
    {
        const BOOL bIsWriteablePath = (pPaths[i] == aWriteablePath);
        SearchForDictionaries( aDicList, pPaths[i], bIsWriteablePath );
    }

	// create IgnoreAllList dictionary with empty URL (non persistent)
	// and add it to list
    rtl::OUString aDicName( A2OU( "IgnoreAllList" ) );
	uno::Reference< XDictionary > xIgnAll(
			createDictionary( aDicName, CreateLocale( LANGUAGE_NONE ),
                              DictionaryType_POSITIVE, rtl::OUString() ) );
	if (xIgnAll.is())
	{
		AddUserData( xIgnAll );
		xIgnAll->setActive( TRUE );
		addDictionary( xIgnAll );
	}
    
    
    // evaluate list of dictionaries to be activated from configuration
	//
	//! to suppress overwriting the list of active dictionaries in the
	//! configuration with incorrect arguments during the following
	//! activation of the dictionaries
	pDicEvtLstnrHelper->BeginCollectEvents();
	//
    const uno::Sequence< rtl::OUString > aActiveDics( aOpt.GetActiveDics() );
    const rtl::OUString *pActiveDic = aActiveDics.getConstArray();
	INT32 nLen = aActiveDics.getLength();
	for (INT32 i = 0;  i < nLen;  ++i)
	{
		if (pActiveDic[i].getLength())
		{
			uno::Reference< XDictionary > xDic( getDictionaryByName( pActiveDic[i] ) );
			if (xDic.is())
				xDic->setActive( TRUE );
		}
	}
    
    // suppress collected events during creation of the dictionary list.
    // there should be no events during creation.
    pDicEvtLstnrHelper->ClearEvents();
    
    pDicEvtLstnrHelper->EndCollectEvents();

	bInCreation = FALSE;
}


void DicList::SaveDics()
{
    // save dics only if they have already been used/created.
    //! don't create them just for the purpose of saving them !
    if (aDicList.size() > 0)
    {
        // save (modified) dictionaries
        DictionaryVec_t& rDicList = GetOrCreateDicList();
        size_t nCount = rDicList.size();;
        for (size_t i = 0;  i < nCount;  i++)
        {
            // save (modified) dictionaries
            uno::Reference< frame::XStorable >  xStor( rDicList[i], UNO_QUERY );
            if (xStor.is())
            {
                try
                {
                    if (!xStor->isReadonly() && xStor->hasLocation())
                        xStor->store();
                }
                catch(Exception &)
                {
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////
// Service specific part
//

rtl::OUString SAL_CALL DicList::getImplementationName(  ) throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );
	return getImplementationName_Static();
}


sal_Bool SAL_CALL DicList::supportsService( const rtl::OUString& ServiceName )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );

    uno::Sequence< rtl::OUString > aSNL = getSupportedServiceNames();
    const rtl::OUString * pArray = aSNL.getConstArray();
	for( INT32 i = 0; i < aSNL.getLength(); i++ )
		if( pArray[i] == ServiceName )
			return TRUE;
	return FALSE;
}


uno::Sequence< rtl::OUString > SAL_CALL DicList::getSupportedServiceNames(  )
		throw(RuntimeException)
{
    osl::MutexGuard aGuard( GetLinguMutex() );
	return getSupportedServiceNames_Static();
}


uno::Sequence< rtl::OUString > DicList::getSupportedServiceNames_Static() throw()
{
    osl::MutexGuard aGuard( GetLinguMutex() );

    uno::Sequence< rtl::OUString > aSNS( 1 );   // auch mehr als 1 Service moeglich
	aSNS.getArray()[0] = A2OU( SN_DICTIONARY_LIST );
	return aSNS;
}


sal_Bool SAL_CALL DicList_writeInfo(
	void * /*pServiceManager*/, registry::XRegistryKey * pRegistryKey )
{
	try
	{
		String aImpl( '/' );
		aImpl += DicList::getImplementationName_Static().getStr();
		aImpl.AppendAscii( "/UNO/SERVICES" );
		uno::Reference< registry::XRegistryKey > xNewKey =
				pRegistryKey->createKey(aImpl );
        uno::Sequence< rtl::OUString > aServices =
				DicList::getSupportedServiceNames_Static();
		for( INT32 i = 0; i < aServices.getLength(); i++ )
			xNewKey->createKey( aServices.getConstArray()[i]);

		return sal_True;
	}
	catch(Exception &)
	{
		return sal_False;
	}
}


void * SAL_CALL DicList_getFactory(	const sal_Char * pImplName,
		XMultiServiceFactory * pServiceManager, void *  )
{
	void * pRet = 0;
	if ( !DicList::getImplementationName_Static().compareToAscii( pImplName ) )
	{
		uno::Reference< XSingleServiceFactory > xFactory =
			cppu::createOneInstanceFactory(
				pServiceManager,
				DicList::getImplementationName_Static(),
				DicList_CreateInstance,
				DicList::getSupportedServiceNames_Static());
		// acquire, because we return an interface pointer instead of a reference
		xFactory->acquire();
		pRet = xFactory.get();
	}
	return pRet;
}

///////////////////////////////////////////////////////////////////////////

xub_StrLen lcl_GetToken( String &rToken,
			const String &rText, xub_StrLen nPos, const String &rDelim )
{
	xub_StrLen nRes = STRING_LEN;

	if (rText.Len() == 0  ||  nPos >= rText.Len())
		rToken = String();
	else if (rDelim.Len() == 0)
	{
		rToken = rText;
		if (rToken.Len())
			nRes = rText.Len();
	}
	else
	{
		xub_StrLen	i;
		for (i = nPos;  i < rText.Len();  ++i)
		{
			if (STRING_NOTFOUND != rDelim.Search( rText.GetChar(i) ))
				break;
		}

		if (i >= rText.Len())	// delimeter not found
			rToken	= rText.Copy( nPos );
		else
            rToken  = rText.Copy( nPos, sal::static_int_cast< xub_StrLen >((INT32) i - nPos) );
		nRes	= i + 1;	// continue after found delimeter
	}

	return nRes;
}


static void AddInternal(
		const uno::Reference<XDictionary> &rDic,
        const rtl::OUString& rNew )
{
	if (rDic.is())
	{
		//! TL TODO: word iterator should be used to break up the text
		static const char *pDefWordDelim =
				"!\"#$%&'()*+,-./:;<=>?[]\\_^`{|}~\t \n";
		ByteString aDummy( pDefWordDelim );
        String aDelim( aDummy, osl_getThreadTextEncoding() );
		aDelim.EraseAllChars( '.' );

		String 		aToken;
		xub_StrLen  nPos = 0;
		while (STRING_LEN !=
					(nPos = lcl_GetToken( aToken, rNew, nPos, aDelim )))
		{
        	if( aToken.Len()  &&  !IsNumeric( aToken ) )
			{
                rDic->add( aToken, FALSE, rtl::OUString() );
			}
		}
	}
}

static void AddUserData( const uno::Reference< XDictionary > &rDic )
{
	if (rDic.is())
	{
		SvtUserOptions aUserOpt;
		AddInternal( rDic, aUserOpt.GetFullName() );
		AddInternal( rDic, aUserOpt.GetCompany() );
		AddInternal( rDic, aUserOpt.GetStreet() );
		AddInternal( rDic, aUserOpt.GetCity() );
		AddInternal( rDic, aUserOpt.GetTitle() );
		AddInternal( rDic, aUserOpt.GetPosition() );
		AddInternal( rDic, aUserOpt.GetEmail() );
	}
}

///////////////////////////////////////////////////////////////////////////

#if defined _MSC_VER
#pragma optimize("g",off)
#endif

static BOOL IsVers2OrNewer( const String& rFileURL, USHORT& nLng, BOOL& bNeg )
{
	if (rFileURL.Len() == 0)
		return FALSE;
	String aDIC( GetDicExtension() );
	String aExt;
	xub_StrLen nPos = rFileURL.SearchBackward( '.' );
	if (STRING_NOTFOUND != nPos)
		aExt = rFileURL.Copy( nPos + 1 );
	aExt.ToLowerAscii();

	if(aExt != aDIC)
		return FALSE;

	// get stream to be used
	SfxMedium aMedium( rFileURL, STREAM_READ | STREAM_SHARE_DENYWRITE, FALSE );
	SvStream *pStream = aMedium.GetInStream();
    
    int nDicVersion = ReadDicVersion (pStream, nLng, bNeg);
    if (2 == nDicVersion || nDicVersion >= 5)
        return TRUE;
    
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////

