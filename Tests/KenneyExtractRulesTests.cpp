#include "KenneyExtractRules.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using GameEngine::KenneyAssets::ImportEntry;
using GameEngine::KenneyAssets::SelectImportEntries;

namespace
{

// A fixture is the entry list of a real kenney.nl download (`unzip -Z1`).
std::vector<std::string> LoadFixture(const std::string& name)
{
    std::ifstream in(std::filesystem::path(KENNEY_FIXTURES_DIR) / (name + ".txt"));
    std::vector<std::string> entries;
    for (std::string line; std::getline(in, line);)
    {
        if (!line.empty())
            entries.push_back(line);
    }
    return entries;
}

size_t CountIf(const std::vector<std::string>& paths, bool (*predicate)(const std::string&))
{
    return static_cast<size_t>(std::count_if(paths.begin(), paths.end(), predicate));
}

bool HasExt(const std::string& path, const char* ext)
{
    const std::string lower = [&] {
        std::string s = path;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }();
    return lower.ends_with(ext);
}

std::vector<std::string> Sources(const std::vector<ImportEntry>& entries)
{
    std::vector<std::string> sources;
    for (const ImportEntry& entry : entries)
        sources.push_back(entry.Source);
    return sources;
}

std::vector<std::string> Destinations(const std::vector<ImportEntry>& entries)
{
    std::vector<std::string> destinations;
    for (const ImportEntry& entry : entries)
        destinations.push_back(entry.Destination);
    return destinations;
}

bool Contains(const std::vector<std::string>& paths, const std::string& path)
{
    return std::find(paths.begin(), paths.end(), path) != paths.end();
}

} // namespace

TEST(KenneyExtractRules, NewLayoutPackKeepsGlbTexturesAndRootFilesOnly)
{
    const auto kept = Sources(SelectImportEntries(LoadFixture("kenney_modular-buildings")));
    EXPECT_EQ(CountIf(kept, [](const std::string& p) { return HasExt(p, ".glb"); }), 108u);
    EXPECT_TRUE(Contains(kept, "Models/GLB format/Textures/colormap.png"));
    EXPECT_FALSE(Contains(kept, "Models/FBX format/Textures/colormap.png"));
    EXPECT_TRUE(Contains(kept, "License.txt"));
    EXPECT_TRUE(Contains(kept, "Preview.png"));
    EXPECT_TRUE(Contains(kept, "Preview (Variation A).png"));
    EXPECT_TRUE(Contains(kept, "Sample.png"));
    EXPECT_FALSE(Contains(kept, "Overview.html"));
    EXPECT_FALSE(Contains(kept, "Visit Kenney.url"));
    for (const std::string& path : kept)
    {
        EXPECT_FALSE(path.starts_with("Previews/")) << path;
        EXPECT_FALSE(HasExt(path, ".fbx") || HasExt(path, ".obj") || HasExt(path, ".mtl")) << path;
    }
}

TEST(KenneyExtractRules, NewLayoutPackKeepsVariationTextures)
{
    const auto entries = LoadFixture("kenney_modular-buildings");
    const auto kept = Sources(SelectImportEntries(entries));
    size_t variations = 0;
    for (const std::string& entry : entries)
    {
        if (entry.starts_with("Models/Textures/") && entry.back() != '/')
        {
            ++variations;
            EXPECT_TRUE(Contains(kept, entry)) << entry;
        }
    }
    EXPECT_GT(variations, 0u);
}

TEST(KenneyExtractRules, OldLayoutPackKeepsGlbFromGltfFormatFolderAndDropsSprites)
{
    const auto kept = Sources(SelectImportEntries(LoadFixture("kenney_city-kit-commercial")));
    EXPECT_EQ(CountIf(kept, [](const std::string& p) { return HasExt(p, ".glb"); }), 56u);
    for (const std::string& path : kept)
    {
        const bool root = path.find('/') == std::string::npos;
        EXPECT_TRUE(root || path.starts_with("Models/GLTF format/")) << path;
    }
}

