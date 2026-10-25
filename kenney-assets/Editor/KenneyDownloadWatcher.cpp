#include "KenneyDownloadWatcher.h"

#include "KenneyPackName.h"
#include "KenneyZip.h"

#include "Engine/Build/CancellableShellProcess.h"
#include "Types/StringUtils.h"

#include <cstdlib>
#include <system_error>

namespace GameEngine::KenneyAssets
{
namespace
{

constexpr std::chrono::seconds kXdgUserDirTimeout{5};

bool IsDirectory(const std::filesystem::path& path)
{
    std::error_code ec;
    return !path.empty() && std::filesystem::is_directory(path, ec);
}

} // namespace

std::filesystem::path FindDownloadDirectory()
{
    if (const char* fromEnvironment = std::getenv("XDG_DOWNLOAD_DIR"); fromEnvironment && IsDirectory(fromEnvironment))
        return fromEnvironment;

    const ShellProcessResult result = RunProcessCaptured(
        "xdg-user-dir", {"DOWNLOAD"}, std::chrono::duration_cast<std::chrono::milliseconds>(kXdgUserDirTimeout));
    if (result.exitCode != 0)
        return {};
    const std::filesystem::path printed = TrimWhitespace(result.output);
    return IsDirectory(printed) ? printed : std::filesystem::path{};
}

KenneyDownloadWatcher::~KenneyDownloadWatcher()
{
    Stop();
}

bool KenneyDownloadWatcher::Start(const std::filesystem::path& directory)
{
    Stop();
    m_Watcher.ClearExtensionFilters();
    m_Watcher.AddExtensionFilter(".zip");
    m_Watcher.SetCallback([this](const FileChangeEvent& event) { OnFileChange(event); });
    return m_Watcher.StartWatching(directory, false);
}

void KenneyDownloadWatcher::Stop()
{
    m_Watcher.StopWatching();
    std::lock_guard lock(m_HeardMutex);
    m_Heard.clear();
    m_Candidates.clear();
}

void KenneyDownloadWatcher::OnFileChange(const FileChangeEvent& event)
{
    if (event.Type == FileChangeType::Deleted)
        return;
    if (!ParseKenneyZipName(event.Path.filename().string()))
        return;
    std::lock_guard lock(m_HeardMutex);
    m_Heard.push_back(event.Path);
}

std::vector<std::filesystem::path> KenneyDownloadWatcher::TakeFinishedDownloads(std::chrono::steady_clock::time_point now)
{
    {
        std::lock_guard lock(m_HeardMutex);
        for (std::filesystem::path& path : m_Heard)
            m_Candidates[std::move(path)] = Candidate{static_cast<std::uintmax_t>(-1), now};
        m_Heard.clear();
    }

    std::vector<std::filesystem::path> finished;
    for (auto it = m_Candidates.begin(); it != m_Candidates.end();)
    {
        std::error_code ec;
        const std::uintmax_t size = std::filesystem::file_size(it->first, ec);
        if (ec)
        {
            it = m_Candidates.erase(it); // deleted or renamed away
            continue;
        }
        Candidate& candidate = it->second;
        if (size != candidate.Size)
        {
            candidate.Size = size;
            candidate.LastChange = now;
        }
        // A settled file that is not yet a complete zip (Firefox's empty
        // placeholder) stays a candidate: the finished download replaces it,
        // and the watcher reports that change.
        if (now - candidate.LastChange >= kSettleTime && size > 0 && IsCompleteZip(it->first))
        {
            finished.push_back(it->first);
            it = m_Candidates.erase(it);
            continue;
        }
        ++it;
    }
    return finished;
}

} // namespace GameEngine::KenneyAssets
