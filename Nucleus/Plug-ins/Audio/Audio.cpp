// Audio.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Audio.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <Nucleus.h>
#include <NuPlugin.h>
#include <NuDevice.h>
#include <NuDeviceManager.h>
#include <NuCommandInfo.h>
#include <NuCommand.h>
#include <NuTrack.h>
#include <NuTrackSet.h>
#include <mutex>
#include <cmath>
#include <array>
#include <cstdint>
#include <algorithm>
#include <map>
#include <vector>
#include <filesystem>
#include <memory>
#include <string>
#include <limits>
#include <atomic>
#include <fstream>
#include <cstring>

// Audio engine configuration and DSP tuning. Kept together for easy auditing.
namespace
{
	constexpr ma_uint32 AUDIO_SAMPLE_RATE = 48000;
	constexpr ma_uint32 AUDIO_OUTPUT_CHANNELS = 2;
	constexpr double TWO_PI = 6.2831853071795864769;
	constexpr double SEMITONES_PER_OCTAVE = 12.0;
	constexpr float DECIBELS_PER_AMPLITUDE_DECADE = 20.0f;
	constexpr size_t INITIAL_VOICE_CAPACITY = 5000;
	constexpr float DEFAULT_TONE_FREQUENCY_HZ = 261.63f;
	constexpr float DEFAULT_VOICE_AMPLITUDE = 0.15f;
	constexpr float DEFAULT_EVENT_DURATION_SECONDS = 0.25f;
	constexpr float DEFAULT_LOWPASS_CUTOFF_HZ = 20000.0f;
	constexpr float DEFAULT_HIGHPASS_CUTOFF_HZ = 0.0f;
	constexpr float MAX_FILTER_CUTOFF_HZ = AUDIO_SAMPLE_RATE * 0.5f - 1.0f;
	constexpr float MODULATION_RATE_HZ = 5.0f;
	constexpr float TONE_PEAK_PROPORTION = 0.1f;
	constexpr float TONE_SUSTAIN_PROPORTION = 0.6f;
	constexpr float TONE_SUSTAIN_BALANCE = 0.5f;
	constexpr float TONE_SUSTAIN_LEVEL = 0.9f;
	constexpr float TONE_SUSTAIN_FALLOFF = 0.1f;
	constexpr float NOISE_PEAK_PROPORTION = 0.0f;
	constexpr float NOISE_SUSTAIN_PROPORTION = 1.0f;
	constexpr float NOISE_SUSTAIN_BALANCE = 0.0f;
	constexpr float NOISE_SUSTAIN_LEVEL = 1.0f;
	constexpr float NOISE_SUSTAIN_FALLOFF = 0.0f;
	constexpr float PAN_MIN = -1.0f;
	constexpr float PAN_MAX = 1.0f;
	constexpr float WET_MIN = 0.0f;
	constexpr float WET_MAX = 1.0f;
	constexpr float DELAY_TIME_SECONDS = 0.350f;
	constexpr float DELAY_FEEDBACK = 0.35f;
	constexpr float REVERB_FEEDBACK = 0.72f;
	constexpr float REVERB_DAMPING_INPUT = 0.70f;
	constexpr float REVERB_DAMPING_HISTORY = 1.0f - REVERB_DAMPING_INPUT;
	constexpr size_t REVERB_COMB_COUNT = 4;
	constexpr std::array<size_t, REVERB_COMB_COUNT> REVERB_COMB_LENGTHS = { 1499, 1601, 1867, 1999 };
	constexpr float REVERB_COMB_OUTPUT_LEVEL = 1.0f / float(REVERB_COMB_COUNT);
	constexpr float BROWN_NOISE_STEP = 0.02f;
	constexpr float BROWN_NOISE_DAMPING = 0.9995f;
	constexpr float BROWN_NOISE_OUTPUT_GAIN = 3.0f;
	constexpr float NOISE_DIFFERENCE_GAIN = 0.5f;
	constexpr float BLUE_NOISE_OUTPUT_GAIN = 2.0f;
	constexpr float PINK_NOISE_OUTPUT_GAIN = 0.11f;
	constexpr float PINK_NOISE_DIRECT_GAIN = 0.5362f;
	constexpr float PINK_NOISE_LAST_GAIN = 0.115926f;
	constexpr uint32_t NOISE_RANDOM_SEED = 0x9E3779B9u;
	constexpr size_t SAMPLE_DECODE_CHUNK_FRAMES = 16384;
	constexpr uint64_t SAMPLE_GRAIN_HOP_FRAMES = 512;
	constexpr double SAMPLE_DIRECT_SPEED_EPSILON = 0.000001;
	constexpr float DEFAULT_SAMPLE_PITCH_MULTIPLIER = 1.0f;
	constexpr float SAMPLE_VOICE_AMPLITUDE = 1.0f;
	constexpr float SAMPLE_MAX_PITCH_MULTIPLIER = 16.0f;
	constexpr float SAMPLE_MIN_PITCH_MULTIPLIER = 0.0625f;
	constexpr float UINT32_TO_BIPOLAR = 2.0f / 4294967296.0f;
	constexpr std::array<float, 6> PINK_FILTER_FEEDBACK = {0.99886f, 0.99332f, 0.96900f, 0.86650f, 0.55000f, -0.7616f};
	constexpr std::array<float, 6> PINK_FILTER_INPUT = {0.0555179f, 0.0750759f, 0.1538520f, 0.3104856f, 0.5329522f, -0.0168980f};
}


using namespace std;


NUPLUGIN_NAME("Audio");
NUPLUGIN_SHORTNAME("Audio");
NUPLUGIN_AUTHOR("Keelan Stuart");
NUPLUGIN_WEBSITE("");
NUPLUGIN_DESC("Provides audio playback mechanisms");
NUPLUGIN_COPYRIGHT("2019-2026, Keelan Stuart");
NUPLUGIN_VERSION(1000);
NUPLUGIN_NUCLEUSVERSION();		// required to determine if this plug-in is compatible with the version of Nucleus that's loading it
NUPLUGIN_GETPLATFORM();
NUPLUGIN_ISUNICODE();


class CAudioDevice : nucleus::IDevice
{

public:
	using WaveformType = enum { Sine, Square, Triangle, Sawtooth };
	using NoiseType = enum { Brown, Pink, White, Blue, Violet };

	// The array indices are shared by track and composition effects.
	enum Effect
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
		NumEffects
	};

	struct SEffectInfo
	{
		struct SPoint
		{
			double m_Time = 0.0;
			double m_Value = 0.0;
			props::TVec2F m_Handle{};
		} m_Start, m_End;
		bool active = false;
	};

	// Hermite tangent directions are supplied as (dx, dy) in timeline units.
	static void ReadEffectPoint(const nucleus::ICommand *cmd, SEffectInfo::SPoint &point)
	{
		cmd->GetOwner(&point.m_Time);

		const props::IPropertySet *params = cmd->GetParams();
		const props::IProperty *value = params->GetPropertyById('CPNT');
		const props::IProperty *tangent = params->GetPropertyById('CVEC');
		point.m_Value = value ? value->AsFloat() : 0.0;

		if (tangent)
		{
			const props::TVec2F *vec = tangent->AsVec2F();
			if (vec)
				point.m_Handle = *vec;
		}
	}

	static float Evaluate(const SEffectInfo &segment, double time, float fallback)
	{
		if (!segment.active)
			return fallback;

		const auto &a = segment.m_Start;
		const auto &b = segment.m_End;
		const double delta = b.m_Time - a.m_Time;

		if (delta <= 0.0 || time <= a.m_Time)
			return float(a.m_Value);

		if (time >= b.m_Time)
			return float(b.m_Value);

		const double u = (time - a.m_Time) / delta;
		const double u2 = u * u, u3 = u2 * u;
		const double m0 = a.m_Handle.x == 0.0f ? 0.0 : double(a.m_Handle.y) / a.m_Handle.x;
		const double m1 = b.m_Handle.x == 0.0f ? 0.0 : double(b.m_Handle.y) / b.m_Handle.x;

		return float((2*u3 - 3*u2 + 1)*a.m_Value + (u3 - 2*u2 + u)*delta*m0 +
					 (-2*u3 + 3*u2)*b.m_Value + (u3 - u2)*delta*m1);
	}


