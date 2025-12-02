# Plan d'implémentation — BLE Audio Spoof + HID Injection pour Bruce

## Version finale pour Claude Code

---

## Règles fondamentales

### Règle 1 — Gestion du code existant

**Interdit :**
- Modifier les méthodes existantes
- Modifier la logique du code existant
- Supprimer du code existant
- Changer le comportement des fonctionnalités existantes

**Autorisé :**
- Créer de nouveaux fichiers
- Créer de nouvelles méthodes/classes
- Modifier des fichiers existants **uniquement pour ajouter** des entrées de menu
- Ajouter des imports/includes pour les nouveaux modules

**Condition obligatoire pour toute modification de fichier existant :**
1. Vérifier que la modification n'impacte pas le code existant
2. Documenter la modification dans `memory.md` (fichier, ligne, changement)
3. Documenter dans `process.md` (justification, vérification effectuée)
4. La modification doit être **additive uniquement** (ajout de lignes, pas de changement)

---

### Règle 2 — Fichiers de suivi obligatoires

**`memory.md`** — Base de connaissances persistante

Stocke :
- Toutes les informations découvertes sur l'architecture Bruce
- Les chemins des fichiers analysés et leur rôle
- Les noms des fonctions/classes réutilisables
- Les résultats de chaque test
- Les problèmes rencontrés et leurs solutions
- **Les modifications effectuées sur les fichiers existants** (fichier, ligne, avant/après)

**`process.md`** — Suivi d'avancement

Stocke :
- Liste de toutes les étapes du plan
- Statut de chaque étape : `[ ]` À faire / `[~]` En cours / `[x]` Terminé / `[!]` Bloqué
- Date/heure de complétion
- Dépendances entre étapes
- Historique des validations
- **Journal des modifications de fichiers existants** (justification, vérification)

**Workflow obligatoire pour chaque tâche :**
```
1. LIRE memory.md
2. LIRE process.md
3. EXÉCUTER la tâche
4. SI modification de fichier existant:
   a. Vérifier que le code existant n'est pas impacté
   b. Documenter dans memory.md
   c. Documenter dans process.md
5. METTRE À JOUR memory.md avec les découvertes
6. METTRE À JOUR process.md avec l'avancement
```

---

### Règle 3 — Validation utilisateur obligatoire entre phases

**Avant de passer à une nouvelle phase, Claude Code DOIT :**

1. Terminer toutes les étapes de la phase en cours
2. Mettre à jour `memory.md` et `process.md`
3. Afficher un résumé de la phase complétée
4. Demander explicitement la validation de l'utilisateur
5. **ATTENDRE** la réponse avant de continuer

**Format du message de fin de phase :**

```
═══════════════════════════════════════════
✅ PHASE [N] TERMINÉE — [Nom de la phase]
═══════════════════════════════════════════

📋 Résumé :
- [Liste des étapes complétées]

📝 Découvertes stockées dans memory.md :
- [Résumé des infos ajoutées]

🧪 Tests effectués :
- [Test 1] : ✅ OK / ❌ Échec
- [Test 2] : ✅ OK / ❌ Échec

⚠️ Problèmes rencontrés :
- [Liste ou "Aucun"]

📁 Fichiers créés :
- [Liste des nouveaux fichiers]

📝 Fichiers existants modifiés :
- [Fichier] : [Description de l'ajout] — Vérifié : ✅
- Ou "Aucun"

═══════════════════════════════════════════
➡️ Prochaine phase : [N+1] — [Nom]
   Étapes prévues : [Liste courte]

👤 EN ATTENTE DE VALIDATION
   Tapez "ok" ou "continue" pour continuer
   Tapez "détail [étape]" pour plus d'infos
   Tapez "stop" pour arrêter
═══════════════════════════════════════════
```

**Claude Code ne doit JAMAIS :**
- Passer automatiquement à la phase suivante
- Commencer une nouvelle phase sans "ok" explicite de l'utilisateur
- Ignorer une demande de détails ou d'arrêt

---

## Workflow complet

