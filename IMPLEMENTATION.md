# Tâche : BAD NRF (MouseJack) pour Bruce Firmware

## Objectif

Ajouter une fonctionnalité **MouseJack** dans le menu NRF24 de Bruce firmware pour le T-Embed CC1101 Plus permettant :

1. Scanner les claviers/souris wireless 2.4GHz (Logitech Unifying et autres)
2. Tester la vulnérabilité du device ciblé
3. Injecter des frappes clavier via scripts Ducky (même format que BadUSB Bruce)

---

## Phase 1 : Analyse et Définition (Mode Plan)

### 1.1 Analyse de l'architecture existante

- [ ] Examiner la structure du menu NRF24 actuel dans `src/core/menu_items/NRF24.cpp`
- [ ] Analyser les fonctions NRF24 existantes (`nrf_spectrum()`, `nrf_info()`, etc.)
- [ ] Étudier l'utilisation de la librairie RF24 v1.4.11 dans le projet
- [ ] Examiner le code BadUSB/BadBLE existant pour le parsing Ducky script
- [ ] Identifier le chemin de stockage des scripts Ducky (`/badusb/` ou similaire)

### 1.2 Analyse technique MouseJack

- [ ] Documenter le protocole Logitech Unifying :
  - Adressage 5 bytes (préfixe + suffixe)
  - Channel hopping pattern (canaux 2-83, pas de 3 ou 5)
  - Format des packets clavier (encrypted vs unencrypted)
  - Checksum CRC
- [ ] Identifier les dongles vulnérables :
  - Logitech Unifying (C-U0007, C-U0008 anciens)
  - Logitech non-Unifying (certains C-U0010)
  - Microsoft wireless
  - Autres génériques
- [ ] Définir la méthode de scan :
  - Mode promiscuous du nRF24L01+
  - Détection des preambles (0xAA, 0x55)
  - Identification du type de device

### 1.3 Configuration nRF24L01+ pour MouseJack

```cpp
// Configuration promiscuous pour scan
radio.setAutoAck(false);
radio.setPALevel(RF24_PA_MAX);
radio.setDataRate(RF24_2MBPS);  // Logitech utilise 2Mbps
radio.setPayloadSize(32);
radio.setCRCLength(RF24_CRC_DISABLED);  // Pour mode promiscuous
radio.setAddressWidth(2);  // Réduit pour capturer plus de packets
```

### 1.4 Spécifications fonctionnelles

#### Structure du menu (style Bruce)

```
NRF24
├── Spectrum        (existant)
├── Info            (existant)
├── Config          (existant)
└── BAD NRF         (nouveau)
    ├── Scan Devices
    ├── Saved Targets
    ├── Test Target
    └── Run Payload
```

#### Scan Devices
- Scanner les canaux 2.4GHz pour détecter les devices wireless
- Afficher liste avec :
  - Adresse MAC/ID du dongle
  - Type détecté (Logitech/Microsoft/Générique)
  - RSSI/Force du signal
  - Indicateur de vulnérabilité potentielle
- Option de sauvegarder la cible

#### Saved Targets
- Liste des cibles précédemment sauvegardées
- Fichiers JSON dans `/badnrf/targets/`
- Sélection pour test ou attaque

#### Test Target
- Envoyer une séquence de test non-destructive
- Options de test :
  - Shift x5 (trigger StickyKeys beep sur Windows)
  - Caps Lock toggle x2
  - Ou simplement vérifier la sync radio
- Afficher résultat : "Vulnérable" / "Non vulnérable" / "Timeout"

#### Run Payload
- Browser de fichiers pour scripts Ducky (`.txt`)
- Chemin : `/badnrf/payloads/` ou réutiliser `/badusb/`
- Parser le script Ducky (réutiliser le parser BadUSB existant)
- Injecter les frappes via radio

### 1.5 Format de fichier cible (JSON)

