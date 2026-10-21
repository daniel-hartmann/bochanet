# Plano: regras de bocha (ponto–rafa–tiro) no minijogo

Referência: `Docs/regras_de_bocha.pdf` (CBTG, Anexo 02, versão 2018).
Objetivo: uma partida **individual 1×1** jogável em rede (Listen Server, 2 jogadores) que siga as regras
centrais: vez de jogar, validade do bolim, bochas anuladas, contagem de pontos por mão e vitória.
As regras de arbitragem e de equipes ficam de fora (lista no fim).

## O que o jogo faz hoje (e por que não dá para pontuar)

- Cada máquina roda o seu `ABochaMinigameManager` e o jogador arremessa quando quiser: **não existe vez**.
- Cada manager começa entregando um bolim, então **cada jogador joga o seu bolim** (2 bolins na cancha).
- A bola arremessada é criada no servidor (`ABochaPlayerController::ServerThrow`, replicada), mas ninguém
  acompanha essas bolas: não há "bola parou", nem pontos, nem fim de mão (TODO em `NotifyBolimThrown`).
- O cliente escolhe se arremessa bolim ou bocha (`bIsBolim`) e onde a bola nasce; o servidor só obedece.
- O impulso usa só o X do forward e o personagem só anda para os lados: dá para jogar a ponto, mas não
  dá para mirar em ângulo.

Conclusão: a partida precisa de um **estado único e autoritativo no servidor**, replicado para os dois.

## A cancha da `CanchaMap` (medida pelo export)

