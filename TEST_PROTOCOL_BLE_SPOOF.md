# Protocole de Test — BLE Audio Spoof + HID Module

## ⚠️ Phase 5 : Tests critiques des popups

Cette phase détermine si l'attaque fonctionne réellement sur les téléphones cibles.

---

## 📋 Pré-requis matériel

- ✅ ESP32-S3 T-Embed CC1101 flashé avec Bruce + module BLE Spoof
- ✅ Téléphone Xiaomi avec MIUI (pour test 5.1)
- ✅ Téléphone Android stock - Pixel/Samsung/OnePlus (pour test 5.3)
- ✅ Écouteurs BLE réels (Redmi Buds 4 ou similaires) pour capturer un profil
- ✅ Application nRF Connect (optionnel mais recommandé pour vérifier les advertisements)

---

## 🎯 Objectif des tests

**But** : Vérifier si le popup de connexion BLE mentionne "clavier" ou "saisie" quand on se fait passer pour des écouteurs audio tout en annonçant le service HID.

**Résultats possibles** :
1. ✅ **Popup Audio uniquement** → L'attaque fonctionne, le HID est invisible
2. ⚠️ **Popup mentionne "clavier"** → L'attaque est détectée, il faut ajuster
3. ❌ **Erreur de connexion** → Problème technique à résoudre

---

## 📝 Test 5.1 — Popup MIUI (Xiaomi)

### Préparation

1. **Capturer un profil d'écouteurs réels** :
   ```
   Menu Bruce → Bluetooth → BLE Audio Spoof → Scan & Capture
   - Mettre les écouteurs en mode pairing
   - Scanner pendant 5 secondes
   - Sélectionner les écouteurs dans la liste
   - Choisir LittleFS ou SD Card
   - Nommer le profil (ex: "redmi_buds_4")
   - Confirmer la sauvegarde
   ```

2. **Vérifier le profil sauvegardé** :
   ```
   Menu Bruce → Bluetooth → BLE Audio Spoof → Load Profile
   - Vérifier que le profil apparaît dans la liste
   - Sélectionner pour voir les détails :
     * Nom
     * Adresse MAC
     * RSSI
     * Appearance (devrait être 0x0340 ou similaire pour audio)
   ```

### Procédure de test

1. **Éteindre les vrais écouteurs** (important !)

2. **Démarrer le spoofing sur le T-Embed** :
   ```
   Menu Bruce → Bluetooth → BLE Audio Spoof → Start Spoofing
   - Sélectionner le système de fichiers (LittleFS/SD)
   - Sélectionner le profil des écouteurs
   - Le T-Embed affiche "Spoofing active!"
   ```

3. **Sur le téléphone Xiaomi** :
   - Ouvrir Paramètres → Bluetooth
   - Chercher les appareils disponibles
   - Observer le popup qui apparaît quand les "écouteurs" sont détectés

4. **DOCUMENTER EXACTEMENT** :
   - **Prendre une capture d'écran du popup**
   - **Noter le texte exact affiché** :
     * Nom du device
     * Type d'appareil mentionné (audio/écouteurs/casque/clavier/accessoire)
     * Y a-t-il mention de "clavier", "saisie", "keyboard" ?
     * Y a-t-il un avertissement de sécurité ?
   - **Noter l'icône affichée** (casque audio ou clavier ?)

5. **Accepter la connexion** :
   - Appuyer sur "Connecter" ou "Pair"
   - Observer si la connexion s'établit
   - Vérifier dans Paramètres → Bluetooth → [Device] → Détails
     * Quels services sont listés ?
     * Voit-on "Clavier" ou "HID" dans les services ?

### Résultats à enregistrer

Remplir ce tableau dans `memory.md` :

