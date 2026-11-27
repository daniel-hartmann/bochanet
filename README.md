# BochaNet

## Diffing Unreal assets (`.uasset` / `.umap`)

After cloning, run once:

```bash
./setup-ue5-diff.sh
```

This wires `git diff`/`git difftool` on `.uasset`/`.umap` files to open Unreal Editor's
own asset diff (via `ue5diff.sh`), using **local** repo config only — it doesn't touch
your global `~/.gitconfig`. Blueprints and similar assets get Unreal's native graph
diff window; Levels and most other asset types fall back to a text dump of the
asset's properties opened in FileMerge (Mac) — that's an Unreal limitation, not
something this script can improve on.
## C++ (VS Code)

The gameplay code lives in `BochaNet/Source`. Binaries and IDE project files are not
in git; each machine builds and generates its own.

**Requirements**
- Mac: Xcode (provides the compiler).
- Windows: Visual Studio 2022 or the *Build Tools for Visual Studio 2022*, with the
  "Game development with C++" workload (provides MSVC, even if you edit in VS Code).
- VS Code with the Microsoft **C/C++** extension.

**First time after cloning**: open `BochaNet/BochaNet.uproject` and answer *Yes* when
the editor asks to rebuild the missing modules.

**Generate the VS Code workspace** (again whenever C++ files are added or removed):
in the editor, *Tools → Refresh Visual Studio Code Project*, or from the repo root:

- Mac:
  ```bash
  "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -project="$PWD/BochaNet/BochaNet.uproject" -game -vscode
  ```
- Windows:
  ```bat
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="%CD%\BochaNet\BochaNet.uproject" -game -vscode
  ```

Then open `BochaNet/BochaNet.code-workspace` in VS Code (*File → Open Workspace from File…*).
In VS Code, build with the `BochaNetEditor … Development Build` task. On Mac, building the
game target (`BochaNet …`) regenerates an Xcode project and makes the editor open a missing
`BochaNet_Mac.code-workspace`; if that happens, generate the VS Code workspace again.
The editor is set to open C++ classes in VS Code (`Config/DefaultEditorPerProjectUserSettings.ini`).

**Build without the editor** (close the editor first; with it open, the build produces
hot-reload binaries the editor may not pick up):

- Mac:
  ```bash
  "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" BochaNetEditor Mac Development -project="$PWD/BochaNet/BochaNet.uproject" -waitmutex
  ```
- Windows:
  ```bat
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" BochaNetEditor Win64 Development -project="%CD%\BochaNet\BochaNet.uproject" -waitmutex
  ```

## Packaging the game

Close the editor, then from the repo root:

- Mac:
  ```bash
  ./build-game.sh
  ```
- Windows:
  ```bat
  build-game.bat
  ```

Pass `Development` as the first argument to keep the in-game console (`~`); the default is
`Shipping`. The result goes to `Packaged/Mac` or `Packaged/Windows` (zip that folder to share).
Each platform has to be packaged on its own OS. Set `UE_ROOT` if the engine is not in the
default location.

**Playing over the network** (Listen Server, UDP port 7777 must be reachable): one player hosts,
the others join by IP. Shipping builds ignore a map or address on the command line, so the game
reads its own `-host` and `-join=<IP>` (`UBochaGameInstance`).

- Windows: `BochaNet.exe -host` (host) and `BochaNet.exe -join=<host IP>` (join).
- Mac: `open BochaNet-Mac-Shipping.app --args -host` (host) and
  `open -n BochaNet-Mac-Shipping.app --args -join=<host IP>` (join; `-n` opens a second copy).

Add `-windowed -ResX=1280 -ResY=720` to play in a window.

---

# Trabalho Prático 2 — Multiplayer competitivo (Programação II e Redes)

BochaNet é uma partida de bocha (regras da CBTG) para **2 jogadores** em Listen Server. Os jogadores esperam no
lobby (mapa do template Third Person) até os dois estarem conectados e então entram na cancha pelo portal. Na
cancha, disputam as mesmas bolas: cada arremesso pode deslocar o bolim e as bochas do adversário. Vence quem
chegar a 12 pontos.

## Setup depois de puxar esta versão

As classes `ABochaGameMode`, `ABochaGameState`, `ABochaPlayerState` e `UBochaLobbyWidget` são novas. Depois de
compilar (*Build* do `BochaNetEditor` com o editor fechado), o `BP_ThirdPersonGameMode` precisa passar a ser
filho de `BochaGameMode`. Faça **uma** das duas opções:

