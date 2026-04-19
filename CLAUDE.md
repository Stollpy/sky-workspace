# CLAUDE.md — Sky Hub monorepo

> Fichier toujours chargé en contexte par Claude Code depuis la racine `stollper-pcb/`.
> Source de vérité : les ADR figés dans `docs/logiciel/adr/` (11 avril 2026) + les `.cursorrules` nichés par jalon/milestone/feature.
> Deadline MVP : **~décembre 2026**. Domaine : `sky.stollpy.com`.

## Produit — Sky Hub

Hub domotique **local-first** tournant sur un SBC ARM à la maison, pensé comme assistant domestique autonome.
Toute l'intelligence (LLM, STT, TTS, orchestration Matter) tourne **localement** : pas de dépendance cloud critique.

Quatre briques produit :

| Brique | Rôle | Réseau | Techno |
|--------|------|--------|--------|
| **Sky** | Hub central (OPi5+) : k3s + dashboard + LLM + pipeline vocale + partage réseau (Ethernet uplink ↔ hotspot Wi-Fi `sky-hub`) | Wi-Fi AP + Ethernet + BLE + mDNS `sky-hub.local` + VPN (Tailscale/WireGuard) | Laravel + Rust + k3s + Ansible |
| **Orbit** | Dashboard web (écran tactile 10" kiosk + remote) pour interagir avec le hub | Laravel Reverb (WebSocket) | Laravel 12 + Inertia v2 + Vue 3 + Pg + Redis |
| **Luxia** | Premier objet connecté : moteur pas-à-pas qui tire la chaînette d'un store via l'engrenage latéral | Matter (Wi-Fi puis Thread en Phase B) | ESP32-WROOM-32 (dev) → ESP32-C6 (prod) + `sky-framework` + `esp_matter` |
| **Sky Watch** | Montre connectée : capture voix + biométrie (HR, accel, gyro, GPS) + notifications + TTS feedback | BLE (pairing/proximité) + Wi-Fi (data, audio streaming) | ESP32-Pico-V3-02 + ESP-IDF + UI écran |

Flux type : l'utilisateur parle dans la Watch → Wi-Fi WS vers Sky Hub → **STT (Whisper.cpp)** → **LLM (Ollama + function calling + `reasoning`)** → matrice de risque RBAC → commande → ESP32 co-processeur → Matter → Luxia bouge → **TTS (Piper)** renvoyé sur la Watch. Latence cible bout-en-bout : **3-5 s** (ADR-006).

## Matériel — cibles et compromis en cours

| Composant | Cible prod | En dev / prototype | Notes |
|-----------|-----------|--------------------|-------|
| SBC hub | **Orange Pi 5+** (OPi5+) | Raspberry Pi 4B (fallback), KiwiPi Pro (alt), future board maison `sky-hub` — **config live dans `.claude/context/sky-hub-state.md`** | LLM / STT / TTS locaux sur NPU/CPU + NVMe. Mini variante possible (`sky-infra-min`, sans k3s, Node + Postgres systemd). |
| Co-processeur Matter | **ESP32-C6-WROOM-1-N8** (Thread + Wi-Fi) | **Phase A : ESP32-WROOM-32** (Wi-Fi seul) → le code haut niveau ne change pas, seule la target ESP-IDF (`esp32` → `esp32c6`) et l'activation OTBR changent | Repo `sky-hub-antenna` : Phase B = cible prod (C6). |
| Luxia | ESP32-WROOM-32 (classique) | **Variante S3** en test dans `sky-luxia-s3/` | Matter Window Covering cluster `0x0102`. |
| Watch | ESP32-Pico-V3-02 | Scaffold minimal `sky-watch/` (repo en cours) | BLE NimBLE + Wi-Fi + écran + micro. |
| Autres firmwares | — | `sky-hub-antenna-wroom/` (variante bridge WROOM Wi-Fi), `firmware/` (legacy PCB local) | Voir ci-dessous. |

> Règle : quand un compromis matériel temporaire existe (C6 pas encore arrivé → WROOM-32, OPi5+ non provisionné → RPi4B), le code applicatif **n'est pas fourché** — on abstrait via `sky-framework` / `platformio.ini` multi-env / `sdkconfig.esp32*` / docs du jalon 2.

## Layout du monorepo

Racine = `stollper-pcb/` (sera renommée `sky-workspace` sur GitHub, remote `Stollpy/sky-workspace`) — documentation, scripts, firmware PCB legacy. **Chaque dépôt Sky est un git séparé** (clone côte à côte, pas de submodule racine) dans un sous-dossier.

| Chemin | Rôle | Git status | Stack principale |
|--------|------|------------|------------------|
| `docs/` | Documentation **canonique** : ADR, jalons (M*/F*/T*.md), guides dev | local | Markdown uniquement |
| `firmware/` | Firmware PCB **legacy** (Luxia historique) — contient la structure `.cursor/{rules,skills,plans,learning}/` la plus riche | local (pas submodule) | ESP-IDF C + PlatformIO |
| `pcb/` | Tooling schéma PCB en TypeScript (tscircuit) | local | TS + Vite |
| `pcb-kicad/` | Projet KiCad Luxia (schémas, PCB, BOM) | local | KiCad 6/7 |
| `sky-luxia/` | Firmware **produit** Luxia (WROOM-32, Matter Wi-Fi) | repo | ESP-IDF + `esp_matter` + `sky-framework` v0.2.0 |
| `sky-luxia-s3/` | Variante expérimentale ESP32-S3 | repo | ESP-IDF |
| `sky-hub-antenna/` | Firmware **co-processeur Matter** (Phase A ESP32, Phase B C6) | repo | ESP-IDF + `esp_matter` + serial bus |
| `sky-framework/` | **SDK firmware partagé** (`sky_core`, `sky_stepper`, `sky_wifi`, `sky_matter`, `sky_ota`, `sky_power`, `sky_serial`…) distribué via ESP-IDF Component Manager, SemVer | repo | ESP-IDF components C |
| `sky-hub-services/` | Services hub (bridge série↔WebSocket, STT, LLM glue, TTS…). Contient `device-service/` en Rust. | repo | **Rust** (tokio, serialport, tokio-tungstenite) + possible TS |
| `sky-infra/` | IaC **production** : Ansible + Helm + Docker + scripts k3s | repo | YAML, Bash, Jinja2, Helm 3 |
| `sky-infra-min/` | IaC **minimale** sans k3s (systemd + Postgres + Node) | repo | YAML, Bash |
| `sky-orbit/` | Dashboard (Laravel 12 + Inertia v2 + Vue 3 + Reverb). Packages internes symlinkés sous `packages/sky/{core,device,feature,llm-config,member,menu,settings,widget}`. | repo | PHP 8.2, TypeScript, Vite, Pint, ESLint flat, Spatie permission, Sanctum, Ziggy, Breeze |
| `sky-proto/` | **Registre unique des clusters Matter** + codegen (C, PHP, TS). CI GitHub Actions. Source de vérité sémantique. | repo | Node + JSON Schema + Bash |
| `sky-watch/` | Firmware montre (scaffold en cours) | repo | ESP-IDF C (à venir) |
| `sky-hub-antenna-wroom/` | Firmware bridge WROOM (variante Wi-Fi du C6) | repo | ESP-IDF C |
| `scripts/` | `sky-worktree.sh` — orchestration des worktrees par jalon | local | Bash |
| `worktrees/` | **Ignoré** — emplacement des worktrees parallèles (un par jalon × repo) | gitignored | — |
| `init-sky-repos.sh` | Initialisation idempotente (.gitignore, README, stubs `.cursorrules`) des 9 repos Sky | local | Bash |
| `laravelbin.html` | Dump binaire Laravel, ignorable | local | — |
| `ssh-fingerprint.md` | Empreinte SSH de travail | local | — |

### Stack technique (ADR-004, ADR-009)

- **Firmwares** → C/C++ sur ESP-IDF 5.x (+ `esp_matter` pour Matter, NimBLE pour BLE). Build via **PlatformIO** ou `idf.py`. Dépendances SDK : **ESP-IDF Component Manager** (pas de submodules git pour tirer `sky-framework`).
- **Services hub perf** → **Rust** (tokio async, serialport, tokio-tungstenite client Pusher v7 pour Reverb, hmac/sha2 pour channels privés).
- **Services hub légers / tooling** → **TypeScript** (Node).
- **Orbit** → **PHP 8.2 + Laravel 12 + Inertia v2 + Vue 3 (Composition API + TypeScript strict)**, PostgreSQL 16, Redis 7, **Laravel Reverb** (WebSocket protocole Pusher v7).
- **LLM** → **Ollama** (pod k3s, limits RAM strictes 4-16 Go).
- **STT** → **Whisper.cpp / faster-whisper** (pod, budget 500-1500 ms).
- **TTS** → **Piper** (pod, ~50-100 Mo RAM, mode adaptatif normal/silencieux).
- **Conteneurs** → **k3s** single-node sur OPi5+ (requests/limits obligatoires dans tout manifest, ADR-007).
- **Bare-metal systemd** → **BlueZ** (BLE), **OTBR** (OpenThread Border Router), **hostapd** (hotspot `sky-hub`), **Avahi** (mDNS), **dnsmasq**.
- **Provisioning** → **Ansible** (roles idempotents), **Helm 3**, **Docker / docker-compose**.
- **Python** n'est **pas** un langage par défaut — chaque usage doit être argumenté (ex : wrapper Whisper).
- **Go est exclu** de la stack.

### Côté Orbit — packages internes

`sky-orbit/packages/sky/*` (symlinkés via `"repositories": [{"type": "path", "url": "packages/sky/*"}]` dans `composer.json`) :

- `sky/core` — contrats partagés (`RoleChecker`, `ResponseFactory` driver pattern avec `InertiaResponseFactory` + `JsonResponseFactory`, `SecurityLevelCalculator` basé sur weights de permissions)
- `sky/feature` — `Feature` abstract + `FeatureManager` + `HasConfigurableMiddleware` (inspiré Invictus Feature)
- `sky/menu` — Builder + Composite pattern (cohérent `stollper/menu`)
- `sky/widget` — Builder + Composite + modèles DB
- `sky/device` — devices, pairing, contrôle réel (J2)
- `sky/member` — utilisateurs, rôles, profils (J3 montre = 1 user)
- `sky/settings` — préférences hub
- `sky/llm-config` — config LLM / tools / matrice de risque (J4)

Pattern obligatoire : les controllers **n'appellent jamais `Inertia::render` directement** — toujours via `ResponseFactory`. Middleware RBAC via `HasConfigurableMiddleware`. UI ciblée écran tactile 10" (1280×800, cibles min 44×44px), dark theme Stitch (fond quasi-noir, accents bleu/vert, Material Icons).

## Documentation canonique — où regarder avant d'agir

Toute décision doit pointer vers :

1. **`docs/logiciel/adr/`** — 14 ADR figés qui font autorité (transport, données, sécurité, stack Orbit, UX, pipeline vocale, infra k3s, repos, langages, monitoring, équipe, jalons, branding, offline-first). Index dans `01-adr-00-decisions.md`, résumé dans `adr-00-resume-choix-techniques.md`.
2. **`docs/logiciel/jalons/.cursorrules`** — règles transversales (stack, squads, offline-first, ordonnancement).
3. **`docs/logiciel/jalons/jalon-{1..6}-*/.cursorrules`** — périmètre, ADR pertinents, critères de succès, squads, langages autorisés du jalon.
4. **`docs/logiciel/jalons/jalon-*/M*/.cursorrules`** et **`M*/F*/.cursorrules`** — spécifiques au milestone / feature (nommage strict `Mnn-*/Fnn.nn-*/Tnn-*.md`).
5. **`docs/logiciel/developpement/`** :
   - `git-worktrees-sky.md` (workflow worktrees par jalon)
   - `platformio-esp-matter-setup.md` (PIO + ESP-Matter pour luxia & c6)
   - `cablage-serie-rpi4b-esp32.md` (câblage UART RPi4B ↔ ESP32, `/dev/ttyAMA0` @ 115200)
   - `reverb-compatibilite-rust.md` (Pusher v7, signatures HMAC-SHA256 private channels)

## Jalons (ADR-012) — planification

| # | Mois | Contenu | Prérequis |
|---|------|---------|-----------|
| 1 | 1-2 | k3s up + Orbit affiché kiosk sur écran 10" (mock devices) | — |
| 2 | 2-3 | Luxia contrôlée depuis Orbit via Matter + Wi-Fi (Phase A WROOM, Phase B C6) | J1 |
| 3 | 3-5 | Watch BLE/Wi-Fi connectée au hub | J2 (parallèle J4) |
| 4 | 3-5 | LLM tool calling texte → Luxia + préparation routes audio | J2 (parallèle J3) |
| 5 | 5-6 | Pipeline vocal complète Watch → STT → LLM → Luxia | J3 + J4 |
| 6 | 6-7 | TTS feedback + notifications push montre | J5 |
| — | 7-8 | Stabilisation, tests intégration, polish, docs | — |

## Équipe / squads (ADR-011)

| Squad | Effectif | Périmètre |
|-------|----------|-----------|
| Watch | 3 | Firmware montre, UI, BLE/Wi-Fi, voice pipeline |
| Orbit | 3 | Laravel, Vue, data, UX, WebSocket |
| Hub/Infra | 2 | k3s, Helm, device-service, Matter bridge, monitoring, CI/CD |
| LLM/AI | 1 | Ollama, tool calling, prompts, STT/TTS, benchmarks |
| Framework/Luxia | 1 | sky-framework, firmware Luxia, OTA, nouveaux composants |

## Règles pour moi (Claude)

1. **ADR = sacrés.** Ne jamais contredire un ADR figé sans que l'utilisateur en débatte explicitement. Si un conflit apparaît, le signaler avant de continuer.
2. **Lire les `.cursorrules` nichés** avant de toucher au code d'un jalon/milestone/feature. Nommage strict : `Mnn-*/Fnn.nn-*/Tnn-*.md`. Le fichier le plus proche prime.
3. **Respecter le périmètre de squad** quand c'est évident. Si une tâche traverse plusieurs squads, le dire.
4. **Langages autorisés** (ADR-009) : C/C++ (firmwares), Rust (services perf), TS (services légers / tooling), PHP+Vue+TS (Orbit), YAML/Jinja2/Bash (infra). **Python = exception argumentée. Go = exclu.**
5. **Ne jamais modifier l'intérieur d'un sous-repo sans confirmation** de l'utilisateur — chaque sous-dossier a son propre git (sky-luxia, sky-luxia-s3, sky-hub-antenna, sky-hub-antenna-wroom, sky-framework, sky-hub-services, sky-infra, sky-infra-min, sky-orbit, sky-proto, sky-watch). Préférer des commits dans le workspace racine quand on touche aux docs/scripts locaux.
6. **Worktrees plutôt que branches multiples** : un jalon en parallèle ⇒ `./scripts/sky-worktree.sh add <repo> <slug-jalon>` crée `worktrees/<repo>/<slug>/` sur la branche `jalon/<slug>`. Le dossier `worktrees/` est gitignoré.
7. **Clusters Matter = source unique via `sky-proto/`.** Ne jamais dupliquer des définitions de cluster : la CI génère les headers C, classes PHP, types TS. Ajouter un cluster = PR sur `sky-proto` d'abord, puis consommation dans les autres repos.
8. **Events temps réel Orbit ↔ device-service** : Laravel Reverb (Pusher v7 protocol). Channels privés signés HMAC-SHA256 avec `REVERB_APP_SECRET` côté Rust dans `websocket/client.rs`.
9. **Offline-first (ADR-014)** : aucune feature critique ne doit dépendre d'Internet. Modèles LLM/STT/TTS pré-chargés sur NVMe. Synchro S3 différée (backup nocturne / rapports).
10. **Branding (ADR-013)** : noms figés `sky`, `orbit`, `luxia`, `sky-watch`. Domaine `sky.stollpy.com`. Ne pas renommer.
11. **GPIO Luxia** (référence rapide) : stepper `IN1=17 IN2=16 IN3=4 IN4=0`, battery ADC `34`, wake button `35`. GPIO 0 = boot mode, à surveiller.
12. **Câblage série RPi4B ↔ ESP32** : `Pin 8 (GPIO14 TXD) → GPIO 16 (RX UART2)`, `Pin 10 (GPIO15 RXD) ← GPIO 17 (TX UART2)`, GND commun. Device-service lit `/dev/ttyAMA0 @ 115200`. PL011 libéré du Bluetooth via `dtoverlay=disable-bt` dans `/boot/firmware/config.txt`.
13. **Design Orbit** : MCP **Stitch** gère les maquettes, project ID **`9751820179692887842`** (voir `docs/.cursorrules`). Utiliser les outils `mcp__stitch__*` quand disponibles pour récupérer / générer des écrans.
14. **Base UI écran 10"** : taille `1280×800`, cibles interactives min `44×44 px`, thème dark, accents bleu/vert, Material Icons.
15. **Budget latences (ADR-006)** : audio capture+transport ~200-300 ms, STT ~500-1500 ms, LLM+tool ~1000-2000 ms, Matter exec ~200 ms → total cible **2.2-4.5 s**. Feedback TTS ~300-500 ms.
16. **LLM function calling** : le champ **`reasoning`** est **obligatoire** dans chaque réponse (ReAct-like). Matrice de risque actions ↔ rôles RBAC stockée en DB (évolutive). Actions critiques → confirmation explicite.
17. **Sécurité (ADR-003)** : auth device ↔ hub = Matter fabric natif (pas de credentials manuelles). OTA auto pour patchs sécurité, manuel (Orbit) pour features. Accès distant = tunnel VPN (Tailscale/WireGuard), pas d'exposition publique.
18. **Firmware conventions** (voir `firmware/CLAUDE.md` et `firmware/.claude/skills/`) : préfixes `sky_` (framework) / `luxia_` (produit), `esp_err_t` partout, `ESP_LOG{I,W,E,D}(TAG,...)`, TAG `"sky_<component>"`, lifecycle `init → start → stop → deinit` idempotent, événements avec ID par range (core `0x00xx`, stepper `0x01xx`, network `0x02xx`, OTA `0x028x`, power `0x03xx`), NVS namespace/key ≤ 15 chars.

## Hub SSH & tracking d'état

Le hub Sky est une carte SBC physique accessible en SSH depuis le mac de dev.
Pour ne pas supposer l'architecture (RPi4B dev / OPi5+ prod / KiwiPi alt / future
board maison), Claude doit **consulter `.claude/context/sky-hub-state.md` en début
de session** pour connaître la carte courante, l'OS, les services en running, le
mode orchestration (k3s vs systemd), et le chemin du checkout.

