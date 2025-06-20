#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/FLAlertLayer.hpp>
#include <Geode/modify/SongInfoLayer.hpp>
#include "MenuLoopLayer.hpp"
#include "MusicManagerUtils.hpp"
#include <filesystem>
#include <Windows.h>

using namespace geode::prelude;

extern std::string gCurrentCustomSong;

$execute{
    srand(static_cast<unsigned>(time(nullptr)));

    std::string menuLoopSongsDir = fmt::format("{}\\MenuLoop Songs", geode::dirs::getGameDir().string().data());
    if (!std::filesystem::exists(menuLoopSongsDir)) {
        std::filesystem::create_directory(menuLoopSongsDir);
        geode::log::info("Created a directory: {}", menuLoopSongsDir);
    }
 else {
  geode::log::info("This directory already exists: {}", menuLoopSongsDir);
}
}

namespace {
    bool getRedirectedSongAvailable(const gd::string& song) {
        std::string path = fmt::format("{}\\MenuLoop Songs\\{}", geode::dirs::getGameDir().string().data(), song.c_str());
        return std::filesystem::exists(path) &&
            (std::filesystem::is_directory(path) || std::filesystem::is_regular_file(path)) &&
            !std::filesystem::is_empty(path);
    }

    gd::string getRandomSongFromPath(const gd::string& song) {
        std::string path = fmt::format("{}\\MenuLoop Songs\\{}", geode::dirs::getGameDir().string().data(), song.c_str());
        std::vector<std::string> files;

        if (!std::filesystem::is_directory(path))
            return gd::string(fmt::format("..\\MenuLoop Songs\\{}", song.c_str()).data());

        for (const auto& entry : std::filesystem::directory_iterator(path))
            if (!std::filesystem::is_directory(entry))
                files.push_back(entry.path().filename().string());

        return gd::string{ fmt::format("..\\MenuLoop Songs\\{}\\{}", song.c_str(), files.at(rand() % files.size())).data() };
    }

    std::string gLastPlayedTrack;
    std::string gLastRedirectedTrack;
    std::string gLastPlayedEffect;
    std::string gLastRedirectedEffect;

    bool isMenuLoopMusic(const std::string& musicPath) {
        return musicPath.find("menuLoop.mp3") != std::string::npos;
    }
}

class $modify(FMODAudioEngine) {
    void loadMusic(gd::string p0, float p1, float p2, float p3, bool p4, int p5, int p6) {
        if (gCurrentCustomSong.empty() &&
            (gLastPlayedTrack.empty() ||
                (gLastPlayedTrack != std::string{ p0 } &&
                    std::string{ p0 }.find("MenuLoop Songs\\") == std::string::npos))) {
            gLastPlayedTrack = p0;
        }
        return this->FMODAudioEngine::loadMusic(p0, p1, p2, p3, p4, p5, p6);
    }

    void playMusic(gd::string p0, bool p1, float p2, int p3) {
        if (isMenuLoopMusic(std::string{ p0 })) {
            if (!gCurrentCustomSong.empty()) {
                p0 = gCurrentCustomSong;
            }
        }

        if (!gLastPlayedTrack.empty() && gLastPlayedTrack == std::string{ p0 }) return;
        gLastPlayedTrack = p0.data();

        if (getRedirectedSongAvailable(p0)) {
            gd::string newString = getRandomSongFromPath(p0);
            gLastRedirectedTrack = newString;
            return this->FMODAudioEngine::playMusic(newString, p1, p2, p3);
        }

        return this->FMODAudioEngine::playMusic(p0, true, p2, p3);
    }
};

class $modify(MyMenuLayer, MenuLayer) {
    struct Fields {
        CCSprite* customBtnSprite;
        CCMenuItemSpriteExtra* customBtn;
    };

    bool init() {
        if (!MenuLayer::init()) return false;

        auto rightSideMenu = getChildByID("right-side-menu");
        if (rightSideMenu) {
            m_fields->customBtnSprite = CCSprite::create("button_logo.png"_spr);
            m_fields->customBtnSprite->setScale(0.40);
            m_fields->customBtn = CCMenuItemSpriteExtra::create(m_fields->customBtnSprite, this, menu_selector(MyMenuLayer::onCustomButton));

            rightSideMenu->addChild(m_fields->customBtn, -1);
            rightSideMenu->updateLayout(false);
        }
        else {
            geode::log::error("Right-side menu not found");
        }
        return true;
    }

