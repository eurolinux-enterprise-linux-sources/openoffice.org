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

#ifndef _TOOLKIT_HELPER_PROPERTY_HXX_
#define _TOOLKIT_HELPER_PROPERTY_HXX_

#include <toolkit/dllapi.h>

#include <sal/types.h>

namespace com {
namespace sun {
namespace star {
namespace uno {
	class Type;
	class Any;
} } } }

namespace rtl {
	class OUString;
}


#define BASEPROPERTY_NOTFOUND				         0

#define BASEPROPERTY_TEXT					         1	// ::rtl::OUString
#define BASEPROPERTY_BACKGROUNDCOLOR		         2	// sal_Int32
#define BASEPROPERTY_FILLCOLOR				         3	// sal_Int32
#define BASEPROPERTY_TEXTCOLOR			             4	// sal_Int32
#define BASEPROPERTY_LINECOLOR				         5	// sal_Int32
#define BASEPROPERTY_BORDER				 	         6	// sal_Int16
#define BASEPROPERTY_ALIGN					         7	// sal_Int16
#define BASEPROPERTY_FONTDESCRIPTOR			         8	// ::com::sun::star::awt::FontDescriptor, war mal Font, aber nie gespeichert...
#define BASEPROPERTY_DROPDOWN				         9	// sal_Bool
#define BASEPROPERTY_MULTILINE				         10	// sal_Bool
#define BASEPROPERTY_STRINGITEMLIST			         11	// UStringSequence
#define BASEPROPERTY_HSCROLL				         12	// sal_Bool
#define BASEPROPERTY_VSCROLL				         13	// sal_Bool
#define BASEPROPERTY_TABSTOP				         14	// sal_Bool
#define BASEPROPERTY_STATE					         15	// sal_Int16
#define BASEPROPERTY_FONT_TYPE				         16	// OLD: Font_Type
#define BASEPROPERTY_FONT_SIZE				         17	// OLD: Font_Size
#define BASEPROPERTY_FONT_ATTRIBS			         18	// OLD: Font_Attribs
#define BASEPROPERTY_DEFAULTCONTROL			         19	// ::rtl::OUString (ServiceName)
#define BASEPROPERTY_LABEL					         20	// ::rtl::OUString
#define BASEPROPERTY_LINECOUNT				         21	// sal_Int16
#define BASEPROPERTY_EXTDATEFORMAT			         22	// sal_Int16
#define BASEPROPERTY_DATESHOWCENTURY                 23	// sal_Bool
#define BASEPROPERTY_EXTTIMEFORMAT			         24	// sal_Int16
#define BASEPROPERTY_NUMSHOWTHOUSANDSEP		         25	// sal_Bool
#define BASEPROPERTY_CURRENCYSYMBOL			         26	// ::rtl::OUString
#define BASEPROPERTY_SPIN					         27	// sal_Bool
#define BASEPROPERTY_STRICTFORMAT			         28	// sal_Bool
#define BASEPROPERTY_DECIMALACCURACY		         29	// sal_Int16
#define BASEPROPERTY_DATE					         30	// sal_Int32
#define BASEPROPERTY_DATEMIN                         31	// sal_Int32
#define BASEPROPERTY_DATEMAX                         32	// sal_Int32
#define BASEPROPERTY_TIME					         33	// sal_Int32
#define BASEPROPERTY_TIMEMIN                         34	// sal_Int32
#define BASEPROPERTY_TIMEMAX                         35	// sal_Int32
#define BASEPROPERTY_VALUE_INT32			         36	// sal_Int32
#define BASEPROPERTY_VALUEMIN_INT32			         37	// sal_Int32
#define BASEPROPERTY_VALUEMAX_INT32			         38	// sal_Int32
#define BASEPROPERTY_VALUESTEP_INT32		         39	// sal_Int32
#define BASEPROPERTY_EDITMASK				         40	// ::rtl::OUString
#define BASEPROPERTY_LITERALMASK			         41	// ::rtl::OUString
#define BASEPROPERTY_IMAGEURL				         42	// ::rtl::OUString
#define BASEPROPERTY_READONLY				         43	// sal_Bool
#define BASEPROPERTY_ENABLED				         44	// sal_Bool
#define BASEPROPERTY_PRINTABLE				         45	// sal_Bool
#define BASEPROPERTY_ECHOCHAR				         46	// sal_Int16
#define BASEPROPERTY_MAXTEXTLEN				         47	// sal_Int16
#define BASEPROPERTY_HARDLINEBREAKS			         48	// sal_Int16
#define BASEPROPERTY_AUTOCOMPLETE			         49	// sal_Bool
#define BASEPROPERTY_MULTISELECTION			         50	// sal_Bool
#define BASEPROPERTY_SELECTEDITEMS			         51	// INT16Sequence
#define BASEPROPERTY_VALUE_DOUBLE			         52	// DOUBLE
#define BASEPROPERTY_VALUEMIN_DOUBLE		         53	// DOUBLE
#define BASEPROPERTY_VALUEMAX_DOUBLE		         54	// DOUBLE
#define BASEPROPERTY_VALUESTEP_DOUBLE		         55	// DOUBLE
#define BASEPROPERTY_TRISTATE				         56	// sal_Bool
#define BASEPROPERTY_DEFAULTBUTTON			         57	// sal_Bool
#define BASEPROPERTY_HELPURL				         58	// ::rtl::OUString
#define BASEPROPERTY_AUTOTOGGLE				         59	// sal_Bool
//#define BASEPROPERTY_FOCUSSELECTIONHIDE		     60	// sal_Bool
#define BASEPROPERTY_FORMATKEY				         61	// sal_Bool
#define BASEPROPERTY_FORMATSSUPPLIER		         62	// ::com::sun::star::util::XNumberFormatsSupplier
#define BASEPROPERTY_EFFECTIVE_VALUE		         63	// Any (double or string)
#define BASEPROPERTY_TREATASNUMBER			         64	// sal_Bool
#define BASEPROPERTY_EFFECTIVE_DEFAULT		         65	// Any (double or string)
#define BASEPROPERTY_EFFECTIVE_MIN			         66	// Double
#define BASEPROPERTY_EFFECTIVE_MAX			         67	// Double
#define BASEPROPERTY_CURSYM_POSITION		         68	// sal_Bool
#define BASEPROPERTY_TITLE					         69	// ::rtl::OUString
#define BASEPROPERTY_MOVEABLE				         70	// sal_Bool
#define BASEPROPERTY_CLOSEABLE				         71	// sal_Bool
#define BASEPROPERTY_SIZEABLE				         72	// sal_Bool
#define BASEPROPERTY_HELPTEXT				         73	// ::rtl::OUString
#define BASEPROPERTY_PROGRESSVALUE  	             74	// sal_Int32
#define BASEPROPERTY_PROGRESSVALUE_MIN 	             75	// sal_Int32
#define BASEPROPERTY_PROGRESSVALUE_MAX 	             76	// sal_Int32
#define BASEPROPERTY_SCROLLVALUE	 	             77	// sal_Int32
#define BASEPROPERTY_SCROLLVALUE_MAX 	             78	// sal_Int32
#define BASEPROPERTY_LINEINCREMENT	 	             79	// sal_Int32
#define BASEPROPERTY_BLOCKINCREMENT	 	             80	// sal_Int32
#define BASEPROPERTY_VISIBLESIZE	 	             81	// sal_Int32
#define BASEPROPERTY_ORIENTATION	 	             82	// sal_Int32
#define BASEPROPERTY_FONTRELIEF	 	                 83	// sal_Int16
#define BASEPROPERTY_FONTEMPHASISMARK	 	         84	// sal_Int16
#define BASEPROPERTY_TEXTLINECOLOR			         85	// sal_Int32
#define BASEPROPERTY_IMAGEALIGN			             86	// sal_Int16
#define BASEPROPERTY_SCALEIMAGE                      87	// sal_Bool
#define BASEPROPERTY_PUSHBUTTONTYPE                  88	// sal_Int16
#define BASEPROPERTY_DISPLAYBACKGROUNDCOLOR          89	// sal_Int32
#define BASEPROPERTY_AUTOMNEMONICS                   90	// sal_Bool
#define BASEPROPERTY_MOUSETRANSPARENT                91	// sal_Bool
#define BASEPROPERTY_ACCESSIBLENAME                  92  // ::rtl::OUString
#define BASEPROPERTY_PLUGINPARENT	                 93  // sal_Int64
#define BASEPROPERTY_SCROLLVALUE_MIN 	             94  // sal_Int32
#define BASEPROPERTY_REPEAT_DELAY                    95  // sal_Int32
#define BASEPROPERTY_SYMBOL_COLOR                    96  // sal_Int32
#define BASEPROPERTY_SPINVALUE                       97  // sal_Int32
#define BASEPROPERTY_SPINVALUE_MIN                   98  // sal_Int32
#define BASEPROPERTY_SPINVALUE_MAX                   99  // sal_Int32
#define BASEPROPERTY_SPININCREMENT                  100  // sal_Int32
#define BASEPROPERTY_REPEAT                         101  // sal_Bool
#define BASEPROPERTY_ENFORCE_FORMAT                 102  // sal_Bool
#define BASEPROPERTY_LIVE_SCROLL                    103  // sal_Bool
#define BASEPROPERTY_LINE_END_FORMAT                104  // sal_Int16
#define BASEPROPERTY_ACTIVATED                      105  // sal Bool
#define BASEPROPERTY_COMPLETE                       106  // sal_Bool
#define BASEPROPERTY_CURRENTITEMID                  107  // sal_Int16
#define BASEPROPERTY_TOGGLE                         108  // sal_Bool
#define BASEPROPERTY_FOCUSONCLICK                   109  // sal_Bool
#define BASEPROPERTY_HIDEINACTIVESELECTION          110  // sal_Bool
#define BASEPROPERTY_VISUALEFFECT                   111  // sal_Int16
#define BASEPROPERTY_BORDERCOLOR                    112  // sal_Int32
#define BASEPROPERTY_IMAGEPOSITION                  113  // sal_Int16
#define BASEPROPERTY_NATIVE_WIDGET_LOOK             114  // sal_Bool
#define BASEPROPERTY_VERTICALALIGN                  115  // VerticalAlignment
#define BASEPROPERTY_MOUSE_WHEEL_BEHAVIOUR          116  // sal_Int16
#define BASEPROPERTY_GRAPHIC                        117  // css.graphic.XGraphic
#define BASEPROPERTY_STEP_TIME                      118  // sal_Int32
#define BASEPROPERTY_DECORATION                     119  // sal_Bool
#define BASEPROPERTY_PAINTTRANSPARENT               120  // sal_Bool
#define BASEPROPERTY_AUTOHSCROLL                    121	// sal_Bool
#define BASEPROPERTY_AUTOVSCROLL                    122  // sal_Bool
#define BASEPROPERTY_DESKTOP_AS_PARENT              123  // sal_Bool
#define BASEPROPERTY_TREE_START						124
#define BASEPROPERTY_TREE_SELECTIONTYPE				124
#define BASEPROPERTY_TREE_EDITABLE					125
#define BASEPROPERTY_TREE_DATAMODEL					126
#define BASEPROPERTY_TREE_ROOTDISPLAYED				127
#define BASEPROPERTY_TREE_SHOWSHANDLES				128
#define BASEPROPERTY_TREE_SHOWSROOTHANDLES			129
#define BASEPROPERTY_TREE_ROWHEIGHT					130
#define BASEPROPERTY_TREE_INVOKESSTOPNODEEDITING	131
#define BASEPROPERTY_TREE_END						131
#define BASEPROPERTY_DIALOGSOURCEURL				132
#define BASEPROPERTY_NOLABEL                        133  // ::rtl::OUString  added for issue79712
#define BASEPROPERTY_URL                            134  // ::rtl::OUString
#define BASEPROPERTY_UNIT                           135  // ::awt::FieldUnit
#define BASEPROPERTY_CUSTOMUNITTEXT                 136  // ::rtl::OUString
#define BASEPROPERTY_IMAGE_SCALE_MODE               137
#define BASEPROPERTY_WRITING_MODE                   138
#define BASEPROPERTY_CONTEXT_WRITING_MODE           139
#define BASEPROPERTY_GRID_SHOWROWHEADER             140
#define BASEPROPERTY_GRID_SHOWCOLUMNHEADER          141
#define BASEPROPERTY_GRID_DATAMODEL                 142
#define BASEPROPERTY_GRID_COLUMNMODEL               143
#define BASEPROPERTY_GRID_SELECTIONMODE             144
#define BASEPROPERTY_ENABLEVISIBLE                  145  // sal_Bool


