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
#include "stdafx.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "dns_sd.h"
#include "AutoDiscoveryDlg.h"

using namespace std;

DNSServiceRef client = NULL;
DNSServiceRef sc1;

#define LONG_TIME   10
static volatile int stopNow = 0;
static volatile int timeOut = LONG_TIME;
struct timeval tv;
typedef struct dev_info_t {
    char txtbuffer[200];
}dev_info;
dev_info dev_info_instance[40];
dev_info* dev_p;
int deviceNum = 0;
std::vector<std::pair<string, int> > info_list;
std::vector<string> host_list;

static void generateListItemTxt(int num, const char* hostname, int port, char* buffer, size_t buffersize)
{
    sprintf_s(buffer, buffersize, "%d  %s  %d "
        , num
        , hostname
        , port);
}

static int CopyLabels(char* dst, const char* lim, const char** srcp, int labels)
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
    char* ip_char = NULL;
    struct hostent* host = gethostbyname(name);
    if (!host) {
        return ip_char;
    }
    ip_char = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);

    return ip_char;
}

static void DNSSD_API zonedata_resolve(DNSServiceRef sdref, const DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
    const char* fullname, const char* hosttarget, uint16_t opaqueport, uint16_t txtLen, const unsigned char* txt, void* context)
{
    union { uint16_t s; u_char b[2]; } port = { opaqueport };
    uint16_t PortAsNumber = ((uint16_t)port.b[0]) << 8 | port.b[1];

    const char* p = fullname;
    char n[kDNSServiceMaxDomainName];
    char t[kDNSServiceMaxDomainName];
    char* m_ipaddr;
    string ip_str;
    if (errorCode)
        return;

    if (CopyLabels(n, n + kDNSServiceMaxDomainName, &p, 3)) return;
    p = fullname;
    if (CopyLabels(t, t + kDNSServiceMaxDomainName, &p, 1)) return;
    if (CopyLabels(t, t + kDNSServiceMaxDomainName, &p, 2)) return;

    generateListItemTxt(deviceNum, n, PortAsNumber, dev_p->txtbuffer, _countof(dev_p->txtbuffer));
    std::pair<string, int> current;
    current.first = hosttarget;
    current.second = PortAsNumber;
    info_list.push_back(current);
    dev_p++;
    deviceNum++;

    DNSServiceRefDeallocate(sdref);
    free(context);

    if (!(flags & kDNSServiceFlagsMoreComing))
    {
        fflush(stdout);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
    }

}

static void DNSSD_API zonedata_browse(DNSServiceRef sdref, const DNSServiceFlags flags, uint32_t ifIndex, DNSServiceErrorType errorCode,
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

static void HandleEvents(void)
{
    int dns_sd_fd = DNSServiceRefSockFD(client);
    int nfds = dns_sd_fd + 1;
    fd_set readfds;

    int result;
    dev_p = dev_info_instance;
    while (!stopNow)
    {
        FD_ZERO(&readfds);
        if (client) FD_SET(dns_sd_fd, &readfds);
        result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
        if (result > 0)
        {
            DNSServiceErrorType err = kDNSServiceErr_NoError;
            if (client && FD_ISSET(dns_sd_fd, &readfds))
                err = DNSServiceProcessResult(client);
            if (err)
            {
                stopNow = 1;
            }
        }
        else if (result == 0)
        {
            stopNow = 1;
        }
        else
        {
            if (errno != EINTR)
                stopNow = 1;
        }
    }
}

CAutoDiscoveryDlg::CAutoDiscoveryDlg()
{
}
CAutoDiscoveryDlg::~CAutoDiscoveryDlg()
{
    if (client)
    {
        DNSServiceRefDeallocate(client);
        client = 0;
    }

}

CAutoDiscoveryDlg::CAutoDiscoveryDlg(CString ip,CString port)
{
    m_IpAdress.SetWindowTextA(ip);
    m_edit_IpPort.SetWindowTextA(port);
}

u_result CAutoDiscoveryDlg::_testingThrd(void)
{
    DNSServiceErrorType err;
    char* typ, * dom;

    dom = "";
    typ = "_lidar._udp";
    tv.tv_sec = timeOut;
    tv.tv_usec = 0;

    err = DNSServiceCreateConnection(&client);
    sc1 = client;
    err = DNSServiceBrowse(&sc1, kDNSServiceFlagsShareConnection, kDNSServiceInterfaceIndexAny, typ, dom, zonedata_browse, NULL);

    if (!client || err != kDNSServiceErr_NoError)
    {
        return false;
    }

    HandleEvents();

    if (client)
    {
        DNSServiceRefDeallocate(client);
        client = 0;
    }
    if (!info_list.size())
    {
        m_list_ip.AddString("未发现指定类型设备");
        return false;
    }

    for (int i = 0; i < info_list.size(); ++i)
    {
        char* m_ipaddr;
        const char* target = info_list[i].first.c_str();
        m_ipaddr = get_ip(target);
        info_list[i].first = m_ipaddr;
    }

    for (int i = 0; i < deviceNum; ++i)
    {
        m_list_ip.AddString(dev_info_instance[i].txtbuffer);
    }
}

LRESULT CAutoDiscoveryDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
    this->DoDataExchange();
    rp::hal::Thread  _thrdWorker;
    _thrdWorker = CLASS_THREAD(CAutoDiscoveryDlg, _testingThrd);
    return TRUE;
}

LRESULT CAutoDiscoveryDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(IDOK);
    return 0;
}

LRESULT CAutoDiscoveryDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(IDCANCEL);
    return 0;
}