```
POUR CHAQUE PHASE:
│
├─► DÉBUT DE PHASE
│   ├── Lire memory.md
│   ├── Lire process.md
│   └── Annoncer le début de la phase
│
├─► POUR CHAQUE ÉTAPE:
│   ├── Exécuter l'étape
│   ├── SI modification fichier existant:
│   │   ├── Vérifier impact sur code existant
│   │   ├── Documenter dans memory.md
│   │   └── Documenter dans process.md
│   ├── Effectuer le test de validation (si applicable)
│   ├── Mettre à jour memory.md
│   ├── Mettre à jour process.md
│   └── Passer à l'étape suivante
│
├─► FIN DE PHASE
│   ├── Afficher le résumé de phase
│   ├── Mettre à jour process.md (phase complète)
│   └── ATTENDRE validation utilisateur ◄── BLOQUANT
│
└─► SI validation reçue ("ok" / "continue"):
    └── Passer à la phase suivante
```

---

## Objectif final du module

Créer un module dans Bruce firmware qui :

1. Scanne et capture les advertisements BLE d'écouteurs (Redmi Buds 4 ou autre)
2. Permet de spoofer ce device avec un nom/apparence identique
3. Annonce les profils Audio (A2DP/AVRCP) + HID (clavier) simultanément
4. Utilise un Class of Device "Audio" pour tromper l'affichage du popup
5. Une fois connecté, permet l'injection de frappes clavier

---

## Phase 0 — Initialisation

### Étape 0.1 — Création des fichiers de suivi

**Action :**
- Créer le fichier `memory.md` dans le projet Bruce (emplacement à déterminer selon structure du projet)
- Créer le fichier `process.md` au même endroit
- Initialiser `process.md` avec la liste complète des étapes
- Initialiser `memory.md` avec les sections vides

**Structure initiale de `memory.md` :**
```markdown
# Memory — BLE Audio Spoof + HID Module

## Architecture Bruce
<!-- Informations sur la structure du projet -->

## Stack BLE
<!-- Infos sur NimBLE/Bluedroid utilisé -->

## Fichiers pertinents
<!-- Liste des fichiers analysés avec leur rôle -->

## Fonctions réutilisables
<!-- Fonctions existantes qu'on peut appeler -->

## Classes réutilisables
<!-- Classes existantes qu'on peut instancier -->

## Système de menus
<!-- Navigation, sélection, retour, annulation -->

## Système de stockage
<!-- SPIFFS/LittleFS/SD, fonctions, formats -->

## Résultats de tests
<!-- Résultats de chaque test effectué -->

## Problèmes rencontrés
<!-- Problèmes et leurs solutions -->

## Modifications de fichiers existants
<!-- Journal de toutes les modifications sur des fichiers existants -->
| Fichier | Ligne | Type | Description | Impact vérifié |
|---------|-------|------|-------------|----------------|
| — | — | — | — | — |

## Notes techniques
<!-- Autres informations utiles -->
```