protected:

	BEGIN_COMMAND_DEFINITION(Tone, "Tone Generator")
		AddParam(_T("Duration (s)"),			'DURA', DEFAULT_EVENT_DURATION_SECONDS);
		AddParam(_T("Waveform Shape"),			'SHAP', _T("Sine,Square,Triangle,Sawtooth"), size_t(WaveformType::Sine));
		AddParam(_T("Frequency (Hz)"),			'FREQ', DEFAULT_TONE_FREQUENCY_HZ);
		AddParam(_T("Peak Proportion (%)"),		'PEAK', TONE_PEAK_PROPORTION);
		AddParam(_T("Sustain Proportion (%)"),	'SUSP', TONE_SUSTAIN_PROPORTION);
		AddParam(_T("Sustain Balance (%)"),		'SUSB', TONE_SUSTAIN_BALANCE);
		AddParam(_T("Sustain Level (%)"),		'SUSL', TONE_SUSTAIN_LEVEL);
		AddParam(_T("Sustain Falloff (%)"),		'SUSF', TONE_SUSTAIN_FALLOFF);
		AddParam(_T("Icon"),					'ICON', _T("./icons/icon_tone_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(Tone);

	BEGIN_COMMAND_DEFINITION(Noise, "Noise Generator")
		AddParam(_T("Duration (s)"),			'DURA', DEFAULT_EVENT_DURATION_SECONDS);
		AddParam(_T("Noise Type"),				'SHAP', _T("Brown,Pink,White,Blue,Violet"), size_t(NoiseType::White));
		AddParam(_T("Frequency (Hz)"),			'FREQ', DEFAULT_TONE_FREQUENCY_HZ);
		AddParam(_T("Peak Proportion (%)"),		'PEAK', NOISE_PEAK_PROPORTION);
		AddParam(_T("Sustain Proportion (%)"),	'SUSP', NOISE_SUSTAIN_PROPORTION);
		AddParam(_T("Sustain Balance (%)"),		'SUSB', NOISE_SUSTAIN_BALANCE);
		AddParam(_T("Sustain Level (%)"),		'SUSL', NOISE_SUSTAIN_LEVEL);
		AddParam(_T("Sustain Falloff (%)"),		'SUSF', NOISE_SUSTAIN_FALLOFF);
		AddParam(_T("Icon"),					'ICON', _T("./icons/icon_noise_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(Noise);

	BEGIN_COMMAND_DEFINITION(Sample, "File Playback")
		AddParam(_T("Sample Path"),				'PATH', _T(""), props::IProperty::PA_FILENAME);
		AddParam(_T("Duration (s)"),			'DURA', DEFAULT_EVENT_DURATION_SECONDS);
		AddParam(_T("Pitch Multiplier"),		'PMUL', 1.0f, props::IProperty::PA_GENERIC, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
		AddParam(_T("Icon"),					'ICON', _T("./icons/icon_sample_color_normal.png"), props::IProperty::PA_FILENAME, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
		SetOnCreatedHandler([this](nucleus::ICommand *pcmd)
		{
			pcmd->SetPropertyChangeHandler([this](nucleus::ICommand *command, const props::IProperty *pprop)
			{
				if (pprop->GetID() == 'PATH')
				{
					CAudioDevice *pad = (CAudioDevice *)this->GetDevice();
					auto ps = pad->GetCachedSample(pprop->AsString());
					props::IProperty *pdp = command->GetParams()->GetPropertyById('DURA');
					if (pdp && ps)
						pdp->SetFloat((float)ps->durationSeconds);
				}
			});
		});
		END_COMMAND_DEFINITION(Sample);

	// Editor-generated resource dependency: executed synchronously before the player clock starts.
	BEGIN_COMMAND_DEFINITION(PreloadSample, "Preload Audio Sample")
		AddParam(_T("Sample Path"), 'PATH', _T(""), props::IProperty::PA_FILENAME);
		AddParam(_T("Prepare Before Playback"), 'PREP', true, props::IProperty::PA_GENERIC, props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
	END_COMMAND_DEFINITION(PreloadSample);

	// Instantansous, editor-only command, not for timeline insertion: exports a nucleus command stream file to an audio file
	BEGIN_COMMAND_DEFINITION(Export, "Export")
		AddParam(_T("Input Path"), 'IN', _T(""), props::IProperty::PA_FILENAME);
		AddParam(_T("Output Path"), 'OUT', _T(""), props::IProperty::PA_FILENAME);
	END_COMMAND_DEFINITION(Export);


	// No icon property means it's implicit and won't show up in a list of commands or be drawn on a timeline the same way


	// Additional Possum-style automation. ControlVec is the Hermite tangent (dx, dy).
	// Hermite control points drive the live mixer DSP.
#define DEFINE_EFFECT_COMMAND(name, description, initial) \
	BEGIN_COMMAND_DEFINITION(name, description) \
		AddParam(_T("ControlPoint"), 'CPNT', initial); \
		AddParam(_T("ControlVec"), 'CVEC', props::TVec2F(1.0f, 0.0f)); \
	END_COMMAND_DEFINITION(name)

	DEFINE_EFFECT_COMMAND(TrackVolume, "Track Volume Envelope", 1.0f);
	DEFINE_EFFECT_COMMAND(TrackGain, "Track Gain Envelope", 0.0f);
	DEFINE_EFFECT_COMMAND(TrackTremolo, "Track Tremolo Effect Envelope", 0.0f);
	DEFINE_EFFECT_COMMAND(TrackPitchBend, "Track Pitch Bend (semitones)", 0.0f);
	DEFINE_EFFECT_COMMAND(TrackVibrato, "Track Vibrato Depth (semitones)", 0.0f);
	DEFINE_EFFECT_COMMAND(TrackPan, "Track Pan (-1 left, +1 right)", 0.0f);
	DEFINE_EFFECT_COMMAND(TrackDelay, "Track Delay Mix", 0.0f);
	DEFINE_EFFECT_COMMAND(TrackReverb, "Track Reverb Mix", 0.0f);
	DEFINE_EFFECT_COMMAND(TrackLowPass, "Track Low-Pass Cutoff (Hz)", DEFAULT_LOWPASS_CUTOFF_HZ);
	DEFINE_EFFECT_COMMAND(TrackHighPass, "Track High-Pass Cutoff (Hz)", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalVolume, "Global Volume Envelope", 1.0f);
	DEFINE_EFFECT_COMMAND(GlobalGain, "GlobalGain Envelope", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalTremolo, "Global Tremolo Effect Envelope", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalPitchBend, "Global Pitch Bend (semitones)", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalVibrato, "Global Vibrato Depth (semitones)", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalPan, "Global Pan (-1 left, +1 right)", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalDelay, "Global Delay Mix", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalReverb, "Global Reverb Mix", 0.0f);
	DEFINE_EFFECT_COMMAND(GlobalLowPass, "Global Low-Pass Cutoff (Hz)", DEFAULT_LOWPASS_CUTOFF_HZ);
	DEFINE_EFFECT_COMMAND(GlobalHighPass, "Global High-Pass Cutoff (Hz)", 0.0f);

#undef DEFINE_EFFECT_COMMAND

	// Internal commands: no ICON, so these do not appear as timeline content.
	BEGIN_COMMAND_DEFINITION(StartAudition, "Begin audition")
	END_COMMAND_DEFINITION(StartAudition);
	BEGIN_COMMAND_DEFINITION(UpdateAudition, "Update audition")
	END_COMMAND_DEFINITION(UpdateAudition);
	BEGIN_COMMAND_DEFINITION(StopAudition, "End audition")
	END_COMMAND_DEFINITION(StopAudition);

	COMMAND_REGISTRY();

public:
	CAudioDevice()
	{
		m_pSys = nullptr;
		m_pProps = props::IPropertySet::CreatePropertySet();
		m_DeviceReady = false;
		m_Voices.reserve(INITIAL_VOICE_CAPACITY);
		m_HasTimeline = false;
		m_MixTime = 0.0;
		m_MeterL = m_MeterR = 0.0f;

		if (props::IProperty *pp = m_pProps->CreateReferenceProperty(_T("Meter (Left Channel)"), 'MTRL', &m_MeterL, props::IProperty::PT_FLOAT))
		{
			pp->Flags().Set(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
		}

		if (props::IProperty *pp = m_pProps->CreateReferenceProperty(_T("Meter (Right Channel)"), 'MTRR', &m_MeterR, props::IProperty::PT_FLOAT))
		{
			pp->Flags().Set(props::IProperty::PROPFLAG(props::IProperty::EPropFlag::HIDDEN));
		}

		REGISTER_COMMAND(StartAudition);
		REGISTER_COMMAND(UpdateAudition);
		REGISTER_COMMAND(StopAudition);
		REGISTER_COMMAND(Tone);
		REGISTER_COMMAND(Noise);
		REGISTER_COMMAND(Sample);
		REGISTER_COMMAND(PreloadSample);
		REGISTER_COMMAND(Export);

		REGISTER_COMMAND(TrackVolume);
		REGISTER_COMMAND(TrackGain);
		REGISTER_COMMAND(TrackTremolo);
		REGISTER_COMMAND(TrackPitchBend);
		REGISTER_COMMAND(TrackVibrato);
		REGISTER_COMMAND(TrackPan);
		REGISTER_COMMAND(TrackDelay);
		REGISTER_COMMAND(TrackReverb);
		REGISTER_COMMAND(TrackLowPass);
		REGISTER_COMMAND(TrackHighPass);

		REGISTER_COMMAND(GlobalVolume);
		REGISTER_COMMAND(GlobalGain);
		REGISTER_COMMAND(GlobalTremolo);
		REGISTER_COMMAND(GlobalPitchBend);
		REGISTER_COMMAND(GlobalVibrato);
		REGISTER_COMMAND(GlobalPan);
		REGISTER_COMMAND(GlobalDelay);
		REGISTER_COMMAND(GlobalReverb);
		REGISTER_COMMAND(GlobalLowPass);
		REGISTER_COMMAND(GlobalHighPass);
	}

	virtual ~CAudioDevice()
	{
		Shutdown();

		if (m_pProps)
			m_pProps->Release();

		RELEASE_COMMANDS();
	}

	virtual props::IPropertySet *GetProperties() { return m_pProps; }

	/// Initializes the device, giving it the INucleus interface that it is registered with
	virtual nucleus::RetCode Initialize(const nucleus::INucleus *psys)
	{
		m_pSys = (nucleus::INucleus *)psys;
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32;
		config.playback.channels = AUDIO_OUTPUT_CHANNELS;
		config.sampleRate = AUDIO_SAMPLE_RATE;
		config.dataCallback = AudioCallback;
		config.pUserData = this;

		if (ma_device_init(nullptr, &config, &m_Device) != MA_SUCCESS)
			return nucleus::RetCode::UNKNOWN_ERROR;

		m_DeviceReady = true;

		if (ma_device_start(&m_Device) != MA_SUCCESS)
		{
			Shutdown();
			return nucleus::RetCode::UNKNOWN_ERROR;
		}

		return nucleus::RetCode::OK;
	}

	virtual nucleus::RetCode Shutdown()
	{
		if (m_DeviceReady)
		{
			ma_device_uninit(&m_Device);
			m_DeviceReady = false;
		}

		std::lock_guard<std::mutex> lock(m_VoiceLock);

		m_MeterL = m_MeterR = 0.0f;
		StopAudition();
		m_Voices.clear();
		m_SampleVoices.clear();
		m_SampleCache.clear();
		m_TrackEffectControls.clear();
		m_TrackDSP.clear();
		m_GlobalDSP = {};
		m_GlobalEffectControls = {};
		m_HasTimeline = false;
		m_MixTime = 0.0;

		return nucleus::RetCode::OK;
	}

	virtual nucleus::RetCode Reset()
	{
		std::lock_guard<std::mutex> lock(m_VoiceLock);

		m_MeterL = m_MeterR = 0.0f;
		StopAudition();
		m_Voices.clear();
		m_SampleVoices.clear();
		m_TrackEffectControls.clear();
		m_TrackDSP.clear();
		m_GlobalDSP = {};
		m_GlobalEffectControls = {};
		m_HasTimeline = false;
		m_MixTime = 0.0;
		m_Paused = false;

		return nucleus::RetCode::OK;
	}

	virtual nucleus::RetCode SetPaused(bool pause)
	{
		m_Paused = pause;

		return nucleus::RetCode::OK;
	}

	/// Performs any updates that the device may require, giving it a chance to poll, etc.
	virtual void Update(double time)
	{

	}

	/// Reports the status of the device, given then 
	virtual nucleus::IDevice::Status GetStatus() const
	{
		return Ready;
	}

	/// Returns the name of the device
	virtual const TCHAR *GetName() const
	{
		return _T("Audio");
	}

	/// Returns a description of the device
	virtual const TCHAR *GetDescription() const
	{
		return _T("Provides audio rendering");
	}

	/// Causes the device to take action; executes the given command with a set of arguments
	virtual nucleus::RetCode ExecuteCommand(const nucleus::ICommand *pcmd, double time, nucleus::ICommand **pcmd_undo = nullptr)
	{
		const nucleus::ICommandInfo *pcmdinfo = pcmd->GetCommandInfo();
		const CAudioDevice *identity = m_CommandIdentity ? m_CommandIdentity : this;
		if (pcmdinfo->GetDevice() != identity)
			return nucleus::RetCode::COMMAND_GIVEN_TO_WRONG_DEVICE;

		const nucleus::ITrack *pt = pcmd->GetOwner();
		const props::IPropertySet *params = pcmd->GetParams();

		const auto ReadFloatProp = [params](int id, float fallback) -> float
		{
			const props::IProperty *p = params->GetPropertyById(id);
			return p ? p->AsFloat() : fallback;
		};

		const auto ReadIntProp = [params](int id, int64_t fallback) -> int64_t
		{
			const props::IProperty *p = params->GetPropertyById(id);
			return p ? p->AsInt() : fallback;
		};

        if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(StartAudition) ||
            pcmdinfo == identity->COMMAND_INSTPTR_NAME(UpdateAudition) ||
            pcmdinfo == identity->COMMAND_INSTPTR_NAME(StopAudition))
        {
            std::lock_guard<std::mutex> lock(m_VoiceLock);
			if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(StartAudition))
			{
				StartAudition(time);
				return nucleus::RetCode::OK;
			}
			else if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(StopAudition))
			{
				StopAudition();
				return nucleus::RetCode::OK;
			}
            else if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(UpdateAudition))
            {
				static int test = 0;
				test++;
				if (test > 10)
					test = 0;
                RefreshAudition();
                m_AuditionReady = true;
				return nucleus::RetCode::OK;
			}
        }

		if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(Tone) || pcmdinfo == identity->COMMAND_INSTPTR_NAME(Noise))
		{
			bool noise = pcmdinfo == identity->COMMAND_INSTPTR_NAME(Noise);

			SVoice voice{};
			voice.track = pt;
			voice.command = m_IsAudition ? pcmd : nullptr;
			voice.noise = noise;
			voice.frequency = ReadFloatProp('FREQ', DEFAULT_TONE_FREQUENCY_HZ);
			voice.framesRemaining = (uint64_t)(ReadFloatProp('DURA', 0.0) * double(AUDIO_SAMPLE_RATE));
			voice.totalFrames = voice.framesRemaining;
			voice.peak = std::clamp(ReadFloatProp('PEAK', noise ? NOISE_PEAK_PROPORTION : TONE_PEAK_PROPORTION), 0.0f, 1.0f);
			voice.sustain = std::clamp(ReadFloatProp('SUSP', noise ? NOISE_SUSTAIN_PROPORTION : TONE_SUSTAIN_PROPORTION), 0.0f, 1.0f);
			voice.balance = std::clamp(ReadFloatProp('SUSB', noise ? NOISE_SUSTAIN_BALANCE : TONE_SUSTAIN_BALANCE), 0.0f, 1.0f);
			voice.level = std::clamp(ReadFloatProp('SUSL', noise ? NOISE_SUSTAIN_LEVEL : TONE_SUSTAIN_LEVEL), 0.0f, 1.0f);
			voice.falloff = std::clamp(ReadFloatProp('SUSF', noise ? NOISE_SUSTAIN_FALLOFF : TONE_SUSTAIN_FALLOFF), 0.0f, 1.0f);
			voice.phase = 0.0;
			voice.vibratoPhase = 0.0;
			voice.gain = DEFAULT_VOICE_AMPLITUDE;
			if (m_IsAudition)
			{
				voice.framesRemaining = 1;
				voice.totalFrames = 1;
			}

			if (noise)
				voice.noiseType = (NoiseType)ReadIntProp('SHAP', NoiseType::White);
			else
				voice.shape = (WaveformType)ReadIntProp('SHAP', WaveformType::Sine);

			// Give each noise voice independent deterministic PRNG state. No rand() in the callback.
			voice.rng = NOISE_RANDOM_SEED ^ uint32_t(reinterpret_cast<uintptr_t>(pcmdinfo));
			if (!voice.rng)
				voice.rng = 1;

			if (voice.framesRemaining && (noise || voice.frequency > 0.0))
			{
				std::lock_guard<std::mutex> lock(m_VoiceLock);

				if (!m_HasTimeline)
				{
					m_MixTime = time;
					m_HasTimeline = true;
				}

				m_TrackDSP.try_emplace(pt); // A voice needs a bus even with no effect commands.
				m_Voices.push_back(voice);
			}
		}
		else if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(PreloadSample))
		{
			const props::IProperty *path = params->GetPropertyById('PATH');
			return path ? PreloadSampleFile(path->AsString()) : nucleus::RetCode::UNKNOWN_ERROR;
		}
		else if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(Sample))
		{
			const props::IProperty *path = params->GetPropertyById('PATH');
			if (!path || !path->AsString() || !*path->AsString())
				return nucleus::RetCode::UNKNOWN_ERROR;

			// Normally a fast cache lookup: the editor-generated preload at 0 prepared the file.
			// As a fallback, tolerate hand-authored streams without preload commands.
			const auto audio = GetCachedSample(path->AsString());
			if (!audio || audio->pcm.empty())
				return nucleus::RetCode::UNKNOWN_ERROR;

			const double duration = m_IsAudition ? audio->durationSeconds : ReadFloatProp('DURA', 0.0);
			if (duration <= 0.0)
				return nucleus::RetCode::OK;
			const props::IProperty *pitch = params->GetPropertyById('PMUL');
			const float multiplier = pitch ? pitch->AsFloat() : DEFAULT_SAMPLE_PITCH_MULTIPLIER;

			SSampleVoice voice{};
			voice.track = pt;
			voice.command = m_IsAudition ? pcmd : nullptr;
			voice.audio = audio;
			voice.outputFrames = std::max<uint64_t>(1, uint64_t(duration * double(AUDIO_SAMPLE_RATE)));
			voice.sourceFramesPerOutputFrame = double(audio->pcm.size() / AUDIO_OUTPUT_CHANNELS) / double(voice.outputFrames);
			voice.pitchMultiplier = std::clamp(multiplier, SAMPLE_MIN_PITCH_MULTIPLIER, SAMPLE_MAX_PITCH_MULTIPLIER);
            voice.loop = m_IsAudition;

			std::lock_guard<std::mutex> lock(m_VoiceLock);
			if (!m_HasTimeline)
			{
				m_MixTime = time;
				m_HasTimeline = true;
			}

			m_TrackDSP.try_emplace(pt);
			m_SampleVoices.push_back(std::move(voice));
		}
		else if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(Export))
		{
			// the editor will have to save to a temporary file and use it as 'IN'
			const props::IProperty *input_path = params->GetPropertyById('IN');
			const props::IProperty *output_path = params->GetPropertyById('OUT');

			if (!input_path || !output_path)
				return nucleus::RetCode::UNKNOWN_ERROR;

			const TCHAR *pins = input_path->AsString();
			const TCHAR *pouts = output_path->AsString();

			if (!pins || !*pins || !pouts || !*pouts)
				return nucleus::RetCode::UNKNOWN_ERROR;

			// Do not recursively export files containing Export commands.
			if (m_CommandIdentity)
				return nucleus::RetCode::UNKNOWN_ERROR;

			return ExportToWav(input_path->AsString(), output_path->AsString());
		}
		else
		{
			bool global = false;
			Effect effect = Effect::NumEffects;

#define MAP_EFFECT(name, target, scope) \
			if (pcmdinfo == identity->COMMAND_INSTPTR_NAME(name)) { effect = Effect::target; global = scope; } else

			MAP_EFFECT(TrackVolume, Volume, false)
			MAP_EFFECT(TrackGain, Gain, false)
			MAP_EFFECT(TrackTremolo, Tremolo, false)
			MAP_EFFECT(TrackPitchBend, PitchBend, false)
			MAP_EFFECT(TrackVibrato, Vibrato, false)
			MAP_EFFECT(TrackPan, Pan, false)
			MAP_EFFECT(TrackDelay, Delay, false)
			MAP_EFFECT(TrackReverb, Reverb, false)
			MAP_EFFECT(TrackLowPass, LowPass, false)
			MAP_EFFECT(TrackHighPass, HighPass, false)

			MAP_EFFECT(GlobalVolume, Volume, true)
			MAP_EFFECT(GlobalGain, Gain, true)
			MAP_EFFECT(GlobalTremolo, Tremolo, true)
			MAP_EFFECT(GlobalPitchBend, PitchBend, true)
			MAP_EFFECT(GlobalVibrato, Vibrato, true)
			MAP_EFFECT(GlobalPan, Pan, true)
			MAP_EFFECT(GlobalDelay, Delay, true)
			MAP_EFFECT(GlobalReverb, Reverb, true)
			MAP_EFFECT(GlobalLowPass, LowPass, true)
			MAP_EFFECT(GlobalHighPass, HighPass, true)
			{ }

#undef MAP_EFFECT

			if (effect != Effect::NumEffects && pt)
			{
				SEffectInfo segment{};
				ReadEffectPoint(pcmd, segment.m_Start);
				const nucleus::ICommand *next = pt->GetNextCommand(pcmd, pcmdinfo);

				if (next)
					ReadEffectPoint(next, segment.m_End);
				else
					segment.m_End = segment.m_Start;

				segment.active = true;

				std::lock_guard<std::mutex> lock(m_VoiceLock);

				if (!m_HasTimeline)
				{
					// For mid-show startup, the player dispatches the preceding
					// control point at the requested playback position. The
					// segment endpoints still retain their original timestamps.
					m_MixTime = time;
					m_HasTimeline = true;
				}

				if (global)
				{
					m_GlobalEffectControls[effect] = segment;
					PrepareStage(m_GlobalDSP, effect);
				}
				else
				{
					m_TrackEffectControls[pt][effect] = segment;
					PrepareStage(m_TrackDSP[pt], effect);
				}
			}
		}

		return nucleus::RetCode::OK;
	}

