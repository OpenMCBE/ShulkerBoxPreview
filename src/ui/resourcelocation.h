#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

enum class ResourceFileSystem : int {
    UserPackage   = 0,
    AppPackage    = 1,
    Raw           = 2,
    RawPersistent = 3,
    SettingsDir   = 4,
    ExternalDir   = 5,
    ServerPackage = 6,
    DataDir       = 7,
    UserDir       = 8,
    ScreenshotsDir= 9,
    StoreCache    = 10,
    MaterialsDir  = 11,
    Invalid       = 12
};

class ResourceLocation {
public:
    ResourceFileSystem mFileSystem;
    std::string mPath;
    std::uint64_t mPathHash;
    std::uint64_t mFullHash;

    ResourceLocation() : ResourceLocation("", ResourceFileSystem::UserPackage) {}

    explicit ResourceLocation(const std::string& path, ResourceFileSystem fs = ResourceFileSystem::UserPackage)
        : mFileSystem(fs),
          mPath(path),
          mPathHash(computePathHash(path)),
          mFullHash(mPathHash ^ static_cast<std::uint64_t>(mFileSystem)) {}

    explicit ResourceLocation(const char* path, ResourceFileSystem fs = ResourceFileSystem::UserPackage)
        : ResourceLocation(path ? std::string(path) : std::string(), fs) {}

private:
    static std::uint64_t computePathHash(std::string_view path) {
        constexpr std::uint64_t kOffset = 1469598103934665603ULL;
        constexpr std::uint64_t kPrime  = 1099511628211ULL;
        std::uint64_t hash = kOffset;
        for (unsigned char ch : path)
            hash = static_cast<std::uint64_t>(ch) ^ (kPrime * hash);
        return hash;
    }
};

static_assert(offsetof(ResourceLocation, mPath) == 8);
