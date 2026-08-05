#include "history.h"
#include "game_state.h"

void SaveHistory() {
    // 占位
}

void LoadHistory() {
    // 占位
}

void StartRecording() {
    g_state.isRecording = true;
    g_state.replayFrames.clear();
}

void StopRecording() {
    g_state.isRecording = false;
}

void Playback() {
    g_state.isPlaying = true;
    g_state.playbackIndex = 0;
}

void ExternalSetParam(const std::string& key, const std::string& value) {
    // 占位
}

std::string ExternalGetStateJSON() {
    return "{}";
}