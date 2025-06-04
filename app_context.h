#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "arena.h"
#include "process.h"

typedef struct {
  // game process ID
  pid_t pid;
  // mach task port (handle) to read/write game memory
  mach_port_t task_port;
  // address of the game binary; it must be found on startup due to Address
  // Space Layout Randomization
  mach_vm_address_t module_base_address;
  // general purpose arena allocator with the lifetime of the main function
  Arena *arena;
} AppContext;

#define ASSAULT_CUBE_PROCESS_NAME "assaultcube"
#define ASSAULT_CUBE_DYLD_IMAGE_PATH_FRAGMENT                                  \
  "assaultcube.app/Contents/MacOS/assaultcube"

/**
 * @brief Initializes the application context by finding the target process,
 * obtaining its task port, and finding its main module base address.
 * @param process_name_to_find The name of the target process.
 * @param target_module_path_fragment A specific path fragment to identify the
 * main executable module.
 * @param app_ctx Pointer to an AppContext struct to be filled.
 * @return KERN_SUCCESS on full success, or an error code if any step fails.
 */
kern_return_t initialize_target_context(const char *process_name_to_find,
                                        const char *target_module_path_fragment,
                                        AppContext *app_ctx);

#endif // APP_CONTEXT_H
