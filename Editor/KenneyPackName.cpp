#include "KenneyPackName.h"

#include <algorithm>
#include <cctype>

namespace GameEngine::KenneyAssets
{
namespace
{

constexpr std::string_view kPrefix = "kenney_";
constexpr std::string_view kExtension = ".zip";

bool IsSlugChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
}

bool IsVersion(std::string_view text)
{
    return !text.empty() && std::isdigit(static_cast<unsigned char>(text.front())) &&
           std::isdigit(static_cast<unsigned char>(text.back())) &&
           std::all_of(text.begin(), text.end(),
                       [](char c) { return c == '.' || std::isdigit(static_cast<unsigned char>(c)); });
}

// Strips the " (N)" a browser appends when the file name is already taken.
std::string_view StripDuplicateCounter(std::string_view stem)
{
    if (!stem.ends_with(')'))
        return stem;
    const size_t open = stem.rfind(" (");
    if (open == std::string_view::npos)
        return stem;
    const std::string_view counter = stem.substr(open + 2, stem.size() - open - 3);
    const bool digits = !counter.empty() && std::all_of(counter.begin(), counter.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
    });
    return digits ? stem.substr(0, open) : stem;
}

} // namespace

std::optional<KenneyPackName> ParseKenneyZipName(std::string_view fileName)
{
    if (!fileName.starts_with(kPrefix) || !fileName.ends_with(kExtension))
        return std::nullopt;
    std::string_view stem = fileName.substr(kPrefix.size(), fileName.size() - kPrefix.size() - kExtension.size());
    stem = StripDuplicateCounter(stem);

    KenneyPackName name;
    const size_t underscore = stem.rfind('_');
    if (underscore != std::string_view::npos)
    {
        name.Version = std::string(stem.substr(underscore + 1));
        stem = stem.substr(0, underscore);
        if (!IsVersion(name.Version))
            return std::nullopt;
    }
    if (stem.empty() || !std::all_of(stem.begin(), stem.end(), IsSlugChar))
        return std::nullopt;
    name.Slug = std::string(stem);
    return name;
}

std::string KenneyPackPageUrl(std::string_view slug)
{
    return "https://kenney.nl/assets/" + std::string(slug);
}

} // namespace GameEngine::KenneyAssets
