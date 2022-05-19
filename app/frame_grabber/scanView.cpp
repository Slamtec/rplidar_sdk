/*
 *  SLAMTEC LIDAR
 *  Win32 Demo Application
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2020 Shanghai Slamtec Co., Ltd.
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
#include <math.h>
#include "scanView.h"

const int DEF_MARGIN = 20;
const int DISP_RING_ABS_DIST  = 100;
const float DISP_FULL_DIST    = 28000;
const float DISP_DEFAULT_DIST = 8000;
const float DISP_MIN_DIST     = 1000;
const float PI   = (float)3.14159265;

const COLORREF dot_full_brightness = RGB(44,233,22);
CScanView::CScanView()
{
    bigfont.CreateFontA(32/*-MulDiv(8, GetDeviceCaps(desktopdc, LOGPIXELSY), 72)*/,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH ,"Verdana");
    stdfont.CreateFontA(14/*-MulDiv(8, GetDeviceCaps(desktopdc, LOGPIXELSY), 72)*/,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH ,"Verdana");

    _current_display_range = DISP_DEFAULT_DIST;

    _mouse_angle = 0;
    _mouse_pt.x= _mouse_pt.y = 0;
    _is_scanning = false;
}

BOOL CScanView::PreTranslateMessage(MSG* pMsg)
{
    pMsg;
    return FALSE;
}

void CScanView::DoPaint(CDCHandle dc)
{

}


void CScanView::onDrawSelf(CDCHandle dc)
{
    CDC memDC;
    CBitmap memBitmap;
    memDC.CreateCompatibleDC(dc);
    CRect clientRECT;
    this->GetClientRect(&clientRECT);

    memBitmap.CreateCompatibleBitmap(dc,clientRECT.Width() , clientRECT.Height());
    HBITMAP oldBitmap = memDC.SelectBitmap(memBitmap);

    HPEN oldPen = memDC.SelectStockPen(DC_PEN);
   
    HBRUSH oldBrush = memDC.SelectStockBrush(NULL_BRUSH);
    HFONT  oldFont  = memDC.SelectFont(stdfont);

    memDC.SetBkMode(0);
    memDC.SetTextColor(RGB(90, 90, 90));
    memDC.SetTextAlign(TA_CENTER | TA_BASELINE);

    memDC.SetDCPenColor(RGB(60,60,60));

    CPoint centerPt(clientRECT.Width()/2, clientRECT.Height()/2);
    const int maxPixelR = min(clientRECT.Width(), clientRECT.Height())/2 - DEF_MARGIN;
    const float distScale = (float)maxPixelR/_current_display_range;

    char txtBuffer[100];

    // plot rings
    for (int angle = 0; angle<360; angle += 30) {
        float rad = (float)(angle*PI/180.0);

        float endptX = sin(rad)*(maxPixelR+DEF_MARGIN/2) + centerPt.x;
        float endptY = centerPt.y - cos(rad)*(maxPixelR+DEF_MARGIN/2);

        memDC.MoveTo(centerPt);
        memDC.LineTo((int)endptX, (int)endptY);

        sprintf(txtBuffer, "%d", angle);
        memDC.TextOutA((int)endptX, (int)endptY, txtBuffer);

    }

    for (int plotR = maxPixelR; plotR>0; plotR-=DISP_RING_ABS_DIST)
    {
        memDC.Ellipse(centerPt.x-plotR, centerPt.y-plotR,
            centerPt.x+plotR, centerPt.y+plotR);

        sprintf(txtBuffer, "%.1f", (float)plotR/distScale);
        memDC.TextOutA(centerPt.x, centerPt.y-plotR, txtBuffer);
    }

    memDC.SelectStockBrush(DC_BRUSH);
    memDC.SelectStockPen(NULL_PEN);

    int picked_point = 0;
    float min_picked_dangle = 100;

    for (int pos =0; pos < (int)_scan_data.size(); ++pos) {
        float distPixel = _scan_data[pos].dist*distScale;
        float rad = (float)(_scan_data[pos].angle*PI/180.0);
        float endptX = sin(rad)*(distPixel) + centerPt.x;
        float endptY = centerPt.y - cos(rad)*(distPixel);

        float dangle = fabs(rad - _mouse_angle);

        if (dangle<min_picked_dangle) {
            min_picked_dangle = dangle;
            picked_point = pos;
        }

        int brightness = (_scan_data[pos].quality<<1) + 128;
        if (brightness>255) brightness=255;

        memDC.FillSolidRect((int)endptX-1,(int)endptY-1, 2, 2,RGB(0,brightness,brightness));
    }

    memDC.SelectFont(bigfont);

    memDC.SetTextAlign(TA_LEFT | TA_BASELINE);
    memDC.SetTextColor(RGB(255,255,255));

    sprintf(txtBuffer, "%.1f Hz (%d RPM)", _scan_speed, (int)(_scan_speed*60));
    memDC.TextOutA(DEF_MARGIN, DEF_MARGIN + 40, txtBuffer);

    if ((int)_scan_data.size() > picked_point) {
        float distPixel = _scan_data[picked_point].dist*distScale;
        float rad = (float)(_scan_data[picked_point].angle*PI/180.0);
        float endptX = sin(rad)*(distPixel) + centerPt.x;
        float endptY = centerPt.y - cos(rad)*(distPixel);


        memDC.SetDCPenColor(RGB(129,10,16));
        memDC.SelectStockPen(DC_PEN);
        memDC.MoveTo(centerPt.x,centerPt.y);
        memDC.LineTo((int)endptX,(int)endptY);
        memDC.SelectStockPen(NULL_PEN);

        memDC.FillSolidRect((int)endptX-1,(int)endptY-1, 2, 2,RGB(255,0,0));

        memDC.SetTextColor(RGB(255,0,0));
        sprintf(txtBuffer, "Current: %.2f Deg: %.2f,Quality:%d", _scan_data[picked_point].dist,  _scan_data[picked_point].angle, _scan_data[picked_point].quality);
        memDC.TextOutA(DEF_MARGIN, DEF_MARGIN + 20, txtBuffer);

        memDC.SetTextColor(RGB(255,255,255));
        memDC.SetDCBrushColor(RGB(255,0,0));
        memDC.Rectangle(clientRECT.Width() - 100, DEF_MARGIN + 25, clientRECT.Width() - 60, DEF_MARGIN + 30);
        int frequency = -1;
        if(_is_scanning)
            frequency = int(floor(1000.0 / _sample_duration+0.5));
        else
            frequency = 0;
        sprintf(txtBuffer, "%d K", frequency);
        memDC.TextOutA(clientRECT.Width() - 100, DEF_MARGIN + 20, txtBuffer);
    }

    dc.BitBlt(0, 0, clientRECT.Width(), clientRECT.Height()
        , memDC, 0, 0, SRCCOPY);

    memDC.SelectFont(oldFont);
    memDC.SelectBrush(oldBrush);
    memDC.SelectPen(oldPen);
    memDC.SelectBitmap(oldBitmap);

    
}


