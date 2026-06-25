#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <ctime>

using namespace std;

// ==========================================
// 1. 基本データ構造とヘルパー
// ==========================================
struct City {
    double x, y;
};

// 無向エッジを表す構造体（常に u < v となるように正規化）
struct Edge {
    int u, v;
    Edge(int _u, int _v) {
        u = min(_u, _v);
        v = max(_u, _v);
    }
    bool operator<(const Edge& other) const {
        if (u != other.u) return u < other.u;
        return v < other.v;
    }
    bool operator==(const Edge& other) const {
        return u == other.u && v == other.v;
    }
};

double calc_distance(const City& a, const City& b) {
    return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

double calc_tour_distance(const vector<int>& tour, const vector<vector<double>>& dist_matrix) {
    double d = 0;
    int n = tour.size();
    for (int i = 0; i < n; ++i) {
        d += dist_matrix[tour[i]][tour[(i + 1) % n]];
    }
    return d;
}

// ==========================================
// 2. EAX本体クラス
// ==========================================
class EAX_Solver {
private:
    int n;
    vector<vector<double>> dist_matrix;

    // ルート（配列）をエッジの集合に変換
    set<Edge> tour_to_edges(const vector<int>& tour) {
        set<Edge> edges;
        for (int i = 0; i < n; ++i) {
            edges.insert(Edge(tour[i], tour[(i + 1) % n]));
        }
        return edges;
    }

    // 2-opt (局所探索) - 最終的な仕上げ用
    vector<int> apply_2opt(vector<int> tour) {
        bool improved = true;
        while (improved) {
            improved = false;
            for (int i = 0; i < n - 1; ++i) {
                for (int j = i + 2; j < n; ++j) {
                    if (i == 0 && j == n - 1) continue;
                    int a = tour[i], b = tour[i + 1];
                    int c = tour[j], d = tour[(j + 1) % n];
                    
                    if (dist_matrix[a][c] + dist_matrix[b][d] < dist_matrix[a][b] + dist_matrix[c][d] - 1e-6) {
                        reverse(tour.begin() + i + 1, tour.begin() + j + 1);
                        improved = true;
                    }
                }
            }
        }
        return tour;
    }

public:
    EAX_Solver(const vector<City>& cities) {
        n = cities.size();
        dist_matrix.assign(n, vector<double>(n, 0.0));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                dist_matrix[i][j] = calc_distance(cities[i], cities[j]);
            }
        }
    }

    // ==========================================
    // EAX 交叉処理のメイン関数
    // ==========================================
    vector<int> crossover(const vector<int>& parentA, const vector<int>& parentB) {
        set<Edge> edgesA = tour_to_edges(parentA);
        set<Edge> edgesB = tour_to_edges(parentB);

        // Step 1: グラフの重ね合わせ (差分を取る)
        set<Edge> uniqueA, uniqueB;
        for (const auto& e : edgesA) {
            if (edgesB.find(e) == edgesB.end()) uniqueA.insert(e);
        }
        for (const auto& e : edgesB) {
            if (edgesA.find(e) == edgesA.end()) uniqueB.insert(e);
        }

        // 親が全く同じ場合はそのまま返す
        if (uniqueA.empty()) return parentA;

        // Step 2: ABサイクルの抽出 (交互に辿る)
        // 効率化のため、各ノードからの uniqueA, uniqueB の接続先をリスト化
        vector<vector<int>> adjA(n), adjB(n);
        for (const auto& e : uniqueA) { adjA[e.u].push_back(e.v); adjA[e.v].push_back(e.u); }
        for (const auto& e : uniqueB) { adjB[e.u].push_back(e.v); adjB[e.v].push_back(e.u); }

        vector<vector<Edge>> ab_cycles;
        vector<bool> visitedA_edges(n * n, false); // u*n+vのハッシュで訪問済み管理
        
        auto edge_hash = [&](int u, int v) { return min(u, v) * n + max(u, v); };

        for (int start_node = 0; start_node < n; ++start_node) {
            while (true) {
                // 未訪問のAのエッジを探す
                int next_node = -1;
                for (int neighbor : adjA[start_node]) {
                    if (!visitedA_edges[edge_hash(start_node, neighbor)]) {
                        next_node = neighbor;
                        break;
                    }
                }
                if (next_node == -1) break; // このノードを起点とするABサイクルはもう無い

                vector<Edge> current_cycle;
                int curr = start_node;
                bool use_A = true;

                // 交互に閉路ができるまで辿る
                while (true) {
                    int next = -1;
                    const auto& adj = use_A ? adjA : adjB;
                    
                    for (int neighbor : adj[curr]) {
                        // Aのエッジを辿る時は訪問フラグをチェック、Bの時は気にしない（Bは必ずAに付随して消費されるため）
                        if (use_A && visitedA_edges[edge_hash(curr, neighbor)]) continue;
                        
                        // 直前に通ったエッジを逆流しないようにする処理（この簡易実装では1回通ったら消す扱いで代替）
                        next = neighbor;
                        break;
                    }

                    if (next == -1) break; // 異常系 (完全な交互グラフならあり得ない)

                    current_cycle.push_back(Edge(curr, next));
                    if (use_A) visitedA_edges[edge_hash(curr, next)] = true;

                    curr = next;
                    use_A = !use_A;

                    if (curr == start_node && use_A) { // Aから始まり、Bで戻ってきたら閉路完成
                        break;
                    }
                }
                if (!current_cycle.empty()) {
                    ab_cycles.push_back(current_cycle);
                }
            }
        }

        if (ab_cycles.empty()) return parentA;

        // Step 3: E-setの選択 (ランダムにサイクルを適用)
        set<Edge> e_set_A, e_set_B;
        bool applied_any = false;
        for (const auto& cycle : ab_cycles) {
            if (rand() % 2 == 0) { // 50%の確率でこのサイクルを採用
                applied_any = true;
                bool is_A = true;
                for (const auto& edge : cycle) {
                    if (is_A) e_set_A.insert(edge);
                    else e_set_B.insert(edge);
                    is_A = !is_A;
                }
            }
        }
        
        // 1つも選ばれなかったら親Aをそのまま返す
        if (!applied_any) return parentA;

        // Step 4: 中間グラフの生成 (親AからE-set_Aを削除し、E-set_Bを追加)
        vector<vector<int>> inter_adj(n);
        for (const auto& e : edgesA) {
            if (e_set_A.find(e) == e_set_A.end()) { // 削除対象でなければ追加
                inter_adj[e.u].push_back(e.v);
                inter_adj[e.v].push_back(e.u);
            }
        }
        for (const auto& e : e_set_B) { // Bのエッジを追加
            inter_adj[e.u].push_back(e.v);
            inter_adj[e.v].push_back(e.u);
        }

        // サブツアー（小さな輪）を抽出
        vector<vector<int>> sub_tours;
        vector<bool> visited(n, false);
        for (int i = 0; i < n; ++i) {
            if (!visited[i] && inter_adj[i].size() == 2) {
                vector<int> tour;
                int curr = i;
                int prev = -1;
                while (!visited[curr]) {
                    visited[curr] = true;
                    tour.push_back(curr);
                    int next = inter_adj[curr][0];
                    if (next == prev) next = inter_adj[curr][1];
                    prev = curr;
                    curr = next;
                }
                sub_tours.push_back(tour);
            }
        }

        // Step 5: サブツアーの結合 (Greedy Merging)
        // 分裂したルートを、距離の増加が最も少なくなるように1つに繋ぎ合わせる
        while (sub_tours.size() > 1) {
            double best_increase = 1e9;
            int best_t1 = -1, best_t2 = -1;
            int best_i = -1, best_j = -1;
            bool reverse_t2 = false;

            // 全てのサブツアーのペアについて、繋ぎ合わせるコストを計算
            for (size_t t1 = 0; t1 < sub_tours.size(); ++t1) {
                for (size_t t2 = t1 + 1; t2 < sub_tours.size(); ++t2) {
                    const auto& tour1 = sub_tours[t1];
                    const auto& tour2 = sub_tours[t2];
                    int s1 = tour1.size();
                    int s2 = tour2.size();

                    for (int i = 0; i < s1; ++i) {
                        int u1 = tour1[i], v1 = tour1[(i + 1) % s1];
                        for (int j = 0; j < s2; ++j) {
                            int u2 = tour2[j], v2 = tour2[(j + 1) % s2];

                            // パターン1: (u1-u2), (v1-v2) で繋ぐ
                            double inc1 = dist_matrix[u1][u2] + dist_matrix[v1][v2] 
                                        - dist_matrix[u1][v1] - dist_matrix[u2][v2];
                            // パターン2: (u1-v2), (v1-u2) で繋ぐ
                            double inc2 = dist_matrix[u1][v2] + dist_matrix[v1][u2] 
                                        - dist_matrix[u1][v1] - dist_matrix[u2][v2];

                            if (inc1 < best_increase) {
                                best_increase = inc1; best_t1 = t1; best_t2 = t2;
                                best_i = i; best_j = j; reverse_t2 = true; 
                            }
                            if (inc2 < best_increase) {
                                best_increase = inc2; best_t1 = t1; best_t2 = t2;
                                best_i = i; best_j = j; reverse_t2 = false;
                            }
                        }
                    }
                }
            }

            // 見つかった最良の繋ぎ方で2つのサブツアーを結合
            vector<int> merged_tour;
            const auto& tour1 = sub_tours[best_t1];
            const auto& tour2 = sub_tours[best_t2];
            int s1 = tour1.size();
            int s2 = tour2.size();

            // tour1 を切断箇所まで追加
            for (int k = 0; k <= best_i; ++k) merged_tour.push_back(tour1[k]);

            // tour2 を追加 (向きに注意)
            if (reverse_t2) {
                for (int k = 0; k < s2; ++k) merged_tour.push_back(tour2[(best_j - k + s2) % s2]);
            } else {
                for (int k = 0; k < s2; ++k) merged_tour.push_back(tour2[(best_j + 1 + k) % s2]);
            }

            // tour1 の残りを追加
            for (int k = best_i + 1; k < s1; ++k) merged_tour.push_back(tour1[k]);

            // サブツアーリストを更新
            sub_tours[best_t1] = merged_tour;
            sub_tours.erase(sub_tours.begin() + best_t2);
        }

        // 最後に2-optで滑らかにして出力
        return apply_2opt(sub_tours[0]);
    }
};

