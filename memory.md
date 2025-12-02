# Memory — BLE Audio Spoof + HID Module

## Architecture Bruce

- **Structure des modules** : `/src/modules/` contient tous les modules fonctionnels
- **Modules BLE existants** :
  - `/src/modules/ble/` : Fonctions BLE communes (scan, spam, ninebot)
  - `/src/modules/badusb_ble/` : Module BadBLE (injection clavier via BLE)
  - `/src/modules/rfid/pn532ble.*` : Interface RFID via BLE
- **Bibliothèques** : `/lib/Bad_Usb_Lib/` contient les implémentations HID (USB et BLE)
- **Menu principal** : `/src/core/menu_items/BleMenu.cpp` (entrée pour tous les menus BLE)
- **Plateforme cible** : ESP32-S3 (lilygo-t-embed-cc1101 dans platformio.ini)
- **Système de fichiers** : LittleFS (défini dans platformio.ini : `board_build.filesystem = littlefs`)

## Stack BLE

- **Stack utilisé** : **NimBLE-Arduino** (v1.4.0)
- **Includes principaux** :
  - `<NimBLEDevice.h>` : Initialisation et gestion du périphérique BLE
  - `<NimBLEServer.h>` : Serveur GATT
  - `<NimBLEUtils.h>` : Utilitaires
  - `<NimBLEScan.h>` : Scanner BLE
  - `<NimBLEAdvertisedDevice.h>` : Devices découverts
  - `<NimBLEBeacon.h>` : Beacons
  - `<NimBLEHIDDevice.h>` : Périphérique HID over GATT
- **Configuration** : `USE_NIMBLE` défini dans BleKeyboard.h et détecté par CONFIG_BT_ENABLED
- **Dépendance** : `h2zero/NimBLE-Arduino@^1.4.0` (platformio.ini ligne 139)

## Fichiers pertinents

### Modules BLE existants

**1. Module BLE Scan** : `/src/modules/ble/ble_common.cpp|h`
- Fonction : `ble_scan()` - Scanner les périphériques BLE
- Fonction : `ble_scan_setup()` - Initialisation du scanner
- Classe : `AdvertisedDeviceCallbacks` - Callbacks pour les devices découverts
- Variables globales :
  - `BLEScan *pBLEScan` - Instance du scanner
  - `int scanTime = 5` (secondes)
  - `SCAN_INT = 100`, `SCAN_WINDOW = 99`
- Données capturées : nom, adresse, RSSI
- ⚠️ **Limitation** : Ne capture PAS les manufacturer data ni les service UUIDs (à implémenter nous-mêmes)

**2. Module BadBLE** : `/src/modules/badusb_ble/ducky_typer.cpp|h`
- Fonction : `ducky_setup(HIDInterface *&hid, bool ble)` - Initialiser BadUSB/BadBLE
- Fonction : `ducky_keyboard(HIDInterface *&hid, bool ble)` - Mode clavier libre
- Fonction : `key_input(FS fs, String bad_script, HIDInterface *hid)` - Exécuter un script DuckyScript
- Fonction : `MediaCommands(HIDInterface *hid, bool ble)` - Commandes média
- Variables globales :
  - `HIDInterface *hid_ble` - Instance du clavier BLE (globale)
  - `bool BLEConnected` - État de la connexion BLE
- Parser DuckyScript : Supporte STRING, DELAY, REPEAT, combinaisons de touches (CTRL-ALT, etc.)

**3. Bibliothèque BleKeyboard** : `/lib/Bad_Usb_Lib/BleKeyboard.cpp|h`
- Classe : `BleKeyboard` (hérite de `BLEServerCallbacks`, `BLECharacteristicCallbacks`, `HIDInterface`)
- Méthodes publiques importantes :
  - `void begin(const uint8_t *layout, uint16_t showAs)` - Démarrer le clavier BLE
  - `void end()` - Arrêter le clavier BLE
  - `void setName(String deviceName)` - Changer le nom BLE
  - `void setAppearence(uint16_t v)` - Changer l'appearance (0x03C1 = clavier par défaut)
  - `void setBatteryLevel(uint8_t level)` - Niveau de batterie
  - `bool isConnected()` - Vérifier la connexion
  - `size_t press(uint8_t k)` - Presser une touche
  - `size_t release(uint8_t k)` - Relâcher une touche
  - `size_t write(uint8_t c)` - Écrire un caractère
  - `void releaseAll()` - Relâcher toutes les touches
  - `void setRandomUUID()` - UUID aléatoire
