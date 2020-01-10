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

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif
#include "connpooloptions.hxx"
#ifndef _OFFAPP_CONNPOOLOPTIONS_HRC_
#include "connpooloptions.hrc"
#endif
#include <svtools/editbrowsebox.hxx>
#include <vcl/field.hxx>
#include "connpoolsettings.hxx"
#include <svtools/eitem.hxx>

#include <svx/dialogs.hrc>
#include "helpid.hrc"
#include <svx/dialmgr.hxx>

//........................................................................
namespace offapp
{
//........................................................................

	//====================================================================
	//= DriverListControl
	//====================================================================
	typedef ::svt::EditBrowseBox DriverListControl_Base;
	class DriverListControl : public DriverListControl_Base
	{
		using Window::Update;
	protected:
		DriverPoolingSettings					m_aSavedSettings;
		DriverPoolingSettings					m_aSettings;
		DriverPoolingSettings::const_iterator	m_aSeekRow;

		String									m_sYes;
		String									m_sNo;

		Link									m_aRowChangeHandler;

	public:
		DriverListControl( Window* _pParent, const ResId& _rId);

		virtual	void Init();
				void Update(const DriverPoolingSettings& _rSettings);
		virtual String GetCellText( long nRow, USHORT nColId ) const;

		// the handler will be called with a DriverPoolingSettings::const_iterator as parameter,
		// or NULL if no valid current row exists
		void SetRowChangeHandler(const Link& _rHdl) { m_aRowChangeHandler = _rHdl; }
		Link GetRowChangeHandler() const { return m_aRowChangeHandler; }

		const DriverPooling* getCurrentRow() const;
		DriverPooling* getCurrentRow();
		void									updateCurrentRow();

		const DriverPoolingSettings& getSettings() const { return m_aSettings; }

		void		saveValue()				{ m_aSavedSettings = m_aSettings; }
		sal_Bool	isModified() const;

	protected:
		virtual void InitController( ::svt::CellControllerRef& rController, long nRow, USHORT nCol );
		virtual ::svt::CellController* GetController( long nRow, USHORT nCol );

		virtual void PaintCell( OutputDevice& rDev, const Rectangle& rRect, USHORT nColId ) const;

		virtual BOOL SeekRow( long nRow );
		virtual BOOL SaveModified();

		virtual sal_Bool IsTabAllowed(sal_Bool _bForward) const;

		virtual void StateChanged( StateChangedType nStateChange );

		virtual void CursorMoved();

	protected:
		virtual sal_uInt32 GetTotalCellWidth(long nRow, USHORT nColId);


	private:
		String implGetCellText(DriverPoolingSettings::const_iterator _rPos, sal_uInt16 _nColId) const;
	};

	//--------------------------------------------------------------------
	DriverListControl::DriverListControl( Window* _pParent, const ResId& _rId)
//		:DriverListControl_Base(_pParent, _rId, DBBF_NOROWPICTURE, BROWSER_AUTO_VSCROLL | BROWSER_AUTO_HSCROLL | BROWSER_COLUMNSELECTION | BROWSER_HLINESFULL | BROWSER_VLINESFULL | BROWSER_HIDESELECT | BROWSER_CURSOR_WO_FOCUS)
		:DriverListControl_Base(_pParent, _rId, EBBF_NOROWPICTURE, BROWSER_AUTO_VSCROLL | BROWSER_AUTO_HSCROLL | BROWSER_HIDECURSOR | BROWSER_AUTOSIZE_LASTCOL)
		,m_aSeekRow(m_aSettings.end())
		,m_sYes(ResId(STR_YES,*_rId.GetResMgr()))
		,m_sNo(ResId(STR_NO,*_rId.GetResMgr()))
	{
		SetStyle((GetStyle() & ~WB_HSCROLL) | WB_AUTOHSCROLL);

		SetUniqueId(UID_OFA_CONNPOOL_DRIVERLIST_BACK);
		GetDataWindow().SetHelpId(HID_OFA_CONNPOOL_DRIVERLIST);
	}

