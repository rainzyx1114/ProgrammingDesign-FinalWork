#include "ai_analyzer.h"
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + s.size() / 10);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r'))
        ++start;
    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\n' || s[end - 1] == '\r'))
        --end;
    return s.substr(start, end - start);
}

static std::string extractJSONString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    std::string val = json.substr(pos + 1, end - pos - 1);
    // unescape
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i + 1 < val.size()) {
            switch (val[i + 1]) {
                case 'n': val.replace(i, 2, "\n"); break;
                case 'r': val.replace(i, 2, "\r"); break;
                case 't': val.replace(i, 2, "\t"); break;
                case '"': val.replace(i, 2, "\""); break;
                case '\\': val.replace(i, 2, "\\"); break;
                default: break;
            }
        }
    }
    return val;
}


#ifdef _WIN32
// Windows: use WinHTTP
static std::string platformPost(const std::string& url,
                                const std::string& apiKey,
                                const std::string& jsonBody) {
    // Parse URL
    std::string host, path;
    bool useSSL = false;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTP_PORT;

    std::string u = url;
    if (u.compare(0, 8, "https://") == 0) {
        useSSL = true;
        port = INTERNET_DEFAULT_HTTPS_PORT;
        u = u.substr(8);
    } else if (u.compare(0, 7, "http://") == 0) {
        u = u.substr(7);
    }

    size_t slash = u.find('/');
    if (slash != std::string::npos) {
        host = u.substr(0, slash);
        path = u.substr(slash);
    } else {
        host = u;
        path = "/";
    }

    // Convert to wide strings
    int wlen;
    wlen = MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, nullptr, 0);
    std::wstring whost(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, host.c_str(), -1, &whost[0], wlen);

    wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);

    // Open session
    HINTERNET hSession = WinHttpOpen(L"CodeAnalyzer/1.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    DWORD flags = useSSL ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wpath.c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Headers
    std::string authHeader = "Authorization: Bearer " + apiKey;
    std::wstring wauth(authHeader.begin(), authHeader.end());
    std::wstring headers = L"Content-Type: application/json\r\n" + wauth + L"\r\n";

    WinHttpAddRequestHeaders(hRequest, headers.c_str(), (DWORD)-1,
                              WINHTTP_ADDREQ_FLAG_ADD);

    // Send
    std::string body = jsonBody;
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                             (LPVOID)body.c_str(), (DWORD)body.size(),
                             (DWORD)body.size(), 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Read response
    std::string response;
    DWORD bytesAvailable = 0;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        DWORD toRead = (bytesAvailable < sizeof(buffer)) ? bytesAvailable : sizeof(buffer);
        if (WinHttpReadData(hRequest, buffer, toRead, &bytesRead)) {
            response.append(buffer, bytesRead);
        } else {
            break;
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}
#else
// Linux/macOS: use curl via popen
static std::string platformPost(const std::string& url,
                                const std::string& apiKey,
                                const std::string& jsonBody) {
    // Escape the JSON body for shell
    std::string escapedBody;
    for (char c : jsonBody) {
        if (c == '\'') escapedBody += "'\\''";
        else escapedBody += c;
    }

    std::string cmd =
        "curl -s -X POST '" + url + "' "
        "-H 'Content-Type: application/json' "
        "-H 'Authorization: Bearer " + apiKey + "' "
        "-d '" + escapedBody + "' 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    std::string response;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        response += buffer;
    }

    int status = pclose(pipe);
    (void)status;  // ignore — caller checks response content

    return response;
}
#endif

AIAnalyzer::AIAnalyzer()
    : apiEndpoint("https://api.openai.com/v1/chat/completions")
    , model("gpt-4o")
    , configured(false)
{}

void AIAnalyzer::setAPIKey(const std::string& key) {
    apiKey = key;
    configured = !key.empty();
}

void AIAnalyzer::setEndpoint(const std::string& endpoint) {
    if (!endpoint.empty())
        apiEndpoint = endpoint;
}

void AIAnalyzer::setModel(const std::string& m) {
    if (!m.empty())
        model = m;
}

