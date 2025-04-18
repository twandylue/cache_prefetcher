# Cache Prefetcher

Run cache prefetcher with `NULL`, `ADJACENT`, `SEQUENTIAL`, and `CUSTOM` mode.

## Run the Prefetcher

- Prefetch strategy: this will be one of the following: `NULL`, `ADJACENT`, `SEQUENTIAL`, or `CUSTOM` representing the prefetch strategy.
- Prefetch amount: this will be an integer representing N, the number of additional cache lines to prefetch (this parameter is only used for the `SEQUENTIAL` strategy and the `CUSTOM` strategy if you choose to make your strategy depend on N).

```bash
$ make
$ ./cachesim <mode> <cache_size> <line_size> <associativity> <prefetch mode> <prefetch amount> < <trace_file>
(...)
```

For example:

```bash
$ ./cachesim LRU 32768 2048 4 SEQUENTIAL 2 < ./inputs/trace1
(...)
```

