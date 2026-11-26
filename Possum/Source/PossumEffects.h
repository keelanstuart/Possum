#pragma once
#include <NuTrackSet.h>
#include <NuTrack.h>
#include <array>

// Editor-only descriptions of Audio's effect commands. Nucleus knows nothing
// about track roles, envelope visibility, or display colors.
namespace PossumEffects
{
    enum Layer
	{
		Volume,
		Gain,
		Tremolo,
		PitchBend,
		Vibrato,
		Pan,
		Delay,
		Reverb,
		LowPass,
		HighPass,
		Count
	};

	struct Definition
	{
		const TCHAR *trackCommand, *globalCommand, *label;
		double minimum, maximum;
		COLORREF color;
	};

	const Definition &Get(size_t layer);

	int FindLayer(const nucleus::ICommand *command, bool global);
    bool IsMaster(const nucleus::ITrack *track);
    nucleus::ITrack *EnsureMaster(nucleus::ITrackSet *set);
    void InitializeTrack(nucleus::ITrack *track, bool master);

	unsigned Visible(const nucleus::ITrack *track);
    void SetVisible(nucleus::ITrack *track, unsigned mask);

	int Active(const nucleus::ITrack *track);
    void SetActive(nucleus::ITrack *track, int layer);

	COLORREF Color(const nucleus::ITrackSet *set, int layer);
    void SetColor(nucleus::ITrackSet *set, int layer, COLORREF color);

    // App-wide defaults: loaded from and saved to plain XML. These initialize
    // document properties only when the document is first created.
    class Defaults
    {
    public:
        Defaults();
        bool Load(const TCHAR *filename);
        bool Save(const TCHAR *filename) const;
        void Apply(nucleus::ITrackSet *set) const;

		COLORREF colors[Count];
        unsigned visible = (1u << Count) - 1;
        int active = Volume;
        int masterHeight = 85;
    };
}
