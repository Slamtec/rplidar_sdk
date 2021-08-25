/*
 *  SLAMTEC LIDAR
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

// IpConfigDlg.h : interface of the CIpConfigDlg class
//
/////////////////////////////////////////////////////////////////////////////
#pragma once



class CIpConfigDlg : public CDialogImpl<CIpConfigDlg>,
    public CWinDataExchange<CIpConfigDlg>
{
public:
    CIpConfigDlg();
	enum { IDD = IDD_IP_CONFIG };

    BEGIN_MSG_MAP(CIpConfigDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_DHCP_CLIENT, OnDhcpClient);
    END_MSG_MAP()


	BEGIN_DDX_MAP(CIpConfigDlg)
        DDX_CONTROL_HANDLE(IDC_CONFIG_IP, m_ip)
        DDX_CONTROL_HANDLE(IDC_CONFIG_MASK, m_mask)
        DDX_CONTROL_HANDLE(IDC_CONFIG_GATEWAY, m_gw)
	END_DDX_MAP();
	
	static unsigned char ip_[32];
	static unsigned char mask_[32];
	static unsigned char gw_[32];

    static void initIpconfig(const char* ip, const char* mask, const char* gw)
    {
        if (ip)
			memcpy(ip_, ip, sizeof(ip_));	
        if (mask)
			memcpy(mask_, mask,  sizeof(mask_));		
        if (gw)
			memcpy(gw_, gw,  sizeof(gw_));	
    }

    bool getIpConfResult()
    {
        return is_successful_;
    }

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDhcpClient(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
protected:
    CIPAddressCtrl m_ip;
    CIPAddressCtrl m_mask;
    CIPAddressCtrl m_gw;
    bool is_successful_;

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// VisualFC AppWizard will insert additional declarations immediately before the previous line.
