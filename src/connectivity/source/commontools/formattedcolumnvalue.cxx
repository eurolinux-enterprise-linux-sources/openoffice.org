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
#include "precompiled_connectivity.hxx"

#include "connectivity/formattedcolumnvalue.hxx"
#include "connectivity/dbtools.hxx"
#include "connectivity/dbconversion.hxx"

/** === begin UNO includes === **/
#include <com/sun/star/util/XNumberFormatter.hpp>
#include <com/sun/star/util/Date.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/util/XNumberFormatTypes.hpp>
#include <com/sun/star/util/NumberFormat.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
/** === end UNO includes === **/

#include <svtools/syslocale.hxx>

#include <tools/diagnose_ex.h>

#include <i18npool/mslangid.hxx>

#include <comphelper/numbers.hxx>
#include <comphelper/componentcontext.hxx>

//........................................................................
namespace dbtools
{
//........................................................................

	/** === begin UNO using === **/
	using ::com::sun::star::uno::Reference;
	using ::com::sun::star::uno::UNO_QUERY;
	using ::com::sun::star::uno::UNO_QUERY_THROW;
	using ::com::sun::star::uno::Exception;
	using ::com::sun::star::uno::RuntimeException;
	using ::com::sun::star::uno::Any;
	using ::com::sun::star::uno::makeAny;
    using ::com::sun::star::sdbc::XRowSet;
    using ::com::sun::star::beans::XPropertySet;
    using ::com::sun::star::util::XNumberFormatter;
    using ::com::sun::star::util::Date;
    using ::com::sun::star::sdbc::XConnection;
    using ::com::sun::star::util::XNumberFormatsSupplier;
    using ::com::sun::star::beans::XPropertySetInfo;
    using ::com::sun::star::lang::Locale;
    using ::com::sun::star::util::XNumberFormatTypes;
    using ::com::sun::star::sdb::XColumn;
    using ::com::sun::star::sdb::XColumnUpdate;
    using ::com::sun::star::lang::XComponent;
	/** === end UNO using === **/
    namespace DataType = ::com::sun::star::sdbc::DataType;
    namespace NumberFormat = ::com::sun::star::util::NumberFormat;

	//====================================================================
	//= FormattedColumnValue_Data
	//====================================================================
    struct FormattedColumnValue_Data
    {
        Reference< XNumberFormatter >   m_xFormatter;
        Date                            m_aNullDate;
        sal_Int32                       m_nFormatKey;
        sal_Int32                       m_nFieldType;
        sal_Int16                       m_nKeyType;
        bool                            m_bNumericField;

        Reference< XColumn >            m_xColumn;
        Reference< XColumnUpdate >      m_xColumnUpdate;

        FormattedColumnValue_Data()
            :m_xFormatter()
            ,m_aNullDate( DBTypeConversion::getStandardDate() )
            ,m_nFormatKey( 0 )
	        ,m_nFieldType( DataType::OTHER )
	        ,m_nKeyType( NumberFormat::UNDEFINED )
            ,m_bNumericField( false )
            ,m_xColumn()
            ,m_xColumnUpdate()
        {
        }
    };

	//--------------------------------------------------------------------
    namespace
    {
	    //................................................................
        void lcl_clear_nothrow( FormattedColumnValue_Data& _rData )
        {
            if ( _rData.m_xFormatter.is() )
            {
                try
                {
                    Reference< XComponent > xFormatterComp( _rData.m_xFormatter, UNO_QUERY );
                    if ( xFormatterComp.is() )
                        xFormatterComp->dispose();
                }
                catch( const Exception& )
                {
    	            DBG_UNHANDLED_EXCEPTION();
                }
                _rData.m_xFormatter.clear();
            }

            _rData.m_nFormatKey = 0;
            _rData.m_nFieldType = DataType::OTHER;
            _rData.m_nKeyType = NumberFormat::UNDEFINED;
            _rData.m_bNumericField = false;

            _rData.m_xColumn.clear();
            _rData.m_xColumnUpdate.clear();
        }

