#ifndef GA_HPP
#define GA_HPP

#include "tsp.hpp"
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <set>

// Indivíduo na população do GA
struct Individual
{
    Rota rota;
    double fitness; // comprimento da rota (menor é melhor)

    Individual() : fitness(std::numeric_limits<double>::max()) {}
    Individual(const Rota &t, double f) : rota(t), fitness(f) {}

    bool operator<(const Individual &other) const
    {
        return fitness < other.fitness; // para ordenação (melhor primeiro)
    }
};

// Parâmetros do Algoritmo Genético
struct GAConfig
{
    int tam_populacao = 200;
    int num_epocas = 500;
    double taxa_mutacao = 0.05;
    int tam_torneio = 3;
    int quant_alpha = 2;
    int paciencia = 100;

    enum SelectionType
    {
        TORNEIO,
        ROLETA
    };
    SelectionType selection = TORNEIO;

    enum CrossoverType
    {
        OX,
        PMX
    };
    CrossoverType crossover = OX;

    int seed = 42;
};

// Algoritmo Genético para TSP
class GeneticAlgorithm
{
protected:
    const InstanciaTSP &tsp;
    GAConfig config;
    std::mt19937 rng;

    std::vector<Individual> populacao;
    std::vector<double> melhor_por_epoca;
    std::vector<double> media_por_epoca;
    std::vector<double> pior_por_epoca;

    Individual melhor_todos;
    int generations_without_improvement;

public:
    GeneticAlgorithm(const InstanciaTSP &tsp_instance, const GAConfig &cfg)
        : tsp(tsp_instance), config(cfg), rng(cfg.seed),
          generations_without_improvement(0) {}

    // Inicializa população com rotas aleatórias
    void initPop()
    {
        populacao.clear();
        populacao.reserve(config.tam_populacao);

        for (int i = 0; i < config.tam_populacao; ++i)
        {
            Rota rota = tsp.randomRota(rng);
            double fitness = tsp.compRota(rota);
            populacao.emplace_back(rota, fitness);
        }

        // Encontra o melhor inicial (com validação)
        if (!populacao.empty())
        {
            melhor_todos = *std::min_element(populacao.begin(), populacao.end());
        }
    }

    // Seleção por torneio
    // Seleciona k indivíduos aleatórios e retorna o melhor
    Individual selecaoTorneio()
    {
        std::uniform_int_distribution<int> dist(0, populacao.size() - 1);

        Individual melhor = populacao[dist(rng)];
        for (int i = 1; i < config.tam_torneio; ++i)
        {
            Individual candidate = populacao[dist(rng)];
            if (candidate.fitness < melhor.fitness)
            {
                melhor = candidate;
            }
        }
        return melhor;
    }

    // Seleção por roleta (proporcional à fitness)
    // Para TSP (minimização), usa fitness inversa
    Individual selecaoRoleta()
    {
        // Calcula fitness inversa (já que minimizamos a distância)
        std::vector<double> inverse_fitness(populacao.size());
        double max_fitness = std::max_element(populacao.begin(), populacao.end())->fitness;
        double total = 0.0;

        for (size_t i = 0; i < populacao.size(); ++i)
        {
            // Inverse fitness: max - fitness + 1 (evita valores negativos)
            inverse_fitness[i] = max_fitness - populacao[i].fitness + 1.0;
            total += inverse_fitness[i];
        }

        // Gira a roleta
        std::uniform_real_distribution<double> dist(0.0, total);
        double spin = dist(rng);
        double cumulative = 0.0;

        for (size_t i = 0; i < populacao.size(); ++i)
        {
            cumulative += inverse_fitness[i];
            if (cumulative >= spin)
            {
                return populacao[i];
            }
        }
        return populacao.back();
    }

    // Seleciona pai com base no método de seleção configurado
    Individual selecaoParente()
    {
        if (config.selection == GAConfig::TORNEIO)
        {
            return selecaoTorneio();
        }
        else
        {
            return selecaoRoleta();
        }
    }

    // Crossover Ordenado (OX)
    // Preserva a ordem relativa das pontos de um pai
    Rota crossoverOrdenado(const Rota &parente1, const Rota &parente2)
    {
        int n = parente1.size();
        std::uniform_int_distribution<int> dist(0, n - 1);

        int start = dist(rng);
        int end = dist(rng);
        if (start > end)
            std::swap(start, end);

        Rota child(n, -1);

        // Copia segmento do parente1
        for (int i = start; i <= end; ++i)
        {
            child[i] = parente1[i];
        }

        // Preenche posições restantes com pontos do parente2 em ordem
        int child_pos = (end + 1) % n;
        for (int i = 0; i < n; ++i)
        {
            int parente2_pos = (end + 1 + i) % n;
            int pto = parente2[parente2_pos];

            // Verifica se a ponto já está no filho (em TODO o vetor)
            bool found = std::find(child.begin(), child.end(), pto) != child.end();

            if (!found)
            {
                child[child_pos] = pto;
                child_pos = (child_pos + 1) % n;
            }
        }

        return child;
    }

