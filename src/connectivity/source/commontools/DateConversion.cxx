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


#include "connectivity/dbconversion.hxx"
#include <connectivity/dbtools.hxx>
#include <com/sun/star/script/XTypeConverter.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/util/NumberFormat.hpp>
#include <com/sun/star/util/XNumberFormatTypes.hpp>
#include <com/sun/star/sdb/XColumnUpdate.hpp>
#include <com/sun/star/sdb/XColumn.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <comphelper/extract.hxx>
#include "TConnection.hxx"
#include "diagnose_ex.h"
#include <comphelper/numbers.hxx>
#include <rtl/ustrbuf.hxx>


using namespace ::connectivity;
using namespace ::comphelper;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::dbtools;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::beans;
// -----------------------------------------------------------------------------
::rtl::OUString DBTypeConversion::toSQLString(sal_Int32 eType, const Any& _rVal, sal_Bool bQuote,
											  const Reference< XTypeConverter >&  _rxTypeConverter)
{
	::rtl::OUStringBuffer aRet;
	if (_rVal.hasValue())
	{
		try
		{
			switch (eType)
			{
				case DataType::INTEGER:
				case DataType::BIT:
				case DataType::BOOLEAN:
				case DataType::TINYINT:
				case DataType::SMALLINT:
					if (_rVal.getValueType().getTypeClass() == ::com::sun::star::uno::TypeClass_BOOLEAN)
					{
						if (::cppu::any2bool(_rVal))
							aRet.appendAscii("1");
						else
							aRet.appendAscii("0");
					}
					else
                    {
                        ::rtl::OUString sTemp;
					    _rxTypeConverter->convertToSimpleType(_rVal, TypeClass_STRING) >>= sTemp;
                        aRet.append(sTemp);
                    }
					break;
				case DataType::CHAR:
				case DataType::VARCHAR:
                case DataType::LONGVARCHAR:
					if (bQuote)
						aRet.appendAscii("'");
					{
						::rtl::OUString aTemp;
						_rxTypeConverter->convertToSimpleType(_rVal, TypeClass_STRING) >>= aTemp;
						sal_Int32 nIndex = (sal_Int32)-1;
						const ::rtl::OUString sQuot(RTL_CONSTASCII_USTRINGPARAM("\'"));
						const ::rtl::OUString sQuotToReplace(RTL_CONSTASCII_USTRINGPARAM("\'\'"));
						do
						{
							nIndex += 2;
							nIndex = aTemp.indexOf(sQuot,nIndex);
							if(nIndex != -1)
								aTemp = aTemp.replaceAt(nIndex,sQuot.getLength(),sQuotToReplace);
						} while (nIndex != -1);

						aRet.append(aTemp);
					}
					if (bQuote)
						aRet.appendAscii("'");
					break;
				case DataType::REAL:
				case DataType::DOUBLE:
				case DataType::DECIMAL:
				case DataType::NUMERIC:
				case DataType::BIGINT:
                default:
                    {
                        ::rtl::OUString sTemp;
					    _rxTypeConverter->convertToSimpleType(_rVal, TypeClass_STRING) >>= sTemp;
                        aRet.append(sTemp);
                    }
					break;
				case DataType::TIMESTAMP:
				{
					DateTime aDateTime;

					// check if this is really a timestamp or only a date
					if ( _rVal >>= aDateTime )
					{
						if (bQuote) 
                            aRet.appendAscii("{TS '");
						aRet.append(DBTypeConversion::toDateTimeString(aDateTime));
						if (bQuote) 
                            aRet.appendAscii("'}");
						break;
					}
					break;
				}
				case DataType::DATE:
				{
					Date aDate;
                    OSL_VERIFY_RES( _rVal >>= aDate, "DBTypeConversion::toSQLString: _rVal is not date!");
					if (bQuote) 
                        aRet.appendAscii("{D '");
					aRet.append(DBTypeConversion::toDateString(aDate));
					if (bQuote) 
                        aRet.appendAscii("'}");
				}	break;
				case DataType::TIME:
				{
					Time aTime;
                    OSL_VERIFY_RES( _rVal >>= aTime,"DBTypeConversion::toSQLString: _rVal is not time!");
					if (bQuote) 
                        aRet.appendAscii("{T '");
					aRet.append(DBTypeConversion::toTimeString(aTime));
					if (bQuote) 
                        aRet.appendAscii("'}");
				} break;
			}
		}
		catch ( const Exception&  )
		{
			OSL_ENSURE(0,"TypeConversion Error");
		}
	}
	else
		aRet.appendAscii(" NULL ");
	return aRet.makeStringAndClear();
}
// -----------------------------------------------------------------------------
Date DBTypeConversion::getNULLDate(const Reference< XNumberFormatsSupplier > &xSupplier)
{
	OSL_ENSURE(xSupplier.is(), "getNULLDate : the formatter doesn't implement a supplier !");
	if (xSupplier.is())
	{
		try
		{
			// get the null date
			Date aDate;
			xSupplier->getNumberFormatSettings()->getPropertyValue(::rtl::OUString::createFromAscii("NullDate")) >>= aDate;
			return aDate;
		}
		catch ( const Exception&  )
		{
		}
	}

	return getStandardDate();
}
// -----------------------------------------------------------------------------
void DBTypeConversion::setValue(const Reference<XColumnUpdate>& xVariant,
								const Reference<XNumberFormatter>& xFormatter,
								const Date& rNullDate,
								const ::rtl::OUString& rString,
								sal_Int32 nKey,
								sal_Int16 nFieldType,
								sal_Int16 nKeyType) throw(::com::sun::star::lang::IllegalArgumentException)
{
	double fValue = 0;
	if (rString.getLength())
	{
			// Muss der String formatiert werden?
		sal_Int16 nTypeClass = nKeyType & ~NumberFormat::DEFINED;
		sal_Bool bTextFormat = nTypeClass == NumberFormat::TEXT;
		sal_Int32 nKeyToUse  = bTextFormat ? 0 : nKey;
		sal_Int16 nRealUsedTypeClass = nTypeClass;
			// bei einem Text-Format muessen wir dem Formatter etwas mehr Freiheiten einraeumen, sonst
			// wirft convertStringToNumber eine NotNumericException
		try
		{
			fValue = xFormatter->convertStringToNumber(nKeyToUse, rString);
			sal_Int32 nRealUsedKey = xFormatter->detectNumberFormat(0, rString);
			if (nRealUsedKey != nKeyToUse)
				nRealUsedTypeClass = getNumberFormatType(xFormatter, nRealUsedKey) & ~NumberFormat::DEFINED;

			// und noch eine Sonderbehandlung, diesmal fuer Prozent-Formate
			if ((NumberFormat::NUMBER == nRealUsedTypeClass) && (NumberFormat::PERCENT == nTypeClass))
			{	// die Formatierung soll eigentlich als Prozent erfolgen, aber der String stellt nur eine
				// einfache Nummer dar -> anpassen
				::rtl::OUString sExpanded(rString);
				static ::rtl::OUString s_sPercentSymbol = ::rtl::OUString::createFromAscii("%");
					// need a method to add a sal_Unicode to a string, 'til then we use a static string
				sExpanded += s_sPercentSymbol;
				fValue = xFormatter->convertStringToNumber(nKeyToUse, sExpanded);
			}

			switch (nRealUsedTypeClass)
			{
				case NumberFormat::DATE:
				case NumberFormat::DATETIME:
				case NumberFormat::TIME:
					DBTypeConversion::setValue(xVariant,rNullDate,fValue,nRealUsedTypeClass);
					//	xVariant->updateDouble(toStandardDbDate(rNullDate, fValue));
					break;
				case NumberFormat::CURRENCY:
				case NumberFormat::NUMBER:
				case NumberFormat::SCIENTIFIC:
				case NumberFormat::FRACTION:
				case NumberFormat::PERCENT:
					xVariant->updateDouble(fValue);
					break;
				default:
					xVariant->updateString(rString);
			}
		}
		catch(const Exception& )
		{
			xVariant->updateString(rString);
		}
	}
	else
	{
		switch (nFieldType)
		{
			case ::com::sun::star::sdbc::DataType::CHAR:
			case ::com::sun::star::sdbc::DataType::VARCHAR:
			case ::com::sun::star::sdbc::DataType::LONGVARCHAR:
				xVariant->updateString(rString);
				break;
			default:
				xVariant->updateNull();
		}
	}
}

