## Scheduler
a small job scheduler.

### Usage

```c++
    static scheduler::Scheduler scheduler;

    auto [job, ok] = scheduler.set(scheduler::ONCE, "alarm", 5s, 
              []
              {
                    std::cout << "ding dong\n";
              });

    if(!ok)
        return;
    
    std::cout << job.name << " is running on thread #" << job.thread.get_id() << '\n';
    
    scheduler.wait();
```

## API

jobs are represented with the `Job` struct.

```c++
    struct Job
    {
        using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

        const std::string_view name;
        
        TimePoint created_at = std::chrono::system_clock::now();
        TimePoint end_point;
        
        std::thread thread;
        
        bool should_run = true;
        bool should_loop;
    };
```

the `should_run` field can be used to pause and resume the job and the `should_loop` field represents if a job should continue looping.

you can set a job to repeat with `scheduler::REPEAT` or for it to run once with `scheduler::ONCE` by supplying it as the first param to `scheduler::Scheduler::set`.

all pending jobs are represented with `Scheduler::Jobs` which is an alias for `std::map<std::string_view, Job>`.

### Job durations
you can set the job duration one of 3 ways.

* `std::chrono::Duration`
* `std::chrono::time_point`
* with a cron syntax 

#### Crun syntax example
```c++
    scheduler.set(scheduler::ONCE, "job", "* * * 1 1 0", []{std::cout << "hello\n";});
```

this will set the job for the first day of the year

### Helper methods

* `cancel` cancels a job by its id
* `clear` clears all pending jobs
*  `wait` waits for all jobs to finish
