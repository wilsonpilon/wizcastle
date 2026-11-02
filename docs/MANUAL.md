# Manual Completo (`MANUAL.md`)
## The Wizard's Castle — MSX2+ Deluxe Edition

---

## Parte I — Guia Técnico e de Compilação

### 1. Estrutura de Diretórios do Repositório
```
C:\dos\wizcastle\
├── docs\                         # Documentação técnica e manual do projeto
│   ├── PLAN.md                   # Plano de arquitetura e implementação
│   ├── SPEC.md                   # Especificação técnica detalhada
│   ├── OUTLINE.md                # Status de desenvolvimento e roadmap
│   └── MANUAL.md                 # Este manual completo (compilação + jogo)
├── winwiz\                       # Código-fonte C++ de referência (WinWiz por Daniel D. Miller)
└── MSXgl\                        # Engine MSXgl e ferramentas de compilação
    └── projects\
        └── wizcastle\            # Projeto MSX2+ Deluxe Edition
            ├── build.bat         # Script de compilação para Windows
            ├── project_config.js # Configuração de build MSXgl (Target: DOS2)
            ├── msxgl_config.h    # Configuração de módulos da engine MSXgl
            ├── wizcastle.c       # Ponto de entrada e Diagnóstico de Hardware
            ├── src\              # Módulos em C do jogo
            └── out\              # Binários gerados (WIZCASTL.COM e wizcastle.dsk)
```

### 2. Como Compilar o Projeto
O repositório já inclui todas as ferramentas necessárias dentro de `MSXgl/tools/` (compilador **SDCC** para Z80, conversor **MSXhex** e criador de imagens de disco **msxtar**). É necessário apenas ter o **Node.js** instalado no Windows.

1. Abra o PowerShell ou Prompt de Comando na pasta do projeto:
   ```powershell
   cd C:\dos\wizcastle\MSXgl\projects\wizcastle
   ```
2. Execute o script de build:
   ```powershell
   .\build.bat
   ```
