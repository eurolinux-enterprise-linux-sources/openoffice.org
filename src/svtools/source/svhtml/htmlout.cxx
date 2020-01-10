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
#include "precompiled_svtools.hxx"

#include <tools/urlobj.hxx>
#include <svtools/zformat.hxx>
#include <svtools/macitem.hxx>
#include <tools/cachestr.hxx>
#include <vcl/svapp.hxx>
#include <svtools/zforlist.hxx>

#include <svtools/htmlout.hxx>
#include "htmlkywd.hxx"
#include <svtools/imap.hxx>
#include <svtools/imaprect.hxx>
#include <svtools/imapcirc.hxx>
#include <svtools/imappoly.hxx>
#include "urihelper.hxx"

#ifndef RTL_CONSTASCII_STRINGPARAM
#define RTL_CONSTASCII_STRINGPARAM( c ) c, sizeof(c)-1
#endif

#if defined(UNX)
const sal_Char HTMLOutFuncs::sNewLine = '\012';
#else
const sal_Char __FAR_DATA HTMLOutFuncs::sNewLine[] = "\015\012";
#endif

#define TXTCONV_BUFFER_SIZE 20

HTMLOutContext::HTMLOutContext( rtl_TextEncoding eDestEnc )
{
	m_eDestEnc = RTL_TEXTENCODING_DONTKNOW == eDestEnc 
					? gsl_getSystemTextEncoding()
					: eDestEnc;

	m_hConv = rtl_createUnicodeToTextConverter( eDestEnc );
	DBG_ASSERT( m_hConv, 
		"HTMLOutContext::HTMLOutContext: no converter for source encoding" );
	m_hContext = m_hConv ? rtl_createUnicodeToTextContext( m_hConv ) 
					 : (rtl_TextToUnicodeContext)1;
}

HTMLOutContext::~HTMLOutContext()
{
	rtl_destroyUnicodeToTextContext( m_hConv, m_hContext );
	rtl_destroyUnicodeToTextConverter( m_hConv );
}

