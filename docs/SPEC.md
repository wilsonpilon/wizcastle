# Especificação Técnica (`SPEC.md`)
## The Wizard's Castle — MSX2+ Deluxe Edition

---

## 1. Visão Geral do Sistema
**The Wizard's Castle — MSX2+ Deluxe Edition** é uma recriação gráfica e sonora para computadores **MSX2+** (e compatíveis superiores como MSX turbo R) rodando **MSX-DOS 2**, mantendo 100% de fidelidade às regras tanto do port em C++ (**WinWiz**, de Daniel D. Miller, 1985–2025) quanto do jogo original em BASIC (**The Wizard's Castle**, de Joseph R. Power, *Recreational Computing*, Jul/Ago 1980).

### 1.1 Requisitos de Hardware, Disco de Boot Universal e Diagnóstico Gráfico (`v1.6`)
| Componente | Requisito Mínimo (Obrigatório) | Recomendado (Ideal) | Comportamento do Diagnóstico Gráfico (`SCREEN 7` / `SCREEN 2`) |
| :--- | :--- | :--- | :--- |
| **Geração MSX** | **MSX2+** (`ID >= 2` em `002Dh`) | **MSX2+** ou **MSX turbo R** | Em MSX2/2+/TR renderiza dashboard em **`SCREEN 7` (`512×212`)**; em MSX1 renderiza dashboard em **`SCREEN 2` (`256×192`)** com badges `[✓]`/`[✗]`/`[!]`/`[i]`. |
| **VDP (Vídeo)** | **Yamaha V9958** com **128 KB VRAM** | **Yamaha V9958** (128 KB VRAM) | Verifica `VDP_GetVersion() == VDP_VERSION_V9958` e `g_MODE` (`128 KB VRAM`). Em MSX1 (`TMS9918A`) usa escrita direta `R#0..R#7` para evitar espelhamento de 3 bits. |
| **Sistema Operacional** | **MSX-DOS 2.20+** ou **Nextor** | **MSX-DOS 2.20+** / **Nextor 2.1+** | O disco `DOS2_wizcastle.dsk` inclui `MSXDOS2.SYS` + `COMMAND2.COM` (DOS2), `MSXDOS.SYS` + `COMMAND.COM` (DOS1) e `AUTOEXEC.BAS` (Disk-BASIC `<64KB` fallback). |
| **RAM (Memory Mapper)** | **256 KB** (mín. 8 segmentos livres) | **1024 KB (1 MB)** (64 segmentos) | Verifica `DOSMapper_Init()`, lista Slot, total de RAM (KB) e RAM livre (KB). |
| **Áudio Base** | **PSG** (AY-3-8910 / YM2149) | **PSG** (3 canais + gerador de ruído) | Testa leitura/escrita nos registradores do PSG (`A0h–A2h`). |
| **Áudio FM** | Opcional (Recomendado) | **MSX-MUSIC (YM2413 / OPLL)** | Detecta chip interno ou cartucho externo (FM-PAC, habilitando bit 0 de `7FF6h`) e exibe o Slot/Subslot (`[✓]` ou `[!]`). |
| **Áudio Wavetable** | Opcional (Suportado) | **Konami SCC / SCC+** (K051649) | Varre slots em busca de cartucho SCC/SCC+ e exibe o Slot/Subslot (`[✓]` ou `[i]`). |

---

## 2. Arquitetura de Memória

### 2.1 Espaço de Endereçamento Z80 (64 KB)
Sob o **MSX-DOS 2**, as 4 páginas de 16 KB do Z80 estão configuradas na RAM do Mapper primário:
* **`0000h–00FFh` (Zero Page):** Vetores de sistema (`0000h` Reboot, `0005h` BDOS, `0006h` Top of TPA, `000Ch` RDSLT, `0014h` WRSLT, `001Ch` CALSLT, `0024h` ENASLT, `0038h` ISR).
* **`0100h–7FFFh` (Pages 0 & 1 — 31,75 KB):** Executável principal `WIZCASTL.COM` (código da engine MSXgl, lógica de jogo, combate, UI, áudio e diagnóstico de hardware).
* **`8000h–BFFFh` (Page 2 — 16 KB):** **Janela de Paginação Dinâmica do Memory Mapper** (`DOSMapper_SetPage2(seg)`). Nunca armazena código residente; é chaveada sob demanda para ler sprites, descomprimir telas SCREEN 12, acessar strings EN/PT e tocar streams musicais `.LVM`.
* **`C000h–F37Fh` (Page 3 — ~13 KB):** Variáveis globais (`player`, `castle[8][8][8]`, buffers de estado), pilha do Z80 e área residente do MSX-DOS 2 + Jump Table da Extended BIOS.

