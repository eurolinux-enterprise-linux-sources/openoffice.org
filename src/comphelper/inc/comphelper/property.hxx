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

#ifndef _COMPHELPER_PROPERTY_HXX_
#define _COMPHELPER_PROPERTY_HXX_

#include <cppuhelper/proptypehlp.hxx>
#include <comphelper/extract.hxx>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <functional>
#include "comphelper/comphelperdllapi.h"
#include "cppu/unotype.hxx"

//=========================================================================
//= property helper classes
//=========================================================================

//... namespace comphelper .......................................................
namespace comphelper
{
//.........................................................................

	namespace starbeans	= ::com::sun::star::beans;
	namespace staruno	= ::com::sun::star::uno;

/** compare two properties by name
*/
	struct PropertyStringLessFunctor : ::std::binary_function< ::com::sun::star::beans::Property, ::rtl::OUString, bool > 
	{
		// ................................................................
		inline bool operator()( const ::com::sun::star::beans::Property& lhs, const ::rtl::OUString& rhs ) const
		{
			return lhs.Name.compareTo(rhs) < 0;
		}
		// ................................................................
		inline bool operator()( const ::rtl::OUString& lhs, const ::com::sun::star::beans::Property& rhs ) const
		{
			return lhs.compareTo(rhs.Name) < 0;
		}
	};
	//--------------------------------------------------------------------------
	// comparing two property instances
	struct PropertyCompareByName : public ::std::binary_function< ::com::sun::star::beans::Property, ::com::sun::star::beans::Property, bool >
	{
		bool operator() (const ::com::sun::star::beans::Property& x, const ::com::sun::star::beans::Property& y) const
		{
			return x.Name.compareTo(y.Name) < 0;// ? true : false;
		}
	};

