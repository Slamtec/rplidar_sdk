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

//
/////////////////////////////////////////////////////////////////////////////

#pragma once


struct scanDot {
    _u8   quality;
    float angle;
    float dist;
};

class CScanView : public CWindowImpl<CScanView>
{
public:
    DECLARE_WND_CLASS(NULL)

    
    BOOL PreTranslateMessage(MSG* pMsg);

    BEGIN_MSG_MAP(CScanView)
        MSG_WM_MOUSEWHEEL(OnMouseWheel)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_MOUSEMOVE(OnMouseMove)
    END_MSG_MAP()

    void DoPaint(CDCHandle dc);



    const std::vector<scanDot> & getScanList() const {
        return _scan_data;
    }


    void onDrawSelf(CDCHandle dc);
    void setScanData(rplidar_response_measurement_node_hq_t *buffer, size_t count, float sampleDuration);
    void stopScan();
    CScanView();

    BOOL OnEraseBkgnd(CDCHandle dc);
    void OnMouseMove(UINT nFlags, CPoint point);
    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnPaint(CDCHandle dc);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
protected:
    CFont stdfont;
    CFont bigfont;
    POINT                _mouse_pt;
    float                _mouse_angle;
    std::vector<scanDot> _scan_data;
    float                _scan_speed;
    float                _sample_duration;
    float                _current_display_range;
    int                  _sample_counter;
    _u64                 _last_update_ts;
    bool                 _is_scanning;
};
