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

#ifndef SC_XIESCHER_HXX
#define SC_XIESCHER_HXX

#include <vector>
#include <map>
#include <svx/msdffimp.hxx>
#include <svx/msocximex.hxx>
#include <vcl/graph.hxx>
#include "xlescher.hxx"
#include "xiroot.hxx"
#include "xistring.hxx"

namespace com { namespace sun { namespace star {
    namespace drawing { class XShape; }
    namespace form { class XForm; }
} } }

class ScfProgressBar;
class ScfPropertySet;
class XclImpChart;

// Drawing objects ============================================================

class XclImpDrawObjBase;
typedef ScfRef< XclImpDrawObjBase > XclImpDrawObjRef;

/** Base class for drawing objects (OBJ records). */
class XclImpDrawObjBase : protected XclImpRoot
{
public:
    explicit            XclImpDrawObjBase( const XclImpRoot& rRoot );
    virtual             ~XclImpDrawObjBase();

    /** Reads the BIFF3 OBJ record, returns a new drawing object. */
    static XclImpDrawObjRef ReadObj3( XclImpStream& rStrm );
    /** Reads the BIFF4 OBJ record, returns a new drawing object. */
    static XclImpDrawObjRef ReadObj4( XclImpStream& rStrm );
    /** Reads the BIFF5 OBJ record, returns a new drawing object. */
    static XclImpDrawObjRef ReadObj5( XclImpStream& rStrm );
    /** Reads the BIFF8 OBJ record, returns a new drawing object. */
    static XclImpDrawObjRef ReadObj8( XclImpStream& rStrm );

    /** Sets whether this is an area object (then its width and height must be greater than 0). */
    inline void         SetAreaObj( bool bAreaObj ) { mbAreaObj = bAreaObj; }
    /** If set to true, a new SdrObject will be created while in DFF import. */
    inline void         SetSimpleMacro( bool bMacro ) { mbSimpleMacro = bMacro; }

    /** Sets shape data from DFF stream. */
    void                SetDffData( const DffObjData& rDffObjData, const String& rObjName, const String& rHyperlink, bool bVisible, bool bAutoMargin );
    /** Sets the object anchor explicitly. */
    void                SetAnchor( const XclObjAnchor& rAnchor );

    /** If set to false, the SdrObject will not be created, processed, or inserted into the draw page. */
    inline void         SetProcessSdrObj( bool bProcess ) { mbProcessSdr = bProcess; }
    /** If set to false, the SdrObject will be created or processed, but not be inserted into the draw page. */
    inline void         SetInsertSdrObj( bool bInsert ) { mbInsertSdr = bInsert; }
    /** If set to true, a new SdrObject will be created while in DFF import. */
    inline void         SetCustomDffObj( bool bCustom ) { mbCustomDff = bCustom; }

    /** Returns the Calc sheet index of this object. */
    inline SCTAB        GetScTab() const { return maObjId.mnScTab; }
    /** Returns the sheet index and Excel object identifier from OBJ record. */
    inline const XclObjId& GetObjId() const { return maObjId; }
    /** Returns the Excel object type from OBJ record. */
    inline sal_uInt16   GetObjType() const { return mnObjType; }
    /** Returns the name of this object, may generate a default name. */
    String              GetObjName() const;
    /** Returns associated macro name, if set, otherwise zero length string. */
    inline const String& GetMacroName() const { return maMacroName; }

    /** Returns the shape identifier used in the DFF stream. */
    inline sal_uInt32   GetDffShapeId() const { return mnDffShapeId; }
    /** Returns the shape flags from the DFF stream. */
    inline sal_uInt32   GetDffFlags() const { return mnDffFlags; }

    /** Returns true, if the object is hidden. */
    inline bool         IsHidden() const { return mbHidden; }
    /** Returns true, if the object is visible. */
    inline bool         IsVisible() const { return mbVisible; }
    /** Returns true, if the object is printable. */
    inline bool         IsPrintable() const { return mbPrintable; }

    /** Returns true, if the passed size is valid for this object. */
    bool                IsValidSize( const Rectangle& rAnchorRect ) const;
    /** Returns the area in the sheet used by this object. */
    ScRange             GetUsedArea() const;
    /** Returns the area on the drawing layer for this object. */
    Rectangle           GetAnchorRect() const;

    /** Returns true, if the object is valid and will be processed.. */
    inline bool         IsProcessSdrObj() const { return mbProcessSdr && !mbHidden; }
    /** Returns true, if the SdrObject will be created or processed, but not be inserted into the draw page. */
    inline bool         IsInsertSdrObj() const { return mbInsertSdr; }

