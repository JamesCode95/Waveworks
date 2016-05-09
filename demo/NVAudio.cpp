#include "DXUT.h"

#ifdef ENABLE_FMOD
#include <fmod.hpp>
#include <fmod_errors.h>
#endif

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "SFX_TABLE.h"
#include "NVAudio.h"

#define MAX_SOUNDEFFECTS 100

#pragma warning (disable: 4996 4505)

BOOL NVAudio;

#ifdef ENABLE_FMOD

// FMOD vars
FMOD_RESULT result;
FMOD::System *AUDIOsystem;

// SOUNDS
FMOD::Sound *MusicTrack;

// channels / voices
FMOD::Channel *MusicChannel;  // pull one aside for MUSIC (background track)
FMOD::Channel *AudioChannels[NVAUDIO_MAX_CHANNELS];
float MasterVolumes[NVAUDIO_MAX_CHANNELS];

typedef struct {
	BOOL loaded;
	FMOD::Sound *Wave;
	char Filename[100];
	} SoundStruct;

SoundStruct SoundEffects[ MAX_SOUNDEFFECTS ];

#endif // ENABLE_FMOD

/*
typedef struct {
	FMOD::Channel  Channel;
} ChannelStruct;

ChannelStruct AudioChannels[ NVAUDIO_MAX_CHANNELS ];
*/


// --------------------------------------------------------------------
// LOADS a sample into wave format and processes it
//		- Filename ( the filename )
//      - EffectNumber ( the reference number of the sound effect ), Build an enum table and refer to htat
//      - CompressionFlag ( Force to remain in native format if set to NVAUDIO_REMAINCOMPRESSED (ideal for mp3/ogg) ) 
// --------------------------------------------------------------------


void NVAudioLoadSoundEffect( char*filename, int EffectNumber , int CompressionFlag, bool loop, float frequencyVar, float volumeVar )
{
	assert(CompressionFlag==NVAUDIO_REMAINCOMPRESSED || CompressionFlag==NVAUDIO_UNCOMPRESSED);
	OutputDebugStringA( "\r\nTrying to Load ->" );
	OutputDebugStringA( filename );

#ifdef ENABLE_FMOD
	if (NVAudio && SoundEffects[ EffectNumber ].loaded!=TRUE )
	{
		int cmpr = FMOD_DEFAULT;

		if (CompressionFlag==NVAUDIO_REMAINCOMPRESSED) 
			cmpr = FMOD_CREATECOMPRESSEDSAMPLE;

		if (loop)
			cmpr |= FMOD_LOOP_NORMAL;

		result = AUDIOsystem->createSound(filename, cmpr , 0, &SoundEffects[ EffectNumber ].Wave);		// FMOD_DEFAULT uses the defaults.  These are the same as FMOD_LOOP_OFF | FMOD_2D | FMOD_HARDWARE.
		
		if(result==FMOD_OK)
		{
			if(CompressionFlag==NVAUDIO_REMAINCOMPRESSED)
			{
				OutputDebugStringA("->REMAINING COMPRESSED->LOADED OK");
			}
			else
			{
				OutputDebugStringA("->DECODED->LOADED\r\n");
			}
			SoundEffects[ EffectNumber ].loaded = TRUE;
			result = SoundEffects[ EffectNumber ].Wave->setVariations(frequencyVar, volumeVar, 0);
			assert(result == FMOD_OK);
		}
		else
		{
			OutputDebugStringA("->FAILED\r\n");
			SoundEffects[ EffectNumber ].loaded = FALSE;
		}
	}
#endif // ENABLE_FMOD
}

// ----------------------------
// quick error decode function
// ----------------------------
#ifdef ENABLE_FMOD
void NVAudioDecodeErrorAndOutput( FMOD_RESULT r )
{
	char DbugTxt[200];
	
	if (NVAudio)
	{
		sprintf(&DbugTxt[0],"\r\nAUDIO %d errorcode \r\n", r);
		OutputDebugStringA(DbugTxt);
	}
}
#endif // ENABLE_FMOD

// non public audio Test Function.

void NVAUDIOLoadMusicTrack()
{
#ifdef ENABLE_FMOD
	if(NVAudio)
	{
		OutputDebugStringA("\r\nLoading background track\r\n");

		const char* trackName = "Media\\Audio\\BoatInTheSeaSoundsLooping.wav";
		result = AUDIOsystem->createSound(trackName, FMOD_DEFAULT | FMOD_LOOP_NORMAL, 0, &MusicTrack);		// FMOD_DEFAULT uses the defaults.  These are the same as FMOD_LOOP_OFF | FMOD_2D | FMOD_HARDWARE.
		
		if(result != FMOD_OK)
			NVAudioDecodeErrorAndOutput( result );
		
	OutputDebugStringA("\r\nLoaded background Track\r\n");

	}
#endif // ENABLE_FMOD
}

void NVAUDIOPlayMusicTrack( void )
{
#ifdef ENABLE_FMOD
	if(MusicTrack)
	{
		OutputDebugStringA("\r\n\r\nStarting Backround Sync track");
	
		result = AUDIOsystem->playSound(FMOD_CHANNEL_REUSE , MusicTrack, false, &AudioChannels[ MUSIC_VOICE ] );
		
		OutputDebugStringA("\r\nBackground track syncronised and started\r\n");
	}
#endif // ENABLE_FMOD
}


