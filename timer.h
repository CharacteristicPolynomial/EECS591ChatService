#pragma once
#include <chrono>
#include "configure.h"
using namespace std;

class Timer {
    public:
        void init(chrono::nanoseconds p) {
            period = p;
            last_time = chrono::system_clock::now().time_since_epoch();
        }
        bool coolDown() {
            chrono::nanoseconds ct = chrono::system_clock::now().time_since_epoch();
            if(ct-last_time < period) {
                return false;
            } else {
                last_time = ct;
                return true;
            }
        }
        void use() {
            last_time = chrono::system_clock::now().time_since_epoch();
        }
    private:
        chrono::nanoseconds period;
        chrono::nanoseconds last_time;
};

class RoundRobin {
    public:
        void next_round() {
            cap += seg;
            counter = 0;
        }
        bool filter(int length) {
            int temp = cap % length;
            counter ++;
            counter = counter % length;
            if(counter >= temp && counter < temp + seg) {
                return true;
            }
            if(counter <= temp && counter+length < temp+seg) {
                return true;
            }
            return false;
        }
    private:
        int counter = 0;
        int cap = 0;
        int seg = ROUNDROBIN;
};