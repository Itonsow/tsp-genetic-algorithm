#include "tsp.hpp"
#include "ga.hpp"
#include "plot_utils.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>

// Analisador simples de argumentos da linha de comando
struct Config {
    std::string scenario = "uniform";
    int n = 50;
    int epochs = 500;
    int pop = 200;
    double mut = 0.05;
    std::string selection = "tournament";
    int tournament = 3;
    std::string crossover = "ox";
    int elite = 2;
    int patience = 100;
    int seed = 42;
    std::string outdir = "./outputs";
    std::string framesdir = "./frames";
    bool check_mode = false;
    
    void print() const {
        std::cout << "\n=== Configuração do Algoritmo Genético TSP ===\n";
        std::cout << "Cenário:                " << scenario << "\n";
        std::cout << "Número de cidades:      " << n << "\n";
        std::cout << "Épocas:                 " << epochs << "\n";
        std::cout << "Tamanho da população:   " << pop << "\n";
        std::cout << "Taxa de mutação:        " << mut << "\n";
        std::cout << "Seleção:                " << selection << "\n";
        if (selection == "tournament") {
            std::cout << "Tamanho do torneio:     " << tournament << "\n";
        }
        std::cout << "Crossover:              " << crossover << "\n";
        std::cout << "Contagem de elite:      " << elite << "\n";
        std::cout << "Paciência:              " << patience << "\n";
        std::cout << "Semente aleatória:      " << seed << "\n";
        std::cout << "Diretório de saída:     " << outdir << "\n";
        std::cout << "Diretório de quadros:   " << framesdir << "\n";
        std::cout << "==============================================\n\n";
    }
};

void printUsage(const char* program) {
    std::cout << "Uso: " << program << " [OPÇÕES]\n\n";
    std::cout << "Opções:\n";
    std::cout << "  --scenario {uniform|circle}  Tipo de cenário (padrão: uniform)\n";
    std::cout << "  --n <int>                    Número de cidades (padrão: 50)\n";
    std::cout << "  --epochs <int>               Número de gerações (padrão: 500)\n";
    std::cout << "  --pop <int>                  Tamanho da população (padrão: 200)\n";
    std::cout << "  --mut <float>                Taxa de mutação (padrão: 0.05)\n";
    std::cout << "  --selection {tournament|roulette}  Método de seleção (padrão: tournament)\n";
    std::cout << "  --tournament <int>           Tamanho do torneio (padrão: 3)\n";
    std::cout << "  --crossover {ox|pmx}         Tipo de crossover (padrão: ox)\n";
    std::cout << "  --elite <int>                Contagem de elite (padrão: 2)\n";
    std::cout << "  --patience <int>             Paciência para parada antecipada (padrão: 100)\n";
    std::cout << "  --seed <int>                 Semente aleatória (padrão: 42)\n";
    std::cout << "  --outdir <path>              Diretório de saída (padrão: ./outputs)\n";
    std::cout << "  --frames <path>              Diretório de quadros (padrão: ./frames)\n";
    std::cout << "  --check                      Executar modo de validação rápida\n";
    std::cout << "  --help                       Mostrar esta mensagem de ajuda\n\n";
    std::cout << "Exemplos:\n";
    std::cout << "  " << program << " --scenario uniform --n 60 --epochs 800 --crossover pmx --mut 0.08\n";
    std::cout << "  " << program << " --scenario circle --n 80 --epochs 1200 --selection roulette --elite 4\n";
    std::cout << "  " << program << " --check\n\n";
}

