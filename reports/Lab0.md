## Lab 0 networking warmup

### 1 Setting up

[course guide](https://web.stanford.edu/class/cs144/vm_howto/)

1. win10下使用windows teminal + wsl2 + vscode remote wsl， 即guide中第二种方式。[参考链接](https://blog.csdn.net/u011026329/article/details/104447767)

   （提示：执行setup_dev_env.sh提示 command not found，chmod a + x解决，原因是.sh文件原不是可执行文件。

2. 在wsl内clone项目框架，并按照链接readme.md检查项目。

```
git clone https://github.com/CS144/sponge.git
```

### 2 Networking by hand

#### 2.1 Fetch a Web page

![1611800822803](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\1611800822803.png)

输入GET行与Host行间隔要短，Host输完后Enter两次

![1611800757614](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\1611800757614.png)



### 3 Writing a network program using an OS stream socket

程序规范：

>  Use the language documentation at https://en.cppreference.com as a resource.
> • Never use malloc() or free().
> • Never use new or delete.
> • Essentially never use raw pointers (*), and use “smart” pointers (unique ptr or
> shared ptr) only when necessary. (You will not need to use these in CS144.)
> • Avoid templates, threads, locks, and virtual functions. (You will not need to use these
> in CS144.)
> • Avoid C-style strings (char *str) or string functions (strlen(), strcpy()). These
> are pretty error-prone. Use a std::string instead.
> • Never use C-style casts (e.g., (FILE *)x). Use a C++ static cast if you have to (you
> generally will not need this in CS144).
> • Prefer passing function arguments by const reference (e.g.: const Address & address).
> • Make every variable const unless it needs to be mutated.
> • Make every method const unless it needs to mutate the object.
> • Avoid global variables, and give every variable the smallest scope possible.
> • Before handing in an assignment, please run make format to normalize the coding
> style



#### 3.4 Writing webget

1. 服务器端用 bind() 函数将套接字与特定的IP地址和端口绑定，客户端用 connect() 函数建立连接。

2. shutdown不会关闭fd，close会关闭fd。

3. fd被多进程持有或被复制时，不会立刻关闭连接（而是引用计数-1）；shutdown会直接关闭连接（发送fin），无论是否又其他打开的文件描述符指向套接字。
4. shutdown有利于TCP结束时四次挥手的正常进行。[为什么TCP网络编程中close前shutdown？](
   https://www.zhihu.com/question/437091851/answer/1652250018)

```c++
void get_URL(const string &host, const string &path) {
    TCPSocket sock;
    sock.connect(Address(host, "http"));    //客户端用connect建立连接
    sock.write("GET "+path+" HTTP/1.1\r\nHOST: "+host+"\r\n\r\n");
    sock.shutdown(SHUT_WR);		//拒绝本端发送
    while(!sock.eof()){
        auto recvd = sock.read();
        cout<<recvd;
    }
    sock.close();
    return;
}
```



### 4 An in-memory reliable byte stream

```c++
class ByteStream {
  private:
    // Your code here -- add private members as necessary.
    std::string _buf;
    size_t _capacity;
    size_t _write_count;
    size_t _read_count;
    bool _is_eof;
    ...
}
```

```c++
// 需要使用初始化列表来初始化
ByteStream::ByteStream(const size_t capacity) :
    _buf({}),
    _capacity(capacity),
    _write_count(0),
    _read_count(0),
    _is_eof(false)
{}

size_t ByteStream::write(const string &data) {
    size_t len = data.size();
    if(len > _capacity - _buf.size()){
        len = _capacity - _buf.size();
    }
    _buf += data.substr(0, len);
    _write_count += len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return _buf.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
// 需要在这一步增加read_count
void ByteStream::pop_output(const size_t len) {
    _read_count += len;
    _buf.erase(0, len);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::size_t read_len = len > _buf.size()? _buf.size() : len;
    string ans = peek_output(read_len);
    pop_output(read_len);
    return ans;
}

void ByteStream::end_input() { _is_eof = 1; }

bool ByteStream::input_ended() const { return _is_eof; }

size_t ByteStream::buffer_size() const { return _buf.size(); }

bool ByteStream::buffer_empty() const { return _buf.size() == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _write_count; }

size_t ByteStream::bytes_read() const { return _read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buf.size(); }
```