TEST(KenneyExtractRules, OldLayoutPackDropsTexturesOfOtherFormats)
{
    const auto kept = Sources(SelectImportEntries(LoadFixture("kenney_retro-urban-kit")));
    EXPECT_EQ(CountIf(kept, [](const std::string& p) { return HasExt(p, ".glb"); }), 100u);
    for (const std::string& path : kept)
    {
        EXPECT_EQ(path.find("Textures"), std::string::npos) << path;
        EXPECT_FALSE(path.starts_with("Isometric/") || path.starts_with("Side/")) << path;
    }
}

TEST(KenneyExtractRules, GltfOnlyPackKeepsGltfAndDropsScripts)
{
    const auto kept = Sources(SelectImportEntries(LoadFixture("kenney_3d-road-tiles")));
    EXPECT_EQ(CountIf(kept, [](const std::string& p) { return HasExt(p, ".gltf"); }), 302u);
    for (const std::string& path : kept)
        EXPECT_FALSE(path.starts_with("Convert/") || path.starts_with("Unity package/")) << path;
}

TEST(KenneyExtractRules, FlatPackKeepsPngsAndAtlasesButNotVectors)
{
    const auto kept = Sources(SelectImportEntries(LoadFixture("kenney_domino-pack")));
    EXPECT_EQ(CountIf(kept, [](const std::string& p) { return HasExt(p, ".svg"); }), 0u);
    EXPECT_EQ(CountIf(kept, [](const std::string& p) { return p.starts_with("PNG/"); }), 145u);
    EXPECT_GT(CountIf(kept, [](const std::string& p) { return p.starts_with("Tilesheets/"); }), 0u);
}

TEST(KenneyExtractRules, FlatPackDropsRetinaFolderAndVectorSources)
{
    const auto kept = Sources(SelectImportEntries(LoadFixture("kenney_tower-defense-top-down")));
    EXPECT_EQ(CountIf(kept, [](const std::string& p) { return p.starts_with("PNG/Default size/"); }), 299u);
    for (const std::string& path : kept)
    {
        EXPECT_FALSE(path.starts_with("PNG/Retina/")) << path;
        EXPECT_FALSE(path.starts_with("Vector/")) << path;
    }
    EXPECT_TRUE(Contains(kept, "Preview_KenneyNL.png"));
}

TEST(KenneyExtractRules, BothGlbAndGltfFormatFoldersPreferGlbAndKeepUniqueGltfModels)
{
    const std::vector<std::string> entries = {
        "License.txt",
        "Models/GLB format/wall.glb",
        "Models/GLB format/Textures/colormap.png",
        "Models/GLTF format/wall.glb",
        "Models/GLTF format/tower.glb",
        "Models/FBX format/wall.fbx",
    };
    const auto kept = Sources(SelectImportEntries(entries));
    EXPECT_EQ(kept, (std::vector<std::string>{"License.txt", "Models/GLB format/wall.glb",
                                              "Models/GLB format/Textures/colormap.png",
                                              "Models/GLTF format/tower.glb"}));
}

TEST(KenneyExtractRules, FbxOnlyPackKeepsFbxAndSkinsButNotSources)
{
    const std::vector<std::string> entries = {
        "License.txt",
        "Preview.png",
        "Models/characterMedium.fbx",
        "Models/Source/characterMedium.blend",
        "Animations/run.fbx",
        "Skins/survivorMaleA.png",
        "Skins/Source/survivorMaleA.psd",
        "Preview/run.png",
    };
    const auto kept = Sources(SelectImportEntries(entries));
    EXPECT_EQ(kept, (std::vector<std::string>{"License.txt", "Preview.png", "Models/characterMedium.fbx",
                                              "Animations/run.fbx", "Skins/survivorMaleA.png"}));
}

TEST(KenneyExtractRules, AtlasSizeSuffixKeepsBaseSheetOnly)
{
    const std::vector<std::string> entries = {
        "Spritesheet/sheet_white1x.png",
        "Spritesheet/sheet_white1x.xml",
        "Spritesheet/sheet_white2x.png",
        "Spritesheet/sheet_white2x.xml",
        "Spritesheet/spritesheet_default.png",
        "Spritesheet/spritesheet_retina.png",
        "Tilemap/tilemap.png",
        "Tilemap/tilemap_packed.png",
    };
    const auto kept = Sources(SelectImportEntries(entries));
    EXPECT_EQ(kept, (std::vector<std::string>{"Spritesheet/sheet_white1x.png", "Spritesheet/sheet_white1x.xml",
                                              "Spritesheet/spritesheet_default.png", "Tilemap/tilemap.png",
                                              "Tilemap/tilemap_packed.png"}));
}