    /** Returns the needed size on the progress bar (calls virtual DoGetProgressSize() function). */
    sal_Size            GetProgressSize() const;
    /** Creates and returns an SdrObject from the contained data. Caller takes ownership! */
    SdrObject*          CreateSdrObject( const Rectangle& rAnchorRect, ScfProgressBar& rProgress, bool bDffImport ) const;
    /** Additional processing for the passed SdrObject (calls virtual DoProcessSdrObj() function). */
    void                ProcessSdrObject( SdrObject& rSdrObj ) const;

protected:
    /** Reads the object name in a BIFF5 OBJ record. */
    void                ReadName5( XclImpStream& rStrm, sal_uInt16 nNameLen );
    /** Reads the macro link in a BIFF3 OBJ record. */
    void                ReadMacro3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the macro link in a BIFF4 OBJ record. */
    void                ReadMacro4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the macro link in a BIFF5 OBJ record. */
    void                ReadMacro5( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the ftMacro sub structure in an OBJ record. */
    void                ReadMacro8( XclImpStream& rStrm );

    /** Converts the passed line formatting to the passed SdrObject. */
    void                ConvertLineStyle( SdrObject& rSdrObj, const XclObjLineData& rLineData ) const;
    /** Converts the passed fill formatting to the passed SdrObject. */
    void                ConvertFillStyle( SdrObject& rSdrObj, const XclObjFillData& rFillData ) const;
    /** Converts the passed frame flags to the passed SdrObject. */
    void                ConvertFrameStyle( SdrObject& rSdrObj, sal_uInt16 nFrameFlags ) const;

    /** Returns a solid line color from the passed line data struct. */
    Color               GetSolidLineColor( const XclObjLineData& rLineData ) const;
    /** Returns a solid fill color from the passed fill data struct. */
    Color               GetSolidFillColor( const XclObjFillData& rFillData ) const;

    /** Derived classes read the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Derived classes read the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Derived classes read the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Derived classes read the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );

    /** Derived classes may return a progress bar size different from 1. */
    virtual sal_Size    DoGetProgressSize() const;
    /** Derived classes create and return a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;
    /** Derived classes may perform additional processing for the passed SdrObject. */
    virtual void        DoProcessSdrObj( SdrObject& rSdrObj ) const;

private:
    /** Reads the contents of a BIFF3 OBJ record. */
    void                ImplReadObj3( XclImpStream& rStrm );
    /** Reads the contents of a BIFF4 OBJ record. */
    void                ImplReadObj4( XclImpStream& rStrm );
    /** Reads the contents of a BIFF5 OBJ record. */
    void                ImplReadObj5( XclImpStream& rStrm );
    /** Reads the contents of a BIFF8 OBJ record. */
    void                ImplReadObj8( XclImpStream& rStrm );

private:
    typedef ScfRef< XclObjAnchor > XclObjAnchorRef;

    XclObjAnchorRef     mxAnchor;       /// The position of the object in the containing sheet.
    XclObjId            maObjId;        /// Sheet index and object identifier.
    sal_uInt16          mnObjType;      /// The Excel object type from OBJ record.
    sal_uInt32          mnDffShapeId;   /// Shape ID from DFF stream.
    sal_uInt32          mnDffFlags;     /// Shape flags from DFF stream.
    String              maObjName;      /// Name of the object.
    String              maMacroName;    /// Name of an attached macro.
    String              maHyperlink;    /// On-click hyperlink URL.
    bool                mbHidden;       /// true = Object is hidden.
    bool                mbVisible;      /// true = Object is visible.
    bool                mbPrintable;    /// true = Object is printable.
    bool                mbAreaObj;      /// true = Width and height must be greater than 0.
    bool                mbAutoMargin;   /// true = Set automatic text margin.
    bool                mbSimpleMacro;  /// true = Create simple macro link and hyperlink.
    bool                mbProcessSdr;   /// true = Object is valid, do processing and insertion.
    bool                mbInsertSdr;    /// true = Insert the SdrObject into draw page.
    bool                mbCustomDff;    /// true = Recreate SdrObject in DFF import.
};

// ----------------------------------------------------------------------------

class XclImpDrawObjVector : public ::std::vector< XclImpDrawObjRef >
{
public:
    inline explicit     XclImpDrawObjVector() {}

    /** Tries to insert the passed object into the last group or appends it. */
    void                InsertGrouped( XclImpDrawObjRef xDrawObj );

    /** Returns the needed size on the progress bar for all contained objects. */
    sal_Size            GetProgressSize() const;
};

// ----------------------------------------------------------------------------

/** A placeholder object for unknown object types. */
class XclImpPhObj : public XclImpDrawObjBase
{
public:
    explicit            XclImpPhObj( const XclImpRoot& rRoot );
};

// ----------------------------------------------------------------------------

/** A group object. */
class XclImpGroupObj : public XclImpDrawObjBase
{
public:
    explicit            XclImpGroupObj( const XclImpRoot& rRoot );

    /** Tries to insert the drawing object into this or a nested group. */
    bool                TryInsert( XclImpDrawObjRef xDrawObj );

protected:
    /** Reads the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Returns a progress bar size that takes all group children into account. */
    virtual sal_Size    DoGetProgressSize() const;
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;

protected:
    XclImpDrawObjVector maChildren;         /// Grouped objects.
    sal_uInt16          mnFirstUngrouped;   /// Object identfier of first object not grouped into this group.
};

// ----------------------------------------------------------------------------

/** A line object. */
class XclImpLineObj : public XclImpDrawObjBase
{
public:
    explicit            XclImpLineObj( const XclImpRoot& rRoot );

protected:
    /** Reads the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;

protected:
    XclObjLineData      maLineData;     /// BIFF5 line formatting.
    sal_uInt16          mnArrows;       /// Line arrows.
    sal_uInt8           mnStartPoint;   /// Starting point.
};

// ----------------------------------------------------------------------------

/** A rectangle or oval object. */
class XclImpRectObj : public XclImpDrawObjBase
{
public:
    explicit            XclImpRectObj( const XclImpRoot& rRoot );

protected:
    /** Reads fil data, line data, and frame flags. */
    void                ReadFrameData( XclImpStream& rStrm );

