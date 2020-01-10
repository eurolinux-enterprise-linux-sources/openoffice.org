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

#ifndef SD_FRAMEWORK_TASK_PANE_MODULE_HXX
#define SD_FRAMEWORK_TASK_PANE_MODULE_HXX

#include "ResourceManager.hxx"

#include <rtl/ref.hxx>

namespace sd { namespace framework {

class ReadOnlyModeObserver;

/** This module is responsible for showing the task pane.
*/
class TaskPaneModule
{
public:
    static void Initialize (
        const ::com::sun::star::uno::Reference<com::sun::star::frame::XController>& rxController);
};

} } // end of namespace sd::framework

#endif
