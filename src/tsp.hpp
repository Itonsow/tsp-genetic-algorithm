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
struct Point {
    double x, y;
    
    Point() : x(0), y(0) {}
    Point(double x_, double y_) : x(x_), y(y_) {}
    
    // Euclidean distance to another point
    double distance(const Point& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::hypot(dx, dy);
    }
};

// Representação da rota TSP (permutação dos índices das cidades)
using Tour = std::vector<int>;

// Instância TSP com cidades/pontos
class TSPInstance {
private:
    std::vector<Point> cities;
    int seed;
    
public:
    TSPInstance() : seed(42) {}
    
    // Gera cidades aleatórias uniformes em [0,1] x [0,1]
    void generateUniform(int n, int random_seed) {
        seed = random_seed;
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        
        cities.clear();
        cities.reserve(n);
        for (int i = 0; i < n; ++i) {
            cities.emplace_back(dist(rng), dist(rng));
        }
    }
    
    // Gera cidades em um círculo (cenário de benchmark)
    // N pontos igualmente espaçados em um círculo de raio R
    void generateCircle(int n, double radius = 1.0, double start_angle = 0.0) {
        cities.clear();
        cities.reserve(n);
        
        for (int i = 0; i < n; ++i) {
            double angle = start_angle + 2.0 * M_PI * i / n;
            double x = 0.5 + radius * std::cos(angle);
            double y = 0.5 + radius * std::sin(angle);
            cities.emplace_back(x, y);
        }
    }
    
    // Calcula o comprimento total da rota (fitness)
    // A rota é cíclica: visita todas as cidades e retorna ao início
    double calculateTourLength(const Tour& tour) const {
        if (tour.empty()) return 0.0;
        
        double length = 0.0;
        for (size_t i = 0; i < tour.size(); ++i) {
            int from = tour[i];
            int to = tour[(i + 1) % tour.size()]; // cyclic
            length += cities[from].distance(cities[to]);
        }
        return length;
    }
    
    // Gera uma rota válida aleatória (permutação)
    Tour randomTour(std::mt19937& rng) const {
        Tour tour(cities.size());
        for (size_t i = 0; i < cities.size(); ++i) {
            tour[i] = static_cast<int>(i);
        }
        std::shuffle(tour.begin(), tour.end(), rng);
        return tour;
    }
    
    // Verifica se a rota é uma permutação válida
    bool isValidTour(const Tour& tour) const {
        if (tour.size() != cities.size()) return false;
        
        std::vector<bool> visited(cities.size(), false);
        for (int city : tour) {
            if (city < 0 || city >= static_cast<int>(cities.size())) return false;
            if (visited[city]) return false;
            visited[city] = true;
        }
        return true;
    }
    
    // Métodos de acesso
    const std::vector<Point>& getCities() const { return cities; }
    int getSize() const { return static_cast<int>(cities.size()); }
    int getSeed() const { return seed; }
    const Point& getCity(int idx) const { return cities[idx]; }
};

#endif // TSP_HPP
