Trame (Frame) et paquet (packet) sont la même chose pour moi (truss). 

# Scénario complet : Émission d'un message

## 1. Détection de l'appui sur le bouton (`main.cpp`)

- Le GPIO0 détecte qu'on a appuyer sur le bouton
- L'interruption `buttonISR()` est déclenchée.
- L'ISR wake up simplement `uartTxTask` avec `vTaskNotifyGiveFromISR()`.

## 2. Réveil de la tâche de transmission (`tx_task.cpp`)

- `uartTxTask` est débloquée par `ulTaskNotifyTake()`.
- On tombe dans `sendButtonMessage()`, donc on prépare le message + appel de `protocolSendMessage()` situé dans `protocol.cpp`.

## 3. Construction des trames (`protocol.cpp`)

`protocolSendMessage()` est responsable le giga chad qui s'occupe de la "liaison de données". En gros, il fait ça:


1. Calcul du nombre de trames **DATA** nécessaires en fonction de la taille du message (80 octets maximum par trame).
2. Construit la trame **START** contenant le nombre total de paquets à transmettre.
3. Découpage du message en plusieurs trames **DATA** si nécessaire.
4. Attribut un numéro de séquence à chaque trame DATA.
5. Calcul du CRC16 de chaque trame (j'ai pogné une librairie truss).
6. Construit la trame **END** pour marquer la fin de la transmission.
7. Ajout de chaque trame dans :
   - la file d'attente d'émission **TxPendingQueue** ;
   - le **TxHistoryQueue**, permettant une retransmission ultérieure en cas de réception d'un NACK. J'ai pas encore trop faite cette partie là.

À la sortie de cette fonction, l'ensemble des trames sont prêtes à être transmise.

## 4. Transmission des trames (`tx_task.cpp`)

- `uartTxTask` unqueue une a la suite des autres les trames présentes dans la **TxPendingQueue**.
- Pour chaque trame, elle appelle `sendFrame()`.

## 5. Émission UART (`send_frame.cpp`)

`sendFrame()` envoie la structure `Frame` en envoyant un a la suite de l'autre :

1. le préambule ;
2. le START ;
3. l'entête ;
4. le payload ;
5. le CRC16 ;
6. le END.

Les bytes sont transmis sur `Serial1` (qui est présentement en loopback, donc c'est le même esp32 qui reçoit le data).

# Scénario complet : Réception d'un message

## 1. Attente d'une nouvelle trame (`rx_task.cpp`)

- `uartRxTask` attend continuellement des données provenant de `Serial1` (tant que `readByteBlocking()` retourne false, c'est qui ce passe rien sur le UART).
- Dès que du data est détectée, elle appelle `receivedFrame()`.

## 2. Reconstruction de la trame (`receive_frame.cpp`)

`receivedFrame()` lit les bytes reçus un à un pour reconstruire une `Frame`.

Les champs sont les même que ceux envoyés, soit:

1. le préambule ;
2. le START ;
3. l'entête ;
4. le payload ;
5. le CRC16 ;
6. le END.
Une fois la lecture terminée, la structure `Frame` complète est "retournée" à `uartRxTask`. (c'est pas un actual return, c'est qu'on update le pointeur)

## 3. Validation du protocole (`protocol.cpp`)

`uartRxTask` transmet ensuite la trame à `processFrame()`.

Cette fonction valide la logique de réception, soit :

- validation des champs de la trame ;
- vérification du CRC16 ;
- validation du numéro de séquence ;
- mise à jour du `ReceptionContext` ;
- détermination du résultat du traitement :
  - **ACCEPT** : la trame est valide ;
  - **REJECT** : la trame est invalide ;
  - **NACK** : une retransmission doit être demandée.

Le `ReceptionContext` est une structure permettant de suivre le traitement des trames (vérifier qu'elles sont dans le bonne ordre et tous présentes).   

Il y a aussi des prints qui sont faite à chaque réception de trame afin de pourvoir les analyser.
Voici un exemple des prints de debug, quand j'envoie le data "Appui bouton GPIO0":
```TEXT
send message

========== RX FRAME ==========
Type            : START
Sequence number : 0
Payload length  : 0
Parameter       : 3
CRC             : 0xFEA8
Expected frame  : 1
Expected total  : 3
Error           : COMM_OK
Result          : ACCEPT
==============================

========== RX FRAME ==========
Type            : DATA
Sequence number : 1
Payload length  : 18
Parameter       : 0
CRC             : 0xD679
Expected frame  : 2
Expected total  : 3
Error           : COMM_OK
Result          : ACCEPT
==============================

========== RX FRAME ==========
Type            : END
Sequence number : 2
Payload length  : 0
Parameter       : 0
CRC             : 0xFC02
Expected frame  : 3
Expected total  : 3
Error           : COMM_OK
Result          : ACCEPT
```

## 4. Traitement du résultat (`rx_task.cpp`)

Selon le résultat retourné par `processFrame()` :

- une trame valide (dont le résultat du traitement est **ACCEPT** est ajoutée au buffer de réception (RxFrameQueue) via `storeFrame()` ;
- un **NACK** peut être généré afin de demander la retransmission d'une trame ; **À FAIRE**
- une trame invalide est rejetée et aucune donnée n'est enregistrée.
Présentement, `RxFrameQueue` ne ce fait jamais vidé. En théorie, sa serait à l'application de le vidée pour utiliser les donnéss.
Il y a la méthode `getFrame()` qui permet de vidée `RxFrameQueue`. 
En théorie, les buffers devrait pas overflow, car si je me souviens bien, les queues FreeRTOS sont des queues circulaire. Donc truss
