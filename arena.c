#include "arena.h"
#include <assert.h> // For assertions
#include <stdlib.h> // For malloc, free
#include <string.h> // For memset

// Default size for a new arena region 1MB
// TODO: Will change it probably in the future if more granular control is
// needed.
#define ARENA_DEFAULT_CAPACITY (1024 * 1024)
// Default alignment for allocations (8 bytes for 64-bit systems)
#define ARENA_DEFAULT_ALIGNMENT 8

// Helper function to align a pointer/offset upwards
static inline U64 AlignForward(U64 ptr_value, U64 align) {
  return (ptr_value + (align - 1)) & ~(align - 1);
}

Arena *ArenaAlloc(void) {
  // Allocate the Arena struct itself
  Arena *arena = (Arena *)malloc(sizeof(Arena));
  if (arena == NULL) {
    return NULL;
  }

  // Allocate the backing buffer for the arena
  arena->buffer = (unsigned char *)malloc(ARENA_DEFAULT_CAPACITY);
  if (arena->buffer == NULL) {
    free(arena); // Clean up the already allocated Arena struct
    return NULL;
  }

  arena->capacity = ARENA_DEFAULT_CAPACITY;
  arena->current_offset = 0;
  arena->previous_offset = 0;

  assert(arena->buffer != NULL);
  assert(arena->capacity == ARENA_DEFAULT_CAPACITY);
  assert(arena->current_offset == 0);

  return arena;
}

void ArenaRelease(Arena *arena) {
  assert(arena != NULL); // Assert that we are not trying to free a NULL arena
                         // pointer itself
  assert(arena->buffer != NULL ||
         arena->capacity == 0); // Buffer should be valid or capacity 0

  free(arena->buffer);
  free(arena);
}

void *ArenaPush(Arena *arena, U64 size) {
  assert(arena != NULL);
  assert(arena->buffer != NULL);
  assert(size > 0); // Typically, allocating zero bytes has specific meanings or
                    // isn't useful. If size can be 0, this assert should be
                    // removed and behavior defined.

  if (arena == NULL || arena->buffer == NULL || size == 0) {
    return NULL; // Invalid input or zero size allocation
  }

  // Calculate the next aligned offset
  // The address we will return must be aligned.
  // The current_offset itself might not be aligned after the previous
  // allocation.
  U64 aligned_current_offset =
      AlignForward(arena->current_offset, ARENA_DEFAULT_ALIGNMENT);

  // Check if there's enough space for the aligned allocation + requested size
  if (aligned_current_offset + size > arena->capacity) {
    // Out of memory region
    return NULL;
  }

  // Get the pointer to the start of the newly allocated aligned block
  void *ptr = arena->buffer + aligned_current_offset;

  // Update the current offset for the next allocation
  arena->previous_offset = arena->current_offset; // Store before update
  arena->current_offset = aligned_current_offset + size;

  assert(ptr != NULL);
  assert((U64)((unsigned char *)ptr - arena->buffer) == aligned_current_offset);
  assert(arena->current_offset <= arena->capacity);

  return ptr;
}

void *ArenaPushZero(Arena *arena, U64 size) {
  // Assertions for arena and size are handled by ArenaPush
  void *result = ArenaPush(arena, size);

  if (result != NULL) {
    // Zero out the allocated memory
    memset(result, 0, size);
  }

  assert(result == NULL || (arena->current_offset >= size &&
                            (U64)((unsigned char *)result - arena->buffer +
                                  size) <= arena->current_offset));

  return result;
}
