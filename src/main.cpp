//
// Copyright (c) 2022 Christopher Gassib
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "config.h"
#include "pd.hpp"

#include <cassert>
#include <cmath>
#include <algorithm>
#include "clg-math/clg_rectangle.hpp"
#include "box2d/box2d.h"
#include "sin_table.hpp"
#include "memory.hpp"
#include "drawing.hpp"
#include "car_physics.hpp"

namespace clg
{
extern "C" // NOTE: <- This is sometimes unnessesary with the -g compile switch?
int update(void *userdata);
}

namespace game
{
// Game Code
/////////////

using PaintTextureFunc = void (*)(uint8_t* pCanvas, int width, int height);

float elapsedFrameTime;

clg::point p;
float step;
uint8_t* pHollowRectangle;
uint8_t* pTriangle;
uint8_t* pCheckerboard;
int compressedLinePitchWithTransparency;
clg::sizev b2Scale;
float b2Angle;
float cycle;
PDButtons held;

float ups;
float fps;

auto timerTotal = 0.0f;
auto timerCount = 0;
b2World* pWorldPhysics = nullptr;
clg::Car* pCarSim = nullptr;
clg::memory_arena* pLevelArena = nullptr;
clg::memory_arena* pFrameArena = nullptr;

bool InitializePhysics()
{
    b2Vec2 gravity(0.0f, 0.0f);
    pWorldPhysics = new (std::nothrow) b2World(gravity);
    if (nullptr == pWorldPhysics)
    {
        pd::error("ERROR: failed to allocated memory for world physics");
        return false;
    }

    b2Body* carBody;
    {
        b2BodyDef carBodyDef;
        carBodyDef.type = b2BodyType::b2_dynamicBody;
        carBody = pWorldPhysics->CreateBody(&carBodyDef);
    }
    // car fixture setup
    {
        b2PolygonShape carBox;
        carBox.SetAsBox(1.143f, 2.286f);
        b2FixtureDef carFixture;
        carFixture.shape = &carBox;
        carFixture.density = 1.0f; // TODO: make this more accurate???
        carBody->CreateFixture(&carFixture);
    }
    // car mass
    {
        // massData.center = b2Vec2();
        // massData.I = 0.0f; // TODO: before b2Body* is passed in?
        auto massData = carBody->GetMassData();
        massData.mass = (clg::Car::totalWeight - 4.0f * clg::Car::wheelWeight) / clg::formula::gravitationalAcceleration;
        carBody->SetMassData(&massData);
    }
    // tire bodies
    const b2Vec2 tirePos[4] =
    {
        b2Vec2(-0.84455f,  1.47955f), b2Vec2(0.84455f,  1.47955f),
        b2Vec2(-0.84455f, -1.47955f), b2Vec2(0.84455f, -1.47955f)
    };
    b2Body* tireBodies[4];
    {
        b2BodyDef tireBodyDef;
        tireBodyDef.type = b2BodyType::b2_dynamicBody;
        b2FixtureDef tireFixture; // 66.5inch track and 116.5inch wheelbase (0.84455m hx, 1.47955m hy)
        b2PolygonShape tireBox; // 245/35R21 tires (245mm wide; 245mm * 0.35 sidewall; fit 21inch rims)
        {
            tireBox.SetAsBox(0.1225f, 0.309575f); // 533.4mm rim + (0.35 * 245mm tires) == 533.4mm + 85.75mm == 619.15mm == 0.61915m
            tireFixture.shape = &tireBox;
            tireFixture.density = 1.0f; // TODO: make this more accurate???
        }
        auto pTirePos = tirePos;
        for (auto& pTireBody : tireBodies)
        {
            tireBodyDef.position = *pTirePos++;
            pTireBody = pWorldPhysics->CreateBody(&tireBodyDef);
            pTireBody->CreateFixture(&tireFixture);
            // tire mass
            {
                auto massData = pTireBody->GetMassData();
                massData.mass = clg::Car::wheelWeight / clg::formula::gravitationalAcceleration;
                pTireBody->SetMassData(&massData);
            }
        }
    }
    // front wheel pivot joints
    b2RevoluteJoint* wheelJoint[2];
    for (int i = 0; i < 2; i++)
    {
        b2RevoluteJointDef jointDef;
        jointDef.enableLimit = true;
        jointDef.lowerAngle = clg::to_radians(-40.0f);
        jointDef.upperAngle = clg::to_radians(40.0f);
        jointDef.maxMotorTorque = 500.0f;
        jointDef.Initialize(carBody, tireBodies[i], tirePos[i]);
        wheelJoint[i] = static_cast<b2RevoluteJoint*>(pWorldPhysics->CreateJoint(&jointDef));
    }

    pCarSim = new (std::nothrow) clg::Car();
    if (nullptr == pCarSim)
    {
        pd::error("ERROR: failed to allocate memory for vehicle physics simulation");
        return false;
    }

    pCarSim->Initialize(carBody, tireBodies, wheelJoint);

    // b2Log("test log from box2d %s %d %s...", "one", 2, "three");
    return true;
}

void PaintHollowRectangle(uint8_t* pCanvas, int width, int height)
{
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            pCanvas[y * width + x] = (0 == x || (width - 1) == x || 0 == y || (height - 1) == y) ? 3 : 0;
}

void PaintTriangle(uint8_t* pCanvas, int width, int height)
{
    PaintHollowRectangle(pCanvas, width, height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (x >= (y / 2) && x <= (height - y / 2))
                pCanvas[y * width + x] = 3;
        }
    }
}

