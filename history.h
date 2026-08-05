#ifndef HISTORY_H
#define HISTORY_H

#include <string>

void SaveHistory();
void LoadHistory();
void StartRecording();
void StopRecording();
void Playback();

void ExternalSetParam(const std::string& key, const std::string& value);
std::string ExternalGetStateJSON();

#endif