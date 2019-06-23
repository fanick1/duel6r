//
// Created by fanick on 8.3.19.
//

#include "MainMenu.h"
#include "../Menu.h"

void Duel6::MainMenu::keyEvent(const Duel6::KeyPressEvent &event) {
    gui.keyEvent(event);

    if (event.getCode() == SDLK_ESCAPE) {
        close();
    }
}

void Duel6::MainMenu::textInputEvent(const Duel6::TextInputEvent &event) {
    gui.textInputEvent(event);
}

void Duel6::MainMenu::mouseButtonEvent(const Duel6::MouseButtonEvent &event) {
    gui.mouseButtonEvent(event);
}

void Duel6::MainMenu::mouseMotionEvent(const Duel6::MouseMotionEvent &event) {
    gui.mouseMotionEvent(event);
}

void Duel6::MainMenu::mouseWheelEvent(const Duel6::MouseWheelEvent &event) {
    gui.mouseWheelEvent(event);
}

void Duel6::MainMenu::joyDeviceAddedEvent(const Duel6::JoyDeviceAddedEvent &event) {

}

void Duel6::MainMenu::joyDeviceRemovedEvent(const Duel6::JoyDeviceRemovedEvent &event) {

}

void Duel6::MainMenu::update(Duel6::Float32 elapsedTime) {
    gui.update(elapsedTime);

    if(hasReplay){
        if(backgroundReplay->isFinished()){
            setupReplay();
        }
        replayGame.update(elapsedTime);
    }
}
void Duel6::MainMenu::setupReplay() {

    if(!hasReplay){
        return;
    }
    backgroundReplay->rewind();

    Int32 gameMode;
        if(!hasReplay){
            return;
        }
        std::vector<Game::PlayerDefinition> playerDefinitions;
        backgroundReplay->configureGameSettings(replayGame.getSettings(), gameMode);
        demoPersons = std::make_unique<PersonList>();

        for (const ReplayPlayerProfile &demoPlayerProfile : backgroundReplay->players) {
            demoPersons->add(Person(demoPlayerProfile.getName(), nullptr));
        }

        for (const ReplayPlayerProfile &demoPlayerProfile : backgroundReplay->players) {
            Person &person = demoPersons->getByName(
                    demoPlayerProfile.getName());// demoPersons->get(demoPersons->getLength()-1);

            PlayerSkinColors colors = demoPlayerProfile.getSkinColors();
            playerDefinitions.push_back(Game::PlayerDefinition(person, colors, defaultPlayerSounds, dummyControls));
        }
        GameMode &selectedMode = *gameModes[gameMode];
        selectedMode.initializePlayers(playerDefinitions);
        std::vector<std::string> levels;
        ScreenMode screenMode = ScreenMode::FullScreen;

        replayGame.start(backgroundReplay.get(), playerDefinitions, levels, screenMode, 13, selectedMode);
}
void Duel6::MainMenu::render() const {
    Int32 trX = (video.getScreen().getClientWidth() - 800) / 2;
    Int32 trY = (video.getScreen().getClientHeight() - 700) / 2;

    if(hasReplay) {
        replayGame.render();
    }
   // menu.render();
   gui.draw(font);

    //globRenderer->setViewMatrix(Matrix::translate(Float32(trX), -Float32(trY), 0));

    font.print(687, video.getScreen().getClientHeight() - 20, Color::WHITE,
               Format("{0} {1}") << "version" << APP_VERSION);
}

void Duel6::MainMenu::beforeStart(Duel6::Context *prevContext) {

}

void Duel6::MainMenu::beforeClose(Duel6::Context *nextContext) {

}
