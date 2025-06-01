#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syslimits.h>

#include "arena.h"
#include "process.h"

// assault cube health pointers.
// [base() + 0x1CBD38] + 0x100 ❌
// [base() + 0x1F5288] + 0x100
// [base() + 0x1D5C48] + 0x100
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

  mach_vm_address_t assault_cube_base_module_address = 0;
  char image_path_buffer[PATH_MAX];

  const char *target_module_name = "assaultcube.app/Contents/MacOS/assaultcube";

  for (uint32_t i = 0; i < assault_cube_dyld_all_images_info.infoArrayCount;
       i++) {
    mach_vm_address_t path_address_in_target =
        (mach_vm_address_t)local_image_infos[i].imageFilePath;

    if (path_address_in_target == 0) {
      continue;
    }

    // Zero out buffer before reading into it for safety
    memset(image_path_buffer, 0, PATH_MAX);
    mach_vm_size_t path_bytes_actually_read = 0;

    // Read the path string from the target process.
    // Read PATH_MAX-1 to leave space for a null terminator if the path is
    // exactly PATH_MAX long.
    result_code = read_target_memory(
        assault_cube_task_port, path_address_in_target, PATH_MAX - 1,
        image_path_buffer, &path_bytes_actually_read);

    if (result_code == KERN_SUCCESS && path_bytes_actually_read > 0) {
      // printf("  Image %u: Path: '%s', Load Address: 0x%llx\n", i,
      //        image_path_buffer,
      //        (unsigned long long)local_image_infos[i].imageLoadAddress);
      if (strstr(image_path_buffer, target_module_name) != NULL) {
        assault_cube_base_module_address =
            (mach_vm_address_t)local_image_infos[i].imageLoadAddress;
        printf("!!! Found Target Module !!!\n");
        printf("    Path: %s\n", image_path_buffer);
        printf("    Base Address: 0x%llx\n",
               (unsigned long long)assault_cube_base_module_address);
        break;
      }
    } else if (result_code != KERN_SUCCESS) {
      fprintf(stderr,
              "Warning: Failed to read image path for image %u at 0x%llx: %s\n",
              i, (unsigned long long)path_address_in_target,
              mach_error_string(result_code));
    }
  }

  if (assault_cube_base_module_address == 0) {
    fprintf(stderr, "Failed to find the base address for module: %s\n",
            target_module_name);
    ArenaRelease(main_arena);
    return 1;
  }

  mach_vm_address_t health_address = 0;
  const intptr_t health_offsets[] = {0x1D5C48, 0x100};
  uint num_offsets = sizeof(health_offsets) / sizeof(health_offsets[0]);

  result_code = resolve_pointer_path(
      assault_cube_task_port, assault_cube_base_module_address, health_offsets,
      num_offsets, &health_address);

  if (result_code == KERN_SUCCESS) {
    printf("Resolved final health address: 0x%llx\n",
           (unsigned long long)health_address);

    int player_health = 0;

    result_code =
        read_target_memory(assault_cube_task_port, health_address,
                           sizeof(player_health), &player_health, NULL);

    if (result_code == KERN_SUCCESS) {
      printf("Current player health: %d\n", player_health);
    } else {
      fprintf(stderr, "Failed to read health from resolved address 0x%llx\n",
              (unsigned long long)health_address);
    }
  } else {
    fprintf(stderr, "Failed to resolve pointer path for health.\n");
  }

  ArenaRelease(main_arena);
  fprintf(stdout, "\nCheat process finished, closing.\n");
  return 0;
}
