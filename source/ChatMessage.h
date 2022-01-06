//
// Created by fanick on 15.10.20.
//

#ifndef DUEL6_CHATMESSAGE_H
#define DUEL6_CHATMESSAGE_H

#include <string>
#include "Type.h"
namespace Duel6 {
    class ChatMessage {
    public:
        Float32 remainingTime;
        bool system;
        std::string origin; // I don't know
        std::string text;

    public:
        ChatMessage(const std::string &text, Float32 duration, const std::string &origin = "", bool system = false)
                : remainingTime(duration), system(system), origin(origin), text(text){}

        ChatMessage(const ChatMessage &msg)
                : remainingTime(msg.remainingTime), system(msg.system), origin(msg.origin), text(msg.text) {}

        ChatMessage &operator=(const ChatMessage &msg) {
            remainingTime = msg.remainingTime;
            system = msg.system;
            origin = msg.origin;
            text = msg.text;
            return *this;
        }

        const std::string &getOrigin() const {
            return origin;
        }

        const std::string &getText() const {
            return text;
        }

        Float32 getRemainingTime() const {
            return remainingTime;
        }

        bool isSystem() const {
            return system;
        }

        ChatMessage &updateRemainingTime(Float32 elapsedTime) {
            remainingTime -= elapsedTime;
            return *this;
        }
    };
}
#endif //DUEL6_CHATMESSAGE_H
