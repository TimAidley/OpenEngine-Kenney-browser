#pragma once

#include <filesystem>
#include <string>

namespace GameEngine::KenneyAssets
{

/// True when the file ends with a zip end-of-central-directory record: the
/// download has finished writing. A browser's 0-byte placeholder and a file
/// still being written both read false.
bool IsCompleteZip(const std::filesystem::path& zipFile);

/// Extracts the whole archive into `destination` with the system `unzip` tool
/// (a stopgap until the editor exposes its own zip reader to packages). unzip
/// refuses entries that would land outside `destination`. Blocks until unzip
/// exits; call it from a worker thread. False with `outError` on failure.
bool ExtractZip(const std::filesystem::path& zipFile, const std::filesystem::path& destination,
                std::string& outError);

} // namespace GameEngine::KenneyAssets
