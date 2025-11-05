// Simplified SVG plotting header inspired by Signalsmith Basic C++ Plots
// Header-only library for generating SVG plots
#ifndef SIGNALSMITH_PLOT_H
#define SIGNALSMITH_PLOT_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

namespace signalsmith {
namespace plot {

class Plot2D {
private:
    struct Point {
        double x, y;
        Point(double x_, double y_) : x(x_), y(y_) {}
    };
    
    struct Line {
        std::vector<Point> points;
        std::string color;
        double width;
        std::string label;
    };
    
    struct Scatter {
        std::vector<Point> points;
        std::string color;
        double size;
        std::string label;
    };
    
    std::vector<Line> lines;
    std::vector<Scatter> scatters;
    std::string title_text;
    std::string xlabel_text;
    std::string ylabel_text;
    
    double minX, maxX, minY, maxY;
    int width, height;
    int margin;
    bool auto_bounds;
    
public:
    Plot2D() : minX(0), maxX(1), minY(0), maxY(1), 
               width(800), height(600), margin(60), auto_bounds(true) {}
    
    Plot2D& size(int w, int h) {
        width = w;
        height = h;
        return *this;
    }
    
    Plot2D& title(const std::string& t) {
        title_text = t;
        return *this;
    }
    
    Plot2D& xlabel(const std::string& label) {
        xlabel_text = label;
        return *this;
    }
    
    Plot2D& ylabel(const std::string& label) {
        ylabel_text = label;
        return *this;
    }
    
    Plot2D& bounds(double xmin, double xmax, double ymin, double ymax) {
        minX = xmin;
        maxX = xmax;
        minY = ymin;
        maxY = ymax;
        auto_bounds = false;
        return *this;
    }
    
    void line(const std::vector<double>& x, const std::vector<double>& y, 
              const std::string& color = "#0066cc", double linewidth = 2.0,
              const std::string& label = "") {
        Line l;
        l.color = color;
        l.width = linewidth;
        l.label = label;
        for (size_t i = 0; i < std::min(x.size(), y.size()); ++i) {
            l.points.emplace_back(x[i], y[i]);
            if (auto_bounds) {
                updateBounds(x[i], y[i]);
            }
        }
        lines.push_back(l);
    }
    
    void scatter(const std::vector<double>& x, const std::vector<double>& y,
                 const std::string& color = "#ff6600", double pointsize = 5.0,
                 const std::string& label = "") {
        Scatter s;
        s.color = color;
        s.size = pointsize;
        s.label = label;
        for (size_t i = 0; i < std::min(x.size(), y.size()); ++i) {
            s.points.emplace_back(x[i], y[i]);
            if (auto_bounds) {
                updateBounds(x[i], y[i]);
            }
        }
        scatters.push_back(s);
    }
    
    bool write(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        
        // Add padding to bounds
        if ((auto_bounds && !lines.empty()) || !scatters.empty()) {
            double padX = (maxX - minX) * 0.05;
            double padY = (maxY - minY) * 0.05;
            minX -= padX; maxX += padX;
            minY -= padY; maxY += padY;
        }
        
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        file << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
        file << "width=\"" << width << "\" height=\"" << height << "\" ";
        file << "viewBox=\"0 0 " << width << " " << height << "\">\n";
        
        // Background
        file << "<rect width=\"" << width << "\" height=\"" << height << "\" fill=\"white\"/>\n";
        
        // Plot area
        int plotX = margin;
        int plotY = margin;
        int plotW = width - 2 * margin;
        int plotH = height - 2 * margin;
        
        file << "<rect x=\"" << plotX << "\" y=\"" << plotY << "\" ";
        file << "width=\"" << plotW << "\" height=\"" << plotH << "\" ";
        file << "fill=\"#f9f9f9\" stroke=\"#cccccc\" stroke-width=\"1\"/>\n";
        
        // Clip path for plot area
        file << "<defs><clipPath id=\"plotarea\">";
        file << "<rect x=\"" << plotX << "\" y=\"" << plotY << "\" ";
        file << "width=\"" << plotW << "\" height=\"" << plotH << "\"/>";
        file << "</clipPath></defs>\n";
        
        // Draw grid
        drawGrid(file, plotX, plotY, plotW, plotH);
        
        // Group for plot elements (with clipping)
        file << "<g clip-path=\"url(#plotarea)\">\n";
        
        // Draw lines
        for (const auto& l : lines) {
            if (l.points.empty()) continue;
            file << "<polyline fill=\"none\" stroke=\"" << l.color << "\" ";
            file << "stroke-width=\"" << l.width << "\" points=\"";
            for (const auto& p : l.points) {
                auto [sx, sy] = toScreen(p.x, p.y, plotX, plotY, plotW, plotH);
                file << sx << "," << sy << " ";
            }
            file << "\"/>\n";
        }
        
        // Draw scatter points
        for (const auto& s : scatters) {
            for (const auto& p : s.points) {
                auto [sx, sy] = toScreen(p.x, p.y, plotX, plotY, plotW, plotH);
                file << "<circle cx=\"" << sx << "\" cy=\"" << sy << "\" ";
                file << "r=\"" << s.size << "\" fill=\"" << s.color << "\" ";
                file << "stroke=\"#333333\" stroke-width=\"1\"/>\n";
            }
        }
        
        file << "</g>\n";
        
        // Draw axes labels
        drawAxes(file, plotX, plotY, plotW, plotH);
        
        // Title
        if (!title_text.empty()) {
            file << "<text x=\"" << width/2 << "\" y=\"25\" ";
            file << "text-anchor=\"middle\" font-family=\"Arial\" font-size=\"18\" ";
            file << "font-weight=\"bold\" fill=\"#333333\">" << escapeXml(title_text) << "</text>\n";
        }
        
        file << "</svg>\n";
        file.close();
        return true;
    }
    
private:
    void updateBounds(double x, double y) {
        if (lines.empty() && scatters.empty()) {
            minX = maxX = x;
            minY = maxY = y;
        } else {
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }
    }
    
