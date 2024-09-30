/****************************************************************************
GFC - Games Fundamental Classes version 2.70
Copyright (C) 2009-2022 Jarek Francik, Kingston University London

This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License:
http://creativecommons.org/licenses/by-sa/4.0/

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Created by Jarek Francik
jarek@kingston.ac.uk
****************************************************************************/
#include "pch.h"
#include <map>
#include "GFC.h"

int CSoundPlayer::c_frequency = 44100;
Uint16 CSoundPlayer::c_format = AUDIO_S16SYS;
int CSoundPlayer::c_channels = 2;
int CSoundPlayer::c_chunksize = 2048;
int CSoundPlayer::c_cnt = 0;

CFileMgr<Mix_Music> CSound::c_filemgr("%;%sounds\\;.\\;sounds\\",
										[](std::string filename) 
										{ 
											return Mix_LoadMUS(filename.c_str()); 
										},
										[](Mix_Music* pMusic)
										{ 
											Mix_FreeMusic(pMusic);
										});

//////////////////////////////////////////////////////////////////////////////////////////////
// CSound

CSound::CSound()
{
	CSoundPlayer::InitAudio();
	m_pMusic = NULL;
}

CSound::CSound(std::string filename)
{
	CSoundPlayer::InitAudio();
	m_pMusic = NULL;
	Load(filename);
}

CSound::CSound(Mix_Music *pMusic)
{
	CSoundPlayer::InitAudio();
	m_pMusic = NULL;
	Attach(pMusic);
}

CSound::~CSound(void)
{
	Unload();
	CSoundPlayer::DoneAudio();
}

void CSound::Load(std::string filename)
{
	Unload();
	m_pMusic = c_filemgr.Load(filename);
}

void CSound::Unload()
{
	// nothing special to do - chunks are cached somewhere else
	m_pMusic = NULL;
}

void CSound::Attach(Mix_Music *pMusic)
{
	Unload();
	m_pMusic = pMusic;
}

void CSound::Detach()
{
	Unload();
}

//////////////////////////////////////////////////////////////////////////////////////////////
// CSoundPlayer

std::map<int, CSoundPlayer*> c_mapChan2Player;

CSoundPlayer::CSoundPlayer()
{
	InitAudio();
	m_nChannel = -1;
	m_pSound = NULL;
	m_bSoundOwned = false;
	m_mode = TERMINATE_AND_PLAY;
}

CSoundPlayer::~CSoundPlayer(void)
{
	DoneAudio();
	if (m_bSoundOwned && m_pSound) delete m_pSound;
}

void CSoundPlayer::InitAudio()
{
	if (c_cnt++ == 0)
		Mix_OpenAudio(c_frequency, c_format, c_channels, c_chunksize);
}

void CSoundPlayer::DoneAudio()
{
	if (--c_cnt == 0)
		Mix_CloseAudio();
}

void CSoundPlayer::SetAudioParams(int frequency, Uint16 format, int channels, int chunksize)
{
	c_frequency = frequency;
	c_format = format;
	c_channels = channels;
	c_chunksize = chunksize;
	if (c_cnt)
	{
		Mix_CloseAudio();
		Mix_OpenAudio(c_frequency, c_format, c_channels, c_chunksize);
	}
}

void CSoundPlayer::Play(CSound *pSnd, int nRep, int nFadeIn)
{
	CSound *pNowPlaying = IsPlaying();
	CSound *pLastPlaying = LastPlaying();

	switch (m_mode)
	{
		case TERMINATE_AND_PLAY:	// always terminate and play
			break;
		case PLAY_IF_IDLE:			// only play if idle (not playing)
			if (pNowPlaying) return;
		case PLAY_IF_NEW:			// only play if idle or a new sound - prevent restarting the same sound whilst playing
			if (pSnd && pNowPlaying  && pSnd->GetMusic() == pNowPlaying->GetMusic()) return;
			break;
		case PLAY_ONCE:				// only play if a new sound - prevent re-playing the same sound
			if (pSnd && pLastPlaying && pSnd->GetMusic() == pLastPlaying->GetMusic()) return;
			break;
	}
	
	if (pNowPlaying)
		if (nFadeIn)
			Mix_FadeOutChannel(m_nChannel, nFadeIn);
		else
			Mix_HaltChannel(m_nChannel);

	if (m_bSoundOwned && m_pSound) delete m_pSound;
	m_bSoundOwned = false;
	m_pSound = pSnd;
	if (!m_pSound) return;
	Mix_Music *pMusic = m_pSound->GetMusic();
	if (!pMusic) return;

	if (nFadeIn)
		;// m_nChannel = Mix_FadeInChannel(-1, pMusic, nRep, nFadeIn);							CURRENTLY NOT IMPLEMENTED!!!
	else
		m_nChannel = Mix_PlayMusic(pMusic, nRep);

	c_mapChan2Player[m_nChannel] = this;
}

void CSoundPlayer::Play(std::string filename, int nRep, int nFadeIn)
{
	std::cout << filename << std::endl;
	Play(new CSound(filename), nRep, nFadeIn);
	m_bSoundOwned = true;
}

CSound *CSoundPlayer::IsPlaying()
{ 
	if (m_nChannel >= 0 
		&& m_pSound
		&& c_mapChan2Player[m_nChannel] == this
		&& Mix_Playing(m_nChannel) != 0)
			return m_pSound;

	m_nChannel = -1;
	return NULL;
}

CSound *CSoundPlayer::LastPlaying()		{ return m_pSound; }

void CSoundPlayer::Pause()				{ if (IsPlaying()) Mix_Pause(m_nChannel); }
void CSoundPlayer::Resume()				{ if (IsPlaying()) Mix_Resume(m_nChannel); }
bool CSoundPlayer::IsPaused()			{ return Mix_Paused(m_nChannel) != 0; }
void CSoundPlayer::Volume(float vol)	{ if (IsPlaying()) Mix_Volume(m_nChannel, (int)(vol * 128)); }
void CSoundPlayer::Stop()				{ if (IsPlaying()) Mix_HaltChannel(m_nChannel); m_nChannel = -1; }
void CSoundPlayer::FadeOut(int ms)		{ if (IsPlaying()) Mix_FadeOutChannel(m_nChannel, ms); }
void CSoundPlayer::Expire(int ms)		{ if (IsPlaying()) Mix_ExpireChannel(m_nChannel, ms); }

void CSoundPlayer::SetPosition(Sint16 angle, Uint8 distance)	{ Mix_SetPosition(m_nChannel, angle, distance); }
