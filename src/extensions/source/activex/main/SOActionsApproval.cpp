// SOActionsApproval.cpp : Implementation of CHelpApp and DLL registration.

#include "stdafx2.h"

#include "so_activex.h"
#include "SOActionsApproval.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP SOActionsApproval::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISOActionsApproval,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
		if (InlineIsEqualGUID(*arr[i],riid))
#else
		if (::ATL::InlineIsEqualGUID(*arr[i],riid))
#endif
			return S_OK;
	}
	return S_FALSE;
}