//------------------------------------------------------------------------------
void DBTypeConversion::setValue(const Reference<XColumnUpdate>& xVariant,
								const Date& rNullDate,
								const double& rValue,
								sal_Int16 nKeyType) throw(::com::sun::star::lang::IllegalArgumentException)
{
	switch (nKeyType & ~NumberFormat::DEFINED)
	{
		case NumberFormat::DATE:
			xVariant->updateDate(toDate( rValue, rNullDate));
			break;
		case NumberFormat::DATETIME:
			xVariant->updateTimestamp(toDateTime(rValue,rNullDate));
			break;
		case NumberFormat::TIME:
			xVariant->updateTime(toTime(rValue));
			break;
		default:
			{
				double nValue = rValue;
//				Reference<XPropertySet> xProp(xVariant,UNO_QUERY);
//				if (	xProp.is()
//					&&	xProp->getPropertySetInfo()->hasPropertyByName(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISSIGNED))
//					&& !::comphelper::getBOOL(xProp->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISSIGNED))) )
//				{
//					switch (::comphelper::getINT32(xProp->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE))))
//					{
//						case DataType::TINYINT:
//							nValue = static_cast<sal_uInt8>(rValue);
//							break;
//						case DataType::SMALLINT:
//							nValue = static_cast<sal_uInt16>(rValue);
//							break;
//						case DataType::INTEGER:
//							nValue = static_cast<sal_uInt32>(rValue);
//							break;
//						case DataType::BIGINT:
//							nValue = static_cast<sal_uInt64>(rValue);
//							break;
//					}
//				}
				xVariant->updateDouble(nValue);
			}
	}
}