3. Os arquivos gerados estarão em `C:\dos\wizcastle\MSXgl\projects\wizcastle\out\`:
   * **`wizcastle.com`** (`WIZCASTL.COM`): Executável para MSX-DOS 2.
   * **`wizcastle.dsk`**: Imagem de disquete de 720 KB pronta para rodar em qualquer emulador de MSX2+ (openMSX, Emulicious, blueMSX, WebMSX) ou gravar em SD/Floppy para um MSX2+ real (Carnivore2, MegaFlashROM SCC+ SD, Beer IDE, Rookie Drive, etc.).

### 3. Requisitos de Máquina (Emulador ou MSX Real)
Para a experiência completa, configure seu emulador ou hardware real com:
* **Máquina:** **MSX2+** (ex: *Panasonic FS-A1WX*, *FS-A1WSX*, *Sony HB-F1XDJ* ou *Sanyo PHC-70FD*) ou **MSX turbo R** (*FS-A1ST* / *FS-A1GT*).
* **Vídeo:** VDP **Yamaha V9958** com **128 KB de VRAM**.
* **Memória RAM:** **Memory Mapper de 1024 KB (1 MB)** (mínimo absoluto de 256 KB).
* **Sistema Operacional:** **MSX-DOS 2.20+** ou **Nextor 2.1+**.
* **Som:**
  * **PSG** (AY-3-8910 / YM2149 — padrão em todo MSX) para efeitos sonoros e base musical.
  * **MSX-MUSIC (YM2413 / OPLL — FM)** interno ou via cartucho (FM-PAC / Carnivore2) para a trilha sonora FM.
  * **Konami SCC / SCC+** (opcional, detectado automaticamente se inserido em um slot).

### 4. Utilitário Gráfico de Diagnóstico de Hardware, Memory Mapper (`WIZDATA.DAT`) e Disco Universal (`v2.1`)
A imagem de disco **`emul/dsk/DOS2_wizcastle.dsk`** conta com **Boot Universal em 3 camadas** e já traz embarcado o banco gráfico **`WIZDATA.DAT` (`112 KB`, 7 segmentos de 16 KB)**:
1. **MSX-DOS 2 (`MSXDOS2.SYS` + `COMMAND2.COM`):** Inicia o `WIZCASTL.COM` em máquinas MSX2 / MSX2+ / turbo R com MSX-DOS 2 ou Nextor.
2. **MSX-DOS 1 (`MSXDOS.SYS` + `COMMAND.COM`):** Inicia o `WIZCASTL.COM` em máquinas MSX1 / MSX2 / MSX2+ que possuem apenas ROM de MSX-DOS 1 e `>= 64 KB` de RAM.
3. **MSX Disk-BASIC (`AUTOEXEC.BAS`):** Caso a máquina tenha `< 64 KB` de RAM (ex: MSX1 de 16 KB ou 32 KB sem RAM na Página 0) e caia no Disk-BASIC, executa automaticamente o `AUTOEXEC.BAS` exibindo o relatório de incompatibilidade.

Ao executar `WIZCASTL.COM`, o programa abre automaticamente um **Dashboard Gráfico Adaptativo**:
* **Em MSX2 / MSX2+ / MSX turbo R (`V9938` / `V9958`):** Renderiza um painel gráfico em **`SCREEN 7` (`512×212`, 16 cores)** com cartões e *badges* gráficos `20×12` pixels (`[✓]` Verde, `[✗]` Vermelho, `[!]` Amarelo e `[i]` Ciano).
* **Em MSX1 (`TMS9918A`):** Renderiza um painel gráfico em **`SCREEN 2` (`256×192`, 32×24 tiles)** com ícones gráficos customizados `8×8` (`[✓]`, `[✗]`, `[!]`, `[i]`) usando acesso direto aos registradores `R#0..R#7` (evitando o espelhamento de 3 bits do `TMS9918A`).
* **Itens Inspecionados:**
  1. **OS / MSX-DOS:** Verifica se está rodando sob MSX-DOS 2 / Nextor (`Kernel >= 2.20`) e exibe o topo da TPA (`0006h`).
  2. **MSX Generation:** Verifica a geração da máquina (`MSX1`, `MSX2`, `MSX2+`, `MSX turbo R`) e a frequência vertical (`50Hz` PAL / `60Hz` NTSC).
  3. **Video / VDP & VRAM:** Identifica o VDP (`TMS9918A`, `V9938`, `V9958`) e a quantidade de VRAM (`16 KB`, `64 KB`, `128 KB`).
  4. **Memory Mapper:** Consulta a Extended BIOS (`ALL_SEG` / `GET_VAR`) e mostra o Slot, RAM total em KB (ex: `1024 KB`), RAM livre em KB e status do carregamento de `WIZDATA.DAT` (`112 KB / 7 segs`).
  5. **Chips de Áudio (PSG, FM OPLL, Konami SCC):** Testa os registradores do PSG e escaneia todos os slots primários e expandidos em busca do **MSX-MUSIC (YM2413)** e do **Konami SCC**, exibindo o Slot/Subslot exato onde cada chip foi encontrado.
* **Atalhos Interativos (quando compatível):**
  * **`[V]`:** Inicia o **Showcase do Motor de Vídeo da Fase 3 (`Wiz_VideoRunShowcase` em `src/wiz_video.c`)**, demonstrando as telas de evento em `SCREEN 12` (`[1]` Splash, `[2]` Victory, `[3]` Game Over em 19.268 cores YJK — `v3.2`) seguidas da inicialização da `SCREEN 8`, cache na VRAM Page 1 e as 6 regiões chanfradas da interface (`v3.1`).
  * **`[T]`:** Aloca 7 segmentos na Memory Mapper (`112 KB`), carrega `WIZDATA.DAT` com barra gráfica de progresso e executa a demonstração interativa:
    - **Na `SCREEN 12` (19.268 cores YJK):** Pressione **`[1]`** (Combat), **`[2]`** (Victory) ou **`[3]`** (Death) para alternar instantaneamente entre as artes YJK dos Segmentos 4, 5 e 6 da Mapper RAM sem acesso a disco; qualquer outra tecla avança para a `SCREEN 8`.
    - **Na `SCREEN 8` (256 cores GRB):** Exibe o mapa `8×8` com os 40 tiles `16×16` reais cacheados na VRAM Page 1 (copiados via `HMMM`), os 10 ícones `16×16` de tesouros na bandeja e permite pressionar **`[1]`** (Dragon), **`[2]`** (Balrog), **`[3]`** (Chest) ou **`[4]`** (Elf) para trocar em tempo real o sprite `32×32` da Viewport paginando os Segmentos 0, 1 e 2 da Mapper!
  * **`[S]`:** Reproduz o teste sonoro polifônico (`PSG + FM + SCC`).
  * **`[Q]` / `[ESC]`:** Libera todos os segmentos alocados na Memory Mapper (`Wiz_MapperCleanup`), restaura a fonte original da `SCREEN 0` (salva em `0xD800` da VRAM) e retorna limpo ao prompt do MSX-DOS 2.

