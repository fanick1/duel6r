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

#include "Player.h"
#include "ElevatorList.h"
#include "json/JsonParser.h"

namespace Duel6 {
    ElevatorList::ElevatorList(Texture texture)
            : texture(texture) {}

    void ElevatorList::add(Elevator &elevator) {
        elevators.push_back(elevator);
        elevators.back().start();
    }

    void ElevatorList::load(const std::vector<Elevator> & elevators) {
        this->elevators = elevators;
    }

    void ElevatorList::load(const std::string &path, bool mirror) {
        Json::Parser parser;
        Json::Value root = parser.parse(path);

        Int32 width = root.get("width").asInt();
        Int32 height = root.get("height").asInt();

        Size elevators = root.get("elevators").getLength();
        for (Size i = 0; i < elevators; i++) {
            Json::Value definition = root.get("elevators").get(i);
            bool circular = definition.getOrDefault("circular", Json::Value::makeBoolean(false)).asBoolean();
            Elevator elevator(circular);
            Json::Value points = definition.get("controlPoints");
            for (Size j = 0; j < points.getLength(); j++) {
                Int32 x = points.get(j).get("x").asInt();
                Int32 y = points.get(j).get("y").asInt();
                Int32 wait = points.get(j).getOrDefault("wait", Json::Value::makeNumber(0)).asInt();
                elevator.addControlPoint(Elevator::ControlPoint(mirror ? width - 1 - x : x, height - y, wait));
            }
            add(elevator);
        }
    }

    void ElevatorList::update(Float32 elapsedTime) {
        for (Elevator &elevator : elevators) {
            elevator.update(elapsedTime);
        }
    }

    void ElevatorList::render() const {
        for (const Elevator &elevator : elevators) {
            elevator.render(texture);
        }
    }

    const Elevator *ElevatorList::checkCollider(CollidingEntity &collider, Float32 speedFactor) {
        const Float32 distanceThreshold = 0.05f;
        Float32 y = collider.position.y;
        Float32 cX = collider.position.x + 0.5f;
        Float32 leeway = 0.25f * collider.dimensions.x;
        Vector playerVelocity = collider.velocity;
        Float32 playerVerticalStep = playerVelocity.y * speedFactor;

        for (const Elevator &elevator : elevators) {
            const Vector &pos = elevator.getPosition();
            const Vector &acceleratedVelocity = elevator.getVelocity();

            if (elevator.getVelocity().y < 0 && playerVelocity.y > 0) {
                continue;
            }

            Float32 elevatorVerticalStep = acceleratedVelocity.y * speedFactor;
            if (cX + leeway >= pos.x && cX - leeway <= pos.x + 1.0f)
            {
                bool before_below = y <= pos.y - distanceThreshold - elevatorVerticalStep;
                bool before_above = y >= pos.y - elevatorVerticalStep + distanceThreshold;
                bool before_inside = y >= pos.y - distanceThreshold - elevatorVerticalStep &&
                                     y - playerVerticalStep <=
                                     pos.y - elevatorVerticalStep + distanceThreshold;
                bool after_above = y + 2 * playerVerticalStep >= pos.y + distanceThreshold;
                bool after_below = y + playerVerticalStep <= pos.y - distanceThreshold;
                bool after_inside = y + playerVerticalStep >= pos.y - distanceThreshold &&
                                    y + playerVerticalStep <= pos.y + distanceThreshold;

                if ((before_below && after_inside && !after_above)
                    || (before_above && (after_below || after_inside))
                    || (before_inside && (after_below || after_inside))) {
                    return &elevator;
                }
            }
        }
        return nullptr;
    }
}
