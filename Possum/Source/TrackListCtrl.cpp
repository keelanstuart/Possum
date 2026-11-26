#include "stdafx.h"
#include "Possum.h"
#include "TrackListCtrl.h"
#include "Util.h"
#include "PossumFrame.h"
#include "PossumDocView.h"
#include "PossumDocument.h"
#include "PossumEffects.h"
#include <Nucleus.h>
#include <NuDeviceManager.h>
#include <NuDevice.h>
#include <NuTrack.h>
#include <NuCommandInfo.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <limits>

IMPLEMENT_DYNAMIC(CTrackListCtrl, CWnd)

CTrackListCtrl::ClipCommandList CTrackListCtrl::s_Clipboard;
size_t CTrackListCtrl::s_ClipboardFirstRow = 0;

namespace
{
	constexpr double MIDDLE_C = 261.63;
	constexpr int MIN_ROW_HEIGHT = 36;
	constexpr int MAX_ROW_HEIGHT = 2000;
	constexpr int ROW_RESIZE_GRAB = 5;
	constexpr int NOTE_HEIGHT = 13;
	constexpr int NOTE_RESIZE_GRAB = 5;
	constexpr int NOTE_RESIZE_MIN_WIDTH = NOTE_RESIZE_GRAB * 3;
	constexpr int DEFAULT_HEADER = 150;
	constexpr double SEMITONES = 12.0;
	constexpr double MIN_DURATION = 0.005;
	constexpr double TIME_SNAP = 0.5;
	constexpr int LAYER_BUTTON_SIZE = 10;
	constexpr int LAYER_BUTTON_SPACING = 3;
	constexpr int LAYER_BUTTON_LEFT = 8;
	constexpr int LAYER_BUTTON_BOTTOM = 7;
	constexpr int LAYER_BUTTON_MIN_HEIGHT = 36;
	constexpr int LAYER_CLEF_MIN_HEIGHT = 136;
	constexpr int LAYER_DESCRIPTION_MIN_HEIGHT = 85;
	constexpr int LAYER_BUTTON_BORDER = 1;
	constexpr int ENVELOPE_POINT_GRAB = 7;
	constexpr int ENVELOPE_HANDLE_GRAB = 6;
	constexpr float MIN_HANDLE_TIME = 0.001f;
	constexpr int DRAG_HINT_PADDING = 7;

	props::IProperty *Field(nucleus::ITrack* track, int id, const TCHAR* name)
	{
		props::IPropertySet* props = track->GetProperties();
		props::IProperty* p = props->GetPropertyById(id);

		return p ? p : props->CreateProperty(name, id);
	}

	double GetNumber(const nucleus::ITrack* track, int id, double fallback)
	{
		if (!track)
			return fallback;

		auto* p = track->GetProperties()->GetPropertyById(id);

		return p ? p->AsFloat() : fallback;
	}

	double GetBool(const nucleus::ITrack* track, int id, bool fallback)
	{
		if (!track)
			return fallback;

		auto* p = track->GetProperties()->GetPropertyById(id);

		return p ? p->AsBool() : fallback;
	}

	// AppendPropertySet is retained for any additional plugin-defined parameters.
	// Explicitly restore the Audio properties we know about so existing default
	// properties on a freshly created command cannot hide the pasted values.
	void RestoreAudioParams(props::IPropertySet* destination, const props::IPropertySet* source)
	{
		if (!destination || !source)
			return;

		// PATH must be set before DURA: Sample's property-change handler may
		// recompute duration when the sample filename changes.
		for (int id : { 'PATH', 'ICON', 'IN', 'OUT' })
		{
			auto* from = source->GetPropertyById(id);
			auto* to = destination->GetPropertyById(id);
			if (from && to)
				to->SetString(from->AsString());
		}

		for (int id : { 'FREQ', 'PMUL', 'CPNT', 'DURA', 'PEAK', 'SUSP', 'SUSB', 'SUSL', 'SUSF' })
		{
			auto* from = source->GetPropertyById(id);
			auto* to = destination->GetPropertyById(id);
			if (from && to)
				to->SetFloat(from->AsFloat());
		}

		if (auto* from = source->GetPropertyById('SHAP'))
			if (auto* to = destination->GetPropertyById('SHAP'))
				to->SetInt(from->AsInt());

		if (auto* from = source->GetPropertyById('PREP'))
			if (auto* to = destination->GetPropertyById('PREP'))
				to->SetBool(from->AsBool());

		if (auto* from = source->GetPropertyById('CVEC'))
			if (auto* to = destination->GetPropertyById('CVEC'))
				if (const auto* vector = from->AsVec2F())
					to->SetVec2F(*vector);
	}
}

BEGIN_MESSAGE_MAP(CTrackListCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_CONTEXTMENU()
	ON_WM_DESTROY()
	ON_COMMAND(ID_TRACK_INSERT_ABOVE, &CTrackListCtrl::OnTrackInsertAbove)
	ON_COMMAND(ID_TRACK_INSERT_BELOW, &CTrackListCtrl::OnTrackInsertBelow)
	ON_COMMAND(ID_TRACK_REMOVE, &CTrackListCtrl::OnTrackRemove)
	ON_COMMAND(ID_TRACK_DUPLICATE, &CTrackListCtrl::OnTrackDuplicate)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_CAPTURECHANGED()
	ON_WM_CANCELMODE()
	ON_WM_KEYDOWN()
	ON_WM_DROPFILES()
	ON_WM_SETFOCUS()
	ON_WM_SETCURSOR()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_EDIT_DUPLICATE, &CTrackListCtrl::OnEditDuplicate)
END_MESSAGE_MAP()

CTrackListCtrl::CTrackListCtrl() : m_MediaDropTarget(this)
{ }

BOOL CTrackListCtrl::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	const LPCTSTR name = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
	if (!CWnd::Create(name, nullptr, style, rect, parent, id))
		return FALSE;

	if (m_LayerTooltip.Create(this, TTS_ALWAYSTIP))
	{
		CRect client;
		GetClientRect(&client);

		m_LayerTooltip.AddTool(this, _T(""), &client, 1);
		m_LayerTooltip.SetDelayTime(TTDT_INITIAL, 1000);
		m_LayerTooltip.SetMaxTipWidth(300);
		m_LayerTooltip.Activate(FALSE);
	}

	// Create fonts once instead of allocating GDI fonts for each row / repaint.
	m_NameFont.CreatePointFont(96, _T("Arial"));
	m_DescriptionFont.CreatePointFont(72, _T("Arial"));
#ifdef _UNICODE
	m_ClefFont.CreatePointFont(340, _T("Segoe UI Symbol"));
#endif

	m_MediaDropTarget.Register(this);

	m_AudioDevice = theApp.m_pSys->DeviceManager()->GetDeviceByName(_T("Audio"));
	if (const nucleus::ICommandInfo *pci = m_AudioDevice->GetCommandByName(_T("StartAudition")))
	{
		m_AuditionStart = nucleus::ICommand::CreateCommand(pci);
	}
	if (const nucleus::ICommandInfo *pci = m_AudioDevice->GetCommandByName(_T("UpdateAudition")))
	{
		m_AuditionUpdate = nucleus::ICommand::CreateCommand(pci);
	}
	if (const nucleus::ICommandInfo *pci = m_AudioDevice->GetCommandByName(_T("StopAudition")))
	{
		m_AuditionStop = nucleus::ICommand::CreateCommand(pci);
	}

	return m_AudioDevice ? TRUE : FALSE;
}

CTrackListCtrl::~CTrackListCtrl()
{
	// Safe even when WM_DESTROY already released the drawing resources.
	ReleaseBackBuffer();

	if (m_AuditionSet)
		m_AuditionSet->Release();

	if (m_AuditionStart)
		m_AuditionStart->Release();

	if (m_AuditionUpdate)
		m_AuditionUpdate->Release();

	if (m_AuditionStop)
		m_AuditionStop->Release();
}

CPossumDocView* CTrackListCtrl::View() const
{
	return static_cast<CPossumDocView*>(GetParent());
}

CPossumDocument* CTrackListCtrl::Document() const
{
	return View()->GetDocument();
}

nucleus::ITrackSet* CTrackListCtrl::TrackSet() const
{
	auto* doc = Document();
	return doc ? doc->GetTrackSet() : nullptr;
}

CTrackListCtrl::GridConfig CTrackListCtrl::GetGrid(const nucleus::ITrack* track) const
{
	GridConfig c;

	if (!track)
		return c;

	c.height = std::clamp(int(GetNumber(track, 'RHGT', c.height)), MIN_ROW_HEIGHT, MAX_ROW_HEIGHT);
	c.minimum = GetNumber(track, 'VMIN', c.minimum);
	c.maximum = GetNumber(track, 'VMAX', c.maximum);
	c.interval = GetNumber(track, 'VGRD', c.interval);
	c.valueSnap = GetNumber(track, 'VSNP', c.valueSnap);
	c.timeSnap = GetNumber(track, 'TSNP', c.timeSnap);
	c.timeSnapEnabled = GetBool(track, 'TSEN', true) != 0.0;
	c.valueSnapEnabled = GetBool (track, 'VSEN', true) != 0.0;
	c.scale = GetNumber(track, 'VSCL', 1.0) != 0.0 ? Scale::Logarithmic : Scale::Linear;

	if (auto* p = track->GetProperties()->GetPropertyById('PRST'))
	{
		TClefGlyphMap::const_iterator git = g_PresetClefs.find(p->AsString());
		if (git != g_PresetClefs.cend())
			c.preset = git->second.c_str();
	}

	if (c.maximum <= c.minimum)
		c.maximum = c.minimum + 1.0;

	if (c.scale == Scale::Logarithmic && c.minimum <= 0.0)
		c.minimum = 1.0;

	return c;
}

