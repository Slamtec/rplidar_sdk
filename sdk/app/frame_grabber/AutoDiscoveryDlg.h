/*
 *  RPLIDAR
 *  Win32 Demo Application
 *
 *  Copyright (c) 2016 - 2020 Shanghai Slamtec Co., Ltd.
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

//
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include "hal/thread.h"


class CAutoDiscoveryDlg : public CDialogImpl<CAutoDiscoveryDlg>,
    public CWinDataExchange<CAutoDiscoveryDlg>
{
public:
    CAutoDiscoveryDlg();
    CAutoDiscoveryDlg(CString ip, CString port);
    ~CAutoDiscoveryDlg();
	enum { IDD = IDD_DLG_AUTO_DISCOVERY };

	BEGIN_MSG_MAP(CAutoDiscoveryDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()


	BEGIN_DDX_MAP(CAutoDiscoveryDlg)
        DDX_CONTROL_HANDLE(IDC_IPADDRESS_SEL, m_IpAdress)
        DDX_CONTROL_HANDLE(IDC_EDIT_IP_PORT, m_edit_IpPort)
        DDX_CONTROL_HANDLE(IDC_LIST_IP, m_list_ip)

	END_DDX_MAP();
	
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    


   
protected:
    CIPAddressCtrl m_IpAdress;
    CEdit m_edit_IpPort;
    CListBox m_list_ip;
    u_result _testingThrd(void);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// VisualFC AppWizard will insert additional declarations immediately before the previous line.
