// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Hyper Duel recompilation project contributors.
//
// Hyper Duel's own sources. RegisterGame() is called by the port before the
// machine starts; this is where the game's lifted/replaced routines, features
// and widescreen layout are registered (runtime/lift.h, runtime/game.h).
//
// Nothing is replaced yet: the first boot runs entirely on the interpreter.

#include "game.h"
#include "lift.h"

namespace satrecomp {

void RegisterGame() {
    // RegisterLifts(...) and TheGame().features/wideSplits go here once the
    // game's file loader, CD library and sound calls have been identified.
}

} // namespace satrecomp
