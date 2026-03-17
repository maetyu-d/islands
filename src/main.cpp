#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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
    constexpr unsigned char kKeyKUpper = 75;
    constexpr unsigned char kKeyKLower = 107;
    constexpr unsigned char kKeySUpper = 83;
    constexpr unsigned char kKeySLower = 115;
    constexpr unsigned char kKeyComma = 44;
    constexpr unsigned char kKeyPeriod = 46;
    constexpr unsigned char kKeyEscape = 27;

    struct Color3f
    {
        float r;
        float g;
        float b;
    };

    struct MeshVertex
    {
        float x;
        float y;
        float z;
        float r;
        float g;
        float b;
    };

    struct SceneMesh
    {
        std::vector<MeshVertex> quads;
        std::vector<MeshVertex> lines;
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
    std::int32_t gSelectedKey = 0;
    std::size_t gSelectedScale = 1;
    std::uint32_t gSeed = 0xC0FFEEu;
    std::vector<std::uint8_t> gHeights(kFloorCount * kGridWidth * kGridDepth);
    SceneMesh gSceneMesh;
    bool gSceneMeshDirty = true;

    constexpr Color3f kSkyTop = {0.06f, 0.08f, 0.16f};
    constexpr Color3f kSkyBottom = {0.01f, 0.01f, 0.03f};
    constexpr Color3f kHorizonGlow = {0.16f, 0.20f, 0.34f};
    constexpr Color3f kOutline = {0.93f, 0.95f, 0.99f};
    constexpr Color3f kShadow = {0.03f, 0.03f, 0.05f};

    std::size_t layerStride()
    {
        return static_cast<std::size_t>(kGridWidth) * kGridDepth;
    }

    std::size_t indexFor(std::int32_t floorIndex, std::int32_t x, std::int32_t y)
    {
        return static_cast<std::size_t>(floorIndex) * layerStride()
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
        for (std::int32_t floorIndex = 0; floorIndex < kFloorCount; ++floorIndex)
        {
            for (std::int32_t y = 0; y < kGridDepth; ++y)
            {
                for (std::int32_t x = 0; x < kGridWidth; ++x)
                {
                    gHeights[indexFor(floorIndex, x, y)] = sampleHeight(floorIndex, x, y);
                }
            }
        }
    }

    std::int32_t activeFloorCount()
    {
        return gFourFloorMode ? kFloorCount : 1;
    }

    std::int32_t activeSegmentLayerCount()
    {
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
        if (!gIslandMode)
        {
            return static_cast<float>(kGridWidth) * kCubeSize;
        }

        return static_cast<float>(kIslandSize * 2 + kIslandGap) * kCubeSize;
    }

    float sceneSpanY()
    {
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

    std::int32_t islandSourceStartX(std::int32_t islandIndex)
    {
        return (islandIndex % 2 == 0) ? 32 : (kGridWidth - kIslandSize - 32);
    }

    std::int32_t islandSourceStartY(std::int32_t islandIndex)
    {
        return (islandIndex / 2 == 0) ? 32 : (kGridDepth - kIslandSize - 32);
    }

    std::int32_t blockHeight(std::int32_t floorIndex, std::int32_t x, std::int32_t y)
    {
        if (floorIndex < 0 || floorIndex >= kFloorCount || x < 0 || x >= kGridWidth || y < 0 || y >= kGridDepth)
        {
            return 0;
        }
        return gHeights[indexFor(floorIndex, x, y)];
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

    void setColor(const Color3f& color, float alpha)
    {
        glColor4f(color.r, color.g, color.b, alpha);
    }

    void emitVertex(float x, float y, float z)
    {
        glVertex3f(x, y, z);
    }

    void appendVertex(std::vector<MeshVertex>& vertices, float x, float y, float z, const Color3f& color)
    {
        vertices.push_back({x, y, z, color.r, color.g, color.b});
    }

    void appendQuad(
        std::vector<MeshVertex>& vertices,
        float x0,
        float y0,
        float z0,
        float x1,
        float y1,
        float z1,
        float x2,
        float y2,
        float z2,
        float x3,
        float y3,
        float z3,
        const Color3f& c0,
        const Color3f& c1,
        const Color3f& c2,
        const Color3f& c3)
    {
        appendVertex(vertices, x0, y0, z0, c0);
        appendVertex(vertices, x1, y1, z1, c1);
        appendVertex(vertices, x2, y2, z2, c2);
        appendVertex(vertices, x3, y3, z3, c3);
    }

    void appendSolidQuad(
        std::vector<MeshVertex>& vertices,
        float x0,
        float y0,
        float z0,
        float x1,
        float y1,
        float z1,
        float x2,
        float y2,
        float z2,
        float x3,
        float y3,
        float z3,
        const Color3f& color)
    {
        appendQuad(vertices, x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, color, color, color, color);
    }

    void appendLine(
        std::vector<MeshVertex>& vertices,
        float x0,
        float y0,
        float z0,
        float x1,
        float y1,
        float z1,
        const Color3f& color)
    {
        appendVertex(vertices, x0, y0, z0, color);
        appendVertex(vertices, x1, y1, z1, color);
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

        for (std::int32_t x = 0; x <= width; x += 8)
        {
            const float xf = originX + static_cast<float>(x) * kCubeSize;
            emitVertex(xf, originY, z);
            emitVertex(xf, maxY, z);
        }

        for (std::int32_t y = 0; y <= depth; y += 8)
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
        std::int32_t floorIndex)
    {
        const float maxX = originX + static_cast<float>(width) * kCubeSize;
        const float maxY = originY + static_cast<float>(depth) * kCubeSize;
        const float topZ = floorBaseZ(floorIndex);
        const float bottomZ = topZ - kSlabThickness;
        const float sideTopZ = topZ - 0.12f;
        const auto topColor = Color3f{0.05f, 0.07f, 0.14f};
        const auto eastColor = Color3f{0.11f, 0.16f, 0.30f};
        const auto northColor = Color3f{0.08f, 0.11f, 0.22f};

        glBegin(GL_QUADS);
        setColor(topColor, 1.0f);
        emitVertex(originX, originY, topZ);
        emitVertex(maxX, originY, topZ);
        emitVertex(maxX, maxY, topZ);
        emitVertex(originX, maxY, topZ);
        glEnd();

        drawPlatformGrid(originX, originY, width, depth, topZ + 0.02f);

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

    bool cubeExistsAtLocalLayer(std::int32_t sourceFloorIndex, std::int32_t x, std::int32_t y, std::int32_t localLayer)
    {
        return blockHeight(sourceFloorIndex, x, y) >= localLayer;
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

        return cubeExistsAtLocalLayer(sourceFloorIndex, x, y, localLayer) && isLayerAllowed(layerStart + localLayer - 1);
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

    void appendPlatformGrid(std::vector<MeshVertex>& lines, float originX, float originY, std::int32_t width, std::int32_t depth, float z)
    {
        const auto color = Color3f{0.23f, 0.29f, 0.43f};
        const float maxX = originX + static_cast<float>(width) * kCubeSize;
        const float maxY = originY + static_cast<float>(depth) * kCubeSize;

        for (std::int32_t x = 0; x <= width; ++x)
        {
            const float xf = originX + static_cast<float>(x) * kCubeSize;
            appendLine(lines, xf, originY, z, xf, maxY, z, color);
        }

        for (std::int32_t y = 0; y <= depth; ++y)
        {
            const float yf = originY + static_cast<float>(y) * kCubeSize;
            appendLine(lines, originX, yf, z, maxX, yf, z, color);
        }
    }

    void appendPlatformPatch(
        SceneMesh& mesh,
        float originX,
        float originY,
        std::int32_t width,
        std::int32_t depth,
        std::int32_t floorIndex)
    {
        const float maxX = originX + static_cast<float>(width) * kCubeSize;
        const float maxY = originY + static_cast<float>(depth) * kCubeSize;
        const float topZ = floorBaseZ(floorIndex);
        const float bottomZ = topZ - kSlabThickness;
        const float sideTopZ = topZ - 0.12f;
        const auto topColor = Color3f{0.05f, 0.07f, 0.14f};
        const auto eastColor = Color3f{0.11f, 0.16f, 0.30f};
        const auto northColor = Color3f{0.08f, 0.11f, 0.22f};

        appendSolidQuad(mesh.quads, originX, originY, topZ, maxX, originY, topZ, maxX, maxY, topZ, originX, maxY, topZ, topColor);
        appendPlatformGrid(mesh.lines, originX, originY, width, depth, topZ + 0.02f);
        appendSolidQuad(mesh.quads, maxX, originY, bottomZ, maxX, maxY, bottomZ, maxX, maxY, sideTopZ, maxX, originY, sideTopZ, eastColor);
        appendSolidQuad(mesh.quads, originX, maxY, bottomZ, maxX, maxY, bottomZ, maxX, maxY, sideTopZ, originX, maxY, sideTopZ, northColor);
    }

    void appendBlockPatch(
        SceneMesh& mesh,
        std::int32_t sourceFloorIndex,
        std::int32_t sourceStartX,
        std::int32_t sourceStartY,
        std::int32_t width,
        std::int32_t depth,
        float originX,
        float originY,
        std::int32_t floorIndex,
        std::int32_t layerStart,
        std::int32_t layerCount)
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
                    if (!cubeExistsInBand(sourceFloorIndex, srcX, srcY, layerStart, layerCount, localLayer))
                    {
                        continue;
                    }

                    const auto absoluteLayer = layerStart + localLayer - 1;
                    const float z0 = baseZ + static_cast<float>(localLayer - 1) * kCubeSize;
                    const float z1 = baseZ + static_cast<float>(localLayer) * kCubeSize;

                    if (!cubeExistsInBand(sourceFloorIndex, srcX, srcY, layerStart, layerCount, localLayer + 1))
                    {
                        appendSolidQuad(mesh.quads, xf, yf, z1, xf + kCubeSize, yf, z1, xf + kCubeSize, yf + kCubeSize, z1, xf, yf + kCubeSize, z1, levelColor(absoluteLayer));
                    }

                    if (!cubeExistsInBand(sourceFloorIndex, srcX + 1, srcY, layerStart, layerCount, localLayer))
                    {
                        const auto shadeA = scaleColor(levelColor(absoluteLayer), 0.58f);
                        const auto shadeB = scaleColor(levelColor(absoluteLayer), 0.75f);
                        appendQuad(
                            mesh.quads,
                            xf + kCubeSize, yf, z0,
                            xf + kCubeSize, yf + kCubeSize, z0,
                            xf + kCubeSize, yf + kCubeSize, z1,
                            xf + kCubeSize, yf, z1,
                            shadeA, shadeA, shadeB, shadeB);
                    }

                    if (!cubeExistsInBand(sourceFloorIndex, srcX, srcY + 1, layerStart, layerCount, localLayer))
                    {
                        const auto shadeA = scaleColor(levelColor(absoluteLayer), 0.48f);
                        const auto shadeB = scaleColor(levelColor(absoluteLayer), 0.65f);
                        appendQuad(
                            mesh.quads,
                            xf, yf + kCubeSize, z0,
                            xf + kCubeSize, yf + kCubeSize, z0,
                            xf + kCubeSize, yf + kCubeSize, z1,
                            xf, yf + kCubeSize, z1,
                            shadeA, shadeA, shadeB, shadeB);
                    }

                    if (!cubeExistsInBand(sourceFloorIndex, srcX - 1, srcY, layerStart, layerCount, localLayer))
                    {
                        const auto shadeA = scaleColor(levelColor(absoluteLayer), 0.54f);
                        const auto shadeB = scaleColor(levelColor(absoluteLayer), 0.70f);
                        appendQuad(
                            mesh.quads,
                            xf, yf + kCubeSize, z0,
                            xf, yf, z0,
                            xf, yf, z1,
                            xf, yf + kCubeSize, z1,
                            shadeA, shadeA, shadeB, shadeB);
                    }

                    if (!cubeExistsInBand(sourceFloorIndex, srcX, srcY - 1, layerStart, layerCount, localLayer))
                    {
                        const auto shadeA = scaleColor(levelColor(absoluteLayer), 0.44f);
                        const auto shadeB = scaleColor(levelColor(absoluteLayer), 0.60f);
                        appendQuad(
                            mesh.quads,
                            xf + kCubeSize, yf, z0,
                            xf, yf, z0,
                            xf, yf, z1,
                            xf + kCubeSize, yf, z1,
                            shadeA, shadeA, shadeB, shadeB);
                    }
                }
            }
        }
    }

    void appendTopLinesPatch(
        SceneMesh& mesh,
        std::int32_t sourceFloorIndex,
        std::int32_t sourceStartX,
        std::int32_t sourceStartY,
        std::int32_t width,
        std::int32_t depth,
        float originX,
        float originY,
        std::int32_t floorIndex,
        std::int32_t layerStart,
        std::int32_t layerCount)
    {
        const float baseZ = floorBaseZ(floorIndex);
        constexpr float kLineLift = 0.06f;
        const auto color = Color3f{0.03f, 0.05f, 0.10f};

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
                    if (!cubeExistsInBand(sourceFloorIndex, srcX, srcY, layerStart, layerCount, localLayer))
                    {
                        continue;
                    }

                    if (cubeExistsInBand(sourceFloorIndex, srcX, srcY, layerStart, layerCount, localLayer + 1))
                    {
                        continue;
                    }

                    const float zf = baseZ + static_cast<float>(localLayer) * kCubeSize + kLineLift;
                    appendLine(mesh.lines, xf, yf, zf, xf + kCubeSize, yf, zf, color);
                    appendLine(mesh.lines, xf + kCubeSize, yf, zf, xf + kCubeSize, yf + kCubeSize, zf, color);
                    appendLine(mesh.lines, xf + kCubeSize, yf + kCubeSize, zf, xf, yf + kCubeSize, zf, color);
                    appendLine(mesh.lines, xf, yf + kCubeSize, zf, xf, yf, zf, color);
                }
            }
        }
    }

    void rebuildSceneMesh()
    {
        SceneMesh mesh;

        if (!gIslandMode)
        {
            for (std::int32_t floorIndex = 0; floorIndex < activeFloorCount(); ++floorIndex)
            {
                appendPlatformPatch(mesh, 0.0f, 0.0f, kGridWidth, kGridDepth, floorIndex);
                appendBlockPatch(mesh, floorIndex, 0, 0, kGridWidth, kGridDepth, 0.0f, 0.0f, floorIndex, 1, kGridHeight);
                appendTopLinesPatch(mesh, floorIndex, 0, 0, kGridWidth, kGridDepth, 0.0f, 0.0f, floorIndex, 1, kGridHeight);
            }
        }
        else
        {
            for (std::int32_t islandIndex = 0; islandIndex < kIslandCount; ++islandIndex)
            {
                const float originX = islandOriginX(islandIndex);
                const float originY = islandOriginY(islandIndex);
                const auto sourceStartX = islandSourceStartX(islandIndex);
                const auto sourceStartY = islandSourceStartY(islandIndex);

                if (!gFourFloorMode)
                {
                    appendPlatformPatch(mesh, originX, originY, kIslandSize, kIslandSize, 0);
                    appendBlockPatch(mesh, islandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, 0, 1, kGridHeight);
                    appendTopLinesPatch(mesh, islandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, 0, 1, kGridHeight);
                    continue;
                }

                for (std::int32_t bandIndex = 0; bandIndex < kFloorCount; ++bandIndex)
                {
                    appendPlatformPatch(mesh, originX, originY, kIslandSize, kIslandSize, bandIndex);
                    appendBlockPatch(mesh, bandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, bandIndex, 1 + bandIndex * kPitchBandHeight, kPitchBandHeight);
                    appendTopLinesPatch(mesh, bandIndex, sourceStartX, sourceStartY, kIslandSize, kIslandSize, originX, originY, bandIndex, 1 + bandIndex * kPitchBandHeight, kPitchBandHeight);
                }
            }
        }

        gSceneMesh = std::move(mesh);
        gSceneMeshDirty = false;
    }

    void renderMeshVertices(const std::vector<MeshVertex>& vertices, GLenum mode)
    {
        if (vertices.empty())
        {
            return;
        }

        const auto* data = vertices.data();
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof(MeshVertex), reinterpret_cast<const GLvoid*>(&data->x));
        glColorPointer(3, GL_FLOAT, sizeof(MeshVertex), reinterpret_cast<const GLvoid*>(&data->r));
        glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
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
        glVertex2f(22.0f, static_cast<float>(gWindowHeight) - 146.0f);
        glVertex2f(980.0f, static_cast<float>(gWindowHeight) - 146.0f);
        glColor4f(0.08f, 0.10f, 0.16f, 0.88f);
        glVertex2f(980.0f, static_cast<float>(gWindowHeight) - 20.0f);
        glVertex2f(22.0f, static_cast<float>(gWindowHeight) - 20.0f);
        glEnd();

        glColor4f(0.95f, 0.97f, 1.0f, 0.96f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 42.0f, GLUT_BITMAP_HELVETICA_18, "Block Stage View");
        glColor4f(0.72f, 0.79f, 0.93f, 0.92f);
        const std::string mode = gFourFloorMode ? "Mode: 4 floors" : "Mode: 1 floor";
        const std::string layout = gIslandMode ? "Layout: 4 islands" : "Layout: full grid";
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 66.0f, GLUT_BITMAP_HELVETICA_12, mode + "   " + layout + "   ,/. rotate   F switch   G islands   N regenerate   K key   S scale   =/- zoom   0 reset");
        glColor4f(0.62f, 0.72f, 0.88f, 0.90f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 88.0f, GLUT_BITMAP_HELVETICA_12, pitchCycleSummary());
        glColor4f(0.78f, 0.84f, 0.96f, 0.92f);
        drawBitmapText(38.0f, static_cast<float>(gWindowHeight) - 110.0f, GLUT_BITMAP_HELVETICA_12, quantizerSummary());

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    void display()
    {
        if (gSceneMeshDirty)
        {
            rebuildSceneMesh();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawBackdrop();
        setProjection();

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LINE_SMOOTH);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        renderMeshVertices(gSceneMesh.quads, GL_QUADS);
        glDisable(GL_POLYGON_OFFSET_FILL);

        glDepthMask(GL_FALSE);
        renderMeshVertices(gSceneMesh.lines, GL_LINES);
        glDepthMask(GL_TRUE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawHud();

        glutSwapBuffers();
    }

    void reshape(int width, int height)
    {
        gWindowWidth = std::max(width, 1);
        gWindowHeight = std::max(height, 1);
        glutPostRedisplay();
    }

    void keyboard(unsigned char key, int, int)
    {
        if (key == kKeyEquals)
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
        else if (key == kKeyFLower || key == kKeyFUpper)
        {
            gFourFloorMode = !gFourFloorMode;
            gSceneMeshDirty = true;
        }
        else if (key == kKeyNLower || key == kKeyNUpper)
        {
            gSeed = hashU32(gSeed + 0x9e3779b9u);
            regenerateBlocks();
            gSceneMeshDirty = true;
        }
        else if (key == kKeyGLower || key == kKeyGUpper)
        {
            gIslandMode = !gIslandMode;
            gSceneMeshDirty = true;
        }
        else if (key == kKeyKLower || key == kKeyKUpper)
        {
            gSelectedKey = (gSelectedKey + 1) % 12;
            gSceneMeshDirty = true;
        }
        else if (key == kKeySLower || key == kKeySUpper)
        {
            gSelectedScale = (gSelectedScale + 1) % (sizeof(kScaleDefinitions) / sizeof(kScaleDefinitions[0]));
            gSceneMeshDirty = true;
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
            std::exit(0);
        }

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
    glutIdleFunc(display);

    glutMainLoop();
    return 0;
}
