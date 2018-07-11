// SerialSelDlg.cpp : implementation of the CSerialSelDlg class
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"

#include "TcpChannelSelDlg.h"
#include "drvlogic\lidarmgr.h"

using namespace std;

TCPChannelSelDlg::TCPChannelSelDlg()
    : port_(20108)
    , server_ip_("192.168.0.7")
{
}


LRESULT TCPChannelSelDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
    this->DoDataExchange(false);
    m_edt_server_ip_.SetWindowTextA(server_ip_.c_str());
    char portStr[10];
    itoa(port_, portStr, 10);
    m_edt_server_port_.SetWindowTextA(portStr);
    return TRUE;
}

LRESULT TCPChannelSelDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    char buffer[1024];

    m_edt_server_port_.GetWindowTextA(buffer, sizeof(buffer));
    port_ = atoi(buffer);
    m_edt_server_ip_.GetWindowTextA(buffer, sizeof(buffer));
    server_ip_ = std::string(buffer);

    EndDialog(wID);
    return 0;
}

LRESULT TCPChannelSelDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