    /** Converts fill formatting, line formattind, and frame style. */
    void                ConvertRectStyle( SdrObject& rSdrObj ) const;

    /** Reads the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;

protected:
    XclObjFillData      maFillData;     /// BIFF5 fill formatting.
    XclObjLineData      maLineData;     /// BIFF5 line formatting.
    sal_uInt16          mnFrameFlags;   /// Additional flags.
};

// ----------------------------------------------------------------------------

/** An oval object. */
class XclImpOvalObj : public XclImpRectObj
{
public:
    explicit            XclImpOvalObj( const XclImpRoot& rRoot );

protected:
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;
};

// ----------------------------------------------------------------------------

/** An arc object. */
class XclImpArcObj : public XclImpDrawObjBase
{
public:
    explicit            XclImpArcObj( const XclImpRoot& rRoot );

protected:
    /** Reads the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;

protected:
    XclObjFillData      maFillData;     /// BIFF5 fill formatting.
    XclObjLineData      maLineData;     /// BIFF5 line formatting.
    sal_uInt8           mnQuadrant;     /// Visible quadrant of the circle.
};

// ----------------------------------------------------------------------------

/** A polygon object. */
class XclImpPolygonObj : public XclImpRectObj
{
public:
    explicit            XclImpPolygonObj( const XclImpRoot& rRoot );

protected:
    /** Reads the COORDLIST record following the OBJ record. */
    void                ReadCoordList( XclImpStream& rStrm );

    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;

protected:
    typedef ::std::vector< Point > PointVector;
    PointVector         maCoords;       /// Coordinates relative to bounding rectangle.
    sal_uInt16          mnPolyFlags;    /// Additional flags.
    sal_uInt16          mnPointCount;   /// Polygon point count.
};

// ----------------------------------------------------------------------------

struct XclImpObjTextData
{
    XclObjTextData      maData;         /// BIFF5 text data.
    XclImpStringRef     mxString;       /// Plain or rich string.

    /** Reads a byte string from the passed stream. */
    void                ReadByteString( XclImpStream& rStrm );
    /** Reads text formatting from the passed stream. */
    void                ReadFormats( XclImpStream& rStrm );
};

// ----------------------------------------------------------------------------

/** A drawing object supporting text contents. Used for all simple objects in BIFF8. */
class XclImpTextObj : public XclImpRectObj
{
public:
    explicit            XclImpTextObj( const XclImpRoot& rRoot );

    /** Stores the passed textbox data. */
    inline void         SetTextData( const XclImpObjTextData& rTextData ) { maTextData = rTextData; }

protected:
    /** Reads the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;
    /** Inserts the contained text data at the passed object. */
    virtual void        DoProcessSdrObj( SdrObject& rSdrObj ) const;

protected:
    XclImpObjTextData   maTextData;     /// Textbox data from BIFF5 OBJ or BIFF8 TXO record.
};

// ----------------------------------------------------------------------------

/** A chart object. This is the drawing object wrapper for the chart data. */
class XclImpChartObj : public XclImpRectObj
{
public:
    /** @param bOwnTab  True = chart is on an own sheet; false = chart is an embedded object. */
    explicit            XclImpChartObj( const XclImpRoot& rRoot, bool bOwnTab = false );

    /** Reads the complete chart substream (BOF/EOF block). */
    void                ReadChartSubStream( XclImpStream& rStrm );

protected:
    /** Reads the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Returns the needed size on the progress bar. */
    virtual sal_Size    DoGetProgressSize() const;
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;

private:
    /** Calculates the object anchor of a sheet chart (chart fills one page). */
    void                FinalizeTabChart();

private:
    typedef ScfRef< XclImpChart > XclImpChartRef;

    XclImpChartRef      mxChart;        /// The chart itself (BOF/EOF substream data).
    bool                mbOwnTab;       /// true = own sheet; false = embedded object.
};

// ----------------------------------------------------------------------------

/** A note object, which is a specialized text box objext. */
class XclImpNoteObj : public XclImpTextObj
{
public:
    explicit            XclImpNoteObj( const XclImpRoot& rRoot );

    /** Sets note flags and the note position in the Calc sheet. */
    void                SetNoteData( const ScAddress& rScPos, sal_uInt16 nNoteFlags );

protected:
    /** Inserts the note into the document, sets visibility. */
    virtual void        DoProcessSdrObj( SdrObject& rSdrObj ) const;

private:
    ScAddress           maScPos;        /// Cell position of the note object.
    sal_uInt16          mnNoteFlags;    /// Flags from NOTE record.
};

// ----------------------------------------------------------------------------

/** Helper base class for TBX and OCX form controls to manage spreadsheet links. */
class XclImpControlHelper
{
public:
    explicit            XclImpControlHelper( const XclImpRoot& rRoot, XclCtrlBindMode eBindMode );
    virtual             ~XclImpControlHelper();

    /** Returns true, if a linked cell address is present. */
    inline bool         HasCellLink() const { return mxCellLink.is(); }
    /** Returns true, if a linked source cell range is present. */
    inline bool         HasSourceRange() const { return mxSrcRange.is(); }

    /** Returns the SdrObject from the passed control shape and sets the bounding rectangle. */
    SdrObject*          CreateSdrObjectFromShape(
                            const ::com::sun::star::uno::Reference< ::com::sun::star::drawing::XShape >& rxShape,
                            const Rectangle& rAnchorRect ) const;

