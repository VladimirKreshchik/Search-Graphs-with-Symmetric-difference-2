#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <cstdlib>

using namespace std;

// Функция для подсчёта количества установленных битов в числе mask
int countBits(int mask) {
    int cnt = 0;
    while (mask) {
        cnt += (mask & 1);
        mask >>= 1;
    }
    return cnt;
}

// Функция для проверки, содержит ли граф индуцированный цикл (без дополнительных рёбер) размера не меньше 5
bool has_induced_cycle_at_least_5(const vector< vector<bool> > &adj, int n) {
    // Перебираем все подмножества вершин в виде битовой маски (максимум 2^n, n <= 11)
    int total = 1 << n;
    for (int mask = 0; mask < total; mask++) {
        int cnt = countBits(mask);
        if (cnt < 5)
            continue;

        // Собираем индексы вершин, входящих в это подмножество.
        vector<int> subset;
        for (int i = 0; i < n; i++) {
            if (mask & (1 << i))
                subset.push_back(i);
        }

        // Проверяем: в индуцированном подграфе каждая вершина должна иметь степень ровно 2.
        bool ok = true;
        for (int v : subset) {
            int deg = 0;
            for (int u : subset) {
                if (u != v && adj[v][u])
                    deg++;
            }
            if (deg != 2) {
                ok = false;
                break;
            }
        }
        if (!ok)
            continue;

        // Проверка связности индуцированного подграфа:
        vector<bool> visited(n, false);
        queue<int> q;
        q.push(subset[0]);
        visited[subset[0]] = true;
        int visCount = 0;
        while (!q.empty()) {
            int cur = q.front();
            q.pop();
            visCount++;
            for (int u : subset) {
                if (!visited[u] && adj[cur][u]) {
                    visited[u] = true;
                    q.push(u);
                }
            }
        }
        if (visCount == (int)subset.size()) {
            // Нашли индуцированный цикл
            return true;
        }
    }
    return false;
}

// Новая функция для проверки наличия цикла длины ровно 3 (треугольника)
bool has_triangle(const vector< vector<bool> > &adj, int n) {
    // Перебираем все комбинации трёх различных вершин.
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (!adj[i][j]) continue;  // если между i и j нет ребра, то треугольник невозможен
            for (int k = j + 1; k < n; k++) {
                if (adj[i][k] && adj[j][k])
                    return true;
            }
        }
    }
    return false;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file.g6>\n";
        return 1;
    }
    ifstream infile(argv[1]);
    if (!infile.is_open()) {
        cerr << "Error: cannot open input file.\n";
        return 1;
    }

    string line;
    int graphIndex = 0;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        // Пропускаем строку-заголовок >>graph6<< или служебные строки (':',';','&')
        unsigned char firstChar = line[0];
        if (firstChar < 63 || firstChar > 126) {
            continue;
        }
        graphIndex++;

        // Расшифровка количества вершин n
        int n;
        size_t pos = 0;  // позиция в строке после кодирования n
        if (firstChar == 126) {
            // Расширенный формат N(n) – случай n >= 63 (но для n <= 11 он не нужен)
            if (line.size() >= 4 && (unsigned char)line[1] != 126) {
                // 63 <= n <= 258047, кодируется как 126 + 3 байта
                if (line.size() < 4) continue;
                n = ((line[1] - 63) << 12) | ((line[2] - 63) << 6) | (line[3] - 63);
                pos = 4;
            } else if (line.size() >= 8 && (unsigned char)line[1] == 126) {
                // 258048 <= n <= 2^36-1, кодируется как 126,126 + 6 байт
                if (line.size() < 8) continue;
                n = ((line[2] - 63) << 30) | ((line[3] - 63) << 24) | 
                    ((line[4] - 63) << 18) | ((line[5] - 63) << 12) | 
                    ((line[6] - 63) << 6)  |  (line[7] - 63);
                pos = 8;
            } else {
                // Неверный формат
                continue;
            }
        } else {
            // n <= 62, кодируется одним символом
            n = firstChar - 63;
            pos = 1;
        }

        if (n < 1) {
            // Граф без вершин или с одной вершиной:
            // - Нулевой граф (n=0) пропускаем (несвязный и условие неприменимо).
            // - Граф с 1 вершиной (n=1) можно считать удовлетворяющим условию тривиально, 
            //   но он не интересен (связный тривиально, пар вершин нет). Пропустим из вывода.
            continue;
        }

        // Заполняем матрицу смежности
        vector< vector<bool> > adj(n, vector<bool>(n, false));
        int totalBits = n * (n - 1) / 2;
        int bitCount = 0;
        unsigned int currVal = 0;
        int bitsLeft = 0;
        // Перебираем пары (i,j) для верхнего треугольника
        for (int j = 1; j < n; ++j) {
            for (int i = 0; i < j; ++i) {
                // Получаем следующий бит из последовательности
                if (bitsLeft == 0) {
                    if (pos >= line.size()) {
                        // Строка неожиданно завершилась
                        bitsLeft = 0;
                        break;
                    }
                    currVal = (unsigned char)line[pos] - 63;
                    pos++;
                    bitsLeft = 6;
                }
                bitsLeft--;
                int bit = (currVal >> bitsLeft) & 1;
                if (bit) {
                    adj[i][j] = true;
                    adj[j][i] = true;
                }
                bitCount++;
            }
            if (bitCount < j * (j + 1) / 2) {
                // Если вышли из внутреннего цикла не обработав все пары до j (не должно случиться в корректном формате)
                break;
            }
        }

        // Проверка связности (BFS)
        vector<char> visited(n, 0);
        queue<int> q;
        visited[0] = 1;
        q.push(0);
        int reachCount = 1;
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (int v = 0; v < n; ++v) {
                if (!visited[v] && adj[u][v]) {
                    visited[v] = 1;
                    q.push(v);
                    reachCount++;
                }
            }
        }
        if (reachCount != n) {
            continue;  // граф не связный, пропускаем
        }

        if (has_triangle(adj, n))
            continue;

        // Проверка условия для каждой пары вершин
        bool valid = true;
        for (int u = 0; u < n && valid; ++u) {
            for (int v = u + 1; v < n && valid; ++v) {
                int exclusiveCount = 0;
                for (int w = 0; w < n && exclusiveCount < 2; ++w) {
                    if (w == u || w == v) continue;
                    // Проверяем XOR смежности: считаем w, смежные ровно с одним из (u,v)
                    if (adj[u][w] != adj[v][w]) {
                        exclusiveCount++;
                    }
                }
                if (exclusiveCount < 2) {
                    valid = false;
                }
            }
        }
        if (!valid) {
            continue;
        }
        if (has_induced_cycle_at_least_5(adj, n))
            continue;
        // Вывод строки graph6 для графа, удовлетворяющего условиям
        cout << line << "\n";
    }

    return 0;
}


