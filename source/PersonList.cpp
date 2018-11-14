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

#include "PersonList.h"

namespace Duel6 {
    Person &PersonList::getByName(const std::string &name) {
        auto iter = std::find_if(persons.begin(), persons.end(), [&name](const Person &person) {
            return person.getName() == name;
        });
        return *iter;
    }

    bool PersonList::contains(const std::string &name) const {
        auto iter = std::find_if(persons.cbegin(), persons.cend(), [&name](const Person &person) {
            return person.getName() == name;
        });
        return iter != persons.end();
    }

    Json::Value PersonList::toJson() const {
        Json::Value json = Json::Value::makeArray();
        for (const Person &person : persons) {
            json.add(person.toJson());
        }

        return json;
    }

    void PersonList::fromJson(const Json::Value &json, PersonProfileList &profileList) {
        persons.clear();
        for (Size i = 0; i < json.getLength(); i++) {
            persons.push_back(Person::fromJson(json.get(i)));

            auto& person = persons.back();
            auto profile = profileList.find(person.getName());
            if (profile != profileList.end()) {
                person.setProfile(profile->second.get());
            }
        }
    }

    PersonList &PersonList::remove(const std::string &name) {
        auto iter = std::find_if(persons.cbegin(), persons.cend(), [&name](const Person &person) {
            return person.getName() == name;
        });
        persons.erase(iter);
        return *this;
    }
}