    /** Sets additional properties to the form control model, calls virtual DoProcessControl(). */
    void                ProcessControl( const XclImpDrawObjBase& rDrawObj ) const;

protected:
    /** Reads the formula for the linked cell from the current position of the stream. */
    void                ReadCellLinkFormula( XclImpStream& rStrm, bool bWithBoundSize );
    /** Reads the formula for the source range from the current position of the stream. */
    void                ReadSourceRangeFormula( XclImpStream& rStrm, bool bWithBoundSize );

    /** Derived classes will set additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;

private:
    /** Reads a list of cell ranges from a formula at the current stream position. */
    void                ReadRangeList( ScRangeList& rScRanges, XclImpStream& rStrm );
    /** Reads leading formula size and a list of cell ranges from a formula if the leading size is not zero. */
    void                ReadRangeList( ScRangeList& rScRanges, XclImpStream& rStrm, bool bWithBoundSize );

private:
    const XclImpRoot&   mrRoot;         /// Not derived from XclImpRoot to allow multiple inheritance.
    mutable ::com::sun::star::uno::Reference< ::com::sun::star::drawing::XShape >
                        mxShape;        /// The UNO wrapper of the control shape.
    ScfRef< ScAddress > mxCellLink;     /// Linked cell in the Calc document.
    ScfRef< ScRange >   mxSrcRange;     /// Source data range in the Calc document.
    XclCtrlBindMode     meBindMode;     /// Value binding mode.
};

// ----------------------------------------------------------------------------

/** Base class for textbox based form controls. */
class XclImpTbxObjBase : public XclImpTextObj, public XclImpControlHelper
{
public:
    explicit            XclImpTbxObjBase( const XclImpRoot& rRoot );

    /** Sets line and fill formatting from the passed DFF property set. */
    void                SetDffProperties( const DffPropSet& rDffPropSet );

    /** Returns the service name of the control component to be created. */
    inline ::rtl::OUString GetServiceName() const { return DoGetServiceName(); }
    /** Fills the passed macro event descriptor. */
    bool                FillMacroDescriptor(
                            ::com::sun::star::script::ScriptEventDescriptor& rDescriptor ) const;

protected:
    /** Sets control text formatting. */
    void                ConvertFont( ScfPropertySet& rPropSet ) const;
    /** Sets control label and text formatting. */
    void                ConvertLabel( ScfPropertySet& rPropSet ) const;

    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;
    /** Additional processing on the SdrObject, calls new virtual function DoProcessControl(). */
    virtual void        DoProcessSdrObj( SdrObject& rSdrObj ) const;

    /** Derived classes return the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const = 0;
    /** Derived classes return the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const = 0;
};

// ----------------------------------------------------------------------------

/** A button control. */
class XclImpButtonObj : public XclImpTbxObjBase
{
public:
    explicit            XclImpButtonObj( const XclImpRoot& rRoot );

protected:
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;
};

// ----------------------------------------------------------------------------

/** A checkbox control. */
class XclImpCheckBoxObj : public XclImpTbxObjBase
{
public:
    explicit            XclImpCheckBoxObj( const XclImpRoot& rRoot );

protected:
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;

protected:
    sal_uInt16          mnState;
    sal_uInt16          mnCheckBoxFlags;
};

// ----------------------------------------------------------------------------

/** An option button control. */
class XclImpOptionButtonObj : public XclImpCheckBoxObj
{
public:
    explicit            XclImpOptionButtonObj( const XclImpRoot& rRoot );

protected:
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;

protected:
    sal_uInt16          mnNextInGroup;      /// Next option button in a group.
    sal_uInt16          mnFirstInGroup;     /// 1 = Button is the first in a group.
};

// ----------------------------------------------------------------------------

/** A label control. */
class XclImpLabelObj : public XclImpTbxObjBase
{
public:
    explicit            XclImpLabelObj( const XclImpRoot& rRoot );

protected:
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;
};

// ----------------------------------------------------------------------------

/** A groupbox control. */
class XclImpGroupBoxObj : public XclImpTbxObjBase
{
public:
    explicit            XclImpGroupBoxObj( const XclImpRoot& rRoot );

protected:
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;

protected:
    sal_uInt16          mnGroupBoxFlags;
};

// ----------------------------------------------------------------------------

/** A dialog control. */
class XclImpDialogObj : public XclImpTbxObjBase
{
public:
    explicit            XclImpDialogObj( const XclImpRoot& rRoot );

protected:
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;
};

// ----------------------------------------------------------------------------

/** An edit control. */
class XclImpEditObj : public XclImpTbxObjBase
{
public:
    explicit            XclImpEditObj( const XclImpRoot& rRoot );

protected:
    /** REturns true, if the field type is numeric. */
    bool                IsNumeric() const;

    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;

protected:
    sal_uInt16          mnContentType;
    sal_uInt16          mnMultiLine;
    sal_uInt16          mnScrollBar;
    sal_uInt16          mnListBoxObjId;
};

// ----------------------------------------------------------------------------

/** Base class of scrollable form controls (spin button, scrollbar, listbox, dropdown). */
class XclImpTbxObjScrollableBase : public XclImpTbxObjBase
{
public:
    explicit            XclImpTbxObjScrollableBase( const XclImpRoot& rRoot );

protected:
    /** Reads scrollbar data. */
    void                ReadSbs( XclImpStream& rStrm );

    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );

protected:
    sal_uInt16          mnValue;
    sal_uInt16          mnMin;
    sal_uInt16          mnMax;
    sal_uInt16          mnStep;
    sal_uInt16          mnPageStep;
    sal_uInt16          mnOrient;
    sal_uInt16          mnThumbWidth;
    sal_uInt16          mnScrollFlags;
};

// ----------------------------------------------------------------------------

/** A spinbutton control. */
class XclImpSpinButtonObj : public XclImpTbxObjScrollableBase
{
public:
    explicit            XclImpSpinButtonObj( const XclImpRoot& rRoot );

protected:
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;
};

// ----------------------------------------------------------------------------

/** A scrollbar control. */
class XclImpScrollBarObj : public XclImpTbxObjScrollableBase
{
public:
    explicit            XclImpScrollBarObj( const XclImpRoot& rRoot );

protected:
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;
};

// ----------------------------------------------------------------------------

/** Base class for list controls (listbox, dropdown). */
class XclImpTbxObjListBase : public XclImpTbxObjScrollableBase
{
public:
    explicit            XclImpTbxObjListBase( const XclImpRoot& rRoot );

protected:
    /** Reads common listbox settings. */
    void                ReadLbsData( XclImpStream& rStrm );
    /** Sets common listbox/dropdown formatting attributes. */
    void                SetBoxFormatting( ScfPropertySet& rPropSet ) const;

protected:
    sal_uInt16          mnEntryCount;
    sal_uInt16          mnSelEntry;
    sal_uInt16          mnListFlags;
    sal_uInt16          mnEditObjId;
    bool                mbHasDefFontIdx;
};

// ----------------------------------------------------------------------------

/** A listbox control. */
class XclImpListBoxObj : public XclImpTbxObjListBase
{
public:
    explicit            XclImpListBoxObj( const XclImpRoot& rRoot );

protected:
    /** Reads listbox settings and selection. */
    void                ReadFullLbsData( XclImpStream& rStrm, sal_Size nRecLeft );

    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;

protected:
    ScfUInt8Vec         maSelection;
};

// ----------------------------------------------------------------------------

/** A dropdown listbox control. */
class XclImpDropDownObj : public XclImpTbxObjListBase
{
public:
    explicit            XclImpDropDownObj( const XclImpRoot& rRoot );

protected:
    /** Returns the type of the dropdown control. */
    sal_uInt16          GetDropDownType() const;

