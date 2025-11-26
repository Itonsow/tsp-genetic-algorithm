#ifndef TSP_HPP
#define TSP_HPP

#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

// Define constante PI (compatível com C++17)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Ponto no espaço 2D
struct Point
{
    double x, y;

    Point() : x(0), y(0) {}
    Point(double x_, double y_) : x(x_), y(y_) {}

    double distance(const Point &other) const
    {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::hypot(dx, dy);
    }
};

// Representação da rota TSP (permutação dos índices das pontos)
using Rota = std::vector<int>;

// Instância TSP com pontos/pontos
class InstanciaTSP
{
private:
    std::vector<Point> pontos;
    int seed;

public:
    InstanciaTSP() : seed(42) {}

    // Gera pontos aleatórias uniformes em [0,1] x [0,1]
    void generateUniform(int n, int random_seed)
    {
        seed = random_seed;
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        pontos.clear();
        pontos.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            pontos.emplace_back(dist(rng), dist(rng));
        }
    }

    // Gera pontos em um círculo (cenário de benchmark)
    // N pontos igualmente espaçados em um círculo de raio R
    void generateCircle(int n, double radius = 1.0, double start_angle = 0.0)
    {
        pontos.clear();
        pontos.reserve(n);

        for (int i = 0; i < n; ++i)
        {
            double angle = start_angle + 2.0 * M_PI * i / n;
            double x = 0.5 + radius * std::cos(angle);
            double y = 0.5 + radius * std::sin(angle);
            pontos.emplace_back(x, y);
        }
    }

    // Calcula o comprimento total da rota (fitness)
    // A rota é cíclica: visita todas as pontos e retorna ao início
    double compRota(const Rota &rota) const
    {
        if (rota.empty())
            return 0.0;

        double length = 0.0;
        for (size_t i = 0; i < rota.size(); ++i)
        {
            int from = rota[i];
            int to = rota[(i + 1) % rota.size()]; // cyclic
            length += pontos[from].distance(pontos[to]);
        }
        return length;
    }

    // Gera uma rota válida aleatória (permutação)
    Rota randomRota(std::mt19937 &rng) const
    {
        Rota rota(pontos.size());
        for (size_t i = 0; i < pontos.size(); ++i)
        {
            rota[i] = static_cast<int>(i);
        }
        std::shuffle(rota.begin(), rota.end(), rng);
        return rota;
    }

    // Verifica se a rota é uma permutação válida
    bool rotaValida(const Rota &rota) const
    {
        if (rota.size() != pontos.size())
            return false;

        std::vector<bool> visited(pontos.size(), false);
        for (int pto : rota)
        {
            if (pto < 0 || pto >= static_cast<int>(pontos.size()))
                return false;
            if (visited[pto])
                return false;
            visited[pto] = true;
        }
        return true;
    }

    // Métodos de acesso
    const std::vector<Point> &getPoints() const { return pontos; }
    int getSize() const { return static_cast<int>(pontos.size()); }
    int getSeed() const { return seed; }
    const Point &getPoint(int idx) const { return pontos[idx]; }
};

#endif // TSP_HPP