        //................................................................
        void lcl_initColumnDataValue_nothrow( const ::comphelper::ComponentContext& _rContext, FormattedColumnValue_Data& _rData,
            const Reference< XRowSet >& _rxRowSet, const Reference< XPropertySet >& _rxColumn )
        {
            lcl_clear_nothrow( _rData );

            OSL_PRECOND( _rxRowSet.is(), "lcl_initColumnDataValue_nothrow: no row set!" );
            OSL_PRECOND( _rxColumn.is(), "lcl_initColumnDataValue_nothrow: no column!" );
            if ( !_rxRowSet.is() || !_rxColumn.is() )
                return;

            try
            {
                _rData.m_xColumn.set( _rxColumn, UNO_QUERY_THROW );
                _rData.m_xColumnUpdate.set( _rxColumn, UNO_QUERY );

                OSL_VERIFY( _rxColumn->getPropertyValue( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Type" ) ) ) >>= _rData.m_nFieldType );

                switch ( _rData.m_nFieldType )
                {
                    case DataType::DATE:
                    case DataType::TIME:
                    case DataType::TIMESTAMP:
                    case DataType::BIT:
                    case DataType::BOOLEAN:
                    case DataType::TINYINT:
                    case DataType::SMALLINT:
                    case DataType::INTEGER:
                    case DataType::REAL:
                    case DataType::BIGINT:
                    case DataType::DOUBLE:
                    case DataType::NUMERIC:
                    case DataType::DECIMAL:
                        _rData.m_bNumericField = true;
                        break;
                    default:
                        _rData.m_bNumericField = false;
                        break;
                }

                // get the number formats supplier of the connection of the form
                Reference< XConnection > xConnection( getConnection( _rxRowSet ), UNO_QUERY_THROW );
                Reference< XNumberFormatsSupplier > xSupplier( getNumberFormats( xConnection, sal_False, _rContext.getLegacyServiceFactory() ), UNO_QUERY_THROW );

                // get the format key of our bound field
                Reference< XPropertySetInfo > xPSI( _rxColumn->getPropertySetInfo(), UNO_QUERY_THROW );
                bool bHaveFieldFormat = false;
                const ::rtl::OUString sFormatKeyProperty( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "FormatKey" ) ) );
                if ( xPSI->hasPropertyByName( sFormatKeyProperty ) )
                {
                    bHaveFieldFormat = ( _rxColumn->getPropertyValue( sFormatKeyProperty ) >>= _rData.m_nFormatKey );
                }
                if ( !bHaveFieldFormat )
                {
                    // fall back to a format key as indicated by the field type
                    Locale aSystemLocale;
                    MsLangId::convertLanguageToLocale( MsLangId::getSystemLanguage(), aSystemLocale );
                    Reference< XNumberFormatTypes > xNumTypes( xSupplier->getNumberFormats(), UNO_QUERY_THROW );
                    _rData.m_nFormatKey = getDefaultNumberFormat( _rxColumn, xNumTypes, aSystemLocale );
                }

                // some more formatter settings
                _rData.m_nKeyType  = ::comphelper::getNumberFormatType( xSupplier->getNumberFormats(), _rData.m_nFormatKey );
                Reference< XPropertySet > xFormatSettings( xSupplier->getNumberFormatSettings(), UNO_QUERY_THROW );
                OSL_VERIFY( xFormatSettings->getPropertyValue( ::rtl::OUString::createFromAscii( "NullDate" ) ) >>= _rData.m_aNullDate );

                // create a formatter working with the connection's number format supplier
                _rData.m_xFormatter.set( _rContext.createComponent( "com.sun.star.util.NumberFormatter" ), UNO_QUERY_THROW );
		        _rData.m_xFormatter->attachNumberFormatsSupplier( xSupplier );
            }
            catch( const Exception& )
            {
    	        DBG_UNHANDLED_EXCEPTION();
            }
        }
    }

	//====================================================================
	//= FormattedColumnValue
	//====================================================================
	//--------------------------------------------------------------------
    FormattedColumnValue::FormattedColumnValue( const ::comphelper::ComponentContext& _rContext, 
            const Reference< XRowSet >& _rxRowSet, const Reference< XPropertySet >& _rxColumn )
        :m_pData( new FormattedColumnValue_Data )
    {
        lcl_initColumnDataValue_nothrow( _rContext, *m_pData, _rxRowSet, _rxColumn );
    }

	//--------------------------------------------------------------------
    void FormattedColumnValue::clear()
    {
        lcl_clear_nothrow( *m_pData );
    }

	//--------------------------------------------------------------------
    FormattedColumnValue::~FormattedColumnValue()
    {
        clear();
    }

    //--------------------------------------------------------------------
    sal_Int32 FormattedColumnValue::getFormatKey() const
    {
        return m_pData->m_nFormatKey;
    }

    //--------------------------------------------------------------------
    sal_Int32 FormattedColumnValue::getFieldType() const
    {
        return m_pData->m_nFieldType;
    }

    //--------------------------------------------------------------------
    sal_Int16 FormattedColumnValue::getKeyType() const
    {
        return m_pData->m_nKeyType;
    }

    //--------------------------------------------------------------------
    bool FormattedColumnValue::isNumericField() const
    {
        return m_pData->m_bNumericField;
    }

	//--------------------------------------------------------------------
    const Reference< XColumn >& FormattedColumnValue::getColumn() const
    {
        return m_pData->m_xColumn;
    }

	//--------------------------------------------------------------------
    const Reference< XColumnUpdate >& FormattedColumnValue::getColumnUpdate() const
    {
        return m_pData->m_xColumnUpdate;
    }

	//--------------------------------------------------------------------
    bool FormattedColumnValue::setFormattedValue( const ::rtl::OUString& _rFormattedStringValue ) const
    {
        OSL_PRECOND( m_pData->m_xColumnUpdate.is(), "FormattedColumnValue::setFormattedValue: no column!" );
        if ( !m_pData->m_xColumnUpdate.is() )
            return false;

        try
        {
            if ( m_pData->m_bNumericField )
            {
                ::dbtools::DBTypeConversion::setValue( m_pData->m_xColumnUpdate, m_pData->m_xFormatter, m_pData->m_aNullDate,
                    _rFormattedStringValue, m_pData->m_nFormatKey, ::sal::static_int_cast< sal_Int16 >( m_pData->m_nFieldType ),
                    m_pData->m_nKeyType );
            }
            else
            {
                m_pData->m_xColumnUpdate->updateString( _rFormattedStringValue );
            }
        }
        catch( const Exception& )
        {
            return false;
        }
        return true;
    }

	//--------------------------------------------------------------------
    ::rtl::OUString FormattedColumnValue::getFormattedValue() const
    {
        OSL_PRECOND( m_pData->m_xColumn.is(), "FormattedColumnValue::setFormattedValue: no column!" );

        ::rtl::OUString sStringValue;
        if ( m_pData->m_xColumn.is() )
        {
            sStringValue = DBTypeConversion::getValue(
                m_pData->m_xColumn, m_pData->m_xFormatter, m_pData->m_aNullDate, m_pData->m_nFormatKey, m_pData->m_nKeyType
            );
        }
        return sStringValue;
    }

//........................................................................
} // namespace dbtools
//........................................................................
