# 07-00 — Sky Hub · Questions d'Architecture

> **Objectif** : cadrer toutes les décisions avant de coder. Chaque section contient les questions, le contexte, et les options connues. L'équipe répond dans la colonne « Réponse » ou en commentaire, puis on fige une **ADR** (Architecture Decision Record) par sujet.

---

## 0. Contexte rappel

- **Sky Container** : Orange Pi 5 Plus 32 Go, k3s single-node, NVMe 512 Go, écran 10" tactile
- **Sky Orbit** : app Laravel 12 / Inertia / Vue 3, dashboard domotique, tournant dans k3s
- **LLM** : 2 profils d'inférence (taille à confirmer après bench OPi5+)
- **Radio** : ESP32-C6 (Thread/Matter/802.15.4), dongle BLE USB, Wi-Fi M.2
- **Devices actuels** : Luxia (ESP32, stepper, Matter Window Covering), Sky Watch (ESP32-S3, T-Watch S3+)
- **Équipe** : 10 personnes, dont 3 sur la montre

---

## 1. Contrat d'intégration devices ↔ hub

C'est **LA** décision structurante. Chaque device (Luxia, Watch, capteurs futurs) doit parler au hub via un protocole normalisé.

### 1.1 Transport principal

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q1** : Le hub Sky communique-t-il avec les devices via **Matter exclusivement**, via **MQTT**, ou via un **mix** ? | Matter est standard mais le controller côté Linux est complexe (chip-tool / python-matter-server). MQTT est simple et battle-tested. | **A)** Matter pur (ESP32-C6 border router + chip-tool sur Linux) — **B)** MQTT broker (Mosquitto dans k3s) + devices publient en MQTT — **C)** Matter pour le discovery/commissioning, puis MQTT pour le data flow temps réel — **D)** Matter pour Luxia (car déjà implémenté), BLE/Wi-Fi custom pour la montre |
| **Q2** : Si Matter, quel rôle exact pour l'ESP32-C6 ? | Le C6 peut être un **RCP** (Radio Co-Processor, le Linux fait le routage Thread) ou un **co-processeur autonome** (le C6 gère tout Thread + Matter, expose une API série/USB au Linux). | **A)** RCP pur (OTBR sur Linux + C6 = radio 802.15.4 seulement) — **B)** Co-processeur autonome (C6 = border router complet, parle au Linux via UART/USB protobuf ou MQTT local) |
| **Q3** : Pour la montre (BLE/Wi-Fi), quel transport vers le hub ? | La montre n'a pas de Thread. Elle fait BLE + Wi-Fi. Les commandes vocales passent par elle. | **A)** BLE GATT direct (dongle BLE USB → service BlueZ → hub) — **B)** Wi-Fi HTTP/WebSocket (la montre se connecte en Wi-Fi à Orbit directement) — **C)** BLE pour le pairing/proximité, puis Wi-Fi pour le data (comme Apple Watch) |
| **Q4** : Quel format de message entre devices et hub ? | Il faut un schéma versionné pour ne pas casser les devices déjà déployés quand le hub évolue. | **A)** JSON over MQTT (simple, lisible, un peu lourd) — **B)** Protobuf over MQTT/UART (compact, typé, versionné nativement) — **C)** Matter TLV natif (pas besoin de format custom si tout passe par Matter) — **D)** CBOR (compact comme protobuf, sans codegen) |