TEST(KenneyExtractRules, FontsPreferTtfAndKeepGlyphMaps)
{
    const std::vector<std::string> entries = {
        "Generic/Fonts/kenney_input_generic.ttf",
        "Generic/Fonts/kenney_input_generic.otf",
        "Generic/Fonts/kenney_input_generic_map.txt",
        "Generic/Default/button.png",
        "Generic/Double/button.png",
        "Generic/Vector/button.svg",
        "Generic/sprites.xml",
        "Generic/generic_sheet_default.png",
        "Generic/generic_sheet_default.xml",
        "Generic/generic_sheet_double.png",
        "Generic/generic_sheet_double.xml",
    };
    const auto kept = Sources(SelectImportEntries(entries));
    EXPECT_EQ(kept, (std::vector<std::string>{"Generic/Fonts/kenney_input_generic.ttf",
                                              "Generic/Fonts/kenney_input_generic_map.txt",
                                              "Generic/Default/button.png", "Generic/generic_sheet_default.png",
                                              "Generic/generic_sheet_default.xml"}));
}

TEST(KenneyExtractRules, AudioPackKeepsOggAndPreviewSound)
{
    const std::vector<std::string> entries = {"License.txt", "Preview.ogg", "Audio/click.ogg", "Audio/click.wav"};
    const auto kept = Sources(SelectImportEntries(entries));
    EXPECT_EQ(kept, (std::vector<std::string>{"License.txt", "Preview.ogg", "Audio/click.ogg"}));
}

TEST(KenneyExtractRules, VectorOnlyPackKeepsSvg)
{
    const std::vector<std::string> entries = {"License.txt", "Vector/a.svg", "Vector/a.swf"};
    const auto kept = Sources(SelectImportEntries(entries));
    EXPECT_EQ(kept, (std::vector<std::string>{"License.txt", "Vector/a.svg"}));
}

TEST(KenneyExtractRules, DropsMacMetadataAndDirectories)
{
    const std::vector<std::string> entries = {"PNG/", "PNG/a.png", "__MACOSX/PNG/._a.png", "PNG/.DS_Store"};
    EXPECT_EQ(Sources(SelectImportEntries(entries)), (std::vector<std::string>{"PNG/a.png"}));
}

TEST(KenneyExtractRules, DestinationDropsModelsAndFormatFolders)
{
    const auto destinations = Destinations(SelectImportEntries(LoadFixture("kenney_modular-buildings")));
    EXPECT_TRUE(Contains(destinations, "Textures/colormap.png"));
    EXPECT_TRUE(Contains(destinations, "Textures/variation-a.png"));
    EXPECT_TRUE(Contains(destinations, "License.txt"));
    for (const std::string& path : destinations)
    {
        EXPECT_EQ(path.find("Models"), std::string::npos) << path;
        EXPECT_EQ(path.find("format"), std::string::npos) << path;
        if (HasExt(path, ".glb"))
            EXPECT_EQ(path.find('/'), std::string::npos) << path;
    }
}

TEST(KenneyExtractRules, DestinationDropsBareGltfFolder)
{
    const auto entries = SelectImportEntries(LoadFixture("kenney_3d-road-tiles"));
    for (const ImportEntry& entry : entries)
        EXPECT_EQ(entry.Destination.find('/'), std::string::npos) << entry.Destination;
}

TEST(KenneyExtractRules, DestinationDropsPngAndBaseSizeFolders)
{
    const std::vector<std::string> entries = {
        "PNG/Default size/tile.png",
        "PNG/Retina/tile.png",
        "Generic/Default/button.png",
        "Generic/Double/button.png",
        "Tiles/Large tiles/Thick outline/a.png",
        "Tilesheets/sheet.png",
    };
    EXPECT_EQ(Destinations(SelectImportEntries(entries)),
              (std::vector<std::string>{"tile.png", "Generic/button.png", "Tiles/Large tiles/Thick outline/a.png",
                                        "Tilesheets/sheet.png"}));
}