- **No editor:** abra `Content/ThirdPerson/Blueprints/BP_ThirdPersonGameMode` → *Class Settings* →
  *Parent Class* = `BochaGameMode` → *Compile* → *Save*.
- **Pelo commandlet do projeto** (editor fechado; sem `-Save` é só uma simulação):
  ```bat
  "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%CD%\BochaNet\BochaNet.uproject" -run=BochaReparent -Spec="%CD%\Docs\reparent\rede-gamestate.json" -Save -unattended -nullrhi -nosplash -stdout
  ```

Sem esse passo, o lobby não usa as classes novas e o log mostra `Team 0 has no ABochaPlayerState` /
`No ABochaGameState`. O `BochaSoloGameMode` (a cancha aberta direto, para jogar sozinho) já herda de
`ABochaGameMode`. Para testar sozinho **pelo lobby**, mude *Required Players* para 1 nos *Class Defaults* do
`BP_ThirdPersonGameMode`; nesse caso, o jogador arremessa pelos dois times.

## Arquitetura de rede

O servidor (host do Listen Server) tem autoridade sobre tudo que afeta a partida. Os clientes só pedem ações
por Server RPC e exibem o estado replicado.

| Classe | Onde existe | Papel |
|---|---|---|
| `ABochaGameMode` (pai do `BP_ThirdPersonGameMode` e do `BochaSoloGameMode`) | só no servidor | Escolhe o GameState e o PlayerState e conta os jogadores conectados (`PostLogin` / `Logout`). Define quantos jogadores são necessários para começar (`RequiredPlayers` = 2). |
| `ABochaGameState` | servidor + todos os clientes | **Estado global:** ciclo de vida da partida, jogadores conectados/necessários, cronômetro, fase da jogada e de quem é a vez, vencedor. |
| `ABochaPlayerState` | servidor + todos os clientes | **Estado de cada jogador:** time, pontos e bochas restantes na mão. |
| `ABochaMatch` | servidor + todos os clientes | Regras da bocha (medição, ponto, anulações, rodadas) e a lista de jogadores por time. Criado pelo servidor quando a partida começa e destruído no fim; grava o estado no GameState e nos PlayerStates. |
| `ABochaBall` / `ABochaBocha` / `ABochaBolim` | servidor + todos os clientes | **Recursos compartilhados:** criados e simulados (física) só no servidor e replicados (`SetReplicatingMovement`). |
| `ABochaReferee` | servidor + todos os clientes | NPC árbitro: anda pela NavMesh no servidor; a animação de medir vai por multicast. |
| `ABochaPlayerController` | servidor + dono | RPCs `ServerRequestStart`, `ServerThrow`, `ServerClaimVictory`, `ServerLeaveMinigame`, `ClientThrowRejected`. |
| `ABochaMinigameCharacter`, `ABochaMinigameManager`, HUD, aviso do lobby, câmeras, pausa | **só local** | Mira, força, câmera e interface de quem está jogando (`bReplicates = false`, `IsLocalPlayerController`). |

### Ciclo de vida da partida (controlado pelo servidor, em `ABochaGameState`)

1. **`WaitingForPlayers`:** os jogadores estão no lobby. O `ABochaGameMode` conta os conectados a cada entrada
   e saída e replica a contagem; o aviso do lobby mostra `Aguardando jogadores: 1/2`. Com 2/2, ele passa a
   dizer para ir até o portal. O cliente pede `ServerRequestStart` ao interagir com o portal, e o **servidor
   recusa** enquanto não houver jogadores suficientes ou se já houver partida.
2. **`InProgress`:** o servidor cria o `ABochaMatch`, põe cada jogador num time do seu `ABochaPlayerState`, zera
   os pontos e inicia o cronômetro. A cancha é carregada em todas as máquinas (level streaming).
3. **`Finished`:** um time chegou a 12 pontos. O servidor confirma a vitória, para o cronômetro e anuncia o
   resultado a todos (`MulticastGameOver`). Depois de alguns segundos, todos voltam ao lobby e a fase volta a
   `WaitingForPlayers`.

O cronômetro aparece na HUD (`Tempo de partida m:ss`). Ele não é replicado a cada segundo: o servidor replica
só o instante de início e o de fim, e cada máquina calcula o tempo com `GetServerWorldTimeSeconds()`.

