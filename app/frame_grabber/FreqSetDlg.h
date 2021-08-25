/*
 *  SLAMTEC LIDAR
 *  Win32 Demo Application
 *
 *  Copyright (c) 2016 - 2018 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// FreqSetDlg.h : interface of the CFreqSetDlg class
//
/////////////////////////////////////////////////////////////////////////////
#pragma once


class CFreqSetDlg : public CDialogImpl<CFreqSetDlg>,
    public CWinDataExchange<CFreqSetDlg>
{
public:
	CFreqSetDlg(int lidarType);
	enum { IDD = IDD_DLG_SETFREQ };


	BEGIN_MSG_MAP(CFreqSetDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        MSG_WM_HSCROLL(OnHScroll)
    END_MSG_MAP()


	BEGIN_DDX_MAP(CFreqSetDlg)
        DDX_CONTROL_HANDLE(IDC_SLIDER_FREQ, m_sld_freq)
        DDX_CONTROL_HANDLE(IDC_EDIT_FREQ, m_edt_freq)
		DDX_CONTROL_HANDLE(IDC_STATIC_MINFREQ, m_min_freq)
        DDX_CONTROL_HANDLE(IDC_STATIC_MAXFREQ, m_max_freq)
	END_DDX_MAP();
	
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    void OnHScroll(UINT nSBCode, LPARAM /*lParam*/, CScrollBar pScrollBar);
    
    void setMotorinfo(const LidarMotorInfo& motorInfo);
    CTrackBarCtrl   m_sld_freq;
    CEdit           m_edt_freq;
	CEdit           m_min_freq;
    CEdit           m_max_freq;
protected:
	int lidarType_;
    sl_u16 maxSpeed_;
    sl_u16 minSpeed_;
    sl_u16 desiredSpeed_;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// VisualFC AppWizard will insert additional declarations immediately before the previous line.
