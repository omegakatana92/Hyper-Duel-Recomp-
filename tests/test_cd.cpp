#include <bits/stdc++.h>
#define private public
#include "machine/machine.h"
#undef private
#include "runtime/music.h"
// Isolate the real CD command implementation; CPUs, renderer, and buses do not run.
namespace satrecomp::machine {
uint64 g_activity=0;
SH2::SH2(Machine &m,bool master):m_machine(m),m_master(master){}
Machine::Machine():masterSH2(*this,true),slaveSH2(*this,false){}
Machine::~Machine()=default;
uint8 Machine::Read8(uint32){return 0;}
uint16 Machine::Read16(uint32){return 0;}
void Machine::Write8(uint32,uint8){}
void Machine::Write16(uint32,uint16){}
}
using namespace satrecomp;
using namespace satrecomp::machine;
int main(){
 auto m=std::make_unique<Machine>();
 m->SetDisc({{{1,4,150},{2,0,1000},{3,0,1002}},1004});
 music::SetSectorReader([](uint32_t f,uint8_t *out){std::memset(out,1,2352);return f>=1000 && f<1004;});
 auto cmd=[&](uint16_t a,uint16_t b,uint16_t c,uint16_t d){m->m_cd.command[0]=a;m->m_cd.command[1]=b;m->m_cd.command[2]=c;m->m_cd.command[3]=d;m->CdCommand();};
 // Track 2, index 1: two sectors, not an incorrectly masked FAD 0x201.
 cmd(0x1000,0x0201,0,0x0201);
 assert(m->m_cd.fad==1000 && m->m_cd.playEnd==1002 && music::SectorPlayback());
 int16_t l,r;
 for(int i=0;i<588;++i) assert(music::Next(l,r));
 m->CdPeriodic(); assert(m->m_cd.fad==1001 && m->m_cd.sectorCount==0);
 cmd(0x11FF,0xFFFF,0,0); assert(!music::Next(l,r));
 cmd(0x10FF,0xFFFF,0xFFFF,0xFFFF); assert(music::Next(l,r));
 for(int i=1;i<588;++i) assert(music::Next(l,r));
 assert(!music::Next(l,r)); m->CdPeriodic();
 assert(!m->m_cd.reading && (m->m_cd.HIRQ&0x10));
 // FAD addressing plus length, with infinite repeat.
 cmd(0x1080,1001,0x0F80,1);
 for(int i=0;i<588*3;++i) assert(music::Next(l,r));
 // New data playback cancels audio and continues through the original data FIFO.
 int reads=0; m->readSector=[&](uint32_t f,uint8_t *out){++reads;std::memset(out,0x42,2048);return f==150;};
 cmd(0x1080,150,0x0080,1); assert(!music::SectorPlayback());
 m->CdPeriodic(); assert(reads==1 && m->m_cd.sectorCount==1);
 cmd(0x1100,0,0,0); assert(music::GetState()==music::State::Stopped);
}
