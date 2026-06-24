#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include "analyzer.h"

namespace fs = std::filesystem;

enum class Verbosity { QUIET, CONCISE, VERBOSE };

static std::string readFile(const fs::path& path) {
    std::ifstream in(path);
    if (!in) return "";
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
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
            for (const auto& obj : s.state.objectsOnHeap) {
                std::cout << "      heap: " << obj.objectId << " class=" << obj.className;
                if (!obj.baseClass.empty()) std::cout << " base=" << obj.baseClass;
                std::cout << " members=" << obj.members.size() << std::endl;
            }
        }
    }
}

static void printClassViews(const std::vector<ClassView>& views, Verbosity v) {
    if (v == Verbosity::QUIET) return;
    for (const auto& cv : views) {
        std::cout << "  Class: " << cv.classname;
        if (!cv.baseClass.empty()) std::cout << " : " << cv.baseClass;
        std::cout << " (depth=" << cv.inheritance_depth << ")" << std::endl;
        if (!cv.members.empty()) {
            std::cout << "    Data members:" << std::endl;
            for (const auto& m : cv.members) {
                std::cout << "      " << m.name << " : " << m.type
                          << " [" << m.accessLevel << "]" << std::endl;
            }
        }
        if (!cv.methods.empty()) {
            std::cout << "    Methods:" << std::endl;
            for (const auto& m : cv.methods) {
                std::cout << "      " << m.name << " : " << m.type
                          << " [" << m.accessLevel << "]";
                if (!m.value.empty()) std::cout << " (" << m.value << ")";
                std::cout << std::endl;
            }
        }
        if (!cv.vtable.empty()) {
            std::cout << "    VTable:" << std::endl;
            for (const auto& vt : cv.vtable) {
                std::cout << "      " << vt.first << " -> " << vt.second << std::endl;
            }
        }
        if (!cv.derived_classes.empty()) {
            std::cout << "    Derived: ";
            for (size_t i = 0; i < cv.derived_classes.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << cv.derived_classes[i];
            }
            std::cout << std::endl;
        }
    }
}

static bool runSourceTest(const std::string& name, const std::string& source,
                          bool expectSuccess, Verbosity verbosity) {
    CodeAnalyzer analyzer;
    bool ok = analyzer.loadCode(source);

    if (verbosity != Verbosity::QUIET) {
        std::cout << "  [" << name << "] ";
    }

    if (ok == expectSuccess) {
        if (verbosity != Verbosity::QUIET) std::cout << "PASS";
        if (ok) {
            analyzer.start();
            if (verbosity == Verbosity::VERBOSE) {
                auto vars = analyzer.getVariables();
                auto trace = analyzer.getExecutionTrace();
                std::cout << " (vars=" << vars.size() << " trace=" << trace.size() << ")";
                std::cout << std::endl;
                printExecutionTrace(trace);
                auto classViews = analyzer.getAllClassViews();
                if (!classViews.empty()) {
                    std::cout << "  Class views:" << std::endl;
                    printClassViews(classViews, verbosity);
                }
            }
        }
        if (verbosity != Verbosity::VERBOSE) std::cout << std::endl;
        return true;
    }

    if (verbosity != Verbosity::QUIET) {
        std::cout << "FAIL";
        if (!ok) {
            std::cout << " (parse failed)";
        } else {
            std::cout << " (unexpected success)";
        }
        std::cout << std::endl;
    }
    return false;
}

static bool runFileTest(const fs::path& path, Verbosity verbosity) {
    std::string source = readFile(path);
    if (source.empty()) {
        if (verbosity != Verbosity::QUIET) {
            std::cerr << "  FAIL: Cannot read file: " << path.filename().string() << std::endl;
        }
        return false;
    }

    CodeAnalyzer analyzer;
    bool ok = analyzer.loadCode(source);

    if (verbosity != Verbosity::QUIET) {
        std::cout << "  [" << path.filename().string() << "] ";
    }

    if (!ok) {
        if (verbosity != Verbosity::QUIET) std::cout << "FAIL (parse error)" << std::endl;
        return false;
    }

    analyzer.start();
    auto vars = analyzer.getVariables();
    auto trace = analyzer.getExecutionTrace();
    auto classViews = analyzer.getAllClassViews();

    if (verbosity != Verbosity::QUIET) {
        std::cout << "PASS (vars=" << vars.size() << " trace=" << trace.size() << " classes=" << classViews.size() << ")" << std::endl;
    }

    if (verbosity == Verbosity::VERBOSE) {
        std::cout << "  Execution trace:" << std::endl;
        printExecutionTrace(trace);
        if (!classViews.empty()) {
            std::cout << "  Class views:" << std::endl;
            printClassViews(classViews, verbosity);
        }
    }

    return true;
}

