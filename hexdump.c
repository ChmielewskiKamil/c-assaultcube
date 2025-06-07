#include "hexdump.h"
#include "entity_player.h"
#include "offsets.h"
#include "process.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

kern_return_t run_exploration_mode(AppContext *ctx) {
  // 1. Get player entity base address and point it at Player Entity struct.
  // 2. Read memory region after player's base address.

  mach_vm_address_t player_entity_address = 0;
  intptr_t offsets[] = {PLAYER_ENTITY_BASE_POINTER_OFFSET, 0};
  uint32_t num_offsets = sizeof(offsets) / sizeof(offsets[0]);
  kern_return_t kr;
  kr = resolve_pointer_path(ctx->task_port, ctx->module_base_address, offsets,
                            num_offsets, &player_entity_address);
  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "Could not resolve the provided pointer path: %s",
            mach_error_string(kr));
    return kr;
  }

  mach_vm_size_t hex_dump_size = 768;

  unsigned char hex_dump_current[hex_dump_size];
  unsigned char hex_dump_previous[hex_dump_size];

  memset(hex_dump_previous, 0, hex_dump_size);

  kr = read_target_memory(ctx->task_port, player_entity_address,
                          hex_dump_size, &hex_dump_current, NULL);
  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "Could read memory at player entity address: %s",
            mach_error_string(kr));
    return kr;
  }

    for (uint i = 0; i < hex_dump_size; i += 16) {
        printf("+0x%04x | ", i);
        for (uint j = 0; j < 16; j++) {
            printf("%02X ", hex_dump_current[i + j]);
            if (j == 7) {
                printf(" ");
            }
        }
        printf("\n");
    } 

  return KERN_SUCCESS;
}
