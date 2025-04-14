//
// This file contains all of the implementations of the replacement_policy
// constructors from the replacement_policies.h file.
//
// It also contains stubs of all of the functions that are added to each
// replacement_policy struct at construction time.
//
// ============================================================================
// NOTE: It is recommended that you read the comments in the
// replacement_policies.h file for further context on what each function is
// for.
// ============================================================================
//

#include "replacement_policies.h"

// For LRU
struct lru_data
{
    uint32_t **ages; // 2D array for tracking age: [set][age]
    uint32_t sets;
    uint32_t associativity;
};

// LRU Replacement Policy
// ============================================================================
// TODO feel free to create additional structs/enums as necessary

void lru_cache_access(struct replacement_policy *replacement_policy,
                      struct cache_system *cache_system, uint32_t set_idx, uint32_t tag)
{
    // NOTE update the LRU replacement policy state given a new memory access
    struct lru_data *lru = (struct lru_data *)replacement_policy->data;

    // Find the cache line with the tag
    int start_index = set_idx * cache_system->associativity;
    int accessed_index = -1;
    for (uint32_t i = 0; i < lru->associativity; i++)
    {
        struct cache_line *line = &cache_system->cache_lines[start_index + i];
        if (line->status != INVALID && line->tag == tag)
        {
            accessed_index = i;
            break;
        }
    }

    if (accessed_index == -1)
    { // Miss
        return;
    }

    // Update the ages
    uint32_t current_age = lru->ages[set_idx][accessed_index];
    for (uint32_t i = 0; i < lru->associativity; i++)
    {
        if (lru->ages[set_idx][i] > current_age)
        {
            lru->ages[set_idx][i]--;
        }
    }
    lru->ages[set_idx][accessed_index] = lru->associativity - 1;
}

uint32_t lru_eviction_index(struct replacement_policy *replacement_policy,
                            struct cache_system *cache_system, uint32_t set_idx)
{
    // NOTE return the index within the set that should be evicted.
    struct lru_data *lru = (struct lru_data *)replacement_policy->data;

    // Search for the oldest age (0) in the set
    for (uint32_t i = 0; i < lru->associativity; i++)
    {
        if (lru->ages[set_idx][i] == 0)
        {
            return i;
        }
    }
    perror("No invalid cache line found");
    exit(1);
}

void lru_replacement_policy_cleanup(struct replacement_policy *replacement_policy)
{
    // NOTE cleanup any additional memory that you allocated in the
    // lru_replacement_policy_new function.
    struct lru_data *lru = (struct lru_data *)replacement_policy->data;
    for (uint32_t i = 0; i < lru->sets; i++)
    {
        free(lru->ages[i]);
    }
    free(lru->ages);
    free(lru);
}

struct replacement_policy *lru_replacement_policy_new(uint32_t sets, uint32_t associativity)
{
    struct replacement_policy *lru_rp = calloc(1, sizeof(struct replacement_policy));
    lru_rp->cache_access = &lru_cache_access;
    lru_rp->eviction_index = &lru_eviction_index;
    lru_rp->cleanup = &lru_replacement_policy_cleanup;

    // NOTE allocate any additional memory to store metadata here and assign to
    // lru_rp->data.
    struct lru_data *lru = calloc(1, sizeof(struct lru_data));
    lru->sets = sets;
    lru->associativity = associativity;
    // Allocate the ages 2D array
    lru->ages = calloc(sets, sizeof(uint32_t *));
    for (int i = 0; i < sets; i++)
    {
        lru->ages[i] = calloc(associativity, sizeof(uint32_t));
        // Initialize the ages - initially all lines are 0, 1, 2, ..., associativity-1
        // This ensures all lines in a set have unique ages
        for (int j = 0; j < associativity; j++)
        {
            lru->ages[i][j] = j;
        }
    }
    lru_rp->data = lru;
    return lru_rp;
}

// RAND Replacement Policy
// ============================================================================
void rand_cache_access(struct replacement_policy *replacement_policy,
                       struct cache_system *cache_system, uint32_t set_idx, uint32_t tag)
{
    // NOTE: update the RAND replacement policy state given a new memory access
    // Do not need to do anything for RAND policy
    return;
}

uint32_t rand_eviction_index(struct replacement_policy *replacement_policy,
                             struct cache_system *cache_system, uint32_t set_idx)
{
    // NOTE: return the index within the set that should be evicted.
    // This should be a random index within the set.
    return rand() % cache_system->associativity;
}

void rand_replacement_policy_cleanup(struct replacement_policy *replacement_policy)
{
    // NOTE: cleanup any additional memory that you allocated in the
    // rand_replacement_policy_new function.
    // No additional memory was allocated in rand_replacement_policy_new,
    // so there is nothing to clean up here.
    return;
}

struct replacement_policy *rand_replacement_policy_new(uint32_t sets, uint32_t associativity)
{
    // Seed randomness
    srand(time(NULL));

