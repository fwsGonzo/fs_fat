#ifndef UTIL_ASYNC_LOOP_HPP
#define UTIL_ASYNC_LOOP_HPP

typedef std::function<void(bool)> next_func_t;

inline void
async_loop(
  std::function<void(next_func_t)> func,
  std::function<void()> on_done)
{
  std::function<void()> next;
  
  // loop:
  next = 
  [next] (bool done)
  {
    // check we are done, and if so,
    // execute the callback function and return
    if (done)
    {
      on_done();
      return;
    }
    // otherwise,
    // execute one iteration of the loop
    func(next);
  }
  // start the process
  next(false);
}

#endif
