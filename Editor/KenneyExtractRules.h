#pragma once

#include <span>
#include <string>
#include <vector>

namespace GameEngine::KenneyAssets
{

/// One pack file the import keeps, and where it goes inside the pack's asset folder.
struct ImportEntry
{
    std::string Source;      ///< path inside the pack, as given
    std::string Destination; ///< path inside the pack's asset folder
};

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
///
/// The destination drops the folders that only told the formats or sizes apart:
/// "Models", format folders ("GLB format", "gLTF", "PNG") and a base-size folder
/// ("Default") whose large sibling was dropped. "Models/GLB format/fox.glb" and
/// "Models/GLB format/Textures/colormap.png" become "fox.glb" and
/// "Textures/colormap.png", so a model's relative texture path still resolves.
/// Files that would land on the same destination keep their source paths.
std::vector<ImportEntry> SelectImportEntries(std::span<const std::string> entries);

} // namespace GameEngine::KenneyAssets