void CTrackListCtrl::SetGrid(nucleus::ITrack* track, const GridConfig& c)
{
	if (!track)
		return;

	auto &undo = Document()->UndoHistory();
	auto before = CUndoManager::Snapshot(track->GetProperties());

	Field(track, 'RHGT', _T("Editor\\Row Height"))->SetInt(int64_t(std::clamp(c.height, MIN_ROW_HEIGHT, MAX_ROW_HEIGHT)));
	Field(track, 'VMIN', _T("Editor\\Vertical Minimum"))->SetFloat(float(c.minimum));
	Field(track, 'VMAX', _T("Editor\\Vertical Maximum"))->SetFloat(float(c.maximum));
	Field(track, 'VGRD', _T("Editor\\Grid Interval"))->SetFloat(float(c.interval));
	Field(track, 'VSNP', _T("Editor\\Snap Interval"))->SetFloat(float(c.valueSnap));
	Field(track, 'TSNP', _T("Editor\\Time Snap (s)"))->SetFloat(float(c.timeSnap));
	Field(track, 'TSEN', _T("Editor\\Time Snap Enabled"))->SetBool(c.timeSnapEnabled);
	Field(track, 'VSEN', _T("Editor\\Snap Enabled"))->SetBool(c.valueSnapEnabled);
	if (!PossumEffects::IsMaster(track))
	{
		Field(track, 'VSCL', _T("Editor\\Logarithmic Scale"))->SetBool(c.scale == Scale::Logarithmic);
		if (props::IProperty *pp = Field(track, 'PRST', _T("Editor\\Preset")))
		{
			pp->SetString(g_DefaultPreset.c_str());
			pp->Flags().Set(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
		}
	}

#if 0
	undo.Begin(_T("Edit Track Grid"));
	undo.RecordProperties(track->GetProperties(), before, CUndoManager::Snapshot(track->GetProperties()));
	undo.Commit();
#endif

	Document()->SetModifiedFlag();
	UpdateScroll();

	Invalidate(FALSE);
}

void CTrackListCtrl::SetRowHeight(nucleus::ITrack* track, int pixels)
{
	if (!track)
		return;

	auto c = GetGrid(track);

	int height = std::clamp<int>(pixels, MIN_ROW_HEIGHT, MAX_ROW_HEIGHT);

	if (height == c.height)
		return;

	double factor = double(height - 1) / double(c.height - 1);

	if (c.scale == Scale::Logarithmic)
	{
		double center = std::sqrt(c.minimum * c.maximum);
		double halfRange = std::log(c.maximum / c.minimum) * 0.5 * factor;

		c.minimum = center * std::exp(-halfRange);
		c.maximum = center * std::exp(halfRange);
	}
	else
	{
		double center = (c.minimum + c.maximum) * 0.5;
		double halfRange = (c.maximum - c.minimum) * 0.5 * factor;

		c.minimum = center - halfRange;
		c.maximum = center + halfRange;
	}

	c.height = height;

	SetGrid(track, c);
}

void CTrackListCtrl::SetHeaderWidth(int pixels)
{
	m_HeaderWidth = std::max<int>(80, pixels);

	Invalidate(FALSE);
}

CRect CTrackListCtrl::LayerButtonRect(int row, int layer) const
{
	int bottom = RowTop(row) + GetGrid(TrackSet()->GetTrack(row)).height;
	int left = LAYER_BUTTON_LEFT + layer * (LAYER_BUTTON_SIZE + LAYER_BUTTON_SPACING);
	int top = bottom - LAYER_BUTTON_BOTTOM - LAYER_BUTTON_SIZE;

	return CRect(left, top, left + LAYER_BUTTON_SIZE, top + LAYER_BUTTON_SIZE);
}

int CTrackListCtrl::HitLayerButton(CPoint point, int row) const
{
	if (row < 0 || !TrackSet() || !TrackSet()->GetTrack(row) ||
		GetGrid(TrackSet()->GetTrack(row)).height < LAYER_BUTTON_MIN_HEIGHT)
		return -1;

	for (int layer = 0; layer < PossumEffects::Count; ++layer)
		if (LayerButtonRect(row, layer).PtInRect(point))
			return layer;

	return -1;
}

void CTrackListCtrl::UpdateLayerTooltip(CPoint point)
{
	if (!m_LayerTooltip.GetSafeHwnd())
		return;

	int row = RowAt(point.y);
	int layer = point.x < m_HeaderWidth ? HitLayerButton(point, row) : -1;

	if (row == m_HoverLayerRow && layer == m_HoverLayer)
		return;

	m_HoverLayerRow = row;
	m_HoverLayer = layer;
	if (layer < 0)
	{
		m_LayerTooltip.Activate(FALSE);
		return;
	}

	const auto *track = TrackSet()->GetTrack(row);
	bool visible = (PossumEffects::Visible(track) & (1u << layer)) != 0;

	CString text;
	text.Format(_T("%s - %s\nClick: show and edit this envelope\nShift+click: toggle visibility"),
				PossumEffects::Get(layer).label, visible ? _T("visible") : _T("hidden"));

	m_LayerTooltip.UpdateTipText(text, this, 1);
	m_LayerTooltip.Activate(TRUE);
}

BOOL CTrackListCtrl::PreTranslateMessage(MSG *message)
{
	if (m_LayerTooltip.GetSafeHwnd())
	{
		// Update the hovered tool before relaying this mouse event. Otherwise
		// the first event over a button is relayed while tips are inactive.
		if (message->hwnd == m_hWnd && message->message == WM_MOUSEMOVE)
		{
			CPoint point(GET_X_LPARAM(message->lParam), GET_Y_LPARAM(message->lParam));
			UpdateLayerTooltip(point);
		}

		m_LayerTooltip.RelayEvent(message);
	}

	return CWnd::PreTranslateMessage(message);
}

void CTrackListCtrl::RefreshTracks()
{
	UpdateScroll();
	Invalidate(FALSE);
}

bool CTrackListCtrl::AddTrack(const TCHAR* name)
{
	auto* ts = TrackSet();
	if (!ts || !name)
		return false;

	nucleus::ITrack* track = ts->GetTrackByName(name);
	bool existed = track != nullptr;
	if (!track && ts->CreateTrack(&track, name, nullptr) != nucleus::RetCode::OK)
		return false;

	if (track)
	{
		// Only a newly created track is an insertion transaction.
		if (!existed)
		{
			PossumEffects::InitializeTrack(track, false);
			PossumEffects::SetVisible(track, theApp.m_EffectDefaults.visible);
			PossumEffects::SetActive(track, theApp.m_EffectDefaults.active);

			auto &undo = Document()->UndoHistory();

			undo.Begin(_T("Add Track"));
			undo.RecordTrack(track, { }, {ts, ts->GetTrackIndex(track)});
			undo.Commit();

			Document()->SetModifiedFlag();
		}

		UpdateScroll();
		Invalidate(FALSE);
		theApp.RefreshCommandList(true);

		return true;
	}

	return false;
}

bool CTrackListCtrl::DeleteTrack(nucleus::ITrack* track)
{
	if (!track || !CanEdit() || PossumEffects::IsMaster(track))
		return false;

	auto &undo = Document()->UndoHistory();
	undo.Begin(_T("Delete Track"));
	Document()->ClearSelection();

	auto *ts = TrackSet();
	size_t index = ts->GetTrackIndex(track);
	if (ts->RemoveTrack(track, false) != nucleus::RetCode::OK)
	{
		undo.Cancel();
		return false;
	}

	undo.RecordTrack(track, {ts, index}, { });
	undo.Commit();

	Document()->SetModifiedFlag();
	RefreshTracks();
	theApp.RefreshCommandList(true);

	return true;
}

int CTrackListCtrl::RowTop(int row) const
{
	int y = -m_ScrollY;
	auto* ts = TrackSet();

	if (ts)
	{
		for (int i = 0; i < row && i < int(ts->GetNumTracks()); i++)
			y += GetGrid(ts->GetTrack(i)).height;
	}

	return y;
}

int CTrackListCtrl::RowAt(int y) const
{
	auto* ts = TrackSet();
	if (!ts)
		return -1;

	int top = -m_ScrollY;

	for (int i = 0; i < int(ts->GetNumTracks()); i++)
	{
		top += GetGrid(ts->GetTrack(i)).height;
		if (y < top)
			return y >= -m_ScrollY ? i : -1;
	}

	return -1;
}

int CTrackListCtrl::TimelineWidth() const
{
	CRect r;
	GetClientRect(&r);

	return std::max<int>(1, r.Width() - m_HeaderWidth);
}

double CTrackListCtrl::TimeAt(int x) const
{
	double start = View()->GetDisplayStartTime();
	double end = View()->GetDisplayEndTime();

	return std::max<double>(0.0, start + (double(x - m_HeaderWidth) / TimelineWidth()) * (end - start));
}

int CTrackListCtrl::XAt(double t) const
{
	double start = View()->GetDisplayStartTime();
	double end = View()->GetDisplayEndTime();

	return m_HeaderWidth + int((t - start) / std::max<double>(0.0001, end - start) * TimelineWidth());
}

int CTrackListCtrl::YAt(double value, int row, const GridConfig& c) const
{
	double f = 0.5;

	if (c.scale == Scale::Logarithmic)
	{
		value = std::max<double>(0.00001, value);
		f = std::log(value / c.minimum) / std::log(c.maximum / c.minimum);
	}
	else
	{
		f = (value - c.minimum) / (c.maximum - c.minimum);
	}

	return RowTop(row) + int((1.0 - std::clamp(f, 0.0, 1.0)) * (c.height - 1));
}

double CTrackListCtrl::ValueAt(int y, int row, const GridConfig& c) const
{
	double f = std::clamp<double>(1.0 - double(y - RowTop(row)) / std::max<double>(1, c.height - 1), 0.0, 1.0);

	return c.scale == Scale::Logarithmic ? c.minimum * std::pow(c.maximum / c.minimum, f) : c.minimum + (c.maximum - c.minimum) * f;
}

double CTrackListCtrl::SnapValue(double value, const GridConfig& c) const
{
	if (!theApp.m_LayoutSnapEnabled || c.valueSnap <= 0)
		return value;

	if (c.scale == Scale::Logarithmic)
		return MIDDLE_C * std::exp2(std::round(std::log2(std::max<double>(value, 0.00001) / MIDDLE_C) * SEMITONES / c.valueSnap) * c.valueSnap / SEMITONES);

	return std::round(value / c.valueSnap) * c.valueSnap;
}

double CTrackListCtrl::CommandValue(const nucleus::ICommand* cmd) const
{
	auto* params = cmd->GetParams();

	if (auto* p = params->GetPropertyById('FREQ'))
		return p->AsFloat();

	if (auto* p = params->GetPropertyById('PMUL'))
		return double(p->AsFloat()) * MIDDLE_C;

	return MIDDLE_C;
}

void CTrackListCtrl::SetCommandValue(nucleus::ICommand* cmd, double value) const
{
	auto* params = cmd->GetParams();
	if (auto* p = params->GetPropertyById('FREQ'))
	{
		p->SetFloat(float(value));
	}
	else if (auto* p = params->GetPropertyById('PMUL'))
	{
		p->SetFloat(float(value / MIDDLE_C));
	}
}

CRect CTrackListCtrl::CommandRect(int row, nucleus::ICommand* cmd, double t, const GridConfig& c) const
{
	int y = YAt(CommandValue(cmd), row, c);
	int x1 = XAt(t);
	int x2 = XAt(t + std::max<double>(MIN_DURATION, cmd->GetDuration()));

	return CRect(x1, y - NOTE_HEIGHT / 2, std::max<int>(x1 + 5, x2), y + NOTE_HEIGHT / 2);
}

CTrackListCtrl::ResizeEdge CTrackListCtrl::CommandResizeEdge(CPoint point, int row, nucleus::ICommand *command, double time) const
{
	const CRect rect = CommandRect(row, command, time, GetGrid(TrackSet()->GetTrack(row)));

	if (!rect.PtInRect(point) || rect.Width() < NOTE_RESIZE_MIN_WIDTH)
		return ResizeEdge::None;

	if (point.x - rect.left < NOTE_RESIZE_GRAB)
		return ResizeEdge::Left;

	if (rect.right - point.x <= NOTE_RESIZE_GRAB)
		return ResizeEdge::Right;

	return ResizeEdge::None;
}

double CTrackListCtrl::ResizeDelta(CPoint point) const
{
	if (!m_ResizeTrack || m_ResizeEdge == ResizeEdge::None ||
		std::abs(point.x - m_Down.x) <= 2)
		return 0.0;

	auto grid = GetGrid(m_ResizeTrack);
	double anchor = m_ResizeStartTime + (m_ResizeEdge == ResizeEdge::Right ? m_ResizeStartDuration : 0.0);
	double edge = anchor + TimeAt(point.x) - TimeAt(m_Down.x);

	if (theApp.m_LayoutSnapEnabled && grid.timeSnap > 0.0)
		edge = std::round(edge / grid.timeSnap) * grid.timeSnap;

	double delta = edge - anchor;
	for (const auto &entry : m_ResizeCommands)
	{
		if (m_ResizeEdge == ResizeEdge::Left)
		{
			delta = std::max<double>(delta, -entry.time);
			delta = std::min<double>(delta, entry.duration - MIN_DURATION);
		}
		else
			delta = std::max<double>(delta, MIN_DURATION - entry.duration);
	}

	return delta;
}

void CTrackListCtrl::ResizePlacement(CPoint point, double &time, double &duration) const
{
	double delta = ResizeDelta(point);
	time = m_ResizeStartTime + (m_ResizeEdge == ResizeEdge::Left ? delta : 0.0);
	duration = m_ResizeStartDuration + (m_ResizeEdge == ResizeEdge::Left ? -delta : delta);
}

bool CTrackListCtrl::BeginScale(nucleus::ICommand *grabbed, nucleus::ITrack *grabTrack, double handleTime, ResizeEdge edge)
{
	if (!grabbed || !grabTrack || !CanEdit() || !Document()->IsSelected(grabbed))
		return false;

	auto *doc = Document();
	m_ScaleCommands.clear();
	m_ScaleCommands.reserve(doc->GetNumSelected());

	double first = DBL_MAX;
	double last = -DBL_MAX;

	for (size_t i = 0; i < doc->GetNumSelected(); ++i)
	{
		auto *cmd = doc->GetSelection(i);
		auto *track = cmd ? const_cast<nucleus::ITrack *>(cmd->GetOwner()) : nullptr;
		if (!track)
		{
			m_ScaleCommands.clear();
			return false;
		}

		double time = 0.0;
		if (!track->FindCommand(cmd, &time))
		{
			m_ScaleCommands.clear();
			return false;
		}

		const bool envelope = PossumEffects::FindLayer(cmd, PossumEffects::IsMaster(track)) >= 0;
		const bool resizable = !envelope && cmd->GetParams()->GetPropertyById('DURA') != nullptr;
		const double duration = envelope ? 0.0 : cmd->GetDuration();

		first = std::min<double>(first, time);
		last = std::max<double>(last, time + duration);
		m_ScaleCommands.push_back({cmd, track, time, duration, time, duration, resizable});
	}

	// An isolated envelope point has no time extent to stretch.
	if (m_ScaleCommands.empty() || last - first <= 0.0000001)
	{
		m_ScaleCommands.clear();
		return false;
	}

	m_ScaleStart = first;
	m_ScaleEnd = last;
	m_ScaleHandleTime = handleTime;

	// Pick the stationary endpoint ONCE, when the gesture begins. Choosing it
	// from the direction of each mouse move made both drag directions stretch
	// interior handles (factor > 1) and prevented compression.
	constexpr double epsilon = 0.0000001;
	if (std::abs(handleTime - first) <= epsilon)
		m_ScaleAnchor = last;
	else if (std::abs(handleTime - last) <= epsilon)
		m_ScaleAnchor = first;
	else if (edge == ResizeEdge::Left)
		m_ScaleAnchor = last;
	else if (edge == ResizeEdge::Right)
		m_ScaleAnchor = first;
	else
		// Envelope point: anchor at the farther endpoint of the selection.
		m_ScaleAnchor = handleTime - first >= last - handleTime ? first : last;


	m_ScaleSnap = GetGrid(grabTrack).timeSnap;
	m_ScaleFactor = 1.0;
	m_Scaling = true;
	return true;
}

void CTrackListCtrl::UpdateScale(CPoint point)
{
	if (!m_Scaling)
		return;

	const double delta = TimeAt(point.x) - TimeAt(m_Down.x);
	double factor = 1.0;
	double anchor = m_ScaleAnchor;

	if (std::abs(point.x - m_Down.x) > 2)
	{
		double originalSpan = m_ScaleHandleTime - anchor;
		if (std::abs(originalSpan) < 0.0000001)
			return;

		double movingTime = m_ScaleHandleTime + delta;
		if (theApp.m_LayoutSnapEnabled && m_ScaleSnap > 0.0)
			movingTime = std::round(movingTime / m_ScaleSnap) * m_ScaleSnap;

		factor = (movingTime - anchor) / originalSpan;

		// Every duration-bearing command must stay at least MIN_DURATION long.
		double minimumFactor = 0.000001;
		for (const auto &entry : m_ScaleCommands)
			if (entry.resizable && entry.duration > 0.0)
				minimumFactor = std::max<double>(minimumFactor, MIN_DURATION / entry.duration);

		// Stretching leftwards cannot move the first selected command before 0.
		double maximumFactor = 1000000.0;
		if (anchor > m_ScaleStart && m_ScaleStart < anchor)
			maximumFactor = std::min<double>(maximumFactor, anchor / (anchor - m_ScaleStart));

		if (minimumFactor > maximumFactor)
			return; // This selection cannot fit within the timeline constraints.
		factor = std::clamp(factor, minimumFactor, maximumFactor);
	}

	// Compute all destinations from the snapshot, never from the previous
	// mousemove; otherwise repeated scaling introduces cumulative errors.
	for (auto &entry : m_ScaleCommands)
	{
		entry.targetTime = std::max<double>(0.0, anchor + (entry.time - anchor) * factor);
		entry.targetDuration = entry.resizable ? entry.duration * factor : entry.duration;
	}

	// Validate as a group against all *unselected* commands.
	const auto *selection = Document()->SelectionSet();
	for (const auto &entry : m_ScaleCommands)
	{
		if (entry.resizable && entry.targetDuration < MIN_DURATION - 0.0000001)
			return;

		const double duration = entry.resizable ? entry.targetDuration : entry.command->GetDuration();
		if (!entry.track->CanFit(entry.targetTime, duration, entry.command, selection))
			return;
	}

	// CanFit intentionally ignores other selected commands, so also validate
	// transformed commands against each other when a track disallows overlap.
	for (size_t i = 0; i < m_ScaleCommands.size(); ++i)
	{
		const auto &a = m_ScaleCommands[i];
		const auto *allowOverlap = a.track->GetProperties()->GetPropertyById('ACOL');
		if (!allowOverlap || allowOverlap->AsBool())
			continue;

		for (size_t j = i + 1; j < m_ScaleCommands.size(); ++j)
		{
			const auto &b = m_ScaleCommands[j];
			if (a.track != b.track)
				continue;

			const double ad = a.resizable ? a.targetDuration : a.command->GetDuration();
			const double bd = b.resizable ? b.targetDuration : b.command->GetDuration();
			if (ad > 0.0 && bd > 0.0 &&
				a.targetTime < b.targetTime + bd - 0.000000001 &&
				b.targetTime < a.targetTime + ad - 0.000000001)
				return;
		}
	}

	// Same live-editing approach as m_Dragging: move the real commands now and
	// record the full before/after state as one undo transaction on mouse-up.
	for (auto &entry : m_ScaleCommands)
	{
		double currentTime = 0.0;
		if (!entry.track->FindCommand(entry.command, &currentTime))
			continue;

		const double currentDuration = entry.command->GetDuration();
		if (std::abs(currentTime - entry.targetTime) < 0.0000001 &&
			(!entry.resizable || std::abs(currentDuration - entry.targetDuration) < 0.0000001))
			continue;

		const double *newDuration = entry.resizable ? &entry.targetDuration : nullptr;
		entry.track->MoveCommand(entry.command, entry.targetTime, newDuration, selection);
	}

	m_ScaleFactor = factor;
	Invalidate(FALSE);
}

void CTrackListCtrl::EndScale(bool commit)
{
	if (!m_Scaling)
		return;

	auto *doc = Document();
	if (commit && CanEdit())
	{
		auto &undo = doc->UndoHistory();
		undo.Begin(_T("Scale Selection"));
		bool changed = false;

		for (const auto &entry : m_ScaleCommands)
		{
			double currentTime = entry.time;
			if (!entry.track->FindCommand(entry.command, &currentTime))
				continue;

			const double currentDuration = entry.command->GetDuration();
			const bool moved = std::abs(currentTime - entry.time) > 0.0000001;
			const bool resized = entry.resizable &&
				std::abs(currentDuration - entry.duration) > 0.0000001;
			if (!moved && !resized)
				continue;

			const double beforeDuration = entry.resizable ? entry.duration : -1.0;
			const double afterDuration = entry.resizable ? currentDuration : -1.0;
			undo.RecordCommand(entry.command,
							   {entry.track, entry.time, beforeDuration},
							   {entry.track, currentTime, afterDuration});
			changed = true;
		}

		if (changed)
		{
			undo.Commit();
			doc->SetModifiedFlag();
			doc->UpdatePropertiesForSelection();
			theApp.RefreshCommandList(false);
		}
		else
			undo.Cancel();
	}
	else
	{
		// Capture loss / Esc: return to the mousedown snapshot without Undo.
		const auto *selection = doc->SelectionSet();
		for (auto &entry : m_ScaleCommands)
		{
			const double *duration = entry.resizable ? &entry.duration : nullptr;
			entry.track->MoveCommand(entry.command, entry.time, duration, selection);
		}
	}

	m_Scaling = false;
	m_ScaleCommands.clear();
	m_ScaleFactor = 1.0;
	Invalidate(FALSE);
}

CTrackListCtrl::Hit CTrackListCtrl::HitTest(CPoint p) const
{
	Hit hit;

	hit.row = RowAt(p.y);
	if (hit.row < 0)
		return hit;

	hit.track = TrackSet()->GetTrack(hit.row);
	hit.header = p.x < m_HeaderWidth;
	hit.time = TimeAt(p.x);
	if (hit.header)
		return hit;

	auto cfg = GetGrid(hit.track);

	// Back-to-front, so more recently inserted overlapping commands are clickable.
	for (size_t i = hit.track->GetNumCommands(); i > 0; i--)
	{
		double t;

		auto* cmd = hit.track->GetCommand(i - 1, &t);

		if (cmd && PossumEffects::FindLayer(cmd, PossumEffects::IsMaster(hit.track)) < 0 &&
			CommandRect(hit.row, cmd, t, cfg).PtInRect(p))
		{
			hit.cmd = cmd;
			break;
		}
	}

	return hit;
}

void CTrackListCtrl::UpdateScroll()
{
	if (!GetSafeHwnd())
		return;

	CRect r;
	GetClientRect(r);

	int total = 0;
	if (auto* ts = TrackSet())
	{
		for (size_t i = 0; i < ts->GetNumTracks(); i++)
			total += GetGrid(ts->GetTrack(i)).height;
	}

	m_ScrollY = std::clamp<int>(m_ScrollY, 0, std::max<int>(0, total - r.Height()));

	SCROLLINFO si{ };

	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = std::max<int>(0, total - 1);
	si.nPage = UINT(std::max<int>(1, r.Height()));
	si.nPos = m_ScrollY;

	SetScrollInfo(SB_VERT, &si, TRUE);
}

bool CTrackListCtrl::CanEdit() const
{
	if (!theApp.m_Player)
		return true;

	// Structural edits are deliberately forbidden while the player is active.
	return theApp.m_Player->GetPlayState() == nucleus::IPlayer::State::Stopped;
}

BOOL CTrackListCtrl::OnEraseBkgnd(CDC*)
{
	return TRUE;
}

int CTrackListCtrl::EnvelopeY(double value, int row, int layer) const
{
	const auto &def = PossumEffects::Get(layer);

	int top = RowTop(row), height = GetGrid(TrackSet()->GetTrack(row)).height;
	double frac = (value - def.minimum) / (def.maximum - def.minimum);

	return top + 6 + int((1.0 - std::clamp<double>(frac, 0.0, 1.0)) * std::max<int>(1, height - 13));
}

double CTrackListCtrl::EnvelopeValue(int y, int row, int layer) const
{
	const auto &def = PossumEffects::Get(layer);

	int top = RowTop(row), height = GetGrid(TrackSet()->GetTrack(row)).height;
	double frac = std::clamp<double>(double(y - top - 6) / std::max<int>(1, height - 13), 0.0, 1.0);

	return def.maximum - frac * (def.maximum - def.minimum);
}

CPoint CTrackListCtrl::EnvelopeHandlePosition(const nucleus::ICommand *command, double time, int row, int layer) const
{
	double value = 0.0;
	props::TVec2F vector(1.0f, 0.0f);

	if (auto *property = command->GetParams()->GetPropertyById('CPNT'))
		value = property->AsFloat();

	if (auto *property = command->GetParams()->GetPropertyById('CVEC'))
		if (const auto *stored = property->AsVec2F())
			vector = *stored;

	return CPoint(XAt(time + std::max<double>(MIN_HANDLE_TIME, vector.x)),
				  EnvelopeY(value + vector.y, row, layer));
}

nucleus::ICommand *CTrackListCtrl::HitEnvelopePoint(CPoint point, int row, nucleus::ITrack *track, bool &handle) const
{
	handle = false;
	const bool global = PossumEffects::IsMaster(track);
	const int active = PossumEffects::Active(track);

	if (!(PossumEffects::Visible(track) & (1u << active)))
		return nullptr;

	// Pick tangent handles before any control points, including overlapping ones.
	for (size_t i = track->GetNumCommands(); i > 0; --i)
	{
		double time = 0.0;
		auto *cmd = track->GetCommand(i - 1, &time);
		if (PossumEffects::FindLayer(cmd, global) != active)
			continue;

		CPoint hp = EnvelopeHandlePosition(cmd, time, row, active);
		if (std::abs(hp.x - point.x) <= ENVELOPE_HANDLE_GRAB &&
			std::abs(hp.y - point.y) <= ENVELOPE_HANDLE_GRAB)
		{
			handle = true;
			return cmd;
		}
	}

	for (size_t i = track->GetNumCommands(); i > 0; --i)
	{
		double time = 0.0;
		auto *cmd = track->GetCommand(i - 1, &time);

		if (PossumEffects::FindLayer(cmd, global) != active)
			continue;

		auto *p = cmd->GetParams()->GetPropertyById('CPNT');
		if (p && std::abs(XAt(time) - point.x) <= ENVELOPE_POINT_GRAB &&
			std::abs(EnvelopeY(p->AsFloat(), row, active) - point.y) <= ENVELOPE_POINT_GRAB)
			return cmd;
	}

	return nullptr;
}

void CTrackListCtrl::DrawEnvelopes(CDC &dc, nucleus::ITrack *track, int top, const GridConfig &cfg,
								   const CRect &client, double displayStart, double displayEnd)
{
	const bool global = PossumEffects::IsMaster(track);
	const int active = PossumEffects::Active(track);
	const unsigned visible = PossumEffects::Visible(track);
	if (!visible)
		return;

	// Single pass through the track rather than scanning it once per layer.
	for (auto &knots : m_EnvelopeKnots)
		knots.clear();

	const size_t commandCount = track->GetNumCommands();
	for (size_t i = 0; i < commandCount; i++)
	{
		double time = 0.0;
		auto *cmd = track->GetCommand(i, &time);
		if (!cmd)
			continue;

		const int layer = PossumEffects::FindLayer(cmd, global);
		if (layer < 0 || layer >= PossumEffects::Count || !(visible & (1u << layer)))
			continue;

		auto *params = cmd->GetParams();
		auto *value = params->GetPropertyById('CPNT');
		if (!value)
			continue;

		double slope = 0.0;
		if (auto *tangent = params->GetPropertyById('CVEC'))
			if (auto *vector = tangent->AsVec2F())
				if (vector->x != 0.0f)
					slope = double(vector->y) / vector->x;

		m_EnvelopeKnots[layer].push_back({time, value->AsFloat(), slope, cmd});
	}

	const int timelineWidth = std::max<int>(1, client.Width() - m_HeaderWidth);
	const double secondsPerPixel = (displayEnd - displayStart) / timelineWidth;
	const double pixelsPerSecond = double(timelineWidth) / std::max<double>(0.0001, displayEnd - displayStart);
	const auto toX = [&](double time) -> int
	{
		return m_HeaderWidth + int((time - displayStart) * pixelsPerSecond);
	};

	const int x0 = std::max<int>(m_HeaderWidth, client.left);
	const int xEnd = client.right;
	auto *doc = Document();
	auto *trackSet = TrackSet();

	for (int layer = 0; layer < PossumEffects::Count; layer++)
	{
		auto &knots = m_EnvelopeKnots[layer];
		if (knots.empty())
			continue;

		// Preserve chronological interpolation without assuming the track's
		// storage order is guaranteed to be chronological.
		std::sort(knots.begin(), knots.end(), [](const EnvelopeKnot &a, const EnvelopeKnot &b)
		{
			return a.time < b.time;
		});

		const auto &def = PossumEffects::Get(layer);
		const double inverseRange = 1.0 / (def.maximum - def.minimum);
		const int span = std::max<int>(1, cfg.height - 13);
		const auto toY = [&](double value) -> int
		{
			const double f = std::clamp<double>((value - def.minimum) * inverseRange, 0.0, 1.0);
			return top + 6 + int((1.0 - f) * span);
		};

		COLORREF color = PossumEffects::Color(trackSet, layer);
		if (layer != active)
		{
			const COLORREF bg = RGB(39, 41, 46);
			color = RGB((GetRValue(color) + 2 * GetRValue(bg)) / 3,
						(GetGValue(color) + 2 * GetGValue(bg)) / 3,
						(GetBValue(color) + 2 * GetBValue(bg)) / 3);
		}

		CPen pen(PS_SOLID, layer == active ? 2 : 1, color);
		auto *old = dc.SelectObject(&pen);

		// Before the first control point Audio returns its initial value once
		// the effect command is scheduled; draw only from that point onward.
		for (size_t k = 0; k < knots.size(); k++)
		{
			const EnvelopeKnot &a = knots[k];
			const bool last = k + 1 == knots.size();
			const EnvelopeKnot &b = last ? a : knots[k + 1];

			const int start = std::max<int>(x0, toX(a.time));
			const int end = last ? xEnd : std::min<int>(xEnd, toX(b.time));
			if (end < start)
				continue;

			const double dt = b.time - a.time;
			for (int x = start; x <= end; x += 3)
			{
				double value = a.value;
				if (!last && dt > 0.0)
				{
					const double t = std::max<double>(0.0, displayStart + (x - m_HeaderWidth) * secondsPerPixel);
					const double u = std::clamp<double>((t - a.time) / dt, 0.0, 1.0);
					const double u2 = u * u, u3 = u2 * u;
					value = (2*u3 - 3*u2 + 1)*a.value + (u3 - 2*u2 + u)*dt*a.slope
						+ (-2*u3 + 3*u2)*b.value + (u3-u2)*dt*b.slope;
				}

				if (x == start)
					dc.MoveTo(x, toY(value));
				else
					dc.LineTo(x, toY(value));
			}

			if (end > start)
				dc.LineTo(end, toY(last ? a.value : b.value));
		}

		dc.SelectObject(old);
		if (layer == active)
		{
			CPen tangentPen(PS_DOT, 1, color);
			auto *previous = dc.SelectObject(&tangentPen);
			for (const auto &point : knots)
			{
				const int x = toX(point.time);
				const int y = toY(point.value);
				if (x < x0 || x >= xEnd)
					continue;

				props::TVec2F vector(1.0f, 0.0f);
				if (auto *property = point.command->GetParams()->GetPropertyById('CVEC'))
					if (const auto *stored = property->AsVec2F())
						vector = *stored;

				const int hx = toX(point.time + std::max<double>(MIN_HANDLE_TIME, vector.x));
				const int hy = toY(point.value + vector.y);
				if (hx < xEnd)
				{
					dc.MoveTo(x, y);
					dc.LineTo(hx, hy);
					dc.FillSolidRect(hx - 3, hy - 3, 7, 7, color);
				}

				dc.FillSolidRect(x - 4, y - 4, 9, 9, color);
				dc.Draw3dRect(x - 4, y - 4, 9, 9, RGB(245, 245, 245), RGB(15, 15, 15));

				if (doc->IsSelected(point.command))
					dc.Draw3dRect(x - 7, y - 7, 15, 15, RGB(255, 255, 255), RGB(255, 255, 255));
			}
			dc.SelectObject(previous);
		}
	}
}

bool CTrackListCtrl::EnsureBackBuffer(CDC &paint, CSize size)
{
	if (size.cx <= 0 || size.cy <= 0)
		return false;

	if (!m_BackDC.GetSafeHdc() && !m_BackDC.CreateCompatibleDC(&paint))
		return false;

	if (m_BackBitmap.GetSafeHandle() && m_BackSize.cx == size.cx && m_BackSize.cy == size.cy)
		return true;

	// A bitmap must be deselected before it can be deleted or replaced.
	if (m_OriginalBackBitmap)
	{
		::SelectObject(m_BackDC.GetSafeHdc(), m_OriginalBackBitmap);
		m_OriginalBackBitmap = nullptr;
	}
	if (m_BackBitmap.GetSafeHandle())
		m_BackBitmap.DeleteObject();
	m_BackSize = CSize(0, 0);

	if (!m_BackBitmap.CreateCompatibleBitmap(&paint, size.cx, size.cy))
		return false;

	HGDIOBJ old = ::SelectObject(m_BackDC.GetSafeHdc(), m_BackBitmap.GetSafeHandle());
	if (!old || old == HGDI_ERROR)
	{
		m_BackBitmap.DeleteObject();
		return false;
	}

	m_OriginalBackBitmap = old;
	m_BackSize = size;
	return true;
}

void CTrackListCtrl::ReleaseBackBuffer()
{
	if (m_BackDC.GetSafeHdc() && m_OriginalBackBitmap)
		::SelectObject(m_BackDC.GetSafeHdc(), m_OriginalBackBitmap);

	m_OriginalBackBitmap = nullptr;
	if (m_BackBitmap.GetSafeHandle())
		m_BackBitmap.DeleteObject();
	if (m_BackDC.GetSafeHdc())
		m_BackDC.DeleteDC();
	m_BackSize = CSize(0, 0);
}

void CTrackListCtrl::OnPaint()
{
	CPaintDC paint(this);

	CRect client;
	GetClientRect(&client);
	if (client.IsRectEmpty())
		return;

	// Reuse the memory DC and bitmap. If allocation fails, still paint directly.
	const bool buffered = EnsureBackBuffer(paint, client.Size());
	CDC &buffer = buffered ? m_BackDC : static_cast<CDC &>(paint);
	buffer.FillSolidRect(client, RGB(31, 33, 38));
	buffer.SetBkMode(TRANSPARENT);

	auto *ts = TrackSet();
	auto *doc = Document();
	auto *view = View();
	const double displayStart = view ? view->GetDisplayStartTime() : 0.0;
	const double displayEnd = view ? view->GetDisplayEndTime() : 1.0;
	const int timelineWidth = std::max<int>(1, client.Width() - m_HeaderWidth);
	const double pixelsPerSecond = double(timelineWidth) / std::max<double>(0.0001, displayEnd - displayStart);
	const auto toX = [&](double time) -> int
	{
		return m_HeaderWidth + int((time - displayStart) * pixelsPerSecond);
	};

	// Calculate the resize preview once and index the affected commands once.
	const bool resizing = m_ResizeCommand && m_ResizeEdge != ResizeEdge::None;
	const double resizeDelta = resizing ? ResizeDelta(m_Mouse) : 0.0;
	std::unordered_map<nucleus::ICommand *, const ResizeCommand *> resizeLookup;
	if (resizing)
	{
		resizeLookup.reserve(m_ResizeCommands.size());
		for (const auto &entry : m_ResizeCommands)
			resizeLookup.emplace(entry.command, &entry);
	}

	// Large selections benefit from O(1) membership checks per drawn command.
	const size_t selectionCount = doc ? doc->GetNumSelected() : 0;
	const bool useSelectionLookup = selectionCount > 8;
	std::unordered_set<const nucleus::ICommand *> selected;
	if (useSelectionLookup)
	{
		selected.reserve(selectionCount);
		for (size_t i = 0; i < selectionCount; i++)
			selected.insert(doc->GetSelection(i));
	}
	const auto isSelected = [&](const nucleus::ICommand *cmd) -> bool
	{
		return useSelectionLookup ? selected.find(cmd) != selected.end() : (doc && doc->IsSelected((nucleus::ICommand *)cmd));
	};

	if (ts)
	{
		int top = -m_ScrollY;
		for (int row = 0; row < int(ts->GetNumTracks()); row++)
		{
			auto *track = ts->GetTrack(row);
			const auto cfg = GetGrid(track);
			const int bottom = top + cfg.height;

			if (bottom < 0)
			{
				top = bottom;
				continue;
			}
			if (top > client.bottom)
				break;

			const bool global = PossumEffects::IsMaster(track);
			const double linearInverse = 1.0 / (cfg.maximum - cfg.minimum);
			const double logInverse = cfg.scale == Scale::Logarithmic
				? 1.0 / std::log(cfg.maximum / cfg.minimum) : 0.0;
			const auto toY = [&](double value) -> int
			{
				const double f = cfg.scale == Scale::Logarithmic
					? std::log(std::max<double>(0.00001, value) / cfg.minimum) * logInverse
					: (value - cfg.minimum) * linearInverse;
				return top + int((1.0 - std::clamp<double>(f, 0.0, 1.0)) * (cfg.height - 1));
			};

			CRect header(0, top, m_HeaderWidth, bottom);
			CRect timeline(m_HeaderWidth, top, client.right, bottom);
			buffer.FillSolidRect(header, RGB(49, 52, 58));
			buffer.FillSolidRect(timeline, row & 1 ? RGB(37, 39, 44) : RGB(40, 42, 47));

			if (cfg.interval > 0.0)
			{
				// Grid intervals are semitones in logarithmic mode and absolute units otherwise.
				if (cfg.scale == Scale::Logarithmic)
				{
					const int low = int(std::floor(SEMITONES * std::log2(cfg.minimum / MIDDLE_C) / cfg.interval));
					const int high = int(std::ceil(SEMITONES * std::log2(cfg.maximum / MIDDLE_C) / cfg.interval));
					if (high - low < 300)
						for (int i = low; i <= high; i++)
						{
							const double value = MIDDLE_C * std::exp2(i * cfg.interval / SEMITONES);
							const int y = toY(value);
							buffer.FillSolidRect(m_HeaderWidth, y, timeline.Width(), 1,
												 i == 0 ? RGB(95, 93, 73) : RGB(55, 57, 62));
						}
				}
				else
				{
					const int count = int((cfg.maximum - cfg.minimum) / cfg.interval);
					if (count < 300)
						for (int i = 0; i <= count; i++)
						{
							const int y = toY(cfg.minimum + i * cfg.interval);
							buffer.FillSolidRect(m_HeaderWidth, y, timeline.Width(), 1, RGB(55, 57, 62));
						}
				}
			}

			auto *enabled = track->GetProperties()->GetPropertyById('ENAB');
			const bool on = !enabled || enabled->AsBool();
			CRect check(8, top + 10, 23, top + 25);
			buffer.DrawFrameControl(check, DFC_BUTTON, DFCS_BUTTONCHECK | (on ? DFCS_CHECKED : 0));

			if (m_NameFont.GetSafeHandle())
			{
				CFont *previous = buffer.SelectObject(&m_NameFont);
				CRect nameRect(31, top + 8, m_HeaderWidth - 5, top + 30);
				buffer.SetTextColor(RGB(220, 223, 229));
				buffer.DrawText(track->GetName(), nameRect, DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER);
				buffer.SelectObject(previous);
			}

			if (cfg.height >= LAYER_DESCRIPTION_MIN_HEIGHT && m_DescriptionFont.GetSafeHandle())
			{
				CFont *previous = buffer.SelectObject(&m_DescriptionFont);
				CRect descRect(9, top + 31, m_HeaderWidth - 5, std::min<int>(bottom, top + 53));
				buffer.SetTextColor(RGB(160, 180, 170));
				buffer.DrawText(track->GetDescription(), descRect, DT_SINGLELINE | DT_END_ELLIPSIS);
				buffer.SelectObject(previous);
			}

#ifdef _UNICODE
			// Unicode clef glyphs require a suitable installed music font.
			if (!cfg.preset.IsEmpty() && cfg.height >= LAYER_CLEF_MIN_HEIGHT && m_ClefFont.GetSafeHandle())
			{
				CFont *previous = buffer.SelectObject(&m_ClefFont);
				CRect glyphRect(m_HeaderWidth - 43, top + 33, m_HeaderWidth - 4, bottom);
				buffer.SetTextColor(RGB(140, 159, 177));
				buffer.DrawText(cfg.preset, glyphRect, DT_SINGLELINE | DT_TOP | DT_CENTER);
				buffer.SelectObject(previous);
			}
#endif

			const size_t commandCount = track->GetNumCommands();
			for (size_t i = 0; i < commandCount; i++)
			{
				double time = 0.0;
				auto *cmd = track->GetCommand(i, &time);
				if (!cmd || PossumEffects::FindLayer(cmd, global) >= 0)
					continue;

				const int x1 = toX(time);
				const int x2 = toX(time + std::max<double>(MIN_DURATION, cmd->GetDuration()));
				// Avoid property and pitch calculations for offscreen notes.
				// Resizing may bring a previously offscreen note into view.
				if (!resizing && (x2 < m_HeaderWidth || x1 > client.right))
					continue;

				const int y = toY(CommandValue(cmd));
				CRect rc(x1, y - NOTE_HEIGHT / 2, std::max<int>(x1 + 5, x2), y + NOTE_HEIGHT / 2);

				if (resizing)
				{
					const auto it = resizeLookup.find(cmd);
					if (it != resizeLookup.end())
					{
						const auto &entry = *it->second;
						const double newTime = entry.time + (m_ResizeEdge == ResizeEdge::Left ? resizeDelta : 0.0);
						const double newDuration = entry.duration + (m_ResizeEdge == ResizeEdge::Left ? -resizeDelta : resizeDelta);
						rc.left = toX(newTime);
						rc.right = std::max<int>(rc.left + 5, toX(newTime + newDuration));
					}
				}

				if (rc.right < m_HeaderWidth || rc.left > client.right)
					continue;

				rc.left = std::max<int>(rc.left, m_HeaderWidth);
				buffer.FillSolidRect(rc, isSelected(cmd) ? RGB(239, 189, 52) : RGB(102, 166, 207));
				buffer.Draw3dRect(rc, RGB(225, 231, 237), RGB(15, 20, 26));
			}

			DrawEnvelopes(buffer, track, top, cfg, client, displayStart, displayEnd);

			if (cfg.height >= LAYER_BUTTON_MIN_HEIGHT)
			{
				const unsigned mask = PossumEffects::Visible(track);
				const int buttonTop = bottom - LAYER_BUTTON_BOTTOM - LAYER_BUTTON_SIZE;
				const int active = PossumEffects::Active(track);
				for (int layer = 0; layer < PossumEffects::Count; layer++)
				{
					const int left = LAYER_BUTTON_LEFT + layer * (LAYER_BUTTON_SIZE + LAYER_BUTTON_SPACING);
					CRect button(left, buttonTop, left + LAYER_BUTTON_SIZE, buttonTop + LAYER_BUTTON_SIZE);
					COLORREF color = PossumEffects::Color(ts, layer);
					if (!(mask & (1u << layer)))
						color = RGB(76, 78, 81);

					buffer.FillSolidRect(button, color);
					if (active == layer)
					{
						CRect outline = button;
						outline.InflateRect(LAYER_BUTTON_BORDER, LAYER_BUTTON_BORDER);
						buffer.Draw3dRect(outline, RGB(255, 255, 255), RGB(255, 255, 255));
					}
				}
			}

			buffer.FillSolidRect(0, bottom - 1, client.right, 1, RGB(88, 92, 101));
			top = bottom;
		}
	}

	if (view)
	{
		const int play = toX(view->GetPlayingTime());
		if (play >= m_HeaderWidth && play < client.right)
			buffer.FillSolidRect(play, 0, 2, client.Height(), RGB(64, 218, 113));

		if (view->IsShowingSelectedTime() && !m_BoxSelecting)
		{
			const int chosen = toX(view->GetSelectedTime());
			if (chosen >= m_HeaderWidth && chosen < client.right)
				buffer.FillSolidRect(chosen, 0, 1, client.Height(), RGB(210, 92, 196));
		}
	}

	if (m_BoxSelecting)
	{
		CRect r = m_SelectionRect;
		r.NormalizeRect();

		if (view->IsShowingSelectedTime())
		{
			buffer.FillSolidRect(r.left, 0, 1, client.Height(), RGB(210, 92, 196));
			buffer.FillSolidRect(r.right, 0, 1, client.Height(), RGB(210, 92, 196));
		}

		buffer.DrawFocusRect(r);
	}

	if (m_Scaling && std::abs(m_ScaleFactor - 1.0) > 0.000001)
	{
		CString hint;
		hint.Format(_T("Scale: %.1f%%"), m_ScaleFactor * 100.0);
		CRect label(0, 0, 0, 0);
		buffer.DrawText(hint, label, DT_CALCRECT | DT_SINGLELINE);
		label.InflateRect(DRAG_HINT_PADDING, DRAG_HINT_PADDING / 2);
		label.OffsetRect(m_Mouse.x + 14, m_Mouse.y - label.Height() - 10);
		label.OffsetRect(std::min<int>(0, client.right - label.right - 2),
						 std::max<int>(0, 2 - label.top));
		buffer.FillSolidRect(label, RGB(22, 25, 31));
		buffer.Draw3dRect(label, RGB(145, 151, 160), RGB(145, 151, 160));
		buffer.SetTextColor(RGB(246, 246, 240));
		buffer.DrawText(hint, label, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	}

	// Live note-drag feedback. Musical presets report semitones; generic grids report Hz.
	if (m_Dragging && m_DragStartRow >= 0 &&
		(std::abs(m_Mouse.x - m_Down.x) > 2 || std::abs(m_Mouse.y - m_Down.y) > 2))
	{
		auto *track = ts ? ts->GetTrack(m_DragStartRow) : nullptr;
		if (track)
		{
			const auto cfg = GetGrid(track);
			int originalY = YAt(m_DragStartValue, m_DragStartRow, cfg);
			double nextValue = std::clamp<double>(SnapValue(ValueAt(originalY + m_Mouse.y - m_Down.y,
																	m_DragStartRow, cfg), cfg), cfg.minimum, cfg.maximum);

			double dTime = TimeAt(m_Mouse.x) - TimeAt(m_Down.x);
			if (theApp.m_LayoutSnapEnabled)
				dTime = std::round(dTime / cfg.timeSnap) * cfg.timeSnap;

			CString hint;
			if (cfg.scale == Scale::Logarithmic && !cfg.preset.IsEmpty())
			{
				const int steps = m_DragStartValue > 0.0 && nextValue > 0.0 ?
					int(std::lround(SEMITONES * std::log2(nextValue / m_DragStartValue))) : 0;
				const int octaves = std::abs(steps) / 12;
				const int remainder = std::abs(steps) % 12;

				if (octaves && !remainder)
					hint.Format(_T("%+.3fs\n%+d octave%s"), dTime, steps / 12, octaves == 1 ? _T("") : _T("s"));
				else
					hint.Format(_T("%+.3fs\n%+d semitones"), dTime, steps);
			}
			else
				hint.Format(_T("%+.3fs\n%+.2f Hz"), dTime, nextValue - m_DragStartValue);

			CRect label(0, 0, 0, 0);
			buffer.DrawText(hint, label, DT_CALCRECT);
			label.InflateRect(DRAG_HINT_PADDING, DRAG_HINT_PADDING / 2);
			label.OffsetRect(m_Mouse.x + 14, m_Mouse.y - label.Height() - 10);
			label.OffsetRect(std::min<int>(0, client.right - label.right - 2), std::max<int>(0, 2 - label.top));

			buffer.FillSolidRect(label, RGB(22, 25, 31));
			buffer.Draw3dRect(label, RGB(145, 151, 160), RGB(145, 151, 160));
			buffer.SetTextColor(RGB(246, 246, 240));
			label.top += 2;
			buffer.DrawText(hint, label, DT_CENTER | DT_VCENTER);
		}
	}

	if (buffered)
		paint.BitBlt(0, 0, client.Width(), client.Height(), &buffer, 0, 0, SRCCOPY);
}

void CTrackListCtrl::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);

	if (m_LayerTooltip.GetSafeHwnd())
	{
		CRect client;
		GetClientRect(&client);
		m_LayerTooltip.SetToolRect(this, 1, &client);
	}

	UpdateScroll();
}