	//--------------------------------------------------------------------
    sal_Bool DriverListControl::IsTabAllowed(sal_Bool /*_bForward*/) const
	{
		// no travinling within the fields via RETURN and TAB
		return sal_False;
	}

	//--------------------------------------------------------------------
	sal_Bool DriverListControl::isModified() const
	{
		if (m_aSettings.size() != m_aSavedSettings.size())
			return sal_True;

		DriverPoolingSettings::const_iterator aCurrent = m_aSettings.begin();
		DriverPoolingSettings::const_iterator aCurrentEnd = m_aSettings.end();
		DriverPoolingSettings::const_iterator aSaved = m_aSavedSettings.begin();
		for (;aCurrent != aCurrentEnd; ++aCurrent, ++aSaved)
		{
			if (*aCurrent != *aSaved)
				return sal_True;
		}

		return sal_False;
	}

	//--------------------------------------------------------------------
	void DriverListControl::Init()
	{
		DriverListControl_Base::Init();

		Size aColWidth = LogicToPixel(Size(160, 0), MAP_APPFONT);
		InsertDataColumn(1, String(SVX_RES(STR_DRIVER_NAME)), aColWidth.Width());
		aColWidth = LogicToPixel(Size(30, 0), MAP_APPFONT);
		InsertDataColumn(2, String(SVX_RES(STR_POOLED_FLAG)), aColWidth.Width());
		aColWidth = LogicToPixel(Size(60, 0), MAP_APPFONT);
		InsertDataColumn(3, String(SVX_RES(STR_POOL_TIMEOUT)), aColWidth.Width());
			// Attention: the resource of the string is local to the resource of the enclosing dialog!
	}

	//--------------------------------------------------------------------
	void DriverListControl::CursorMoved()
	{
		DriverListControl_Base::CursorMoved();

		// call the row change handler
		if ( m_aRowChangeHandler.IsSet() )
        {
            if ( GetCurRow() >= 0 )
            {   // == -1 may happen in case the browse box has just been cleared
    			m_aRowChangeHandler.Call( getCurrentRow() );
            }
        }
	}

	//--------------------------------------------------------------------
	const DriverPooling* DriverListControl::getCurrentRow() const
	{
		OSL_ENSURE( ( GetCurRow() < m_aSettings.size() ) && ( GetCurRow() >= 0 ),
            "DriverListControl::getCurrentRow: invalid current row!");

        if ( ( GetCurRow() >= 0 ) && ( GetCurRow() < m_aSettings.size() ) )
			return &(*(m_aSettings.begin() + GetCurRow()));

		return NULL;
	}

	//--------------------------------------------------------------------
	DriverPooling* DriverListControl::getCurrentRow()
	{
		OSL_ENSURE( ( GetCurRow() < m_aSettings.size() ) && ( GetCurRow() >= 0 ),
		    "DriverListControl::getCurrentRow: invalid current row!");

        if ( ( GetCurRow() >= 0 ) && ( GetCurRow() < m_aSettings.size() ) )
			return &(*(m_aSettings.begin() + GetCurRow()));

		return NULL;
	}

	//--------------------------------------------------------------------
	void DriverListControl::updateCurrentRow()
	{
		Window::Invalidate( GetRowRectPixel( GetCurRow() ), INVALIDATE_UPDATE );
	}

	//--------------------------------------------------------------------
	void DriverListControl::Update(const DriverPoolingSettings& _rSettings)
	{
		m_aSettings = _rSettings;

		SetUpdateMode(sal_False);
		RowRemoved(0, GetRowCount());
		RowInserted(0, m_aSettings.size());
		SetUpdateMode(sal_True);

		ActivateCell(1, 0);
	}

