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

#ifndef _LINGUISTIC_LNGPROPHELP_HXX_
#define _LINGUISTIC_LNGPROPHELP_HXX_

#include <tools/solar.h>

#include <uno/lbnames.h>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/interfacecontainer.h>
#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <com/sun/star/beans/PropertyValues.hpp>

#include <com/sun/star/linguistic2/XLinguServiceEventBroadcaster.hpp>

namespace com { namespace sun { namespace star { namespace beans {
	class	XPropertySet;
}}}}

namespace com { namespace sun { namespace star { namespace linguistic2 {
	struct	LinguServiceEvent;
}}}}


namespace linguistic
{

///////////////////////////////////////////////////////////////////////////
// PropertyChgHelper
// Base class for all XPropertyChangeListener members of the
// various lingu services.


// Flags for type of events allowed to be launched
#define	AE_SPELLCHECKER	1
#define	AE_HYPHENATOR 	2
//#define	AE_THESAURUS	4

typedef cppu::WeakImplHelper2
<
    ::com::sun::star::beans::XPropertyChangeListener,
    ::com::sun::star::linguistic2::XLinguServiceEventBroadcaster
> PropertyChgHelperBase;

class PropertyChgHelper :
    public PropertyChgHelperBase
{
	::com::sun::star::uno::Sequence< ::rtl::OUString >  aPropNames;
	::com::sun::star::uno::Reference<
		::com::sun::star::uno::XInterface >				xMyEvtObj;
	::cppu::OInterfaceContainerHelper					aLngSvcEvtListeners;
	::com::sun::star::uno::Reference<
		::com::sun::star::beans::XPropertySet >			xPropSet;

	int		nEvtFlags;	// flags for event types allowed to be launched

	// default values
	BOOL	bIsIgnoreControlCharacters;
	BOOL	bIsUseDictionaryList;
	
	// return values, will be set to default value or current temporary value
	BOOL	bResIsIgnoreControlCharacters;
	BOOL	bResIsUseDictionaryList;
	
	
	// disallow use of copy-constructor and assignment-operator
	PropertyChgHelper( const PropertyChgHelper & );
	PropertyChgHelper & operator = ( const PropertyChgHelper & );

protected:
	virtual void	SetDefaultValues();
	virtual void	GetCurrentValues();
	
	::com::sun::star::uno::Sequence< ::rtl::OUString > &
			GetPropNames()	{ return aPropNames; }
	::com::sun::star::uno::Reference<
		::com::sun::star::beans::XPropertySet > &
			GetPropSet()	{ return xPropSet; }

	void	AddPropNames( const char *pNewNames[], INT32 nCount );
	
	virtual BOOL	propertyChange_Impl( 
							const ::com::sun::star::beans::PropertyChangeEvent& rEvt );

public:
	PropertyChgHelper(
			const ::com::sun::star::uno::Reference<
				::com::sun::star::uno::XInterface > &rxSource,
			::com::sun::star::uno::Reference<
				::com::sun::star::beans::XPropertySet > &rxPropSet,
			int nAllowedEvents );
	virtual ~PropertyChgHelper();

	virtual void	SetTmpPropVals( const com::sun::star::beans::PropertyValues &rPropVals );
	
	// XEventListener
	virtual void SAL_CALL
		disposing( const ::com::sun::star::lang::EventObject& rSource )
			throw(::com::sun::star::uno::RuntimeException);

	// XPropertyChangeListener
	virtual void SAL_CALL
		propertyChange( const ::com::sun::star::beans::PropertyChangeEvent& rEvt )
			throw(::com::sun::star::uno::RuntimeException);

	// XLinguServiceEventBroadcaster
    virtual sal_Bool SAL_CALL
		addLinguServiceEventListener(
				const ::com::sun::star::uno::Reference<
					::com::sun::star::linguistic2::XLinguServiceEventListener >& rxListener )
			throw(::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL
		removeLinguServiceEventListener(
				const ::com::sun::star::uno::Reference<
					::com::sun::star::linguistic2::XLinguServiceEventListener >& rxListener )
			throw(::com::sun::star::uno::RuntimeException);

	// non-UNO functions
	void 	AddAsPropListener();
	void	RemoveAsPropListener();
	void	LaunchEvent(
				const ::com::sun::star::linguistic2::LinguServiceEvent& rEvt );

	const ::com::sun::star::uno::Sequence< ::rtl::OUString > &
			GetPropNames() const	{ return aPropNames; }
	const ::com::sun::star::uno::Reference<
		::com::sun::star::beans::XPropertySet >	&
			GetPropSet() const		{ return xPropSet; }
	const ::com::sun::star::uno::Reference<
		::com::sun::star::uno::XInterface > &
			GetEvtObj() const		{ return xMyEvtObj; }

	BOOL	IsIgnoreControlCharacters() const	{ return bResIsIgnoreControlCharacters; }
	BOOL	IsUseDictionaryList() const			{ return bResIsUseDictionaryList; }
};


///////////////////////////////////////////////////////////////////////////

class PropertyHelper_Thes :
	public PropertyChgHelper
{
	// disallow use of copy-constructor and assignment-operator
	PropertyHelper_Thes( const PropertyHelper_Thes & );
	PropertyHelper_Thes & operator = ( const PropertyHelper_Thes & );

public:
	PropertyHelper_Thes(
			const ::com::sun::star::uno::Reference<
				::com::sun::star::uno::XInterface > &rxSource,
			::com::sun::star::uno::Reference<
				::com::sun::star::beans::XPropertySet > &rxPropSet );
	virtual ~PropertyHelper_Thes();
	
	// XPropertyChangeListener
	virtual void SAL_CALL
		propertyChange( const ::com::sun::star::beans::PropertyChangeEvent& rEvt )
			throw(::com::sun::star::uno::RuntimeException);
};

///////////////////////////////////////////////////////////////////////////

class PropertyHelper_Spell :
	public PropertyChgHelper
{
	// default values
	BOOL	bIsSpellUpperCase;
	BOOL	bIsSpellWithDigits;
	BOOL	bIsSpellCapitalization;

	// return values, will be set to default value or current temporary value
    INT16   nResMaxNumberOfSuggestions; // special value that is not part of the property set and thus needs to be handled differently
	BOOL	bResIsSpellUpperCase;
	BOOL	bResIsSpellWithDigits;
	BOOL	bResIsSpellCapitalization;


	// disallow use of copy-constructor and assignment-operator
	PropertyHelper_Spell( const PropertyHelper_Spell & );
	PropertyHelper_Spell & operator = ( const PropertyHelper_Spell & );

protected:
	// PropertyChgHelper
	virtual void	SetDefaultValues();
	virtual void	GetCurrentValues();
	virtual BOOL	propertyChange_Impl( 
							const ::com::sun::star::beans::PropertyChangeEvent& rEvt );

public:
	PropertyHelper_Spell(
			const ::com::sun::star::uno::Reference<
				::com::sun::star::uno::XInterface > &rxSource,
			::com::sun::star::uno::Reference<
				::com::sun::star::beans::XPropertySet > &rxPropSet );
	virtual ~PropertyHelper_Spell();

	virtual void	SetTmpPropVals( const com::sun::star::beans::PropertyValues &rPropVals );

	// XPropertyChangeListener
	virtual void SAL_CALL
		propertyChange( const ::com::sun::star::beans::PropertyChangeEvent& rEvt )
			throw(::com::sun::star::uno::RuntimeException);

    virtual INT16   GetDefaultNumberOfSuggestions() const;

    INT16   GetMaxNumberOfSuggestions() const   { return nResMaxNumberOfSuggestions; }
	BOOL	IsSpellUpperCase() const			{ return bResIsSpellUpperCase; }
	BOOL	IsSpellWithDigits() const			{ return bResIsSpellWithDigits; }
	BOOL	IsSpellCapitalization() const		{ return bResIsSpellCapitalization; }
};

///////////////////////////////////////////////////////////////////////////

class PropertyHelper_Hyphen :
	public PropertyChgHelper
{
	// default values
	INT16	nHyphMinLeading,
			nHyphMinTrailing,
			nHyphMinWordLength;

	// return values, will be set to default value or current temporary value
	INT16	nResHyphMinLeading,
			nResHyphMinTrailing,
			nResHyphMinWordLength;

	// disallow use of copy-constructor and assignment-operator
    PropertyHelper_Hyphen( const PropertyHelper_Hyphen & );
    PropertyHelper_Hyphen & operator = ( const PropertyHelper_Hyphen & );

protected:
	// PropertyChgHelper
	virtual void	SetDefaultValues();
	virtual void	GetCurrentValues();
	virtual BOOL	propertyChange_Impl( 
							const ::com::sun::star::beans::PropertyChangeEvent& rEvt );

public:
    PropertyHelper_Hyphen(
			const ::com::sun::star::uno::Reference<
				::com::sun::star::uno::XInterface > &rxSource,
			::com::sun::star::uno::Reference<
				::com::sun::star::beans::XPropertySet > &rxPropSet);
    virtual ~PropertyHelper_Hyphen();

	virtual void	SetTmpPropVals( const com::sun::star::beans::PropertyValues &rPropVals );

	// XPropertyChangeListener
	virtual void SAL_CALL
		propertyChange( const ::com::sun::star::beans::PropertyChangeEvent& rEvt )
			throw(::com::sun::star::uno::RuntimeException);

	INT16	GetMinLeading() const				{ return nResHyphMinLeading; }
	INT16	GetMinTrailing() const				{ return nResHyphMinTrailing; }
	INT16	GetMinWordLength() const			{ return nResHyphMinWordLength; }
};

///////////////////////////////////////////////////////////////////////////

}   // namespace linguistic

#endif

