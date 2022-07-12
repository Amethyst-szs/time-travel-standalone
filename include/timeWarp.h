#include <stdint.h>
#include "container/seadSafeArray.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerModelHolder.h"
#include "game/StageScene/StageScene.h"
#include "gfx/seadColor.h"
#include "prim/seadSafeString.h"
#include "sead/math/seadVector.h"

class TimeFrameCap { //All data stored in a frame related to cappy
public:
    bool isFlying = false; //Is cappy on Mario's head
    sead::Vector3f position = sead::Vector3f::zero; //Cappy position
    sead::Vector3f rotation = sead::Vector3f::zero; //Cappy rotation
    sead::FixedSafeString<0x20> action; //Cappy animation
};

class TimeFramePlayer { //All data stored in a frame related to the player or hack
public:
    sead::Vector3f velocity = sead::Vector3f::zero; //Player velocity
    sead::Vector3f gravity = -1*sead::Vector3f::ey; //Player gravity
    sead::Quatf rotation; //Player quaternion rotation
    sead::FixedSafeString<0x20> action; //Current player animation
    float actionFrame; //Frame of the current animation
};

class TimeFrame { //Data stored in a frame dot
public:
    float colorFrame = 0; //Color of dot (passed into calcColorFrame to get a sead::Color4f)
    sead::Vector3f position = sead::Vector3f::zero; //Vector position of player and dot

    TimeFramePlayer playerFrame; //Player related frame values
    TimeFrameCap capFrame; //Cappy related frame values
};

class TimeContainer {
private:
    //Current State
    bool isRewinding = false; //Is the player actively rewinding
    bool isCapture = false;
    bool isCaptureInvalid = false; //Is the current hack on the invalid list
    bool is2D = false; //Is the player in 2D
    int sceneInvactiveTime = -1; //Disable timewarp trails during scene transition while this frame counter is higher than -1

    //Timewarp Trail Array
    static const int maxFrames = 400; //Maximum number of dots
    sead::PtrArray<TimeFrame> timeFrames; //Full array of dot frames

    //Timewarp stats/settings
    int rewindFrameDelay = 0; //Counter for the delay frames
    int rewindFrameDelayTarget = 0; //How many extra frames to stall before rewinding a frame, used for debugging
    int minTrailLength = 40; //Minimum number of dots to rewind
    float minPushDistance = 20.f; //Minimum distance to move before rewinding

    //Trail color
    float colorFrame = 0.f; //Current color state, passed into calcColorFrame
    float colorFrameRate = 0.05f; //Rate that the color value increases/decreases when drawing/rewinding
    float colorFrameOffset = 0.f; //Scrolls the color on the dots while actively rewinding
    float colorFrameOffsetRate = 0.07f; //Rate that the rewind color scoll moves
    int dotBounceIndex = -1; //Index of the center of the dot bounce effect when recharged

    //Cooldown information
    bool isCooldown = false; //Is currently recharging timewarp
    float cooldownCharge = 0.f; //Float 0-100 where 0 is empty and 100 is fully recharged
    float cooldownRate = 0.15f; //Rate the power is recharged
    float cooldownDischarge = 0.3f; //The charge is set to 60 at rewind start, slowly lower the charge by this rate

    //Private functions
    void pushNewFrame(); //Adds new value to array
    void rewindFrame(PlayerActorHakoniwa* p1); //Pops newest array entry off and places the player there
    void updateHackCap(HackCap* cap, al::LiveActor* headModel); //Updates cappy to the current frame information
    void startRewind(PlayerActorHakoniwa* p1); //Inits a rewind
    void endRewind(PlayerActorHakoniwa* p1); //Ends a rewind
    void setPostProcessingId(int id); //Sets the post processing filter ID
    void emptyFrameInfo(); //Wipes the whole frame array
    void resetCooldown(); //Restarts the cooldown

public:
    //References
    StageScene* stageSceneRef; //Current stage scene
    PlayerOxygen* oxygen; //Current oxygen meter and timewarp cooldown meter
    bool isPInWater = false; //Bool holding if the player is in water

    //Enter points
    void init(); //Prepares the PtrArray of dots
    void updateTimeStates(PlayerActorHakoniwa* p1); //Main loop run every frame

    //Getters
    TimeFrame* getTimeFrame(uint32_t index); //Returns a single time frame at an index
    int getTimeArraySize(); //Gets total size of the PtrArray
    float getColorFrame(); //Gets the float that is used to calculate color
    float getCooldownTimer(); //Gets current 0-100 cooldown charge
    int getRewindDelay(); //Returns the target for the rewind delay, 0 = none

    //Is
    bool isSceneActive(); //Checks if the scene inactive time is -1 and draws can happen
    bool isRewind();
    bool isOnCooldown();
    bool isRewindCappyThrow(); //Checks if the most recent TimeFrame has cappy flying
    bool isInvalidCapture(const char* curName); //Checks if the player's current capture is on the invalid list

    //Setters
    void setRewindDelay(int index); //Modifies the rewind delay based on the amount of the index
    void setInactiveTimer(int time); //Sets the scene invctivity timer
    void setTimeFramesEmpty(); //Deletes all time frames

    //Calcs
    sead::Color4f calcColorFrame(float colorFrame, int dotIndex); //Calculates the sead::Color4f of the colorFrame
    sead::Vector3f calcDotTrans(sead::Vector3f position, int dotIndex); //Calculates the vector position of a dot
    float calcCooldownPercent(); //Calculates a number 0-1 from the cooldownCharge 0-100 value
};

TimeContainer& getTimeContainer();