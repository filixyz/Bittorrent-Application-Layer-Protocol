#include <thread>
#include <vector>
#include <atomic>
#include <functional>

class simple_thread_pool
{
  std::vector<std::thread> threads;
  std::thread_safe_queue<std::function<void()>> pool;
  std::atomic<bool> done;
  thread_joiner<std::thread> joiner;

  void worker_thread() {
    while(!done) {
      std::function<void()> task;
      if(pool.try_pop(task))
        task();
      else
        std::this_thread::yield();
    }
  }
public:
  simple_thread_pool(): done(false), joiner(threads)
  {
    const unsigned count=std::thread::hardware_concurrency();

    try {
      for(auto i=0; i!=count; ++i)
        threads.push_back(
            std::thread(&simple_thread_pool::worker_thread, this));
    }
    catch(...)
    {
      done=true;
      throw;
    }
  }
  ~simple_thread_pool()
  {
    done=true; 
  }

  template<typename FunctionType>
  void submit(FunctionType f)
  {
    pool.push(std::function<void()>(f));
  }
};
