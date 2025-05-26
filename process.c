#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>

#include "process.h"

pid_t process_id_find_by_name(const char *process_name) {
  char process_name_truncated[17];
  process_name_truncate(process_name, process_name_truncated,
                        sizeof(process_name_truncated));

  size_t process_list_size;
  struct kinfo_proc *process_list = process_list_get(&process_list_size);
  if (!process_list) {
    return -1;
  }

  size_t process_list_length = process_list_size / sizeof(struct kinfo_proc);

  // Default if not found.
  pid_t process_id = -1;

  for (size_t i = 0; i < process_list_length; i++) {
    if (strcmp(process_list[i].kp_proc.p_comm, process_name_truncated) == 0) {
      process_id = process_list[i].kp_proc.p_pid;
      break;
    }
  }

  free(process_list);
  return process_id;
}

struct kinfo_proc *process_list_get(size_t *process_list_size) {
  // Get the size of the processes list first so that we know how much memory to
  // allocate.
  if (sysctlbyname("kern.proc.all", NULL, process_list_size, NULL, 0) != 0) {
    perror("Error getting the size of the process list");
    return NULL;
  };

  struct kinfo_proc *process_list = malloc(*process_list_size);
  if (!process_list) {
    perror("Memory allocation for process list failed");
    return NULL;
  }

  if (sysctlbyname("kern.proc.all", process_list, process_list_size, NULL, 0) !=
      0) {
    perror("Error getting the process list");
    free(process_list);
    return NULL;
  };

  return process_list;
}

void process_name_truncate(const char *input, char *output,
                           size_t output_size) {
  // TODO checks that strings exist?
  strncpy(output, input, output_size);
  output[output_size - 1] = '\0';
}