	//--------------------------------------------------------------------
	sal_uInt32 DriverListControl::GetTotalCellWidth(long nRow, USHORT nColId)
	{
		return GetDataWindow().GetTextWidth(GetCellText(nRow, nColId));
	}

	//--------------------------------------------------------------------
	String DriverListControl::implGetCellText(DriverPoolingSettings::const_iterator _rPos, sal_uInt16 _nColId) const
	{
		OSL_ENSURE(_rPos < m_aSettings.end(), "DriverListControl::implGetCellText: invalid position!");

		String sReturn;
		switch (_nColId)
		{
			case 1:
				sReturn = _rPos->sName;
				break;
			case 2:
				sReturn = _rPos->bEnabled ? m_sYes : m_sNo;
				break;
			case 3:
				if (_rPos->bEnabled)
					sReturn = String::CreateFromInt32(_rPos->nTimeoutSeconds);
				break;
			default:
				OSL_ENSURE(sal_False, "DriverListControl::implGetCellText: invalid column id!");
		}
		return sReturn;
	}

	//--------------------------------------------------------------------
	void DriverListControl::StateChanged( StateChangedType nStateChange )
	{
		if (STATE_CHANGE_ENABLE == nStateChange)
			Window::Invalidate(INVALIDATE_UPDATE);
		DriverListControl_Base::StateChanged( nStateChange );
	}

	//--------------------------------------------------------------------
	String DriverListControl::GetCellText( long nRow, USHORT nColId ) const
	{
		String sReturn;
		if (nRow > m_aSettings.size())
		{
			OSL_ENSURE(sal_False, "DriverListControl::GetCellText: don't ask me for such rows!");
		}
		else
		{
			sReturn = implGetCellText(m_aSettings.begin() + nRow, nColId);
		}
		return sReturn;
	}

	//--------------------------------------------------------------------
	void DriverListControl::InitController( ::svt::CellControllerRef& rController, long nRow, USHORT nCol )
	{
		rController->GetWindow().SetText(GetCellText(nRow, nCol));
	}

	//--------------------------------------------------------------------
    ::svt::CellController* DriverListControl::GetController( long /*nRow*/, USHORT /*nCol*/ )
	{
		return NULL;
	}

	//--------------------------------------------------------------------
	BOOL DriverListControl::SaveModified()
	{
		return TRUE;
	}

	//--------------------------------------------------------------------
	BOOL DriverListControl::SeekRow( long _nRow )
	{
		DriverListControl_Base::SeekRow(_nRow);

		if (_nRow < m_aSettings.size())
			m_aSeekRow = m_aSettings.begin() + _nRow;
		else
			m_aSeekRow = m_aSettings.end();

		return m_aSeekRow != m_aSettings.end();
	}

	//--------------------------------------------------------------------
	void DriverListControl::PaintCell( OutputDevice& rDev, const Rectangle& rRect, USHORT nColId ) const
	{
		OSL_ENSURE(m_aSeekRow != m_aSettings.end(), "DriverListControl::PaintCell: invalid row!");

		if (m_aSeekRow != m_aSettings.end())
		{
			rDev.SetClipRegion(rRect);

			sal_uInt16 nStyle = TEXT_DRAW_CLIP;
			if (!IsEnabled())
				nStyle |= TEXT_DRAW_DISABLE;
			switch (nColId)
			{
				case 1: nStyle |= TEXT_DRAW_LEFT; break;
				case 2:
				case 3: nStyle |= TEXT_DRAW_CENTER; break;
			}

			rDev.DrawText(rRect, implGetCellText(m_aSeekRow, nColId), nStyle);

			rDev.SetClipRegion();
		}
	}

