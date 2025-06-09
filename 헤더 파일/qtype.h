#ifndef _QTYPE_H
#define _QTYPE_H

#include <atomic>
#include <mutex>

typedef unsigned int Key;   // 값이 클수록 높은 우선순위
typedef void* Value;

// Item 구조체: 깊은 복사를 위한 value_size 필드 포함
typedef struct {
    Key key;
    Value value;
    int value_size;  // 바이트 단위의 value 크기
} Item;

// 연산 결과를 담는 구조체
typedef struct {
    bool success;  // true: 성공, false: 실패
    Item item;
} Reply;

// (Linked list 관련 함수는 사용하지 않지만 정의는 유지)
typedef struct node_t {
    Item item;
    struct node_t* next;
} Node;

// Queue 구조체는 queue.cpp에서 reinterpret_cast<QueueImpl*>로 구현됨
typedef struct Queue Queue;

#endif
