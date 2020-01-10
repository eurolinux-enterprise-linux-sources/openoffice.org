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

#pragma once

#include <rtl/string.hxx>
#include <saldisp.hxx>
#include <salgdi.h>

/** handles graphics drawings requests and performs the needed drawing operations */
class KDESalGraphics : public X11SalGraphics
{
	public:
		KDESalGraphics() {}
		virtual ~KDESalGraphics() {}
		
		/**
			What widgets can be drawn the native way.
			@param type Type of the widget.
			@param part Specification of the widget's part if it consists of more than one.
			@return true if the platform supports native drawing of the widget type defined by part.
		*/
		virtual BOOL IsNativeControlSupported( ControlType type, ControlPart part );
		
		/** Test whether the position is in the native widget.
			If the return value is TRUE, bIsInside contains information whether
			aPos was or was not inside the native widget specified by the
			type/part combination.
		*/
		virtual BOOL hitTestNativeControl( ControlType type, ControlPart part,
										const Region& rControlRegion, const Point& aPos,
										SalControlHandle& rControlHandle, BOOL& rIsInside );
		/** Draw the requested control described by part/nControlState.
		
			@param rControlRegion
			The bounding region of the complete control in VCL frame coordinates.
			
			@param aValue
			An optional value (tristate/numerical/string).
			
			@param rControlHandle
			Carries platform dependent data and is maintained by the SalFrame implementation.
			
			@param aCaption
			A caption or title string (like button text etc.)
		*/
		virtual BOOL drawNativeControl( ControlType type, ControlPart part,
										const Region& rControlRegion, ControlState nControlState,
										const ImplControlValue& aValue, SalControlHandle& rControlHandle,
										const rtl::OUString& aCaption );
										
		/** Draw text on the widget.
			OPTIONAL. Draws the requested text for the control described by part/nControlState.
			Used if text is not drawn by DrawNativeControl().
		
			@param rControlRegion The bounding region of the complete control in VCL frame coordinates.
			@param aValue An optional value (tristate/numerical/string)
			@param rControlHandle Carries platform dependent data and is maintained by the SalFrame implementation.
			@param aCaption	A caption or title string (like button text etc.)
		*/
		virtual BOOL drawNativeControlText( ControlType, ControlPart,
											const Region&, ControlState,
											const ImplControlValue&, SalControlHandle&,
											const rtl::OUString& ) { return false; }
		/** Check if the bounding regions match.
			
			If the return value is TRUE, rNativeBoundingRegion
			contains the true bounding region covered by the control
			including any adornment, while rNativeContentRegion contains the area
			within the control that can be safely drawn into without drawing over
			the borders of the control.

			@param rControlRegion
			The bounding region of the control in VCL frame coordinates.
			
			@param aValue
			An optional value (tristate/numerical/string)
			
			@param rControlHandle
			Carries platform dependent data and is maintained by the SalFrame implementation.
			
			@param aCaption
			A caption or title string (like button text etc.)
		*/
		virtual BOOL getNativeControlRegion( ControlType type, ControlPart part,
											const Region& rControlRegion, ControlState nControlState,
											const ImplControlValue& aValue, SalControlHandle& rControlHandle,
											const rtl::OUString& aCaption,
											Region &rNativeBoundingRegion, Region &rNativeContentRegion );
};