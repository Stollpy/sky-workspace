## CONTEXT
Objectif : Créer le design d'un PCB compact pour un contrôleur de moteur pas à pas de store enrouleur, alimenté par batterie Li-Ion, avec un système de communication sans fil et un port de charge magnétique.

Architecture du Circuit (Schématique) :

Microcontrôleur (Le Cœur) : Intégrer un module ESP32-WROOM-32 (ou l'empreinte équivalente pour ton ESP32 spécifique). Prévoir l'accès aux pins de programmation (UART : TX, RX, GND, 3V3) via des "test points" ou une embase discrète.

Contrôle Moteur (Les Muscles) : Connecter les pins GPIO de l'ESP32 (par exemple GPIO 13, 12, 14, 27) aux entrées d'un circuit intégré driver ULN2003A (boîtier SOP-16 pour compacité). Les sorties de l'ULN2003A doivent aller vers un connecteur JST à 5 broches (JST-XH ou similaire) pour brancher le moteur 28BYJ-48.

Alimentation (L'Énergie) :

Un connecteur de batterie (JST-PH 2 broches) pour brancher une batterie Li-Ion 3.7V.

Un circuit de gestion de charge Li-Ion complet basé sur le contrôleur TP4056 (boîtier SOP-8). Inclure les résistances de programmation du courant de charge (ex: 1A) et les condensateurs de découplage nécessaires.

Connecter deux LEDs d'état de charge (ex: Rouge = En charge, Vert = Chargé) aux pins correspondants du TP4056.

Ajouter un régulateur de tension LDO 3.3V (ex: AP2112-3.3 ou AMS1117-3.3) pour alimenter l'ESP32 depuis la tension de la batterie (3.7V-4.2V).

Inclure un pont diviseur de tension (deux résistances) connecté à une pin ADC de l'ESP32 pour mesurer le voltage de la batterie.

Connecteur de Charge Magnétique (L'Innovation) : Intégrer une empreinte pour un connecteur magnétique à Pogo Pins 2 broches (ex: pas de 2.54mm ou spécifique selon le modèle choisi). Connecter la pin "VCC" de ce connecteur à l'entrée "IN+" du TP4056 et la pin "GND" à la masse commune.

Contraintes de Placement (Layout) :

Forme et Taille : Le PCB doit être aussi compact que possible (ex: environ 40mm x 60mm), de forme rectangulaire, pour s'intégrer dans un boîtier mural étroit.

Moteur : Placer le connecteur JST du moteur près d'un bord du PCB pour faciliter le câblage vers l'axe central du boîtier mécanique.

Antenne : Placer le module ESP32 de manière à ce que son antenne PCB dépasse du bord du PCB ou ne soit pas recouverte par des plans de masse, pour maximiser la portée BLE/Wi-Fi.

Connecteur Magnétique : Positionner l'empreinte du connecteur magnétique à Pogo Pins sur le bord opposé au moteur, à un endroit accessible de l'extérieur du boîtier mécanique une fois assemblé.

LEDs : Placer les deux LEDs d'état de charge TP4056 près du connecteur magnétique pour qu'elles soient visibles de l'extérieur.

Fixation : Prévoir 2 ou 4 trous de montage (diamètre 2mm ou 3mm) dans les coins pour fixer le PCB à l'intérieur du boîtier imprimé en 3D.

Plans de Masse : Utiliser des plans de masse généreux sur les deux couches (Top et Bottom) pour une bonne dissipation thermique et intégrité du signal, en évitant la zone sous l'antenne de l'ESP32.

<ask>
1. Vois tu des contraites technique ? 
2. Vois tu des point bloquants ?
</ask>