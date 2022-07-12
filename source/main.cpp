#include "main.hpp"
#include "al/LiveActor/LiveActor.h"
#include "al/PlayerHolder/PlayerHolder.h"
#include "al/area/ChangeStageInfo.h"
#include "al/camera/CameraDirector.h"
#include "al/camera/CameraPoser.h"
#include "al/camera/Projection.h"
#include "al/camera/alCameraPoserFunction.h"
#include "al/scene/Scene.h"
#include "al/scene/SceneObjHolder.h"
#include "al/util.hpp"
#include "debugMenu.hpp"
#include "game/GameData/GameDataFunction.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/GameData/GameDataHolderBase.h"
#include "game/GameData/GameDataHolderWriter.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerInput.h"
#include "game/Player/PlayerOxygen.h"
#include "gfx/seadCamera.h"
#include "gfx/seadColor.h"
#include "rs/util.hpp"
#include "sead/math/seadVector.h"
#include "sead/prim/seadSafeString.h"
#include "timeWarp.h"
#include "types.h"
#include <stdint.h>
#include <string>

static bool prevFrameInvalidScene = true;
static HakoniwaSequence* curGlobalSequence;

// DebugWarpPoint warpPoints[40];

int listCount = 0;
int curWarpPoint = 0;

static int debugCheckFrame = 0;

