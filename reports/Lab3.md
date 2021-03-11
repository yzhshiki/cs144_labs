---
typora-copy-images-to: images
---

## Lab 3  the TCP sender

### 1 TCPSender 职能

1. 掌握receiver的接收窗口，处理到来的ackno、window size
2. 通过读取字节流、创造新的TCP segments（可能包括SYN、FIN标志）并发送出去来填充窗口。Sender需要一直发送segments知道窗口已满或者字节流已空。
3. 掌握已发送但没有让receiver收到的segments，称为outstanding segment。
4. 在指定的延迟时间过后，重新发送 3中的segments（即ARQ automatic repeat request）。

### 2 报文段丢失检测

1. SYN和FIN是一个字节流一个。
2. TCPSender的owner会周期性的调用tick方法得到时间。
3. TCPSender检测最早发送的outstanding segment是否已超过制定时间没有被ack（从那之后的都没有被ack），如果是则重传。
4. RTO：retransmission timeout，在TCPSender构造时设定的参数，标志着还有多长时间重传outstanding segment。RTO随时间变化，其initial value不变，并存在一个成员变量_initial_retransmission_timeout中
5. 实现一个重传计时器，可以在特定时间启动，并在RTO用完时终止。
6. 每当发送一个含有数据的报文段时，如果计时器没在运行，则启动计时器。
7. 所有outstanding data都被ack后，停止计时器。
8. 当tick被调用而重传计时器已终止：
   1. 重传最早的未被完全ack的报文段（需要用一定的数据结构存储这些outstanding segments）
   2. 如果窗口非空：
      1. 提高“连续重传”的计数，用于之后的TCPConnetion决定这个连接是否需要重启。
      2. RTO翻倍，即指数退避，防止重传过多。
   3. 重启重传计时器（记得刚刚RTO可能翻倍过）。
9. sender收到一个比之前的ackno都大的新ackno后：
   1. 重置RTO为initial value
   2. 如果sender有outstanding data，则重启重传计时器。
   3. 将“连续重传计数”重置为0。
10. 建议将重传计时器实现为一个单独的类。



### 3 实现TCP sender

4种可能导致发送TCPsegment的情况：

1. void fill_window()

   注意不超过1452字节（因为二型以太网规定了帧的大小，如果超过将要分片）。注意SYN和FIN标志也占用seqno，意味着也占用window的空间。

   如果window大小为0，此方法应该将之视为1，这样sender可以送一个字节给receiver然后被拒绝，同时知道最新的window大小。若不然，sender就无法知道最新window大小。

2. void ack_received( const WrappingInt32 ackno, const uint16_t window_size)

   此时sender知晓了新窗口的左右边界，则检查outstanding segments，并将已被完全读取的segments删除。然后重新填满窗口。

3. void tick( const size_t ms_since_last_tick)

   经过了特定长的时间， 可能需要重传一个outstanding segment

4. void send_empty_segment()

   发送一个报文段，它不占用seqno，不需要被追踪是否outstanding，也不会被重传。

### 5 疑问

1. 为什么指数退避要在window size 非0时进行，因为window size非零时的超时不是由于网络拥塞引起的，所以没必要。