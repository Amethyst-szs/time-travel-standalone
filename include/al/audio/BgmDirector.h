#pragma once

namespace al
{
    class BgmDirector{
    public:
        void stopAllBgm(int unk, bool isClearSituation);
        void pauseBgm(char const* trackName, int unk);
        void pauseBgmById(unsigned int, int, bool);
        void resumeBgm(char const* trackName, int unk);
        void resumeBgmById(unsigned int, int, bool);
    };

};