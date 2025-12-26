#pragma once

#include <string>
#include <vector>
#include <functional>
#include "npc_config.h"

struct NPCMessage {
    std::string role;
    std::string content;
};

class NPCChat {
public:
    // Simple constructor (backward compatible)
    NPCChat(const std::string& apiKey, const std::string& npcName = "Village Guard");

    // Config-based constructor
    NPCChat(const std::string& apiKey, const NPCConfig& config);

    std::string chat(const std::string& playerMessage);
    void setPersonality(const std::string& personality);
    void setConfig(const NPCConfig& config);
    void clearHistory();

    const std::string& getName() const { return m_npcName; }

private:
    std::string m_apiKey;
    std::string m_npcName;
    std::string m_systemPrompt;
    std::vector<NPCMessage> m_history;

    std::string makeRequest(const std::string& jsonPayload);
    std::string buildRequestJson();
};
