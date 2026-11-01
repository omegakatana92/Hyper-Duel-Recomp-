# SPDX-License-Identifier: GPL-3.0-only
# Hyper Duel (Japan) recompilation project.
#
# The toolkit is the copy in satrecomp/ (made by satrecomp/tools/sync-toolkit.sh).
# local.mk is the developer's own file (git-ignored) and may set DISC to an
# absolute path containing spaces.
GAME ?= hyperduel
SR ?= satrecomp

-include local.mk

include $(SR)/build.mk
