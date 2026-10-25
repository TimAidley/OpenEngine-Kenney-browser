#include "KenneyExtractRules.h"

#include "Types/StringUtils.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string_view>

namespace GameEngine::KenneyAssets
{
namespace
{

// One pack file split into the parts the rules test. Dirs, Stem and Ext are
// lower case; Path keeps the original spelling.
struct PackFile
{
    std::string Path;
    std::string Dir; // lower-case parent path, "" for the pack root
    std::vector<std::string> Dirs;
    std::string Stem;
    std::string Ext;
};

enum class FormatFolder : uint8_t
{
    None,
    Gltf,
    Other,
};

constexpr std::string_view kModelExts[] = {"glb", "gltf", "fbx", "obj", "dae", "stl", "blend"};
constexpr std::string_view kImageExts[] = {"png", "jpg", "jpeg", "tga", "webp"};
constexpr std::string_view kAudioExts[] = {"ogg", "wav", "mp3", "flac"};
constexpr std::string_view kAtlasFolderPrefixes[] = {"spritesheet", "tilesheet", "tilemap"};

template <size_t N>
bool IsOneOf(std::string_view value, const std::string_view (&set)[N])
{
    return std::find(std::begin(set), std::end(set), value) != std::end(set);
}

PackFile SplitPath(const std::string& path)
{
    PackFile file;
    file.Path = path;
    const std::vector<std::string_view> parts = Split(path, '/');
    for (size_t i = 0; i + 1 < parts.size(); ++i)
    {
        if (!parts[i].empty())
            file.Dirs.push_back(ToLowerAscii(parts[i]));
    }
    file.Dir = Join(file.Dirs, "/");
    const std::string name = ToLowerAscii(parts.empty() ? std::string_view{} : parts.back());
    const size_t dot = name.rfind('.');
    file.Stem = dot == std::string::npos ? name : name.substr(0, dot);
    file.Ext = dot == std::string::npos ? std::string{} : name.substr(dot + 1);
    return file;
}

bool IsJunk(const PackFile& file)
{
    for (const std::string& dir : file.Dirs)
    {
        if (dir == "__macosx")
            return true;
    }
    // An empty stem is a dot file (".DS_Store"); "._name" is a macOS resource fork.
    return file.Stem.empty() || file.Stem.starts_with("._") || (file.Stem == "thumbs" && file.Ext == "db");
}

bool HasDir(const PackFile& file, std::string_view name)
{
    return std::find(file.Dirs.begin(), file.Dirs.end(), name) != file.Dirs.end();
}

// "GLB format", "GLTF format", "glTF format" and a bare "gLTF" hold glTF models;
// "FBX format", "OBJ format" and the rest hold the duplicates the import skips.
FormatFolder ClassifyFormatFolder(std::string_view dir)
{
    if (dir == "gltf" || dir == "glb")
        return FormatFolder::Gltf;
    if (!dir.ends_with(" format"))
        return FormatFolder::None;
    const std::string_view kind = dir.substr(0, dir.size() - std::string_view(" format").size());
    return (kind == "gltf" || kind == "glb") ? FormatFolder::Gltf : FormatFolder::Other;
}

bool InFormatFolder(const PackFile& file, FormatFolder kind)
{
    return std::any_of(file.Dirs.begin(), file.Dirs.end(),
                       [kind](const std::string& dir) { return ClassifyFormatFolder(dir) == kind; });
}

bool InGlbFormatFolder(const PackFile& file)
{
    return HasDir(file, "glb format") || HasDir(file, "glb");
}

bool IsRootKeeper(const PackFile& file)
{
    if (file.Ext == "txt")
        return true;
    const bool previewName = file.Stem.starts_with("preview") || file.Stem.starts_with("sample");
    return previewName && (file.Ext == "png" || file.Ext == "ogg");
}

// A folder holding the doubled-resolution copy of its sibling: "Double",
// "Double (128px)", "Retina", "2x", "Sprites X2", "Large (2×)".
bool IsLargeVariantFolder(std::string_view dir)
{
    return dir == "double" || dir.starts_with("double ") || dir == "retina" || dir == "2x" ||
           dir == "sprites x2" || dir.find("(2x)") != std::string_view::npos ||
           dir.find("(2\xc3\x97)") != std::string_view::npos; // U+00D7 multiplication sign
}

// The stem with a size suffix removed ("sheet_white2x" and "sheet_white1x" give
// "sheet_white", "spritesheet_retina" gives "spritesheet"); `outLarge` says
// whether the suffix named the large copy.
std::string CanonicalSizeStem(const std::string& stem, bool& outLarge)
{
    constexpr std::pair<std::string_view, bool> kSuffixes[] = {
        {"@2x", true}, {"_retina", true}, {"-retina", true}, {"_double", true}, {"_2x", true},
        {"2x", true}, {"_1x", false}, {"1x", false}, {"_default", false},
    };
    outLarge = false;
    for (const auto& [suffix, large] : kSuffixes)
    {
        if (stem.size() > suffix.size() && stem.ends_with(suffix))
        {
            outLarge = large;
            return stem.substr(0, stem.size() - suffix.size());
        }
    }
    return stem;
}

// Directories (as lower-case paths) that are a large-variant copy of a sibling
// folder. A "Double" folder with no other folder beside it is the only copy and
// stays.
std::set<std::string> FindLargeVariantDirs(const std::vector<PackFile>& files)
{
    std::map<std::string, std::set<std::string>> children;
    for (const PackFile& file : files)
    {
        std::string parent;
        for (const std::string& dir : file.Dirs)
        {
            children[parent].insert(dir);
            parent = parent.empty() ? dir : parent + "/" + dir;
        }
    }

    std::set<std::string> large;
    for (const auto& [parent, dirs] : children)
    {
        const bool hasBase = std::any_of(dirs.begin(), dirs.end(),
                                         [](const std::string& dir) { return !IsLargeVariantFolder(dir); });
        if (!hasBase)
            continue;
        for (const std::string& dir : dirs)
        {
            if (IsLargeVariantFolder(dir))
                large.insert(parent.empty() ? dir : parent + "/" + dir);
        }
    }
    return large;
}

bool UnderAnyDir(const PackFile& file, const std::set<std::string>& dirs)
{
    std::string path;
    for (const std::string& dir : file.Dirs)
    {
        path = path.empty() ? dir : path + "/" + dir;
        if (dirs.contains(path))
            return true;
    }
    return false;
}

bool InAtlasFolder(const PackFile& file)
{
    return std::any_of(file.Dirs.begin(), file.Dirs.end(), [](const std::string& dir) {
        return std::any_of(std::begin(kAtlasFolderPrefixes), std::end(kAtlasFolderPrefixes),
                           [&dir](std::string_view prefix) { return dir.starts_with(prefix); });
    });
}

// --- 3D packs ---------------------------------------------------------------

bool KeepGltfPackFile(const PackFile& file)
{
    if (InFormatFolder(file, FormatFolder::Other) || HasDir(file, "previews"))
        return false;
    if (file.Ext == "glb" || file.Ext == "gltf" || file.Ext == "bin")
        return true;
    return IsOneOf(file.Ext, kImageExts) && (InFormatFolder(file, FormatFolder::Gltf) || HasDir(file, "textures"));
}

bool KeepFbxPackFile(const PackFile& file)
{
    for (const char* skipped : {"source", "sources", "preview", "previews"})
    {
        if (HasDir(file, skipped))
            return false;
    }
    return file.Ext == "fbx" || IsOneOf(file.Ext, kImageExts);
}

// A pack with both "GLB format" and "GLTF format" folders ships some models
// twice: the GLB folder wins, and a model from elsewhere stays only when its
// name is not in the GLB folder. A .gltf with the same name as a .glb also goes,
// together with its .bin.
void DropDuplicateModels(const std::vector<PackFile>& files, std::vector<bool>& keep)
{
    std::set<std::string> glbFolderStems;
    std::set<std::string> glbFileKeys; // dir + stem of every kept .glb
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (!keep[i] || files[i].Ext != "glb")
            continue;
        if (InGlbFormatFolder(files[i]))
            glbFolderStems.insert(files[i].Stem);
        glbFileKeys.insert(files[i].Dir + "/" + files[i].Stem);
    }

