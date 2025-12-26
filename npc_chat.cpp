#include "npc_chat.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <cstdlib>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Escape special characters for JSON string
static std::string escapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 2);
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

NPCChat::NPCChat(const std::string& apiKey, const std::string& npcName)
    : m_apiKey(apiKey), m_npcName(npcName) {

    m_systemPrompt =
        "You are " + m_npcName + ", an NPC in a fantasy MMO game.\n\n"
        "RULES:\n"
        "- Respond in 1-2 short sentences MAX\n"
        "- Stay in character at all times\n"
        "- Use simple, direct speech\n"
        "- Never break the fourth wall\n"
        "- Never mention being an AI\n"
        "- React naturally to player questions about quests, directions, or lore\n"
        "- ONLY output spoken dialogue - NO actions, NO asterisks, NO stage directions";
}

NPCChat::NPCChat(const std::string& apiKey, const NPCConfig& config)
    : m_apiKey(apiKey), m_npcName(config.name) {
    m_systemPrompt = config.buildPrompt();
}

void NPCChat::setConfig(const NPCConfig& config) {
    m_npcName = config.name;
    m_systemPrompt = config.buildPrompt();
}

void NPCChat::setPersonality(const std::string& personality) {
    m_systemPrompt = personality;
}

void NPCChat::clearHistory() {
    m_history.clear();
}

std::string NPCChat::chat(const std::string& playerMessage) {
    m_history.push_back({"user", playerMessage});

    std::string json = buildRequestJson();
    std::string response = makeRequest(json);

    // Parse response - simple extraction
    size_t contentStart = response.find("\"content\":\"");
    if (contentStart == std::string::npos) {
        contentStart = response.find("\"content\": \"");
        if (contentStart != std::string::npos) contentStart += 12;
    } else {
        contentStart += 11;
    }

    if (contentStart == std::string::npos) {
        std::cerr << "Failed to parse response: " << response << std::endl;
        return "Hmm, I didn't quite catch that.";
    }

    size_t contentEnd = response.find("\"", contentStart);
    std::string npcResponse = response.substr(contentStart, contentEnd - contentStart);

    // Unescape basic sequences
    size_t pos = 0;
    while ((pos = npcResponse.find("\\n", pos)) != std::string::npos) {
        npcResponse.replace(pos, 2, "\n");
        pos++;
    }

    m_history.push_back({"assistant", npcResponse});
    return npcResponse;
}

std::string NPCChat::buildRequestJson() {
    std::ostringstream json;
    json << "{";
    json << "\"model\":\"anthropic/claude-3-haiku\",";
    json << "\"max_tokens\":100,";
    json << "\"messages\":[";
    json << "{\"role\":\"system\",\"content\":\"" << escapeJson(m_systemPrompt) << "\"},";

    for (size_t i = 0; i < m_history.size(); ++i) {
        json << "{\"role\":\"" << m_history[i].role << "\",";
        json << "\"content\":\"" << escapeJson(m_history[i].content) << "\"}";
        if (i < m_history.size() - 1) json << ",";
    }

    json << "]}";
    return json.str();
}

std::string NPCChat::makeRequest(const std::string& jsonPayload) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string authHeader = "Authorization: Bearer " + m_apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // SSL CA certificate bundle for Windows
#ifdef _WIN32
        const char* caBundle = getenv("CURL_CA_BUNDLE");
        if (caBundle) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, caBundle);
        } else {
            // Try common MSYS2 locations
            curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/msys64/usr/ssl/certs/ca-bundle.crt");
        }
#endif

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return response;
}
