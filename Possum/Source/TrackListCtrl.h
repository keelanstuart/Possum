#pragma once
#include <NuTrackSet.h>
#include "PossumEffects.h"
#include <NuCommand.h>
#include <array>
#include <vector>
#include <string>
#include <memory>

namespace nucleus { class IDevice; }

class CTrackListCtrl;

class CTrackMediaDropTarget : public COleDropTarget
{
public:
    explicit CTrackMediaDropTarget(CTrackListCtrl *owner = nullptr) : m_Owner(owner) {}
    DROPEFFECT OnDragEnter(CWnd *window, COleDataObject *data, DWORD keys, CPoint point) override;
    DROPEFFECT OnDragOver(CWnd *window, COleDataObject *data, DWORD keys, CPoint point) override;
    BOOL OnDrop(CWnd *window, COleDataObject *data, DROPEFFECT effect, CPoint point) override;
private:
    CTrackListCtrl *m_Owner = nullptr;
};

// Generic, independently configurable per-track timeline. The configuration is
// stored in the Nucleus track properties; no editor-only shadow document.
class CTrackListCtrl : public CWnd
{
    DECLARE_DYNAMIC(CTrackListCtrl)

public:
    enum class Scale { Linear, Logarithmic };
    struct GridConfig
    {
        int height = 180;
        double minimum = 25.0;
        double maximum = 1500.0;
        double interval = 1.0;      // 1 semitone in logarithmic mode
        double valueSnap = 0.5;
        double timeSnap = 0.25;		// 1/4 seconds
        bool timeSnapEnabled = true;
        bool valueSnapEnabled = true;
        Scale scale = Scale::Logarithmic;
        CString preset;
    };

	struct Hit
	{
		nucleus::ITrack* track = nullptr;
		nucleus::ICommand* cmd = nullptr;
		int row = -1;
		double time = 0.0;
		bool header = false;
	};

	struct ClipCommand
	{
		const nucleus::ICommandInfo* info = nullptr;
		double relativeTime = 0.0;
		int relativeRow = 0;       // Preserve multi-track selections.
		bool fromMaster = false; // Do not paste master effects onto audio tracks.
		std::shared_ptr<props::IPropertySet> properties;
	};

	using ClipCommandList = std::vector<CTrackListCtrl::ClipCommand>;

    CTrackListCtrl();
	virtual ~CTrackListCtrl();

	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id);

	bool AddTrack(const TCHAR* name);
    bool DeleteTrack(nucleus::ITrack* track);

	GridConfig GetGrid(const nucleus::ITrack* track) const;
    void SetGrid(nucleus::ITrack* track, const GridConfig& config);

	void SetRowHeight(nucleus::ITrack* track, int pixels);

	int GetHeaderWidth() const { return m_HeaderWidth; }
    void SetHeaderWidth(int pixels);

	void RefreshTracks();

	bool CopySelection();
    void CutSelection();
    void PasteSelection();
    void DuplicateSelection();

