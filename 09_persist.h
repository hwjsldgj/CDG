#ifndef PERSIST_H
#define PERSIST_H

#include <string>

void LoadHighScore();
void SaveHighScore();
void SaveReplayFile(); 
void LoadReplayFile(const std::string& filename);

#endif