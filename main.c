#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"
// assault cube health pointers.
// [[base() + 0x1d9ef0]] + 0x418
// [[base() + 0x1cc658] + 0x3e8] + 0x638
// [[base() + 0x1cc640] + 0x7d8] + 0x638
// [[base() + 0x1cc648] + 0x358] + 0x638
// [[base() + 0x1cc658] + 0x238] + 0x7e8
// [[base() + 0x1cc640] + 0x628] + 0x7e8
// [[base() + 0x1cc648] + 0x1a8] + 0x7e8
// [[base() + 0x1cc658] + 0x478] + 0x5a8
// [[base() + 0x1cc648] + 0x3e8] + 0x5a8
// [[base() + 0x1cc658] + 0x2c8] + 0x758
// [[base() + 0x1cc640] + 0x6b8] + 0x758
// [[base() + 0x1cc648] + 0x238] + 0x758
// [[base() + 0x1cc658] + 0x358] + 0x6c8
// [[base() + 0x1cc640] + 0x748] + 0x6c8
// [[base() + 0x1cc648] + 0x2c8] + 0x6c8
//
// 1. Get all running processes and find assault cube.
// 2. Get access to modify the game (handle?) -> On MacOS: handle == task port
// 3. Get module base address (base pointer?)
// 4. Read process memory function
// 5. Write process memory function

int main(void) {
  printf("Give me the process name:\n");

  size_t buffer_size = 256;
  char *process_name = malloc(buffer_size);

  if (!process_name) {
    perror("Memory allocation failed");
    return 1;
  }

  if (getline(&process_name, &buffer_size, stdin) != -1) {
    size_t len = strlen(process_name);
    // getline returns a string with a newline delimeter, but not with
    // null byte terminator.
    if (len > 0 && process_name[len - 1] == '\n') {
      process_name[len - 1] = '\0';
    }
    printf("You've entered: %s\n", process_name);
  } else {
    printf("Input error.\n");
    free(process_name);
    return 1;
  }

  pid_t process_id;

  process_id = process_id_find_by_name(process_name);

  if (process_id == -1) {
    printf("PID not found for process: %s\n", process_name);

    free(process_name);

    return 1;
  }
  free(process_name);

  printf("PID found: %d\n", process_id);

  mach_port_t assault_cube_task_port;
  kern_return_t result_code;

  result_code = task_port_find_by_pid(process_id, &assault_cube_task_port);
  if (result_code != KERN_SUCCESS) {
    fprintf(stderr, "Failed to get task port: %s (kern_return_t: %d)\n",
            mach_error_string(result_code), result_code);
    return 1;
  } else {
    printf("Successfully obtained task port: %u\n", assault_cube_task_port);
  }

  task_dyld_info_data_t assault_cube_dyld_info;
  result_code = task_dyld_info_find_by_task_port(assault_cube_task_port,
                                                 &assault_cube_dyld_info);
  if (result_code != KERN_SUCCESS) {
    fprintf(stderr, "Failed to get dyld info: %s (kern_return_t: %d)\n",
            mach_error_string(result_code), result_code);
    return 1;
  } else {
    printf("Successfully obtained task_dyld_info_data_t.\n");
    printf("  all_image_info_addr in target process: 0x%llx\n",
           (unsigned long long)assault_cube_dyld_info.all_image_info_addr);
    printf("  all_image_info_size in target process: %llu bytes\n",
           (unsigned long long)assault_cube_dyld_info.all_image_info_size);
    printf("  all_image_info_format: %d (1 means 32-bit, 2 means 64-bit)\n",
           assault_cube_dyld_info.all_image_info_format);
  }

  // Now that we have the task_dyld_info struct, we are ready to read the
  // actual image info data from Assault Cube's memory. We will store the
  // read data in the following struct.
  struct dyld_all_image_infos assault_cube_dyld_all_images_info;

  result_code = read_target_memory(assault_cube_task_port,
                                   assault_cube_dyld_info.all_image_info_addr,
                                   assault_cube_dyld_info.all_image_info_size,
                                   &assault_cube_dyld_all_images_info, NULL);
  if (result_code != KERN_SUCCESS) {
    fprintf(stderr,
            "Failed to read dyld_all_image_infos from target: %s "
            "(kern_return_t: %d)\n",
            mach_error_string(result_code), result_code);
    return 1;
  } else {
    printf("Successfully read dyld_all_image_infos structure from target "
           "process!\n");

    // Print some key fields from the struct dyld_all_image_infos
    // The actual struct definition is in <mach-o/dyld_images.h>
    printf("  Version: %u\n", assault_cube_dyld_all_images_info.version);
    printf("  Number of loaded images (numImages): %u\n",
           assault_cube_dyld_all_images_info.infoArrayCount);

    // infoArray is a POINTER (an address) IN THE TARGET PROCESS'S MEMORY.
    // It points to an array of dyld_image_info structures.
    printf("  infoArray address in target: 0x%llx\n",
           (unsigned long long)assault_cube_dyld_all_images_info.infoArray);

    // dyldImageLoadAddress is the base address of dyld itself in the target
    // process.
    printf("  dyld image load address in target: 0x%llx\n",
           (unsigned long long)
               assault_cube_dyld_all_images_info.dyldImageLoadAddress);

    // This is the address of the dyld_all_image_infos struct itself in the
    // target process. It should match what
    // assault_cube_dyld_info.all_image_info_addr reported.
    if (assault_cube_dyld_all_images_info.dyldAllImageInfosAddress != NULL) {
      printf("  Self-reported address of this dyld_all_image_infos struct in "
             "target: 0x%llx\n",
             (unsigned long long)
                 assault_cube_dyld_all_images_info.dyldAllImageInfosAddress);
    }
  }

  return 0;
}
