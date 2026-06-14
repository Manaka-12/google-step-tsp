#!/usr/bin/env python3

import sys
import math
from common import print_tour, read_input

# input 都市の座標、[(x0,y0),(x1,y1)...]
# output 都市を見る順番 [0,1,...]

def solve(cities):
    # 2都市の距離を計算する
    def culc_distance(city0_index, city1_index): 
        city0 = cities[city0_index]
        city1 = cities[city1_index]
        distance = math.sqrt((city0[0] - city1[0])**2 + (city0[1] - city1[1])**2)
        return distance 

    # ツアーの総移動距離を計算する
    def calc_total_distance(tour_list):
        total_dist = 0
        for i in range(len(tour_list)):
            # 最後の都市から最初の都市への距離も加算する
            total_dist += culc_distance(tour_list[i], tour_list[(i + 1) % len(tour_list)])
        return total_dist

    # 訪問していない都市の中から最短距離の都市を探す
    def find_mindis_city(city_index, visited):
        # mindis = [最小距離, 最小距離を持つ都市のインデックス]
        mindis = [float("inf"), -1]  
        for index in range(len(cities)):
            if not visited[index]:
                distance = culc_distance(city_index, index)
                if distance < mindis[0]:
                    mindis = [distance, index]
        return mindis[1]

    # 最短のツアー経路を保存する
    best_tour = []
    min_total_distance = float("inf")
    n_cities = len(cities)

    # =============== すべての都市を始点として試す ===============
    for start_city in range(n_cities):
        # 変数の初期化。各始点ごとにリセット
        visited = [False] * n_cities
        tour = [start_city]
        current = start_city
        visited[start_city] = True
        
        # ============ ここから greedy ============ #
        while len(tour) < n_cities:
            current = find_mindis_city(current, visited)
            tour.append(current)
            visited[current] = True


        # Greedy完了後、この始点から作ったツアーの総距離を計算
        current_tour_distance = calc_total_distance(tour)

        # 今までの最小距離より短ければ、ベストを更新
        if current_tour_distance < min_total_distance:
            min_total_distance = current_tour_distance
            best_tour = list(tour)

    # すべての始点を試し終わったら、一番良かったツアーを返す
    return best_tour


if __name__ == '__main__':
    assert len(sys.argv) > 1
    tour = solve(read_input(sys.argv[1]))
    print_tour(tour)