int main(int argc, char** argv) {
    Verbosity verbosity = Verbosity::CONCISE;
    std::string targetPath;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            verbosity = Verbosity::VERBOSE;
        } else if (arg == "-q" || arg == "--quiet") {
            verbosity = Verbosity::QUIET;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: test_runner [options] [path]" << std::endl;
            std::cout << "  path       File or directory to test (default: ../data/)" << std::endl;
            std::cout << "  -v, --verbose  Show full execution trace" << std::endl;
            std::cout << "  -q, --quiet    Show only summary line" << std::endl;
            std::cout << "  -h, --help     Show this help" << std::endl;
            return 0;
        } else {
            targetPath = arg;
        }
    }

    // Determine what to test
    std::vector<fs::path> testFiles;
    fs::path dataDir = targetPath.empty() ? fs::path("../data") : fs::path(targetPath);

    if (fs::exists(dataDir)) {
        if (fs::is_regular_file(dataDir)) {
            // Single file mode
            auto ext = dataDir.extension().string();
            if (ext == ".c" || ext == ".cpp") {
                testFiles.push_back(dataDir);
            } else {
                std::cerr << "Error: File must have .c or .cpp extension: " << dataDir << std::endl;
                return 1;
            }
        } else if (fs::is_directory(dataDir)) {
            // Directory scan mode
            for (auto& entry : fs::directory_iterator(dataDir)) {
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension().string();
                if (ext == ".c" || ext == ".cpp") {
                    testFiles.push_back(entry.path());
                }
            }
        }
    } else {
        std::cerr << "Error: Path does not exist: " << dataDir << std::endl;
        return 1;
    }

    if (testFiles.empty()) {
        std::cerr << "Warning: No .c/.cpp test files found in: " << dataDir << std::endl;
    }

    int total = 0;
    int passed = 0;

    // Run file tests
    if (!testFiles.empty()) {
        if (verbosity != Verbosity::QUIET) {
            std::cout << "=== File Tests (" << testFiles.size() << " files) ===" << std::endl;
        }
        for (size_t i = 0; i < testFiles.size(); ++i) {
            total++;
            if (runFileTest(testFiles[i], verbosity)) {
                passed++;
            }
        }
    }

    // Run inline validation tests
    if (verbosity != Verbosity::QUIET) {
        std::cout << "\n=== Inline Tests ===" << std::endl;
    }
    std::vector<std::tuple<std::string, std::string, bool>> customTests = {
        {"valid assignment", "int x = 0;\nx = x + 2;", true},
        {"invalid include", "#include <iostream>\nint a = 1;", false},
        {"invalid missing semicolon", "int a = 1", false}
    };

    for (auto& test : customTests) {
        total++;
        bool ok = runSourceTest(std::get<0>(test), std::get<1>(test), std::get<2>(test), verbosity);
        if (ok) passed++;
    }

    // Summary
    if (verbosity != Verbosity::QUIET) {
        std::cout << "\n============================" << std::endl;
    }
    std::cout << "Summary: " << passed << " / " << total << " tests passed." << std::endl;
    if (passed < total && verbosity != Verbosity::QUIET) {
        std::cout << "FAILURES: " << (total - passed) << " test(s) failed." << std::endl;
    }

#ifdef _WIN32
    if (verbosity != Verbosity::QUIET) {
        std::cout << "\nPress Enter to exit..." << std::endl;
        std::cin.get();
    }
#endif

    return passed == total ? 0 : 1;
}
