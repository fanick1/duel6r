//
// Created by fanick on 8.3.19.
//

#ifndef DUEL6R_MAINMENU_H
#define DUEL6R_MAINMENU_H

#include "../Context.h"
#include "../gui/Desktop.h"
#include "../gui/Button.h"
#include "../Game.h"
#include "../serialisation/FileBinaryStream.h"
#include "../gamemodes/DeathMatch.h"
#include "../gamemodes/Predator.h"
#include "../gamemodes/TeamDeathMatch.h"
#include "../PersonList.h"
namespace Duel6 {
    class MainMenu : public Context {
    private:
        AppService &appService;
        Font &font;
        Video &video;
        Sound &sound;

        Game replayGame;
        std::unique_ptr<Replay> backgroundReplay;
        Gui::Desktop gui;
        PlayerSounds defaultPlayerSounds;
        std::unique_ptr<PersonList> demoPersons;
        PlayerControls dummyControls{"", new DummyControl(), new DummyControl(), new DummyControl(), new DummyControl(),
                                     new DummyControl(), new DummyControl(), new DummyControl()};
        std::vector<std::unique_ptr<GameMode>> gameModes;
        bool hasReplay = false;
        Menu & menu;
    public:
        MainMenu(AppService &appService, GameResources &gameResources, GameSettings &gameSettings, Menu & menu) :
                appService(appService), font(appService.getFont()), video(appService.getVideo()),
                sound(appService.getSound()),
                replayGame(appService, gameResources, gameSettings),
                defaultPlayerSounds(PlayerSounds::makeDefault(sound)), menu(menu) {
            initializeGameModes();
        }

        virtual ~MainMenu() = default;
        void initializeGameModes() {
            gameModes.push_back(std::make_unique<DeathMatch>());
            gameModes.push_back(std::make_unique<Predator>());
            gameModes.push_back(std::make_unique<TeamDeathMatch>(2, false));
            gameModes.push_back(std::make_unique<TeamDeathMatch>(2, true));
            gameModes.push_back(std::make_unique<TeamDeathMatch>(3, false));
            gameModes.push_back(std::make_unique<TeamDeathMatch>(3, true));
            gameModes.push_back(std::make_unique<TeamDeathMatch>(4, false));
            gameModes.push_back(std::make_unique<TeamDeathMatch>(4, true));
        }
        void loadReplay() {
            hasReplay = false;
            fbinarystream bs("last.dem", std::ios_base::in);
            if (bs.is_open()) {
                this->backgroundReplay = std::make_unique<Replay>();
                hasReplay = bs >> *backgroundReplay;
                bs.close();
            }

            if(!hasReplay){
                return;
            }
            backgroundReplay->rewind();
        }

