#pragma once

#include "KenneyPackName.h"

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace GameEngine::KenneyAssets
{

struct KenneyImportJob
{
    std::filesystem::path Zip;
    KenneyPackName Name;
    bool FromDownload = false;       ///< copy the zip into the cache first
    std::filesystem::path AssetsDir; ///< the project's asset folder; empty caches only
};

struct KenneyJobOutcome
{
    KenneyPackName Name;
    size_t ImportedFiles = 0;
    std::string Error; ///< empty on success
};

/// Runs pack imports one at a time on a worker thread. The queue lives for the
/// whole module, so an import keeps going when the panel closes and its outcome
/// waits for the next panel. Enqueue and TakeOutcomes are for the UI thread.
///
/// A plain thread rather than the job system, like the unity-import package:
/// each job blocks for seconds on the unzip child process.
class KenneyImportQueue
{
public:
    static KenneyImportQueue& Get();

    ~KenneyImportQueue();

    void Enqueue(KenneyImportJob job);
    std::vector<KenneyJobOutcome> TakeOutcomes();
    /// Jobs waiting or running.
    size_t PendingCount() const;

private:
    KenneyImportQueue() = default;

    void WorkerLoop();
    static KenneyJobOutcome Run(const KenneyImportJob& job);

    mutable std::mutex m_Mutex;
    std::condition_variable m_Wake;
    std::deque<KenneyImportJob> m_Jobs;
    std::vector<KenneyJobOutcome> m_Outcomes;
    size_t m_Running = 0;
    bool m_Stop = false;
    std::thread m_Worker;
};

} // namespace GameEngine::KenneyAssets
