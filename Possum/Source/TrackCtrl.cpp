// source\TrackCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "Possum.h"
#include "TrackCtrl.h"


// CTrackCtrl

IMPLEMENT_DYNAMIC(CTrackCtrl, CWnd)

CTrackCtrl::CTrackCtrl()
{
	m_StartTime = 0.0;
	m_EndTime = 60.0;
}

CTrackCtrl::~CTrackCtrl()
{
}

void CTrackCtrl::SetTimeRange(double start_time, double end_time)
{

}

void CTrackCtrl::GetTimeRange(double &start_time, double &end_time)
{

}


BEGIN_MESSAGE_MAP(CTrackCtrl, CWnd)
END_MESSAGE_MAP()



// CTrackCtrl message handlers


