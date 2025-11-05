#!/bin/bash
# Versão SUPER otimizada: Processa frame por frame via pipe

# Usa o diretório atual do script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "╔═════════════════════════════════════════════════════════════╗"
echo "║  SVG → GIF (Pipeline Ultra-Otimizado - Frame por Frame)     ║"
echo "╚═════════════════════════════════════════════════════════════╝"
echo ""

# Parâmetros configuráveis
WIDTH=800
HEIGHT=600
FPS=10
OUTPUT="outputs/evolution.gif"

# Parse argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        --size)
            WIDTH="$2"
            HEIGHT="$3"
            shift 3
            ;;
        --fps)
            FPS="$2"
            shift 2
            ;;
        --output)
            OUTPUT="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

echo "Configuracao:"
echo "   - Resolucao: ${WIDTH}x${HEIGHT}"
echo "   - FPS: $FPS"
echo "   - Saida: $OUTPUT"
echo ""

# Contar frames
FRAME_COUNT=$(ls frames/epocas_*.svg 2>/dev/null | wc -l | tr -d ' ')
if [ "$FRAME_COUNT" -eq 0 ]; then
    echo "Nenhum frame SVG encontrado"
    exit 1
fi

echo " $FRAME_COUNT frames encontrados"
echo ""

# Criar temp para cache mínimo
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Fase 1/3: Conversão SVG → PNG (cache temporário)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

count=0
for f in frames/epocas_*.svg; do
    basename=$(basename "$f" .svg)
    rsvg-convert "$f" -o "$TEMP_DIR/${basename}.png" -w $WIDTH -h $HEIGHT >/dev/null 2>&1
    count=$((count + 1))
    
    # Progress bar
    pct=$((count * 100 / FRAME_COUNT))
    bar_filled=$((pct / 2))
    bar_empty=$((50 - bar_filled))
    printf "\r   ["
    printf "%${bar_filled}s" | tr ' ' '█'
    printf "%${bar_empty}s" | tr ' ' '░'
    printf "] %3d%% (%d/%d)" $pct $count $FRAME_COUNT
done
echo ""
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Fase 2/3: Análise e geração de paleta otimizada"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

ffmpeg -hide_banner -loglevel error -stats \
  -framerate $FPS -pattern_type glob -i "$TEMP_DIR/epocas_*.png" \
  -vf "fps=$FPS,scale=${WIDTH}:-1:flags=lanczos,palettegen=stats_mode=diff:max_colors=256" \
  -y "$TEMP_DIR/palette.png"

echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo " Fase 3/3: Renderizando GIF com paleta customizada"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

ffmpeg -hide_banner -loglevel error -stats \
  -framerate $FPS -pattern_type glob -i "$TEMP_DIR/epocas_*.png" \
  -i "$TEMP_DIR/palette.png" \
  -lavfi "fps=$FPS,scale=${WIDTH}:-1:flags=lanczos[x];[x][1:v]paletteuse=dither=bayer:bayer_scale=5:diff_mode=rectangle" \
  -loop 0 -y "$OUTPUT"

echo ""
echo ""

# Resultado
if [ -f "$OUTPUT" ]; then
    FILE_SIZE=$(ls -lh "$OUTPUT" | awk '{print $5}')
    DURATION=$(echo "scale=2; $FRAME_COUNT / $FPS" | bc)
    
    echo "╔═════════════════════════════════════════════════════════════╗"
    echo "║                     SUCESSO TOTAL!                          ║"
    echo "╚═════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  GIF Gerado:"
    echo "     Arquivo:     $OUTPUT"
    echo "     Tamanho:     $FILE_SIZE"
    echo "     Resolucao:   ${WIDTH}x${HEIGHT}px"
    echo "     Frames:      $FRAME_COUNT"
    echo "     FPS:         $FPS"
    echo "     Duracao:     ${DURATION}s"
    echo "     Loop:        Infinito"
    echo ""
    echo "   Abrir agora:"
    echo "   open $OUTPUT"
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  Criar variacoes:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "   Pequeno (600px):"
    echo "   $0 --size 600 450 --output outputs/evolution_small.gif"
    echo ""
    echo "   Rápido (15 fps):"
    echo "   $0 --fps 15 --output outputs/evolution_fast.gif"
    echo ""
    echo "   Alta qualidade (1280px):"
    echo "   $0 --size 1280 960 --output outputs/evolution_hq.gif"
    echo ""
else
    echo "  Erro ao gerar GIF"
    exit 1
fi