const sal_Char *lcl_svhtml_GetEntityForChar( sal_Unicode c, 
											 rtl_TextEncoding eDestEnc )
{
	const sal_Char* pStr = 0;

    // Note: We currently handle special cases for ISO-8859-2 here simply because
    // the code was already submitted.  But we should also handle other code pages
    // as well as the code becomes available.

    if( eDestEnc == RTL_TEXTENCODING_ISO_8859_2 || eDestEnc == RTL_TEXTENCODING_MS_1250 )
    {
        // Don't handle the following characters for Easter European (ISO-8859-2).
        switch ( c )
        {
        case 164: // curren
        case 184: // ccedil
        case 193: // Aacute
        case 194: // Acirc
        case 196: // Auml 
        case 199: // Ccedil
        case 201: // Eacute
        case 203: // Euml 
        case 205: // Iacute
        case 206: // Icirc
        case 211: // Oacute
        case 212: // Ocirc
        case 214: // Ouml 
        case 215: // times
        case 218: // Uacute
        case 220: // Uuml 
        case 221: // Yacute
        case 225: // aacute
        case 226: // acirc
        case 228: // auml 
        case 233: // eacute
        case 235: // euml 
        case 237: // iacute
        case 238: // icirc
        case 243: // oacute
        case 244: // ocirc
        case 246: // ouml 
        case 247: // divide
        case 250: // uacute
        case 252: // uuml
        case 253: // yacute
        case 352: // Scaron
        case 353: // scaron
            return pStr;
        }
    }

    // TODO: handle more special cases for other code pages.

	switch( c )
	{
//	    case '\x0a':   return HTMLOutFuncs::Out_Tag( rStream, OOO_STRING_SVTOOLS_HTML_linebreak );

	case '<':		pStr = OOO_STRING_SVTOOLS_HTML_C_lt;		break;
	case '>':		pStr = OOO_STRING_SVTOOLS_HTML_C_gt;		break;
	case '&':		pStr = OOO_STRING_SVTOOLS_HTML_C_amp;		break;
	case '"':		pStr = OOO_STRING_SVTOOLS_HTML_C_quot;	break;

	case 161:		pStr = OOO_STRING_SVTOOLS_HTML_S_iexcl;	break;
	case 162:		pStr = OOO_STRING_SVTOOLS_HTML_S_cent;	break;
	case 163:		pStr = OOO_STRING_SVTOOLS_HTML_S_pound;	break;
	case 164:		pStr = OOO_STRING_SVTOOLS_HTML_S_curren;	break;
	case 165:		pStr = OOO_STRING_SVTOOLS_HTML_S_yen;		break;
	case 166:		pStr = OOO_STRING_SVTOOLS_HTML_S_brvbar;	break;
	case 167:		pStr = OOO_STRING_SVTOOLS_HTML_S_sect;	break;
	case 168:		pStr = OOO_STRING_SVTOOLS_HTML_S_uml;		break;
	case 169:		pStr = OOO_STRING_SVTOOLS_HTML_S_copy;	break;
	case 170:		pStr = OOO_STRING_SVTOOLS_HTML_S_ordf;	break;
	case 171:		pStr = OOO_STRING_SVTOOLS_HTML_S_laquo;	break;
	case 172:		pStr = OOO_STRING_SVTOOLS_HTML_S_not;		break;
	case 174:		pStr = OOO_STRING_SVTOOLS_HTML_S_reg;		break;
	case 175:		pStr = OOO_STRING_SVTOOLS_HTML_S_macr;	break;
	case 176:		pStr = OOO_STRING_SVTOOLS_HTML_S_deg;		break;
	case 177:		pStr = OOO_STRING_SVTOOLS_HTML_S_plusmn;	break;
	case 178:		pStr = OOO_STRING_SVTOOLS_HTML_S_sup2;	break;
	case 179:		pStr = OOO_STRING_SVTOOLS_HTML_S_sup3;	break;
	case 180:		pStr = OOO_STRING_SVTOOLS_HTML_S_acute;	break;
	case 181:		pStr = OOO_STRING_SVTOOLS_HTML_S_micro;	break;
	case 182:		pStr = OOO_STRING_SVTOOLS_HTML_S_para;	break;
	case 183:		pStr = OOO_STRING_SVTOOLS_HTML_S_middot;	break;
	case 184:		pStr = OOO_STRING_SVTOOLS_HTML_S_cedil;	break;
	case 185:		pStr = OOO_STRING_SVTOOLS_HTML_S_sup1;	break;
	case 186:		pStr = OOO_STRING_SVTOOLS_HTML_S_ordm;	break;
	case 187:		pStr = OOO_STRING_SVTOOLS_HTML_S_raquo;	break;
	case 188:		pStr = OOO_STRING_SVTOOLS_HTML_S_frac14;	break;
	case 189:		pStr = OOO_STRING_SVTOOLS_HTML_S_frac12;	break;
	case 190:		pStr = OOO_STRING_SVTOOLS_HTML_S_frac34;	break;
	case 191:		pStr = OOO_STRING_SVTOOLS_HTML_S_iquest;	break;

	case 192:		pStr = OOO_STRING_SVTOOLS_HTML_C_Agrave;	break;
	case 193:		pStr = OOO_STRING_SVTOOLS_HTML_C_Aacute;	break;
	case 194:		pStr = OOO_STRING_SVTOOLS_HTML_C_Acirc;	break;
	case 195:		pStr = OOO_STRING_SVTOOLS_HTML_C_Atilde;	break;
	case 196:		pStr = OOO_STRING_SVTOOLS_HTML_C_Auml;	break;
	case 197:		pStr = OOO_STRING_SVTOOLS_HTML_C_Aring;	break;
	case 198:		pStr = OOO_STRING_SVTOOLS_HTML_C_AElig;	break;
	case 199:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ccedil;	break;
	case 200:		pStr = OOO_STRING_SVTOOLS_HTML_C_Egrave;	break;
	case 201:		pStr = OOO_STRING_SVTOOLS_HTML_C_Eacute;	break;
	case 202:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ecirc;	break;
	case 203:		pStr = OOO_STRING_SVTOOLS_HTML_C_Euml;	break;
	case 204:		pStr = OOO_STRING_SVTOOLS_HTML_C_Igrave;	break;
	case 205:		pStr = OOO_STRING_SVTOOLS_HTML_C_Iacute;	break;
	case 206:		pStr = OOO_STRING_SVTOOLS_HTML_C_Icirc;	break;
	case 207:		pStr = OOO_STRING_SVTOOLS_HTML_C_Iuml;	break;
	case 208:		pStr = OOO_STRING_SVTOOLS_HTML_C_ETH;		break;
	case 209:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ntilde;	break;
	case 210:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ograve;	break;
	case 211:		pStr = OOO_STRING_SVTOOLS_HTML_C_Oacute;	break;
	case 212:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ocirc;	break;
	case 213:		pStr = OOO_STRING_SVTOOLS_HTML_C_Otilde;	break;
	case 214:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ouml;	break;
	case 215:		pStr = OOO_STRING_SVTOOLS_HTML_S_times;	break;
	case 216:		pStr = OOO_STRING_SVTOOLS_HTML_C_Oslash;	break;
	case 217:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ugrave;	break;
	case 218:		pStr = OOO_STRING_SVTOOLS_HTML_C_Uacute;	break;
	case 219:		pStr = OOO_STRING_SVTOOLS_HTML_C_Ucirc;	break;
	case 220:		pStr = OOO_STRING_SVTOOLS_HTML_C_Uuml;	break;
	case 221:		pStr = OOO_STRING_SVTOOLS_HTML_C_Yacute;	break;

	case 222:		pStr = OOO_STRING_SVTOOLS_HTML_C_THORN;	break;
	case 223:		pStr = OOO_STRING_SVTOOLS_HTML_C_szlig;	break;

	case 224:		pStr = OOO_STRING_SVTOOLS_HTML_S_agrave;	break;
	case 225:		pStr = OOO_STRING_SVTOOLS_HTML_S_aacute;	break;
	case 226:		pStr = OOO_STRING_SVTOOLS_HTML_S_acirc;	break;
	case 227:		pStr = OOO_STRING_SVTOOLS_HTML_S_atilde;	break;
	case 228:		pStr = OOO_STRING_SVTOOLS_HTML_S_auml;	break;
	case 229:		pStr = OOO_STRING_SVTOOLS_HTML_S_aring;	break;
	case 230:		pStr = OOO_STRING_SVTOOLS_HTML_S_aelig;	break;
	case 231:		pStr = OOO_STRING_SVTOOLS_HTML_S_ccedil;	break;
	case 232:		pStr = OOO_STRING_SVTOOLS_HTML_S_egrave;	break;
	case 233:		pStr = OOO_STRING_SVTOOLS_HTML_S_eacute;	break;
	case 234:		pStr = OOO_STRING_SVTOOLS_HTML_S_ecirc;	break;
	case 235:		pStr = OOO_STRING_SVTOOLS_HTML_S_euml;	break;
	case 236:		pStr = OOO_STRING_SVTOOLS_HTML_S_igrave;	break;
	case 237:		pStr = OOO_STRING_SVTOOLS_HTML_S_iacute;	break;
	case 238:		pStr = OOO_STRING_SVTOOLS_HTML_S_icirc;	break;
	case 239:		pStr = OOO_STRING_SVTOOLS_HTML_S_iuml;	break;
	case 240:		pStr = OOO_STRING_SVTOOLS_HTML_S_eth;		break;
	case 241:		pStr = OOO_STRING_SVTOOLS_HTML_S_ntilde;	break;
	case 242:		pStr = OOO_STRING_SVTOOLS_HTML_S_ograve;	break;
	case 243:		pStr = OOO_STRING_SVTOOLS_HTML_S_oacute;	break;
	case 244:		pStr = OOO_STRING_SVTOOLS_HTML_S_ocirc;	break;
	case 245:		pStr = OOO_STRING_SVTOOLS_HTML_S_otilde;	break;
	case 246:		pStr = OOO_STRING_SVTOOLS_HTML_S_ouml;	break;
	case 247:		pStr = OOO_STRING_SVTOOLS_HTML_S_divide;	break;
	case 248:		pStr = OOO_STRING_SVTOOLS_HTML_S_oslash;	break;
	case 249:		pStr = OOO_STRING_SVTOOLS_HTML_S_ugrave;	break;
	case 250:		pStr = OOO_STRING_SVTOOLS_HTML_S_uacute;	break;
	case 251:		pStr = OOO_STRING_SVTOOLS_HTML_S_ucirc;	break;
	case 252:		pStr = OOO_STRING_SVTOOLS_HTML_S_uuml;	break;
	case 253:		pStr = OOO_STRING_SVTOOLS_HTML_S_yacute;	break;
	case 254:		pStr = OOO_STRING_SVTOOLS_HTML_S_thorn;	break;
	case 255:     	pStr = OOO_STRING_SVTOOLS_HTML_S_yuml;	break;

	case 338:		pStr = OOO_STRING_SVTOOLS_HTML_S_OElig;	break;
	case 339:		pStr = OOO_STRING_SVTOOLS_HTML_S_oelig;	break;
	case 352:		pStr = OOO_STRING_SVTOOLS_HTML_S_Scaron;	break;
	case 353:		pStr = OOO_STRING_SVTOOLS_HTML_S_scaron;	break;
	case 376:		pStr = OOO_STRING_SVTOOLS_HTML_S_Yuml;	break;
	case 402:		pStr = OOO_STRING_SVTOOLS_HTML_S_fnof;	break;
	case 710:		pStr = OOO_STRING_SVTOOLS_HTML_S_circ;	break;
	case 732:		pStr = OOO_STRING_SVTOOLS_HTML_S_tilde;	break;

	// Greek chars are handled later,
	// since they should *not* be transformed to entities
	// when generating Greek text (== using Greek encoding)

	case 8194:		pStr = OOO_STRING_SVTOOLS_HTML_S_ensp;	break;
	case 8195:		pStr = OOO_STRING_SVTOOLS_HTML_S_emsp;	break;
	case 8201:		pStr = OOO_STRING_SVTOOLS_HTML_S_thinsp;	break;
	case 8204:		pStr = OOO_STRING_SVTOOLS_HTML_S_zwnj;	break;
	case 8205:		pStr = OOO_STRING_SVTOOLS_HTML_S_zwj;		break;
	case 8206:		pStr = OOO_STRING_SVTOOLS_HTML_S_lrm;		break;
	case 8207:		pStr = OOO_STRING_SVTOOLS_HTML_S_rlm;		break;
	case 8211:		pStr = OOO_STRING_SVTOOLS_HTML_S_ndash;	break;
	case 8212:		pStr = OOO_STRING_SVTOOLS_HTML_S_mdash;	break;
	case 8216:		pStr = OOO_STRING_SVTOOLS_HTML_S_lsquo;	break;
	case 8217:		pStr = OOO_STRING_SVTOOLS_HTML_S_rsquo;	break;
	case 8218:		pStr = OOO_STRING_SVTOOLS_HTML_S_sbquo;	break;
	case 8220:		pStr = OOO_STRING_SVTOOLS_HTML_S_ldquo;	break;
	case 8221:		pStr = OOO_STRING_SVTOOLS_HTML_S_rdquo;	break;
	case 8222:		pStr = OOO_STRING_SVTOOLS_HTML_S_bdquo;	break;
	case 8224:		pStr = OOO_STRING_SVTOOLS_HTML_S_dagger;	break;
	case 8225:		pStr = OOO_STRING_SVTOOLS_HTML_S_Dagger;	break;
	case 8226:		pStr = OOO_STRING_SVTOOLS_HTML_S_bull;	break;
	case 8230:		pStr = OOO_STRING_SVTOOLS_HTML_S_hellip;	break;
	case 8240:		pStr = OOO_STRING_SVTOOLS_HTML_S_permil;	break;
	case 8242:		pStr = OOO_STRING_SVTOOLS_HTML_S_prime;	break;
	case 8243:		pStr = OOO_STRING_SVTOOLS_HTML_S_Prime;	break;
	case 8249:		pStr = OOO_STRING_SVTOOLS_HTML_S_lsaquo;	break;
	case 8250:		pStr = OOO_STRING_SVTOOLS_HTML_S_rsaquo;	break;
	case 8254:		pStr = OOO_STRING_SVTOOLS_HTML_S_oline;	break;
	case 8260:		pStr = OOO_STRING_SVTOOLS_HTML_S_frasl;	break;
	case 8364:		pStr = OOO_STRING_SVTOOLS_HTML_S_euro;	break;
	case 8465:		pStr = OOO_STRING_SVTOOLS_HTML_S_image;	break;
	case 8472:		pStr = OOO_STRING_SVTOOLS_HTML_S_weierp;	break;
	case 8476:		pStr = OOO_STRING_SVTOOLS_HTML_S_real;	break;
	case 8482:		pStr = OOO_STRING_SVTOOLS_HTML_S_trade;	break;
	case 8501:		pStr = OOO_STRING_SVTOOLS_HTML_S_alefsym;	break;
	case 8592:		pStr = OOO_STRING_SVTOOLS_HTML_S_larr;	break;
	case 8593:		pStr = OOO_STRING_SVTOOLS_HTML_S_uarr;	break;
	case 8594:		pStr = OOO_STRING_SVTOOLS_HTML_S_rarr;	break;
	case 8595:		pStr = OOO_STRING_SVTOOLS_HTML_S_darr;	break;
	case 8596:		pStr = OOO_STRING_SVTOOLS_HTML_S_harr;	break;
	case 8629:		pStr = OOO_STRING_SVTOOLS_HTML_S_crarr;	break;
	case 8656:		pStr = OOO_STRING_SVTOOLS_HTML_S_lArr;	break;
	case 8657:		pStr = OOO_STRING_SVTOOLS_HTML_S_uArr;	break;
	case 8658:		pStr = OOO_STRING_SVTOOLS_HTML_S_rArr;	break;
	case 8659:		pStr = OOO_STRING_SVTOOLS_HTML_S_dArr;	break;
	case 8660:		pStr = OOO_STRING_SVTOOLS_HTML_S_hArr;	break;
	case 8704:		pStr = OOO_STRING_SVTOOLS_HTML_S_forall;	break;
	case 8706:		pStr = OOO_STRING_SVTOOLS_HTML_S_part;	break;
	case 8707:		pStr = OOO_STRING_SVTOOLS_HTML_S_exist;	break;
	case 8709:		pStr = OOO_STRING_SVTOOLS_HTML_S_empty;	break;
	case 8711:		pStr = OOO_STRING_SVTOOLS_HTML_S_nabla;	break;
	case 8712:		pStr = OOO_STRING_SVTOOLS_HTML_S_isin;	break;
	case 8713:		pStr = OOO_STRING_SVTOOLS_HTML_S_notin;	break;
	case 8715:		pStr = OOO_STRING_SVTOOLS_HTML_S_ni;		break;
	case 8719:		pStr = OOO_STRING_SVTOOLS_HTML_S_prod;	break;
	case 8721:		pStr = OOO_STRING_SVTOOLS_HTML_S_sum;		break;
	case 8722:		pStr = OOO_STRING_SVTOOLS_HTML_S_minus;	break;
	case 8727:		pStr = OOO_STRING_SVTOOLS_HTML_S_lowast;	break;
	case 8730:		pStr = OOO_STRING_SVTOOLS_HTML_S_radic;	break;
	case 8733:		pStr = OOO_STRING_SVTOOLS_HTML_S_prop;	break;
	case 8734:		pStr = OOO_STRING_SVTOOLS_HTML_S_infin;	break;
	case 8736:		pStr = OOO_STRING_SVTOOLS_HTML_S_ang;		break;
	case 8743:		pStr = OOO_STRING_SVTOOLS_HTML_S_and;		break;
	case 8744:		pStr = OOO_STRING_SVTOOLS_HTML_S_or;		break;
	case 8745:		pStr = OOO_STRING_SVTOOLS_HTML_S_cap;		break;
	case 8746:		pStr = OOO_STRING_SVTOOLS_HTML_S_cup;		break;
	case 8747:		pStr = OOO_STRING_SVTOOLS_HTML_S_int;		break;
	case 8756:		pStr = OOO_STRING_SVTOOLS_HTML_S_there4;	break;
	case 8764:		pStr = OOO_STRING_SVTOOLS_HTML_S_sim;		break;
	case 8773:		pStr = OOO_STRING_SVTOOLS_HTML_S_cong;	break;
	case 8776:		pStr = OOO_STRING_SVTOOLS_HTML_S_asymp;	break;
	case 8800:		pStr = OOO_STRING_SVTOOLS_HTML_S_ne;		break;
	case 8801:		pStr = OOO_STRING_SVTOOLS_HTML_S_equiv;	break;
	case 8804:		pStr = OOO_STRING_SVTOOLS_HTML_S_le;		break;
	case 8805:		pStr = OOO_STRING_SVTOOLS_HTML_S_ge;		break;
	case 8834:		pStr = OOO_STRING_SVTOOLS_HTML_S_sub;		break;
	case 8835:		pStr = OOO_STRING_SVTOOLS_HTML_S_sup;		break;
	case 8836:		pStr = OOO_STRING_SVTOOLS_HTML_S_nsub;	break;
	case 8838:		pStr = OOO_STRING_SVTOOLS_HTML_S_sube;	break;
	case 8839:		pStr = OOO_STRING_SVTOOLS_HTML_S_supe;	break;
	case 8853:		pStr = OOO_STRING_SVTOOLS_HTML_S_oplus;	break;
	case 8855:		pStr = OOO_STRING_SVTOOLS_HTML_S_otimes;	break;
	case 8869:		pStr = OOO_STRING_SVTOOLS_HTML_S_perp;	break;
	case 8901:		pStr = OOO_STRING_SVTOOLS_HTML_S_sdot;	break;
	case 8968:		pStr = OOO_STRING_SVTOOLS_HTML_S_lceil;	break;
	case 8969:		pStr = OOO_STRING_SVTOOLS_HTML_S_rceil;	break;
	case 8970:		pStr = OOO_STRING_SVTOOLS_HTML_S_lfloor;	break;
	case 8971:		pStr = OOO_STRING_SVTOOLS_HTML_S_rfloor;	break;
	case 9001:		pStr = OOO_STRING_SVTOOLS_HTML_S_lang;	break;
	case 9002:		pStr = OOO_STRING_SVTOOLS_HTML_S_rang;	break;
	case 9674:		pStr = OOO_STRING_SVTOOLS_HTML_S_loz;		break;
	case 9824:		pStr = OOO_STRING_SVTOOLS_HTML_S_spades;	break;
	case 9827:		pStr = OOO_STRING_SVTOOLS_HTML_S_clubs;	break;
	case 9829:		pStr = OOO_STRING_SVTOOLS_HTML_S_hearts;	break;
	case 9830:		pStr = OOO_STRING_SVTOOLS_HTML_S_diams;	break;
	}

	// Greek chars: if we do not produce a Greek encoding,
	// transform them into entities
	if( !pStr &&
		( eDestEnc != RTL_TEXTENCODING_ISO_8859_7 ) &&
	    ( eDestEnc != RTL_TEXTENCODING_MS_1253 ) )
	{
		switch( c )
		{
		case 913:		pStr = OOO_STRING_SVTOOLS_HTML_S_Alpha;	break;
		case 914:		pStr = OOO_STRING_SVTOOLS_HTML_S_Beta;	break;
		case 915:		pStr = OOO_STRING_SVTOOLS_HTML_S_Gamma;	break;
		case 916:		pStr = OOO_STRING_SVTOOLS_HTML_S_Delta;	break;
		case 917:		pStr = OOO_STRING_SVTOOLS_HTML_S_Epsilon;	break;
		case 918:		pStr = OOO_STRING_SVTOOLS_HTML_S_Zeta;	break;
		case 919:		pStr = OOO_STRING_SVTOOLS_HTML_S_Eta;		break;
		case 920:		pStr = OOO_STRING_SVTOOLS_HTML_S_Theta;	break;
		case 921:		pStr = OOO_STRING_SVTOOLS_HTML_S_Iota;	break;
		case 922:		pStr = OOO_STRING_SVTOOLS_HTML_S_Kappa;	break;
		case 923:		pStr = OOO_STRING_SVTOOLS_HTML_S_Lambda;	break;
		case 924:		pStr = OOO_STRING_SVTOOLS_HTML_S_Mu;		break;
		case 925:		pStr = OOO_STRING_SVTOOLS_HTML_S_Nu;		break;
		case 926:		pStr = OOO_STRING_SVTOOLS_HTML_S_Xi;		break;
		case 927:		pStr = OOO_STRING_SVTOOLS_HTML_S_Omicron;	break;
		case 928:		pStr = OOO_STRING_SVTOOLS_HTML_S_Pi;		break;
		case 929:		pStr = OOO_STRING_SVTOOLS_HTML_S_Rho;		break;
		case 931:		pStr = OOO_STRING_SVTOOLS_HTML_S_Sigma;	break;
		case 932:		pStr = OOO_STRING_SVTOOLS_HTML_S_Tau;		break;
		case 933:		pStr = OOO_STRING_SVTOOLS_HTML_S_Upsilon;	break;
		case 934:		pStr = OOO_STRING_SVTOOLS_HTML_S_Phi;		break;
		case 935:		pStr = OOO_STRING_SVTOOLS_HTML_S_Chi;		break;
		case 936:		pStr = OOO_STRING_SVTOOLS_HTML_S_Psi;		break;
		case 937:		pStr = OOO_STRING_SVTOOLS_HTML_S_Omega;	break;
		case 945:		pStr = OOO_STRING_SVTOOLS_HTML_S_alpha;	break;
		case 946:		pStr = OOO_STRING_SVTOOLS_HTML_S_beta;	break;
		case 947:		pStr = OOO_STRING_SVTOOLS_HTML_S_gamma;	break;
		case 948:		pStr = OOO_STRING_SVTOOLS_HTML_S_delta;	break;
		case 949:		pStr = OOO_STRING_SVTOOLS_HTML_S_epsilon;	break;
		case 950:		pStr = OOO_STRING_SVTOOLS_HTML_S_zeta;	break;
		case 951:		pStr = OOO_STRING_SVTOOLS_HTML_S_eta;		break;
		case 952:		pStr = OOO_STRING_SVTOOLS_HTML_S_theta;	break;
		case 953:		pStr = OOO_STRING_SVTOOLS_HTML_S_iota;	break;
		case 954:		pStr = OOO_STRING_SVTOOLS_HTML_S_kappa;	break;
		case 955:		pStr = OOO_STRING_SVTOOLS_HTML_S_lambda;	break;
		case 956:		pStr = OOO_STRING_SVTOOLS_HTML_S_mu;		break;
		case 957:		pStr = OOO_STRING_SVTOOLS_HTML_S_nu;		break;
		case 958:		pStr = OOO_STRING_SVTOOLS_HTML_S_xi;		break;
		case 959:		pStr = OOO_STRING_SVTOOLS_HTML_S_omicron;	break;
		case 960:		pStr = OOO_STRING_SVTOOLS_HTML_S_pi;		break;
		case 961:		pStr = OOO_STRING_SVTOOLS_HTML_S_rho;		break;
		case 962:		pStr = OOO_STRING_SVTOOLS_HTML_S_sigmaf;	break;
		case 963:		pStr = OOO_STRING_SVTOOLS_HTML_S_sigma;	break;
		case 964:		pStr = OOO_STRING_SVTOOLS_HTML_S_tau;		break;
		case 965:		pStr = OOO_STRING_SVTOOLS_HTML_S_upsilon;	break;
		case 966:		pStr = OOO_STRING_SVTOOLS_HTML_S_phi;		break;
		case 967:		pStr = OOO_STRING_SVTOOLS_HTML_S_chi;		break;
		case 968:		pStr = OOO_STRING_SVTOOLS_HTML_S_psi;		break;
		case 969:		pStr = OOO_STRING_SVTOOLS_HTML_S_omega;	break;
		case 977:		pStr = OOO_STRING_SVTOOLS_HTML_S_thetasym;break;
		case 978:		pStr = OOO_STRING_SVTOOLS_HTML_S_upsih;	break;
		case 982:		pStr = OOO_STRING_SVTOOLS_HTML_S_piv;		break;
		}
	}

	return pStr;
}

