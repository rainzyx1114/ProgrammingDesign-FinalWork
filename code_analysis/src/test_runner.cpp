#include <iostream>
#include <fstream>
#include <sstream>
#include "analyzer.h"

int main(int argc, char** argv) {
    std::string path = "../data/simple1.c";
    if (argc > 1) path = argv[1];

    std::ifstream in(path);
    if (!in) {
        std::cerr << "Cannot open file: " << path << std::endl;
        return 1;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    std::string src = ss.str();

    CodeAnalyzer analyzer;
    if (!analyzer.loadCode(src)) {
        std::cerr << "Failed to parse source." << std::endl;
        return 1;
    }

    analyzer.start();

    auto vars = analyzer.getVariables();
    std::cout << "Variables:\n";
    for (auto& v : vars) {
        std::cout << v.name << " = " << v.value << "\n";
    }

    auto st = analyzer.getExecutionState();
    std::cout << "ExecutionState: isRunning=" << st.isRunning << " currentLine=" << st.currentLine << "\n";

    auto trace = analyzer.getExecutionTrace();
    std::cout << "Trace count=" << trace.size() << "\n";
    for (auto &s : trace) {
        std::cout << s.stepIndex << ": " << s.event << " line=" << s.state.currentLine << " func=" << s.state.currentFunction << "\n";
        for (auto &f : s.state.stackTrace.frames) {
            std::cout << "  frame " << f.functionName << " line=" << f.lineNumber << "\n";
            for (auto &v : f.variables) {
                std::cout << "    " << v.name << " = " << v.value << "\n";
            }
        }
    }

    return 0;
}