void CTrackListCtrl::OnVScroll(UINT code, UINT pos, CScrollBar* bar)
{
	SCROLLINFO si{ };
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);

	int y = m_ScrollY;
	switch (code)
	{
		case SB_LINEUP:			y -= 24;			break;
		case SB_LINEDOWN:		y += 24;			break;
		case SB_PAGEUP:			y -= int(si.nPage);	break;
		case SB_PAGEDOWN:		y += int(si.nPage);	break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:	y = si.nTrackPos;	break;

		default:									break;
	}

	m_ScrollY = std::clamp(y, 0, std::max<int>(0, si.nMax - int(si.nPage) + 1));

	UpdateScroll();
	Invalidate(FALSE);
	CWnd::OnVScroll(code, pos, bar);
}

BOOL CTrackListCtrl::OnMouseWheel(UINT flags, short delta, CPoint pt)
{
	ScreenToClient(&pt);

	constexpr double ZOOM_FACTOR = 1.2;
	constexpr double MIN_TIME_SPAN = 0.01;
	constexpr double MAX_TIME_SPAN = 86400.0;

	double steps = double(delta) / WHEEL_DELTA;
	double factor = std::pow(ZOOM_FACTOR, -steps);

	if (flags & MK_CONTROL)
	{
		if (flags & MK_SHIFT)
		{
			// Vertical zoom affects only the track beneath the cursor.
			const int row = RowAt(pt.y);

			if (row < 0)
				return TRUE;

			auto *track = TrackSet()->GetTrack(row);
			auto c = GetGrid(track);

			// Preserve the value beneath the mouse cursor.
			double anchor = ValueAt(pt.y, row, c);

			if (c.scale == Scale::Logarithmic)
			{
				double minimum = std::log(c.minimum);
				double maximum = std::log(c.maximum);
				double origin = std::log(anchor);

				c.minimum = std::exp(origin + (minimum - origin) * factor);
				c.maximum = std::exp(origin + (maximum - origin) * factor);
			}
			else
			{
				c.minimum = anchor + (c.minimum - anchor) * factor;
				c.maximum = anchor + (c.maximum - anchor) * factor;
			}

			if (std::isfinite(c.minimum) &&
				std::isfinite(c.maximum) &&
				c.maximum > c.minimum &&
				(c.scale != Scale::Logarithmic || c.minimum > 0.0))
			{
				SetGrid(track, c);
			}
		}
		else
		{
			// Horizontal zoom, anchored at the mouse position.
			auto *view = View();

			double start = view->GetDisplayStartTime();
			double end = view->GetDisplayEndTime();
			double span = end - start;

			if (span <= 0.0)
				return TRUE;

			double newSpan = std::clamp<double>(span * factor, MIN_TIME_SPAN, MAX_TIME_SPAN);

			double fraction = std::clamp<double>(double(pt.x - m_HeaderWidth) / TimelineWidth(), 0.0, 1.0);

			double anchor = start + span * fraction;

			// Time cannot extend below zero.
			double newStart = std::max<double>(0.0, anchor - newSpan * fraction);

			view->SetActiveTimelineExtents(newStart, newStart + newSpan);
		}

		return TRUE;
	}

	if (flags & MK_SHIFT)
	{
		auto* view = View();

		double start = view->GetDisplayStartTime();
		double end = view->GetDisplayEndTime();
		double span = end - start;

		// Move 10% of the visible timeline per wheel notch.
		double offset = steps * span * 0.1;
		double newStart = std::max<double>(0.0, start - offset);

		view->SetActiveTimelineExtents(newStart, newStart + span);

		// Keep the horizontal scrollbar synchronized (0.1 second units).
		view->SetScrollPos(SB_HORZ, int(newStart * 10.0), TRUE);

		return TRUE;
	}

	// Ordinary wheel: scroll vertically through the track rows.
	m_ScrollY = std::max<int>(0, m_ScrollY - int(steps * 48.0));

	UpdateScroll();
	Invalidate(FALSE);

	return TRUE;
}