void lcl_ConvertCharToHTML( sal_Unicode c, ByteString& rDest,
							HTMLOutContext& rContext,
							String *pNonConvertableChars )
{
	DBG_ASSERT( RTL_TEXTENCODING_DONTKNOW != rContext.m_eDestEnc,
					"wrong destination encoding" );
	const sal_Char *pStr = 0;
	switch( c )
	{
	case 0xA0:		// is a hard blank
//!! the TextConverter has a problem with this character - so change it to
// a hard space - that's the same as our 5.2
	case 0x2011:	// is a hard hyphen
		pStr = OOO_STRING_SVTOOLS_HTML_S_nbsp;
		break;
	case 0xAD:		// is a soft hyphen
		pStr = OOO_STRING_SVTOOLS_HTML_S_shy;
		break;
	default:
		// There may be an entity for the character.
		// The new HTML4 entities above 255 are not used for UTF-8,
		// because Netscape 4 does support UTF-8 but does not support
		// these entities.
		if( c < 128 || RTL_TEXTENCODING_UTF8 != rContext.m_eDestEnc )
			pStr = lcl_svhtml_GetEntityForChar( c, rContext.m_eDestEnc );
		break;
	}

	sal_Char cBuffer[TXTCONV_BUFFER_SIZE];
	sal_uInt32 nInfo = 0;
	sal_Size nSrcChars;
	const sal_uInt32 nFlags = RTL_UNICODETOTEXT_FLAGS_NONSPACING_IGNORE|
						RTL_UNICODETOTEXT_FLAGS_CONTROL_IGNORE|
						RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR|
						RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR;
	if( pStr )
	{
		sal_Size nLen = rtl_convertUnicodeToText( 
							rContext.m_hConv, rContext.m_hContext, &c, 0,
							cBuffer, TXTCONV_BUFFER_SIZE, 
							nFlags|RTL_UNICODETOTEXT_FLAGS_FLUSH,
							&nInfo, &nSrcChars );
		DBG_ASSERT( (nInfo & (RTL_UNICODETOTEXT_INFO_ERROR|RTL_UNICODETOTEXT_INFO_DESTBUFFERTOSMALL)) == 0, "HTMLOut: error while flushing" );
		sal_Char *pBuffer = cBuffer;
		while( nLen-- )
			rDest += *pBuffer++;
		((rDest += '&') += pStr) += ';';
	}
	else
	{
		sal_Size nLen = rtl_convertUnicodeToText( rContext.m_hConv, 
												  rContext.m_hContext, &c, 1,
										   		  cBuffer, TXTCONV_BUFFER_SIZE,
												  nFlags,
												  &nInfo, &nSrcChars );
		if( nLen > 0 && (nInfo & (RTL_UNICODETOTEXT_INFO_ERROR|RTL_UNICODETOTEXT_INFO_DESTBUFFERTOSMALL)) == 0 )
		{
			sal_Char *pBuffer = cBuffer;
			while( nLen-- )
				rDest += *pBuffer++;
		}
		else
		{
			// If the character could not be converted to the destination
			// character set, the UNICODE character is exported as character
			// entity.
			nLen = rtl_convertUnicodeToText( 
								rContext.m_hConv, rContext.m_hContext, &c, 0,
								cBuffer, TXTCONV_BUFFER_SIZE, 
								nFlags|RTL_UNICODETOTEXT_FLAGS_FLUSH,
								&nInfo, &nSrcChars );
			DBG_ASSERT( (nInfo & (RTL_UNICODETOTEXT_INFO_ERROR|RTL_UNICODETOTEXT_INFO_DESTBUFFERTOSMALL)) == 0, "HTMLOut: error while flushing" );
			sal_Char *pBuffer = cBuffer;
			while( nLen-- )
				rDest += *pBuffer++;

			(((rDest += '&') += '#') +=
					ByteString::CreateFromInt64( (sal_uInt32)c )) += ';';
			if( pNonConvertableChars &&
				STRING_NOTFOUND == pNonConvertableChars->Search( c ) )
				pNonConvertableChars->Append( c );
		}
	}
}

