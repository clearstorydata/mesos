#include <process/id.hpp>
#include <process/latch.hpp>
#include <process/process.hpp>

#include <stout/duration.hpp>

namespace process {

// TODO(benh): Provide an "optimized" implementation of a latch that
// is libprocess aware. That is, allow integrate "waiting" on a latch
// within libprocess such that it doesn't cost a memory allocation, a
// spawn, a message send, a wait, and two user-space context-switchs.

Latch::Latch()
{
  triggered = false;

  // Deadlock is possible if one thread is trying to delete a latch
  // but the libprocess thread(s) is trying to acquire a resource the
  // deleting thread is holding. Hence, we only save the PID for
  // triggering the latch and let the GC actually do the deleting
  // (thus no waiting is necessary, and deadlocks are avoided).
  pid = spawn(new ProcessBase(ID::generate("__latch__")), true);
}


Latch::~Latch()
{
  terminate(pid);
}


void Latch::trigger()
{
  if (!triggered) {
    terminate(pid);
    triggered = true;
  }
}


bool Latch::await(const Duration& duration)
{
  if (!triggered) {
    return process::wait(pid, duration); // Explict to disambiguate.
  }

  return true;
}

} // namespace process {
