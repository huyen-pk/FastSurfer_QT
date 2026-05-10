#pragma once

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace OpenHC::tests::fastsurfer::support {

inline std::filesystem::path makeFreshDirectory(const std::string &name)
{
    const auto root = [&]() {
        if (const char *envTmp = std::getenv("FASTSURFER_TEST_TMPDIR"); envTmp != nullptr && envTmp[0] != '\0') {
            return std::filesystem::path(envTmp) / name;
        }

        const std::filesystem::path repoRoot = OPENHC_REPO_ROOT;
        return repoRoot / ".tmp" / name;
    }();

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    ec.clear();
    std::filesystem::create_directories(root, ec);
    if (ec || !std::filesystem::exists(root)) {
        throw std::runtime_error("Failed to create temporary test directory: " + root.string());
    }

    return root;
}

inline std::string shellEscape(const std::filesystem::path &path)
{
    const std::string value = path.string();
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('\'');
    for (const char character : value) {
        if (character == '\'') {
            escaped += "'\\''";
        } else {
            escaped.push_back(character);
        }
    }
    escaped.push_back('\'');
    return escaped;
}

inline std::filesystem::path resolvePythonExecutable(const std::filesystem::path &repoRoot)
{
    if (const char *configured = std::getenv("FASTSURFER_PYTHON_EXECUTABLE"); configured != nullptr && configured[0] != '\0') {
        return configured;
    }

    const auto workspaceVenv = repoRoot / ".venv-parity" / "bin" / "python";
    if (std::filesystem::exists(workspaceVenv)) {
        return workspaceVenv;
    }

    const auto repoVenv = repoRoot / ".venv" / "bin" / "python";
    if (std::filesystem::exists(repoVenv)) {
        return repoVenv;
    }

    return "python3";
}

inline int runCommand(const std::string &command)
{
    return std::system(command.c_str());
}

} // namespace OpenHC::tests::fastsurfer::support