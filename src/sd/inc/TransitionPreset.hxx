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

#ifndef _SD_TRANSITIONPRESET_HXX
#define _SD_TRANSITIONPRESET_HXX

#ifndef BOOST_SHARED_PTR_HPP_INCLUDED
#include <boost/shared_ptr.hpp>
#endif
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#ifndef _UTL_STLTYPES_HXX_
#include <comphelper/stl_types.hxx>
#endif

#include <list>
#include <hash_map>

namespace com { namespace sun { namespace star { 
	namespace animations { class XAnimationNode; }
	namespace uno { template<class X> class Reference; }
} } }

class SdPage;
class String;

namespace sd {

class TransitionPreset;
typedef boost::shared_ptr< TransitionPreset > TransitionPresetPtr;
typedef std::list< TransitionPresetPtr > TransitionPresetList;
typedef std::hash_map< rtl::OUString, rtl::OUString, comphelper::UStringHash, comphelper::UStringEqual > UStringMap;

class TransitionPreset
{
public:
	static const TransitionPresetList& getTransitionPresetList();
	static bool importTransitionPresetList( TransitionPresetList& rList );

	void apply( SdPage* pSlide ) const;

	sal_Int16 getTransition() const { return mnTransition; }
	sal_Int16 getSubtype() const { return mnSubtype; }
	sal_Bool getDirection() const { return mbDirection; }
	sal_Int32 getFadeColor() const { return mnFadeColor; }

	const rtl::OUString& getUIName() const { return maUIName; }
	const rtl::OUString& getPresetId() const { return maPresetId; }

private:
	TransitionPreset( const ::com::sun::star::uno::Reference< ::com::sun::star::animations::XAnimationNode >& xNode );

	sal_Int16 mnTransition;
	sal_Int16 mnSubtype;
	sal_Bool mbDirection;
	sal_Int32 mnFadeColor;
	rtl::OUString maPresetId;
	rtl::OUString maUIName;

	static sd::TransitionPresetList* mpTransitionPresetList;

    static bool importTransitionsFile( TransitionPresetList& rList,
                                       ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xServiceFactory,
                                       UStringMap& rTransitionNameMape,
                                       String aFilaname );
};

}

#endif // _SD_TRANSITIONPRESET_HXX

