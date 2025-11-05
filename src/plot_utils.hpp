#ifndef PLOT_UTILS_HPP
#define PLOT_UTILS_HPP

#include "tsp.hpp"
#include "ga.hpp"
#include "third_party/plot.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Funções utilitárias para plotagem com Signalsmith plot.h

// Cria diretórios de saída se não existirem
inline void ensureDirectories(const std::string& outdir, const std::string& framesdir) {
    if (fs::exists(outdir))
        fs::remove_all(outdir);
    
    if (fs::exists(framesdir))
        fs::remove_all(framesdir);

    fs::create_directories(outdir);
    fs::create_directories(framesdir);
}

// Plota uma rota (cidades e arestas) e salva como SVG
inline bool plotTour(const TSPInstance& tsp, const Tour& tour, const std::string& filename,
                     const std::string& title = "") {
    const auto& cities = tsp.getCities();
    
    // Extrai coordenadas
    std::vector<double> x, y;
    for (int idx : tour) {
        x.push_back(cities[idx].x);
        y.push_back(cities[idx].y);
    }
    // Adiciona a primeira cidade novamente para fechar o loop
    x.push_back(cities[tour[0]].x);
    y.push_back(cities[tour[0]].y);
    
    // Todas as posições das cidades para dispersão
    std::vector<double> all_x, all_y;
    for (const auto& city : cities) {
        all_x.push_back(city.x);
        all_y.push_back(city.y);
    }
    
    // Cria gráfico
    signalsmith::plot::Plot2D plot;
    plot.size(800, 600);
    
    if (!title.empty()) {
        plot.title(title);
    }
    
    // Desenha arestas da rota
    plot.line(x, y, "#0066cc", 2.0, "Tour");
    
    // Desenha cidades como pontos dispersos
    plot.scatter(all_x, all_y, "#ff6600", 6.0, "Cities");
    
    return plot.write(filename);
}

// Plota uma rota com informações da geração
inline bool plotEpochTour(const TSPInstance& tsp, const Individual& melhor, int epocas,
                          const GAConfig& /* config */, const std::string& filename) {
    std::ostringstream title;
    title << "Quadros: " << epocas << " | Caminho: " 
          << std::fixed << std::setprecision(2) << melhor.fitness;
    
    return plotTour(tsp, melhor.tour, filename, title.str());
}

// Plota curva de convergência (melhor aptidão ao longo das gerações)
inline bool plotConvergence(const std::vector<double>& melhor_por_epoca,
                            const std::string& filename,
                            const std::string& title = "Convergencia AG") {
    // Verifica se há dados
    if (melhor_por_epoca.empty()) {
        std::cerr << "Aviso: melhor_por_epoca esta vazio! Nao e possivel gerar grafico de convergencia.\n";
        return false;
    }
    
    std::vector<double> epochs;
    for (size_t i = 0; i < melhor_por_epoca.size(); ++i) {
        epochs.push_back(static_cast<double>(i));
    }
    
    // Calcula limites manualmente
    double min_fitness = *std::min_element(melhor_por_epoca.begin(), melhor_por_epoca.end());
    double max_fitness = *std::max_element(melhor_por_epoca.begin(), melhor_por_epoca.end());
    double padding_y = (max_fitness - min_fitness) * 0.1;
    double padding_x = melhor_por_epoca.size() * 0.05;
    
    signalsmith::plot::Plot2D plot;
    plot.size(1000, 600);
    plot.title(title);
    plot.xlabel("Epocas");
    plot.ylabel("Custo do Melhor Caminho");
    
    // Define limites explicitamente
    plot.bounds(-padding_x, melhor_por_epoca.size() - 1 + padding_x,
                min_fitness - padding_y, max_fitness + padding_y);
    
    // Adiciona a linha com os dados
    plot.line(epochs, melhor_por_epoca, "#cc0000", 2.5, "Melhor Fitness");
    
    return plot.write(filename);
}

// Salva métricas em CSV
inline bool saveMetricsCSV(const std::string& filename,
                          const std::vector<double>& melhor,
                          const std::vector<double>& fitnessmedio,
                          const std::vector<double>& pior,
                          double taxa_mutacao,
                          int seed) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    // Cabeçalho
    file << "epocas,melhor,fitnessmedio,pior,taxa_mutacao,seed\n";
    
    // Dados
    for (size_t i = 0; i < melhor.size(); ++i) {
        file << i << ","
             << std::fixed << std::setprecision(6) << melhor[i] << ","
             << fitnessmedio[i] << ","
             << pior[i] << ","
             << taxa_mutacao << ","
             << seed << "\n";
    }
    
    file.close();
    return true;
}

// Salva a melhor rota em arquivo de texto
inline bool saveTourToFile(const std::string& filename, const Tour& tour, double fitness) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "# Melhor Caminho - Comprimento: " << std::fixed << std::setprecision(6) << fitness << "\n";
    file << "# Ordem de Visita:\n";
    
    for (size_t i = 0; i < tour.size(); ++i) {
        file << tour[i];
        if (i < tour.size() - 1) file << " -> ";
        if ((i + 1) % 20 == 0) file << "\n";
    }
    file << " -> " << tour[0] << " (retorna para o inicio)\n";
    
    file.close();
    return true;
}

// Gera todos os quadros das gerações (chamado periodicamente durante a evolução)
// Para eficiência, salva quadros apenas em intervalos regulares
inline void saveEpochFrame(const TSPInstance& tsp, const Individual& melhor, int epocas,
                          const GAConfig& config, const std::string& framesdir,
                          int frame_interval = 1) {
    if (epocas % frame_interval != 0 && epocas != 0) return;
    
    std::ostringstream filename;
    filename << framesdir << "/epocas_" << std::setfill('0') << std::setw(4) << epocas << ".svg";
    
    plotEpochTour(tsp, melhor, epocas, config, filename.str());
}

// Auxiliar para formatar nomes de arquivos de quadros para ffmpeg
inline std::string getFrameFilename(const std::string& framesdir, int epocas) {
    std::ostringstream oss;
    oss << framesdir << "/epocas_" << std::setfill('0') << std::setw(4) << epocas << ".svg";
    return oss.str();
}

#endif // PLOT_UTILS_HPP