```json
{
  "version": "1.0",
  "name": "Bureau PC",
  "type": "logitech_unifying",
  "address": "BB:29:0A:F1:C8",
  "channel": 32,
  "data_rate": "2MBPS",
  "discovered_at": "2025-01-15T10:30:00Z",
  "last_tested": "2025-01-15T10:35:00Z",
  "vulnerable": true,
  "notes": "PC bureau 2ème étage"
}
```

### 1.6 Mapping Ducky Script → Logitech HID

- [ ] Documenter la conversion des commandes Ducky vers packets Logitech
- [ ] Gérer les modificateurs (CTRL, ALT, SHIFT, GUI)
- [ ] Gérer les délais (DELAY)
- [ ] Gérer les chaînes de caractères (STRING)
- [ ] Gérer les touches spéciales (ENTER, TAB, ESC, etc.)

### 1.7 Dépendances et contraintes

- [ ] Vérifier que RF24 1.4.11 supporte le mode promiscuous
- [ ] Estimer le temps de scan (channel hopping)
- [ ] Limites de portée du nRF24L01+ intégré au T-Embed
- [ ] Compatibilité avec le reste du firmware (pas de conflit SPI)

---

## Phase 2 : Implémentation Scan & Détection (Mode Build)

### 2.1 Création des fichiers source

- [ ] Créer `src/modules/nrf/bad_nrf.h`
- [ ] Créer `src/modules/nrf/bad_nrf.cpp`

### 2.2 Fonctionnalités du scanner

```cpp
// Structure pour device détecté
struct NRFDevice {
    uint8_t address[5];
    uint8_t channel;
    int8_t rssi;
    String type;  // "logitech", "microsoft", "generic"
    bool potentially_vulnerable;
    uint32_t last_seen;
};

// Fonctions principales
void badnrf_scan();           // Lance le scan
void badnrf_stop_scan();      // Arrête le scan
void badnrf_show_devices();   // Affiche les devices trouvés
void badnrf_save_target(NRFDevice* device);  // Sauvegarde une cible
```

### 2.3 Algorithme de scan

1. Configurer nRF24 en mode promiscuous
2. Pour chaque canal (2-83, pas de 3) :
   - Écouter pendant ~5ms
   - Capturer les packets
   - Analyser les préambules et adresses
3. Identifier le type de device via patterns connus
4. Stocker les devices uniques
5. Afficher en temps réel sur l'écran

### 2.4 Identification des devices

```cpp
// Patterns connus
const uint8_t LOGITECH_PREAMBLE[] = {0xAA, 0x00};
const uint8_t MS_PREAMBLE[] = {0x55, 0x00};

// Identifier via les premiers bytes du packet
DeviceType identifyDevice(uint8_t* packet, uint8_t len);
```

---

## Phase 3 : Implémentation Test & Injection (Mode Build)

### 3.1 Test de vulnérabilité

```cpp
bool badnrf_test_target(NRFDevice* target) {
    // 1. Configurer radio sur le canal de la cible
    // 2. Envoyer séquence de test (Shift x5)
    // 3. Attendre ACK ou observer timing
    // 4. Retourner résultat
}
```

### 3.2 Parser Ducky Script

- [ ] Réutiliser le parser existant de BadUSB (`lib/Bad_Usb_Lib/` ou similaire)
- [ ] Adapter pour sortie vers packets nRF au lieu de USB HID

### 3.3 Injection de frappes

```cpp
// Conversion touche → packet Logitech
void badnrf_send_keystroke(uint8_t key, uint8_t modifiers);

// Envoi d'une chaîne
void badnrf_send_string(const char* str);

// Exécution d'un script Ducky
void badnrf_run_script(const char* filepath);
```

### 3.4 Format packet Logitech Unifying (non-chiffré)

```
Byte 0: Device type (0x00 = keepalive, 0x04 = keyboard)
Byte 1: Key modifiers (Ctrl, Shift, Alt, GUI)
Byte 2: Reserved (0x00)
Byte 3-8: Key codes (up to 6 keys)
Byte 9: Checksum
```