BOOL CScanView::OnEraseBkgnd(CDCHandle dc)
{
 

    return 0;
}

void CScanView::OnMouseMove(UINT nFlags, CPoint point)
{
    _mouse_pt = point;

    CRect clientRECT;
    this->GetClientRect(&clientRECT);

    int dy = -(point.y - ((clientRECT.bottom-clientRECT.top)/2));
    int dx = point.x - ((clientRECT.right-clientRECT.left)/2);

    if (dx >=0 ) {

        _mouse_angle = atan2((float)dx, (float)dy);
    } else {
        _mouse_angle = PI*2 - atan2((float)-dx, (float)dy);
    }

    this->Invalidate();
}

int CScanView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    _last_update_ts = 0;
    _scan_speed = 0;
    _sample_counter = 0;
    return 0;//CScrollWindowImpl<CPeakgrabberView>::OnCreate(lpCreateStruct);
}
void CScanView::OnPaint(CDCHandle dc)
{
    if (dc) 
    {
        onDrawSelf(dc);
    }
    else
    {
        CPaintDC tDC(m_hWnd);
        onDrawSelf(tDC.m_hDC);
    }
}

BOOL CScanView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    _current_display_range+=zDelta;
    if (_current_display_range > DISP_FULL_DIST) _current_display_range= DISP_FULL_DIST;
    else if (_current_display_range < DISP_MIN_DIST) _current_display_range = DISP_MIN_DIST;
    this->Invalidate();
    return 0;
}

void CScanView::setScanData(sl_lidar_response_measurement_node_hq_t *buffer, size_t count, float sampleDuration)
{
    _scan_data.clear();
    _is_scanning = true;
    for (int pos = 0; pos < (int)count; ++pos) {
        scanDot dot;
        if (!buffer[pos].dist_mm_q2) continue;

        dot.quality = buffer[pos].quality;
        dot.angle = buffer[pos].angle_z_q14 * 90.f / 16384.f;
        dot.dist = buffer[pos].dist_mm_q2 /4.0f;
        _scan_data.push_back(dot);
    }

    _sample_duration = sampleDuration;
    _scan_speed = 1000000.0f / (count * sampleDuration);
    this->Invalidate();
}

void CScanView::stopScan()
{
    _is_scanning = false;
    this->Invalidate();
}