	//--------------------------------------------------------------------------
    /** compare two properties by name
     */
	struct PropertyStringEqualFunctor : ::std::binary_function< ::com::sun::star::beans::Property, ::rtl::OUString, bool > 
	{
		// ................................................................
		inline bool operator()( const ::com::sun::star::beans::Property& lhs, const ::rtl::OUString& rhs ) const
		{
			return lhs.Name.compareTo(rhs) == 0;
		}
		// ................................................................
		inline bool operator()( const ::rtl::OUString& lhs, const ::com::sun::star::beans::Property& rhs ) const
		{
			return lhs.compareTo(rhs.Name) == 0;
		}
	};
	//--------------------------------------------------------------------------
	// comparing two property instances
	struct PropertyEqualByName : public ::std::binary_function< ::com::sun::star::beans::Property, ::com::sun::star::beans::Property, bool >
	{
		bool operator() (const ::com::sun::star::beans::Property& x, const ::com::sun::star::beans::Property& y) const
		{
			return x.Name.compareTo(y.Name) == 0;
		}
	};

//------------------------------------------------------------------
/** find property with given name

    @param o_rProp
    Output parameter receiving the property, if found

    @param i_rPropName
    Name of the property to find

    @return false, if property was not found
 */
COMPHELPER_DLLPUBLIC bool findProperty(starbeans::Property& o_rProp, staruno::Sequence<starbeans::Property>& i_seqProps, const ::rtl::OUString& i_rPropName);

//------------------------------------------------------------------
/// remove the property with the given name from the given sequence
COMPHELPER_DLLPUBLIC void RemoveProperty(staruno::Sequence<starbeans::Property>& seqProps, const ::rtl::OUString& _rPropName);

//------------------------------------------------------------------
/** within the given property sequence, modify attributes of a special property
	@param	_rProps			the sequence of properties to search in
	@param	_sPropName		the name of the property which's attributes should be modified
	@param	_nAddAttrib		the attributes which should be added
	@param	_nRemoveAttrib	the attributes which should be removed
*/
COMPHELPER_DLLPUBLIC void ModifyPropertyAttributes(staruno::Sequence<starbeans::Property>& _rProps, const ::rtl::OUString& _sPropName, sal_Int16 _nAddAttrib, sal_Int16 _nRemoveAttrib);

//------------------------------------------------------------------
/**	check if the given set has the given property.
*/
COMPHELPER_DLLPUBLIC sal_Bool hasProperty(const rtl::OUString& _rName, const staruno::Reference<starbeans::XPropertySet>& _rxSet);

//------------------------------------------------------------------
/**	copy properties between property sets, in compliance with the property
	attributes of the target object
*/
COMPHELPER_DLLPUBLIC void copyProperties(const staruno::Reference<starbeans::XPropertySet>& _rxSource,
					const staruno::Reference<starbeans::XPropertySet>& _rxDest);

//==================================================================
//= property conversion helpers
//==================================================================

/** helper for implementing ::cppu::OPropertySetHelper::convertFastPropertyValue
	@param			_rConvertedValue	the conversion result (if successfull)
	@param			_rOldValue			the old value of the property, calculated from _rCurrentValue
	@param			_rValueToSet		the new value which is about to be set
	@param			_rCurrentValue		the current value of the property
	@return			sal_True, if the value could be converted and has changed
					sal_False, if the value could be converted and has not changed
	@exception		InvalidArgumentException thrown if the value could not be converted to the requested type (which is the template argument)
*/
template <class TYPE>
sal_Bool tryPropertyValue(staruno::Any& /*out*/_rConvertedValue, staruno::Any& /*out*/_rOldValue, const staruno::Any& _rValueToSet, const TYPE& _rCurrentValue)
{
	sal_Bool bModified(sal_False);
	TYPE aNewValue;
	::cppu::convertPropertyValue(aNewValue, _rValueToSet);
	if (aNewValue != _rCurrentValue)
	{
		_rConvertedValue <<= aNewValue;
		_rOldValue <<= _rCurrentValue;
		bModified = sal_True;
	}
	return bModified;
}

/** helper for implementing ::cppu::OPropertySetHelper::convertFastPropertyValue for enum values
	@param			_rConvertedValue	the conversion result (if successfull)
	@param			_rOldValue			the old value of the property, calculated from _rCurrentValue
	@param			_rValueToSet		the new value which is about to be set
	@param			_rCurrentValue		the current value of the property
	@return			sal_True, if the value could be converted and has changed
					sal_False, if the value could be converted and has not changed
	@exception		InvalidArgumentException thrown if the value could not be converted to the requested type (which is the template argument)
*/
template <class ENUMTYPE>
sal_Bool tryPropertyValueEnum(staruno::Any& /*out*/_rConvertedValue, staruno::Any& /*out*/_rOldValue, const staruno::Any& _rValueToSet, const ENUMTYPE& _rCurrentValue)
{
    if (cppu::getTypeFavourUnsigned(&_rCurrentValue).getTypeClass()
        != staruno::TypeClass_ENUM)
		return tryPropertyValue(_rConvertedValue, _rOldValue, _rValueToSet, _rCurrentValue);

	sal_Bool bModified(sal_False);
	ENUMTYPE aNewValue;
	::cppu::any2enum(aNewValue, _rValueToSet);
		// will throw an exception if not convertible

	if (aNewValue != _rCurrentValue)
	{
		_rConvertedValue <<= aNewValue;
		_rOldValue <<= _rCurrentValue;
		bModified = sal_True;
	}
	return bModified;
}

/** helper for implementing ::cppu::OPropertySetHelper::convertFastPropertyValue for boolean properties
	@param			_rConvertedValue	the conversion result (if successfull)
	@param			_rOldValue			the old value of the property, calculated from _rCurrentValue
	@param			_rValueToSet		the new value which is about to be set
	@param			_rCurrentValue		the current value of the property
	@return			sal_True, if the value could be converted and has changed
					sal_False, if the value could be converted and has not changed
	@exception		InvalidArgumentException thrown if the value could not be converted to a boolean type
*/
inline sal_Bool tryPropertyValue(staruno::Any& /*out*/_rConvertedValue, staruno::Any& /*out*/_rOldValue, const staruno::Any& _rValueToSet, sal_Bool _bCurrentValue)
{
	sal_Bool bModified(sal_False);
	sal_Bool bNewValue;
	::cppu::convertPropertyValue(bNewValue, _rValueToSet);
	if (bNewValue != _bCurrentValue)
	{
		_rConvertedValue.setValue(&bNewValue, ::getBooleanCppuType());
		_rOldValue.setValue(&_bCurrentValue, ::getBooleanCppuType());
		bModified = sal_True;
	}
	return bModified;
}

/** helper for implementing ::cppu::OPropertySetHelper::convertFastPropertyValue
	@param			_rConvertedValue	the conversion result (if successfull)
	@param			_rOldValue			the old value of the property, calculated from _rCurrentValue
	@param			_rValueToSet		the new value which is about to be set
	@param			_rCurrentValue		the current value of the property
	@param			_rExpectedType		the type which the property should have (if not void)
	@return			sal_True, if the value could be converted and has changed
					sal_False, if the value could be converted and has not changed
	@exception		InvalidArgumentException thrown if the value could not be converted to the requested type (which is the template argument)
*/
COMPHELPER_DLLPUBLIC sal_Bool tryPropertyValue(staruno::Any& _rConvertedValue, staruno::Any& _rOldValue, const staruno::Any& _rValueToSet, const staruno::Any& _rCurrentValue, const staruno::Type& _rExpectedType);

//.........................................................................
}
//... namespace comphelper .......................................................

#endif // _COMPHELPER_PROPERTY_HXX_

