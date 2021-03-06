## Lab 1  the TCP receiver

### 1 实验注意事项

1. 在StreamReassembler中每个字节有一个64位的index，在TCP首部由于空间问题，每个字节只能用32位的sequence number (seqno) 表示。
2. 为了提高安全性、避免同样的两端被以前的通信中的segments干扰，一段数据流的seqno并不是从零开始的，而是从一个随机的32位数(ISN)开始，往后则(ISN+1)%(2^32)
3. SYN和FIN标志并不属于数据流，但也占有sequence number

### 2 实验过程