**Structure initiale de `process.md` :**
```markdown
# Process — Avancement du projet

## Statut global
- Phase actuelle : 0
- Dernière mise à jour : [date]
- Prochaine étape : 0.1
- En attente de validation : NON

## Légende
- [ ] À faire
- [~] En cours
- [x] Terminé
- [!] Bloqué
- [⏸️] En attente validation utilisateur

## Étapes

### Phase 0 — Initialisation
- [ ] 0.1 — Création des fichiers de suivi
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 1

### Phase 1 — Analyse (lecture seule)
- [ ] 1.1 — Cartographie des modules BLE
- [ ] 1.2 — Analyse du module badBLE
- [ ] 1.3 — Analyse du module BLE Scan
- [ ] 1.4 — Analyse du système de menus (structure, navigation, sélection, retour, annulation)
- [ ] 1.5 — Analyse du système de stockage
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 2

### Phase 2 — Module de capture
- [ ] 2.1 — Création des fichiers du nouveau module
- [ ] 2.2 — Structure de données pour profil de device
- [ ] 2.3 — Méthode de scan et capture
- [ ] 2.4 — Méthode de sauvegarde de profil
- [ ] 2.5 — Méthode de chargement de profil
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 3

### Phase 3 — Module de spoofing
- [ ] 3.1 — Méthode de changement de nom BLE
- [ ] 3.2 — Méthode de changement de MAC
- [ ] 3.3 — Méthode de configuration du Class of Device
- [ ] 3.4 — Méthode de configuration des advertisements
- [ ] 3.5 — Méthode de démarrage du spoof
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 4

### Phase 4 — Multi-profils Audio + HID
- [ ] 4.1 — Initialisation des services GATT
- [ ] 4.2 — Ajout du service HID
- [ ] 4.3 — Ajout du service Battery
- [ ] 4.4 — Configuration du pairing
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 5

### Phase 5 — Tests popup (CRITIQUE)
- [ ] 5.1 — Test popup MIUI
- [ ] 5.2 — Tests alternatifs si HID détecté
- [ ] 5.3 — Test popup Android stock
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 6

### Phase 6 — Injection HID
- [ ] 6.1 — Méthode d'envoi de touche
- [ ] 6.2 — Méthode d'envoi de chaîne
- [ ] 6.3 — Méthode d'exécution de payload DuckyScript
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 7

### Phase 7 — Interface utilisateur
- [ ] 7.1 — Intégration menu Bruce (modification fichier existant)
- [ ] 7.2 — Écran de statut
- [ ] 7.3 — Écran sélection payload
- [ ] ⏸️ VALIDATION REQUISE AVANT PHASE 8

### Phase 8 — Tests finaux
- [ ] 8.1 — Test complet Xiaomi
- [ ] 8.2 — Test complet Android stock
- [ ] 8.3 — Test persistance/reconnexion
- [ ] ⏸️ VALIDATION FINALE — PROJET TERMINÉ

## Historique des validations
| Phase | Date | Validé par | Notes |
|-------|------|------------|-------|
| — | — | — | — |

## Journal des modifications de fichiers existants
| Date | Fichier | Modification | Justification | Vérification |
|------|---------|--------------|---------------|--------------|
| — | — | — | — | — |
```

**Résultat attendu :**
- Les deux fichiers existent et sont initialisés

---

## Phase 1 — Analyse de l'architecture (LECTURE SEULE)

> ⚠️ Cette phase est en **lecture seule**. Aucun fichier ne doit être créé ou modifié (sauf memory.md et process.md).

### Étape 1.1 — Cartographie des modules BLE

**Action :**
- Explorer la structure du projet Bruce
- Identifier tous les fichiers liés au BLE
- Noter les noms de fichiers, leur chemin, leur rôle apparent
- Identifier le stack BLE utilisé (NimBLE, Bluedroid, ou autre)

**À stocker dans `memory.md` :**
- Liste des fichiers BLE avec chemin complet
- Stack BLE identifié
- Fichiers de configuration BLE
- Dépendances BLE

**Résultat attendu :**
- Section "Fichiers pertinents" remplie avec les fichiers BLE
- Section "Stack BLE" remplie

---

### Étape 1.2 — Analyse du module badBLE existant

**Action :**
- Localiser le code du module badBLE dans le projet
- Lire les fichiers SANS les modifier
- Identifier :
  - Comment le clavier BLE est initialisé
  - Comment les touches sont envoyées
  - Comment les payloads sont chargés
  - Les fonctions publiques appelables
  - Les dépendances utilisées

**À stocker dans `memory.md` :**
- Chemin des fichiers badBLE
- Nom de la classe/structure principale
- Liste des méthodes publiques avec leur signature
- Dépendances utilisées (librairies)
- Notes sur le fonctionnement interne

**Résultat attendu :**
- Compréhension du badBLE existant sans le modifier
- Liste des fonctions qu'on pourra appeler depuis notre nouveau module

---

### Étape 1.3 — Analyse du module BLE Scan existant

**Action :**
- Localiser le code du scanner BLE dans le projet
- Lire les fichiers SANS les modifier
- Identifier :
  - Comment le scan est lancé
  - Quelles données sont récupérées
  - Comment les résultats sont stockés
  - Les callbacks utilisés
  - Les structures de données utilisées

**À stocker dans `memory.md` :**
- Chemin des fichiers BLE Scan
- Nom de la classe/structure principale
- Liste des méthodes publiques avec leur signature
- Structure des données scannées
- Notes sur le fonctionnement interne

**Résultat attendu :**
- Compréhension du scanner existant
- Identification de ce qu'on peut réutiliser vs ce qu'on doit recréer

