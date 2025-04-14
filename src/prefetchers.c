//
// This file defines the function signatures necessary for creating the three
// cache systems and defines the prefetcher struct.
//

#include "prefetchers.h"

// Null Prefetcher
// ============================================================================
uint32_t null_handle_mem_access(struct prefetcher *prefetcher, struct cache_system *cache_system,
                                uint32_t address, bool is_miss)
{
    return 0; // No lines prefetched
}

void null_cleanup(struct prefetcher *prefetcher) {}

struct prefetcher *null_prefetcher_new()
{
    struct prefetcher *null_prefetcher = calloc(1, sizeof(struct prefetcher));
    null_prefetcher->handle_mem_access = &null_handle_mem_access;
    null_prefetcher->cleanup = &null_cleanup;
    return null_prefetcher;
}

// Sequential Prefetcher
// ============================================================================
// TODO feel free to create additional structs/enums as necessary

// Structure to store sequential prefetcher data
struct sequential_data
{
    uint32_t prefetch_amount;
};

uint32_t sequential_handle_mem_access(struct prefetcher *prefetcher,
                                      struct cache_system *cache_system, uint32_t address,
                                      bool is_miss)
{
    // Cast the data pointer to the sequential_data struct
    struct sequential_data *data = (struct sequential_data *)prefetcher->data;
    uint32_t prefetch_amount = data->prefetch_amount;

    // If prefetch_amount is 0, don't prefetch anything
    if (prefetch_amount == 0)
    {
        return 0;
    }

    // Calculate the next sequential addresses to prefetch
    uint32_t line_size = cache_system->line_size;
    uint32_t lines_prefetched = 0;

    for (uint32_t i = 1; i <= prefetch_amount; i++)
    {
        // Calculate the next sequential address (current address + i * line_size)
        uint32_t next_address = address + (i * line_size);

        // Perform the prefetch by calling cache_system_mem_access with is_prefetch=true
        if (cache_system_mem_access(cache_system, next_address, 'R', true) == 0)
        {
            lines_prefetched++;
        }
    }

    return lines_prefetched;
}

void sequential_cleanup(struct prefetcher *prefetcher)
{
    // Free the sequential_data struct that was allocated in sequential_prefetcher_new
    free(prefetcher->data);
}

struct prefetcher *sequential_prefetcher_new(uint32_t prefetch_amount)
{
    struct prefetcher *sequential_prefetcher = calloc(1, sizeof(struct prefetcher));
    sequential_prefetcher->handle_mem_access = &sequential_handle_mem_access;
    sequential_prefetcher->cleanup = &sequential_cleanup;

    // Allocate and initialize data for the sequential prefetcher
    struct sequential_data *data = calloc(1, sizeof(struct sequential_data));
    data->prefetch_amount = prefetch_amount;
    sequential_prefetcher->data = data;

    return sequential_prefetcher;
}

// Adjacent Prefetcher
// ============================================================================
uint32_t adjacent_handle_mem_access(struct prefetcher *prefetcher,
                                    struct cache_system *cache_system, uint32_t address,
                                    bool is_miss)
{
    // Get the cache line size to calculate the next address
    uint32_t line_size = cache_system->line_size;
    uint32_t lines_prefetched = 0;

    // Prefetch the line after the current one (the adjacent line)
    uint32_t next_address = address + line_size;
    if (cache_system_mem_access(cache_system, next_address, 'R', true) == 0)
    {
        lines_prefetched++;
    }

    return lines_prefetched;
}

void adjacent_cleanup(struct prefetcher *prefetcher)
{
    // No additional memory was allocated in adjacent_prefetcher_new,
    // so no cleanup is needed
}

struct prefetcher *adjacent_prefetcher_new()
{
    struct prefetcher *adjacent_prefetcher = calloc(1, sizeof(struct prefetcher));
    adjacent_prefetcher->handle_mem_access = &adjacent_handle_mem_access;
    adjacent_prefetcher->cleanup = &adjacent_cleanup;

    // The adjacent prefetcher doesn't need any additional data
    // since it simply prefetches the next adjacent line on every access

    return adjacent_prefetcher;
}

// Custom Prefetcher
// ============================================================================
uint32_t custom_handle_mem_access(struct prefetcher *prefetcher, struct cache_system *cache_system,
                                  uint32_t address, bool is_miss)
{
    // TODO perform the necessary prefetches for your custom strategy.

    // TODO: Return the number of lines that were prefetched.
    return 0;
}

void custom_cleanup(struct prefetcher *prefetcher)
{
    // TODO cleanup any additional memory that you allocated in the
    // custom_prefetcher_new function.
}

struct prefetcher *custom_prefetcher_new()
{
    struct prefetcher *custom_prefetcher = calloc(1, sizeof(struct prefetcher));
    custom_prefetcher->handle_mem_access = &custom_handle_mem_access;
    custom_prefetcher->cleanup = &custom_cleanup;

    // TODO allocate any additional memory needed to store metadata here and
    // assign to custom_prefetcher->data.

    return custom_prefetcher;
}