---

## Parte II — Manual do Jogador: The Wizard's Castle

### 1. A Lenda do Castelo de Zot
> *"Há muitos ciclos, no reino de N'dic, o gnômico mago Zot forjou seu grande **Orbe do Poder**. Ele logo desapareceu, deixando para trás seu vasto castelo subterrâneo repleto de monstros famintos, tesouros fabulosos e o incrível **Orbe de Zot** (`Orb of Zot`). Desde então, muitos jovens corajosos se aventuraram no Castelo do Mago. Até hoje, **nenhum** jamais emergiu vitoriosamente! CUIDADO!!"*

Seu objetivo é adentrar um dos 10 castelos subterrâneos de Zot (estruturados como um labirinto tridimensional de **8 andares**, cada um com uma grade de **8×8 salas**), derrotar o monstro guardião que carrega o **Cajado Rúnico (*Runestaff*)**, usar o poder do cajado para alcançar o **Orbe de Zot (*Orb of Zot*)** e escapar vivo do castelo!

---

### 2. Modos de Jogo Configuráveis na Abertura
Na tela de abertura, você pode escolher:
1. **Idioma:** **Inglês** (textos originais) ou **Português (PT-BR)**.
2. **Conjunto de Regras:**
   * **Modo WinWiz (Padrão):** Segue exatamente as regras e o balanceamento em C++ criados por Daniel D. Miller (`HP = 2 × ST`, paredes sólidas nas bordas `0..7` do mapa, saída vitoriosa através de uma piscina mágica quando estiver de posse do *Orb of Zot*).
   * **Modo Classic 1980:** Segue as regras originais de Joseph R. Power de 1980 (mapa circular *wrap-around* onde atravessar a borda norte leva à borda sul, *Orb of Zot* disfarçado como um *Warp*, saída pela entrada do castelo em `(0,3,0)` ao norte).

---

### 3. Criação do Personagem
Ao iniciar, você pode pressionar:
* **`[D]` (Derelict Defaults):** Inicia imediatamente com a configuração favorita do autor do WinWiz (`ST=8, DX=16, IQ=8`, *Leather Armor*, *Sword* e *Lamp*).
* **`[S]` (Select Attributes):** Abre a tela de criação customizada com **60 peças de ouro (GP)** e **8 pontos extras** para distribuir:

#### 3.1 Raças e Atributos Iniciais
| Raça | Strength (`ST`) | Dexterity (`DX`) | Intelligence (`IQ`) | Pontos Extras | Perfil Recomendado |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Human (Humano)** | 8 | 8 | 8 | +8 | Equilibrado entre combate e magia. |
| **Dwarf (Anão)** | 10 | 8 | 6 | +8 | Alta resistência física e pontos de vida. |
| **Hobbit** | 4 | 8 | 12 | +8 | Especialista em inteligência/magia inicial, porém frágil. |
| **Elf (Elfo)** | 6 | 8 | 10 | +8 | Bom conjurador com agilidade equilibrada. |