void CTrackListCtrl::OnLButtonDown(UINT flags, CPoint p)
{
	SetFocus();
	m_Down = p;
	m_Mouse = p;

	auto hit = HitTest(p);
	if (!hit.track)
		return;

	if (hit.header)
	{
		int top = RowTop(hit.row);
		int layer = HitLayerButton(p, hit.row);
		if (layer >= 0)
		{
			const unsigned bit = 1u << layer;
			unsigned visible = PossumEffects::Visible(hit.track);
			if (flags & (MK_SHIFT | MK_CONTROL))
			{
				visible ^= bit;
				PossumEffects::SetVisible(hit.track, visible);
				// A newly enabled layer is the target for point insertion/editing.
				if (visible & bit)
					PossumEffects::SetActive(hit.track, layer);
			}
			else
			{
				// Selecting a hidden layer must also make it editable.
				PossumEffects::SetVisible(hit.track, visible | bit);
				PossumEffects::SetActive(hit.track, layer);
			}

			m_HoverLayer = -1; // Refresh the hint after a visibility toggle.
			UpdateLayerTooltip(p);
			Document()->SetModifiedFlag();
			Invalidate(FALSE);

			return; // Editor presentation state; never part of Undo.
		}

		int bottom = top + GetGrid(hit.track).height;
		if (std::abs(p.y - bottom) <= ROW_RESIZE_GRAB)
		{
			m_ResizingRow = true;
			m_ResizeRow = hit.row;
			m_ResizeHeight = GetGrid(hit.track).height;
			SetCapture();

			return;
		}

		if (p.x >= 5 && p.x <= 27 && p.y >= top + 7 && p.y <= top + 28)
		{
			auto* field = Field(hit.track, 'ENAB', _T("Enabled"));
			auto &undo = Document()->UndoHistory();
			auto before = CUndoManager::Snapshot(hit.track->GetProperties());

			field->SetBool(!field->AsBool());
			undo.Begin(_T("Toggle Track"));
			undo.RecordProperties(hit.track->GetProperties(), before, CUndoManager::Snapshot(hit.track->GetProperties()));
			undo.Commit();
			Document()->SetModifiedFlag();

			Invalidate(FALSE);
		}

		// Select the track properties rather than command properties.
		Document()->ClearSelection();

		CString title;
		title.Format(_T("Track: %s"), hit.track->GetName());

		theApp.SetActiveProperties(hit.track->GetProperties(), false, title);

		return;
	}

	if (!hit.header && CanEdit())
	{
		bool tangent = false;
		if (auto *point = HitEnvelopePoint(p, hit.row, hit.track, tangent))
		{
			auto *doc = Document();

			if (flags & MK_CONTROL)
			{
				if (doc->IsSelected(point)) doc->RemoveFromSelection(point);
				else doc->AddToSelection(point);
				Invalidate(FALSE);
				return;
			}

			if (!doc->IsSelected(point))
			{
				doc->ClearSelection();
				doc->AddToSelection(point);
			}

			// Z+dragging a selected control point stretches the whole selection.
			// Tangent handles retain their original editing behavior.
			if (!tangent && (GetKeyState('Z') & 0x8000))
			{
				double pointTime = 0.0;
				if (hit.track->FindCommand(point, &pointTime) && BeginScale(point, hit.track, pointTime, ResizeEdge::None))
				{
					SetCapture();
					Invalidate(FALSE);
					return;
				}
			}

			m_EnvelopeDrag = point;
			m_EnvelopeHandle = tangent;
			m_EnvelopeLayer = PossumEffects::Active(hit.track);
			m_EnvelopeEdits.clear();

			// Handle edits belong only to the clicked control point.
			// Value edits move all selected points on this envelope together.
			for (size_t i = 0; i < doc->GetNumSelected(); i++)
			{
				auto *cmd = doc->GetSelection(i);
				if ((tangent && cmd != point) || cmd->GetOwner() != hit.track ||
					PossumEffects::FindLayer(cmd, PossumEffects::IsMaster(hit.track)) != m_EnvelopeLayer)
					continue;

				auto *value = cmd->GetParams()->GetPropertyById('CPNT');
				if (value)
				{
					double originalTime = 0.0;
					hit.track->FindCommand(cmd, &originalTime);
					m_EnvelopeEdits.push_back({cmd, value->AsFloat(), originalTime, CUndoManager::Snapshot(cmd->GetParams())});
				}
			}

			SetCapture();
			Invalidate(FALSE);

			return;
		}
	}

	if (hit.cmd)
	{
		if (flags & MK_CONTROL)
		{
			if (Document()->IsSelected(hit.cmd))
				Document()->RemoveFromSelection(hit.cmd);
			else
				Document()->AddToSelection(hit.cmd);
		}
		else if (!Document()->IsSelected(hit.cmd))
		{
			Document()->ClearSelection();
			Document()->AddToSelection(hit.cmd);
		}

		// The edges resize the clicked command; the center moves the selection.
		// Ctrl+click is reserved for selection changes, not resizing.
		double startTime = 0.0;
		hit.track->FindCommand(hit.cmd, &startTime);

		ResizeEdge edge = (flags & MK_CONTROL) ? ResizeEdge::None : CommandResizeEdge(p, hit.row, hit.cmd, startTime);

		if (edge != ResizeEdge::None && CanEdit() && (GetKeyState('Z') & 0x8000))
		{
			const double handleTime = edge == ResizeEdge::Left ? startTime : startTime + hit.cmd->GetDuration();
			if (BeginScale(hit.cmd, hit.track, handleTime, edge))
			{
				SetCapture();
				Invalidate(FALSE);
				return;
			}
		}

		if (edge != ResizeEdge::None && CanEdit())
		{
			m_ResizeEdge = edge;
			m_ResizeCommand = hit.cmd;
			m_ResizeTrack = hit.track;
			m_ResizeStartTime = startTime;
			m_ResizeStartDuration = hit.cmd->GetDuration();

			m_ResizeCommands.clear();
			for (size_t i = 0; i < Document()->GetNumSelected(); ++i)
			{
				auto *cmd = Document()->GetSelection(i);
				auto *track = const_cast<nucleus::ITrack *>(cmd->GetOwner());
				if (!track || !cmd->GetParams()->GetPropertyById('DURA'))
					continue;

				// Envelope knots and commands without a duration are not resizable notes.
				if (PossumEffects::FindLayer(cmd, PossumEffects::IsMaster(track)) >= 0)
					continue;

				double t = 0.0;
				if (track->FindCommand(cmd, &t))
					m_ResizeCommands.push_back({cmd, track, t, cmd->GetDuration()});
			}

			SetCapture();
			Invalidate(FALSE);

			return;
		}

		m_DragCommands.clear();

		for (size_t i = 0; i < Document()->GetNumSelected(); i++)
		{
			auto* cmd = Document()->GetSelection(i);
			double t = 0;
			cmd->GetOwner(&t);
			m_DragCommands.push_back({cmd, t, CommandValue(cmd)});
		}

		m_DragStartTime = hit.time;
		m_DragStartRow = hit.row;
		m_DragStartValue = CommandValue(hit.cmd);
		m_Dragging = true;
		SetCapture(); // Capture mouse-up even while sample preparation is running.

		BeginAudition(hit.cmd);
		UpdateAudition();
	}
	else
	{
		m_BoxSelecting = true;
		m_PasteTargetRow = hit.row;
		m_SelectionRect = CRect(p, p);
		View()->SetSelectedTime(hit.time);
		View()->ShowSelectedTime(true);
	}

	SetCapture();
	Invalidate(FALSE);
}

