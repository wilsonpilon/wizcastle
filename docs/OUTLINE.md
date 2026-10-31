# Roteiro e Acompanhamento do Projeto (`OUTLINE.md`)
## The Wizard's Castle — MSX2+ Deluxe Edition

Este documento concentra o escopo completo do projeto, as novas funcionalidades em relação ao jogo original de 1980 e ao `winwiz`, o que já foi concluído e o que falta implementar.

---

## 1. Novas Funcionalidades ("MSX2+ Deluxe Edition")

Além de ser **100% compatível** com a lógica e regras do jogo original e do port `winwiz`, esta edição traz:

1. **Diagnóstico Inteligente de Hardware na Inicialização (Boot Check):**
   - Inspeciona o computador MSX item a item (Sistema Operacional MSX-DOS 1 vs 2/Nextor, Geração MSX1/2/2+/turbo R, VDP TMS9918/V9938/V9958, VRAM 16/64/128 KB, Memory Mapper e RAM total/livre, PSG, MSX-MUSIC FM/OPLL e Konami SCC) e informa claramente se o sistema atende aos requisitos para rodar o jogo.
2. **Modo de Regras Híbrido Configurável (`WinWiz` vs `Classic 1980`):**
   - Permite jogar com as regras exatas do port C++ de Daniel D. Miller (`WinWiz`) ou com as regras originais de Joseph R. Power de 1980 (`Classic 1980` com mapa *wrap-around*, *Orb of Zot* oculto em *Warp* e saída pelo portão norte).
   - Suporta simultaneamente a tecla contextual `[E]xecute` e os comandos clássicos individuais (`[G]aze`, `[O]pen`, `[T]rade`, `[D]rink`/`[R]`).
3. **Suporte Bilíngue Nativo (`Inglês` e `Português`):**
   - Escolha do idioma na abertura, traduzindo toda a interface, mensagens de combate, visões do Orbe, livros, armadilhas e diálogos sem perder o humor clássico.
4. **Telas de Abertura, Vitória e Game Over em MSX2+ SCREEN 12 (19.268 Cores):**
   - Uso do modo YJK do V9958 para exibir artes ricas comprimidas na Mapper RAM.
5. **Interface Gráfica em SCREEN 8 (256 Cores) com Viewport Ilustrada:**
   - **Mapa 8×8 gráfico** em tempo real com tiles `16×16` coloridos e cursor animado via **Sprite Modo 2** do V9958.
   - **Viewport da Sala (`48×48`)** desenhando o interior da masmorra e o gráfico grande (`32×32`) do monstro, objeto, baú, piscina, mercador ou herói.
   - **Bandeja Visual de Tesouros:** Ícones gráficos para as 8 joias lendárias, *Runestaff* e *Orb of Zot*.
   - **Terminal Colorido Rolável:** Log de mensagens com 8 cores semânticas e rolagem acelerada por comandos de hardware do VDP (`YMMM`/`HMMM`).
6. **Trilha Sonora Polifônica (PSG + FM OPLL + SCC) + Efeitos Sonoros (ayFX):**
   - Música de fundo contínua por contexto (Abertura, Exploração, Combate, Vitória, Derrota).
   - 16 efeitos sonoros de jogo (passos, escadas, espada, magia, dano, moedas, baú, *warp*, *sinkhole*, piscina, fanfarras).

---

## 2. Status de Implementação (O que já foi feito vs. O que falta fazer)

### Fase 1: Documentação, Arquitetura e Diagnóstico de Hardware (STUB)
- [x] Análise completa do código-fonte C++ de `winwiz` (`wizard.h`, `CastleInit.cpp`, `initscrn.cpp`, `keyboard.cpp`, `combat.cpp`, `vendor.cpp`, `wfuncs.cpp`, `winwiz.cpp`).
- [x] Análise da arquitetura e módulos do `MSXgl` (`dos`, `dos_mapper`, `vdp`, `msx-music`, `scc`, `psg`, `lvgm_player`, `ayfx_player`).
- [x] Criação da pasta `docs/` com:
  - [x] `docs/PLAN.md` (Plano de implementação aprovado)
  - [x] `docs/SPEC.md` (Especificação técnica detalhada)
  - [x] `docs/OUTLINE.md` (Este documento de acompanhamento)
  - [x] `docs/MANUAL.md` (Manual completo de compilação e do jogador)
- [x] Criação da estrutura do projeto em `MSXgl/projects/wizcastle/` (`project_config.js`, `msxgl_config.h`, `build.bat`, `wizcastle.c`).
- [x] Implementação e compilação do **STUB de Detecção de Hardware** (`wizcastle.c`), verificando:
  - [x] Versão do MSX-DOS (DOS 1.x vs DOS 2.x / Nextor) e TPA livre
  - [x] Geração do MSX (MSX1, MSX2, MSX2+, MSX turbo R) e frequência VBlank (50Hz / 60Hz)
  - [x] Chip VDP (TMS9918A, V9938, V9958) e quantidade de VRAM (16 KB, 64 KB, 128 KB)
  - [x] Memory Mapper (Extended BIOS, número de segmentos, RAM Total em KB e RAM Livre em KB)
  - [x] Chip de som PSG (AY-3-8910 / YM2149) com teste de registrador e bip sonoro
  - [x] Chip de som FM MSX-MUSIC (YM2413 / OPLL interno ou externo + Slot/Subslot)
  - [x] Chip de som Konami SCC / SCC+ (+ Slot/Subslot)
  - [x] Veredito final de compatibilidade (`READY TO PLAY` vs lista de requisitos ausentes)

