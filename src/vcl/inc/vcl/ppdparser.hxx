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
#ifndef _PSPRINT_PPDPARSER_HXX_
#define _PSPRINT_PPDPARSER_HXX_

#include <list>
#include <vector>
#include <hash_map>

#include "tools/string.hxx"
#include "tools/stream.hxx"

#define PRINTER_PPDDIR "driver"

namespace psp {

class PPDParser;

enum PPDValueType { eInvocation, eQuoted, eSymbol, eString, eNo };

struct PPDValue
{
    PPDValueType    m_eType;
    String          m_aOption;
    String          m_aOptionTranslation;
    String          m_aValue;
    String          m_aValueTranslation;
};

// ----------------------------------------------------------------------

/*
 * PPDKey - a container for the available options (=values) of a PPD keyword
 */

class PPDKey
{
    friend class PPDParser;

    typedef ::std::hash_map< ::rtl::OUString, PPDValue, ::rtl::OUStringHash > hash_type;
    typedef ::std::vector< PPDValue* > value_type;

    String          	m_aKey;
    hash_type			m_aValues;
    value_type			m_aOrderedValues;
    const PPDValue*		m_pDefaultValue;
    bool            	m_bQueryValue;
    PPDValue        	m_aQueryValue;

public:
    enum UIType { PickOne, PickMany, Boolean };
    enum SetupType { ExitServer, Prolog, DocumentSetup, PageSetup, JCLSetup, AnySetup };
private:

    bool				m_bUIOption;
    String				m_aUITranslation;
    UIType				m_eUIType;
    int					m_nOrderDependency;
    SetupType			m_eSetupType;

    void eraseValue( const String& rOption );
public:
    PPDKey( const String& rKey );
    ~PPDKey();

    PPDValue*           insertValue( const String& rOption );
    int                 countValues() const
    { return m_aValues.size(); }
    // neither getValue will return the query option
    const PPDValue*         getValue( int n ) const;
    const PPDValue*         getValue( const String& rOption ) const;
    const PPDValue*         getValueCaseInsensitive( const String& rOption ) const;
    const PPDValue*         getDefaultValue() const { return m_pDefaultValue; }
    const PPDValue*     getQueryValue() const { return m_bQueryValue ? &m_aQueryValue : NULL; }

    const String&       getKey() const { return m_aKey; }
    bool                isUIKey() const { return m_bUIOption; }
    const String&       getUITranslation() const { return m_aUITranslation; }
    UIType              getUIType() const { return m_eUIType; }
    SetupType           getSetupType() const { return m_eSetupType; }
    int                 getOrderDependency() const { return m_nOrderDependency; }
};

// define a hash for PPDKey
struct PPDKeyhash
{
    size_t operator()( const PPDKey * pKey) const
        { return (size_t)pKey; }
};

// ----------------------------------------------------------------------

/*
 * PPDParser - parses a PPD file and contains all available keys from it
 */

class PPDContext;
class CUPSManager;

class PPDParser
{
    friend class PPDContext;
    friend class CUPSManager;

    typedef ::std::hash_map< ::rtl::OUString, PPDKey*, ::rtl::OUStringHash > hash_type;
    typedef ::std::vector< PPDKey* > value_type;

    void insertKey( const String& rKey, PPDKey* pKey );
public:
    struct PPDConstraint
    {
        const PPDKey*       m_pKey1;
        const PPDValue*     m_pOption1;
        const PPDKey*       m_pKey2;
        const PPDValue*     m_pOption2;

        PPDConstraint() : m_pKey1( NULL ), m_pOption1( NULL ), m_pKey2( NULL ), m_pOption2( NULL ) {}
    };
private:

    static ::std::list< PPDParser* >           aAllParsers;
    static ::std::hash_map< rtl::OUString, rtl::OUString, rtl::OUStringHash >*
    											pAllPPDFiles;

    hash_type									m_aKeys;
    value_type									m_aOrderedKeys;
    ::std::list< PPDConstraint >				m_aConstraints;

    // some identifying fields
    String                          			m_aPrinterName;
    String                          			m_aNickName;
    // the full path of the PPD file
    String                          			m_aFile;
    // some basic attributes
    bool                            			m_bColorDevice;
    bool                            			m_bType42Capable;
    ULONG                           			m_nLanguageLevel;
    rtl_TextEncoding                            m_aFileEncoding;


    // shortcuts to important keys and their default values
    // imageable area
    const PPDValue*                     		m_pDefaultImageableArea;
    const PPDKey*                       		m_pImageableAreas;
    // paper dimensions
    const PPDValue*                     		m_pDefaultPaperDimension;
    const PPDKey*                       		m_pPaperDimensions;
    // paper trays
    const PPDValue*                     		m_pDefaultInputSlot;
    const PPDKey*                       		m_pInputSlots;
    // resolutions
    const PPDValue*                     		m_pDefaultResolution;
    const PPDKey*                       		m_pResolutions;
    // duplex commands
    const PPDValue*                     		m_pDefaultDuplexType;
    const PPDKey*                       		m_pDuplexTypes;

    // fonts
    const PPDKey*                       		m_pFontList;

    PPDParser( const String& rFile );
    ~PPDParser();

    void parseOrderDependency( const ByteString& rLine );
    void parseOpenUI( const ByteString& rLine );
    void parseConstraint( const ByteString& rLine );
    void parse( std::list< ByteString >& rLines );
    
