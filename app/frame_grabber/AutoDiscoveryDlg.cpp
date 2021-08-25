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
#include "stdafx.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "AutoDiscoveryDlg.h"



#define LONG_TIME   10
//#define LONG_TIME   3600000
static DNSServiceRef client = NULL;
static struct timeval _DnsServiceTv;
static dev_info* dev_p;
static int _deviceNum = 0;
static std::vector<std::pair<std::string, int> > info_list;
static bool _dialog_alive = false;

void CAutoDiscoveryDlg::generateListItemTxt(const int num, const char* hostname, int port, char* buffer, size_t buffersize)
{
    sprintf_s(buffer, buffersize, "%d  %s  %d "
        , num
        , hostname
        , port);
}

int CAutoDiscoveryDlg::copyLabels(char* dst, const char* lim, const char** srcp, int labels)
{
    const char* src = *srcp;
    while (*src != '.' || --labels > 0)
    {
        if (*src == '\\') *dst++ = *src++;
        if (!*src || dst >= lim) return -1;
        *dst++ = *src++;
        if (!*src || dst >= lim) return -1;
    }
    *dst++ = 0;
    *srcp = src + 1;
    return 0;
}

static char* get_ip(const char* const name)
{
    char* ip_char = "";
    struct hostent* host = gethostbyname(name);
    if (host == NULL) {
        return ip_char;
    }
    ip_char = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);

    return ip_char;
}

void DNSSD_API CAutoDiscoveryDlg::zonedata_resolve(DNSServiceRef sdref, const DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
    const char* fullname, const char* hosttarget, uint16_t opaqueport, uint16_t txtLen, const unsigned char* txt, void* context)
{
    union { uint16_t s; u_char b[2]; } port = { opaqueport };
    uint16_t PortAsNumber = ((uint16_t)port.b[0]) << 8 | port.b[1];

    const char* p = fullname;
    char n[kDNSServiceMaxDomainName];
    char t[kDNSServiceMaxDomainName];
    char* m_ipaddr;
    std::string ip_str;
    if (errorCode)
        return;

    if (copyLabels(n, n + kDNSServiceMaxDomainName, &p, 3)) return;
    p = fullname;
    if (copyLabels(t, t + kDNSServiceMaxDomainName, &p, 1)) return;
    if (copyLabels(t, t + kDNSServiceMaxDomainName, &p, 2)) return;

    generateListItemTxt(_deviceNum, n, PortAsNumber, dev_p->txtbuffer, _countof(dev_p->txtbuffer));
    std::pair<std::string, int> current;
    current.first = hosttarget;
    current.second = PortAsNumber;
    info_list.push_back(current);
    dev_p++;
    _deviceNum++;

    DNSServiceRefDeallocate(sdref);
    free(context);

    if (!(flags & kDNSServiceFlagsMoreComing))
    {
        fflush(stdout);
        _DnsServiceTv.tv_sec = 1;
        _DnsServiceTv.tv_usec = 0;
    }

}

void DNSSD_API CAutoDiscoveryDlg::zonedata_browse(DNSServiceRef sdref, const DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
    const char* replyName, const char* replyType, const char* replyDomain, void* context)
{
    DNSServiceRef* newref;

    if (!(flags & kDNSServiceFlagsAdd))
        return;
    if (errorCode)
        return;

    newref = (DNSServiceRef*)malloc(sizeof(newref));
    *newref = client;
    DNSServiceResolve(newref, kDNSServiceFlagsShareConnection, ifIndex, replyName, replyType, replyDomain, zonedata_resolve, newref);
}

void CAutoDiscoveryDlg::HandleEvents(dev_info *dev_info_instance)
{
    int dns_sd_fd = DNSServiceRefSockFD(client);
    int nfds = dns_sd_fd + 1;
    fd_set readfds;

    int result;
    dev_p = dev_info_instance;

    while (1)
    {
#ifdef AUTO_DISCOVERY_DLG
        if (!_dialog_alive) break;
#endif
        FD_ZERO(&readfds);
        if (client) FD_SET(dns_sd_fd, &readfds);
        result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &_DnsServiceTv);
        if (result > 0)
        {
            DNSServiceErrorType err = kDNSServiceErr_NoError;
            if (client && FD_ISSET(dns_sd_fd, &readfds))
                err = DNSServiceProcessResult(client);
            if (err)
            {
                break;
            }
        }
        else if (result == 0)
        {
            break;
        }
        else
        {
            if (errno != EINTR)
                break;
        }
    }
}