* **Strength (`ST`):** Determina sua força e seus Pontos de Vida (`HP = 2 × ST` no modo WinWiz, ou `HP = ST` no modo Classic 1980). Máximo: `18`.
* **Dexterity (`DX`):** Determina sua chance de acertar ataques físicos e de esquivar dos golpes inimigos. Máximo: `18`.
* **Intelligence (`IQ`):**Necessária para lançar magias em combate (*Web*, *Fireball*, *Deathspell*). Máximo: `18`.

#### 3.2 Equipamentos Iniciais
* **Armaduras (*Armour*):**
  * *None / Prayers* (0 GP) — 0 pts de proteção, +0 defesa.
  * *Leather* (10 GP) — 7 pts de durabilidade, +4 defesa.
  * *Chainmail* (20 GP) — 14 pts de durabilidade, +10 defesa.
  * *Plate* (30 GP) — 21 pts de durabilidade, +20 defesa.
* **Armas (*Weapons*):**
  * *None / Hands* (0 GP) — Inofensivo (faz cócegas nos monstros!).
  * *Dagger* (10 GP) — Dano 1..6.
  * *Mace* (20 GP) — Dano 1..8.
  * *Sword* (30 GP) — Dano 1..10.
* **Lâmpada (*Lamp* — 20 GP):** Essencial! Ilumina automaticamente as 4 salas adjacentes (Norte, Sul, Leste, Oeste) enquanto você caminha e permite espiar salas vizinhas sem entrar nelas.

---

### 4. Comandos de Exploração

| Tecla | Ação | Descrição |
| :---: | :--- | :--- |
| **`Setas`** | Mover (N, S, L, O) | Move o personagem para a sala adjacente na direção escolhida. |
| **`Shift + Setas`** ou **`[L]`** | Olhar (*Look / Shine Lamp*) | Usa a lâmpada para revelar o conteúdo de uma sala adjacente sem precisar entrar nela (cuidado: digitar direção inválida cega você temporariamente!). |
| **`[U]`** | Subir Escadas (*Up*) | Sobe 1 andar (quando estiver em uma sala com `Stairs Up`). |
| **`[D]`** | Descer Escadas (*Down*) | Desce 1 andar (quando estiver em uma sala com `Stairs Down`). |
| **`[F]`** | Acender Sinalizador (*Flare*) | Consome 1 *Flare* para revelar todas as salas ao redor (área 3×3) no mapa. |
| **`[E]`** | Executar / Interagir (*Execute*) | Interage automaticamente com o objeto da sala atual (beber da piscina, abrir livro/baú, olhar no orbe de cristal ou negociar com o mercador). |
| **`[G]` / `[O]` / `[R]` / `[T]`** | Comandos Clássicos | Atalhos clássicos individuais: `[G]`aze Orb, `[O]`pen Book/Chest, D`[R]`ink Pool, `[T]`rade with Vendor. |
| **`[A]`** | Atacar Mercador (*Attack*) | Provoca e ataca um *Vendor* na sala atual (todos os mercadores ficam hostis!). |
| **`[P]`** | Teletransportar (*Teleport*) | Requer a **Runestaff**. Permite digitar as coordenadas `X (0..7)`, `Y (0..7)` e `Z/Level (0..7)` para se teletransportar instantaneamente para qualquer sala do castelo. |
| **`[J]`** | Ver Itens e Status (*Items*) | Lista no terminal sua raça, o nome do castelo atual, tesouros coletados e maldições ativas. |
| **`[H]`** | Ajuda (*Help*) | Exibe o resumo de comandos e referências de tesouros/maldições. |

---

