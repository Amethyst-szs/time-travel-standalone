#include "timeWarp.h"
#include "al/LiveActor/LiveActor.h"
#include "al/scene/Scene.h"
#include "al/util.hpp"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "game/GameData/GameDataFunction.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "gfx/seadColor.h"
#include "math/seadMathCalcCommon.h"
#include "math/seadVector.h"
#include "prim/seadSafeString.h"
#include "game/Player/PlayerFunction.h"
#include "rs/util.hpp"
#include <cmath>

TimeContainer& getTimeContainer()
{
    static TimeContainer i; //Returns a constant TimeContainer, can be accessed from anywhere
    return i;
}

void TimeContainer::init() //Run on game bootup
{
    timeFrames.allocBuffer(maxFrames, nullptr); //Required for PtrArray for function
    sceneInvactiveTime = 60;
    return;
}

void TimeContainer::updateTimeStates(PlayerActorHakoniwa* p1)
{   
    al::LiveActor* hack = p1->mHackKeeper->currentHackActor;
    bool isCur2D = p1->mDimKeeper->is2D;
    
     //Cancel early if the invactive time is still running
    if(sceneInvactiveTime >= 0){
        sceneInvactiveTime--;
        return;
    }

    if(dotBounceIndex >= 0) dotBounceIndex--; //Update the dot bounce index

    //Clear history on a capture
    if(isCapture != (hack != nullptr)){
        isCapture = hack != nullptr;
        isCaptureInvalid = false;
        emptyFrameInfo();

        if(isRewinding) endRewind(p1);

        //Don't update anything related to the trail on invalid captures
        if(isCapture)
            if(isInvalidCapture(p1->mHackKeeper->getCurrentHackName())) isCaptureInvalid = true;
    }

    //Clear history on 2D
    if(is2D != isCur2D){
        is2D = isCur2D;
        emptyFrameInfo();

        if(isRewinding) endRewind(p1);
    }

    //Before allowing frame rewinds, check cooldown status
    if(isCooldown){
        cooldownCharge += cooldownRate;
        if(cooldownCharge >= 100.f){
            isCooldown = false;
            dotBounceIndex = timeFrames.size()+15;
            if(!p1->mDimKeeper->is2D)
                al::emitEffect(p1->mPlayerModelHolder->currentModel->mLiveActor, "DotReady", al::getTransPtr(p1));
        }
    }

    if(isCaptureInvalid) return;

    // If position has changed enough, push a new frame
    if (timeFrames.isEmpty()){
        if (!rs::isActiveDemo(p1) && !isRewinding)
            pushNewFrame();
    } else {
        if ((al::calcDistance(p1, timeFrames.at(timeFrames.size()-1)->position) > minPushDistance
        || p1->mHackCap->isFlying()) && !rs::isActiveDemo(p1) && !PlayerFunction::isPlayerDeadStatus(p1) && !isRewinding)
            pushNewFrame();
    }
    if (al::isPadHoldR(-1) && !rs::isActiveDemo(p1) && !PlayerFunction::isPlayerDeadStatus(p1) && (timeFrames.size() >= minTrailLength || isRewinding) && !isCooldown) {
        if(rewindFrameDelay >= rewindFrameDelayTarget) rewindFrame(p1);
        else rewindFrameDelay++;
    } else if (isRewinding) {
        endRewind(p1);
    }
}

void TimeContainer::pushNewFrame()
{
    colorFrame += colorFrameRate;
    dotBounceIndex--;
    TimeFrame* newFrame = nullptr; 
    sead::Heap* seqHeap = sead::HeapMgr::instance()->findHeapByName("SceneHeap",0);
    if (seqHeap) {
        newFrame = new (seqHeap) TimeFrame();
    } else {
        newFrame = new TimeFrame();
    }

    // Before doing anything, if the frame container is full push data down
    if (maxFrames <= timeFrames.size())
        delete timeFrames.popFront();

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageSceneRef);
    PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);
    al::LiveActor* hack = p1->mHackKeeper->currentHackActor;
    
    newFrame->colorFrame = colorFrame;
    if (!hack) {
        //Mario
        newFrame->position = al::getTrans(p1);
        newFrame->playerFrame.gravity = al::getGravity(p1);
        newFrame->playerFrame.velocity = al::getVelocity(p1);
        newFrame->playerFrame.rotation = p1->mPoseKeeper->getQuat();
        newFrame->playerFrame.action.append(p1->mPlayerAnimator->curAnim);
        newFrame->playerFrame.actionFrame = p1->mPlayerAnimator->getAnimFrame();

        //Cappy
        if(GameDataFunction::isEnableCap(stageSceneRef)){
            if(p1->mHackCap){
                newFrame->capFrame.isFlying = p1->mHackCap->isFlying();
                newFrame->capFrame.position = al::getTrans(p1->mHackCap);
                newFrame->capFrame.rotation = p1->mHackCap->mJointKeeper->mJointRot;
                newFrame->capFrame.action = al::getActionName(p1->mHackCap);
            }
        }
    } else {
        //Capture
        newFrame->position = al::getTrans(hack);
        newFrame->playerFrame.velocity = al::getVelocity(hack);
        newFrame->playerFrame.rotation = hack->mPoseKeeper->getQuat();
        newFrame->playerFrame.action = al::getActionName(hack);
    }
    
    timeFrames.pushBack(newFrame);
    return;
}