### 2.2 Organização dos Segmentos do Memory Mapper (`WIZDATA.DAT` — `v2.1`)
O arquivo `emul/WIZDATA.DAT` (**114.688 bytes = 112 KB**, gerado por `tools/build_assets.ps1` a partir de `winwiz/tiles32.png` e `winwiz/images.png`) é carregado em **7 segmentos de 16.384 bytes (16 KB)** alocados via `DOSMapper_Alloc(DOS_ALLOC_USER, DOS_SEGSLOT_PRIM | DOS_SEGSLOT_THISFIRST, &g_WizSegs[i])`:

| Índice Lógico (`g_WizSegs`) | Conteúdo do Segmento (16 KB) | Descrição Técnica (`0x8000–0xBFFF` quando paginado) |
| :---: | :--- | :--- |
| **`0: WIZ_SEG_SPR32_0`** | Sprites `32×32` IDs `0..15` (`SCREEN 8` GRB 3-3-2) | `16 × 1.024 bytes = 16.384 B` (`UNSEEN_ROOM` até `KRESH`). |
| **`1: WIZ_SEG_SPR32_1`** | Sprites `32×32` IDs `16..31` (`SCREEN 8` GRB 3-3-2) | `16 × 1.024 bytes = 16.384 B` (`GOBLIN` até `BLUE_FLAME`). |
| **`2: WIZ_SEG_SPR32_2`** | Sprites `32×32` IDs `32..39` + Tag `"WIZDATA1"` | `8 × 1.024 bytes = 8.192 B` (`PALANTIR` até `ELF`) + assinatura `"WIZDATA1"` em `0x3F00`. |
| **`3: WIZ_SEG_MAP_TILES`** | 40 Tiles `16×16` do Mapa + 10 Ícones `16×16` de Tesouro | `40 × 256 B = 10 KB` em `0x0000` + `10 × 256 B = 2,5 KB` em `0x2800` (copiados para VRAM Page 1 para blit via `HMMM`). |
| **`4: WIZ_SEG_YJK_COMBAT`** | Arte `160×100` de Abertura/Combate (`SCREEN 12` YJK) | `160 × 100 = 16.000 bytes` codificados em pacotes YJK de 4 pixels (19.268 cores). |
| **`5: WIZ_SEG_YJK_VICTORY`** | Arte `160×100` de Vitória (`SCREEN 12` YJK) | `160 × 100 = 16.000 bytes` codificados em pacotes YJK de 4 pixels (`TILE_VICTORY`). |
| **`6: WIZ_SEG_YJK_DEATH`** | Arte `160×100` de Game Over (`SCREEN 12` YJK) | `160 × 100 = 16.000 bytes` codificados em pacotes YJK de 4 pixels (`TILE_DEATH`). |

> **Regra de Segurança MSX-DOS 2 BDOS + Page 2 (`src/wiz_mapper.c`):** Durante a leitura de `WIZDATA.DAT` via BDOS `48h` (`_READ`), o segmento original da TPA (`g_OrigSegPage2`) é mantido ativo em `0x8000–0xBFFF` e os dados são lidos em blocos de 2.048 bytes para `g_DiskBuf[2048]` na Page 1 (abaixo de `0x8000`), preservando o registrador `IX` (`push ix`/`pop ix`) antes de copiar cada bloco para o segmento alvo e liberando todos os segmentos (`Wiz_MapperCleanup`) ao sair para o MSX-DOS 2.

### 2.3 Mapa da VRAM no V9958 (128 KB)
Durante a execução principal em **SCREEN 8** (`256×212`, 256 cores, 256 bytes por linha):
* **VRAM Page 0 (`00000h–0FFFFh`, `Y = 0..255`):**
  * `Y = 0..211` (`00000h–0D3FFh`): Framebuffer visível.
  * `Y = 212..255` (`0D400h–0FFFFh`): Buffer off-screen para rolagem rápida do terminal e double-buffering da Viewport `48×48`.
  * `0FA00h–0FFFFh`: Tabelas de Sprites Modo 2 (Sprite Attribute Table em `0FA00h`, Sprite Color Table em `0F800h`, Sprite Pattern Table em `0F000h`).