- Attributs privés :
  - `BLEHIDDevice *hid` - Device HID
  - `BLECharacteristic *inputKeyboard` - Caractéristique HID clavier
  - `BLECharacteristic *inputMediaKeys` - Caractéristique HID média
  - `BLEAdvertising *advertising` - Publicité BLE
  - `uint16_t appearance = 0x03C1` - Appearance (clavier)
  - `uint16_t vid = 0x05ac, pid = 0x820a` - VID/PID (Apple)
- Report Descriptor : HID clavier + media keys (défini dans le .cpp)

## Fonctions réutilisables

### Module BadBLE (`/src/modules/badusb_ble/ducky_typer.cpp|h`)

**Fonctions publiques utilisables** :
- `void ducky_startKb(HIDInterface *&hid, const uint8_t *layout, bool ble)` - Initialiser le clavier HID (USB ou BLE)
- `void ducky_setup(HIDInterface *&hid, bool ble)` - Lancer l'interface complète BadUSB/BadBLE avec sélection de fichier
- `void key_input(FS fs, String bad_script, HIDInterface *hid)` - Exécuter un script DuckyScript depuis un fichier
- `void key_input_from_string(String text)` - Envoyer une simple commande (USB uniquement)
- `void ducky_keyboard(HIDInterface *&hid, bool ble)` - Mode clavier libre (saisie en temps réel)
- `void MediaCommands(HIDInterface *hid, bool ble)` - Interface de commandes média

