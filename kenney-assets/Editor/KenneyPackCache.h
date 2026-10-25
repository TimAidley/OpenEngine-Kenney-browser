#pragma once

#include "KenneyPackName.h"

#include <filesystem>
#include <string>
#include <vector>

namespace GameEngine::KenneyAssets
{

/// A pack zip kept in the machine-wide cache, so another project can import it
/// without a new download.
struct CachedPack
{
    KenneyPackName Name;
    std::filesystem::path ZipPath;
};

/// <user cache>/GameEngine/Kenney (on Linux ~/.cache/GameEngine/Kenney).
std::filesystem::path KenneyCacheDirectory();

/// Copies a downloaded zip into the cache as kenney_<slug>[_<version>].zip and
/// removes any other version of the same pack. The copy lands under a temporary
/// name first, so a listing never sees half a file. Worker thread. Returns the
/// cached path, or empty with `outError`.
std::filesystem::path AddToCache(const std::filesystem::path& download, const KenneyPackName& name,
                                 std::string& outError);

/// The cached packs, sorted by slug.
std::vector<CachedPack> ListCachedPacks();

} // namespace GameEngine::KenneyAssets
