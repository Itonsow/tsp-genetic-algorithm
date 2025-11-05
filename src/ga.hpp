#ifndef GA_HPP
#define GA_HPP

#include "tsp.hpp"
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <set>

// Indivíduo na população do GA
struct Individual {
    Tour tour;
    double fitness; // comprimento da rota (menor é melhor)
    
    Individual() : fitness(std::numeric_limits<double>::max()) {}
    Individual(const Tour& t, double f) : tour(t), fitness(f) {}
    
    bool operator<(const Individual& other) const {
        return fitness < other.fitness; // para ordenação (melhor primeiro)
    }
};

// Parâmetros do Algoritmo Genético
struct GAConfig {
    int population_size = 200;
    int num_epochs = 500;
    double mutation_rate = 0.05;
    int torneio_size = 3;
    int alpha_count = 2;
    int paciencia = 100;
    
    enum SelectionType { TORNEIO, ROULETTE };
    SelectionType selection = TORNEIO;
    
    enum CrossoverType { OX, PMX };
    CrossoverType crossover = OX;
    
    int seed = 42;
};

// Algoritmo Genético para TSP
class GeneticAlgorithm {
protected:
    const TSPInstance& tsp;
    GAConfig config;
    std::mt19937 rng;
    
    std::vector<Individual> population;
    std::vector<double> melhor_por_epoca;
    std::vector<double> media_por_epoca;
    std::vector<double> pior_por_epoca;
    
    Individual best_ever;
    int generations_without_improvement;
    
public:
    GeneticAlgorithm(const TSPInstance& tsp_instance, const GAConfig& cfg)
        : tsp(tsp_instance), config(cfg), rng(cfg.seed), 
          generations_without_improvement(0) {}
    
    // Inicializa população com rotas aleatórias
    void initializePopulation() {
        population.clear();
        population.reserve(config.population_size);
        
        for (int i = 0; i < config.population_size; ++i) {
            Tour tour = tsp.randomTour(rng);
            double fitness = tsp.calculateTourLength(tour);
            population.emplace_back(tour, fitness);
        }
        
        // Encontra o melhor inicial (com validação)
        if (!population.empty()) {
            best_ever = *std::min_element(population.begin(), population.end());
        }
    }
    
    // Seleção por torneio
    // Seleciona k indivíduos aleatórios e retorna o melhor
    Individual tournamentSelection() {
        std::uniform_int_distribution<int> dist(0, population.size() - 1);
        
        Individual melhor = population[dist(rng)];
        for (int i = 1; i < config.torneio_size; ++i) {
            Individual candidate = population[dist(rng)];
            if (candidate.fitness < melhor.fitness) {
                melhor = candidate;
            }
        }
        return melhor;
    }
    
    // Seleção por roleta (proporcional à fitness)
    // Para TSP (minimização), usa fitness inversa
    Individual rouletteSelection() {
        // Calcula fitness inversa (já que minimizamos a distância)
        std::vector<double> inverse_fitness(population.size());
        double max_fitness = std::max_element(population.begin(), population.end())->fitness;
        double total = 0.0;
        
        for (size_t i = 0; i < population.size(); ++i) {
            // Inverse fitness: max - fitness + 1 (evita valores negativos)
            inverse_fitness[i] = max_fitness - population[i].fitness + 1.0;
            total += inverse_fitness[i];
        }
        
        // Gira a roleta
        std::uniform_real_distribution<double> dist(0.0, total);
        double spin = dist(rng);
        double cumulative = 0.0;
        
        for (size_t i = 0; i < population.size(); ++i) {
            cumulative += inverse_fitness[i];
            if (cumulative >= spin) {
                return population[i];
            }
        }
        return population.back();
    }
    
    // Seleciona pai com base no método de seleção configurado
    Individual selectParent() {
        if (config.selection == GAConfig::TORNEIO) {
            return tournamentSelection();
        } else {
            return rouletteSelection();
        }
    }
    
    // Crossover Ordenado (OX)
    // Preserva a ordem relativa das cidades de um pai
    Tour orderedCrossover(const Tour& parent1, const Tour& parent2) {
        int n = parent1.size();
        std::uniform_int_distribution<int> dist(0, n - 1);
        
        int start = dist(rng);
        int end = dist(rng);
        if (start > end) std::swap(start, end);
        
        Tour child(n, -1);
        
        // Copia segmento do parent1
        for (int i = start; i <= end; ++i) {
            child[i] = parent1[i];
        }
        
        // Preenche posições restantes com cidades do parent2 em ordem
        int child_pos = (end + 1) % n;
        for (int i = 0; i < n; ++i) {
            int parent2_pos = (end + 1 + i) % n;
            int city = parent2[parent2_pos];
            
            // Verifica se a cidade já está no filho (em TODO o vetor)
            bool found = std::find(child.begin(), child.end(), city) != child.end();
            
            if (!found) {
                child[child_pos] = city;
                child_pos = (child_pos + 1) % n;
            }
        }
        
        return child;
    }
    
