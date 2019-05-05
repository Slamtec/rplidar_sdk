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

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "drvlogic\lidarmgr.h"

#define    SCANMODE_SUB 1

class CMainFrame : 
    public CFrameWindowImpl<CMainFrame>, 
    public CUpdateUI<CMainFrame>,
    public CMessageFilter, public CIdleHandler
{
public:
    enum {
        WORKING_MODE_IDLE       = 0,
        WORKING_MODE_SCAN       = 3,
    };
    enum {
        RPLIDAR_A_SERIES_MINUM_MAJOR_ID       = 0,
        RPLIDAR_S_SERIES_MINUM_MAJOR_ID       = 5,
        RPLIDAR_T_SERIES_MINUM_MAJOR_ID       = 8,
    };

    DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)
    CScanView         m_scanview;
    CCommandBarCtrl m_CmdBar;

    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnIdle();

    BEGIN_UPDATE_UI_MAP(CMainFrame)
        UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
        UPDATE_ELEMENT(ID_CMD_STOP, UPDUI_MENUPOPUP| UPDUI_TOOLBAR)
        UPDATE_ELEMENT(ID_CMD_SCAN, UPDUI_MENUPOPUP| UPDUI_TOOLBAR)

        UPDATE_ELEMENT(ID_OPT_FORCESCAN, UPDUI_MENUPOPUP)
    END_UPDATE_UI_MAP()

    BEGIN_MSG_MAP(CMainFrame)
        COMMAND_ID_HANDLER(ID_FILE_DUMPDATA, OnFileDumpdata)
        COMMAND_ID_HANDLER(ID_OPT_FORCESCAN, OnOptForcescan)
        COMMAND_ID_HANDLER(ID_CMD_SCAN, OnCmdScan)
        COMMAND_ID_HANDLER(ID_CMD_STOP, OnCmdStop)
        COMMAND_ID_HANDLER(ID_CMD_RESET, OnCmdReset)
        COMMAND_ID_HANDLER(ID_CMD_SET_FREQ, OnSetFreq)
        MSG_WM_TIMER(OnTimer)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
        COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
        COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
        COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
        COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
        CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()

    void    onUpdateTitle();
    void    onSwitchMode(int newMode);
    void    onRefreshScanData();
    void    checkDeviceHealth();

    void    updateControlStatus();
    void    scanModeSelect(int mode);

    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
    LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    void OnTimer(UINT_PTR nIDEvent);
    LRESULT OnCmdReset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSetFreq(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCmdStop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCmdScan(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnOptForcescan(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnFileDumpdata(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    

protected:
    int     workingMode; // 0 - idle 1 - framegrabber
    bool    forcescan;
    bool    useExpressMode;
    bool    inExpressMode;
    bool    support_motor_ctrl;

    //lidar changeable parameters
    _u16     usingScanMode_;   //record the currently using scan mode

    //firmware 1.24
    std::vector<RplidarScanMode> modeVec_;

    rplidar_response_device_info_t devInfo;

    //subMenu of scan mode
    CMenu scanModeSubMenu_;
    size_t scanModeMenuRecBegin_;
};
