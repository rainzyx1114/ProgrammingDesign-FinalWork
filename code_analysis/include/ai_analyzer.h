#ifndef AI_ANALYZER_H
#define AI_ANALYZER_H

#include <string>
#include <vector>
#include <optional>
#include "visualization_data.h"

enum class AnalysisMode {
    MANUAL,        // Existing hand-built static + execution analysis
    AI_TEACHING    // AI-powered detailed teaching, using user's own API key
};

struct AIAnalysisResult {
    bool success;                    // true if AI returned a valid response
    std::string rawResponse;         // Full text response from the AI
    std::string explanation;         // General explanation of what the code does
    std::string stepByStep;          // Step-by-step execution walkthrough
    std::string suggestions;         // Improvement suggestions
    std::string potentialBugs;       // Identified issues / bugs
    std::string concepts;            // OOP / language concepts demonstrated
    std::string errorMessage;        // Non-empty if something went wrong
};

class AIAnalyzer {
private:
    std::string apiKey;              // User-provided API key
    std::string apiEndpoint;         // Base URL for the API (default OpenAI)
    std::string model;               // Model name to use
    bool configured;                 // true when apiKey is set

    // Conversation history: pairs of (role, content)
    mutable std::vector<std::pair<std::string, std::string>> conversationHistory;

    std::string buildPrompt(const std::string& sourceCode) const;

    // Make an HTTP POST request to the OpenAI-compatible chat completions API
    // Accepts a list of messages (role, content pairs)
    std::string callChatAPI(const std::vector<std::pair<std::string, std::string>>& messages) const;

    // Parse the AI's response into structured fields
    AIAnalysisResult parseResponse(const std::string& rawJSON,
                                   bool allowPlainText = false) const;

    // Parse a URL into host, port, path components
    struct URLParts {
        std::string host;
        int port;
        std::string path;
        bool useSSL;
    };
    URLParts parseURL(const std::string& url) const;

    // Minimal HTTPS client: POST JSON, return response body
    std::string httpsPost(const URLParts& url, const std::string& jsonBody) const;

public:
    AIAnalyzer();

    // Configuration — call before analyze()
    void setAPIKey(const std::string& key);
    void setEndpoint(const std::string& endpoint);
    void setModel(const std::string& model);

    bool isConfigured() const { return configured; }
    const std::string& getModel() const { return model; }

    // Run AI analysis on source code only — starts a new conversation
    AIAnalysisResult analyze(const std::string& sourceCode);

    // Ask a follow-up question — continues the existing conversation
    AIAnalysisResult askFollowUp(const std::string& question);
};

#endif
