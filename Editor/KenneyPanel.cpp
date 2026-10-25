#include "KenneyPanel.h"

#include "KenneyImportQueue.h"

#include "Editor/EditorPaths.h"
#include "Platform/Shell.h"
#include "UI/Controls/Button.h"
#include "UI/Controls/Label.h"
#include "UI/UIManager.h"

#include <memory>
#include <system_error>
#include <vector>

namespace GameEngine::KenneyAssets
{
namespace
{

constexpr const char* kSiteUrl = "https://kenney.nl/assets";
// Where kenney.nl's own "All-in-1" link points.
constexpr const char* kBundleUrl = "https://kenney.itch.io/kenney-game-assets";

// The open project's asset folder ("Assets", or "assets" in an older project);
// empty when no project is open.
std::filesystem::path CurrentProjectAssetsDir()
{
    const std::filesystem::path projectRoot = Editor::GetCurrentEditorProjectPaths().projectRoot;
    if (projectRoot.empty())
        return {};
    std::error_code ec;
    if (std::filesystem::exists(projectRoot / "assets", ec) && !std::filesystem::exists(projectRoot / "Assets", ec))
        return projectRoot / "assets";
    return projectRoot / "Assets";
}

std::string DisplayName(const KenneyPackName& name)
{
    return name.Version.empty() ? name.Slug : name.Slug + " " + name.Version;
}

std::string DescribeOutcome(const KenneyJobOutcome& outcome)
{
    if (!outcome.Error.empty())
        return DisplayName(outcome.Name) + ": " + outcome.Error;
    if (outcome.ImportedFiles == 0)
        return DisplayName(outcome.Name) + " is saved in the download cache. Open a project to import it.";
    return "Imported " + DisplayName(outcome.Name) + " into Assets/Kenney/" + outcome.Name.Slug + " (" +
           std::to_string(outcome.ImportedFiles) + " files).";
}

void WireLink(UIElement* element, const char* url)
{
    if (auto* button = dynamic_cast<Button*>(element))
        button->SetOnClick([url](UIEvent&) { Platform::OpenUrl(url); });
}

} // namespace

KenneyPanel::KenneyPanel()
    : AssetBoundDockPanel("Kenney Assets", "kenney-assets", "Editor/UI/panels/KenneyPanel.uxml",
                          "Editor/UI/panels/KenneyPanel.css")
{
    AddClass("kenney-panel");
}

KenneyPanel::~KenneyPanel()
{
    if (UIManager* manager = m_RefreshManager.Get(); manager && m_RefreshToken != 0)
        manager->UnregisterPeriodicRefresh(m_RefreshToken);
}

void KenneyPanel::OnPostLayout()
{
    AssetBoundDockPanel::OnPostLayout(); // deferred first-show asset bind
}

void KenneyPanel::ResolveElements()
{
    WireLink(FindById("KenneyOpenSite"_sid), kSiteUrl);
    WireLink(FindById("KenneyOpenBundle"_sid), kBundleUrl);
    m_StatusLabel = MakeWeakRef(dynamic_cast<Label*>(FindById("KenneyStatus"_sid)));
    m_CacheEmptyLabel = MakeWeakRef(dynamic_cast<Label*>(FindById("KenneyCacheEmpty"_sid)));
    m_PackRows = MakeWeakRef(FindById("KenneyPackRows"_sid));
}

void KenneyPanel::OnLayoutReconciled()
{
    ResolveElements();
    m_LastPackSnapshot.clear(); // the rows container may be a new, empty element
    SetStatus(m_WatchStatus, false);
    RefreshPackRows();
}

void KenneyPanel::OnLayoutBound()
{
    ResolveElements();
    StartWatchingDownloads();
    RefreshPackRows();

    if (m_RefreshToken == 0)
    {
        if (UIManager* manager = GetOwnerManager())
        {
            m_RefreshManager = UIManagerRef(manager);
            m_RefreshToken = manager->RegisterPeriodicRefresh([this]() { Tick(); });
        }
    }
}

void KenneyPanel::StartWatchingDownloads()
{
    m_DownloadDirectory = FindDownloadDirectory();
    if (m_DownloadDirectory.empty())
    {
        m_WatchStatus = "Cannot find your download folder: set XDG_DOWNLOAD_DIR or install xdg-user-dirs.";
        SetStatus(m_WatchStatus, true);
        return;
    }
    if (!m_Watcher.Start(m_DownloadDirectory))
    {
        m_WatchStatus = "Cannot watch " + m_DownloadDirectory.string() + ".";
        SetStatus(m_WatchStatus, true);
        return;
    }
    m_WatchStatus = "Watching " + m_DownloadDirectory.string() + " for kenney.nl downloads.";
    SetStatus(m_WatchStatus, false);
}

void KenneyPanel::Tick()
{
    if (!IsLayoutBound())
        return;

    for (const std::filesystem::path& download : m_Watcher.TakeFinishedDownloads(std::chrono::steady_clock::now()))
    {
        auto name = ParseKenneyZipName(download.filename().string());
        if (!name)
            continue;
        SetStatus("Importing " + DisplayName(*name) + "...", false);
        KenneyImportQueue::Get().Enqueue({download, std::move(*name), true, CurrentProjectAssetsDir()});
    }

    const std::vector<KenneyJobOutcome> outcomes = KenneyImportQueue::Get().TakeOutcomes();
    for (const KenneyJobOutcome& outcome : outcomes)
        SetStatus(DescribeOutcome(outcome), !outcome.Error.empty());
    if (!outcomes.empty())
        RefreshPackRows();
}

void KenneyPanel::ImportCachedPack(const CachedPack& pack)
{
    const std::filesystem::path assetsDir = CurrentProjectAssetsDir();
    if (assetsDir.empty())
    {
        SetStatus("Open a project to import " + DisplayName(pack.Name) + ".", true);
        return;
    }
    SetStatus("Importing " + DisplayName(pack.Name) + "...", false);
    KenneyImportQueue::Get().Enqueue({pack.ZipPath, pack.Name, false, assetsDir});
}

void KenneyPanel::SetStatus(const std::string& text, bool isError)
{
    Label* label = m_StatusLabel.Get();
    if (!label)
        return;
    label->SetText(text);
    if (isError)
        label->AddClass("error");
    else
        label->RemoveClass("error");
}

void KenneyPanel::RefreshPackRows()
{
    UIElement* rows = m_PackRows.Get();
    if (!rows)
        return;

    const std::vector<CachedPack> packs = ListCachedPacks();
    std::string snapshot{'\1'}; // sentinel: never equals the default-empty member
    for (const CachedPack& pack : packs)
        snapshot += pack.ZipPath.string() + '\0';
    if (snapshot == m_LastPackSnapshot)
        return;
    m_LastPackSnapshot = std::move(snapshot);

    if (Label* empty = m_CacheEmptyLabel.Get())
    {
        if (packs.empty())
            empty->RemoveClass("hidden");
        else
            empty->AddClass("hidden");
    }

    rows->RemoveAllChildren();
    for (const CachedPack& pack : packs)
    {
        auto row = std::make_unique<UIElement>();
        row->AddClass("kenney-pack-row");

        auto name = std::make_unique<Label>();
        name->SetText(DisplayName(pack.Name));
        name->AddClass("kenney-pack-name");
        row->AddChild(std::move(name));

        auto import = std::make_unique<Button>();
        import->SetText("Import");
        import->AddClass("small");
        import->AddClass("secondary");
        import->SetOnClick([this, pack](UIEvent&) { ImportCachedPack(pack); });
        row->AddChild(std::move(import));

        rows->AddChild(std::move(row));
    }
}

} // namespace GameEngine::KenneyAssets
