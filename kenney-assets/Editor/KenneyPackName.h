#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace GameEngine::KenneyAssets
{

/// A pack as its kenney.nl download names it: "kenney_cube-pets_1.0.zip" is
/// slug "cube-pets", version "1.0"; "kenney_3d-road-tiles.zip" has no version.
struct KenneyPackName
{
    std::string Slug;
    std::string Version;
};

/// Parses a kenney.nl download file name. Accepts the " (1)" a browser adds to a
/// repeated download. Nullopt for any other name, including the browsers'
/// partial-download names (".part", ".crdownload") and the All-in-1 bundle.
std::optional<KenneyPackName> ParseKenneyZipName(std::string_view fileName);

/// The pack's page on kenney.nl.
std::string KenneyPackPageUrl(std::string_view slug);

} // namespace GameEngine::KenneyAssets
