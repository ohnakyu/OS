#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include "queue.h"

using namespace std;

#define REQUEST_PER_CLIENT 10000
#define NUM_CLIENTS 32

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

    // 총 요청 개수는 NUM_CLIENTS * REQUEST_PER_CLIENT
    const int total_requests = NUM_CLIENTS * REQUEST_PER_CLIENT;
    Request* all_requests = new Request[total_requests];

    // 요청 배열 채우기: 클라이언트마다 절반은 SET, 절반은 GET
    for (int c = 0; c < NUM_CLIENTS; c++) {
        int offset = c * REQUEST_PER_CLIENT;
        for (int i = 0; i < REQUEST_PER_CLIENT / 2; i++) {
            all_requests[offset + i].op = SET;
            all_requests[offset + i].item.key = i;
            all_requests[offset + i].item.value = (void*)(intptr_t)(rand() % 1000000);
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
        delete[] all_requests;
        return -1;
    }

    auto start_time = chrono::high_resolution_clock::now();

    // 32개 스레드 생성
    vector<thread> clients;
    for (int c = 0; c < NUM_CLIENTS; c++) {
        clients.emplace_back(client_func, queue, &all_requests[c * REQUEST_PER_CLIENT], REQUEST_PER_CLIENT);
    }

    // 모든 스레드 종료 대기
    for (auto& t : clients) {
        t.join();
    }

    auto end_time = chrono::high_resolution_clock::now();

    release(queue);
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