* **VRAM Page 1 (`10000h–1FFFFh`, `Y = 256..511`):**
  * `Y = 256..303`: Atlas dos **40 Tiles `16×16`** do mapa (16 tiles por linha de 256 px = 3 linhas de 16 px = 48 linhas de altura).
  * `Y = 304..319`: **10 Ícones `16×16` de Tesouros Conquistados** (borda dourada e cores vivas, `X = 0..159`).
  * `Y = 320..335`: **10 Ícones `16×16` de Tesouros Não Encontrados** (silhueta escura e borda cinza, `X = 0..159`).
  * `Y = 336..383`: **2 Cenários `48×48` da Viewport da Sala** (`X = 0..47` = Exploração com moldura dourada; `X = 48..95` = Combate com moldura carmesim).

---

## 3. Especificação de Vídeo e Interface

### 3.1 Telas em SCREEN 12 (MSX2+ YJK, 19.268 Cores — `v3.2`)
* **Modo VDP:** `VDP_MODE_SCREEN12` (`R#0 = 0Eh`, `R#1 = 40h`, `R#25 = 08h` — bit `YJK = 1`, `YAE = 0`), inicializado via `Wiz_VideoInitScreen12()`, `Wiz_VideoDrawYJKBar()` e `Wiz_VideoShowScreen12()`.
* **Uso:**
  1. **Splash Screen (Abertura):** Exibe a arte de abertura com o pergaminho da lenda de Zot e menu inferior de configuração inicial (Idioma `EN/PT`, Regras `WinWiz/1980`, Atributos `[S]elect/[D]erelict`).
  2. **Victory Screen:** Exibe a cena do riacho na floresta (`TILE_VICTORY` de [images.png](../winwiz/images.png)) quando o jogador escapa vivo com o *Orb of Zot*.
  3. **Death Screen (Game Over):** Exibe a cena do cemitério (`TILE_DEATH` de [images.png](../winwiz/images.png)) com o epitáfio contextual (morte comum, morte segurando a *Runestaff* que colapsa o castelo, ou morte segurando o *Orb of Zot* que suga a alma do jogador).

### 3.2 Tela Principal em SCREEN 8 (256×212, 256 Cores GRB 3-3-2)
* **Formato de Pixel SCREEN 8:** `GGGRRRBB` (Bits 7–5: Verde 0..7, Bits 4–2: Vermelho 0..7, Bits 1–0: Azul 0..3).
* **Regiões de Tela:**
  1. **Barra de Título (`X: 0..255, Y: 0..9`):** Nome do Castelo atual, coordenadas `X, Y, LVL` (ou `?` quando cego/esquecido) e indicador do modo de regra.
  2. **Mapa 8×8 do Andar (`X: 4..131, Y: 14..141` — `128×128 px`):**
     - Cada sala `(col, row)` ocupa `16×16` pixels (`X = 4 + col*16`, `Y = 14 + row*16`).
     - Salas desconhecidas (`is_known == false`) exibem o tile de névoa/pedra escura (`UNSEEN_ROOM`).
     - Salas vazias conhecidas (`EMPTY_ROOM`) exibem piso de pedra da masmorra.
     - Salas com objetos, monstros, escadas, entrada ou tesouros exibem o tile `16×16` correspondente copiado via `VDP_CommandHMMM` da VRAM Page 1.
     - A posição atual do jogador é destacada com o tile da sua raça (`Human`, `Dwarf`, `Hobbit`, `Elf` ou `alt_race`) e uma moldura animada em **Sprite Modo 2** de 16×16 pixels.
  3. **Viewport da Sala / Combate (`X: 136..183, Y: 14..61` — `48×48 px`):**
     - Desenha o fundo contextual `48×48` (paredes da sala da masmorra, escadaria, poço, portal mágico, tenda do mercador) e centraliza o sprite detalhado **`32×32`** do objeto, monstro ou personagem atual.
     - Durante o combate, exibe abaixo da Viewport (`X: 136..183, Y: 63..78`) o nome, nível (`L1..L8`) e barra de HP do monstro enfrentado.
  4. **Painel de Status / HUD (`X: 186..253, Y: 14..78`):**
     - Atributos: `ST` (Strength), `DX` (Dexterity), `IQ` (Intelligence), `HP` (Hit Points).
     - Equipamento: `WP` (Hands / Dagger / Mace / Sword / A Book), `AR` (Prayers / Leather / Chainmail / Plate + pontos de armadura).
     - Recursos: `FL` (Flares), `LM` (Lamp ON/OFF), `GP` (Gold Pieces) e indicadores de maldição ativa (`Lethargy`, `Leech`, `Amnesia`, `Blind`).
  5. **Bandeja de Tesouros (`X: 136..253, Y: 80..141`):**
     - Exibe os **8 Tesouros** (*Ruby Red, Norn Stone, Pale Pearl, Opal Eye, Green Gem, Blue Flame, Palantir, Silmaril*) + **Runestaff** + **Orb of Zot**.
     - Tesouros ainda não encontrados aparecem em silhueta escura; tesouros em posse do jogador brilham com seus ícones coloridos.
  6. **Terminal de Mensagens (`X: 0..255, Y: 144..211` — 8 linhas de `6×8 px`, 42 colunas):**
     - Suporta rolagem vertical acelerada via `VDP_CommandYMMM` e as 8 cores de mensagens definidas em `wizard.h`.

