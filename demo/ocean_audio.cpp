#include "DXUT.h"
#include <assert.h>
#include <limits>
#include <sstream>
#include "SFX_TABLE.h"
#include "NVAudio.h"

#undef min
#undef max

static const bool DISABLE_AUDIO = false;	// To test perf.
static float engineInitialFreq = 0;
static bool fmodIsPaused = false;

float randInRange(float a, float b) 
{ 
	return ((b-a)*((float)rand()/RAND_MAX))+a; 
}

static void AudioLog( const char* LogText )
{
	// simple debugger log string.
	OutputDebugStringA( LogText );
}

// To avoid continuous, constant volume drone on some loops, we randomize the volume.
// This seeks a random new volume within a range every 2-4s.
class VolumeRandomizer
{
public:
	VolumeRandomizer(float l, float u): m_lower(l), m_upper(u), m_volume(l), m_volumeSpeed(0), m_nextTransitionTime(std::numeric_limits<double>::min()), m_modulation(1) {}
	void SetVolume(double now, float elapsedTime, int channel);
	void Modulate(float m)	{ m_modulation = m; }
private:
	const float m_lower, m_upper;
	float m_volume;
	float m_volumeSpeed;
	float m_modulation;
	double m_nextTransitionTime;

	VolumeRandomizer& operator=(const VolumeRandomizer&);
};

void VolumeRandomizer::SetVolume(double now, float elapsedTime, int channel)
{
	m_volume += elapsedTime * m_volumeSpeed;
	NVAudioSetVolume(channel, m_modulation * m_volume);
	//std::ostringstream ostr;
	//ostr << "now=" << now << ", elapsed=" << elapsedTime << ", vol=" << m_volume << std::endl << std::ends;
	//AudioLog(ostr.str().c_str());

	if (now > m_nextTransitionTime)
	{
		const float timeDelta = randInRange(2.0f, 4.0f);
		const float nextVol   = randInRange(m_lower, m_upper);
		m_nextTransitionTime = now + timeDelta;

		m_volumeSpeed = (nextVol - m_volume) / timeDelta;

		//std::ostringstream ostr;
		//ostr << "now=" << now << ", next=" << m_nextTransitionTime << ", vol=" << m_volume << ", nextVol=" << nextVol << ", speed=" << m_volumeSpeed << std::endl << std::ends;
		//AudioLog(ostr.str().c_str());
	}
}