sal_Bool lcl_FlushToAscii( ByteString& rDest, HTMLOutContext& rContext )
{
	sal_Unicode c = 0;
	sal_Char cBuffer[TXTCONV_BUFFER_SIZE];
	sal_uInt32 nInfo = 0;
	sal_Size nSrcChars;
	const sal_uInt32 nFlags = RTL_UNICODETOTEXT_FLAGS_NONSPACING_IGNORE|
						RTL_UNICODETOTEXT_FLAGS_CONTROL_IGNORE|
						RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR|
						RTL_UNICODETOTEXT_FLAGS_FLUSH|
						RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR;
	sal_Size nLen = rtl_convertUnicodeToText( 
						rContext.m_hConv, rContext.m_hContext, &c, 0,
						cBuffer, TXTCONV_BUFFER_SIZE, nFlags,
						&nInfo, &nSrcChars );
	DBG_ASSERT( (nInfo & (RTL_UNICODETOTEXT_INFO_ERROR|RTL_UNICODETOTEXT_INFO_DESTBUFFERTOSMALL)) == 0, "HTMLOut: error while flushing" );
	sal_Bool bRet = nLen > 0;
	sal_Char *pBuffer = cBuffer;
	while( nLen-- )
		rDest += *pBuffer++;
	return bRet;
}

