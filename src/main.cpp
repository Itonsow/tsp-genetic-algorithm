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
        std::cout << "\n=== TSP Genetic Algorithm Configuration ===\n";
        std::cout << "Scenario:          " << scenario << "\n";
        std::cout << "Number of cities:  " << n << "\n";
        std::cout << "Epochs:            " << epochs << "\n";
        std::cout << "Population size:   " << pop << "\n";
        std::cout << "Mutation rate:     " << mut << "\n";
        std::cout << "Selection:         " << selection << "\n";
        if (selection == "tournament") {
            std::cout << "Tournament size:   " << tournament << "\n";
        }
        std::cout << "Crossover:         " << crossover << "\n";
        std::cout << "Elite count:       " << elite << "\n";
        std::cout << "Patience:          " << patience << "\n";
        std::cout << "Random seed:       " << seed << "\n";
        std::cout << "Output directory:  " << outdir << "\n";
        std::cout << "Frames directory:  " << framesdir << "\n";
        std::cout << "==========================================\n\n";
    }
};

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --scenario {uniform|circle}  Scenario type (default: uniform)\n";
    std::cout << "  --n <int>                    Number of cities (default: 50)\n";
    std::cout << "  --epochs <int>               Number of generations (default: 500)\n";
    std::cout << "  --pop <int>                  Population size (default: 200)\n";
    std::cout << "  --mut <float>                Mutation rate (default: 0.05)\n";
    std::cout << "  --selection {tournament|roulette}  Selection method (default: tournament)\n";
    std::cout << "  --tournament <int>           Tournament size (default: 3)\n";
    std::cout << "  --crossover {ox|pmx}         Crossover type (default: ox)\n";
    std::cout << "  --elite <int>                Elite count (default: 2)\n";
    std::cout << "  --patience <int>             Early stopping patience (default: 100)\n";
    std::cout << "  --seed <int>                 Random seed (default: 42)\n";
    std::cout << "  --outdir <path>              Output directory (default: ./outputs)\n";
    std::cout << "  --frames <path>              Frames directory (default: ./frames)\n";
    std::cout << "  --check                      Run quick validation mode\n";
    std::cout << "  --help                       Show this help message\n\n";
    std::cout << "Examples:\n";
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
            std::cerr << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return false;
        }
    }
    
    return true;
}

bool validateConfig(const Config& config) {
    if (config.n < 8) {
        std::cerr << "Error: Minimum 8 cities required\n";
        return false;
    }
    if (config.scenario != "uniform" && config.scenario != "circle") {
        std::cerr << "Error: Scenario must be 'uniform' or 'circle'\n";
        return false;
    }
    if (config.selection != "tournament" && config.selection != "roulette") {
        std::cerr << "Error: Selection must be 'tournament' or 'roulette'\n";
        return false;
    }
    if (config.crossover != "ox" && config.crossover != "pmx") {
        std::cerr << "Error: Crossover must be 'ox' or 'pmx'\n";
        return false;
    }
    if (config.mut < 0.0 || config.mut > 1.0) {
        std::cerr << "Error: Mutation rate must be between 0.0 and 1.0\n";
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
        
        GAConfig ga_cfg;
        ga_cfg.population_size = config_ref.pop;
        ga_cfg.mutation_rate = config_ref.mut;
        
        for (int epoch = 0; epoch < getBestPerEpoch().capacity(); ++epoch) {
            // Salva quadro antes da evolução
            if (epoch % frame_interval == 0) {
                saveEpochFrame(tsp_ref, getBestEver(), epoch, ga_cfg, config_ref.framesdir, 1);
            }
            
            evolve();
            
            // Indicador de progresso
            if (epoch % 50 == 0 || epoch == 0) {
                std::cout << "Epoch " << epoch << " | Best: " << getBestEver().fitness << "\n";
            }
            
            // Verifica paciência
            if (getActualEpochs() > 0 && epoch >= getActualEpochs() - 1) {
                std::cout << "Early stopping at epoch " << epoch << " (patience reached)\n";
                // Salva quadro final
                saveEpochFrame(tsp_ref, getBestEver(), epoch, ga_cfg, config_ref.framesdir, 1);
                break;
            }
        }
        
        // Sempre salva o quadro final
        int final_epoch = getActualEpochs() - 1;
        saveEpochFrame(tsp_ref, getBestEver(), final_epoch, ga_cfg, config_ref.framesdir, 1);
    }
};

