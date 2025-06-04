#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslimits.h>

#include "app_context.h"
#include "arena.h"
#include "process.h"

// assault cube health pointers.
// [base() + 0x1CBD38] + 0x100 ❌
// [base() + 0x1F5288] + 0x100
// [base() + 0x1D5C48] + 0x100 ✅
//
// 1. Get all running processes and find assault cube.
// 2. Get access to modify the game (handle?) -> On MacOS: handle == task port
// 3. Get module base address (base pointer?)
// 4. Read process memory function
// 5. Write process memory function

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr,
            "Usage: %s < --explore | --run > [target_process_name] "
            "[target_module_fragment]\n",
            argv[0]);
    fprintf(stderr,
            "Example: %s --run assaultcube "
            "assaultcube.app/Contents/MacOS/assaultcube\n",
            argv[0]);
    fprintf(stderr,
            "Example: %s --explore assaultcube "
            "assaultcube.app/Contents/MacOS/assaultcube\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  const char *mode_flag = argv[1];
  const char *process_name_to_find =
      ASSAULT_CUBE_PROCESS_NAME; // Default or from argv[2]
  const char *module_path_fragment =
      ASSAULT_CUBE_DYLD_IMAGE_PATH_FRAGMENT; // Default or from argv[3]

  if (argc > 2) {
    process_name_to_find = argv[2];
  }
  if (argc > 3) {
    module_path_fragment = argv[3];
  }

  printf("Mode: %s, Process: %s, Module Fragment: %s\n", mode_flag,
         process_name_to_find, module_path_fragment);

  AppContext app_ctx;
  kern_return_t kr = initialize_target_context(process_name_to_find,
                                               module_path_fragment, &app_ctx);

  app_ctx.arena = ArenaAlloc(); // main arena
  if (app_ctx.arena == NULL) {
    perror("main: Failed to allocate main arena");
    return EXIT_FAILURE;
  }

  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "main: Failed to initialize target context. Exiting.\n");
    return EXIT_FAILURE;
  }

  if (strcmp(mode_flag, "--explore") == 0) {
      printf("Exploration mode goes brr...");
  } else if (strcmp(mode_flag, "--run") == 0) {
      printf("Cheat mode goes brr...");
  } else {
    fprintf(stderr, "Unknown mode: %s\n", mode_flag);
    fprintf(stderr,
            "Usage: %s < --explore | --run > [target_process_name] "
            "[target_module_fragment]\n",
            argv[0]);
    ArenaRelease(app_ctx.arena);
    return EXIT_FAILURE;
  }

  ArenaRelease(app_ctx.arena);

  fprintf(stdout, "\nCheat process finished, closing.\n");
  return EXIT_SUCCESS;
}
