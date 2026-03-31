<user-query>
# CONTEXT
L'idée c'est de créer un firmware pour "controller" des stores / rideaux.
On va avoir un stepper motor @.cursor/learning/00-stepper-motor.md qui permettra de faire tourner une pièce pour faire tourner la chaine des stores.
Il sera alimenter par une batterie externe via des magnetic connector.
Le firmware devra être capable de gérer les cas suivant:
- L'implémentation du protocole de communication Matter via ESP-Matter SDK
- Recevoir des appels pour lancer / arrêter le moteur
- Stater le moteur et lancer l'enregistrement d'une collection de "half_step" et les renvoyers au client pour enregistrer des patterns
- Une initialisation pour déterminer le nombre de tour pour par courrir le store

Je veux que une architecture bien précise, ce firmware est le premier d'une longue ligné de composant domotique pour ma maison, je veux donc que tout soit réutilisable.

Créer des "packages" avec la solution proposer par espidf pour créer l'implémentation du protocole matter dans notre "infra".

Créer un package pour l'implémentation du stepper motor, je veux pouvoir en "enregistrer" un dans un firmware rapidement via ce package. Je veux m'inspiré de l'init de Fortify dans Laravel. Je veux un pattern similaire pour donner une configuration pour init un stepper motor rapidement et interagire avec.
</user-query>
<ask>
1. Quel design pattern compte tu utilisé ?
2. Comprends tu l'idée du projet final ? (HomeAssistant, Domotique maison, ect ...)
3. As-tu des conseils à me donner ?
</ask>