---

## Phase 4 : Intégration Menu (Mode Build)

### 4.1 Modification NRF24.cpp

- [ ] Ajouter entrée "BAD NRF" dans le menu NRF24
- [ ] Créer sous-menu avec les 4 options
- [ ] Intégrer les includes nécessaires

### 4.2 Structure du menu (code)

```cpp
void NRF24Menu::optionsMenu() {
    options = {
        {"Spectrum",    [=]() { nrf_spectrum(); }},
        {"Info",        [=]() { nrf_info(); }},
        {"Config",      [=]() { nrf_config(); }},
        {"BAD NRF",     [=]() { badnrf_menu(); }},  // Nouveau
        {"Main Menu",   [=]() { backToMenu(); }}
    };
    loopOptions(options, false, true, "NRF24");
}

void badnrf_menu() {
    options = {
        {"Scan Devices",  [=]() { badnrf_scan(); }},
        {"Saved Targets", [=]() { badnrf_targets(); }},
        {"Test Target",   [=]() { badnrf_test(); }},
        {"Run Payload",   [=]() { badnrf_payload(); }},
        {"Back",          [=]() { backToMenu(); }}
    };
    loopOptions(options, false, true, "BAD NRF");
}
```

### 4.3 Gestion des fichiers

- [ ] Créer répertoire `/badnrf/targets/` pour les cibles sauvegardées
- [ ] Créer/réutiliser répertoire pour les payloads Ducky
- [ ] Utiliser les fonctions SD existantes (`sd_functions.cpp`)

---

## Phase 5 : Tests (Mode Build)

### 5.1 Tests unitaires

- [ ] Test initialisation nRF24 en mode promiscuous
- [ ] Test scan sur plusieurs canaux
- [ ] Test parsing script Ducky
- [ ] Test génération packets Logitech

### 5.2 Tests d'intégration

- [ ] Test complet scan → sauvegarde → test → injection
- [ ] Test avec un vrai dongle Logitech Unifying (ancien)
- [ ] Test sur T-Embed CC1101 Plus

### 5.3 Tests de sécurité

- [ ] Vérifier que l'injection ne fonctionne que sur cibles validées
- [ ] Timeout appropriés pour éviter les boucles infinies

---

## Fichiers concernés

```
src/
├── core/
│   └── menu_items/
│       └── NRF24.cpp            # Modification pour ajouter BAD NRF
├── modules/
│   └── nrf/
│       ├── bad_nrf.h            # Nouveau - Déclarations
│       └── bad_nrf.cpp          # Nouveau - Implémentation
```

Répertoires SD card :
```
/badnrf/
├── targets/
│   ├── bureau_pc.json
│   └── laptop_dev.json
└── payloads/                    # Ou réutiliser /badusb/
    ├── rickroll.txt
    └── reverse_shell.txt
```

---

## Références techniques

- [MouseJack Whitepaper (Bastille)](https://www.bastille.net/research/vulnerabilities/mousejack/technical-details)
- [uC_mousejack (Arduino)](https://github.com/insecurityofthings/uC_mousejack)
- [LOGITacker (nRF52)](https://github.com/RoganDawes/LOGITacker)
- [Logitech Unifying Protocol](https://lekensteyn.nl/files/logitech-unifying-2016-06-25.pdf)
- [nRF24L01+ Datasheet](https://www.nordicsemi.com/Products/nRF24-series)
- [Ducky Script Documentation](https://docs.hak5.org/hak5-usb-rubber-ducky/)

---

## Notes importantes

- **Légalité** : Cette fonctionnalité est destinée uniquement aux tests de sécurité autorisés (pentest, red team). L'utilisation sur des systèmes sans autorisation est illégale.
- **Limitations** : Les dongles Logitech récents avec firmware à jour peuvent ne pas être vulnérables.
- **Hardware** : Le nRF24L01+ intégré au T-Embed CC1101 Plus est suffisant mais la portée est limitée (~10m sans PA/LNA).
