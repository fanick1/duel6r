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


#include <cstdlib>
#include <vector>
#include <SDL2/SDL.h>
#include "Automation.h"
#include "../AppService.h"
#include "../CommandLine.h"
#include "../Defines.h"
#include "../Format.h"
#include "../Image.h"
#include "../Menu.h"
#include "../math/Math.h"

namespace Duel6 {
    namespace {
        Float64 getSeconds(const CommandLine &commandLine, const std::string &option, Float64 defaultValue) {
            const char *value = commandLine.get(option);
            return value != nullptr ? atof(value) : defaultValue;
        }

        void requestQuit() {
            SDL_Event event{};
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
        }
    }

    Automation::Automation(const CommandLine &commandLine, AppService &appService, Menu &menu)
            : console(appService.getConsole()), video(appService.getVideo()) {
        const char *seed = commandLine.get("-seed");
        if (seed != nullptr) {
            Math::seed(static_cast<Uint32>(strtoul(seed, nullptr, 10)));
        }

        if (commandLine.has("-play")) {
            const char *level = commandLine.get("-level");
            std::vector<std::string> levels;
            if (level != nullptr) {
                levels.push_back(D6_FILE_LEVEL + std::string(level) + D6_LEVEL_EXTENSION);
            }
            timeline.atStart([&menu, levels]() {
                menu.play(levels.empty() ? menu.listMaps() : levels, false, false);
            });
        }

        if (commandLine.get("-fullscreen-after") != nullptr) {
            timeline.at(getSeconds(commandLine, "-fullscreen-after", 0), [this]() {
                console.printLine("[automation] toggling fullscreen");
                video.toggleFullscreen();
            });
        }

        const char *screenshot = commandLine.get("-screenshot");
        if (screenshot != nullptr) {
            scheduleScreenshot(getSeconds(commandLine, "-screenshot-after", 3.0), screenshot);
        }

        printFps = commandLine.has("-print-fps");

        if (commandLine.get("-quit-after") != nullptr) {
            timeline.at(getSeconds(commandLine, "-quit-after", 0), [this]() {
                console.printLine("[automation] quit");
                requestQuit();
            });
        }
    }

    void Automation::scheduleScreenshot(Float64 seconds, const std::string &path) {
        timeline.at(seconds, [this, path]() {
            video.captureNextFrame([this, path](const Image &image) {
                video.logSizes(Format("screenshot at tick {0}") << timeline.getTick());
                image.save(path);
                console.printLine(Format("[automation] screenshot {0}x{1} saved to {2}")
                                  << image.getWidth() << image.getHeight() << path);
            });
        });
    }

    void Automation::onStart() {
        nextFpsReport = SDL_GetTicks() + 1000;
        timeline.start();
    }

    void Automation::onTick() {
        timeline.tick();

        // Wall-clock time: at start-up the game catches up on many ticks within one frame
        if (printFps && SDL_GetTicks() >= nextFpsReport) {
            nextFpsReport = SDL_GetTicks() + 1000;
            console.printLine(Format("[automation] fps {0}") << video.getFps());
        }
    }
}