- Connexion : `ssh sky-hub` (user `sky`, clé déjà provisionnée, sudo `pwdno`).
- Rafraîchir l'état : `./scripts/sky-hub-refresh.sh` ou slash command `/sky-hub-refresh`.
  Un hook SessionStart (`.claude/settings.json`) le lance automatiquement en
  arrière-plan à chaque démarrage (timeout 5 s, silencieux si le hub est down).
- Build `device-service` : **obligatoirement sur le hub** (ARM natif, pas de
  cross-compile). Pattern : push local → `git pull` + `cargo build --release`
  sur le hub → `systemctl restart device-service`.
- Règle d'ops : UART, BlueZ, OTBR, hostapd, systemd, k3s, réseau LAN ⇒ **hub obligatoire**.
  Édition code, Orbit Vite, Laravel test, codegen TS ⇒ **local OK**.
- Skill détaillé : `.claude/skills/sky-hub-ssh/SKILL.md`.
- Guide humain : `docs/logiciel/developpement/sky-hub-ssh.md` (incluant troubleshooting
  connexion / clé / fingerprint / changement de carte).

## Migration `.cursor` → `.claude`

| Ancien | Nouveau (Claude Code) | Statut |
|--------|-----------------------|--------|
| `.cursorrules` (racine, stub) | `CLAUDE.md` (ce fichier) | créé |
| `docs/.cursorrules` | `docs/CLAUDE.md` + `.cursorrules` conservé | créé (miroir) |
| `docs/logiciel/jalons/**/.cursorrules` (~70 fichiers) | **laissés en place** — lisibles à la demande, trop nombreux à dupliquer. Claude doit les `Read` quand il travaille sur un jalon/milestone/feature | inchangé |
| `firmware/.cursor/rules/*.mdc` | `firmware/CLAUDE.md` (consolidé) | créé |
| `firmware/.cursor/skills/<n>/SKILL.md` (8 skills) | `firmware/.claude/skills/<n>/SKILL.md` | copié (format identique) |
| `firmware/.cursor/plans/` et `firmware/.cursor/learning/` | laissés en place, accessibles via `Read` | inchangé |
| `.claude/settings.local.json` | **préservé** (config user existante) | inchangé |
| — | `.claude/settings.json` (défauts projet) | créé |

