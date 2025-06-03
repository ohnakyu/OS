#ifndef QTYPE_H
#define QTYPE_H

#include <mutex>

typedef int Key;
typedef int Value;

typedef struct {
    Key key;
    Value value;
} Item;

typedef struct node_t {
    Item item;
    struct node_t* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    std::mutex mtx;  // 스레드 동기화를 위한 mutex
} Queue;

typedef struct {
    bool success;  // true: 성공, false: 실패
    Item item;
} Reply;

#endif // QTYPE_H