void CTrackListCtrl::OnLButtonDblClk(UINT flags, CPoint p)
{
	EndScale(false);
	// The initial click of a double-click may have begun a box selection.
	m_BoxSelecting = m_Dragging = false;
	m_ResizeEdge = ResizeEdge::None;
	m_ResizeCommand = nullptr;
	m_ResizeTrack = nullptr;
	m_ResizeCommands.clear();
	m_DragCommands.clear();
	m_EnvelopeDrag = nullptr;
	m_EnvelopeEdits.clear();
	m_EnvelopeHandle = false;
	m_EnvelopeLayer = -1;

	if (GetCapture() == this)
		ReleaseCapture();

	if (!CanEdit())
		return;

	auto hit = HitTest(p);
	if (!hit.track || hit.header)
		return;

	int layer = PossumEffects::Active(hit.track);
	bool global = PossumEffects::IsMaster(hit.track);
	if (!(PossumEffects::Visible(hit.track) & (1u << layer)))
		return;

	// Double-clicking an existing knot selects it; it doesn't create another.
	bool handle = false;
	if (auto *existing = HitEnvelopePoint(p, hit.row, hit.track, handle))
	{
		Document()->ClearSelection();
		Document()->AddToSelection(existing);
		Invalidate(FALSE);

		return;
	}

	double time = hit.time;
	auto grid = GetGrid(hit.track);
	if (theApp.m_LayoutSnapEnabled && grid.timeSnap > 0.0)
		time = std::max<double>(0.0, std::round(time / grid.timeSnap) * grid.timeSnap);

	// Locate the surrounding knots in the current envelope. The inserted
	// value and derivative match the original Hermite segment at this time.
	struct Knot
	{
		double time, value, slope;
	};
	std::vector<Knot> knots;

	for (size_t i = 0; i < hit.track->GetNumCommands(); i++)
	{
		double t = 0.0;
		auto *cmd = hit.track->GetCommand(i, &t);

		if (PossumEffects::FindLayer(cmd, global) != layer)
			continue;

		auto *value = cmd->GetParams()->GetPropertyById('CPNT');
		if (!value)
			continue;

		double slope = 0.0;

		if (auto *pvec = cmd->GetParams()->GetPropertyById('CVEC'))
			if (const auto *vec = pvec->AsVec2F())
				if (vec->x != 0.0f)
					slope = double(vec->y) / vec->x;

		knots.push_back({t, value->AsFloat(), slope});
	}

	std::sort(knots.begin(), knots.end(), [](const Knot &a, const Knot &b)
	{
		return a.time < b.time;
	});

	for (const auto &knot : knots)
		if (std::abs(knot.time - time) < 0.000001)
			return;

	double value = EnvelopeValue(p.y, hit.row, layer);
	double slope = 0.0;

	auto following = std::lower_bound(knots.begin(), knots.end(), time, [](const Knot &knot, double t)
	{
		return knot.time < t;
	});

	if (following != knots.begin() && following != knots.end())
	{
		Knot &a = *(following - 1);
		Knot &b = *following;

		double dt = b.time - a.time;
		if (dt > 0.0)
		{
			double u = (time - a.time) / dt, u2 = u * u, u3 = u2 * u;

			value = (2*u3 - 3*u2 + 1)*a.value + (u3 - 2*u2 + u)*dt*a.slope
				+ (-2*u3 + 3*u2)*b.value + (u3-u2)*dt*b.slope;

			slope = ((6*u2 - 6*u)*(a.value - b.value) / dt)
				+ (3*u2 - 4*u + 1)*a.slope + (3*u2 - 2*u)*b.slope;
		}
	}
	else if (following == knots.end() && !knots.empty())
		value = knots.back().value;
	else if (!knots.empty())
		value = knots.front().value;

	auto *system = TrackSet()->GetNucleus();
	auto *m_AudioDevice = system ? system->DeviceManager()->GetDeviceByName(_T("Audio")) : nullptr;
	const auto *info = m_AudioDevice ? m_AudioDevice->GetCommandByName(global ? PossumEffects::Get(layer).globalCommand
																	   : PossumEffects::Get(layer).trackCommand) : nullptr;
	if (!info)
		return;

	auto *doc = Document();
	auto &undo = doc->UndoHistory();

	undo.Begin(_T("Add Envelope Point"));

	nucleus::ICommand *cmd = nullptr;
	if (hit.track->AddCommand(info, time, &cmd) == nucleus::RetCode::OK && cmd)
	{
		if (auto *param = cmd->GetParams()->GetPropertyById('CPNT'))
			param->SetFloat(float(value));

		if (auto *param = cmd->GetParams()->GetPropertyById('CVEC'))
			param->SetVec2F(props::TVec2F(1.0f, float(slope)));

		undo.RecordCommand(cmd, { }, {hit.track, time});

		doc->ClearSelection();
		doc->AddToSelection(cmd);
		undo.Commit();
		doc->SetModifiedFlag();

		Invalidate(FALSE);
	}
	else
		undo.Cancel();
}