CAutoDiscoveryDlg::CAutoDiscoveryDlg()
    :portSel(-1)
{
    for (int pos = 0; pos < sizeof(ipSel) / sizeof(unsigned char); pos++)
        ipSel[pos] = 0;
}
CAutoDiscoveryDlg::~CAutoDiscoveryDlg()
{
    if (client)
    {
        DNSServiceRefDeallocate(client);
        client = NULL;
    }

}

CAutoDiscoveryDlg::CAutoDiscoveryDlg(CString ip,CString port)
{
    m_IpAdress.SetWindowTextA(ip);
    m_edit_IpPort.SetWindowTextA(port);
}

sl_result CAutoDiscoveryDlg::_testingThrdInit(void)
{
    _deviceNum = 0;
    info_list.clear();
    _dialog_alive = true;
    _thrdWorker = CLASS_THREAD(CAutoDiscoveryDlg, _testingThrd);
    return  TRUE;
}

sl_result CAutoDiscoveryDlg::_testingThrd(void)
{
    DNSServiceErrorType err;
    DNSServiceRef sc1;
    char type[256], * dom;

    dom = "";
    m_edit_dev_type.GetWindowTextA(type, sizeof(type));
    //typ = "_lidar._udp";
    _DnsServiceTv.tv_sec = LONG_TIME;
    _DnsServiceTv.tv_usec = 0;

    err = DNSServiceCreateConnection(&client);
    sc1 = client;
    err = DNSServiceBrowse(&sc1, kDNSServiceFlagsShareConnection, kDNSServiceInterfaceIndexAny, type, dom, zonedata_browse, NULL);

    if (!client || err != kDNSServiceErr_NoError)
    {
        return false;
    }

    HandleEvents(_dev_info_instance);

#ifdef AUTO_DISCOVERY_DLG
    if (!_dialog_alive) return TRUE;
#endif

    if (client)
    {
        DNSServiceRefDeallocate(client);
        client = 0;
    }
    if (!info_list.size())
    {
        m_list_ip.AddString("The specified device was not found!");
        return false;
    }

    for (int i = 0; i < info_list.size(); ++i)
    {
        char* m_ipaddr;
        const char* target = info_list[i].first.c_str();
        m_ipaddr = get_ip(target);
        info_list[i].first = m_ipaddr;
    }

    for (int i = 0; i < _deviceNum; ++i)
    {
        m_list_ip.AddString(_dev_info_instance[i].txtbuffer);
    }
    return TRUE;
}

LRESULT CAutoDiscoveryDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
    this->DoDataExchange();
    _testingThrdInit();
    m_edit_dev_type.SetWindowTextA("_lidar._udp");
    return TRUE;
}

LRESULT CAutoDiscoveryDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    _thrdWorker.terminate();
    
    if (client)
    {
        DNSServiceRefDeallocate(client);
        client = NULL;
    }
    _dialog_alive = false;
    EndDialog(IDOK);
    return 0;
}

LRESULT CAutoDiscoveryDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    _thrdWorker.terminate();
    if (client)
    {
        DNSServiceRefDeallocate(client);
        client = NULL;
    }
    _dialog_alive = false;
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CAutoDiscoveryDlg::OnRefresh(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    _thrdWorker.terminate();
    int size = m_list_ip.GetCount();
    if (size)
    {   
        size--;
        m_list_ip.DeleteString(size);
    }
    _testingThrdInit();
   
    return 0;
}

LRESULT CAutoDiscoveryDlg::OnDoubleIpList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    char buffer[128];
    int currentSel = m_list_ip.GetCurSel();
    if (currentSel < 0) return 0;

    sprintf(buffer, "%s", info_list[currentSel].first.c_str());
    m_IpAdress.SetWindowTextA(buffer);
    sprintf(buffer, "%d", info_list[currentSel].second);
    m_edit_IpPort.SetWindowTextA(buffer);
    m_IpAdress.GetAddress((LPDWORD)ipSel);
    m_edit_IpPort.GetWindowTextA(buffer, sizeof(buffer));
    portSel = atoi(buffer);
    _DnsServiceTv.tv_sec = 1;
    m_edit_dev_type.GetWindowTextA(buffer,sizeof(buffer));
    std::string dev_type(buffer);
    if (dev_type.find("udp") == -1)
    {
        protocol = "TCP";
    }
    else
    {
        protocol = "UDP";
    }
    return 0;
}