---

### Étape 1.4 — Analyse du système de menus

**Action :**
- Localiser le code gérant les menus de Bruce
- Identifier :
  - Comment un nouveau menu est ajouté
  - Comment les sous-menus fonctionnent
  - Comment les actions sont liées aux items de menu
  - Le point d'entrée pour ajouter notre module
  - **La structure exacte pour ajouter une entrée sans casser l'existant**

- **Analyser la navigation :**
  - Comment l'utilisateur navigue entre les items (haut/bas, gauche/droite)
  - Quels boutons/inputs sont utilisés pour la navigation
  - Comment le focus/sélection est géré visuellement

- **Analyser la sélection :**
  - Comment un item est sélectionné/validé
  - Quel bouton/input déclenche la sélection
  - Comment une action est exécutée après sélection

- **Analyser le retour et l'annulation :**
  - Comment l'utilisateur revient au menu précédent
  - Quel bouton/input déclenche le retour
  - Comment l'annulation d'une action est gérée
  - Y a-t-il une différence entre "retour" et "annuler" ?

- **Analyser les patterns d'interaction :**
  - Comment les menus avec saisie de texte fonctionnent
  - Comment les listes déroulantes/sélections fonctionnent
  - Comment les confirmations (Oui/Non) sont gérées

**À stocker dans `memory.md` :**
- Chemin des fichiers de menu
- Méthode/pattern pour ajouter un nouveau menu
- Structure d'un item de menu
- Fichier(s) à modifier pour l'intégration
- Ligne(s) exacte(s) où ajouter l'entrée
- Format/syntaxe de l'entrée à ajouter
- Exemples d'entrées existantes
- **Mapping des boutons/inputs :**
  - Bouton navigation haut
  - Bouton navigation bas
  - Bouton navigation gauche (si applicable)
  - Bouton navigation droite (si applicable)
  - Bouton sélection/validation
  - Bouton retour
  - Bouton annulation (si différent de retour)
- **Fonctions de navigation à réutiliser :**
  - Fonction pour afficher un menu
  - Fonction pour gérer la navigation
  - Fonction pour gérer la sélection
  - Fonction pour revenir en arrière
  - Fonction pour annuler une action
- **Patterns UI existants à réutiliser :**
  - Pattern pour écran de liste
  - Pattern pour écran de saisie texte
  - Pattern pour écran de confirmation
  - Pattern pour écran de statut
  - Pattern pour afficher un message/alerte

**Résultat attendu :**
- Savoir exactement où et comment intégrer notre nouveau module dans le menu
- Comprendre comment implémenter la navigation dans nos propres écrans
- Documentation précise pour la modification future (Phase 7)
- Liste des fonctions de navigation réutilisables

---

### Étape 1.5 — Analyse du système de stockage

**Action :**
- Identifier comment Bruce gère le stockage
- Localiser les fonctions de lecture/écriture de fichiers
- Identifier le format utilisé pour les configs

**À stocker dans `memory.md` :**
- Système de fichiers utilisé (SPIFFS, LittleFS, SD, ou autre)
- Fonctions de stockage disponibles avec signatures
- Chemin racine du stockage
- Format des fichiers de config (JSON, texte, binaire, etc.)
- Exemples de fichiers de config existants

**Résultat attendu :**
- Savoir comment sauvegarder nos profils de devices

---

## Phase 2 — Création du module de capture

> À partir de cette phase, on CRÉE de nouveaux fichiers. On ne modifie RIEN d'existant.

### Étape 2.1 — Création des fichiers du nouveau module

**Action :**
- Créer les fichiers header et source pour le nouveau module
- Choisir l'emplacement en cohérence avec la structure du projet (identifiée en Phase 1)
- Définir la classe/structure principale
- Inclure les headers nécessaires (identifiés en Phase 1)

**Résultat attendu :**
- Fichiers créés et compilables (même vides)
- Classe/structure principale déclarée

**Validation :**
- Le projet compile sans erreur

---

### Étape 2.2 — Structure de données pour profil de device

**Action :**
- Créer une structure/classe pour stocker un profil de device capturé
- Attributs nécessaires : nom, MAC, manufacturer data, service UUIDs, Class of Device, RSSI, raw advertisement data