void CTrackListCtrl::OnMouseMove(UINT flags, CPoint p)
{
	m_Mouse = p;
	UpdateLayerTooltip(p);

	if (flags & MK_LBUTTON)
	{
		if (m_Scaling)
		{
			UpdateScale(p);
		}
		else if (m_EnvelopeDrag)
		{
			auto *track = const_cast<nucleus::ITrack *>(m_EnvelopeDrag->GetOwner());
			if (track)
			{
				const size_t row = TrackSet()->GetTrackIndex(track);
				if (row != size_t(INVALID_TRACK))
				{
					if (m_EnvelopeHandle)
					{
						auto *params = m_EnvelopeDrag->GetParams();
						auto *field = params->GetPropertyById('CPNT');
						if (field)
						{
							double t = 0.0;
							track->FindCommand(m_EnvelopeDrag, &t);

							// CVEC is a real time/value displacement, not a screen-normalized slope.
							double dx = std::max<double>(MIN_HANDLE_TIME, TimeAt(p.x) - t);
							double dy = EnvelopeValue(p.y, int(row), m_EnvelopeLayer) - field->AsFloat();

							if (auto *vector = params->GetPropertyById('CVEC'))
								vector->SetVec2F(props::TVec2F(float(dx), float(dy)));
						}
					}
					else
					{
						double deltaValue = EnvelopeValue(p.y, int(row), m_EnvelopeLayer) - EnvelopeValue(m_Down.y, int(row), m_EnvelopeLayer);

						auto &def = PossumEffects::Get(m_EnvelopeLayer);
						auto grid = GetGrid(track);

						double deltaTime = TimeAt(p.x) - TimeAt(m_Down.x);

						if (theApp.m_LayoutSnapEnabled && grid.timeSnap > 0.0)
							deltaTime = std::round(deltaTime / grid.timeSnap) * grid.timeSnap;

						// Preserve relative spacing and prevent the earliest selected point going negative.
						double earliest = DBL_MAX;
						for (const auto &edit : m_EnvelopeEdits)
							earliest = std::min<double>(earliest, edit.originalTime);

						deltaTime = std::max<double>(deltaTime, -earliest);

						// Moving points live updates the Hermite curve, not just a temporary rectangle.
						// Test destinations before changing the command map.
						bool fits = true;
						for (const auto &edit : m_EnvelopeEdits)
							fits &= track->CanFit(edit.originalTime + deltaTime,
												  edit.command->GetDuration(), edit.command, Document()->SelectionSet());

						if (fits)
						{
							for (const auto &edit : m_EnvelopeEdits)
							{
								if (auto *field = edit.command->GetParams()->GetPropertyById('CPNT'))
									field->SetFloat(float(std::clamp<double>(edit.originalValue + deltaValue,
																			 def.minimum, def.maximum)));

								double currentTime = 0.0;
								track->FindCommand(edit.command, &currentTime);

								if (std::abs(currentTime - (edit.originalTime + deltaTime)) > 0.0000001)
									track->MoveCommand(edit.command, edit.originalTime + deltaTime,
													   nullptr, Document()->SelectionSet());
							}
						}
					}

					Invalidate(FALSE);
				}
			}
		}
		else if (m_ResizeCommand)
		{
			// Preview only. Commit once on mouse-up, preserving realtime thread safety.
			Invalidate(FALSE);
		}
		else if (m_ResizingRow)
		{
			auto* track = TrackSet()->GetTrack(m_ResizeRow);

			SetRowHeight(track, m_ResizeHeight + p.y - m_Down.y);
		}
		else if (m_BoxSelecting)
		{
			m_SelectionRect.BottomRight() = p;

			Invalidate(FALSE);
		}
		else if (m_Dragging)
		{
			auto *originTrack = TrackSet()->GetTrack(m_DragStartRow);
			auto cfg = GetGrid(originTrack);

			double time_delta = (TimeAt(m_Mouse.x) - TimeAt(m_Down.x));

			if (theApp.m_LayoutSnapEnabled && cfg.timeSnap > 0)
				time_delta = std::round(time_delta / cfg.timeSnap) * cfg.timeSnap;

			double dy = double(p.y - m_Down.y);
			double value_delta = 0.0;
			double value_ratio = 1.0;

			if (cfg.scale == Scale::Logarithmic)
			{
				value_ratio = std::pow(cfg.maximum / cfg.minimum, -dy / double(cfg.height - 1));

				if (theApp.m_LayoutSnapEnabled && cfg.valueSnap > 0.0)
				{
					double steps = SEMITONES * std::log2(value_ratio);
					steps = std::round(steps / cfg.valueSnap) * cfg.valueSnap;
					value_ratio = std::exp2(steps / SEMITONES);
				}
			}
			else
			{
				value_delta = -dy * (cfg.maximum - cfg.minimum) / double(cfg.height - 1);

				if (theApp.m_LayoutSnapEnabled && cfg.valueSnap > 0.0)
					value_delta = std::round(value_delta / cfg.valueSnap) * cfg.valueSnap;
			}

			bool fits = true;

			for (auto entry : m_DragCommands)
			{
				nucleus::ITrack *track = (nucleus::ITrack *)(entry.cmd->GetOwner());
				double new_time = std::max<double>(0.0, entry.origtime + time_delta);

				if (!track || !track->CanFit(new_time, entry.cmd->GetDuration(), entry.cmd, Document()->SelectionSet()))
				{
					fits = false;
					break;
				}
			}

			if (fits)
			{
				for (const auto &entry : m_DragCommands)
				{
					nucleus::ITrack *track = (nucleus::ITrack *)(entry.cmd->GetOwner());

					track->MoveCommand(entry.cmd, entry.origtime + time_delta, nullptr, Document()->SelectionSet());
					SetCommandValue(entry.cmd, cfg.scale == Scale::Logarithmic ? entry.origval * value_ratio : entry.origval + value_delta);
				}
			}

			Invalidate(FALSE);
		}

		if (m_AuditionMode)
			UpdateAudition();
	}

	CWnd::OnMouseMove(flags, p);
}

