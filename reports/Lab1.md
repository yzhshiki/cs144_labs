## Lab 1 stitching substrings into a byte stream

### 1 Getting start

由于后面要用的代码在lab1-startercode下，若并不是git clone官方仓库，则需要新增一个远程仓库

```
git remote add raw https://github.com/CS144/sponge.git
```

将新仓库数据抓取到本仓库

```
git fetch raw
...
From https://github.com/CS144/sponge
 * [new branch]      lab1-startercode -> raw/lab1-startercode
 * [new branch]      lab2-startercode -> raw/lab2-startercode
 * [new branch]      lab3-startercode -> raw/lab3-startercode
 * [new branch]      lab4-startercode -> raw/lab4-startercode
 * [new branch]      lab5-startercode -> raw/lab5-startercode
 * [new branch]      lab6-startercode -> raw/lab6-startercode
 * [new branch]      lab7-startercode -> raw/lab7-startercode
 * [new branch]      master           -> raw/master
```

利用新仓库的分支，在本地新建分支，并将main分支合并过来

```
git checkout -b lab1-startcode raw/lab1-startercode
git merge main
```

在新的分支下git push，即可在origin仓库创建对应分支

```
git checkout lab1-startcode
git commit -m "preparing for lab1"
git push origin lab1-startcode
git branch -r
    origin/HEAD -> origin/main
    origin/lab1-startcode
    origin/main
    raw/lab1-startercode
    raw/lab2-startercode
    raw/lab3-startercode
    raw/lab4-startercode
    raw/lab5-startercode
    raw/lab6-startercode
    raw/lab7-startercode
    raw/master
```



### 2 实验过程

实验目标：实现一个streamReassembler类，将杂乱的数据段正确地传入字节流（Lab0 实现）

主要流程有：容量判断，处理字串的冗余前缀，以字节为单位写入缓冲区，以字节为单位写入字节流，EOF判断

```c++
/* stream_reassembler.hh */
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity, nextIndex, bytesUnAssembled;    //!< The maximum number of bytes
    bool eof_flag;
    std::map<size_t,std::string> myMap;
    /* other */
}
```

```c++
/* stream_reassembler.cc */
#include "stream_reassembler.hh"
#include <iostream>

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), nextIndex(0), bytesUnAssembled(0), eof_flag(false), myMap() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //如果是太久之后的消息,则丢弃
    if(index > nextIndex && index - nextIndex >= _capacity)   return;
    //如果完全被写入过,则丢弃
    if(index + data.size() < nextIndex) return;
    //eof标志到来时，消息的有用部分必须全部都能存进缓冲区，这样才将达成条件。
    if(eof && data.size() - nextIndex + index <= _capacity)
        eof_flag = true;
    //数据长度、字节流的剩余空间的较小值，用于index>=nextindex时的读取
    auto maxLen = min(data.size(), stream_out().remaining_capacity());
    //可以直接递归把不需要的部分忽略掉
    if(index < nextIndex){
        if(index + data.size() > nextIndex)
            return push_substring(data.substr(nextIndex - index), nextIndex, eof);
        else
            return;
    }

    //来的数据流序号大于所需，将其放进缓冲区。
    if(index > nextIndex){
        for (size_t i=0;i<maxLen;i++) {
            if (myMap.count(index+i)!=0) {
                continue;
            }
            myMap[index+i] = data.substr(i,1);
            bytesUnAssembled++;
        }
        return;
    }

    //直接写入字节流
    if(index == nextIndex){
        for(size_t i = 0; i < maxLen; i ++){
            stream_out().write(data.substr(i, 1));
            nextIndex ++;
            if(myMap.count(index + i) != 0){
                myMap.erase(index + i);
                bytesUnAssembled --;
            }
        }
    }
    //在index==nextIndex的情况下，可能写完data后，缓冲区里的可以写进字节流，所以进行检测
    while(myMap.count(nextIndex)!=0){
        if(stream_out().remaining_capacity() > 0){
            stream_out().write(myMap[nextIndex]);
            myMap.erase(nextIndex);
            nextIndex ++;
            bytesUnAssembled --;
        }
        //字节流已满，则跳出等下一次读数据
        else
            break;
    }
    //eof标志已到来+到来时的那个数据末位没有被丢弃+缓冲区已全部输出到字节流，则告诉字节流eof
    if(eof_flag && empty())
        stream_out().end_input();
    return;
}

size_t StreamReassembler::unassembled_bytes() const { return bytesUnAssembled; }

bool StreamReassembler::empty() const { return bytesUnAssembled == 0; }

```

