#pragma execution_character_set("utf-8")

#include <iostream>
#include <vector>
#include <stack>
#include <memory>
#include <chrono>
#include <string>


struct Point {
    int x, y;
};

// Стек на основе массива
class ArrayStack {
    std::unique_ptr<Point[]> arr; // Умный указатель
    // Память автоматически освободится при выходе объекта из области видимости
    int top_idx = -1;
public:
    // explicit - запрет на неявное преобразование
    // В списке инициализации умному указателю присваивается блок памяти из кучи
    explicit ArrayStack(int max_capacity) : arr(std::make_unique<Point[]>(max_capacity)) {}
    
    // Метод для очистки стека без переаллокации памяти
    // Память выделяется один раз и новые значения перезаписываются поверх старых
    void clear() { top_idx = -1; }
    
    void push(Point p) { arr[++top_idx] = p; }
    // const относится к объекту, который этот метод выполняет
    // В этой позиции const модифицирует тип неявного указателя this,
    // накладывая запрет на изменение полей класса и вызов неконстантных методов внутри данной функции
    Point top() const  { return arr[top_idx]; }
    void pop()         { --top_idx; }
    bool empty() const { return top_idx == -1; }

    // Чтобы не было неочевидных ошибок (по типу двойного высвобождения памяти)
    ArrayStack(const ArrayStack&) = delete; // Запрет конструктора копирования
    ArrayStack& operator=(const ArrayStack&) = delete; // Запрет оператора присваивания
};

// Стек на основе связанного списка
class ListStack {
    struct Node {
        Point p;
        Node* next;
    };
    Node* head = nullptr;
public:
    // Параметры не принимает, но нужна для совместимости с шаблонным интерфейсом
    explicit ListStack(int /*unused*/) {}
    ~ListStack() { clear(); } // Деструктор

    void clear() {
        while (!empty()) pop();
    }

    void push(Point p) { head = new Node{p, head}; }
    Point top() const  { return head->p; }
    void pop() {
        Node* temp = head;
        head = head->next;
        delete temp;
    }
    bool empty() const { return head == nullptr; }

    ListStack(const ListStack&) = delete;
    ListStack& operator=(const ListStack&) = delete;
};

// Стек на основе STL
class StlStack {
    std::stack<Point> s; // По умолчанию использует std::deque
public:
    // Параметр max_capacity здесь игнорируется, так как у deque нет reserve()
    // Оставляем его только для совместимости с шаблонным интерфейсом
    explicit StlStack(int /*max_capacity*/) {}

    void clear() { 
        // Быстрый способ очистить стандартный стек
        s = std::stack<Point>(); 
    }
    
    void push(Point p) { s.push(p); }
    Point top() const  { return s.top(); }
    void pop()         { s.pop(); }
    bool empty() const { return s.empty(); }
};

// Алгоритм поиска в глубину (DFS)
// Шаблонная функция - пример статического полиморфизма
template <typename StackType>
int countComponents(int M, int N, const std::vector<bool>& grid, bool isCylinder) {

    // std::vector<bool> реализует битовую упаковку (1 бит на значение)
    // Инициализируем все клетки нулями
    std::vector<bool> visited(M * N, false);
    int components = 0;

    // Создаем стек один раз
    StackType st(M * N);

    // Векторы смещения для обхода графов на сетке
    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};

    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < M; ++x) {
            int current_idx = y * M + x;
            
            // false в grid означает пустое место
            if (grid[current_idx] && !visited[current_idx]) {
                components++;

                st.clear(); // Сбрасываем индекс/указатель, не перевыделяя память
                st.push({x, y}); // Занесли точку
                visited[current_idx] = true; // Пометили её прочитанной (важно)

                while (!st.empty()) {
                    Point p = st.top(); // Создаём локальную копию
                    st.pop(); // Сразу удаляем

                    // Благодаря векторам смещения за один цикл проверяем все 4 стороны
                    for (int i = 0; i < 4; ++i) {
                        int nx = p.x + dx[i];
                        int ny = p.y + dy[i];

                        if (ny >= 0 && ny < N) { // Проверка вертикальных границ листа
                            // Кольцевой перенос координат
                            if (isCylinder) {
                                if (nx >= M) nx = 0; // Вышли за правый край - перешли в начало
                                else if (nx < 0) nx = M - 1; // Вышли за левый край - перешли в конец
                            }

                            if (nx >= 0 && nx < M) { // Проверка горизонтальных границ
                                int neighbor_idx = ny * M + nx;
                                
                                if (grid[neighbor_idx] && !visited[neighbor_idx]) {
                                    visited[neighbor_idx] = true;
                                    st.push({nx, ny});
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return components;
}

// Замеры времени
template <typename StackType>
void benchmark(const std::string& name, int M, int N, const std::vector<bool>& grid, bool isCylinder) {
    auto start = std::chrono::high_resolution_clock::now();
    int result = countComponents<StackType>(M, N, grid, isCylinder);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << name << ": " << result << " частей за " << duration.count() << " мс\n";
}

int main() {

    const int M = 2000;
    const int N = 2000;

    std::vector<bool> grid(M * N, true);
    
    // Нарезаем лист диагональными полосами
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < M; ++x) {
            // Период 7 не кратен ширине 2000. 
            if ((x + y) % 7 == 0) {
                grid[y * M + x] = false;
            }
        }
    }
    
    std::cout << "\n===== ПЛОСКИЙ ЛИСТ =====\n";
    benchmark<ArrayStack>("Стек на основе массива ", M, N, grid, false);
    benchmark<ListStack> ("Стек на основе списка  ", M, N, grid, false);
    benchmark<StlStack>  ("Стек STL               ", M, N, grid, false);

    std::cout << "\n===== ЦИЛИНДР =====\n";
    benchmark<ArrayStack>("Стек на основе массива ", M, N, grid, true);
    benchmark<ListStack> ("Стек на основе списка  ", M, N, grid, true);
    benchmark<StlStack>  ("Стек STL               ", M, N, grid, true);

    std::cout << "\nРаботу выполнил Кузьмич А. Д. Группа 090301-ПОВа-о25" << std::endl;

    return 0;
}