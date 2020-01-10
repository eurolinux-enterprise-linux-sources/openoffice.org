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
#include "precompiled_svx.hxx"
#include <svx/framelink.hxx>

#include <math.h>
#include <vcl/outdev.hxx>
#include <svx/borderline.hxx>

// ----------------------------------------------------------------------------

/** Define to select the drawing mode of thin dotted lines.

    0 = Draw lines using an own implementation (recommended). Draws always
        little dots in an appropriate distance.
    1 = Draw dotted lines using vcl/LineInfo. Results in dashed lines instead
        of dotted lines, which may look ugly for diagonal lines.
 */
#define SVX_FRAME_USE_LINEINFO 0

// ----------------------------------------------------------------------------

#if SVX_FRAME_USE_LINEINFO
#include <vcl/lineinfo.hxx>
#endif

namespace svx {
namespace frame {

// ============================================================================
// ============================================================================

namespace {

typedef std::vector< Point > PointVec;

// ----------------------------------------------------------------------------
// Link result structs for horizontal and vertical lines and borders.

/** Result struct used by the horizontal/vertical frame link functions.

    This struct is used to return coordinate offsets for one end of a single
    line in a frame border, i.e. the left end of the primary line of a
    horizontal frame border.

    1) Usage for horizontal lines

    If this struct is returned by the lclLinkHorFrameBorder() function, each
    member refers to the X coordinate of one edge of a single line end in a
    horizontal frame border. They specify an offset to modify this coordinate
    when the line is painted. The values in this struct may change a
    rectangular line shape into a line with slanted left or right border, which
    is used to connect the line with diagonal lines.

    Usage for a left line end:          Usage for a right line end:
                mnOffs1                         mnOffs1
                <------->                       <------->
                    +-------------------------------+
                    | the original horizontal line  |
                    +-------------------------------+
                <------->                       <------->
                mnOffs2                         mnOffs2

    2) Usage for vertical lines

    If this struct is returned by the lclLinkVerFrameBorder() function, each
    member refers to the Y coordinate of one edge of a single line end in a
    vertical frame border. They specify an offset to modify this coordinate
    when the line is painted. The values in this struct may change a
    rectangular line shape into a line with slanted top or bottom border, which
    is used to connect the line with diagonal lines.

    Usage for a top line end:       mnOffs1 ^               ^ mnOffs2
                                            |   +-------+   |
                                            v   |       |   v
                                                |       |
                                                |       |
                the original vertical line ---> |       |
                                                |       |
                                                |       |
                                            ^   |       |   ^
                                            |   +-------+   |
    Usage for a bottom line end:    mnOffs1 v               v mnOffs2
 */
struct LineEndResult
{
    long                mnOffs1;    /// Offset for top or left edge, dependent of context.
    long                mnOffs2;    /// Offset for bottom or right edge, dependent of context

    inline explicit     LineEndResult() : mnOffs1( 0 ), mnOffs2( 0 ) {}

    inline void         Swap() { std::swap( mnOffs1, mnOffs2 ); }
    inline void         Negate() { mnOffs1 = -mnOffs1; mnOffs2 = -mnOffs2; }
};

/** Result struct used by the horizontal/vertical frame link functions.

    This struct contains the linking results for one end of a frame border,
    including both the primary and secondary line ends.
 */
struct BorderEndResult
{
    LineEndResult       maPrim;     /// Result for primary line.
    LineEndResult       maSecn;     /// Result for secondary line.

    inline void         Negate() { maPrim.Negate(); maSecn.Negate(); }
};

/** Result struct used by the horizontal/vertical frame link functions.

    This struct contains the linking results for both frame border ends, and
    therefore for the complete frame border.
 */
struct BorderResult
{
    BorderEndResult     maBeg;      /// Result for begin of border line (left or top end).
    BorderEndResult     maEnd;      /// Result for end of border line (right or bottom end).
};

// ----------------------------------------------------------------------------
// Link result structs for diagonal lines and borders.

/** Result struct used by the diagonal frame link functions.

    This struct contains the linking results for one line of a diagonal frame
    border.
 */
struct DiagLineResult
{
    long                mnLClip;    /// Offset for left border of clipping rectangle.
    long                mnRClip;    /// Offset for right border of clipping rectangle.
    long                mnTClip;    /// Offset for top border of clipping rectangle.
    long                mnBClip;    /// Offset for bottom border of clipping rectangle.

    inline explicit     DiagLineResult() : mnLClip( 0 ), mnRClip( 0 ), mnTClip( 0 ), mnBClip( 0 ) {}
};

/** Result struct used by the diagonal frame link functions.

    This struct contains the linking results for one diagonal frame border.
 */
struct DiagBorderResult
{
    DiagLineResult      maPrim;     /// Result for primary line.
    DiagLineResult      maSecn;     /// Result for secondary line.
};

/** Result struct used by the diagonal frame link functions.

    This struct contains the linking results for both diagonal frame borders.
 */
struct DiagBordersResult
{
    DiagBorderResult    maTLBR;     /// Result for top-left to bottom-right frame border.
    DiagBorderResult    maBLTR;     /// Result for bottom-left to top-right frame border.
};

// ----------------------------------------------------------------------------

/** A helper struct containing two points of a line.
 */
struct LinePoints
{
    Point               maBeg;      /// Start position of the line.
    Point               maEnd;      /// End position of the line.