    /** Reads dropdown box settings. */
    void                ReadFullLbsData( XclImpStream& rStrm );

    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Sets additional properties for the current form control. */
    virtual void        DoProcessControl( ScfPropertySet& rPropSet ) const;
    /** Returns the service name of the control component to be created. */
    virtual ::rtl::OUString DoGetServiceName() const;
    /** Returns the type of the macro event to be created. */
    virtual XclTbxEventType DoGetEventType() const;

protected:
    sal_uInt16          mnLeft;
    sal_uInt16          mnTop;
    sal_uInt16          mnRight;
    sal_uInt16          mnBottom;
    sal_uInt16          mnDropDownFlags;
    sal_uInt16          mnLineCount;
    sal_uInt16          mnMinWidth;
};

// ----------------------------------------------------------------------------

/** A picture, an embedded or linked OLE object, or an OCX form control. */
class XclImpPictureObj : public XclImpRectObj, public XclImpControlHelper
{
public:
    explicit            XclImpPictureObj( const XclImpRoot& rRoot );

    /** Returns the graphic imported from the IMGDATA record. */
    inline const Graphic& GetGraphic() const { return maGraphic; }
    /** Returns the visible area of the imported graphic. */
    inline const Rectangle& GetVisArea() const { return maVisArea; }

    /** Returns true, if the OLE object will be shown as symbol. */
    inline bool         IsSymbol() const { return mbSymbol; }
    /** Returns the storage name for the OLE object. */
    String              GetOleStorageName() const;

    /** Returns true, if this object is an OCX form control. */
    inline bool         IsOcxControl() const { return mbEmbedded && mbControl && mbUseCtlsStrm; }
    /** Returns the position in the 'Ctls' stream for additional form control data. */
    inline sal_Size     GetCtlsStreamPos() const { return mnCtlsStrmPos; }
    /** Returns the size in the 'Ctls' stream for additional form control data. */
    inline sal_Size     GetCtlsStreamSize() const { return mnCtlsStrmSize; }

protected:
    /** Reads the contents of the a BIFF3 OBJ record from the passed stream. */
    virtual void        DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF4 OBJ record from the passed stream. */
    virtual void        DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize );
    /** Reads the contents of the a BIFF5 OBJ record from the passed stream. */
    virtual void        DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize );
    /** Reads the contents of the specified subrecord of a BIFF8 OBJ record from stream. */
    virtual void        DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize );
    /** Creates and returns a new SdrObject from the contained data. Caller takes ownership! */
    virtual SdrObject*  DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const;
    /** Overloaded to do additional processing on the SdrObject. */
    virtual void        DoProcessSdrObj( SdrObject& rSdrObj ) const;

private:
    /** Reads and sets the picture flags from a BIFF3-BIFF5 OBJ picture record. */
    void                ReadFlags3( XclImpStream& rStrm );
    /** Reads the contents of the OBJFLAGS subrecord. */
    void                ReadFlags8( XclImpStream& rStrm );
    /** Reads the contents of the OBJPICTFMLA subrecord. */
    void                ReadPictFmla( XclImpStream& rStrm, sal_uInt16 nLinkSize );

private:
    Graphic             maGraphic;      /// Picture or OLE placeholder graphic.
    Rectangle           maVisArea;      /// Size of graphic.
    String              maClassName;    /// Class name of embedded OLE object.
    sal_uInt32          mnStorageId;    /// Identifier of the storage for this object.
    sal_Size            mnCtlsStrmPos;  /// Position in 'Ctls' stream for this control.
    sal_Size            mnCtlsStrmSize; /// Size in 'Ctls' stream for this control.
    bool                mbEmbedded;     /// true = Embedded OLE object.
    bool                mbLinked;       /// true = Linked OLE object.
    bool                mbSymbol;       /// true = Show as symbol.
    bool                mbControl;      /// true = Form control, false = OLE object.
    bool                mbUseCtlsStrm;  /// true = Form control data in 'Ctls' stream, false = Own storage.
};

// DFF stream conversion ======================================================

/** The solver container collects all connector rules for connected objects. */
class XclImpSolverContainer : public SvxMSDffSolverContainer
{
public:
//UNUSED2009-05 /** Reads the entire solver container. Stream must point to begin of container header. */
//UNUSED2009-05 void                ReadSolverContainer( SvStream& rDffStrm );

