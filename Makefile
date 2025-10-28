# Makefile para TSP Genetic Algorithm
# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra
INCLUDES = -I. -Ithird_party

# Diretórios
SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/tsp_ga

# Arquivos fonte
SOURCES = $(SRC_DIR)/main.cpp
HEADERS = $(SRC_DIR)/tsp.hpp $(SRC_DIR)/ga.hpp $(SRC_DIR)/plot_utils.hpp

# Regra padrão
all: $(TARGET)

# Criar diretório build se não existir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilar o executável
$(TARGET): $(SOURCES) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET)
	@echo "✓ Compilação concluída: $(TARGET)"

# Executar com parâmetros padrão
run: $(TARGET)
	./$(TARGET)

# Executar teste rápido
check: $(TARGET)
	./$(TARGET) --check

# Executar com cenário uniforme
test-uniform: $(TARGET)
	./$(TARGET) --scenario uniform --n 50 --epochs 500

# Executar com cenário círculo
test-circle: $(TARGET)
	./$(TARGET) --scenario circle --n 50 --epochs 500

# Limpar outputs e rodar novamente
fresh-run: clean-output test-uniform
	@echo "✓ Execução limpa concluída"

# Rodar e gerar GIF automaticamente
run-gif: test-uniform
	@echo "Gerando GIF..."
	@./svg2gif_optimized.sh
	@echo "✓ GIF gerado: outputs/evolution.gif"

# Limpar arquivos compilados
clean:
	rm -rf $(BUILD_DIR)
	@echo "✓ Limpeza concluída"

# Limpar outputs e frames
clean-output:
	rm -rf outputs/* frames/*
	@echo "✓ Outputs limpos"

# Limpar tudo
clean-all: clean clean-output

# Instalar (copiar para /usr/local/bin)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/tsp_ga
	@echo "✓ Instalado em /usr/local/bin/tsp_ga"

# Verificar dependências
check-deps:
	@echo "Verificando compilador C++17..."
	@$(CXX) --version
	@echo "\n✓ Sistema pronto para compilar"

# Ajuda
help:
	@echo "Makefile para TSP Genetic Algorithm"
	@echo ""
	@echo "Comandos disponíveis:"
	@echo "  make              - Compila o projeto"
	@echo "  make run          - Compila e executa com parâmetros padrão"
	@echo "  make check        - Executa validação rápida"
	@echo "  make test-uniform - Executa com cenário uniforme"
	@echo "  make test-circle  - Executa com cenário círculo"
	@echo "  make fresh-run    - Limpa outputs e roda novamente"
	@echo "  make run-gif      - Executa e gera GIF automaticamente"
	@echo "  make clean        - Remove arquivos compilados"
	@echo "  make clean-output - Remove outputs e frames"
	@echo "  make clean-all    - Remove tudo"
	@echo "  make install      - Instala em /usr/local/bin"
	@echo "  make check-deps   - Verifica dependências"
	@echo "  make help         - Mostra esta ajuda"

.PHONY: all run check test-uniform test-circle clean clean-output clean-all install check-deps help