void InitOceanAudio()
{
	if (DISABLE_AUDIO)
		return;

	// To give lightning a big hit impact, we default all channels to less than full volume and
	// pump up only the lightning to max.
	const float defaultMasterVolume=0.7f;
	const BOOL Audio = NVAudioInit(defaultMasterVolume);
	NVAudioSetChannelMasterVolume(LIGHTNING1, 1);
	NVAudioSetChannelMasterVolume(LIGHTNING2, 1);
	NVAudioSetChannelMasterVolume(LIGHTNING3, 1);
	NVAudioSetChannelMasterVolume(LIGHTNING4, 1);
	NVAudioSetChannelMasterVolume(LIGHTNING5, 1);
	
	if( Audio == TRUE ) 
	{
		AudioLog("Audio Engine Started");		
	}
	else
	{
		AudioLog("Audio Engine failed to start");
	}

	// Track name is buried in this music fn.  Silly.
	NVAUDIOLoadMusicTrack();

	// Looping
	NVAudioLoadSoundEffect("Media/Audio/WindLightWhistling.wav",          WIND_LIGHT_WHISTLING, NVAUDIO_UNCOMPRESSED, true);
	NVAudioLoadSoundEffect("Media/Audio/WindHowling.wav",                 WIND_HOWLING,         NVAUDIO_UNCOMPRESSED, true);
	NVAudioLoadSoundEffect("Media/Audio/WindStrongGusty.wav",             WIND_STRONG_GUSTY,    NVAUDIO_UNCOMPRESSED, true);
	NVAudioLoadSoundEffect("Media/Audio/WavesLoop_LessHiss.wav",          WAVES_LOOP,           NVAUDIO_UNCOMPRESSED, true);
	NVAudioLoadSoundEffect("Media/Audio/Boat_engine_motor_idle_loop.wav", ENGINE_LOOP,          NVAUDIO_UNCOMPRESSED, true);	
	NVAudioLoadSoundEffect("Media/Audio/LongSurfLoop_28k.wav",            SURF_LOOP,            NVAUDIO_UNCOMPRESSED, true);

	// Non-looping lightning hits
	NVAudioLoadSoundEffect("Media/Audio/ThunderLightningStrike01.wav", LIGHTNING1, NVAUDIO_UNCOMPRESSED, false);
	NVAudioLoadSoundEffect("Media/Audio/ThunderLightningStrike02.wav", LIGHTNING2, NVAUDIO_UNCOMPRESSED, false);
	NVAudioLoadSoundEffect("Media/Audio/ThunderLightningStrike03.wav", LIGHTNING3, NVAUDIO_UNCOMPRESSED, false);
	NVAudioLoadSoundEffect("Media/Audio/06156FastHugeThunderbolt.wav", LIGHTNING4, NVAUDIO_UNCOMPRESSED, false);
	NVAudioLoadSoundEffect("Media/Audio/00225Thunder18StrikesMe.wav",  LIGHTNING5, NVAUDIO_UNCOMPRESSED, false);

	// Individual splashe hits.
	NVAudioLoadSoundEffect("Media/Audio/CrashSurf1_Deeper.wav",    SURF_HIT1,     NVAUDIO_UNCOMPRESSED, false);
	NVAudioLoadSoundEffect("Media/Audio/CrashSurf2_Deeper.wav",    SURF_HIT2,     NVAUDIO_UNCOMPRESSED, false);
	NVAudioLoadSoundEffect("Media/Audio/CrashSurf3_Deeper.wav",    SURF_HIT3,     NVAUDIO_UNCOMPRESSED, false);

	NVAUDIOPlayMusicTrack();   // plays the music track onto CHANNEL(0)

	// Set all loop volumes to zero when the app starts (which is before the window appears).  Then the 
	// wind/spray/randomization upates will fade the volume up when frame updates start.
	NVAudioSetVolume(MUSIC_VOICE, 0.0f);

	NVAudioPlaySound(WIND_LIGHT_WHISTLING, WIND_LIGHT_WHISTLING);
	NVAudioPlaySound(WIND_HOWLING,         WIND_HOWLING);
	NVAudioPlaySound(WIND_STRONG_GUSTY,    WIND_STRONG_GUSTY);	
	NVAudioPlaySound(WAVES_LOOP,           WAVES_LOOP);
	NVAudioPlaySound(ENGINE_LOOP,          ENGINE_LOOP);
	NVAudioPlaySound(SURF_LOOP,            SURF_LOOP);
	NVAudioSetVolume(WIND_LIGHT_WHISTLING, 0.0f);
	NVAudioSetVolume(WIND_HOWLING,         0.0f);
	NVAudioSetVolume(WIND_STRONG_GUSTY,    0.0f);
	NVAudioSetVolume(WAVES_LOOP,           0.0f);
	NVAudioSetVolume(ENGINE_LOOP,          0.0f);
	NVAudioSetVolume(SURF_LOOP,            0.0f);

	engineInitialFreq = NVAudioGetFrequency(ENGINE_LOOP);
}

const float MAX_WIND = 12;	// Beaufort

static float clampVolume(float v)
{
	return std::min(1.0f, std::max(0.0f, v));
}

static float fadeWind(float wind, float windSpeedZeroVolume, float windSpeedMaxVolume)
{
	// Just a lerp.
	return clampVolume((wind-windSpeedZeroVolume) / (windSpeedMaxVolume-windSpeedZeroVolume));
}

static float fadeSplash(float power, float powerZeroVolume, float powerMaxVolume)
{
	// Just a lerp.
	return clampVolume((power-powerZeroVolume) / (powerMaxVolume-powerZeroVolume));
}

static void PlayRandomSplashHit(float power)
{
	const int channel = SURF_HIT1;
	if (!NVAudioIsPlaying(channel))
	{
		int index = 0;
		static int last = -1;

		// Pick a random sound, but never the same one twice in a row.
		do 
			index = rand() % 3;
		while (index == last);
		last = index;

		const int effect  = SURF_HIT1 + index;
		const float vol = clampVolume((power-10.0f)/8.0f + randInRange(0.1f, 0.4f));
		NVAudioPlaySound(effect, channel);
		NVAudioSetVolume(channel, vol);

		//std::ostringstream ostr;
		//ostr << "Big Splash! power=" << power << ", effect=" << effect << ", vol=" << vol << std::endl << std::ends;
		//AudioLog(ostr.str().c_str());
	}
}

// Don't call this multiple times for multiple values.  It ought to be a class.
static float RollingAverage(float val)
{
	const int WINDOW_SIZE = 24;
	static float window[WINDOW_SIZE] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	static int index=0;

	window[index] = val;
	index = (index+1) % WINDOW_SIZE;

	float average=0;
	for (int i=0; i!=WINDOW_SIZE; ++i)
		average += window[i];
	average /= (float) WINDOW_SIZE;

	return average;
}

