# Sky Hub — état courant

> Rafraîchi : `2026-04-18T01:01:07Z` — via `ssh sky-hub`
> Source : `scripts/sky-hub-refresh.sh`

> Le hardware réel varie (RPi4B dev / OPi5+ prod / KiwiPi alt / custom futur).
> Toujours lire la section `model` avant d'assumer quoi que ce soit.

## os
```
PRETTY_NAME="Debian GNU/Linux 13 (trixie)"
VERSION_ID="13"
ID=debian
```

## kernel
```
Linux sky-hub 6.12.75+rpt-rpi-v8 #1 SMP PREEMPT Debian 1:6.12.75-1+rpt1 (2026-03-11) aarch64 GNU/Linux
```

## model
```
Raspberry Pi 4 Model B Rev 1.2
```

## cpu
```
Architecture:                            aarch64
CPU(s):                                  4
Model name:                              Cortex-A72
```

## mem
```
               total        used        free      shared  buff/cache   available
Mem:           1.8Gi       390Mi       384Mi       116Mi       1.3Gi       1.4Gi
Swap:          1.8Gi       302Mi       1.5Gi
```

## disk
```
/dev/mmcblk0p2   29G  6.4G   22G  24% /
```

## host
```
sky-hub
 Static hostname: sky-hub
      Machine ID: 2ad08784e6d04dc9a45989eeb8b84bd0
         Boot ID: b0cd9f4c540f4d1f9f5943cf7823ee3b
Operating System: Debian GNU/Linux 13 (trixie)
          Kernel: Linux 6.12.75+rpt-rpi-v8
    Architecture: arm64
```

## net
```
eth0             UP             192.168.2.24/24 
```

## uart
```
lrwxrwxrwx 1 root root          7 Apr 14 13:39 /dev/serial0 -> ttyAMA0
crw-rw---- 1 root dialout 204, 64 Apr 17 20:55 /dev/ttyAMA0
crw-rw---- 1 root dialout   4, 64 Apr 14 13:39 /dev/ttyS0
aucun device série classique trouvé
```

## boot-uart-cfg
```
dtoverlay=vc4-kms-v3d
dtoverlay=dwc2,dr_mode=host
enable_uart=1
dtoverlay=disable-bt
```

## services
```
avahi-daemon.service
device-service.service
nginx.service
orbit-reverb.service
orbit-worker.service
php8.4-fpm.service
postgresql@17-main.service
redis-server.service
```

## k3s-pods
```
kubectl: non installé
```

## checkout
```
```

## device-service-bin
```
-rwxrwxr-x 2 sky sky 4280448 Apr 16 01:26 /home/sky/device-service-src/target/release/device-service
```
