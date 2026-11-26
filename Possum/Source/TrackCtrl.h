#pragma once


// CTrackCtrl

class CTrackCtrl : public CWnd
{
	DECLARE_DYNAMIC(CTrackCtrl)

public:
	CTrackCtrl();
	virtual ~CTrackCtrl();

	void SetTimeRange(double start_time, double end_time);
	void GetTimeRange(double &start_time, double &end_time);

protected:
	// time in seconds
	double m_StartTime, m_EndTime;


	DECLARE_MESSAGE_MAP()
};