void TimeContainer::rewindFrame(PlayerActorHakoniwa* p1)
{
    al::LiveActor* hack = p1->mHackKeeper->currentHackActor;
    al::LiveActor* headModel = al::getSubActor(p1->mPlayerModelHolder->currentModel->mLiveActor, "頭");
    rewindFrameDelay = 0;
    colorFrame -= colorFrameRate;
    colorFrameOffset += colorFrameOffsetRate;

    if (!isRewinding) startRewind(p1);
    if(cooldownCharge > 0.f) cooldownCharge -= cooldownDischarge;

    if (!hack) {
        //Mario
        al::setTrans(p1, timeFrames.back()->position);
        al::setGravity(p1, timeFrames.back()->playerFrame.gravity);
        al::setVelocity(p1, timeFrames.back()->playerFrame.velocity);
        al::setQuat(p1, timeFrames.back()->playerFrame.rotation);
        if (!timeFrames.back()->playerFrame.action.isEqual(p1->mPlayerAnimator->curAnim))
            p1->mPlayerAnimator->startAnim(timeFrames.back()->playerFrame.action);
        p1->mPlayerAnimator->setAnimFrame(timeFrames.back()->playerFrame.actionFrame);

        //Cappy
        if(GameDataFunction::isEnableCap(stageSceneRef)){
            if(!p1->mDimKeeper->is2D && p1->mHackCap){
                updateHackCap(p1->mHackCap, headModel);
                al::setTrans(p1->mHackCap, timeFrames.back()->capFrame.position);
                p1->mHackCap->mJointKeeper->mJointRot = timeFrames.back()->capFrame.rotation;
                al::startAction(p1->mHackCap, timeFrames.back()->capFrame.action.cstr());
            }
        }
    } else {
        al::setTrans(hack, timeFrames.back()->position);
        al::setVelocity(hack, timeFrames.back()->playerFrame.velocity);
        al::setQuat(hack, timeFrames.back()->playerFrame.rotation);
        al::startAction(hack, timeFrames.back()->playerFrame.action.cstr());
    }
    
    if(!p1->mDimKeeper->is2D)
        al::emitEffect(p1->mPlayerModelHolder->currentModel->mLiveActor, "DotTravel", al::getTransPtr(p1));

    delete timeFrames.popBack();
    if (timeFrames.isEmpty())
        endRewind(p1);

    return;
}

void TimeContainer::updateHackCap(HackCap* cap, al::LiveActor* headModel)
{
    //Initalize puppet cappy's state
    if(timeFrames.back()->capFrame.isFlying != cap->isFlying()){
        if(timeFrames.back()->capFrame.isFlying){ 
            cap->setupThrowStart();
        } else {
            cap->startCatch("Default", true, al::getTrans(cap));
            cap->forcePutOn();
        } 
    }

    //Toggle the puppet cap's visiblity
    if(timeFrames.back()->capFrame.isFlying){
        cap->showPuppetCap();
        al::startVisAnimForAction(headModel, "CapOff");
    } else {
        cap->hidePuppetCap();
        al::startVisAnimForAction(headModel, "CapOn");
    }
    
    return;
}

void TimeContainer::startRewind(PlayerActorHakoniwa* p1)
{
    al::Scene* scene = stageSceneRef;
    int filterID = al::getPostProcessingFilterPresetId(scene);

    if (!p1->mHackKeeper->currentHackActor) {
        p1->startDemoPuppetable();
        p1->mPlayerAnimator->startAnim("Default");
    }

    isRewinding = true;
    cooldownCharge = 60.f;

    setPostProcessingId(4); //Sets the post processing filter to black and white

    return;
}
void TimeContainer::endRewind(PlayerActorHakoniwa* p1)
{
    al::Scene* scene = stageSceneRef;
    int filterID = al::getPostProcessingFilterPresetId(scene);

    if (!p1->mHackKeeper->currentHackActor){
        p1->endDemoPuppetable();
        
        //Cleanup cappy state
        p1->mHackCap->startCatch("Default", true, al::getTrans(p1));
        p1->mHackCap->forcePutOn();
        p1->mHackCap->hidePuppetCap();
    }

    if(GameDataFunction::isEnableCap(stageSceneRef)){
        if(p1->mHackCap){
            p1->mHackCap->mCapActionHistory->mIsCapJumpReady = false;
        }
    }
    
    isRewinding = false;
    resetCooldown();

    setPostProcessingId(0); //Disables the black and white filter

    return;
}