void HTMLOutFuncs::ConvertStringToHTML( const String& rSrc,
										ByteString& rDest,
										rtl_TextEncoding eDestEnc,
									 	String *pNonConvertableChars )
{
	HTMLOutContext aContext( eDestEnc );
	for( sal_uInt32 i=0UL, nLen = rSrc.Len(); i < nLen; i++ )
		lcl_ConvertCharToHTML( rSrc.GetChar( (xub_StrLen)i ), rDest, aContext,
							   pNonConvertableChars );
	lcl_FlushToAscii( rDest, aContext );
}

SvStream& HTMLOutFuncs::Out_AsciiTag( SvStream& rStream, const sal_Char *pStr,
									  BOOL bOn, rtl_TextEncoding )
{
	sal_Char sStt[3] = "</";
	if( bOn )
		sStt[1] = 0;
	return (rStream << sStt << pStr << '>');
}

SvStream& HTMLOutFuncs::Out_Char( SvStream& rStream, sal_Unicode c,
								  HTMLOutContext& rContext, 
								  String *pNonConvertableChars )
{
	ByteString sOut;
	lcl_ConvertCharToHTML( c, sOut,  rContext, pNonConvertableChars );
	rStream << sOut.GetBuffer();
	return rStream;
}

SvStream& HTMLOutFuncs::Out_String( SvStream& rStream, const String& rStr,
								    rtl_TextEncoding eDestEnc,
								    String *pNonConvertableChars )
{
	HTMLOutContext aContext( eDestEnc );
	xub_StrLen nLen = rStr.Len();
	for( xub_StrLen n = 0; n < nLen; n++ )
		HTMLOutFuncs::Out_Char( rStream, rStr.GetChar( (xub_StrLen)n ),
								aContext, pNonConvertableChars );
	HTMLOutFuncs::FlushToAscii( rStream, aContext );
	return rStream;
}

