#include "stream_reassembler.hh"
#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

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
    if(index < nextIndex){
        if(index + data.size() > nextIndex){
            for (size_t i = nextIndex - index;i<data.size();i++) {  //这里不能用maxLen
                if (myMap.count(index+i)!=0) {
                    continue;
                }
                myMap[index+i] = data.substr(i,1);  //将data一个字符一个字符的存进缓冲区
                bytesUnAssembled++;
//                cout<<"bytesUnAssembled: "<<bytesUnAssembled<<endl;
            }

        }
        else
            return;
    }

//    cout<<"executing index: "<<index<<endl;
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
    //

    if(index == nextIndex){
//        auto input_size = min(data.size(), stream_out().remaining_capacity());
        for(size_t i = 0; i < maxLen; i ++){
            stream_out().write(data.substr(i, 1));
//            cout<<"writed letter: "<<data.substr(i, 1)<<endl;
            nextIndex ++;
            if(myMap.count(index + i) != 0){
                myMap.erase(index + i);
                bytesUnAssembled --;
//                cout<<"bytesUnAssembled: "<<bytesUnAssembled<<endl;
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

    if(eof_flag && empty())
        stream_out().end_input();
    return;
}

size_t StreamReassembler::unassembled_bytes() const { return bytesUnAssembled; }

bool StreamReassembler::empty() const { return bytesUnAssembled == 0; }
