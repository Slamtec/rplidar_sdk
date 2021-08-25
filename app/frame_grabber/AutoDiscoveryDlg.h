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

//
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include "hal/thread.h"
#include "dns_sd.h"

typedef struct dev_info_t {
    char txtbuffer[200];
}dev_info;

class CAutoDiscoveryDlg : public CDialogImpl<CAutoDiscoveryDlg>,
    public CWinDataExchange<CAutoDiscoveryDlg>
{
#define AUTO_DISCOVERY_DLG
public:
    CAutoDiscoveryDlg();
    CAutoDiscoveryDlg(CString ip, CString port);
    ~CAutoDiscoveryDlg();
	enum { IDD = IDD_DLG_AUTO_DISCOVERY };
    unsigned char ipSel[4];
    int portSel;
    std::string protocol;

	BEGIN_MSG_MAP(CAutoDiscoveryDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_BTN_REFRESH,OnRefresh)
        COMMAND_HANDLER(IDC_LIST_IP, LBN_DBLCLK, OnDoubleIpList)
    END_MSG_MAP()


	BEGIN_DDX_MAP(CAutoDiscoveryDlg)
        DDX_CONTROL_HANDLE(IDC_IPADDRESS_SEL, m_IpAdress)
        DDX_CONTROL_HANDLE(IDC_EDIT_IP_PORT, m_edit_IpPort)
        DDX_CONTROL_HANDLE(IDC_LIST_IP, m_list_ip)
        DDX_CONTROL_HANDLE(IDC_EDIT_DEV_TYPE, m_edit_dev_type)
	END_DDX_MAP();
	
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnRefresh(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


   
protected:
    CIPAddressCtrl m_IpAdress;
    CEdit m_edit_IpPort;
    CEdit m_edit_dev_type;
    CListBox m_list_ip;
    rp::hal::Thread  _thrdWorker;
    std::vector<std::string> host_list;

    dev_info _dev_info_instance[40];
    sl_result _testingThrdInit(void);
    sl_result _testingThrd(void);
    static void DNSSD_API zonedata_resolve(DNSServiceRef sdref, const DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
        const char* fullname, const char* hosttarget, uint16_t opaqueport, uint16_t txtLen, const unsigned char* txt, void* context);
    static void DNSSD_API zonedata_browse(DNSServiceRef sdref, const DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
        const char* replyName, const char* replyType, const char* replyDomain, void* context);
    static void HandleEvents(dev_info* dev_info_instance);
    static void generateListItemTxt(const int num, const char* hostname, int port, char* buffer, size_t buffersize);
    static int copyLabels(char* dst, const char* lim, const char** srcp, int labels);
public:
    LRESULT OnDoubleIpList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// VisualFC AppWizard will insert additional declarations immediately before the previous line.
