#include "MusicManagerUtils.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Windows.h>
#include <filesystem>
#include <string>

std::string gCurrentCustomSong;

std::string openFileExplorer() {
    char filePath[MAX_PATH] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = sizeof(filePath);
    ofn.lpstrFilter = "MP3 Files\0*.mp3\0";
    ofn.lpstrTitle = "Select an MP3 file";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        return std::string(filePath);
    }
    return "";
}

void copyFileToMenuLoopSongs(const std::string& filePath) {
    std::string destinationPath = fmt::format("{}\\MenuLoop Songs\\{}", geode::dirs::getGameDir().string().data(), std::filesystem::path(filePath).filename().string());
    std::filesystem::copy(filePath, destinationPath, std::filesystem::copy_options::overwrite_existing);
    geode::log::info("Copied file to: {}", destinationPath);
}

void playSpecificSong(const std::string& songPath) {
    gCurrentCustomSong = songPath;
    FMODAudioEngine::sharedEngine()->playMusic(songPath.c_str(), false, 0.0f, 0);
    geode::log::info("Playing song: {}", songPath);
}

void resumeLastCustomSong() {
    if (!gCurrentCustomSong.empty()) {
        geode::log::info("Resuming custom song: {}", gCurrentCustomSong);
        // Force music restart by setting as empty string then back to current song
        std::string currentSong = gCurrentCustomSong;
        gCurrentCustomSong = "";
        FMODAudioEngine::sharedEngine()->playMusic("menuLoop.mp3", false, 0.0f, 0);
        gCurrentCustomSong = currentSong;
        FMODAudioEngine::sharedEngine()->playMusic(gCurrentCustomSong.c_str(), false, 1.0f, 0);
    }
    else {
        geode::log::info("No custom song to resume, playing default menuLoop.mp3");
        FMODAudioEngine::sharedEngine()->playMusic("menuLoop.mp3", false, 0.0f, 0);
    }
}