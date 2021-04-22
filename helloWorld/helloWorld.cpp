//
// Created by Edipo on 21/04/2021.
//

#include <cstdio>
#include "pico/stdlib.h"

[[noreturn]] int main(){
    stdio_init_all(); // enable USB serial
    while (true){
        printf("Hello World!\n");
        sleep_ms(1000);
    }
}