    /** Inserts information about a new SdrObject. */
    void                InsertSdrObjectInfo( SdrObject& rSdrObj, sal_uInt32 nDffShapeId, sal_uInt32 nDffFlags );
    /** Removes inforamtion of an SdrObject (and all child objects if it is a group). */
    void                RemoveSdrObjectInfo( SdrObject& rSdrObj );

    /** Inserts the SdrObject pointers into all connector rules. */
    void                UpdateConnectorRules();
    /** Removes all contained connector rules. */
    void                RemoveConnectorRules();

private:
    /** Returns the first connector rule from the internal list. */
    SvxMSDffConnectorRule* GetFirstRule();
    /** Returns the next connector rule from the internal list. */
    SvxMSDffConnectorRule* GetNextRule();
    /** Updates the data of a connected shape in a connector rule. */
    void                UpdateConnection( sal_uInt32 nDffShapeId, SdrObject*& rpSdrObj, sal_uInt32* pnDffFlags = 0 );

private:
    /** Stores data about an SdrObject processed during import. */
    struct XclImpSdrInfo
    {
        SdrObject*          mpSdrObj;       /// Pointer to an SdrObject.
        sal_uInt32          mnDffFlags;     /// Shape flags from DFF stream.
        inline explicit     XclImpSdrInfo() : mpSdrObj( 0 ), mnDffFlags( 0 ) {}
        inline void         Set( SdrObject* pSdrObj, sal_uInt32 nDffFlags )
                                { mpSdrObj = pSdrObj; mnDffFlags = nDffFlags; }
    };
    typedef ::std::map< sal_uInt32, XclImpSdrInfo > XclImpSdrInfoMap;
    typedef ::std::map< SdrObject*, sal_uInt32 >    XclImpSdrObjMap;

    XclImpSdrInfoMap    maSdrInfoMap;   /// Maps shape IDs to SdrObjects and flags.
    XclImpSdrObjMap     maSdrObjMap;    /// Maps SdrObjects to shape IDs.
};

// ----------------------------------------------------------------------------

/** Simple implementation of the SVX DFF manager. Implements resolving palette
    colors. Used by XclImpDffPropSet (as is), extended by XclImpDffManager.
 */
class XclImpSimpleDffManager : public SvxMSDffManager, protected XclImpRoot
{
public:
    explicit            XclImpSimpleDffManager( const XclImpRoot& rRoot, SvStream& rDffStrm );
    virtual             ~XclImpSimpleDffManager();

protected:
    /** Returns a color from the Excel color palette. */
    virtual FASTBOOL    GetColorFromPalette( USHORT nIndex, Color& rColor ) const;
};

// ----------------------------------------------------------------------------

class XclImpObjectManager;
class SdrObjList;

/** Derived from SvxMSDffManager and SvxMSConvertOCXControls, contains core
    implementation of DFF stream import and OCX form control import.
 */
class XclImpDffManager : protected XclImpSimpleDffManager, protected SvxMSConvertOCXControls
{
public:
    explicit            XclImpDffManager(
                            const XclImpRoot& rRoot,
                            XclImpObjectManager& rObjManager,
                            SvStream& rDffStrm );
    virtual             ~XclImpDffManager();

    /** Initializes the internal progress bar with the passed size and starts it. */
    void                StartProgressBar( sal_Size nProgressSize );

    /** Processes BIFF5 drawing objects without DFF data, inserts into the passed object list. */
    void                ProcessObject( SdrObjList* pObjList, const XclImpDrawObjBase& rDrawObj );
    /** Processes the leading drawing group container in the DFF stream. */
    void                ProcessDrawingGroup( SvStream& rDffStrm );
    /** Processes a drawing container for a sheet in the DFF stream, converts all objects. */
    void                ProcessDrawing( SvStream& rDffStrm, sal_Size nStrmPos );

    /** Creates the SdrObject for the passed Excel TBX form control object. */
    SdrObject*          CreateSdrObject( const XclImpTbxObjBase& rTbxObj, const Rectangle& rAnchorRect );
    /** Creates the SdrObject for the passed Excel OLE object or OCX form control object. */
    SdrObject*          CreateSdrObject( const XclImpPictureObj& rPicObj, const Rectangle& rAnchorRect );