template<int run_length = 4>
void PaintCheckerboard(uint8_t* pCanvas, int width, int height)
{
    int yrun = 0;
    for (int y = 0; y < height; y++)
    {
        int xrun = yrun < run_length ? 0 : run_length;
        for (int x = 0; x < width; x++)
        {
            pCanvas[y * width + x] = (xrun < run_length) ? 3 : 0;
            if (++xrun >= run_length * 2)
            {
                xrun = 0;
            }
        }

        if (++yrun >= run_length * 2)
        {
            yrun = 0;
        }
    }
}

// Creates a compressed texture with without an alpha channel for transparency
// Format: 1-bit per pixel; y starts at the bottom
uint8_t* CreateTexture(clg::memory_arena* pDstArena, clg::memory_arena* pTransientArena, int width, int height,
    const PaintTextureFunc PaintTexture, int& linePitch)
{
    auto pUncompressed = static_cast<uint8_t*>(pTransientArena->alloc(width * height));
    if (nullptr == pUncompressed)
    {
        pd::error("ERROR: failed to allocate enough memory to paint texture");
        return nullptr;
    }

    PaintTexture(pUncompressed, width, height);
    const auto uncompressedLinePitch = width;
    const auto compressedLinePitch = clg::GetCompressedTextureLinePitch<sizeof(uint16_t), false>(width);
    auto pCompressed = static_cast<uint8_t*>(pDstArena->aligned_alloc<pd::PageAlignment>(compressedLinePitch * height));
    if (nullptr == pCompressed)
    {
        pd::error("ERROR: failed to allocate enough memory to compress texture");
        return nullptr;
    }

    clg::CompressTexture<false>(width, height, pUncompressed, uncompressedLinePitch, pCompressed, compressedLinePitch);
    linePitch = compressedLinePitch;
    return pCompressed;
}

// Creates a compressed texture with an alpha channel for transparency
// Format: 2-bits per pixel, 1-alpha bit and 1-color bit; y starts at the bottom
uint8_t* CreateTextureWithTransparency(clg::memory_arena* pDstArena, clg::memory_arena* pTransientArena, int width, int height,
    const PaintTextureFunc PaintTexture, int& linePitch)
{
    auto pUncompressed = static_cast<uint8_t*>(pTransientArena->alloc(width * height));
    if (nullptr == pUncompressed)
    {
        pd::error("ERROR: failed to allocate enough memory to paint texture");
        return nullptr;
    }

    PaintTexture(pUncompressed, width, height);
    const auto uncompressedLinePitch = width;
    const auto compressedLinePitch = clg::GetCompressedTextureLinePitch<sizeof(uint16_t), true>(width);
    auto pCompressed = static_cast<uint8_t*>(pDstArena->aligned_alloc<pd::PageAlignment>(compressedLinePitch * height));
    if (nullptr == pCompressed)
    {
        pd::error("ERROR: failed to allocate enough memory to compress texture");
        return nullptr;
    }

    clg::CompressTexture<true>(width, height, pUncompressed, uncompressedLinePitch, pCompressed, compressedLinePitch);
    linePitch = compressedLinePitch;
    return pCompressed;
}

