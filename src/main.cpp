#include "tsp.hpp"
#include "ga.hpp"
#include "plot_utils.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>

#define MAXQ 200

// Analisador simples de argumentos da linha de comando
struct Config
{
    std::string cenario = "uniforme";
    int pontos = 50;
    int epocas = 500;
    int pop = 200;
    double tx_mutacao = 0.05;
    std::string selection = "torneio";
    int torneio = 3;
    std::string crossover = "ox";
    int alpha = 2;
    int paciencia = 100;
    int seed = 42;
    std::string outdir = "./outputs";
    std::string framesdir = "./frames";
    bool check_mode = false;

    void print() const
    {
        std::cout << "\n=== Configuração do Algoritmo Genético TSP ===\n";
        std::cout << "Cenário:                " << cenario << "\n";
        std::cout << "Número de pontos:      " << pontos << "\n";
        std::cout << "Épocas:                 " << epocas << "\n";
        std::cout << "Tamanho da população:   " << pop << "\n";
        std::cout << "Taxa de mutação:        " << tx_mutacao << "\n";
        std::cout << "Seleção:                " << selection << "\n";
        if (selection == "torneio")
        {
            std::cout << "Tamanho do torneio:     " << torneio << "\n";
        }
        std::cout << "Contagem de alpha:      " << alpha << "\n";
        std::cout << "Paciência:              " << paciencia << "\n";
        std::cout << "Semente aleatória:      " << seed << "\n";
        std::cout << "Diretório de saída:     " << outdir << "\n";
        std::cout << "Diretório de quadros:   " << framesdir << "\n";
        std::cout << "==============================================\n\n";
    }
};

void printHelp(const char *program)
{
    std::cout << "Uso: " << program << " [OPÇÕES]\n\n";
    std::cout << "Opções:\n";
    std::cout << "  --cenario {uniforme|circulo}  Tipo de cenário (padrão: uniforme)\n";
    std::cout << "  --pontos <int>                    Número de pontos (padrão: 50)\n";
    std::cout << "  --epocas <int>               Número de gerações (padrão: 500)\n";
    std::cout << "  --pop <int>                  Tamanho da população (padrão: 200)\n";
    std::cout << "  --mut <float>                Taxa de mutação (padrão: 0.05)\n";
    std::cout << "  --selection {torneio|roulette}  Método de seleção (padrão: torneio)\n";
    std::cout << "  --torneio <int>           Tamanho do torneio (padrão: 3)\n";
    std::cout << "  --alpha <int>                Contagem de alpha (padrão: 2)\n";
    std::cout << "  --paciencia <int>             Paciência para parada antecipada (padrão: 100)\n";
    std::cout << "  --seed <int>                 Semente aleatória (padrão: 42)\n";
    std::cout << "  --check                      Executar modo de validação rápida\n";
    std::cout << "  --help                       Mostrar esta mensagem de ajuda\n\n";
    std::cout << "Exemplos:\n";
    std::cout << "  " << program << " --cenario uniforme --pontos 60 --epocas 800 --mut 0.08\n";
    std::cout << "  " << program << " --cenario circulo --pontos 80 --epocas 1200 --alpha 4\n";
    std::cout << "  " << program << " --check\n\n";
}