    for (size_t i = 0; i < files.size(); ++i)
    {
        const PackFile& file = files[i];
        if (!keep[i] || (file.Ext != "glb" && file.Ext != "gltf" && file.Ext != "bin"))
            continue;
        if (file.Ext != "glb" && glbFileKeys.contains(file.Dir + "/" + file.Stem))
            keep[i] = false;
        else if (!InGlbFormatFolder(file) && glbFolderStems.contains(file.Stem))
            keep[i] = false;
    }
}

void Select3DPack(const std::vector<PackFile>& files, std::vector<bool>& keep)
{
    const bool hasGltf = std::any_of(files.begin(), files.end(),
                                     [](const PackFile& file) { return file.Ext == "glb" || file.Ext == "gltf"; });
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (!files[i].Dirs.empty())
            keep[i] = hasGltf ? KeepGltfPackFile(files[i]) : KeepFbxPackFile(files[i]);
    }
    if (hasGltf)
        DropDuplicateModels(files, keep);
}

// --- 2D, UI, icon and audio packs -------------------------------------------

// `pngKeys` holds dir + "/" + stem of every PNG in the pack: an XML file is an
// atlas description when it sits in an atlas folder or beside its sheet.
bool KeepFlatPackFile(const PackFile& file, bool keepVectors, const std::set<std::string>& pngKeys)
{
    for (const char* toolFolder : {"tiled", "construct 3", "unity samples", "godot source", "source", "sources"})
    {
        if (HasDir(file, toolFolder))
            return false;
    }
    if (file.Ext == "txt")
        return true;
    if (IsOneOf(file.Ext, kImageExts))
        return !HasDir(file, "vector");
    if (file.Ext == "svg")
        return keepVectors;
    if (file.Ext == "xml")
        return InAtlasFolder(file) || pngKeys.contains(file.Dir + "/" + file.Stem);
    return file.Ext == "ttf" || file.Ext == "otf" || IsOneOf(file.Ext, kAudioExts);
}