//------------------------------------------------------------------------------
double DBTypeConversion::getValue(const Reference<XColumn>& xVariant,
								  const Date& rNullDate,
								  sal_Int16 nKeyType)
{
	try
	{
		switch (nKeyType & ~NumberFormat::DEFINED)
		{
			case NumberFormat::DATE:
				return toDouble( xVariant->getDate(), rNullDate);
			case NumberFormat::DATETIME:
				return toDouble(xVariant->getTimestamp(),rNullDate);
			case NumberFormat::TIME:
				return toDouble(xVariant->getTime());
			default:
			{
				Reference<XPropertySet> xProp(xVariant,UNO_QUERY);
				if (	xProp.is()
					&&	xProp->getPropertySetInfo()->hasPropertyByName(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISSIGNED))
					&& !::comphelper::getBOOL(xProp->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISSIGNED))) )
				{
					switch (::comphelper::getINT32(xProp->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE))))
					{
						case DataType::TINYINT:
							return static_cast<double>(static_cast<sal_uInt8>(xVariant->getByte()));
						case DataType::SMALLINT:
							return static_cast<double>(static_cast<sal_uInt16>(xVariant->getShort()));
						case DataType::INTEGER:
							return static_cast<double>(static_cast<sal_uInt32>(xVariant->getInt()));
						case DataType::BIGINT:
							return static_cast<double>(static_cast<sal_uInt64>(xVariant->getLong()));
					}
				}

				return xVariant->getDouble();
			}
		}
	}
	catch(const Exception& )
	{
		return 0.0;
	}
}
//------------------------------------------------------------------------------
::rtl::OUString DBTypeConversion::getValue(const Reference< XPropertySet>& _xColumn,
										   const Reference<XNumberFormatter>& _xFormatter,
										   const ::com::sun::star::lang::Locale& _rLocale,
										   const Date& _rNullDate)
{
	OSL_ENSURE(_xColumn.is() && _xFormatter.is(), "DBTypeConversion::getValue: invalid arg !");
	if (!_xColumn.is() || !_xFormatter.is())
		return ::rtl::OUString();

	sal_Int32 nKey(0);
	try
	{
		_xColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_FORMATKEY)) >>= nKey;
	}
	catch (const Exception& )
	{
        OSL_ENSURE(false, "DBTypeConversion::getValue: caught an exception while asking for the format key!");
	}

	if (!nKey)
	{
		Reference<XNumberFormats> xFormats( _xFormatter->getNumberFormatsSupplier()->getNumberFormats() );
		Reference<XNumberFormatTypes> xTypeList(_xFormatter->getNumberFormatsSupplier()->getNumberFormats(), UNO_QUERY);

		nKey = ::dbtools::getDefaultNumberFormat(_xColumn,
										   Reference< XNumberFormatTypes > (xFormats, UNO_QUERY),
										   _rLocale);

	}

	sal_Int16 nKeyType = getNumberFormatType(_xFormatter, nKey) & ~NumberFormat::DEFINED;

	return DBTypeConversion::getValue(Reference< XColumn > (_xColumn, UNO_QUERY), _xFormatter, _rNullDate, nKey, nKeyType);
}

