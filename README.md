# Dynamic Thread Pool

### Basic

For each thread in the thread pool, it may receive a task at some point. The exact task is not known when the thread is created. Expressed in C++ language, this means that a thread in a thread pool:

- should be able to execute arbitrary functions - supporting any list of parameters, and any return value type.

    > the modern C++ approach would be to use the infrastructure provided by the functional header file (std::bind , std::function , etc.) in combination with the template parameter package.

- it should be possible to send the results of the execution of a task back to the publisher of the task.

    > the old-fashioned approach is to register the callback function at the same time as the task is published; the modern C++ approach would be to use std::packaged_task in combination with std::future to implement it.

- should be able to be woken up to perform a task when needed, without taking up excessive CPU resources when not needed.

    > consider std::condition_variable for less frequent tasks, or std::this_thread::yield for very frequent tasks.

- should be controllable by the master thread to pause tasks, stop receiving tasks, discard unfinished tasks, etc. when appropriate.

    > this can be achieved by setting an internal variable as a token and having each worker thread check that token periodically.

### Feature

- Lazy initialization.

- Dynamicly recycle thread.

- Different way for executing task: getting result from aysnchronous task and no result.

### TODO List

- Swith for recycle thread or not

- Swith for log

- Priority Queue