### 1.2 Modèle de données « device »

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q5** : Comment un device se déclare-t-il au hub ? | Il faut un registre des devices connus, leur type, leurs capacités, et leur état. | **A)** Matter commissioning natif (le hub découvre via mDNS/DNS-SD) — **B)** Enregistrement manuel dans Orbit (l'admin ajoute un device avec son ID) — **C)** Auto-discovery : le device s'annonce en MQTT à `sky/devices/announce` avec un manifest JSON de ses capacités |
| **Q6** : Quelle taxonomie de « capacités » ? | Luxia a des capacités (move_to, open, close, stop, calibrate). La montre a des capacités (voice_command, sensor_data, notifications). Un capteur futur aura d'autres capacités. | **A)** On réutilise les **clusters Matter** comme taxonomie (Window Covering, Temperature, etc.) — **B)** On invente un schéma maison `sky_capability` plus flexible — **C)** On mappe les clusters Matter quand ils existent, et on étend avec un namespace `sky:custom:*` pour le reste |
| **Q7** : Granularité des événements temps réel ? | Le bus `sky_event` du firmware publie des events internes (STEPPER_POSITION, BATTERY_LOW…). Est-ce qu'on veut les mêmes events au niveau du hub, ou un modèle plus agrégé ? | **A)** Mapping 1:1 : chaque `SKY_EVENT_*` firmware est republié vers le hub — **B)** Agrégation : le device publie un « état complet » périodiquement (toutes les N secondes) + des events critiques — **C)** Les deux : état périodique + events push pour les changements importants |

### 1.3 Sécurité

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q8** : Auth device → hub ? | Si MQTT, il faut des credentials par device. Si Matter, le fabric gère. Si HTTP, il faut des tokens. | **A)** Matter fabric (natif, pas de credentials manuelles) — **B)** MQTT avec username/password par device (stocké en NVS côté device) — **C)** mTLS (certificats client, plus lourd mais très solide) — **D)** Token JWT pré-provisionné en usine |
| **Q9** : Le hub est-il exposé sur Internet ou LAN only ? | Impact sur TLS, auth, firewall. | **A)** LAN only (réseau local, pas d'accès distant) — **B)** LAN + tunnel (Cloudflare Tunnel, Tailscale, WireGuard) pour accès distant — **C)** Exposition directe (déconseillé, mais posons la question) |
| **Q10** : Politique de mise à jour des devices ? | Luxia a déjà `sky_ota`. Mais qui déclenche l'update ? | **A)** Le hub pousse l'OTA automatiquement (canal MQTT/HTTP) — **B)** L'utilisateur déclenche depuis Orbit (bouton "mettre à jour") — **C)** Les deux : auto pour les patchs de sécurité, manuel pour les features |

---

## 2. Sky Orbit — Architecture applicative

### 2.1 Stack & déploiement

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q11** : Quelle version Laravel / Inertia / Vue ? | Laravel 12 est sorti. Inertia v2 est stable. Vue 3.5+. | Confirmer : **Laravel 12 + Inertia v2 + Vue 3.5** (ou autre preference) |
| **Q12** : Base de données ? | PostgreSQL est le standard. SQLite serait plus léger pour un SBC. MySQL/MariaDB aussi possible. | **A)** PostgreSQL (dans k3s) — **B)** SQLite (fichier sur NVMe, zéro overhead, suffisant pour un hub local) — **C)** MariaDB |
| **Q13** : Cache / Queue ? | Laravel a besoin d'un driver pour les queues (LLM jobs, events async). | **A)** Redis (dans k3s, mais ~30-50 Mo RAM de plus) — **B)** SQLite (driver queue database, zéro dépendance) — **C)** Beanstalkd (léger) |
| **Q14** : Temps réel (WebSocket) ? | Orbit doit afficher les changements d'état des devices en live. | **A)** Laravel Reverb (natif Laravel 12, Rust, léger) — **B)** Soketi (open-source Pusher-compatible) — **C)** Centrifugo (Go, très performant) — **D)** SSE (Server-Sent Events, plus simple, pas de WebSocket) |
| **Q15** : L'écran tactile 10" affiche quoi exactement ? | C'est un écran HDMI branché sur l'OPi5+. | **A)** Un navigateur kiosk (Chromium) qui affiche Orbit en plein écran — **B)** Un desktop Linux complet — **C)** Une app native Qt/Flutter pour l'écran + Orbit en web pour les mobiles |

### 2.2 UX / Features

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q16** : Orbit est multi-utilisateur ? | Une famille de N personnes utilise la malette. Rôles ? | **A)** Un seul admin, pas de multi-user — **B)** Multi-user avec rôles (admin, membre famille, invité) — **C)** Multi-user + profils par montre (chaque Sky Watch = un utilisateur) |
| **Q17** : Quelles « pages » au minimum pour le MVP ? | Il faut définir le scope du dashboard. | Proposition de pages MVP : 1) **Home** (vue d'ensemble, état des devices) — 2) **Devices** (liste, détail par device, contrôles) — 3) **Scènes** (groupes d'actions : "Bonne nuit" = fermer tous les stores) — 4) **LLM / Assistant** (chat, historique) — 5) **Settings** (réseau, comptes, OTA) |
| **Q18** : Notifications / alertes ? | Batterie faible Luxia, device déconnecté, OTA disponible… | **A)** Notifications dans Orbit uniquement (toast + page alertes) — **B)** Push sur la montre aussi (via BLE) — **C)** Push montre + optionnel email/SMS (via LTE-M de la montre ou API externe) |