        void initialize() {

            gui.screenSize(video.getScreen().getClientWidth(), video.getScreen().getClientHeight(),
                           /*(video.getScreen().getClientWidth() - 800) / 2*/ 0, /*(video.getScreen().getClientHeight() - 700) / 2*/ 0);

            auto quitButton = new Gui::Button(gui);
            quitButton->setPosition(10, 50, 80, 25);
            quitButton->setCaption("Quit");
            quitButton->onClick([this](Gui::Button &) {
                close();
            });

            auto playButton = new Gui::Button(gui);
            playButton->setPosition(10, 100, 80, 25);
            playButton->setCaption("Play");
            playButton->onClick([this](Gui::Button &) {
                play();
            });
//            scoreListBox = new Gui::ListBox(gui, true);
//            scoreListBox->setPosition(10, 199, 97, 12, 16);
//
//            personListBox = new Gui::ListBox(gui, true);
//            personListBox->setPosition(10, 539, 20, 15, 18);
//            personListBox->onDoubleClick([this](Int32 index, const std::string &item) {
//                addPlayer(index);
//            });
//
//            playerListBox = new Gui::ListBox(gui, false);
//            playerListBox->setPosition(200, 541, 15, D6_MAX_PLAYERS, 18);
//            playerListBox->onDoubleClick([this](Int32 index, const std::string &item) {
//                removePlayer(index);
//            });
//
//            levelListBox = new Gui::ListBox(gui, true);
//            levelListBox->setPosition(654, 475, 17, 15, 16);
//
//            screenModeListBox = new Gui::ListBox(gui, false);
//            screenModeListBox->setPosition(654, 541, 19, 2, 16);
//            screenModeListBox->addItem("Fullscreen");
//            screenModeListBox->addItem("Split screen");
//            screenModeListBox->selectItem(0);
//
//            loadPersonProfiles(D6_FILE_PROFILES);
//            loadPersonData(D6_FILE_PHIST);
//
//            auto addPlayerButton = new Gui::Button(gui);
//            addPlayerButton->setPosition(200, 253, 80, 25);
//            addPlayerButton->setCaption(">>");
//            addPlayerButton->onClick([this](Gui::Button &) {
//                addPlayer(personListBox->selectedIndex());
//            });
//
//            auto removePlayerButton = new Gui::Button(gui);
//            removePlayerButton->setPosition(105, 253, 85, 25);
//            removePlayerButton->setCaption("<<");
//            removePlayerButton->onClick([this](Gui::Button &) {
//                removePlayer(playerListBox->selectedIndex());
//            });
//
//            auto removePersonButton = new Gui::Button(gui);
//            removePersonButton->setPosition(10, 253, 90, 25);
//            removePersonButton->setCaption("Remove");
//            removePersonButton->onClick([this](Gui::Button &) {
//                deletePerson();
//                rebuildTable();
//            });
//
//            auto addPersonButton = new Gui::Button(gui);
//            addPersonButton->setPosition(284, 253, 80, 25);
//            addPersonButton->setCaption("Add");
//            addPersonButton->onClick([this](Gui::Button &) {
//                addPerson();
//            });
//            auto replayButton = new Gui::Button(gui);
//            replayButton->setPosition(815, 0, 100, 50);
//            replayButton->setCaption("RePlay ");
//            replayButton->onClick([this](Gui::Button &) {
//                startReplay();
//            });
//            auto playButton = new Gui::Button(gui);
//            playButton->setPosition(350, 0, 150, 50);
//            playButton->setCaption("Play (F1)");
//            playButton->onClick([this](Gui::Button &) {
//                play();
//            });
//
//            auto clearButton = new Gui::Button(gui);
//            clearButton->setPosition(505, 0, 150, 50);
//            clearButton->setCaption("Clear (F3)");
//            clearButton->onClick([this](Gui::Button &) {
//                if (deleteQuestion()) {
//                    cleanPersonData();
//                }
//            });
//
//            auto quitButton = new Gui::Button(gui);
//            quitButton->setPosition(660, 0, 150, 50);
//            quitButton->setCaption("Quit (ESC)");
//            quitButton->onClick([this](Gui::Button &) {
//                close();
//            });
//
//            auto scoreLabel = new Gui::Label(gui);
//            scoreLabel->setPosition(10, 219, 800, 18);
//            scoreLabel->setCaption(
//                    "    Name   | Games | Wins | Shots | Acc. | Kills | Assists | Pen | PTS | Alive | Damage  | Time");
//
//            auto levelLabel = new Gui::Label(gui);
//            levelLabel->setPosition(654, 494, 155, 18);
//            levelLabel->setCaption("Level");
//
//            auto screenModeLabel = new Gui::Label(gui);
//            screenModeLabel->setPosition(654, 560, 155, 18);
//            screenModeLabel->setCaption("Screen mode");
//
//            auto personsLabel = new Gui::Label(gui);
//            personsLabel->setPosition(10, 560, 181, 18);
//            personsLabel->setCaption("Persons");
//
//            playersLabel = new Gui::Label(gui);
//            playersLabel->setPosition(200, 560, 105, 18);
//
//            updatePlayerCount();
//
//            Gui::Button *shuffleButton = new Gui::Button(gui);
//            shuffleButton->setCaption("S");
//            shuffleButton->setPosition(307, 560, 17, 17);
//            shuffleButton->onClick([this](Gui::Button &) {
//                shufflePlayers();
//            });
//
//            auto controllerLabel = new Gui::Label(gui);
//            controllerLabel->setPosition(330, 560, 192, 18);
//            controllerLabel->setCaption("Controller");
//
//            textbox = new Gui::Textbox(gui);
//            textbox->setPosition(370, 252, 14, 10, D6_ALL_CHR);
//
//            // Player controls
//            for (Size i = 0; i < D6_MAX_PLAYERS; i++) {
//                controlSwitch[i] = new Gui::Spinner(gui);
//                controlSwitch[i]->setPosition(330, 539 - Int32(i) * 18, 192, 0);
//
//                Gui::Button *button = new Gui::Button(gui);
//                button->setCaption("D");
//                button->setPosition(526, 537 - Int32(i) * 18, 17, 17);
//                button->onClick([this, i](Gui::Button &) {
//                    detectControls(i);
//                });
//            }
//
//            // Button to detect all user's controllers in a batch
//            Gui::Button *button = new Gui::Button(gui);
//            button->setCaption("D");
//            button->setPosition(526, 558, 17, 17);
//            button->onClick([this](Gui::Button &) {
//                joyRescan();
//                Size curPlayersCount = playerListBox->size();
//                for (Size j = 0; j < curPlayersCount; j++) {
//                    detectControls(j);
//                }
//            });
//
//            initializeGameModes();
//            gameModeSwitch = new Gui::Spinner(gui);
//            for (auto &gameMode : gameModes) {
//                gameModeSwitch->addItem(gameMode->getName());
//            }
//            gameModeSwitch->setPosition(10, 0, 330, 20);
//            gameModeSwitch->onToggled([this](Int32 selectedIndex) {
//                if (selectedIndex < 2) {
//                    playerListBox->onColorize(Gui::ListBox::defaultColorize);
//                } else {
//                    Int32 teamCount = 1 + selectedIndex / 2;
//                    playerListBox->onColorize([teamCount](Int32 index, const std::string &label) {
//                        return Gui::ListBox::ItemColor{Color::BLACK, TEAMS[index % teamCount].color};
//                    });
//                }
//            });
//
//            globalAssistanceCheckBox = new Gui::CheckBox(gui);
//            globalAssistanceCheckBox->setLabel("Assistance");
//            globalAssistanceCheckBox->setPosition(11, -21, 130, 20);
//            quickLiquidCheckBox = new Gui::CheckBox(gui);
//            quickLiquidCheckBox->setLabel("Quick Liquid");
//            quickLiquidCheckBox->setPosition(151, -21, 150, 20);
//
//            backgroundCount = File::countFiles(D6_TEXTURE_BCG_PATH);
//            levelList.initialize(D6_FILE_LEVEL, D6_LEVEL_EXTENSION);
//
//            levelListBox->addItem("Random");
//            for (Size i = 0; i < levelList.getLength(); i++) {
//                levelListBox->addItem(levelList.getFileName(i).substr(0, levelList.getFileName(i).rfind(".")));
//            }
//            levelListBox->selectItem(0);

            menuTrack = sound.loadModule("sound/undead.xm");
            menuTrack.play(false);

            loadReplay();
            setupReplay();
        }

        Sound::Track menuTrack;


        void keyEvent(const KeyPressEvent &event) override;

        void textInputEvent(const TextInputEvent &event) override;

        void mouseButtonEvent(const MouseButtonEvent &event) override;

        void mouseMotionEvent(const MouseMotionEvent &event) override;

        void mouseWheelEvent(const MouseWheelEvent &event) override;

        void joyDeviceAddedEvent(const JoyDeviceAddedEvent & event) override;

        void joyDeviceRemovedEvent(const JoyDeviceRemovedEvent & event) override;

        void update(Float32 elapsedTime) override;

        void render() const override;

    private:
        void beforeStart(Context *prevContext) override;

        void beforeClose(Context *nextContext) override;

        void play() {
            Duel6::Context::push((Context &) menu);
        }


        void setupReplay();
    };
}

#endif //DUEL6R_MAINMENU_H
