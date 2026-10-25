#pragma once

#include <span>
#include <string>
#include <vector>

namespace GameEngine::KenneyAssets
{

/// The entries of a kenney.nl pack that the import keeps: one useful format per
/// asset, no duplicates, no source or engine-project files.
///
/// `entries` are file paths relative to the pack root with '/' separators, as a
/// zip lists them or as a walk of the extracted pack produces them; entries that
/// end in '/' are directories and are ignored. The result keeps the input order.
///
/// Every pack keeps the root .txt files and the root preview and sample images.
/// A 3D pack keeps its glTF/GLB models and the textures beside them (a pack with
/// no glTF keeps its FBX instead). A 2D, UI, icon or audio pack keeps individual
/// PNGs and atlases at the base size, TTF fonts and OGG audio.
std::vector<std::string> SelectImportEntries(std::span<const std::string> entries);

} // namespace GameEngine::KenneyAssets