    // Crossover Mapeado Parcialmente (PMX)
    // Mapeia um segmento entre pais e preenche o resto
    Tour partiallyMappedCrossover(const Tour& parent1, const Tour& parent2) {
        int n = parent1.size();
        std::uniform_int_distribution<int> dist(0, n - 1);
        
        int start = dist(rng);
        int end = dist(rng);
        if (start > end) std::swap(start, end);
        
        Tour child = parent1;
        
        // Cria mapeamento do segmento parent1 para parent2
        std::vector<int> mapping(n, -1);
        for (int i = start; i <= end; ++i) {
            mapping[parent1[i]] = parent2[i];
            child[i] = parent2[i];
        }
        
        // Preenche posições fora do segmento
        for (int i = 0; i < n; ++i) {
            if (i >= start && i <= end) continue;
            
            int city = parent1[i];
            // Segue a cadeia de mapeamento até encontrar uma cidade não no segmento
            while (mapping[city] != -1) {
                city = mapping[city];
            }
            child[i] = city;
        }
        
        return child;
    }
    
    // Crossover baseado no tipo configurado
    Tour crossover(const Tour& parent1, const Tour& parent2) {
        if (config.crossover == GAConfig::OX) {
            return orderedCrossover(parent1, parent2);
        } else {
            return partiallyMappedCrossover(parent1, parent2);
        }
    }
    
    // Mutação por troca: troca duas posições aleatórias
    void mutate(Tour& tour) {
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
        
        if (prob_dist(rng) < config.mutation_rate) {
            std::uniform_int_distribution<int> pos_dist(0, tour.size() - 1);
            int pos1 = pos_dist(rng);
            int pos2 = pos_dist(rng);
            std::swap(tour[pos1], tour[pos2]);
        }
    }
    
    // Evolui população por uma geração
    void evolve() {
        std::vector<Individual> new_population;
        new_population.reserve(config.population_size);
        
        // Elitismo: mantém os melhores indivíduos
        std::sort(population.begin(), population.end());
        for (int i = 0; i < config.alpha_count && i < static_cast<int>(population.size()); ++i) {
            new_population.push_back(population[i]);
        }
        
        // Gera descendentes
        while (new_population.size() < static_cast<size_t>(config.population_size)) {
            Individual parent1 = selectParent();
            Individual parent2 = selectParent();
            
            Tour child = crossover(parent1.tour, parent2.tour);
            mutate(child);
            
            double fitness = tsp.calculateTourLength(child);
            new_population.emplace_back(child, fitness);
        }
        
        population = std::move(new_population);
        
        // Atualiza o melhor de todos
        Individual current_best = *std::min_element(population.begin(), population.end());
        if (current_best.fitness < best_ever.fitness) {
            best_ever = current_best;
            generations_without_improvement = 0;
        } else {
            generations_without_improvement++;
        }
    }
    
    // Executa GA pelas épocas configuradas
    void run() {
        initializePopulation();
        
        for (int epocas = 0; epocas < config.num_epochs; ++epocas) {
            evolve();
            
            // Rastreia estatísticas
            std::sort(population.begin(), population.end());
            double melhor = population[0].fitness;
            double pior = population.back().fitness;
            double sum = 0.0;
            for (const auto& ind : population) {
                sum += ind.fitness;
            }
            double media = sum / population.size();
            
            melhor_por_epoca.push_back(melhor);
            media_por_epoca.push_back(media);
            pior_por_epoca.push_back(pior);
            
            // Verifica paciência (parada antecipada)
            if (generations_without_improvement >= config.paciencia) {
                // Corta vetores para as épocas realmente executadas
                melhor_por_epoca.resize(epocas + 1);
                media_por_epoca.resize(epocas + 1);
                pior_por_epoca.resize(epocas + 1);
                break;
            }
        }
    }
    
    // Métodos de acesso
    const Individual& getBestEver() const { return best_ever; }
    const std::vector<double>& getBestPerEpoch() const { return melhor_por_epoca; }
    const std::vector<double>& getMeanPerEpoch() const { return media_por_epoca; }
    const std::vector<double>& getWorstPerEpoch() const { return pior_por_epoca; }
    const std::vector<Individual>& getPopulation() const { return population; }
    int getActualEpochs() const { return melhor_por_epoca.size(); }
};

#endif // GA_HPP