TEST(KenneyExtractRules, DestinationKeepsDefaultFolderWithoutLargeSibling)
{
    const std::vector<std::string> entries = {"Tiles/Default/a.png", "Tiles/Transparent/a.png"};
    EXPECT_EQ(Destinations(SelectImportEntries(entries)),
              (std::vector<std::string>{"Tiles/Default/a.png", "Tiles/Transparent/a.png"}));
}

TEST(KenneyExtractRules, CollidingDestinationsKeepSourcePaths)
{
    const std::vector<std::string> entries = {
        "Models/GLB format/Textures/colormap.png",
        "Models/GLTF format/Textures/colormap.png",
        "Models/GLB format/wall.glb",
        "Models/GLTF format/tower.glb",
    };
    EXPECT_EQ(Destinations(SelectImportEntries(entries)),
              (std::vector<std::string>{"Models/GLB format/Textures/colormap.png",
                                        "Models/GLTF format/Textures/colormap.png", "wall.glb", "tower.glb"}));
}

// Every pack of an unzipped All-in-1 bundle: the import keeps License.txt, no
// model duplicates, no vectors beside PNGs and no large-size folders.
TEST(KenneyExtractRules, BundlePacksFollowTheRules)
{
    const char* bundle = std::getenv("KENNEY_BUNDLE_DIR");
    if (bundle == nullptr)
        GTEST_SKIP() << "KENNEY_BUNDLE_DIR is not set";

    namespace fs = std::filesystem;
    size_t packs = 0;
    for (const fs::directory_entry& category : fs::directory_iterator(bundle))
    {
        if (!category.is_directory())
            continue;
        for (const fs::directory_entry& pack : fs::directory_iterator(category.path()))
        {
            if (!fs::exists(pack.path() / "License.txt"))
                continue;
            ++packs;
            std::vector<std::string> entries;
            for (const fs::directory_entry& file : fs::recursive_directory_iterator(pack.path()))
            {
                if (file.is_regular_file())
                    entries.push_back(fs::relative(file.path(), pack.path()).generic_string());
            }
            const auto selected = SelectImportEntries(entries);
            const auto kept = Sources(selected);
            std::vector<std::string> destinations = Destinations(selected);
            std::sort(destinations.begin(), destinations.end());
            EXPECT_EQ(std::adjacent_find(destinations.begin(), destinations.end()), destinations.end())
                << pack.path().filename().string() << ": two files share a destination";
            const std::string label = pack.path().filename().string();
            EXPECT_TRUE(Contains(kept, "License.txt")) << label;

            const bool hasGl = std::any_of(entries.begin(), entries.end(),
                                           [](const std::string& e) { return HasExt(e, ".glb") || HasExt(e, ".gltf"); });
            const bool hasPng = std::any_of(entries.begin(), entries.end(), [](const std::string& e) {
                return HasExt(e, ".png") && e.find('/') != std::string::npos;
            });
            for (const std::string& path : kept)
            {
                if (hasGl)
                    EXPECT_FALSE(HasExt(path, ".fbx") || HasExt(path, ".obj")) << label << ": " << path;
                if (hasPng)
                    EXPECT_FALSE(HasExt(path, ".svg")) << label << ": " << path;
                EXPECT_FALSE(HasExt(path, ".url") || HasExt(path, ".html") || HasExt(path, ".zip")) << label << ": " << path;
                EXPECT_EQ(path.find("/Retina/"), std::string::npos) << label << ": " << path;
                EXPECT_FALSE(path.find("Double/") != std::string::npos && path.find("Default/") == std::string::npos)
                    << label << ": " << path;
            }
            if (std::getenv("KENNEY_DUMP") != nullptr)
                std::printf("%-50s %6zu -> %6zu\n", (category.path().filename().string() + "/" + label).c_str(),
                            entries.size(), kept.size());
        }
    }
    EXPECT_GT(packs, 0u);
}