void StartUp()
{
    // Initialize Globals
    /////////////////////
    elapsedFrameTime = 0.0f;
    p = clg::point(200.0f, 120.0f);
    step = 100.0f;
    b2Scale = clg::sizev(2.0f);
    b2Angle = 0.0f;
    cycle = 0.0f;
    held = static_cast<PDButtons>(0);
    ups = 0.0f;
    fps = 0.0f;

    clg::InitializeDrawing();

    auto isPhysicsInitialized = InitializePhysics();
    if (!isPhysicsInitialized)
    {
        // TODO: some sort of error screen
        return;
    }

    // allocate memory per frame memory arena
    {
        pLevelArena = new (std::nothrow) clg::memory_arena(); // current level heap
        pFrameArena = new (std::nothrow) clg::memory_arena(); // per frame heap
        if (nullptr == pLevelArena || nullptr == pFrameArena)
        {
            pd::error("ERROR: failed to memory arena object");
            // TODO: error message splash screen
            return;
        }

        // largest size allocated was (after physics and test textures): 16,294,156 (15.53MB)
        size_t levelHeapSize = 8u * 1024u * 1024u;
        size_t frameHeapSize = 6u * 1024u * 1024u;
        levelHeapSize = pLevelArena->initialize(levelHeapSize);
        if (0 == levelHeapSize)
        {
            pd::error("ERROR: failed to allocate memory pool for level heap");
            // TODO: error message splash screen
            return;
        }

        frameHeapSize = pFrameArena->initialize(frameHeapSize);
        if (0 == frameHeapSize)
        {
            pd::error("ERROR: failed to allocate memory pool for frame heap");
            // TODO: error message splash screen
            return;
        }

        pd::logToConsole("memory allocated for level heap = %d", levelHeapSize);
        pd::logToConsole("memory allocated for frame heap = %d", frameHeapSize);
    }

    // create some test textures
    {
        pHollowRectangle = CreateTextureWithTransparency(pLevelArena, pFrameArena, 100, 100, &PaintHollowRectangle, compressedLinePitchWithTransparency);
        pTriangle = CreateTextureWithTransparency(pLevelArena, pFrameArena, 100, 100, &PaintTriangle, compressedLinePitchWithTransparency);
        pCheckerboard = CreateTextureWithTransparency(pLevelArena, pFrameArena, 100, 100, &PaintCheckerboard, compressedLinePitchWithTransparency);
    }
}

void FixedUpdate(float elapsedFixedGameTimeInSeconds, float fixedUpdateDeltaT)
{
    ups = (ups + 1.0f / fixedUpdateDeltaT) * 0.5f;
}

void ProcessInput(float elapsedSeconds)
{
    float dx = 0.0f;
    float dy = 0.0f;

    PDButtons current = static_cast<PDButtons>(0);
    PDButtons pushed = static_cast<PDButtons>(0);
    PDButtons released = static_cast<PDButtons>(0);
    pd::getButtonState(&current, &pushed, &released);
    held = static_cast<PDButtons>(held & ~released);
    held = static_cast<PDButtons>(held | pushed);

    if (held & kButtonLeft)
        dx -= elapsedSeconds * step;
    if (held & kButtonRight)
        dx += elapsedSeconds * step;
    if (held & kButtonDown)
        dy -= elapsedSeconds * step;
    if (held & kButtonUp)
        dy += elapsedSeconds * step;

    p += clg::point(dx, dy);

    if (held & kButtonB)
        b2Scale += elapsedSeconds;
    if (held & kButtonA)
        b2Scale -= elapsedSeconds;

    const auto crank = pd::getCrankChange();
    if (0 != crank)
        cycle -= clg::to_radians(crank);

    cycle = clg::clamp_radians(cycle);
    b2Angle = cycle;
}

