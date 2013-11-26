#ifndef NUMBER_H
#define NUMBER_H

extern void convertNumber(unsigned char *buf, int number); // 把数字写入缓存
extern int getNumber(unsigned char *buf); // 在缓冲读取字节数，就是这个缓存可能会有的字节数

#endif
