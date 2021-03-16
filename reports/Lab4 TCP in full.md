## Lab 4 TCP in full

### 1 The TCP connection

- 接收报文段（segment_received）
  - 如果RST标志已设置，将inbound和outbound流都设为error状态并永远关闭连接。
  - 将报文段传递给TCPReceiver
  - 如果ACK标志已设置，则将ackno和window size告诉TCPSender
  - 只要到来的报文段含有seqno，TCPConnection将保证至少回应一个报文段（来更新己方的ackno和window size）**其实不一定，例外出现在四次挥手的一种顺序下：发送FIN、接收FIN、发送ACK、接收ACK**
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



### 3 The end of a TCP connection

连接结束的两种方式，第一种unclean shutdown，连接发送或者收到一个带有RST标志的报文段。这时outbound和inbound字节流都应该设为error状态，并且若调用active函数会得到false

第二种clean shutdown是没有error的结束，两个字节流都可靠的完全传输完了。这种结束方式有4个前提

1. inbound字节流已经完全assembled并ended。
2. outbound字节流被本地应用ended并且已经完全发送完。
3. outbound字节流已完全被另一端ack
4. 己方连接确信另一端满足前提3，有两种情况：
   1. 10倍的初始重传延时间没有收到任何报文段（这段时间称连接处于lingering）
   2. 另一端来结束这个连接

即若连接的inbound字节流在连接没发送过FIN报文段前就结束了，那么在两个字节流都结束后，连接不需要进行linger。

从实践上来看，即linger after streams finish变量在“inbound结束而outbound未eof”时要设置成false。而此变量为false时，前提123都满足则连接已结束，否则需要linger后结束。