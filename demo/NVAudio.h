#pragma once
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "resource.h"
#include "SFX_TABLE.h"

#define NVAUDIO_UNCOMPRESSED 0
#define NVAUDIO_REMAINCOMPRESSED 1

#define MUSIC_VOICE 0
#define NVAUDIO_MAX_CHANNELS 200

BOOL NVAudioInit(float defaultMasterVolume=1.0f);
void NVAudioTick( void );
void NVAudioTerminate( void );
void NVLoadAudioAssets( void );


void NVAUDIOLoadMusicTrack( void );
void NVAUDIOPlayMusicTrack( void );

// Refer to SFX_TABLE.h for list of effects names.		
// Non-zero frequencyVar and volumeVar will give random variations.  They are simply 
// passed to the fmod API.  frequency is in Hz; volume is [0,1].
void NVAudioLoadSoundEffect( char* filename , int EffectNumber , int CompressionFlag, bool loop=false, float frequencyVar=0, float volumeVar=0 );

void NVAudioSetVolume(int channel, float v);
float NVAudioGetVolume(int channel);
void NVAudioSetFrequency(int channel, float v);
float NVAudioGetFrequency(int channel);

// This is a volume scale applied to the channel volume.  All values passed to SetVolume will get 
// multiplied by this.  The idea is to control balance between channels without having to mess 
// with their individual volume calculations.  The result of GetVolume does not reflect this value.
void NVAudioSetChannelMasterVolume(int channel, float v);

void NVAudioPlaySound( int effect, int channel );
void NVAudioPauseSound( int channel );
void NVAudioResumeSound( int channel );
void NVAudioStopSound( int channel );
BOOL NVAudioIsPlaying( int channel );

