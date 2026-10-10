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

#ifndef DUEL6_APPLICATION_H
#define DUEL6_APPLICATION_H

#include <utility>
#include <vector>
#include <memory>
#include "Context.h"
#include "AppService.h"
#include "Menu.h"
#include "Game.h"
#include "Video.h"
#include "script/ScriptManager.h"
#include "net/Net.h"
#include "net/NetHost.h"
#include "net/NetClient.h"
#include "net/ClientGameProxy.h"
#include "net/ServerGameProxy.h"

namespace Duel6 {
    class CommandLine;
    class Recorder;

    class Application {
    private:
        struct CommandLineOptions {
            bool dedicated;
            bool windowed;
            bool gameStateIntegration = false;
            std::string gsiURL = "http://127.0.0.1:3000/api/duel6r-gsi";
            Uint16 gsiPort = 0;
            Int32 windowWidth;
            Int32 windowHeight;

            // Automation for tests. Times are seconds since the main loop started, converted to
            // game ticks, so two runs with the same seed render the same frame.
            Uint32 seed = 0;                               // 0 = not seeded
            bool play = false;                             // start a game instead of staying in the menu
            std::string level;                             // level name for play, empty = all levels
            bool splitScreen = false;
            std::vector<std::pair<Int32, Int32>> resizes;  // applied one by one, resizeInterval apart
            Float64 resizeAfter = 1.0;
            Float64 resizeInterval = 0.5;
            Float64 fullscreenAfter = -1.0;
            std::string screenshotPath;
            Float64 screenshotAfter = 3.0;
            Float64 quitAfter = -1.0;
            bool printFps = false;                         // log the frame rate once a second
        };
        CommandLineOptions commandLineOptions;

    private:
        Console console;
        std::unique_ptr<Recorder> recorder; //TODO Have one recorder per capture device available in the system.
        std::unique_ptr<Video> video;
        std::unique_ptr<Font> font;
        std::unique_ptr<TextureManager> textureManager;
        Input input;
        PlayerControlsManager controlsManager;
        Sound sound;
        GameSettings gameSettings;
        GameResources gameResources;
        Script::ScriptContext scriptContext;
        Script::ScriptManager scriptManager;
        std::unique_ptr<Menu> menu;
        std::unique_ptr<Game> game;
        std::unique_ptr<AppService> service;
        std::unique_ptr<net::Net> net;
        std::unique_ptr<net::ClientGameProxy> clientGameProxy;
        std::unique_ptr<net::ServerGameProxy> serverGameProxy;
        std::unique_ptr<net::NetHost> netHost;
        std::unique_ptr<net::NetClient> netClient;
        bool requestClose;
        Uint32 automationTick = 0;
        size_t automationResizesDone = 0;
        bool automationFullscreenDone = false;
        bool automationScreenshotTaken = false;
        Uint32 automationNextFpsTick = 0;

    public:
        Application(Int32 argc, char **argv);

        ~Application();

        void run();

    private:
        void processEvents(Context &context);

        void textInputEvent(Context &context, const TextInputEvent &event);

        void keyEvent(Context &context, const KeyPressEvent &event);

        void mouseButtonEvent(Context &context, const MouseButtonEvent &event);

        void mouseMotionEvent(Context &context, const MouseMotionEvent &event);

        void mouseWheelEvent(Context &context, const MouseWheelEvent &event);

        void joyDeviceAddedEvent(Context & context, const JoyDeviceAddedEvent & event);
        void joyDeviceRemovedEvent(Context & context, const JoyDeviceRemovedEvent & event);

        void windowFocusEvent(Context & context, const WindowFocusEvent &event);

        Float64 syncUpdateAndRender(Context &context);

        void parseCommandLineOptions(const CommandLine &commandLine);

        void networkFlushCommands();

        void runAutomation();
        bool automationWantsScreenshot() const;
        void saveAutomationScreenshot(const Image &image);

        void networkRun(const Float64 elapsedTime);
    };
}

#endif
