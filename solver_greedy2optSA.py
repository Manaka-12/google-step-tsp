#!/usr/bin/env python3
#https://qiita.com/take314/items/7eae18045e989d7eaf52
import sys
import math
import random
from common import print_tour, read_input

# input 都市の座標、[(x0,y0),(x1,y1)...]
# output 都市を見る順番 [0,1,...]

def solve(cities):
    # 2都市の距離を計算する
    def culc_distance(city0_index, city1_index): 
        city0 = cities[city0_index]
        city1 = cities[city1_index]
        return math.sqrt((city0[0] - city1[0])**2 + (city0[1] - city1[1])**2)

    # ツアーの総移動距離を計算する
    def calc_total_distance(tour_list):
        total_dist = 0
        n = len(tour_list)
        for i in range(n):
            total_dist += culc_distance(tour_list[i], tour_list[(i + 1) % n])
        return total_dist

    # 訪問していない都市の中から最短距離の都市を探す
    def find_mindis_city(city_index, visited):
        mindis = [float("inf"), -1]  
        for index in range(len(cities)):
            if not visited[index]:
                distance = culc_distance(city_index, index)
                if distance < mindis[0]:
                    mindis = [distance, index]
        return mindis[1]

    n_cities = len(cities)

    # ============ここからreedy============ #
    visited = [False] * n_cities
    tour = [0]
    current = 0
    visited[0] = True
    
    while len(tour) < n_cities:
        current = find_mindis_city(current, visited)
        tour.append(current)
        visited[current] = True

    # ============ここから焼きなまし法 (SA) + 2-opt ============ #
    n = len(tour)
    
    # 都市数が3以下の場合は交差しないのでそのまま返す
    if n < 4:
        return tour

    # パラメータ設定
    T = 10000.0
    cooling_rate = 0.9999 
    min_T = 0.001

    best_tour = list(tour)
    best_dist = calc_total_distance(tour)
    current_dist = best_dist

    iteration = 0

    # 温度が下がりきるまで1回ずつ試す
    while T > min_T:
        i = random.randint(0, n - 3)
        j = random.randint(i + 2, n - 1)
        
        a = tour[i]
        b = tour[i + 1]
        c = tour[j]
        d = tour[(j + 1) % n] 

        old_dist = culc_distance(a, b) + culc_distance(c, d)
        new_dist = culc_distance(a, c) + culc_distance(b, d)
        delta_dist = new_dist - old_dist

        if delta_dist < 0 or random.random() < math.exp(-delta_dist / T):
            tour[i + 1:j + 1] = reversed(tour[i + 1:j + 1])
            current_dist += delta_dist

            if current_dist < best_dist:
                best_dist = current_dist
                best_tour = list(tour)

        # 1回交換を試すたびに温度を下げる
        T *= cooling_rate
        iteration += 1

        # 1万回ループが回るごとに進捗を表示
        if iteration % 10000 == 0:
            print(f"進捗: 試行回数={iteration}, 現在の温度(T)={T:.4f}, ベスト距離={best_dist:.2f}", file=sys.stderr)

    return best_tour


if __name__ == '__main__':
    assert len(sys.argv) > 1
    tour = solve(read_input(sys.argv[1]))
    print_tour(tour)