void drawMainHook(HakoniwaSequence* curSequence, sead::Viewport* viewport, sead::DrawContext* drawContext)
{
    TimeContainer& container = getTimeContainer();

    al::Scene* curScene = curSequence->curScene;
    sead::PrimitiveRenderer* renderer = sead::PrimitiveRenderer::instance();

    if (curScene && isInGame && container.isSceneActive() && container.getTimeArraySize() > 1) {
        sead::LookAtCamera* cam = al::getLookAtCamera(curScene, 0);
        sead::Projection* projection = al::getProjectionSead(curScene, 0);
        renderer->setDrawContext(drawContext);
        renderer->setCamera(*cam);
        renderer->setProjection(*projection);
        renderer->begin();
        renderer->setModelMatrix(sead::Matrix34f::ident);
        for (int i = 0; i < container.getTimeArraySize(); i++) {
            TimeFrame* frame = container.getTimeFrame(i);
            renderer->drawSphere4x8(container.calcDotTrans(frame->position, i),
                sead::MathCalcCommon<int>::min((container.getTimeArraySize()-i)/2, 9),
                container.calcColorFrame(frame->colorFrame, i));
        }
        renderer->end();
    }

    if (!debugMode) {
        al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
        return;
    }

    int dispWidth = al::getLayoutDisplayWidth();
    int dispHeight = al::getLayoutDisplayHeight();

    gTextWriter->mViewport = viewport;

    gTextWriter->mColor = sead::Color4f(1.f, 1.f, 1.f, 0.8f);

    drawBackground((agl::DrawContext*)drawContext);

    gTextWriter->beginDraw();
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, 10.f));

    if (curScene && isInGame) {
        sead::LookAtCamera* cam = al::getLookAtCamera(curScene, 0);
        sead::Projection* projection = al::getProjectionSead(curScene, 0);

        renderer->setDrawContext(drawContext);
        renderer->setCamera(*cam);
        renderer->setProjection(*projection);

        PlayerActorHakoniwa* p1 = rs::getPlayerActor(curScene);

        float curColorFrame = container.getColorFrame();
        al::LiveActor* hack = p1->mHackKeeper->currentHackActor;

        const char* captureName;
        if(hack) captureName = p1->mHackKeeper->getCurrentHackName();

        sead::Color4f curColor = container.calcColorFrame(curColorFrame, -1);
        TimeFrame* curFrame = container.getTimeFrame(debugCheckFrame);

        gTextWriter->setCursorFromTopLeft(sead::Vector2f((dispWidth / 5.f) * 3.f + 15.f, (dispHeight / 3.f) + 30.f));
        gTextWriter->setScaleFromFontHeight(20.f);
        if(al::isPadHoldL(-1) && container.oxygen){
            gTextWriter->printf("Oxygen Header Data:\n");
            gTextWriter->printf("Visual Percentage: %f\n", container.oxygen->mPercentage);
            gTextWriter->printf("Oxygen Frames: %i\n", container.oxygen->mOxygenFrames);
            gTextWriter->printf("Damage Frames: %i\n", container.oxygen->mDamageFrames);
            gTextWriter->printf("Percentage Delay: %i\n", container.oxygen->mPercentageDelay);
            gTextWriter->printf("Oxygen Target: %i\n", container.oxygen->mOxygenTarget);
            gTextWriter->printf("Recovery Frames: %i\n", container.oxygen->mRecoveryFrames);
            gTextWriter->printf("Damage Target: %i\n\n", container.oxygen->mDamageTarget);
        } else {
            gTextWriter->printf("Is Rewind: %s\n", container.isRewind() ? "True" : "False");
            gTextWriter->printf("Is Cooldown: %s\n", container.isOnCooldown() ? "True" : "False");
            gTextWriter->printf("Cooldown Charge: %f\n", container.getCooldownTimer());
            gTextWriter->printf("Rewind Delay: %i\n", container.getRewindDelay());
            gTextWriter->printf("Filter ID: %i\n", al::getPostProcessingFilterPresetId(curScene));
            if(hack) gTextWriter->printf("Current Capture Name: %s\n", captureName);
            gTextWriter->printf("Color Frame: %f\n", curColorFrame);
            gTextWriter->printf("Color R: %f\n", curColor.r);
            gTextWriter->printf("Color G: %f\n", curColor.g);
            gTextWriter->printf("Color B: %f\n", curColor.b);
            gTextWriter->printf("Array Size: %i\n", container.getTimeArraySize());
            gTextWriter->printf("\nFrame Data #%i:\n-----------------\n", debugCheckFrame);
            if (curFrame){
                gTextWriter->printf("Animation: %s\n", curFrame->playerFrame.action.cstr());
                gTextWriter->printf("Current Animation Already This: %s\n", curFrame->playerFrame.action.isEqual(p1->mPlayerAnimator->curAnim) ? "True" : "False");
                gTextWriter->printf("Animation Frame: %f\n", curFrame->playerFrame.actionFrame);
                gTextWriter->printf("Is Cap Flying: %s\n", curFrame->capFrame.isFlying ? "True" : "False");
                gTextWriter->printf("Position X: %f\n", curFrame->capFrame.position.x);
                gTextWriter->printf("Position Y: %f\n", curFrame->capFrame.position.y);
                gTextWriter->printf("Position Z: %f\n", curFrame->capFrame.position.z);
            } else {
                gTextWriter->printf("Array is empty\n");
            }
        }

        // Frame scrolling
        if (al::isPadHoldL(-1) && al::isPadTriggerRight(-1) && debugCheckFrame < container.getTimeArraySize())
            debugCheckFrame++;
        if (al::isPadHoldL(-1) && al::isPadTriggerLeft(-1) && debugCheckFrame > 0)
            debugCheckFrame--;

        // Frame fast jumping
        if (al::isPadHoldL(-1) && al::isPadTriggerUp(-1) && debugCheckFrame < container.getTimeArraySize() - 10)
            debugCheckFrame += 10;
        if (al::isPadHoldL(-1) && al::isPadTriggerUp(-1) && debugCheckFrame > container.getTimeArraySize())
            debugCheckFrame = 0;
        
        //Rewind speed edit
        if (al::isPadHoldZR(-1) && al::isPadTriggerRight(-1))
            container.setRewindDelay(1);
        if (al::isPadHoldZR(-1) && al::isPadTriggerLeft(-1))
            container.setRewindDelay(-1);

        renderer->begin();

        // sead::Matrix34f mat = sead::Matrix34f::ident;
        // mat.setBase(3, sead::Vector3f::zero); // Sets the position of the matrix.
        //  For cubes, you need to put this at the location.
        //  For spheres, you can leave this at 0 0 0 since you set it in its draw function.
        renderer->setModelMatrix(sead::Matrix34f::ident);
        renderer->end();
        isInGame = false;
    }

    gTextWriter->endDraw();

    al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
}

