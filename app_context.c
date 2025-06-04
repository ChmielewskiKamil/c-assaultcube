#include "app_context.h"
#include "process.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

kern_return_t initialize_target_context(const char *process_name_to_find,
                                        const char *target_module_path_fragment,
                                        AppContext *app_ctx) {
  assert(process_name_to_find != NULL);
  assert(target_module_path_fragment != NULL);
  assert(app_ctx != NULL);

  kern_return_t kr;

  app_ctx->pid = -1;
  app_ctx->task_port = MACH_PORT_NULL;
  app_ctx->module_base_address = 0;
  app_ctx->arena = NULL;

  app_ctx->pid = process_id_find_by_name(process_name_to_find);
  if (app_ctx->pid == -1) {
    fprintf(stderr, "Setup: PID not found for process: %s\n",
            process_name_to_find);
    return KERN_FAILURE;
  }

  kr = task_port_find_by_pid(app_ctx->pid, &app_ctx->task_port);
  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "Setup: Failed to get task port: %s\n",
            mach_error_string(kr));
    return kr;
  }

  task_dyld_info_data_t target_dyld_info;
  kr = task_dyld_info_find_by_task_port(app_ctx->task_port, &target_dyld_info);
  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "Setup: Failed to get dyld info: %s\n",
            mach_error_string(kr));
    return kr;
  }

  struct dyld_all_image_infos target_dyld_all_images_infos;
  kr = read_target_memory(app_ctx->task_port,
                          target_dyld_info.all_image_info_addr,
                          target_dyld_info.all_image_info_size,
                          &target_dyld_all_images_infos, NULL);
  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "Setup: Failed to read dyld_all_image_infos: %s\n",
            mach_error_string(kr));
    return kr;
  }

  if (target_dyld_all_images_infos.infoArrayCount == 0) {
    fprintf(stderr, "Setup: Target process reported zero loaded images.\n");
    return KERN_FAILURE;
  }

  mach_vm_size_t tmp_image_info_array_size =
      target_dyld_all_images_infos.infoArrayCount *
      sizeof(struct dyld_image_info);

  struct dyld_image_info *tmp_image_info_array =
      malloc(tmp_image_info_array_size);

  if (!tmp_image_info_array) {
    fprintf(stderr, "Setup: Memory allocation for image info array failed.\n");
    return KERN_RESOURCE_SHORTAGE;
  }

  kr = read_target_memory(
      app_ctx->task_port,
      (mach_vm_address_t)target_dyld_all_images_infos.infoArray,
      tmp_image_info_array_size, tmp_image_info_array, NULL);
  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "Setup: Failed to read image_info_array: %s\n",
            mach_error_string(kr));
    free(tmp_image_info_array);
    return KERN_FAILURE;
  }

  kr = find_module_base_address(app_ctx->task_port, tmp_image_info_array,
                                target_dyld_all_images_infos.infoArrayCount,
                                target_module_path_fragment,
                                &app_ctx->module_base_address);
  if (kr != KERN_SUCCESS || app_ctx->module_base_address == 0) {
    fprintf(stderr, "Setup: Failed to find base address for module: %s\n",
            target_module_path_fragment);
    free(tmp_image_info_array);

    return (kr == KERN_SUCCESS) ? KERN_FAILURE : kr;
  }

  free(tmp_image_info_array);

  return KERN_SUCCESS;
}
