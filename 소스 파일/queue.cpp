#include <iostream>
#include <mutex>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include "queue.h"

#define MAX_CAPACITY 100000

struct QueueImpl {
    Item* heap;
    int capacity;
    std::atomic<int> size;
    std::mutex enqueue_mtx;
    std::mutex dequeue_mtx;
};

static QueueImpl* to_impl(Queue* q) {
    return reinterpret_cast<QueueImpl*>(q);
}

static int parent(int i) { return (i - 1) / 2; }
static int left(int i) { return 2 * i + 1; }
static int right(int i) { return 2 * i + 2; }

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

// value를 value_size만큼 깊은 복사
static void* deep_copy(Value src, int size) {
    if (!src || size <= 0) return nullptr;
    void* dst = malloc(size);
    if (dst) memcpy(dst, src, size);
    return dst;
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
    int sz = pq->size.load();
    for (int i = 0; i < sz; ++i) {
        free(pq->heap[i].value);
    }
    delete[] pq->heap;
    delete pq;
}

Node* nalloc(Item item) { return nullptr; }
void nfree(Node* node) {}
Node* nclone(Node* node) { return nullptr; }

Reply enqueue(Queue* queue, Item item) {
    Reply reply = { false, {0, nullptr, 0} };
    if (!queue) return reply;
    QueueImpl* pq = to_impl(queue);

    std::lock_guard<std::mutex> lock(pq->enqueue_mtx);
    int sz = pq->size.load();

    // key 중복 확인 및 value 덮어쓰기
    for (int i = 0; i < sz; ++i) {
        if (pq->heap[i].key == item.key) {
            free(pq->heap[i].value);  // 기존 value 해제
            pq->heap[i].value = deep_copy(item.value, item.value_size);
            pq->heap[i].value_size = item.value_size;

            reply.success = true;
            reply.item = pq->heap[i];
            return reply;
        }
    }

    if (sz >= pq->capacity) return reply;

    Item copied;
    copied.key = item.key;
    copied.value = deep_copy(item.value, item.value_size);
    copied.value_size = item.value_size;

    pq->heap[sz] = copied;
    pq->size.fetch_add(1);
    heapify_up(pq, sz);

    reply.success = true;
    reply.item = copied;
    return reply;
}

Reply dequeue(Queue* queue) {
    Reply reply = { false, {0, nullptr, 0} };
    if (!queue) return reply;
    QueueImpl* pq = to_impl(queue);

    std::lock_guard<std::mutex> lock(pq->dequeue_mtx);
    int sz = pq->size.load();
    if (sz <= 0) return reply;

    Item ret = pq->heap[0];
    pq->size.fetch_sub(1);
    int new_size = pq->size.load();

    if (new_size > 0) {
        pq->heap[0] = pq->heap[new_size];
        heapify_down(pq, 0);
    }

    reply.success = true;
    reply.item = ret;
    return reply;
}

Queue* range(Queue* queue, Key start, Key end) {
    if (!queue) return nullptr;
    QueueImpl* src = to_impl(queue);

    // 새 Queue 초기화
    QueueImpl* dst = new QueueImpl;
    dst->capacity = MAX_CAPACITY;
    dst->size = 0;
    dst->heap = new Item[dst->capacity];

    // 원본 큐에 대한 동기화
    std::lock_guard<std::mutex> lock_enq(src->enqueue_mtx);
    std::lock_guard<std::mutex> lock_deq(src->dequeue_mtx);

    int sz = src->size.load();
    for (int i = 0; i < sz; ++i) {
        Key k = src->heap[i].key;
        if (k >= start && k <= end) {
            Item copied;
            copied.key = src->heap[i].key;
            copied.value_size = src->heap[i].value_size;
            copied.value = deep_copy(src->heap[i].value, copied.value_size);

            dst->heap[dst->size] = copied;
            heapify_up(dst, dst->size);
            dst->size.fetch_add(1);
        }
    }

    return reinterpret_cast<Queue*>(dst);
}
