//
// Created by fanick on 25.11.21.
//

#ifndef DUEL6R_RECORDER_H
#define DUEL6R_RECORDER_H

#include <string>
#include <SDL2/SDL_audio.h>
#include "../Type.h"
#include "VoiceSettings.h"
class OpusEncoder;

namespace Duel6 {
    class Recorder {
        static void forwardCallback(void *userdata, Uint8 *stream, int len) {
            static_cast<Recorder *>(userdata)->recordAudio(stream, len);
        }

    private:
        SDL_AudioDeviceID deviceId;
        SDL_AudioStream *stream;
        OpusEncoder *opusEncoder;

        bool opened = false;
        std::string name;
        bool paused = true;

        //little workaround - first player to trigger the voice recorder is the owner
        Int32 owningPlayerId = 0;
        // if more players play on the same computer,
        // we have to track number of players simultaneously using this device
        // TODO: The voice packet should carry info of all the speaking players.
        // TODO: case when player A triggers recording, then player B starts and later player A stops (the owner should transfer to B)
        size_t triggerCnt = 0;
    public:
        Recorder(int deviceIndex = 0);

        ~Recorder();

        // optionally we can switch the capture device
        int useDevice(int deviceIndex);

        const std::string &getName() const;

        bool isOpened() const;

        void startRecording(Int32 playerId);

        void stopRecording();

        bool hasEncodedAudioReady();

        [[nodiscard]] size_t dequeueEncodedAudio(Uint8 *buffer, const size_t maxLen, bool &errorOut);

        void dequeueRawAudio(Uint8 *buff, int maxLen);

        Int32 getOwningPlayerId();

        void pause(bool value);

        //basically stops recording
        void pause();

        //basically starts recording
        void unpause();
    private:

        void toggle();

        int open(int deviceIndex);

        void close();

        void recordAudio(Uint8 *buff, int len);

    };
}

#endif //DUEL6R_RECORDER_H
