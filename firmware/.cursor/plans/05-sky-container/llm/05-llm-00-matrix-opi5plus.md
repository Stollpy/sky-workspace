# 05-LLM-00 — Matrice LLM pour Orange Pi 5 Plus 32 Go

> **Matériel** : Orange Pi 5 Plus — RK3588 (4× A76 + 4× A55), 32 Go LPDDR4X, NPU 6 TOPS, NVMe 512 Go
>
> **Budget RAM disponible pour l'inférence** : variable selon les services k3s actifs (voir §5).

---

## 1. Contexte d'exécution

Deux runtimes possibles sur RK3588 :

| Runtime | Accélération | Compatibilité modèles | Notes |
|---------|-------------|----------------------|-------|
| **Ollama / llama.cpp** | CPU ARM uniquement | GGUF standard (tous modèles HuggingFace) | Écosystème large, API OpenAI-compatible, facile à déployer. **Mais lent** : les cœurs A76 plafonnent à ~2-12 tok/s selon taille. |
| **RKLLM (rknn-llm)** | NPU RK3588 | `.rkllm` format propriétaire (conversion nécessaire depuis HuggingFace, outil x86-64 requis) | **5-20× plus rapide** que CPU. Modèles pré-convertis limités. Écosystème jeune. API serveur OpenAI-compatible via RKLLM-API-Server. |

**Recommandation proto** : commencer avec **Ollama** (déploiement immédiat, test fonctionnel), puis migrer vers **RKLLM** pour la production (performance).

---

## 2. Matrice des modèles — Vue d'ensemble

### Légende des catégories

| Catégorie | Signification | Critères |
|-----------|---------------|----------|
| **LARGE** | Fonctionne très confortablement | RAM modèle < 30% de la RAM libre, vitesse > 10 tok/s, marge pour le contexte |
| **OK** | Fonctionne bien | RAM modèle < 50% de la RAM libre, vitesse > 5 tok/s, utilisable en interactif |
| **SERRÉ** | Fonctionne mais tendu | RAM modèle 50-80% de la RAM libre, vitesse 2-5 tok/s, latence notable |
| **TROP LOURD** | Inutilisable en pratique | RAM modèle > 80% RAM libre, vitesse < 2 tok/s, ou swap constant |

---

## 3. Modèles par cas d'usage

### 3.1 — Cas principal : Assistant domotique avec Tool Calling

C'est le modèle qui reçoit les commandes vocales transcrites et génère des tool calls JSON pour piloter les devices. Il doit être **rapide** (latence interactive) et supporter le **function calling** nativement.

