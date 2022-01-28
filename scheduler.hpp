#pragma once

#include <map>
#include <string_view>
#include <chrono>
#include <thread>
#include <utility>
#include <ctime>

namespace scheduler
{

    static constexpr bool REPEAT = true;
    static constexpr bool ONCE   = false;

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

    class Scheduler
    {
    public:

        using Jobs = std::map<std::string_view, Job>;

        Scheduler() = default;

        template<typename T, typename ...A>
        std::pair<Job&, bool> set(bool repeat, std::string_view job_name, std::string_view cron_str, T fn, A ...a)
        {
            auto time_point = parse_cron(cron_str);
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(time_point-std::chrono::system_clock::now());
            return schedule_job(repeat, job_name, duration, fn, a...);
        }

        template<typename D, typename P, typename T, typename ...A>
        std::pair<Job&, bool> set(bool repeat, std::string_view job_name, std::chrono::duration<D, P> duration, T fn, A ...a)
        {
            return schedule_job(repeat, job_name, duration, fn, a...);
        }

        template<typename C, typename P, typename T, typename ...A>
        std::pair<Job&, bool> set(bool repeat, std::string_view job_name, std::chrono::time_point<C, P> time_point, T fn, A ...a)
        {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(time_point-std::chrono::system_clock::now());
            return schedule_job(repeat, job_name, duration, fn, a...);
        }

        // cancels the given job by its name
        void cancel(std::string_view job_name)
        {
            if(!jobs.contains(job_name))
                return;

            Job &job = jobs.at(job_name);

            job.thread.detach();

            jobs.erase(job_name);
        }

        // waits till all jobs have finished
        // if the stop_loop param is set to true it will prevent any job from looping
        void wait(bool stop_loop = false)
        {
            for(auto &[_, job] : jobs)
            {
                if(stop_loop)
                    job.should_loop = false;
                if(job.thread.joinable())
                    job.thread.join();
            }
        }

        // cancels all pending jobs
        void clear()
        {
            // one could call std::map::clear instead but all active threads have to be detached beforehand
            for(auto &[name, _] : jobs)
                cancel(name);
        }

        Jobs jobs;

    private:

        template<typename D, typename P, typename T, typename ...A>
        std::pair<Job&, bool> schedule_job(bool loop, std::string_view job_name, std::chrono::duration<D, P> duration, T fn, A ...a)
        {
            Jobs& _jobs = jobs;

            try
            {
                std::thread t
                {
                    [loop, duration, fn, &_jobs, job_name, a...] ()
                    {
                        do
                        {
                            std::this_thread::sleep_for(duration);

                            if(!_jobs.contains(job_name))
                                return;

                            Job &job = _jobs.at(job_name);

                            while(!job.should_run)
                                std::this_thread::yield();

                            fn(a...);

                            if(!job.should_loop)
                                return;
                        }
                        while(loop);
                    }
                };

                jobs[job_name] =
                {
                    .name        = job_name,
                    .end_point   = std::chrono::system_clock::now() + duration,
                    .thread      = std::move(t),
                    .should_loop = loop,
                };
            }
            catch(...)
            {
                Job j = Job();
                return {j, false};
            }

            Job &job = jobs.at(job_name);

            return {job, true};
        }

        std::chrono::time_point<std::chrono::system_clock>
        parse_cron(std::string_view str)
        {
            int section = 0;
            int start = 0;

            std::tm time{};

            for(size_t i = 0; i < str.size(); i++)
            {
                if(str[i] == '*')
                {
                    section++;
                    continue;
                }

                if(isdigit(str[i]))
                {
                    start = i;

                    while(i < str.size() && isdigit(str[i]))
                        i++;

                    section++;

                    std::string_view s = str.substr(start, i-start);

                    int n = std::atoi(s.data());

                    switch(section)
                    {
                        case 1: time.tm_sec = n; break;
                        case 2: time.tm_min = n; break;
                        case 3: time.tm_hour = n; break;
                        case 4: time.tm_mday = n; break;
                        case 5: time.tm_mon  = n-1; break;
                        case 6: time.tm_wday = n; break;
                    }
                }
            }

            tm *temp;
            time_t tt;

            std::time(&tt);

            temp = std::localtime(&tt);

            time.tm_year = temp->tm_year;

            tt = std::mktime(&time);

            return std::chrono::system_clock::from_time_t(tt);
        }
    };
}
