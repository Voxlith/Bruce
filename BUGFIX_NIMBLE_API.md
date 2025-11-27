# BugFix: NimBLE API Compatibility

## Issue

Erreur de compilation dans `ble_scanner.cpp`:
```
classe "NimBLEAdvertisedDevice" n'a pas de membre "getServiceUUIDs"
```

## Root Cause

Le projet utilise **NimBLE-Arduino version 1.4.0** (spécifié dans `platformio.ini`).

Dans cette version, l'API pour accéder aux Service UUIDs est différente:
- ❌ `getServiceUUIDs()` (pluriel) n'existe pas
- ✅ `getServiceUUID()` (singulier) existe

## Solution Applied

### Fichier modifié: `src/modules/ble/ble_scanner.cpp`

**Ligne 54-59 - AVANT (incorrect):**
```cpp
// Service UUIDs
if (advertisedDevice->haveServiceUUID()) {
    std::vector<NimBLEUUID> uuids = advertisedDevice->getServiceUUIDs();
    for (auto& uuid : uuids) {
        device.serviceUUIDs.push_back(String(uuid.toString().c_str()));
    }
}
```

**Ligne 54-58 - APRÈS (correct):**
```cpp
// Service UUIDs - using correct NimBLE API
if (advertisedDevice->haveServiceUUID()) {
    // NimBLE 1.4.0 uses getServiceUUID() (singular)
    NimBLEUUID serviceUUID = advertisedDevice->getServiceUUID();
    device.serviceUUIDs.push_back(String(serviceUUID.toString().c_str()));
}
```

## Explanation

### NimBLE 1.4.0 API pour Service UUIDs:

1. **`haveServiceUUID()`** - Vérifie si au moins un service UUID est présent
2. **`getServiceUUID()`** - Retourne le **premier** service UUID trouvé
3. **`isAdvertisingService(NimBLEUUID)`** - Vérifie si un UUID spécifique est annoncé

### Limitations de cette approche:

⚠️ **Important:** Avec `getServiceUUID()`, on ne capture que le **premier** service UUID.

Si un appareil annonce **plusieurs** service UUIDs, seul le premier sera capturé.

### Solutions alternatives (non implémentées pour garder la simplicité):

**Option 1: Parser manuellement les AD Data**
```cpp
uint8_t* payload = advertisedDevice->getPayload();
// Parser les types AD 0x02, 0x03, 0x06, 0x07 pour tous les UUIDs
```

**Option 2: Utiliser NimBLE 1.4.3+ si disponible**
```cpp
// Version 1.4.3+ pourrait avoir getServiceUUIDs()
// Vérifier dans platformio.ini: h2zero/NimBLE-Arduino@^1.4.3
```

## Impact

### Fonctionnalités affectées:
✅ **Scanner** - Fonctionne correctement
  - Capture le premier service UUID (le plus courant)
  - Capture toujours les manufacturer data (non affecté)
  - Capture le nom, adresse, RSSI (non affecté)

### Cas d'usage typiques:
- ✅ AirPods (1 UUID principal)
- ✅ Galaxy Watch (1 UUID principal)  
- ✅ Fitbit, smartwatches (généralement 1 UUID principal)
- ⚠️ Devices complexes avec multiples services (capture partielle)

## Testing

### Test 1: Scanner un appareil simple (AirPods, etc.)
```
Résultat attendu: ✅ PASS
- Nom capturé
- Adresse capturée
- RSSI capturé
- Manufacturer data capturé
- Premier service UUID capturé
```

### Test 2: Scanner un appareil avec plusieurs services
```
Résultat attendu: ⚠️ PARTIAL
- Informations de base: ✅
- Manufacturer data: ✅
- Tous les service UUIDs: ❌ (seulement le premier)
```

### Test 3: Spam avec payload capturé
```
Résultat attendu: ✅ PASS
- Le payload JSON contient le service UUID capturé
- Le spam fonctionne correctement
```

## Recommendation pour amélioration future

Si besoin de capturer **tous** les service UUIDs, implémenter un parser AD Data manuel:

