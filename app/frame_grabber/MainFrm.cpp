/*
 *  SLAMTEC LIDAR
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

// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "scanView.h"
#include "MainFrm.h"
#include "sdkcommon.h"
#include "FreqSetDlg.h"
#include "IpConfigDlg.h"

const int REFRESEH_TIMER = 0x800;
static int lidarType;

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    //intercept dynamic created menu item msg firstly
    if(pMsg->wParam >= scanModeMenuRecBegin_ && pMsg->wParam < scanModeMenuRecBegin_ + modeVec_.size())
    {
        scanModeSelect(pMsg->wParam);
    }

	if (pMsg->wParam == IPCONFIG_SUB)
    {
        ipConfig();
    }
    
    if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
        return TRUE;
    if (m_hWndClient == m_scanview.m_hWnd){
        return m_scanview.PreTranslateMessage(pMsg);
    } else {
        return FALSE;
    }
}

BOOL CMainFrame::OnIdle()
{
    UIUpdateToolBar();
    return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // create command bar window
    HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
    // attach menu
    m_CmdBar.AttachMenu(GetMenu());
    // load command bar images
    m_CmdBar.LoadImages(IDR_MAINFRAME);
    // remove old menu
    SetMenu(NULL);

    HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE); 

    CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
    AddSimpleReBarBand(hWndCmdBar);
    AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

    CreateSimpleStatusBar();
    m_hWndClient =m_scanview.Create(m_hWnd, rcDefault, NULL, WS_CHILD  | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

    UIAddToolBar(hWndToolBar);
    UISetCheck(ID_VIEW_TOOLBAR, 1);
    UISetCheck(ID_VIEW_STATUS_BAR, 1);

    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    workingMode = WORKING_MODE_IDLE;
    LidarMgr::GetInstance().lidar_drv->getDeviceInfo(devInfo);


    LidarMgr::GetInstance().lidar_drv->getModelNameDescriptionString(lidarModelName);

    //get scan mode information
    modeVec_.clear();

    LidarMgr::GetInstance().lidar_drv->getAllSupportedScanModes(modeVec_); 

    sl_u16 typicalMode;
    LidarMgr::GetInstance().lidar_drv->getTypicalScanMode(typicalMode); 

    scanModeSubMenu_.CreateMenu();
    scanModeMenuRecBegin_ = 2001;
    std::vector<LidarScanMode>::iterator modeIter = modeVec_.begin();
    int index = 0;
    for(; modeIter != modeVec_.end(); modeIter++)
    {
        scanModeSubMenu_.AppendMenuA(MF_STRING, scanModeMenuRecBegin_ + index, modeIter->scan_mode);
        index++;
    }
	ipConfigMenu_.CreateMenu();
	lidarType = 1;
	if ((devInfo.model >> 4) > (LIDAR_S_SERIES_MINUM_MAJOR_ID+1)) {
        sl_result  ans = LidarMgr::GetInstance().lidar_drv->getDeviceMacAddr(devMac.macaddr);
        if (SL_IS_OK(ans)) {
            m_CmdBar.GetMenu().GetSubMenu(1).AppendMenuA(MF_STRING, IPCONFIG_SUB, TEXT("Ip Config"));
            support_motor_ctrl = true;
            lidarType = 2;
        }
       
    }
	
    MotorCtrlSupport motorSupport;
    LidarMgr::GetInstance().lidar_drv->checkMotorCtrlSupport(motorSupport);

    if (motorSupport == MotorCtrlSupportNone)
        support_motor_ctrl = false;
    else
        support_motor_ctrl = true;
  
    motorInfo_.motorCtrlSupport = motorSupport;
    LidarMgr::GetInstance().lidar_drv->getMotorInfo(motorInfo_);

    m_CmdBar.GetMenu().GetSubMenu(2).InsertMenuA(SCANMODE_SUB, MF_POPUP | MF_BYPOSITION, (UINT)scanModeSubMenu_.m_hMenu, "Scan Mode");
    usingScanMode_ = typicalMode;
    updateControlStatus();

    
    // setup timer
    this->SetTimer(REFRESEH_TIMER, 1000/30);
    checkDeviceHealth();
    UISetCheck(ID_CMD_STOP, 1);
    forcescan = 0;
    UISetCheck(ID_OPTION_EXPRESSMODE, TRUE);
    return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    LidarMgr::GetInstance().lidar_drv->setMotorSpeed(0);
    // unregister message filtering and idle updates
    this->KillTimer(REFRESEH_TIMER);
    
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    pLoop->RemoveIdleHandler(this);

    bHandled = FALSE;

    return 1;
}

void CMainFrame::ipConfig()
{
	struct _network
    {
        char ip[32];
        int port;
    } network ;

    onSwitchMode(WORKING_MODE_IDLE);
    CIpConfigDlg dlg;
    dlg.initIpconfig(channelRecord_.network.ip, "255.255.255.0", "192.168.11.1");
    dlg.DoModal();
    if (dlg.getIpConfResult())
    {
		memset(network.ip, 0, sizeof(network.ip));
		memcpy(network.ip, dlg.ip_, sizeof(network.ip));

		//wait for LPX Lidar startup
		delay(1000);
		LidarMgr::GetInstance().onDisconnect();
        if (!LidarMgr::GetInstance().onConnectUdp(network.ip, channelRecord_.network.port)) {
		   MessageBox("Cannot bind to the udp server,please reset the lidar", "Error", MB_OK);
		}
		else
		{
			char display_buffer[64];
			sprintf(display_buffer,"Reconnect to the lidar(%s)", network.ip);
			MessageBox(display_buffer, "Success", MB_OK);
		}
    }

}

void CMainFrame::scanModeSelect(int mode)
{
    if((unsigned int)mode >= scanModeMenuRecBegin_ && (unsigned int)mode < scanModeMenuRecBegin_ + modeVec_.size())
    {
        if (WORKING_MODE_SCAN == workingMode){
            MessageBox("Lidar is scanning, please stop scan before switch working mode!", "Warning", MB_OK);
            return;
        }
        usingScanMode_ = mode - scanModeMenuRecBegin_;
        onSwitchMode(WORKING_MODE_SCAN);
    }
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    PostMessage(WM_CLOSE);
    return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // TODO: add code to initialize document

    return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    static BOOL bVisible = TRUE;    // initially visible
    bVisible = !bVisible;
    CReBarCtrl rebar = m_hWndToolBar;
    int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);    // toolbar is 2nd added band
    rebar.ShowBand(nBandIndex, bVisible);
    UISetCheck(ID_VIEW_TOOLBAR, bVisible);
    UpdateLayout();
    return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
    ::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
    UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
    UpdateLayout();
    return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAboutDlg dlg;
    dlg.DoModal();
    return 0;
}


void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    switch (workingMode)
    {
    case WORKING_MODE_SCAN:
        onRefreshScanData();
        break;
    }
}

LRESULT CMainFrame::OnCmdReset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (MessageBox("The device will reboot.", "Are you sure?",
        MB_OKCANCEL|MB_ICONQUESTION) != IDOK) {
            return 0;
    }


    onSwitchMode(WORKING_MODE_IDLE);
    LidarMgr::GetInstance().lidar_drv->reset();
    return 0;
}
LRESULT CMainFrame::OnCmdStop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    onSwitchMode(WORKING_MODE_IDLE);
    m_scanview.stopScan();
    return 0;
}


LRESULT CMainFrame::OnCmdScan(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    onSwitchMode(WORKING_MODE_SCAN);
    return 0;
}

LRESULT CMainFrame::OnOptForcescan(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    forcescan=!forcescan;
    UISetCheck(ID_OPT_FORCESCAN, forcescan?1:0);
    return 0;
}

LRESULT CMainFrame::OnFileDumpdata(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    switch (workingMode) {
    case WORKING_MODE_SCAN:
        {
            //capture the snapshot
            std::vector<scanDot> snapshot = m_scanview.getScanList();

            //prompt
            CFileDialog dlg(FALSE);
            if (dlg.DoModal()==IDOK) {
                FILE * outputfile = fopen(dlg.m_szFileName, "w");
                fprintf(outputfile, "#RPLIDAR SCAN DATA\n#COUNT=%d\n#Angule\tDistance\tQuality\n",snapshot.size());
                for (int pos = 0; pos < (int)snapshot.size(); ++pos) {
                    fprintf(outputfile, "%.4f %.1f %d\n", snapshot[pos].angle, snapshot[pos].dist, snapshot[pos].quality);
                }
                fclose(outputfile);
            }
        }
        break;

    }
    return 0;
}


void    CMainFrame::onRefreshScanData()
{
    sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t cnt = _countof(nodes);
    ILidarDriver * lidar_drv = LidarMgr::GetInstance().lidar_drv;

    if (IS_OK(lidar_drv->grabScanDataHq(nodes, cnt, 0)))
    {
        float sampleDurationRefresh = modeVec_[usingScanMode_].us_per_sample;
        m_scanview.setScanData(nodes, cnt, sampleDurationRefresh);
    }
}

void    CMainFrame::updateControlStatus()
{
    //update menu item scan mode text
    char menuText[100];
    sprintf(menuText, "%s%s", "Scan mode: ", modeVec_[usingScanMode_].scan_mode);
    CString strTmp(menuText);
    LPCTSTR lp = (LPCTSTR)strTmp;

    m_CmdBar.GetMenu().GetSubMenu(2).ModifyMenuA(SCANMODE_SUB, MF_BYPOSITION, (UINT)scanModeSubMenu_.m_hMenu, lp);
    scanModeSubMenu_.CheckMenuRadioItem(0, modeVec_.size()-1,usingScanMode_,MF_BYPOSITION);

    //determine if menu items are usable
    switch (workingMode)
    {
    case WORKING_MODE_IDLE:
        m_CmdBar.GetMenu().GetSubMenu(2).EnableMenuItem(SCANMODE_SUB, MF_BYPOSITION | MF_ENABLED);
        break;

    case WORKING_MODE_SCAN:
        m_CmdBar.GetMenu().GetSubMenu(2).EnableMenuItem(SCANMODE_SUB, MF_BYPOSITION | MF_DISABLED);
        break;
    }

    onUpdateTitle();
}

void    CMainFrame::onUpdateTitle()
{
    char titleMsg[200];
    const char * workingmodeDesc;
    switch (workingMode) {
    case WORKING_MODE_IDLE:
        workingmodeDesc = "IDLE";
        break;
    case WORKING_MODE_SCAN:
        workingmodeDesc = "SCAN";
        break;
    default:
        assert(!"should not come here");
    }


    sprintf(titleMsg, "[%s] Model: %s(%d) FW: %d.%02d HW: %d Serial: "
        , workingmodeDesc
        , lidarModelName.c_str()
        , devInfo.model
        , devInfo.firmware_version>>8
        , devInfo.firmware_version & 0xFF, devInfo.hardware_version);

    
    for (int pos = 0, startpos = strlen(titleMsg); pos < sizeof(devInfo.serialnum); ++pos)
    {
        sprintf(&titleMsg[startpos], "%02X", devInfo.serialnum[pos]); 
        startpos+=2;
    }

	if ((devInfo.model >> 4) > LIDAR_T_SERIES_MINUM_MAJOR_ID) {
        int startpos = strlen(titleMsg);
        sprintf(&titleMsg[startpos], " MAC: ");
        startpos = strlen(titleMsg);
        for (int pos = 0; pos < sizeof(devMac.macaddr); ++pos)
        {
            sprintf(&titleMsg[startpos], "%02X-", devMac.macaddr[pos]);
            startpos += 3;
        }
        titleMsg[startpos - 1] = 0;
    }

    this->SetWindowTextA(titleMsg);
}

void    CMainFrame::onSwitchMode(int newMode)
{
    
    // switch mode
    if (newMode == workingMode) return;

    switch (newMode) {
    case WORKING_MODE_IDLE:
        {
            // stop the previous operation
            LidarMgr::GetInstance().lidar_drv->stop();
            UISetCheck(ID_CMD_STOP, 1);
            UISetCheck(ID_CMD_GRAB_PEAK, 0);
            UISetCheck(ID_CMD_GRAB_FRAME, 0);
            UISetCheck(ID_CMD_SCAN, 0);
            UISetCheck(ID_CMD_GRABFRAMENONEDIFF, 0);
        }
        break;
    case WORKING_MODE_SCAN:
        {
            CWindow  hwnd = m_hWndClient;
            hwnd.ShowWindow(SW_HIDE);
            m_hWndClient = m_scanview;
            m_scanview.ShowWindow(SW_SHOW);
            checkDeviceHealth();
            LidarMgr::GetInstance().lidar_drv->setMotorSpeed();
            LidarMgr::GetInstance().lidar_drv->startScanExpress(forcescan, usingScanMode_);
            UISetCheck(ID_CMD_STOP, 0);
            UISetCheck(ID_CMD_GRAB_PEAK, 0);
            UISetCheck(ID_CMD_GRAB_FRAME, 0);
            UISetCheck(ID_CMD_SCAN, 1);
            UISetCheck(ID_CMD_GRABFRAMENONEDIFF, 0);
        }
        break;
    default:
        assert(!"unsupported mode");
    }
    
    UpdateLayout();
    workingMode = newMode;
    updateControlStatus();
}

void    CMainFrame::checkDeviceHealth()
{
    int errorcode;
    if (!LidarMgr::GetInstance().checkDeviceHealth(&errorcode)){
        char msg[200];
        sprintf(msg, "The device is in unhealthy status.\n"
                   "You need to reset it.\n"
                   "Errorcode: 0x%08x", errorcode);
        
        MessageBox(msg, "Warning", MB_OK);

    }
}

LRESULT CMainFrame::OnSetFreq(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (!support_motor_ctrl) {
        MessageBox("The device is not supported to set frequency.\n", "Warning", MB_OK);
    } else {
        CFreqSetDlg dlg(lidarType);
        dlg.setMotorinfo(motorInfo_);
        dlg.DoModal();
    }
    return 0;
}
