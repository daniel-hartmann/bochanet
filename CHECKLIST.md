# Checklist: Trabalho 1 - Desenvolvimento do Minigame (Puzzle 3D)

## Programação II

Este checklist contém os requisitos técnicos para o desenvolvimento do minigame (puzzle 3D) que servirá como base para a integração em rede. Os itens abaixo refletem os conteúdos abordados em **Programação II** e devem ser implementados considerando a execução local no lado do cliente.

### 1. Estruturação do Nível e Atores

- [ ]  **Criação do Subnível:** O minigame deve ser construído em um nível (mapa) isolado, preparado para ser carregado via *Level Streaming* no mapa principal.
- [ ]  **Construção do Ambiente:** Utilização de formas geométricas primitivas (cubos, esferas, cilindros) ou assets externos para compor a área do puzzle.
- [ ]  **Sistemas de Coordenadas:** Posicionamento e transformação (Translação, Rotação, Escala) corretos dos atores que compõem o desafio lógico.

### 2. Controle e Câmera

- [ ]  **Configuração do Pawn/Character:** Definição do ator controlado pelo jogador durante a execução do minigame.
- [ ]  **Câmera:** Implementação de câmera adequada ao puzzle (em primeira ou terceira pessoa, fixa ou móvel), aplicando os conceitos de navegação vistos em aula.
- [ ]  **Eventos de Entrada (Inputs):** Mapeamento e implementação dos comandos de controle (teclado/mouse ou gamepad) necessários para resolver o puzzle, **utilizando o sistema de *Enhanced Input***.

### 3. Lógica de Gameplay (Blueprints e/ou C++)

- [ ]  **Mecânica Principal:** Implementação da lógica de resolução do puzzle utilizando *Visual Scripting* (Blueprints) ou código nativo (C++).
- [ ]  **Condição de Vitória:** Programação de um **estado claro** que valide a conclusão do puzzle (ex: variável booleana `bPuzzleResolvido` definida como *True*, ou disparo de um *Event Dispatcher* local).

### 4. Física e Colisões

- [ ]  **Volumes de Colisão:** Configuração de colisões (canais *Block* e/ou *Overlap*) adequadas para impedir que o jogador saia da área do puzzle ou atravesse objetos indevidos.
- [ ]  **Corpos Rígidos (Rigid Bodies):** Se a mecânica do puzzle exigir (ex: empurrar blocos, botões de pressão), ativação e configuração da simulação de física nos atores correspondentes.

### 5. Boas Práticas e Organização

- [ ]  **Organização do Outliner:** Agrupamento lógico dos elementos do cenário e nomenclatura padronizada e descritiva para todos os atores no *World Outliner*.
- [ ]  **Limpeza de Lógica:** Uso de cores padrão nos nós de Blueprint (caso utilize visual scripting) e formatação correta de código, mantendo a distinção arquitetural do que é processado na interface e no ambiente físico do jogo.
- [ ]  Entrega de acordo com o documento *Instruções para Entrega dos Trabalhos* (no Moodle)


## Redes

- [ ] O ambiente principal deverá utilizar o template Third Person padrão da engine.
- [ ] O mapa base (level do 3rd person) deve suportar a instanciação simultânea de múltiplos jogadores (Listen Server)
- [ ] Deve existir um objeto interativo compartilhado no mapa principal. Todos os jogadores devem poder interagir com ele sem que o objeto seja coletado ou destruído.
- [ ] A interação com este objeto deve disparar localmente (apenas para o cliente que interagiu) um minijogo 3D utilizando a técnica de Level Streaming. O minijogo roda no lado do cliente.
- [ ] O primeiro jogador a satisfazer a condição de vitória do puzzle deve enviar
um evento de validação para o servidor (Server RPC).
- [ ] O servidor, ao receber o sinal de vitória, deve processar o encerramento da partida e emitir um aviso global (Game Over) para todos os clientes, indicando o vencedor.
- [ ] Lembre-se de utilizar nós de validação (como Is Locally Controlled ou Is Local Player Controller) para garantir que o Level Streaming não puxe a câmera dos outros jogadores da rede.
- [ ] Testar utilizando o modo Play in Editor (PIE) configurado como Listen Server com pelo menos 2 jogadores
