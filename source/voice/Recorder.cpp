//
// Created by fanick on 25.11.21.
//
#include <array>
#include <vector>
#include <functional>
#include <opus/opus.h>
#include "Recorder.h"
namespace Duel6 {
    Recorder::Recorder(int deviceIndex) {
        useDevice(deviceIndex);
        // AUDIO_S16LSB is used throughout the audio code because it is consumed by opus
        const SDL_AudioFormat src_format = AUDIO_S16LSB;
        const Uint8 src_channels = 1;
        const int src_rate = VOICE_FREQ;
        const SDL_AudioFormat dst_format = AUDIO_S16LSB;
        const Uint8 dst_channels = 1;
        const int dst_rate = VOICE_FREQ;

        //todo take input params
        stream = SDL_NewAudioStream(src_format, src_channels, src_rate, dst_format, dst_channels, dst_rate);

        int error;
        int application = OPUS_APPLICATION_VOIP;

        opusEncoder = opus_encoder_create(src_rate, src_channels, application, &error);
        if (error != OPUS_OK) {
            printf("Failed to initialize Opus encoder %d\n", error);
        }
        opus_encoder_ctl(opusEncoder, OPUS_SET_DTX(1));
        opus_encoder_ctl(opusEncoder, OPUS_SET_COMPLEXITY(10));
//        opus_encoder_ctl(opusEncoder, OPUS_SET_PREDICTION_DISABLED(1));
        opus_encoder_ctl(opusEncoder, OPUS_SET_PACKET_LOSS_PERC(0));
        opus_encoder_ctl(opusEncoder, OPUS_SET_VBR(1));
    }

    int Recorder::useDevice(int deviceIndex) {
        int num = SDL_GetNumAudioDevices(SDL_TRUE);
        if (deviceIndex >= num) {
            return -1;
        }
        close();
        return open(deviceIndex);
    }

    int Recorder::open(int deviceIndex) {
        SDL_AudioSpec requested, received;

        requested.channels = 1;
        requested.format = AUDIO_S16LSB;
        requested.freq = VOICE_FREQ;
        requested.samples = VOICE_FRAME_SIZE;
        requested.callback = Recorder::forwardCallback;
        requested.userdata = this;
        name = SDL_GetAudioDeviceName(deviceIndex, SDL_TRUE);
        deviceId = SDL_OpenAudioDevice(name.c_str(), SDL_TRUE, &requested, &received, 0);
        if (deviceId >= 2) {
            opened = true;
        } else {
            printf("Recorder failed to open capture device\n");
        }
        return deviceId;
    }

    void Recorder::close() {
        if (opened) {
            SDL_CloseAudioDevice(deviceId);
            opened = false;
        }
    }

    Recorder::~Recorder() {
        close();
        SDL_FreeAudioStream(stream);
        opus_encoder_destroy(opusEncoder);
    }

    const std::string &Recorder::getName() const {
        return name;
    }

    bool Recorder::isOpened() const {
        return opened;
    }

    void Recorder::pause() {
        pause(true);
    }

    void Recorder::unpause() {
        pause(false);
    }

    void Recorder::toggle() {
        pause(!paused);
    }

    void Recorder::pause(bool value) {
        paused = value;
        if (opened) {
            SDL_PauseAudioDevice(deviceId, paused ? SDL_TRUE : SDL_FALSE);
        }
    }

    void Recorder::startRecording(Int32 playerId) {
        if(triggerCnt == 0){
            owningPlayerId = playerId;
        }
        triggerCnt++;
        unpause();
    }

    void Recorder::stopRecording() {
        if(--triggerCnt == 0){
            pause();
        }
    }

    void Recorder::recordAudio(Uint8 *buff, int len) {
        if (len < 1) {
            return;
        }
        SDL_AudioStreamPut(stream, buff, len);
    }
    size_t Recorder::dequeueEncodedAudio(Uint8 *buffer, const size_t maxLen, bool &errorOut) {
        static opus_int16 soundFrame[VOICE_FRAME_SIZE];

        int avail = SDL_AudioStreamAvailable(stream) * sizeof(opus_int16); // Uint8 in stream vs opus_int16 in soundFrame
        SDL_LockAudioDevice(deviceId); //TODO check if locking is necessary ( check SDL_AudioStream thread safety)
        auto readRes = SDL_AudioStreamGet(stream, (Uint8 *) soundFrame, VOICE_FRAME_SIZE * sizeof(opus_int16));
        SDL_UnlockAudioDevice(deviceId);

        if(readRes == -1){
            printf("Failed to get audio from stream %i\n", readRes);
            errorOut = true;
            return 0;
        } else if(readRes < VOICE_FRAME_SIZE * sizeof(opus_int16)){
            printf("Stream underflow %i/%u\n", readRes, VOICE_FRAME_SIZE * sizeof(opus_int16));
        }
        int res = opus_encode(opusEncoder, soundFrame, VOICE_FRAME_SIZE, buffer, maxLen);
        if (res < 0) {
            printf("opus_encode error %i\n", res);
            errorOut = true;
            return 0;
        } else {
            printf("opus res %d\n", res);
        }
        errorOut = false;
        return res;
    }

    void Recorder::dequeueRawAudio(Uint8 *buff, int len) {
        SDL_LockAudioDevice(deviceId);
        int avail = SDL_AudioStreamAvailable(stream);
        if (avail >= len) {
            SDL_AudioStreamGet(stream, buff, len);
        }
        SDL_UnlockAudioDevice(deviceId);
    }

    Int32 Recorder::getOwningPlayerId() {
        return owningPlayerId;
    }

    bool Recorder::hasEncodedAudioReady() {
        return SDL_AudioStreamAvailable(stream) * sizeof(opus_int16) >= VOICE_FRAME_SIZE;
    }
}