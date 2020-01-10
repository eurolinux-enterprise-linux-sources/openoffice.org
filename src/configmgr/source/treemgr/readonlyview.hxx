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

#ifndef CONFIGMGR_READONLYVIEW_HXX_
#define CONFIGMGR_READONLYVIEW_HXX_

#include "viewstrategy.hxx"


namespace configmgr
{
	namespace view
	{
//-----------------------------------------------------------------------------
// View behavior for direct read-only access
//-----------------------------------------------------------------------------

		class ReadOnlyViewStrategy : public ViewStrategy
		{
		public:
			explicit 
            ReadOnlyViewStrategy() {}

		protected:
            // change handling -required
			virtual bool doHasChanges(Node const& _aNode) const;
			virtual void doMarkChanged(Node const& _aNode);

            // common attributes 
            virtual node::Attributes doAdjustAttributes(node::Attributes const& _aAttributes) const;

            // group member access
            virtual configuration::ValueMemberNode doGetValueMember(GroupNode const& _aNode, rtl::OUString const& _aName, bool _bForUpdate) const;

            // set element access
            virtual void doInsertElement(SetNode const& _aNode, rtl::OUString const& aName, configuration::SetEntry const& aNewEntry);
			virtual void doRemoveElement(SetNode const& _aNode, rtl::OUString const& aName);

            virtual NodeFactory& doGetNodeFactory();
        private:
            void failReadOnly() const;
		};
//-----------------------------------------------------------------------------
    }
}

#endif // CONFIGMGR_READONLYVIEW_HXX_