bool parseArgs(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return false;
        } else if (arg == "--check") {
            config.check_mode = true;
        } else if (arg == "--scenario" && i + 1 < argc) {
            config.scenario = argv[++i];
        } else if (arg == "--n" && i + 1 < argc) {
            config.n = std::atoi(argv[++i]);
        } else if (arg == "--epochs" && i + 1 < argc) {
            config.epochs = std::atoi(argv[++i]);
        } else if (arg == "--pop" && i + 1 < argc) {
            config.pop = std::atoi(argv[++i]);
        } else if (arg == "--mut" && i + 1 < argc) {
            config.mut = std::atof(argv[++i]);
        } else if (arg == "--selection" && i + 1 < argc) {
            config.selection = argv[++i];
        } else if (arg == "--tournament" && i + 1 < argc) {
            config.tournament = std::atoi(argv[++i]);
        } else if (arg == "--crossover" && i + 1 < argc) {
            config.crossover = argv[++i];
        } else if (arg == "--elite" && i + 1 < argc) {
            config.elite = std::atoi(argv[++i]);
        } else if (arg == "--patience" && i + 1 < argc) {
            config.patience = std::atoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.seed = std::atoi(argv[++i]);
        } else if (arg == "--outdir" && i + 1 < argc) {
            config.outdir = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            config.framesdir = argv[++i];
        } else {
            std::cerr << "Argumento desconhecido: " << arg << "\n";
            printUsage(argv[0]);
            return false;
        }
    }
    
    return true;
}

bool validateConfig(const Config& config) {
    if (config.n < 8) {
        std::cerr << "Erro: Mínimo de 8 cidades necessário\n";
        return false;
    }
    if (config.scenario != "uniform" && config.scenario != "circle") {
        std::cerr << "Erro: Cenário deve ser 'uniform' ou 'circle'\n";
        return false;
    }
    if (config.selection != "tournament" && config.selection != "roulette") {
        std::cerr << "Erro: Seleção deve ser 'tournament' ou 'roulette'\n";
        return false;
    }
    if (config.crossover != "ox" && config.crossover != "pmx") {
        std::cerr << "Erro: Crossover deve ser 'ox' ou 'pmx'\n";
        return false;
    }
    if (config.mut < 0.0 || config.mut > 1.0) {
        std::cerr << "Erro: Taxa de mutação deve estar entre 0.0 e 1.0\n";
        return false;
    }
    return true;
}

// GA modificado com geração de quadros
class GAWithFrames : public GeneticAlgorithm {
private:
    const TSPInstance& tsp_ref;
    const Config& config_ref;
    int frame_interval;
    
public:
    GAWithFrames(const TSPInstance& tsp, const GAConfig& cfg, const Config& user_config)
        : GeneticAlgorithm(tsp, cfg), tsp_ref(tsp), config_ref(user_config) {
        // Salva quadros a cada N gerações (ajusta com base no total de gerações)
        frame_interval = std::max(1, cfg.num_epochs / 200); // máx ~200 quadros
    }
    
