#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

using namespace std;

struct City {
    double x;
    double y;
};

static vector<int> g_best_tour;
static string g_save_path;
static bool g_has_save_path = false;
static volatile sig_atomic_t g_interrupted = 0;

static void write_tour_file(const string& path, const vector<int>& tour) {
    const string temp_path = path + ".tmp";
    ofstream out(temp_path, ios::out | ios::trunc);
    out << "index\n";
    for (int city : tour) {
        out << city << '\n';
    }
    out.close();
    remove(path.c_str());
    rename(temp_path.c_str(), path.c_str());
}

static void checkpoint_best() {
    if (g_has_save_path && !g_best_tour.empty()) {
        write_tour_file(g_save_path, g_best_tour);
    }
}

static void on_sigint(int) {
    g_interrupted = 1;
}

static double calc_distance(const City& a, const City& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}

static double calc_total_distance(const vector<int>& tour, const vector<vector<double>>& dist_matrix) {
    double total = 0.0;
    const int n = static_cast<int>(tour.size());
    for (int i = 0; i < n; ++i) {
        total += dist_matrix[tour[i]][tour[(i + 1) % n]];
    }
    return total;
}

static bool read_tour_file(const string& path, int n_cities, vector<int>& tour) {
    ifstream in(path);
    if (!in) {
        return false;
    }

    string header;
    if (!getline(in, header) || header != "index") {
        return false;
    }

    vector<int> loaded;
    loaded.reserve(n_cities);
    vector<char> seen(n_cities, 0);
    string line;
    while (getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        int city = stoi(line);
        if (city < 0 || city >= n_cities || seen[city]) {
            return false;
        }
        seen[city] = 1;
        loaded.push_back(city);
    }

    if (static_cast<int>(loaded.size()) != n_cities) {
        return false;
    }

    tour = move(loaded);
    return true;
}

static vector<vector<int>> build_nearest_neighbors(const vector<vector<double>>& dist_matrix, int k) {
    const int n = static_cast<int>(dist_matrix.size());
    vector<vector<int>> neighbors(n);
    if (n <= 1) {
        return neighbors;
    }
    k = min(k, n - 1);
    for (int i = 0; i < n; ++i) {
        vector<pair<double, int>> dists;
        dists.reserve(n - 1);
        for (int j = 0; j < n; ++j) {
            if (i == j) {
                continue;
            }
            dists.push_back({dist_matrix[i][j], j});
        }
        nth_element(dists.begin(), dists.begin() + k, dists.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });
        dists.resize(k);
        sort(dists.begin(), dists.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });
        neighbors[i].reserve(k);
        for (const auto& entry : dists) {
            neighbors[i].push_back(entry.second);
        }
    }
    return neighbors;
}

static vector<int> complete_2opt_candidate(const vector<int>& original,
                                           const vector<vector<double>>& dist_matrix,
                                           const vector<vector<int>>& neighbors) {
    vector<int> tour = original;
    const int n = static_cast<int>(tour.size());
    vector<int> pos(n, 0);
    bool improved = true;
    while (improved) {
        if (g_interrupted) {
            break;
        }
        for (int i = 0; i < n; ++i) {
            pos[tour[i]] = i;
        }
        improved = false;
        for (int i = 0; i < n; ++i) {
            if (g_interrupted) {
                return tour;
            }
            const int a = tour[i];
            const int b = tour[(i + 1) % n];
            for (int c : neighbors[a]) {
                if (g_interrupted) {
                    return tour;
                }
                const int j = pos[c];
                if (j <= i + 1) {
                    continue;
                }
                if (j >= n) {
                    continue;
                }
                if (i == 0 && j == n - 1) {
                    continue;
                }
                const int c_city = tour[j];
                const int d = tour[(j + 1) % n];
                if (dist_matrix[a][c_city] + dist_matrix[b][d] < dist_matrix[a][b] + dist_matrix[c_city][d] - 1e-9) {
                    reverse(tour.begin() + i + 1, tour.begin() + j + 1);
                    improved = true;
                    break;
                }
            }
            if (improved) {
                break;
            }
        }
    }
    return tour;
}