SvStream& HTMLOutFuncs::FlushToAscii( SvStream& rStream,
		 							  HTMLOutContext& rContext )
{
	ByteString sOut;
	if( lcl_FlushToAscii( sOut, rContext ) )
		rStream << sOut.GetBuffer();

	return rStream;
}

SvStream& HTMLOutFuncs::Out_Hex( SvStream& rStream, ULONG nHex, BYTE nLen,
							  	 rtl_TextEncoding )
{												   // in einen Stream aus
	sal_Char aNToABuf[] = "0000000000000000";

	DBG_ASSERT( nLen < sizeof(aNToABuf), "zu viele Stellen" );
	if( nLen>=sizeof(aNToABuf) )
		nLen = (sizeof(aNToABuf)-1);

	// Pointer an das Bufferende setzen
	sal_Char *pStr = aNToABuf + (sizeof(aNToABuf)-1);
	for( BYTE n = 0; n < nLen; ++n )
	{
		*(--pStr) = (sal_Char)(nHex & 0xf ) + 48;
		if( *pStr > '9' )
			*pStr += 39;
		nHex >>= 4;
	}
	return rStream << pStr;
}


SvStream& HTMLOutFuncs::Out_Color( SvStream& rStream, const Color& rColor,
								   rtl_TextEncoding )
{
	rStream << "\"#";
	if( rColor.GetColor() == COL_AUTO )
	{
		rStream << "000000";
	}
	else
	{
		Out_Hex( rStream, rColor.GetRed(), 2 );
		Out_Hex( rStream, rColor.GetGreen(), 2 );
		Out_Hex( rStream, rColor.GetBlue(), 2 );
	}
	rStream << '\"';

	return rStream;
}