protected:
	struct SCachedSample
	{
		std::vector<float> pcm; // Interleaved stereo 48 kHz float32, ready to render.
		std::filesystem::file_time_type modified{};
		uintmax_t fileSize = 0;
		double durationSeconds = 0.0;
	};

	struct SSampleVoice
	{
        const nucleus::ICommand *command = nullptr;
        bool loop = false;
        double auditionPosition = 0.0;
		const nucleus::ITrack *track = nullptr;
		std::shared_ptr<const SCachedSample> audio;
		uint64_t outputFrames = 0;
		uint64_t playedFrames = 0;
		double sourceFramesPerOutputFrame = 1.0;
		float pitchMultiplier = DEFAULT_SAMPLE_PITCH_MULTIPLIER;
		double vibratoPhase = 0.0;
	};

	// Decode on a command/prepare thread, never on the miniaudio output callback.
	std::shared_ptr<const SCachedSample> GetCachedSample(const TCHAR *filename)
	{
		if (!filename || !*filename)
			return {};

		std::filesystem::path path(filename);
		std::error_code ec;

		auto normalized = std::filesystem::absolute(path, ec);
		if (ec)
			return {};

		path = normalized.lexically_normal();
		const auto modified = std::filesystem::last_write_time(path, ec);
		if (ec)
			return {};

		const auto fileSize = std::filesystem::file_size(path, ec);
		if (ec)
			return {};

		const auto key = path;

		{
			std::lock_guard<std::mutex> lock(m_VoiceLock);
			auto found = m_SampleCache.find(key);
			if (found != m_SampleCache.end() && found->second->modified == modified && found->second->fileSize == fileSize)
				return found->second;
		}

		ma_decoder decoder{};
		ma_decoder_config config = ma_decoder_config_init(ma_format_f32, AUDIO_OUTPUT_CHANNELS, AUDIO_SAMPLE_RATE);

#ifdef _UNICODE
		const ma_result opened = ma_decoder_init_file_w(path.c_str(), &config, &decoder);
#else
		const ma_result opened = ma_decoder_init_file(path.string().c_str(), &config, &decoder);
#endif

		if (opened != MA_SUCCESS)
			return {};

		auto decoded = std::make_shared<SCachedSample>();
		decoded->modified = modified;
		decoded->fileSize = fileSize;
		// Keep the decode scratch buffer off the stack (player thread stacks can be small).
		std::vector<float> buffer(SAMPLE_DECODE_CHUNK_FRAMES * AUDIO_OUTPUT_CHANNELS);

		ma_uint64 read = 0;
		ma_result result = MA_SUCCESS;
		for (;;)
		{
			read = 0;
			result = ma_decoder_read_pcm_frames(
				&decoder, buffer.data(), SAMPLE_DECODE_CHUNK_FRAMES, &read);

			decoded->pcm.insert(decoded->pcm.end(), buffer.data(), buffer.data() + size_t(read) * AUDIO_OUTPUT_CHANNELS);

			if (result != MA_SUCCESS || read == 0)
				break;
		}

		ma_decoder_uninit(&decoder);

		if (decoded->pcm.empty() ||	(result != MA_SUCCESS && result != MA_AT_END))
			return {};

		decoded->durationSeconds = double(decoded->pcm.size() / AUDIO_OUTPUT_CHANNELS) / double(AUDIO_SAMPLE_RATE);

		std::lock_guard<std::mutex> lock(m_VoiceLock);
		m_SampleCache[key] = decoded; // Active voices retain their own immutable data.
		return decoded;
	}

	nucleus::RetCode PreloadSampleFile(const TCHAR *filename)
	{
		return GetCachedSample(filename) ? nucleus::RetCode::OK : nucleus::RetCode::UNKNOWN_ERROR;
	}

	static void ReadSampleAt(const SCachedSample &audio, double position, float &left, float &right, bool loop = false)
	{
		const size_t frames = audio.pcm.size() / AUDIO_OUTPUT_CHANNELS;
		if (position < 0.0 || position >= double(frames) || !frames)
		{
			left = right = 0.0f;
			return;
		}

		size_t a = size_t(position);
		size_t b = loop ? (a + 1) % frames : std::min<size_t>(a + 1, frames - 1);

		float fraction = float(position - double(a));

		left = audio.pcm[a * 2] + (audio.pcm[b * 2] - audio.pcm[a * 2]) * fraction;
		right = audio.pcm[a * 2 + 1] + (audio.pcm[b * 2 + 1] - audio.pcm[a * 2 + 1]) * fraction;
	}

	static void NextSample(SSampleVoice &voice, double pitchSemitones, float &left, float &right)
	{
        if (voice.loop)
        {
            const double frames = double(voice.audio->pcm.size() / AUDIO_OUTPUT_CHANNELS);
            ReadSampleAt(*voice.audio, voice.auditionPosition, left, right, true);
            voice.auditionPosition = std::fmod(voice.auditionPosition +
                double(voice.pitchMultiplier) * std::exp2(pitchSemitones / SEMITONES_PER_OCTAVE), frames);
            return;
        }
		if (voice.playedFrames >= voice.outputFrames)
		{
			left = right = 0.0f;
			return;
		}

		double transposition = double(voice.pitchMultiplier) * std::exp2(pitchSemitones / SEMITONES_PER_OCTAVE);
		double current = double(voice.playedFrames);

		// Straight playback is exact and avoids unnecessary overlap-add coloration.
		if (std::abs(voice.sourceFramesPerOutputFrame - 1.0) < SAMPLE_DIRECT_SPEED_EPSILON &&
			std::abs(transposition - 1.0) < SAMPLE_DIRECT_SPEED_EPSILON)
		{
			ReadSampleAt(*voice.audio, current, left, right);
		}
		else
		{
			// Two overlapping triangular-window grains. Their 50% overlap gives
			// unity total weight while each grain advances at the transposed pitch.
			// The grain origins advance with the time-stretch ratio independently.
			uint64_t hop = voice.playedFrames / SAMPLE_GRAIN_HOP_FRAMES;
			double offset = double(voice.playedFrames % SAMPLE_GRAIN_HOP_FRAMES);
			double grainBase = double(hop * SAMPLE_GRAIN_HOP_FRAMES) * voice.sourceFramesPerOutputFrame;

			float newerL = 0.0f, newerR = 0.0f;
			float olderL = 0.0f, olderR = 0.0f;

			ReadSampleAt(*voice.audio, grainBase + offset * transposition, newerL, newerR);
			if (hop > 0)
			{
				const double previousBase = double((hop - 1) * SAMPLE_GRAIN_HOP_FRAMES) * voice.sourceFramesPerOutputFrame;
				ReadSampleAt(*voice.audio, previousBase + (offset + SAMPLE_GRAIN_HOP_FRAMES) * transposition, olderL, olderR);
			}

			float w = float(offset / double(SAMPLE_GRAIN_HOP_FRAMES));
			left = newerL * w + olderL * (1.0f - w);
			right = newerR * w + olderR * (1.0f - w);

			// Start cleanly without requiring a negative-time grain.
			if (hop == 0)
			{
				left = newerL;
				right = newerR;
			}
		}

		voice.playedFrames++;
	}

	struct SVoice
	{
        const nucleus::ICommand *command = nullptr;
		const nucleus::ITrack *track;
		double phase;
		double frequency;
		double vibratoPhase;
		float gain;
		bool noise;

		union
		{
			WaveformType shape;
			NoiseType noiseType;
		};

		uint64_t framesRemaining;
		uint64_t totalFrames;
		float peak, sustain, balance, level, falloff;
		uint32_t rng;

		union
		{
			float pink[7];
			float brown;
		};

		union
		{
			float priorPink;
			float priorWhite;
		};
	};

	// Linear five-point S-A-B-C-E envelope, evaluated against this voice's own lifetime.
	static float Envelope(const SVoice &voice)
	{
		if (voice.command) return voice.level;

		if (voice.totalFrames <= 1)
			return 0.0f;

		double t = double(voice.totalFrames - voice.framesRemaining) / double(voice.totalFrames - 1);
		double a = voice.peak;
		double remaining = 1.0 - a;
		double length = remaining * voice.sustain;
		double b = a + (remaining - length) * voice.balance;
		double c = b + length;

		auto lerp = [](double x, double x0, double x1, double y0, double y1) -> float
		{
			return float(y0 + (y1 - y0) * ((x - x0) / (x1 - x0)));
		};

		if (t < a)
			return lerp(t, 0.0, a, 0.0, 1.0);

		if (t < b)
			return lerp(t, a, b, 1.0, voice.level);

		if (t < c)
			return lerp(t, b, c, voice.level, voice.level * (1.0f - voice.falloff));

		if (t < 1.0)
			return lerp(t, c, 1.0, voice.level * (1.0f - voice.falloff), 0.0);

		return 0.0f;
	}

	static float NextWhite(SVoice &voice)
	{
		// Xorshift32: independent per-voice state, producing [-1, 1).
		uint32_t x = voice.rng;
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		voice.rng = x;

		return float(x) * UINT32_TO_BIPOLAR - 1.0f;
	}

	static float NextNoise(SVoice &voice)
	{
		float white = NextWhite(voice);

		switch (voice.noiseType)
		{
			case NoiseType::White:
				return white;

			case NoiseType::Brown:
			{
				// Damped random walk, scaled to avoid excessive low-frequency level.
				voice.brown = (voice.brown + BROWN_NOISE_STEP * white) * BROWN_NOISE_DAMPING;
				return voice.brown * BROWN_NOISE_OUTPUT_GAIN;
			}

			case NoiseType::Violet:
			{
				float value = NOISE_DIFFERENCE_GAIN * (white - voice.priorWhite);
				voice.priorWhite = white;
				return value;
			}

			case NoiseType::Pink:
			case NoiseType::Blue:
			{
				// Paul Kellet's seven-stage pink approximation (tuned for AUDIO_SAMPLE_RATE).
				float pinksum = 0.0f;
				for (int i = 0; i < 6; i++)
				{
					voice.pink[i] = PINK_FILTER_FEEDBACK[i] * voice.pink[i] + PINK_FILTER_INPUT[i] * white;
					pinksum += voice.pink[i];
				}

				float pink = (pinksum +	voice.pink[6] + white * PINK_NOISE_DIRECT_GAIN) * PINK_NOISE_OUTPUT_GAIN;
				voice.pink[6] = white * PINK_NOISE_LAST_GAIN;

				if (voice.noiseType == NoiseType::Blue)
				{
					// Differencing pink noise adds ~6 dB/oct, yielding blue (~+3 dB/oct).
					const float blue = (pink - voice.priorPink) * BLUE_NOISE_OUTPUT_GAIN;
					voice.priorPink = pink;
					return blue;
				}

				return pink;
			}
		}

		return 0.0f;
	}

	static float NextTone(SVoice &voice, double pitchSemitones)
	{
		constexpr double twoPi = TWO_PI;
		const double cycle = voice.phase / twoPi;
		float sample = 0.0f;

		switch (voice.shape)
		{
			case WaveformType::Sine:
				sample = float(std::sin(voice.phase));
				break;

			case WaveformType::Square:
				sample = cycle < 0.5 ? 1.0f : -1.0f;
				break;

			case WaveformType::Triangle:
				sample = float(1.0 - 4.0 * std::abs(cycle - 0.5));
				break;

			case WaveformType::Sawtooth:
				sample = float(2.0 * cycle - 1.0);
				break;
		}

		double adjustedFrequency = voice.frequency * std::exp2(pitchSemitones / SEMITONES_PER_OCTAVE);

		voice.phase += twoPi * adjustedFrequency / double(AUDIO_SAMPLE_RATE);
		if (voice.phase >= twoPi || voice.phase < 0.0)
			voice.phase = std::fmod(voice.phase, twoPi);

		return sample;
	}

	// Per-track and master buses own filter memories and delay lines. Allocation happens
	// in ExecuteCommand, never while miniaudio is calling AudioCallback.
	struct SDSPStage
	{
		struct SComb
		{
			std::vector<std::array<float, 2>> buffer;
			size_t position = 0;
			float damping[2] = {};
		};

		float dry[2] = {};
		float lowPass[2] = {};
		float highPass[2] = {};
		std::vector<std::array<float, 2>> delay;
		size_t delayPosition = 0;
		std::array<SComb, REVERB_COMB_COUNT> reverb;
	};

	static void PrepareStage(SDSPStage &stage, Effect effect)
	{
		// Fixed delay time and reverb character for now; CPNT automates wet mix.
		if (effect == Effect::Delay && stage.delay.empty())
			stage.delay.resize(size_t(DELAY_TIME_SECONDS * AUDIO_SAMPLE_RATE)); // Fixed delay length.

		if (effect == Effect::Reverb && stage.reverb[0].buffer.empty())
		{
			const auto &lengths = REVERB_COMB_LENGTHS;
			for (size_t i = 0; i < REVERB_COMB_COUNT; ++i)
				stage.reverb[i].buffer.resize(lengths[i]);
		}
	}

	static float ProcessFilter(float input, float cutoff, float &memory, bool highPass)
	{
		// One pole, with bypass at the neutral cutoff values.
		cutoff = std::clamp(cutoff, 0.0f, MAX_FILTER_CUTOFF_HZ);

		if ((!highPass && cutoff >= DEFAULT_LOWPASS_CUTOFF_HZ) || (highPass && cutoff <= 0.0f))
		{
			memory = input; // Don't retain a stale filter history during bypass.
			return input;
		}

		float alpha = 1.0f - std::exp(-float(TWO_PI) * cutoff / float(AUDIO_SAMPLE_RATE));
		memory += alpha * (input - memory);

		return highPass ? input - memory : memory;
	}

	static void ProcessStage(SDSPStage &stage, const std::array<SEffectInfo, Effect::NumEffects> *effects,
							 double time, float &left, float &right)
	{
		auto effect = [effects, time](Effect id, float fallback) -> float
		{
			return effects ? Evaluate((*effects)[id], time, fallback) : fallback;
		};

		float sample[2] = { left, right };

		float low = effect(Effect::LowPass, DEFAULT_LOWPASS_CUTOFF_HZ);
		float high = effect(Effect::HighPass, DEFAULT_HIGHPASS_CUTOFF_HZ);

		for (int c = 0; c < 2; c++)
		{
			sample[c] = ProcessFilter(sample[c], low, stage.lowPass[c], false);
			sample[c] = ProcessFilter(sample[c], high, stage.highPass[c], true);
		}

		if (!stage.delay.empty())
		{
			float wet = std::clamp(effect(Effect::Delay, 0.0f), WET_MIN, WET_MAX);

			auto &slot = stage.delay[stage.delayPosition];

			for (int c = 0; c < 2; c++)
			{
				float delayed = slot[c];
				slot[c] = sample[c] + delayed * DELAY_FEEDBACK;
				sample[c] = sample[c] * (1.0f - wet) + delayed * wet;
			}

			stage.delayPosition++;
			if (stage.delayPosition == stage.delay.size())
				stage.delayPosition = 0;
		}

		if (!stage.reverb[0].buffer.empty())
		{
			float wet = std::clamp(effect(Effect::Reverb, 0.0f), WET_MIN, WET_MAX);
			float reverberated[2] = {};

			for (auto &comb : stage.reverb)
			{
				auto &slot = comb.buffer[comb.position];

				for (int c = 0; c < 2; c++)
				{
					float delayed = slot[c];
					comb.damping[c] = REVERB_DAMPING_INPUT * delayed + REVERB_DAMPING_HISTORY * comb.damping[c];
					slot[c] = sample[c] + REVERB_FEEDBACK * comb.damping[c];
					reverberated[c] += delayed * REVERB_COMB_OUTPUT_LEVEL;
				}

				comb.position++;
				if (comb.position == comb.buffer.size())
					comb.position = 0;
			}

			for (int c = 0; c < 2; c++)
				sample[c] = sample[c] * (1.0f - wet) + reverberated[c] * wet;
		}

		left = sample[0];
		right = sample[1];
	}

	static void AudioCallback(ma_device *device, void *output, const void *, ma_uint32 frames)
	{
		RenderFrames(static_cast<CAudioDevice *>(device->pUserData), static_cast<float *>(output), frames);
	}

	static void RenderFrames(CAudioDevice *self, float *samples, ma_uint32 frames)
	{
        std::lock_guard<std::mutex> lock(self->m_VoiceLock);
        if (self->m_IsAudition && !self->m_AuditionReady)
        {
            std::memset(samples, 0, size_t(frames) * AUDIO_OUTPUT_CHANNELS * sizeof(float));
            return;
        }

		if (self->m_Paused.load(std::memory_order_relaxed))
		{
			std::memset(samples, 0, size_t(frames) * AUDIO_OUTPUT_CHANNELS * sizeof(float));
			self->m_MeterL = self->m_MeterR = 0.0f;

			return;
		}

		constexpr double samplePeriod = 1.0 / double(AUDIO_SAMPLE_RATE);
		constexpr double twoPi = TWO_PI;

		float peakL = 0.0f, peakR = 0.0f;

		for (ma_uint32 i = 0; i < frames; i++)
		{
			double t = self->m_MixTime;

			auto &master = self->m_GlobalEffectControls;
			double globalPitch = Evaluate(master[PitchBend], t, 0.0f);
			double globalVibrato = Evaluate(master[Vibrato], t, 0.0f);

			float left = 0.0f, right = 0.0f;

			for (auto &bus : self->m_TrackDSP)
				bus.second.dry[0] = bus.second.dry[1] = 0.0f;

			for (auto &voice : self->m_Voices)
			{
				if (!voice.framesRemaining)
					continue;

				auto effects = self->m_TrackEffectControls.find(voice.track);
				auto *track = effects == self->m_TrackEffectControls.end() ? nullptr : &effects->second;

				auto effect = [track, t](Effect index, float defaultValue)
				{
					return track ? Evaluate((*track)[index], t, defaultValue) : defaultValue;
				};

				double bend = globalPitch + effect(PitchBend, 0.0f);
				double vibratoDepth = globalVibrato + effect(Vibrato, 0.0f);
				double vibrato = vibratoDepth * std::sin(voice.vibratoPhase);

				voice.vibratoPhase += twoPi * MODULATION_RATE_HZ * samplePeriod; // 5 Hz LFO; rate automation later.
				if (voice.vibratoPhase >= twoPi)
					voice.vibratoPhase -= twoPi;

				float value = (voice.noise ? NextNoise(voice) : NextTone(voice, bend + vibrato)) * Envelope(voice);

				props::IProperty *penabled = voice.track->GetProperties()->GetPropertyById('ENAB');
				bool enabled = !penabled || penabled->AsBool() || self->m_IsAudition;
				if (enabled)
				{
					const float gainDb = effect(Gain, 0.0f);
					const float volume = effect(Volume, 1.0f);
					const float tremoloDepth = std::clamp(effect(Tremolo, 0.0f), 0.0f, 1.0f);
					const float tremolo = 1.0f - tremoloDepth * (0.5f - 0.5f * float(std::cos(voice.vibratoPhase)));
					const float amp = voice.gain * value * volume * std::pow(10.0f, gainDb / DECIBELS_PER_AMPLITUDE_DECADE) * tremolo;
					const float pan = std::clamp(effect(Pan, 0.0f), PAN_MIN, PAN_MAX);

					// Linear balance: unity in each channel at center, no level compensation.
					// The owning track's bus processes the combined voices, not each voice independently.
					auto &bus = self->m_TrackDSP.find(voice.track)->second;
					bus.dry[0] += amp * (pan > 0.0f ? 1.0f - pan : 1.0f);
					bus.dry[1] += amp * (pan < 0.0f ? 1.0f + pan : 1.0f);
				}

				if (!self->m_IsAudition)
					voice.framesRemaining--;
			}

			for (auto &voice : self->m_SampleVoices)
			{
				if (voice.playedFrames >= voice.outputFrames)
					continue;

				auto found = self->m_TrackEffectControls.find(voice.track);
				auto *track = found == self->m_TrackEffectControls.end() ? nullptr : &found->second;

				auto effect = [track, t](Effect index, float fallback) -> float
				{
					return track ? Evaluate((*track)[index], t, fallback) : fallback;
				};

				double bend = globalPitch + effect(PitchBend, 0.0f);
				double vibratoDepth = globalVibrato + effect(Vibrato, 0.0f);
				double vibrato = vibratoDepth * std::sin(voice.vibratoPhase);

				voice.vibratoPhase += TWO_PI * MODULATION_RATE_HZ * samplePeriod;
				if (voice.vibratoPhase >= TWO_PI)
					voice.vibratoPhase -= TWO_PI;

				float sampleL = 0.0f, sampleR = 0.0f;

				NextSample(voice, bend + vibrato, sampleL, sampleR);

				props::IProperty *penabled = voice.track->GetProperties()->GetPropertyById('ENAB');
				bool enabled = !penabled || penabled->AsBool() || self->m_IsAudition;
				if (enabled)
				{
					float gainDb = effect(Gain, 0.0f);
					float volume = effect(Volume, 1.0f);
					float depth = std::clamp(effect(Tremolo, 0.0f), 0.0f, 1.0f);
					float tremolo = 1.0f - depth * (0.5f - 0.5f * float(std::cos(voice.vibratoPhase)));
					float amplitude = SAMPLE_VOICE_AMPLITUDE * volume * std::pow(10.0f, gainDb / DECIBELS_PER_AMPLITUDE_DECADE) * tremolo;
					float pan = std::clamp(effect(Pan, 0.0f), PAN_MIN, PAN_MAX);

					auto &bus = self->m_TrackDSP.find(voice.track)->second;
					bus.dry[0] += sampleL * amplitude * (pan > 0.0f ? 1.0f - pan : 1.0f);
					bus.dry[1] += sampleR * amplitude * (pan < 0.0f ? 1.0f + pan : 1.0f);
				}
			}

			// Process every bus, even after its last voice ends, so echoes decay naturally.
			for (auto &entry : self->m_TrackDSP)
			{
				auto *track = entry.first;
				auto &bus = entry.second;
				auto found = self->m_TrackEffectControls.find(track);
				auto *effects = found == self->m_TrackEffectControls.end() ? nullptr : &found->second;

				float busL = bus.dry[0], busR = bus.dry[1];
				ProcessStage(bus, effects, t, busL, busR);

				const props::IProperty *enabled = track->GetProperties()->GetPropertyById('ENAB');
				if (!enabled || enabled->AsBool())
				{
					left += busL;
					right += busR;
				}
			}

			ProcessStage(self->m_GlobalDSP, &master, t, left, right);

			float volume = Evaluate(master[Volume], t, 1.0f);
			float gainDb = Evaluate(master[Gain], t, 0.0f);
			float tremoloDepth = std::clamp(Evaluate(master[Tremolo], t, 0.0f), 0.0f, 1.0f);
			float tremolo = 1.0f - tremoloDepth * (0.5f - 0.5f * float(std::cos(twoPi * MODULATION_RATE_HZ * t)));
			float amplitude = volume * std::pow(10.0f, gainDb / DECIBELS_PER_AMPLITUDE_DECADE) * tremolo;
			float pan = std::clamp(Evaluate(master[Pan], t, 0.0f), PAN_MIN, PAN_MAX);

			samples[i * 2] = left * amplitude * (pan > 0.0f ? 1.0f - pan : 1.0f);
			samples[i * 2 + 1] = right * amplitude * (pan < 0.0f ? 1.0f + pan : 1.0f);

			// Absolute peak of the actual stereo output, including all master effects.
			peakL = std::max<float>(peakL, std::abs(samples[i * 2]));
			peakR = std::max<float>(peakR, std::abs(samples[i * 2 + 1]));

			if (self->m_HasTimeline && !self->m_IsAudition)
				self->m_MixTime += samplePeriod;
		}

		// Publish once per callback buffer; no artificial attack/release or clipping.
		self->m_MeterL = peakL;
		self->m_MeterR = peakR;

		self->m_SampleVoices.erase(std::remove_if(self->m_SampleVoices.begin(), self->m_SampleVoices.end(),
												  [](const SSampleVoice &v) { return v.playedFrames >= v.outputFrames; }), self->m_SampleVoices.end());

		self->m_Voices.erase(std::remove_if(self->m_Voices.begin(), self->m_Voices.end(),
											[](const SVoice &v) { return v.framesRemaining == 0; }), self->m_Voices.end());
	}


	// Write ordinary stereo 16-bit PCM WAV. File I/O never runs on the audio callback thread.
	static void WriteLE16(std::ostream &out, uint16_t value)
	{
		const char bytes[2] = {char(value), char(value >> 8)};
		out.write(bytes, 2);
	}

	static void WriteLE32(std::ostream &out, uint32_t value)
	{
		const char bytes[4] = {char(value), char(value >> 8), char(value >> 16), char(value >> 24)};
		out.write(bytes, 4);
	}

	nucleus::RetCode ExportToWav(const TCHAR *input, const TCHAR *output)
	{
		if (!m_pSys)
			return nucleus::RetCode::UNKNOWN_ERROR;

		// Never overwrite the source show, even if IN and OUT were accidentally identical.
		std::error_code pathError;

		const auto sourcePath = std::filesystem::absolute(std::filesystem::path(input), pathError);
		if (pathError)
			return nucleus::RetCode::UNKNOWN_ERROR;

		const auto targetPath = std::filesystem::absolute(std::filesystem::path(output), pathError);
		if (pathError || sourcePath.lexically_normal() == targetPath.lexically_normal())
			return nucleus::RetCode::UNKNOWN_ERROR;

		// Destroy the TrackSet before the isolated renderer, whose voices refer to its tracks.
		CAudioDevice renderer;
		renderer.m_CommandIdentity = this;
		renderer.m_pSys = m_pSys;

		struct TrackSetOwner
		{
			nucleus::ITrackSet *value;
			~TrackSetOwner()
			{
				if (value)
					value->Release();
			}
		} trackset{nucleus::ITrackSet::Create(m_pSys)};

		if (!trackset.value || trackset.value->Load(input) != nucleus::RetCode::OK)
			return nucleus::RetCode::UNKNOWN_ERROR;

		struct TimedCommand
		{
			const nucleus::ICommand *command;
			double time;
		};

		std::vector<TimedCommand> events;

		for (size_t i = 0; i < trackset.value->GetNumTracks(); i++)
		{
			const nucleus::ITrack *track = trackset.value->GetTrack(i);
			const props::IProperty *enabled = track->GetProperties()->GetPropertyById('ENAB');
			if (enabled && !enabled->AsBool())
				continue;

			for (size_t j = 0; j < track->GetNumCommands(); j++)
			{
				double time = 0.0;
				const nucleus::ICommand *cmd = track->GetCommand(j, &time);
				if (cmd && cmd->GetCommandInfo() && std::isfinite(time) && time >= 0.0 &&
					cmd->GetCommandInfo()->GetDevice() == this &&
					cmd->GetCommandInfo() != COMMAND_INSTPTR_NAME(Export))
					events.push_back({cmd, time});
			}
		}

		std::stable_sort(events.begin(), events.end(), [](const TimedCommand &a, const TimedCommand &b)
		{
			return a.time < b.time;
		});

		const double length = trackset.value->Length();
		if (!std::isfinite(length) || length < 0.0)
			return nucleus::RetCode::UNKNOWN_ERROR;

		// Two seconds of silence at the end permit delay/reverb tails to decay.
		double duration = length + 2.0;
		uint64_t totalFrames = uint64_t(std::ceil(duration * double(AUDIO_SAMPLE_RATE)));
		uint64_t bytes = totalFrames * AUDIO_OUTPUT_CHANNELS * sizeof(int16_t);

		if (duration <= 0.0 || duration > 14400.0 || bytes > UINT32_MAX - 36u)
			return nucleus::RetCode::UNKNOWN_ERROR;

		std::ofstream wav(std::filesystem::path(output), std::ios::binary | std::ios::trunc);
		if (!wav)
			return nucleus::RetCode::UNKNOWN_ERROR;

		wav.write("RIFF", 4);
		WriteLE32(wav, uint32_t(36 + bytes));
		wav.write("WAVEfmt ", 8);
		WriteLE32(wav, 16);                     // PCM format chunk size
		WriteLE16(wav, 1);                      // integer PCM
		WriteLE16(wav, AUDIO_OUTPUT_CHANNELS);
		WriteLE32(wav, AUDIO_SAMPLE_RATE);
		WriteLE32(wav, AUDIO_SAMPLE_RATE * AUDIO_OUTPUT_CHANNELS * sizeof(int16_t));
		WriteLE16(wav, AUDIO_OUTPUT_CHANNELS * sizeof(int16_t));
		WriteLE16(wav, 16);
		wav.write("data", 4);
		WriteLE32(wav, uint32_t(bytes));

		constexpr ma_uint32 BLOCK = 1024;
		std::array<float, BLOCK * AUDIO_OUTPUT_CHANNELS> mix{};
		std::array<int16_t, BLOCK * AUDIO_OUTPUT_CHANNELS> pcm{};
		renderer.m_MixTime = 0.0;
		renderer.m_HasTimeline = true;

		uint64_t frame = 0;
		size_t next = 0;
		while (frame < totalFrames && wav)
		{
			// Dispatch each command at its exact output sample; handle simultaneous events together.
			while (next < events.size() &&
				uint64_t(std::llround(events[next].time * double(AUDIO_SAMPLE_RATE))) <= frame)
			{
				auto &event = events[next++];
				if (renderer.ExecuteCommand(event.command, event.time) != nucleus::RetCode::OK)
					return nucleus::RetCode::UNKNOWN_ERROR;
			}

			uint64_t limit = std::min<uint64_t>(BLOCK, totalFrames - frame);
			if (next < events.size())
			{
				uint64_t target = uint64_t(std::llround(events[next].time * double(AUDIO_SAMPLE_RATE)));
				if (target > frame)
					limit = std::min<uint64_t>(limit, target - frame);
			}

			if (!limit)
				continue;

			RenderFrames(&renderer, mix.data(), ma_uint32(limit));

			for (size_t i = 0; i < limit * AUDIO_OUTPUT_CHANNELS; i++)
			{
				const float value = std::clamp(mix[i], -1.0f, 1.0f);
				pcm[i] = int16_t(std::lrint(value * (value < 0.0f ? 32768.0f : 32767.0f)));
			}

			// PCM is little-endian on Windows, the supported target for this plug-in.
			wav.write(reinterpret_cast<const char *>(pcm.data()), std::streamsize(limit * AUDIO_OUTPUT_CHANNELS * sizeof(int16_t)));
			frame += limit;
		}

		wav.flush();
		return wav ? nucleus::RetCode::OK : nucleus::RetCode::UNKNOWN_ERROR;
	}

    // Called by the command path while this device's render lock is held.
    // Audio callbacks read copied voice values, never the changing command properties.
    void RefreshAudition()
    {
        for (auto &voice : m_Voices)
            if (auto *p = voice.command->GetParams()->GetPropertyById('FREQ'))
				voice.frequency = p->AsFloat();

		for (auto &voice : m_SampleVoices)
            if (auto *p = voice.command->GetParams()->GetPropertyById('PMUL'))
                voice.pitchMultiplier = std::clamp(p->AsFloat(), SAMPLE_MIN_PITCH_MULTIPLIER, SAMPLE_MAX_PITCH_MULTIPLIER);
    }


    void StartAudition(double time)
    {
        // Repeated begins replace the preview, never overwrite the saved state.
        StopAudition();
        m_IsAudition = true;
        m_AuditionReady = false;
        m_MixTime = time;
        m_HasTimeline = true;
    }

	void StopAudition()
	{
		if (!m_IsAudition)
			return;

		m_IsAudition = false;
		m_AuditionReady = false;

		m_Voices.clear();
		m_SampleVoices.clear();

		m_TrackEffectControls.clear();
		m_TrackDSP.clear();
		m_GlobalEffectControls = {};
		m_GlobalDSP = {};

		m_MixTime = 0.0;
		m_HasTimeline = false;
	}

	bool m_IsAudition = false;
    bool m_AuditionReady = false;
	ma_device m_Device{};
	bool m_DeviceReady;
	const CAudioDevice *m_CommandIdentity = nullptr;
	std::mutex m_VoiceLock;
	std::vector<SVoice> m_Voices;
	std::vector<SSampleVoice> m_SampleVoices;
	std::map<std::filesystem::path, std::shared_ptr<const SCachedSample>> m_SampleCache;

public:
	nucleus::INucleus *m_pSys;
	props::IPropertySet *m_pProps;

	std::array<SEffectInfo, Effect::NumEffects> m_GlobalEffectControls{};
	std::map<const nucleus::ITrack *, std::array<SEffectInfo, Effect::NumEffects>> m_TrackEffectControls;
	std::map<const nucleus::ITrack *, SDSPStage> m_TrackDSP;
	SDSPStage m_GlobalDSP;
	double m_MixTime = 0.0;
	bool m_HasTimeline = false;

protected:
	float m_MeterL, m_MeterR;
	std::atomic<bool> m_Paused = false;

} AudioDevice;

NUPLUGIN_ONACTIVATE_BEGIN()
psys->DeviceManager()->RegisterDevice((nucleus::IDevice *)&AudioDevice);

return true;
NUPLUGIN_ONACTIVATE_END()

NUPLUGIN_ONDEACTIVATE_BEGIN()
psys->DeviceManager()->UnregisterDevice((nucleus::IDevice *)&AudioDevice);

return true;
NUPLUGIN_ONDEACTIVATE_END()