    void onCustomButton(CCObject*) {
        geode::log::info("Button clicked");
        MenuLoopLayer::create()->show();
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void onQuit() {
        PlayLayer::onQuit();
        geode::log::info("PlayLayer onQuit was called, resuming custom song if available...");
        resumeLastCustomSong();
    }
};

class $modify(MyEditorPauseLayer, EditorPauseLayer) {
    void onExitEditor(cocos2d::CCObject * sender) {
        EditorPauseLayer::onExitEditor(sender);
        geode::log::info("EditorPauseLayer onExitEditor was called, resuming custom song if available...");
        resumeLastCustomSong();
    }

    void onExitNoSave(cocos2d::CCObject * sender) {
        EditorPauseLayer::onExitNoSave(sender);
        geode::log::info("EditorPauseLayer onExitNoSave was called, setting FLAlertLayer hook...");

        // hooking the FLAlertLayer
        class $modify(MyFLAlertLayer, FLAlertLayer) {
            void onBtn2(cocos2d::CCObject * sender) {
                // Checking if sender is CCMenuItemSpriteExtra
                auto menuItem = dynamic_cast<CCMenuItemSpriteExtra*>(sender);
                if (menuItem) {
                    // Get parent of menuItem
                    auto parent = menuItem->getParent();
                    if (parent) {
                        // Check if parent contains label with "Exit"
                        auto children = parent->getChildren();
                        for (int i = 0; i < children->count(); ++i) {
                            auto child = children->objectAtIndex(i);
                            auto label = dynamic_cast<cocos2d::CCLabelBMFont*>(child);
                            if (label && std::string(label->getString()) == "Exit") {
                                geode::log::info("FLAlertLayer has confirmed exit button, resuming custom song if available");
                                resumeLastCustomSong();
                                break;
                            }
                        }
                    }
                }
                // Calling original function
                this->FLAlertLayer::onBtn2(sender);
            }
        };
    }

    void onSaveAndExit(cocos2d::CCObject * sender) {
        EditorPauseLayer::onSaveAndExit(sender);
        geode::log::info("EditorPauseLayer onSaveAndExit was called, resuming custom song if available");
        resumeLastCustomSong();
    }
};

class $modify(MySongInfoLayer, SongInfoLayer) {
    bool init(gd::string p0, gd::string p1, gd::string p2, gd::string p3, gd::string p4, gd::string p5, int p6, gd::string p7, int p8) {
        if (!SongInfoLayer::init(p0, p1, p2, p3, p4, p5, p6, p7, p8)) return false;

        // Get main layer
        auto mainLayer = this->getChildren()->objectAtIndex(0);

        // Check if main layer is CCLayer
        if (auto mainCCLayer = dynamic_cast<cocos2d::CCLayer*>(mainLayer)) {
            // Iterate all children of the layer
            for (int i = 0; i < mainCCLayer->getChildrenCount(); ++i) {
                auto child = mainCCLayer->getChildren()->objectAtIndex(i);

                // If the child is CCMenu
                if (auto menu = dynamic_cast<cocos2d::CCMenu*>(child)) {
                    // Iterate all children of the menu
                    for (int j = 0; j < menu->getChildrenCount(); ++j) {
                        auto menuItem = menu->getChildren()->objectAtIndex(j);

                        // If menu item is CCMenuItemToggler, remove that
                        if (dynamic_cast<CCMenuItemToggler*>(menuItem)) {
                            menu->removeChild(dynamic_cast<cocos2d::CCNode*>(menuItem), true);
                        }
                    }
                }

                // If the child is CCLabelBMFont
                if (auto label = dynamic_cast<cocos2d::CCLabelBMFont*>(child)) {
                    std::string labelText = label->getString();
                    // Remove the Text/Sprite
                    if (labelText == "Menu") {
                        mainCCLayer->removeChild(label, true);
                    }
                }
            }
        }

        return true;
    }
};
