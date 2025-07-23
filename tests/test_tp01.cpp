#include <iostream>
#include <vector>
#include <future>
#include <chrono>
#include <cassert>

#include <ThreadPool.h>

using namespace std;
using namespace std::chrono_literals;

// 基础任务测试
void basic_test() {
    cout << "=== 基础任务测试 ===" << endl;
    ThreadPool pool(4);
    auto result = pool.enqueue([](int a, int b) {
        return a + b;
        }, 2, 3);

    assert(result.get() == 5);
    cout << "基础任务测试通过\n\n";
}

// 并发任务测试
void concurrency_test() {
    cout << "=== 并发任务测试 ===" << endl;
    ThreadPool pool(2);
    vector<future<int>> results;

    for (int i = 0; i < 4; ++i) {
        results.emplace_back(pool.enqueue([i] {
            this_thread::sleep_for(100ms); // 模拟耗时操作
            return i * i;
            }));
    }

    // 验证所有结果
    for (int i = 0; i < 4; ++i) {
        assert(results[i].get() == i * i);
    }
    cout << "并发任务测试通过\n\n";
}

// 逆序并发任务测试
void reverse_concurrency_test() {
    cout << "=== 逆序并发任务测试 ===" << endl;
    ThreadPool pool(2);
    vector<future<int>> results;

    for (int i = 0; i < 4; ++i) {
        results.emplace_back(pool.enqueue([i] {
            this_thread::sleep_for(100ms); // 模拟耗时操作
			cout << "任务" << i << "完成\n";
            return i + i;
            }));
    }

    // 验证所有结果
    for (int i = 3; i >= 0; --i) {
        assert(results[i].get() == i + i);
    }
    cout << "逆序并发任务测试通过\n\n";
}

// 异常处理测试
void exception_test() {
    cout << "=== 异常处理测试 ===" << endl;
    ThreadPool pool(1);
    auto result = pool.enqueue([] {
        throw runtime_error("预期内的异常");
        return 1;
        });

    try {
        result.get();
        assert(false); // 不应该执行到这里
    }
    catch (const exception& e) {
        assert(string(e.what()) == "预期内的异常");
        cout << "成功捕获异常: " << e.what() << "\n";
    }
    cout << "异常处理测试通过\n\n";
}

// 析构函数行为测试
void destructor_test() {
    cout << "=== 析构函数行为测试 ===" << endl;
    {
        ThreadPool pool(2);
        // 提交多个长时间任务
        for (int i = 0; i < 4; ++i) {
            pool.enqueue([i] {
                this_thread::sleep_for(500ms);
                cout << "任务" << i << "完成\n";
                });
        }
    } // 作用域结束触发析构
    cout << "所有任务完成后线程池才销毁\n";
}

int main() {
    basic_test();
    concurrency_test();
    reverse_concurrency_test();
    exception_test();
    destructor_test();

    cout << "\n===== 所有测试通过 =====" << endl;
    return 0;
}