	//====================================================================
	//= ConnectionPoolOptionsPage
	//====================================================================
	//--------------------------------------------------------------------
	ConnectionPoolOptionsPage::ConnectionPoolOptionsPage(Window* _pParent, const SfxItemSet& _rAttrSet)
		:SfxTabPage(_pParent, SVX_RES(RID_OFAPAGE_CONNPOOLOPTIONS ), _rAttrSet)
		,m_aFrame				(this,				SVX_RES(FL_POOLING))
		,m_aEnablePooling		(this,		SVX_RES(CB_POOL_CONNS))
		,m_aDriversLabel		(this,		SVX_RES(FT_DRIVERS))
		,m_pDriverList(new DriverListControl(this, SVX_RES(CTRL_DRIVER_LIST)))
		,m_aDriverLabel			(this,		SVX_RES(FT_DRIVERLABEL))
		,m_aDriver				(this,		SVX_RES(FT_DRIVER))
		,m_aDriverPoolingEnabled(this,		SVX_RES(CB_DRIVERPOOLING))
		,m_aTimeoutLabel		(this,		SVX_RES(FT_TIMEOUT))
		,m_aTimeout				(this,		SVX_RES(NF_TIMEOUT))
	{
		m_pDriverList->Init();
		m_pDriverList->Show();

		FreeResource();

		m_aEnablePooling.SetClickHdl( LINK(this, ConnectionPoolOptionsPage, OnEnabledDisabled) );
		m_aDriverPoolingEnabled.SetClickHdl( LINK(this, ConnectionPoolOptionsPage, OnEnabledDisabled) );

		m_pDriverList->SetRowChangeHandler( LINK(this, ConnectionPoolOptionsPage, OnDriverRowChanged) );
	}

	//--------------------------------------------------------------------
	SfxTabPage* ConnectionPoolOptionsPage::Create(Window* _pParent, const SfxItemSet& _rAttrSet)
	{
		return new ConnectionPoolOptionsPage(_pParent, _rAttrSet);
	}

	//--------------------------------------------------------------------
	ConnectionPoolOptionsPage::~ConnectionPoolOptionsPage()
	{
		delete m_pDriverList;
	}

	//--------------------------------------------------------------------
	void ConnectionPoolOptionsPage::implInitControls(const SfxItemSet& _rSet, sal_Bool /*_bFromReset*/)
	{
		// the enabled flag
		SFX_ITEMSET_GET( _rSet, pEnabled, SfxBoolItem, SID_SB_POOLING_ENABLED, sal_True );
		OSL_ENSURE(pEnabled, "ConnectionPoolOptionsPage::implInitControls: missing the Enabled item!");
		m_aEnablePooling.Check(pEnabled ? pEnabled->GetValue() : sal_True);

		m_aEnablePooling.SaveValue();

		// the settings for the single drivers
		SFX_ITEMSET_GET( _rSet, pDriverSettings, DriverPoolingSettingsItem, SID_SB_DRIVER_TIMEOUTS, sal_True );
		if (pDriverSettings)
			m_pDriverList->Update(pDriverSettings->getSettings());
		else
		{
			OSL_ENSURE(sal_False, "ConnectionPoolOptionsPage::implInitControls: missing the DriverTimeouts item!");
			m_pDriverList->Update(DriverPoolingSettings());
		}
		m_pDriverList->saveValue();

		// reflect the new settings
		OnEnabledDisabled(&m_aEnablePooling);
	}

	//--------------------------------------------------------------------
	long ConnectionPoolOptionsPage::Notify( NotifyEvent& _rNEvt )
	{
		if (EVENT_LOSEFOCUS == _rNEvt.GetType())
			if (m_aTimeout.IsWindowOrChild(_rNEvt.GetWindow()))
				commitTimeoutField();

		return SfxTabPage::Notify(_rNEvt);
	}

