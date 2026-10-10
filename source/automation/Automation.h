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


#ifndef DUEL6_AUTOMATION_AUTOMATION_H
#define DUEL6_AUTOMATION_AUTOMATION_H

#include <string>
#include "../ApplicationListener.h"
#include "Timeline.h"

namespace Duel6 {
    class AppService;
    class CommandLine;
    class Console;
    class Menu;
    class Video;

    // Scripted runs for tests (tests/render_test.py), built with the CMake option D6_AUTOMATION.
    // The run is described by command-line flags:
    //   -seed N                  seed the random generator, so runs are reproducible
    //   -play [-level NAME]      start a game right away, on all levels when no level is given
    //   -fullscreen-after S      toggle fullscreen
    //   -screenshot PATH         save the presented frame at -screenshot-after S (default 3)
    //   -quit-after S            quit
    //   -print-fps               log the frame rate once a second
    // Times are seconds of game time (see Timeline). Progress is logged with an "[automation]" prefix.
    class Automation : public ApplicationListener {
    private:
        Console &console;
        Video &video;
        Timeline timeline;
        bool printFps = false;
        Uint32 nextFpsReport = 0;

    public:
        Automation(const CommandLine &commandLine, AppService &appService, Menu &menu);

        void onStart() override;

        void onTick() override;

    private:
        void scheduleScreenshot(Float64 seconds, const std::string &path);
    };
}

#endif