## Commandes rapides (dev quotidien)

```bash
# Créer un worktree pour un jalon
./scripts/sky-worktree.sh ensure-commits      # une fois
./scripts/sky-worktree.sh add sky-orbit 1-k3s-orbit-ecran
./scripts/sky-worktree.sh list
./scripts/sky-worktree.sh remove sky-orbit 1-k3s-orbit-ecran

# Orbit (dans sky-orbit/)
composer install && npm install
npm run dev          # Vite + artisan serve + queue + pail en concurrent
php artisan reverb:start --debug

# Firmware Luxia (PlatformIO)
pio run -e esp32                  # WROOM-32 (Phase A)
pio run -e esp32c6                # Phase B quand le C6 arrive
pio run -e esp32 -t upload && pio device monitor

# Firmware ESP-IDF classique
idf.py set-target esp32 && idf.py build flash monitor

# Device-service (Rust)
cd sky-hub-services/device-service && cargo run

# Proto codegen
cd sky-proto && npm run validate && npm run generate

# Infra
cd sky-infra && ansible-playbook -i ansible/inventory ansible/playbooks/site.yml
helm upgrade --install orbit helm/charts/orbit -f values.yaml
```

## Pointeurs rapides

- Code firmware → `firmware/`, `sky-luxia/`, `sky-hub-antenna/`, `sky-framework/`
- Code Orbit → `sky-orbit/app/`, `sky-orbit/resources/js/`, `sky-orbit/packages/sky/*`
- Code services hub → `sky-hub-services/device-service/src/`
- Infra → `sky-infra/{ansible,helm,docker,manifests}/`
- Clusters Matter → `sky-proto/clusters/{standard,sky-custom}/`
- Docs produit → `docs/logiciel/{adr,jalons,developpement}/`
- Plans firmware détaillés → `firmware/.cursor/plans/` (00-init, 01-http-server, 04-smartwatch, 05-sky-container, 07-sky-hub)
- Skills firmware → `firmware/.claude/skills/` (sky-core-usage, sky-stepper-usage, sky-wifi-usage, sky-matter-usage, sky-ota-usage, sky-power-usage, sky-new-component, luxia-firmware-assembly)
