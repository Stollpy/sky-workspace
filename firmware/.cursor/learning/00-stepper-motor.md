# Moteur pas-à-pas (Stepper Motor) — 28BYJ-48 + ULN2003

## Principe de base

Un stepper motor tourne par **incréments fixes** (pas/steps) plutôt qu'en rotation continue. Chaque impulsion électrique fait tourner le rotor d'un angle précis. En contrôlant le nombre d'impulsions, on contrôle la position exacte de l'arbre.

## Anatomie du 28BYJ-48

```
                  Rotor (aimant permanent)
                        ↓
            ┌───────────────────────┐
            │    N ←── aimant ──→ S │   ← tourne par pas
            └───────────────────────┘
                        ↑
      Bobines (stator) autour du rotor
      activées séquentiellement pour
      "tirer" l'aimant d'un cran
```

Le 28BYJ-48 est un moteur **unipolaire** à 4 bobines avec un **réducteur interne** :

- **5 fils** : 4 bobines + 1 commun (VCC)
- **Réducteur** : ratio ~1:64 → l'arbre de sortie tourne 64x plus lentement que le rotor interne
- **Pas par tour** : 4096 half-steps = 1 tour complet de l'arbre de sortie
- **Angle par half-step** : 360° / 4096 = **0.088°**

## Câblage des bobines

Le moteur a 2 bobines, chacune avec un point central (commun = fil rouge) :

```
        Fil Rouge (VCC commun)
           ┌────┴────┐
           │         │
     Bobine A    Bobine B
      ┌──┴──┐    ┌──┴──┐
      │     │    │     │
    IN1   IN3  IN2   IN4
   (Bleu)(Jaune)(Rose)(Orange)
```

**Point crucial** : IN1 et IN3 sont les deux extrémités de la **même** bobine A. IN2 et IN4 sont les deux extrémités de la **même** bobine B. Les bobines adjacentes dans la séquence de stepping ne sont **pas** IN1→IN2 mais IN1→IN3→IN2→IN4.

## Le driver ULN2003

Le ULN2003 est un array de 7 transistors Darlington. Chaque entrée (IN1-IN4) contrôle un transistor qui connecte le fil de la bobine à la masse (GND), ce qui fait circuler le courant depuis VCC (fil rouge) à travers la bobine.

```
    ESP32 GPIO ──→ ULN2003 IN ──→ Transistor ──→ Bobine ──→ GND
                                                     ↑
                                                 VCC (5V)
```

L'ESP32 (3.3V) ne peut pas alimenter directement les bobines (5V, ~240mA). Le ULN2003 fait office d'amplificateur de courant.

## Modes de stepping

### Wave Drive (1 phase)
Une seule bobine active à la fois. Couple faible.

```
Step  IN1  IN2  IN3  IN4
  1    1    0    0    0   ← Bobine A+ seule
  2    0    0    1    0   ← Bobine A- seule
  3    0    1    0    0   ← Bobine B+ seule
  4    0    0    0    1   ← Bobine B- seule
```

### Full Step (2 phases)
Deux bobines actives simultanément. ~40% plus de couple.

```
Step  IN1  IN2  IN3  IN4
  1    1    0    1    0   ← A+ et A-
  2    0    1    1    0   ← B+ et A-
  3    0    1    0    1   ← B+ et B-
  4    1    0    0    1   ← A+ et B-
```

### Half Step (8 phases) — notre choix
Alterne entre 1 et 2 bobines. Double la résolution (4096 steps/tour). Rotation la plus fluide.

```
Step  IN1  IN2  IN3  IN4  Bobines actives
  1    1    0    0    0    A+
  2    1    0    1    0    A+ A-
  3    0    0    1    0       A-
  4    0    1    1    0    B+ A-
  5    0    1    0    0    B+
  6    0    1    0    1    B+ B-
  7    0    0    0    1       B-
  8    1    0    0    1    A+ B-
```

## Timing

Le 28BYJ-48 a une vitesse maximale d'environ 500-600 PPS (pulses per second) avec charge, soit un délai minimum de ~2ms entre steps.

**Piège FreeRTOS** : `vTaskDelay(pdMS_TO_TICKS(2))` avec un tick de 10ms (100Hz par défaut) donne **0 ticks = pas de délai**. Pour des intervalles < 10ms, utiliser `ets_delay_us()` (délai bloquant en microsecondes).

## Calculs utiles

| Besoin | Calcul |
|--------|--------|
| 1 tour complet | 4096 half-steps |
| Quart de tour | 1024 half-steps |
| Durée d'un tour à 2ms/step | 4096 × 2ms = **8.2 secondes** |
| Angle par half-step | 0.088° |
| Steps pour X degrés | X / 0.088 |

## Erreurs courantes

1. **Séquence IN1→IN2→IN3→IN4** : faux ! L'ordre physique est IN1→IN3→IN2→IN4 (suivre les bobines, pas les numéros)
2. **Délai trop court** : `vTaskDelay` arrondit au tick inférieur → 0 = pas de délai
3. **Ne pas couper les bobines** : laisser les bobines alimentées au repos chauffe le moteur et le ULN2003
4. **Un seul step isolé** : invisible (0.088°), toujours faire un burst de steps
5. **Alimenter via l'ESP32** : les bobines tirent ~240mA, utiliser une alimentation 5V séparée