void NVAudioPlaySound( int effect, int channel )
{
	assert(channel < NVAUDIO_MAX_CHANNELS);
	char string[100];

	sprintf(string,"\r\n%d Effect Number, %d channel\r\n", effect, channel );

	OutputDebugStringA( string );
	
#ifdef ENABLE_FMOD
	bool bIsPlaying;
	if(NVAudio)
	{
		AudioChannels[ channel ]->isPlaying( &bIsPlaying );

		if(bIsPlaying)
		{	
			OutputDebugStringA( "It is Playing" );
			AudioChannels[ channel ]->stop();
		}
		result = AUDIOsystem->playSound( FMOD_CHANNEL_FREE , SoundEffects[effect].Wave , false, &AudioChannels[ channel ] );
	}
#endif // ENABLE_FMOD
}

void NVAudioSetVolume(int channel, float v)
{
#ifdef ENABLE_FMOD
	assert(channel < NVAUDIO_MAX_CHANNELS);
	AudioChannels[ channel ]->setVolume(MasterVolumes[channel] * v);
#endif // ENABLE_FMOD
}

float NVAudioGetVolume(int channel)
{
	assert(channel < NVAUDIO_MAX_CHANNELS);
	float result=0;
#ifdef ENABLE_FMOD
	AudioChannels[ channel ]->getVolume(&result);
#endif // ENABLE_FMOD
	return result;
}

void NVAudioSetFrequency(int channel, float f)
{
	assert(channel < NVAUDIO_MAX_CHANNELS);
#ifdef ENABLE_FMOD
	AudioChannels[ channel ]->setFrequency(f);
#endif // ENABLE_FMOD
}

float NVAudioGetFrequency(int channel)
{
	assert(channel < NVAUDIO_MAX_CHANNELS);
	float result=0;
#ifdef ENABLE_FMOD
	AudioChannels[ channel ]->getFrequency(&result);
#endif // ENABLE_FMOD
	return result;
}

void NVAudioSetChannelMasterVolume(int channel, float v)
{
}

void NVAudioStopSound( int channel )
{
#ifdef ENABLE_FMOD
	assert(channel < NVAUDIO_MAX_CHANNELS);
	bool bIsPlaying;

	AudioChannels[ channel ]->isPlaying( &bIsPlaying );

	if(bIsPlaying)
	{	
		OutputDebugStringA( "It is Playing" );
		AudioChannels[ channel ]->stop();
	}
	else
	{
		OutputDebugStringA( "NOT PLAYING" );
	}
#endif // ENABLE_FMOD
}

void NVAudioPauseSound( int channel )
{
	assert(channel < NVAUDIO_MAX_CHANNELS);

#ifdef ENABLE_FMOD
	AudioChannels[ channel ]->setPaused(true);
#endif // ENABLE_FMOD
}

void NVAudioResumeSound( int channel )
{
	assert(channel < NVAUDIO_MAX_CHANNELS);

#ifdef ENABLE_FMOD
	AudioChannels[ channel ]->setPaused(false);
#endif // ENABLE_FMOD
}

BOOL NVAudioIsPlaying( int channel )
{
#ifdef ENABLE_FMOD
	assert(channel < NVAUDIO_MAX_CHANNELS);
	bool bIsPlaying;

	AudioChannels[ channel ]->isPlaying( &bIsPlaying );

	if(bIsPlaying)
	{	
		return TRUE;
	}
#endif // ENABLE_FMOD

	return FALSE;
}	



// ---------------------------------------------
// Call this to initialise the engine.
// ---------------------------------------------

BOOL NVAudioInit(float defaultMasterVolume)
{

	NVAudio = FALSE;

#ifdef ENABLE_FMOD
	result = FMOD::System_Create(&AUDIOsystem);		// Create the main system object.
	if (result != FMOD_OK)
	{
		OutputDebugStringA("Failed to create Audio System");	
		return FALSE;
	}

	result = AUDIOsystem->init(100, FMOD_INIT_NORMAL, 0);	// Initialize FMOD.
	
	if (result != FMOD_OK)
	{
		OutputDebugStringA("Failed to initialise Audio Channels");
		return FALSE;
	}

	for (int c=0; c!=NVAUDIO_MAX_CHANNELS; ++c)
		MasterVolumes[c] = defaultMasterVolume;

	// This sets up all the voices so we can control where they get played.
	/*
	OutputDebugStringA("\r\nFinding Voices\r\n");
	int c;
	for(c =0;c<100;c++)
	{
		result = AUDIOsystem->getChannel( 0, &AudioChannels[ c ] );		
			
		if(result== FMOD_OK)
		{
			char dbg[100];		
			sprintf(dbg,"%d ",c);
			OutputDebugStringA(dbg);
		}
	}

	OutputDebugStringA("\r\nFinding Voices Completed\r\n");
	*/
#endif // ENABLE_FMOD

	NVAudio = TRUE;	
	return TRUE;
}

// ---------------------------------------------
// FUNCTION to load all the assets for the demo.
// ---------------------------------------------

void NVLoadAudioAssets()
{
#ifdef ENABLE_FMOD
	if(NVAudio)
	{
		OutputDebugStringA("\r\nSample Loading\r\n");	
		// load the music track seperately. (this does not launch t5
		NVAUDIOLoadMusicTrack();  

		// finished loading sound effects.
	}
#endif ENABLE_FMOD
}

// ---------------------------------------------
// Call this EACH FRAME exactly once.
// ---------------------------------------------

void NVAudioTick( void )
{
#ifdef ENABLE_FMOD
	if(NVAudio)
	{
		AUDIOsystem->update();
	}
#endif // ENABLE_FMOD
}

// ---------------------------------------------
// Call this BEFORE EXITING the app.
// ---------------------------------------------

void NVAudioTerminate( void )
{
	if(NVAudio)
	{
		OutputDebugStringA("\r\nAudio Closed Down\r\n");
#ifdef ENABLE_FMOD
		AUDIOsystem->release();
#endif // ENABLE_FMOD
		NVAudio = FALSE;
	}
}	