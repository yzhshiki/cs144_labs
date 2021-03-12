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

   ```c++
   /* tcp_sender.hh */
   class RETimer{
     private:
       //retransmission timeout
       unsigned int rto;
       unsigned int _time;
       bool _isrunning;
   
     public:
       RETimer(unsigned int _initial_retrans_timeout);
       bool isRunning();
       void passTime(unsigned int passedTime);
       void doubleRTO();
       void resetTime();
       void resetRTO(unsigned int _initial_retrans_timeout);
       void start();
   };
   
   class TCPSender {
     private:
       //! our initial sequence number, the number for our SYN.
       WrappingInt32 _isn;
   
       //! outbound queue of segments that the TCPSender wants sent
       std::queue<TCPSegment> _segments_out{};
   
       //! retransmission timer for the connection
       unsigned int _initial_retransmission_timeout;
   
       //! outgoing stream of bytes that have not yet been sent
       ByteStream _stream;
   
       //! the (absolute) sequence number for the next byte to be sent
       uint64_t _next_seqno{0};
   
       //存储已发送但未被ack的segments
       std::queue<TCPSegment> outSegments{};
   
       uint64_t _bytes_in_flight;
       WrappingInt32 _ackGot;
       uint16_t _true_window_size;
       uint16_t _consecutive_retransmission;
       bool _fin_flag;
       RETimer timer;
     public:
   	void sendSegment(TCPSegment &segment);
       ...
   }
   ```

   ```c++
   /* tcp_sender.cc */
   CPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
       : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
       , _initial_retransmission_timeout{retx_timeout}
       , _stream(capacity)
       , _bytes_in_flight(0)
       ,_ackGot(_isn.raw_value())
       ,_true_window_size(0)
       ,_consecutive_retransmission(0)
       ,_fin_flag(false)
       ,timer(retx_timeout){}
   
   uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }
   
   void TCPSender::fill_window() {
       if(_fin_flag)
           return;
       //用于区分窗口为1、窗口为0视作1的情况
       bool zero_flag = false;
       //字节流中下一个字节的seqno
       WrappingInt32 _next_seq32 = wrap(_next_seqno, _isn);
       //字节流中下一个字节的absolute seqno为0 说明还没有握手
       if(_next_seqno == 0){
           TCPSegment segment;
           segment.header().seqno = _next_seq32;
           segment.header().syn = true;
           sendSegment(segment);
           return;
       }
       if(_true_window_size == 0){
           zero_flag = true;
           _true_window_size = 1;
       }
   
       size_t remain;
       //判断窗口剩余空间，是否还能发送
       while((remain = _true_window_size - (_next_seq32.raw_value() - _ackGot.raw_value()))!=0 && !_fin_flag){
           TCPSegment segment;
           segment.header().seqno = _next_seq32;
           //注意Buffer的构造函数，只允许传右值（即常数等不能放在等式左边，有地址的值）。
           size_t dataLen = min(size_t(remain), TCPConfig::MAX_PAYLOAD_SIZE);
           segment.payload() = _stream.read(dataLen);
           //字节流已结束，且有空间留给FIN标志
           if(_stream.eof() && remain >= segment.length_in_sequence_space() + 1){
               segment.header().fin = true;
               _fin_flag = true;
           }
   
           //字节流是空的，但没有eof(这是由使用者指定的，并不是空了就结束了)。
           if(segment.length_in_sequence_space()==0){
               return;
           }
           sendSegment(segment);
           //更新seqno用于循环判断
           _next_seq32 = wrap(_next_seqno, _isn);
       }
       if(zero_flag){
           _true_window_size = 0;
       }
   
   }
   
   //! \param ackno The remote receiver's ackno (acknowledgment number)
   //! \param window_size The remote receiver's advertised window size
   void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
   
       //将已发送报文段中，完全被ack的那些丢弃。
       while(!outSegments.empty()){
           TCPSegment segment = outSegments.front();
           if(segment.header().seqno.raw_value() + segment.length_in_sequence_space() <= ackno.raw_value()){
               _bytes_in_flight -= segment.length_in_sequence_space();
               outSegments.pop();
           }
           else
               break;
       }
       if(ackno.raw_value() > wrap(_next_seqno, _isn).raw_value())
           return;
       //收到一个比之前的ackno都大的新ackno后
       if(ackno.raw_value() > _ackGot.raw_value()) {
           _ackGot = ackno;
           timer.resetTime();
           timer.resetRTO(_initial_retransmission_timeout);
           if (!outSegments.empty())
               timer.start();
           _consecutive_retransmission = 0;
       }
       _true_window_size = window_size;
       fill_window();
   }
   
   //! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
   void TCPSender::tick(const size_t ms_since_last_tick) {
       timer.passTime(ms_since_last_tick);
       if(!timer.isRunning() && !outSegments.empty()){
           TCPSegment segment = outSegments.front();
           _segments_out.push(segment);
           if(_true_window_size!=0 || _next_seqno == 1){
               _consecutive_retransmission++;
               timer.doubleRTO();
           }
           timer.start();
       }
   }
   
   unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }
   
   void TCPSender::send_empty_segment() {}
   
   void TCPSender::sendSegment(TCPSegment &segment) {
       _segments_out.push(segment);
       outSegments.push(segment);
       _next_seqno += segment.length_in_sequence_space();
       _bytes_in_flight += segment.length_in_sequence_space();
       if(!timer.isRunning()){
           timer.resetTime();
           timer.start();
       }
   }
   
   RETimer::RETimer(unsigned int _initial_retrans_timeout) :
       rto(_initial_retrans_timeout)
       ,_time(0)
       ,_isrunning(true){}
   bool RETimer::isRunning() { return _isrunning; }
   void RETimer::passTime(unsigned int passedTime) {
       if(!isRunning())
           return;
       _time += passedTime;
       if(_time >= rto){
           _isrunning = false;
           _time = 0;
       }
   }
   void RETimer::doubleRTO() {
       rto *= 2;
   }
   void RETimer::resetTime(){
       _time = 0;
   }
   void RETimer::resetRTO(unsigned int _initial_retrans_timeout) {
       rto = _initial_retrans_timeout;
   }
   void RETimer::start() { _isrunning = true;}
   ```


### 5 疑问

1. 为什么指数退避要在window size 非0时进行，因为window size非零时的超时不是由于网络拥塞引起的，所以没必要。