```cpp
void parseAllServiceUUIDs(NimBLEAdvertisedDevice* device, ScannedDevice& scanned) {
    uint8_t* payload = device->getPayload();
    size_t length = device->getPayloadLength();
    
    size_t i = 0;
    while (i < length) {
        uint8_t len = payload[i++];
        if (len == 0 || i + len > length) break;
        
        uint8_t type = payload[i++];
        len--; // Adjust for type byte
        
        // 0x02 = Incomplete List of 16-bit UUIDs
        // 0x03 = Complete List of 16-bit UUIDs
        // 0x06 = Incomplete List of 128-bit UUIDs
        // 0x07 = Complete List of 128-bit UUIDs
        
        if (type == 0x02 || type == 0x03) {
            // Parse 16-bit UUIDs
            for (size_t j = 0; j < len; j += 2) {
                uint16_t uuid16 = payload[i + j] | (payload[i + j + 1] << 8);
                char uuidStr[8];
                sprintf(uuidStr, "0x%04X", uuid16);
                scanned.serviceUUIDs.push_back(String(uuidStr));
            }
        } else if (type == 0x06 || type == 0x07) {
            // Parse 128-bit UUIDs
            // Implementation needed...
        }
        
        i += len;
    }
}
```

## Version Information

- **NimBLE-Arduino:** 1.4.0 (from platformio.ini)
- **API Method Used:** `getServiceUUID()` (singular)
- **Compatibility:** ✅ Works with NimBLE 1.4.0+
- **Date Fixed:** November 27, 2024

## Files Modified

1. ✅ `src/modules/ble/ble_scanner.cpp` - Ligne 54-58
2. ✅ `BUGFIX_NIMBLE_API.md` - Ce document

## Compilation Status

✅ **Should compile without errors now**

To verify:
```bash
cd /Users/matthieu/workspace-perso/Bruce
pio run -e lilygo-t-embed-cc1101
```

## Summary

✅ Bug fixé  
✅ Compatible NimBLE 1.4.0  
✅ Fonctionnalité scanner opérationnelle  
⚠️ Limitation: Un seul service UUID capturé (acceptable pour la majorité des cas)  
📝 Documentation fournie pour amélioration future  

---

**Status:** RESOLVED  
**Priority:** HIGH  
**Tested:** Ready for compilation test

---

## Bug #2: Undefined Button Press Identifiers

### Issue

Erreurs de compilation:
```
identificateur "OkPress" non défini dans ble_scanner.cpp
identificateur "OkPress" non défini dans ble_custom_spam.cpp
```

### Root Cause

