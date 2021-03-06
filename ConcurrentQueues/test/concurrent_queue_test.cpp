#include "concurrent_queue_test.h"
#include <iostream>
#include <chrono>
#include <type_traits>

using ::testing::Values;
using ::testing::Combine;
using ::testing::Types;
using std::cout;
using std::endl;

std::ostream& operator<<(std::ostream& os, const std::chrono::duration<double>& t) {
    double val;
    if ((val = std::chrono::duration<double>(t).count()) > 1.0) {
        os << val << " seconds";
    }
    else if ((val = std::chrono::duration<double, std::milli>(t).count()) > 1.0) {
        os << val << " milliseconds";
    }
    else if ((val = std::chrono::duration<double, std::micro>(t).count()) > 1.0) {
        os << val << " microseconds";
    }
    else {
        val = std::chrono::duration<double, std::nano>(t).count();
        os << val << " nanoseconds";
    }
    return os;
}

void QueueTest::SetUp() {
    auto tupleParams = GetParam();
    _params = TestParameters{ ::testing::get<0>(tupleParams), ::testing::get<1>(tupleParams), ::testing::get<2>(tupleParams), ::testing::get<3>(tupleParams), ::testing::get<4>(tupleParams), ::testing::get<5>(tupleParams) };
    readers.resize(_params.nReaders, basic_timer());
    writers.resize(_params.nWriters, basic_timer());
    cout << "Readers: " << _params.nReaders << endl;
    cout << "Writers: " << _params.nWriters << endl;
    cout << "Elements: " << _params.nElements << endl;
    cout << "Queue Size: " << _params.queueSize << endl;
    cout << "Subqueue Size: " << _params.subqueueSize << endl;
    cout << "Test Type: ";
    switch (_params.testType) {
    case BUSY_TEST: cout << "Busy Test"; break;
    case YIELD_TEST: cout << "Yield Test"; break;
    case SLEEP_TEST: cout << "Sleep Test"; break;
    case BACKOFF_TEST: cout << "Backoff Test"; break;
    default: break;
    }
    cout << endl;
}

void QueueTest::TearDown() {
    cout << "Enqueue:" << endl;
    auto writeDur = writers[0].getElapsedDuration();
    auto writeMax = writers[0].getElapsedDuration();
    int toMeasure = 1;
    for (size_t i = 1; i < writers.size(); ++i) {
        auto dur = writers[i].getElapsedDuration();
        if (dur > std::chrono::nanoseconds(1)) {
            writeDur += dur;
            if (dur > writeMax) writeMax = dur;
            ++toMeasure;
        }
    }
    writeDur /= toMeasure;
    cout << "Max write thread duration: " << writeMax << endl;
    cout << "Average write thread duration: " << writeDur << endl;
    cout << "Time per enqueue (average): " << writeDur / _params.nElements << endl;
    cout << "Time per enqueue (worst case): " << writeMax / _params.nElements << endl;
    cout << "Enqueue ops/second (average): " << static_cast<double>(_params.nElements) / writeDur.count() << std::endl;
    cout << "Enqueue ops/second (worst case): " << static_cast<double>(_params.nElements) / writeMax.count() << std::endl;
    cout << "Enqueue ops/second/thread (worst case): " << static_cast<double>(_params.nElements) / writeMax.count() / _params.nWriters << std::endl;

    cout << "Dequeue:" << endl;
    auto readDur = readers[0].getElapsedDuration();
    auto readMax = readers[0].getElapsedDuration();
    toMeasure = 1;
    for (size_t i = 1; i < readers.size(); ++i) {
        auto dur = readers[i].getElapsedDuration();
        if (dur > std::chrono::nanoseconds(1)) {
            readDur += dur;
            if (dur > readMax) readMax = dur;
            ++toMeasure;
        }
    }
    readDur /= toMeasure;
    cout << "Max read thread duration: " << readMax << endl;
    cout << "Average read thread duration: " << readDur << endl;
    cout << "Time per dequeue (average case)" << readDur / _params.nElements << std::endl;
    cout << "Time per dequeue (worst case): " << readMax / _params.nElements << endl;
    cout << "Dequeue ops/second (average case): " << static_cast<double>(_params.nElements) / readDur.count() << std::endl;
    cout << "Dequeue ops/second (worst case): " << static_cast<double>(_params.nElements) / readMax.count() << std::endl;
    cout << "Dequeue ops/second/thread (worst case): " << static_cast<double>(_params.nElements) / readMax.count() / _params.nReaders << std::endl;
}

INSTANTIATE_TEST_CASE_P(
    queue_benchmark,
    QueueTest,
    testing::Combine(
        Values(1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024), //readers
        Values(1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024), //writers
        Values(size_t(1e6), size_t(1e7), size_t(1e8), size_t(1e9), size_t(1e10)), //elements
        Values(8192, 32768, 131072, 524288, 2097152, 4194304, 8388608, 16777216, 33554432), //queue size (bounded only)
        Values(2, 4, 8, 16, 32, 64), //subqueue size (multiqueue only)
        Values(QueueTestType::BUSY_TEST, QueueTestType::YIELD_TEST, QueueTestType::SLEEP_TEST, QueueTestType::BACKOFF_TEST)) //test type
);