	//--------------------------------------------------------------------
	BOOL ConnectionPoolOptionsPage::FillItemSet(SfxItemSet& _rSet)
	{
		commitTimeoutField();

		sal_Bool bModified = sal_False;
		// the enabled flag
		if (m_aEnablePooling.GetSavedValue() != m_aEnablePooling.IsChecked())
		{
			_rSet.Put(SfxBoolItem(SID_SB_POOLING_ENABLED, m_aEnablePooling.IsChecked()), SID_SB_POOLING_ENABLED);
			bModified = sal_True;
		}

		// the settings for the single drivers
		if (m_pDriverList->isModified())
		{
			_rSet.Put(DriverPoolingSettingsItem(SID_SB_DRIVER_TIMEOUTS, m_pDriverList->getSettings()), SID_SB_DRIVER_TIMEOUTS);
			bModified = sal_True;
		}

		return bModified;
	}

	//--------------------------------------------------------------------
	void ConnectionPoolOptionsPage::ActivatePage( const SfxItemSet& _rSet)
	{
		SfxTabPage::ActivatePage(_rSet);
		implInitControls(_rSet, sal_False);
	}

	//--------------------------------------------------------------------
	void ConnectionPoolOptionsPage::Reset(const SfxItemSet& _rSet)
	{
		implInitControls(_rSet, sal_True);
	}

	//--------------------------------------------------------------------
	IMPL_LINK( ConnectionPoolOptionsPage, OnDriverRowChanged, const void*, _pRowIterator )
	{
		sal_Bool bValidRow = (NULL != _pRowIterator);
		m_aDriverPoolingEnabled.Enable(bValidRow && m_aEnablePooling.IsChecked());
		m_aTimeoutLabel.Enable(bValidRow);
		m_aTimeout.Enable(bValidRow);

		if (!bValidRow)
		{	// positioned on an invalid row
			m_aDriver.SetText(String());
		}
		else
		{
			const DriverPooling *pDriverPos = static_cast<const DriverPooling*>(_pRowIterator);

			m_aDriver.SetText(pDriverPos->sName);
			m_aDriverPoolingEnabled.Check(pDriverPos->bEnabled);
			m_aTimeout.SetText(String::CreateFromInt32(pDriverPos->nTimeoutSeconds));

			OnEnabledDisabled(&m_aDriverPoolingEnabled);
		}

		return 0L;
	}

	//--------------------------------------------------------------------
	void ConnectionPoolOptionsPage::commitTimeoutField()
	{
		if (DriverPooling* pCurrentDriver = m_pDriverList->getCurrentRow())
		{
			pCurrentDriver->nTimeoutSeconds = static_cast<long>(m_aTimeout.GetValue());
			m_pDriverList->updateCurrentRow();
		}
	}

	//--------------------------------------------------------------------
	IMPL_LINK( ConnectionPoolOptionsPage, OnEnabledDisabled, const CheckBox*, _pCheckBox )
	{
		sal_Bool bGloballyEnabled = m_aEnablePooling.IsChecked();
		sal_Bool bLocalDriverChanged = &m_aDriverPoolingEnabled == _pCheckBox;

		if (&m_aEnablePooling == _pCheckBox)
		{
			m_aDriversLabel.Enable(bGloballyEnabled);
			m_pDriverList->Enable(bGloballyEnabled);
			m_aDriverLabel.Enable(bGloballyEnabled);
			m_aDriver.Enable(bGloballyEnabled);
			m_aDriverPoolingEnabled.Enable(bGloballyEnabled);
		}
		else
			OSL_ENSURE(bLocalDriverChanged, "ConnectionPoolOptionsPage::OnEnabledDisabled: where did this come from?");

		m_aTimeoutLabel.Enable(bGloballyEnabled && m_aDriverPoolingEnabled.IsChecked());
		m_aTimeout.Enable(bGloballyEnabled && m_aDriverPoolingEnabled.IsChecked());

		if (bLocalDriverChanged)
		{
			// update the list
			m_pDriverList->getCurrentRow()->bEnabled = m_aDriverPoolingEnabled.IsChecked();
			m_pDriverList->updateCurrentRow();
		}

		return 0L;
	}

//........................................................................
}	// namespace offapp
//........................................................................