| Modèle | Params | Quant. | RAM GGUF | RAM RKLLM | Ollama CPU tok/s | RKLLM NPU tok/s | Tool Calling | Catégorie |
|--------|--------|--------|----------|-----------|-----------------|-----------------|-------------|-----------|
| **Qwen3 0.6B** | 0.6B | Q4_K_M | ~0.5 Go | ~0.4 Go | ~15-20 | ~25-30 | Oui (natif) | **LARGE** |
| **Llama 3.2 1B Instruct** | 1B | Q4_K_M | ~0.7 Go | ~0.6 Go | ~12-18 | ~18-25 | Oui (natif) | **LARGE** |
| **Qwen3 1.7B** | 1.7B | Q4_K_M | ~1.2 Go | ~1.0 Go | ~10-15 | ~15-20 | Oui (natif) | **LARGE** |
| **Gemma 4 2B** | 2B | Q4_K_M | ~1.5 Go | ~1.2 Go | ~8-12 | ~12-18 | Oui | **LARGE** |
| **Phi-4 Mini 3.8B** | 3.8B | Q4_K_M | ~2.5 Go | ~2.0 Go | ~6-9 | ~8-12 | Oui (natif `<\|tool\|>`) | **OK** |
| **Llama 3.2 3B Instruct** | 3B | Q4_K_M | ~2.0 Go | ~1.7 Go | ~7-10 | ~10-14 | Oui (natif) | **OK** |
| **Qwen3 4B** | 4B | Q4_K_M | ~2.8 Go | ~2.3 Go | ~5-8 | ~5-8 | Oui (natif) | **OK** |
| **Gemma 4 E4B** | 4B | Q4_K_M | ~2.8 Go | ~2.3 Go | ~5-8 | ~5-8 | Oui | **OK** |
| **Qwen3 8B** | 8B | Q4_K_M | ~5.0 Go | ~4.2 Go | ~3-5 | ~3-5 | Oui (natif) | **SERRÉ** |
| **Gemma 4 9B** | 9B | Q4_K_M | ~5.5 Go | ~4.5 Go | ~2-4 | ~3-5 | Oui | **SERRÉ** |
| **Gemma 4 12B** | 12B | Q4_K_M | ~7.0 Go | N/A | ~1.5-3 | N/A (trop gros NPU?) | Oui | **SERRÉ** |
| **Mistral Small 3.2 24B** | 24B | Q4_K_M | ~14 Go | N/A | ~0.5-1 | N/A | Oui (excellent) | **TROP LOURD** |
| **Gemma 4 27B** | 27B | Q4_K_M | ~16 Go | N/A | ~0.3-0.6 | N/A | Oui | **TROP LOURD** |
| **Gemma 4 26B MoE** | 26B (3.8B actifs) | Q4_K_M | ~15 Go | N/A | ~0.8-1.5 | N/A | Oui | **TROP LOURD** |

> **Note** : les vitesses RKLLM NPU sont estimées à partir des benchmarks publiés (Llama-3.2-1B : 18 tok/s, Qwen2.5-Coder-3B : 7.7 tok/s, DeepSeek-7B : 3.6 tok/s). Les performances Ollama CPU sont extrapolées des benchmarks llama.cpp sur Qwen3.5 / RK3588 (0.6B : 12 tok/s, 2B : 8.7 tok/s, 9B : 2.5 tok/s, 27B : 0.6 tok/s).

#### Recommandation : Assistant domotique

| Stratégie | Modèle | Pourquoi |
|-----------|--------|----------|
| **Proto (Ollama, test rapide)** | **Qwen3 1.7B Q4** ou **Llama 3.2 3B Q4** | Rapide à tester, tool calling natif, ~1-2 Go RAM, > 8 tok/s en CPU |
| **Production (RKLLM, NPU)** | **Qwen3 4B** ou **Phi-4 Mini 3.8B** | Meilleur ratio qualité/vitesse avec NPU, tool calling solide, < 3 Go RAM |
| **Si qualité insuffisante** | **Qwen3 8B Q4** | Plus intelligent mais ~5 Go RAM, ~3-5 tok/s — accepter la latence |

---

### 3.2 — Cas secondaire : Raisonneur / Chatbot conversationnel

Pour les conversations plus complexes (questions ouvertes, aide à la configuration, RAG sur la documentation).

| Modèle | Params | Quant. | RAM GGUF | Ollama CPU tok/s | Catégorie | Notes |
|--------|--------|--------|----------|-----------------|-----------|-------|
| **Qwen3 4B** | 4B | Q4_K_M | ~2.8 Go | ~5-8 | **OK** | Bon pour du Q&A, suffisant pour du RAG simple |
| **Gemma 4 E4B** | 4B | Q4_K_M | ~2.8 Go | ~5-8 | **OK** | Multilingue, bon en français |
| **Qwen3 8B** | 8B | Q4_K_M | ~5.0 Go | ~3-5 | **SERRÉ** | Nettement plus intelligent, mais lent |
| **Gemma 4 9B** | 9B | Q5_K_M | ~6.5 Go | ~2-3 | **SERRÉ** | Excellente qualité texte, lent |
| **Mistral Small 3.2 24B** | 24B | Q3_K_M | ~11 Go | ~0.5-1 | **TROP LOURD** | Qualité pro mais inutilisable interactif |