SvStream& HTMLOutFuncs::Out_ImageMap( SvStream& rStream,
                                      const String& rBaseURL,
									  const ImageMap& rIMap,
									  const String& rName,
									  const HTMLOutEvent *pEventTable,
									  BOOL bOutStarBasic,
									  const sal_Char *pDelim,
									  const sal_Char *pIndentArea,
									  const sal_Char *pIndentMap,
								      rtl_TextEncoding eDestEnc,
								  	  String *pNonConvertableChars	)
{
	if( RTL_TEXTENCODING_DONTKNOW == eDestEnc )
		eDestEnc = gsl_getSystemTextEncoding();

	const String& rOutName = rName.Len() ? rName : rIMap.GetName();
	DBG_ASSERT( rOutName.Len(), "Kein ImageMap-Name" );
	if( !rOutName.Len() )
		return rStream;

	ByteString sOut( '<' );
	sOut.Append( RTL_CONSTASCII_STRINGPARAM(OOO_STRING_SVTOOLS_HTML_map ) );
	sOut.Append( ' ' );
	sOut.Append( RTL_CONSTASCII_STRINGPARAM(OOO_STRING_SVTOOLS_HTML_O_name) );
	sOut.Append( RTL_CONSTASCII_STRINGPARAM("=\"") );
	rStream << sOut.GetBuffer();
	sOut.Erase();
	Out_String( rStream, rOutName, eDestEnc, pNonConvertableChars );
	rStream << "\">";

	for( USHORT i=0U; i<rIMap.GetIMapObjectCount(); i++ )
	{
		const IMapObject* pObj = rIMap.GetIMapObject( i );
		DBG_ASSERT( pObj, "Wo ist das ImageMap-Object?" );

		if( pObj )
		{
			const sal_Char *pShape = 0;
			ByteString aCoords;
			switch( pObj->GetType() )
			{
			case( IMAP_OBJ_RECTANGLE ):
				{
					const IMapRectangleObject* pRectObj =
						(const IMapRectangleObject *)pObj;
					pShape = OOO_STRING_SVTOOLS_HTML_SH_rect;
					Rectangle aRect( pRectObj->GetRectangle() );
					((((((aCoords =
						ByteString::CreateFromInt32(aRect.Left())) += ',')
						+= ByteString::CreateFromInt32(aRect.Top())) += ',')
						+= ByteString::CreateFromInt32(aRect.Right())) += ',')
						+= ByteString::CreateFromInt32(aRect.Bottom());
				}
				break;
			case( IMAP_OBJ_CIRCLE ):
				{
					const IMapCircleObject* pCirc =
						(const IMapCircleObject *)pObj;
					pShape= OOO_STRING_SVTOOLS_HTML_SH_circ;
					Point aCenter( pCirc->GetCenter() );
					long nOff = pCirc->GetRadius();
					((((aCoords =
						ByteString::CreateFromInt32(aCenter.X())) += ',')
						+= ByteString::CreateFromInt32(aCenter.Y())) += ',')
						+= ByteString::CreateFromInt32(nOff);
				}
				break;
			case( IMAP_OBJ_POLYGON ):
				{
					const IMapPolygonObject* pPolyObj =
						(const IMapPolygonObject *)pObj;
					pShape= OOO_STRING_SVTOOLS_HTML_SH_poly;
					Polygon aPoly( pPolyObj->GetPolygon() );
					USHORT nCount = aPoly.GetSize();
					if( nCount>0 )
					{
						const Point& rPoint = aPoly[0];
						((aCoords =
							ByteString::CreateFromInt32(rPoint.X())) += ',')
							+= ByteString::CreateFromInt32(rPoint.Y());
					}
					for( USHORT j=1; j<nCount; j++ )
					{
						const Point& rPoint = aPoly[j];
						(((aCoords += ',')
						  	+= ByteString::CreateFromInt32(rPoint.X())) += ',')
							+= ByteString::CreateFromInt32(rPoint.Y());
					}
				}
				break;
			default:
				DBG_ASSERT( pShape, "unbekanntes IMapObject" );
				break;
			}

			if( pShape )
			{
				if( pDelim )
					rStream << pDelim;
				if( pIndentArea )
					rStream << pIndentArea;

				((((((((((sOut = '<') += OOO_STRING_SVTOOLS_HTML_area) += ' ')
					+= OOO_STRING_SVTOOLS_HTML_O_shape) += '=') += pShape) += ' ')
					+= OOO_STRING_SVTOOLS_HTML_O_coords) += "=\"") += aCoords) += "\" ";
				rStream << sOut.GetBuffer();

				String aURL( pObj->GetURL() );
				if( aURL.Len() && pObj->IsActive() )
				{
                    aURL = URIHelper::simpleNormalizedMakeRelative(
                        rBaseURL, aURL );
					(sOut = OOO_STRING_SVTOOLS_HTML_O_href) += "=\"";
					rStream << sOut.GetBuffer();
					Out_String( rStream, aURL, eDestEnc, pNonConvertableChars ) << '\"';
				}
				else
					rStream << OOO_STRING_SVTOOLS_HTML_O_nohref;

				const String& rObjName = pObj->GetName();
				if( rObjName.Len() )
				{
					((sOut = ' ') += OOO_STRING_SVTOOLS_HTML_O_name) += "=\"";
					rStream << sOut.GetBuffer();
					Out_String( rStream, rObjName, eDestEnc, pNonConvertableChars ) << '\"';
				}

				const String& rTarget = pObj->GetTarget();
				if( rTarget.Len() && pObj->IsActive() )
				{
					((sOut = ' ') += OOO_STRING_SVTOOLS_HTML_O_target) += "=\"";
					rStream << sOut.GetBuffer();
					Out_String( rStream, rTarget, eDestEnc, pNonConvertableChars ) << '\"';
				}

				String rDesc( pObj->GetAltText() );
				if( rDesc.Len() == 0 )
					rDesc = pObj->GetDesc();

				if( rDesc.Len() )
				{
					((sOut = ' ') += OOO_STRING_SVTOOLS_HTML_O_alt) += "=\"";
					rStream << sOut.GetBuffer();
					Out_String( rStream, rDesc, eDestEnc, pNonConvertableChars ) << '\"';
				}

				const SvxMacroTableDtor& rMacroTab = pObj->GetMacroTable();
				if( pEventTable && rMacroTab.Count() )
					Out_Events( rStream, rMacroTab, pEventTable,
								bOutStarBasic, eDestEnc, pNonConvertableChars );

				rStream << '>';
			}
		}

	}

	if( pDelim )
		rStream << pDelim;
	if( pIndentMap )
		rStream << pIndentMap;
	Out_AsciiTag( rStream, OOO_STRING_SVTOOLS_HTML_map, FALSE );

	return rStream;
}