| Aspect | Observation | ✅/⚠️/❌ |
|--------|-------------|---------|
| **Popup affiché** | [Décrire ou capture d'écran] | |
| **Texte du popup** | [Copier le texte exact] | |
| **Type de device** | Audio / Clavier / Accessoire / Autre | |
| **Mention "clavier"** | Oui / Non | |
| **Icône affichée** | 🎧 Casque / ⌨️ Clavier / Autre | |
| **Connexion réussie** | Oui / Non | |
| **Services visibles** | [Liste des services] | |
| **HID visible ?** | Oui / Non / Pas vérifié | |

---

## 📝 Test 5.2 — Tests alternatifs si HID détecté

**Si le test 5.1 montre que le HID est détecté (popup mentionne "clavier")**, essayer les variantes suivantes :

### Variante A : Changer l'ordre des services

**Hypothèse** : Peut-être que le système Android affiche le type du premier service trouvé.

**Modification à tester** :
```cpp
// Dans startSpoofing(), inverser l'ordre :
// AVANT :
pAdvertising->addServiceUUID(profile.serviceUUIDs...);  // Audio d'abord
pAdvertising->addServiceUUID(0x1812);  // HID ensuite

// APRÈS :
pAdvertising->addServiceUUID(0x1812);  // HID d'abord
pAdvertising->addServiceUUID(profile.serviceUUIDs...);  // Audio ensuite
```

### Variante B : Retarder l'annonce du HID

**Hypothèse** : Annoncer uniquement l'audio dans l'advertisement initial, ajouter le HID après connexion.

**Modification à tester** :
```cpp
// Dans startSpoofing(), NE PAS ajouter le UUID HID aux advertisements
// Le service HID existe dans le GATT mais n'est pas annoncé
// L'appareil découvrira le HID après connexion lors de la découverte de services
```

### Variante C : Class of Device différent

**Hypothèse** : Tester différentes valeurs d'appearance.

**Valeurs à tester** :
- `0x0340` : Generic Audio Headphones
- `0x0341` : In-ear Headphones
- `0x0342` : Over-ear Headphones
- `0x0343` : Headset (audio + micro)

### Variante D : Manufacturer Data spécifique

**Hypothèse** : Xiaomi détecte ses propres écouteurs via les manufacturer data et fait confiance.

**Test** :
- S'assurer que les manufacturer data capturées sont bien reproduites
- Vérifier avec nRF Connect que les données sont identiques

---

## 📝 Test 5.3 — Popup Android stock

### Téléphones à tester

- Google Pixel (Android pur)
- Samsung Galaxy (One UI)
- OnePlus (OxygenOS)

### Procédure

**Identique au Test 5.1**, mais sur un téléphone Android non-Xiaomi.

### Résultats à comparer

| Aspect | MIUI (Xiaomi) | Android Stock |
|--------|---------------|---------------|
| Popup mentionneclavier ? | [Résultat 5.1] | [Résultat 5.3] |
| Connexion réussie ? | [Résultat 5.1] | [Résultat 5.3] |
| HID visible ? | [Résultat 5.1] | [Résultat 5.3] |

**Hypothèse** : Android stock pourrait être plus ou moins strict que MIUI.

---

## 🔍 Vérification avec nRF Connect (optionnel mais recommandé)

### Installation

- Installer "nRF Connect for Mobile" depuis Play Store

### Vérification des advertisements

1. **Pendant que le T-Embed spoofing est actif** :
   - Ouvrir nRF Connect
   - Scanner les devices
   - Trouver le device spoofé

2. **Vérifier** :
   - **Complete Local Name** : Correspond au profil ?
   - **Appearance** : Valeur correcte (0x0340 ou autre) ?
   - **Manufacturer Data** : Correspond au profil ?
   - **Service UUIDs** : Liste complète visible ?
     * UUIDs du profil audio
     * 0x1812 (HID)
     * 0x180F (Battery)

3. **Se connecter via nRF Connect** :
   - Appuyer sur "Connect"
   - Explorer les services GATT
   - Vérifier que le service HID (0x1812) est bien présent avec ses caractéristiques

---

## 📊 Analyse des résultats

### Scénario 1 : Popup audio uniquement (✅ Succès !)

**Si le popup ne mentionne PAS le clavier** :
- ✅ L'attaque fonctionne !
- ✅ Le système affiche le device comme audio
- ✅ Le HID est "caché" derrière l'apparence audio
- ➡️ Passer à la Phase 6 (Injection HID)

### Scénario 2 : Popup mentionne clavier (⚠️ Détecté)

**Si le popup mentionne "clavier" ou "saisie"** :
- ⚠️ Le système détecte le service HID
- 🔄 Essayer les variantes du Test 5.2
- 📝 Documenter quelle variante fonctionne (si elle existe)
- 🤔 Peut-être que l'attaque ne fonctionne pas sur certains systèmes

### Scénario 3 : Erreur de connexion (❌ Problème technique)

**Si la connexion échoue** :
- ❌ Problème d'implémentation
- 🔍 Vérifier avec nRF Connect que les advertisements sont corrects
- 🐛 Debug nécessaire dans le code

---

## 📝 Documentation des résultats

**Tous les résultats doivent être enregistrés dans `memory.md` section "Résultats de tests".**

Format :
```markdown
### Test 5.1 - Popup MIUI (Date: YYYY-MM-DD)

**Téléphone** : Xiaomi [Modèle] (MIUI [Version])
**Profil spoofé** : [Nom du profil]
**Appearance** : 0x[Valeur]

**Popup affiché** :
[Capture d'écran ou description]

**Texte exact** :
"[Copier le texte]"

**Type de device affiché** : Audio / Clavier / Autre
**Mention clavier** : OUI / NON
**Connexion réussie** : OUI / NON

**Conclusion** : ✅ Succès / ⚠️ Détecté / ❌ Échec

**Notes supplémentaires** :
[Observations, comportements particuliers]
```

---

## ⚠️ Points critiques à observer

1. **Premier popup de détection** (le plus important)
   - C'est le moment où l'utilisateur décide de se connecter
   - Si ce popup dit "Casque audio", il va accepter
   - Si ce popup dit "Clavier", il va se méfier

2. **Icône affichée**
   - 🎧 = Audio → Bonne nouvelle
   - ⌨️ = Clavier → Mauvaise nouvelle

3. **Services après connexion**
   - Même si le popup était "Audio", vérifier ce qui est affiché dans les détails
   - Certains systèmes peuvent lister les services après coup

4. **Quick Connect popup (Xiaomi)**
   - MIUI a parfois des popups spéciaux pour les écouteurs Xiaomi
   - Observer si ce popup apparaît et ce qu'il affiche

---

## 🚀 Après les tests

**Une fois tous les tests effectués** :
- Compiler les résultats dans `memory.md`
- Mettre à jour `process.md` avec les étapes complétées
- Décider si on passe à la Phase 6 (si les tests sont positifs)
- Ou si on doit ajuster l'approche (si le HID est détecté)

---

## 📞 En cas de problème

Si les tests révèlent que l'attaque est systématiquement détectée :

**Options** :
1. Essayer toutes les variantes du Test 5.2
2. Envisager d'autres vecteurs :
   - Spoofer uniquement audio, ajouter HID après un certain temps
   - Utiliser BT Classic au lieu de BLE
   - Spoofer d'autres types de devices (souris ?)
3. Documenter la limitation dans le README du projet

---

*Fin du protocole de test Phase 5*
