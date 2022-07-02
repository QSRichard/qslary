#include "qslary/net/Poller.h"
#include "Epoller.h"

#include <stdlib.h>

namespace qslary
{
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
//   if (::getenv("MUDUO_USE_POLL"))
//   {
//     return new PollPoller(loop);
//   }
    return new EPollPoller(loop);
}
}

