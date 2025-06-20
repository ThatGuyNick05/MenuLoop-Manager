#pragma once

#include <Geode/DefaultInclude.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/GJSearchObject.hpp>
#include <cocos2d.h>
#include <Geode/utils/file.hpp>
#include "MusicManagerUtils.hpp"

using namespace geode::prelude;

class MenuLoopLayer : public geode::Popup<> {
public:
    static constexpr float POPUP_WIDTH = 464.f;
    static constexpr float POPUP_HEIGHT = 275.f;

    static MenuLoopLayer* create();

private:
    ScrollLayer* m_scrollView;

    bool setup() override;
    void refreshSongList();
    void onAddSong(CCObject* sender);
    void onOpenFolder(CCObject* sender);
    void onSongSelected(CCObject* sender);
};

MenuLoopLayer* MenuLoopLayer::create() {
    auto ret = new MenuLoopLayer;
    if (ret && ret->initAnchored(POPUP_WIDTH, POPUP_HEIGHT)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool MenuLoopLayer::setup() {
    auto mainLayer = static_cast<CCLayer*>(this->getChildren()->objectAtIndex(0));

    // Get title label
    auto titleLabel = CCLabelBMFont::create("Menu Loop Selector", "goldFont.fnt");
    titleLabel->setPosition(230.0f, 244.0f);
    titleLabel->setContentSize({ 260.5f, 26.0f });
    mainLayer->addChild(titleLabel);

    // Get the background of scroll view
    auto namesBG = CCScale9Sprite::create("square02_001-uhd.png");
    namesBG->setContentSize(CCSize(395, 169));
    namesBG->setPosition(230.0f, 138.5f);
    namesBG->setOpacity(80);
    mainLayer->addChild(namesBG);

    // Make the scroll view
    m_scrollView = ScrollLayer::create({ 394, 163 }, true, true);
    m_scrollView->setPosition({ 33.5f, 57.5f });
    m_scrollView->m_contentLayer->setLayout(
        ColumnLayout::create()
        ->setGap(15.f)
        ->setAxisReverse(true)
        ->setAxisAlignment(AxisAlignment::End)
        ->setAutoGrowAxis(212)
        ->setAutoScale(true)
    );
    m_scrollView->setID("song-list-scroll-view");
    mainLayer->addChild(m_scrollView, 1);

    // the position and scale of content
    auto contentLayer = m_scrollView->m_contentLayer;
    contentLayer->setPosition({ 197.0f, -25.815f });
    contentLayer->setScale(0.8f);
    contentLayer->updateLayout();

    // adding the buttons
    auto addBtnSprite = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
    auto addBtn = CCMenuItemSpriteExtra::create(addBtnSprite, this, menu_selector(MenuLoopLayer::onAddSong));
    addBtn->setPosition(0.0f, -17.0f);

    auto folderBtnSprite = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
    auto folderBtn = CCMenuItemSpriteExtra::create(folderBtnSprite, this, menu_selector(MenuLoopLayer::onOpenFolder));
    folderBtn->setPosition(-200.0f, 0.0f);

    auto menu = CCMenu::create(addBtn, folderBtn, nullptr);
    menu->setPosition(POPUP_WIDTH / 2, 30);
    mainLayer->addChild(menu);

    // Populate the list of songs
    refreshSongList();

    return true;
}

void MenuLoopLayer::refreshSongList() {
    geode::log::info("Refreshing the song list...");

    if (!m_scrollView) {
        geode::log::error("no scroll view found!");
        return;
    }

    m_scrollView->m_contentLayer->removeAllChildren();
    m_scrollView->m_contentLayer->updateLayout();

    std::string menuLoopSongsDir = fmt::format("{}\\MenuLoop Songs", geode::dirs::getGameDir().string().data());
    geode::log::info("Reading directory: {}", menuLoopSongsDir);

    for (const auto& entry : std::filesystem::directory_iterator(menuLoopSongsDir)) {
        geode::log::info("Found file: {}", entry.path().string());
        if (entry.path().extension() == ".mp3") {
            auto menu = CCMenu::create();
            menu->setPositionY(menu->getPositionY() + 9);
            menu->setContentSize({ 0, 20 });

            auto name = CCLabelBMFont::create(entry.path().filename().string().c_str(), "bigFont.fnt");
            name->setScale(0.65);
            name->setAnchorPoint({ 0, 0.5 });
            auto nameHolder = CCMenuItemSpriteExtra::create(name, this, menu_selector(MenuLoopLayer::onSongSelected));
            nameHolder->setUserObject(CCString::create(entry.path().string()));

            menu->addChild(nameHolder);
            m_scrollView->m_contentLayer->addChild(menu);
        }
    }

    m_scrollView->m_contentLayer->updateLayout();
}

void MenuLoopLayer::onAddSong(CCObject*) {
    auto filePath = openFileExplorer();
    if (!filePath.empty()) {
        geode::log::info("Selected file: {}", filePath);
        copyFileToMenuLoopSongs(filePath);
        refreshSongList();
    }
}

void MenuLoopLayer::onOpenFolder(CCObject*) {
    geode::log::info("Clicked the open folder button...");
    std::string folderPath = fmt::format("{}\\MenuLoop Songs", geode::dirs::getGameDir().string().data());
    ShellExecuteA(NULL, "open", folderPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
}

void MenuLoopLayer::onSongSelected(CCObject* sender) {
    auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender);
    auto songPath = static_cast<CCString*>(menuItem->getUserObject())->getCString();
    geode::log::info("Selected song: {}", songPath);
    playSpecificSong(songPath);
}