    struct replacement_policy *rand_rp = malloc(sizeof(struct replacement_policy));
    rand_rp->cache_access = &rand_cache_access;
    rand_rp->eviction_index = &rand_eviction_index;
    rand_rp->cleanup = &rand_replacement_policy_cleanup;

    // NOTE: allocate any additional memory to store metadata here and assign to
    // rand_rp->data.
    // No addition memory is needed for the RAND policy.
    rand_rp->data = NULL;
    return rand_rp;
}

// LRU_PREFER_CLEAN Replacement Policy
// ============================================================================
void lru_prefer_clean_cache_access(struct replacement_policy *replacement_policy,
                                   struct cache_system *cache_system, uint32_t set_idx,
                                   uint32_t tag)
{
    // NOTE update the LRU_PREFER_CLEAN replacement policy state given a new
    // memory access
    // NOTE: you may be able to share code with the LRU policy
    struct lru_data *lru = (struct lru_data *)replacement_policy->data;

    // Find the cache line with the tag
    int start_idx = set_idx * cache_system->associativity;
    int access_index = -1;
    for (uint32_t i = 0; i < lru->associativity; i++)
    {
        struct cache_line *line = &cache_system->cache_lines[start_idx + i];
        if (line->status != INVALID && line->tag == tag)
        {
            access_index = i;
            break;
        }
    }

    if (access_index == -1)
    { // Miss
        return;
    }

    // Update the age
    uint32_t current_age = lru->ages[set_idx][access_index];
    for (uint32_t i = 0; i < lru->associativity; i++)
    {
        if (lru->ages[set_idx][i] > current_age)
        {
            lru->ages[set_idx][i]--;
        }
    }
    lru->ages[set_idx][access_index] = lru->associativity - 1;
}

uint32_t lru_prefer_clean_eviction_index(struct replacement_policy *replacement_policy,
                                         struct cache_system *cache_system, uint32_t set_idx)
{
    // NOTE return the index within the set that should be evicted.
    struct lru_data *lru_pc = (struct lru_data *)replacement_policy->data;
    int set_start = set_idx * cache_system->associativity;
    // First, try to find the least recently used clean line
    uint32_t oldest_clean_index = UINT32_MAX;
    uint32_t oldest_clean_age = UINT32_MAX;
    for (int i = 0; i < cache_system->associativity; i++)
    {
        struct cache_line *line = &cache_system->cache_lines[set_start + i];
        if (line->status == EXCLUSIVE)
        { // Clean line
            if (lru_pc->ages[set_idx][i] < oldest_clean_age)
            {
                oldest_clean_age = lru_pc->ages[set_idx][i];
                oldest_clean_index = i;
            }
        }
    }

    if (oldest_clean_index != UINT32_MAX)
    {
        return oldest_clean_index;
    }

    // Otherwise, find the least recently used dirty line
    uint32_t oldest_dirty_age = UINT32_MAX;
    uint32_t oldest_dirty_index = 0;
    for (int i = 0; i < cache_system->associativity; i++)
    {
        if (lru_pc->ages[set_idx][i] < oldest_dirty_age)
        {
            oldest_dirty_age = lru_pc->ages[set_idx][i];
            oldest_dirty_index = i;
        }
    }
    return oldest_dirty_index;
}

void lru_prefer_clean_replacement_policy_cleanup(struct replacement_policy *replacement_policy)
{
    // NOTE cleanup any additional memory that you allocated in the
    // lru_prefer_clean_replacement_policy_new function.
    struct lru_data *lru = (struct lru_data *)replacement_policy->data;
    for (uint32_t i = 0; i < lru->sets; i++)
    {
        free(lru->ages[i]);
    }
    free(lru->ages);
    free(lru);
}

struct replacement_policy *lru_prefer_clean_replacement_policy_new(uint32_t sets,
                                                                   uint32_t associativity)
{
    struct replacement_policy *lru_prefer_clean_rp = malloc(sizeof(struct replacement_policy));
    lru_prefer_clean_rp->cache_access = &lru_prefer_clean_cache_access;
    lru_prefer_clean_rp->eviction_index = &lru_prefer_clean_eviction_index;
    lru_prefer_clean_rp->cleanup = &lru_prefer_clean_replacement_policy_cleanup;

    // NOTE allocate any additional memory to store metadata here and assign to
    // lru_prefer_clean_rp->data.
    struct lru_data *lru = calloc(1, sizeof(struct lru_data));
    lru->sets = sets;
    lru->associativity = associativity;
    // Allocate the ages 2D array
    lru->ages = calloc(sets, sizeof(uint32_t *));
    for (uint32_t i = 0; i < sets; i++)
    {
        lru->ages[i] = calloc(associativity, sizeof(uint32_t));
        // Initialize the ages - initially all lines are 0, 1, 2, ..., associativity-1
        for (uint32_t j = 0; j < associativity; j++)
        {
            lru->ages[i][j] = j;
        }
    }
    lru_prefer_clean_rp->data = lru;

    return lru_prefer_clean_rp;
}
