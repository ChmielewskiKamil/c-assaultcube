#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "process.h"

pid_t process_id_find_by_name(const char *process_name) {
  assert(process_name != NULL);

  char process_name_truncated[17];
  process_name_truncate(process_name, process_name_truncated,
                        sizeof(process_name_truncated));

  assert(strlen(process_name_truncated) < sizeof(process_name_truncated));
  assert(memchr(process_name_truncated, '\0', sizeof(process_name_truncated)) !=
         NULL);

  size_t process_list_size = 0;
  struct kinfo_proc *process_list = process_list_get(&process_list_size);
  if (!process_list) {
    assert(process_list == NULL);
    return -1;
  }

  assert(process_list != NULL);
  assert(process_list_size % sizeof(struct kinfo_proc) == 0);
  size_t process_list_length = process_list_size / sizeof(struct kinfo_proc);

  // Default if not found.
  pid_t process_id = -1;

  for (size_t i = 0; i < process_list_length; i++) {
    assert(memchr(process_list[i].kp_proc.p_comm, '\0',
                  sizeof(process_list[i].kp_proc.p_comm)) != NULL);

    if (strcmp(process_list[i].kp_proc.p_comm, process_name_truncated) == 0) {
      process_id = process_list[i].kp_proc.p_pid;
      assert(process_id > 0);
      break;
    }
  }

  assert(process_list != NULL);
  free(process_list);
  return process_id;
}

struct kinfo_proc *process_list_get(size_t *process_list_size) {
  assert(process_list_size != NULL);
  // Initialize to known state.
  *process_list_size = 0;
  // Get the size of the processes list first so that we know how much memory to
  // allocate.
  if (sysctlbyname("kern.proc.all", NULL, process_list_size, NULL, 0) != 0) {
    perror("Error getting the size of the process list");

    assert(*process_list_size == 0);
    return NULL;
  }

  struct kinfo_proc *process_list = NULL;
  if (*process_list_size > 0) {
    process_list = malloc(*process_list_size);
    if (!process_list) {
      assert(process_list == NULL);
      perror("Memory allocation for process list failed");
      *process_list_size = 0;
      return NULL;
    }
  } else {
    assert(*process_list_size == 0);
    return NULL;
  }

  assert(process_list != NULL);
  assert(*process_list_size > 0);

  size_t original_requested_size = *process_list_size; // Used for assertion.
  if (sysctlbyname("kern.proc.all", process_list, process_list_size, NULL, 0) !=
      0) {
    perror("Error getting the process list");
    free(process_list);

    assert(*process_list_size == original_requested_size);

    return NULL;
  }

  assert(process_list != NULL);
  assert(*process_list_size <= original_requested_size);
  assert(*process_list_size % sizeof(struct kinfo_proc) == 0);

  return process_list;
}

void process_name_truncate(const char *input, char *output,
                           size_t output_size) {
  if (input == NULL || output == NULL) {
    if (output != NULL && output_size > 0)
      output[0] = '\0';
    assert(output[0] == '\0' && strlen(output) == 0);
    return;
  }

  if (output_size == 0)
    return;

  assert(input != NULL);
  assert(output != NULL);
  assert(output_size > 0);

  // ptr to buffer, size, format, strings
  int ret = snprintf(output, output_size, "%s", input);
  assert(ret >= 0);

  assert(memchr(output, '\0', output_size)); // should be null-terminated
  assert(strlen(output) < output_size); // str length < str length + null term
}
