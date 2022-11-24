#include <stdio.h>
#include <string.h>

char *code = "1234";

void success(){
    printf("success\n");
    exit(0);
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