---

## 3. Flow vocal complet (Watch → Orbit → LLM → Device)

C'est le flow critique qui traverse **tous** les composants. Il faut le définir bout en bout.

### 3.1 Pipeline détaillé

```
┌─────────────┐    audio     ┌─────────────┐   texte    ┌──────────┐
│  Sky Watch   │ ──────────→ │   STT       │ ────────→  │  Orbit   │
│  (micro)     │  BLE/Wi-Fi  │ (sur hub)   │  API POST  │  (Laravel)│
└─────────────┘              └─────────────┘            └────┬─────┘
                                                             │ prompt + context
                                                             ▼
                                                        ┌──────────┐
                                                        │   LLM    │
                                                        │ (Ollama) │
                                                        └────┬─────┘
                                                             │ tool call JSON
                                                             ▼
                                                        ┌──────────┐
                                                        │  Orbit   │
                                                        │ (tools)  │ → valide + exécute
                                                        └────┬─────┘
                                                             │ commande normalisée
                                                             ▼
                                                        ┌──────────┐
                                                        │ Device   │
                                                        │ Service  │ → Matter/BLE/MQTT
                                                        └────┬─────┘
                                                             │ état mis à jour
                                                             ▼
                                                        ┌──────────┐   push    ┌───────────┐
                                                        │  Orbit   │ ───────→  │ Sky Watch  │
                                                        │ (WS/SSE) │  BLE/Wi-Fi│ (feedback) │
                                                        └──────────┘           └───────────┘
```

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q19** : Où tourne le STT (Speech-to-Text) ? | Le micro de la montre capture l'audio. Il faut le transcrire. | **A)** Sur la montre elle-même (ESP32-S3 + micro modèle TinyML) → texte envoyé au hub — **B)** Audio brut envoyé au hub → STT sur l'OPi5+ (Whisper.cpp / faster-whisper) — **C)** Audio brut envoyé au hub → STT cloud (Google, Deepgram) |
| **Q20** : Quel format audio pour le transport Watch → Hub ? | L'ESP32-S3 a un micro PDM (SPM1423HM4H-B). | **A)** PCM 16-bit 16 kHz brut (simple, ~32 Ko/s) — **B)** Opus compressé (~3-6 Ko/s, moins de bande passante) — **C)** ADPCM (compromis) |
| **Q21** : Quel transport pour l'audio ? | BLE a un débit limité (~2 Mbps théorique, ~200 Kbps pratique). Wi-Fi est plus rapide. | **A)** BLE GATT notify (petits chunks, ~20-244 bytes par paquet, latence ok pour du STT) — **B)** Wi-Fi HTTP POST (buffer complet puis envoi) — **C)** Wi-Fi WebSocket (streaming continu, meilleure latence) |
| **Q22** : Comment le LLM reçoit-il les tool calls ? | Le LLM doit pouvoir « appeler » des outils (fermer un store, lire un capteur, etc.). | **A)** Function calling natif (Gemma 2 / Mistral supportent le tool calling, le LLM retourne un JSON structuré) — **B)** Parsing regex de la réponse texte (fragile) — **C)** ReAct pattern (le LLM raisonne puis émet une action formatée) |
| **Q23** : Politique de validation avant exécution ? | Si l'utilisateur dit "ferme tous les stores", le LLM génère un tool call. Faut-il une confirmation ? | **A)** Exécution directe (confiance totale dans le LLM, rapide) — **B)** Confirmation systématique (la montre affiche "Fermer 3 stores ? ✓ ✗") — **C)** Par niveau de risque : actions réversibles = direct, actions critiques (factory reset, unlock porte) = confirmation |
| **Q24** : TTS (Text-to-Speech) pour la réponse vocale ? | Après l'action, la montre doit donner un feedback vocal. | **A)** TTS sur le hub (Piper TTS, léger, ~50 Mo RAM) → audio envoyé à la montre — **B)** TTS sur la montre (modèle embarqué, qualité limitée) — **C)** Feedback haptique + texte sur l'écran de la montre (pas de TTS, plus simple) |

