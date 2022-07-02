#ifndef __QSLARY_EPOLL_H_
#define __QSLARY_EPOLL_H_

#include "qslary/net/Channel.h"
#include "qslary/net/EventLoop.h"
#include "qslary/net/Poller.h"

#include <vector>
#include <memory>

struct epoll_event;

namespace qslary
{

class EPollPoller : public Poller
{
public:
  EPollPoller(EventLoop* loop);
  ~EPollPoller() override;

  Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
  void updateChannel(Channel* channel) override;
  void removeChannel(Channel* channel) override;

private:
  static const int kInitEventListSize = 16;

  static const char* operationToString(int op);

  void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;

  int epollfd_;
  EventList events_;
};


}


#endif
