#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr std::int32_t kGridWidth = 256;
    constexpr std::int32_t kGridDepth = 256;
    constexpr std::int32_t kGridHeight = 48;
    constexpr std::int32_t kFloorCount = 4;
    constexpr std::int32_t kFloorGap = 12;
    constexpr std::int32_t kPitchBandHeight = 12;
    constexpr std::int32_t kIslandCount = 4;
    constexpr std::int32_t kIslandSize = 64;
    constexpr std::int32_t kIslandGap = 28;
    constexpr std::int32_t kFocusedSubIslandCount = 4;
    constexpr std::int32_t kFocusedSubIslandSize = 32;
    constexpr std::int32_t kFocusedSubIslandGap = 16;
    constexpr float kCubeSize = 1.0f;
    constexpr float kSlabThickness = 2.5f;

    constexpr unsigned char kKeyMinus = 45;
    constexpr unsigned char kKeyEquals = 61;
    constexpr unsigned char kKeyUnderscore = 95;
    constexpr unsigned char kKeyZero = 48;
    constexpr unsigned char kKeyFUpper = 70;
    constexpr unsigned char kKeyFLower = 102;
    constexpr unsigned char kKeyNUpper = 78;
    constexpr unsigned char kKeyNLower = 110;
    constexpr unsigned char kKeyGUpper = 71;
    constexpr unsigned char kKeyGLower = 103;
    constexpr unsigned char kKeyJUpper = 74;
    constexpr unsigned char kKeyJLower = 106;
    constexpr unsigned char kKeyKUpper = 75;
    constexpr unsigned char kKeyKLower = 107;
    constexpr unsigned char kKeyLUpper = 76;
    constexpr unsigned char kKeyLLower = 108;
    constexpr unsigned char kKeyUUpper = 85;
    constexpr unsigned char kKeyULower = 117;
    constexpr unsigned char kKeyWUpper = 87;
    constexpr unsigned char kKeyWLower = 119;
    constexpr unsigned char kKeyAUpper = 65;
    constexpr unsigned char kKeyALower = 97;
    constexpr unsigned char kKeySUpper = 83;
    constexpr unsigned char kKeySLower = 115;
    constexpr unsigned char kKeyDUpper = 68;
    constexpr unsigned char kKeyDLower = 100;
    constexpr unsigned char kKeyQUpper = 81;
    constexpr unsigned char kKeyQLower = 113;
    constexpr unsigned char kKeyEUpper = 69;
    constexpr unsigned char kKeyELower = 101;
    constexpr unsigned char kKeyRUpper = 82;
    constexpr unsigned char kKeyRLower = 114;
    constexpr unsigned char kKeyTUpper = 84;
    constexpr unsigned char kKeyTLower = 116;
    constexpr unsigned char kKeyEnter = 13;
    constexpr unsigned char kKeyComma = 44;
    constexpr unsigned char kKeyPeriod = 46;
    constexpr unsigned char kKeyEscape = 27;
    constexpr int kSpecialLeft = GLUT_KEY_LEFT;
    constexpr int kSpecialUp = GLUT_KEY_UP;
    constexpr int kSpecialRight = GLUT_KEY_RIGHT;
    constexpr int kSpecialDown = GLUT_KEY_DOWN;
    constexpr unsigned int kAutosaveIntervalMs = 120000u;

    struct Color3f
    {
        float r;
        float g;
        float b;
    };

    struct ScaleDefinition
    {
        const char* name;
        bool allowed[12];
    };

    constexpr Color3f kLevelPalette[12] = {
        {0.98f, 0.87f, 0.02f},
        {0.72f, 0.84f, 0.27f},
        {0.33f, 0.70f, 0.35f},
        {0.46f, 0.63f, 0.78f},
        {0.62f, 0.30f, 0.58f},
        {0.92f, 0.31f, 0.52f},
        {0.82f, 0.29f, 0.25f},
        {0.98f, 0.40f, 0.10f},
        {0.96f, 0.42f, 0.00f},
        {0.95f, 0.44f, 0.00f},
        {0.96f, 0.50f, 0.02f},
        {0.97f, 0.62f, 0.10f}
    };

    constexpr const char* kPitchClasses[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    constexpr ScaleDefinition kScaleDefinitions[] = {
        {"Major",      {true,  false, true,  false, true,  true,  false, true,  false, true,  false, true }},
        {"Minor",      {true,  false, true,  true,  false, true,  false, true,  true,  false, true,  false}},
        {"Dorian",     {true,  false, true,  true,  false, true,  false, true,  false, true,  true,  false}},
        {"Mixolydian", {true,  false, true,  false, true,  true,  false, true,  false, true,  true,  false}},
        {"Pentatonic", {true,  false, true,  false, true,  false, false, true,  false, true,  false, false}},
        {"Chromatic",  {true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true }}
    };

    int gWindowWidth = 1600;
    int gWindowHeight = 900;
    float gZoom = 1.0f;
    constexpr float kMinZoom = 1.0f;
    constexpr float kMaxZoom = 25.0f;
    float gCameraAngle = -0.78539816339f;
    bool gFourFloorMode = false;
    bool gIslandMode = false;
    bool gDetailView = false;
    bool gDetailSubIslandMode = false;
    bool gDetailQuarterView = false;
    std::int32_t gSelectedKey = 0;
    std::size_t gSelectedScale = 1;
    std::uint32_t gSeed = 0xC0FFEEu;
    std::vector<std::uint8_t> gBlocks(static_cast<std::size_t>(kFloorCount) * kGridWidth * kGridDepth * kGridHeight, 0);
    std::int32_t gActiveIsland = 2;
    std::int32_t gActiveFloor = kFloorCount - 1;
    std::int32_t gActiveDetailSubIsland = 0;
    std::int32_t gCursorX = kGridWidth / 2;
    std::int32_t gCursorY = kGridDepth / 2;
    std::int32_t gCursorZ = 0;
    std::string gSaveStatus = "Save: not yet saved";
    std::string gSavePath;
    bool gAutosaveEnabled = false;

    constexpr Color3f kSkyTop = {0.06f, 0.08f, 0.16f};
    constexpr Color3f kSkyBottom = {0.01f, 0.01f, 0.03f};
    constexpr Color3f kHorizonGlow = {0.16f, 0.20f, 0.34f};
    constexpr Color3f kOutline = {0.93f, 0.95f, 0.99f};
    constexpr Color3f kShadow = {0.03f, 0.03f, 0.05f};

    std::size_t layerStride()
    {
        return static_cast<std::size_t>(kGridWidth) * kGridDepth;
    }

    struct DrdHeader
    {
        char magic[4];
        std::uint32_t version;
        std::uint32_t gridWidth;
        std::uint32_t gridDepth;
        std::uint32_t gridHeight;
        std::uint32_t floorCount;
        std::uint32_t seed;
        std::int32_t selectedKey;
        std::uint32_t selectedScale;
        std::uint8_t fourFloorMode;
        std::uint8_t islandMode;
        std::uint8_t detailView;
        std::uint8_t detailSubIslandMode;
        std::uint8_t detailQuarterView;
        std::int32_t activeIsland;
        std::int32_t activeFloor;
        std::int32_t activeDetailSubIsland;
        std::int32_t cursorX;
        std::int32_t cursorY;
        std::int32_t cursorZ;
        float zoom;
        float cameraAngle;
    };

    void syncActiveSelection();
    bool isLayerAllowed(std::int32_t absoluteLayer);
    float floorBaseZ(std::int32_t floorIndex);
    std::int32_t islandSourceStartX(std::int32_t islandIndex);
    std::int32_t islandSourceStartY(std::int32_t islandIndex);

    struct BuildContext
    {
        bool valid;
        std::int32_t sourceStartX;
        std::int32_t sourceStartY;
        std::int32_t width;
        std::int32_t depth;
        std::int32_t totalLayers;
    };

    std::size_t indexFor(std::int32_t floorIndex, std::int32_t x, std::int32_t y)
    {
        return static_cast<std::size_t>(floorIndex) * layerStride()
             + static_cast<std::size_t>(y) * kGridWidth
             + x;
    }

    std::size_t blockIndexFor(std::int32_t floorIndex, std::int32_t x, std::int32_t y, std::int32_t z)
    {
        const auto floorStride = static_cast<std::size_t>(kGridWidth) * kGridDepth * kGridHeight;
        return static_cast<std::size_t>(floorIndex) * floorStride
             + static_cast<std::size_t>(z) * kGridWidth * kGridDepth
             + static_cast<std::size_t>(y) * kGridWidth
             + x;
    }

    std::uint32_t hashU32(std::uint32_t x)
    {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return x;
    }

    float random01(std::int32_t x, std::int32_t y, std::uint32_t seed)
    {
        const std::uint32_t mixed = hashU32(static_cast<std::uint32_t>(x) * 0x9e3779b9u
            ^ static_cast<std::uint32_t>(y) * 0x85ebca6bu
            ^ seed);
        return static_cast<float>(mixed & 0x00ffffffu) / static_cast<float>(0x01000000u);
    }

    float smoothstep01(float t)
    {
        return t * t * (3.0f - 2.0f * t);
    }

    float valueNoise(float x, float y, float cellSize, std::uint32_t seed)
    {
        const float gx = x / cellSize;
        const float gy = y / cellSize;
        const auto x0 = static_cast<std::int32_t>(std::floor(gx));
        const auto y0 = static_cast<std::int32_t>(std::floor(gy));
        const auto x1 = x0 + 1;
        const auto y1 = y0 + 1;

        const float tx = smoothstep01(gx - static_cast<float>(x0));
        const float ty = smoothstep01(gy - static_cast<float>(y0));

        const float n00 = random01(x0, y0, seed);
        const float n10 = random01(x1, y0, seed);
        const float n01 = random01(x0, y1, seed);
        const float n11 = random01(x1, y1, seed);

        const float nx0 = n00 + (n10 - n00) * tx;
        const float nx1 = n01 + (n11 - n01) * tx;
        return nx0 + (nx1 - nx0) * ty;
    }

    std::uint8_t sampleHeight(std::int32_t floorIndex, std::int32_t x, std::int32_t y)
    {
        const float xf = static_cast<float>(x);
        const float yf = static_cast<float>(y);
        const std::uint32_t floorSeed = hashU32(gSeed + static_cast<std::uint32_t>(floorIndex) * 0x517cc1b7u);

        const float broad = valueNoise(xf, yf, 40.0f, floorSeed ^ 0x13572468u);
        const float medium = valueNoise(xf, yf, 18.0f, floorSeed ^ 0x24681357u);
        const float detail = valueNoise(xf, yf, 8.0f, floorSeed ^ 0xabcdef01u);
        const float mask = valueNoise(xf, yf, 70.0f, floorSeed ^ 0x55aa55aau);

        const float combined = (broad * 0.55f) + (medium * 0.30f) + (detail * 0.15f);
        const float shaped = std::pow(std::max(0.0f, combined - 0.28f) / 0.72f, 1.65f);
        const float clustered = shaped * (0.45f + mask * 0.95f);
        const auto height = static_cast<std::int32_t>(std::round(clustered * static_cast<float>(kGridHeight)));
        return static_cast<std::uint8_t>(std::clamp(height, 0, kGridHeight));
    }

    void regenerateBlocks()
    {
        std::fill(gBlocks.begin(), gBlocks.end(), static_cast<std::uint8_t>(0));

        for (std::int32_t floorIndex = 0; floorIndex < kFloorCount; ++floorIndex)
        {
            for (std::int32_t y = 0; y < kGridDepth; ++y)
            {
                for (std::int32_t x = 0; x < kGridWidth; ++x)
                {
                    const auto height = sampleHeight(floorIndex, x, y);
                    for (std::int32_t z = 0; z < height; ++z)
                    {
                        if (isLayerAllowed(z + 1))
                        {
                            gBlocks[blockIndexFor(floorIndex, x, y, z)] = 1;
                        }
                    }
                }
            }
        }
    }

    std::string nextSavePath()
    {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
#if defined(_WIN32)
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        char buffer[64];
        if (std::strftime(buffer, sizeof(buffer), "world_%Y%m%d_%H%M%S.drd", &localTime) == 0)
        {
            return "world.drd";
        }

        return std::string(buffer);
    }

    std::string shellSingleQuoted(const std::string& value)
    {
        std::string escaped = "'";
        for (const char ch : value)
        {
            if (ch == '\'')
            {
                escaped += "'\\''";
            }
            else
            {
                escaped += ch;
            }
        }
        escaped += "'";
        return escaped;
    }

    std::string chooseSavePath()
    {
#ifdef __APPLE__
        const auto suggestedName = nextSavePath();
        const auto cwd = std::filesystem::current_path().string();

        std::string command = "/usr/bin/osascript";
        command += " -e ";
        command += shellSingleQuoted(std::string("set suggestedName to \"") + suggestedName + "\"");
        command += " -e ";
        command += shellSingleQuoted(std::string("set defaultLocation to POSIX file \"") + cwd + "\"");
        command += " -e ";
        command += shellSingleQuoted("try");
        command += " -e ";
        command += shellSingleQuoted("POSIX path of (choose file name with prompt \"Save block world as\" default name suggestedName default location defaultLocation)");
        command += " -e ";
        command += shellSingleQuoted("on error number -128");
        command += " -e ";
        command += shellSingleQuoted("return \"\"");
        command += " -e ";
        command += shellSingleQuoted("end try");

        std::array<char, 1024> buffer{};
        std::string output;
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe == nullptr)
        {
            return std::string();
        }

        while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
        {
            output += buffer.data();
        }

        pclose(pipe);

        while (!output.empty() && (output.back() == '\n' || output.back() == '\r'))
        {
            output.pop_back();
        }

        return output;
#else
        return nextSavePath();
#endif
    }

    bool saveWorldToFile(const std::string& path)
    {
        std::ofstream output(path, std::ios::binary);
        if (!output)
        {
            return false;
        }

        const DrdHeader header = {
            {'D', 'R', 'D', '1'},
            2u,
            static_cast<std::uint32_t>(kGridWidth),
            static_cast<std::uint32_t>(kGridDepth),
            static_cast<std::uint32_t>(kGridHeight),
            static_cast<std::uint32_t>(kFloorCount),
            gSeed,
            gSelectedKey,
            static_cast<std::uint32_t>(gSelectedScale),
            static_cast<std::uint8_t>(gFourFloorMode ? 1 : 0),
            static_cast<std::uint8_t>(gIslandMode ? 1 : 0),
            static_cast<std::uint8_t>(gDetailView ? 1 : 0),
            static_cast<std::uint8_t>(gDetailSubIslandMode ? 1 : 0),
            static_cast<std::uint8_t>(gDetailQuarterView ? 1 : 0),
            gActiveIsland,
            gActiveFloor,
            gActiveDetailSubIsland,
            gCursorX,
            gCursorY,
            gCursorZ,
            gZoom,
            gCameraAngle
        };

        output.write(reinterpret_cast<const char*>(&header), sizeof(header));
        output.write(reinterpret_cast<const char*>(gBlocks.data()), static_cast<std::streamsize>(gBlocks.size()));
        return static_cast<bool>(output);
    }

    bool loadWorldFromFile(const std::string& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            return false;
        }

        DrdHeader header{};
        input.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!input)
        {
            return false;
        }

        if (header.magic[0] != 'D' || header.magic[1] != 'R' || header.magic[2] != 'D' || header.magic[3] != '1')
        {
            return false;
        }

        if (header.version != 2u
            || header.gridWidth != static_cast<std::uint32_t>(kGridWidth)
            || header.gridDepth != static_cast<std::uint32_t>(kGridDepth)
            || header.gridHeight != static_cast<std::uint32_t>(kGridHeight)
            || header.floorCount != static_cast<std::uint32_t>(kFloorCount))
        {
            return false;
        }

        std::vector<std::uint8_t> loadedBlocks(gBlocks.size());
        input.read(reinterpret_cast<char*>(loadedBlocks.data()), static_cast<std::streamsize>(loadedBlocks.size()));
        if (!input)
        {
            return false;
        }

        gBlocks = std::move(loadedBlocks);
        gSeed = header.seed;
        gSelectedKey = std::clamp(header.selectedKey, 0, 11);
        gSelectedScale = std::min<std::size_t>(header.selectedScale, (sizeof(kScaleDefinitions) / sizeof(kScaleDefinitions[0])) - 1);
        gFourFloorMode = header.fourFloorMode != 0;
        gIslandMode = header.islandMode != 0;
        gDetailView = header.detailView != 0;
        gDetailSubIslandMode = header.detailSubIslandMode != 0;
        gDetailQuarterView = header.detailQuarterView != 0;
        gActiveIsland = std::clamp(header.activeIsland, 0, kIslandCount - 1);
        gActiveFloor = std::clamp(header.activeFloor, 0, kFloorCount - 1);
        gActiveDetailSubIsland = std::clamp(header.activeDetailSubIsland, 0, kFocusedSubIslandCount - 1);
        gCursorX = header.cursorX;
        gCursorY = header.cursorY;
        gCursorZ = header.cursorZ;
        gZoom = std::clamp(header.zoom, kMinZoom, kMaxZoom);
        gCameraAngle = header.cameraAngle;
        syncActiveSelection();
        return true;
    }

    std::string latestSavePath()
    {
        namespace fs = std::filesystem;
        fs::path bestPath;
        fs::file_time_type bestTime;
        bool found = false;

        for (const auto& entry : fs::directory_iterator(fs::current_path()))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (entry.path().extension() != ".drd")
            {
                continue;
            }

            const auto writeTime = entry.last_write_time();
            if (!found || writeTime > bestTime)
            {
                found = true;
                bestTime = writeTime;
                bestPath = entry.path();
            }
        }

        return found ? bestPath.filename().string() : std::string();
    }

    void autosaveCallback(int)
    {
        if (gAutosaveEnabled && !gSavePath.empty())
        {
            if (saveWorldToFile(gSavePath))
            {
                gSaveStatus = "Autosave: " + gSavePath;
            }
            else
            {
                gSaveStatus = "Autosave failed: " + gSavePath;
            }

            glutPostRedisplay();
        }

        glutTimerFunc(kAutosaveIntervalMs, autosaveCallback, 0);
    }

    bool splitIslandViewActive()
    {
        if (gDetailView)
        {
            return gDetailSubIslandMode && !gDetailQuarterView;
        }

        return gIslandMode;
    }

    BuildContext currentBuildContext()
    {
        if (gDetailQuarterView)
        {
            return {
                true,
                islandSourceStartX(gActiveIsland) + (gActiveDetailSubIsland % 2) * kFocusedSubIslandSize,
                islandSourceStartY(gActiveIsland) + (gActiveDetailSubIsland / 2) * kFocusedSubIslandSize,
                kFocusedSubIslandSize,
                kFocusedSubIslandSize,
                kPitchBandHeight
            };
        }

        if (gDetailView && !gDetailSubIslandMode)
        {
            return {
                true,
                islandSourceStartX(gActiveIsland),
                islandSourceStartY(gActiveIsland),
                kIslandSize,
                kIslandSize,
                kPitchBandHeight
            };
        }

        if (gIslandMode)
        {
            return {false, 0, 0, 0, 0, 0};
        }

        if (!gDetailView && !gIslandMode)
        {
            return {
                true,
                0,
                0,
                kGridWidth,
                kGridDepth,
                gFourFloorMode ? kFloorCount * kGridHeight : kGridHeight
            };
        }

        return {false, 0, 0, 0, 0, 0};
    }

    void clampBuildCursor()
    {
        const auto context = currentBuildContext();
        if (!context.valid)
        {
            return;
        }

        gCursorX = std::clamp(gCursorX, 0, context.width - 1);
        gCursorY = std::clamp(gCursorY, 0, context.depth - 1);
        gCursorZ = std::clamp(gCursorZ, 0, context.totalLayers - 1);
    }

    bool blockExists(std::int32_t floorIndex, std::int32_t x, std::int32_t y, std::int32_t z)
    {
        if (floorIndex < 0 || floorIndex >= kFloorCount
            || x < 0 || x >= kGridWidth
            || y < 0 || y >= kGridDepth
            || z < 0 || z >= kGridHeight)
        {
            return false;
        }

        return gBlocks[blockIndexFor(floorIndex, x, y, z)] != 0;
    }

    void setBlock(std::int32_t floorIndex, std::int32_t x, std::int32_t y, std::int32_t z, bool present)
    {
        if (floorIndex < 0 || floorIndex >= kFloorCount
            || x < 0 || x >= kGridWidth
            || y < 0 || y >= kGridDepth
            || z < 0 || z >= kGridHeight)
        {
            return;
        }

        gBlocks[blockIndexFor(floorIndex, x, y, z)] = static_cast<std::uint8_t>(present ? 1 : 0);
    }

    struct CursorBlockLocation
    {
        bool valid;
        std::int32_t sourceFloorIndex;
        std::int32_t sourceX;
        std::int32_t sourceY;
        std::int32_t localZ;
        std::int32_t colorLayer;
        float markerBaseZ;
        float guideBaseZ;
        float originX;
        float originY;
    };

    CursorBlockLocation currentCursorLocation()
    {
        const auto context = currentBuildContext();
        if (!context.valid)
        {
            return {false, 0, 0, 0, 0, 1, 0.0f, 0.0f, 0.0f, 0.0f};
        }

        clampBuildCursor();

        if (gDetailQuarterView)
        {
            return {
                true,
                gActiveFloor,
                context.sourceStartX + gCursorX,
                context.sourceStartY + gCursorY,
                gCursorZ,
                1 + gActiveFloor * kPitchBandHeight + gCursorZ,
                static_cast<float>(gCursorZ) * kCubeSize,
                0.0f,
                static_cast<float>(gCursorX) * kCubeSize,
                static_cast<float>(gCursorY) * kCubeSize
            };
        }

        if (gDetailView && !gDetailSubIslandMode)
        {
            return {
                true,
                gActiveFloor,
                context.sourceStartX + gCursorX,
                context.sourceStartY + gCursorY,
                gCursorZ,
                1 + gActiveFloor * kPitchBandHeight + gCursorZ,
                static_cast<float>(gCursorZ) * kCubeSize,
                0.0f,
                static_cast<float>(gCursorX) * kCubeSize,
                static_cast<float>(gCursorY) * kCubeSize
            };
        }

        const auto sourceFloorIndex = gFourFloorMode ? std::clamp(gCursorZ / kGridHeight, 0, kFloorCount - 1) : 0;
        const auto localZ = gFourFloorMode ? (gCursorZ % kGridHeight) : gCursorZ;
        return {
            true,
            sourceFloorIndex,
            gCursorX,
            gCursorY,
            localZ,
            localZ + 1,
            floorBaseZ(sourceFloorIndex) + static_cast<float>(localZ) * kCubeSize,
            floorBaseZ(sourceFloorIndex),
            static_cast<float>(gCursorX) * kCubeSize,
            static_cast<float>(gCursorY) * kCubeSize
        };
    }

    std::int32_t activeFloorCount()
    {
        if (gDetailView)
        {
            return 1;
        }
        return gFourFloorMode ? kFloorCount : 1;
    }

    std::int32_t activeSegmentLayerCount()
    {
        if (gDetailView)
        {
            return kPitchBandHeight;
        }
        return (gIslandMode && gFourFloorMode) ? kPitchBandHeight : kGridHeight;
    }

    float floorBaseZ(std::int32_t floorIndex)
    {
        return static_cast<float>(floorIndex * (activeSegmentLayerCount() + kFloorGap)) * kCubeSize;
    }

    float totalSceneHeight()
    {
        return floorBaseZ(activeFloorCount() - 1)
             + static_cast<float>(activeSegmentLayerCount()) * kCubeSize;
    }

    float sceneSpanX()
    {
        if (gDetailView)
        {
            if (gDetailSubIslandMode)
            {
                if (gDetailQuarterView)
                {
                    return static_cast<float>(kFocusedSubIslandSize) * kCubeSize;
                }
                return static_cast<float>(kFocusedSubIslandSize * 2 + kFocusedSubIslandGap) * kCubeSize;
            }
            return static_cast<float>(kIslandSize) * kCubeSize;
        }

        if (!gIslandMode)
        {
            return static_cast<float>(kGridWidth) * kCubeSize;
        }

        return static_cast<float>(kIslandSize * 2 + kIslandGap) * kCubeSize;
    }

    float sceneSpanY()
    {
        if (gDetailView)
        {
            if (gDetailSubIslandMode)
            {
                if (gDetailQuarterView)
                {
                    return static_cast<float>(kFocusedSubIslandSize) * kCubeSize;
                }
                return static_cast<float>(kFocusedSubIslandSize * 2 + kFocusedSubIslandGap) * kCubeSize;
            }
            return static_cast<float>(kIslandSize) * kCubeSize;
        }

        if (!gIslandMode)
        {
            return static_cast<float>(kGridDepth) * kCubeSize;
        }

        return static_cast<float>(kIslandSize * 2 + kIslandGap) * kCubeSize;
    }

    float islandOriginX(std::int32_t islandIndex)
    {
        const float stride = static_cast<float>(kIslandSize + kIslandGap) * kCubeSize;
        return static_cast<float>(islandIndex % 2) * stride;
    }

    float islandOriginY(std::int32_t islandIndex)
    {
        const float stride = static_cast<float>(kIslandSize + kIslandGap) * kCubeSize;
        return static_cast<float>(islandIndex / 2) * stride;
    }

    float focusedSubIslandOriginX(std::int32_t islandIndex)
    {
        const float stride = static_cast<float>(kFocusedSubIslandSize + kFocusedSubIslandGap) * kCubeSize;
        return static_cast<float>(islandIndex % 2) * stride;
    }

    float focusedSubIslandOriginY(std::int32_t islandIndex)
    {
        const float stride = static_cast<float>(kFocusedSubIslandSize + kFocusedSubIslandGap) * kCubeSize;
        return static_cast<float>(islandIndex / 2) * stride;
    }

    std::int32_t islandSourceStartX(std::int32_t islandIndex)
    {
        return (islandIndex % 2 == 0) ? 32 : (kGridWidth - kIslandSize - 32);
    }

    std::int32_t islandSourceStartY(std::int32_t islandIndex)
    {
        return (islandIndex / 2 == 0) ? 32 : (kGridDepth - kIslandSize - 32);
    }

    Color3f levelColor(std::int32_t z)
    {
        return kLevelPalette[(z - 1) % 12];
    }

    const char* levelPitchClass(std::int32_t z)
    {
        return kPitchClasses[(z - 1) % 12];
    }

    std::int32_t levelPitchIndex(std::int32_t z)
    {
        return (z - 1) % 12;
    }

    const ScaleDefinition& selectedScale()
    {
        return kScaleDefinitions[gSelectedScale];
    }

    bool isLayerAllowed(std::int32_t absoluteLayer)
    {
        const auto pitchIndex = levelPitchIndex(absoluteLayer);
        const auto relative = (pitchIndex - gSelectedKey + 12) % 12;
        return selectedScale().allowed[relative];
    }

    std::string quantizerSummary()
    {
        std::ostringstream stream;
        stream << "Quantize: " << kPitchClasses[gSelectedKey] << ' ' << selectedScale().name;
        return stream.str();
    }

    std::string pitchCycleSummary()
    {
        std::ostringstream stream;
        stream << "Z pitches: ";
        for (std::int32_t z = 1; z <= 12; ++z)
        {
            if (z > 1)
            {
                stream << ' ';
            }
            stream << z << '=' << levelPitchClass(z);
        }
        stream << "   repeat for 13-24, 25-36, 37-48";
        return stream.str();
    }

    const char* islandPositionName(std::int32_t islandIndex)
    {
        switch (islandIndex)
        {
        case 1:
            return "Right";
        case 2:
            return "Top";
        case 3:
            return "Bottom";
        case 0:
            return "Left";
        default:
            return "Unknown";
        }
    }

    std::string activeSelectionSummary()
    {
        if (gDetailView && gDetailSubIslandMode)
        {
            static constexpr const char* kQuarterNames[4] = {
                "Top Left", "Top Right", "Bottom Left", "Bottom Right"
            };

            std::ostringstream stream;
            stream << "Selection: " << kQuarterNames[gActiveDetailSubIsland];
            if (gDetailQuarterView)
            {
                stream << " / Focused";
            }
            return stream.str();
        }

        if (!gIslandMode || !gFourFloorMode)
        {
            return "Selection: n/a";
        }

        std::ostringstream stream;
        stream << "Selection: " << islandPositionName(gActiveIsland)
               << " / Layer " << (gActiveFloor + 1);
        return stream.str();
    }

    std::string buildStatusSummary()
    {
        const auto context = currentBuildContext();
        if (!context.valid)
        {
            return "Build: unavailable in split island view";
        }

        const auto location = currentCursorLocation();
        std::ostringstream stream;
        stream << "Build: x=" << gCursorX
               << " y=" << gCursorY
               << " z=" << gCursorZ;
        if (!gDetailView && gFourFloorMode)
        {
            stream << " floor=" << (location.sourceFloorIndex + 1);
        }
        return stream.str();
    }

    Color3f scaleColor(Color3f color, float amount)
    {
        return {
            std::clamp(color.r * amount, 0.0f, 1.0f),
            std::clamp(color.g * amount, 0.0f, 1.0f),
            std::clamp(color.b * amount, 0.0f, 1.0f)
        };
    }

    Color3f mixColor(Color3f a, Color3f b, float t)
    {
        return {
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t
        };
    }

    float focusForRegion(std::int32_t islandIndex, std::int32_t floorIndex)
    {
        if (!gIslandMode || !gFourFloorMode)
        {
            return 1.0f;
        }

        return (gActiveIsland == islandIndex && gActiveFloor == floorIndex) ? 1.0f : 0.18f;
    }

    float focusForDetailSubIsland(std::int32_t subIslandIndex)
    {
        if (!gDetailView || !gDetailSubIslandMode || gDetailQuarterView)
        {
            return 1.0f;
        }

        return (gActiveDetailSubIsland == subIslandIndex) ? 1.0f : 0.18f;
    }

    std::int32_t spiralIslandForStep(std::int32_t spiralIndex)
    {
        constexpr std::int32_t kSpiralIslands[4] = {2, 1, 3, 0};
        return kSpiralIslands[spiralIndex % 4];
    }

    std::int32_t spiralFloorForStep(std::int32_t spiralIndex)
    {
        return kFloorCount - 1 - (spiralIndex / 4);
    }

    std::int32_t spiralIndexForSelection(std::int32_t islandIndex, std::int32_t floorIndex)
    {
        constexpr std::int32_t kSpiralIslands[4] = {2, 1, 3, 0};
        const auto floorOffset = (kFloorCount - 1 - floorIndex) * 4;
        for (std::int32_t i = 0; i < 4; ++i)
        {
            if (kSpiralIslands[i] == islandIndex)
            {
                return floorOffset + i;
            }
        }
        return 0;
    }

    void setSelectionFromSpiralIndex(std::int32_t spiralIndex)
    {
        const auto clamped = std::clamp(spiralIndex, 0, kIslandCount * kFloorCount - 1);
        gActiveIsland = spiralIslandForStep(clamped);
        gActiveFloor = spiralFloorForStep(clamped);
    }

    void resetSelectionToSpiralStart()
    {
        setSelectionFromSpiralIndex(0);
    }

    Color3f applyFocus(Color3f color, float focus)
    {
        const auto muted = mixColor(kSkyTop, color, 0.28f);
        return mixColor(muted, color, focus);
    }

    void setColor(const Color3f& color, float alpha)
    {
        glColor4f(color.r, color.g, color.b, alpha);
    }

    void emitVertex(float x, float y, float z)
    {
        glVertex3f(x, y, z);
    }

    void drawScreenQuad(float x0, float y0, float x1, float y1, const Color3f& top, const Color3f& bottom, float alpha)
    {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, static_cast<double>(gWindowWidth), 0.0, static_cast<double>(gWindowHeight), -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
        glColor4f(bottom.r, bottom.g, bottom.b, alpha);
        glVertex2f(x0, y0);
        glVertex2f(x1, y0);
        glColor4f(top.r, top.g, top.b, alpha);
        glVertex2f(x1, y1);
        glVertex2f(x0, y1);
        glEnd();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    void drawBackdrop()
    {
        glDisable(GL_BLEND);
        drawScreenQuad(0.0f, 0.0f, static_cast<float>(gWindowWidth), static_cast<float>(gWindowHeight), kSkyTop, kSkyTop, 1.0f);
    }

    void drawQuadTop(float x, float y, float z, const Color3f& color)
    {
        glBegin(GL_QUADS);
        setColor(color, 1.0f);
        emitVertex(x, y, z);
        emitVertex(x + kCubeSize, y, z);
        emitVertex(x + kCubeSize, y + kCubeSize, z);
        emitVertex(x, y + kCubeSize, z);
        glEnd();
    }

    void drawQuadEast(float x, float y, float z0, float z1, const Color3f& color)
    {
        const auto shadeA = scaleColor(color, 0.58f);
        const auto shadeB = scaleColor(color, 0.75f);

        glBegin(GL_QUADS);
        setColor(shadeA, 1.0f);
        emitVertex(x, y, z0);
        emitVertex(x, y + kCubeSize, z0);
        setColor(shadeB, 1.0f);
        emitVertex(x, y + kCubeSize, z1);
        emitVertex(x, y, z1);
        glEnd();
    }

    void drawQuadNorth(float x0, float x1, float y, float z0, float z1, const Color3f& color)
    {
        const auto shadeA = scaleColor(color, 0.48f);
        const auto shadeB = scaleColor(color, 0.65f);

        glBegin(GL_QUADS);
        setColor(shadeA, 1.0f);
        emitVertex(x0, y, z0);
        emitVertex(x1, y, z0);
        setColor(shadeB, 1.0f);
        emitVertex(x1, y, z1);
        emitVertex(x0, y, z1);
        glEnd();
    }

    void drawPlatformGlow(std::int32_t floorIndex)
    {
        const float pad = 18.0f;
        const float z = floorBaseZ(floorIndex) - kSlabThickness + 0.1f;
        const float maxX = sceneSpanX();
        const float maxY = sceneSpanY();
        const auto glow = Color3f{0.16f, 0.28f, 0.72f};

        glBegin(GL_QUADS);
        setColor(glow, 0.10f);
        emitVertex(-pad, -pad, z);
        emitVertex(maxX + pad, -pad, z);
        emitVertex(maxX + pad, maxY + pad, z);
        emitVertex(-pad, maxY + pad, z);
        glEnd();
    }

    void drawPlatformGrid(float originX, float originY, std::int32_t width, std::int32_t depth, float z)
    {
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        setColor(Color3f{0.23f, 0.29f, 0.43f}, 0.24f);

        const float maxX = originX + static_cast<float>(width) * kCubeSize;
        const float maxY = originY + static_cast<float>(depth) * kCubeSize;

        for (std::int32_t x = 0; x <= width; ++x)
        {
            const float xf = originX + static_cast<float>(x) * kCubeSize;
            emitVertex(xf, originY, z);
            emitVertex(xf, maxY, z);
        }

        for (std::int32_t y = 0; y <= depth; ++y)
        {
            const float yf = originY + static_cast<float>(y) * kCubeSize;
            emitVertex(originX, yf, z);
            emitVertex(maxX, yf, z);
        }

        glEnd();
        glLineWidth(1.0f);
    }

    void drawPlatform(
        float originX,
        float originY,
        std::int32_t width,
        std::int32_t depth,
        std::int32_t floorIndex,
        float focus)
    {
        const float maxX = originX + static_cast<float>(width) * kCubeSize;
        const float maxY = originY + static_cast<float>(depth) * kCubeSize;
        const float topZ = floorBaseZ(floorIndex);
        const float bottomZ = topZ - kSlabThickness;
        const float sideTopZ = topZ - 0.12f;
        const auto topColor = applyFocus(Color3f{0.05f, 0.07f, 0.14f}, focus);
        const auto eastColor = applyFocus(Color3f{0.11f, 0.16f, 0.30f}, focus);
        const auto northColor = applyFocus(Color3f{0.08f, 0.11f, 0.22f}, focus);

        glBegin(GL_QUADS);
        setColor(topColor, 1.0f);
        emitVertex(originX, originY, topZ);
        emitVertex(maxX, originY, topZ);
        emitVertex(maxX, maxY, topZ);
        emitVertex(originX, maxY, topZ);
        glEnd();

        glLineWidth(1.0f);
        glBegin(GL_LINES);
        setColor(applyFocus(Color3f{0.23f, 0.29f, 0.43f}, focus), 0.24f + 0.16f * focus);

        for (std::int32_t x = 0; x <= width; ++x)
        {
            const float xf = originX + static_cast<float>(x) * kCubeSize;
            emitVertex(xf, originY, topZ + 0.02f);
            emitVertex(xf, maxY, topZ + 0.02f);
        }

        for (std::int32_t y = 0; y <= depth; ++y)
        {
            const float yf = originY + static_cast<float>(y) * kCubeSize;
            emitVertex(originX, yf, topZ + 0.02f);
            emitVertex(maxX, yf, topZ + 0.02f);
        }

        glEnd();
        glLineWidth(1.0f);

        glBegin(GL_QUADS);
        setColor(eastColor, 1.0f);
        emitVertex(maxX, originY, bottomZ);
        emitVertex(maxX, maxY, bottomZ);
        emitVertex(maxX, maxY, sideTopZ);
        emitVertex(maxX, originY, sideTopZ);
        glEnd();

        glBegin(GL_QUADS);
        setColor(northColor, 1.0f);
        emitVertex(originX, maxY, bottomZ);
        emitVertex(maxX, maxY, bottomZ);
        emitVertex(maxX, maxY, sideTopZ);
        emitVertex(originX, maxY, sideTopZ);
        glEnd();
    }

    bool cubeExistsInBand(
        std::int32_t sourceFloorIndex,
        std::int32_t x,
        std::int32_t y,
        std::int32_t layerStart,
        std::int32_t layerCount,
        std::int32_t localLayer)
    {
        if (localLayer < 1 || localLayer > layerCount)
        {
            return false;
        }

        return blockExists(sourceFloorIndex, x, y, localLayer - 1);
    }

    bool cubeExistsInPatchBand(
        std::int32_t sourceFloorIndex,
        std::int32_t sourceStartX,
        std::int32_t sourceStartY,
        std::int32_t width,
        std::int32_t depth,
        std::int32_t x,
        std::int32_t y,
        std::int32_t layerStart,
        std::int32_t layerCount,
        std::int32_t localLayer)
    {
        if (x < sourceStartX || x >= sourceStartX + width || y < sourceStartY || y >= sourceStartY + depth)
        {
            return false;
        }

        return cubeExistsInBand(sourceFloorIndex, x, y, layerStart, layerCount, localLayer);
    }

    void drawBlockPatchImmediate(
        std::int32_t sourceFloorIndex,
        std::int32_t sourceStartX,
        std::int32_t sourceStartY,
        std::int32_t width,
        std::int32_t depth,
        float originX,
        float originY,
        std::int32_t floorIndex,
        std::int32_t layerStart,
        std::int32_t layerCount,
        float focus)
    {
        const float baseZ = floorBaseZ(floorIndex);

        for (std::int32_t localY = 0; localY < depth; ++localY)
        {
            for (std::int32_t localX = 0; localX < width; ++localX)
            {
                const auto srcX = sourceStartX + localX;
                const auto srcY = sourceStartY + localY;
                const float xf = originX + static_cast<float>(localX) * kCubeSize;
                const float yf = originY + static_cast<float>(localY) * kCubeSize;

                for (std::int32_t localLayer = 1; localLayer <= layerCount; ++localLayer)
                {
                    if (!cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX, srcY, layerStart, layerCount, localLayer))
                    {
                        continue;
                    }

                    const auto absoluteLayer = layerStart + localLayer - 1;
                    const float z0 = baseZ + static_cast<float>(localLayer - 1) * kCubeSize;
                    const float z1 = baseZ + static_cast<float>(localLayer) * kCubeSize;
                    const auto faceColor = applyFocus(levelColor(absoluteLayer), focus);

                    if (!cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX, srcY, layerStart, layerCount, localLayer + 1))
                    {
                        drawQuadTop(xf, yf, z1, faceColor);
                    }

                    if (!cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX + 1, srcY, layerStart, layerCount, localLayer))
                    {
                        drawQuadEast(xf + kCubeSize, yf, z0, z1, faceColor);
                    }

                    if (!cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX, srcY + 1, layerStart, layerCount, localLayer))
                    {
                        drawQuadNorth(xf, xf + kCubeSize, yf + kCubeSize, z0, z1, faceColor);
                    }

                    if (!cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX - 1, srcY, layerStart, layerCount, localLayer))
                    {
                        drawQuadEast(xf, yf, z1, z0, faceColor);
                    }

                    if (!cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX, srcY - 1, layerStart, layerCount, localLayer))
                    {
                        drawQuadNorth(xf + kCubeSize, xf, yf, z1, z0, faceColor);
                    }
                }
            }
        }
    }

    void drawTopLinesPatchImmediate(
        std::int32_t sourceFloorIndex,
        std::int32_t sourceStartX,
        std::int32_t sourceStartY,
        std::int32_t width,
        std::int32_t depth,
        float originX,
        float originY,
        std::int32_t floorIndex,
        std::int32_t layerStart,
        std::int32_t layerCount,
        float focus)
    {
        const float baseZ = floorBaseZ(floorIndex);
        constexpr float kLineLift = 0.06f;
        const auto color = applyFocus(Color3f{0.03f, 0.05f, 0.10f}, focus);

        for (std::int32_t localY = 0; localY < depth; ++localY)
        {
            for (std::int32_t localX = 0; localX < width; ++localX)
            {
                const auto srcX = sourceStartX + localX;
                const auto srcY = sourceStartY + localY;
                const float xf = originX + static_cast<float>(localX) * kCubeSize;
                const float yf = originY + static_cast<float>(localY) * kCubeSize;

                for (std::int32_t localLayer = 1; localLayer <= layerCount; ++localLayer)
                {
                    if (!cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX, srcY, layerStart, layerCount, localLayer))
                    {
                        continue;
                    }

                    if (cubeExistsInPatchBand(sourceFloorIndex, sourceStartX, sourceStartY, width, depth, srcX, srcY, layerStart, layerCount, localLayer + 1))
                    {
                        continue;
                    }

                    const float zf = baseZ + static_cast<float>(localLayer) * kCubeSize + kLineLift;
                    glBegin(GL_LINE_LOOP);
                    setColor(color, 0.35f + 0.65f * focus);
                    emitVertex(xf, yf, zf);
                    emitVertex(xf + kCubeSize, yf, zf);
                    emitVertex(xf + kCubeSize, yf + kCubeSize, zf);
                    emitVertex(xf, yf + kCubeSize, zf);
                    glEnd();
                }
            }
        }
    }

    void drawBuildMarker()
    {
        const auto location = currentCursorLocation();
        if (!location.valid)
        {
            return;
        }

        constexpr float kLift = 0.08f;
        const float x0 = location.originX;
        const float y0 = location.originY;
        const float x1 = x0 + kCubeSize;
        const float y1 = y0 + kCubeSize;
        const float z = location.markerBaseZ + kLift;
        const float guideBase = location.guideBaseZ + 0.02f;
        const auto outline = Color3f{0.20f, 0.95f, 0.98f};
        const auto guide = Color3f{0.18f, 0.75f, 0.96f};

        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        setColor(outline, 1.0f);
        emitVertex(x0, y0, z);
        emitVertex(x1, y0, z);
        emitVertex(x1, y1, z);
        emitVertex(x0, y1, z);
        glEnd();

        glBegin(GL_LINES);
        setColor(guide, 0.75f);
        emitVertex(x0, y0, z);
        emitVertex(x0, y0, guideBase);
        emitVertex(x1, y0, z);
        emitVertex(x1, y0, guideBase);
        emitVertex(x1, y1, z);
        emitVertex(x1, y1, guideBase);
        emitVertex(x0, y1, z);
        emitVertex(x0, y1, guideBase);
        glEnd();
        glLineWidth(1.0f);
    }

    void setProjection()
    {
        glViewport(0, 0, gWindowWidth, gWindowHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        const float aspect = static_cast<float>(gWindowWidth) / static_cast<float>(gWindowHeight);
        const float sceneHeight = totalSceneHeight();
        const float worldHalfExtent = std::max({
            sceneSpanX(),
            sceneSpanY(),
            sceneHeight * 1.62f
        }) * 0.80f / gZoom;

        const float depthHalfRange = 1600.0f;

        if (aspect >= 1.0f)
        {
            glOrtho(-worldHalfExtent * aspect, worldHalfExtent * aspect,
                    -worldHalfExtent, worldHalfExtent,
                    -depthHalfRange, depthHalfRange);
        }
        else
        {
            glOrtho(-worldHalfExtent, worldHalfExtent,
                    -worldHalfExtent / aspect, worldHalfExtent / aspect,
                    -depthHalfRange, depthHalfRange);
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        const float centerX = sceneSpanX() * 0.5f;
        const float centerY = sceneSpanY() * 0.5f;
        const float centerZ = totalSceneHeight() * 0.5f;
        const float radiusXY = 620.0f;
        const float cameraX = centerX + std::cos(gCameraAngle) * radiusXY;
        const float cameraY = centerY + std::sin(gCameraAngle) * radiusXY;
        const float cameraZ = centerZ + 380.0f;

        gluLookAt(cameraX,
                  cameraY,
                  cameraZ,
                  centerX,
                  centerY,
                  centerZ,
                  0.0f,
                  0.0f,
                  1.0f);
    }

    void drawBitmapText(float x, float y, void* font, const std::string& text)
    {
        glRasterPos2f(x, y);
        for (const unsigned char c : text)
        {
            glutBitmapCharacter(font, c);
        }
    }

    void drawHud()
    {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, static_cast<double>(gWindowWidth), 0.0, static_cast<double>(gWindowHeight), -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
        glColor4f(0.03f, 0.04f, 0.07f, 0.76f);
        glVertex2f(22.0f, static_cast<float>(gWindowHeight) - 168.0f);
        glVertex2f(980.0f, static_cast<float>(gWindowHeight) - 168.0f);
        glColor4f(0.08f, 0.10f, 0.16f, 0.88f);
        glVertex2f(980.0f, static_cast<float>(gWindowHeight) - 20.0f);
        glVertex2f(22.0f, static_cast<float>(gWindowHeight) - 20.0f);
        glEnd();

        glColor4f(0.95f, 0.97f, 1.0f, 0.96f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 42.0f, GLUT_BITMAP_HELVETICA_18, "Block Stage View");
        glColor4f(0.72f, 0.79f, 0.93f, 0.92f);
        const std::string mode = gFourFloorMode ? "Mode: 4 floors" : "Mode: 1 floor";
        const std::string layout = gIslandMode ? "Layout: 4 islands" : "Layout: full grid";
        const std::string view = gDetailView ? "View: focused" : "View: stage";
        const std::string gMode = gDetailView ? "G split-focus" : "G islands";
        const std::string controls = splitIslandViewActive()
            ? "arrows select   Enter focus   K save   L load   Esc back   ,/. rotate   F switch   " + gMode + "   N regenerate   J key   U scale   =/- zoom   0 reset"
            : "WASD move   Q/E z   R place   T remove   Enter focus   K save   L load   Esc back   ,/. rotate   F switch   " + gMode + "   N regenerate   J key   U scale   =/- zoom   0 reset";
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 66.0f, GLUT_BITMAP_HELVETICA_12, mode + "   " + layout + "   " + view + "   " + controls);
        glColor4f(0.62f, 0.72f, 0.88f, 0.90f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 88.0f, GLUT_BITMAP_HELVETICA_12, pitchCycleSummary());
        glColor4f(0.78f, 0.84f, 0.96f, 0.92f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 110.0f, GLUT_BITMAP_HELVETICA_12, buildStatusSummary());
        glColor4f(0.86f, 0.90f, 0.98f, 0.94f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 132.0f, GLUT_BITMAP_HELVETICA_12, activeSelectionSummary());
        glColor4f(0.73f, 0.82f, 0.98f, 0.92f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 154.0f, GLUT_BITMAP_HELVETICA_12, gSaveStatus);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    void drawSceneImmediate()
    {
        if (gDetailView)
        {
            const auto sourceStartX = islandSourceStartX(gActiveIsland);
            const auto sourceStartY = islandSourceStartY(gActiveIsland);
            const auto layerStart = 1 + gActiveFloor * kPitchBandHeight;

            if (!gDetailSubIslandMode)
            {
                drawPlatform(0.0f, 0.0f, kIslandSize, kIslandSize, 0, 1.0f);
                drawBlockPatchImmediate(gActiveFloor, sourceStartX, sourceStartY, kIslandSize, kIslandSize, 0.0f, 0.0f, 0, layerStart, kPitchBandHeight, 1.0f);
                drawTopLinesPatchImmediate(gActiveFloor, sourceStartX, sourceStartY, kIslandSize, kIslandSize, 0.0f, 0.0f, 0, layerStart, kPitchBandHeight, 1.0f);
                return;
            }

            if (gDetailQuarterView)
            {
                const auto subSourceStartX = sourceStartX + (gActiveDetailSubIsland % 2) * kFocusedSubIslandSize;
                const auto subSourceStartY = sourceStartY + (gActiveDetailSubIsland / 2) * kFocusedSubIslandSize;
                drawPlatform(0.0f, 0.0f, kFocusedSubIslandSize, kFocusedSubIslandSize, 0, 1.0f);
                drawBlockPatchImmediate(gActiveFloor, subSourceStartX, subSourceStartY, kFocusedSubIslandSize, kFocusedSubIslandSize, 0.0f, 0.0f, 0, layerStart, kPitchBandHeight, 1.0f);
                drawTopLinesPatchImmediate(gActiveFloor, subSourceStartX, subSourceStartY, kFocusedSubIslandSize, kFocusedSubIslandSize, 0.0f, 0.0f, 0, layerStart, kPitchBandHeight, 1.0f);
                return;
            }

            for (std::int32_t subIslandIndex = 0; subIslandIndex < kFocusedSubIslandCount; ++subIslandIndex)
            {
                const auto subSourceStartX = sourceStartX + (subIslandIndex % 2) * kFocusedSubIslandSize;
                const auto subSourceStartY = sourceStartY + (subIslandIndex / 2) * kFocusedSubIslandSize;
                const float originX = focusedSubIslandOriginX(subIslandIndex);
                const float originY = focusedSubIslandOriginY(subIslandIndex);
                const float focus = focusForDetailSubIsland(subIslandIndex);
                drawPlatform(originX, originY, kFocusedSubIslandSize, kFocusedSubIslandSize, 0, focus);
                drawBlockPatchImmediate(gActiveFloor, subSourceStartX, subSourceStartY, kFocusedSubIslandSize, kFocusedSubIslandSize, originX, originY, 0, layerStart, kPitchBandHeight, focus);
                drawTopLinesPatchImmediate(gActiveFloor, subSourceStartX, subSourceStartY, kFocusedSubIslandSize, kFocusedSubIslandSize, originX, originY, 0, layerStart, kPitchBandHeight, focus);
            }
            return;
        }

        if (!gIslandMode)
        {
            for (std::int32_t floorIndex = 0; floorIndex < activeFloorCount(); ++floorIndex)
            {
                drawPlatform(0.0f, 0.0f, kGridWidth, kGridDepth, floorIndex, 1.0f);
                drawBlockPatchImmediate(floorIndex, 0, 0, kGridWidth, kGridDepth, 0.0f, 0.0f, floorIndex, 1, kGridHeight, 1.0f);
                drawTopLinesPatchImmediate(floorIndex, 0, 0, kGridWidth, kGridDepth, 0.0f, 0.0f, floorIndex, 1, kGridHeight, 1.0f);
            }
            return;
        }

        for (std::int32_t islandIndex = 0; islandIndex < kIslandCount; ++islandIndex)
        {
            const float originX = islandOriginX(islandIndex);
            const float originY = islandOriginY(islandIndex);
            const auto sourceStartX = islandSourceStartX(islandIndex);
            const auto sourceStartY = islandSourceStartY(islandIndex);

            if (!gFourFloorMode)
            {
                drawPlatform(originX, originY, kIslandSize, kIslandSize, 0, 1.0f);
                drawBlockPatchImmediate(islandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, 0, 1, kGridHeight, 1.0f);
                drawTopLinesPatchImmediate(islandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, 0, 1, kGridHeight, 1.0f);
                continue;
            }

            for (std::int32_t bandIndex = 0; bandIndex < kFloorCount; ++bandIndex)
            {
                const float focus = focusForRegion(islandIndex, bandIndex);
                drawPlatform(originX, originY, kIslandSize, kIslandSize, bandIndex, focus);
                drawBlockPatchImmediate(bandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, bandIndex, 1 + bandIndex * kPitchBandHeight, kPitchBandHeight, focus);
                drawTopLinesPatchImmediate(bandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, bandIndex, 1 + bandIndex * kPitchBandHeight, kPitchBandHeight, focus);
            }
        }
    }

    void clampActiveSelection()
    {
        gActiveIsland = std::clamp(gActiveIsland, 0, kIslandCount - 1);
        gActiveFloor = std::clamp(gActiveFloor, 0, kFloorCount - 1);
    }

    void syncActiveSelection()
    {
        if (!gIslandMode || !gFourFloorMode)
        {
            gDetailView = false;
            gDetailSubIslandMode = false;
            gDetailQuarterView = false;
            clampBuildCursor();
            return;
        }

        clampActiveSelection();
        setSelectionFromSpiralIndex(spiralIndexForSelection(gActiveIsland, gActiveFloor));
        clampBuildCursor();
    }

    void display()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawBackdrop();
        setProjection();

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LINE_SMOOTH);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        drawSceneImmediate();
        glDisable(GL_POLYGON_OFFSET_FILL);
        drawBuildMarker();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawHud();

        glutSwapBuffers();
    }

    void reshape(int width, int height)
    {
        gWindowWidth = std::max(width, 1);
        gWindowHeight = std::max(height, 1);
        syncActiveSelection();
        glutPostRedisplay();
    }

    void keyboard(unsigned char key, int, int)
    {
        const auto buildContext = currentBuildContext();

        if (buildContext.valid && (key == kKeyWLower || key == kKeyWUpper))
        {
            gCursorY = std::max(gCursorY - 1, 0);
        }
        else if (buildContext.valid && (key == kKeySLower || key == kKeySUpper))
        {
            gCursorY = std::min(gCursorY + 1, buildContext.depth - 1);
        }
        else if (buildContext.valid && (key == kKeyALower || key == kKeyAUpper))
        {
            gCursorX = std::max(gCursorX - 1, 0);
        }
        else if (buildContext.valid && (key == kKeyDLower || key == kKeyDUpper))
        {
            gCursorX = std::min(gCursorX + 1, buildContext.width - 1);
        }
        else if (buildContext.valid && (key == kKeyQLower || key == kKeyQUpper))
        {
            gCursorZ = std::max(gCursorZ - 1, 0);
        }
        else if (buildContext.valid && (key == kKeyELower || key == kKeyEUpper))
        {
            gCursorZ = std::min(gCursorZ + 1, buildContext.totalLayers - 1);
        }
        else if (buildContext.valid && (key == kKeyRLower || key == kKeyRUpper))
        {
            const auto location = currentCursorLocation();
            if (location.valid)
            {
                setBlock(location.sourceFloorIndex, location.sourceX, location.sourceY, location.localZ, true);
            }
        }
        else if (buildContext.valid && (key == kKeyTLower || key == kKeyTUpper))
        {
            const auto location = currentCursorLocation();
            if (location.valid)
            {
                setBlock(location.sourceFloorIndex, location.sourceX, location.sourceY, location.localZ, false);
            }
        }
        else if (key == kKeyEquals)
        {
            gZoom = std::min(gZoom * 1.25f, kMaxZoom);
        }
        else if (key == kKeyMinus || key == kKeyUnderscore)
        {
            gZoom = std::max(gZoom / 1.25f, kMinZoom);
        }
        else if (key == kKeyZero)
        {
            gZoom = kMinZoom;
        }
        else if (key == kKeyEnter)
        {
            if (gDetailView && gDetailSubIslandMode && !gDetailQuarterView)
            {
                gDetailQuarterView = true;
                gZoom = kMinZoom;
            }
            else if (gIslandMode && gFourFloorMode && !gDetailView)
            {
                gDetailView = true;
                gDetailSubIslandMode = false;
                gDetailQuarterView = false;
                gZoom = kMinZoom;
            }
        }
        else if (key == kKeyFLower || key == kKeyFUpper)
        {
            gFourFloorMode = !gFourFloorMode;
            gDetailView = false;
            gDetailSubIslandMode = false;
            gDetailQuarterView = false;
            if (gIslandMode && gFourFloorMode)
            {
                resetSelectionToSpiralStart();
            }
        }
        else if (key == kKeyNLower || key == kKeyNUpper)
        {
            gSeed = hashU32(gSeed + 0x9e3779b9u);
            regenerateBlocks();
        }
        else if (key == kKeyKLower || key == kKeyKUpper)
        {
            const auto path = chooseSavePath();
            if (path.empty())
            {
                gSaveStatus = "Save canceled";
            }
            else if (saveWorldToFile(path))
            {
                gSavePath = path;
                gAutosaveEnabled = true;
                gSaveStatus = "Save: " + path;
            }
            else
            {
                gSaveStatus = "Save failed: " + path;
            }
        }
        else if (key == kKeyLLower || key == kKeyLUpper)
        {
            const auto path = !gSavePath.empty() ? gSavePath : latestSavePath();
            if (path.empty())
            {
                gSaveStatus = "Load failed: no .drd file";
            }
            else if (loadWorldFromFile(path))
            {
                gSavePath = path;
                gAutosaveEnabled = true;
                gSaveStatus = "Load: " + path;
            }
            else
            {
                gSaveStatus = "Load failed: " + path;
            }
        }
        else if (key == kKeyGLower || key == kKeyGUpper)
        {
            if (gDetailView)
            {
                if (!gDetailQuarterView)
                {
                    gDetailSubIslandMode = !gDetailSubIslandMode;
                    if (!gDetailSubIslandMode)
                    {
                        gActiveDetailSubIsland = 0;
                    }
                }
            }
            else
            {
                gIslandMode = !gIslandMode;
                gDetailView = false;
                gDetailSubIslandMode = false;
                gDetailQuarterView = false;
                if (gIslandMode && gFourFloorMode)
                {
                    resetSelectionToSpiralStart();
                }
            }
        }
        else if (key == kKeyJLower || key == kKeyJUpper)
        {
            gSelectedKey = (gSelectedKey + 1) % 12;
        }
        else if (key == kKeyULower || key == kKeyUUpper)
        {
            gSelectedScale = (gSelectedScale + 1) % (sizeof(kScaleDefinitions) / sizeof(kScaleDefinitions[0]));
        }
        else if (key == kKeyComma)
        {
            gCameraAngle -= 1.57079632679f;
        }
        else if (key == kKeyPeriod)
        {
            gCameraAngle += 1.57079632679f;
        }
        else if (key == kKeyEscape)
        {
            if (gDetailQuarterView)
            {
                gDetailQuarterView = false;
            }
            else if (gDetailView)
            {
                gDetailView = false;
                gDetailSubIslandMode = false;
                gDetailQuarterView = false;
            }
            else
            {
                gDetailView = false;
                gDetailSubIslandMode = false;
                gDetailQuarterView = false;
            }
        }

        syncActiveSelection();
        glutPostRedisplay();
    }

    void special(int key, int, int)
    {
        if (currentBuildContext().valid)
        {
            return;
        }

        if (gDetailView && gDetailSubIslandMode && !gDetailQuarterView)
        {
            auto subIsland = gActiveDetailSubIsland;

            if (key == kSpecialLeft && (subIsland % 2) == 1)
            {
                subIsland -= 1;
            }
            else if (key == kSpecialRight && (subIsland % 2) == 0)
            {
                subIsland += 1;
            }
            else if (key == kSpecialUp && subIsland >= 2)
            {
                subIsland -= 2;
            }
            else if (key == kSpecialDown && subIsland < 2)
            {
                subIsland += 2;
            }
            else
            {
                return;
            }

            gActiveDetailSubIsland = std::clamp(subIsland, 0, kFocusedSubIslandCount - 1);
            glutPostRedisplay();
            return;
        }

        if (!gIslandMode || !gFourFloorMode)
        {
            return;
        }

        auto spiralIndex = spiralIndexForSelection(gActiveIsland, gActiveFloor);

        if (key == kSpecialLeft || key == kSpecialUp)
        {
            spiralIndex = (spiralIndex + (kIslandCount * kFloorCount) - 1) % (kIslandCount * kFloorCount);
        }
        else if (key == kSpecialRight || key == kSpecialDown)
        {
            spiralIndex = (spiralIndex + 1) % (kIslandCount * kFloorCount);
        }
        else
        {
            return;
        }

        setSelectionFromSpiralIndex(spiralIndex);
        glutPostRedisplay();
    }
}

int main(int argc, char** argv)
{
    regenerateBlocks();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutCreateWindow("Block Stage View");

    glClearColor(kSkyBottom.r, kSkyBottom.g, kSkyBottom.b, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(kAutosaveIntervalMs, autosaveCallback, 0);

    glutMainLoop();
    return 0;
}
