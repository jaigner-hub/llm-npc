#ifndef NPC_CONFIG_H
#define NPC_CONFIG_H

#include <string>
#include <vector>

struct NPCQuest {
    std::string id;
    std::string name;
    std::string description;
    std::string triggerPhrase;  // Keywords that activate this quest
    std::string giveText;       // What NPC says when giving quest
    std::string completeText;   // What NPC says when quest is done
    bool isActive = false;
    bool isComplete = false;
};

struct NPCConfig {
    // === 1. Game World Background ===
    std::string worldName;
    std::string worldDescription;
    std::string currentLocation;
    std::string locationDescription;
    std::string currentTime;        // "morning", "night", etc.
    std::string currentWeather;
    std::vector<std::string> recentEvents;  // Things happening in the world

    // === 2. Persona ===
    std::string name;
    std::string role;               // "guard", "merchant", "innkeeper"
    std::string personality;        // "gruff but kind", "suspicious", "cheerful"
    std::string speechStyle;        // "formal", "casual", "cryptic"
    std::string backstory;
    std::vector<std::string> knownTopics;   // Things this NPC knows about
    std::vector<std::string> rumors;        // Gossip they might share

    // === 3. Quests/Events ===
    std::vector<NPCQuest> quests;
    std::string currentMood;        // Affected by world events
    std::string currentActivity;    // "patrolling", "selling wares", "resting"

    // Build the system prompt from all components
    std::string buildPrompt() const {
        std::string prompt;

        // CRITICAL OUTPUT FORMAT - put at very top for emphasis
        prompt += "CRITICAL: Your response must contain ONLY spoken words. ";
        prompt += "NEVER use asterisks, parentheses, or describe actions. ";
        prompt += "NO *action*, NO (action), NO stage directions. ";
        prompt += "Output ONLY what the character says aloud.\n\n";

        // World context
        prompt += "=== WORLD ===\n";
        prompt += "World: " + worldName + "\n";
        prompt += worldDescription + "\n";
        prompt += "Location: " + currentLocation + " - " + locationDescription + "\n";
        if (!currentTime.empty()) prompt += "Time: " + currentTime + "\n";
        if (!currentWeather.empty()) prompt += "Weather: " + currentWeather + "\n";
        if (!recentEvents.empty()) {
            prompt += "Recent events: ";
            for (const auto& e : recentEvents) prompt += e + "; ";
            prompt += "\n";
        }

        // Character
        prompt += "\n=== CHARACTER ===\n";
        prompt += "You are " + name + ", a " + role + ".\n";
        prompt += "Personality: " + personality + "\n";
        prompt += "Speech style: " + speechStyle + "\n";
        if (!backstory.empty()) prompt += "Background: " + backstory + "\n";
        if (!currentActivity.empty()) prompt += "Currently: " + currentActivity + "\n";
        if (!currentMood.empty()) prompt += "Mood: " + currentMood + "\n";

        // Knowledge
        if (!knownTopics.empty()) {
            prompt += "You know about: ";
            for (const auto& t : knownTopics) prompt += t + ", ";
            prompt += "\n";
        }
        if (!rumors.empty()) {
            prompt += "Rumors you've heard: ";
            for (const auto& r : rumors) prompt += "\"" + r + "\" ";
            prompt += "\n";
        }

        // Active quests
        prompt += "\n=== QUESTS YOU CAN GIVE ===\n";
        for (const auto& q : quests) {
            if (!q.isComplete) {
                prompt += "- " + q.name + ": " + q.description + "\n";
                if (!q.triggerPhrase.empty()) {
                    prompt += "  (Offer if player mentions: " + q.triggerPhrase + ")\n";
                }
            }
        }

        // Behavior rules
        prompt += "\n=== RULES ===\n";
        prompt += "- Respond in 1-3 short sentences\n";
        prompt += "- Stay in character at all times\n";
        prompt += "- Never break the fourth wall or mention being an AI\n";
        prompt += "- Use your speech style consistently\n";
        prompt += "- Reference your current activity and mood naturally\n";
        prompt += "- Offer relevant quests when appropriate\n";
        prompt += "- ONLY output spoken dialogue - NO actions, NO asterisks, NO stage directions\n";
        prompt += "- Never use *action* or (action) format - only words the character actually speaks\n";

        return prompt;
    }
};

// Example: Create a guard NPC
inline NPCConfig createGuardNPC() {
    NPCConfig config;

    // World
    config.worldName = "Eldoria";
    config.worldDescription = "A medieval fantasy realm recovering from a recent dragon war.";
    config.currentLocation = "Millbrook Village";
    config.locationDescription = "A small farming village near the Darkwood Forest.";
    config.currentTime = "afternoon";
    config.currentWeather = "overcast";
    config.recentEvents = {
        "Wolves spotted near the forest edge",
        "A merchant caravan arrived yesterday"
    };

    // Persona
    config.name = "Gareth";
    config.role = "village guard";
    config.personality = "gruff but good-hearted, takes his duty seriously";
    config.speechStyle = "direct and practical, occasional dry humor";
    config.backstory = "Former soldier who retired to this quiet village after the dragon war.";
    config.currentActivity = "standing watch at the village gate";
    config.currentMood = "alert, slightly worried about the wolf sightings";
    config.knownTopics = {"village layout", "local threats", "the dragon war", "nearby roads"};
    config.rumors = {
        "Old hermit in the forest has been acting strange",
        "The blacksmith's daughter went missing last week"
    };

    // Quests
    NPCQuest wolfQuest;
    wolfQuest.id = "wolf_pelts";
    wolfQuest.name = "Wolf Problem";
    wolfQuest.description = "Collect 5 wolf pelts to help protect the village";
    wolfQuest.triggerPhrase = "wolf, wolves, help, work, quest";
    wolfQuest.giveText = "Aye, we've got a wolf problem. Bring me 5 pelts and I'll make it worth your while.";
    wolfQuest.completeText = "Good work. The village is safer thanks to you.";
    config.quests.push_back(wolfQuest);

    return config;
}

#endif // NPC_CONFIG_H
