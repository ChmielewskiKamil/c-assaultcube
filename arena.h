#ifndef ARENA_H
#define ARENA_H

#include <stddef.h> // For size_t, NULL
#include <stdint.h> // For uint64_t

// Define U64 for convenience
typedef uint64_t U64;

// Arena structure
typedef struct Arena {
  unsigned char *buffer; // Pointer to the start of the allocated memory block
  U64 capacity;          // Total size of the buffer in bytes
  U64 current_offset;  // Current allocation offset from the start of the buffer
  U64 previous_offset; // Offset before the last push (useful for some reset/pop
                       // operations)
} Arena;

// API Functions

/**
 * @brief Allocates and initializes a new memory arena.
 * The arena allocates a default-sized block of memory from the heap.
 * @return Pointer to the newly created Arena, or NULL if allocation fails.
 */
Arena *ArenaAlloc(void);

/**
 * @brief Releases all memory associated with an arena.
 * This includes the main memory block and the Arena structure itself.
 * After this call, the arena pointer and any pointers obtained from it are
 * invalid.
 * @param arena Pointer to the Arena to be released.
 */
void ArenaRelease(Arena *arena);

/**
 * @brief Allocates a block of memory from the arena.
 * Memory is aligned to a default boundary (e.g., 8 bytes).
 * The memory is NOT initialized.
 * @param arena Pointer to the Arena from which to allocate.
 * @param size The number of bytes to allocate.
 * @return Pointer to the allocated block of memory, or NULL if the arena
 * does not have enough space or if an error occurs.
 */
void *ArenaPush(Arena *arena, U64 size);

/**
 * @brief Allocates a block of memory from the arena and initializes it to zero.
 * Memory is aligned to a default boundary (e.g., 8 bytes).
 * @param arena Pointer to the Arena from which to allocate.
 * @param size The number of bytes to allocate.
 * @return Pointer to the allocated and zeroed block of memory, or NULL if the
 * arena does not have enough space or if an error occurs.
 */
void *ArenaPushZero(Arena *arena, U64 size);

#endif // ARENA_H
