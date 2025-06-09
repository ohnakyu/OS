#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "queue.h"

using namespace std;

#define REQUEST_PER_CLIENT 10000
#define NUM_CLIENTS 5

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
    Reply reply = { false, {0, nullptr} };

    for (int i = 0; i < n_request; i++) {
        if (requests[i].op == GET) {
            reply = dequeue(queue);
        }
        else {
            reply = enqueue(queue, requests[i].item);
        }

        if (reply.success && reply.item.value) {
            sum_key += reply.item.key;
            sum_value += *(int*)(reply.item.value);
        }
    }
}

int main(void) {
    srand((unsigned int)time(NULL));

    const int total_requests = NUM_CLIENTS * REQUEST_PER_CLIENT;
    Request* all_requests = new Request[total_requests];

    for (int c = 0; c < NUM_CLIENTS; c++) {
        int offset = c * REQUEST_PER_CLIENT;
        for (int i = 0; i < REQUEST_PER_CLIENT / 2; i++) {
            all_requests[offset + i].op = SET;
            all_requests[offset + i].item.key = i;

            int* val = new int(rand() % 1000000);
            all_requests[offset + i].item.value = val;
        }
        for (int i = REQUEST_PER_CLIENT / 2; i < REQUEST_PER_CLIENT; i++) {
            all_requests[offset + i].op = GET;
            all_requests[offset + i].item.key = 0;
            all_requests[offset + i].item.value = nullptr;
        }
    }

    Queue* queue = init();
    if (!queue) {
        cerr << "큐 초기화 실패" << endl;
        for (int i = 0; i < total_requests / 2; ++i) {
            delete (int*)all_requests[i].item.value;
        }
        delete[] all_requests;
        return -1;
    }

    auto start_time = chrono::high_resolution_clock::now();

    std::thread clients[NUM_CLIENTS];  // ✅ 벡터 대신 배열 사용
    for (int c = 0; c < NUM_CLIENTS; c++) {
        clients[c] = std::thread(client_func, queue, &all_requests[c * REQUEST_PER_CLIENT], REQUEST_PER_CLIENT);
    }
    for (int c = 0; c < NUM_CLIENTS; c++) {
        clients[c].join();
    }

    auto end_time = chrono::high_resolution_clock::now();

    release(queue);

    for (int i = 0; i < total_requests / 2; ++i) {
        delete (int*)all_requests[i].item.value;
    }
    delete[] all_requests;

    chrono::duration<double> elapsed = end_time - start_time;
    double seconds = elapsed.count();
    double throughput = total_requests / seconds;

    cout << "반환된 키 값 합계 = " << sum_key << endl;
    cout << "반환된 값 합계 = " << sum_value << endl;
    cout << "총 처리 요청 수 = " << total_requests << endl;
    cout << "경과 시간(초) = " << seconds << endl;
    cout << "평균 처리율(요청/초) = " << throughput << endl;

    return 0;
}
