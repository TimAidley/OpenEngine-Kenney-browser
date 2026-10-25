#include "KenneyZip.h"

#include "Engine/Build/CancellableShellProcess.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <system_error>
#include <vector>

namespace GameEngine::KenneyAssets
{
namespace
{

constexpr std::array<char, 4> kEndOfCentralDirectory = {'P', 'K', '\x05', '\x06'};
// The record is 22 bytes plus a comment of up to 65535 bytes.
constexpr std::streamoff kMaxRecordSearch = 22 + 65535;
constexpr std::chrono::minutes kUnzipTimeout{5};

} // namespace

bool IsCompleteZip(const std::filesystem::path& zipFile)
{
    std::error_code ec;
    const auto size = static_cast<std::streamoff>(std::filesystem::file_size(zipFile, ec));
    if (ec || size < 22)
        return false;

    std::ifstream in(zipFile, std::ios::binary);
    const std::streamoff searchSize = std::min(size, kMaxRecordSearch);
    std::vector<char> tail(static_cast<size_t>(searchSize));
    in.seekg(size - searchSize);
    if (!in.read(tail.data(), searchSize))
        return false;
    const auto found = std::search(tail.rbegin(), tail.rend(), kEndOfCentralDirectory.rbegin(),
                                   kEndOfCentralDirectory.rend());
    return found != tail.rend();
}

bool ExtractZip(const std::filesystem::path& zipFile, const std::filesystem::path& destination,
                std::string& outError)
{
    std::error_code ec;
    std::filesystem::create_directories(destination, ec);
    if (ec)
    {
        outError = "cannot create " + destination.string() + ": " + ec.message();
        return false;
    }

    // -qq: quiet; -o: overwrite without prompting; -d: target folder.
    const ShellProcessResult result = RunProcessCaptured(
        "unzip", {"-qq", "-o", zipFile.string(), "-d", destination.string()},
        std::chrono::duration_cast<std::chrono::milliseconds>(kUnzipTimeout));
    // 127 is the child's exit when exec found no unzip; -1 is no child at all.
    if (result.exitCode == 127 || result.exitCode == -1)
    {
        outError = "cannot run the unzip tool; install it (for example `sudo dnf install unzip`)";
        return false;
    }
    // 1 is "finished with warnings" (for example a skipped unsafe entry name).
    if (result.cancelled || result.exitCode > 1)
    {
        outError = "unzip failed (exit code " + std::to_string(result.exitCode) + "): " + result.output;
        return false;
    }
    return true;
}

} // namespace GameEngine::KenneyAssets
