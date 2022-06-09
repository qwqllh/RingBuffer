#include "RingBuffer.h"

#include <cstdio>
#include <cstdlib>
#include <random>

static constexpr size_t TEST_DATA_NUM   = 65536;
static constexpr size_t TEST_GROUP_NUM  = 4;
static constexpr size_t RINGBUFFER_SIZE = 4096;

struct TestData {
    size_t group;
    size_t index;
    size_t number;
};

static TestData g_test_data[TEST_GROUP_NUM][TEST_DATA_NUM];
static RingBuffer<TestData, RINGBUFFER_SIZE> g_buffer;

static constexpr size_t BUFFER_SIZE = sizeof(g_buffer);

static void InitTestData() {
    std::default_random_engine            rand_engine(std::random_device{}());
    std::uniform_int_distribution<size_t> distribution;
    for (size_t i = 0; i < TEST_GROUP_NUM; ++i) {
        for (size_t j = 0; j < TEST_DATA_NUM; ++j) {
            auto &data  = g_test_data[i][j];
            data.group  = i;
            data.index  = j;
            data.number = distribution(rand_engine);
        }
    }

    for (const auto &group : g_test_data) {
        for (const auto &data : group) {
            if (data.number != g_test_data[data.group][data.index].number) {
                printf("in line %d: group: %zu, index: %zu, data: %zu, "
                       "TestDataSet is %zu.\n",
                       __LINE__,
                       data.group,
                       data.index,
                       data.number,
                       g_test_data[data.group][data.index].number);
                std::abort();
            }
        }
    }
}

static void ProducerFunc(size_t group_id) {
    for (size_t i = 0; i < TEST_DATA_NUM; ++i) {
        while (g_buffer.IsFull())
            std::this_thread::yield();
        g_buffer.Push(g_test_data[group_id][i]);
        // printf("%s %zu %zu\n", __func__, group_id, i);
    }
}

static void ConsumerFunc() {
    for (size_t counter = 0; counter < TEST_DATA_NUM * TEST_GROUP_NUM;
         ++counter) {
        while (g_buffer.IsEmpty())
            std::this_thread::yield();
        auto data = g_buffer.Pop();

        if (data.number != g_test_data[data.group][data.index].number) {
            printf("in line %d: group: %zu, index: %zu, data: %zu, "
                   "TestDataSet is %zu.\n",
                   __LINE__,
                   data.group,
                   data.index,
                   data.number,
                   g_test_data[data.group][data.index].number);
            std::abort();
        }

        // printf("%s %zu\n", __func__, counter);
    }
}

int main() {
    InitTestData();
    asm volatile("" ::: "memory");
    std::thread consumer(ConsumerFunc);
    std::thread producer0(ProducerFunc, 0);
    std::thread producer1(ProducerFunc, 1);
    std::thread producer2(ProducerFunc, 2);
    std::thread producer3(ProducerFunc, 3);

    consumer.join();
    producer0.join();
    producer1.join();
    producer2.join();
    producer3.join();
    return 0;
}
