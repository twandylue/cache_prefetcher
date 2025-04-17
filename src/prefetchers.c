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

// Define constants for the custom prefetcher
#define STREAM_TABLE_SIZE 16    // Number of streams to track
#define CONFIDENCE_THRESHOLD 2  // Number of times a stride must be seen before prefetching
#define MAX_PREFETCH_DISTANCE 4 // Maximum number of lines to prefetch ahead

// Structure to store information about memory access streams
struct stream_entry
{
    uint32_t last_address; // Last address accessed in this stream
    int32_t stride;        // Detected stride (can be negative)
    uint32_t confidence;   // Confidence in the detected strideＦ
    bool valid;            // Whether this entry is valid
};

// Structure to store custom prefetcher data
struct custom_data
{
    struct stream_entry streams[STREAM_TABLE_SIZE]; // Table of tracked streams
    uint32_t next_stream;                           // Index to replace when all entries are valid
    uint32_t prefetches_issued;                     // Total prefetches issued
    uint32_t useful_prefetches;                     // Prefetches that were later accessed
};

// Helper function to find a matching stream or allocate a new one
static struct stream_entry *find_or_allocate_stream(struct custom_data *data, uint32_t address)
{
    // First, look for a matching stream or the first invalid entry
    for (int i = 0; i < STREAM_TABLE_SIZE; i++)
    {
        struct stream_entry *entry = &data->streams[i];

        // Check if this is the stream we're looking for
        if (entry->valid &&
            (address == entry->last_address ||
             address == entry->last_address + entry->stride))
        {
            return entry;
        }

        // If we find an invalid entry, we can use it
        if (!entry->valid)
        {
            entry->valid = true;
            entry->last_address = address;
            entry->stride = 0;     // No stride yet
            entry->confidence = 0; // No confidence yet
            return entry;
        }
    }

    // If all entries are valid, replace using round-robin
    struct stream_entry *victim = &data->streams[data->next_stream];
    data->next_stream = (data->next_stream + 1) % STREAM_TABLE_SIZE;

    // Initialize the entry
    victim->last_address = address;
    victim->stride = 0;
    victim->confidence = 0;
    return victim;
}

uint32_t custom_handle_mem_access(struct prefetcher *prefetcher, struct cache_system *cache_system,
                                  uint32_t address, bool is_miss)
{
    struct custom_data *data = (struct custom_data *)prefetcher->data;
    uint32_t line_size = cache_system->line_size;
    uint32_t lines_prefetched = 0;

    // Align address to cache line boundary
    uint32_t line_address = address - (address % line_size);

    // Find or allocate a stream for this address
    struct stream_entry *stream = find_or_allocate_stream(data, line_address);

    // If this is not the first access to this stream
    if (stream->last_address != line_address)
    {
        int32_t current_stride = (int32_t)(line_address - stream->last_address);

        // If stride is consistent, increase confidence
        if (stream->stride == current_stride)
        {
            stream->confidence = (stream->confidence < 255) ? stream->confidence + 1 : 255;
        }
        else
        {
            // New stride detected, reset confidence and update stride
            stream->stride = current_stride;
            stream->confidence = 1;
        }

        // Update last address
        stream->last_address = line_address;

        // Only prefetch if confidence is above threshold
        if (stream->confidence >= CONFIDENCE_THRESHOLD)
        {
            // Calculate how many lines to prefetch based on confidence
            uint32_t prefetch_distance = (stream->confidence > 10) ? MAX_PREFETCH_DISTANCE : (stream->confidence / 5) + 1;

            // Prefetch lines ahead
            for (uint32_t i = 1; i <= prefetch_distance; i++)
            {
                uint32_t prefetch_addr = line_address + (i * stream->stride);

                // Perform the prefetch
                if (cache_system_mem_access(cache_system, prefetch_addr, 'R', true) == 0)
                {
                    lines_prefetched++;
                    data->prefetches_issued++;
                }
            }
        }
    }
    else
    {
        // First access to this stream, no prefetching
        // Because we don't have a stride yet, we can't prefetch
    }

    return lines_prefetched;
}

void custom_cleanup(struct prefetcher *prefetcher)
{
    // Free the custom_data struct that was allocated
    free(prefetcher->data);
}

struct prefetcher *custom_prefetcher_new()
{
    struct prefetcher *custom_prefetcher = calloc(1, sizeof(struct prefetcher));
    custom_prefetcher->handle_mem_access = &custom_handle_mem_access;
    custom_prefetcher->cleanup = &custom_cleanup;

    // Allocate and initialize data for the custom prefetcher
    struct custom_data *data = calloc(1, sizeof(struct custom_data));

    // Initialize all stream entries to invalid
    for (int i = 0; i < STREAM_TABLE_SIZE; i++)
    {
        data->streams[i].valid = false;
    }

    data->next_stream = 0;
    data->prefetches_issued = 0;
    data->useful_prefetches = 0;

    custom_prefetcher->data = data;
    return custom_prefetcher;
}
