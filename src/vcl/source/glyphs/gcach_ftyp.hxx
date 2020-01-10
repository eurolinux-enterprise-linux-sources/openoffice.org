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

#ifndef _SV_GCACHFTYP_HXX
#define _SV_GCACHFTYP_HXX

#include <vcl/glyphcache.hxx>
#include <rtl/textcvt.h>

#include <ft2build.h>
#include FT_FREETYPE_H
class FreetypeServerFont;
struct FT_GlyphRec_;

// -----------------------------------------------------------------------

// FtFontFile has the responsibility that a font file is only mapped once.
// (#86621#) the old directly ft-managed solution caused it to be mapped
// in up to nTTC*nSizes*nOrientation*nSynthetic times
class FtFontFile
{
public:
    static FtFontFile*      FindFontFile( const ::rtl::OString& rNativeFileName );

    bool                    Map();
    void                    Unmap();

    const unsigned char*    GetBuffer() const { return mpFileMap; }
    int                     GetFileSize() const { return mnFileSize; }
    const ::rtl::OString*   GetFileName() const { return &maNativeFileName; }
    int                     GetLangBoost() const { return mnLangBoost; }

private:
                            FtFontFile( const ::rtl::OString& rNativeFileName );

    const ::rtl::OString    maNativeFileName;
    const unsigned char*    mpFileMap;
    int                     mnFileSize;
    int                     mnRefCount;
    int                     mnLangBoost;
};

// -----------------------------------------------------------------------

// FtFontInfo corresponds to an unscaled font face
class FtFontInfo
{
public:
                           FtFontInfo( const ImplDevFontAttributes&,
                               const ::rtl::OString& rNativeFileName,
                               int nFaceNum, sal_IntPtr nFontId, int nSynthetic,
                                const ExtraKernInfo* );
                          ~FtFontInfo();

    const unsigned char*  GetTable( const char*, ULONG* pLength=0 ) const;

    FT_FaceRec_*          GetFaceFT();
    void                  ReleaseFaceFT( FT_FaceRec_* );

    const ::rtl::OString* GetFontFileName() const   { return mpFontFile->GetFileName(); }
    int                   GetFaceNum() const        { return mnFaceNum; }
    int                   GetSynthetic() const      { return mnSynthetic; }
    sal_IntPtr            GetFontId() const         { return mnFontId; }
    bool                  DontUseAntiAlias() const  
        { return maDevFontAttributes.UseAntiAlias() == ANTIALIAS_FALSE; }
    bool                  DontUseEmbeddedBitmaps() const 
        { return maDevFontAttributes.UseEmbeddedBitmap() == EMBEDDEDBITMAP_FALSE; }
    bool                  IsSymbolFont() const      { return maDevFontAttributes.IsSymbolFont(); }
    const ImplFontAttributes& GetFontAttributes() const { return maDevFontAttributes; }

    void                  AnnounceFont( ImplDevFontList* );

    int                   GetGlyphIndex( sal_UCS4 cChar ) const;
    void                  CacheGlyphIndex( sal_UCS4 cChar, int nGI ) const;

    bool                  HasExtraKerning() const;
    int                   GetExtraKernPairs( ImplKernPairData** ) const;
    int                   GetExtraGlyphKernValue( int nLeftGlyph, int nRightGlyph ) const;

private:
    FT_FaceRec_*    maFaceFT;
    FtFontFile*     mpFontFile;
    const int       mnFaceNum;
    int             mnRefCount;
    const int       mnSynthetic;

    sal_IntPtr      mnFontId;
    ImplDevFontAttributes maDevFontAttributes;

    // cache unicode->glyphid mapping because looking it up is expensive
    // TODO: change to hash_multimap when a use case requires a m:n mapping
    typedef ::std::hash_map<int,int> Int2IntMap;
    mutable Int2IntMap* mpChar2Glyph;
    mutable Int2IntMap* mpGlyph2Char;
    void InitHashes() const;

    const ExtraKernInfo* mpExtraKernInfo;
};

// these two inlines are very important for performance

inline int FtFontInfo::GetGlyphIndex( sal_UCS4 cChar ) const
{
    if( !mpChar2Glyph )
        return -1;
    Int2IntMap::const_iterator it = mpChar2Glyph->find( cChar );
    if( it == mpChar2Glyph->end() )
        return -1;
    return it->second;
}

inline void FtFontInfo::CacheGlyphIndex( sal_UCS4 cChar, int nIndex ) const
{
    if( !mpChar2Glyph )
        InitHashes();
    (*mpChar2Glyph)[ cChar ] = nIndex;
    (*mpGlyph2Char)[ nIndex ] = cChar;
}

// -----------------------------------------------------------------------

class FreetypeManager
{
public:
                        FreetypeManager();
                        ~FreetypeManager();