---

## 4. Especificação Completa das Regras do Jogo (100% Compatível)

### 4.1 Estrutura de Dados do Castelo e Jogador
* **Castelo:** Cubo `castle[8][8][8]` (512 salas).
* **Geração (`Castle_Init` — idêntica a [CastleInit.cpp](../winwiz/CastleInit.cpp)):**
  1. Todas as 512 salas iniciam como `EMPTY_ROOM` (`is_known = false`).
  2. Entrada (`CASTLE_ENTRANCE`) fixa em `castle[0][3][0]` (`X=0, Y=3, Level=0`, `is_known = true`).
  3. **Escadas:** Em cada nível `0..6`, posiciona 2 pares de escadas conectadas (`castle[X][Y][level] = STAIRS_DOWN` e `castle[X][Y][level+1] = STAIRS_UP`).
  4. **Monstros (por andar `0..7`):** 1 de cada um dos 12 monstros (`KOBOLD` ID 13 até `DRAGON` ID 24).
  5. **Objetos de Sala e Mercadores (por andar `0..7`):** 3 de cada um dos 8 objetos (`POOL`, `CHEST`, `GOLD_PIECES`, `FLARES`, `WARP`, `SINKHOLE`, `CRYSTAL_ORB`, `BOOK`) + 3 `VENDOR`.
  6. **Tesouros (no castelo todo):** 1 de cada um dos 8 tesouros (`RUBY_RED` ID 26 até `SILMARIL` ID 33) em salas vazias aleatórias.
  7. **Maldições:** 3 salas vazias aleatórias recebem `curse_rooms[0..2]` (*Lethargy*, *Leech*, *Forgetfulness*).
  8. **Runestaff:** Associada a uma sala aleatória que contenha um monstro (`MONSTER_BASE..MONSTER_END`). Matar esse monstro concede a *Runestaff*.
  9. **Orb of Zot:** Associado a uma sala vazia aleatória (`orb_room`). No modo `Classic 1980`, essa sala aparenta conter um `WARP` até ser acessada com a *Runestaff*.

### 4.2 Criação de Personagem
* **Padrão "Derelict" (`[D]`):** Raça `"DERELICT"` (índice 0), `ST=8, DX=16, IQ=8, HP=16 (WinWiz) / 8 (1980)`, `Armour=Leather (1, 7 pts)`, `Weapon=Sword (3)`, `Lamp=ON`, `Gold=0`.
* **Seleção Customizada (`[S]`):**
  - Inicia com `60 GP` e `8 pontos extras` para distribuir até o máximo de `18` por atributo:
    - **Human:** `ST=8, DX=8, IQ=8`
    - **Dwarf:** `ST=10, DX=8, IQ=6`
    - **Hobbit:** `ST=4, DX=8, IQ=12`
    - **Elf:** `ST=6, DX=8, IQ=10`
  - Custos de Armadura: `None (0 GP, 0 pts)`, `Leather (10 GP, 7 pts)`, `Chainmail (20 GP, 14 pts)`, `Plate (30 GP, 21 pts)`.
  - Custos de Arma: `None (0 GP)`, `Dagger (10 GP)`, `Mace (20 GP)`, `Sword (30 GP)`.
  - `Lamp`: `20 GP`. `Flares`: `1 GP` por flare (com o ouro restante no modo Classic 1980 ou opcional no modo WinWiz).