    std::pair<double, double> toScreen(double x, double y, int px, int py, int pw, int ph) {
        double sx = px + (x - minX) / (maxX - minX) * pw;
        double sy = py + ph - (y - minY) / (maxY - minY) * ph;
        return {sx, sy};
    }
    
    void drawGrid(std::ofstream& file, int px, int py, int pw, int ph) {
        // Simple grid lines
        file << "<g stroke=\"#e0e0e0\" stroke-width=\"1\">\n";
        for (int i = 0; i <= 5; ++i) {
            int x = px + i * pw / 5;
            int y = py + i * ph / 5;
            file << "<line x1=\"" << x << "\" y1=\"" << py << "\" x2=\"" << x << "\" y2=\"" << py + ph << "\"/>\n";
            file << "<line x1=\"" << px << "\" y1=\"" << y << "\" x2=\"" << px + pw << "\" y2=\"" << y << "\"/>\n";
        }
        file << "</g>\n";
    }
    
    void drawAxes(std::ofstream& file, int px, int py, int pw, int ph) {
        file << "<g font-family=\"Arial\" font-size=\"12\" fill=\"#333333\">\n";
        
        // X-axis labels
        for (int i = 0; i <= 5; ++i) {
            double val = minX + (maxX - minX) * i / 5.0;
            int x = px + i * pw / 5;
            file << "<text x=\"" << x << "\" y=\"" << py + ph + 20 << "\" ";
            file << "text-anchor=\"middle\">" << formatNumber(val) << "</text>\n";
        }
        
        // Y-axis labels
        for (int i = 0; i <= 5; ++i) {
            double val = minY + (maxY - minY) * i / 5.0;
            int y = py + ph - i * ph / 5;
            file << "<text x=\"" << px - 10 << "\" y=\"" << y + 4 << "\" ";
            file << "text-anchor=\"end\">" << formatNumber(val) << "</text>\n";
        }
        
        // Axis labels
        if (!xlabel_text.empty()) {
            file << "<text x=\"" << px + pw/2 << "\" y=\"" << height - 10 << "\" ";
            file << "text-anchor=\"middle\" font-size=\"14\">" << escapeXml(xlabel_text) << "</text>\n";
        }
        if (!ylabel_text.empty()) {
            file << "<text x=\"15\" y=\"" << py + ph/2 << "\" ";
            file << "text-anchor=\"middle\" font-size=\"14\" ";
            file << "transform=\"rotate(-90 15 " << py + ph/2 << ")\">" << escapeXml(ylabel_text) << "</text>\n";
        }
        
        file << "</g>\n";
    }
    
    std::string formatNumber(double val) {
        std::ostringstream oss;
        if (std::abs(val) < 0.01 || std::abs(val) > 10000) {
            oss << std::scientific << std::setprecision(1) << val;
        } else {
            oss << std::fixed << std::setprecision(2) << val;
        }
        return oss.str();
    }
    
    std::string escapeXml(const std::string& str) {
        std::string escaped;
        for (char c : str) {
            switch (c) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '"': escaped += "&quot;"; break;
                case '\'': escaped += "&apos;"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
};

} // namespace plot
} // namespace signalsmith

#endif // SIGNALSMITH_PLOT_H
