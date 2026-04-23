## Low Latency Trading System

_This repository is a fork of the code from [Building Low Latency Applications with C++](https://www.packtpub.com/product/building-low-latency-applications-with-c/9781837639359)._

The aim of this project is to create a model low latency trading system that will run on x86-64 platforms.

This fork builds on some foundations provided by an open-source demo that I believe can be improved significantly in
a number of ways based on review of the code.

### Upcoming fixes and (description of impact of the change on performance):

- [ ] Many uses of move semantics and declarations of move constructors are incorrect - these should be fixed.
- [x] Fix the provided lock-free queue implementation (see commit: https://github.com/jr0246/low_latency_trading_system/commit/f233730e2ef42fd8c634e21983052549e890ec2d)
  - **_Outcome: Resolving this resulted in a reduction of the most severe latency spikes by roughly 40%_**
- [ ] Logging should not be done on the hot path, but rather dispatched to a SPSC buffer that is consumed by a single process (pinned to a separate CPU).
  - [ ] Lots of string concatenation in logs; this needs to be replaced by deterministic logging, i.e. FIX protocol has a well-defined structure, so we can use a `char` array as a skeleton and simply update the missing fields with the relevant symbol data (and the like).
  - [ ] Dispatch binary data only into queue and into memory-mapped file (see below); worry about formatting into strings when the trading session is over.
- [ ] Replace uses of `std::unordered_map` - memory for lookup tables should be allocated up front and in contiguous memory.
  - [ ] Either use Robin Hood Hash table or use a PMR allocator-aware `std::pmr::unordered_map` with pre-allocated memory pool.
- [ ] Replace UDP and TCP sockets with DPDK kernel bypass code to remove the impact of the Linux kernel and network stack on deterministic latency.
- [ ] There is no current support for failover and replay.
  - [ ] We should have the event queue mapped to a file on disk (using `mmap`), so orders and other data are not held in volatile memory (i.e. DRAM).
  - [ ] Build a service that can read logs (or memory-mapped queue contents) and replay them to verify correctness.