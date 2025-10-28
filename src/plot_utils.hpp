#ifndef PLOT_UTILS_HPP
#define PLOT_UTILS_HPP

#include "tsp.hpp"
#include "ga.hpp"
#include "third_party/plot.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Funções utilitárias para plotagem com Signalsmith plot.h

// Cria diretórios de saída se não existirem
inline void ensureDirectories(const std::string& outdir, const std::string& framesdir) {
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
inline bool plotEpochTour(const TSPInstance& tsp, const Individual& best, int epoch,
                          const GAConfig& config, const std::string& filename) {
    std::ostringstream title;
    title << "Generation: " << epoch;
    
    return plotTour(tsp, best.tour, filename, title.str());
}

// Plota curva de convergência (melhor aptidão ao longo das gerações)
inline bool plotConvergence(const std::vector<double>& best_per_epoch,
                            const std::string& filename,
                            const std::string& title = "GA Convergence") {
    std::vector<double> epochs;
    for (size_t i = 0; i < best_per_epoch.size(); ++i) {
        epochs.push_back(static_cast<double>(i));
    }
    
    signalsmith::plot::Plot2D plot;
    plot.size(1000, 600);
    plot.title(title);
    plot.xlabel("Epoch");
    plot.ylabel("Best Tour Length");
    
    plot.line(epochs, best_per_epoch, "#cc0000", 2.5, "Best Fitness");
    
    return plot.write(filename);
}

// Salva métricas em CSV
inline bool saveMetricsCSV(const std::string& filename,
                          const std::vector<double>& best,
                          const std::vector<double>& mean,
                          const std::vector<double>& worst,
                          double mutation_rate,
                          int seed) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    // Cabeçalho
    file << "epoch,best,mean,worst,mutation_rate,seed\n";
    
    // Dados
    for (size_t i = 0; i < best.size(); ++i) {
        file << i << ","
             << std::fixed << std::setprecision(6) << best[i] << ","
             << mean[i] << ","
             << worst[i] << ","
             << mutation_rate << ","
             << seed << "\n";
    }
    
    file.close();
    return true;
}

// Salva a melhor rota em arquivo de texto
inline bool saveTourToFile(const std::string& filename, const Tour& tour, double fitness) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << "# Best Tour - Length: " << std::fixed << std::setprecision(6) << fitness << "\n";
    file << "# Visit order (city indices):\n";
    
    for (size_t i = 0; i < tour.size(); ++i) {
        file << tour[i];
        if (i < tour.size() - 1) file << " -> ";
        if ((i + 1) % 20 == 0) file << "\n";
    }
    file << " -> " << tour[0] << " (return to start)\n";
    
    file.close();
    return true;
}

// Gera todos os quadros das gerações (chamado periodicamente durante a evolução)
// Para eficiência, salva quadros apenas em intervalos regulares
inline void saveEpochFrame(const TSPInstance& tsp, const Individual& best, int epoch,
                          const GAConfig& config, const std::string& framesdir,
                          int frame_interval = 1) {
    if (epoch % frame_interval != 0 && epoch != 0) return;
    
    std::ostringstream filename;
    filename << framesdir << "/epoch_" << std::setfill('0') << std::setw(4) << epoch << ".svg";
    
    plotEpochTour(tsp, best, epoch, config, filename.str());
}

// Auxiliar para formatar nomes de arquivos de quadros para ffmpeg
inline std::string getFrameFilename(const std::string& framesdir, int epoch) {
    std::ostringstream oss;
    oss << framesdir << "/epoch_" << std::setfill('0') << std::setw(4) << epoch << ".svg";
    return oss.str();
}

#endif // PLOT_UTILS_HPP
