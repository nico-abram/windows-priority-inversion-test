This is a very simple priority inversion test to see how windows handles it.

Apparently, windows 10 and earlier has a behaviour to execute threads that have not executed for 4+ seconds with maximum priority for a couple quantums (Described in Windows Internals 7th Edition).

In windows 11 the scheduling component windows calls AutoBoost seems to have been enabled for user mode synchronization objects, instead of only kernel mode objects like it was before in windows 10.

The result is that a priority inversion test with the following three threads:

 - Low priority thread: Holds a lock for a mutex and is queued to execute, and will release it when it does.
 - Medium priority thread: Infinitely consumes CPU.
 - High priority thread: Will attempt to lock the same mutex and block until the low priority thread finishes.

Running all three with a thread affinity mask such that they are forced to execute on the same core will starve the low priority thread if priority inversion is not handled in some way.

I'm running the test scenario with two configurations. One is the exact same as described above. The other changes the "high priority thread"'s priority to the same as the low priority thread. This means that the "medium priority thread" will always have a higher priority thant either of them.

The results on windows 10 and window server 2022 (Via github actions)  are:

 - Normal configuration: Takes 4-8 seconds to finish execution
 - No high priority configuration: Takes 4-8 seconds to finish execution


The results on windows server 2025 (Via github actions) are:

 - Normal configuration: Takes 0-1 seconds to finish execution
 - No high priority configuration: Takes 4-8 seconds to finish execution