int main(int argc, char* argv[]) {
    Config config;
    
    if (!parseArgs(argc, argv, config)) {
        return 1;
    }
    
    // Modo de verificação: execução rápida de validação
    if (config.check_mode) {
        std::cout << "Running in CHECK mode (quick validation)\n";
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
        std::cout << "Generating " << config.n << " random cities (uniform distribution)...\n";
        tsp.generateUniform(config.n, config.seed);
    } else {
        std::cout << "Generating " << config.n << " cities on a circle...\n";
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
    
    // Executa GA
    std::cout << "Starting Genetic Algorithm...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    
    GeneticAlgorithm ga(tsp, ga_config);
    ga.run();
    
    // Gera quadros durante a evolução (re-executa com salvamento de quadros)
    std::cout << "\nGenerating epoch frames...\n";
    GeneticAlgorithm ga_frames(tsp, ga_config);
    ga_frames.initializePopulation();
    
    int frame_interval = std::max(1, config.epochs / 200);
    for (int epoch = 0; epoch < config.epochs; ++epoch) {
        ga_frames.evolve();
        
        if (epoch % frame_interval == 0 || epoch == config.epochs - 1) {
            saveEpochFrame(tsp, ga_frames.getBestEver(), epoch, ga_config, config.framesdir, 1);
        }
        
        if (ga_frames.getActualEpochs() > 0 && epoch >= ga_frames.getActualEpochs() - 1) {
            break;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\nGA completed in " << duration.count() / 1000.0 << " seconds\n";
    std::cout << "Actual epochs run: " << ga.getActualEpochs() << "\n";
    std::cout << "Best tour length: " << ga.getBestEver().fitness << "\n";
    
    // Salva saídas
    std::cout << "\nSaving outputs...\n";
    
    // Melhor rota SVG
    std::string best_tour_svg = config.outdir + "/best_tour.svg";
    plotTour(tsp, ga.getBestEver().tour, best_tour_svg, 
             "Best Tour - Length: " + std::to_string(ga.getBestEver().fitness));
    std::cout << "  Saved: " << best_tour_svg << "\n";
    
    // Melhor rota texto
    std::string best_tour_txt = config.outdir + "/best_tour.txt";
    saveTourToFile(best_tour_txt, ga.getBestEver().tour, ga.getBestEver().fitness);
    std::cout << "  Saved: " << best_tour_txt << "\n";
    
    // Gráfico de convergência
    std::string convergence_svg = config.outdir + "/convergence.svg";
    plotConvergence(ga.getBestPerEpoch(), convergence_svg);
    std::cout << "  Saved: " << convergence_svg << "\n";
    
    // Métricas CSV
    std::string metrics_csv = config.outdir + "/metrics.csv";
    saveMetricsCSV(metrics_csv, ga.getBestPerEpoch(), ga.getMeanPerEpoch(), 
                   ga.getWorstPerEpoch(), config.mut, config.seed);
    std::cout << "  Saved: " << metrics_csv << "\n";
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "Frames saved in: " << config.framesdir << "/\n";
    std::cout << "To create video with ffmpeg (if SVG supported):\n";
    std::cout << "  ffmpeg -framerate 30 -pattern_type glob -i '" << config.framesdir 
              << "/epoch_*.svg' -c:v libx264 -pix_fmt yuv420p " << config.outdir << "/evolution.mp4\n\n";
    std::cout << "Or convert to PNG first:\n";
    std::cout << "  mkdir -p " << config.framesdir << "_png\n";
    std::cout << "  for f in " << config.framesdir << "/epoch_*.svg; do\n";
    std::cout << "    rsvg-convert \"$f\" -o \"" << config.framesdir 
              << "_png/$(basename \"${f%.svg}\").png\" -w 1280 -h 720\n";
    std::cout << "  done\n";
    std::cout << "  ffmpeg -framerate 30 -pattern_type glob -i '" << config.framesdir 
              << "_png/epoch_*.png' -c:v libx264 -pix_fmt yuv420p " << config.outdir << "/evolution.mp4\n\n";
    
    if (config.check_mode) {
        std::cout << "\n✓ CHECK mode completed successfully!\n";
        std::cout << "  - Frames were generated in " << config.framesdir << "/\n";
        std::cout << "  - Metrics saved to " << metrics_csv << "\n";
        std::cout << "  - Best tour saved to " << best_tour_svg << "\n";
    }
    
    return 0;
}