    // Crossover Mapeado Parcialmente (PMX)
    // Mapeia um segmento entre parentes e preenche o resto
    Rota crossoverParcialmenteMapeado(const Rota &parente1, const Rota &parente2)
    {
        int n = parente1.size();
        std::uniform_int_distribution<int> dist(0, n - 1);

        int start = dist(rng);
        int end = dist(rng);
        if (start > end)
            std::swap(start, end);

        Rota child = parente1;

        // Cria mapeamento do segmento parente1 para parente2
        std::vector<int> mapeamento(n, -1);
        for (int i = start; i <= end; ++i)
        {
            mapeamento[parente1[i]] = parente2[i];
            child[i] = parente2[i];
        }

        // Preenche posições fora do segmento
        for (int i = 0; i < n; ++i)
        {
            if (i >= start && i <= end)
                continue;

            int pto = parente1[i];
            // Segue a cadeia de mapeamento até encontrar uma ponto não no segmento
            while (mapeamento[pto] != -1)
            {
                pto = mapeamento[pto];
            }
            child[i] = pto;
        }

        return child;
    }

    // Crossover baseado no tipo configurado
    Rota crossover(const Rota &parente1, const Rota &parente2)
    {
        if (config.crossover == GAConfig::OX)
        {
            return crossoverOrdenado(parente1, parente2);
        }
        else
        {
            return crossoverParcialmenteMapeado(parente1, parente2);
        }
    }

    // Mutação por troca: troca duas posições aleatórias
    void mutate(Rota &rota)
    {
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

        if (prob_dist(rng) < config.taxa_mutacao)
        {
            std::uniform_int_distribution<int> pos_dist(0, rota.size() - 1);
            int pos1 = pos_dist(rng);
            int pos2 = pos_dist(rng);
            std::swap(rota[pos1], rota[pos2]);
        }
    }

    // Evolui população por uma geração
    void evoluir()
    {
        std::vector<Individual> new_populacao;
        new_populacao.reserve(config.tam_populacao);

        // Elitismo: mantém os melhores indivíduos
        std::sort(populacao.begin(), populacao.end());
        for (int i = 0; i < config.quant_alpha && i < static_cast<int>(populacao.size()); ++i)
        {
            new_populacao.push_back(populacao[i]);
        }

        // Gera descendentes
        while (new_populacao.size() < static_cast<size_t>(config.tam_populacao))
        {
            Individual parente1 = selecaoParente();
            Individual parente2 = selecaoParente();

            Rota child = crossover(parente1.rota, parente2.rota);
            mutate(child);

            double fitness = tsp.compRota(child);
            new_populacao.emplace_back(child, fitness);
        }

        populacao = std::move(new_populacao);

        // Atualiza o melhor de todos
        Individual current_best = *std::min_element(populacao.begin(), populacao.end());
        if (current_best.fitness < melhor_todos.fitness)
        {
            melhor_todos = current_best;
            generations_without_improvement = 0;
        }
        else
        {
            generations_without_improvement++;
        }
    }

    // Executa GA pelas épocas configuradas
    void run()
    {
        initPop();

        for (int epocas = 0; epocas < config.num_epocas; ++epocas)
        {
            evoluir();

            // Rastreia estatísticas
            std::sort(populacao.begin(), populacao.end());
            double melhor = populacao[0].fitness;
            double pior = populacao.back().fitness;
            double soma = 0.0;
            for (const auto &ind : populacao)
            {
                soma += ind.fitness;
            }
            double media = soma / populacao.size();

            melhor_por_epoca.push_back(melhor);
            media_por_epoca.push_back(media);
            pior_por_epoca.push_back(pior);

            // Verifica paciência (parada antecipada)
            if (generations_without_improvement >= config.paciencia)
            {
                // Corta vetores para as épocas realmente executadas
                melhor_por_epoca.resize(epocas + 1);
                media_por_epoca.resize(epocas + 1);
                pior_por_epoca.resize(epocas + 1);
                break;
            }
        }
    }

    // Métodos de acesso
    const Individual &getMelhorTodos() const { return melhor_todos; }
    const std::vector<double> &getMelhorPorEpoca() const { return melhor_por_epoca; }
    const std::vector<double> &getMediaPorEpoca() const { return media_por_epoca; }
    const std::vector<double> &getPiorPorEpoca() const { return pior_por_epoca; }
    const std::vector<Individual> &getPopulacao() const { return populacao; }
    int getMelhorEpocaAtual() const { return melhor_por_epoca.size(); }
};

#endif // GA_HPP
