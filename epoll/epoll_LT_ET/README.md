此demo更清晰的演示epoll的LT和ET两种触发模式：

即：

EPOLL事件有两种模型：

Edge Triggered (ET) 边缘触发只有数据到来才触发，不管缓存区中是否还有数据。

Level Triggered (LT) 水平触发只要有数据都会触发。

- 水平触发LT：
  - 缓存区剩余未读尽的数据会导致 epoll_wait 返回
- 边缘触发ET：
  - 缓存区剩余未读尽的数据不会导致 epoll_wait 返回，新的事件满足，才会触发。
  - event.events = EPOLLIN|EPOLLET ;

基于阻塞的ET模型Demo：epoll_server1.c，client1.c

基于非阻塞的ET模型Demo：epoll_server2.c，client2.c