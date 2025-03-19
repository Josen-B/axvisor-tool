#ifndef axvisor_EVENT_H
#define axvisor_EVENT_H
#include <sys/epoll.h>

struct axvisor_event {
    void (*handler)(int, int, void *);
    void *param;
    int fd;
    int epoll_type;
};

int initialize_event_monitor(void);
void destroy_event_monitor();
struct axvisor_event *add_event(int fd, int epoll_type,
                               void (*handler)(int, int, void *), void *param);
#endif // axvisor_EVENT_H