std::string AIAnalyzer::callChatAPI(const std::string& userPrompt) const {
    // Build the JSON request body
    std::string systemMsg =
        "You are an expert programming teacher who analyses C/C++ code. "
        "You explain code execution step by step, identify bugs, suggest improvements, "
        "and highlight OOP / language concepts. "
        "Always respond in the following JSON format exactly — do NOT include markdown fences or extra text:\n"
        "{\n"
        "  \"explanation\": \"...high level overview...\",\n"
        "  \"stepByStep\": \"...detailed step-by-step walkthrough...\",\n"
        "  \"suggestions\": \"...improvement ideas...\",\n"
        "  \"potentialBugs\": \"...bugs or issues found...\",\n"
        "  \"concepts\": \"...OOP / language concepts demonstrated...\"\n"
        "}\n"
        "Use proper JSON escaping for quotes and newlines inside string values.";

    std::string jsonPayload =
        "{\n"
        "  \"model\": \"" + model + "\",\n"
        "  \"messages\": [\n"
        "    {\"role\": \"system\", \"content\": \"" + jsonEscape(systemMsg) + "\"},\n"
        "    {\"role\": \"user\", \"content\": \"" + jsonEscape(userPrompt) + "\"}\n"
        "  ],\n"
        "  \"temperature\": 0.3\n"
        "}\n";

    return platformPost(apiEndpoint, apiKey, jsonPayload);
}

std::string AIAnalyzer::buildPrompt(
    const std::string& sourceCode,
    const std::vector<Stepsnapshot>& trace,
    const std::vector<ClassView>& classViews,
    const std::vector<ObjectView>& objectsOnHeap,
    const std::vector<VariableInfo>& variables) const
{
    std::ostringstream prompt;

    prompt << "Please analyse the following C/C++ code and its execution.  "
           << "Give me a complete teaching explanation.\n\n";

    prompt << "=== SOURCE CODE ===\n" << sourceCode << "\n\n";

    if (!trace.empty()) {
        prompt << "=== EXECUTION TRACE (" << trace.size() << " steps) ===\n";
        for (const auto& s : trace) {
            prompt << "Step " << s.stepIndex << " [" << s.event << "] "
                   << "line=" << s.state.currentLine
                   << " func=" << s.state.currentFunction << "\n";
            for (const auto& frame : s.state.stackTrace.frames) {
                prompt << "  Frame: " << frame.functionName
                       << " line=" << frame.lineNumber << "\n";
                for (const auto& var : frame.variables) {
                    prompt << "    var " << var.name
                           << " (" << var.type << ") = " << var.value << "\n";
                }
            }
        }
        prompt << "\n";
    }

    if (!variables.empty()) {
        prompt << "=== FINAL VARIABLES (" << variables.size() << ") ===\n";
        for (const auto& v : variables) {
            prompt << "  " << v.name << " : " << v.type << " = " << v.value << "\n";
        }
        prompt << "\n";
    }

    if (!objectsOnHeap.empty()) {
        prompt << "=== HEAP OBJECTS (" << objectsOnHeap.size() << ") ===\n";
        for (const auto& obj : objectsOnHeap) {
            prompt << "  " << obj.objectId << " class=" << obj.className;
            if (!obj.baseClass.empty()) prompt << " base=" << obj.baseClass;
            prompt << "\n";
            for (const auto& m : obj.members) {
                prompt << "    " << m.name << " (" << m.type << ") = "
                       << m.value << " [" << m.accessLevel << "]\n";
            }
        }
        prompt << "\n";
    }

    if (!classViews.empty()) {
        prompt << "=== CLASS HIERARCHY (" << classViews.size() << " classes) ===\n";
        for (const auto& cv : classViews) {
            prompt << "  Class: " << cv.classname;
            if (!cv.baseClass.empty()) prompt << " : " << cv.baseClass;
            prompt << " (inheritance_depth=" << cv.inheritance_depth << ")\n";

            if (!cv.members.empty()) {
                prompt << "    Data members:\n";
                for (const auto& m : cv.members)
                    prompt << "      " << m.name << " : " << m.type
                           << " [" << m.accessLevel << "]\n";
            }
            if (!cv.methods.empty()) {
                prompt << "    Methods:\n";
                for (const auto& m : cv.methods) {
                    prompt << "      " << m.name << " : " << m.type
                           << " [" << m.accessLevel << "]";
                    if (!m.value.empty()) prompt << " (" << m.value << ")";
                    prompt << "\n";
                }
            }
            if (!cv.derived_classes.empty()) {
                prompt << "    Derived classes: ";
                for (size_t i = 0; i < cv.derived_classes.size(); ++i) {
                    if (i > 0) prompt << ", ";
                    prompt << cv.derived_classes[i];
                }
                prompt << "\n";
            }
            if (!cv.vtable.empty()) {
                prompt << "    VTable:\n";
                for (const auto& vt : cv.vtable)
                    prompt << "      " << vt.first << " -> " << vt.second << "\n";
            }
        }
        prompt << "\n";
    }

    prompt << "Based on the above, provide your JSON response with fields: "
           << "explanation, stepByStep, suggestions, potentialBugs, concepts.";

    return prompt.str();
}

