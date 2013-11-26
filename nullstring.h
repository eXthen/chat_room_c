#ifndef NULLSTRING_H
#define NULLSTRING_H

void nullStrcat(unsigned char *to, unsigned char *from, int offset);
int nullStrlen(unsigned char *str, int offset); 
void nullInsertType(unsigned char *str, char c); // 为缓存头部添加标志信息
void nullStrcpyFrom(unsigned char *to, unsigned char *from, int offset);
void nullStrcpyTo(unsigned char *to, unsigned char *from, int offset);
void nullStrcpy(unsigned char *to, unsigned char *from);
  
#endif
