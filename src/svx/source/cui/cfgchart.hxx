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

#ifndef _SVX_CFGCHART_HXX
#define _SVX_CFGCHART_HXX

// header for TYPEINFO
#include <tools/rtti.hxx>
// header for ConfigItem
#include <unotools/configitem.hxx>
// header for SfxPoolItem
#include <svtools/poolitem.hxx>
// header for XColorEntry
#include <svx/xtable.hxx>

#include <vector>

class SvxChartColorTable
{
private:
    ::std::vector< XColorEntry >     m_aColorEntries;

public:
    SvxChartColorTable();
    explicit SvxChartColorTable( const SvxChartColorTable & _rSource );

    // accessors
    size_t size() const;
    const XColorEntry & operator[]( size_t _nIndex ) const;
    ColorData getColorData( size_t _nIndex ) const;

    // mutators
    void clear();
    void append( const XColorEntry & _rEntry );
	void replace( size_t _nIndex, const XColorEntry & _rEntry );
    void useDefault();

    // comparison
    bool operator==( const SvxChartColorTable & _rOther ) const;
};

// ====================
// all options
// ====================
class SvxChartOptions : public ::utl::ConfigItem
{
private:
	SvxChartColorTable		maDefColors;
	BOOL					mbIsInitialized;

	::com::sun::star::uno::Sequence< ::rtl::OUString >
							maPropertyNames;

	inline ::com::sun::star::uno::Sequence< ::rtl::OUString > GetPropertyNames() const
		{ return maPropertyNames; }
	BOOL RetrieveOptions();

public:
	SvxChartOptions();
	virtual ~SvxChartOptions();

	const SvxChartColorTable&	GetDefaultColors();
	void						SetDefaultColors( const SvxChartColorTable& aCol );

	virtual void				Commit();
};

// ====================
// items
// ====================
class SvxChartColorTableItem : public SfxPoolItem
{
public:
	TYPEINFO();
	SvxChartColorTableItem( USHORT nWhich, const SvxChartColorTable& );
	SvxChartColorTableItem( const SvxChartColorTableItem& );

	virtual SfxPoolItem*	Clone( SfxItemPool *pPool = 0 ) const;
	virtual int 			operator==( const SfxPoolItem& ) const;
	void					SetOptions( SvxChartOptions* pOpts ) const;

    const SvxChartColorTable & GetColorTable() const ;
    SvxChartColorTable &       GetColorTable();
	void                    ReplaceColorByIndex( size_t _nIndex, const XColorEntry & _rEntry );

private:
    SvxChartColorTable      m_aColorTable;
};

#endif	// _SVX_CFGCHART_HXX