bool parseArgs(int argc, char *argv[], Config &config)
{
    // Se não houver argumentos, perguntar se deseja usar config padrão
    if (argc == 1)
    {
        std::cout << "Deseja usar configuração padrão? (1=Sim, 0=Não): ";
        int p = 0;
        std::cin >> p;
        if (p > 0)
        {
            return true; // Continua com config padrão
        }
        else
        {
            printHelp(argv[0]);
            return false; // Sai do programa
        }
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            printHelp(argv[0]);
            return false;
        }
        else if (arg == "--check")
        {
            config.check_mode = true;
        }
        else if (arg == "--cenario" && i + 1 < argc)
        {
            config.cenario = argv[++i];
        }
        else if (arg == "--pontos" && i + 1 < argc)
        {
            config.pontos = std::atoi(argv[++i]);
        }
        else if (arg == "--epocas" && i + 1 < argc)
        {
            config.epocas = std::atoi(argv[++i]);
        }
        else if (arg == "--pop" && i + 1 < argc)
        {
            config.pop = std::atoi(argv[++i]);
        }
        else if (arg == "--mut" && i + 1 < argc)
        {
            config.tx_mutacao = std::atof(argv[++i]);
        }
        else if (arg == "--selection" && i + 1 < argc)
        {
            config.selection = argv[++i];
        }
        else if (arg == "--torneio" && i + 1 < argc)
        {
            config.torneio = std::atoi(argv[++i]);
        }
        else if (arg == "--crossover" && i + 1 < argc)
        {
            config.crossover = argv[++i];
        }
        else if (arg == "--alpha" && i + 1 < argc)
        {
            config.alpha = std::atoi(argv[++i]);
        }
        else if (arg == "--paciencia" && i + 1 < argc)
        {
            config.paciencia = std::atoi(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc)
        {
            config.seed = std::atoi(argv[++i]);
        }
        else if (arg == "--outdir" && i + 1 < argc)
        {
            config.outdir = argv[++i];
        }
        else if (arg == "--frames" && i + 1 < argc)
        {
            config.framesdir = argv[++i];
        }
        else
        {
            std::cerr << "Argumento desconhecido: " << arg << "\n";
            printHelp(argv[0]);
            return false;
        }
    }

    return true;
}

bool validarConfig(const Config &config)
{
    if (config.pontos < 8)
    {
        std::cerr << "Erro: Mínimo de 8 pontos necessário\n";
        return false;
    }
    if (config.cenario != "uniforme" && config.cenario != "circulo")
    {
        std::cerr << "Erro: Cenário deve ser 'uniforme' ou 'circulo'\n";
        return false;
    }
    if (config.selection != "torneio" && config.selection != "roulette")
    {
        std::cerr << "Erro: Seleção deve ser 'torneio' ou 'roulette'\n";
        return false;
    }
    if (config.crossover != "ox" && config.crossover != "pmx")
    {
        std::cerr << "Erro: Crossover deve ser 'ox' ou 'pmx'\n";
        return false;
    }
    if (config.tx_mutacao < 0.0 || config.tx_mutacao > 1.0)
    {
        std::cerr << "Erro: Taxa de mutação deve estar entre 0.0 e 1.0\n";
        return false;
    }
    return true;
}

// GA modificado com geração de quadros
class GAWithFrames : public GeneticAlgorithm
{
private:
    const InstanciaTSP &tsp_ref;
    const Config &config_ref;
    int frame_interval;

public:
    GAWithFrames(const InstanciaTSP &tsp, const GAConfig &cfg, const Config &user_config)
        : GeneticAlgorithm(tsp, cfg), tsp_ref(tsp), config_ref(user_config)
    {
        // Salva quadros a cada N gerações (ajusta com base no total de gerações)
        frame_interval = std::max(1, cfg.num_epocas / MAXQ); // máx ~200 quadros
    }

    void runWithFrames()
    {
        initPop();

        // Salva quadro inicial
        salvarEpocaFrame(tsp_ref, melhor_todos, 0, config, config_ref.framesdir, 1);

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

            // Salva quadro em intervalos regulares
            if ((epocas + 1) % frame_interval == 0 || epocas == config.num_epocas - 1)
            {
                salvarEpocaFrame(tsp_ref, melhor_todos, epocas + 1, config, config_ref.framesdir, 1);
            }

            // Indicador de progresso
            if ((epocas + 1) % 50 == 0)
            {
                std::cout << "Época " << (epocas + 1) << " | Melhor: " << melhor << "\n";
            }

            // Verifica paciência (parada antecipada)
            if (generations_without_improvement >= config.paciencia)
            {
                // Corta vetores para as épocas realmente executadas
                melhor_por_epoca.resize(epocas + 1);
                media_por_epoca.resize(epocas + 1);
                pior_por_epoca.resize(epocas + 1);
                std::cout << "Parada antecipada na época " << (epocas + 1) << " (paciência atingida)\n";
                salvarEpocaFrame(tsp_ref, melhor_todos, epocas + 1, config, config_ref.framesdir, 1);
                break;
            }
        }
    }
};

