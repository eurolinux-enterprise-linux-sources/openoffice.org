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

#ifndef ADC_UNOIDL_HXX
#define ADC_UNOIDL_HXX



// USED SERVICES
	// BASE CLASSES
#include <parser/parser.hxx>
#include <parser/parserinfo.hxx>
	// COMPONENTS
	// PARAMETERS


namespace ary
{
	class Repository;
}
namespace autodoc
{
    class FileCollector_Ifc;
}

namespace autodoc
{


class IdlParser : public ::CodeParser,
                  public ::ParserInfo
{
  public:
						IdlParser(
							ary::Repository &	    io_rRepository );

	virtual void		Run(
							const autodoc::FileCollector_Ifc &
												i_rFiles );

  private:
	// DATA
	ary::Repository *	pRepository;
};



// IMPLEMENTATION


}   // namespace autodoc


#endif

