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

// IpConfigDlg.cpp : implementation of the CIpConfigDlg class
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"
#include "drvlogic\lidarmgr.h"
#include "IpConfigDlg.h"

unsigned char CIpConfigDlg::ip_[32] = {0};
unsigned char CIpConfigDlg::mask_[32] = {0};
unsigned char CIpConfigDlg::gw_[32] = {0};

CIpConfigDlg::CIpConfigDlg()
    :is_successful_(false)
{

}

LRESULT CIpConfigDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
    this->DoDataExchange();
    m_ip.SetWindowTextA(CString(ip_));
    m_mask.SetWindowTextA(CString(mask_));
    m_gw.SetWindowTextA(CString(gw_));
    return TRUE;
}

LRESULT CIpConfigDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    sl_lidar_ip_conf_t manual_conf;
    unsigned char sel[4];
    m_ip.GetAddress((LPDWORD)sel);
    sprintf((char *)ip_, "%u.%u.%u.%u", sel[3], sel[2], sel[1], sel[0]);
    for (int i = 0; i < 4; i++)
    {
        manual_conf.ip_addr[i] = sel[3 - i];
    }
    m_mask.GetAddress((LPDWORD)sel);
    sprintf((char*)mask_, "%u.%u.%u.%u", sel[3], sel[2], sel[1], sel[0]);
    for (int i = 0; i < 4; i++)
    {
        manual_conf.net_mask[i] = sel[3 - i];
    }

    m_gw.GetAddress((LPDWORD)sel);
    sprintf((char*)gw_, "%u.%u.%u.%u", sel[3], sel[2], sel[1], sel[0]);
    for (int i = 0; i < 4; i++)
    {
        manual_conf.gw[i] = sel[3 - i];
    }
    
	sl_result ans = LidarMgr::GetInstance().lidar_drv->setLidarIpConf(manual_conf);
   
	if (SL_IS_FAIL(ans)) {
		sl_result ans = LidarMgr::GetInstance().lidar_drv->setLidarIpConf(manual_conf);
        if (SL_IS_FAIL(ans)) {
			MessageBox("IP Config failed.", "error", MB_OK);
			is_successful_ = false;
		}
		else
            is_successful_ = true;
    }
    else
        is_successful_ = true;

    EndDialog(wID);
    return 0;
}

LRESULT CIpConfigDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CIpConfigDlg::OnDhcpClient(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    sl_lidar_ip_conf_t client_conf;
    for (int i = 0; i < 4; i++)
    {
        client_conf.ip_addr[i] = 0;
        client_conf.net_mask[i] = 0;
        client_conf.gw[i] = 0;
    }
    sl_result ans = LidarMgr::GetInstance().lidar_drv->setLidarIpConf(client_conf);
    if (SL_IS_FAIL(ans)) {
        MessageBox("Config lidar as DHCP Client failed.", "error", MB_OK);
        is_successful_ = false;
    }
    else
        MessageBox("Please connect Lidar to DHCP Server.", "Success", MB_OK);
    EndDialog(wID);
    return 0;
}