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

#include "ScriptManager.h"

#ifdef D6_SCRIPTING_LUA

#include "lua/LuaScriptLoader.h"

#endif

namespace Duel6::Script {
    ScriptManager::ScriptManager(ScriptContext &context)
            : context(context) {}

    void ScriptManager::registerLoaders() {
#ifdef D6_SCRIPTING_LUA
        auto luaLoader = std::make_unique<LuaScriptLoader>(context);
        loaders.push_back(std::move(luaLoader));
#endif
    }

    ScriptManager::LevelScriptList ScriptManager::loadLevelScripts() {
        LevelScriptList result;

        for (auto &loader : loaders) {
            auto script = loader->loadLevelScript();
            if (script) {
                result.push_back(std::move(script));
            }
        }

        return result;
    }

    ScriptManager::PlayerScriptList
    ScriptManager::loadPlayerScripts(const std::string &name, const std::string &directory) {
        PlayerScriptList result;

        for (auto &loader : loaders) {
            auto script = loader->loadPlayerScript(name, directory);
            if (script) {
                result.push_back(std::move(script));
            }
        }

        return result;
    }
}