// Within one folder: the large copy of a file whose base copy exists, an OTF
// whose TTF exists, and a WAV or MP3 whose OGG exists all go.
void DropFlatDuplicates(const std::vector<PackFile>& files, std::vector<bool>& keep)
{
    std::set<std::string> baseSizeKeys;
    std::set<std::string> keptKeys; // dir + stem + ext
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (!keep[i])
            continue;
        const PackFile& file = files[i];
        bool large = false;
        const std::string canonical = CanonicalSizeStem(file.Stem, large);
        if (!large)
            baseSizeKeys.insert(file.Dir + "/" + canonical + "." + file.Ext);
        keptKeys.insert(file.Dir + "/" + file.Stem + "." + file.Ext);
    }

    for (size_t i = 0; i < files.size(); ++i)
    {
        if (!keep[i])
            continue;
        const PackFile& file = files[i];
        const std::string key = file.Dir + "/" + file.Stem + ".";
        bool large = false;
        const std::string canonical = CanonicalSizeStem(file.Stem, large);
        if (large && baseSizeKeys.contains(file.Dir + "/" + canonical + "." + file.Ext))
            keep[i] = false;
        else if (file.Ext == "otf" && keptKeys.contains(key + "ttf"))
            keep[i] = false;
        else if ((file.Ext == "wav" || file.Ext == "mp3" || file.Ext == "flac") && keptKeys.contains(key + "ogg"))
            keep[i] = false;
    }
}

