#ifndef PLAYER_ENTITY_H
#define PLAYER_ENTITY_H

#include "offsets.h"
#include <stdint.h>

typedef struct {
    unsigned char unknown_block0[PLAYER_ENTITY_OFFSET_HEALTH];

    int health; // at offset PLAYER_ENTITY_OFFSET_HEALTH

} PlayerEntity;

#endif // PLAYER_ENTITY_H
