#ifndef PLAYER_ENTITY_H
#define PLAYER_ENTITY_H

#include "offsets.h"
#include <stdint.h>

typedef struct {
  unsigned char unknown_block0[PE_OFFSET_IS_CROUCHING];
  char is_crouching;
  unsigned char unknown_block1[PE_OFFSET_HEALTH -
                               (PE_OFFSET_IS_CROUCHING + sizeof(char))];
  int32_t health;
  int32_t armor;
  unsigned char unknown_block2[PE_OFFSET_SECONDARY_WEAPON_AMMO_LOADED -
                               (PE_OFFSET_ARMOR + sizeof(int32_t))];
  int32_t secondary_weapon_ammo_loaded;
} PlayerEntity;

#endif // PLAYER_ENTITY_H