void SelectFlatPack(const std::vector<PackFile>& files, std::vector<bool>& keep)
{
    std::set<std::string> pngKeys;
    for (const PackFile& file : files)
    {
        if (file.Ext == "png" && !file.Dirs.empty())
            pngKeys.insert(file.Dir + "/" + file.Stem);
    }
    const std::set<std::string> largeDirs = FindLargeVariantDirs(files);
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (!files[i].Dirs.empty())
            keep[i] = !UnderAnyDir(files[i], largeDirs) && KeepFlatPackFile(files[i], pngKeys.empty(), pngKeys);
    }
    DropFlatDuplicates(files, keep);
}

// --- Destination paths ------------------------------------------------------

bool IsBaseSizeFolder(std::string_view dir)
{
    return dir == "default" || dir == "default size" || dir.starts_with("default (") || dir == "sprites" ||
           dir == "1x";
}

// Whether the folder at Dirs[0..index] only told formats or sizes apart.
bool IsDroppedFromDestination(const PackFile& file, size_t index, const std::set<std::string>& largeDirs)
{
    const std::string& dir = file.Dirs[index];
    if (dir == "models" || dir == "model" || dir == "png" || ClassifyFormatFolder(dir) != FormatFolder::None)
        return true;
    if (!IsBaseSizeFolder(dir))
        return false;
    std::string parent;
    for (size_t i = 0; i < index; ++i)
        parent += file.Dirs[i] + "/";
    return std::any_of(largeDirs.begin(), largeDirs.end(), [&parent](const std::string& large) {
        return large.starts_with(parent) && large.find('/', parent.size()) == std::string::npos;
    });
}

std::string DestinationPath(const PackFile& file, const std::set<std::string>& largeDirs)
{
    const std::vector<std::string_view> parts = Split(file.Path, '/');
    std::vector<std::string_view> kept;
    size_t dirIndex = 0;
    for (size_t i = 0; i + 1 < parts.size(); ++i)
    {
        if (parts[i].empty())
            continue;
        if (!IsDroppedFromDestination(file, dirIndex, largeDirs))
            kept.push_back(parts[i]);
        ++dirIndex;
    }
    kept.push_back(parts.back());
    return Join(kept, "/");
}

std::vector<ImportEntry> BuildImportEntries(const std::vector<PackFile>& files, const std::vector<bool>& keep)
{
    const std::set<std::string> largeDirs = FindLargeVariantDirs(files);
    std::vector<ImportEntry> entries;
    std::map<std::string, size_t> destinationCounts; // lower case: the asset folder may not tell case apart
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (!keep[i])
            continue;
        entries.push_back({files[i].Path, DestinationPath(files[i], largeDirs)});
        ++destinationCounts[ToLowerAscii(entries.back().Destination)];
    }
    for (ImportEntry& entry : entries)
    {
        if (destinationCounts[ToLowerAscii(entry.Destination)] > 1)
            entry.Destination = entry.Source;
    }
    return entries;
}

} // namespace

std::vector<ImportEntry> SelectImportEntries(std::span<const std::string> entries)
{
    std::vector<PackFile> files;
    files.reserve(entries.size());
    for (const std::string& entry : entries)
    {
        if (entry.empty() || entry.back() == '/')
            continue;
        PackFile file = SplitPath(entry);
        if (!IsJunk(file))
            files.push_back(std::move(file));
    }

    std::vector<bool> keep(files.size(), false);
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (files[i].Dirs.empty())
            keep[i] = IsRootKeeper(files[i]);
    }

    const bool is3D = std::any_of(files.begin(), files.end(),
                                  [](const PackFile& file) { return IsOneOf(file.Ext, kModelExts); });
    if (is3D)
        Select3DPack(files, keep);
    else
        SelectFlatPack(files, keep);

    return BuildImportEntries(files, keep);
}

} // namespace GameEngine::KenneyAssets
