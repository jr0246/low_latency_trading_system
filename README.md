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
  - [ ] Lots of string concatenation in logs; this needs to be replaced by deterministic log outputs.
- [ ] Replace uses of `std::unordered_map` - memory for lookup tables should be allocated up front and in contiguous memory.
  - [ ] Either use Robin Hood Hash table or use a PMR allocator-aware `std::pmr::unordered_map` with pre-allocated memory pool.
- [ ] Replace UDP and TCP sockets with DPDK kernel bypass code to remove the impact of the Linux kernel and network stack on deterministic latency.