void CTrackListCtrl::OnLButtonUp(UINT flags, CPoint p)
{
	if (m_Scaling)
	{
		UpdateScale(p);
		EndScale(true);
		if (GetCapture() == this)
			ReleaseCapture();
		return;
	}

	EndAudition();
	if (GetCapture() == this)
		ReleaseCapture();

	if (m_ResizeCommand)
	{
		const double delta = ResizeDelta(p);
		if (CanEdit() && std::abs(delta) > 0.0000001 && !m_ResizeCommands.empty())
		{
			std::vector<CUndoManager::CommandChange> changes;
			changes.reserve(m_ResizeCommands.size());

			for (const auto &entry : m_ResizeCommands)
			{
				double time = entry.time + (m_ResizeEdge == ResizeEdge::Left ? delta : 0.0);
				double duration = entry.duration + (m_ResizeEdge == ResizeEdge::Left ? -delta : delta);

				changes.push_back({entry.command,{entry.track, entry.time, entry.duration},{entry.track, time, duration}});
			}

			// Apply atomically, using the same path as Undo/Redo. If any
			// destination violates a track's overlap policy, nothing changes.
			if (CUndoManager::ApplyCommandGroup(changes, true))
			{
				auto &undo = Document()->UndoHistory();
				undo.Begin(_T("Resize Notes"));
				undo.RecordCommandGroup(std::move(changes));
				undo.Commit();

				Document()->SetModifiedFlag();
				Document()->UpdatePropertiesForSelection();

				theApp.RefreshCommandList(false);
			}
		}

		m_ResizeCommand = nullptr;
		m_ResizeTrack = nullptr;
		m_ResizeEdge = ResizeEdge::None;
		m_ResizeCommands.clear();

		Invalidate(FALSE);

		return;
	}

	if (m_EnvelopeDrag)
	{
		auto &undo = Document()->UndoHistory();
		undo.Begin(_T("Edit Envelope Points"));

		for (const auto &edit : m_EnvelopeEdits)
		{
			auto *track = const_cast<nucleus::ITrack *>(edit.command->GetOwner());
			if (track)
			{
				double endTime = 0.0;
				track->FindCommand(edit.command, &endTime);

				if (std::abs(endTime - edit.originalTime) > 0.0000001)
					undo.RecordCommand(edit.command, {track, edit.originalTime}, {track, endTime});
			}

			undo.RecordProperties(edit.command->GetParams(), edit.before, CUndoManager::Snapshot(edit.command->GetParams()));
		}

		undo.Commit();

		Document()->SetModifiedFlag();
		Document()->UpdatePropertiesForSelection();

		m_EnvelopeDrag = nullptr;
		m_EnvelopeEdits.clear();
		m_EnvelopeLayer = -1;
		m_EnvelopeHandle = false;

		Invalidate(FALSE);

		return;
	}

	if (m_BoxSelecting)
	{
		if (!(flags & (MK_CONTROL | MK_SHIFT)))
			Document()->ClearSelection();

		CRect box = m_SelectionRect;
		box.BottomRight() = p;
		box.NormalizeRect();

		auto* ts = TrackSet();
		if (ts && box.Width() > 2 && box.Height() > 2)
		{
			for (int row = 0; row < int(ts->GetNumTracks()); row++)
			{
				auto* tr = ts->GetTrack(row);
				const auto cfg = GetGrid(tr);
				const bool global = PossumEffects::IsMaster(tr);
				const int active = PossumEffects::Active(tr);
				const bool activeVisible = (PossumEffects::Visible(tr) & (1u << active)) != 0;

				for (size_t i = 0; i < tr->GetNumCommands(); i++)
				{
					double t = 0.0;
					auto* cmd = tr->GetCommand(i, &t);
					if (!cmd)
						continue;

					const int layer = PossumEffects::FindLayer(cmd, global);
					CRect r;
					if (layer >= 0)
					{
						// Envelope points are drawn using their own time/value coordinates.
						// Only the active, visible layer displays selectable points.
						if (!activeVisible || layer != active)
							continue;
						auto* value = cmd->GetParams()->GetPropertyById('CPNT');
						if (!value)
							continue;
						const int x = XAt(t);
						const int y = EnvelopeY(value->AsFloat(), row, layer);
						r.SetRect(x - ENVELOPE_POINT_GRAB, y - ENVELOPE_POINT_GRAB,
								  x + ENVELOPE_POINT_GRAB + 1, y + ENVELOPE_POINT_GRAB + 1);
					}
					else
						r = CommandRect(row, cmd, t, cfg);

					CRect overlap;
					if (overlap.IntersectRect(box, r))
					{
						if (flags & MK_CONTROL)
							Document()->RemoveFromSelection(cmd, false);
						else
							Document()->AddToSelection(cmd, false);
					}
				}
			}

			theApp.RedrawActiveTimeline();
			theApp.RefreshCommandList(false);
			Document()->UpdatePropertiesForSelection();
		}
	}

	if (m_Dragging && CanEdit())
	{
		auto &undo = Document()->UndoHistory();
		undo.Begin(_T("Move Notes"));

		for (const auto &entry : m_DragCommands)
		{
			auto *track = const_cast<nucleus::ITrack*>(entry.cmd->GetOwner());

			double t;
			track->FindCommand(entry.cmd, &t);

			auto before = CUndoManager::Snapshot(entry.cmd->GetParams());
			if (auto *pp = before->GetPropertyById('FREQ'))
				pp->SetFloat(float(entry.origval));
			else if (auto *pp = before->GetPropertyById('PMUL'))
				pp->SetFloat(float(entry.origval / MIDDLE_C));

			undo.RecordCommand(entry.cmd, {track, entry.origtime, -1.0}, {track, t, -1.0});
			undo.RecordProperties(entry.cmd->GetParams(), before, CUndoManager::Snapshot(entry.cmd->GetParams()));
		}

		undo.Commit();

		Document()->SetModifiedFlag();
		Document()->UpdatePropertiesForSelection();

		theApp.RefreshCommandList(false);
	}

	m_BoxSelecting = m_Dragging = m_ResizingRow = false;
	m_DragCommands.clear();

	Invalidate(FALSE);
	CWnd::OnLButtonUp(flags, p);
}

bool CTrackListCtrl::CopySelection()
{
	auto* doc = Document();
	auto* ts = TrackSet();
	if (!doc || !ts || !doc->GetNumSelected())
		return false; // An empty selection shouldn't erase the previous clipboard.

	double baseline = DBL_MAX;
	int firstRow = INT_MAX;
	for (size_t i = 0; i < doc->GetNumSelected(); i++)
	{
		auto* cmd = doc->GetSelection(i);
		auto* track = cmd ? cmd->GetOwner() : nullptr;
		if (!track)
			return false;
		const auto index = ts->GetTrackIndex(track);
		if (index == size_t(INVALID_TRACK))
			return false;
		double time = 0.0;
		if (!track->FindCommand(cmd, &time))
			return false;
		baseline = std::min<double>(baseline, time);
		firstRow = std::min<int>(firstRow, int(index));
	}

	ClipCommandList clipboard;
	clipboard.reserve(doc->GetNumSelected());
	for (size_t i = 0; i < doc->GetNumSelected(); i++)
	{
		auto* cmd = doc->GetSelection(i);
		auto* track = cmd->GetOwner();
		double time = 0.0;
		if (!track->FindCommand(cmd, &time))
			return false;

		ClipCommand copied;
		copied.info = cmd->GetCommandInfo();
		copied.relativeTime = time - baseline;
		copied.relativeRow = int(ts->GetTrackIndex(track)) - firstRow;
		copied.fromMaster = PossumEffects::IsMaster(track);
		copied.properties = std::shared_ptr<props::IPropertySet>(
			props::IPropertySet::CreatePropertySet(), [](props::IPropertySet* p)
		{ if (p) p->Release(); });
		if (!copied.properties || !copied.info)
			return false;

		copied.properties->AppendPropertySet(cmd->GetParams());
		clipboard.push_back(std::move(copied));
	}

	// Commit only after every selected command has been copied.
	s_Clipboard.swap(clipboard);
	s_ClipboardFirstRow = size_t(firstRow);
	m_PasteTargetRow = firstRow;
	return true;
}

void CTrackListCtrl::DeleteSelection()
{
	if (!CanEdit())
		return;

	auto* doc = Document();
	if (!doc || !doc->GetNumSelected())
		return;

	std::vector<nucleus::ICommand*> commands;
	for (size_t i = 0; i < doc->GetNumSelected(); i++)
		commands.push_back(doc->GetSelection(i));

	auto &undo = doc->UndoHistory();
	undo.Begin(_T("Delete Commands"));
	doc->ClearSelection();

	for (auto *cmd : commands)
	{
		auto *track = const_cast<nucleus::ITrack*>(cmd->GetOwner());
		double time = 0.0;
		if (track && track->FindCommand(cmd, &time) && track->RemoveCommand(cmd, false) == nucleus::RetCode::OK)
			undo.RecordCommand(cmd, {track, time}, { });
	}

	undo.Commit();
	doc->SetModifiedFlag();
	doc->UpdatePropertiesForSelection();
	Invalidate(FALSE);
}

void CTrackListCtrl::CutSelection()
{
	if (!CanEdit() || !Document() || !Document()->GetNumSelected())
		return;

	// Never delete commands unless the current selection was fully captured.
	if (CopySelection())
		DeleteSelection();
}

void CTrackListCtrl::PasteAt(nucleus::ITrack* firstTrack, double time)
{
	auto* ts = TrackSet();
	auto* doc = Document();
	if (!ts || !doc || !firstTrack || !CanEdit() || s_Clipboard.empty())
		return;

	const auto baseIndex = ts->GetTrackIndex(firstTrack);
	if (baseIndex == size_t(INVALID_TRACK))
		return;

	struct Inserted
	{
		nucleus::ITrack* track;
		nucleus::ICommand* command;
		double time;
	};
	std::vector<Inserted> inserted;
	inserted.reserve(s_Clipboard.size());
	bool success = true;

	// Do not disturb the current selection or undo history until the full
	// batch has been inserted. A failed placement rolls back all new commands.
	for (const auto &copy : s_Clipboard)
	{
		const auto row = baseIndex + size_t(copy.relativeRow);
		if (row >= ts->GetNumTracks())
		{
			success = false; break;
		}

		auto* track = ts->GetTrack(row);
		if (!track || PossumEffects::IsMaster(track) != copy.fromMaster || !copy.info || !copy.properties)
		{
			success = false; break;
		}

		const double destinationTime = std::max<double>(0.0, time + copy.relativeTime);
		const auto* savedDuration = copy.properties->GetPropertyById('DURA');
		const double duration = savedDuration ? savedDuration->AsFloat() : 0.0;
		if (savedDuration && !track->CanFit(destinationTime, duration, nullptr))
		{
			success = false; break;
		}

		nucleus::ICommand* cmd = nullptr;
		if (track->AddCommand(copy.info, destinationTime, &cmd) != nucleus::RetCode::OK || !cmd)
		{
			success = false; break;
		}
		inserted.push_back({track, cmd, destinationTime});

		// Audio's known command schema can be restored directly over defaults;
		// avoid appending a duplicate set of default property IDs. Other devices
		// retain the existing generic property-set merge behavior.
		auto* device = copy.info->GetDevice();
		if (device && !_tcscmp(device->GetName(), _T("Audio")))
			RestoreAudioParams(cmd->GetParams(), copy.properties.get());
		else
			cmd->GetParams()->AppendPropertySet(copy.properties.get());

		if (!track->CanFit(destinationTime, cmd->GetDuration(), cmd))
		{
			success = false; break;
		}
	}

	if (!success)
	{
		for (auto it = inserted.rbegin(); it != inserted.rend(); ++it)
			it->track->RemoveCommand(it->command, true);
		::MessageBeep(MB_ICONWARNING); // Nothing pasted: incompatible lane or overlap.
		return;
	}

	auto &undo = doc->UndoHistory();
	undo.Begin(_T("Paste Commands"));
	doc->ClearSelection();
	for (const auto &entry : inserted)
	{
		undo.RecordCommand(entry.command, { }, {entry.track, entry.time});
		doc->AddToSelection(entry.command, false);
	}
	undo.Commit();
	doc->UpdatePropertiesForSelection();
	doc->SetModifiedFlag();
	Invalidate(FALSE);
}

void CTrackListCtrl::PasteSelection()
{
	auto* ts = TrackSet();
	auto* doc = Document();
	if (!ts || !doc || s_Clipboard.empty() || !ts->GetNumTracks())
		return;

	// Keyboard paste uses the selected timeline time, not a stale mouse X.
	// A current selection provides the destination's topmost track.
	nucleus::ITrack* target = nullptr;
	if (doc->GetNumSelected())
	{
		size_t first = ts->GetNumTracks();
		for (size_t i = 0; i < doc->GetNumSelected(); i++)
		{
			auto* cmd = doc->GetSelection(i);
			if (cmd && cmd->GetOwner())
				first = std::min<size_t>(first, ts->GetTrackIndex(cmd->GetOwner()));
		}
		if (first < ts->GetNumTracks())
			target = ts->GetTrack(first);
	}
	if (!target)
	{
		// A click on empty timeline space establishes a lane; otherwise use
		// the original clipboard lane, not a potentially stale mouse location.
		const size_t row = m_PasteTargetRow >= 0 ? size_t(m_PasteTargetRow) : s_ClipboardFirstRow;
		if (row < ts->GetNumTracks())
			target = ts->GetTrack(row);
	}
	if (!target)
	{
		const bool masterFirst = std::any_of(s_Clipboard.begin(), s_Clipboard.end(),
											 [](const ClipCommand &cmd) { return cmd.relativeRow == 0 && cmd.fromMaster; });
		target = ts->GetTrack(masterFirst ? 0 :
							  std::min<size_t>(1, ts->GetNumTracks() - 1));
	}

	PasteAt(target, View()->GetSelectedTime());
}

void CTrackListCtrl::DuplicateSelection()
{
	auto* doc = Document();
	auto* ts = TrackSet();
	if (!CanEdit() || !doc || !ts || !doc->GetNumSelected())
		return;

	double firstTime = DBL_MAX;
	double lastTime = 0.0;
	size_t firstRow = ts->GetNumTracks();
	for (size_t i = 0; i < doc->GetNumSelected(); i++)
	{
		auto* cmd = doc->GetSelection(i);
		auto* track = cmd ? cmd->GetOwner() : nullptr;
		double t = 0.0;
		if (!track || !track->FindCommand(cmd, &t))
			return;
		firstRow = std::min<size_t>(firstRow, ts->GetTrackIndex(track));
		firstTime = std::min<double>(firstTime, t);
		lastTime = std::max<double>(lastTime, t + cmd->GetDuration());
	}
	if (firstRow >= ts->GetNumTracks())
		return;

	if (CopySelection())
	{
		PasteAt(ts->GetTrack(firstRow), firstTime + std::max<double>(TIME_SNAP, lastTime - firstTime));
	}
}

void CTrackListCtrl::OnEditCopy()
{
	CopySelection();
}

void CTrackListCtrl::OnEditCut()
{
	CutSelection();
}

void CTrackListCtrl::OnEditPaste()
{
	PasteSelection();
}

void CTrackListCtrl::OnEditClear()
{
	DeleteSelection();
}