int main(int argc, char *argv[])
{
    // Cria Struct com configuracoes padrões
    Config config;

    // caso nao seja passado argumentos na execucao do programa e o usuario opte por nao usar as configuracoes padroes, finaliza o programa
    if (!parseArgs(argc, argv, config))
    {
        return 1;
    }
    // --check: executa com algumas modificacoes simples na config, util para debuggar ou testar o programa
    if (config.check_mode)
    {
        std::cout << "Executando no modo CHECK (validação rápida)\n";
        config.epocas = 30;
        config.pontos = 20;
        config.pop = 50;
    }

    // valida se parametros passados para config sao validos, se nao, finaliza o programa
    if (!validarConfig(config))
    {
        return 1;
    }

    // printa configuracoes finais que serao utilizadas no algoritmo
    config.print();

    // Cria diretórios de saida
    ensureDirectories(config.outdir, config.framesdir);

    // Configura instância TSP
    InstanciaTSP tsp;
    if (config.cenario == "uniforme")
    {
        std::cout << "Gerando " << config.pontos << " pontos aleatórias (distribuição uniforme)...\n";
        tsp.generateUniform(config.pontos, config.seed);
    }
    else
    {
        std::cout << "Gerando " << config.pontos << " pontos em um círculo...\n";
        tsp.generateCircle(config.pontos);
    }

    // Define configuração GA
    GAConfig ga_config;
    ga_config.tam_populacao = config.pop;
    ga_config.num_epocas = config.epocas;
    ga_config.taxa_mutacao = config.tx_mutacao;
    ga_config.tam_torneio = config.torneio;
    ga_config.quant_alpha = config.alpha;
    ga_config.paciencia = config.paciencia;
    ga_config.seed = config.seed;
    ga_config.crossover = GAConfig::OX;

    // Método de seleção
    if (config.selection == "torneio")
    {
        ga_config.selection = GAConfig::TORNEIO;
    }
    else
    {
        ga_config.selection = GAConfig::ROLETA;
    }

    // Executa GA com geração de frames em uma única execução
    std::cout << "Iniciando Algoritmo Genético...\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    GAWithFrames ga(tsp, ga_config, config);
    ga.runWithFrames();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\nGA concluído em " << duration.count() / 1000.0 << " segundos\n";
    std::cout << "Épocas executadas: " << ga.getMelhorEpocaAtual() << "\n";
    std::cout << "Melhor comprimento da rota: " << ga.getMelhorTodos().fitness << "\n";

    // Salva saídas
    std::cout << "\nSalvando saídas...\n";

    // Melhor rota SVG
    std::string best_rota_svg = config.outdir + "/melhor_volta.svg";
    plotRota(tsp, ga.getMelhorTodos().rota, best_rota_svg,
             "Melhor Rota - Comprimento: " + std::to_string(ga.getMelhorTodos().fitness));
    std::cout << "  Salvo: " << best_rota_svg << "\n";

    // Melhor rota texto
    std::string best_rota_txt = config.outdir + "/melhor_volta.txt";
    salvarRotaParaFile(best_rota_txt, ga.getMelhorTodos().rota, ga.getMelhorTodos().fitness);
    std::cout << "  Salvo: " << best_rota_txt << "\n";

    // Gráfico de convergência
    std::string convergencia_svg = config.outdir + "/convergencia.svg";
    plotaConvergencia(ga.getMelhorPorEpoca(), convergencia_svg);
    std::cout << "  Salvo: " << convergencia_svg << "\n";

    // Métricas CSV
    std::string metricas_csv = config.outdir + "/metricas.csv";
    salvarMetricasCSV(metricas_csv, ga.getMelhorPorEpoca(), ga.getMediaPorEpoca(),
                      ga.getPiorPorEpoca(), config.tx_mutacao, config.seed);
    std::cout << "  Salvo: " << metricas_csv << "\n";

    std::cout << "\n=== Resumo ===\n";
    std::cout << "Quadros salvos em: " << config.framesdir << "/\n";

    // roda modo check, para testar algoritmo, essencialmente
    if (config.check_mode)
    {
        std::cout << "\n  Modo CHECK concluído com sucesso!\n";
        std::cout << "  - Quadros foram gerados em " << config.framesdir << "/\n";
        std::cout << "  - Métricas salvas em " << metricas_csv << "\n";
        std::cout << "  - Melhor rota salva em " << best_rota_svg << "\n";
    }

    return 0;
}