    void runWithFrames() {
        initializePopulation();
        
        // Salva quadro inicial
        saveEpochFrame(tsp_ref, best_ever, 0, config, config_ref.framesdir, 1);
        
        for (int epoch = 0; epoch < config.num_epochs; ++epoch) {
            evolve();
            
            // Rastreia estatísticas
            std::sort(population.begin(), population.end());
            double best = population[0].fitness;
            double worst = population.back().fitness;
            double sum = 0.0;
            for (const auto& ind : population) {
                sum += ind.fitness;
            }
            double mean = sum / population.size();
            
            best_per_epoch.push_back(best);
            mean_per_epoch.push_back(mean);
            worst_per_epoch.push_back(worst);
            
            // Salva quadro em intervalos regulares
            if ((epoch + 1) % frame_interval == 0 || epoch == config.num_epochs - 1) {
                saveEpochFrame(tsp_ref, best_ever, epoch + 1, config, config_ref.framesdir, 1);
            }
            
            // Indicador de progresso
            if ((epoch + 1) % 50 == 0) {
                std::cout << "Época " << (epoch + 1) << " | Melhor: " << best << "\n";
            }
            
            // Verifica paciência (parada antecipada)
            if (generations_without_improvement >= config.patience) {
                // Corta vetores para as épocas realmente executadas
                best_per_epoch.resize(epoch + 1);
                mean_per_epoch.resize(epoch + 1);
                worst_per_epoch.resize(epoch + 1);
                std::cout << "Parada antecipada na época " << (epoch + 1) << " (paciência atingida)\n";
                saveEpochFrame(tsp_ref, best_ever, epoch + 1, config, config_ref.framesdir, 1);
                break;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    Config config;
    
    if (!parseArgs(argc, argv, config)) {
        return 1;
    }
    
    // Modo de verificação: execução rápida de validação
    if (config.check_mode) {
        std::cout << "Executando no modo CHECK (validação rápida)\n";
        config.epochs = 30;
        config.n = 20;
        config.pop = 50;
    }
    
    if (!validateConfig(config)) {
        return 1;
    }
    
    config.print();
    
    // Cria diretórios de saída
    ensureDirectories(config.outdir, config.framesdir);
    
    // Configura instância TSP
    TSPInstance tsp;
    if (config.scenario == "uniform") {
        std::cout << "Gerando " << config.n << " cidades aleatórias (distribuição uniforme)...\n";
        tsp.generateUniform(config.n, config.seed);
    } else {
        std::cout << "Gerando " << config.n << " cidades em um círculo...\n";
        tsp.generateCircle(config.n);
    }
    
    // Configura configuração GA
    GAConfig ga_config;
    ga_config.population_size = config.pop;
    ga_config.num_epochs = config.epochs;
    ga_config.mutation_rate = config.mut;
    ga_config.tournament_size = config.tournament;
    ga_config.elite_count = config.elite;
    ga_config.patience = config.patience;
    ga_config.seed = config.seed;
    
    // Método de seleção
    if (config.selection == "tournament") {
        ga_config.selection = GAConfig::TOURNAMENT;
    } else {
        ga_config.selection = GAConfig::ROULETTE;
    }
    
    // Método de crossover
    if (config.crossover == "ox") {
        ga_config.crossover = GAConfig::OX;
    } else {
        ga_config.crossover = GAConfig::PMX;
    }
    
    // Executa GA com geração de frames em uma única execução
    std::cout << "Iniciando Algoritmo Genético...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    
    GAWithFrames ga(tsp, ga_config, config);
    ga.runWithFrames();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\nGA concluído em " << duration.count() / 1000.0 << " segundos\n";
    std::cout << "Épocas executadas: " << ga.getActualEpochs() << "\n";
    std::cout << "Melhor comprimento da rota: " << ga.getBestEver().fitness << "\n";
    
    // Salva saídas
    std::cout << "\nSalvando saídas...\n";
    
    // Melhor rota SVG
    std::string best_tour_svg = config.outdir + "/best_tour.svg";
    plotTour(tsp, ga.getBestEver().tour, best_tour_svg, 
             "Melhor Rota - Comprimento: " + std::to_string(ga.getBestEver().fitness));
    std::cout << "  Salvo: " << best_tour_svg << "\n";
    
    // Melhor rota texto
    std::string best_tour_txt = config.outdir + "/best_tour.txt";
    saveTourToFile(best_tour_txt, ga.getBestEver().tour, ga.getBestEver().fitness);
    std::cout << "  Salvo: " << best_tour_txt << "\n";
    
    // Gráfico de convergência
    std::string convergence_svg = config.outdir + "/convergence.svg";
    plotConvergence(ga.getBestPerEpoch(), convergence_svg);
    std::cout << "  Salvo: " << convergence_svg << "\n";
    
    // Métricas CSV
    std::string metrics_csv = config.outdir + "/metrics.csv";
    saveMetricsCSV(metrics_csv, ga.getBestPerEpoch(), ga.getMeanPerEpoch(), 
                   ga.getWorstPerEpoch(), config.mut, config.seed);
    std::cout << "  Salvo: " << metrics_csv << "\n";
    
    std::cout << "\n=== Resumo ===\n";
    std::cout << "Quadros salvos em: " << config.framesdir << "/\n";
    
    if (config.check_mode) {
        std::cout << "\n✓ Modo CHECK concluído com sucesso!\n";
        std::cout << "  - Quadros foram gerados em " << config.framesdir << "/\n";
        std::cout << "  - Métricas salvas em " << metrics_csv << "\n";
        std::cout << "  - Melhor rota salva em " << best_tour_svg << "\n";
    }
    
    return 0;
}
