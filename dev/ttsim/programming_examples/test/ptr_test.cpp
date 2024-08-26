#include <iostream>
#include <thread>
using namespace std;

struct Test {
    int *a;
    int *b;
    int i;
};

void thread_func(Test *t) {
    (*(t->a))++;
    (*(t->b))++;
    printf("Thread %d: a = %d, b = %d\n", t->i, *(t->a), *(t->b));
}

int main() {
    Test t;
    t.a = new int(10);
    t.b = new int(20);
    Test this_t[2];

    for (int i = 0; i < 2; i++) {
        this_t[i] = t;   // Share the same a and b
        this_t[i].i = i; // Different i for each
        thread thread_obj(thread_func, &this_t[i]);
        thread_obj.join(); // Wait for the thread to finish
    }

    delete t.a;
    delete t.b;
    return 0;
}
