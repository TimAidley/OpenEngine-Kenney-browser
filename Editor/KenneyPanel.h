#pragma once

#include "KenneyDownloadWatcher.h"
#include "KenneyPackCache.h"

#include "UI/AssetBoundDockPanel.h"
#include "UI/UIManagerRef.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace GameEngine
{
class Label;
class UIElement;
}

namespace GameEngine::KenneyAssets
{

// Kenney Assets dock panel: opens kenney.nl and the All-in-1 bundle in the
// system browser, imports packs the user downloads while it is open, and lists
// the packs in the machine-wide cache so another project can import them
// without a new download.
class KenneyPanel final : public Editor::AssetBoundDockPanel
{
public:
    KenneyPanel();
    ~KenneyPanel() override;

    void OnPostLayout() override;

protected:
    void OnLayoutBound() override;
    void OnLayoutReconciled() override;

private:
    void ResolveElements();
    void StartWatchingDownloads();
    // Periodic refresh tick: hands finished downloads to the import queue and
    // shows the outcomes of finished imports.
    void Tick();
    void ImportCachedPack(const CachedPack& pack);
    void SetStatus(const std::string& text, bool isError);
    void RefreshPackRows();

    KenneyDownloadWatcher m_Watcher;
    std::filesystem::path m_DownloadDirectory;
    std::string m_WatchStatus;

    WeakRef<Label> m_StatusLabel;
    WeakRef<Label> m_CacheEmptyLabel;
    WeakRef<UIElement> m_PackRows;
    std::string m_LastPackSnapshot; // rebuild rows only when the cache changed

    UIManagerRef m_RefreshManager;
    uint64_t m_RefreshToken = 0;
};

} // namespace GameEngine::KenneyAssets
