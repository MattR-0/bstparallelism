# Parallel Data Structure Analysis: AVL Tree

## Object
We are going to implement parallel-safe AVL trees using coarse-grained locks, fine-grained locks and lock-free mechanism using C++. We will analyze the cost and speedup of each implementation on a variety of workloads to draw conclusions on the advantages and disadvantages of each approach.

## Schedule
| Week | Date | Task | In-charge |
|:----:|:-----|:-----|:---------:|
| 1    | 3/25 - 3/31 | Create correctness/speed tests and workload sets | Matt |
| 2    | 4/1 - 4/7   |Finish coarse-grained lock implementation | Matt |
| 3    | 4/8 - 4/14  | Finish fine-grained lock implementation | Tra |
| 4    | 4/15 - 4/21 | Enforce correctness, optimize performance, and write milestone report| Both |
| 5    | 4/22 - 4/28 | Finish lock-free implementation| Both |
| 6    | 4/29 - 5/5  | Conduct performance analysis and write final report | Both |