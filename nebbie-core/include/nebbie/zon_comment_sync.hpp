#pragma once

#include "types.hpp"
#include "world.hpp"

namespace nebbie {

/**
 * Strip legacy name-based * comments and regenerate Castelli-style inline *!
 * notes on reset commands from current mob/obj/room short names.
 * For M/O also append MW/MR from the command's arg2/arg4 (max world / max room
 * as written on the .zon line); for G/E/P append MW from arg2.
 * Returns true if the zone commands were modified.
 */
bool sync_zone_reset_comments(Zone& zone, const World& world);

/** Sync every zone in world. Returns true if any zone changed. */
bool sync_all_zone_reset_comments(World& world);

/** True if sync_zone_reset_comments would modify zone (zone itself unchanged). */
bool zone_reset_comments_need_sync(const Zone& zone, const World& world);

} // namespace nebbie
