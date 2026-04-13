# 00 — Blueprint.am → KiCad · Plan Général

## Contexte

Blueprint.am a généré une architecture système (BOM, connexions électriques/mécaniques, guide d'assemblage, rendu 3D) pour notre montre connectée. Ce plan décrit comment **transformer ce output en un vrai projet KiCad**, en corrigeant les erreurs identifiées.

## Fichiers source Blueprint.am

```
assets/
├── smartwatch_prototype_BOM.csv                    ← BOM (41 composants)
├── smartwatch_prototype_ELECTRICAL_CONNECTIONS.json ← Graphe connexions
├── smartwatch_prototype_MECHANICAL_CONNECTIONS.json ← Assemblage mécanique
├── smartwatch_prototype_CONFIG.json                ← Config complète (nodes, positions, pins)
├── smartwatch_prototype_GUIDE.md                   ← Guide assemblage (partiel)
└── smartwatch_prototype_VISUAL.png                 ← Rendu 3D
```

## Erreurs Blueprint.am à corriger AVANT KiCad

### Bloquantes

| # | Erreur | Correction |
|---|--------|------------|
| 1 | **UART0 assigné au SIM7080G** | UART0 = console/USB. Utiliser **UART2** (GPIO 16 TX / GPIO 17 RX) pour le LTE |
| 2 | **GPIO9 assigné au BOOT button** | GPIO9 = flash SPI interne. Utiliser **GPIO0** pour BOOT (strapping pin), POWER via **AXP2101 IRQ/PWRKEY** |
| 3 | **Aucun condensateur de découplage** | Ajouter 100 nF + 10 µF par IC (ESP32, AXP2101, BMI270, MAX30102, etc.) → ~20 condensateurs |
| 4 | **Pas d'inductance pour TPS62743** | Ajouter **inductance 2.2 µH** + résistance feedback + condo entrée/sortie |
| 5 | **Pas de matching réseau RF** | Ajouter réseau pi (L/C) pour antenne WiFi/BT ESP32 + lignes 50 Ω pour FPC RF |
| 6 | **1000 µF "céramique"** | Remplacer par **tantale ou polymère** (ex. Panasonic EEFSX0D471E4) |
| 7 | **CR1220 trop grosse (12.5 mm)** | Remplacer par **ML621** (6.8 mm) rechargeable ou supprimer si l'AXP2101 VBACKUP suffit |

### Mineures

| # | Erreur | Correction |
|---|--------|------------|
| 8 | TPS62743 appelé "LDO" | C'est un **DC-DC buck**. Mettre à jour la nomenclature |
| 9 | MAX98357A L_OUT/R_OUT | Renommer en **OUT+/OUT-** (sortie mono différentielle) |
| 10 | GPIO mapping incomplet | Assigner chaque signal à un GPIO réel ESP32-PICO-V3-02 |

## Phases du plan

| Fichier | Phase |
|---------|-------|
| `01-blueprint-to-kicad-1-project-setup.md` | Création projet KiCad, librairies, symboles |
| `02-blueprint-to-kicad-2-schematic.md` | Schéma complet avec GPIO mapping corrigé |
| `03-blueprint-to-kicad-3-pcb-layout.md` | PCB layout, stackup 4 couches, placement, RF |
| `04-blueprint-to-kicad-4-fabrication.md` | Export Gerbers, BOM finale, commande JLCPCB/PCBWay |
