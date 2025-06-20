#pragma once

#include <string>

extern std::string gCurrentCustomSong;

std::string openFileExplorer();
void copyFileToMenuLoopSongs(const std::string& filePath);
void playSpecificSong(const std::string& songPath);
void resumeLastCustomSong();