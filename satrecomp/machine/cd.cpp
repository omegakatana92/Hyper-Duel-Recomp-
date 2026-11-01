// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of satrecomp: https://github.com/tmcleroy/satrecomp
// The CD drive's host interface, as far as the game still uses it. Files, the opening movie's data and CD
// music are read by the port itself (games/<id>/src); what is left are the commands the game's CD library
// gives when it starts (and when the movie player does): status, table of contents, set-up of the buffer's
// filters, seeks, and a "play" that only has to be seen to be going.
//
// The buffer commands 0x50-0x53 and 0x60-0x63 are here for a game whose own file system reads the disc
// through the drive rather than through a replaced loader: the drive fills one partition (the games ported
// so far use one), and the host takes its sectors through the data port, directly or by SCU DMA. The
// command layouts and responses follow the CD block's, as Ymir's cdblock.cpp describes them; only the
// parts those commands need are modelled (no filters, no multiple partitions).
#include "machine.h"

#include "../runtime/music.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>

namespace satrecomp::machine {

namespace {
    // Interrupt request bits (HIRQ)
    enum : uint16 {
        kCMOK = 0x0001, // command accepted, response ready
        kDRDY = 0x0002, // data ready at the data port
        kCSCT = 0x0004, // a sector was read
        kBFUL = 0x0008, // the buffer is full
        kPEND = 0x0010, // the end of what was to be played
        kDCHG = 0x0020, // the disc was changed
        kESEL = 0x0040, // selector set-up done
        kEHST = 0x0080, // data transfer ended
        kEFLS = 0x0200, // file system command done
        kSCDQ = 0x0400, // subcode read
    };
    constexpr uint32 kBufferSectors = 200;
} // namespace

void Machine::CdReset() {
    satrecomp::music::Stop();
    m_cd = {};
    m_cd.periodicDue = m_now + m_clock / 75;
    static const bool log = getenv("SATRECOMP_CD_LOG") != nullptr;
    if (log) {
        for (const Disc::Track &t : m_disc.tracks) {
            fprintf(stderr, "cd track %2u %s start %u end %u\n", t.number, t.control == 0 ? "audio" : "data ", t.startFAD,
                    t.endFAD);
        }
    }
}

uint16 Machine::CdRead(uint32 address) {
    auto &cd = m_cd;
    if (address >= 0x98000 || address < 0x20000 || (address & 0xFFFF) < 0x08) { // the data port
        if (cd.dataPortLength != 0) {
            // A sector transfer (0x61/0x63): the buffer's sectors, a 16-bit word at a time
            if (cd.dataPortOffset >= cd.dataPortLength) {
                return 0xFFFF;
            }
            const uint32 word = cd.dataPortOffset++;
            cd.transferred++;
            const uint32 wordsPerSector = cd.sectorLength / 2;
            const uint32 sector = cd.dataPortSector + (wordsPerSector ? word / wordsPerSector : 0);
            const uint32 offset = wordsPerSector ? word % wordsPerSector : 0;
            if (sector < cd.sectorCount && offset * 2 + 1 < 2048) {
                const uint8 *p = cd.sector[sector] + offset * 2;
                return static_cast<uint16>((p[0] << 8) | p[1]);
            }
            return 0;
        }
        return cd.transferOffset < cd.transferSize ? cd.transfer[cd.transferOffset++] : 0xFFFF;
    }
    switch (address & 0x3C) {
    case 0x08: return cd.HIRQ;
    case 0x0C: return cd.HIRQMask;
    case 0x18: return cd.CR[0];
    case 0x1C: return cd.CR[1];
    case 0x20: return cd.CR[2];
    case 0x24:
        cd.responseTaken = true; // (from now on the drive reports its status by itself again)
        return cd.CR[3];
    default: return 0;
    }
}

void Machine::CdWrite(uint32 address, uint16 value) {
    auto &cd = m_cd;
    switch (address & 0x3C) {
    case 0x08: cd.HIRQ &= value; break; // (writing 0 to a bit clears it)
    case 0x0C: cd.HIRQMask = value; break;
    case 0x18: cd.command[0] = value; break;
    case 0x1C: cd.command[1] = value; break;
    case 0x20: cd.command[2] = value; break;
    case 0x24:
        cd.command[3] = value;
        CdCommand();
        break;
    default: break;
    }
}

// The usual response: status, what is under the pickup (track 1's control bits, track and index), where
void Machine::CdReport(uint8 status) {
    auto &cd = m_cd;
    if (cd.status == 0x02) { // (stopped: no position)
        cd.CR[0] = static_cast<uint16>((status << 8) | 0xFF);
        cd.CR[1] = cd.CR[2] = cd.CR[3] = 0xFFFF;
        return;
    }
    uint8 track = 1, control = 0x41;
    for (const Disc::Track &t : m_disc.tracks) {
        if (cd.fad >= t.startFAD) {
            track = t.number;
            control = static_cast<uint8>((t.control << 4) | 1);
        }
    }
    cd.CR[0] = static_cast<uint16>((status << 8) | 0x80);
    cd.CR[1] = static_cast<uint16>((control << 8) | track);
    cd.CR[2] = static_cast<uint16>(0x0100 | ((cd.fad >> 16) & 0xFF));
    cd.CR[3] = static_cast<uint16>(cd.fad);
}

void Machine::CdCommand() {
    auto &cd = m_cd;
    const uint8 command = cd.command[0] >> 8;
    cdCommands[command]++;
    ++g_activity;
    static const bool log = getenv("SATRECOMP_CD_LOG") != nullptr;
    if (log) {
        fprintf(stderr, "cd cmd %02X %04X %04X %04X %04X (HIRQ %04X, buffered %u)\n", command, cd.command[0], cd.command[1],
                cd.command[2], cd.command[3], cd.HIRQ, cd.buffered);
    }
    uint16 flags = kCMOK;
    switch (command) {
    case 0x00: CdReport(cd.status); break; // status
    case 0x02: { // table of contents: 102 long words at the data port
        std::fill(std::begin(cd.transfer), std::end(cd.transfer), 0xFFFF);
        cd.transferSize = 204;
        auto entry = [&](size_t index, uint32 value) {
            cd.transfer[index * 2] = static_cast<uint16>(value >> 16);
            cd.transfer[index * 2 + 1] = static_cast<uint16>(value);
        };
        uint8 first = 1, last = 1, firstControl = 0x41, lastControl = 0x41;
        for (size_t i = 0; i < m_disc.tracks.size(); i++) {
            const Disc::Track &t = m_disc.tracks[i];
            const uint8 control = static_cast<uint8>((t.control << 4) | 1);
            if (t.number >= 1 && t.number <= 99) {
                entry(t.number - 1u, (static_cast<uint32>(control) << 24) | t.startFAD);
            }
            if (i == 0) {
                first = t.number;
                firstControl = control;
            }
            last = t.number;
            lastControl = control;
        }
        entry(99, (static_cast<uint32>(firstControl) << 24) | (static_cast<uint32>(first) << 16));
        entry(100, (static_cast<uint32>(lastControl) << 24) | (static_cast<uint32>(last) << 16));
        entry(101, (static_cast<uint32>(lastControl) << 24) | m_disc.leadOutFAD);
        cd.transferOffset = 0;
        cd.CR[0] = static_cast<uint16>(cd.status << 8);
        cd.CR[1] = 204;
        cd.CR[2] = cd.CR[3] = 0;
        flags |= kDRDY;
        break;
    }
    case 0x03: // session information
        cd.CR[0] = static_cast<uint16>(cd.status << 8);
        cd.CR[1] = 0;
        cd.CR[2] = 0x0100;
        cd.CR[3] = 0;
        break;
    case 0x04: // initialise
        CdReport(cd.status);
        flags |= kESEL;
        break;
    case 0x06: // end of a data transfer: how much was taken
        if (cd.dataPortLength != 0) {
            const uint32 words = cd.transferred;
            if (cd.dataPortDelete) {
                // (0x63: the sectors are deleted once the host has taken them)
                const uint32 start = cd.dataPortSector;
                const uint32 count = cd.dataPortSectors;
                if (start < cd.sectorCount) {
                    const uint32 take = std::min(count, cd.sectorCount - start);
                    std::memmove(cd.sector[start], cd.sector[start + take],
                                 static_cast<size_t>(cd.sectorCount - start - take) * 2048);
                    cd.sectorCount -= take;
                    cd.buffered = cd.sectorCount;
                }
            }
            cd.CR[0] = static_cast<uint16>((cd.status << 8) | ((words >> 16) & 0xFF));
            cd.CR[1] = static_cast<uint16>(words);
            cd.CR[2] = cd.CR[3] = 0;
            cd.dataPortLength = 0;
            cd.dataPortOffset = 0;
            cd.dataPortSectors = 0;
            cd.dataPortDelete = false;
            cd.transferred = 0;
        } else if (cd.transferSize == 0) {
            cd.CR[0] = static_cast<uint16>((cd.status << 8) | 0xFF);
            cd.CR[1] = 0xFFFF;
            cd.CR[2] = cd.CR[3] = 0;
        } else {
            cd.CR[0] = static_cast<uint16>(cd.status << 8);
            cd.CR[1] = static_cast<uint16>(cd.transferOffset);
            cd.CR[2] = cd.CR[3] = 0;
        }
        cd.transferSize = 0;
        cd.transferOffset = 0;
        flags |= kEHST;
        break;
    case 0x10: { // play: a frame address range, a track:index range, or a resume
        const uint32 start = ((cd.command[0] & 0xFF) << 16) | cd.command[1];
        const uint32 end = ((cd.command[2] & 0xFF) << 16) | cd.command[3];
        const uint8 mode = static_cast<uint8>(cd.command[2] >> 8);
        if (satrecomp::music::SectorPlayback()) {
            cd.fad = satrecomp::music::CurrentFAD();
        }
        // All-ones parameters resume the saved range without discarding a partial sector.
        if (start == 0xFFFFFF && end == 0xFFFFFF && mode == 0xFF) {
            if (log) {
                fprintf(stderr, "cd 10 resume at %u\n", cd.fad);
            }
            satrecomp::music::Resume();
            cd.status = 0x03;
            cd.reading = true;
            CdReport(cd.status);
            break;
        }
        auto trackStart = [&](uint32 parameter) {
            if (m_disc.tracks.empty()) {
                return uint32{150};
            }
            const uint32 number =
                std::clamp<uint32>((parameter >> 8) & 0xFF, m_disc.tracks.front().number, m_disc.tracks.back().number);
            for (const auto &t : m_disc.tracks) {
                if (t.number == number) {
                    return t.startFAD;
                }
            }
            return m_disc.leadOutFAD;
        };
        auto trackEnd = [&](uint32 parameter) {
            if (m_disc.tracks.empty()) {
                return m_disc.leadOutFAD;
            }
            const uint32 number =
                std::clamp<uint32>((parameter >> 8) & 0xFF, m_disc.tracks.front().number, m_disc.tracks.back().number);
            for (const auto &t : m_disc.tracks) {
                if (t.number == number) {
                    // The next track's INDEX 01 leaves the 2-second pregap in the image; the track's own
                    // last sector is the end of its audio (the CUE puts INDEX 00 at the file start).
                    if (t.endFAD >= t.startFAD) {
                        return t.endFAD + 1;
                    }
                    for (const auto &next : m_disc.tracks) {
                        if (next.number > t.number) {
                            return next.startFAD;
                        }
                    }
                    return m_disc.leadOutFAD;
                }
            }
            return m_disc.leadOutFAD;
        };
        uint32 first = start == 0xFFFFFF ? cd.playStart
                       : (start & 0x800000) ? (start & 0x7FFFFF)
                                            : trackStart(start);
        uint32 limit = cd.playEnd;
        if (end != 0xFFFFFF) {
            limit = m_disc.leadOutFAD;
            if (end & 0x800000) {
                limit = first + (end & 0x7FFFFF);
            } else if (end != 0) {
                limit = trackEnd(end);
            }
        }
        limit = std::min(limit, m_disc.leadOutFAD);
        if (mode != 0xFF) {
            cd.playMode = mode;
        }
        const bool continuePosition = mode == 0xFF || (mode & 0x80);
        if (!continuePosition || cd.fad < first || cd.fad >= limit) {
            cd.fad = first;
        }
        cd.playStart = first;
        cd.playEnd = limit;
        satrecomp::music::Stop();
        bool audio = false;
        for (const auto &t : m_disc.tracks) {
            if (t.startFAD <= cd.fad) {
                audio = !(t.control & 4);
            }
        }
        if (log) {
            fprintf(stderr,
                    "cd 10 play start=%06X end=%06X mode=%02X -> FAD %u..%u (%s), caller PC %08X PR %08X\n", start,
                    end, mode, first, limit, audio ? "CDDA" : "data", masterSH2.PC, masterSH2.PR);
        }
        if (audio) {
            satrecomp::music::PlaySectors(cd.fad, limit, cd.playMode & 0x0F);
        }
        cd.status = 0x03;
        cd.reading = true;
        CdReport(0x04);
        break;
    }
    case 0x11: { // seek: to a frame address and pause, or stop, or pause where it is
        const uint32 target = ((cd.command[0] & 0xFF) << 16) | cd.command[1];
        if (satrecomp::music::SectorPlayback()) {
            cd.fad = satrecomp::music::CurrentFAD();
        }
        if (log) {
            fprintf(stderr, "cd 11 seek target=%06X (%s) at %u\n", target,
                    target == 0xFFFFFF ? "pause" : target == 0 ? "stop" : "seek", cd.fad);
        }
        cd.reading = false;
        if (target == 0xFFFFFF) {
            satrecomp::music::Pause();
            cd.status = 0x01;
        } else {
            satrecomp::music::Stop();
            cd.status = target == 0 ? 0x02 : 0x01;
            if (target & 0x800000) {
                cd.fad = target & 0x7FFFFF;
            } else if (target != 0) {
                for (const auto &t : m_disc.tracks) {
                    if (t.number == ((target >> 8) & 0xFF)) {
                        cd.fad = t.startFAD;
                        break;
                    }
                }
            }
        }
        CdReport(cd.status);
        break;
    }
    case 0x30: // which filter the drive's sectors go to
    case 0x42: // a filter's subheader conditions, mode and outputs
    case 0x44:
    case 0x46:
        CdReport(cd.status);
        flags |= kESEL;
        break;
    case 0x48: // reset the selector: the buffer is emptied
        cd.sectorCount = 0;
        cd.buffered = 0;
        cd.dataPortLength = 0;
        cd.dataPortOffset = 0;
        cd.dataPortSectors = 0;
        cd.dataPortDelete = false;
        cd.transferred = 0;
        cd.HIRQ &= ~kBFUL;
        CdReport(cd.status);
        flags |= kESEL;
        break;
    case 0x50: // buffer size: free sectors, filters, sectors in all
        cd.CR[0] = static_cast<uint16>(cd.status << 8);
        cd.CR[1] = static_cast<uint16>(kBufferSectors - cd.sectorCount);
        cd.CR[2] = 0x1800;
        cd.CR[3] = kBufferSectors;
        break;
    case 0x51: { // sectors in a buffer partition
        const uint8 partition = static_cast<uint8>(cd.command[2] >> 8);
        cd.CR[0] = static_cast<uint16>(cd.status << 8);
        cd.CR[1] = cd.CR[2] = 0;
        cd.CR[3] = static_cast<uint16>(partition == 0 ? cd.sectorCount : 0);
        break;
    }
    case 0x52: { // calculate the size of sectors in the buffer, in words
        uint32 offset = cd.command[1];
        const uint8 partition = static_cast<uint8>(cd.command[2] >> 8);
        uint32 count = cd.command[3];
        bool wait = false, reject = partition != 0;
        if (!reject) {
            if (offset == 0xFFFF) {
                offset = cd.sectorCount ? cd.sectorCount - 1 : 0;
            }
            if (count == 0xFFFF) {
                count = cd.sectorCount > offset ? cd.sectorCount - offset : 0;
            }
            if (count == 0 || offset >= cd.sectorCount || offset + count > cd.sectorCount) {
                wait = true;
            } else {
                cd.calculatedSize = count * (cd.sectorLength / 2);
            }
        }
        CdReport(reject ? static_cast<uint8>(0xFF) : wait ? static_cast<uint8>(cd.status | 0x80) : cd.status);
        if (!reject && !wait) {
            flags |= kESEL;
        }
        break;
    }
    case 0x53: // the size calculated by 0x52
        cd.CR[0] = static_cast<uint16>((cd.status << 8) | ((cd.calculatedSize >> 16) & 0xFF));
        cd.CR[1] = static_cast<uint16>(cd.calculatedSize);
        cd.CR[2] = cd.CR[3] = 0;
        break;
    case 0x60: { // sector length: get in CR1's low byte, put in CR2's high byte
        static const uint32 lengths[] = {2048, 2336, 2340, 2352};
        const uint8 get = static_cast<uint8>(cd.command[0] & 0xFF);
        if (get < 4) {
            cd.sectorLength = lengths[get];
        }
        CdReport(cd.status);
        flags |= kESEL;
        break;
    }
    case 0x61:   // get sector data
    case 0x63: { // get then delete sector data
        uint32 offset = cd.command[1];
        const uint8 partition = static_cast<uint8>(cd.command[2] >> 8);
        uint32 count = cd.command[3];
        bool wait = false, reject = partition != 0;
        if (!reject) {
            if (offset == 0xFFFF) {
                offset = cd.sectorCount ? cd.sectorCount - 1 : 0;
            }
            if (count == 0xFFFF) {
                count = cd.sectorCount > offset ? cd.sectorCount - offset : 0;
            }
            if (count == 0 || offset >= cd.sectorCount || offset + count > cd.sectorCount) {
                wait = true;
            }
        }
        if (reject) {
            CdReport(0xFF);
        } else if (wait) {
            CdReport(static_cast<uint8>(cd.status | 0x80));
        } else {
            cd.dataPortSector = offset;
            cd.dataPortSectors = count;
            cd.dataPortOffset = 0;
            cd.dataPortLength = count * (cd.sectorLength / 2);
            cd.transferred = 0;
            cd.dataPortDelete = command == 0x63;
            cd.CR[0] = static_cast<uint16>((cd.status << 8) | 0x40); // data is at the port
            cd.CR[1] = cd.CR[2] = cd.CR[3] = 0;
            flags |= kDRDY;
        }
        break;
    }
    case 0x62: { // delete sector data
        uint32 offset = cd.command[1];
        const uint8 partition = static_cast<uint8>(cd.command[2] >> 8);
        uint32 count = cd.command[3];
        bool wait = false, reject = partition != 0;
        if (!reject) {
            if (offset == 0xFFFF) {
                offset = cd.sectorCount ? cd.sectorCount - 1 : 0;
            }
            if (count == 0xFFFF) {
                count = cd.sectorCount > offset ? cd.sectorCount - offset : 0;
            }
            if (count == 0 || offset >= cd.sectorCount || offset + count > cd.sectorCount) {
                wait = true;
            } else {
                std::memmove(cd.sector[offset], cd.sector[offset + count],
                             static_cast<size_t>(cd.sectorCount - offset - count) * 2048);
                cd.sectorCount -= count;
                cd.buffered = cd.sectorCount;
            }
        }
        CdReport(reject ? static_cast<uint8>(0xFF) : wait ? static_cast<uint8>(cd.status | 0x80) : cd.status);
        flags |= kEHST;
        break;
    }
    case 0x75: // abort file
        CdReport(cd.status);
        flags |= kEFLS;
        break;
    case 0xE0: // authenticate the disc
        cd.CR[0] = static_cast<uint16>((cd.status << 8) | 0xFF);
        cd.CR[1] = cd.CR[2] = cd.CR[3] = 0xFFFF;
        flags |= kEFLS;
        break;
    default: {
        static bool reported[256];
        if (!reported[command]) {
            reported[command] = true;
            fprintf(stderr, "CD drive: command %02X is not one this machine knows (%04X %04X %04X %04X)\n", command, cd.command[0],
                    cd.command[1], cd.command[2], cd.command[3]);
        }
        CdReport(cd.status);
        break;
    }
    }
    if (log) {
        fprintf(stderr, "cd  -> %04X %04X %04X %04X\n", cd.CR[0], cd.CR[1], cd.CR[2], cd.CR[3]);
    }
    cd.HIRQ |= flags;
    cd.responseTaken = false;
}

// 150 times a second while the drive reads (data is read at double speed): a sector goes into the buffer,
// until that is full or the end of what was asked for is reached
void Machine::CdPeriodic() {
    auto &cd = m_cd;
    cd.periodicDue += m_clock / 150;
    if (cd.status != 0x02) {
        cd.HIRQ |= kSCDQ; // (a disc that is turning: its subcode is read all the time)
    }
    // CD-DA advances at the sound generator's 44.1 kHz sample clock, not the data drive's
    // 150-sector/second buffer clock. It never fills the data FIFO.
    if (satrecomp::music::SectorPlayback()) {
        cd.fad = satrecomp::music::CurrentFAD();
        if (cd.reading && satrecomp::music::GetState() != satrecomp::music::State::Playing) {
            cd.reading = false;
            cd.status = 0x01;
            cd.HIRQ |= kPEND;
        }
        if (cd.responseTaken) {
            CdReport(cd.status | 0x20);
        }
        return;
    }
    if (cd.responseTaken) {
        CdReport(cd.status | 0x20); // the periodic report (marked as one)
    }
    if (!cd.reading) {
        return;
    }
    if (cd.fad >= cd.playEnd) {
        cd.reading = false;
        cd.status = 0x01;
        cd.HIRQ |= kPEND;
        ++g_activity;
        return;
    }
    if (cd.sectorCount >= kBufferSectors) {
        cd.reading = false; // (it waits there, still "playing", until the buffer is emptied)
        cd.HIRQ |= kBFUL;
        ++g_activity;
        return;
    }
    uint8 data[2048] = {};
    if (readSector) {
        readSector(cd.fad, data);
    }
    std::memcpy(cd.sector[cd.sectorCount], data, sizeof data);
    cd.sectorCount++;
    cd.buffered = cd.sectorCount;
    cd.fad++;
    cd.HIRQ |= kCSCT;
    ++g_activity;
}

} // namespace satrecomp::machine
