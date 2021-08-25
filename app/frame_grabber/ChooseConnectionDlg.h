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

// ChooseConnectionDlg.h : interface of the CChooseConnectionDlg class
//
/////////////////////////////////////////////////////////////////////////////
#pragma once



class CChooseConnectionDlg : public CDialogImpl<CChooseConnectionDlg>,
    public CWinDataExchange<CChooseConnectionDlg>
{
public:
    CChooseConnectionDlg();
	enum { IDD = IDD_DLG_CHOOSE_CONNECTION };

	BEGIN_MSG_MAP(CChooseConnectionDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_RADIO_VIA_SERIAL_PORT, OnConnectionTypeClicked)
        COMMAND_ID_HANDLER(IDC_RADIO_VIA_NETWORK, OnConnectionTypeClicked)
        COMMAND_ID_HANDLER(IDC_BUTTON_AUTO_DISCOVERY, OnAutoDiscoveryClicked)
    END_MSG_MAP()


	BEGIN_DDX_MAP(CChooseConnectionDlg)
        DDX_CONTROL_HANDLE(IDC_COMBO_SERIAL_PORT, m_combo_SerialPort)
        DDX_CONTROL_HANDLE(IDC_COMBO_SERIAL_BAUD, m_combo_SerialBaudSel)
        DDX_CONTROL_HANDLE(IDC_IPADDRESS, m_IpAdress)
        DDX_CONTROL_HANDLE(IDC_IP_PORT, m_edit_IpPort)
        DDX_CONTROL_HANDLE(IDC_COMBO_NETWORK_PROTOCOL, m_combo_NetProtocol)
        DDX_CONTROL_HANDLE(IDC_RADIO_VIA_SERIAL_PORT, m_radio_ViaSerialPort)
        DDX_CONTROL_HANDLE(IDC_RADIO_VIA_NETWORK, m_radio_ViaNetwork)
        DDX_CONTROL_HANDLE(IDC_BUTTON_AUTO_DISCOVERY, m_btn_auto_discovery)
	END_DDX_MAP();
	
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnConnectionTypeClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnAutoDiscoveryClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


    typedef union _connection_type_info {
        struct _serial
        {
            bool usingNetwork;
            char serialPath[64];
            int baud;
        } serial;
        struct _network
        {
            bool usingNetwork;
            char ip[32];
            char protocol[16];
            int port;
        } network;

    } connection_type_info;

    connection_type_info getSelectedConnectionTypeInfo()
    {
        return _selectedConnectType;
    }

protected:
    CComboBox m_combo_SerialPort;
    CComboBox m_combo_SerialBaudSel;
    CComboBox m_combo_NetProtocol;
    CIPAddressCtrl m_IpAdress;
    CEdit m_edit_IpPort;
    CButton m_radio_ViaSerialPort;
    CButton m_radio_ViaNetwork;
    CButton m_btn_auto_discovery;

    int     _selectedID;
    int     _selectedBaudRate;
    bool    _usingNetwork;
    connection_type_info _selectedConnectType;
    std::vector<std::string> _protocolList;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// VisualFC AppWizard will insert additional declarations immediately before the previous line.
