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

int main() {
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

  printf("PID found: %d\n", process_id);

  mach_port_t assault_cube_port_task;
  kern_return_t result_code;

  result_code = task_port_find_by_pid(process_id, &assault_cube_port_task);
  if (result_code != KERN_SUCCESS) {
    fprintf(stderr, "Failed to get task port: %s (kern_return_t: %d)\n",
            mach_error_string(result_code), result_code);
    free(process_name);
    return 1;
  } else {
    printf("Successfully obtained task port: %u\n", assault_cube_port_task);
  }

  free(process_name);
  return 0;
}