void CTrackListCtrl::OnKeyDown(UINT ch, UINT rep, UINT flags)
{
	if (ch == VK_ESCAPE && m_Scaling)
	{
		EndScale(false);
		if (GetCapture() == this)
			ReleaseCapture();
		return;
	}

	if (GetKeyState(VK_CONTROL) & 0x8000)
	{
		switch (ch)
		{
			case 'C':	CopySelection();		return;
			case 'X':	CutSelection();			return;
			case 'V':	PasteSelection();		return;
			case 'D':	DuplicateSelection();	return;
			case 'Z':	Document()->Undo();		return;
			case 'Y':	Document()->Redo();		return;
		}
	}
	else if (ch == VK_DELETE)
	{
		DeleteSelection();
		return;
	}
	CWnd::OnKeyDown(ch, rep, flags);
}

void CTrackListCtrl::OnSetFocus(CWnd* previous)
{
	CWnd::OnSetFocus(previous);
	Invalidate(FALSE);
}

BOOL CTrackListCtrl::OnSetCursor(CWnd* w, UINT hit, UINT message)
{
	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	auto info = HitTest(pt);
	if (info.header && info.track && abs(pt.y - (RowTop(info.row) + GetGrid(info.track).height)) <= ROW_RESIZE_GRAB)
	{
		SetCursor(LoadCursor(nullptr, IDC_SIZENS));
		return TRUE;
	}

	if (info.track && !info.header)
	{
		bool handle = false;
		if (HitEnvelopePoint(pt, info.row, info.track, handle))
		{
			SetCursor(LoadCursor(nullptr, handle ? IDC_CROSS : IDC_SIZEALL));
			return TRUE;
		}
	}

	if (info.cmd && CanEdit())
	{
		double time = 0.0;
		info.track->FindCommand(info.cmd, &time);
		if (CommandResizeEdge(pt, info.row, info.cmd, time) != ResizeEdge::None)
		{
			SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
			return TRUE;
		}
	}

	if (info.cmd)
	{
		SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
		return TRUE;
	}

	return CWnd::OnSetCursor(w, hit, message);
}

void CTrackListCtrl::OnDropFiles(HDROP drop)
{
	// Explorer-style file drops and Nucleus command drags share this handler.
	if (::DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0) > 0)
	{
		TCHAR file[MAX_PATH] = { };
		CPoint point;
		BOOL inClient = ::DragQueryPoint(drop, &point);

		if (inClient && ::DragQueryFile(drop, 0, file, MAX_PATH))
			InsertSampleFile(file, point);

		::DragFinish(drop);

		return;
	}

	CPoint p;
	BOOL inClient = ::DragQueryPoint(drop, &p);
	auto hit = inClient ? HitTest(p) : Hit{ };

	if (CanEdit() && hit.track && !hit.header &&
		::DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0) == 0 &&
		theApp.m_DragData.m_eMode == CPossum::SDragData::CMDINFO &&
		theApp.m_DragData.m_pCmdInfo)
	{
		const auto *info = theApp.m_DragData.m_pCmdInfo;
		const TCHAR *commandName = info->GetName();
		bool globalCommand = _tcsncmp(commandName, _T("Global"), 6) == 0;
		bool trackCommand = _tcsncmp(commandName, _T("Track"), 5) == 0;

		if ((globalCommand && !PossumEffects::IsMaster(hit.track)) ||
			(trackCommand && PossumEffects::IsMaster(hit.track)))
		{
			::DragFinish(drop);
			return;
		}

		auto config = GetGrid(hit.track);
		double t = hit.time;

		if (theApp.m_LayoutSnapEnabled && config.timeSnap > 0)
			t = std::round(t / config.timeSnap) * config.timeSnap;

		auto &undo = Document()->UndoHistory();
		undo.Begin(_T("Add Note"));

		nucleus::ICommand *cmd = nullptr;
		if (hit.track->AddCommand(theApp.m_DragData.m_pCmdInfo, t, &cmd) == nucleus::RetCode::OK && cmd)
		{
			SetCommandValue(cmd, SnapValue(ValueAt(p.y, hit.row, config), config));
			undo.RecordCommand(cmd, { }, {hit.track, t});

			Document()->ClearSelection();
			Document()->AddToSelection(cmd);
			undo.Commit();
			Document()->SetModifiedFlag();

			theApp.RefreshCommandList(true);
			Invalidate(FALSE);
		}
		else
			undo.Cancel();
	}

	::DragFinish(drop);
}

void CTrackListCtrl::OnDestroy()
{
	m_MediaDropTarget.Revoke();
	ReleaseBackBuffer();
	if (m_NameFont.GetSafeHandle())
		m_NameFont.DeleteObject();
	if (m_DescriptionFont.GetSafeHandle())
		m_DescriptionFont.DeleteObject();
	if (m_ClefFont.GetSafeHandle())
		m_ClefFont.DeleteObject();

	CWnd::OnDestroy();
}

DROPEFFECT CTrackMediaDropTarget::OnDragEnter(CWnd *window, COleDataObject *data, DWORD keys, CPoint point)
{
	return OnDragOver(window, data, keys, point);
}

DROPEFFECT CTrackMediaDropTarget::OnDragOver(CWnd *, COleDataObject *data, DWORD, CPoint)
{
	return data && data->IsDataAvailable(CF_HDROP) ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}

BOOL CTrackMediaDropTarget::OnDrop(CWnd *, COleDataObject *data, DROPEFFECT, CPoint point)
{
	if (!m_Owner || !data || !data->IsDataAvailable(CF_HDROP))
		return FALSE;

	HGLOBAL memory = data->GetGlobalData(CF_HDROP);
	if (!memory)
		return FALSE;

	TCHAR path[MAX_PATH] = { };
	bool file = ::DragQueryFile(reinterpret_cast<HDROP>(memory), 0, path, MAX_PATH) != 0;
	bool result = file && m_Owner->InsertSampleFile(path, point);

	::GlobalFree(memory);

	return result ? TRUE : FALSE;
}

bool CTrackListCtrl::InsertSampleFile(const TCHAR *file, CPoint point)
{
	if (!file || !*file || !CanEdit())
		return false;

	CString path(file);

	if (path.Right(4).CompareNoCase(_T(".wav")) != 0)
		return false;

	auto hit = HitTest(point);
	if (!hit.track || hit.header || PossumEffects::IsMaster(hit.track))
		return false;

	auto *system = TrackSet()->GetNucleus();
	const auto *info = m_AudioDevice ? m_AudioDevice->GetCommandByName(_T("Sample")) : nullptr;

	if (!info)
		return false;

	const auto grid = GetGrid(hit.track);
	double time = hit.time;
	if (theApp.m_LayoutSnapEnabled && grid.timeSnap > 0.0)
		time = std::round(time / grid.timeSnap) * grid.timeSnap;

	auto &undo = Document()->UndoHistory();
	undo.Begin(_T("Insert Sample"));

	nucleus::ICommand *command = nullptr;
	if (hit.track->AddCommand(info, time, &command) != nucleus::RetCode::OK || !command)
	{
		undo.Cancel();
		return false;
	}

	if (auto *property = command->GetParams()->GetPropertyById('PATH'))
		property->SetString(path);

	// The sample is anchored at the vertical pitch under the drop cursor.
	SetCommandValue(command, SnapValue(ValueAt(point.y, hit.row, grid), grid));
	undo.RecordCommand(command, { }, {hit.track, time});

	Document()->ClearSelection();
	Document()->AddToSelection(command);
	undo.Commit();

	Document()->SetModifiedFlag();
	theApp.RefreshCommandList(true);
	Invalidate(FALSE);

	return true;
}

void CTrackListCtrl::OnContextMenu(CWnd *, CPoint screen)
{
	if (screen.x == -1 && screen.y == -1)
		::GetCursorPos(&screen);

	CPoint client(screen);
	ScreenToClient(&client);

	const auto hit = HitTest(client);
	m_ContextTrack = hit.track;

	HMENU menu = ::CreatePopupMenu();
	if (!menu)
		return;

	const bool selected = Document() && Document()->GetNumSelected() != 0;
	const bool canEdit = CanEdit();

	MyAppendMenuItem(menu, MFT_STRING, _T("Insert Track Above"), canEdit, 0, ID_TRACK_INSERT_ABOVE);
	MyAppendMenuItem(menu, MFT_STRING, _T("Insert Track Below"), canEdit, 0, ID_TRACK_INSERT_BELOW);
	if (!PossumEffects::IsMaster(hit.track))
	{
		MyAppendMenuItem(menu, MFT_STRING, _T("Duplicate Track"), canEdit, 0, ID_TRACK_INSERT_BELOW);
		MyAppendMenuItem(menu, MFT_STRING, _T("Remove Track"), canEdit && hit.track, 0, ID_TRACK_REMOVE);
	}
	if (!hit.header)
	{
		MyAppendMenuItem(menu, MFT_SEPARATOR);
		MyAppendMenuItem(menu, MFT_STRING, _T("Cut"), canEdit && selected, 0, ID_EDIT_CUT);
		MyAppendMenuItem(menu, MFT_STRING, _T("Copy"), selected, 0, ID_EDIT_COPY);
		MyAppendMenuItem(menu, MFT_STRING, _T("Paste"), canEdit, 0, ID_EDIT_PASTE);
		MyAppendMenuItem(menu, MFT_STRING, _T("Duplicate"), canEdit && selected, 0, ID_EDIT_DUPLICATE);
		MyAppendMenuItem(menu, MFT_STRING, _T("Delete"), canEdit && selected, 0, ID_EDIT_CLEAR);
	}

	UINT chosen = ::TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
								   screen.x, screen.y, 0, GetSafeHwnd(), NULL);
	::DestroyMenu(menu);

	if (chosen == ID_EDIT_PASTE && hit.track && !hit.header)
		PasteAt(hit.track, hit.time);
	else if (chosen)
		SendMessage(WM_COMMAND, chosen);
}

void CTrackListCtrl::InsertContextTrack(bool below)
{
	auto *set = TrackSet();
	if (!set || !CanEdit())
		return;

	size_t position = m_ContextTrack ? set->GetTrackIndex(m_ContextTrack) + (below ? 1 : 0) : set->GetNumTracks();

	// The master effects lane remains the first track.
	position = std::max<size_t>(1, position);

	CString name;
	for (int n = 1;; n++)
	{
		name.Format(_T("Audio %d"), n);
		if (!set->GetTrackByName(name))
			break;
	}

	auto &undo = Document()->UndoHistory();
	undo.Begin(_T("Insert Track"));

	nucleus::ITrack *track = nullptr;
	if (set->CreateTrack(&track, name, _T("")) != nucleus::RetCode::OK || !track)
	{
		undo.Cancel();
		return;
	}

	PossumEffects::InitializeTrack(track, false);

	// Do not populate the track with effect commands or turn on absent layers.
	set->MoveTrack(track, position);
	undo.RecordTrack(track, { }, {set, set->GetTrackIndex(track)});
	undo.Commit();

	Document()->SetModifiedFlag();
	RefreshTracks();
	theApp.RefreshCommandList(true);
}

void CTrackListCtrl::OnTrackInsertAbove()
{
	InsertContextTrack(false);
}

void CTrackListCtrl::OnTrackInsertBelow()
{
	InsertContextTrack(true);
}

void CTrackListCtrl::OnTrackRemove()
{
	if (m_ContextTrack && !PossumEffects::IsMaster(m_ContextTrack))
		DeleteTrack(m_ContextTrack);
}

void CTrackListCtrl::OnTrackDuplicate()
{

}

void CTrackListCtrl::OnEditDuplicate()
{
	DuplicateSelection();
}

void FindOverlappingCommands(const nucleus::ICommand *clicked, const nucleus::ICommandSet *commands, nucleus::ICommandSet *results)
{
	if (!clicked || !commands || !results)
		return;

	results->Clear();

	double start = 0.0;
	clicked->GetOwner(&start);
	const double end = start + clicked->GetDuration();

	commands->ForEach([start, end, results](nucleus::ICommand *pcmd)
	{
		assert(pcmd);

		double lstart = 0.0;
		pcmd->GetOwner(&lstart);
		const double lend = lstart + pcmd->GetDuration();

		if (lstart < end && lend > start)
			results->Add(pcmd);
	});
}

void CTrackListCtrl::BeginAudition(nucleus::ICommand *pcmd)
{
	auto *frame = DYNAMIC_DOWNCAST(CPossumMainFrame, AfxGetMainWnd());
	if (!pcmd || !m_AudioDevice || !frame || !frame->IsAuditionEnabled() || !CanEdit())
		return;

	auto audible = [](const nucleus::ICommand *command)
	{
		const auto *info = command->GetCommandInfo();
		const auto *name = info->GetName();
		return (!_tcscmp(name, _T("Tone")) || !_tcscmp(name, _T("Noise")) || !_tcscmp(name, _T("Sample")));
	};

	if (!audible(pcmd))
		return;

	if (!m_AuditionSet)
		m_AuditionSet = nucleus::ICommandSet::Create();

	FindOverlappingCommands(pcmd, Document()->SelectionSet(), m_AuditionSet);

	if (m_AudioDevice->ExecuteCommand(m_AuditionStart, 0.0) != nucleus::RetCode::OK)
		return;

	m_AuditionSet->ForEach([this](nucleus::ICommand *pcmd)
	{
		assert(pcmd);
		m_AudioDevice->ExecuteCommand(pcmd, 0.0);
	});

	m_AuditionMode = true;
}

void CTrackListCtrl::UpdateAudition()
{
	if (m_AuditionMode)
		m_AudioDevice->ExecuteCommand(m_AuditionUpdate, 0.0);
}

void CTrackListCtrl::EndAudition()
{
	if (m_AuditionMode)
	{
		// Destroy voices before releasing the temporary commands/tracks they reference.
		m_AudioDevice->ExecuteCommand(m_AuditionStop, 0.0);
		m_AuditionMode = false;
	}
}

void CTrackListCtrl::OnCaptureChanged(CWnd *window)
{
	EndScale(false);
	CWnd::OnCaptureChanged(window);
}

void CTrackListCtrl::OnCancelMode()
{
	EndScale(false);
	CWnd::OnCancelMode();
}
