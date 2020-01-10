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
#ifndef _SVX_ASIANCFG_HXX
#define _SVX_ASIANCFG_HXX

#include <unotools/configitem.hxx>
#include <com/sun/star/uno/Sequence.h>
#include "svx/svxdllapi.h"

namespace com{namespace sun{namespace star{
namespace lang{
    struct Locale;
}}}}
//-----------------------------------------------------------------------------
struct SvxAsianConfig_Impl;
class SVX_DLLPUBLIC SvxAsianConfig : public utl::ConfigItem
{
    SvxAsianConfig_Impl* pImpl;

public:
    SvxAsianConfig(sal_Bool bEnableNotify = sal_True);
    virtual ~SvxAsianConfig();

    void            Load();
    virtual void    Commit();
    virtual void    Notify( const com::sun::star::uno::Sequence<rtl::OUString>& aPropertyNames);

    sal_Bool    IsKerningWesternTextOnly() const;
    void        SetKerningWesternTextOnly(sal_Bool bSet);

    sal_Int16   GetCharDistanceCompression() const;
    void        SetCharDistanceCompression(sal_Int16 nSet);

    com::sun::star::uno::Sequence<com::sun::star::lang::Locale>
                GetStartEndCharLocales();

    sal_Bool    GetStartEndChars( const com::sun::star::lang::Locale& rLocale,
                                    rtl::OUString& rStartChars,
                                    rtl::OUString& rEndChars );
    void        SetStartEndChars( const com::sun::star::lang::Locale& rLocale,
                                    const rtl::OUString* pStartChars,
                                    const rtl::OUString* pEndChars );
};

#endif
