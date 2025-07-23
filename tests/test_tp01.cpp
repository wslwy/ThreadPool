#include <iostream>
#include <vector>
#include <future>
#include <chrono>
#include <cassert>

#include <ThreadPool.h>

using namespace std;
using namespace std::chrono_literals;

// �����������
void basic_test() {
    cout << "=== ����������� ===" << endl;
    ThreadPool pool(4);
    auto result = pool.enqueue([](int a, int b) {
        return a + b;
        }, 2, 3);

    assert(result.get() == 5);
    cout << "�����������ͨ��\n\n";
}

// �����������
void concurrency_test() {
    cout << "=== ����������� ===" << endl;
    ThreadPool pool(2);
    vector<future<int>> results;

    for (int i = 0; i < 4; ++i) {
        results.emplace_back(pool.enqueue([i] {
            this_thread::sleep_for(100ms); // ģ���ʱ����
            return i * i;
            }));
    }

    // ��֤���н��
    for (int i = 0; i < 4; ++i) {
        assert(results[i].get() == i * i);
    }
    cout << "�����������ͨ��\n\n";
}

// ���򲢷��������
void reverse_concurrency_test() {
    cout << "=== ���򲢷�������� ===" << endl;
    ThreadPool pool(2);
    vector<future<int>> results;

    for (int i = 0; i < 4; ++i) {
        results.emplace_back(pool.enqueue([i] {
            this_thread::sleep_for(100ms); // ģ���ʱ����
			cout << "����" << i << "���\n";
            return i + i;
            }));
    }

    // ��֤���н��
    for (int i = 3; i >= 0; --i) {
        assert(results[i].get() == i + i);
    }
    cout << "���򲢷��������ͨ��\n\n";
}

// �쳣�������
void exception_test() {
    cout << "=== �쳣������� ===" << endl;
    ThreadPool pool(1);
    auto result = pool.enqueue([] {
        throw runtime_error("Ԥ���ڵ��쳣");
        return 1;
        });

    try {
        result.get();
        assert(false); // ��Ӧ��ִ�е�����
    }
    catch (const exception& e) {
        assert(string(e.what()) == "Ԥ���ڵ��쳣");
        cout << "�ɹ������쳣: " << e.what() << "\n";
    }
    cout << "�쳣�������ͨ��\n\n";
}

// ����������Ϊ����
void destructor_test() {
    cout << "=== ����������Ϊ���� ===" << endl;
    {
        ThreadPool pool(2);
        // �ύ�����ʱ������
        for (int i = 0; i < 4; ++i) {
            pool.enqueue([i] {
                this_thread::sleep_for(500ms);
                cout << "����" << i << "���\n";
                });
        }
    } // �����������������
    cout << "����������ɺ��̳߳ز�����\n";
}

int main() {
    basic_test();
    concurrency_test();
    reverse_concurrency_test();
    exception_test();
    destructor_test();

    cout << "\n===== ���в���ͨ�� =====" << endl;
    return 0;
}