SvStream& HTMLOutFuncs::OutScript( SvStream& rStrm, 
                                   const String& rBaseURL,
								   const String& rSource,
								   const String& rLanguage,
								   ScriptType eScriptType,
								   const String& rSrc,
								   const String *pSBLibrary,
								   const String *pSBModule,
								   rtl_TextEncoding eDestEnc,
								   String *pNonConvertableChars )
{
	if( RTL_TEXTENCODING_DONTKNOW == eDestEnc )
		eDestEnc = gsl_getSystemTextEncoding();

	// Script wird komplett nicht eingerueckt!
	ByteString sOut( '<' );
	sOut.Append( RTL_CONSTASCII_STRINGPARAM(OOO_STRING_SVTOOLS_HTML_script) );

	if( rLanguage.Len() )
	{
		sOut.Append( ' ' );
		sOut.Append( RTL_CONSTASCII_STRINGPARAM(OOO_STRING_SVTOOLS_HTML_O_language) );
		sOut.Append( RTL_CONSTASCII_STRINGPARAM("=\"") );
		rStrm << sOut.GetBuffer();
		Out_String( rStrm, rLanguage, eDestEnc, pNonConvertableChars );
		sOut = '\"';
	}

	if( rSrc.Len() )
	{
		((sOut += ' ') += OOO_STRING_SVTOOLS_HTML_O_src) += "=\"";
		rStrm << sOut.GetBuffer();
        Out_String( rStrm, URIHelper::simpleNormalizedMakeRelative(rBaseURL, rSrc), eDestEnc, pNonConvertableChars );
		sOut = '\"';
	}

	if( STARBASIC != eScriptType && pSBLibrary )
	{
		((sOut += ' ') += OOO_STRING_SVTOOLS_HTML_O_sdlibrary) += "=\"";
		rStrm << sOut.GetBuffer();
		Out_String( rStrm, *pSBLibrary, eDestEnc, pNonConvertableChars );
		sOut = '\"';
	}

	if( STARBASIC != eScriptType && pSBModule )
	{
		((sOut += ' ') += OOO_STRING_SVTOOLS_HTML_O_sdmodule) += "=\"";
		rStrm << sOut.GetBuffer();
		Out_String( rStrm, *pSBModule, eDestEnc, pNonConvertableChars );
		sOut = '\"';
	}

	sOut += '>';

	rStrm << sOut.GetBuffer();

	if( rSource.Len() || pSBLibrary || pSBModule )
	{
		rStrm << sNewLine;

		if( JAVASCRIPT != eScriptType )
		{
			rStrm << "<!--"
				  << sNewLine;
		}

		if( STARBASIC == eScriptType )
		{
			if( pSBLibrary )
			{
				sOut.Assign( RTL_CONSTASCII_STRINGPARAM("' ") );
				sOut.Append( RTL_CONSTASCII_STRINGPARAM(OOO_STRING_SVTOOLS_HTML_SB_library) );
				sOut.Append( ' ' );
				ByteString sTmp( *pSBLibrary, eDestEnc );
				sOut.Append( sTmp );
				rStrm << sOut.GetBuffer() << sNewLine;
			}

			if( pSBModule )
			{
				sOut.Assign( RTL_CONSTASCII_STRINGPARAM("' ") );
				sOut.Append( RTL_CONSTASCII_STRINGPARAM(OOO_STRING_SVTOOLS_HTML_SB_module) );
				sOut.Append( ' ' );
				ByteString sTmp( *pSBModule, eDestEnc );
				sOut.Append( sTmp );
				rStrm << sOut.GetBuffer() << sNewLine;
			}
		}

		if( rSource.Len() )
		{
			// Wir schreiben das Modul mm ANSI-Zeichensatz, aber mit
			// System-Zeilenumbruechen raus.
			ByteString sSource( rSource, eDestEnc );
			sSource.ConvertLineEnd( GetSystemLineEnd() );
			rStrm << sSource.GetBuffer();
		}
		rStrm << sNewLine;

		if( JAVASCRIPT != eScriptType )
		{
			// MIB/MM: Wenn es kein StarBasic ist, kann ein // natuerlich
			// falsch sein. Da der Kommentar aber beim Einlesen wider
			// entfernt wird, schickt uns das nicht weiter ...
			rStrm << (STARBASIC == eScriptType ? "' -->" : "// -->")
				  << sNewLine;
		}
	}

	HTMLOutFuncs::Out_AsciiTag( rStrm, OOO_STRING_SVTOOLS_HTML_script, FALSE );

	return rStrm;
}


SvStream& HTMLOutFuncs::Out_Events( SvStream& rStrm,
									const SvxMacroTableDtor& rMacroTable,
									const HTMLOutEvent *pEventTable,
									BOOL bOutStarBasic,
								    rtl_TextEncoding eDestEnc,
								    String *pNonConvertableChars )
{
	USHORT i=0;
	while( pEventTable[i].pBasicName || pEventTable[i].pJavaName )
	{
		const SvxMacro *pMacro =
			rMacroTable.Get( pEventTable[i].nEvent );

		if( pMacro && pMacro->GetMacName().Len() &&
			( JAVASCRIPT == pMacro->GetScriptType() || bOutStarBasic ))
		{
			const sal_Char *pStr = STARBASIC == pMacro->GetScriptType()
				? pEventTable[i].pBasicName
				: pEventTable[i].pJavaName;

			if( pStr )
			{
				ByteString sOut( ' ' );
				(sOut += pStr) += "=\"";
				rStrm << sOut.GetBuffer();

				Out_String( rStrm, pMacro->GetMacName(), eDestEnc, pNonConvertableChars ) << '\"';
			}
		}
		i++;
	}

	return rStrm;
}

ByteString& HTMLOutFuncs::CreateTableDataOptionsValNum( ByteString& aStrTD,
			BOOL bValue,
			double fVal, ULONG nFormat, SvNumberFormatter& rFormatter,
            rtl_TextEncoding eDestEnc, String* pNonConvertableChars )
{
	if ( bValue )
	{
		// printf / scanf ist zu ungenau
		String aValStr;
		rFormatter.GetInputLineString( fVal, 0, aValStr );
		ByteString sTmp( aValStr, eDestEnc );
		((((aStrTD += ' ') += OOO_STRING_SVTOOLS_HTML_O_SDval) += "=\"") += sTmp) += '\"';
	}
	if ( bValue || nFormat )
	{
		((aStrTD += ' ') += OOO_STRING_SVTOOLS_HTML_O_SDnum) += "=\"";
		(aStrTD += ByteString::CreateFromInt32(
								Application::GetSettings().GetLanguage() ))
			+= ';';	// Language fuer Format 0
		if ( nFormat )
		{
			ByteString aNumStr;
			LanguageType nLang;
			const SvNumberformat* pFormatEntry = rFormatter.GetEntry( nFormat );
			if ( pFormatEntry )
			{
                ConvertStringToHTML( pFormatEntry->GetFormatstring(), aNumStr,
                    eDestEnc, pNonConvertableChars );
				nLang = pFormatEntry->GetLanguage();
			}
			else
				nLang = LANGUAGE_SYSTEM;
			((aStrTD += ByteString::CreateFromInt32(nLang)) += ';') += aNumStr;
		}
		aStrTD += '\"';
	}
	return aStrTD;
}

BOOL HTMLOutFuncs::PrivateURLToInternalImg( String& rURL )
{
	if( rURL.Len() > 14UL &&
		rURL.CompareToAscii( OOO_STRING_SVTOOLS_HTML_private_image, 14UL ) == COMPARE_EQUAL )
	{
		rURL.Erase( 0UL, 14UL );
		return TRUE;
	}

	return FALSE;
}


