#include "hexdump.h"
#include "entity_player.h"
#include "offsets.h"
#include "process.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef enum {
  TYPE_UNKNOWN, // default
  TYPE_BOOL,    // 1-byte char
  TYPE_INT16,   // 2-byte short
  TYPE_INT32,   // 4-byte integer
  TYPE_FLOAT,   // 4-byte float
  TYPE_DOUBLE,  // 8-byte double
  TYPE_POINTER, // 8-byte pointer/address
} MemberType;

typedef struct {
  mach_vm_size_t offset;
  MemberType type;
  const char *name;
} MemberInfoStruct;

MemberInfoStruct player_entity_map[] = {
    {offsetof(PlayerEntity, health), TYPE_INT32, "health"},
};

uint player_entity_map_count =
    sizeof(player_entity_map) / sizeof(MemberInfoStruct);

kern_return_t run_exploration_mode(AppContext *ctx) {
  assert(ctx->task_port > 0);
  assert(ctx->module_base_address != 0);

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

  mach_vm_size_t hex_dump_size =
      640; // 40 lines of 16 bytes each fit nicely in the terminal window

  unsigned char hex_dump_current[hex_dump_size];
  unsigned char hex_dump_previous[hex_dump_size];

  memset(hex_dump_previous, 0, hex_dump_size);

  while (true) {
    kr = read_target_memory(ctx->task_port, player_entity_address,
                            hex_dump_size, &hex_dump_current, NULL);
    if (kr != KERN_SUCCESS) {
      fprintf(stderr, "Could read memory at player entity address: %s",
              mach_error_string(kr));
      return kr;
    }

    printf("\033[H\033[2J"); // move cursor to [0:0] and clear entire screen

    for (uint i = 0; i < hex_dump_size; i += 16) {
      // --- Offset from base address ---

      printf("+0x%04x | ", i);

      // --- Hex Dump ---

      for (uint j = 0; j < 16; j++) {
        if (hex_dump_current[i + j] != hex_dump_previous[i + j]) {
          printf("\033[31m%02X \033[0m", hex_dump_current[i + j]);
        } else {
          printf("%02X ", hex_dump_current[i + j]);
        }
        if (j == 7) {
          printf(" ");
        }
      }

      // --- Annotations ---

      for (uint j = 0; j < player_entity_map_count; j++) {
        // Check if the known member starts at the current line of hex dump.
        if (player_entity_map[j].offset >= i &&
            player_entity_map[j].offset < (i + 16)) {
          printf(" | %s: ", player_entity_map[j].name);
          switch (player_entity_map[j].type) {
          case TYPE_INT16: {
            uint16_t val;
            memcpy(&val, hex_dump_current + player_entity_map[j].offset,
                   sizeof(val));
            printf("%d", val);
            break;
          }
          case TYPE_INT32: {
            uint32_t val;
            memcpy(&val, hex_dump_current + player_entity_map[j].offset,
                   sizeof(val));
            printf("%d", val);
            break;
          }
          default:
            printf("N/A");
          }
        }
      }

      printf("\n");
    }

    memcpy(hex_dump_previous, hex_dump_current, hex_dump_size);

    usleep(300000); // 300k microseconds = 0.3 seconds
  }
  return KERN_SUCCESS;
}
