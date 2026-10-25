#include "KenneyImportQueue.h"

#include "KenneyImport.h"
#include "KenneyPackCache.h"

#include "Logger/Logger.h"

#include <utility>

namespace GameEngine::KenneyAssets
{

KenneyImportQueue& KenneyImportQueue::Get()
{
    static KenneyImportQueue s_Queue;
    return s_Queue;
}

KenneyImportQueue::~KenneyImportQueue()
{
    {
        std::lock_guard lock(m_Mutex);
        m_Stop = true;
        m_Jobs.clear();
    }
    m_Wake.notify_all();
    if (m_Worker.joinable())
        m_Worker.join();
}

void KenneyImportQueue::Enqueue(KenneyImportJob job)
{
    {
        std::lock_guard lock(m_Mutex);
        m_Jobs.push_back(std::move(job));
        if (!m_Worker.joinable())
            m_Worker = std::thread([this] { WorkerLoop(); });
    }
    m_Wake.notify_one();
}

std::vector<KenneyJobOutcome> KenneyImportQueue::TakeOutcomes()
{
    std::lock_guard lock(m_Mutex);
    return std::exchange(m_Outcomes, {});
}

size_t KenneyImportQueue::PendingCount() const
{
    std::lock_guard lock(m_Mutex);
    return m_Jobs.size() + m_Running;
}

void KenneyImportQueue::WorkerLoop()
{
    std::unique_lock lock(m_Mutex);
    while (true)
    {
        m_Wake.wait(lock, [this] { return m_Stop || !m_Jobs.empty(); });
        if (m_Stop)
            return;
        KenneyImportJob job = std::move(m_Jobs.front());
        m_Jobs.pop_front();
        ++m_Running;

        lock.unlock();
        KenneyJobOutcome outcome = Run(job);
        lock.lock();

        --m_Running;
        m_Outcomes.push_back(std::move(outcome));
    }
}

KenneyJobOutcome KenneyImportQueue::Run(const KenneyImportJob& job)
{
    KenneyJobOutcome outcome{job.Name, 0, {}};
    std::filesystem::path zip = job.Zip;
    if (job.FromDownload)
        zip = AddToCache(job.Zip, job.Name, outcome.Error);

    if (!zip.empty() && !job.AssetsDir.empty())
    {
        const KenneyImportResult result = ImportKenneyPack(zip, job.Name, job.AssetsDir);
        outcome.ImportedFiles = result.Files.size();
        outcome.Error = result.Error;
    }
    if (!outcome.Error.empty())
        Logger::Log::Error("Kenney: importing {} failed: {}", job.Name.Slug, outcome.Error);
    return outcome;
}

} // namespace GameEngine::KenneyAssets
