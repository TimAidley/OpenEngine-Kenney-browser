#include "KenneyPackCache.h"

#include "Editor/EditorPaths.h"

#include <algorithm>
#include <system_error>

namespace GameEngine::KenneyAssets
{
namespace
{

std::string CacheFileName(const KenneyPackName& name)
{
    std::string fileName = "kenney_" + name.Slug;
    if (!name.Version.empty())
        fileName += "_" + name.Version;
    return fileName + ".zip";
}

void RemoveOtherVersions(const std::filesystem::path& keep, const std::string& slug)
{
    for (const CachedPack& pack : ListCachedPacks())
    {
        std::error_code ec;
        if (pack.Name.Slug == slug && pack.ZipPath != keep)
            std::filesystem::remove(pack.ZipPath, ec);
    }
}

} // namespace

std::filesystem::path KenneyCacheDirectory()
{
    return Editor::GetEditorGlobalPaths().userCacheRoot / "Kenney";
}

std::filesystem::path AddToCache(const std::filesystem::path& download, const KenneyPackName& name,
                                 std::string& outError)
{
    const std::filesystem::path directory = KenneyCacheDirectory();
    const std::filesystem::path target = directory / CacheFileName(name);
    // A leading dot keeps the partial copy out of ListCachedPacks.
    const std::filesystem::path temporary = directory / ("." + CacheFileName(name) + ".tmp");

    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (!ec)
        std::filesystem::copy_file(download, temporary, std::filesystem::copy_options::overwrite_existing, ec);
    if (!ec)
        std::filesystem::rename(temporary, target, ec);
    if (ec)
    {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        outError = "cannot copy " + download.filename().string() + " to " + directory.string() + ": " + ec.message();
        return {};
    }
    RemoveOtherVersions(target, name.Slug);
    return target;
}

std::vector<CachedPack> ListCachedPacks()
{
    std::vector<CachedPack> packs;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(KenneyCacheDirectory(), ec))
    {
        if (!entry.is_regular_file(ec))
            continue;
        if (auto name = ParseKenneyZipName(entry.path().filename().string()))
            packs.push_back({std::move(*name), entry.path()});
    }
    std::sort(packs.begin(), packs.end(),
              [](const CachedPack& a, const CachedPack& b) { return a.Name.Slug < b.Name.Slug; });
    return packs;
}

} // namespace GameEngine::KenneyAssets
