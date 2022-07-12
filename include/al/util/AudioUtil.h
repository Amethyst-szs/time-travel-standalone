#pragma once

#include "al/audio/AudioKeeper.h"
#include "al/audio/BgmDirector.h"
#include "sead/prim/seadSafeString.h"

namespace al {

al::BgmDirector* getBgmDirector(al::IUseAudioKeeper const*);
bool checkIsPlayingSe(al::IUseAudioKeeper const*, const sead::SafeString&, const char*);
bool isPlayingBgm(al::IUseAudioKeeper const*);
bool isPlayingBgm(al::IUseAudioKeeper const*,char const*);
void stopAllBgm(al::IUseAudioKeeper const*, int);
bool tryStopAllBgm(al::IUseAudioKeeper const *, int);
const char* getBgmResourceNameInCurPosition(al::IUseAudioKeeper const*, bool);

}