    /** Returns the default text margin in drawing layer units. */
    inline sal_Int32    GetDefaultTextMargin() const { return mnDefTextMargin; }
    /** Returns the used area in the sheet with the passed index. */
    ScRange             GetUsedArea( SCTAB nScTab ) const;

protected:
    // virtual functions of SvxMSDffManager

    /** Reads the client anchor from the DFF stream and sets it at the correct object. */
    virtual void        ProcessClientAnchor2(
                            SvStream& rDffStrm,
                            DffRecordHeader& rHeader,
                            void* pClientData,
                            DffObjData& rObjData );
    /** Processes an DFF object, reads properties from DFF stream. */
    virtual SdrObject*  ProcessObj(
                            SvStream& rDffStrm,
                            DffObjData& rDffObjData,
                            void* pClientData,
                            Rectangle& rTextRect,
                            SdrObject* pOldSdrObj = 0 );
    /** Returns the BLIP stream position, based on the passed DFF stream position. */
    virtual ULONG       Calc_nBLIPPos( ULONG nOrgVal, ULONG nStreamPos ) const;

    // virtual functions of SvxMSConvertOCXControls

    /** Inserts the passed control rxFComp into the form. Needs call to SetCurrentForm() before. */
    virtual sal_Bool    InsertControl(
                            const ::com::sun::star::uno::Reference<
                                ::com::sun::star::form::XFormComponent >& rxFormComp,
                            const ::com::sun::star::awt::Size& rSize,
                            ::com::sun::star::uno::Reference<
                                ::com::sun::star::drawing::XShape >* pxShape,
                            BOOL bFloatingCtrl );

private:
    /** Reads contents of a hyperlink property and returns the extracted URL. */
    String              ReadHlinkProperty( SvStream& rDffStrm ) const;

    /** Processes a drawing group container (global drawing data). */
    void                ProcessDggContainer( SvStream& rDffStrm, const DffRecordHeader& rDggHeader );
    /** Processes a drawing container (all drawing data of a sheet). */
    void                ProcessDgContainer( SvStream& rDffStrm, const DffRecordHeader& rDgHeader );
    /** Processes the global shape group container (all shapes of a sheet). */
    void                ProcessShGrContainer( SvStream& rDffStrm, const DffRecordHeader& rShGrHeader );
    /** Processes the solver container (connectors of a sheet). */
    void                ProcessSolverContainer( SvStream& rDffStrm, const DffRecordHeader& rSolverHeader );
    /** Processes a shape or shape group container (one top-level shape). */
    void                ProcessShContainer( SvStream& rDffStrm, const DffRecordHeader& rShHeader );

    /** Inserts the passed SdrObject into the document. This function takes ownership of pSdrObj! */
    void                InsertSdrObject( SdrObjList* pObjList, const XclImpDrawObjBase& rDrawObj, SdrObject* pSdrObj );
    /** Stores the standard controls form for the passed sheet in mxCurrForm member. */
    void                SetCurrentForm( SCTAB nScTab );

    /** Updates the used area of a sheet with the position and size of the passed object. */
    void                UpdateUsedArea( const XclImpDrawObjBase& rDrawObj );

private:
    typedef ::std::map< SCTAB, ScRange >    ScRangeMap;
    typedef ScfRef< ScfProgressBar >        ScfProgressBarRef;

    XclImpObjectManager& mrObjManager;      /// The Excel object manager.
    XclImpSolverContainer maSolverCont;     /// The solver container for connector rules.
    SotStorageStreamRef mxCtlsStrm;         /// The 'Ctls' stream for OCX form controls.
    ScRangeMap          maUsedAreaMap;      /// Used ranges for all sheets.
    ScfProgressBarRef   mxProgress;         /// The progress bar used in ProcessObj().
    ::com::sun::star::uno::Reference< ::com::sun::star::form::XForm >
                        mxCurrForm;         /// Controls form of current sheet (needed in virtual functions).
    sal_uInt32          mnOleImpFlags;      /// Application OLE import settings.
    sal_Int32           mnDefTextMargin;    /// Default margin in text boxes.
    sal_Int32           mnLastCtrlIndex;    /// Last insertion index of a form control (for macro events).
    SCTAB               mnCurrFormScTab;    /// Sheet index of form stored in mxCurrForm.
};

// The object manager =========================================================

/** Stores all drawing and OLE objects and additional data related to these objects. */
class XclImpObjectManager : protected XclImpRoot
{
public:
    explicit            XclImpObjectManager( const XclImpRoot& rRoot );
    virtual             ~XclImpObjectManager();

    // *** Read Excel records *** ---------------------------------------------

    /** Reads and returns a bitmap from the IMGDATA record. */
    static Graphic      ReadImgData( XclImpStream& rStrm );

    /** Reads a plain OBJ record (without leading DFF data). */
    void                ReadObj( XclImpStream& rStrm );
    /** Reads the MSODRAWINGGROUP record. */
    void                ReadMsoDrawingGroup( XclImpStream& rStrm );
    /** Reads the MSODRAWING or MSODRAWINGSELECTION record. */
    void                ReadMsoDrawing( XclImpStream& rStrm );
    /** Reads the NOTE record. */
    void                ReadNote( XclImpStream& rStrm );