Piso de X = −2600 (cabeceira A) a X = +50 (cabeceira A'), Y de −400 a 0, topo do piso em Z = 50.
Arremesso no sentido +X. Linhas do Art. 2º a (cancha de 26,50 m), em X:

| Linha | Distância de A | X (jogando de A) | Uso no jogo |
|---|---|---|---|
| A | 0 | −2600 | cabeceira (fundo) |
| B | 4 m | −2200 | lançar bolim/ponto/rafa daqui para trás |
| C | 7 m | −1900 | limite para tiro (bochada) |
| D | 9 m | −1700 | — |
| E | 13,25 m | −1275 | bolim tem que passar inteiro (Art. 9 g) |
| B' | 22,50 m | −350 | bolim não pode chegar (Art. 9 h II) |
| P' | ≈ 19,9 m | ≈ −612 (Y = −200) | não usado (bolim irregular é relançado pelo mesmo jogador) |
| A' | 26,50 m | +50 | cabeceira oposta (fundo) |

(P' = cruzamento das diagonais da metade oposta, Figura 1.)
Escala real confere: bocha com 10,7 cm (Art. 6 c) e bolim com 4 cm (Art. 6 b).

## Regras que entram

| Regra | Artigo | Como fica no jogo |
|---|---|---|
| Individual: 4 bochas por jogador | 4.1 a | 4 bochas por jogador por mão |
| Sorteio de quem começa | 9 c | servidor sorteia na primeira mão |
| Bolim válido só se passar inteiro a linha E e não chegar a B', nem ficar a ≤ 13 cm das laterais | 9 g/h | checado quando o bolim para |
| Bolim irregular: passa ao adversário (1 tentativa); 2ª irregularidade → bolim no ponto P' | 9 f | idem |
| Na mão seguinte, o bolim é lançado por quem pontuou na anterior | 9 f | idem |
| Quem não tem o ponto joga (até tomar o ponto ou acabar as bochas) | 3 c, 13 c (implícito) | ordem de jogo dentro da mão |
| Empate de distância: quem empatou joga de novo; empate no fim → mão anulada e repetida | 13 c | tolerância de 1 cm |
| Sem bochas de um lado, o outro joga as que restam | 13 b | idem |
| Bocha que sai da cancha ou bate direto na cabeceira do fundo sem fazer jogo é anulada | 10 g, 11 b, 13 e | removida da cancha |
| Bolim que sai da cancha ou volta para antes da linha E → mão anulada | 9 i | mão repetida da mesma cabeceira |
| Ponto: 1 ponto por bocha mais perto do bolim que a melhor do adversário | 5 b | contagem no fim da mão |
| Vitória: primeiro a 12 pontos (regulamento: 12 a 15) | 5 a | configurável no BP |
| Jogar só de trás da linha B | 2 a II | posição do personagem travada atrás de B |
| 1 minuto por bocha | 4.2 d | opcional (Fase R6) |

## Arquitetura

**`ABochaMatch`** (novo, `Source/BochaNet/Game/`): ator replicado (`bAlwaysRelevant`) que o **servidor**
spawna quando o minijogo começa (no `MulticastStartMinigame`, só com autoridade). É a fonte da verdade:

- `Phase`: `WaitingForPlayers`, `ThrowingBolim`, `ThrowingBochas`, `BallsMoving`, `RoundOver`, `GameOver`.
- `Players[2]` (os `APlayerState` dos dois), `CurrentThrower` (0/1), `Score[2]`, `BochasLeft[2]`,
  `BolimThrower`, `BolimAttempts`, `Bolim`, `Bochas` (lista com dono de cada uma), `TargetScore = 12`.
- Tudo replicado com `OnRep` para a interface reagir.

**`ABochaCourt`** (novo): só as medidas da tabela acima (linhas, laterais, cabeceiras, P') como
propriedades editáveis, com desenho das linhas no editor (`DrawDebug` / componentes de linha).
Fica colocado no `CanchaMap`; o `ABochaMatch` o encontra quando o nível aparece. Dá para começar com
os valores fixos da tabela e só depois colocar o ator no mapa.

**`ABochaPlayerController::ServerThrow`** passa a receber só o impulso (e a posição lateral).
O servidor decide **se** pode jogar (é a vez dele? a fase permite?) e **o quê** (bolim ou bocha,
cor pela equipe), e registra a bola no `ABochaMatch`. Some o `bIsBolim` vindo do cliente.

**`ABochaMinigameManager`** vira só apresentação local: mostra a bola na mão **quando o `ABochaMatch`
diz que é a sua vez**, do tipo certo, e atualiza o HUD. Deixa de decidir a sequência de jogo.

**Fluxo de uma mão (no servidor):**

```
Sorteio/vencedor anterior lança o bolim
  → BallsMoving: espera todas as bolas pararem (velocidade < limiar por 0,5 s, ou 10 s de timeout)
  → bolim válido? não: passa ao adversário (1x), depois coloca em P'
  → ThrowingBochas: joga quem não tem o ponto (ou quem empatou / quem ainda tem bochas)
  → após cada bocha: BallsMoving → anula bochas fora da cancha → bolim ainda válido? senão anula a mão
  → acabaram as 8 bochas: conta os pontos → RoundOver (alguns segundos mostrando o resultado)
  → alguém chegou a 12? GameOver : limpa a cancha e começa a próxima mão
```

**Vitória e rede (checklist de Redes):** quando o servidor vê `Score >= TargetScore`, faz um Multicast de
Game Over com o vencedor para todos. Para cumprir o item "o primeiro jogador a satisfazer a condição de
vitória envia um evento de validação ao servidor (Server RPC)", o cliente vencedor chama um
`ServerClaimVictory`, e o servidor só aceita se o placar dele confirmar. Assim o RPC existe e o servidor
continua sem confiar no cliente.

## Fases

Andamento: R1 a R5 testadas; R6 implementada (falta teste no PIE). Detalhes da R1 como ficou:
- `ABochaMatch` (`Source/BochaNet/Game/BochaMatch.*`): spawnado pelo servidor no `MulticastStartMinigame` do Portal;
  host = equipe 0 (vermelho), outro jogador = equipe 1 (azul); sozinho no PIE, o mesmo jogador é as duas equipes.
- Turno provisório até a R4: depois do bolim joga quem lançou o bolim; depois, alterna enquanto houver bochas;
  na mão seguinte o bolim alterna. Fim da mão: pausa de 3 s e a cancha é limpa.
- `ServerThrow(BallLocation, Impulse)`: sem `bIsBolim`; recusa fora da vez e avisa o cliente (`ClientThrowRejected`).
- Manager: sem bolim próprio e sem `Notify*Thrown`; a bola na mão aparece/some com o estado do `ABochaMatch`.
- HUD: `P1_Name`/`P2_Name` (marcados como variáveis no `WBP_HUD`) mostram "Vermelho"/"Azul", "(você)" e "> " na vez;
  ícone do bolim (`P1_Bolim`/`P2_Bolim`) em quem vai lançar o bolim. `SetP1Bolim` saiu.

Ajustes depois dos testes (fora do regulamento, a pedido):
- **Bolim irregular → o mesmo jogador lança de novo**, sem limite (no lugar do Art. 9 f: adversário e depois ponto P).
  O bolim irregular fica `IrregularBolimDelay` = 2 s na cancha antes de sair, com o motivo no aviso.
- **Bolim sempre válido:** no lançamento do bolim, a força escolhe onde ele para, de `BolimLineMargin` (50 cm) depois
  de E até 50 cm antes de B'. Ele sai reto a `BolimLaunchSpeed` (900 cm/s) e, ao tocar o chão, é freado pelo servidor
  para parar exatamente ali (`ABochaBall::SetStopTarget`). Bochas continuam com física livre.
- **Física das bolas (medida com `Bocha.CalibrateThrows`, ver abaixo):** as bolas são **roladas rente ao chão**, já
  girando na velocidade de rolamento (`ABochaBall::Launch`); antes, soltas da altura da mão com arco, perdiam ~60% da
  velocidade no impacto. Sem `AngularDamping` (fazia a bola escorregar) e sem o teto de 3600°/s de rotação do projeto
  (o bolim de 4 cm só conseguia rolar a ~1,25 m/s). Freada `RollingDeceleration` = 210 cm/s²: distância = v²/420
  (força 500 → 5,9 m, 750 → ~13 m, 900 → 19 m, 1000 → 23,7 m). A força começa em 750. Bolim sai a 1000 cm/s.
- **Calibração:** `UnrealEditor BochaNet.uproject /Game/Bocha/CanchaMap -game -nullrhi -benchmark -fps=60
  -ExecCmds="Bocha.CalibrateThrows"` grava `Saved/BochaCalibration.txt` (distância por velocidade, bolim e bocha).
- **Avisos no HUD:** texto grande no alto da tela (`ShowMessage`, `MessageDuration` = 4 s, `MessageFontSize` = 34),
  criado em tempo de execução no canvas do `WBP_HUD`; também vão para o log (`LogBochaMatch`).

Detalhes da R2 como ficou:
- Medidas em `FBochaCourt` (propriedade `Court` do `ABochaMatch`), em coordenadas do `CanchaMap`; o Portal informa
  onde o mapa foi carregado (`SetCourtOrigin`). Não há tábua na cabeceira A (atrás do jogador): só laterais e fundo.
- Linhas desenhadas com debug lines (`bDrawCourt`): A/A' brancas, B/B' verdes, C/C' azuis, D/D' amarelas,
  E vermelha.
- Bolim (Art. 9 h): passa inteiro a E, não chega a B', fica a mais de 13 cm das laterais. Irregular: adversário
  lança; 2º irregular: bolim no P' e a primeira bocha continua com quem tinha o direito.
- Bochas anuladas: fora da cancha (qualquer uma) ou a arremessada que bateu no fundo sem tocar outra bola.
- Mão anulada (Art. 9 i): bolim fora da cancha ou de volta antes de E → mão repetida, mesmo lançador de bolim.
- `ServerThrow` recusa bola solta além da linha B.
- Avisos com `AddOnScreenDebugMessage` (Multicast); HUD de verdade na R6.


Cada fase termina com teste no PIE (Listen Server, 2 jogadores) e commit.

**R1 — Partida autoritativa e vez de jogar.** ✔ implementada (falta teste no PIE). `ABochaMatch` replicado; equipes (host vermelho, cliente
azul); um bolim só; vez alternando (ainda sem distâncias); `ServerThrow` valida a vez e escolhe a bola;
manager mostra a bola na mão só na sua vez; "bola parou" no servidor (resolve o TODO). HUD mínimo:
"Sua vez" / "Vez do adversário".
*Teste:* só um jogador consegue arremessar por vez; aparece um único bolim; as cores batem.

**R2 — Cancha e validade.** ✔ implementada (falta teste no PIE). `ABochaCourt` com as linhas; bolim irregular (Art. 9 h) passa ao adversário,
depois vai para P'; bochas fora da cancha ou que batem direto na cabeceira do fundo são anuladas;
bolim fora da cancha ou de volta antes da linha E anula a mão (Art. 9 i). Personagem preso atrás de B.
*Teste:* bolim fraco (antes de E) e forte (além de B') trocam a vez; bocha jogada para fora some.

**R3 — Mira.** ✔ implementada (falta teste no PIE): **Q/E** no eixo Y do `IA_BochaMove` (velocidade `AimSpeed`)
e **mouse X** na action nova `IA_BochaAimMouse` (`MouseAimSensitivity` graus por unidade), ±30° (`MaxAimAngle`);
A/D andam sempre na lateral da cancha. (As setas foram testadas e descartadas.) Junto:
- Resistência de rolamento no servidor (`RollingDeceleration` = 80 cm/s², para de vez abaixo de `StopSpeed` = 5 cm/s),
  ajustável no BP de cada bola.
- A força virou velocidade de saída (100–1000 cm/s, `AddImpulse` com `bVelChange`): bolim e bocha saem com a mesma
  velocidade; antes o bolim (8x mais leve) saía 8x mais rápido.
*Teste:* arremessar em diagonal e contornar uma bocha na frente do bolim.

**R4 — Ordem por distância e pontos.** ✔ implementada (falta teste no PIE). Distância medida no chão (centro a centro);
empate até `TieTolerance` = 1 cm. Sem bocha válida na cancha, quem jogou joga de novo (9 d). Mão sem bochas válidas:
0 pontos e o bolim fica com o mesmo lado. `Score` e `PointHolder` já replicados (HUD na R6); avisos com o placar. Quem não tem o ponto joga; empate (13 c); sem bochas (13 b);
contagem da mão (5 b, até 4 pontos); próxima mão com bolim de quem pontuou (9 f) e cancha limpa.
*Teste:* mãos montadas à mão (bochas perto/longe) dão a pontuação esperada.

**R5 — Vitória e Game Over.** ✔ implementada (falta teste no PIE). `TargetScore` = 12 no `BP_BochaMatch` (BP novo, filho de
`ABochaMatch`; o Portal usa ele via `MatchClass`). Quem chega a `TargetScore`: o servidor marca `Winner`; a máquina do
vencedor chama `ServerClaimVictory` (Server RPC); o servidor confere o placar e faz `MulticastGameOver` (fallback:
`VictoryClaimTimeout` = 3 s). Mensagem fixa "Fim de jogo! X venceu por A x B"; após `GameOverDuration` = 8 s,
`ABochaPortal::MulticastEndMinigame` descarrega a cancha, remove personagem/HUD/minimapa/IMC, devolve controle e câmera
ao personagem do lobby e libera o portal para nova partida. Placar até 12 (configurável); `ServerClaimVictory` + Multicast de Game Over
com o vencedor; tela final e volta ao lobby (ou nova partida).
*Teste:* partida até o fim nos dois lados; o perdedor vê o anúncio do vencedor.

**R6 — HUD.** ✔ implementada (falta teste no PIE): linhas `P1_Name`/`P2_Name` com "> " na vez, "(você)", placar e
"(ponto)"; ícones `P*_B1..B4` apagados (opacidade 0,2) conforme as bochas são jogadas, `P*_B5` escondido; ícone do bolim
em quem vai lançá-lo; avisos grandes no alto. Relógio de 1 min (4.2 d) não feito — fica para a R7 se for pedido. Placar; bochas restantes de cada um (`P1_B1..B4`/`P2_B1..B4` já existem no `WBP_HUD`,
o quinto ícone some); ícone do bolim (`SetP1Bolim`) em quem lança o bolim; quem tem o ponto;
resultado da mão; opcional: relógio de 1 min por bocha (4.2 d).

**Minimapa (feito junto com o início da R7).** ✔ implementado (falta teste no PIE): `UBochaMinimapWidget`, desenhado no HUD
(canto superior direito), fundo escuro de alto contraste, linhas E/D'/C'/B'/A', bolas como bolinhas ampliadas
(`BallScale` = 2, mínimo `MinBallSize` = 8 px); mostra `WindowLength` = 10 m da cancha centrados no bolim (antes do bolim,
a zona E..B'). Substitui a câmera (`BP_MinimapCapture` deixou de capturar; ele, o `WBP_Minimap` e o `RT_Minimap` podem
ser apagados no editor).

**R7 — Câmera que segue a bola.** ✔ implementada (falta teste no PIE): `ABochaBallCamera` (local, em cada máquina) persegue
a bola arremessada (`ABochaMatch::ThrownBall`, agora replicada) enquanto a fase é `BallsMoving`, por trás e acima
(`Offset`, `LookAhead`, `FollowSpeed`); depois volta ao personagem do minijogo (`CameraBlendTime` = 0,5 s no manager).
Escolhida das opcionais abaixo; as outras ficaram de fora.

**Opcionais não escolhidos (R7).**
- Câmera que acompanha a bola arremessada e volta ao jogador.
- Alternar a cabeceira a cada mão (jogar de A e de A').
- Arrasto de mais de 70 cm na jogada a ponto (Art. 14.1–14.2): guardar as posições antes de cada jogada,
  medir o deslocamento e, se passar de 70 cm, anular a bocha e recolocar as outras.
- Rafa e tiro (Art. 15–16): declarar o tipo e o alvo antes de jogar, com as linhas D/C e o "berçário".
- Regra da vantagem (Art. 8): o adversário escolhe aceitar ou anular uma jogada irregular.

## Fora do escopo

Duplas e trios, técnicos e tempos técnicos, uniformes, substituições, passeio/reconhecimento (Art. 12),
WO, suspensão de partida (Art. 17), marcação física das bochas e medição pelo árbitro, disciplina (4.3).

## Decisões

1. **Pontos para vencer: 12**, em `TargetScore` (`EditDefaultsOnly`) no BP do `ABochaMatch`, para mudar sem recompilar.
2. **Quem começa: sorteio** (Art. 9 c), feito pelo servidor na primeira mão.
3. **Sempre da mesma cabeceira** (A → A'): no fim da mão as bolas somem e a próxima começa do mesmo lugar.
4. **Mira em ângulo antes da contagem de pontos**: fase R3 abaixo.
