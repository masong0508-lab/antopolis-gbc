/* GBC nest cursor targeting: the live team and modifier are both 0 or 1.
 * A macro avoids an out-of-line function call in the almost-full fixed
 * ROM bank. There are no save format, RAM or simulation changes. */
#ifndef ANTOPOLIS_COLONY_CURSOR_H
#define ANTOPOLIS_COLONY_CURSOR_H
#include <stdint.h>
#define colony_cursor_target(team, rival) ((uint8_t)((team) ^ (rival)))
#endif
