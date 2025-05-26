#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

// process_id_find_by_name finds the process id (PID) that matches the provided
// string. The caller of this function:
// - SHOULD check the return value: -1 on error;
// - does not have to truncate the process name;
//
// Since the names of processes returned by the system are truncated to 17 chars
// (16 + null terminator), this function will truncate user provided input to
// get a correct match. As an example the system sees a game called
// "VeryLongGameTitleThatDoesNotEnd" as "VeryLongGameTitl" (16 chars). The
// caller can provide a long string, and this function will represent it in a
// correct way.
pid_t process_id_find_by_name(const char *process_name);

// process_list_get gets the list of all active processes on the system. The
// caller of this function:
// - MUST free the memory allocated for the process list;
// - SHOULD check if the returned list is not NULL.
struct kinfo_proc *process_list_get(size_t *process_list_size);

// process_name_truncate truncates the input string to the provided output size.
// The output string is null terminated with '\0'. This function is useful when
// searching for a process in a process list, since the names of the processes
// are truncated to 17 chars (16 + '\0').
void process_name_truncate(const char *input, char *output, size_t output_size);

#endif // PROCESS_H