//------------------------------------------------------------------------------
::rtl::OUString DBTypeConversion::getValue(const Reference<XColumn>& xVariant,
								   const Reference<XNumberFormatter>& xFormatter,
								   const Date& rNullDate,
								   sal_Int32 nKey,
								   sal_Int16 nKeyType)
{
	::rtl::OUString aString;
	if (xVariant.is())
	{
		try
		{
			switch (nKeyType & ~NumberFormat::DEFINED)
			{
				case NumberFormat::DATE:
				case NumberFormat::DATETIME:
				{
                    // get a value which represents the given date, relative to the given null date
                    double fValue = getValue(xVariant, rNullDate, nKeyType);
                    if ( !xVariant->wasNull() )
                    {
                         // get the null date of the formatter
                         Date aFormatterNullDate( rNullDate );
                         try
                         {
                             Reference< XPropertySet > xFormatterSettings;
                             Reference< XNumberFormatsSupplier > xSupplier( xFormatter->getNumberFormatsSupplier( ) );
                             if ( xSupplier.is() )
                                 xFormatterSettings = xSupplier->getNumberFormatSettings();
                             if ( xFormatterSettings.is() )
                                 xFormatterSettings->getPropertyValue( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "NullDate" ) ) ) >>= aFormatterNullDate;
                         }
                         catch( const Exception& )
                         {
                            OSL_ENSURE( sal_False, "DBTypeConversion::getValue: caught an exception while retrieving the formatter's NullDate!" );
                         }
                         // get a value which represents the given date, relative to the null date of the formatter
                         fValue -= toDays( rNullDate, aFormatterNullDate );
                         // format this value
                        aString = xFormatter->convertNumberToString( nKey, fValue );
                    }
                }
				break;
				case NumberFormat::TIME:
				case NumberFormat::NUMBER:
				case NumberFormat::SCIENTIFIC:
				case NumberFormat::FRACTION:
				case NumberFormat::PERCENT:
				{
					double fValue = xVariant->getDouble();
					if (!xVariant->wasNull())
						aString = xFormatter->convertNumberToString(nKey, fValue);
				}	break;
				case NumberFormat::CURRENCY:
				{
					double fValue = xVariant->getDouble();
					if (!xVariant->wasNull())
						aString = xFormatter->getInputString(nKey, fValue);
				}	break;
				case NumberFormat::TEXT:
					aString = xFormatter->formatString(nKey, xVariant->getString());
					break;
				default:
					aString = xVariant->getString();
			}
		}
		catch(const Exception& )
		{
			aString = xVariant->getString();
		}
	}
	return aString;
}
//------------------------------------------------------------------
