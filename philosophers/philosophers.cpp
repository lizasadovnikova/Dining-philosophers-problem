#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <random>
#include <chrono>

// Структура для виделки, що гарантує ексклюзивний доступ
struct Fork {
    std::mutex mtx;
};

// Алгоритм дій філософа
// id передається лише для початкового сідінгу генератора випадкових чисел,
// щоб уникнути ситуації, коли потоки генерують однакові послідовності через одночасний старт.
void philosopher(int id, Fork& left_fork, Fork& right_fork) {
    // Ініціалізація локального генератора випадкових чисел
    std::mt19937 rng(std::random_device{}() + id);
    std::uniform_int_distribution<int> coin_flip(0, 1);
    std::uniform_int_distribution<int> backoff(1, 5); // Випадкова затримка в мілісекундах
    std::uniform_int_distribution<int> action_time(10, 30);

    for (int meals = 0; meals < 5; ++meals) { // Симуляція 5 прийомів їжі
        // 1. Філософ думає
        std::this_thread::sleep_for(std::chrono::milliseconds(action_time(rng)));

        bool ate = false;
        while (!ate) {
            // 2. Кидаємо монетку: 0 - спочатку ліва виделка, 1 - спочатку права
            // Це ламає симетрію дедлоку, не використовуючи ID або пріоритети.
            bool choose_left_first = (coin_flip(rng) == 0);

            Fork& first_fork = choose_left_first ? left_fork : right_fork;
            Fork& second_fork = choose_left_first ? right_fork : left_fork;

            // 3. Беремо першу виделку
            first_fork.mtx.lock();

            // 4. Пробуємо взяти другу виделку без блокування
            if (second_fork.mtx.try_lock()) {

                // 5. Філософ їсть
                std::this_thread::sleep_for(std::chrono::milliseconds(action_time(rng)));

                // 6. Звільняємо обидві виделки
                second_fork.mtx.unlock();
                first_fork.mtx.unlock();

                ate = true; // Виходимо з циклу спроб поїсти
            }
            else {
                // Друга виделка зайнята сусідом.
                // Щоб уникнути Deadlock, ми зобов'язані покласти першу виделку назад.
                first_fork.mtx.unlock();

                // Щоб уникнути Livelock, коли філософи синхронно беруть і кладуть виделки,
                // робимо паузу на випадковий короткий проміжок часу перед наступною спробою.
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff(rng)));
            }
        }
    }
}

int main() {
    const int num_philosophers = 5;

    // Створюємо виделки
    std::vector<Fork> forks(num_philosophers);
    std::vector<std::thread> philosophers;

    // Запускаємо потоки
    for (int i = 0; i < num_philosophers; ++i) {
        Fork& left = forks[i];
        Fork& right = forks[(i + 1) % num_philosophers];

        philosophers.emplace_back(philosopher, i, std::ref(left), std::ref(right));
    }

    // Чекаємо завершення трапези всіх філософів
    for (auto& p : philosophers) {
        p.join();
    }

    std::cout << "All philosophers successfully ate!" << std::endl;
    return 0;
}