### 3.2 Latence cible

| Question | Contexte |
|----------|----------|
| **Q25** : Quel budget latence acceptable pour une commande vocale ? | De "Hey Sky, ferme les stores" à "stores en mouvement". Benchmark typique : Alexa ~1.5–3s, Siri ~2–4s. Sur un SBC ARM local avec LLM petit, on vise combien ? |

Proposition de budget latence :

| Étape | Cible |
|-------|-------|
| Audio capture + transport | ~300 ms |
| STT (Whisper tiny/base sur NPU/CPU) | ~500–1500 ms |
| LLM inference (tool calling) | ~1000–3000 ms |
| Exécution commande (MQTT/Matter) | ~200 ms |
| Feedback (TTS + transport) | ~500 ms |
| **Total** | **~2.5–5.5 s** |

Est-ce acceptable ? Si non, quels compromis (STT plus petit, pas de TTS, etc.) ?

---

## 4. Architecture k3s & services

### 4.1 Topologie des pods

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q26** : Quels services dans k3s vs bare-metal ? | Certains services (BlueZ, OTBR) ont besoin d'un accès hardware direct (USB, GPIO). | **A)** Tout dans k3s (même BlueZ et OTBR, avec `hostNetwork` et device passthrough) — **B)** BlueZ et OTBR en bare-metal (systemd services), le reste dans k3s — **C)** k3s uniquement pour Orbit + LLM, tout le reste en systemd |
| **Q27** : Combien de pods / services minimum pour le MVP ? | Il faut dimensionner la RAM k3s. | Estimation : Orbit (Laravel + Reverb + queue worker) ~300-500 Mo, DB ~100-200 Mo, Ollama ~4-16 Go (selon modèle), MQTT broker ~20 Mo, Device service ~50-100 Mo, STT ~200-500 Mo, TTS ~50-100 Mo. **Total : ~5-18 Go** sur 32 Go |
| **Q28** : Storage class pour les volumes persistants ? | k3s a besoin de persistent volumes (DB, modèles LLM, config). | **A)** `local-path` (défaut k3s, hostPath simple) — **B)** Longhorn (distribué, snapshots, mais overhead CPU/RAM) — **C)** hostPath direct avec backup rsync |

### 4.2 Réseau

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q29** : Le hub crée-t-il son propre réseau Wi-Fi (AP) ou se connecte-t-il à un routeur existant ? | En mode « malette portable », il n'y a peut-être pas de routeur. | **A)** Client Wi-Fi uniquement (se connecte à un réseau existant) — **B)** AP mode (le hub crée son réseau, les devices s'y connectent) — **C)** Les deux : AP pour les devices, client pour l'uplink Internet |
| **Q30** : DNS / mDNS ? | Les devices doivent trouver le hub. | **A)** mDNS (`sky-hub.local`) — **B)** IP fixe configurée dans chaque device — **C)** DHCP + mDNS |

---

## 5. Découpage en repositories

| Question | Contexte | Options connues |
|----------|----------|-----------------|
| **Q31** : Quel découpage de repos pour l'équipe de 10 ? | Il faut que les squads puissent travailler en parallèle sans se marcher dessus. | Proposition : |

```
sky-framework/        → SDK firmware (sky_core, sky_stepper, sky_wifi, sky_matter, sky_power, sky_ota, sky_sleep, sky_http)
luxia-firmware/       → Produit Luxia (main.c, bridges, cmd, http routes) — dépend de sky-framework
sky-watch-firmware/   → Firmware montre (squad de 3) — dépend de sky-framework (partiel)
sky-c6-firmware/      → Firmware ESP32-C6 border router
sky-orbit/            → App Laravel / Inertia / Vue 3
sky-hub-services/     → Microservices pont (device-service, stt-service, tts-service)
sky-infra/            → Helm charts, Dockerfiles, scripts provisioning, Ansible/cloud-init
sky-proto/            → Schémas protobuf / JSON Schema partagés (contrat d'API)
```

