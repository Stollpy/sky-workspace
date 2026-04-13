# Phase 2 — Assemblage Prototype

## Objectif

Câbler les modules breakout (HR/SpO2, IMU, LTE) sur la **T-Watch S3 Plus** pour obtenir un banc de développement firmware fonctionnel.

---

## 1. Bus I2C disponible sur la T-Watch S3 Plus

Le bus I2C principal (Wire) est sur **GPIO 10 (SDA)** / **GPIO 11 (SCL)**.
Appareils déjà présents :

| Appareil | Adresse I2C |
|----------|-------------|
| BMA423 (accéléro) | 0x19 |
| AXP2101 (PMU) | 0x34 |
| PCF8563 (RTC) | 0x51 |
| DRV2605 (haptique) | 0x5A |

**Adresses libres** sur ce bus : 0x57 (MAX30102), 0x68/0x69 (MPU-6050 / BMI270) → **pas de conflit**.

> Le touch (FT6336U, 0x38) est sur un bus I2C **séparé** (Wire1, GPIO 39/40).

---

## 2. Schéma de câblage proto

```
T-Watch S3 Plus
  GPIO 10 (SDA) ──────┬──────────┬──────────── MAX30102 SDA
  GPIO 11 (SCL) ──────┼──────────┼──────────── MAX30102 SCL
                      │          │
                      ├──────────┼──────────── MPU-6050 SDA
                      │          ├──────────── MPU-6050 SCL
                      │          │
                3.3V ─┼──────────┼──────────── VCC breakouts
                 GND ─┴──────────┴──────────── GND breakouts

  GPIO libre (ex. 6?) ──── UART TX ──── SIM7080G RX
  GPIO libre (ex. 43?) ─── UART RX ──── SIM7080G TX
                   3.3V ───────────────── SIM7080G VCC (vérifier régulateur)
                    GND ───────────────── SIM7080G GND
```

### Points d'attention

1. **Pull-ups I2C** : les breakouts Adafruit/SparkFun ont déjà des pull-ups 10 kΩ. Avec 3 devices sur le bus, la résistance parallèle est ok (~3.3 kΩ effective). Si instable, retirer les pull-ups des breakouts et mettre un seul couple 4.7 kΩ.

2. **GPIO pour UART LTE** : la T-Watch S3 Plus utilise presque tous les GPIO. Vérifie les GPIO réellement libres en consultant le pinout (certains GPIO comme 6, 43 peuvent être disponibles selon la version du PCB). En dernier recours, utilise un **USB-UART externe** ou un second ESP32 dédié LTE.

3. **Alimentation SIM7080G** : le module LTE demande des **pics de courant ~2A** en émission. Le rail 3.3 V de l'AXP2101 ne suffira pas. Alimenter le module LTE via une **alimentation USB externe** ou un **régulateur dédié** pendant le prototypage.

4. **Fil au poignet** : pendant le proto, la montre sera attachée à une breadboard avec des fils. L'objectif est uniquement de **valider les drivers** et la **stack logicielle**, pas la forme finale.

---

## 3. Checklist assemblage

- [ ] T-Watch S3 Plus reçue, firmware factory vérifié fonctionnel
- [ ] Breakout MAX30102 soudé (headers) et câblé I2C
- [ ] Scan I2C confirme 0x57 visible (en plus des devices existants)
- [ ] Breakout MPU-6050 / BMI270 câblé I2C
- [ ] Scan I2C confirme 0x68 visible
- [ ] Module SIM7080G câblé UART + alimentation séparée
- [ ] Commande AT basique (`AT\r\n` → `OK`) confirmée sur le module LTE
- [ ] Tous les capteurs existants de la T-Watch toujours fonctionnels (pas de régression bus I2C)

---

## 4. Script de validation I2C

```c
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "i2c_scan";

void i2c_scan(i2c_port_t port)
{
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Device found at 0x%02X", addr);
        }
    }
}
```

Résultat attendu après assemblage :

```
I (xxx) i2c_scan: Device found at 0x19   ← BMA423
I (xxx) i2c_scan: Device found at 0x34   ← AXP2101
I (xxx) i2c_scan: Device found at 0x51   ← PCF8563
I (xxx) i2c_scan: Device found at 0x57   ← MAX30102 ✅ (nouveau)
I (xxx) i2c_scan: Device found at 0x5A   ← DRV2605
I (xxx) i2c_scan: Device found at 0x68   ← MPU-6050 ✅ (nouveau)
```