void TimeContainer::setPostProcessingId(int targetId)
{
    al::Scene* scene = stageSceneRef;
    int curId = al::getPostProcessingFilterPresetId(scene);

    while (targetId != curId) {
        if (curId > targetId) {
            al::decrementPostProcessingFilterPreset(scene);
            curId--;
        } else {
            curId++;
            al::incrementPostProcessingFilterPreset(scene);
        }
    }

    return;
}

void TimeContainer::emptyFrameInfo()
{
    timeFrames.clear();
    colorFrame = 0.f;
    return;
}

void TimeContainer::resetCooldown()
{
    isCooldown = true;
    colorFrameOffset = 0.f;
    return;
}

TimeFrame* TimeContainer::getTimeFrame(uint32_t index)
{
    if(timeFrames.isEmpty()) return nullptr;

    //Return the index requested, or the highest frame possible if too large
    if(index > timeFrames.size()-1) return timeFrames.at(timeFrames.size()-1);
    return timeFrames.at(index);
}

int TimeContainer::getTimeArraySize()
{
    return timeFrames.size();
}

float TimeContainer::getColorFrame()
{
    return colorFrame;
}

float TimeContainer::getCooldownTimer()
{
    return cooldownCharge;
}

int TimeContainer::getRewindDelay()
{
    return rewindFrameDelayTarget;
}

bool TimeContainer::isSceneActive()
{
    return sceneInvactiveTime == -1;
}

bool TimeContainer::isRewind()
{
    return isRewinding;
}

bool TimeContainer::isOnCooldown()
{
    return isCooldown;
}

bool TimeContainer::isRewindCappyThrow()
{
    if(isRewinding){
        return timeFrames.back()->capFrame.isFlying;
    }
    return false;
}

bool TimeContainer::isInvalidCapture(const char* curName)
{
    constexpr static const char* hackList[] = {
        "ElectricWire", "TRex", "Fukankun", //Binoculars
        "Cactus", "BazookaElectric", //Sub-area rocket
        "JugemFishing", //Lakitu
        "Fastener", //Zipper
        "GotogotonLake", "GotogotonCity", //Puzzle pieces
        "Senobi", //Uproot
        "Tree", "RockForest", "FukuwaraiFacePartsKuribo", "Imomu", //Tropical wiggler
        "AnagramAlphabetCharacter", "Car", "Manhole", "Tsukkun", //Pokio
        "Statue", "StatueKoopa", "KaronWing", "Bull", //Chargin' chuck
        "Koopa", "Yoshi"
    };
    int hackListSize = *(&hackList + 1) - hackList;

    for (int i = 0; i < hackListSize; i++) {
        if (al::isEqualString(curName, hackList[i])) {
            return true;
        }
    }

    return false;
}

void TimeContainer::setRewindDelay(int index)
{
    rewindFrameDelayTarget += index;

    if(rewindFrameDelayTarget < 0) rewindFrameDelayTarget = 0;
    return;
}

void TimeContainer::setInactiveTimer(int time)
{
    sceneInvactiveTime = time;
    return;
}

void TimeContainer::setTimeFramesEmpty()
{
    emptyFrameInfo();
    return;
}

sead::Color4f TimeContainer::calcColorFrame(float frame, int dotIndex)
{
    sead::Color4f returnColor = { 0.f, 0.f, 0.f, 0.7f };
    
    if(isCooldown){
        returnColor.r = ((100+cooldownCharge)/100)-1;
        returnColor.g = ((100+cooldownCharge)/100)-1;
        returnColor.b = ((100+cooldownCharge)/100)-1;
    } else {
        if(dotIndex < dotBounceIndex-8){
            returnColor.r = 1.f;
            returnColor.g = 1.f;
            returnColor.b = 1.f;
        } else {
            returnColor.r = sin((frame+colorFrameOffset));
            returnColor.g = sin((frame+colorFrameOffset) - 2.0942f);
            returnColor.b = sin((frame+colorFrameOffset) - 4.1884f);
        }
    }
    return returnColor;
}

sead::Vector3f TimeContainer::calcDotTrans(sead::Vector3f position, int dotIndex)
{
    int relative = dotBounceIndex-dotIndex;
    if(relative <= 15 && relative > 0) position.y += sin(0.21*relative)*80;

    return position;
}

float TimeContainer::calcCooldownPercent()
{
    return sead::MathCalcCommon<float>::max(sead::MathCalcCommon<float>::min(cooldownCharge/100.f, 1.f), 0.05f);
}