#### Recommandation : Chatbot

| Stratégie | Modèle | Pourquoi |
|-----------|--------|----------|
| **Proto** | **Qwen3 4B Q4** (même modèle que l'assistant) | Un seul modèle chargé = moins de RAM, plus simple |
| **Production (si RAM le permet)** | **Qwen3 8B Q4** avec swap à la demande | Charger quand l'utilisateur ouvre le chat, décharger après inactivité |

---

### 3.3 — Pipeline vocal : STT (Speech-to-Text)

| Modèle | Taille | RAM | Perf. CPU RK3588 | Perf. NPU RK3588 | Qualité | Catégorie |
|--------|--------|-----|-------------------|-------------------|---------|-----------|
| **Whisper tiny** | 75 Mo | ~150 Mo | ~5× temps réel | ~30× temps réel (useful-transformers) | Basique, anglais OK, français moyen | **LARGE** |
| **Whisper base** | 150 Mo | ~250 Mo | ~3× temps réel | ~15× temps réel | Correct multi-langue | **LARGE** |
| **Whisper small** | 500 Mo | ~600 Mo | ~1× temps réel | ~2× temps réel | Bon français | **OK** |
| **Whisper medium** | 1.5 Go | ~2 Go | ~0.3× temps réel | Non testé | Excellent français | **SERRÉ** |
| **Whisper large-v3** | 3 Go | ~4 Go | ~0.1× temps réel | Non supporté | État de l'art | **TROP LOURD** |

> **×N temps réel** = traite N secondes d'audio par seconde réelle. Il faut > 1× pour du streaming.

#### Recommandation : STT

| Stratégie | Modèle | Pourquoi |
|-----------|--------|----------|
| **Proto** | **Whisper base** via whisper.cpp | ~250 Mo RAM, qualité correcte, facile à déployer |
| **Production** | **Whisper small** via useful-transformers (NPU) | Meilleure qualité français, ~2× temps réel avec NPU |

---

### 3.4 — Pipeline vocal : TTS (Text-to-Speech)

| Modèle | RAM | Vitesse sur ARM | Qualité | Catégorie |
|--------|-----|-----------------|---------|-----------|
| **Piper TTS (small voice)** | ~30-50 Mo | > 10× temps réel | Synthétique mais clair | **LARGE** |
| **Piper TTS (medium voice)** | ~50-80 Mo | > 5× temps réel | Bon, voix naturelle | **LARGE** |
| **Piper TTS (large voice)** | ~100-150 Mo | > 2× temps réel | Très naturel | **OK** |

#### Recommandation : TTS

**Piper TTS medium** (voix française `fr_FR-siwis-medium`) — ~60 Mo RAM, rapide, qualité correcte. Aucune raison de chercher plus lourd.

---

## 4. Stratégie « deux profils, un seul runtime »

Faire tourner deux LLM **simultanément** est irréaliste sur 32 Go avec k3s + tous les services. La bonne stratégie :

```
┌────────────────────────────────────────────┐
│             Ollama / RKLLM Server          │
│                                            │
│  Profil A : "sky-fast"                     │
│  → Qwen3 1.7B Q4 (ou Phi-4 Mini 3.8B)    │
│  → Chargé par défaut                       │
│  → Commandes vocales, tool calling         │
│  → Latence cible : < 2s pour 50 tokens     │
│                                            │
│  Profil B : "sky-think"                    │
│  → Qwen3 8B Q4 (ou Gemma 4 9B)            │
│  → Chargé à la demande                     │
│  → Chat conversationnel, RAG, aide config  │
│  → Déchargé après 5 min d'inactivité       │
│                                            │
│  ⚠ Un seul modèle chargé à la fois         │
│  ⚠ Swap = ~5-15s de latence au changement  │
└────────────────────────────────────────────┘
```

Avec Ollama, le paramètre `OLLAMA_KEEP_ALIVE` contrôle la durée de rétention en mémoire. On peut aussi utiliser deux tags du même serveur.

---

## 5. Budget RAM réaliste

### Estimation consommation k3s + services (hors LLM)

| Service | RAM estimée |
|---------|-------------|
| Linux (Armbian) + kernel + drivers | ~500 Mo |
| k3s server (control plane) | ~300-500 Mo |
| Orbit (Laravel + PHP-FPM + nginx) | ~200-400 Mo |
| Base de données (PostgreSQL ou SQLite) | ~100-200 Mo |
| Redis (cache + queues) | ~50-100 Mo |
| Laravel Reverb (WebSocket) | ~50-100 Mo |
| Queue worker(s) | ~100-200 Mo |
| MQTT broker (Mosquitto) | ~20-30 Mo |
| Device service (Go/Rust/Python) | ~50-150 Mo |
| Whisper STT (small, chargé à la demande) | ~600 Mo |
| Piper TTS | ~60 Mo |
| Monitoring (Prometheus + node-exporter) | ~100-200 Mo |
| **Sous-total (sans LLM)** | **~2.1–2.9 Go** |

### RAM libre pour le LLM

| Scénario | RAM libre | Modèle max confortable | Catégorie |
|----------|-----------|----------------------|-----------|
| **Minimal** (SQLite, pas de monitoring, STT à la demande) | ~28-29 Go | Qwen3 8B Q4 (~5 Go) → **LARGE** | Beaucoup de marge |
| **Standard** (PostgreSQL, Redis, monitoring, STT résident) | ~25-27 Go | Qwen3 8B Q4 (~5 Go) → **LARGE** | Marge confortable |
| **Chargé** (tout actif + STT + TTS + Profil B chargé) | ~20-22 Go | Qwen3 8B Q4 (~5 Go) → **OK** | Ça passe encore |
| **Pire cas** (swap profil A→B, STT actif) | ~18-20 Go | Gemma 9B Q4 (~5.5 Go) + Whisper small (~0.6 Go) → **SERRÉ** | Attention au swap |

**Conclusion** : avec 32 Go, le LLM n'est **pas** le goulot d'étranglement mémoire. C'est la **vitesse CPU/NPU** qui limite. Même un 8B tient en RAM, mais sera lent en CPU (~3-5 tok/s). La NPU (RKLLM) change la donne pour les modèles ≤ 4B.

---

## 6. Matrice décisionnelle finale

### Proto V1 (Ollama sur CPU, test fonctionnel)

| Rôle | Modèle | RAM | Vitesse estimée | Verdict |
|------|--------|-----|-----------------|---------|
| Assistant tool calling | **Qwen3 1.7B Q4_K_M** | ~1.2 Go | ~10-15 tok/s | Rapide, test du pipeline complet |
| Chatbot (si testé) | Même modèle | — | — | Un seul modèle simplifie le proto |
| STT | **Whisper base** (whisper.cpp) | ~250 Mo | ~3× temps réel | Suffisant pour valider |
| TTS | **Piper medium fr** | ~60 Mo | > 5× temps réel | Impeccable |

### Proto V1.5 (Ollama, montée en qualité)

| Rôle | Modèle | RAM | Vitesse estimée | Verdict |
|------|--------|-----|-----------------|---------|
| Assistant tool calling | **Qwen3 4B Q4_K_M** ou **Phi-4 Mini 3.8B Q4** | ~2.5-2.8 Go | ~5-8 tok/s | Meilleur raisonnement, tool calling fiable |
| Chatbot | **Qwen3 8B Q4_K_M** (swap) | ~5.0 Go | ~3-5 tok/s | Accepter la latence pour le chat |
| STT | **Whisper small** (whisper.cpp) | ~600 Mo | ~1× temps réel | Meilleur français |
| TTS | **Piper medium fr** | ~60 Mo | > 5× temps réel | Inchangé |

### Production (RKLLM NPU)

| Rôle | Modèle | RAM | Vitesse estimée | Verdict |
|------|--------|-----|-----------------|---------|
| Assistant tool calling | **Qwen3 4B W8A8** (RKLLM) | ~2.3 Go | ~5-8 tok/s NPU | Rapide + intelligent |
| Chatbot | **Qwen3 8B W8A8** (RKLLM, swap) | ~4.2 Go | ~3-5 tok/s NPU | Correct pour du conversationnel |
| STT | **Whisper small** (useful-transformers NPU) | ~600 Mo | ~2× temps réel | Exploite la NPU |
| TTS | **Piper medium fr** | ~60 Mo | > 5× temps réel | CPU suffit |

---

## 7. Ce qu'il faut tester en premier sur l'OPi5+

Quand tu reçois l'Orange Pi 5 Plus, voici la séquence de tests recommandée :

### Test 1 — Ollama baseline (30 min)
```bash
# Installer Ollama
curl -fsSL https://ollama.com/install.sh | sh

# Tester le modèle le plus petit
ollama run qwen3:1.7b "Ferme les stores du salon. Réponds en JSON avec un tool call."

# Mesurer
ollama run qwen3:1.7b --verbose  # affiche tok/s
```

### Test 2 — Montée en taille (30 min)
```bash
ollama run qwen3:4b "Même prompt..."
ollama run qwen3:8b "Même prompt..."
# → Noter le tok/s et la RAM (htop dans un second terminal)
```

### Test 3 — STT (20 min)
```bash
# Compiler whisper.cpp
git clone https://github.com/ggerganov/whisper.cpp
cd whisper.cpp && make -j8
bash models/download-ggml-model.sh base
# Tester avec un fichier audio
./main -m models/ggml-base.bin -f samples/jfk.wav
```

### Test 4 — RKLLM NPU (si temps)
```bash
# Nécessite les drivers NPU installés
# Voir https://github.com/Pelochus/ezrknn-llm pour un setup simplifié
```

### Test 5 — Tout ensemble (stress test)
```bash
# Lancer Ollama + whisper + piper en même temps
# Mesurer la RAM totale (free -h) et les performances
```

---

## 8. Modèles à ne PAS utiliser (et pourquoi)

| Modèle | Pourquoi l'éviter |
|--------|-------------------|
| **Mistral Small 3.2 24B** | 14 Go Q4, < 1 tok/s en CPU, inutilisable interactif sur OPi5+ |
| **Gemma 4 27B** | 16 Go Q4, même problème |
| **Gemma 4 26B MoE** | 15 Go Q4, les MoE ne profitent pas de la NPU RK3588 |
| **Llama 3.1 70B** | Hors question (42 Go Q4) |
| **Tout modèle > 12B** | La vitesse CPU sera < 2 tok/s, la NPU ne supporte pas ces tailles |
| **Modèles vision (LLaVA, Gemma 4 vision)** | Pas de cas d'usage immédiat + surcoût RAM/vitesse |

---

## 9. Évolution V2 (Turing Pi 2.5, 4× RK1)

Pour référence, avec le cluster V2 (128 Go RAM, 4 NPU) :

| Config | Modèle accessible | Vitesse estimée |
|--------|-------------------|-----------------|
| 1 nœud 32 Go (master) | Qwen3 8B RKLLM | ~3-5 tok/s NPU |
| 4 nœuds distribués (128 Go) | **Mistral Small 3.2 24B** distribué (exllama/vLLM) | ~2-4 tok/s agrégé |
| 4 nœuds distribués (128 Go) | **Gemma 4 26B MoE** | ~3-5 tok/s (MoE bénéficie du distribué) |

Le passage V1 → V2 ne change pas l'architecture logicielle (Ollama/RKLLM derrière une API OpenAI-compatible), seulement la taille des modèles accessibles.
