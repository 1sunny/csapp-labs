#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

/**
 * 从字符串中解析数字
 * @param s
 * @return
 * -1 : 格式不正确
 * -2 : int溢出
 */
int Atoi(const char* s){
  if(s == NULL || strcmp(s, "") == 0){
    return -1;
  }
  int64_t res = 0, f = (s[0] == '-');
  for(int i = f; s[i]; i++){
    if(isdigit(s[i])){
      res = res * 10 + (s[i] - '0');
      if((f == 0 && res > INT_MAX) || (f == 1 && -res < INT_MIN)){
        return -2;
      }
    }else{
      return -1;
    }
  }
  return f ? -res : res;
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg) {
  fprintf(stdout, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg) {
  fprintf(stdout, "%s\n", msg);
  exit(1);
}
