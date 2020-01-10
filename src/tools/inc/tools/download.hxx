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

#ifndef _DOWNLOAD_HXX
#define _DOWNLOAD_HXX

// Forward declarations
class String;
class Link;

#define DOWNLOAD_SUCCESS	0
#define DOWNLOAD_CONNECT	1
#define DOWNLOAD_LOCATION	2
#define DOWNLOAD_ABORT		3
#define DOWNLOAD_FILEACCESS	4
#define DOWNLOAD_INSTALL	5
#define DOWNLOAD_ERROR		6

class Downloader
/* ***************************************************************************
Purpose: Abstract base class for a file downloader
*************************************************************************** */
{
public:
	Downloader() {};

	virtual void Download(const String &rDestLocation,
						  const String &rSourceLocation,
						  const Link &rFinishedLink) = 0;
};

#endif