protected:

	nucleus::IDevice *m_AudioDevice = nullptr;
    nucleus::ICommand *m_AuditionStart = nullptr;
    nucleus::ICommand *m_AuditionUpdate = nullptr;
    nucleus::ICommand *m_AuditionStop = nullptr;
	nucleus::ICommandSet *m_AuditionSet = nullptr;
	bool m_AuditionMode = false;
	void BeginAudition(nucleus::ICommand *pcmd);
	void UpdateAudition();
	void EndAudition();

    afx_msg void OnCaptureChanged(CWnd *window);
    afx_msg void OnCancelMode();

	static ClipCommandList s_Clipboard;
	static size_t s_ClipboardFirstRow;

	// Persistent painting resources; recreated only for a new size / DPI.
	CFont m_NameFont;
	CFont m_DescriptionFont;
	CFont m_ClefFont;
	CDC m_BackDC;
	CBitmap m_BackBitmap;
	HGDIOBJ m_OriginalBackBitmap = nullptr;
	CSize m_BackSize{};
	bool EnsureBackBuffer(CDC &paint, CSize size);
	void ReleaseBackBuffer();

	// Reuse knot storage across tracks and frames (the vectors retain capacity).
	struct EnvelopeKnot
	{
		double time, value, slope;
		nucleus::ICommand *command;
	};
	std::array<std::vector<EnvelopeKnot>, PossumEffects::Count> m_EnvelopeKnots;

	CToolTipCtrl m_LayerTooltip;
    CTrackMediaDropTarget m_MediaDropTarget;
	int m_HoverLayerRow = -1;
	int m_HoverLayer = -1;

	int m_PasteTargetRow = -1;
	int m_HeaderWidth = 150;
    int m_ScrollY = 0;

	CPoint m_Down{};
    CPoint m_Mouse{};
    CRect m_SelectionRect{};

	bool m_BoxSelecting = false;
    bool m_Dragging = false;
    bool m_ResizingRow = false;

    enum class ResizeEdge { None, Left, Right };
    ResizeEdge m_ResizeEdge = ResizeEdge::None;
    nucleus::ICommand *m_ResizeCommand = nullptr;
    nucleus::ITrack *m_ResizeTrack = nullptr;
    double m_ResizeStartTime = 0.0;
    double m_ResizeStartDuration = 0.0;
    struct ResizeCommand
    {
        nucleus::ICommand *command = nullptr;
        nucleus::ITrack *track = nullptr;
        double time = 0.0;
        double duration = 0.0;
    };
    std::vector<ResizeCommand> m_ResizeCommands;

    // Z+drag: stretch the selected commands about the opposite end of the
    // selection. Original times/durations remain stable throughout the drag.
    struct ScaleCommand
    {
        nucleus::ICommand *command = nullptr;
        nucleus::ITrack *track = nullptr;
        double time = 0.0;
        double duration = 0.0;
        double targetTime = 0.0;
        double targetDuration = 0.0;
        bool resizable = false;
    };
    std::vector<ScaleCommand> m_ScaleCommands;
    bool m_Scaling = false;
    double m_ScaleStart = 0.0;
    double m_ScaleEnd = 0.0;
    double m_ScaleHandleTime = 0.0;
    double m_ScaleAnchor = 0.0;
	double m_ScaleSnap = 0.0;
	double m_ScaleFactor = 1.0;

    bool BeginScale(nucleus::ICommand *grabbed, nucleus::ITrack *track, double handleTime, ResizeEdge edge = ResizeEdge::None);
    void UpdateScale(CPoint point);
    void EndScale(bool commit);

	nucleus::ICommand *m_EnvelopeDrag = nullptr;
    struct EnvelopeEdit
    {
        nucleus::ICommand *command = nullptr;
        double originalValue = 0.0;
		double originalTime = 0.0;
		std::shared_ptr<props::IPropertySet> before;
    };

	std::vector<EnvelopeEdit> m_EnvelopeEdits;
    int m_EnvelopeLayer = -1;
    bool m_EnvelopeHandle = false;

	int m_ResizeRow = -1;
    int m_ResizeHeight = 0;

	double m_DragStartTime = 0.0;
    double m_DragStartValue = 0.0;

	int m_DragStartRow = -1;

	struct SDragEntry
	{
		nucleus::ICommand *cmd;
		double origtime;
		double origval;
	};
	std::vector<SDragEntry> m_DragCommands;

	nucleus::ITrackSet* TrackSet() const;
    class CPossumDocument* Document() const;
    class CPossumDocView* View() const;

	int RowTop(int row) const;
    int RowAt(int y) const;
    int TimelineWidth() const;
    double TimeAt(int x) const;

	int XAt(double time) const;
    double ValueAt(int y, int row, const GridConfig& config) const;
    int YAt(double value, int row, const GridConfig& config) const;

	double SnapValue(double value, const GridConfig& config) const;
    double CommandValue(const nucleus::ICommand* cmd) const;
    void SetCommandValue(nucleus::ICommand* cmd, double value) const;

	CRect CommandRect(int row, nucleus::ICommand* cmd, double time, const GridConfig& config) const;
    ResizeEdge CommandResizeEdge(CPoint point, int row, nucleus::ICommand *command, double time) const;
    double ResizeDelta(CPoint point) const;
    void ResizePlacement(CPoint point, double &time, double &duration) const;

	Hit HitTest(CPoint point) const;

	void DrawEnvelopes(CDC &dc, nucleus::ITrack *track, int top, const GridConfig &grid, const CRect &client, double displayStart, double displayEnd);
    nucleus::ICommand *HitEnvelopePoint(CPoint point, int row, nucleus::ITrack *track, bool &handle) const;
	CPoint EnvelopeHandlePosition(const nucleus::ICommand *command, double time, int row, int layer) const;
	int EnvelopeY(double value, int row, int layer) const;
    double EnvelopeValue(int y, int row, int layer) const;

	CRect LayerButtonRect(int row, int layer) const;
	int HitLayerButton(CPoint point, int row) const;
	void UpdateLayerTooltip(CPoint point);
	virtual BOOL PreTranslateMessage(MSG *message);

	void UpdateScroll();

	bool CanEdit() const;
    
    void PasteAt(nucleus::ITrack *track, double time);
    void DeleteSelection();
    nucleus::ITrack *m_ContextTrack = nullptr;
    void InsertContextTrack(bool below);
public:
    bool InsertSampleFile(const TCHAR *file, CPoint point);
protected:

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnVScroll(UINT, UINT, CScrollBar*);
    afx_msg BOOL OnMouseWheel(UINT, short, CPoint);
    afx_msg void OnLButtonDown(UINT, CPoint);
    afx_msg void OnLButtonDblClk(UINT, CPoint);
    afx_msg void OnMouseMove(UINT, CPoint);
    afx_msg void OnLButtonUp(UINT, CPoint);
    afx_msg void OnKeyDown(UINT, UINT, UINT);
    afx_msg void OnDropFiles(HDROP);
    afx_msg void OnSetFocus(CWnd*);
    afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT);
    afx_msg void OnEditCopy();
    afx_msg void OnEditCut();
    afx_msg void OnEditPaste();
    afx_msg void OnEditClear();
	afx_msg void OnEditDuplicate();
	afx_msg void OnContextMenu(CWnd *window, CPoint screen);
    afx_msg void OnTrackInsertAbove();
    afx_msg void OnTrackInsertBelow();
    afx_msg void OnTrackRemove();
    afx_msg void OnTrackDuplicate();
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()
};