    String handleTranslation( const ByteString& rString );

    static void scanPPDDir( const String& rDir );
    static void initPPDFiles();
    static String getPPDFile( const String& rFile );
public:
    static const PPDParser* getParser( const String& rFile );
    static String getPPDPrinterName( const String& rFile );
    static void freeAll();
    static void getKnownPPDDrivers( std::list< rtl::OUString >& o_rDrivers, bool bRefresh = false );

    const String&   getFilename() const { return m_aFile; }

    const PPDKey*   getKey( int n ) const;
    const PPDKey*   getKey( const String& rKey ) const;
    int             getKeys() const { return m_aKeys.size(); }
    bool            hasKey( const PPDKey* ) const;

    const ::std::list< PPDConstraint >& getConstraints() const { return m_aConstraints; }

    const String&   getPrinterName() const
    { return m_aPrinterName.Len() ? m_aPrinterName : m_aNickName; }
    const String&   getNickName() const
    { return m_aNickName.Len() ? m_aNickName : m_aPrinterName; }

    bool            isColorDevice() const { return m_bColorDevice; }
    bool            isType42Capable() const { return m_bType42Capable; }
    ULONG           getLanguageLevel() const { return m_nLanguageLevel; }

    const String&   getDefaultPaperDimension() const;
    void            getDefaultPaperDimension( int& rWidth, int& rHeight ) const
    { getPaperDimension( getDefaultPaperDimension(), rWidth, rHeight ); }
    bool getPaperDimension( const String& rPaperName,
                            int& rWidth, int& rHeight ) const;
    // width and height in pt
    // returns false if paper not found
    int             getPaperDimensions() const
    { return m_pPaperDimensions ? m_pPaperDimensions->countValues() : 0; }
    const String&   getPaperDimension( int ) const;
    const String&   getPaperDimensionCommand( int ) const;
    const String&   getPaperDimensionCommand( const String & ) const;

    // match the best paper for width and height
    const String&   matchPaper( int nWidth, int nHeight ) const;
    
    bool getMargins( const String& rPaperName,
                     int &rLeft, int& rRight,
                     int &rUpper, int& rLower ) const;
    // values in pt
    // returns true if paper found
    
    // values int pt
    
    const String&   getDefaultInputSlot() const;
    int             getInputSlots() const
    { return m_pInputSlots ? m_pInputSlots->countValues() : 0; }
    const String&   getSlot( int ) const;
    const String&   getSlotCommand( int ) const;
    const String&   getSlotCommand( const String& ) const;

    void            getDefaultResolution( int& rXRes, int& rYRes ) const;
    int             getResolutions() const;
    void            getResolution( int, int& rXRes, int& rYRes ) const;
    const String&   getResolutionCommand( int nXRes, int nYRes ) const;
    // values in dpi
    void            getResolutionFromString( const String&, int&, int& ) const;
    // helper function

    const String&   getDefaultDuplexType() const;
    int             getDuplexTypes() const
    { return m_pDuplexTypes ? m_pDuplexTypes->countValues() : 0; }
    const String&   getDuplex( int ) const;
    const String&   getDuplexCommand( int ) const;
    const String&   getDuplexCommand( const String& ) const;

    int             getFonts() const
    { return m_pFontList ? m_pFontList->countValues() : 0; }
    void            getFontAttributes( int,
                                       String& rEncoding,
                                       String& rCharset ) const;
    void            getFontAttributes( const String&,
                                       String& rEncoding,
                                       String& rCharset ) const;
    const String&   getFont( int ) const;
};

// ----------------------------------------------------------------------

/*
 * PPDContext - a class to manage user definable states based on the
 * contents of a PPDParser.
 */

class PPDContext
{
    typedef ::std::hash_map< const PPDKey*, const PPDValue*, PPDKeyhash > hash_type;
    hash_type m_aCurrentValues;
    const PPDParser*                                    m_pParser;

    // returns false: check failed, new value is constrained
    //         true:  check succeded, new value can be set
    bool checkConstraints( const PPDKey*, const PPDValue*, bool bDoReset );
    bool resetValue( const PPDKey*, bool bDefaultable = false );
public:
    PPDContext( const PPDParser* pParser = NULL );
    PPDContext( const PPDContext& rContext ) { operator=( rContext ); }
    PPDContext& operator=( const PPDContext& rContext );
    ~PPDContext();

    void setParser( const PPDParser* );
    const PPDParser* getParser() const { return m_pParser; }

    const PPDValue* getValue( const PPDKey* ) const;
    const PPDValue* setValue( const PPDKey*, const PPDValue*, bool bDontCareForConstraints = false );

    int countValuesModified() const { return m_aCurrentValues.size(); }
    const PPDKey* getModifiedKey( int n ) const;

    // public wrapper for the private method
    bool checkConstraints( const PPDKey*, const PPDValue* );

    void getUnconstrainedValues( const PPDKey*, ::std::list< const PPDValue* >& rValues );

    // for printer setup
    void*   getStreamableBuffer( ULONG& rBytes ) const;
    void    rebuildFromStreamBuffer( void* pBuffer, ULONG nBytes );

    // convenience
    int getRenderResolution() const;

    // width, height in points, paper will contain the name of the selected
    // paper after the call
    void getPageSize( String& rPaper, int& rWidth, int& rHeight ) const;
};

} // namespace

#endif // _PSPRINT_PPDPARSER_HXX_
