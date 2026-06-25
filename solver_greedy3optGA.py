#!/usr/bin/env python3
import sys
import math
import random
import time
import os
import tempfile
from common import format_tour, print_tour, read_input

def _save_tour(path, tour):
    directory = os.path.dirname(path) or '.'
    with tempfile.NamedTemporaryFile('w', delete=False, dir=directory, encoding='utf-8', newline='\n') as tmp:
        tmp.write(format_tour(tour) + '\n')
        temp_name = tmp.name
    os.replace(temp_name, path)


def solve(cities, save_path=None):
    n_cities = len(cities)
    if n_cities <= 1:
        tour = list(range(n_cities))
        if save_path:
            _save_tour(save_path, tour)
        return tour
    if n_cities <= 3:
        tour = list(range(n_cities))
        if save_path:
            _save_tour(save_path, tour)
        return tour

    def checkpoint(tour):
        if save_path:
            _save_tour(save_path, tour)

    # ============ 1. 距離マトリックスの事前計算 ============
    dist_matrix = [[0.0] * n_cities for _ in range(n_cities)]
    for i in range(n_cities):
        for j in range(i + 1, n_cities):
            dx = cities[i][0] - cities[j][0]
            dy = cities[i][1] - cities[j][1]
            d = math.sqrt(dx**2 + dy**2)
            dist_matrix[i][j] = d
            dist_matrix[j][i] = d

    def calc_total_distance(tour_list):
        total_dist = 0.0
        for i in range(n_cities):
            total_dist += dist_matrix[tour_list[i]][tour_list[(i + 1) % n_cities]]
        return total_dist

    # ============ 2. 強力な局所探索 (2-opt) ============
    # 与えられたルートを、これ以上改善できなくなるまで完全に最適化します
    def complete_2opt(tour):
        tour = list(tour)
        improved = True
        while improved:
            improved = False
            for i in range(n_cities - 1):
                for j in range(i + 2, n_cities):
                    if i == 0 and j == n_cities - 1:
                        continue
                    a, b = tour[i], tour[i + 1]
                    c, d = tour[j], tour[(j + 1) % n_cities]

                    if dist_matrix[a][c] + dist_matrix[b][d] < dist_matrix[a][b] + dist_matrix[c][d] - 1e-9:
                        tour[i + 1:j + 1] = reversed(tour[i + 1:j + 1])
                        improved = True
        return tour

    # ============ 3. 初期解の生成 (多様な貪欲法) ============
    # 開始都市をランダムに変えて、異なる「質の良い初期解」を作ります
    def generate_greedy_tour(start_city):
        visited = [False] * n_cities
        tour = [start_city]
        visited[start_city] = True
        current = start_city
        
        while len(tour) < n_cities:
            min_dist = float('inf')
            nearest_city = -1
            for next_city in range(n_cities):
                if not visited[next_city]:
                    if dist_matrix[current][next_city] < min_dist:
                        min_dist = dist_matrix[current][next_city]
                        nearest_city = next_city
            tour.append(nearest_city)
            visited[nearest_city] = True
            current = nearest_city
        return complete_2opt(tour)

    # ============ 4. 遺伝的アルゴリズム (GA) の設定 ============
    # 時間制限がないため、集団を維持して進化させます
    POP_SIZE = min(30, n_cities) # 集団の個体数
    population = []
    
    print("初期集団（ベースとなる優秀なルート達）を生成中...", file=sys.stderr)
    # 異なる開始地点から貪欲法+2-optで優秀な初期個体を生成
    for k in range(POP_SIZE):
        start_node = random.randint(0, n_cities - 1)
        ind = generate_greedy_tour(start_node)
        dist = calc_total_distance(ind)
        population.append((dist, ind))
    
    # 距離が短い順にソート
    population.sort(key=lambda x: x[0])
    best_dist, best_tour = population[0]
    checkpoint(best_tour)

    print(f"初期ベスト距離: {best_dist:.4f}", file=sys.stderr)
    print("最適化を開始します。終了するには Ctrl+C を押してください。", file=sys.stderr)

    # ============ 5. メイン進化ループ ============
    generation = 0
    no_improvement_count = 0
    
    try:
        # 時間制限がないため、事実上の無限ループ（または収束するまで）
        while no_improvement_count < 5000: # 5000世代改善がなければ終了（お好みで調整）
            generation += 1
            
            # 親の選定（優秀な個体ほど選ばれやすいトーナメント選択）
            parent1 = min(random.sample(population, 3), key=lambda x: x[0])[1]
            parent2 = min(random.sample(population, 3), key=lambda x: x[0])[1]
            
            # 交叉 (Order Crossover: OX) 
            # 親1の一部を子にコピーし、残りの都市を親2の出現順で埋める
            cut1 = random.randint(0, n_cities - 2)
            cut2 = random.randint(cut1 + 1, n_cities - 1)
            
            child_tour = [-1] * n_cities
            child_tour[cut1:cut2] = parent1[cut1:cut2]
            
            p2_idx = 0
            for c_idx in range(n_cities):
                if child_tour[c_idx] == -1:
                    while parent2[p2_idx] in child_tour:
                        p2_idx += 1
                    child_tour[c_idx] = parent2[p2_idx]
            
            # 突然変異（時々ルートの一部をシャッフル）
            if random.random() < 0.2:
                m1 = random.randint(0, n_cities - 2)
                m2 = random.randint(m1 + 1, n_cities - 1)
                child_tour[m1:m2] = reversed(child_tour[m1:m2])
            
            # 子個体を2-optで限界までブラッシュアップ（ここが最大の肝です）
            child_tour = complete_2opt(child_tour)
            child_dist = calc_total_distance(child_tour)
            
            # 集団の一番成績の悪い個体（最も距離が長い個体）と比較
            if child_dist < population[-1][0]:
                # 重複チェック（全く同じ距離の個体がいなければ集団に入れる）
                if not any(abs(ind[0] - child_dist) < 1e-4 for ind in population):
                    population.pop() # 最悪個体を排除
                    population.append((child_dist, child_tour))
                    population.sort(key=lambda x: x[0])
                    
                    # 全体ベストの更新
                    if child_dist < best_dist - 1e-4:
                        best_dist = child_dist
                        best_tour = list(child_tour)
                        no_improvement_count = 0
                        checkpoint(best_tour)
                        print(f"世代 {generation}: ベスト距離更新 -> {best_dist:.4f}", file=sys.stderr)
                    else:
                        no_improvement_count += 1
                else:
                    no_improvement_count += 1
            else:
                no_improvement_count += 1

            # 定期的な進捗表示
            if generation % 500 == 0:
                print(f"世代 {generation} 経過... 現在のベスト: {best_dist:.4f}", file=sys.stderr)

    except KeyboardInterrupt:
        checkpoint(best_tour)
        print("\nユーザーによる中断を検知しました。これまでの最高解を出力します。", file=sys.stderr)

    print(f"最終確定ベスト距離: {best_dist:.4f}", file=sys.stderr)
    checkpoint(best_tour)
    return best_tour

if __name__ == '__main__':
    assert len(sys.argv) > 1
    tour = solve(read_input(sys.argv[1]))
    print_tour(tour)