| Question | Détail |
|----------|--------|
| **Q32** : `sky-framework` reste-t-il un composant PlatformIO (git submodule / PlatformIO library) ou un repo séparé avec CI ? | Actuellement c'est dans `components/` du repo firmware. Pour que la montre et le C6 l'utilisent, il faut l'extraire. |
| **Q33** : Monorepo vs multi-repo ? | Un monorepo (Nx, Turborepo) simplifierait le CI et les dépendances croisées, mais c'est moins familier en embedded. Un multi-repo avec des contrats d'API (sky-proto) est plus classique. |
| **Q34** : Quel CI/CD ? | GitHub Actions ? GitLab CI ? Self-hosted runner sur l'OPi5+ pour les tests d'intégration hardware ? |

---

## 6. Équipe & répartition

| Question | Contexte |
|----------|----------|
| **Q35** : Quelle répartition des 10 personnes par domaine ? | Proposition basée sur les chantiers identifiés : |

| Squad | Personnes | Périmètre |
|-------|-----------|-----------|
| **Watch** | 3 | Firmware montre, UI écran, BLE/Wi-Fi, intégration voice pipeline (STT capture + feedback) |
| **Orbit** | 3 | Laravel backend, Vue frontend, modèle de données, UX/UI dashboard, WebSocket |
| **Hub / Infra** | 2 | k3s, Helm charts, device-service, MQTT/Matter bridge, monitoring, sécurité, CI/CD |
| **LLM / AI** | 1 | Intégration Ollama, tool calling, prompt engineering, STT/TTS services, benchmarks |
| **Framework / Luxia** | 1 | Maintenance sky-framework, firmware Luxia, OTA, nouveaux composants sky_* |

**Q36** : Cette répartition convient-elle ? Des ajustements ?

---

## 7. Priorités & dépendances

| Question | Contexte |
|----------|----------|
| **Q37** : Quel est le MVP minimal pour une démo ? | Proposition : « Dire une commande vocale sur la montre → le store Luxia bouge ». Ça traverse tout le pipeline. |
| **Q38** : Quels sont les jalons intermédiaires ? | Proposition : |

```
Jalon 1 : k3s up + Orbit affiché sur l'écran 10" (mock devices)
Jalon 2 : Luxia contrôlée depuis Orbit via Matter/MQTT (sans voix)
Jalon 3 : Montre connectée en BLE/Wi-Fi au hub, envoie un event
Jalon 4 : LLM qui répond à un prompt texte et exécute un tool call → Luxia bouge
Jalon 5 : Pipeline vocal complet (Watch micro → STT → LLM → tool → Luxia)
Jalon 6 : TTS feedback + notifications push sur la montre
```

**Q39** : Ordre et timeline souhaitée pour ces jalons ?

---

## 8. Questions ouvertes diverses

| # | Question |
|---|----------|
| **Q40** | Le hub doit-il supporter d'autres protocoles que Matter/BLE/Wi-Fi à terme ? (Zigbee natif, Z-Wave, KNX, 433 MHz ?) |
| **Q41** | Y a-t-il un besoin de persistance historique des données capteurs (graphiques de température, historique positions store, etc.) ? Si oui, time-series DB (InfluxDB, TimescaleDB) ? |
| **Q42** | Le hub doit-il fonctionner **offline** (sans Internet) ? Ça impacte le LLM (obligatoirement local), le STT (obligatoirement local), le TTS (local), les mises à jour (USB/LAN). |
| **Q43** | Le nom de domaine / branding est-il figé ? (`sky`, `orbit`, `luxia`, `sky-watch`…) Ça impacte les namespaces, les noms de pods, les routes API. |
| **Q44** | Y a-t-il un budget temps (deadline proto ? deadline démo ?) |
| **Q45** | Langages autorisés côté hub services : Go ? Rust ? Python ? TypeScript ? PHP only (tout dans Laravel) ? |

---

> **Action** : L'équipe répond à chaque question (même partiellement). On transforme ensuite les réponses en ADR (Architecture Decision Records) dans `07-sky-hub/01-adr-*.md`.