### Tabela de informações do jogo

| Informação | Onde fica | Quem altera | Quem precisa saber | Quando / como chega |
|---|---|---|---|---|
| Fase da partida (aguardando / em andamento / fim) | `ABochaGameState::MatchPhase` | Servidor | Todos | Ao mudar, `RepNotify` (`OnRep_MatchState`) |
| Jogadores conectados / necessários | `ABochaGameState::ConnectedPlayers/RequiredPlayers` | Servidor (`PostLogin`/`Logout`) | Todos (aviso do lobby) | `RepNotify` |
| Cronômetro | `ABochaGameState::MatchStartTime/EndTime` | Servidor (início e fim) | Todos | Replicado uma vez; cada máquina calcula o tempo |
| Fase da jogada e de quem é a vez | `ABochaGameState::TurnPhase/CurrentTeam` | Servidor (`ABochaMatch`) | Todos | `RepNotify` (`OnRep_Turn`) |
| Vencedor | `ABochaGameState::WinnerTeam` | Servidor | Todos | Replicado + `MulticastGameOver` |
| Time, pontos, bochas restantes | `ABochaPlayerState` | Servidor | Todos (HUD) | `RepNotify` (`OnRep_BochaState`) + `ForceNetUpdate` |
| Jogadores de cada time, quem tem o ponto | `ABochaMatch` | Servidor | Todos | `RepNotify` (`OnRep_State`) |
| Posição das bolas | `ABochaBall` (física) | Servidor | Todos | Movimento replicado |
| Cor da bocha | `ABochaBocha::bBlue` | Servidor (ao criar) | Todos | `RepNotify` |
| Avisos das regras ("Ponto: Azul", "Bocha anulada") | — | Servidor | Todos | `MulticastAnnounce` (reliable) |
| Mira e força do arremesso | `ABochaMinigameCharacter` | Cliente | Servidor (no arremesso) | `ServerThrow` (Server RPC) |
| Câmera, HUD, pausa | Atores e widgets locais | Cliente | Só ele | Nunca replicado |

### Escopo planejado

- **2 jogadores** (vermelho = host, azul = cliente). A partida só começa com os dois conectados. Um terceiro
  jogador conectado fica fora dos times (não arremessa).
- Ninguém entra no meio da partida, e não há reconexão: se alguém cai, o jogo é reiniciado.
- Ambiente de playtest fechado, sem anticheat. Os riscos estão descritos abaixo.

## Reflexão sobre consistência e segurança

### 1. Condições de corrida: o que acontece se dois jogadores usam o mesmo recurso ao mesmo tempo?

Os RPCs chegam ao servidor e são executados **um de cada vez**, na thread de jogo. O que importa é a ordem de
chegada e se o segundo pedido ainda é válido quando é processado:

- **Dois arremessos ao mesmo tempo:** `ServerThrow` só cria a bola se `ABochaMatch::CanThrow` confirmar que é
  a vez daquele jogador. O primeiro arremesso aceito já muda a fase para `BallsMoving` (`RegisterThrow`), então
  o segundo é recusado e o cliente recebe `ClientThrowRejected`, que devolve a bola à mão. Como o jogo é por
  turnos, a disputa pelos recursos é ordenada pelas regras.
- **Os dois interagem com o portal juntos:** só o primeiro `ServerRequestStart` inicia a partida. O segundo é
  recusado porque a fase já não é `WaitingForPlayers` e porque `bMinigameStarted` já está marcado no servidor.
- **Duas bolas se tocando / quem está mais perto do bolim:** a física e a medição rodam só no servidor, então há
  uma única resposta. Distâncias iguais dentro de 1 cm (`TieTolerance`) contam como empate, como diz a regra.
- **Pedido de vitória repetido:** `ConfirmVictory` ignora o pedido se a partida já está em `GameOver`. Se o
  pedido do vencedor nunca chegar, o servidor encerra mesmo assim depois de `VictoryClaimTimeout`.

**Risco que sobra:** o cliente destrói a bola que está na mão assim que arremessa, antes da resposta do servidor
(`ABochaMinigameCharacter`). Com latência alta, um arremesso recusado faz a bola "sumir e voltar". **Proposta:**
manter a bola visível até a bola do servidor aparecer, ou até a recusa.

### 2. Consistência e latência: um cliente pode ver um estado diferente do servidor?

Sim, por alguns instantes:

