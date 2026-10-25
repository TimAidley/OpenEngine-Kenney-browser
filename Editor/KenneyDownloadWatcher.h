#pragma once

#include "FileWatcher/FileWatcher.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <vector>

namespace GameEngine::KenneyAssets
{

/// The user's download folder: $XDG_DOWNLOAD_DIR, else what `xdg-user-dir
/// DOWNLOAD` prints. Empty when neither names an existing folder.
std::filesystem::path FindDownloadDirectory();

/// Watches the download folder for finished kenney.nl pack zips. Browsers write
/// a download under a temporary name (.part, .crdownload) and rename it when it
/// is done; Firefox also leaves an empty placeholder under the final name. A
/// zip counts as finished once its size has held still for kSettleTime and it
/// ends with a complete zip directory. Zips already in the folder when watching
/// starts are not reported.
class KenneyDownloadWatcher
{
public:
    static constexpr std::chrono::seconds kSettleTime{2};

    ~KenneyDownloadWatcher();

    bool Start(const std::filesystem::path& directory);
    void Stop();
    bool IsWatching() const { return m_Watcher.IsWatching(); }

    /// UI thread: the zips that finished since the last call, each reported
    /// once per change to the file.
    std::vector<std::filesystem::path> TakeFinishedDownloads(std::chrono::steady_clock::time_point now);

private:
    struct Candidate
    {
        std::uintmax_t Size = 0;
        std::chrono::steady_clock::time_point LastChange;
    };

    void OnFileChange(const FileChangeEvent& event);

    FileWatcher m_Watcher;
    std::mutex m_HeardMutex;
    std::vector<std::filesystem::path> m_Heard;              // written by the watch thread
    std::map<std::filesystem::path, Candidate> m_Candidates; // UI thread only
};

} // namespace GameEngine::KenneyAssets
