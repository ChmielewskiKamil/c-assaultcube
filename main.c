#include <stddef.h>
#include <stdio.h>

#include "arena.h"
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
  const char *process_name = "assaultcube";
  pid_t process_id;

  process_id = process_id_find_by_name(process_name);

  if (process_id == -1) {
    printf("PID not found for process: %s\n", process_name);
    return 1;
  }

  mach_port_t assault_cube_task_port;
  kern_return_t result_code;

  result_code = task_port_find_by_pid(process_id, &assault_cube_task_port);
  if (result_code != KERN_SUCCESS) {
    fprintf(stderr, "Failed to get task port: %s (kern_return_t: %d)\n",
            mach_error_string(result_code), result_code);
    return 1;
  }

  task_dyld_info_data_t assault_cube_dyld_info;
  result_code = task_dyld_info_find_by_task_port(assault_cube_task_port,
                                                 &assault_cube_dyld_info);
  if (result_code != KERN_SUCCESS) {
    fprintf(stderr, "Failed to get dyld info: %s (kern_return_t: %d)\n",
            mach_error_string(result_code), result_code);
    return 1;
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
  }

  if (assault_cube_dyld_all_images_info.infoArrayCount == 0) {
    fprintf(stderr, "Target process reported zero loaded images in "
                    "dyld_all_image_infos.\n");
    return 1;
  }

  Arena *main_arena = ArenaAlloc();

  mach_vm_size_t image_info_array_size =
      assault_cube_dyld_all_images_info.infoArrayCount *
      sizeof(struct dyld_image_info);

  printf("Calculated size for image_info array: %llu bytes (for %u images).\n",
         (unsigned long long)image_info_array_size,
         assault_cube_dyld_all_images_info.infoArrayCount);

  struct dyld_image_info *local_image_infos =
      (struct dyld_image_info *)ArenaPush(main_arena, image_info_array_size);

  if (local_image_infos == NULL) {
    fprintf(stderr, "Failed to allocate buffer for local_image_infos from "
                    "arena (arena full or bad size?).\n");
    ArenaRelease(main_arena);
    return 1;
  }

  result_code = read_target_memory(
      assault_cube_task_port,
      (mach_vm_address_t)assault_cube_dyld_all_images_info.infoArray,
      image_info_array_size, local_image_infos, NULL);

  if (result_code != KERN_SUCCESS) {
    fprintf(stderr,
            "Failed to read dyld image info array from target: %s "
            "(kern_return_t: %d)\n",
            mach_error_string(result_code), result_code);
    ArenaRelease(main_arena);
    return 1;
  }

  ArenaRelease(main_arena);

  fprintf(stdout, "Cheat executed successfully, closing.\n");
  return 0;
}
