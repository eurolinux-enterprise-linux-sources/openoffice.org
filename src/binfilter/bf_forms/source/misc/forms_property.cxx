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

#ifndef _FRM_PROPERTY_HXX_
#include "property.hxx"
#endif
#ifndef _FRM_PROPERTY_HRC_
#include "property.hrc"
#endif




#include <algorithm>
namespace binfilter {

//... namespace frm .......................................................
namespace frm
{
//.........................................................................

//==================================================================
//= PropertyInfoService
//==================================================================
PropertyInfoService::PropertyMap PropertyInfoService::s_AllKnownProperties;
//------------------------------------------------------------------
sal_Int32 PropertyInfoService::getPropertyId(const ::rtl::OUString& _rName)
{
	initialize();

	PropertyAssignment aCompareName(_rName, -1);

	::std::pair<PropertyMapIterator,PropertyMapIterator> aPair = ::std::equal_range(
		s_AllKnownProperties.begin(),
		s_AllKnownProperties.end(),
		aCompareName,
		PropertyAssignmentNameCompareLess());

	sal_Int32 nHandle = -1;
	if (aPair.first != aPair.second)
	{	// we found something _and_ we have an identity
		nHandle = aPair.first->nHandle;
	}

	return nHandle;
}

//------------------------------------------------------------------
sal_Int32 ConcretInfoService::getPreferedPropertyId(const ::rtl::OUString& _rName)
{
	return PropertyInfoService::getPropertyId(_rName);
}

//------------------------------------------------------------------
#define ADD_PROP_ASSIGNMENT(varname) \
	s_AllKnownProperties.push_back(PropertyAssignment(PROPERTY_##varname, PROPERTY_ID_##varname))
//..................................................................
void PropertyInfoService::initialize()
{
	if (!s_AllKnownProperties.empty())
		return;

	s_AllKnownProperties.reserve(220);

	ADD_PROP_ASSIGNMENT(NAME);
	ADD_PROP_ASSIGNMENT(TAG);
	ADD_PROP_ASSIGNMENT(TABINDEX);
	ADD_PROP_ASSIGNMENT(CLASSID);
	ADD_PROP_ASSIGNMENT(ALIGN);
	ADD_PROP_ASSIGNMENT(ROWCOUNT);
	ADD_PROP_ASSIGNMENT(ROWCOUNTFINAL);
	ADD_PROP_ASSIGNMENT(FETCHSIZE);
	ADD_PROP_ASSIGNMENT(VALUE);
	ADD_PROP_ASSIGNMENT(VALUEMIN);
	ADD_PROP_ASSIGNMENT(VALUEMAX);
	ADD_PROP_ASSIGNMENT(VALUESTEP);
	ADD_PROP_ASSIGNMENT(TEXT);
	ADD_PROP_ASSIGNMENT(LABEL);
	ADD_PROP_ASSIGNMENT(NAVIGATION);
	ADD_PROP_ASSIGNMENT(CYCLE);
	ADD_PROP_ASSIGNMENT(CONTROLSOURCE);
	ADD_PROP_ASSIGNMENT(ENABLED);
	ADD_PROP_ASSIGNMENT(SPIN);
	ADD_PROP_ASSIGNMENT(READONLY);
	ADD_PROP_ASSIGNMENT(FILTER_CRITERIA);
	ADD_PROP_ASSIGNMENT(AUTOINCREMENT);
	ADD_PROP_ASSIGNMENT(CACHESIZE);
	ADD_PROP_ASSIGNMENT(LASTDIRTY);
	ADD_PROP_ASSIGNMENT(STATEMENT);
	ADD_PROP_ASSIGNMENT(WIDTH);
	ADD_PROP_ASSIGNMENT(SEARCHABLE);
	ADD_PROP_ASSIGNMENT(MULTILINE);
	ADD_PROP_ASSIGNMENT(TARGET_URL);
	ADD_PROP_ASSIGNMENT(DEFAULTCONTROL);
	ADD_PROP_ASSIGNMENT(MAXTEXTLEN);
	ADD_PROP_ASSIGNMENT(SIZE);
	ADD_PROP_ASSIGNMENT(DATE);
	ADD_PROP_ASSIGNMENT(TIME);
	ADD_PROP_ASSIGNMENT(STATE);
	ADD_PROP_ASSIGNMENT(TRISTATE);
	ADD_PROP_ASSIGNMENT(HIDDEN_VALUE);
	ADD_PROP_ASSIGNMENT(TARGET_FRAME);
	ADD_PROP_ASSIGNMENT(BUTTONTYPE);
	ADD_PROP_ASSIGNMENT(STRINGITEMLIST);
	ADD_PROP_ASSIGNMENT(DEFAULT_TEXT);
	ADD_PROP_ASSIGNMENT(DEFAULTCHECKED);
	ADD_PROP_ASSIGNMENT(DEFAULT_DATE);
	ADD_PROP_ASSIGNMENT(DEFAULT_TIME);
	ADD_PROP_ASSIGNMENT(DEFAULT_VALUE);
	ADD_PROP_ASSIGNMENT(FORMATKEY);
	ADD_PROP_ASSIGNMENT(FORMATSSUPPLIER);
	ADD_PROP_ASSIGNMENT(SUBMIT_ACTION);
	ADD_PROP_ASSIGNMENT(SUBMIT_TARGET);
	ADD_PROP_ASSIGNMENT(SUBMIT_METHOD);
	ADD_PROP_ASSIGNMENT(SUBMIT_ENCODING);
	ADD_PROP_ASSIGNMENT(IMAGE_URL);
	ADD_PROP_ASSIGNMENT(EMPTY_IS_NULL);
	ADD_PROP_ASSIGNMENT(LISTSOURCETYPE);
	ADD_PROP_ASSIGNMENT(LISTSOURCE);
	ADD_PROP_ASSIGNMENT(SELECT_SEQ);
	ADD_PROP_ASSIGNMENT(VALUE_SEQ);
	ADD_PROP_ASSIGNMENT(DEFAULT_SELECT_SEQ);
	ADD_PROP_ASSIGNMENT(MULTISELECTION);
	ADD_PROP_ASSIGNMENT(DECIMAL_ACCURACY);
	ADD_PROP_ASSIGNMENT(EDITMASK);
	ADD_PROP_ASSIGNMENT(ISREADONLY);
	ADD_PROP_ASSIGNMENT(ISREQUIRED);
	ADD_PROP_ASSIGNMENT(FIELDTYPE);
	ADD_PROP_ASSIGNMENT(DECIMALS);
	ADD_PROP_ASSIGNMENT(REFVALUE);
	ADD_PROP_ASSIGNMENT(STRICTFORMAT);
	ADD_PROP_ASSIGNMENT(DATASOURCE);
	ADD_PROP_ASSIGNMENT(ALLOWADDITIONS);
	ADD_PROP_ASSIGNMENT(ALLOWEDITS);
	ADD_PROP_ASSIGNMENT(ALLOWDELETIONS);
	ADD_PROP_ASSIGNMENT(MASTERFIELDS);
	ADD_PROP_ASSIGNMENT(ISPASSTHROUGH);
	ADD_PROP_ASSIGNMENT(QUERY);
	ADD_PROP_ASSIGNMENT(LITERALMASK);
	ADD_PROP_ASSIGNMENT(SHOWTHOUSANDSEP);
	ADD_PROP_ASSIGNMENT(CURRENCYSYMBOL);
	ADD_PROP_ASSIGNMENT(DATEFORMAT);
	ADD_PROP_ASSIGNMENT(DATEMIN);
	ADD_PROP_ASSIGNMENT(DATEMAX);
	ADD_PROP_ASSIGNMENT(DATE_SHOW_CENTURY);
	ADD_PROP_ASSIGNMENT(TIMEFORMAT);
	ADD_PROP_ASSIGNMENT(TIMEMIN);
	ADD_PROP_ASSIGNMENT(TIMEMAX);
	ADD_PROP_ASSIGNMENT(LINECOUNT);
	ADD_PROP_ASSIGNMENT(BOUNDCOLUMN);
	ADD_PROP_ASSIGNMENT(HASNAVIGATION);
	ADD_PROP_ASSIGNMENT(FONT);
	ADD_PROP_ASSIGNMENT(BACKGROUNDCOLOR);
	ADD_PROP_ASSIGNMENT(FILLCOLOR);
	ADD_PROP_ASSIGNMENT(TEXTCOLOR);
	ADD_PROP_ASSIGNMENT(LINECOLOR);
	ADD_PROP_ASSIGNMENT(BORDER);
	ADD_PROP_ASSIGNMENT(DROPDOWN);
	ADD_PROP_ASSIGNMENT(MULTI);
	ADD_PROP_ASSIGNMENT(HSCROLL);
	ADD_PROP_ASSIGNMENT(VSCROLL);
	ADD_PROP_ASSIGNMENT(TABSTOP);
	ADD_PROP_ASSIGNMENT(AUTOCOMPLETE);
	ADD_PROP_ASSIGNMENT(HARDLINEBREAKS);
	ADD_PROP_ASSIGNMENT(PRINTABLE);
	ADD_PROP_ASSIGNMENT(ECHO_CHAR);
	ADD_PROP_ASSIGNMENT(ROWHEIGHT);
	ADD_PROP_ASSIGNMENT(HELPTEXT);
	ADD_PROP_ASSIGNMENT(FONT_NAME);
	ADD_PROP_ASSIGNMENT(FONT_STYLENAME);
	ADD_PROP_ASSIGNMENT(FONT_FAMILY);
	ADD_PROP_ASSIGNMENT(FONT_CHARSET);
	ADD_PROP_ASSIGNMENT(FONT_HEIGHT);
	ADD_PROP_ASSIGNMENT(FONT_WEIGHT);
	ADD_PROP_ASSIGNMENT(FONT_SLANT);
	ADD_PROP_ASSIGNMENT(FONT_UNDERLINE);
	ADD_PROP_ASSIGNMENT(FONT_WORDLINEMODE);
	ADD_PROP_ASSIGNMENT(FONT_STRIKEOUT);
	ADD_PROP_ASSIGNMENT(TEXTLINECOLOR);
	ADD_PROP_ASSIGNMENT(FONTEMPHASISMARK);
	ADD_PROP_ASSIGNMENT(FONTRELIEF);
	ADD_PROP_ASSIGNMENT(HELPURL);
	ADD_PROP_ASSIGNMENT(RECORDMARKER);
	ADD_PROP_ASSIGNMENT(BOUNDFIELD);
	ADD_PROP_ASSIGNMENT(TREATASNUMERIC);
	ADD_PROP_ASSIGNMENT(EFFECTIVE_VALUE);
	ADD_PROP_ASSIGNMENT(EFFECTIVE_DEFAULT);
	ADD_PROP_ASSIGNMENT(EFFECTIVE_MIN);
	ADD_PROP_ASSIGNMENT(EFFECTIVE_MAX);
	ADD_PROP_ASSIGNMENT(HIDDEN);
	ADD_PROP_ASSIGNMENT(FILTERPROPOSAL);
	ADD_PROP_ASSIGNMENT(FIELDSOURCE);
	ADD_PROP_ASSIGNMENT(TABLENAME);
	ADD_PROP_ASSIGNMENT(FILTERSUPPLIER);
	ADD_PROP_ASSIGNMENT(CURRENTFILTER);
	ADD_PROP_ASSIGNMENT(SELECTED_FIELDS);
	ADD_PROP_ASSIGNMENT(SELECTED_TABLES);
	ADD_PROP_ASSIGNMENT(THREADSAFE);
	ADD_PROP_ASSIGNMENT(CONTROLLABEL);
	ADD_PROP_ASSIGNMENT(CURRSYM_POSITION);
	ADD_PROP_ASSIGNMENT(SOURCE);
	ADD_PROP_ASSIGNMENT(CURSORCOLOR);
	ADD_PROP_ASSIGNMENT(ALWAYSSHOWCURSOR);
	ADD_PROP_ASSIGNMENT(DISPLAYSYNCHRON);
	ADD_PROP_ASSIGNMENT(ISMODIFIED);
	ADD_PROP_ASSIGNMENT(ISNEW);
	ADD_PROP_ASSIGNMENT(PRIVILEGES);
	ADD_PROP_ASSIGNMENT(DETAILFIELDS);
	ADD_PROP_ASSIGNMENT(COMMAND);
	ADD_PROP_ASSIGNMENT(COMMANDTYPE);
	ADD_PROP_ASSIGNMENT(RESULTSET_CONCURRENCY);
	ADD_PROP_ASSIGNMENT(INSERTONLY);
	ADD_PROP_ASSIGNMENT(RESULTSET_TYPE);
	ADD_PROP_ASSIGNMENT(ESCAPE_PROCESSING);
	ADD_PROP_ASSIGNMENT(APPLYFILTER);
	ADD_PROP_ASSIGNMENT(ISNULLABLE);
	ADD_PROP_ASSIGNMENT(ACTIVECOMMAND);
	ADD_PROP_ASSIGNMENT(ISCURRENCY);
	ADD_PROP_ASSIGNMENT(URL);
	ADD_PROP_ASSIGNMENT(TITLE);
	ADD_PROP_ASSIGNMENT(ACTIVE_CONNECTION);
	ADD_PROP_ASSIGNMENT(SCALE);
	ADD_PROP_ASSIGNMENT(SORT);
	ADD_PROP_ASSIGNMENT(PERSISTENCE_MAXTEXTLENGTH);

	// now sort the array by name
	
	std::sort(
		s_AllKnownProperties.begin(),
		s_AllKnownProperties.end(),
		PropertyAssignmentNameCompareLess()
	);
}

//==================================================================
//= instantiation of property strings
//==================================================================

IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TABINDEX, "TabIndex");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TAG, "Tag");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_NAME, "Name");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CLASSID, "ClassId");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ROWCOUNT, "RowCount");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ROWCOUNTFINAL, "IsRowCountFinal");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FETCHSIZE, "FetchSize");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_VALUE, "Value");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TEXT, "Text");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_LABEL, "Label");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CANINSERT, "CanInsert");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CANUPDATE, "CanUpdate");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CANDELETE, "CanDelete");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_NAVIGATION, "NavigationBarMode");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_HASNAVIGATION, "HasNavigationBar");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CYCLE, "Cycle");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CONTROLSOURCE, "DataField");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ENABLED, "Enabled");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_READONLY, "ReadOnly");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISREADONLY, "IsReadOnly");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FILTER_CRITERIA, "Filter");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISREQUIRED, "IsRequired");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_AUTOINCREMENT, "IsAutoIncrement");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CACHESIZE, "CacheSize");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DATAENTRY, "DataEntry");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_LASTDIRTY, "LastUpdated");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_STATEMENT, "Statement");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_WIDTH, "Width");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SEARCHABLE, "IsSearchable");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_MULTILINE, "MultiLine");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TARGET_URL, "TargetURL");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TARGET_FRAME, "TargetFrame");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DEFAULTCONTROL, "DefaultControl");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_MAXTEXTLEN, "MaxTextLen");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_EDITMASK, "EditMask");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SIZE, "Size");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SPIN, "Spin");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DATE, "Date");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TIME, "Time");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_STATE, "State");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TRISTATE, "TriState");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_HIDDEN_VALUE, "HiddenValue");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_BUTTONTYPE, "ButtonType");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_STRINGITEMLIST, "StringItemList");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DEFAULT_TEXT, "DefaultText");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DEFAULTCHECKED, "DefaultState");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FORMATKEY, "FormatKey");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FORMATSSUPPLIER, "FormatsSupplier");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SUBMIT_ACTION, "SubmitAction");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SUBMIT_TARGET, "SubmitTarget");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SUBMIT_METHOD, "SubmitMethod");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SUBMIT_ENCODING, "SubmitEncoding");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_IMAGE_URL, "ImageURL");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_EMPTY_IS_NULL, "ConvertEmptyToNull");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_LISTSOURCETYPE, "ListSourceType");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_LISTSOURCE, "ListSource");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SELECT_SEQ, "SelectedItems");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_VALUE_SEQ, "ValueItemList");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DEFAULT_SELECT_SEQ, "DefaultSelection");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_MULTISELECTION, "MultiSelection");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ALIGN, "Align");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DEFAULT_DATE, "DefaultDate");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DEFAULT_TIME, "DefaultTime");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DEFAULT_VALUE, "DefaultValue");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DECIMAL_ACCURACY, "DecimalAccuracy");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CURSORSOURCE, "DataSelection");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CURSORSOURCETYPE, "DataSelectionType");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FIELDTYPE, "Type");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DECIMALS, "Decimals");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_REFVALUE, "RefValue");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_VALUEMIN, "ValueMin");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_VALUEMAX, "ValueMax");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_STRICTFORMAT, "StrictFormat");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ALLOWADDITIONS, "AllowInserts");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ALLOWEDITS, "AllowUpdates");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ALLOWDELETIONS, "AllowDeletes");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_MASTERFIELDS, "MasterFields");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISPASSTHROUGH, "IsPassThrough");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_QUERY, "Query");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_LITERALMASK, "LiteralMask");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_VALUESTEP, "ValueStep");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SHOWTHOUSANDSEP, "ShowThousandsSeparator");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CURRENCYSYMBOL, "CurrencySymbol");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DATEFORMAT, "DateFormat");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DATEMIN, "DateMin");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DATEMAX, "DateMax");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DATE_SHOW_CENTURY, "DateShowCentury");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TIMEFORMAT, "TimeFormat");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TIMEMIN, "TimeMin");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TIMEMAX, "TimeMax");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_LINECOUNT, "LineCount");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_BOUNDCOLUMN, "BoundColumn");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT, "FontDescriptor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_BACKGROUNDCOLOR, "BackgroundColor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FILLCOLOR, "FillColor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TEXTCOLOR, "TextColor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_LINECOLOR, "LineColor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_BORDER, "Border");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DROPDOWN, "Dropdown");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_MULTI, "Multi");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_HSCROLL, "HScroll");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_VSCROLL, "VScroll");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TABSTOP, "Tabstop");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_AUTOCOMPLETE, "Autocomplete");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_HARDLINEBREAKS, "HardLineBreaks");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_PRINTABLE, "Printable");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ECHO_CHAR, "EchoChar");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ROWHEIGHT, "RowHeight");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_HELPTEXT, "HelpText");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_NAME, "FontName");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_STYLENAME, "FontStyleName");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_FAMILY, "FontFamily");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_CHARSET, "FontCharset");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_HEIGHT, "FontHeight");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_WEIGHT, "FontWeight");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_SLANT, "FontSlant");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_UNDERLINE, "FontUnderline");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_WORDLINEMODE, "FontWordLineMode");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONT_STRIKEOUT, "FontStrikeout");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TEXTLINECOLOR, "TextLineColor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONTEMPHASISMARK, "FontEmphasisMark");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FONTRELIEF, "FontRelief");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_HELPURL, "HelpURL");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_RECORDMARKER, "HasRecordMarker");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_BOUNDFIELD, "BoundField");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TREATASNUMERIC, "TreatAsNumber");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_EFFECTIVE_VALUE, "EffectiveValue");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_EFFECTIVE_DEFAULT, "EffectiveDefault");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_EFFECTIVE_MIN, "EffectiveMin");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_EFFECTIVE_MAX, "EffectiveMax");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_HIDDEN, "Hidden");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FILTERPROPOSAL, "UseFilterValueProposal");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FIELDSOURCE, "FieldSource");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TABLENAME, "TableName");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_FILTERSUPPLIER, "FilterSupplier");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CURRENTFILTER, "CurrentFilter");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SELECTED_FIELDS, "SelectedFields");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SELECTED_TABLES, "SelectedTables");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_THREADSAFE, "ThreadSafe");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISFILTERAPPLIED, "IsFilterApplied");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CONTROLLABEL, "LabelControl");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CURRSYM_POSITION, "PrependCurrencySymbol");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SOURCE, "Source");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CURSORCOLOR, "CursorColor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ALWAYSSHOWCURSOR, "AlwaysShowCursor");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DISPLAYSYNCHRON, "DisplayIsSynchron");

IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISMODIFIED, "IsModified");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISNEW, "IsNew");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_PRIVILEGES, "Privileges");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_COMMAND, "Command");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_COMMANDTYPE, "CommandType");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_RESULTSET_CONCURRENCY, "ResultSetConcurrency");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_INSERTONLY, "IgnoreResult");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_RESULTSET_TYPE, "ResultSetType");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ESCAPE_PROCESSING, "EscapeProcessing");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_APPLYFILTER, "ApplyFilter");

IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISNULLABLE, "IsNullable");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ACTIVECOMMAND, "ActiveCommand");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ISCURRENCY, "IsCurrency");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_URL, "URL");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_TITLE, "Title");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_ACTIVE_CONNECTION, "ActiveConnection");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SCALE, "Scale");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_SORT, "Order");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DATASOURCE, "DataSourceName");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DETAILFIELDS, "DetailFields");

IMPLEMENT_CONSTASCII_USTRING(PROPERTY_COLUMNSERVICENAME, "ColumnServiceName");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_REALNAME, "RealName");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_CONTROLSOURCEPROPERTY, "DataFieldProperty");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_USER, "User");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_PASSWORD, "Password");
IMPLEMENT_CONSTASCII_USTRING(PROPERTY_DISPATCHURLINTERNAL, "DispatchURLInternal");

IMPLEMENT_CONSTASCII_USTRING(PROPERTY_PERSISTENCE_MAXTEXTLENGTH, "PersistenceMaxTextLength");

//.........................................................................
}
//... namespace frm .......................................................

}
