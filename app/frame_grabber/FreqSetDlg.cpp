/*
 *  SLAMTEC LIDAR
 *  Win32 Demo Application
 *
 *  Copyright (c) 2016 - 2018 Shanghai Slamtec Co., Ltd.
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

// FreqSetDlg.cpp : implementation of the CFreqSetDlg class
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"
#include "drvlogic\lidarmgr.h"
#include "FreqSetDlg.h"

CFreqSetDlg::CFreqSetDlg(int lidarType):lidarType_(lidarType)
{
}

LRESULT CFreqSetDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(GetParent());
    this->DoDataExchange();
	CString str;
	if(1 == lidarType_)
	{
		m_sld_freq.SetRange(0, maxSpeed_);
		m_sld_freq.SetTicFreq(1);
		m_sld_freq.SetPos(minSpeed_);
    
		str.Format("%d", desiredSpeed_);
		m_edt_freq.SetWindowTextA(str);
	}
	else if (2 == lidarType_)
	{	
		m_sld_freq.SetRange(minSpeed_, maxSpeed_);
		m_sld_freq.SetTicFreq(1);
		m_sld_freq.SetPos(minSpeed_);

		str.Format("MIN(%d)", minSpeed_);
		m_min_freq.SetWindowTextA(str);
		str.Format("MAX(%d)", maxSpeed_);
		m_max_freq.SetWindowTextA(str);
		
		str.Format("%d", desiredSpeed_);
		m_edt_freq.SetWindowTextA(str);
	}

    return TRUE;
}

LRESULT CFreqSetDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    char data[10];
    m_edt_freq.GetWindowTextA(data,_countof(data));
    sl_u16 pwm = atoi(data);

    if(1 == lidarType_) {
        if (pwm >= maxSpeed_) {
            pwm = maxSpeed_;
            CString str;
            str.Format("%d", pwm);
            m_edt_freq.SetWindowTextA(str);
        }
    }
    else if (2 == lidarType_) {
        if (pwm >= maxSpeed_) {
            pwm = maxSpeed_;
            CString str;
            str.Format("%d", pwm);
            m_edt_freq.SetWindowTextA(str);
        }
    }

    m_sld_freq.SetPos(pwm);
    LidarMgr::GetInstance().lidar_drv->setMotorSpeed(pwm);
    return 0;
}

LRESULT CFreqSetDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

void CFreqSetDlg::OnHScroll(UINT nSBCode, LPARAM /*lParam*/, CScrollBar pScrollBar)
{
    if (pScrollBar.m_hWnd == m_sld_freq.m_hWnd)
    {
        int realPos = m_sld_freq.GetPos();

        CString str;
        str.Format("%d", realPos);
        m_edt_freq.SetWindowTextA(str);
        LidarMgr::GetInstance().lidar_drv->setMotorSpeed(realPos);
    }
}

void CFreqSetDlg::setMotorinfo(const LidarMotorInfo& motorInfo)
{
    maxSpeed_ = motorInfo.max_speed;
    minSpeed_ = motorInfo.min_speed;
    desiredSpeed_ = motorInfo.desired_speed;
}
