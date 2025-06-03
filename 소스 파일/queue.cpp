#include <iostream>
#include <mutex>
#include <atomic>
#include "queue.h"

#define MAX_CAPACITY 100000

// 내부 구현 구조체 (Queue*가 가리킬 실제 구조)
struct QueueImpl {
    Item* heap;
    int capacity;
    std::atomic<int> size;
    std::mutex enqueue_mtx;
    std::mutex dequeue_mtx;
};

// Queue* → QueueImpl* 캐스팅 헬퍼
static QueueImpl* to_impl(Queue* q) {
    return reinterpret_cast<QueueImpl*>(q);
}

// 힙 인덱스 계산
static int parent(int i) { return (i - 1) / 2; }
static int left(int i) { return 2 * i + 1; }
static int right(int i) { return 2 * i + 2; }

// 아이템 스왑 함수는 더 이상 사용하지 않음

// 위쪽으로 힙 정렬 (임시 변수로 최소 복사)
static void heapify_up(QueueImpl* pq, int idx) {
    Item tmp = pq->heap[idx];
    while (idx > 0) {
        int p = parent(idx);
        if (pq->heap[p].key >= tmp.key) break;
        pq->heap[idx] = pq->heap[p];
        idx = p;
    }
    pq->heap[idx] = tmp;
}

// 아래쪽으로 힙 정렬 (임시 변수로 최소 복사)
static void heapify_down(QueueImpl* pq, int idx) {
    Item tmp = pq->heap[idx];
    int sz = pq->size.load();
    while (true) {
        int l = left(idx);
        int r = right(idx);
        int largest = idx;

        if (l < sz && pq->heap[l].key > tmp.key) largest = l;
        if (r < sz && pq->heap[r].key > pq->heap[largest].key) largest = r;
        if (largest == idx) break;

        pq->heap[idx] = pq->heap[largest];
        idx = largest;
    }
    pq->heap[idx] = tmp;
}

Queue* init(void) {
    QueueImpl* pq = new QueueImpl;
    pq->capacity = MAX_CAPACITY;
    pq->size = 0;
    pq->heap = new Item[pq->capacity];
    return reinterpret_cast<Queue*>(pq);
}

void release(Queue* queue) {
    if (!queue) return;
    QueueImpl* pq = to_impl(queue);
    delete[] pq->heap;
    delete pq;
}

Node* nalloc(Item item) {
    // 배열 기반이라 사용 안함
    return nullptr;
}

void nfree(Node* node) {
    // 배열 기반이라 사용 안함
}

Node* nclone(Node* node) {
    // 배열 기반이라 사용 안함
    return nullptr;
}

Reply enqueue(Queue* queue, Item item) {
    Reply reply = { false, {0, nullptr} };
    if (!queue) return reply;
    QueueImpl* pq = to_impl(queue);

    std::lock_guard<std::mutex> lock(pq->enqueue_mtx);
    int sz = pq->size.load();
    if (sz >= pq->capacity) return reply; // 가득 찼음

    pq->heap[sz] = item;
    pq->size.fetch_add(1);
    heapify_up(pq, sz);

    reply.success = true;
    reply.item = item;
    return reply;
}

Reply dequeue(Queue* queue) {
    Reply reply = { false, {0, nullptr} };
    if (!queue) return reply;
    QueueImpl* pq = to_impl(queue);

    std::lock_guard<std::mutex> lock(pq->dequeue_mtx);
    int sz = pq->size.load();
    if (sz <= 0) return reply; // 빈 큐

    Item ret = pq->heap[0];
    pq->size.fetch_sub(1);
    if (pq->size.load() > 0) {
        pq->heap[0] = pq->heap[pq->size.load()];
        heapify_down(pq, 0);
    }

    reply.success = true;
    reply.item = ret;
    return reply;
}

Queue* range(Queue* queue, Key start, Key end) {
    return nullptr; // 미구현
}