    /** Inserts a new chart object and reads the chart substream (BOF/EOF block).
        @descr  Used to import chart sheets, which do not have a corresponding OBJ record. */
    void                ReadTabChart( XclImpStream& rStrm );

    // *** Drawing objects *** ------------------------------------------------

    /** Finds the OBJ record data related to the DFF shape at the passed position. */
    XclImpDrawObjRef    FindDrawObj( const DffRecordHeader& rHeader ) const;
    /** Finds the OBJ record data specified by the passed object identifier. */
    XclImpDrawObjRef    FindDrawObj( const XclObjId& rObjId ) const;
    /** Finds the textbox data related to the DFF shape at the passed position. */
    const XclImpObjTextData* FindTextData( const DffRecordHeader& rHeader ) const;

    /** Sets the object with the passed identification to be skipped on import. */
    void                SetSkipObj( SCTAB nScTab, sal_uInt16 nObjId );

    // *** Drawing object conversion *** --------------------------------------

    /** Returns the DFF manager (DFF stream converter). Don't call before the DFF stream is read. */
    XclImpDffManager&   GetDffManager();
    /** Inserts all objects into the Calc document. */
    void                ConvertObjects();

    /** Returns the default name for the passed object. */
    String              GetDefaultObjName( const XclImpDrawObjBase& rDrawObj ) const;
    /** Returns the used area in the sheet with the passed index. */
    ScRange             GetUsedArea( SCTAB nScTab ) const;

    // ------------------------------------------------------------------------
private:
    /** Reads and returns a bitmap from WMF/PICT format. */
    static void         ReadWmf( Graphic& rGraphic, XclImpStream& rStrm );
    /** Reads and returns a bitmap from BMP format. */
    static void         ReadBmp( Graphic& rGraphic, XclImpStream& rStrm );

    /** Reads contents of an DFF record and append data to internal DFF stream. */
    void                ReadDffRecord( XclImpStream& rStrm );
    /** Reads a BIFF8 OBJ record following an MSODRAWING record. */
    void                ReadObj8( XclImpStream& rStrm );
    /** Reads the TXO record and following CONTINUE records containing string and formatting. */
    void                ReadTxo( XclImpStream& rStrm );

    /** Reads a BIFF3-BIFF5 NOTE record. */
    void                ReadNote3( XclImpStream& rStrm );
    /** Reads a BIFF8 NOTE record. */
    void                ReadNote8( XclImpStream& rStrm );

    /** Returns the size of the progress bar shown while processing all objects. */
    sal_Size            GetProgressSize() const;

    // ------------------------------------------------------------------------
private:
    typedef ::std::map< sal_Size, XclImpDrawObjRef >    XclImpObjMap;
    typedef ::std::map< XclObjId, XclImpDrawObjRef >    XclImpObjMapById;
    typedef ScfRef< XclImpObjTextData >                 XclImpObjTextRef;
    typedef ::std::map< sal_Size, XclImpObjTextRef >    XclImpObjTextMap;
    typedef ::std::vector< XclObjId >                   XclObjIdVec;

    typedef ::std::map< sal_uInt16, String >            DefObjNameMap;
    typedef ::std::vector< sal_Size >                   StreamPosVec;
    typedef ScfRef< XclImpDffManager >                  XclImpDffMgrRef;

    XclImpDrawObjVector maRawObjs;          /// BIFF5 objects without DFF data.
    XclImpObjMap        maObjMap;           /// Maps BIFF8 drawing objects to DFF stream position.
    XclImpObjMapById    maObjMapId;         /// Maps BIFF8 drawing objects to sheet index and object ID.
    XclImpObjTextMap    maTextMap;          /// Maps BIFF8 TXO textbox data to DFF stream position.
    XclObjIdVec         maSkipObjs;         /// All objects to be skipped.

    DefObjNameMap       maDefObjNames;      /// Default base names for all object types.
    SvMemoryStream      maDffStrm;          /// Copy of DFF stream in memory.
    StreamPosVec        maTabStrmPos;       /// Start positions of DFF stream fragments for all sheets.
    XclImpDffMgrRef     mxDffManager;       /// The DFF stream converter.
};

// DFF property set helper ====================================================

/** This class reads an DFF property set (msofbtOPT record).

    It can return separate property values or an item set which contains items
    translated from these properties.
 */
class XclImpDffPropSet : protected XclImpRoot
{
public:
    explicit            XclImpDffPropSet( const XclImpRoot& rRoot );

    /** Reads an DFF property set from the stream.
        @descr  The stream must point to the start of an DFF record containing properties. */
    void                Read( XclImpStream& rStrm );

    /** Returns the specified property or the default value, if not extant. */
    sal_uInt32          GetPropertyValue( sal_uInt16 nPropId, sal_uInt32 nDefault = 0 ) const;

    /** Translates the properties and fills the item set. */
    void                FillToItemSet( SfxItemSet& rItemSet ) const;

private:
    typedef ::std::auto_ptr< SvMemoryStream > SvMemoryStreamPtr;

    SvMemoryStream      maDummyStrm;    /// Dummy stream for DFF manager.
    XclImpSimpleDffManager maDffManager;/// DFF manager used to resolve palette colors.
    SvMemoryStreamPtr   mxMemStrm;      /// Helper stream.
};

XclImpStream& operator>>( XclImpStream& rStrm, XclImpDffPropSet& rPropSet );

// ============================================================================

#endif

