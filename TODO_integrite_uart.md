# ~~TODO — Intégrité signal UART RPi4B ↔ ESP32~~ → RÉSOLU (2026-04-17)

> **Issue fermée.** Le fichier garde son nom historique pour ne pas casser les références ; le vrai root cause n'était **pas** l'intégrité signal mais une **table CRC16 corrompue** dans `sky-framework`.
>
> **Voir** : `docs/logiciel/developpement/troubleshooting-sky-serial-crc.md` (post-mortem complet + self-test proposé + méthode de diagnostic).

## Résumé ultra-court

- **Symptôme** : 100 % CRC errors sur `sky_serial` → 0 commande Orbit → Luxia ne bouge jamais.
- **Cause** : `sky-framework/components/sky_serial/src/sky_serial_crc.c` avait 64 entrées sur 256 corrompues dans la lookup table CRC16-CCITT-FALSE (indices `0x40`-`0x7F`).
- **Fix** : régénération mathématique de la table, 8 lignes remplacées. `sky_serial_crc16(b"123456789") == 0x29B1` ✓.
- **Validation** : après rebuild + flash, ACKs remontent, 0 NAK, commandes Orbit → Luxia fonctionnelles.

## Changements résiduels à décider

Pendant l'investigation, la baud UART a été baissée **115200 → 57600** des 2 côtés (firmware + hub `.env`). Ce n'était **pas** le fix — la vitesse n'était pas le problème. Options :

### Option A — Rester à 57600 (conservateur)
Rien à faire. Latence négligeable pour nos commandes (12 octets toutes les quelques secondes).

### Option B — Revenir à 115200 (standard, recommandé pour prod)
```bash
# Firmware (sky-hub-antenna-wroom, local)
cd sky-hub-antenna-wroom
sed -i.bak 's/^CONFIG_SKY_SERIAL_BAUD_RATE=.*/CONFIG_SKY_SERIAL_BAUD_RATE=115200/' sdkconfig sdkconfig.esp32-wroom sdkconfig.defaults
idf.py fullclean && idf.py build && idf_flash_and_monitor

# Hub
ssh sky-hub 'echo pwdno | sudo -S sed -i "s/^DS_SERIAL_BAUD_RATE=.*/DS_SERIAL_BAUD_RATE=115200/" /opt/sky-hub/device-service/.env'
ssh sky-hub 'echo pwdno | sudo -S systemctl restart device-service'
ssh sky-hub 'echo pwdno | sudo -S journalctl -u device-service --since "1 min ago" --no-pager | grep baud_rate'
```

## Fichiers à commit

Le fix code est dans le submodule `sky-framework`. Commit suggéré :

```
cd sky-framework
git add components/sky_serial/src/sky_serial_crc.c
git commit -m "fix(sky_serial): correct 64 corrupted CRC16-CCITT-FALSE table entries

The lookup table in sky_serial_crc.c had 64 incorrect entries in the
range [0x40..0x7F], causing sky_serial_crc16() to produce wrong CRCs
(sky_serial_crc16(\"123456789\") returned 0x857F instead of 0x29B1).

Result: 100% of frames from any correct CCITT-FALSE emitter (Rust
device-service, Python test bench) were rejected with CRC mismatch,
blocking all Orbit → ESP32 command flow.

Regenerated the table mathematically and replaced the 8 affected lines.
sky_serial_crc16 now matches the standard CCITT-FALSE check value 0x29B1.

See docs/logiciel/developpement/troubleshooting-sky-serial-crc.md for
the full post-mortem, repro steps, and a proposed boot-time self-test
to prevent silent regression."
git tag v0.2.1  # ou bump approprié
```

Ensuite bumper la ref du submodule `sky-framework` dans le monorepo racine et dans chaque firmware consommateur qui pinning une version spécifique (aujourd'hui : `sky-hub-antenna-wroom`, `sky-hub-antenna`).

Ce fichier peut être supprimé une fois les décisions A/B et le commit faits.