static bool try_3opt_once(vector<int>& tour,
                          const vector<vector<double>>& dist_matrix,
                          const vector<vector<int>>& neighbors) {
    const int n = static_cast<int>(tour.size());
    vector<int> pos(n, 0);
    for (int i = 0; i < n; ++i) {
        pos[tour[i]] = i;
    }

    for (int i = 0; i < n - 4; ++i) {
        if (g_interrupted) {
            return false;
        }
        const int a = tour[i];
        const int b = tour[i + 1];

        for (int c : neighbors[a]) {
            const int j = pos[c];
            if (j <= i + 1 || j >= n - 2) {
                continue;
            }
            const int d = tour[j + 1];

            for (int e : neighbors[b]) {
                const int k = pos[e];
                if (k <= j + 1 || k >= n) {
                    continue;
                }
                if (i == 0 && k == n - 1) {
                    continue;
                }
                const int e_city = tour[k];
                const int f = tour[(k + 1) % n];

                const double old_cost = dist_matrix[a][b] + dist_matrix[c][d] + dist_matrix[e_city][f];
                const double new_cost = dist_matrix[a][c] + dist_matrix[b][e_city] + dist_matrix[d][f];
                if (new_cost < old_cost - 1e-9) {
                    reverse(tour.begin() + i + 1, tour.begin() + j + 1);
                    reverse(tour.begin() + j + 1, tour.begin() + k + 1);
                    return true;
                }
            }
        }
    }
    return false;
}

static vector<int> complete_3opt(const vector<int>& original,
                                 const vector<vector<double>>& dist_matrix,
                                 const vector<vector<int>>& neighbors) {
    vector<int> tour = complete_2opt_candidate(original, dist_matrix, neighbors);
    for (int pass = 0; pass < 3; ++pass) {
        if (g_interrupted) {
            break;
        }
        if (!try_3opt_once(tour, dist_matrix, neighbors)) {
            break;
        }
        tour = complete_2opt_candidate(tour, dist_matrix, neighbors);
    }
    return tour;
}

static vector<int> generate_greedy_tour(int start_city,
                                        const vector<vector<double>>& dist_matrix,
                                        const vector<vector<int>>& neighbors) {
    const int n = static_cast<int>(dist_matrix.size());
    vector<bool> visited(n, false);
    vector<int> tour;
    tour.reserve(n);
    tour.push_back(start_city);
    visited[start_city] = true;

    int current = start_city;
    while (static_cast<int>(tour.size()) < n) {
        if (g_interrupted) {
            break;
        }
        double min_dist = numeric_limits<double>::infinity();
        int nearest_city = -1;
        for (int next_city = 0; next_city < n; ++next_city) {
            if (!visited[next_city] && dist_matrix[current][next_city] < min_dist) {
                min_dist = dist_matrix[current][next_city];
                nearest_city = next_city;
            }
        }
        tour.push_back(nearest_city);
        visited[nearest_city] = true;
        current = nearest_city;
    }

    return complete_3opt(tour, dist_matrix, neighbors);
}