**Résultat attendu :**
- Structure de données complète pour représenter un device BLE

---

### Étape 2.3 — Méthode de scan et capture

**Action :**
- Implémenter une méthode qui scanne les devices BLE
- Utiliser les fonctions BLE existantes de Bruce (identifiées en Phase 1) OU réimplémenter si nécessaire
- Stocker les résultats dans une liste de profils
- Afficher les résultats sur l'écran du T-Embed

**Résultat attendu :**
- Le scan fonctionne et liste les devices trouvés
- Les informations complètes sont capturées (pas juste le nom)

**Test de validation (par l'utilisateur) :**
- Mettre des écouteurs BLE en mode pairing
- Lancer le scan
- Vérifier que le device apparaît avec toutes ses infos

---

### Étape 2.4 — Méthode de sauvegarde de profil

**Action :**
- Implémenter la sauvegarde d'un profil de device en fichier
- Utiliser le système de stockage identifié en Phase 1
- Format recommandé : JSON pour lisibilité
- Créer un dossier dédié pour les profils

**Résultat attendu :**
- Un profil peut être sauvegardé sur le stockage

**Test de validation (par l'utilisateur) :**
- Scanner un device
- Sauvegarder son profil
- Vérifier que le fichier existe et contient les bonnes données

---

### Étape 2.5 — Méthode de chargement de profil

**Action :**
- Implémenter le chargement d'un profil depuis un fichier
- Parser les données et recréer un profil de device
- Gérer les erreurs (fichier inexistant, format invalide)

**Résultat attendu :**
- Un profil peut être rechargé après reboot

**Test de validation (par l'utilisateur) :**
- Rebooter le T-Embed
- Charger un profil sauvegardé
- Vérifier que les données sont correctes

---

## Phase 3 — Module de spoofing BLE

### Étape 3.1 — Méthode de changement de nom BLE

**Action :**
- Méthode pour changer le nom BLE annoncé
- Utiliser les API du stack BLE identifié en Phase 1

**Résultat attendu :**
- Le nom BLE peut être changé dynamiquement

**Test de validation (par l'utilisateur) :**
- Définir un nom de test
- Scanner depuis un téléphone
- Vérifier que le nom apparaît correctement

---

### Étape 3.2 — Méthode de changement de MAC

**Action :**
- Méthode pour spoofer ou randomiser la MAC address
- Attention : certaines opérations nécessitent un redémarrage du stack BLE

**Résultat attendu :**
- La MAC peut être modifiée

**Test de validation (par l'utilisateur) :**
- Définir une MAC spécifique
- Vérifier avec un scanner BLE que la MAC correspond

---

### Étape 3.3 — Méthode de configuration du Class of Device

**Action :**
- Méthode pour définir le Class of Device
- Implémenter des presets : Audio Headphones (0x240404), Audio Speaker, etc.

**Résultat attendu :**
- Le CoD peut être configuré

**Test de validation (par l'utilisateur) :**
- Définir CoD = Audio Headphones
- Chercher depuis Android
- Vérifier l'icône affichée (doit être casque/écouteurs)

---

### Étape 3.4 — Méthode de configuration des advertisements

**Action :**
- Méthode pour définir les données d'advertisement brutes
- Permettre de reproduire exactement les advertisements capturés
- Inclure : manufacturer data, service UUIDs, TX power

**Résultat attendu :**
- Les advertisements peuvent être personnalisés

**Test de validation (par l'utilisateur) :**
- Charger un profil d'écouteurs capturé
- Appliquer ses advertisement data
- Vérifier avec nRF Connect que les données correspondent

---

### Étape 3.5 — Méthode de démarrage du spoof

**Action :**
- Méthode qui combine tout : applique nom, MAC, CoD, advertisements
- Démarre le BLE advertising
- Gère le cycle de vie (start/stop)

**Résultat attendu :**
- Le spoof complet peut être lancé en un appel

**Test de validation (par l'utilisateur) :**
- Charger un profil
- Démarrer le spoof
- Vérifier que le device apparaît correctement sur un téléphone

---

## Phase 4 — Implémentation multi-profils (Audio + HID)

### Étape 4.1 — Initialisation des services GATT

**Action :**
- Initialiser la structure GATT pour supporter plusieurs services
- Préparer l'ajout de HID, Battery, Device Info

**Résultat attendu :**
- Base GATT prête pour les services

---

### Étape 4.2 — Ajout du service HID

**Action :**
- Ajouter le service HID over GATT (UUID 0x1812)
- Caractéristiques nécessaires : Report Map, Report, HID Information, HID Control Point, Protocol Mode
- Report Map pour un clavier standard

**Résultat attendu :**
- Service HID exposé et fonctionnel

**Test de validation (par l'utilisateur) :**
- Connecter avec nRF Connect
- Vérifier que le service HID est visible avec toutes ses caractéristiques

---

### Étape 4.3 — Ajout du service Battery

**Action :**
- Ajouter le service Battery (UUID 0x180F)
- Caractéristique Battery Level

**Résultat attendu :**
- Service Battery exposé (attendu par les écouteurs)

---

### Étape 4.4 — Configuration du pairing

**Action :**
- Configurer le mode de pairing "Just Works"
- IO Capabilities : NoInputNoOutput
- Acceptation automatique des demandes

**Résultat attendu :**
- Pairing sans PIN

**Test de validation (par l'utilisateur) :**
- Initier le pairing depuis un téléphone
- Vérifier qu'aucun PIN n'est demandé

---

## Phase 5 — Tests critiques du popup

> ⚠️ Phase critique — Les résultats déterminent la viabilité de l'attaque

### Étape 5.1 — Test popup MIUI (Xiaomi)

**Action :**
- Configurer le T-Embed : nom d'écouteurs, CoD Audio, profils Audio+HID
- Observer et documenter exactement le popup affiché

**À stocker dans `memory.md` :**
- Description exacte du popup
- Texte affiché mot pour mot
- Si mention de "clavier" ou "saisie"
- Comportement après acceptation

**Résultats possibles et actions :**

| Résultat | Action suivante |
|----------|-----------------|
| Pas de mention clavier | ✅ Continuer Phase 6 |
| Mention clavier | Tester Étape 5.2 alternatives |
| Quick Connect Xiaomi | Documenter et tester HID |
| Erreur connexion | Debug |

---

### Étape 5.2 — Tests alternatifs si HID détecté

**Si le popup mentionne "clavier", tester :**

1. CoD différent (variations de Minor Device Class)
2. Ordre des services (Battery d'abord, HID ensuite)
3. Nom générique au lieu du nom d'écouteurs
4. Retarder l'annonce HID (ajouter après connexion audio)
5. BLE-only vs BT Classic

**À stocker dans `memory.md` :**
- Chaque combinaison testée et son résultat exact

---

### Étape 5.3 — Test popup Android stock

**Action :**
- Même tests sur un téléphone Android non-Xiaomi (Pixel, Samsung, etc.)
- Documenter les différences de comportement

**À stocker dans `memory.md` :**
- Comportement sur Android stock vs MIUI
- Différences dans l'affichage des popups

---

## Phase 6 — Injection HID

### Étape 6.1 — Méthode d'envoi de touche

**Action :**
- Méthode pour envoyer une touche unique
- Paramètres : keycode, modifiers (Ctrl, Shift, Alt, GUI)
- Gestion press/release

**Résultat attendu :**
- Une touche peut être envoyée

**Test de validation (par l'utilisateur) :**
- Ouvrir Notes sur le téléphone
- Envoyer une touche
- Vérifier que le caractère apparaît

---

### Étape 6.2 — Méthode d'envoi de chaîne

**Action :**
- Méthode pour envoyer une chaîne de caractères
- Conversion caractère → keycode
- Délai configurable entre les touches

**Résultat attendu :**
- Une chaîne complète peut être tapée

**Test de validation (par l'utilisateur) :**
- Envoyer une phrase de test
- Vérifier l'affichage dans Notes

---

### Étape 6.3 — Méthode d'exécution de payload DuckyScript

**Action :**
- Parser de scripts DuckyScript
- Charger depuis fichier sur le stockage
- Exécuter les commandes : STRING, DELAY, GUI, ENTER, TAB, etc.

**Résultat attendu :**
- Payloads automatisés fonctionnels

**Test de validation (par l'utilisateur) :**
- Créer un payload simple
- Exécuter depuis le T-Embed
- Vérifier le comportement sur le téléphone

---

## Phase 7 — Interface utilisateur

### Étape 7.1 — Intégration menu Bruce

> ⚠️ Cette étape implique la modification de fichier(s) existant(s)

**Action :**
- Utiliser les informations de Phase 1.4 pour identifier le fichier et l'emplacement exact
- Ajouter une entrée pour le nouveau module dans le menu
- **Ne modifier QUE l'ajout de l'entrée, rien d'autre**
- Créer les sous-menus comme nouvelles fonctions dans les nouveaux fichiers
- Utiliser les patterns de navigation identifiés en Phase 1.4

**Vérification obligatoire avant modification :**
1. Relire memory.md pour les infos de Phase 1.4
2. Lire le fichier complet à modifier
3. Identifier la structure exacte
4. Vérifier que l'ajout ne casse pas la syntaxe
5. Vérifier que les autres entrées restent intactes

**Documentation obligatoire dans `memory.md` :**
```markdown
## Modification : Intégration menu

**Fichier** : [chemin exact]
**Ligne** : [numéro]
**Avant** : [état avant modification - contexte]
**Après** : [état après modification - contexte]
**Type** : Ajout d'entrée de menu
**Impact sur existant** : Aucun (ajout uniquement)
```

**Documentation obligatoire dans `process.md` :**
```markdown
## Journal modification

**Date** : [date]
**Fichier** : [chemin]
**Modification** : Ajout entrée menu
**Justification** : Intégration du nouveau module dans l'UI
**Vérification** :
- [ ] Syntaxe valide
- [ ] Autres entrées intactes
- [ ] Compilation OK
- [ ] Menus existants fonctionnels
- [ ] Navigation fonctionne (haut/bas/sélection/retour)
```

**Structure du menu à créer :**
```
[Nom du module]
├── Scan & Capture
│   └── [Liste des devices trouvés]
│       └── Save Profile
├── Saved Profiles
│   └── [Liste des profils]
│       ├── Load & Spoof
│       └── Delete
├── Manual Config
│   ├── Device Name
│   ├── MAC Address
│   └── Class of Device
├── Start Spoof
│   └── [Affichage statut]
├── HID Inject
│   ├── Type Text
│   ├── Run Payload
│   └── Single Key
└── Stop
```

**Résultat attendu :**
- Le module est accessible depuis le menu Bruce
- Tous les menus existants fonctionnent toujours
- La navigation (haut/bas/sélection/retour/annulation) fonctionne correctement

---

### Étape 7.2 — Écran de statut

**Action :**
- Créer une fonction d'affichage du statut en temps réel
- Utiliser les patterns UI identifiés en Phase 1.4
- Implémenter la navigation/retour selon les patterns existants
- Informations affichées :
  - Mode actuel (Scan / Spoof / Connecté)
  - Nom spoofé
  - MAC utilisée
  - État du pairing : En attente / Paired / Connecté
  - État HID : Actif / Inactif

**Résultat attendu :**
- L'utilisateur voit l'état en permanence sur l'écran du T-Embed
- Le retour au menu précédent fonctionne

---

### Étape 7.3 — Écran de sélection de payload

**Action :**
- Lister les fichiers de payload dans le dossier dédié
- Utiliser les patterns de liste identifiés en Phase 1.4
- Permettre la navigation et sélection
- Afficher un aperçu du contenu
- Option pour exécuter
- Implémenter le retour/annulation

**Résultat attendu :**
- Interface pour choisir et lancer un payload DuckyScript
- Navigation fluide (sélection, retour, annulation)

---

## Phase 8 — Tests finaux d'intégration

### Étape 8.1 — Test complet sur téléphone Xiaomi

**Scénario complet :**
1. Scanner des écouteurs BLE avec le T-Embed
2. Sauvegarder le profil
3. Éteindre les vrais écouteurs
4. Charger le profil et démarrer le spoof
5. Sur le téléphone Xiaomi : chercher les appareils Bluetooth
6. **Observer et documenter le popup affiché**
7. Accepter la connexion
8. Vérifier que le HID est connecté (Paramètres > Bluetooth > Device)
9. Ouvrir une app Notes
10. Injecter du texte depuis le T-Embed
11. Exécuter un payload DuckyScript

**À stocker dans `memory.md` :**
- Résultat détaillé de chaque étape
- Problèmes rencontrés
- Succès ou échec final

**Résultat attendu :**
- Le texte apparaît dans Notes
- L'utilisateur n'a pas eu d'avertissement explicite sur le clavier

---

### Étape 8.2 — Test complet sur Android stock

**Action :**
- Même scénario que 8.1 sur un téléphone Android non-Xiaomi (Pixel, Samsung, OnePlus, etc.)
- Documenter les différences de comportement

**À stocker dans `memory.md` :**
- Comparaison MIUI vs Android stock

---

### Étape 8.3 — Test de persistance et reconnexion

**Scénario :**
1. Établir une connexion complète avec injection HID fonctionnelle
2. S'éloigner ou désactiver le spoof
3. Attendre 1 minute
4. Réactiver le spoof
5. Vérifier si le téléphone se reconnecte automatiquement
6. Vérifier si le HID fonctionne toujours

**À stocker dans `memory.md` :**
- Comportement de reconnexion
- Délai de reconnexion
- État du HID après reconnexion

**Résultat attendu :**
- Le téléphone se reconnecte automatiquement au "faux" device
- Le HID reste fonctionnel

---

## Résumé des fichiers à créer

| Type | Description |
|------|-------------|
| Fichiers de suivi | memory.md, process.md |
| Module principal | Header et source de la classe principale |
| Structure de données | Header pour le profil de device |
| Interface utilisateur | Header et source pour les menus et écrans |
| Dossiers de données | Dossier pour profils sauvegardés, dossier pour payloads |

> Les noms et chemins exacts seront déterminés par Claude Code en Phase 1 selon la structure du projet Bruce.

---

## Résumé des modifications de fichiers existants

| Type | Description | Contrainte |
|------|-------------|------------|
| Fichier menu | Ajout d'une entrée pour le nouveau module | Ajout uniquement, aucune modification du code existant |

> Le fichier exact sera identifié par Claude Code en Phase 1.4

**Règles strictes :**
- Modification = ajout de lignes uniquement
- Aucune modification du code existant
- Documentation obligatoire avant/après dans memory.md
- Vérification obligatoire que l'existant fonctionne toujours
- Vérification que la navigation fonctionne toujours

---

## Résultat final attendu

Un module Bruce complet qui permet :

1. **Capture** — Scanner et sauvegarder le profil BLE de n'importe quels écouteurs
2. **Spoof** — Se faire passer pour ces écouteurs (nom, apparence, advertisements)
3. **Camouflage** — Apparaître comme un device audio dans le popup de pairing
4. **Injection** — Une fois connecté, envoyer des frappes clavier au téléphone
5. **Payloads** — Exécuter des scripts DuckyScript automatisés

**Vecteur d'attaque final :**
> L'attaquant capture le profil des écouteurs de la victime, attend qu'elle cherche à se connecter, présente un faux device identique, et une fois la connexion acceptée, injecte des commandes clavier à l'insu de l'utilisateur.

---

## Notes pour Claude Code

- L'environnement de dev est déjà en place, Bruce est déjà cloné
- Le module badBLE existe déjà dans Bruce (pas badBT)
- **Aucun chemin, nom de fichier ou nom de méthode n'est présumé** — tout doit être découvert en Phase 1
- Le code doit s'intégrer dans l'architecture existante de Bruce
- Réutiliser au maximum les composants existants en les **appelant**, pas en les modifiant
- Respecter le style de code du projet
- L'utilisateur compilera via PlatformIO
- Pas de dépendances externes non présentes dans Bruce
- Tester sur ESP32-S3 (T-Embed CC1101 Plus)
- **Toujours lire memory.md et process.md avant chaque action**
- **Toujours mettre à jour memory.md et process.md après chaque action**
- **Toujours attendre la validation utilisateur entre chaque phase**
- **Documenter toute modification de fichier existant dans les deux fichiers de suivi**
- **Vérifier que les modifications n'impactent pas le code existant**
- **Respecter les patterns de navigation existants (haut/bas/sélection/retour/annulation)**
