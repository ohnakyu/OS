#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "queue.h"

using namespace std;

#define REQUEST_PER_CLIENT 10000

atomic<int> sum_key = 0;
atomic<int> sum_value = 0;

typedef enum {
    GET,
    SET,
    GETRANGE
} Operation;

typedef struct {
    Operation op;
    Item item;
} Request;

void client_func(Queue* queue, Request requests[], int n_request) {
    Reply reply = { false, 0 };

    for (int i = 0; i < n_request; i++) {
        if (requests[i].op == GET) {
            reply = dequeue(queue);
        }
        else { // SET
            reply = enqueue(queue, requests[i].item);
        }

        if (reply.success) {
            sum_key += reply.item.key;
            sum_value += (int)(intptr_t)reply.item.value;
        }
    }
}

int main(void) {
    srand((unsigned int)time(NULL));

    Request requests[REQUEST_PER_CLIENT];
    for (int i = 0; i < REQUEST_PER_CLIENT / 2; i++) {
        requests[i].op = SET;
        requests[i].item.key = i;
        requests[i].item.value = (void*)(intptr_t)(rand() % 1000000);
    }
    for (int i = REQUEST_PER_CLIENT / 2; i < REQUEST_PER_CLIENT; i++) {
        requests[i].op = GET;
        requests[i].item.key = 0;
        requests[i].item.value = nullptr;
    }

    Queue* queue = init();
    if (!queue) {
        cerr << "큐 초기화 실패" << endl;
        return -1;
    }

    auto start_time = chrono::high_resolution_clock::now();

    thread client(client_func, queue, requests, REQUEST_PER_CLIENT);
    client.join();

    auto end_time = chrono::high_resolution_clock::now();

    release(queue);

    chrono::duration<double> elapsed = end_time - start_time;
    double seconds = elapsed.count();
    double throughput = REQUEST_PER_CLIENT / seconds;

    cout << "반환된 키 값 합계 = " << sum_key << endl;
    cout << "반환된 값 합계 = " << sum_value << endl;
    cout << "총 처리 요청 수 = " << REQUEST_PER_CLIENT << endl;
    cout << "경과 시간(초) = " << seconds << endl;
    cout << "평균 처리율(요청/초) = " << throughput << endl;

    return 0;
}
