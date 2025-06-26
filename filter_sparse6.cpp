#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <cstdlib>

using namespace std;

// ---------------------- ФУНКЦИИ ФИЛЬТРАЦИИ ----------------------

// Подсчёт установленных битов
int countBits(int mask) {
    int cnt = 0;
    while (mask) {
        cnt += (mask & 1);
        mask >>= 1;
    }
    return cnt;
}

// Проверка наличия индуцированного цикла (без хордов) длины не меньше 5
bool has_induced_cycle_at_least_5(const vector< vector<bool> > &adj, int n) {
    int total = 1 << n;
    for (int mask = 0; mask < total; mask++) {
        if (countBits(mask) < 5)
            continue;
        vector<int> subset;
        for (int i = 0; i < n; i++) {
            if (mask & (1 << i))
                subset.push_back(i);
        }
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
        vector<bool> visited(n, false);
        queue<int> q;
        q.push(subset[0]);
        visited[subset[0]] = true;
        int visCount = 0;
        while (!q.empty()) {
            int cur = q.front(); q.pop();
            visCount++;
            for (int u : subset) {
                if (!visited[u] && adj[cur][u]) {
                    visited[u] = true;
                    q.push(u);
                }
            }
        }
        if (visCount == (int)subset.size())
            return true;
    }
    return false;
}

// Проверка наличия цикла длины 3 (треугольника)
bool has_triangle(const vector< vector<bool> > &adj, int n) {
    for (int i = 0; i < n; i++){
        for (int j = i+1; j < n; j++){
            if (!adj[i][j])
                continue;
            for (int k = j+1; k < n; k++){
                if (adj[i][k] && adj[j][k])
                    return true;
            }
        }
    }
    return false;
}

// Проверка связности графа (BFS)
bool isConnected(const vector< vector<bool> > &adj, int n) {
    vector<bool> visited(n, false);
    queue<int> q;
    visited[0] = true;
    q.push(0);
    int count = 0;
    while(!q.empty()){
        int u = q.front(); q.pop();
        count++;
        for (int v = 0; v < n; v++){
            if (!visited[v] && adj[u][v]){
                visited[v] = true;
                q.push(v);
            }
        }
    }
    return (count == n);
}

// Проверка условия "эксклюзивных" соседей для каждой пары вершин
bool hasExclusiveNeighbors(const vector< vector<bool> > &adj, int n) {
    for (int u = 0; u < n; u++){
        for (int v = u+1; v < n; v++){
            int count = 0;
            for (int w = 0; w < n; w++){
                if (w == u || w == v)
                    continue;
                if (adj[u][w] != adj[v][w])
                    count++;
                if (count >= 2)
                    break;
            }
            if (count < 2)
                return false;
        }
    }
    return true;
}

// ------------------- ФУНКЦИИ ДЕКОДИРОВАНИЯ -------------------

// Полное декодирование графа в формате sparse6 (ожидается, что строка начинается с ':')
bool decodeFullSparse6(const string &line, vector< vector<bool> > &adj, int &n) {
    if (line.empty() || line[0] != ':')
        return false;
    size_t pos = 1;
    if (pos >= line.size())
        return false;
    n = line[pos] - 63;
    pos++;
    adj.assign(n, vector<bool>(n, false));

    // Формируем список пар (i,j) для верхнего треугольника (i<j)
    vector< pair<int,int> > edgeMapping;
    for (int j = 1; j < n; j++){
        for (int i = 0; i < j; i++){
            edgeMapping.push_back({i, j});
        }
    }
    int totalEdges = n * (n - 1) / 2;
    for (int k = 0; k < totalEdges; k++){
        int charIndex = pos + (k / 6);
        int bitPos = 5 - (k % 6);
        if (charIndex >= line.size()) {
            cerr << "Warning: charIndex (" << charIndex << ") >= line.size() (" << line.size() << ") at k = " << k << "\n";
            break;
        }
        int c = line[charIndex] - 63;
        int bit = (c >> bitPos) & 1;
        if (bit) {
            if (k >= (int)edgeMapping.size()) {
                cerr << "Error: k (" << k << ") >= edgeMapping.size() (" << edgeMapping.size() << ")\n";
                return false;
            }
            int i = edgeMapping[k].first;
            int j = edgeMapping[k].second;
            adj[i][j] = true;
            adj[j][i] = true;
        }
    }
    return true;
}

// Декодирование списка ребер из строки (без первого символа) по sparse6 алгоритму
vector< pair<int,int> > decodeEdgeList(const string &s, int n) {
    vector< pair<int,int> > edges;
    int u = 0, d = -1;
    for (size_t pos = 0; pos < s.size(); pos++) {
        int x = s[pos] - 63;
        d += x;
        while ((n - u - 1) > 0 && d >= (n - u - 1)) {
            d -= (n - u - 1);
            u++;
        }
        if (u >= n)
            break;
        int v = u + d + 1;
        if (v < n)
            edges.push_back({u, v});
    }
    return edges;
}

// Декодирование графа в формате incremental sparse6 (строка начинается с ';')
// Число вершин n берётся из предыдущего графа (prevAdj). В этом случае граф = prevAdj ⊕ (симметричная разность с декодированными ребрами).
bool decodeIncSparse6(const string &line, const vector< vector<bool> > &prevAdj,
                      vector< vector<bool> > &newAdj, int n) {
    if (line.empty() || line[0] != ';')
        return false;
    // Копируем предыдущий граф
    newAdj = prevAdj;
    string edgeStr = line.substr(1);
    vector< pair<int,int> > toggles = decodeEdgeList(edgeStr, n);
    for (auto &p : toggles) {
        int u = p.first, v = p.second;
        if(u < 0 || u >= n || v < 0 || v >= n) {
            cerr << "Error: toggling edge (" << u << ", " << v << ") out of bounds (n=" << n << ")\n";
            continue;
        }
        newAdj[u][v] = !newAdj[u][v];
        newAdj[v][u] = newAdj[u][v];
    }
    return true;
}

// ------------------- ОСНОВНАЯ ФУНКЦИЯ -------------------

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file.is6>\n";
        return 1;
    }
    ifstream infile(argv[1]);
    if (!infile.is_open()){
        cerr << "Ошибка: не удалось открыть файл.\n";
        return 1;
    }
    
    string line;
    bool firstGraph = true;
    int n = 0;
    vector< vector<bool> > prevAdj;
    long long int count = 0;
    while(getline(infile, line)) {
        if (line.empty())
            continue;
        vector< vector<bool> > adj;
        bool ok = false;
        if (line[0] == ';') {
            if (firstGraph) {
                cerr << "Ошибка: первый граф не может быть incremental (начинается с ';').\n";
                return 1;
            }
            ok = decodeIncSparse6(line, prevAdj, adj, n);
        } else {
            ok = decodeFullSparse6(line, adj, n);
        }
        if (!ok)
            continue;
        if (n < 1)
            continue;
        
        // Важно: независимо от прохождения фильтров обновляем prevAdj,
        // так как следующие incremental графы рассчитываются на основе файла!
        prevAdj = adj;
        firstGraph = false;
        count++;
        if (count % 100000 == 0) {
            cerr << count / 100000 << '\n';
        }
        // Фильтрация графа
        if (!isConnected(adj, n))
            continue;
        if (has_triangle(adj, n))
            continue;
        if (!hasExclusiveNeighbors(adj, n))
            continue;
        if (has_induced_cycle_at_least_5(adj, n))
            continue;
        
        // Если граф удовлетворяет всем условиям, выводим исходную строку
        cout << line << "\n";
    }
    return 0;
}

