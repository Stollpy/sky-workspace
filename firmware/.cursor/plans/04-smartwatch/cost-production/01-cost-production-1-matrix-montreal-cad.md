# Matrice de coûts — Distribution Montréal (Canada)

Tous les montants sont en **dollars canadiens (CAD)**. Hypothèse de conversion utilisée pour harmoniser les estimations issues des calculs en euros : **1 EUR ≈ 1,47 CAD** (ordre de grandeur ; le taux réel à la date de facturation peut différer).

## Périmètre

- **Marché** : vente au détail / distribution avec produit conforme au Canada (Québec / Montréal).
- **Inclus dans « coût total / unité » (séries)** : fabrication (BOM + PCB + assemblage + boîtier + packaging de base) + **certifications Canada** (ISED, EMC, SAR, sécurité électrique — bundle indicatif) + **traduction FR** (emballage / notice) + **fret import** + **droits de douane / TPS à l’import** (ordre de grandeur).
- **Non inclus** : marge distributeur, Amazon, marketing, SAV, garantie, marge entreprise, taxes de vente (TPS/TVQ) facturées au client final.

## Bundle réglementaire Canada (référence unique)

Pour comparer les configurations sur une base équivalente, les séries utilisent le **même ordre de grandeur de paquet réglementaire** adapté à un wearable **Wi‑Fi + Bluetooth + batterie** au Canada :

| Poste | Ordre de grandeur (CAD) |
|-------|-------------------------|
| ISED (radio Wi‑Fi / BT) + dossier REL | 4 000 – 7 000 |
| ICES-003 (CEM) | 2 000 – 3 500 |
| RSS-102 (SAR — port au poignet) | 3 500 – 5 000 |
| Sécurité (CSA / cUL — LiPo, charge) | 3 000 – 5 000 |
| **Total indicatif** | **~12 500 – 20 500 CAD** |

Dans les tableaux ci‑dessous, pour les **totaux série**, on amortit **un seul bloc** de **~15 000 CAD** de tests + dossier (milieu de fourchette), sauf mention contraire.

> **LTE** : si le produit inclut un modem cellulaire, ajouter tests / exigences **supplémentaires** (bandes cellulaires, opérateur, SAR spécifique) — **non détaillé ligne par ligne ici** ; la ligne « Complet (LTE + GPS) » reste une **estimation haute** basée sur nos échanges antérieurs.

---

## 1. Proto — matériel seulement (1 à 5 exemplaires)

Pas d’amortissement des certifications (le proto sert au développement ; la conformité s’applique au design de série figé).

| Configuration | Coût matériel estimé (CAD) | Commentaire |
|---------------|---------------------------|-------------|
| **Complet (LTE + GPS + capteurs + écran + audio + haptique)** | **~185** | BOM + PCB petit tirage + boîtier proto |
| **Sans LTE** | **~150** | Retrait modem, eSIM, buck LTE, antenne LTE, gros bulk associé |
| **Sans GPS** (LTE conservé) | **~165** | Retrait MIA-M10Q + antenne FPC GPS + connecteur RF GPS |
| **Sans LTE + sans GPS** | **~130** | Configuration « stack » la plus légère côté radio embarquée |

**Fourchette outillage** (une fois, si tu pars de zéro) : **~150 – 230 CAD** (station air chaud, pâte, loupe, etc.) — non inclus dans le tableau.

---

## 2. Série 300 unités — coût total amorti par unité (Montréal)

Inclut : fabrication + **certifications Canada (~15 kCAD amortis)** + traduction + fret + douane/TPS import (estimés).

| Configuration | Coût total / unité (CAD) | Investissement total approximatif (CAD) |
|---------------|--------------------------|----------------------------------------|
| **Complet (LTE + GPS)** | **~195 – 205** | **~59 000 – 62 000** |
| **Sans LTE** (+ GPS) | **~150 – 155** | **~45 000 – 47 000** |
| **Sans GPS** (+ LTE) | **~175 – 185** | **~53 000 – 56 000** |
| **Sans LTE + sans GPS** | **~135 – 140** | **~40 500 – 42 000** |

**Décomposition indicative « Sans LTE + sans GPS » (300 u.)** — alignée sur nos calculs détaillés :

| Poste | CAD (total) |
|-------|-------------|
| Fabrication 300 × ~75 CAD/u | ~22 500 |
| Certifications (amorti) | ~15 000 |
| Traduction FR / packaging | ~750 |
| Fret | ~750 |
| Douane / TPS import | ~1 750 |
| **Total** | **~40 750** |
| **Par unité** | **~136** |

*(L’écart avec la fourchette 135–140 vient des arrondis ; garde **136 CAD/u** comme ordre de grandeur central.)*

---

## 3. Série 10 000 unités — coût total par unité (Montréal)

Inclut : BOM volume + PCB + SMT + tests + boîtier **injection** (moules amortis) + packaging + **certifications** + fret/douane/trad (ordre de grandeur).

| Configuration | Coût total / unité (CAD) | Investissement total approximatif (kCAD) |
|---------------|--------------------------|------------------------------------------|
| **Complet (LTE + GPS)** | **~115 – 125** | **~1 150 – 1 280** |
| **Sans LTE** (+ GPS) | **~95 – 100** | **~950 – 1 000** |
| **Sans GPS** (+ LTE) | **~105 – 115** | **~1 050 – 1 180** |
| **Sans LTE + sans GPS** | **~80 – 85** | **~800 – 870** |

**Ordre de grandeur central « Sans LTE + sans GPS » (10 k u.)** : **~82 CAD/unité** total amorti (aligné sur ~37,7 EUR/u + one-shots convertis).

---

## 4. Matrice synthèse — coût total / unité (CAD)

| Configuration | Proto (matériel) | 300 unités (total/u) | 10 000 unités (total/u) |
|---------------|------------------|----------------------|-------------------------|
| **Complet LTE + GPS** | ~185 | ~200 | ~120 |
| **Sans LTE** | ~150 | ~152 | ~97 |
| **Sans GPS** (LTE OK) | ~165 | ~180 | ~110 |
| **Sans LTE + sans GPS** | ~130 | **~136** | **~82** |

*(Arrondis « marketing » sur la ligne 10 k pour « Complet » et « Sans GPS seul » : fourchettes §3.)*

---

## 5. Sensibilité (à budgéter à part)

| Risque | Impact typique |
|--------|----------------|
| Retests labo (échec EMC/SAR) | +5 000 – 15 000 CAD par boucle |
| LTE (opérateur / PTCRB / variantes) | souvent **+10 000 – 50 000+ CAD** selon cible |
| Taux EUR/CAD, douanes, volume réel de commande | fait varier **±5 – 15 %** |
| Moules injection (10 k) déjà amortis dans §3 | premier tirage peut nécessiter **acompte moules** séparé |

---

## 6. Résumé exécutif

- **Le coût par unité à Montréal** dépend surtout du **volume**, puis de **LTE** (très coûteux en BOM + réglementation), puis du **GPS** (module + RF + antenne).
- La configuration **sans LTE et sans GPS** est celle qui se rapproche le plus d’une **montre BLE/Wi‑Fi + santé + notifs** : **~136 CAD/u** à 300 unités et **~82 CAD/u** à 10 k unités, avec les hypothèses ci‑dessus.
- Pour un produit **avec LTE** destiné au grand public au Canada, prévoir une **marge réglementaire** plus large que ce tableau « moyen ».
