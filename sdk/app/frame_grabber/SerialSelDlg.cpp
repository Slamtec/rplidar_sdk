/*
 *  RPLIDAR
 *  Win32 Demo Application
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2019 Shanghai Slamtec Co., Ltd.
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

// SerialSelDlg.cpp : implementation of the CSerialSelDlg class
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"

#include "SerialSelDlg.h"

static const int baudRateLists[] = {
    57600,
    115200,
    256000
};

CSerialSelDlg::CSerialSelDlg()
    : selectedID(-1)
    , usingNetwork(false)
    , selectedBaudRate(115200)
{
}

LRESULT CSerialSelDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
    this->DoDataExchange();
    char buf[100]; 
    for (int pos= 0; pos<100; ++pos) {
        sprintf(buf,"COM%d",pos+1);
        m_sel_box.AddString(buf);
    }
    m_sel_box.SetCurSel(2);
    selectedID = 2;

    for (size_t pos = 0; pos < _countof(baudRateLists); ++pos) {
        CString str;
        str.Format("%d", baudRateLists[pos]);
        m_comb_serialbaud.AddString(str);
    }
    m_comb_serialbaud.SetCurSel(0);

    return TRUE;
}

LRESULT CSerialSelDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CSerialSelDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CSerialSelDlg::OnCbnSelchangeCombSerialSel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    selectedID = m_sel_box.GetCurSel();
    return 0;
}


LRESULT CSerialSelDlg::OnBnClickedButtonTcpserver(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // TODO: Add your control notification handler code here
    usingNetwork = true;
    EndDialog(IDOK);
    return 0;
}


LRESULT CSerialSelDlg::OnCbnSelchangeCombBaudrate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // TODO: Add your control notification handler code here
    selectedBaudRate = baudRateLists[m_comb_serialbaud.GetCurSel()];
    return 0;
}
