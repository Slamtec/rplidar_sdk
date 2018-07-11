#pragma once

class TCPChannelSelDlg : public CDialogImpl<TCPChannelSelDlg>,
    public CWinDataExchange<TCPChannelSelDlg>
{
public:
    CEdit m_edt_server_port_;
    CEdit m_edt_server_ip_;
public:
    TCPChannelSelDlg();
    enum { IDD = IDD_DIG_TCP_SEL};


    BEGIN_MSG_MAP(TCPChannelSelDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

    BEGIN_DDX_MAP(TCPChannelSelDlg)
        DDX_CONTROL_HANDLE(IDC_EDIT_PORT, m_edt_server_port_)
        DDX_CONTROL_HANDLE(IDC_EDIT_IP, m_edt_server_ip_)
    END_DDX_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    void setPort(int pt){
        port_ = pt;
    }

    void setIp(std::string ip){
        server_ip_ = ip;
    }
    
    int getPort() {
        return port_;
    }

    std::string getIp() {
        return server_ip_;
    }

protected:
    int     port_;
    std::string server_ip_;
};

