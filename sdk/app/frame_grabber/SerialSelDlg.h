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

// SerialSelDlg.h : interface of the CSerialSelDlg class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __SERIALSELDLG_H__
#define __SERIALSELDLG_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CSerialSelDlg : public CDialogImpl<CSerialSelDlg>,
    public CWinDataExchange<CSerialSelDlg>
{
public:
    CComboBox    m_sel_box;
    CComboBox    m_comb_serialbaud;
    CSerialSelDlg();
    enum { IDD = IDD_DLG_SERIAL_SEL };


    BEGIN_MSG_MAP(CSerialSelDlg)
        COMMAND_HANDLER(IDC_COMB_SERIAL_SEL, CBN_SELCHANGE, OnCbnSelchangeCombSerialSel)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_HANDLER(IDC_BUTTON_TCPSERVER, BN_CLICKED, OnBnClickedButtonTcpserver)
        COMMAND_HANDLER(IDC_COMB_BAUDRATE, CBN_SELCHANGE, OnCbnSelchangeCombBaudrate)
    END_MSG_MAP()

    BEGIN_DDX_MAP(CSerialSelDlg)
        DDX_CONTROL_HANDLE(IDC_COMB_BAUDRATE, m_comb_serialbaud)
        DDX_CONTROL_HANDLE(IDC_COMB_SERIAL_SEL, m_sel_box)
    END_DDX_MAP();
    
    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    
    int     getSelectedBaudRate() {
        return selectedBaudRate;
    }
    int     getSelectedID() {
        return selectedID;
    }
    bool    isUseNetworing() {
        return usingNetwork;
    }
    LRESULT OnCbnSelchangeCombSerialSel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

protected:
    int     selectedID;
    int     selectedBaudRate;
    bool    usingNetwork;
public:
    LRESULT OnBnClickedButtonTcpserver(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnSelchangeCombBaudrate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// VisualFC AppWizard will insert additional declarations immediately before the previous line.
#endif // __SERIALSELDLG_H__
