## Lab 4 TCP in full

### 1 The TCP connection

- 接收报文段（segment_received）
  - 如果RST标志已设置，将inbound和outbound流都设为error状态并永远关闭连接。
  - 将报文段传递给TCPReceiver
  - 如果ACK标志已设置，则将ackno和window size告诉TCPSender
  - 只要到来的报文段含有seqno，TCPConnection将保证至少回应一个报文段（来更新己方的ackno和window size）
- 发送报文段
  - TCPSender在任意时刻将报文段放进发送队列时，要将seqno、SYN、payload、FIN设置好
  - 发送报文段前，TCPConnection向TCPReceiver询问ackno、windowsize，如果ackno存在，则需要将ACK标志设置。
- 时间流逝（TCPConnection的tick方法）
  - 告诉TCPSender
  - 如果TCPSender的连续重传次数大于指定次数，则发送一个RST标志已设置的空报文段，来表示中止连接。
  - 必要时干净的结束连接。



### 2 FAQs

- TCPConnection::inbound stream() 读取inbound stream
- TCPConnection收到一个RST报文段后，应该在inbound和outbound字节流都设置error标志，并让active()返回false
- 发送RST报文段的时机：
  - sender的连续重传次数过多
  - TCPConnection的析构函数被调用且连接仍然为active
- ackno一直存在吗？ACK标志的目的是什么
  - 没有ackno的里外在于connection最开始时，receiver没有收到任何东西。
  - 在发送端，收到的每个报文段都有ACK和ackno
  - 在接收端，只有ACK标志为1的报文段ackno有效。