    explicit            LinePoints( const Point& rBeg, const Point& rEnd ) :
                            maBeg( rBeg ), maEnd( rEnd ) {}
    explicit            LinePoints( const Rectangle& rRect, bool bTLBR ) :
                            maBeg( bTLBR ? rRect.TopLeft() : rRect.TopRight() ),
                            maEnd( bTLBR ? rRect.BottomRight() : rRect.BottomLeft() ) {}
};

// ============================================================================

/** Rounds and casts a double value to a long value. */
inline long lclD2L( double fValue )
{
    return static_cast< long >( (fValue < 0.0) ? (fValue - 0.5) : (fValue + 0.5) );
}

/** Converts a width in twips to a width in another map unit (specified by fScale). */
sal_uInt16 lclScaleValue( long nValue, double fScale, sal_uInt16 nMaxWidth )
{
    // convert any width except 0 to at least 1 unit
    // #i61324# 1 twip must scale to 1/100mm
    return nValue ? static_cast< sal_uInt16 >( std::min< long >( std::max(
        static_cast< long >( nValue * fScale ), 1L ), nMaxWidth ) ) : 0;
}

// ----------------------------------------------------------------------------
// Line width offset calculation.

/** Returns the start offset of the single/primary line across the frame border.

    All following lclGet*Beg() and lclGet*End() functions return sub units to
    increase the computational accuracy, where 256 sub units are equal to
    1 map unit of the used OutputDevice.

    The following pictures show the upper end of a vertical frame border and
    illustrates the return values of all following lclGet*Beg() and lclGet*End()
    functions. The first picture shows a single frame border, the second picture
    shows a double frame border.

    The functions regard the reference point handling mode of the passed border
    style.
    REFMODE_CENTERED:
        All returned offsets are relative to the middle position of the frame
        border (offsets left of the middle are returned negative, offsets right
        of the middle are returned positive).
    REFMODE_BEGIN:
        All returned offsets are relative to the begin of the frame border
        (lclGetBeg() always returns 0).
    REFMODE_END:
        All returned offsets are relative to the end of the frame border
        (lclGetEnd() always returns 0).

                                                        |<- lclGetEnd()
                       |<- lclGetBeforeBeg()            |<- lclGetPrimEnd()
                       |                                |
                       ||<- lclGetBeg()                 ||<- lclGetBehindEnd()
                       ||                               ||
                       |#################################|
       direction of |   #################################
          the frame |   #################################
          border is |   #################################
           vertical |   #################################
                    v   #################################
                                        |
                                        |<- middle of the frame border


                                         lclGetDistEnd() ->||<- lclGetSecnBeg()
                                                           ||
          lclGetBeg() ->|   lclGetDistBeg() ->|            ||           |<- lclGetEnd()
                        |                     |            ||           |
    lclGetBeforeBeg()->||  lclGetPrimEnd() ->||            ||           ||<- lclGetBehindEnd()
                       ||                    ||            ||           ||
                       |######################|            |#############|
       direction of |   ######################              #############
          the frame |   ######################              #############
          border is |   ######################              #############
           vertical |   ######################  |           #############
                    v   ######################  |           #############
                        primary line            |           secondary line
                                                |
                                                |<- middle of the frame border

    @return
        The start offset of the single/primary line relative to the reference
        position of the frame border (sub units; 0 for invisible or one pixel
        wide single frame styles).
 */
long lclGetBeg( const Style& rBorder )
{
    long nPos = 0;
    switch( rBorder.GetRefMode() )
    {
        case REFMODE_CENTERED:  if( rBorder.Prim() ) nPos = -128 * (rBorder.GetWidth() - 1); break;
        case REFMODE_END:       if( rBorder.Prim() ) nPos = -256 * (rBorder.GetWidth() - 1); break;
        case REFMODE_BEGIN:     break;
    }
    return nPos;
}

/** Returns the end offset of the single/secondary line across the frame border.
    @descr  See description of lclGetBeg() for an illustration.
    @return  The end offset of the single/secondary line relative to the
    reference position of the frame border (sub units; 0 for invisible or one
    pixel wide single frame styles). */
long lclGetEnd( const Style& rBorder )
{
    long nPos = 0;
    switch( rBorder.GetRefMode() )
    {
        case REFMODE_CENTERED:  if( rBorder.Prim() ) nPos = 128 * (rBorder.GetWidth() - 1); break;
        case REFMODE_BEGIN:     if( rBorder.Prim() ) nPos = 256 * (rBorder.GetWidth() - 1); break;
        case REFMODE_END:     break;
    }
    return nPos;
}

/** Returns the end offset of the primary line across the frame border.
    @descr  See description of lclGetBeg() for an illustration.
    @return  The end offset of the primary line relative to the reference
    position of the frame border (sub units; the end of the primary line in a
    double frame style, otherwise the same as lclGetEnd()). */
inline long lclGetPrimEnd( const Style& rBorder )
{ return rBorder.Prim() ? (lclGetBeg( rBorder ) + 256 * (rBorder.Prim() - 1)) : 0; }

/** Returns the start offset of the secondary line across the frame border.
    @descr  See description of lclGetBeg() for an illustration.
    @return  The start offset of the secondary line relative to the reference
    position of the frame border (sub units; 0 for single/invisible border
    styles). */
inline long lclGetSecnBeg( const Style& rBorder )
{ return rBorder.Secn() ? (lclGetEnd( rBorder ) - 256 * (rBorder.Secn() - 1)) : 0; }

/** Returns the start offset of the distance space across the frame border.
    @descr  See description of lclGetBeg() for an illustration.
    @return  The start offset of the distance space relative to the reference
    position of the frame border (sub units; 0 for single/invisible border
    styles). */
inline long lclGetDistBeg( const Style& rBorder )
{ return rBorder.Secn() ? (lclGetBeg( rBorder ) + 256 * rBorder.Prim()) : 0; }

/** Returns the end offset of the distance space across the frame border.
    @descr  See description of lclGetBeg() for an illustration.
    @return  The end offset of the distance space relative to the reference
    position of the frame border (sub units; 0 for single/invisible border
    styles). */
inline long lclGetDistEnd( const Style& rBorder )
{ return rBorder.Secn() ? (lclGetEnd( rBorder ) - 256 * rBorder.Secn()) : 0; }

/** Returns the offset before start of single/primary line across the frame border.
    @descr  See description of lclGetBeg() for an illustration.
    @return  The offset directly before start of single/primary line relative
    to the reference position of the frame border (sub units; a value one less
    than lclGetBeg() for visible frame styles, or 0 for invisible frame style). */
inline long lclGetBeforeBeg( const Style& rBorder )
{ return rBorder.Prim() ? (lclGetBeg( rBorder ) - 256) : 0; }

/** Returns the offset behind end of single/secondary line across the frame border.
    @descr  See description of lclGetBeg() for an illustration.
    @return  The offset directly behind end of single/secondary line relative
    to the reference position of the frame border (sub units; a value one
    greater than lclGetEnd() for visible frame styles, or 0 for invisible frame
    style). */
inline long lclGetBehindEnd( const Style& rBorder )
{ return rBorder.Prim() ? (lclGetEnd( rBorder ) + 256) : 0; }

// ============================================================================
// Linking functions
// ============================================================================

// ----------------------------------------------------------------------------
// Linking of single horizontal line ends.

/** Calculates X offsets for the left end of a single horizontal frame border.

    See DrawHorFrameBorder() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for the
        X coordinates of the left line end.
 */
void lclLinkLeftEnd_Single(
        LineEndResult& rResult, const Style& rBorder,
        const DiagStyle& rLFromTR, const Style& rLFromT, const Style& rLFromL, const Style& rLFromB, const DiagStyle& rLFromBR )
{
    // both vertical and diagonal frame borders are double
    if( rLFromT.Secn() && rLFromB.Secn() && rLFromTR.Secn() && rLFromBR.Secn() )
    {
        // take left position of upper and lower secondary start
        rResult.mnOffs1 = GetBLDiagOffset( lclGetBeg( rBorder ), lclGetSecnBeg( rLFromTR ), rLFromTR.GetAngle() );
        rResult.mnOffs2 = GetTLDiagOffset( lclGetEnd( rBorder ), lclGetSecnBeg( rLFromBR ), rLFromBR.GetAngle() );
    }
    else
    {
        // both vertical frame borders are double
        if( rLFromT.Secn() && rLFromB.Secn() )
            rResult.mnOffs1 = (!rLFromTR.Secn() && !rLFromBR.Secn() && (rLFromT.GetWidth() == rLFromB.GetWidth())) ?
                // don't overdraw vertical borders with equal width
                lclGetBehindEnd( rLFromT ) :
                // take leftmost start of both secondary lines (#46488#)
                rResult.mnOffs1 = std::min( lclGetSecnBeg( rLFromT ), lclGetSecnBeg( rLFromB ) );

        // single border with equal width coming from left
        else if( !rLFromL.Secn() && (rLFromL.Prim() == rBorder.Prim()) )
            // draw to connection point
            rResult.mnOffs1 = 0;

        // single border coming from left
        else if( !rLFromL.Secn() && rLFromL.Prim() )
        {
            if( rLFromL.Prim() == rBorder.Prim() )
                // draw to reference position, if from left has equal width
                rResult.mnOffs1 = 0;
            else
                rResult.mnOffs1 = (rLFromL < rBorder) ?
                    // take leftmost start of both frame borders, if from left is thinner
                    std::min( lclGetBeg( rLFromT ), lclGetBeg( rLFromB ) ) :
                    // do not overdraw vertical, if from left is thicker
                    std::max( lclGetBehindEnd( rLFromT ), lclGetBehindEnd( rLFromB ) );
        }

        // no border coming from left
        else if( !rLFromL.Prim() )
            // don't overdraw vertical borders with equal width
            rResult.mnOffs1 = (rLFromT.GetWidth() == rLFromB.GetWidth()) ?
                lclGetBehindEnd( rLFromT ) :
                std::min( lclGetBeg( rLFromT ), lclGetBeg( rLFromB ) );

        // double frame border coming from left and from top
        else if( rLFromT.Secn() )
            // do not overdraw the vertical double frame border
            rResult.mnOffs1 = lclGetBehindEnd( rLFromT );

        // double frame border coming from left and from bottom
        else if( rLFromB.Secn() )
            // do not overdraw the vertical double frame border
            rResult.mnOffs1 = lclGetBehindEnd( rLFromB );

        // double frame border coming from left, both vertical frame borders are single or off
        else
            // draw from leftmost start of both frame borders, if from left is not thicker
            rResult.mnOffs1 = (rLFromL <= rBorder) ?
                std::min( lclGetBeg( rLFromT ), lclGetBeg( rLFromB ) ) :
                std::max( lclGetBehindEnd( rLFromT ), lclGetBehindEnd( rLFromB ) );

        // bottom-left point is equal to top-left point (results in rectangle)
        rResult.mnOffs2 = rResult.mnOffs1;
    }
}

/** Calculates X offsets for the left end of a primary horizontal line.

    See DrawHorFrameBorder() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for the
        X coordinates of the left end of the primary line.
 */
void lclLinkLeftEnd_Prim(
        LineEndResult& rResult, const Style& rBorder,
        const DiagStyle& rLFromTR, const Style& rLFromT, const Style& rLFromL, const Style& rLFromB, const DiagStyle& /*rLFromBR*/ )
{
    // double diagonal frame border coming from top right
    if( rLFromTR.Secn() )
    {
        // draw from where secondary diagonal line meets the own primary
        rResult.mnOffs1 = GetBLDiagOffset( lclGetBeg( rBorder ), lclGetSecnBeg( rLFromTR ), rLFromTR.GetAngle() );
        rResult.mnOffs2 = GetBLDiagOffset( lclGetPrimEnd( rBorder ), lclGetSecnBeg( rLFromTR ), rLFromTR.GetAngle() );
    }

    // no or single diagonal frame border - ignore it
    else
    {
        // double frame border coming from top
        if( rLFromT.Secn() )
            // draw from left edge of secondary vertical
            rResult.mnOffs1 = lclGetSecnBeg( rLFromT );

        // double frame border coming from left (from top is not double)
        else if( rLFromL.Secn() )
            // do not overdraw single frame border coming from top
            rResult.mnOffs1 = (rLFromL.GetWidth() == rBorder.GetWidth()) ?
                0 : lclGetBehindEnd( rLFromT );

        // double frame border coming from bottom (from top and from left are not double)
        else if( rLFromB.Secn() )
            // draw from left edge of primary vertical from bottom
            rResult.mnOffs1 = lclGetBeg( rLFromB );

        // no other frame border is double
        else
            // do not overdraw vertical frame borders
            rResult.mnOffs1 = std::max( lclGetBehindEnd( rLFromT ), lclGetBehindEnd( rLFromB ) );

        // bottom-left point is equal to top-left point (results in rectangle)
        rResult.mnOffs2 = rResult.mnOffs1;
    }
}

/** Calculates X offsets for the left end of a secondary horizontal line.

    See DrawHorFrameBorder() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for the
        X coordinates of the left end of the secondary line.
 */
void lclLinkLeftEnd_Secn(
        LineEndResult& rResult, const Style& rBorder,
        const DiagStyle& rLFromTR, const Style& rLFromT, const Style& rLFromL, const Style& rLFromB, const DiagStyle& rLFromBR )
{
    /*  Recycle lclLinkLeftEnd_Prim() function with mirrored horizontal borders. */
    lclLinkLeftEnd_Prim( rResult, rBorder.Mirror(), rLFromBR, rLFromB, rLFromL.Mirror(), rLFromT, rLFromTR );
    rResult.Swap();
}

// ----------------------------------------------------------------------------
// Linking of horizontal frame border ends.

/** Calculates X offsets for the left end of a horizontal frame border.

    This function can be used for single and double frame borders.
    See DrawHorFrameBorder() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for the
        X coordinates of the left end of the line(s) in the frame border.
 */
void lclLinkLeftEnd(
        BorderEndResult& rResult, const Style& rBorder,
        const DiagStyle& rLFromTR, const Style& rLFromT, const Style& rLFromL, const Style& rLFromB, const DiagStyle& rLFromBR )
{
    if( rBorder.Secn() )
    {
        // current frame border is double
        lclLinkLeftEnd_Prim( rResult.maPrim, rBorder, rLFromTR, rLFromT, rLFromL, rLFromB, rLFromBR );
        lclLinkLeftEnd_Secn( rResult.maSecn, rBorder, rLFromTR, rLFromT, rLFromL, rLFromB, rLFromBR );
    }
    else if( rBorder.Prim() )
    {
        // current frame border is single
        lclLinkLeftEnd_Single( rResult.maPrim, rBorder, rLFromTR, rLFromT, rLFromL, rLFromB, rLFromBR );
    }
    else
    {
        DBG_ERRORFILE( "lclLinkLeftEnd - called for invisible frame style" );
    }
}

/** Calculates X offsets for the right end of a horizontal frame border.

    This function can be used for single and double frame borders.
    See DrawHorFrameBorder() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for the
        X coordinates of the right end of the line(s) in the frame border.
 */
void lclLinkRightEnd(
        BorderEndResult& rResult, const Style& rBorder,
        const DiagStyle& rRFromTL, const Style& rRFromT, const Style& rRFromR, const Style& rRFromB, const DiagStyle& rRFromBL )
{
    /*  Recycle lclLinkLeftEnd() function with mirrored vertical borders. */
    lclLinkLeftEnd( rResult, rBorder, rRFromTL.Mirror(), rRFromT.Mirror(), rRFromR, rRFromB.Mirror(), rRFromBL.Mirror() );
    rResult.Negate();
}

// ----------------------------------------------------------------------------
// Linking of horizontal and vertical frame borders.

/** Calculates X offsets for all line ends of a horizontal frame border.

    This function can be used for single and double frame borders.
    See DrawHorFrameBorder() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for the
        X coordinates of both ends of the line(s) in the frame border. To get
        the actual X coordinates to draw the lines, these offsets have to be
        added to the X coordinates of the reference points of the frame border
        (the offsets may be negative).
 */
void lclLinkHorFrameBorder(
        BorderResult& rResult, const Style& rBorder,
        const DiagStyle& rLFromTR, const Style& rLFromT, const Style& rLFromL, const Style& rLFromB, const DiagStyle& rLFromBR,
        const DiagStyle& rRFromTL, const Style& rRFromT, const Style& rRFromR, const Style& rRFromB, const DiagStyle& rRFromBL )
{
    lclLinkLeftEnd( rResult.maBeg, rBorder, rLFromTR, rLFromT, rLFromL, rLFromB, rLFromBR );
    lclLinkRightEnd( rResult.maEnd, rBorder, rRFromTL, rRFromT, rRFromR, rRFromB, rRFromBL );
}

/** Calculates Y offsets for all line ends of a vertical frame border.

    This function can be used for single and double frame borders.
    See DrawVerFrameBorder() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for the
        Y coordinates of both ends of the line(s) in the frame border. To get
        the actual Y coordinates to draw the lines, these offsets have to be
        added to the Y coordinates of the reference points of the frame border
        (the offsets may be negative).
 */
void lclLinkVerFrameBorder(
        BorderResult& rResult, const Style& rBorder,
        const DiagStyle& rTFromBL, const Style& rTFromL, const Style& rTFromT, const Style& rTFromR, const DiagStyle& rTFromBR,
        const DiagStyle& rBFromTL, const Style& rBFromL, const Style& rBFromB, const Style& rBFromR, const DiagStyle& rBFromTR )
{
    /*  Recycle lclLinkHorFrameBorder() function with correct parameters. The
        frame border is virtually mirrored at the top-left to bottom-right
        diagonal. rTFromBR and rBFromTL are mirrored to process their primary
        and secondary lines correctly. */
    lclLinkHorFrameBorder( rResult, rBorder,
        rTFromBL, rTFromL, rTFromT, rTFromR, rTFromBR.Mirror(),
        rBFromTL.Mirror(), rBFromL, rBFromB, rBFromR, rBFromTR );
}

// ============================================================================

#if 0
//  Not used anymore, but not deleted for possible future usage.

/** Returns the relative Y offset of the intercept point of 2 diagonal borders.

    @param nTLBROffs
        Width offset (sub units) across the top-left to bottom-right frame border.
    @param fTLBRAngle
        Inner angle between horizontal and top-left to bottom-right frame border.
    @param nBLTROffs
        Width offset (sub units) across the bottom-left to top-right frame border.
    @param fBLTRAngle
        Inner angle between horizontal and bottom-left to top-right frame border.
    @return
        Offset (sub units) relative to the Y position of the centered intercept
        point of both diagonal frame borders.
 */
long lclGetDiagDiagOffset( long nTLBROffs, double fTLBRAngle, long nBLTROffs, double fBLTRAngle )
{
    double fASin = sin( fTLBRAngle );
    double fACos = cos( fTLBRAngle );
    double fAX = -nTLBROffs * fASin;
    double fAY = nTLBROffs * fACos;
    double fRAX = fACos;
    double fRAY = fASin;

    double fBSin = sin( fBLTRAngle );
    double fBCos = cos( fBLTRAngle );
    double fBX = nBLTROffs * fBSin;
    double fBY = nBLTROffs * fBCos;
    double fRBX = fBCos;
    double fRBY = -fBSin;

    double fKA = (fRBX * (fBY - fAY) - fRBY * (fBX - fAX)) / (fRBX * fRAY - fRAX * fRBY);
    return lclD2L( fAY + fKA * fRAY );
}
#endif

// ----------------------------------------------------------------------------
// Linking of diagonal frame borders.

/** Calculates clipping offsets for a top-left to bottom-right frame border.

    This function can be used for single and double frame borders.
    See DrawDiagFrameBorders() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for all
        borders of the reference rectangle containing the diagonal frame border.
 */
void lclLinkTLBRFrameBorder(
        DiagBorderResult& rResult, const Style& rBorder,
        const Style& rTLFromB, const Style& rTLFromR, const Style& rBRFromT, const Style& rBRFromL )
{
    bool bIsDbl = rBorder.Secn() != 0;

    rResult.maPrim.mnLClip = lclGetBehindEnd( rTLFromB );
    rResult.maPrim.mnRClip = (bIsDbl && rBRFromT.Secn()) ? lclGetEnd( rBRFromT ) : lclGetBeforeBeg( rBRFromT );
    rResult.maPrim.mnTClip = (bIsDbl && rTLFromR.Secn()) ? lclGetBeg( rTLFromR ) : lclGetBehindEnd( rTLFromR );
    rResult.maPrim.mnBClip = lclGetBeforeBeg( rBRFromL );

    if( bIsDbl )
    {
        rResult.maSecn.mnLClip = rTLFromB.Secn() ? lclGetBeg( rTLFromB ) : lclGetBehindEnd( rTLFromB );
        rResult.maSecn.mnRClip = lclGetBeforeBeg( rBRFromT );
        rResult.maSecn.mnTClip = lclGetBehindEnd( rTLFromR );
        rResult.maSecn.mnBClip = rBRFromL.Secn() ? lclGetEnd( rBRFromL ) : lclGetBeforeBeg( rBRFromL );
    }
}

/** Calculates clipping offsets for a bottom-left to top-right frame border.

    This function can be used for single and double frame borders.
    See DrawDiagFrameBorders() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for all
        borders of the reference rectangle containing the diagonal frame border.
 */
void lclLinkBLTRFrameBorder(
        DiagBorderResult& rResult, const Style& rBorder,
        const Style& rBLFromT, const Style& rBLFromR, const Style& rTRFromB, const Style& rTRFromL )
{
    bool bIsDbl = rBorder.Secn() != 0;

    rResult.maPrim.mnLClip = lclGetBehindEnd( rBLFromT );
    rResult.maPrim.mnRClip = (bIsDbl && rTRFromB.Secn()) ? lclGetEnd( rTRFromB ) : lclGetBeforeBeg( rTRFromB );
    rResult.maPrim.mnTClip = lclGetBehindEnd( rTRFromL );
    rResult.maPrim.mnBClip = (bIsDbl && rBLFromR.Secn()) ? lclGetEnd( rBLFromR ) : lclGetBeforeBeg( rBLFromR );

    if( bIsDbl )
    {
        rResult.maSecn.mnLClip = rBLFromT.Secn() ? lclGetBeg( rBLFromT ) : lclGetBehindEnd( rBLFromT );
        rResult.maSecn.mnRClip = lclGetBeforeBeg( rTRFromB );
        rResult.maSecn.mnTClip = rTRFromL.Secn() ? lclGetBeg( rTRFromL ) : lclGetBehindEnd( rTRFromL );
        rResult.maSecn.mnBClip = lclGetBeforeBeg( rBLFromR );
    }
}

/** Calculates clipping offsets for both diagonal frame borders.

    This function can be used for single and double frame borders.
    See DrawDiagFrameBorders() function for a description of all parameters.

    @param rResult
        (out-param) The contained values (sub units) specify offsets for all
        borders of the reference rectangle containing the diagonal frame
        borders.
 */
void lclLinkDiagFrameBorders(
        DiagBordersResult& rResult, const Style& rTLBR, const Style& rBLTR,
        const Style& rTLFromB, const Style& rTLFromR, const Style& rBRFromT, const Style& rBRFromL,
        const Style& rBLFromT, const Style& rBLFromR, const Style& rTRFromB, const Style& rTRFromL )
{
    lclLinkTLBRFrameBorder( rResult.maTLBR, rTLBR, rTLFromB, rTLFromR, rBRFromT, rBRFromL );
    lclLinkBLTRFrameBorder( rResult.maBLTR, rBLTR, rBLFromT, rBLFromR, rTRFromB, rTRFromL );
}

// ============================================================================
// Drawing functions
// ============================================================================

// ----------------------------------------------------------------------------
// Simple helper functions

/** Converts sub units to OutputDevice map units. */
inline long lclToMapUnit( long nSubUnits )
{
    return ((nSubUnits < 0) ? (nSubUnits - 127) : (nSubUnits + 128)) / 256;
}

/** Converts a point in sub units to an OutputDevice point. */
inline Point lclToMapUnit( long nSubXPos, long nSubYPos )
{
    return Point( lclToMapUnit( nSubXPos ), lclToMapUnit( nSubYPos ) );
}

/** Returns a polygon constructed from a vector of points. */
inline Polygon lclCreatePolygon( const PointVec& rPoints )
{
    return Polygon( static_cast< USHORT >( rPoints.size() ), &rPoints[ 0 ] );
}

/** Returns a polygon constructed from the four passed points. */
Polygon lclCreatePolygon( const Point& rP1, const Point& rP2, const Point& rP3, const Point& rP4 )
{
    PointVec aPoints;
    aPoints.reserve( 4 );
    aPoints.push_back( rP1 );
    aPoints.push_back( rP2 );
    aPoints.push_back( rP3 );
    aPoints.push_back( rP4 );
    return lclCreatePolygon( aPoints );
}

/** Returns a polygon constructed from the five passed points. */
Polygon lclCreatePolygon( const Point& rP1, const Point& rP2, const Point& rP3, const Point& rP4, const Point& rP5 )
{
    PointVec aPoints;
    aPoints.reserve( 5 );
    aPoints.push_back( rP1 );
    aPoints.push_back( rP2 );
    aPoints.push_back( rP3 );
    aPoints.push_back( rP4 );
    aPoints.push_back( rP5 );
    return lclCreatePolygon( aPoints );
}

/** Returns a polygon constructed from the two passed line positions. */
inline Polygon lclCreatePolygon( const LinePoints& rPoints1, const LinePoints& rPoints2 )
{
    return lclCreatePolygon( rPoints1.maBeg, rPoints1.maEnd, rPoints2.maEnd, rPoints2.maBeg );
}

/** Sets the color of the passed frame style to the output device.

    Sets the line color and fill color in the output device.

    @param rDev
        The output device the color has to be set to. The old colors are pushed
        onto the device's stack and can be restored with a call to
        OutputDevice::Pop(). Please take care about the correct calling order
        of Pop() if this function is used with other functions pushing
        something onto the stack.
    @param rStyle
        The border style that contains the line color to be set to the device.
 */
void lclSetColorToOutDev( OutputDevice& rDev, const Style& rStyle, const Color* pForceColor )
{
    rDev.Push( PUSH_LINECOLOR | PUSH_FILLCOLOR );
    rDev.SetLineColor( pForceColor ? *pForceColor : rStyle.GetColor() );
    rDev.SetFillColor( pForceColor ? *pForceColor : rStyle.GetColor() );
}

// ----------------------------------------------------------------------------
// Generic drawing functions.

/** Draws a thin (1 pixel wide) line, optionally dotted, into the passed output device. */
void lclDrawThinLine( OutputDevice& rDev, const Point& rBeg, const Point& rEnd, bool bDotted )
{
#if SVX_FRAME_USE_LINEINFO
    if( bDotted && (rBeg != rEnd) )
    {
// using LineInfo for dotted lines looks ugly and does not work well for diagonal lines
        LineInfo aLineInfo( LINE_DASH, 1 );
        aLineInfo.SetDotCount( 1 );
        aLineInfo.SetDotLen( 1 );
        aLineInfo.SetDistance( 3 );
        rDev.DrawLine( rBeg, rEnd, aLineInfo );
    }
#else
    Point aBeg( rDev.LogicToPixel( rBeg ) );
    Point aEnd( rDev.LogicToPixel( rEnd ) );
    if( bDotted && (aBeg != aEnd) )
    {
        bool bHor = Abs( aEnd.X() - aBeg.X() ) > Abs( aEnd.Y() - aBeg.Y() );
        const Point& rBegPos( bHor ? ((aBeg.X() < aEnd.X()) ? aBeg : aEnd) : ((aBeg.Y() < aEnd.Y()) ? aBeg : aEnd ) );
        const Point& rEndPos( (rBegPos == aBeg) ? aEnd : aBeg );

        long nAlongBeg = bHor ? rBegPos.X() : rBegPos.Y();
        long nAcrssBeg = bHor ? rBegPos.Y() : rBegPos.X();
        long nAlongSize = (bHor ? rEndPos.X() : rEndPos.Y()) - nAlongBeg;
        long nAcrssSize = (bHor ? rEndPos.Y() : rEndPos.X()) - nAcrssBeg;
        double fGradient = static_cast< double >( nAcrssSize ) / nAlongSize;

        PointVec aPoints;
        aPoints.reserve( (nAlongSize + 1) / 2 );
        for( long nAlongIdx = 0; nAlongIdx <= nAlongSize; nAlongIdx += 2 )
        {
            long nAl = nAlongBeg + nAlongIdx;
            long nAc = nAcrssBeg + lclD2L( fGradient * nAlongIdx );
            aPoints.push_back( Point( bHor ? nAl : nAc, bHor ? nAc : nAl ) );
        }

        rDev.Push( PUSH_MAPMODE );
        rDev.SetMapMode( MAP_PIXEL );
        rDev.DrawPixel( lclCreatePolygon( aPoints ) );
        rDev.Pop(); // map mode
    }
#endif
    else
        rDev.DrawLine( rBeg, rEnd );
}

/** Draws a thin (1 pixel wide) line, optionally dotted, into the passed output device. */
inline void lclDrawThinLine( OutputDevice& rDev, const LinePoints& rPoints, bool bDotted )
{
    lclDrawThinLine( rDev, rPoints.maBeg, rPoints.maEnd, bDotted );
}

/** Draws a polygon with four points into the passed output device. */
inline void lclDrawPolygon( OutputDevice& rDev, const Point& rP1, const Point& rP2, const Point& rP3, const Point& rP4 )
{
    rDev.DrawPolygon( lclCreatePolygon( rP1, rP2, rP3, rP4 ) );
}

/** Draws a polygon specified by two borders into the passed output device. */
inline void lclDrawPolygon( OutputDevice& rDev, const LinePoints& rPoints1, const LinePoints& rPoints2 )
{
    rDev.DrawPolygon( lclCreatePolygon( rPoints1, rPoints2 ) );
}

// ============================================================================
// Drawing of horizontal frame borders.

/** Draws a horizontal thin or thick line into the passed output device.

    The X coordinates of the edges of the line are adjusted according to the
    passed LineEndResult structs. A one pixel wide line can be drawn dotted.
 */
void lclDrawHorLine(
        OutputDevice& rDev,
        const Point& rLPos, const LineEndResult& rLRes,
        const Point& rRPos, const LineEndResult& rRRes,
        long nTOffs, long nBOffs, bool bDotted )
{
    LinePoints aTPoints( rLPos + lclToMapUnit( rLRes.mnOffs1, nTOffs ), rRPos + lclToMapUnit( rRRes.mnOffs1, nTOffs ) );
    if( nTOffs == nBOffs )
        lclDrawThinLine( rDev, aTPoints, bDotted );
    else
    {
        LinePoints aBPoints( rLPos + lclToMapUnit( rLRes.mnOffs2, nBOffs ), rRPos + lclToMapUnit( rRRes.mnOffs2, nBOffs ) );
        lclDrawPolygon( rDev, aTPoints, aBPoints );
    }
}

/** Draws a horizontal frame border into the passed output device.

    @param rLPos
        The top-left or bottom-left reference point of the diagonal frame border.
    @param rRPos
        The top-right or bottom-right reference point of the diagonal frame border.
    @param rBorder
        The frame style used to draw the border.
    @param rResult
        The X coordinates of the edges of all lines of the frame border are
        adjusted according to the offsets contained here.
 */
void lclDrawHorFrameBorder(
        OutputDevice& rDev, const Point& rLPos, const Point& rRPos,
        const Style& rBorder, const BorderResult& rResult, const Color* pForceColor )
{
    DBG_ASSERT( rBorder.Prim(), "svx::frame::lclDrawHorFrameBorder - line not visible" );
    DBG_ASSERT( rLPos.X() <= rRPos.X(), "svx::frame::lclDrawHorFrameBorder - wrong order of line ends" );
    DBG_ASSERT( rLPos.Y() == rRPos.Y(), "svx::frame::lclDrawHorFrameBorder - line not horizontal" );
    if( rLPos.X() <= rRPos.X() )
    {
        lclSetColorToOutDev( rDev, rBorder, pForceColor );
        lclDrawHorLine( rDev, rLPos, rResult.maBeg.maPrim, rRPos, rResult.maEnd.maPrim,
            lclGetBeg( rBorder ), lclGetPrimEnd( rBorder ), rBorder.Dotted() );
        if( rBorder.Secn() )
            lclDrawHorLine( rDev, rLPos, rResult.maBeg.maSecn, rRPos, rResult.maEnd.maSecn,
                lclGetSecnBeg( rBorder ), lclGetEnd( rBorder ), rBorder.Dotted() );
        rDev.Pop(); // colors
    }
}

// ----------------------------------------------------------------------------
// Drawing of vertical frame borders.

/** Draws a vertical thin or thick line into the passed output device.

    The Y coordinates of the edges of the line are adjusted according to the
    passed LineEndResult structs. A one pixel wide line can be drawn dotted.
 */
void lclDrawVerLine(
        OutputDevice& rDev,
        const Point& rTPos, const LineEndResult& rTRes,
        const Point& rBPos, const LineEndResult& rBRes,
        long nLOffs, long nROffs, bool bDotted )
{
    LinePoints aLPoints( rTPos + lclToMapUnit( nLOffs, rTRes.mnOffs1 ), rBPos + lclToMapUnit( nLOffs, rBRes.mnOffs1 ) );
    if( nLOffs == nROffs )
        lclDrawThinLine( rDev, aLPoints, bDotted );
    else
    {
        LinePoints aRPoints( rTPos + lclToMapUnit( nROffs, rTRes.mnOffs2 ), rBPos + lclToMapUnit( nROffs, rBRes.mnOffs2 ) );
        lclDrawPolygon( rDev, aLPoints, aRPoints );
    }
}

/** Draws a vertical frame border into the passed output device.

    @param rTPos
        The top-left or top-right reference point of the diagonal frame border.
    @param rBPos
        The bottom-left or bottom-right reference point of the diagonal frame border.
    @param rBorder
        The frame style used to draw the border.
    @param rResult
        The Y coordinates of the edges of all lines of the frame border are
        adjusted according to the offsets contained here.
 */
void lclDrawVerFrameBorder(
        OutputDevice& rDev, const Point& rTPos, const Point& rBPos,
        const Style& rBorder, const BorderResult& rResult, const Color* pForceColor )
{
    DBG_ASSERT( rBorder.Prim(), "svx::frame::lclDrawVerFrameBorder - line not visible" );
    DBG_ASSERT( rTPos.Y() <= rBPos.Y(), "svx::frame::lclDrawVerFrameBorder - wrong order of line ends" );
    DBG_ASSERT( rTPos.X() == rBPos.X(), "svx::frame::lclDrawVerFrameBorder - line not vertical" );
    if( rTPos.Y() <= rBPos.Y() )
    {
        lclSetColorToOutDev( rDev, rBorder, pForceColor );
        lclDrawVerLine( rDev, rTPos, rResult.maBeg.maPrim, rBPos, rResult.maEnd.maPrim,
            lclGetBeg( rBorder ), lclGetPrimEnd( rBorder ), rBorder.Dotted() );
        if( rBorder.Secn() )
            lclDrawVerLine( rDev, rTPos, rResult.maBeg.maSecn, rBPos, rResult.maEnd.maSecn,
                lclGetSecnBeg( rBorder ), lclGetEnd( rBorder ), rBorder.Dotted() );
        rDev.Pop(); // colors
    }
}

// ============================================================================
// Drawing of diagonal frame borders, incudes clipping functions.

/** Returns the drawing coordinates for a diagonal thin line.

    This function can be used for top-left to bottom-right and for bottom-left
    to top-right lines.

    @param rRect
        The reference rectangle of the diagonal frame border.
    @param bTLBR
        true = top-left to bottom-right; false = bottom-left to top-right.
    @param nDiagOffs
        Width offset (sub units) across the diagonal frame border.
    @return
        A struct containg start and end position of the diagonal line.
 */
LinePoints lclGetDiagLineEnds( const Rectangle& rRect, bool bTLBR, long nDiagOffs )
{
    LinePoints aPoints( rRect, bTLBR );
    bool bVert = rRect.GetWidth() < rRect.GetHeight();
    double fAngle = bVert ? GetVerDiagAngle( rRect ) : GetHorDiagAngle( rRect );
    // vertical top-left to bottom-right borders are handled mirrored
    if( bVert && bTLBR )
        nDiagOffs = -nDiagOffs;
    long nTOffs = bTLBR ? GetTLDiagOffset( 0, nDiagOffs, fAngle ) : GetTRDiagOffset( 0, nDiagOffs, fAngle );
    long nBOffs = bTLBR ? GetBRDiagOffset( 0, nDiagOffs, fAngle ) : GetBLDiagOffset( 0, nDiagOffs, fAngle );
    // vertical bottom-left to top-right borders are handled with exchanged end points
    if( bVert && !bTLBR )
        std::swap( nTOffs, nBOffs );
    (bVert ? aPoints.maBeg.Y() : aPoints.maBeg.X()) += lclToMapUnit( nTOffs );
    (bVert ? aPoints.maEnd.Y() : aPoints.maEnd.X()) += lclToMapUnit( nBOffs );
    return aPoints;
}

// ----------------------------------------------------------------------------
// Clipping functions for diagonal frame borders.

/** Limits the clipping region to the inner area of a rectange.

    Takes the values from the passed DiagLineResult struct into account. They
    may specify to not clip one or more borders of a rectangle.

    @param rDev
        The output device with the clipping region to be modified. The old
        clipping region is pushed onto the device's stack and can be restored
        with a call to OutputDevice::Pop(). Please take care about the correct
        calling order of Pop() if this function is used with other functions
        pushing something onto the stack.
    @param rRect
        The reference rectangle of the diagonal frame borders.
    @param rResult
        The result struct containing modifies for each border of the reference
        rectangle.
 */
void lclPushDiagClipRect( OutputDevice& rDev, const Rectangle& rRect, const DiagLineResult& rResult )
{
    // PixelToLogic() regards internal offset of the output device
    Rectangle aClipRect( rRect );
    aClipRect.Left()   += lclToMapUnit( rResult.mnLClip );
    aClipRect.Top()    += lclToMapUnit( rResult.mnTClip );
    aClipRect.Right()  += lclToMapUnit( rResult.mnRClip );
    aClipRect.Bottom() += lclToMapUnit( rResult.mnBClip );
    // output device would adjust the rectangle -> invalidate it before
    if( (aClipRect.GetWidth() < 1) ||(aClipRect.GetHeight() < 1) )
        aClipRect.SetEmpty();

    rDev.Push( PUSH_CLIPREGION );
    rDev.IntersectClipRegion( aClipRect );
}

/** Excludes inner area of a crossing double frame border from clipping region.

    This function is used to modify the clipping region so that it excludes the
    inner free area of a double diagonal frame border. This makes it possible
    to draw a diagonal frame border in one step without taking care of the
    crossing double frame border.

    @param rDev
        The output device with the clipping region to be modified. The old
        clipping region is pushed onto the device's stack and can be restored
        with a call to OutputDevice::Pop(). Please take care about the correct
        calling order of Pop() if this function is used with other functions
        pushing something onto the stack.
    @param rRect
        The reference rectangle of the diagonal frame borders.
    @param bTLBR
        The orientation of the processed frame border (not the orientation of
        the crossing frame border).
    @param bCrossStyle
        The style of the crossing frame border. Must be a double frame style.
 */
void lclPushCrossingClipRegion( OutputDevice& rDev, const Rectangle& rRect, bool bTLBR, const Style& rCrossStyle )
{
    DBG_ASSERT( rCrossStyle.Secn(), "lclGetCrossingClipRegion - use only for double styles" );
    LinePoints aLPoints( lclGetDiagLineEnds( rRect, !bTLBR, lclGetPrimEnd( rCrossStyle ) ) );
    LinePoints aRPoints( lclGetDiagLineEnds( rRect, !bTLBR, lclGetSecnBeg( rCrossStyle ) ) );

    Region aClipReg;
    if( bTLBR )
    {
        aClipReg = lclCreatePolygon(
            aLPoints.maBeg, aLPoints.maEnd, rRect.BottomRight(), rRect.BottomLeft(), rRect.TopLeft() );
        aClipReg.Union( lclCreatePolygon(
            aRPoints.maBeg, aRPoints.maEnd, rRect.BottomRight(), rRect.TopRight(), rRect.TopLeft() ) );
    }
    else
    {
        aClipReg = lclCreatePolygon(
            aLPoints.maBeg, aLPoints.maEnd, rRect.BottomLeft(), rRect.TopLeft(), rRect.TopRight() );
        aClipReg.Union( lclCreatePolygon(
            aRPoints.maBeg, aRPoints.maEnd, rRect.BottomLeft(), rRect.BottomRight(), rRect.TopRight() ) );
    }

    rDev.Push( PUSH_CLIPREGION );
    rDev.IntersectClipRegion( aClipReg );
}

// ----------------------------------------------------------------------------
// Drawing functions for diagonal frame borders.

/** Draws a diagonal thin or thick line into the passed output device.

    The clipping region of the output device is modified according to the
    passed DiagLineResult struct. A one pixel wide line can be drawn dotted.
 */
void lclDrawDiagLine(
        OutputDevice& rDev, const Rectangle& rRect, bool bTLBR,
        const DiagLineResult& rResult, long nDiagOffs1, long nDiagOffs2, bool bDotted )
{
    lclPushDiagClipRect( rDev, rRect, rResult );
    LinePoints aLPoints( lclGetDiagLineEnds( rRect, bTLBR, nDiagOffs1 ) );
    if( nDiagOffs1 == nDiagOffs2 )
        lclDrawThinLine( rDev, aLPoints, bDotted );
    else
        lclDrawPolygon( rDev, aLPoints, lclGetDiagLineEnds( rRect, bTLBR, nDiagOffs2 ) );
    rDev.Pop(); // clipping region
}

/** Draws a diagonal frame border into the passed output device.

    The lines of the frame border are drawn interrupted, if the style of the
    crossing frame border is double.

    @param rRect
        The reference rectangle of the diagonal frame border.
    @param bTLBR
        The orientation of the diagonal frame border.
    @param rBorder
        The frame style used to draw the border.
    @param rResult
        Offsets (sub units) to modify the clipping region of the output device.
    @param rCrossStyle
        Style of the crossing diagonal frame border.
 */
void lclDrawDiagFrameBorder(
        OutputDevice& rDev, const Rectangle& rRect, bool bTLBR,
        const Style& rBorder, const DiagBorderResult& rResult, const Style& rCrossStyle,
        const Color* pForceColor, bool bDiagDblClip )
{
    DBG_ASSERT( rBorder.Prim(), "svx::frame::lclDrawDiagFrameBorder - line not visible" );

    bool bClip = bDiagDblClip && rCrossStyle.Secn();
    if( bClip )
        lclPushCrossingClipRegion( rDev, rRect, bTLBR, rCrossStyle );

    lclSetColorToOutDev( rDev, rBorder, pForceColor );
    lclDrawDiagLine( rDev, rRect, bTLBR, rResult.maPrim, lclGetBeg( rBorder ), lclGetPrimEnd( rBorder ), rBorder.Dotted() );
    if( rBorder.Secn() )
        lclDrawDiagLine( rDev, rRect, bTLBR, rResult.maSecn, lclGetSecnBeg( rBorder ), lclGetEnd( rBorder ), rBorder.Dotted() );
    rDev.Pop(); // colors

    if( bClip )
        rDev.Pop(); // clipping region
}

/** Draws both diagonal frame borders into the passed output device.

    The lines of each frame border is drawn interrupted, if the style of the
    other crossing frame border is double.

    @param rRect
        The reference rectangle of the diagonal frame borders.
    @param rTLBR
        The frame style of the top-left to bottom-right frame border.
    @param rBLTR
        The frame style of the bottom-left to top-right frame border.
    @param rResult
        Offsets (sub units) to modify the clipping region of the output device.
 */
void lclDrawDiagFrameBorders(
        OutputDevice& rDev, const Rectangle& rRect,
        const Style& rTLBR, const Style& rBLTR, const DiagBordersResult& rResult,
        const Color* pForceColor, bool bDiagDblClip )
{
    DBG_ASSERT( (rRect.GetWidth() > 1) && (rRect.GetHeight() > 1), "svx::frame::lclDrawDiagFrameBorders - rectangle too small" );
    if( (rRect.GetWidth() > 1) && (rRect.GetHeight() > 1) )
    {
        bool bDrawTLBR = rTLBR.Prim() != 0;
        bool bDrawBLTR = rBLTR.Prim() != 0;
        bool bFirstDrawBLTR = rTLBR.Secn() != 0;

        if( bDrawBLTR && bFirstDrawBLTR )
            lclDrawDiagFrameBorder( rDev, rRect, false, rBLTR, rResult.maBLTR, rTLBR, pForceColor, bDiagDblClip );
        if( bDrawTLBR )
            lclDrawDiagFrameBorder( rDev, rRect, true, rTLBR, rResult.maTLBR, rBLTR, pForceColor, bDiagDblClip );
        if( bDrawBLTR && !bFirstDrawBLTR )
            lclDrawDiagFrameBorder( rDev, rRect, false, rBLTR, rResult.maBLTR, rTLBR, pForceColor, bDiagDblClip );
    }
}

// ============================================================================

} // namespace

// ============================================================================
// Classes
// ============================================================================

#define SCALEVALUE( value ) lclScaleValue( value, fScale, nMaxWidth )

void Style::Clear()
{
    Set( Color(), 0, 0, 0 );
}

void Style::Set( sal_uInt16 nP, sal_uInt16 nD, sal_uInt16 nS )
{
    /*  nP  nD  nS  ->  mnPrim  mnDist  mnSecn
        --------------------------------------
        any any 0       nP      0       0
        0   any >0      nS      0       0
        >0  0   >0      nP      0       0
        >0  >0  >0      nP      nD      nS
     */
    mnPrim = nP ? nP : nS;
    mnDist = (nP && nS) ? nD : 0;
    mnSecn = (nP && nD) ? nS : 0;
}

void Style::Set( const Color& rColor, sal_uInt16 nP, sal_uInt16 nD, sal_uInt16 nS )
{
    maColor = rColor;
    Set( nP, nD, nS );
}

void Style::Set( const SvxBorderLine& rBorder, double fScale, sal_uInt16 nMaxWidth, bool bUseDots )
{
    maColor = rBorder.GetColor();

    USHORT nPrim = rBorder.GetOutWidth();
    USHORT nDist = rBorder.GetDistance();
    USHORT nSecn = rBorder.GetInWidth();

    if( !nSecn )    // no or single frame border
    {
        Set( SCALEVALUE( nPrim ), 0, 0 );
        mbDotted = bUseDots && (0 < nPrim) && (nPrim < 10);
    }
    else
    {
        Set( SCALEVALUE( nPrim ), SCALEVALUE( nDist ), SCALEVALUE( nSecn ) );
        mbDotted = false;
        // Enlarge the style if distance is too small due to rounding losses.
        sal_uInt16 nPixWidth = SCALEVALUE( nPrim + nDist + nSecn );
        if( nPixWidth > GetWidth() )
            mnDist = nPixWidth - mnPrim - mnSecn;
        // Shrink the style if it is too thick for the control.
        while( GetWidth() > nMaxWidth )
        {
            // First decrease space between lines.
            if( mnDist )
                --mnDist;
            // Still too thick? Decrease the line widths.
            if( GetWidth() > nMaxWidth )
            {
                if( mnPrim && (mnPrim == mnSecn) )
                {
                    // Both lines equal - decrease both to keep symmetry.
                    --mnPrim;
                    --mnSecn;
                }
                else
                {
                    // Decrease each line for itself
                    if( mnPrim )
                        --mnPrim;
                    if( (GetWidth() > nMaxWidth) && mnSecn )
                        --mnSecn;
                }
            }
        }
    }
}

void Style::Set( const SvxBorderLine* pBorder, double fScale, sal_uInt16 nMaxWidth, bool bUseDots )
{
    if( pBorder )
        Set( *pBorder, fScale, nMaxWidth, bUseDots );
    else
    {
        Clear();
        mbDotted = false;
    }
}

Style& Style::ScaleSelf( double fScale, sal_uInt16 nMaxWidth )
{
    Set( SCALEVALUE( mnPrim ), SCALEVALUE( mnDist ), SCALEVALUE( mnSecn ) );
    return *this;
}

Style Style::Scale( double fScale, sal_uInt16 nMaxWidth ) const
{
    return Style( *this ).ScaleSelf( fScale, nMaxWidth );
}

Style& Style::MirrorSelf()
{
    if( mnSecn )
        std::swap( mnPrim, mnSecn );
    if( meRefMode != REFMODE_CENTERED )
        meRefMode = (meRefMode == REFMODE_BEGIN) ? REFMODE_END : REFMODE_BEGIN;
    return *this;
}

Style Style::Mirror() const
{
    return Style( *this ).MirrorSelf();
}

bool operator==( const Style& rL, const Style& rR )
{
    return (rL.Prim() == rR.Prim()) && (rL.Dist() == rR.Dist()) && (rL.Secn() == rR.Secn()) &&
        (rL.GetColor() == rR.GetColor()) && (rL.GetRefMode() == rR.GetRefMode()) && (rL.Dotted() == rR.Dotted());
}

bool operator<( const Style& rL, const Style& rR )
{
    // different total widths -> rL<rR, if rL is thinner
    sal_uInt16 nLW = rL.GetWidth();
    sal_uInt16 nRW = rR.GetWidth();
    if( nLW != nRW ) return nLW < nRW;

    // one line double, the other single -> rL<rR, if rL is single
    if( (rL.Secn() == 0) != (rR.Secn() == 0) ) return rL.Secn() == 0;

    // both lines double with different distances -> rL<rR, if distance of rL greater
    if( (rL.Secn() && rR.Secn()) && (rL.Dist() != rR.Dist()) ) return rL.Dist() > rR.Dist();

    // both lines single and 1 unit thick, only one is dotted -> rL<rR, if rL is dotted
    if( (nLW == 1) && (rL.Dotted() != rR.Dotted()) ) return rL.Dotted();

    // seem to be equal
    return false;
}

#undef SCALEVALUE

// ============================================================================
// Various helper functions
// ============================================================================

double GetHorDiagAngle( long nWidth, long nHeight )
{
    return atan2( static_cast< double >( Abs( nHeight ) ), static_cast< double >( Abs( nWidth ) ) );
}

// ============================================================================

long GetTLDiagOffset( long nVerOffs, long nDiagOffs, double fAngle )
{
    return lclD2L( nVerOffs / tan( fAngle ) + nDiagOffs / sin( fAngle ) );
}

long GetBLDiagOffset( long nVerOffs, long nDiagOffs, double fAngle )
{
    return lclD2L( -nVerOffs / tan( fAngle ) + nDiagOffs / sin( fAngle ) );
}

long GetBRDiagOffset( long nVerOffs, long nDiagOffs, double fAngle )
{
    return -lclD2L( -nVerOffs / tan( fAngle ) - nDiagOffs / sin( fAngle ) );
}

long GetTRDiagOffset( long nVerOffs, long nDiagOffs, double fAngle )
{
    return -lclD2L( nVerOffs / tan( fAngle ) - nDiagOffs / sin( fAngle ) );
}

// ============================================================================

bool CheckFrameBorderConnectable( const Style& rLBorder, const Style& rRBorder,
        const Style& rTFromTL, const Style& rTFromT, const Style& rTFromTR,
        const Style& rBFromBL, const Style& rBFromB, const Style& rBFromBR )
{
    return      // returns 1 AND (2a OR 2b)
        // 1) only, if both frame borders are equal
        (rLBorder == rRBorder)
        &&
        (
            (
                // 2a) if the borders are not double, at least one of the vertical must not be double
                !rLBorder.Secn() && (!rTFromT.Secn() || !rBFromB.Secn())
            )
            ||
            (
                // 2b) if the borders are double, all other borders must not be double
                rLBorder.Secn() &&
                !rTFromTL.Secn() && !rTFromT.Secn() && !rTFromTR.Secn() &&
                !rBFromBL.Secn() && !rBFromB.Secn() && !rBFromBR.Secn()
            )
        );
}

// ============================================================================
// Drawing functions
// ============================================================================

void DrawHorFrameBorder( OutputDevice& rDev,
        const Point& rLPos, const Point& rRPos, const Style& rBorder,
        const DiagStyle& rLFromTR, const Style& rLFromT, const Style& rLFromL, const Style& rLFromB, const DiagStyle& rLFromBR,
        const DiagStyle& rRFromTL, const Style& rRFromT, const Style& rRFromR, const Style& rRFromB, const DiagStyle& rRFromBL,
        const Color* pForceColor )
{
    if( rBorder.Prim() )
    {
        BorderResult aResult;
        lclLinkHorFrameBorder( aResult, rBorder,
            rLFromTR, rLFromT, rLFromL, rLFromB, rLFromBR,
            rRFromTL, rRFromT, rRFromR, rRFromB, rRFromBL );
        lclDrawHorFrameBorder( rDev, rLPos, rRPos, rBorder, aResult, pForceColor );
    }
}

void DrawHorFrameBorder( OutputDevice& rDev,
        const Point& rLPos, const Point& rRPos, const Style& rBorder,
        const Style& rLFromT, const Style& rLFromL, const Style& rLFromB,
        const Style& rRFromT, const Style& rRFromR, const Style& rRFromB,
        const Color* pForceColor )
{
    /*  Recycle complex version of the DrawHorFrameBorder() function with empty diagonals. */
    const DiagStyle aNoStyle;
    DrawHorFrameBorder(
        rDev, rLPos, rRPos, rBorder,
        aNoStyle, rLFromT, rLFromL, rLFromB, aNoStyle,
        aNoStyle, rRFromT, rRFromR, rRFromB, aNoStyle,
        pForceColor );
}

void DrawHorFrameBorder( OutputDevice& rDev,
        const Point& rLPos, const Point& rRPos, const Style& rBorder, const Color* pForceColor )
{
    if( rBorder.Prim() )
        lclDrawHorFrameBorder( rDev, rLPos, rRPos, rBorder, BorderResult(), pForceColor );
}

// ----------------------------------------------------------------------------

void DrawVerFrameBorder( OutputDevice& rDev,
        const Point& rTPos, const Point& rBPos, const Style& rBorder,
        const DiagStyle& rTFromBL, const Style& rTFromL, const Style& rTFromT, const Style& rTFromR, const DiagStyle& rTFromBR,
        const DiagStyle& rBFromTL, const Style& rBFromL, const Style& rBFromB, const Style& rBFromR, const DiagStyle& rBFromTR,
        const Color* pForceColor )
{
    if( rBorder.Prim() )
    {
        BorderResult aResult;
        lclLinkVerFrameBorder( aResult, rBorder,
            rTFromBL, rTFromL, rTFromT, rTFromR, rTFromBR,
            rBFromTL, rBFromL, rBFromB, rBFromR, rBFromTR );
        lclDrawVerFrameBorder( rDev, rTPos, rBPos, rBorder, aResult, pForceColor );
    }
}

void DrawVerFrameBorder( OutputDevice& rDev,
        const Point& rTPos, const Point& rBPos, const Style& rBorder,
        const Style& rTFromL, const Style& rTFromT, const Style& rTFromR,
        const Style& rBFromL, const Style& rBFromB, const Style& rBFromR,
        const Color* pForceColor )
{
    /*  Recycle complex version of the DrawVerFrameBorder() function with empty diagonals. */
    const DiagStyle aNoStyle;
    DrawVerFrameBorder(
        rDev, rTPos, rBPos, rBorder,
        aNoStyle, rTFromL, rTFromT, rTFromR, aNoStyle,
        aNoStyle, rBFromL, rBFromB, rBFromR, aNoStyle,
        pForceColor );
}

void DrawVerFrameBorder( OutputDevice& rDev,
        const Point& rTPos, const Point& rBPos, const Style& rBorder, const Color* pForceColor )
{
    if( rBorder.Prim() )
        lclDrawVerFrameBorder( rDev, rTPos, rBPos, rBorder, BorderResult(), pForceColor );
}

// ----------------------------------------------------------------------------

void DrawVerFrameBorderSlanted( OutputDevice& rDev,
        const Point& rTPos, const Point& rBPos, const Style& rBorder, const Color* pForceColor )
{
    DBG_ASSERT( rTPos.Y() < rBPos.Y(), "svx::frame::DrawVerFrameBorderSlanted - wrong order of line ends" );
    if( rBorder.Prim() && (rTPos.Y() < rBPos.Y()) )
    {
        if( rTPos.X() == rBPos.X() )
        {
            DrawVerFrameBorder( rDev, rTPos, rBPos, rBorder, pForceColor );
        }
        else
        {
            const LineEndResult aRes;

            Style aScaled( rBorder );
            aScaled.ScaleSelf( 1.0 / cos( GetVerDiagAngle( rTPos, rBPos ) ) );

            lclSetColorToOutDev( rDev, aScaled, pForceColor );
            lclDrawVerLine( rDev, rTPos, aRes, rBPos, aRes,
                lclGetBeg( aScaled ), lclGetPrimEnd( aScaled ), aScaled.Dotted() );
            if( aScaled.Secn() )
                lclDrawVerLine( rDev, rTPos, aRes, rBPos, aRes,
                    lclGetSecnBeg( aScaled ), lclGetEnd( aScaled ), aScaled.Dotted() );
            rDev.Pop(); // colors
        }
    }
}

// ============================================================================

void DrawDiagFrameBorders(
        OutputDevice& rDev, const Rectangle& rRect, const Style& rTLBR, const Style& rBLTR,
        const Style& rTLFromB, const Style& rTLFromR, const Style& rBRFromT, const Style& rBRFromL,
        const Style& rBLFromT, const Style& rBLFromR, const Style& rTRFromB, const Style& rTRFromL,
        const Color* pForceColor, bool bDiagDblClip )
{
    if( rTLBR.Prim() || rBLTR.Prim() )
    {
        DiagBordersResult aResult;
        lclLinkDiagFrameBorders( aResult, rTLBR, rBLTR,
            rTLFromB, rTLFromR, rBRFromT, rBRFromL, rBLFromT, rBLFromR, rTRFromB, rTRFromL );
        lclDrawDiagFrameBorders( rDev, rRect, rTLBR, rBLTR, aResult, pForceColor, bDiagDblClip );
    }
}

// ============================================================================

} // namespace frame
} // namespace svx

