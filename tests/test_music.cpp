#include "music.h"
#include "state.h"
#include <cassert>
#include <cstring>
#include <vector>
using namespace satrecomp;
int main() {
    int reads = 0;
    music::SetSectorReader([&](uint32_t fad, uint8_t *out) {
        ++reads;
        if (fad < 150 || fad >= 154) return false;
        for (int i=0; i<588; ++i) {
            out[i*4]=fad; out[i*4+1]=0;
            out[i*4+2]=0xFE; out[i*4+3]=0xFF;
        }
        return true;
    });
    int16_t l,r;
    music::PlaySectors(151,153,0);
    for (int i=0; i<588; ++i) { assert(music::Next(l,r)); assert(l==151 && r==-2); }
    assert(music::CurrentFAD()==152);
    music::Pause(); assert(!music::Next(l,r));
    music::Resume(); assert(music::Next(l,r) && l==152);
    std::vector<uint8_t> saved;
    Archive save(saved,true); music::Serialize(save);
    for (int i=1;i<588;++i) assert(music::Next(l,r) && l==152);
    assert(!music::Next(l,r)); assert(music::GetState()==music::State::Paused);
    Archive restore(saved,false); music::Serialize(restore); assert(restore.Ok());
    assert(music::Next(l,r) && l==152);
    music::PlaySectors(150,151,2);
    for(int i=0;i<588*3;++i) assert(music::Next(l,r) && l==150);
    assert(!music::Next(l,r));
    music::PlaySectors(150,151,15);
    for(int i=0;i<588*20;++i) assert(music::Next(l,r));
    music::Stop(); assert(!music::Next(l,r));
    music::PlaySectors(151,151,0); assert(!music::Next(l,r));
    // Track-based callers retain their original path.
    music::SetDisc({[](int t){return t==2?1u:0u;}, [](int t,uint32_t,uint8_t *out){std::memset(out,0,2352);out[0]=42;return t==2;}});
    music::Play(2,2,0); assert(!music::SectorPlayback()); assert(music::Next(l,r) && l==42);
    assert(reads>=25);
}
