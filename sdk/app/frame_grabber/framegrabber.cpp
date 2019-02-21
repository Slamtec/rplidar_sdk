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

// framegrabber.cpp : main source file for framegrabber.exe
//

#include "stdafx.h"

#include "resource.h"
#include "scanView.h"
#include "aboutdlg.h"
#include "MainFrm.h"
#include "SerialSelDlg.h"
#include "TcpChannelSelDlg.h"
#include "drvlogic\lidarmgr.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);

    CMainFrame wndMain;
    CSerialSelDlg serialsel;

    if (serialsel.DoModal() == IDCANCEL) return 0;

	if (serialsel.isUseNetworing())
	{
		TCPChannelSelDlg tcp_channel_sel;

		if (tcp_channel_sel.DoModal() == IDCANCEL) {
            return false;
        }

        if (!LidarMgr::GetInstance().onConnectTcp(tcp_channel_sel.getIp().c_str(), tcp_channel_sel.getPort())) {
            MessageBox(NULL, "Cannot bind to the tcp server.", "Error", MB_OK);
            return false;
        }
	}
	else
	{
		char serialpath[255];
		sprintf(serialpath, "\\\\.\\com%d", serialsel.getSelectedID()+1);
    
		if (!LidarMgr::GetInstance().onConnect(serialpath, serialsel.getSelectedBaudRate())) {
			MessageBox(NULL, "Cannot bind to the specified port.", "Error", MB_OK);
			return -1;
		}
	}

    if(wndMain.CreateEx() == NULL)
    {
        ATLTRACE(_T("Main window creation failed!\n"));
        return 0;
    }

    wndMain.ShowWindow(nCmdShow);

    int nRet = theLoop.Run();

    _Module.RemoveMessageLoop();

    LidarMgr::GetInstance().onDisconnect();
    return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
    HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//    HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ATLASSERT(SUCCEEDED(hRes));


    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(NULL, 0, 0, 0L);

    AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);    // add flags to support other controls

    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));

    int nRet = Run(lpstrCmdLine, nCmdShow);

    _Module.Term();
    

    ::CoUninitialize();

    return nRet;
}