### 4.3 Fórmulas de Combate e Magia
* **Nível do Monstro:** `L = monster_id - MONSTER_BASE + 1` (`1` para Kobold até `12` para Dragon; `Vendor` usa nível máximo ao ser atacado).
* **HP Inicial do Monstro:**
  - Modo **WinWiz:** `L + random(2 * L) + 1`.
  - Modo **Classic 1980:** `L + 2` (Vendor = `15`).
* **Ataque Físico do Jogador (`[A]ttack`):**
  - Se `weapon == 0` (*Hands*) ou `has_book == true`: não causa dano e o monstro ataca.
  - Modo **WinWiz:**
    - `player_attack = 5 * (1 + random(DX))`
    - `monster_resist = 6 * L + random(6 * L)`
    - Se `player_attack >= monster_resist`: causa `1 + random(weapon_damage[weapon])` onde `weapon_damage = {0, 6, 8, 10}`.
  - Modo **Classic 1980:**
    - Acerta se `DX >= (1 + random(20)) + (3 * is_blind)`. Dano = `weapon` (`1, 2 ou 3`).
  - **Quebra de Arma:** Contra `GARGOYLE` ou `DRAGON`, há `1/8` (`random(8) == 4`) de chance da arma quebrar (`weapon = 0`).
* **Turno do Monstro:**
  - Se preso em teia (`web_turns > 0`), decrementa `web_turns` e não ataca.
  - Modo **WinWiz:**
    - `monster_attack = 1 + random(L * 11)`
    - `player_resist = random(4 * DX) + armour_resist[armour]` (`{0, 4, 10, 20}`)
    - Se `monster_attack > player_resist`: causa `damage = 1 + random(L)` no `HP` e desgasta `0..2` pontos de `armour_points`.
  - Modo **Classic 1980:**
    - Acerta se `(1 + random(20)) > DX - 3*is_blind` (ou similar inverso); dano reduzido pela armadura até os pontos de armadura acabarem.
* **Magias (`[C]ast Spell`):**
  - **`[W]eb`:** Requer `IQ >= 10`. Custa `1 IQ`. Prende o monstro por `random(9)` turnos.
  - **`[F]ireball`:** Requer `IQ >= 12`. Custa `1 IQ` e `1 DX` (se `DX <= 1`, jogador morre). Causa `max(5, 1 + random(monster_hp))` de dano.
  - **`[D]eathspell`:** Se `IQ < 15` ou `IQ < monster_hp`, o feitiço sai pela culatra (`IQ = 0`) e mata o jogador! Caso contrário, mata o monstro instantaneamente (`monster_hp = 0`).
* **Suborno (`[B]ribe`):** Se o jogador possui ao menos 1 tesouro, o monstro pede um tesouro aleatório em troca de encerrar o combate (e acalma os `VENDOR`s se for um mercador).
* **Fuga (`[R]etreat`):** O monstro ganha um último ataque livre; se o jogador sobreviver, foge para uma sala vizinha aleatória.

---

## 5. Especificação de Áudio (PSG + FM OPLL + SCC + ayFX)

* **Motor de Música (`wiz_audio.c`):**
  - Na inicialização, detecta **MSX-MUSIC (YM2413)** via `MSXMusic_Initialize()` e **Konami SCC** via `SCC_Initialize()`.
  - Reproduz arquivos `.LVM` (Light VGM) na interrupção `VBlank` (50/60 Hz) combinando canais FM (YM2413) + canais A/B do PSG (e SCC quando disponível).
* **Motor de Efeitos Sonoros (`ayFX`):**
  - Configurado em modo fixo no **Canal C do PSG** (`ayFX_SetChannel(PSG_CHANNEL_C)`).
  - No hook `VBlank`, `LVGM_Decode()` atualiza o buffer de registradores do PSG, seguido de `ayFX_Update()` (que sobrescreve apenas o Canal C enquanto houver um efeito sonoro ativo) e `PSG_Apply()`.
  - Dessa forma, os efeitos sonoros (passos, espadas, magias, moedas, explosões) tocam com prioridade zero-latência sem interromper a harmonia principal no FM e nos canais A/B do PSG.
