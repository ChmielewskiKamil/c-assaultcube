#ifndef PROCESS_H
#define PROCESS_H

#include <mach-o/dyld_images.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/sysctl.h> // for kinfo_proc struct
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

/**
 * @brief Attempts to get the Mach task port for a given process ID (PID).
 *
 * This function wraps the macOS `task_for_pid` Mach call. To succeed,
 * the calling process usually requires appropriate permissions (e.g., running
 * as root or possessing specific entitlements like
 * `com.apple.security.cs.debugger`).
 *
 * @param pid The process ID of the target task. Must be a positive integer.
 * @param out_task_port Pointer to a mach_port_t variable where the resulting
 * task port will be stored on success. If the function
 * does not return KERN_SUCCESS, this will be set to
 * MACH_PORT_NULL.
 *
 * @return KERN_SUCCESS if the task port was successfully obtained.
 * Otherwise, a Mach kern_return_t error code indicating the failure.
 * Common error codes include:
 * - KERN_FAILURE (5): General failure, often due to permissions.
 * - KERN_INVALID_ARGUMENT (4): Process with the given PID doesn't exist.
 * The specific error can be converted to a string using mach_error_string().
 *
 * @note The caller is responsible for checking the returned kern_return_t.
 * If KERN_SUCCESS is returned, out_task_port will contain a valid port
 * that the caller may need to deallocate eventually if it's a send right
 * that is not automatically managed.
 * If an error is returned, *out_task_port is set to MACH_PORT_NULL.
 */
kern_return_t task_port_find_by_pid(pid_t pid, mach_port_t *out_task_port);

/**
 * @brief Gets information about all loaded software (like the main program and
 * libraries) for a target process.
 *
 * This function asks the operating system (using a low-level call called
 * `task_info`) for details about the software components currently loaded into
 * the memory of the process specified by `target_port`.
 *
 * The most important piece of info this function gets is a memory address. This
 * address, stored in `out_dyld_info->all_image_info_addr`, tells you where to
 * find a complete list of all loaded items (the game itself, system libraries,
 * etc.) within the target process's memory.
 *
 * @param target_port The special ID (Mach port) for the game/process you're
 * interested in. You can get the port from task_port_find_by_pid function.
 * @param out_dyld_info A pointer to a `task_dyld_info_data_t` variable. If this
 * function succeeds, it will fill your variable with the info.
 * If it fails, this variable's contents might not be useful (though the
 * function aims to clear `all_image_info_addr` on failure).
 *
 * @return Returns `KERN_SUCCESS` (which is 0) if it worked.
 * If it didn't work, it returns a different number (an error code from the OS).
 * You MUST check this return value. If it's not `KERN_SUCCESS`,
 * then `out_dyld_info` doesn't have the data you need.
 * You can use `mach_error_string()` with the error code to get a
 * human-readable error message.
 */
kern_return_t
task_dyld_info_find_by_task_port(mach_port_t task_port,
                                 task_dyld_info_data_t *out_task_dyld_info);

/**
 * @brief Finds the base address of a loaded module by searching for a path
 * fragment.
 *
 * Iterates through a list of loaded dyld_image_info structures, reads the
 * file path for each image from the target task's memory, and compares it
 * against the provided path fragment.
 *
 * @param target_task The Mach port of the target process.
 * @param image_list A pointer to an array of dyld_image_info structures
 * (previously read from the target process). This function does NOT take
 * ownership of this memory.
 * @param infoArrayCount The number of elements in the image_list array.
 * @param image_path_fragment A substring to search for within the module file
 * paths (e.g., "AssaultCube.app/Contents/MacOS/assaultcube"). The search is
 * case-sensitive via strstr.
 * @param out_module_base_address Pointer to a mach_vm_address_t where the base
 * address of the first matching module will be stored.
 * Initialized to 0 if not found.
 * @return KERN_SUCCESS if a module matching the path fragment is found.
 * KERN_FAILURE if the module is not found after checking all images.
 * Other kern_return_t codes if reading an image path fails.
 */
kern_return_t find_module_base_address(
    mach_port_t target_task, const struct dyld_image_info *image_list,
    uint32_t infoArrayCount, const char *image_path_fragment,
    mach_vm_address_t *out_module_base_address);

/**
 * @brief Reads a specified number of bytes from the target process's memory.
 *
 * @param target_task The Mach port of the target process.
 * @param address The memory address in the target process to start reading
 * from.
 * @param size The number of bytes to read. MUST be greater than zero.
 * @param buffer A pointer to a buffer in the current process where the read
 * data will be stored. This buffer must be at least 'size' bytes large.
 * @param out_actual_bytes_read Optional pointer to a mach_vm_size_t variable.
 * If not NULL, this will be filled with the number of bytes actually read. It's
 * good practice to check this.
 * @return KERN_SUCCESS if the read was successful and the requested number of
 * bytes were read. Otherwise, a Mach kern_return_t error code.
 */
kern_return_t read_target_memory(mach_port_t target_task,
                                 mach_vm_address_t address, mach_vm_size_t size,
                                 void *buffer,
                                 mach_vm_size_t *out_actual_bytes_read);

/**
 * @brief Resolves a multi-level pointer path starting from a base address.
 *
 * This function navigates a chain of pointers by adding offsets and
 * dereferencing. Each offset in the 'offsets' array (except the very last one)
 * is added to the current address, and then the value at that new address
 * (which is expected to be another pointer/address) is read. This new address
 * becomes the base for the next step. The final offset in the 'offsets' array
 * is added to the last successfully dereferenced pointer to get the final
 * target address.
 *
 * @param target_task The Mach port of the target process.
 * @param base_address The initial base address to start from.
 * @param offsets An array of offsets. All but the last are used for
 * dereferencing. The last offset is added to the final pointer.
 * @param num_offsets The total number of offsets in the 'offsets' array. Must
 * be at least 1. (num_offsets - 1) pointer dereferences will occur.
 * @param out_final_address Pointer to a mach_vm_address_t where the resulting
 * final memory address will be stored.
 * @return KERN_SUCCESS if the entire path was resolved successfully.
 * Otherwise, a Mach error code (e.g., from reading memory) or KERN_FAILURE.
 */
kern_return_t resolve_pointer_path(mach_port_t target_task,
                                   mach_vm_address_t base_address,
                                   const intptr_t offsets[], int num_offsets,
                                   mach_vm_address_t *out_final_address);

#endif // PROCESS_H