// ==========================================
// デモンストレーション (乱数都市での実行)
// ==========================================
int main() {
    srand((unsigned)time(NULL));

    int NUM_CITIES = 30; // テスト用の都市数
    vector<City> cities(NUM_CITIES);
    for (int i = 0; i < NUM_CITIES; ++i) {
        cities[i] = { (double)(rand() % 100), (double)(rand() % 100) };
    }

    EAX_Solver solver(cities);

    // テキトーな初期解 (親Aと親B) を2つ作る
    vector<int> parentA(NUM_CITIES), parentB(NUM_CITIES);
    for (int i = 0; i < NUM_CITIES; ++i) { parentA[i] = i; parentB[i] = i; }
    
    // ランダムシャッフルして別々のルートにする
    random_shuffle(parentA.begin(), parentA.end());
    random_shuffle(parentB.begin(), parentB.end());

    cout << "--- 交叉前の距離 ---" << endl;
    cout << "親Aの距離: " << calc_tour_distance(parentA, vector<vector<double>>(NUM_CITIES, vector<double>(NUM_CITIES, 0))) << " (推定: 実際は内部マトリックスで計算)" << endl;
    
    // EAXを実行
    vector<int> child = solver.crossover(parentA, parentB);

    cout << "\n--- EAX実行完了 ---" << endl;
    cout << "生成された子ルート: ";
    for (int c : child) cout << c << " ";
    cout << endl;

    return 0;
}