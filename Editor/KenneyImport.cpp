#include "KenneyImport.h"

#include "KenneyExtractRules.h"
#include "KenneyPackCache.h"
#include "KenneyZip.h"

#include "Assets/AssetManager.h"
#include "Assets/AssetRegistry.h"
#include "Core/Engine.h"
#include "Editor/Registries/EditorSceneCommands.h"
#include "Logger/Logger.h"
#include "Platform/Process.h"

#include <system_error>

namespace GameEngine::KenneyAssets
{
namespace
{

// Regular files under `root` as '/'-separated relative paths. Symbolic links
// are skipped: a zip can carry them, and following one could read or write
// outside the staging folder.
std::vector<std::string> ListPackFiles(const std::filesystem::path& root)
{
    std::vector<std::string> files;
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
    {
        if (it->is_symlink(ec) || !it->is_regular_file(ec))
            continue;
        files.push_back(std::filesystem::relative(it->path(), root, ec).generic_string());
    }
    return files;
}

bool CopyKeptFiles(const std::filesystem::path& staging, const std::vector<std::string>& kept,
                   const std::filesystem::path& destination, KenneyImportResult& result)
{
    for (const std::string& relative : kept)
    {
        const std::filesystem::path target = destination / std::filesystem::path(relative);
        std::error_code ec;
        std::filesystem::create_directories(target.parent_path(), ec);
        if (!ec)
            std::filesystem::copy_file(staging / std::filesystem::path(relative), target,
                                       std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
        {
            result.Error = "cannot write " + target.string() + ": " + ec.message();
            return false;
        }
        result.Files.push_back(target);
    }
    return true;
}

void RecordSourcePage(const std::vector<std::filesystem::path>& files, const std::string& pageUrl)
{
    const size_t unregistered = Editor::EditorSceneCommands::Get().RegisterImportedAssets(files);
    if (unregistered > 0)
        Logger::Log::Warning("Kenney: {} of {} imported file(s) are registered at the next asset scan",
                             unregistered, files.size());

    AssetManager* assets = EngineCore::GetInstance().TryGetAssetManager();
    if (!assets)
        return;
    for (const std::filesystem::path& file : files)
        assets->GetRegistry().SetMetaValue(file, kSourcePageMetaKey, pageUrl);
}

} // namespace

KenneyImportResult ImportKenneyPack(const std::filesystem::path& zipFile, const KenneyPackName& name,
                                    const std::filesystem::path& assetsDir)
{
    KenneyImportResult result;
    const std::filesystem::path staging = KenneyCacheDirectory() / ".staging" /
                                          (name.Slug + "-" + std::to_string(Platform::GetCurrentProcessId()));
    std::error_code ec;
    std::filesystem::remove_all(staging, ec);

    if (ExtractZip(zipFile, staging, result.Error))
    {
        const std::vector<std::string> kept = SelectImportEntries(ListPackFiles(staging));
        if (kept.empty())
            result.Error = "the pack holds no files to import";
        else
            CopyKeptFiles(staging, kept, assetsDir / "Kenney" / name.Slug, result);
    }
    std::filesystem::remove_all(staging, ec);
    std::filesystem::remove(staging.parent_path(), ec); // only succeeds once empty

    if (!result.Files.empty())
        RecordSourcePage(result.Files, KenneyPackPageUrl(name.Slug));
    return result;
}

} // namespace GameEngine::KenneyAssets
