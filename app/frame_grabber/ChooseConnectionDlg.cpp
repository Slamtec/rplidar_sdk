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

// ChooseConnectionDlg.cpp : implementation of the CChooseConnectionDlg class
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"
#include "drvlogic\lidarmgr.h"
#include "ChooseConnectionDlg.h"
#include "AutoDiscoveryDlg.h"

static const int baudRateLists[] = {
    115200,
    256000,
    1000000
};


CChooseConnectionDlg::CChooseConnectionDlg()
    :_usingNetwork(false)
{
    _protocolList.push_back("TCP");
    _protocolList.push_back("UDP");
}

LRESULT CChooseConnectionDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
    this->DoDataExchange();
    char buf[100];
    for (int pos = 0; pos < 100; ++pos) {
        sprintf(buf, "COM%d", pos + 1);
        m_combo_SerialPort.AddString(buf);
    }
    m_combo_SerialPort.SetCurSel(2);
    //int x = sizeof(protocolList);
    for (int pos = 0; pos < sizeof(baudRateLists)/sizeof(int); ++pos)
    {
        sprintf(buf, "%d", baudRateLists[pos]);
        m_combo_SerialBaudSel.AddString(buf);
    }

    m_combo_SerialBaudSel.SetCurSel(1);

    for (int pos = 0; pos < _protocolList.size(); pos++)
    {
        m_combo_NetProtocol.AddString(_protocolList[pos].c_str());
    }
    m_combo_NetProtocol.SetCurSel(0);
    
    CString   csIP = _T("192.168.11.2");
    m_IpAdress.SetWindowTextA(csIP);
    m_radio_ViaSerialPort.SetCheck(true);

    m_edit_IpPort.SetWindowTextA("8089");

    m_btn_auto_discovery.EnableWindow(false);

    m_radio_ViaSerialPort.Click();
    return TRUE;
}

LRESULT CChooseConnectionDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{

    if (_usingNetwork)
    {
        memset(_selectedConnectType.network.ip, 0, sizeof(_selectedConnectType.network.ip));
        memset(_selectedConnectType.network.protocol, 0, sizeof(_selectedConnectType.network.protocol));

        unsigned char ipSel[4];
        m_IpAdress.GetAddress((LPDWORD)ipSel);
        sprintf(_selectedConnectType.network.ip, "%d.%u.%u.%u", ipSel[3], ipSel[2], ipSel[1], ipSel[0]);

        m_combo_NetProtocol.GetLBText(m_combo_NetProtocol.GetCurSel(), _selectedConnectType.network.protocol);
       
        char buffer[64];
        m_edit_IpPort.GetWindowTextA(buffer,sizeof(buffer));
        _selectedConnectType.network.port = atoi(buffer);
        _selectedConnectType.network.usingNetwork = true;

    } 
    else
    {
        memset(_selectedConnectType.serial.serialPath, 0, sizeof(_selectedConnectType.serial.serialPath));
        sprintf(_selectedConnectType.serial.serialPath, "\\\\.\\com%d", m_combo_SerialPort.GetCurSel() + 1);
        _selectedConnectType.serial.baud = baudRateLists[m_combo_SerialBaudSel.GetCurSel()];
        _selectedConnectType.serial.usingNetwork = false;
    }
    EndDialog(wID);
    return 0;
}

LRESULT CChooseConnectionDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CChooseConnectionDlg::OnConnectionTypeClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int sel;
    sel = m_radio_ViaSerialPort.GetCheck();
    if (sel)
    {
        m_combo_SerialPort.EnableWindow(true);
        m_combo_SerialBaudSel.EnableWindow(true);
        m_IpAdress.EnableWindow(false);
        m_edit_IpPort.EnableWindow(false);
        m_combo_NetProtocol.EnableWindow(false);
        m_btn_auto_discovery.EnableWindow(false);
        _usingNetwork = false;
        return 0;
    }
    sel = m_radio_ViaNetwork.GetCheck();
    if (sel)
    {
        m_combo_SerialPort.EnableWindow(false);
        m_combo_SerialBaudSel.EnableWindow(false);
        m_IpAdress.EnableWindow(true);
        m_edit_IpPort.EnableWindow(true);
        m_combo_NetProtocol.EnableWindow(true);
        m_btn_auto_discovery.EnableWindow(true);
        _usingNetwork = true;
        return 0;
    }
    return 0;
}
LRESULT CChooseConnectionDlg::OnAutoDiscoveryClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAutoDiscoveryDlg device_discovery;
    if (device_discovery.DoModal() == IDCANCEL)
    {
        return false;
    }
    if (device_discovery.portSel != -1) 
    {
        DWORD ipSel = (DWORD) * ((LPDWORD)device_discovery.ipSel);
        m_IpAdress.SetAddress(ipSel);
        char buffer[16];
        sprintf(buffer, "%d", device_discovery.portSel);
        m_edit_IpPort.SetWindowTextA(buffer);
        for (int pos = 0; pos < _protocolList.size(); pos++)
        {

            m_combo_NetProtocol.GetLBText(pos, buffer);
            if (buffer == device_discovery.protocol) 
            {
                m_combo_NetProtocol.SetCurSel(pos);
                break;
            }
        }
    }
   
    return 0;
}