AIAnalysisResult AIAnalyzer::parseResponse(const std::string& raw) const {
    AIAnalysisResult result;
    result.success = false;
    result.rawResponse = raw;

    // Try to extract the JSON from the response (may be wrapped in ```json)
    std::string json = trim(raw);
    if (json.empty()) {
        result.errorMessage = "Empty response from AI API";
        return result;
    }

    // Strip markdown fences if present
    const std::string fence1 = "```json";
    const std::string fence2 = "```";
    if (json.compare(0, fence1.size(), fence1) == 0) {
        json = json.substr(fence1.size());
    } else if (json.compare(0, fence2.size(), fence2) == 0) {
        json = json.substr(fence2.size());
    }
    if (json.size() >= fence2.size() &&
        json.compare(json.size() - fence2.size(), fence2.size(), fence2) == 0) {
        json = json.substr(0, json.size() - fence2.size());
    }
    json = trim(json);

    // First, try to extract from the OpenAI response wrapper
    // Look for "content": "..." in the choices array
    std::string content = extractJSONString(json, "content");
    if (!content.empty()) {
        // This was an OpenAI wrapper — use the content field as the JSON
        json = content;
        // Remove markdown fences from the inner content too
        if (json.compare(0, fence1.size(), fence1) == 0)
            json = json.substr(fence1.size());
        else if (json.compare(0, fence2.size(), fence2) == 0)
            json = json.substr(fence2.size());
        if (json.size() >= fence2.size() &&
            json.compare(json.size() - fence2.size(), fence2.size(), fence2) == 0)
            json = json.substr(0, json.size() - fence2.size());
        json = trim(json);
    }

    // Now parse the actual JSON response
    result.explanation = extractJSONString(json, "explanation");
    result.stepByStep  = extractJSONString(json, "stepByStep");
    result.suggestions = extractJSONString(json, "suggestions");
    result.potentialBugs = extractJSONString(json, "potentialBugs");
    result.concepts     = extractJSONString(json, "concepts");

    // Success if we got at least one meaningful field
    if (!result.explanation.empty() || !result.stepByStep.empty()) {
        result.success = true;
    } else {
        result.errorMessage = "Could not parse AI response JSON. Raw response:\n" + raw;
    }

    return result;
}

AIAnalysisResult AIAnalyzer::analyze(
    const std::string& sourceCode,
    const std::vector<Stepsnapshot>& trace,
    const std::vector<ClassView>& classViews,
    const std::vector<ObjectView>& objectsOnHeap,
    const std::vector<VariableInfo>& variables) const
{
    AIAnalysisResult result;
    result.success = false;

    if (!configured) {
        result.errorMessage =
            "API key not set.  Use --api-key to provide your key, or run in MANUAL mode.";
        return result;
    }

    std::string prompt = buildPrompt(sourceCode, trace, classViews, objectsOnHeap, variables);

    try {
        std::string rawResponse = callChatAPI(prompt);
        if (rawResponse.empty()) {
            result.errorMessage =
                "Empty response from AI API.  Check your API key, endpoint, and network connection.";
            return result;
        }

        result = parseResponse(rawResponse);
        if (!result.success && result.errorMessage.empty()) {
            result.errorMessage = "Failed to parse AI response. Raw:\n" + rawResponse;
        }
    } catch (const std::exception& e) {
        result.errorMessage = std::string("API call failed: ") + e.what();
    } catch (...) {
        result.errorMessage = "Unknown error during API call.";
    }

    return result;
}