// Keine gebundenen Properties, werden immer aus der Property BASEPROPERTY_FONTDESCRIPTOR entnommen.
#define BASEPROPERTY_FONTDESCRIPTORPART_START			1000
#define BASEPROPERTY_FONTDESCRIPTORPART_NAME			1000	// ::rtl::OUString, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_STYLENAME		1001	// ::rtl::OUString, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_FAMILY			1002	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_CHARSET     	1003	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_HEIGHT      	1004	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_WEIGHT      	1005	// Float, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_SLANT       	1006	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_UNDERLINE   	1007	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_STRIKEOUT   	1008	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_WIDTH			1009	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_PITCH			1010	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_CHARWIDTH		1011	// Float, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_ORIENTATION		1012	// Float, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_KERNING			1013	// sal_Bool, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_WORDLINEMODE	1014	// sal_Bool, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_TYPE			1015	// sal_Int16, not bound
#define BASEPROPERTY_FONTDESCRIPTORPART_END         	1015

#define PROPERTY_ALIGN_LEFT						0
#define PROPERTY_ALIGN_CENTER					1
#define PROPERTY_ALIGN_RIGHT					2

#define PROPERTY_STATE_OFF						0
#define PROPERTY_STATE_ON						1
#define PROPERTY_STATE_DONTCARE					2

TOOLKIT_DLLPUBLIC sal_uInt16		GetPropertyId( const ::rtl::OUString& rPropertyName );
const ::com::sun::star::uno::Type*	GetPropertyType( sal_uInt16 nPropertyId );
const ::rtl::OUString&				GetPropertyName( sal_uInt16 nPropertyId );
sal_Int16 							GetPropertyAttribs( sal_uInt16 nPropertyId );
sal_uInt16							GetPropertyOrderNr( sal_uInt16 nPropertyId );
sal_Bool							DoesDependOnOthers( sal_uInt16 nPropertyId );
sal_Bool							CompareProperties( const ::com::sun::star::uno::Any& r1, const ::com::sun::star::uno::Any& r2 );




#endif // _TOOLKIT_HELPER_PROPERTY_HXX_


