# 05 — Sky Container · Plan Général

## Objectif

Concevoir et assembler une **malette portable** hébergeant un **cluster ARM k3s** capable de :

- Faire tourner une **app web** (Laravel / Inertia / Vue.js) dans un pod k3s
- Exécuter un **LLM local** (Gemma 4 26B MoE ou Mistral Small 4 119B en distribué)
- Servir de **hub de communication** avec Luxia (Matter / Wi-Fi) et la montre (BLE / Wi-Fi)
- Exposer des **mini-apps** orchestrées par k3s (MQTT broker, dashboard, etc.)
- Offrir une **station de charge** Qi pour les appareils du système

## Architecture

- **Backplane** : Turing Pi 2.5 (mini-ITX, 4 slots compute, switch GbE intégré)
- **Compute** : 1× RK1 32 Go (master + LLM) + 3× RK1 8/16/32 Go (nœuds worker)
- **Stockage** : 1× NVMe M.2 par nœud (OS + données)
- **Affichage** : écran 10" HDMI tactile dans le couvercle
- **Radio** : ESP32-C6 (Thread/Matter) + dongle BLE USB + antennes SMA externes
- **Charge** : module Qi TX 15 W dans le couvercle
- **Enclosure** : malette rigide (type Pelican 1450 ou équivalent)

## Phases du plan

| Fichier | Phase |
|---------|-------|
| `05-01-sky-container-1-hardware-bom.md` | BOM complète V2 (cluster Turing Pi 4 nœuds) |
| `05-02-sky-container-2-proto-bom.md` | BOM Proto V1 (Orange Pi 5 Plus, malette 3D, ~$500) |
