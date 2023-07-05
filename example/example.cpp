#include "threadpool.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

int test(int i)
{
    std::cout << "hello " << i << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "world " << i << std::endl;
    return i * i;
}

int main()
{
    threadpool pool(4);

    pool.submit([] { 
        test(10);
    });

    std::this_thread::sleep_for(std::chrono::seconds(3));
    pool.submit([] { std::cout << "new task..." << std::endl; });

    std::vector<std::future<int>> results;

    for(int i = 0; i < 8; ++i) 
    {
        auto future = pool.async(test, i);
        // auto async_future = pool.enqueue(test, i);
        results.push_back(std::move(future));
    }

    for (auto & result : results)
        std::cout << result.get() << std::endl;
    
    return 0;
}