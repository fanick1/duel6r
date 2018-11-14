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

#include "BonusList.h"
#include "ElevatorList.h"
#include "Game.h"
#include "GameMode.h"
#include "Explosion.h"

namespace Duel6 {
    void WorldRenderer::setView(const PlayerView &view) const {
        setView(view.getX(), view.getY(), view.getWidth(), view.getHeight());
    }

    void WorldRenderer::setView(int x, int y, int width, int height) const {
        globRenderer->setViewport(x, y, width, height);
    }

    void WorldRenderer::walls(const FaceList &walls) const {
        globRenderer->enableFaceCulling(true);
        walls.render(game.getResources().getBlockTextures(), false);
        globRenderer->enableFaceCulling(false);
    }

    void WorldRenderer::water(const FaceList &water) const {
        globRenderer->enableDepthWrite(false);
        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcColor);

        water.render(game.getResources().getBlockTextures(), false);

        globRenderer->setBlendFunc(Renderer::BlendFunc::None);
        globRenderer->enableDepthWrite(true);
    }

    void WorldRenderer::sprites(const FaceList &sprites) const {
        sprites.render(game.getResources().getBlockTextures(), true);
    }

    void WorldRenderer::background(Texture texture) const {
        Int32 cw = video.getScreen().getClientWidth();
        Int32 ch = video.getScreen().getClientHeight();
        globRenderer->quadXY(Vector(0, 0), Vector(cw, ch), Vector(0, 1), Vector(1, -1), Material::makeTexture(texture));
    }

    void WorldRenderer::playerRankings() const {
        Float32 fontSize = 16;
        Float32 fontWidth = fontSize / 2;
        Ranking ranking = game.getMode().getRanking(game.getPlayers());
        Int32 maxNameLength = ranking.getMaxLength() + 6;

        const PlayerView &view = game.getPlayers().front().getView();
        Int32 posX = view.getX() + view.getWidth() - fontWidth * maxNameLength - 3;
        Int32 posY = view.getY() + view.getHeight() - 20;

        for (const auto &entry : ranking.entries) {
            posY = renderRankingEntry(entry, posX, posY, maxNameLength, fontSize, false);
            for (const auto &nestedRankingEntry : entry.entries) {
                posY = renderRankingEntry(nestedRankingEntry, posX, posY, maxNameLength, fontSize, false);
            }
        }
    }
    Int32 WorldRenderer::renderRankingEntry(const Ranking::Entry &entry, Int32 posX, Int32 posY, Int32 maxLength) const {
        return renderRankingEntry(entry, posX, posY, maxLength, font.getCharHeight(), false);
    }

    Int32 WorldRenderer::renderRankingEntry(const Ranking::Entry &entry, Int32 posX, Int32 posY, Int32 maxLength, Float32 charHeight, bool extended) const {
        if (extended) {
            maxLength += 20;
        }
        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcAlpha);
        Color fontColor(entry.fontColor);
        globRenderer->quadXY(Vector(posX, posY + 1), Vector((charHeight/2) * maxLength, charHeight), entry.bcgColor);

        Int32 paddingLeft = entry.isSuperEntry() ? 0 : 5;
        globRenderer->setBlendFunc(Renderer::BlendFunc::None);
        font.print(posX + paddingLeft, posY, 0.0f, fontColor, entry.name, charHeight);

        if(extended) {
            globRenderer->setBlendFunc(Renderer::BlendFunc::SrcColor);
            if(!entry.isSuperEntry()) {
                fontColor = Color::YELLOW;
            }
            auto kd = (entry.deaths != 0 ? (float)entry.kills / (float)entry.deaths : entry.kills);
            auto kd_int = (int)std::floor(kd);
            auto kd_float = (int)((kd - kd_int) * 100);
            font.print(posX + (charHeight / 2) * (maxLength - 23), posY, 0.0f, fontColor, (Format("|{0,3}|{1,3}|{2,3}|{3,2}.{4,-2}|{5,4}") << entry.kills  << entry.assistances << entry.deaths << kd_int << kd_float << entry.points), charHeight);
        } else {
            font.print(posX + (charHeight / 2) * (maxLength - 5), posY, 0.0f, fontColor, (Format("|{0,4}") << entry.points), charHeight);
        }
        return posY - charHeight;
    }

    void WorldRenderer::roundOverSummary() const {
        Float32 fontSize = 32;
        Float32 fontWidth = fontSize / 2;
        Ranking ranking = game.getMode().getRanking(game.getPlayers());
        Int32 maxLength = ranking.getMaxLength() + 6;
        Int32 maxNameLength = maxLength + 20;
        int height = fontSize * 3; // reserve for 'SCORE'
        int tableWidth = maxNameLength * (fontWidth);
        int width = std::max(tableWidth, 200);
        for (const auto &entry : ranking.entries) {
            height += fontSize * (1 + entry.entries.size());
        }
        const auto score = "---SCORE---";
        const auto kad = " K    A   D  K/D  PTS  ";
        const auto kadWidth = font.getTextWidth(kad, fontSize);
        const auto scoreWidth = font.getTextWidth(score, fontSize);

        int x = video.getScreen().getClientWidth() / 2 - width / 2;
        int y = video.getScreen().getClientHeight() / 2 - height / 2;

        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcAlpha);
        globRenderer->quadXY(Vector(x - fontWidth, y - fontSize), Vector(width + fontWidth, height + 2 * fontSize), Color(255, 255, 255, 80));
        globRenderer->quadXY(Vector(x - fontWidth + 2, y - fontSize + 2), Vector(width + fontWidth - 4, height + 2 * fontSize - 4), Color(0, 0, 255, 80));

        globRenderer->quadXY(Vector(x - fontWidth - 5, height + y - fontSize), Vector(width + fontWidth + 10,fontSize + 4), Color(0, 0, 255, 255));
        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcColor);

        Int32 posX = video.getScreen().getClientWidth() / 2 - tableWidth / 2;;
        Int32 posY = y + height - fontSize * 3;

        Color fontColor = Color::WHITE;

        font.print(x + (width - scoreWidth) / 2, y + height - fontSize, 0.0f, fontColor, score, fontSize);
        font.print(x + width - kadWidth, y + height - 2 * fontSize, 0.0f, fontColor, "  K   A   D   K/D  PTS", fontSize);
        for (const auto &entry : ranking.entries) {
            posY = renderRankingEntry(entry, posX, posY, maxLength, fontSize, true);
            for (const auto &nestedRankingEntry : entry.entries) {
                posY = renderRankingEntry(nestedRankingEntry, posX, posY, maxLength, fontSize, true);
            }
        }
    }

    void WorldRenderer::gameOverSummary() const {
        roundOverSummary();
    }

    void WorldRenderer::roundsPlayed() const {
        int width = 134;
        int x = video.getScreen().getClientWidth() / 2 - width / 2;
        int y = video.getScreen().getClientHeight() - 20;

        globRenderer->quadXY(Vector(x - 1, y - 1), Vector(width + 2, 18), Color::BLACK);
        font.print(x + 8, y, Color::WHITE,
                   Format("Rounds: {0,3}|{1,3}") << game.getPlayedRounds() << game.getSettings().getMaxRounds());
    }

    void WorldRenderer::fpsCounter() const {
        std::string fpsCount = Format("FPS - {0}") << Int32(video.getFps());
        Int32 width = 8 * Int32(fpsCount.size()) + 2;

        Int32 x = Int32(video.getScreen().getClientWidth()) - width;
        Int32 y = Int32(video.getScreen().getClientHeight()) - 20;

        globRenderer->quadXY(Vector(x - 1, y - 1), Vector(width + 2, 18), Color::BLACK);
        font.print(x, y, Color::WHITE, fpsCount);
    }

    void WorldRenderer::youAreHere() const {
        Float32 remainingTime = game.getRound().getRemainingYouAreHere();
        if (remainingTime <= 0) return;

        globRenderer->enableDepthTest(false);

        Float32 radius = 0.5f + 0.5f * std::abs(D6_YOU_ARE_HERE_DURATION / 2 - remainingTime);
        for (const Player &player : game.getPlayers()) {
            Vector playerCentre = player.getCentre();
            playerCentre.z = 0.5;

            Vector lastPoint;
            for (Int32 u = 0; u < 37; u++) {
                Float32 spike = (u % 2 == 0) ? 0.95f : 1.05f;
                Vector pos = playerCentre + spike * radius * Vector::direction(u * 10);
                if (u > 0) {
                    globRenderer->line(lastPoint, pos, 3.0f, Color::YELLOW);
                }
                lastPoint = pos;
            }
        }

        globRenderer->enableDepthTest(true);
    }

    Float32
    WorldRenderer::playerIndicator(const Player &player, const Indicator &indicator, const Color &color, Float32 value,
                                   Float32 xOfs, Float32 yOfs) const {
        Float32 width = value * 0.98f;
        Float32 X = xOfs - 0.5f;
        Float32 Y = yOfs;

        Uint8 alpha = Uint8(255 * indicator.getAlpha());

        globRenderer->enableDepthWrite(false);
        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcAlpha);
        globRenderer->quadXY(Vector(X, Y - 0.1f, 0.5f), Vector(1.0f, 0.1f), Color::BLACK.withAlpha(alpha));
        globRenderer->quadXY(Vector(X + 0.01f, Y - 0.08f, 0.5f), Vector(width, 0.07f), color.withAlpha(alpha));
        globRenderer->setBlendFunc(Renderer::BlendFunc::None);
        globRenderer->enableDepthWrite(true);

        return 0.1f;
    }

    void WorldRenderer::playerName(const Player &player, const Indicator &indicator, Float32 xOfs, Float32 yOfs) const {
        const std::string &name = player.getPerson().getName();

        Float32 width = 0.15f * name.size();
        Float32 X = xOfs - width / 2;
        Float32 Y = yOfs;

        Uint8 alpha = Uint8(255 * indicator.getAlpha());

        globRenderer->enableDepthWrite(false);
        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcAlpha);
        globRenderer->quadXY(Vector(X, Y, 0.5f), Vector(width, 0.3f), Color::BLUE.withAlpha(alpha));
        globRenderer->setBlendFunc(Renderer::BlendFunc::None);

        font.print(X, Y, 0.5f, Color::YELLOW.withAlpha(alpha), name, 0.3f);

        globRenderer->enableDepthWrite(true);
    }

    void
    WorldRenderer::bulletIndicator(const Player &player, const Indicator &indicator, Float32 xOfs, Float32 yOfs) const {
        std::string bulletCount = Format("{0}") << player.getAmmo();

        Float32 width = 0.15f * bulletCount.size();
        Float32 X = xOfs - width / 2;
        Float32 Y = yOfs;

        Uint8 alpha = Uint8(255 * indicator.getAlpha());

        globRenderer->enableDepthWrite(false);

        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcAlpha);
        globRenderer->quadXY(Vector(X, Y, 0.5f), Vector(width, 0.3f), Color::YELLOW.withAlpha(alpha));
        globRenderer->setBlendFunc(Renderer::BlendFunc::None);

        font.print(X, Y, 0.5f, Color::BLUE.withAlpha(alpha), bulletCount, 0.3f);

        globRenderer->enableDepthWrite(true);
    }

    void
    WorldRenderer::bonusIndicator(const Player &player, const Indicator &indicator, Float32 xOfs, Float32 yOfs) const {
        Uint8 alpha = Uint8(255 * indicator.getAlpha());
        Texture texture = player.getBonus().getTexture();
        Material material = Material::makeColoredTexture(texture, Color::WHITE.withAlpha(alpha));

        Float32 size = 0.3f;
        Float32 X = xOfs - size / 2;
        Float32 Y = yOfs;

        globRenderer->enableDepthWrite(false);
        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcAlpha);
        globRenderer->quadXY(Vector(X, Y, 0.5f), Vector(size, size), Vector(0.3f, 0.7f), Vector(0.4f, -0.4f), material);
        globRenderer->setBlendFunc(Renderer::BlendFunc::None);
        globRenderer->enableDepthWrite(true);
    }

    void WorldRenderer::playerStatus(const Player &player) const {
        const auto &indicators = player.getIndicators();

        if (player.isAlive()) {
            Rectangle rect = player.getCollisionRect();
            Float32 xOfs = rect.getCentre().x;
            Float32 yOfs = rect.right.y + 0.15f;

            const auto &reload = indicators.getReload();
            if (reload.isVisible()) {
                Float32 interval = player.getReloadInterval();
                Float32 value = 1.0f - std::min(interval, player.getReloadTime()) / interval;
                yOfs += playerIndicator(player, reload, Color::GREEN, value, xOfs, yOfs);
            }

            const auto &air = indicators.getAir();
            if (air.isVisible()) {
                yOfs += playerIndicator(player, air, Color::BLUE, player.getAir() / D6_MAX_AIR, xOfs, yOfs);
            }

            const auto &bonus = indicators.getBonus();
            if (bonus.isVisible()) {
                yOfs += playerIndicator(player, bonus, Color::MAGENTA,
                                        player.getBonusRemainingTime() / player.getBonusDuration(), xOfs, yOfs);
            }

            const auto &health = indicators.getHealth();
            if (health.isVisible()) {
                yOfs += playerIndicator(player, health, Color::RED, player.getLife() / D6_MAX_LIFE, xOfs, yOfs);
            }

            const auto &name = indicators.getName();
            const auto &bullets = indicators.getBullets();

            Float32 space = 0.08f;
            Float32 nameWidth = name.isVisible() ? space + player.getPerson().getName().size() * 0.15f : 0;
            std::string bulletString = Format("{0}") << player.getAmmo();
            Float32 bulletWidth = bullets.isVisible() ? space + bulletString.size() * 0.15f : 0;
            Float32 bonusWidth = bonus.isVisible() ? space + 0.3f : 0;
            Float32 totalWidth = nameWidth + bulletWidth + bonusWidth;
            Float32 halfWidth = totalWidth / 2;
            Float32 xStart = xOfs - halfWidth;

            if (name.isVisible()) {
                Float32 nameX = xStart + nameWidth / 2;
                playerName(player, name, nameX, yOfs);
            }

            if (bullets.isVisible()) {
                Float32 bulletX = xStart + nameWidth + bulletWidth / 2;
                bulletIndicator(player, bullets, bulletX, yOfs);
            }

            if (bonus.isVisible()) {
                Float32 bonusX = xStart + nameWidth + bulletWidth + bonusWidth / 2;
                bonusIndicator(player, bonus, bonusX, yOfs);
            }

            if (name.isVisible() || bullets.isVisible() || bonus.isVisible()) {
                yOfs += 0.4f;
            }

            roundKills(player, xOfs, yOfs);
        }
    }

    void WorldRenderer::roundKills(const Player &player, Float32 xOfs, Float32 yOfs) const {
        Float32 width = (2 * player.getRoundKills() - 1) * 0.1f;
        Float32 X = xOfs + 0.05f - width / 2;
        Float32 Y = yOfs + 0.1f;

        for (Int32 i = 0; i < player.getRoundKills(); i++, X += 0.2f) {
            globRenderer->point(Vector(X, Y, 0.5f), 5.0f, Color::BLUE);
        }
    }

    void WorldRenderer::invulRing(const Player &player) const {
        Vector playerCentre = player.getCentre();
        Float32 radius = player.getDimensions().length() / 2.0f;
        Int32 p = Int32(player.getBonusRemainingTime() * 30) % 360;

        for (Int32 uh = p; uh < 360 + p; uh += 15) {
            Int32 u = uh % 360;
            Vector pos = playerCentre + radius * Vector::direction(u);
            globRenderer->point(Vector(pos.x, pos.y, 0.5f), 2.0f, Color::RED);
        }
    }

    void WorldRenderer::invulRings(const std::vector<Player> &players) const {
        for (const Player &player : players) {
            if (player.isInvulnerable()) {
                invulRing(player);
            }
        }
    }

    void WorldRenderer::splitBox(const PlayerView &view) const {
        const auto &screen = video.getScreen();
        setView(view.getX() - 2, view.getY() - 2, view.getWidth() + 4, view.getHeight() + 4);
        globRenderer->quadXY(Vector::ZERO, Vector(screen.getClientWidth(), screen.getClientHeight()), Color::RED);
    }

    void WorldRenderer::screenCurtain(const Color &color) const {
        const auto &screen = video.getScreen();

        video.setMode(Video::Mode::Orthogonal);
        globRenderer->setBlendFunc(Renderer::BlendFunc::SrcAlpha);
        globRenderer->quadXY(Vector::ZERO, Vector(screen.getClientWidth(), screen.getClientHeight()), color);
        globRenderer->setBlendFunc(Renderer::BlendFunc::None);
        video.setMode(Video::Mode::Perspective);
    }

    void WorldRenderer::infoMessages() const {
        const InfoMessageQueue &messageQueue = game.getRound().getWorld().getMessageQueue();

        if (game.getSettings().getScreenMode() == ScreenMode::FullScreen) {
            messageQueue.renderAllMessages(game.getPlayers().front().getView(), 20, font);
        } else {
            for (const Player &player : game.getPlayers()) {
                messageQueue.renderPlayerMessages(player, font);
            }
        }
    }

    void WorldRenderer::shotCollisionBox(const ShotList &shotList) const {
        shotList.forEach([](const Shot &shot) -> bool {
            const auto &rect = shot.getCollisionRect();
            globRenderer->frame(Vector(rect.left.x, rect.left.y, 0.6f), rect.getSize(), 1.0f, Color::RED);
            return true;
        });
    }

    void WorldRenderer::view(const Player &player) const {
        const Camera &camera = player.getCamera();
        Matrix viewMatrix = Matrix::lookAt(camera.getPosition(), camera.getFront(), camera.getUp());
        globRenderer->setViewMatrix(viewMatrix);

        if (game.getSettings().isWireframe()) {
            globRenderer->enableWireframe(true);
        }

        const World &world = game.getRound().getWorld();
        walls(world.getLevelRenderData().getWalls());
        sprites(world.getLevelRenderData().getSprites());
        world.getElevatorList().render();
        world.getSpriteList().render();
        world.getBonusList().render();
        invulRings(game.getPlayers());
        water(world.getLevelRenderData().getWater());
        youAreHere();

        for (const Player &hpPlayer : game.getPlayers()) {
            playerStatus(hpPlayer);
        }
        //shotCollisionBox(world.getShotList());

        world.getExplosionList().render();

        if (game.getSettings().isWireframe()) {
            globRenderer->enableWireframe(false);
        }
    }

    Color WorldRenderer::getGameOverOverlay() const {
        Float32 overlay = game.getRound().getRemainingGameOverWait() / D6_GAME_OVER_WAIT;
        return Color(128, 0, 0, Uint8(200 - 200 * overlay));
    }

    void WorldRenderer::fullScreen() const {
        const Player &player = game.getPlayers().front();
        setView(player.getView());
        background(game.getResources().getBcgTextures().at(game.getRound().getWorld().getBackground()));
        video.setMode(Video::Mode::Perspective);
        view(player);

        if (game.getRound().hasWinner()) {
            Color overlayColor = getGameOverOverlay();
            screenCurtain(overlayColor);
        }
    }

    void WorldRenderer::splitScreen() const {
        for (const Player &player : game.getPlayers()) {
            video.setMode(Video::Mode::Orthogonal);
            splitBox(player.getView());

            setView(player.getView());
            background(game.getResources().getBcgTextures().at(game.getRound().getWorld().getBackground()));

            video.setMode(Video::Mode::Perspective);
            view(player);

            if (!player.isAlive()) {
                screenCurtain(Color(255, 0, 0, 128));
            }
        }
    }

    void WorldRenderer::render() const {
        const GameSettings &settings = game.getSettings();

        globRenderer->clearBuffers();

        if (settings.getScreenMode() == ScreenMode::FullScreen) {
            fullScreen();
        } else {
            splitScreen();
        }

        video.setMode(Video::Mode::Orthogonal);
        setView(0, 0, video.getScreen().getClientWidth(), video.getScreen().getClientHeight());

        infoMessages();

        if (settings.isShowFps()) {
            fpsCounter();
        }

        if (settings.isShowRanking() && settings.getScreenMode() == ScreenMode::FullScreen) {
            playerRankings();
        }

        if (settings.isRoundLimit()) {
            roundsPlayed();
        }

        if (game.isDisplayingScoreTab()) {
            roundOverSummary();
        }
        if (game.getRound().hasWinner()) {
            if (game.isOver()) {
                gameOverSummary();
            } else {
                roundOverSummary();
            }
        }
    }
}