void FrameUpdate(float interpolationRatio, float frameTime)
{
    elapsedFrameTime = frameTime;

    clg::ClearFrameBuffer();
    clg::ClearDebugDrawing();

    clg::recti src(0, 0, 100, 100);

    clg::point srcCenterOffset;
    srcCenterOffset.x = std::roundf(src.width() / 2.0f);
    srcCenterOffset.y = std::roundf(src.height() / 2.0f);

    clg::point dst;
    dst.x = p.x;
    dst.y = p.y;

    clg::sizev scale(b2Scale);

    pd::resetElapsedTime();

    clg::BlitTransformedAlphaTexturedRectangle(
        dst,
        scale,
        b2Angle,
        src,
        srcCenterOffset,
        pCheckerboard, // pCheckerboard pHollowRectangle pTriangle
        compressedLinePitchWithTransparency,
        true
        );

    auto t = pd::getElapsedTime();
    pd::logToConsole("%d", (int)(t * 1000000.0f));

//    clg::DrawAxisAlignedBitmap(
//        clg::pointi(10, 10),
//        src,
//        clg::pointi(srcCenterOffset),
//        triangle,
//        src.width(),
//        true
//        );

    /*
    clg::pointi srcCenterOffset2;
    srcCenterOffset2.x = static_cast<int_fast32_t>(std::roundf(src.width / 2.0f));
    srcCenterOffset2.y = static_cast<int_fast32_t>(std::roundf(src.height / 2.0f));

    clg::pointi dst2;
    dst2.x = 200;
    dst2.y = 120;

    DrawAxisAlignedBitmap(
        hollow_rectangle,
        src.width,
        &src,
        &dst2,
        &srcCenterOffset2,
        nullptr //&debugColor
    );
    //*/
}
} // namespace game

namespace clg
{
    const float fixedUpdateDeltaT = 0.02f;
    float currentGameTimeInSeconds = 0.0f;
    float gameTimeAccumulator = 0.0f;

    extern "C" // NOTE: <- This is sometimes unnessesary with the -g compile switch?
    int update(void *userdata)
    {
        const float MaxFrameTimeSeconds = 0.25f; // don't accumulate more than 0.25sec per frame
        bool flushDisplay = true;

        auto frameTime = pd::getElapsedTime();
        pd::resetElapsedTime();
        if (frameTime > MaxFrameTimeSeconds)
        {
            frameTime = MaxFrameTimeSeconds;
        }

        currentGameTimeInSeconds += frameTime;
        gameTimeAccumulator += frameTime;

        game::ProcessInput(frameTime);

        while (gameTimeAccumulator >= fixedUpdateDeltaT)
        {
            game::FixedUpdate(currentGameTimeInSeconds, fixedUpdateDeltaT);
            gameTimeAccumulator -= fixedUpdateDeltaT;
        }

        const auto currentSnapProgress = gameTimeAccumulator / fixedUpdateDeltaT;
        game::FrameUpdate(currentSnapProgress, frameTime);

        pd::drawFPS(pd::LcdWidth - 20, 0);
        pd::markUpdatedRows(0, pd::LcdHeight - 1);

        return flushDisplay ? 1 : 0;
    }
} // namespace clg

#ifdef _WINDLL
__declspec(dllexport)
#endif
extern "C" int eventHandler(PlaydateAPI* playdate, PDSystemEvent event, uint32_t arg)
{
	switch (event)
    {
        default: break;
        case kEventInit:
        {
            const auto pd = playdate;
            pd::InitializeGlobalVariables(pd);
            pd::InitializePlaydateAPI(pd);
            pd::logToConsole("Playdate SDK Version %d.%d.%d", SDK_VERSION_MAJOR, SDK_VERSION_MINOR, SDK_VERSION_PATCH);
            pd::logToConsole("Application Version %d.%d.%d", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);

            // runtime environment setup tasks
            pd::setRefreshRate(0); // refresh as fast as possible
            void *const userdata = nullptr;
            pd::setUpdateCallback(&clg::update, userdata);

            pd::resetElapsedTime(); // time startup
            game::StartUp();
            auto elapsed = pd::getElapsedTime();

            // log the elapsed init time
            pd::logToConsole("startup seconds: %d", (int)(elapsed * 1000.0f));
        } break;
        case kEventTerminate:
        {
            pd::FinalizeGlobalVariables();
        } break;
    }

    return 0;
}
