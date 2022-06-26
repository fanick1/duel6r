/*
* Copyright (c) 2006, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vector>
#include "SoundException.h"
#include "Sound.h"

namespace Duel6 {
    Uint8 Sound::voiceMixBuffer[Sound::frameLen];

    Sound::Sample::Sample(Sound *sound, Mix_Chunk *chunk)
            : sound(sound), chunk(chunk) {}

    Sound::Sample::Sample()
            : Sample(nullptr, nullptr) {}

    Sound::Sample::~Sample() {}

    void Sound::Sample::play() const {
        play(0.0f);
    }

    void Sound::Sample::play(float panning) const {
        if(panning < -1.0f){
            panning = -1.0f;
        }
        if(panning > 1.0f){
            panning = 1.0f;
        }
        panning += 1.0f;
        panning *= 255;
        Uint8 left = panning <= 255 ? 255 : 510 - panning;
        Uint8 right = panning >= 255 ? 255 : panning;
        if (sound != nullptr && chunk != nullptr) {
            sound->playSample(chunk, left, right);
        }
    }


    Sound::Track::Track(Sound *sound, Mix_Music *module)
            : sound(sound), module(module) {}

    Sound::Track::Track()
            : Track(nullptr, nullptr) {}

    Sound::Track::~Track() {}

    void Sound::Track::play(bool loop) const {
        if (sound != nullptr && module != nullptr) {
            sound->startMusic(module, loop);
        }
    }

    Sound::Sound(Int32 channels, Console &console)
            : console(console), channels(channels), playing(false) {
        console.printLine("\n===Initialization of sound sub-system===");
        console.printLine("...Starting SDL_mixer library");
        memset(Sound::voiceMixBuffer, 0, frameLen);
#ifdef D6_RENDERER_HEADLESS
        headless = true;
        console.printLine("Running in headless mode, suppressing sound system.");
        return;
#endif
        // Init SDL audio sub sytem
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            D6_THROW(SoundException, Format("SDL_Mixer error: {0}") << SDL_GetError());
        }


        int num = SDL_GetNumAudioDevices(SDL_FALSE);
        printf("Audio devices number: %i\n", num);
        console.printLine(Format("Audio devices number: {0}") << num);

        for(int i = 0; i < num; i++){
           auto name = SDL_GetAudioDeviceName(i, SDL_FALSE);
           printf("DEBUG: SOUND available device %i: %s\n", i, name);
           console.printLine(Format("DEBUG: SOUND available device {0}: {1}") << i << name);
        }

        auto name = SDL_GetAudioDeviceName(0, 0); //TODO
        SDL_AudioSpec requested, received;
        requested.channels = 1;
        requested.format = AUDIO_S16LSB;
        requested.freq = 48000; //TODO
        requested.samples = 960;//20ms //480;//DEBUG
        requested.callback = nullptr;

        auxdevice = SDL_OpenAudioDevice(NULL, SDL_FALSE, &requested, &received, SDL_FALSE);
        if(auxdevice < 2){
            D6_THROW(SoundException, Format("SDL_OpenAudioDevice error: {0}") << SDL_GetError());
        }
        SDL_PauseAudioDevice(auxdevice, SDL_FALSE);

        // Init SDL_mixer library
        if (Mix_OpenAudio(48000, AUDIO_S16LSB, 2, 480) == -1) {
            D6_THROW(SoundException, Format("SDL_Mixer error: {0}") << Mix_GetError());
        }
        if(Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG){
            D6_THROW(SoundException, Format("SDL_Mixer error whilie initializing OGG support: {0}") << Mix_GetError());
        }
        // Allocate channels
        channels = Mix_AllocateChannels(channels);
        console.print(Format("...Frequency: {0}\n...Channels: {1}\n") << MIX_DEFAULT_FREQUENCY << channels);
    }

    Sound::~Sound() {
        // Stop and free modules
        stopMusic();

        for (Mix_Music *module : modules) {
            Mix_FreeMusic(module);
        }

        // Free all mixing channels and samples
        Mix_AllocateChannels(0);

        for (Mix_Chunk *sample : samples) {
            Mix_FreeChunk(sample);
        }

        modules.clear();
        samples.clear();
        Mix_Quit();
        Mix_CloseAudio();
    }

    Sound::Track Sound::loadModule(const std::string &fileName) {
        if(headless) return  Track(this, nullptr);
        Mix_Music *module = Mix_LoadMUS(fileName.c_str());
        if (module == nullptr) {
            D6_THROW(SoundException,
                     Format("SDL_mixer error: unable to load module {0} ({1})") << fileName << Mix_GetError());
        }
        modules.push_back(module);
        console.printLine(Format("...Module loaded: {0}") << fileName);
        return Track(this, module);
    }

    Sound::Sample Sound::loadSample(const std::string &fileName) {
        if(headless) return  Sample(this, nullptr);
        Mix_Chunk *sample = Mix_LoadWAV(fileName.c_str());
        if (sample == nullptr) {
            D6_THROW(SoundException,
                     Format("SDL_mixer error: unable to load sample {0} ({1})") << fileName << Mix_GetError());
        }
        samples.push_back(sample);
        console.printLine(Format("...Sample loaded: {0}") << fileName);
        return Sample(this, sample);
    }

    void Sound::stopMusic() {
        if(headless) return;
        if (playing) {
            Mix_HaltMusic();
            playing = false;
        }
    }

    void Sound::startMusic(Mix_Music *music, bool loop) {
        if(headless) return;
        stopMusic();
        if (Mix_PlayMusic(music, loop ? -1 : 0) == -1) {
            D6_THROW(SoundException, Format("SDL_Mixer error: {0}") << Mix_GetError());
        }
        playing = true;
    }

    void Sound::playSample(Mix_Chunk *chunk, Uint8 left, Uint8 right) {
        if(headless) return;
        for (Int32 j = 0; j < channels; j++) {
            if (!Mix_Playing(j) && !Mix_Paused(j)) {
                Mix_SetPanning(j, left, right);
                if (Mix_PlayChannel(j, chunk, 0) == -1) {
                    D6_THROW(SoundException, Format("SDL_Mixer error: {0}") << Mix_GetError());
                }
                return;
            }
        }
    }

    void Sound::volume(Int32 volume) {
        if(headless) return;
        console.printLine(Format("...Volume set to {0}") << volume);
        vol = volume;
        Mix_VolumeMusic(volume);
        Mix_Volume(-1, volume);
    }

    void Sound::auxBatchMix(Uint8 * buf, int len){
        if(headless) {
            console.printLine(Format("Sound::auxBatchMix error headless"));
            return;
        }
        SDL_AudioSpec audioSpec;
        audioSpec.format = AUDIO_S16LSB;
        SDL_MixAudioFormat(Sound::voiceMixBuffer, buf, audioSpec.format, len, SDL_MIX_MAXVOLUME);
    }

    void Sound::auxBatchSubmit(){
        if(headless) return;
        auto res = SDL_QueueAudio(auxdevice, Sound::voiceMixBuffer, frameLen);
        if(res == -1){
            printf("Failed to queue audio\n");
            return;
        }
        memset(Sound::voiceMixBuffer, 0, frameLen);
    }
    void Sound::auxQueueSilence() {
        if(headless) return;
        static Uint8 silence[frameLen * 4] = {1};
        SDL_QueueAudio(auxdevice, silence, frameLen);
    }
    void Sound::auxPause(int i) {
        if(headless) return;
        SDL_PauseAudioDevice(auxdevice, SDL_TRUE);
    }

    void Sound::auxUnpause() {
        if(headless) return;
        SDL_PauseAudioDevice(auxdevice, SDL_FALSE);
    }

}
