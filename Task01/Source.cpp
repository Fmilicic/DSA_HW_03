#include <SFML/Graphics.hpp>
#include <vector>
#include <queue>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <optional>
#include <iostream>

static constexpr int GRID_SIZE = 40;
static constexpr int CELL_SIZE = 16;
static constexpr int WINDOW_SIZE = GRID_SIZE * CELL_SIZE;
static constexpr int INF = std::numeric_limits<int>::max();

static constexpr int dx[4] = { 1, -1,  0,  0 };
static constexpr int dy[4] = { 0,  0,  1, -1 };

inline int idx(int x, int y) { return y * GRID_SIZE + x; }

enum class State { Picking, Searching, Animating, Error };

int main()
{
    std::srand(unsigned(std::time(nullptr)));

    std::vector<int> grid(GRID_SIZE * GRID_SIZE, 0);
    for (int i = 0, N = GRID_SIZE * GRID_SIZE / 3; i < N; ++i) {
        int x = std::rand() % GRID_SIZE;
        int y = std::rand() % GRID_SIZE;
        grid[idx(x, y)] = 1;
    }

    sf::RenderWindow window(sf::VideoMode(
        { WINDOW_SIZE, WINDOW_SIZE }),
        "Dijkstra Pathfinding (Animated)",
        sf::Style::Default
    );
    window.setFramerateLimit(60);

    State state = State::Picking;
    std::vector<sf::Vector2i> picks;
    std::vector<int> dist, prev;
    std::vector<int> path_cells; 
    int animation_index = 0, frame_count = 0;
    const int animation_delay = 8;

    while (window.isOpen())
    {
        while (auto optEv = window.pollEvent())
        {
            auto& ev = *optEv;
            if (ev.is<sf::Event::Closed>()) {
                window.close();
                break;
            }

            switch (state)
            {
            case State::Picking:
                if (auto* mb = ev.getIf<sf::Event::MouseButtonPressed>())
                {
                    if (mb->button == sf::Mouse::Button::Left)
                    {
                        sf::Vector2i pixel{ mb->position.x, mb->position.y };
                        sf::Vector2f world = window.mapPixelToCoords(pixel);
                        int gx = std::clamp(int(world.x / CELL_SIZE), 0, GRID_SIZE - 1);
                        int gy = std::clamp(int(world.y / CELL_SIZE), 0, GRID_SIZE - 1);
                        picks.emplace_back(gx, gy);
                        grid[idx(gx, gy)] = 0;
                        if (picks.size() == 2)
                            state = State::Searching;
                    }
                }
                break;

            case State::Error:
                if (ev.is<sf::Event::MouseButtonPressed>() ||
                    ev.is<sf::Event::KeyPressed>())
                {
                    picks.clear();
                    state = State::Picking;
                }
                break;

            default:
                break;
            }
        }

        if (state == State::Searching)
        {
            int total = GRID_SIZE * GRID_SIZE;
            dist.assign(total, INF);
            prev.assign(total, -1);
            int start = idx(picks[0].x, picks[0].y);
            int goal = idx(picks[1].x, picks[1].y);
            dist[start] = 0;

            using Node = std::pair<int, int>;
            auto cmp = [](auto& a, auto& b) { return a.first > b.first; };
            std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
            pq.push({ 0, start });

            while (!pq.empty())
            {
                auto [d, u] = pq.top(); pq.pop();
                if (d > dist[u]) continue;
                if (u == goal) break;

                int ux = u % GRID_SIZE, uy = u / GRID_SIZE;
                for (int k = 0; k < 4; ++k)
                {
                    int vx = ux + dx[k], vy = uy + dy[k];
                    if (vx < 0 || vy < 0 || vx >= GRID_SIZE || vy >= GRID_SIZE) continue;
                    int v = idx(vx, vy);
                    if (grid[v] == 1) continue;
                    if (dist[v] > d + 1)
                    {
                        dist[v] = d + 1;
                        prev[v] = u;
                        pq.push({ dist[v], v });
                    }
                }
            }

            if (dist[goal] == INF)
            {
                std::cout << "No path exists! Click or press any key to retry.\n";
                state = State::Error;
            }
            else
            {
                path_cells.clear();
                for (int cur = goal; cur != -1; cur = prev[cur])
                    path_cells.push_back(cur);
                std::reverse(path_cells.begin(), path_cells.end());

                animation_index = 0;
                frame_count = 0;
                state = State::Animating;
            }
        }

        window.clear();

        sf::RectangleShape cell({ float(CELL_SIZE - 1), float(CELL_SIZE - 1) });
        for (int y = 0; y < GRID_SIZE; ++y)
        {
            for (int x = 0; x < GRID_SIZE; ++x)
            {
                int i = idx(x, y);
                if (state != State::Error && picks.size() > 0 && picks[0] == sf::Vector2i(x, y))
                    cell.setFillColor(sf::Color::Green);
                else if (state != State::Error && picks.size() > 1 && picks[1] == sf::Vector2i(x, y))
                    cell.setFillColor(sf::Color::Blue);
                else if (grid[i] == 1)
                    cell.setFillColor({ 50,50,50 });
                else
                    cell.setFillColor(sf::Color::White);

                if (state == State::Animating)
                {
                    for (int k = 0; k < animation_index; ++k)
                    {
                        if (path_cells[k] == i)
                        {
                            cell.setFillColor(sf::Color::Yellow);
                            break;
                        }
                    }
                }

                cell.setPosition(sf::Vector2f{ float(x * CELL_SIZE), float(y * CELL_SIZE) });
                window.draw(cell);
            }
        }

        if (state == State::Error)
        {
            sf::RectangleShape overlay({ float(WINDOW_SIZE), float(WINDOW_SIZE) });
            overlay.setFillColor({ 255, 0, 0, 100 });
            window.draw(overlay);
        }

        window.display();

        if (state == State::Animating)
        {
            if (++frame_count >= animation_delay &&
                animation_index < int(path_cells.size()))
            {
                ++animation_index;
                frame_count = 0;
            }
        }
    }

    return 0;
}