- **Bolas em movimento:** só o servidor simula a física. O cliente recebe as posições replicadas com atraso
  igual à latência e pode ver pequenos saltos ou a bola parando um pouco depois. As decisões (quem tem o ponto,
  bola anulada) são sempre do servidor, e o árbitro mede antes do anúncio, então o resultado não diverge; só a
  imagem se atrasa. **Proposta:** suavizar o movimento das bolas no cliente (interpolação).
- **Ordem de chegada:** a vez (no `ABochaGameState`), os pontos (nos `ABochaPlayerState`) e o `ABochaMatch` são
  atores diferentes, e a Unreal não garante a ordem entre eles. Por um instante, a HUD pode mostrar a vez nova
  com o placar antigo. **Tratamento aplicado:** `OnRep_Turn`, `OnRep_BochaState` e `OnRep_State` redesenham a
  HUD, então ela se corrige assim que a última parte chega. Pelo mesmo motivo, um aviso por multicast
  ("Vermelho marcou 2 pontos") pode aparecer um pouco antes do placar mudar.
- **Frequência do PlayerState:** por padrão, o PlayerState replica com pouca frequência; sem `ForceNetUpdate`,
  o placar levaria até ~1 s para mudar no cliente. **Tratamento aplicado:** todo setter de `ABochaPlayerState`
  e do `ABochaGameState` chama `ForceNetUpdate`.
- **Cronômetro:** cada máquina calcula o tempo com o relógio do servidor (`GetServerWorldTimeSeconds`), então
  todos veem o mesmo tempo (com diferença de uma fração da latência), sem tráfego a cada segundo.
- **Jogador entrando tarde (fora do escopo):** a cancha é carregada por um multicast
  (`MulticastStartMinigame`), que não chega a quem conecta depois. **Proposta:** trocar por uma variável
  replicada com `RepNotify` que carregue o nível.
- **Queda de um jogador (fora do escopo):** no Listen Server, o host é o servidor; se ele cai, a partida acaba
  para todos. Se o cliente cai, a contagem do lobby se atualiza (`Logout`), mas a partida em andamento fica
  esperando a vez dele. **Decisão:** reiniciar o jogo. **Proposta:** encerrar a partida no `Logout` com vitória
  por W.O. de quem ficou.

### 3. Autoridade e trapaça: o que é confiado ao cliente?

O servidor decide quando a partida começa, a vez, a validade das jogadas, a física, a medição, os pontos e o
vencedor. O que ainda vem do cliente:

| Dado do cliente | Validação atual no servidor | Como trapacear | Proposta |
|---|---|---|---|
| `ServerThrow(BallLocation, Velocity)` | Confere a vez e se a bola sai atrás da linha B | Um cliente modificado pode mandar qualquer velocidade e direção (força máxima, mira perfeita) ou soltar a bola em qualquer ponto atrás da linha, inclusive fora da cancha. Os limites de força e de ângulo existem só no cliente. | Mandar só força (0..1) e ângulo; o servidor aplica os limites e calcula o ponto de saída a partir da posição do jogador na cancha. |
| `ServerClaimVictory()` | Só aceita se o placar do servidor confirmar | Nenhum ganho: o servidor já sabe quem venceu. | Remover o RPC e deixar o servidor encerrar direto. |
| `ServerLeaveMinigame()` | Nenhuma | Qualquer jogador encerra a partida dos dois (por exemplo, quando está perdendo). | Exigir os dois jogadores, ou dar vitória por W.O. a quem ficou. |
| `ServerRequestStart(Portal)` | Recusa sem jogadores suficientes ou com partida em andamento | Iniciar a partida a distância, sem estar no portal. | Conferir no servidor a distância do pawn até o portal. |

A regra do enunciado ("se o servidor consegue descobrir o valor sozinho, ignore o do cliente") está aplicada ao
início da partida, à vez, à linha de arremesso, aos pontos e à vitória. Ela foi identificada, mas não aplicada,
à velocidade e ao ponto de saída da bola.

## Testes

- PIE: *Net Mode* = *Play As Listen Server*, *Number of Players* = 2. Os logs mostram `[server]` ou `[client]`
  (`LogBochaMatch`, `LogBochaGameState`).
- Latência no console: `Net PktLag=150`, `Net PktLoss=5`.
- Build: `build-game.bat` / `build-game.sh` (seção *Packaging the game* acima).