HOOK_ATTR
void stageInitHook(StageScene* initStageScene, al::SceneInitInfo* sceneInitInfo)
{
    __asm("MOV X19, X0");
    __asm("LDR X24, [X1, #0x18]");

    getTimeContainer().setTimeFramesEmpty();

    __asm("MOV X1, X24");
}

HOOK_ATTR
bool sceneKillHook(GameDataHolderAccessor value)
{
    getTimeContainer().setInactiveTimer(25);
    return GameDataFunction::isMissEndPrevStageForSceneDead(value);
}

ulong threadInit()
{ // hook for initializing any threads we need
    __asm("STR X21, [X19,#0x208]");

    return 0x20;
}

HOOK_ATTR
bool triggerR(int port) { return false; }

HOOK_ATTR
bool reduceOxygenForce()
{
    TimeContainer& container = getTimeContainer();
    StageScene* stageScene = container.stageSceneRef;
    if(!stageScene) return false;

    container.isPInWater = rs::isPlayerInWater(rs::getPlayerActor(stageScene));

    //Perform usual check for if player is in water, but if not, force it on if cooldown is active
    if(container.isPInWater) return true;
    else return getTimeContainer().isOnCooldown();
}

HOOK_ATTR
void oxygenReduce(PlayerOxygen* thisPtr)
{
    TimeContainer& container = getTimeContainer();
    container.oxygen = thisPtr;
    float oxygenRingCalc;
    
    //If the player is in water, perform usual calculation
    if(container.isPInWater){
        thisPtr->mOxygenFrames++;
        if (thisPtr->mOxygenFrames >= thisPtr->mOxygenTarget) { thisPtr->mDamageFrames++; }

        oxygenRingCalc = 1.f - (static_cast<float>(thisPtr->mOxygenFrames) / static_cast<float>(thisPtr->mOxygenTarget));
        if (oxygenRingCalc <= 0.f) oxygenRingCalc = 0.f;
    }

    //If the cooldown is running, replace the value in the ring
    if(container.isOnCooldown()){
        oxygenRingCalc = container.calcCooldownPercent();
        if(thisPtr->mOxygenFrames == 0) thisPtr->mOxygenFrames = thisPtr->mPercentageDelay;
        if(thisPtr->mOxygenFrames >= thisPtr->mOxygenTarget) thisPtr->mOxygenFrames = thisPtr->mOxygenTarget-1;
    }

    thisPtr->mPercentage = oxygenRingCalc;
    return;
}

HOOK_ATTR
bool hakoniwaSequenceHook(HakoniwaSequence* sequence) {
    StageScene* stageScene = (StageScene*)sequence->curScene;
    getTimeContainer().stageSceneRef = stageScene;

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorHakoniwa* p1 = al::tryGetPlayerActor(pHolder, 0);
    // al::LiveActor* curHack = player->getPlayerHackKeeper()->currentHackActor;
    
    bool isFirstStep = al::isFirstStep(sequence);
    if(isFirstStep) al::validatePostProcessingFilter(stageScene);
    
    isInGame = !stageScene->isPause();
    if (al::isPadHoldZR(-1) && al::isPadTriggerUp(-1)) debugMode = !debugMode;

    // Time warp code added here
    if(!stageScene->isPause()) getTimeContainer().updateTimeStates(p1);

    return isFirstStep;
}

void seadPrintHook(const char* fmt, ...) // hook for replacing sead::system::print with our custom logger
{
    va_list args;
    va_start(args, fmt);

    va_end(args);
}

// HOOK_ATTR
// uint8_t returnZero()
// {
//     return 0;
// }

// HOOK_ATTR
// uint8_t newSaveGrant(char const* val1, char const* val2)
// {
//     bool isTrue = al::isEqualString(val1, val2);

//     if (isTrue) {
//         StageScene* stageScene = amy::getGlobalStageScene();
//         stageScene->mHolder->mGameDataFile->addCoin(120);
//     }

//     return isTrue;
// }

// HOOK_ATTR
// uint8_t changeValue()
// {
//     return true;
// }