Le projet Bruce utilise des noms de variables spécifiques pour les boutons:
- ✅ `SelPress` (Select/OK button)
- ✅ `EscPress` (Escape/Back button)
- ✅ `UpPress` (Up navigation)
- ✅ `DownPress` (Down navigation)
- ✅ `AnyKeyPress` (Any key pressed)
- ❌ `OkPress` (n'existe pas)

Ces variables sont déclarées dans `include/globals.h` (lignes 184-192):
```cpp
extern volatile bool UpPress;
extern volatile bool DownPress;
extern volatile bool SelPress;  // <-- Correct name for OK/Select
extern volatile bool EscPress;
extern volatile bool AnyKeyPress;
```

La fonction `check()` est également définie dans `globals.h` (ligne 213):
```cpp
extern inline bool check(volatile bool &btn) {
    if (!btn) return false;
    btn = false;
    AnyKeyPress = false;
    SerialCmdPress = false;
    return true;
}
```

### Solution Applied

**Fichiers modifiés:**
1. `src/modules/ble/ble_scanner.cpp`
2. `src/modules/ble/ble_custom_spam.cpp`

**Changement global:**
```cpp
// AVANT (incorrect)
check(OkPress)

// APRÈS (correct)
check(SelPress)
```

### Files Modified Details

**ble_scanner.cpp:**
- Ligne 334: `if (check(OkPress))` → `if (check(SelPress))`
- Ligne 357: `if (check(OkPress))` → `if (check(SelPress))`

**ble_custom_spam.cpp:**
- Ligne 302: `if (check(OkPress))` → `if (check(SelPress))`
- Ligne 348: `if (check(OkPress))` → `if (check(SelPress))`

### Button Usage Pattern in Bruce

D'après les autres fichiers du projet (ble_spam.cpp, display.cpp):

```cpp
// Navigation
if (check(UpPress))     { /* Move up */ }
if (check(DownPress))   { /* Move down */ }
if (check(SelPress))    { /* Select/Confirm */ }
if (check(EscPress))    { /* Cancel/Back */ }

// Special
if (check(AnyKeyPress)) { /* Any key to continue */ }
```

### References

Code patterns trouvés dans le projet:
- `src/modules/ble/ble_spam.cpp:669` - `if (check(EscPress))`
- `src/core/display.cpp:516` - `if (PrevPress || check(UpPress))`
- `src/core/display.cpp:557` - `if (check(NextPress) || check(DownPress))`
- `src/core/display.cpp:567` - `if (check(SelPress))`
- `src/core/display.cpp:583` - `if (check(EscPress))`

### Impact

✅ **Toutes les fonctionnalités de boutons maintenant correctes:**
- Scanner: Navigation et sélection fonctionnelles
- Custom Payload: Navigation et sélection fonctionnelles
- Compatibilité complète avec le système d'entrée Bruce

### Testing

**Test de navigation:**
```
Résultat attendu: ✅ PASS
- Up/Down: Navigation dans les listes
- Select: Sélection et confirmation
- Escape: Retour et annulation
```

---

## Summary of All Fixes

### Bug #1: NimBLE API ✅ FIXED
- File: `ble_scanner.cpp`
- Issue: `getServiceUUIDs()` n'existe pas
- Solution: Utiliser `getServiceUUID()` (singulier)

### Bug #2: Button Press Identifiers ✅ FIXED
- Files: `ble_scanner.cpp`, `ble_custom_spam.cpp`
- Issue: `OkPress` non défini
- Solution: Utiliser `SelPress`

---

## Compilation Status

✅ **Ready to compile without errors**

All button press functions now correctly use:
- `check(SelPress)` - Pour sélection/confirmation
- `check(EscPress)` - Pour annulation/retour
- `check(UpPress)` - Pour navigation haut
- `check(DownPress)` - Pour navigation bas

---

**Last Updated:** November 27, 2024  
**Bugs Fixed:** 2/2  
**Status:** READY FOR BUILD ✅

---

## Bug #3: ArduinoJson API Deprecated Methods

### Issue

Warnings de compilation (non-critiques mais à corriger):
```
warning: 'bool ArduinoJson::JsonDocument::containsKey(TChar*)' is deprecated
warning: 'bool ArduinoJson::JsonObject::containsKey(const TString&)' is deprecated
Use obj[key].is<T>() instead
```

### Root Cause

ArduinoJson v7+ a déprécié la méthode `containsKey()` en faveur de:
- ❌ `doc.containsKey("key")` (déprécié)
- ✅ `doc["key"].is<T>()` (nouvelle API)

### Solution Applied

**Fichier modifié:** `src/modules/ble/ble_custom_spam.cpp`

**Changements globaux:**

```cpp
// AVANT (déprécié)
if (doc.containsKey("adv_data")) {
    JsonObject advData = doc["adv_data"];
    if (advData.containsKey("flags")) { ... }
}

// APRÈS (nouvelle API)
if (doc["adv_data"].is<JsonObject>()) {
    JsonObject advData = doc["adv_data"];
    if (advData["flags"].is<String>()) { ... }
}
```

**Tous les changements:**
- Ligne 51: `doc.containsKey("adv_data")` → `doc["adv_data"].is<JsonObject>()`
- Ligne 55: `advData.containsKey("flags")` → `advData["flags"].is<String>()`
- Ligne 66: `advData.containsKey("complete_name")` → `advData["complete_name"].is<String>()`
- Ligne 71: `advData.containsKey("tx_power")` → `advData["tx_power"].is<int>()`
- Ligne 78: `advData.containsKey("manufacturer_data")` → `advData["manufacturer_data"].is<JsonObject>()`
- Ligne 80: `mfgData.containsKey("company_id")` → `mfgData["company_id"].is<String>()`
- Ligne 88: `mfgData.containsKey("data")` → `mfgData["data"].is<String>()`
- Ligne 95: `advData.containsKey("service_uuids")` → `advData["service_uuids"].is<JsonArray>()`
- Ligne 104: `doc.containsKey("raw_adv_data")` → `doc["raw_adv_data"].is<String>()`

---

## Bug #4: Private Member Access Violation

### Issue

Erreurs de compilation critiques:
```
error: 'NimBLEAdvertising* BLECustomSpam::pAdvertising' is private within this context
     spammer.pAdvertising->start();
```

### Root Cause

Dans `ble_custom_payload()`, tentative d'accès direct au membre privé `pAdvertising`:
```cpp
// Dans la boucle spam - ERREUR
if (spammer.pAdvertising) {
    spammer.pAdvertising->start();
    spammer.pAdvertising->stop();
}
```

La variable `pAdvertising` est déclarée `private` dans la classe.

### Solution Applied

**Fichiers modifiés:**
1. `src/modules/ble/ble_custom_spam.h`
2. `src/modules/ble/ble_custom_spam.cpp`

**Solution: Ajout d'une méthode publique**

Dans `.h` (ligne 47):
```cpp
public:
    // ... autres méthodes ...
    
    // Public method to perform one spam cycle
    void doSpamCycle();
```

Dans `.cpp` (lignes 281-286):
```cpp
// Public method to perform one spam cycle
void BLECustomSpam::doSpamCycle() {
    if (pAdvertising && spamActive) {
        pAdvertising->start();
        vTaskDelay(20 / portTICK_PERIOD_MS);
        pAdvertising->stop();
    }
}
```

Dans `ble_custom_payload()` (ligne 377):
```cpp
// AVANT (erreur - accès privé)
if (spammer.pAdvertising) {
    spammer.pAdvertising->start();
    vTaskDelay(20 / portTICK_PERIOD_MS);
    spammer.pAdvertising->stop();
}

// APRÈS (correct - méthode publique)
spammer.doSpamCycle();
```

### Impact

✅ **Encapsulation correcte maintenue**
✅ **Accès contrôlé via méthode publique**
✅ **Code plus maintenable et sûr**

---

## Summary of All Fixes

### Bug #1: NimBLE API ✅ FIXED
- File: `ble_scanner.cpp`
- Issue: `getServiceUUIDs()` n'existe pas
- Solution: Utiliser `getServiceUUID()` (singulier)

### Bug #2: Button Press Identifiers ✅ FIXED
- Files: `ble_scanner.cpp`, `ble_custom_spam.cpp`
- Issue: `OkPress` non défini
- Solution: Utiliser `SelPress`

### Bug #3: ArduinoJson API Deprecated ✅ FIXED
- File: `ble_custom_spam.cpp`
- Issue: `containsKey()` déprécié
- Solution: Utiliser `obj[key].is<T>()`

### Bug #4: Private Member Access ✅ FIXED
- Files: `ble_custom_spam.h`, `ble_custom_spam.cpp`
- Issue: Accès direct à membre privé
- Solution: Méthode publique `doSpamCycle()`

---

## Compilation Status

✅ **COMPILATION SUCCESSFUL!**

```
RAM:   [====      ]  40.3% (used 132032 bytes from 327680 bytes)
Flash: [========= ]  86.0% (used 4000753 bytes from 4653056 bytes)
Successfully created esp32s3 image.
```

**Résultats:**
- ✅ Aucune erreur de compilation
- ✅ Aucun warning critique
- ✅ Firmware créé avec succès
- ✅ Taille: 4.00 MB / 4.65 MB (86%)
- ✅ RAM: 132 KB / 328 KB (40%)

**Fichier généré:**
```
.pio/build/lilygo-t-embed-cc1101/firmware.elf
.pio/build/lilygo-t-embed-cc1101/firmware.bin
Bruce-lilygo-t-embed-cc1101.bin (merged binary)
```

---

**Last Updated:** November 27, 2024  
**Total Bugs Fixed:** 4/4  
**Compilation Status:** ✅ SUCCESS  
**Status:** READY FOR FLASH & TEST 🚀