### 5. Objetos e Salas do Castelo
* **Castle Entrance `(0,3,0)`:** O ponto de partida no 1º andar.
* **Stairs Up / Stairs Down:** Escadas que conectam os 8 andares do castelo (existem 2 pares de escadas entre cada andar).
* **Gold Pieces:** Pilhas contendo de `1 a 200 GP`, coletadas automaticamente ao entrar na sala.
* **Flares:** Pacotes com `1 a 5` sinalizadores, coletados automaticamente.
* **Sinkhole (Poço):** Faz você cair imediatamente 1 andar para baixo (se cair no último andar `7`, retorna ao topo `0`).
* **Warp (Vórtice):** Teletransporta você para coordenadas `(X, Y, Z)` aleatórias no castelo.
* **Pool (Piscina Mágica — `[E]` ou `[R]`):** Ao beber de suas águas, você pode ganhar ou perder `1 a 3` pontos de `ST`, `IQ` ou `DX`, mudar de raça, ou — **se já possuir o Orb of Zot** — ver uma porta cintilante para a floresta que permite vencer o jogo!
* **Book (Livro — `[E]` ou `[O]`):** Pode conter um *Manual de Força/Destreza/Inteligência* (elevando o atributo para `18`!), poesias horríveis de Zot, uma revista antiga, um clarão que deixa você **cego** por vários turnos, ou cola mágica que prende o livro às suas mãos impedindo-o de sacar sua arma!
* **Chest (Baú — `[E]` ou `[O]`):**
  * **40% Ouro:** Até `1000 GP`!
  * **30% Gás da Confusão:** Embaralha seus atributos (`ST`, `DX`, `IQ`) e faz você cambalear para uma sala vizinha.
  * **20% Explosão (`KABOOM!`):** Causa de `1 a 6` pontos de dano.
  * **10% A Fumaça de Zot:** A sala se enche de fumaça mágica e a imagem de Zot exige que você digite o **nome do castelo** em que está! Se acertar, você sobrevive; se errar, sua visão escurece para sempre.
* **Crystal Orb (Orbe de Cristal — `[E]` ou `[G]`):** Olhar no orbe revela visões do futuro ou do castelo:
  * Pode revelar o conteúdo de uma sala aleatória `(X, Y, Level)`.
  * Pode mostrar qual monstro do castelo carrega a **Runestaff**!
  * Se você já tiver a **Runestaff**, há 50% de chance de mostrar as coordenadas verdadeiras `(X, Y, Level)` do **Orb of Zot** (e 50% de chance de mostrar coordenadas falsas!).
  * Cuidado: a visão de si mesmo em uma poça de sangue causa dano real!

---

### 6. Os 8 Tesouros Lendários e as 3 Maldições
Espalhados pelo castelo existem **8 tesouros únicos**. Além de valerem muito ouro nos mercadores e servirem para subornar monstros, **5 deles protegem contra maldições e aflições**:

| Tesouro | Efeito Mágico / Cura |
| :--- | :--- |
| **1. The Ruby Red** | Cura e imuniza contra a **Curse of Lethargy** (Maldição da Letargia). |
| **2. The Norn Stone** | Joia ancestral de alto valor comercial. |
| **3. The Pale Pearl** | Cura e imuniza contra a **Curse of the Leech** (Maldição da Sanguessuga, que rouba `0..4 GP` por turno). |
| **4. The Opal Eye** | Cura e imuniza contra a **Cegueira (*Blindness*)** causada por livros ou pela lâmpada. |
| **5. The Green Gem** | Cura e imuniza contra a **Curse of Forgetfulness / Amnesia** (que apaga salas conhecidas do mapa a cada turno). |
| **6. The Blue Flame** | Dissolve a cola mágica de **Livros presos às mãos**, liberando sua arma. |
| **7. The Palantir** | Esfera de visão de altíssimo valor para venda ou suborno. |
| **8. The Silmaril** | A joia mais valiosa do castelo (ofertas de até 12.000 GP no mercador!). |

> **Dica de Ouro:** Evite vender no Mercador o *Ruby Red*, *Pale Pearl*, *Opal Eye*, *Green Gem* e *Blue Flame*, pois suas proteções passivas salvam vidas nos andares profundos!

---

### 7. Monstros e Combate
Existem 12 espécies de monstros no castelo, ordenadas por nível de poder (`L1` a `L12`):
1. **Kobold** (`L1`)
2. **Orc** (`L2`)
3. **Kresh** (`L3`)
4. **Goblin** (`L4`)
5. **Ogre** (`L5`)
6. **Troll** (`L6`)
7. **Minotaur** (`L7`)
8. **Ur-Vile** (`L8`)
9. **Gargoyle** (`L9` — *pele rochosa: 12,5% de chance de quebrar sua arma ao ser atingido!*)
10. **Chimera** (`L10`)
11. **Balrog** (`L11`)
12. **Dragon** (`L12` — *escamas dracônicas: 12,5% de chance de quebrar sua arma ao ser atingido!*)

