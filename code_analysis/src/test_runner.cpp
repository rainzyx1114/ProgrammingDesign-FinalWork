#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include "analyzer.h"

namespace fs = std::filesystem;

static std::string readFile(const fs::path& path) {
    std::ifstream in(path);
    if (!in) return "";
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool runSourceTest(const std::string& name, const std::string& source, bool expectSuccess) {
    CodeAnalyzer analyzer;
    bool ok = analyzer.loadCode(source);
    std::cout << "Test: " << name << " => ";
    if (ok == expectSuccess) {
        std::cout << "PASS";
        if (ok) {
            analyzer.start();
            auto vars = analyzer.getVariables();
            std::cout << " (vars=" << vars.size() << ")";
        }
        std::cout << std::endl;
        return true;
    }
    std::cout << "FAIL";
    if (!ok) {
        std::cout << " (loadCode returned false)";
    } else {
        std::cout << " (unexpected success)";
    }
    std::cout << std::endl;
    return false;
}

static void printExecutionTrace(const std::vector<Stepsnapshot>& trace) {
    for (const auto& s : trace) {
        std::cout << "    [" << s.stepIndex << "] event=" << s.event
                  << " line=" << s.state.currentLine
                  << " func=" << s.state.currentFunction << std::endl;
        for (const auto& frame : s.state.stackTrace.frames) {
            std::cout << "      frame=" << frame.functionName << " line=" << frame.lineNumber << std::endl;
            for (const auto& var : frame.variables) {
                std::cout << "        " << var.name << " (" << var.type << ") = " << var.value << std::endl;
            }
        }
        if (!s.state.objectsOnHeap.empty()) {
            std::cout << "      heap objects=" << s.state.objectsOnHeap.size() << std::endl;
        }
    }
}

static bool runFileTest(const fs::path& path) {
    std::string source = readFile(path);
    if (source.empty()) {
        std::cerr << "Cannot read file: " << path << std::endl;
        return false;
    }
    CodeAnalyzer analyzer;
    bool ok = analyzer.loadCode(source);
    std::cout << "File: " << path.filename().string() << " => ";
    if (!ok) {
        std::cout << "FAILED to parse." << std::endl;
        return false;
    }
    std::cout << "Parsed." << std::endl;
    analyzer.start();
    auto vars = analyzer.getVariables();
    auto trace = analyzer.getExecutionTrace();
    std::cout << "  Variables=" << vars.size() << "  Trace=" << trace.size() << std::endl;
    std::cout << "  Execution snapshots:" << std::endl;
    printExecutionTrace(trace);
    return true;
}

int main(int argc, char** argv) {
    fs::path dataDir = fs::path("../data");
    if (argc > 1) {
        dataDir = fs::path(argv[1]);
    }

    std::vector<fs::path> testFiles;
    if (fs::exists(dataDir) && fs::is_directory(dataDir)) {
        for (auto& entry : fs::directory_iterator(dataDir)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            if (ext == ".c" || ext == ".cpp") {
                testFiles.push_back(entry.path());
            }
        }
    }

    int total = 0;
    int passed = 0;

    std::cout << "Running data file tests in: " << dataDir << std::endl;
    for (auto& path : testFiles) {
        total++;
        if (runFileTest(path)) {
            passed++;
        }
    }

    std::cout << "\nRunning custom source tests" << std::endl;
    std::vector<std::tuple<std::string, std::string, bool>> customTests = {
        {"valid assignment", "int x = 0;\nx = x + 2;", true},
        {"invalid include", "#include <iostream>\nint a = 1;", false},
        {"invalid missing semicolon", "int a = 1", false}
    };

    for (auto& test : customTests) {
        total++;
        bool ok = runSourceTest(std::get<0>(test), std::get<1>(test), std::get<2>(test));
        if (ok) passed++;
    }

    std::cout << "\nSummary: " << passed << " / " << total << " tests passed." << std::endl;
    return passed == total ? 0 : 1;
}
