#*************************************************************************
#
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# Copyright 2000, 2010 Oracle and/or its affiliates.
#
# OpenOffice.org - a multi-platform office productivity suite
#
# This file is part of OpenOffice.org.
#
# OpenOffice.org is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# only, as published by the Free Software Foundation.
#
# OpenOffice.org is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License version 3 for more details
# (a copy is included in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU Lesser General Public License
# version 3 along with OpenOffice.org.  If not, see
# <http://www.openoffice.org/license.html>
# for a copy of the LGPLv3 License.
#
#*************************************************************************

PRJ=..$/..
PRJINC=$(PRJ)$/source$/inc
PRJNAME=forms
TARGET=fm_cppugen

# --- Settings ----------------------------------

.INCLUDE : settings.mk
.INCLUDE: $(PRJ)$/makefile.pmk

# --- Types -------------------------------------



# --- Types -------------------------------------

UNOTYPES+=	\
		com.sun.star.form.binding.XBindableValue \
		com.sun.star.form.binding.XValueBinding \
		com.sun.star.form.binding.XListEntrySink \
		com.sun.star.form.binding.XListEntrySource \
		com.sun.star.form.binding.XListEntryListener \
		com.sun.star.form.validation.XValidator \
		com.sun.star.form.validation.XValidatable \
		com.sun.star.form.validation.XValidityConstraintListener \
		com.sun.star.form.validation.XValidatableFormComponent \
		com.sun.star.form.submission.XSubmissionSupplier \
        com.sun.star.xforms.XModel \
        com.sun.star.xforms.XFormsSupplier \
        com.sun.star.xforms.XSubmission \
        com.sun.star.xsd.WhiteSpaceTreatment \
        com.sun.star.xsd.XDataType \

# --- Targets ----------------------------------

.INCLUDE : target.mk

