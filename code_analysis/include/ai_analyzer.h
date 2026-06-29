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

    std::string buildPrompt(
        const std::string& sourceCode,
        const std::vector<Stepsnapshot>& trace,
        const std::vector<ClassView>& classViews,
        const std::vector<ObjectView>& objectsOnHeap,
        const std::vector<VariableInfo>& variables
    ) const;

    // Make an HTTP POST request to the OpenAI-compatible chat completions API
    // Returns the response body as a string, or throws on failure
    std::string callChatAPI(const std::string& prompt) const;

    // Parse the AI's JSON response into structured fields
    AIAnalysisResult parseResponse(const std::string& rawJSON) const;

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

    // Run AI analysis — gathers all state and sends it to the API
    AIAnalysisResult analyze(
        const std::string& sourceCode,
        const std::vector<Stepsnapshot>& trace,
        const std::vector<ClassView>& classViews,
        const std::vector<ObjectView>& objectsOnHeap,
        const std::vector<VariableInfo>& variables
    ) const;
};

#endif