    long                AddFontDir( const String& rUrlName );
    void                AddFontFile( const rtl::OString& rNormalizedName,
                            int nFaceNum, sal_IntPtr nFontId, const ImplDevFontAttributes&,
                            const ExtraKernInfo* );
    void                AnnounceFonts( ImplDevFontList* ) const;
    void                ClearFontList();

    FreetypeServerFont* CreateFont( const ImplFontSelectData& );

private:
    typedef ::std::hash_map<sal_IntPtr,FtFontInfo*> FontList;
    FontList            maFontList;

    sal_IntPtr          mnMaxFontId;
    sal_IntPtr          mnNextFontId;
};

// -----------------------------------------------------------------------

class FreetypeServerFont : public ServerFont
{
public:
                                FreetypeServerFont( const ImplFontSelectData&, FtFontInfo* );
    virtual                     ~FreetypeServerFont();

    virtual const ::rtl::OString* GetFontFileName() const { return mpFontInfo->GetFontFileName(); }
    virtual int                 GetFontFaceNum() const { return mpFontInfo->GetFaceNum(); }
    virtual bool                TestFont() const;
    virtual void*               GetFtFace() const;
    virtual int               	GetLoadFlags() const { return (mnLoadFlags & ~FT_LOAD_IGNORE_TRANSFORM); }
    virtual bool                NeedsArtificialBold() const { return mbArtBold; }
    virtual bool                NeedsArtificialItalic() const { return mbArtItalic; }

    virtual void                FetchFontMetric( ImplFontMetricData&, long& rFactor ) const;

    virtual int                 GetGlyphIndex( sal_UCS4 ) const;
    int                         GetRawGlyphIndex( sal_UCS4 ) const;
    int                         FixupGlyphIndex( int nGlyphIndex, sal_UCS4 ) const;

    virtual bool                GetAntialiasAdvice( void ) const;
    virtual bool                GetGlyphBitmap1( int nGlyphIndex, RawBitmap& ) const;
    virtual bool                GetGlyphBitmap8( int nGlyphIndex, RawBitmap& ) const;
    virtual bool                GetGlyphOutline( int nGlyphIndex, ::basegfx::B2DPolyPolygon& ) const;
    virtual int                 GetGlyphKernValue( int nLeftGlyph, int nRightGlyph ) const;
    virtual ULONG               GetKernPairs( ImplKernPairData** ) const;

    const unsigned char*        GetTable( const char* pName, ULONG* pLength )
                                { return mpFontInfo->GetTable( pName, pLength ); }
    int                         GetEmUnits() const;
    const FT_Size_Metrics&      GetMetricsFT() const { return maSizeFT->metrics; }

protected:
    friend class GlyphCache;

    int                         ApplyGlyphTransform( int nGlyphFlags, FT_GlyphRec_*, bool ) const;
    virtual void                InitGlyphData( int nGlyphIndex, GlyphData& ) const;
    virtual bool                GetFontCodeRanges( CmapResult& ) const;
    bool                        ApplyGSUB( const ImplFontSelectData& );
    virtual ServerFontLayoutEngine* GetLayoutEngine();

private:
    int                         mnWidth;
    int                         mnPrioEmbedded;
    int                         mnPrioAntiAlias;
    FtFontInfo*                 mpFontInfo;
    FT_Int                      mnLoadFlags;
    double                      mfStretch;
    FT_FaceRec_*                maFaceFT;
    FT_SizeRec_*                maSizeFT;

    bool                        mbFaceOk;
    bool			mbArtItalic;
    bool			mbArtBold;
    bool			mbUseGamma;

    typedef ::std::hash_map<int,int> GlyphSubstitution;
    GlyphSubstitution           maGlyphSubstitution;
    rtl_UnicodeToTextConverter  maRecodeConverter;

    ServerFontLayoutEngine*     mpLayoutEngine;
};

// -----------------------------------------------------------------------

class ImplFTSFontData : public ImplFontData
{
private:
    FtFontInfo*             mpFtFontInfo;
    enum { IFTSFONT_MAGIC = 0x1F150A1C };

public:
                            ImplFTSFontData( FtFontInfo*, const ImplDevFontAttributes& );

    FtFontInfo*             GetFtFontInfo() const { return mpFtFontInfo; }

    virtual ImplFontEntry*  CreateFontInstance( ImplFontSelectData& ) const;
    virtual ImplFontData*   Clone() const   { return new ImplFTSFontData( *this ); }
    virtual sal_IntPtr      GetFontId() const { return mpFtFontInfo->GetFontId(); }

    static bool             CheckFontData( const ImplFontData& r ) { return r.CheckMagic( IFTSFONT_MAGIC ); }
};

// -----------------------------------------------------------------------

#endif // _SV_GCACHFTYP_HXX