static vector<int> solve(const vector<City>& cities, const string& save_path = "") {
    const int n_cities = static_cast<int>(cities.size());
    if (n_cities <= 1) {
        vector<int> tour(n_cities);
        for (int i = 0; i < n_cities; ++i) {
            tour[i] = i;
        }
        if (!save_path.empty()) {
            write_tour_file(save_path, tour);
        }
        return tour;
    }
    if (n_cities <= 3) {
        vector<int> tour(n_cities);
        for (int i = 0; i < n_cities; ++i) {
            tour[i] = i;
        }
        if (!save_path.empty()) {
            write_tour_file(save_path, tour);
        }
        return tour;
    }

    g_save_path = save_path;
    g_has_save_path = !save_path.empty();

    vector<vector<double>> dist_matrix(n_cities, vector<double>(n_cities, 0.0));
    for (int i = 0; i < n_cities; ++i) {
        for (int j = i + 1; j < n_cities; ++j) {
            const double d = calc_distance(cities[i], cities[j]);
            dist_matrix[i][j] = d;
            dist_matrix[j][i] = d;
        }
    }

    const int neighbor_k = min(24, n_cities - 1);
    const vector<vector<int>> neighbors = build_nearest_neighbors(dist_matrix, neighbor_k);

    const int pop_size = min(30, n_cities);
    vector<pair<double, vector<int>>> population;
    population.reserve(pop_size);

    vector<int> resumed_tour;
    if (!save_path.empty() && read_tour_file(save_path, n_cities, resumed_tour)) {
        const double resumed_dist = calc_total_distance(resumed_tour, dist_matrix);
        population.push_back({resumed_dist, resumed_tour});
        cerr << "Resumed from existing tour: " << save_path << endl;
    }

    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> city_dist(0, n_cities - 1);
    uniform_real_distribution<double> prob_dist(0.0, 1.0);

    cerr << "Generating initial population..." << endl;
    while (static_cast<int>(population.size()) < pop_size) {
        const int start_node = city_dist(rng);
        vector<int> ind = generate_greedy_tour(start_node, dist_matrix, neighbors);
        const double dist = calc_total_distance(ind, dist_matrix);
        population.push_back({dist, ind});
    }

    sort(population.begin(), population.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    double best_dist = population.front().first;
    vector<int> best_tour = population.front().second;
    g_best_tour = best_tour;
    checkpoint_best();

    cerr << "Initial best distance: " << best_dist << endl;
    cerr << "Starting optimization. Press Ctrl+C to stop." << endl;

    int generation = 0;
    int no_improvement_count = 0;

    try {
        while (no_improvement_count < 5000 && !g_interrupted) {
            ++generation;

            vector<pair<double, vector<int>>> sample1;
            vector<pair<double, vector<int>>> sample2;
            sample1.reserve(3);
            sample2.reserve(3);
            for (int i = 0; i < 3; ++i) {
                sample1.push_back(population[city_dist(rng) % population.size()]);
                sample2.push_back(population[city_dist(rng) % population.size()]);
            }
            sort(sample1.begin(), sample1.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
            sort(sample2.begin(), sample2.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
            const vector<int>& parent1 = sample1[0].second;
            const vector<int>& parent2 = sample2[0].second;

            uniform_int_distribution<int> cut1_dist(0, n_cities - 2);
            const int cut1 = cut1_dist(rng);
            uniform_int_distribution<int> cut2_dist(cut1 + 1, n_cities - 1);
            const int cut2 = cut2_dist(rng);

            vector<int> child_tour(n_cities, -1);
            for (int i = cut1; i < cut2; ++i) {
                child_tour[i] = parent1[i];
            }

            vector<char> used(n_cities, 0);
            for (int i = cut1; i < cut2; ++i) {
                used[child_tour[i]] = 1;
            }

            int p2_idx = 0;
            for (int c_idx = 0; c_idx < n_cities; ++c_idx) {
                if (child_tour[c_idx] == -1) {
                    while (used[parent2[p2_idx]]) {
                        ++p2_idx;
                    }
                    child_tour[c_idx] = parent2[p2_idx];
                    used[parent2[p2_idx]] = 1;
                }
            }

            if (prob_dist(rng) < 0.2) {
                uniform_int_distribution<int> m1_dist(0, n_cities - 2);
                const int m1 = m1_dist(rng);
                uniform_int_distribution<int> m2_dist(m1 + 1, n_cities - 1);
                const int m2 = m2_dist(rng);
                reverse(child_tour.begin() + m1, child_tour.begin() + m2);
            }

            child_tour = complete_3opt(child_tour, dist_matrix, neighbors);
            const double child_dist = calc_total_distance(child_tour, dist_matrix);

            if (child_dist < population.back().first) {
                bool duplicate = false;
                for (const auto& ind : population) {
                    if (fabs(ind.first - child_dist) < 1e-4) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate) {
                    population.pop_back();
                    population.push_back({child_dist, child_tour});
                    sort(population.begin(), population.end(), [](const auto& lhs, const auto& rhs) {
                        return lhs.first < rhs.first;
                    });

                    if (child_dist < best_dist - 1e-4) {
                        best_dist = child_dist;
                        best_tour = child_tour;
                        g_best_tour = best_tour;
                        no_improvement_count = 0;
                        checkpoint_best();
                        cerr << "Generation " << generation << ": Best distance updated -> " << best_dist << endl;
                    } else {
                        ++no_improvement_count;
                    }
                } else {
                    ++no_improvement_count;
                }
            } else {
                ++no_improvement_count;
            }

            if (generation % 500 == 0) {
                cerr << "Generation " << generation << " ... Current best: " << best_dist << endl;
            }
        }
    } catch (...) {
        checkpoint_best();
        throw;
    }

    if (g_interrupted) {
        checkpoint_best();
    }
    checkpoint_best();
    cerr << "Final best distance: " << best_dist << endl;
    return best_tour;
}

static vector<City> read_input(const string& filename) {
    ifstream in(filename);
    vector<City> cities;
    string line;
    getline(in, line);
    while (getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const size_t comma = line.find(',');
        City city{};
        city.x = stod(line.substr(0, comma));
        city.y = stod(line.substr(comma + 1));
        cities.push_back(city);
    }
    return cities;
}

static void print_tour(const vector<int>& tour) {
    cout << "index\n";
    for (int city : tour) {
        cout << city << '\n';
    }
}

int main(int argc, char** argv) {
    signal(SIGINT, on_sigint);

    if (argc <= 1) {
        cerr << "Usage: " << argv[0] << " input.csv [output.csv]" << endl;
        return 1;
    }

    const string input_path = argv[1];
    const string output_path = (argc > 2) ? argv[2] : "";

    vector<City> cities = read_input(input_path);
    vector<int> tour = solve(cities, output_path);
    if (output_path.empty()) {
        print_tour(tour);
    }
    return 0;
}