**Variables globales utilisables** :
- `extern HIDInterface *hid_ble` - Instance globale du clavier BLE (partagée dans toute l'application)
- `extern HIDInterface *hid_usb` - Instance globale du clavier USB
- `extern bool BLEConnected` (dans globals.h) - État de la connexion BLE

**Parser DuckyScript** :
- Commandes supportées : STRING, STRINGLN, DELAY, DEFAULTDELAY, REPEAT, REM (commentaires)
- Touches spéciales : CTRL, ALT, GUI, SHIFT, ENTER, TAB, ESCAPE, F1-F12, flèches, etc.
- Combinaisons : CTRL-ALT, CTRL-SHIFT, CTRL-GUI, ALT-SHIFT, etc.
- ⚠️ **Le parser est dans la fonction `key_input()` - on peut le réutiliser directement**

**Layouts clavier disponibles** :
- KeyboardLayout_en_US, KeyboardLayout_pt_BR, KeyboardLayout_pt_PT, KeyboardLayout_fr_FR, KeyboardLayout_es_ES, KeyboardLayout_it_IT, KeyboardLayout_en_UK, KeyboardLayout_de_DE, KeyboardLayout_sv_SE, KeyboardLayout_da_DK, KeyboardLayout_hu_HU, KeyboardLayout_tr_TR, KeyboardLayout_si_SI

### Module BLE Scan (`/src/modules/ble/ble_common.cpp|h`)

**Fonctions publiques utilisables** :
- `void ble_scan()` - Interface complète de scan BLE avec affichage
- `void ble_scan_setup()` - Initialisation du scanner BLE (sans lancer le scan)
- Variables globales :
  - `extern BLEScan *pBLEScan` - Instance du scanner
  - `extern int scanTime` - Durée du scan (par défaut 5 secondes)

**Callbacks disponibles** :
- `class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks` - Pour capturer les résultats de scan
  - Méthode : `void onResult(NimBLEAdvertisedDevice *advertisedDevice)`
  - Données accessibles : `getName()`, `getAddress()`, `getRSSI()`

⚠️ **Limitation** : Le scanner existant ne capture PAS :
- Manufacturer data
- Service UUIDs
- Advertisement data brutes
→ **Il faudra créer notre propre implémentation de callbacks pour capturer ces données**

**API NimBLE pour accéder aux données complètes** :
À partir de `NimBLEAdvertisedDevice *advertisedDevice` dans le callback `onResult()` :
- `advertisedDevice->getManufacturerData()` - Données fabricant (String)
- `advertisedDevice->haveManufacturerData()` - Vérifier si présent
- `advertisedDevice->getServiceUUID()` - Premier UUID de service
- `advertisedDevice->haveServiceUUID()` - Vérifier si présent
- `advertisedDevice->getPayload()` - Données brutes complètes de l'advertisement (uint8_t *)
- `advertisedDevice->getPayloadLength()` - Longueur du payload
- `advertisedDevice->getAddress()` - Adresse BLE (NimBLEAddress)
- `advertisedDevice->getAddressType()` - Type d'adresse (public/random)
- `advertisedDevice->getName()` - Nom du device
- `advertisedDevice->getRSSI()` - Signal strength
- `advertisedDevice->getTXPower()` - Puissance TX si disponible
- `advertisedDevice->getAppearance()` - Apparence du device

**Pour notre module de capture, il faudra** :
1. Créer une classe héritant de `NimBLEAdvertisedDeviceCallbacks`
2. Surcharger `onResult(NimBLEAdvertisedDevice *advertisedDevice)`
3. Extraire TOUTES les données (pas seulement nom/adresse/RSSI)
4. Stocker dans une structure personnalisée

## Classes réutilisables

### `BleKeyboard` (`/lib/Bad_Usb_Lib/BleKeyboard.cpp|h`)

**Constructeur** :
```cpp
BleKeyboard(String deviceName = "ESP32 Keyboard", String deviceManufacturer = "Espressif", uint8_t batteryLevel = 100)
```

**Méthodes importantes** :
- `void begin(const uint8_t *layout, uint16_t showAs)` - Démarrer le clavier BLE avec un layout et une apparence
- `void end()` - Arrêter le clavier BLE
- `void setName(String deviceName)` - Changer le nom BLE (⚠️ avant begin() uniquement)
- `void setAppearence(uint16_t v)` - Changer l'appearance BLE
  - `0x03C1` = Clavier (HID_KEYBOARD)
  - `0x03C0` = Pointeur
  - `0x0340` = Audio (écouteurs/casque) - **À tester pour le spoof**
- `void setBatteryLevel(uint8_t level)` - Niveau de batterie (0-100)
- `bool isConnected()` - Vérifier si un client est connecté
- `void setDelay(uint32_t ms)` - Délai entre les touches (défaut 7ms)
- `void setRandomUUID()` - Toggle l'UUID aléatoire
- `void set_vendor_id(uint16_t vid)` - Changer le VID (défaut 0x05ac = Apple)
- `void set_product_id(uint16_t pid)` - Changer le PID (défaut 0x820a)
- `void set_version(uint16_t version)` - Changer la version (défaut 0x0210)

**Méthodes d'injection** :
- `size_t press(uint8_t k)` - Presser une touche (sans relâcher)
- `size_t release(uint8_t k)` - Relâcher une touche
- `size_t write(uint8_t c)` - Écrire un caractère (press + release automatique)
- `size_t write(const uint8_t *buffer, size_t size)` - Écrire un buffer
- `void releaseAll()` - Relâcher toutes les touches
- `void sendReport(KeyReport *keys)` - Envoyer un rapport HID brut
- `void sendReport(MediaKeyReport *keys)` - Envoyer un rapport média

**Callbacks (virtuelles, surchargeables)** :
- `void onConnect(BLEServer *pServer)` - Appelé lors d'une connexion
- `void onDisconnect(BLEServer *pServer)` - Appelé lors d'une déconnexion
- `void onAuthenticationComplete(ble_gap_conn_desc *desc)` - Fin d'authentification

**Hérite de** :
- `HIDInterface` - Interface commune pour USB/BLE
- `BLEServerCallbacks` - Callbacks du serveur BLE
- `BLECharacteristicCallbacks` - Callbacks des caractéristiques GATT

### `HIDInterface` (`/lib/Bad_Usb_Lib/Bad_Usb_Lib.h`)

Interface abstraite commune pour USB et BLE. Méthodes :
- `virtual void begin(const uint8_t *layout)`
- `virtual void end()`
- `virtual size_t write(uint8_t k)`
- `virtual size_t press(uint8_t k)`
- `virtual size_t release(uint8_t k)`
- `virtual void releaseAll()`
- `virtual bool isConnected()`
- `virtual void setLayout(const uint8_t *layout)`

⚠️ **Notre nouveau module pourra instancier `BleKeyboard` directement** pour bénéficier de toutes ses fonctionnalités

## Système de menus

### Structure du menu principal

**Fichier** : `/src/core/main_menu.cpp|h`

Le menu principal est géré par la classe `MainMenu` qui contient une liste de `MenuItemInterface *` :
- `wifiMenu` (WifiMenu)
- `bleMenu` (BleMenu) ← **C'est ici qu'on devra ajouter notre module**
- `ethernetMenu` (EthernetMenu)
- `rfMenu` (RFMenu)
- `rfidMenu` (RFIDMenu)
- `irMenu` (IRMenu)
- `fmMenu` (FMMenu)
- `fileMenu` (FileMenu)
- `gpsMenu` (GpsMenu)
- `nrf24Menu` (NRF24Menu)
- `scriptsMenu` (ScriptsMenu)
- `othersMenu` (OthersMenu)
- `clockMenu` (ClockMenu)
- `connectMenu` (ConnectMenu)
- `configMenu` (ConfigMenu)

### Intégration d'un nouveau sous-menu dans BLE

**Fichier à modifier** : `/src/core/menu_items/BleMenu.cpp`
**Méthode** : `void BleMenu::optionsMenu()`
**Ligne** : ~11-40

**Pattern d'ajout** :
```cpp
options.push_back({"Nom du menu", [=]() { ma_fonction(); }});
```

**Exemple existant** :
```cpp
options.push_back({"BLE Scan", ble_scan});
options.push_back({"Bad BLE", [=]() { ducky_setup(hid_ble, true); }});
```

**Après l'ajout des options, appeler** :
```cpp
addOptionToMainMenu();  // Ajoute l'option "Retour au menu principal"
loopOptions(options, MENU_TYPE_SUBMENU, "Bluetooth");  // Lance la boucle de menu
```

### Fonction de navigation : `loopOptions()`

**Signature** :
```cpp
int loopOptions(
    std::vector<Option> &options,
    uint8_t menuType = MENU_TYPE_REGULAR,
    const char *subText = "",
    int index = 0,
    bool interpreter = false
);
```

**Types de menus** :
- `MENU_TYPE_MAIN` - Menu principal avec icônes
- `MENU_TYPE_SUBMENU` - Sous-menu avec titre
- `MENU_TYPE_REGULAR` - Menu simple

**Retour** : Index du dernier élément sélectionné (pour persistance)

### Mapping des boutons/inputs

**Variables globales** (dans `globals.h`) :
- `extern volatile bool SelPress` - Bouton de sélection/validation
- `extern volatile bool EscPress` - Bouton retour/annulation
- `extern volatile bool PrevPress` - Bouton précédent (navigation)
- `extern volatile bool NextPress` - Bouton suivant (navigation)
- `extern volatile bool UpPress` - Bouton haut (navigation)
- `extern volatile bool DownPress` - Bouton bas (navigation)
- `extern volatile bool AnyKeyPress` - N'importe quel bouton
- `extern volatile bool LongPress` - Appui long

**Fonction de vérification** : `bool check(volatile bool &btn)`
- Vérifie si un bouton est pressé
- Remet le bouton à false après lecture
- Gère le debouncing
- Exemple : `if (check(EscPress)) { /* retour */ }`

### Patterns UI réutilisables

**Affichage** :
- `void drawMainBorder(bool clear = true)` - Bordure principale
- `void drawMainBorderWithTitle(String title, bool clear = true)` - Bordure avec titre
- `void printTitle(String title)` - Titre centré
- `void printSubtitle(String subtitle, bool withLine = true)` - Sous-titre
- `void printFootnote(String text)` - Note en bas
- `void printCenterFootnote(String text)` - Note centrée en bas

**Messages** :
- `void displayTextLine(String text)` - Afficher une ligne de texte
- `void displayError(String text, bool wait = false)` - Message d'erreur
- `void displayWarning(String text, bool wait = false)` - Message d'avertissement
- `void displaySuccess(String text, bool wait = false)` - Message de succès

**Navigation dans une fonction** :
```cpp
void ma_fonction() {
    drawMainBorder();
    tft.drawString("Mon texte", 10, 50);
    
    while (!check(EscPress)) {
        // Logique de la fonction
        if (check(SelPress)) {
            // Action sur sélection
        }
        yield(); // Important pour le watchdog
    }
    
    // Retour automatique quand EscPress est détecté
}
```

### Structure Option

**Définition** (dans `globals.h`) :
```cpp
struct Option {
    String label;                                    // Texte affiché
    std::function<void()> operation;                 // Fonction appelée
    bool selected = false;                           // État sélectionné (pour checkbox)
    bool (*hover)(void *hoverPointer, bool shouldRender);  // Callback hover (optionnel)
    void *hoverPointer;                              // Pointeur pour hover (optionnel)
    bool hovered;                                    // État hover pour WebUI
};
```

**Utilisation simple** :
```cpp
options.push_back({"Mon option", [=]() { ma_fonction(); }});
```

**Avec état** :
```cpp
bool mon_flag = true;
options.push_back({"Mon option", [=]() { ma_fonction(); }, mon_flag});
```

### Fonction utilitaire

- `void addOptionToMainMenu()` - Ajoute automatiquement "Main Menu" aux options
- `void backToMenu()` - Force le retour au menu (set returnToMenu = true)
- `extern volatile bool returnToMenu` - Variable pour signaler un retour au menu

### Variable globale pour nos options

```cpp
extern std::vector<Option> options;
```
Cette variable globale est utilisée partout dans Bruce pour stocker temporairement les options de menu.

## Système de stockage

### Systèmes de fichiers disponibles

**LittleFS** : Système de fichiers interne (flash)
- Défini dans platformio.ini : `board_build.filesystem = littlefs`
- Include : `<LittleFS.h>`
- Monté automatiquement au démarrage
- Fonctions : `LittleFS.open()`, `LittleFS.exists()`, `LittleFS.mkdir()`, `LittleFS.remove()`
- Capacité limitée (selon la partition définie)

**SD Card** : Carte SD externe
- Include : `<SD.h>`
- Montage : `bool setupSdCard()` - Retourne true si succès
- Démontage : `void closeSdCard()`
- Toggle : `bool ToggleSDCard()` - Monte/démonte la carte
- Variable globale : `extern bool sdcardMounted` - État du montage

**Type abstrait** : `FS` (File System)
- Permet d'utiliser LittleFS ou SD de manière interchangeable
- Exemple : `FS *fs = &LittleFS;` ou `FS *fs = &SD;`

### Fonctions de gestion de fichiers

**Lecture** :
- `String readSmallFile(FS &fs, String filepath)` - Lire un petit fichier (<3KB) en String
- `char* readBigFile(FS &fs, String filepath, bool binary = false, size_t *fileSize = NULL)` - Lire un gros fichier (utilise PSRAM si disponible)
- `String readLineFromFile(File myFile)` - Lire une ligne jusqu'au ';'

**Écriture** :
- Ouvrir : `File file = fs.open(filepath, FILE_WRITE);`
- Écrire : `file.write(data, length)` ou `file.print(String)`
- Fermer : `file.close()`

**Manipulation** :
- `bool deleteFromSd(FS fs, String path)` - Supprimer fichier ou dossier (récursif)
- `bool renameFile(FS fs, String path, String filename)` - Renommer un fichier
- `bool copyFile(FS fs, String path)` - Copier un fichier (dans buffer mémoire)
- `bool pasteFile(FS fs, String path)` - Coller le fichier copié
- `bool copyToFs(FS from, FS to, String path, bool draw = true)` - Copier entre systèmes de fichiers
- `bool createFolder(FS fs, String path)` - Créer un dossier (demande le nom via clavier)

**Navigation** :
- `void readFs(FS fs, String folder, String allowed_ext = "*")` - Lister les fichiers d'un dossier
- `String loopSD(FS &fs, bool filePicker = false, String allowed_ext = "*", String rootPath = "/")` - Interface de navigation fichiers
- `void viewFile(FS fs, String filepath)` - Afficher le contenu d'un fichier

**Utilitaires** :
- `String md5File(FS &fs, String filepath)` - Hash MD5 d'un fichier
- `String crc32File(FS &fs, String filepath)` - CRC32 d'un fichier
- `bool checkLittleFsSize()` - Vérifier l'espace disponible sur LittleFS

### Format JSON avec ArduinoJson

**Include** : `<ArduinoJson.h>` (déjà dans les dépendances)

**Pattern de sauvegarde** :
```cpp
#include <ArduinoJson.h>

// Créer un document JSON
JsonDocument doc;
doc["name"] = "My Device";
doc["address"] = "AA:BB:CC:DD:EE:FF";
doc["rssi"] = -50;

// Ouvrir un fichier en écriture
File file = LittleFS.open("/path/to/file.json", FILE_WRITE);
if (!file) {
    // Erreur
    return false;
}

// Sérialiser le JSON dans le fichier
serializeJsonPretty(doc, file);  // Pretty = avec indentation
// ou serializeJson(doc, file);  // Compact

file.close();
```

**Pattern de chargement** :
```cpp
#include <ArduinoJson.h>

// Ouvrir le fichier
File file = LittleFS.open("/path/to/file.json", FILE_READ);
if (!file) {
    // Erreur
    return false;
}

// Créer un document JSON
JsonDocument doc;

// Désérialiser depuis le fichier
DeserializationError error = deserializeJson(doc, file);
file.close();

if (error) {
    // Erreur de parsing
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return false;
}

// Lire les valeurs
String name = doc["name"].as<String>();
String address = doc["address"].as<String>();
int rssi = doc["rssi"].as<int>();
```

### Recommandations pour notre module

**Structure de dossiers** :
- Créer un dossier dédié : `/ble_spoof/` ou `/ble_audio_spoof/`
- Sous-dossiers : `/ble_spoof/profiles/` pour les profils capturés

**Format de fichier** :
- Extension : `.json` pour les profils de devices
- Utiliser ArduinoJson pour la sérialisation/désérialisation
- Stocker : nom, adresse, RSSI, manufacturer data (hex), service UUIDs, appearance, TX power, payload brut

**Exemple de structure JSON** :
```json
{
  "name": "Redmi Buds 4",
  "address": "AA:BB:CC:DD:EE:FF",
  "addressType": 1,
  "rssi": -45,
  "appearance": 960,
  "txPower": 0,
  "manufacturerData": "4C00...",
  "serviceUUIDs": ["180F", "180A"],
  "payload": "02010619FF..."
}
```

**Choix du système de fichiers** :
- LittleFS : Pour les profils par défaut, toujours disponible
- SD Card : Pour les profils utilisateur, vérifier `sdcardMounted` avant

## Résultats de tests

### Phase 5 - Tests popup (À effectuer avec hardware réel)

**Protocole de test complet** : Voir `/TEST_PROTOCOL_BLE_SPOOF.md`

#### Test 5.1 - Popup MIUI (EN ATTENTE)

**État** : ⏸️ En attente d'exécution avec hardware réel

**Matériel requis** :
- ESP32-S3 T-Embed flashé avec Bruce + module BLE Spoof
- Téléphone Xiaomi avec MIUI
- Écouteurs BLE réels pour capturer un profil

**Objectif** : Vérifier si le popup mentionne "clavier" ou uniquement "audio"

**Résultats** : [À compléter après test]

#### Test 5.2 - Tests alternatifs (EN ATTENTE)

**État** : ⏸️ En attente (seulement si Test 5.1 détecte le HID)

**Variantes à tester** :
- Ordre des services dans advertisements
- Retarder l'annonce du HID
- Différentes valeurs d'appearance
- Variations des manufacturer data

**Résultats** : [À compléter après test]

#### Test 5.3 - Popup Android stock (EN ATTENTE)

**État** : ⏸️ En attente d'exécution avec hardware réel

**Téléphones à tester** : Google Pixel, Samsung Galaxy, OnePlus

**Objectif** : Comparer le comportement avec MIUI

**Résultats** : [À compléter après test]

## Problèmes rencontrés

### Phase 3 - Spoofing MAC address

**Problème initial** :
- Tentative naïve : deinit() + init() coupe les connexions actives
- ❌ Ne fonctionne pas si on veut changer la MAC pendant le spoofing

**Solution implémentée** :
- ✅ Utilisation de `ble_hs_id_set_rnd()` (fonction native NimBLE)
- ✅ Include de `host/ble_hs_id.h` pour accéder aux fonctions C natives
- ✅ Changement de la MAC AVANT de créer le serveur et l'advertising
- ✅ Séquence : init(name) → setSpoofAddress(mac) → createServer() → startAdvertising()
- ✅ Pas de coupure car fait avant toute connexion

**Détails techniques** :
- BLE Spec : Les adresses random doivent avoir les bits 6 et 7 à 1 (0b11xxxxxx)
- Implementation : `addr[5] |= 0xC0` pour forcer une "random static address"
- Type d'adresse : `BLE_OWN_ADDR_RANDOM` (vs `BLE_OWN_ADDR_PUBLIC`)
- Ordre des bytes : Inversé car BLE utilise little-endian

## Modifications de fichiers existants

<!-- Journal de toutes les modifications sur des fichiers existants -->
| Fichier | Ligne | Type | Description | Impact vérifié |
|---------|-------|------|-------------|----------------|
| — | — | — | — | — |

## Notes techniques

### Nouveaux fichiers créés (Phase 2)

**Module BLE Spoof** :
- `/src/modules/ble/ble_spoof.h` - Header du module
- `/src/modules/ble/ble_spoof.cpp` - Implémentation du module

**Structures de données** :
- `struct BleDeviceProfile` : Stocke un profil complet de device BLE
  - Champs : name, address, addressType, rssi, appearance, txPower, manufacturerData, serviceUUIDs, payload, payloadLength
  - Méthodes : clear() pour réinitialiser

**Classes créées** :
- `class BleSpoofScanCallbacks` : Callbacks personnalisés pour le scan BLE
  - Hérite de `NimBLEAdvertisedDeviceCallbacks`
  - Capture TOUTES les données : manufacturer data, service UUIDs, payload complet, appearance
  - Méthode : `onResult()` - appelé pour chaque device trouvé
  - Méthode : `getDevices()` - retourne la liste des devices capturés
  
- `class BleSpoofModule` : Module principal
  - Méthodes de scan : `startScan()`, `stopScan()`, `getScannedDevices()`, `clearScannedDevices()`
  - Méthodes de gestion de profils : `saveProfile()`, `loadProfile()`, `deleteProfile()`, `listProfiles()`
  - Méthodes JSON : `profileToJson()`, `jsonToProfile()`
  - Instance globale : `bleSpoofModule`

**Fonctions UI** :
- `void ble_spoof_scan_and_capture()` : Interface de scan et sauvegarde
- `void ble_spoof_load_profile()` : Interface de chargement de profil
- `void ble_spoof_main_menu()` : Menu principal du module

**Fonctionnalités implémentées** :
- ✅ Scan BLE avec capture complète des données (manufacturer data, service UUIDs, payload)
- ✅ Sauvegarde de profils en JSON sur LittleFS ou SD Card
- ✅ Chargement de profils depuis JSON
- ✅ Suppression de profils
- ✅ Listage des profils disponibles
- ✅ Interface utilisateur avec menus

### Phase 3 - Spoofing ajouté

**Méthodes de spoofing** :
- `bool setSpoofName(const String &name)` : Change le nom BLE annoncé
- `bool setSpoofAddress(const String &address)` : Change la MAC (limitation documentée)
- `bool setSpoofAppearance(uint16_t appearance)` : Configure l'appearance BLE
- `bool setAdvertisementData(const String &hexData)` : Configure les données d'advertisement
- `bool startSpoofing(const BleDeviceProfile &profile)` : Démarre le spoofing complet
- `bool stopSpoofing()` : Arrête le spoofing

**Fonctionnalités de spoofing** :
- ✅ Changement du nom BLE (re-init de NimBLE)
- ✅ **Changement de MAC (IMPLÉMENTÉ avec `ble_hs_id_set_rnd()` !)**
- ✅ Configuration de l'appearance (0x0340 pour audio, etc.)
- ✅ Ajout des manufacturer data dans les advertisements
- ✅ Ajout des service UUIDs dans les advertisements
- ✅ Démarrage/arrêt du spoofing
- ✅ Advertising actif avec tous les paramètres configurés

**Fonctions UI ajoutées** :
- `void ble_spoof_start_spoofing()` : Interface pour choisir et démarrer un profil
- `void ble_spoof_stop_spoofing()` : Interface pour arrêter le spoofing
- Menu principal mis à jour avec options Start/Stop Spoofing

**Implémentation MAC spoofing (améliorée)** :
- ✅ Utilise `ble_hs_id_set_rnd()` de NimBLE (fonction native)
- ✅ Parse l'adresse MAC depuis String vers tableau uint8_t[6]
- ✅ Inverse l'ordre des bytes (BLE utilise little-endian)
- ✅ Force les bits 6 et 7 à 1 (0xC0) pour adresse random static valide (spec BLE)
- ✅ Appelle `NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM)` pour activer
- ✅ **Séquence correcte** : init() → setSpoofAddress() → createServer() → startAdvertising()
- ⚠️ La MAC est changée AVANT de créer le serveur (pas de coupure de connexion)
- ✅ Fonctionne "à la volée" sans couper les connexions car fait avant advertising

### Phase 4 - Multi-profils Audio + HID

**Services GATT implémentés** :
- ✅ Service HID (UUID 0x1812) - Clavier BLE
- ✅ Service Battery (UUID 0x180F) - Niveau de batterie

**Méthodes HID ajoutées** :
- `bool setupHIDService()` : Configure le service HID avec report descriptor
- `bool setupBatteryService()` : Configure le service Battery
- `bool enableHID(bool)` : Active/désactive le HID
- `bool sendKeyPress(uint8_t key, uint8_t modifiers)` : Envoie une touche
- `bool sendKeyRelease()` : Relâche toutes les touches
- `bool sendString(const String &text)` : Envoie une chaîne de caractères
- `bool setBatteryLevel(uint8_t level)` : Change le niveau de batterie

**Configuration du pairing** :
- ✅ Mode "Just Works" (no PIN)
- ✅ IO Capabilities : NO_INPUT_OUTPUT
- ✅ Security : No bonding, no MITM, Secure Connections uniquement
- ✅ Configuration via `NimBLEDevice::setSecurityAuth()` et `setSecurityIOCap()`

**Intégration dans startSpoofing()** :
- Séquence : init() → setSpoofAddress() → createServer() → setupHIDService() → setupBatteryService() → startAdvertising()
- Les UUIDs HID (0x1812) et Battery (0x180F) sont ajoutés aux advertisements
- Le HID est configuré AVANT de démarrer l'advertising

**HID Report Descriptor** :
- Clavier standard (Report ID 0x01)
- 8 bytes : [modifiers, reserved, key1-key6]
- Support des touches modificatrices (Ctrl, Alt, Shift, GUI)
- Logical Max 101 touches (US layout)

### Phase 6 - Injection HID

**Méthodes d'injection de base** :
- ✅ `bool sendKeyPress(uint8_t key, uint8_t modifiers)` : Envoie une touche avec modificateurs
  - HID Report format : [modifiers, reserved, key1-6]
  - Notifie la caractéristique HID input
  - Délai de 20ms entre press et release
  
- ✅ `bool sendKeyRelease()` : Relâche toutes les touches
  - Envoie un rapport vide (tous les bytes à 0)
  - Appelé automatiquement après chaque touche
  
- ✅ `bool sendString(const String &text)` : Envoie une chaîne complète
  - Conversion ASCII → HID keycode
  - Support : a-z, A-Z, 0-9, espace, entrée
  - Gestion automatique du modificateur Shift pour majuscules
  - 20ms entre chaque touche

**Parser DuckyScript complet** :
- ✅ `bool executeDuckyScript(FS &fs, const String &filepath)` : Exécute un script DuckyScript
  - Lecture ligne par ligne depuis fichier
  - Support des commandes DuckyScript complètes
  - Affichage en temps réel sur l'écran
  - Annulation possible avec ESC
  
**Commandes DuckyScript supportées** :
- `STRING` / `STRINGLN` : Envoie du texte
- `DELAY` / `DEFAULTDELAY` : Pause en millisecondes
- `REPEAT` : Répète la ligne précédente
- `REM` : Commentaire
- Touches spéciales : ENTER, TAB, ESC, DELETE, BACKSPACE, ARROW keys, etc.
- Touches fonction : F1-F12
- Modificateurs simples : CTRL, ALT, SHIFT, GUI
- Combinaisons : CTRL-ALT, CTRL-SHIFT, CTRL-GUI, ALT-SHIFT, etc.
- Combinaisons à 3 touches : CTRL-ALT-SHIFT, CTRL-ALT-GUI, etc.

**Structures de données DuckyScript** :
- Enum `DuckyCommandType` : Unknown, Cmd, Print, Delay, Comment, Loop, Combination
- Struct `DuckyCommand` : command, key, type
- Struct `DuckyCombination` : command, key1, key2, key3
- Tables complètes `duckyCmds[]` et `duckyComb[]` pour le parsing

**Conversion des combinaisons** :
- Parsing des modificateurs en byte modifiers (bits 0-3)
  - Bit 0 : LEFT_CTRL
  - Bit 1 : LEFT_SHIFT
  - Bit 2 : LEFT_ALT
  - Bit 3 : LEFT_GUI
- Extraction de la touche normale (non-modificateur)
- Envoi avec `sendKeyPress(key, modifiers)`

**Interface utilisateur Phase 6** :
- ✅ `void ble_spoof_execute_payload()` : Interface de sélection et exécution de payload
  - Choix du système de fichiers (SD/LittleFS)
  - File picker pour fichiers *.txt
  - Écran de confirmation
  - Exécution du script avec affichage en direct
  - Gestion des erreurs

**Menu principal mis à jour** :
- Option "Execute Payload" apparaît quand spoofing actif
- Permet l'injection de payloads pendant une connexion active

**Limitations actuelles** :
- Conversion ASCII → keycode limitée (US layout basique)
- Pas de support des caractères spéciaux complexes (@, #, {, }, etc.)
- Pour étendre : ajouter plus de conversions dans `sendString()`
  
**Note technique** :
- Le parser DuckyScript est une copie adaptée du module BadBLE existant
- Réutilisation des mêmes structures de commandes pour cohérence
- Fonction `key_input()` de BadBLE pourrait être appelée directement en alternative

**Services GATT implémentés** :
- ✅ Service HID (UUID 0x1812) - Clavier BLE
- ✅ Service Battery (UUID 0x180F) - Niveau de batterie

**Méthodes HID ajoutées** :
- `bool setupHIDService()` : Configure le service HID avec report descriptor
- `bool setupBatteryService()` : Configure le service Battery
- `bool enableHID(bool)` : Active/désactive le HID
- `bool sendKeyPress(uint8_t key, uint8_t modifiers)` : Envoie une touche
- `bool sendKeyRelease()` : Relâche toutes les touches
- `bool sendString(const String &text)` : Envoie une chaîne de caractères
- `bool setBatteryLevel(uint8_t level)` : Change le niveau de batterie

**Configuration du pairing** :
- ✅ Mode "Just Works" (no PIN)
- ✅ IO Capabilities : NO_INPUT_OUTPUT
- ✅ Security : No bonding, no MITM, Secure Connections uniquement
- ✅ Configuration via `NimBLEDevice::setSecurityAuth()` et `setSecurityIOCap()`

**Intégration dans startSpoofing()** :
- Séquence : init() → setSpoofAddress() → createServer() → setupHIDService() → setupBatteryService() → startAdvertising()
- Les UUIDs HID (0x1812) et Battery (0x180F) sont ajoutés aux advertisements
- Le HID est configuré AVANT de démarrer l'advertising

**HID Report Descriptor** :
- Clavier standard (Report ID 0x01)
- 8 bytes : [modifiers, reserved, key1-key6]
- Support des touches modificatrices (Ctrl, Alt, Shift, GUI)
- Logical Max 101 touches (US layout)