static void UpdateSplashVolume(float splashPower, float mute)
{
	// The splash power seems to vary wildly from frame-to-frame.  Using it to control
	// volume directly results in cut-offs that are far too abrupt.
	const float avgPower = RollingAverage(splashPower);
	const float splashVol = fadeSplash(avgPower, 10, 80);
	NVAudioSetVolume(SURF_LOOP, mute * splashVol);

	//std::ostringstream ostr;
	//ostr << "Splash power=" << splashPower << ", avgPower=" << avgPower << ", splashVol=" << splashVol << std::endl << std::ends;
	//AudioLog(ostr.str().c_str());
}

float GetOceanAudioSplashPowerThreshold()
{
	return 150.f;	// Empirically determined
}

void UpdateOceanAudio(double fTime, float fElapsedTime, float windSpeed, float splashPower, bool is_paused)
{
	if (DISABLE_AUDIO)
		return;

	if(fmodIsPaused != is_paused)
	{
		for(int channel = 0; channel != NUM_SFX; ++channel) {
			if(is_paused) NVAudioPauseSound(channel);
			else NVAudioResumeSound(channel);
		}
		fmodIsPaused = is_paused;
	}

	NVAudioTick();

	// Change these in the debugger to enable/disable.
	static float windMute = 1, bgMute = 1, waveLoopMute = 1, engineMute=1, splashMute=1;
	static VolumeRandomizer bgRandomizer(0.1f, 0.3f), engineRandomizer(0.5f, 0.7f), waveLoopRandomizer(0.4f, 0.8f);

	bgRandomizer.Modulate(bgMute);
	bgRandomizer.SetVolume(fTime, fElapsedTime, MUSIC_VOICE);

	const float waveLoopVol = fadeWind(windSpeed, 4, 10);
	waveLoopRandomizer.Modulate(waveLoopVol * waveLoopMute);
	waveLoopRandomizer.SetVolume(fTime, fElapsedTime, WAVES_LOOP);

	// Scale the engine frequency with wind speed.  This makes it both faster and higher pitch, making
	// it sound like it is labouring hard.  (We could pass a boat speed down but it appears to be const.)
	const float engineFreq = 1.0f + 0.25f * fadeWind(windSpeed, 4, 11);
	const float engineVol  = fadeWind(windSpeed, 1, 8);
	engineRandomizer.Modulate(engineVol * engineMute);
	engineRandomizer.SetVolume(fTime, fElapsedTime, ENGINE_LOOP);
	NVAudioSetFrequency(ENGINE_LOOP, engineInitialFreq * engineFreq);

	// All these wind speed are Beaufort scale.
	const float windVol1 = fadeWind(windSpeed, 4, 7) * fadeWind(windSpeed, 9, 7) * 0.5f;
	const float windVol2 = fadeWind(windSpeed, 5, 10);
	const float windVol3 = fadeWind(windSpeed, 8, 12);
	NVAudioSetVolume(WIND_LIGHT_WHISTLING, windMute * windVol1);
	NVAudioSetVolume(WIND_HOWLING,         windMute * windVol2);
	NVAudioSetVolume(WIND_STRONG_GUSTY,    windMute * windVol3);

	UpdateSplashVolume(splashPower, splashMute);

	// Splash sounds are quite compilcated - there are three effects. 1) WAVES_LOOP is a gentle ocean wave
	// sound that increases with wind speed. 2) SURF_LOOP is a continuous surf sound.  3) SURF_HITs are 
	// percusive hits with big attacks.  Playing only individual, non-repeating hit sounds doesn't work 
	// well because the sound duration needs to match the duration of the visual splash effect.  Instead, we 
	// modulate the volume of the continuous SURF_LOOP in UpdateSplashVolume and add a few hits for really big
	// power events.
	if (splashPower > GetOceanAudioSplashPowerThreshold())
		PlayRandomSplashHit(splashPower);
}

void PlayLightningSound()
{
	if (DISABLE_AUDIO)
		return;

	int index = 0;
	static int last = -1;

	// Pick a random sound, but never the same one twice in a row.
	do 
		index = rand() % 5;
	while (index == last);
	last = index;

	const int channel = LIGHTNING1 + index;
	NVAudioPlaySound(channel, channel);
	NVAudioSetVolume(channel, randInRange(0.9f, 1.0f));
}
