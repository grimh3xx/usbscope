#!/usr/bin/env python3
"""
Gera src/banner_data.inc a partir de uma pixel art da logo.

Uso:
    python tools/gen_banner.py tools/logo_grid.png src/banner_data.inc

logo_grid.png: 1 pixel da imagem = 1 "bloco" da logo no terminal.
Pixels escuros (max(R,G,B) < 45) ou transparentes viram vazio.
Cada par de linhas vira 1 linha no terminal (meio-bloco ▀ / ▄).

Pra recriar o grid a partir de um print do terminal do samcli:
    python tools/gen_banner.py --from-screenshot print.png tools/logo_grid.png
"""
import sys
from PIL import Image

# Caracteres usados pra indexar a paleta (sem espaço, aspas, barra invertida e '?')
CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&()*+,-./:;<=>@[]^_`{|}~"


def is_empty(px):
    if len(px) == 4 and px[3] < 128:
        return True
    return max(px[:3]) < 45


def quant(c, step=12):
    return tuple(min(255, int(round(v / step) * step)) for v in c[:3])


def from_screenshot(src, dst):
    """Amostra o print do samcli (célula 9x19 px, meio-bloco = 9x9.5 px)."""
    import numpy as np
    im = np.asarray(Image.open(src).convert("RGB")).astype(int)
    W, X0, H, Y0 = 9.0, 143.5, 9.5, 45.0
    rows = []
    y = Y0
    while y + H <= 450:          # para antes da linha "S O L U T I O N S"
        row = []
        x = X0
        while x + W <= 935:
            cx, cy = int(x + W / 2), int(y + H / 2)
            patch = im[cy - 2:cy + 3, cx - 2:cx + 3].reshape(-1, 3)
            row.append(tuple(int(v) for v in np.median(patch, axis=0)))
            x += W
        rows.append(row)
        y += H
    # recorta bordas vazias
    def empty_row(r): return all(max(c) < 45 for c in r)
    while rows and empty_row(rows[-1]):
        rows.pop()
    while rows and empty_row(rows[0]):
        rows.pop(0)
    out = Image.new("RGBA", (len(rows[0]), len(rows)), (0, 0, 0, 0))
    for yy, r in enumerate(rows):
        for xx, c in enumerate(r):
            if max(c) >= 45:
                out.putpixel((xx, yy), (*c, 255))
    out.save(dst)
    print(f"grid {out.size[0]}x{out.size[1]} salvo em {dst}")


def generate(src, dst):
    im = Image.open(src).convert("RGBA")
    w, h = im.size
    grid = [[None if is_empty(im.getpixel((x, y))) else quant(im.getpixel((x, y)))
             for x in range(w)] for y in range(h)]

    # remove colunas vazias nas bordas
    used = [x for x in range(w) if any(grid[y][x] for y in range(h))]
    x0, x1 = min(used), max(used)
    grid = [r[x0:x1 + 1] for r in grid]
    w = x1 - x0 + 1

    palette = sorted({c for r in grid for c in r if c})
    if len(palette) > len(CHARS):
        # reduz paleta com quantização mais grossa até caber
        step = 16
        while True:
            pal = sorted({quant(c, step) for r in grid for c in r if c})
            if len(pal) <= len(CHARS):
                grid = [[quant(c, step) if c else None for c in r] for r in grid]
                palette = pal
                break
            step += 4
    index = {c: CHARS[i] for i, c in enumerate(palette)}

    # separa ícone e wordmark pela primeira faixa de linhas vazias
    blank = [all(c is None for c in r) for r in grid]
    split = None
    for y in range(h):
        if blank[y]:
            split = y
            break
    word_start = None
    if split is not None:
        for y in range(split, h):
            if not blank[y]:
                word_start = y
                break

    # posição das letras de "SOLUTIONS" (colunas do grid original, menos o recorte)
    solutions_col = 19 - x0

    with open(dst, "w", encoding="utf-8") as f:
        f.write("// Gerado por tools/gen_banner.py — não edite à mão.\n")
        f.write(f"// Logo: {w}x{h} blocos, {len(palette)} cores.\n\n")
        f.write(f"static constexpr int kLogoW = {w};\n")
        f.write(f"static constexpr int kLogoH = {h};\n")
        f.write(f"static constexpr int kIconEnd = {split if split is not None else h};      // linhas [0, kIconEnd) = ícone\n")
        f.write(f"static constexpr int kWordStart = {word_start if word_start is not None else 0};  // linhas [kWordStart, kLogoH) = 'CentralTech'\n")
        f.write(f"static constexpr int kSolutionsCol = {solutions_col};\n\n")
        f.write(f'static const char kPaletteChars[] = "{CHARS[:len(palette)]}";\n')
        f.write("static const unsigned int kPalette[] = {\n")
        for i in range(0, len(palette), 6):
            chunk = palette[i:i + 6]
            f.write("    " + ", ".join(f"0x{r:02X}{g:02X}{b:02X}" for r, g, b in chunk) + ",\n")
        f.write("};\n\n")
        f.write("static const char* const kLogo[] = {\n")
        for r in grid:
            line = "".join(index[c] if c else " " for c in r)
            f.write(f'    "{line}",\n')
        f.write("};\n")
    print(f"{dst}: {w}x{h}, paleta {len(palette)}, ícone até {split}, wordmark de {word_start}")


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "--from-screenshot":
        from_screenshot(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 3:
        generate(sys.argv[1], sys.argv[2])
    else:
        print(__doc__)
        sys.exit(1)
