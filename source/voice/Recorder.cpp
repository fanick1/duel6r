//
// Created by fanick on 25.11.21.
//
#include <array>
#include <vector>
#include <functional>
#include <opus/opus.h>
#include "Recorder.h"
#include "../console/Console.h"
namespace Duel6 {
    Recorder::Recorder(Console &console, int deviceIndex):console(console) {
        useDevice(deviceIndex);

        // AUDIO_S16LSB is used throughout the audio code because it is consumed by opus
        // You can't fucking pass this on mac into SDL_NewAudioStream : const SDL_AudioFormat src_format = AUDIO_S16LSB;
        SDL_AudioSpec dst;
        //REAL WTF RIGHT HERE. YOU HAVE TO PASS THE VALUE FROM THIS STRUCT into SDL_NewAudioStream
        dst.format = AUDIO_S16LSB;

        const int src_rate = VOICE_FREQ;
        const Uint8 dst_channels = 1;
        const int dst_rate = VOICE_FREQ;
        //todo take input params
        stream = SDL_NewAudioStream(captureSpec.format, captureSpec.channels, captureSpec.freq, dst.format, dst_channels, dst_rate);
        if(!stream){
            const char * lastError = SDL_GetError();
            printf("SDL_NewAudioStream failed %s\n", lastError);
            console.printLine(Format("SDL_NewAudioStream failed {0}") << lastError);
        }
        int error;
        int application = OPUS_APPLICATION_VOIP;

        opusEncoder = opus_encoder_create(captureSpec.freq, captureSpec.channels, application, &error);
        if (error != OPUS_OK) {
            printf("Failed to initialize Opus encoder %d\n", error);
            console.printLine(Format("Failed to initialize Opus encoder {0}") << error);
        }
        error = opus_encoder_ctl(opusEncoder, OPUS_SET_DTX(0));
        if (error != OPUS_OK) {
            console.printLine(Format("Failed to set OPUS_SET_DTX on Opus encoder{0}") << error);
        }
        error = opus_encoder_ctl(opusEncoder, OPUS_SET_COMPLEXITY(10));
        if (error != OPUS_OK) {
            console.printLine(Format("Failed to set OPUS_SET_COMPLEXITY on Opus encoder{0}") << error);
        }
//      opus_encoder_ctl(opusEncoder, OPUS_SET_PREDICTION_DISABLED(1));
        error = opus_encoder_ctl(opusEncoder, OPUS_SET_PACKET_LOSS_PERC(10));
        if (error != OPUS_OK) {
            console.printLine(Format("Failed to set OPUS_SET_PACKET_LOSS_PERC on Opus encoder{0}") << error);
        }
        error = opus_encoder_ctl(opusEncoder, OPUS_SET_VBR(1));
        if (error != OPUS_OK) {
            console.printLine(Format("Failed to set OPUS_SET_VBR on Opus encoder{0}") << error);
        }
    }

    int Recorder::useDevice(int deviceIndex) {
        int num = SDL_GetNumAudioDevices(SDL_TRUE);
        console.printLine(Format("Audio devices number: {0}") << num);

        for(int i = 0; i < num; i++){
           name = SDL_GetAudioDeviceName(i, SDL_TRUE);
           console.printLine(Format("DEBUG: available Recorder capture device {0}: {1}") << deviceId << name.c_str());
        }
        if (deviceIndex >= num) {
            return -1;
        }
        close();
        return open(deviceIndex);
    }

    int Recorder::open(int deviceIndex) {
        SDL_AudioSpec requested;

        requested.channels = 1;
        requested.format = AUDIO_S16LSB;
        requested.freq = VOICE_FREQ;
        requested.samples = VOICE_FRAME_SIZE;
        requested.callback = Recorder::forwardCallback;
        requested.userdata = this;
        name = SDL_GetAudioDeviceName(deviceIndex, SDL_TRUE);

        deviceId = SDL_OpenAudioDevice(name.c_str(), SDL_TRUE, &requested, &captureSpec, 0); // 0 - no change allowed

        if (deviceId >= 2) {
            opened = true;
            console.printLine(Format("Recorder opened capture device {0}: {1}") << deviceId << name.c_str());
        } else {
            printf("Recorder failed to open capture device\n");
            console.printLine(Format("Recorder failed to open capture device {0}: {1}") << deviceId << name.c_str());
        }
        return deviceId;
    }

    void Recorder::close() {
        clients.reset();
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
        if(clients.none()){
            owningPlayerId = playerId;
        }
        clients.set(playerId);
        unpause();
    }

    void Recorder::stopRecording(Int32 playerId) {
        clients.reset(playerId);
        if(clients.none()){
            pause();
        }
    }

    void Recorder::recordAudio(Uint8 *buff, int len) {
        if (len < 1) {
            return;
        }
        int err = SDL_AudioStreamPut(stream, buff, len);
        if(err != 0){
           console.printLine(Format("Recorder::recordAudio SDL_AudioStreamPut failed: {0} {1}") << err << SDL_GetError());
        }
    }

    size_t Recorder::dequeueEncodedAudio(Uint8 *buffer, const size_t maxLen, bool &errorOut) {
        static opus_int16 soundFrame[VOICE_FRAME_SIZE];

        SDL_LockAudioDevice(deviceId); //TODO check if locking is necessary ( check SDL_AudioStream thread safety)
        auto readRes = SDL_AudioStreamGet(stream, (Uint8 *) soundFrame, VOICE_FRAME_SIZE * sizeof(opus_int16));
        SDL_UnlockAudioDevice(deviceId);

        if(readRes == -1){
            printf("Failed to get audio from stream %i\n", readRes);
            console.printLine(Format("Failed to get audio from stream {0}:") << readRes);
            errorOut = true;
            return 0;
        } else if(readRes < (int) (VOICE_FRAME_SIZE * sizeof(opus_int16))){
            printf("Stream underflow %i/%lu\n", readRes, VOICE_FRAME_SIZE * sizeof(opus_int16));
            console.printLine(Format("Stream underflow {0}/{1}") << readRes << VOICE_FRAME_SIZE * sizeof(opus_int16));
        }
        int res = opus_encode(opusEncoder, soundFrame, VOICE_FRAME_SIZE, buffer, maxLen);
        if (res < 0) {
            printf("opus_encode error %i\n", res);
            console.printLine(Format("opus_encode error {0}") << res);

            errorOut = true;
            return 0;
        } else {
//            printf("opus res %d\n", res);
        }
        errorOut = false;
        return res;
    }

    Int32 Recorder::getOwningPlayerId() {
        return owningPlayerId;
    }

    bool Recorder::hasEncodedAudioReady() {
        return SDL_AudioStreamAvailable(stream) * sizeof(opus_int16) >= VOICE_FRAME_SIZE;
    }
}