#### Opções no Turno de Combate:
* **`[A]`ttack:** Golpeia com sua arma equipada.
* **`[R]`etreat:** Tenta fugir para uma sala adjacente (o monstro realiza um ataque de oportunidade antes de você escapar).
* **`[B]`ribe:** Oferece um de seus tesouros para encerrar a luta pacificamente (também acalma todos os *Vendors* caso você tenha irritado um deles!).
* **`[C]`ast Spell:** Abre o menu de magias:
  * **`[W]`eb (Teia):** Requer `IQ >= 10` (consome `1 IQ`). Imobiliza o monstro por até 8 rodadas sem que ele possa revidar!
  * **`[F]`ireball (Bola de Fogo):** Requer `IQ >= 12` (consome `1 IQ` e `1 DX`). Causa dano pesado de fogo ao inimigo.
  * **`[D]`eathspell (Feitiço da Morte):** Requer `IQ >= 15` **e** que seu `IQ` seja maior ou igual ao `HP` atual do monstro. Destrói o inimigo instantaneamente — mas se seu `IQ` for menor que o HP do monstro ou menor que 15, o feitiço ricocheteia e mata você na hora!

---

### 8. Mercadores (*Vendors*)
Ao encontrar um **Vendor**, pressione `[E]` (ou `[T]`) para abrir a loja:
* **Vender Tesouros:** O mercador faz ofertas em ouro para cada joia que você possui.
* **Comprar Equipamentos:** Armas (*Dagger* 1250 GP, *Mace* 1500 GP, *Sword* 2000 GP) e Armaduras (*Leather* 1250 GP, *Chainmail* 1500 GP, *Plate* 2000 GP).
* **Comprar Poções de Atributos:** Cada poção custa **1000 GP** e aumenta de `1 a 6` pontos de `ST`, `DX` ou `IQ` (até o limite de 18).
* **Atacar o Mercador (`[A]`):** Se você derrotar um mercador em combate, saqueia todo o estoque dele (*Plate Armor*, *Sword*, *Lamp*, poções de *ST/DX/IQ* e até `1000 GP`), mas todos os mercadores do castelo ficarão hostis até serem subornados com `[B]ribe`!

---

### 9. Passo a Passo para Vencer o Jogo
1. **Explore e Fortaleça-se:** Colete ouro, abra baús e livros com cautela e use os mercadores para manter `ST`, `DX`, `IQ` próximos de `18` e sua armadura em *Plate (21)*.
2. **Encontre a Runestaff:** Derrote monstros pelo castelo (ou olhe em *Crystal Orbs* para descobrir qual espécie de monstro guarda o cajado). Ao matar o guardião certo, você receberá a mensagem:
   `GREAT ZOT!! You've found the RUNESTAFF!!`
3. **Descubra as Coordenadas do Orb of Zot:** Olhe em uma *Crystal Orb* (`[E]`/`[G]`). Quando você possui a *Runestaff*, a visão nº 6 revelará:
   `You see the ** ORB OF ZOT ** at (X, Y), Level Z!!`
   *(Lembre-se: há 50% de chance da coordenada ser verdadeira; se não for, olhe novamente!).*
4. **Teletransporte-se para o Orbe:** Pressione **`[P]`** e digite as coordenadas `X`, `Y` e `Z` reveladas. Ao pousar na sala exata do Orbe, a *Runestaff* desaparecerá e você obterá **THE ORB OF ZOT**!
5. **Escape do Castelo:**
   * **No Modo WinWiz:** Encontre qualquer **Pool (Piscina)**, pressione `[E]` (ou `[R]`) até ver a porta cintilante para a floresta na água e caminhe em qualquer direção (`N/S/L/O` ou `[D]`) para emergir vitorioso!
   * **No Modo Classic 1980:** Retorne até a **Entrada do Castelo `(0,3,0)`** e caminhe para o Norte (ou use o portal da piscina) para sair triunfante com o *Orb of Zot*!
