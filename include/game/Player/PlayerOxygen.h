#pragma once

struct PlayerOxygen {
    void recovery();
    void reduce();
    void reset();
    void setup();

    float mPercentage;
    int mOxygenFrames;
    int mDamageFrames;
    int mPercentageDelay;
    int mOxygenTarget;
    int mRecoveryFrames;
    int mDamageTarget;
};