---

### Fase 2: Pipeline de Assets e Gerenciador de Memory Mapper (`WIZDATA.DAT`)
- [ ] Criar script `tools/build_assets.js` para extrair os 40 sprites de `winwiz/tiles32.png` (`32×32`, `16×16` e `12×12` em SCREEN 8 GRB 3-3-2).
- [ ] Converter as 3 telas de `winwiz/images.png` + arte de abertura para **SCREEN 12 (YJK)** comprimidas com **ZX0**.
- [ ] Gerar pacote de trilhas `.LVM` (PSG + FM OPLL) e banco de 16 efeitos sonoros `.AFX` (ayFX).
- [ ] Implementar `src/wiz_mapper.c` para alocar segmentos no MSX-DOS 2 e carregar `WIZDATA.DAT` na inicialização com barra de progresso.

---

### Fase 3: Motor de Vídeo (SCREEN 12 & SCREEN 8) e Áudio (PSG + FM + SCC + SFX)
- [ ] Implementar `src/wiz_video.c`:
  - [ ] Exibição de telas SCREEN 12 (Splash, Victory, Game Over).
  - [ ] Inicialização do SCREEN 8 e upload do cache de tiles `16×16` e fontes `6×8` para a VRAM Page 1 (`10000h–1FFFFh`).
  - [ ] Renderização do Mapa 8×8 via `VDP_CommandHMMM` e cursor animado via **Sprite Modo 2**.
  - [ ] Renderização da **Viewport da Sala (`48×48`)** com cenários de masmorra e sprites `32×32` de objetos/monstros/personagens.
  - [ ] Renderização do HUD de atributos, equipamentos, maldições e bandeja dos 10 ícones de tesouros.
  - [ ] Terminal colorido de 8 linhas com rolagem via `VDP_CommandYMMM`.
- [ ] Implementar `src/wiz_audio.c`:
  - [ ] Hook `H_TIMI` (VBlank) integrando `LVGM_Decode()`, `ayFX_Update()` e `PSG_Apply()`.
  - [ ] Disparo dos 16 efeitos sonoros contextuais durante a exploração e combate.

---

### Fase 4: Motor de Regras do Jogo (Exploração, Objetos, Maldições e Combate)
- [ ] Implementar `src/wiz_castle.c`:
  - [ ] Geração procedural 100% compatível do castelo `8×8×8` (`init_castle_contents` e `init_player`).
- [ ] Implementar `src/wiz_gameplay.c`:
  - [ ] Movimentação Norte/Sul/Leste/Oeste (com paredes em `WinWiz` e *wrap-around* em `Classic 1980`) e Escadas (`[U]p` / `[D]own`).
  - [ ] Uso da Lâmpada (`[L]ook` / `Shift+Setas`) e Flares (`[F]lare`).
  - [ ] Interação com Objetos (`[E]xecute` e teclas clássicas `[G]`, `[O]`, `[T]`, `[R]`):
    - [ ] *Crystal Orb* (6 visões + chance de achar o *Orb of Zot*).
    - [ ] *Pool* (8 efeitos + portal de vitória com o *Orb of Zot*).
    - [ ] *Book* (7 efeitos + proteção de *Opal Eye* e *Blue Flame*).
    - [ ] *Chest* (Ouro, gás de confusão `ScrambleAttr`, explosão e enigma da fumaça de Zot).
    - [ ] *Warp* e *Sinkhole*.
  - [ ] Teletransporte com a *Runestaff* (`[P]`).
  - [ ] Sistema de Maldições (*Lethargy*, *Leech*, *Forgetfulness*) e curas automáticas pelos tesouros.
- [ ] Implementar `src/wiz_combat.c`:
  - [ ] Combate completo por turnos (`[A]ttack`, `[R]etreat`, `[C]ast Spell`: *Web*, *Fireball*, *Deathspell*, `[B]ribe`) suportando as fórmulas de `WinWiz` e `Classic 1980`.

---

### Fase 5: Diálogos Interativos, Localização Bilíngue e Testes Finais
- [ ] Implementar `src/wiz_dialogs.c`:
  - [ ] Menu de configuração na abertura (Idioma `EN`/`PT-BR`, Regras `WinWiz`/`Classic 1980`, Atributos `[S]`/`[D]`).
  - [ ] Tela gráfica de Criação de Personagem (Raça, pontos extras `STR`/`DEX`/`INT`, Armadura, Arma, Lâmpada, Flares).
  - [ ] Tela gráfica de Comércio com o *Vendor* (Venda de joias, compra de equipamentos e poções de atributos).
  - [ ] Tela de entrada de texto para o enigma do baú de Zot (*WHICH CASTLE you are from*).
  - [ ] Tela de Ajuda In-Game (`[H]`) e inventário especial (`[J]`).
- [ ] Implementar `src/wiz_locale.c` com os bancos completos de textos em Inglês e Português.
- [ ] Empacotamento final da imagem de disco `wizcastle.dsk` e bateria de testes de ponta a ponta.
