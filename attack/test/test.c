#include <stdio.h>
#include <string.h>

char *code = "1234";

void success(){
    exit(0);
    printf("success\n");
}

void fail(){
    printf("failed\n");
}

void check(){
    char s[4];
    gets(s);
    if(strcmp(code, s) == 0){
        success();
    }else{
        fail();
    }
}

int main(){
    freopen("./tmp", "r", stdin);
    check();
}
