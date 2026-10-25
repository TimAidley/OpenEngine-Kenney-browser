#pragma once

#include "KenneyPackName.h"

#include <filesystem>
#include <string>
#include <vector>

namespace GameEngine::KenneyAssets
{

/// The asset-metadata key that records the kenney.nl page an imported file came from.
inline constexpr const char* kSourcePageMetaKey = "kenney.sourcePage";

struct KenneyImportResult
{
    std::vector<std::filesystem::path> Files; ///< imported files, absolute
    std::string Error;                        ///< empty on success
};

/// Imports a pack zip into a project: extracts it to a staging folder in the
/// cache, copies the files SelectImportEntries keeps to
/// <assetsDir>/Kenney/<slug>/ (each at its path inside the pack, so a model's
/// relative texture path still resolves), registers them with the asset
/// pipeline and records the pack's kenney.nl page under kSourcePageMetaKey.
/// Blocks for the whole import; call it from a worker thread.
KenneyImportResult ImportKenneyPack(const std::filesystem::path& zipFile, const KenneyPackName& name,
                                    const std::filesystem::path& assetsDir);

} // namespace GameEngine::KenneyAssets
