# CoDec – Encodeur/Décodeur d’images DIF

## Présentation
Ce projet correspond au **projet de Programmation Avancée (L3 Informatique)**. Il consiste à développer un **CoDec (Encodeur/Décodeur)** d’images basé sur un **codage différentiel** suivi d’un **codage à longueur variable (VLC pseudo‑Huffman)**.

Le format produit, appelé **DIF**, est un format d’échange binaire défini dans le cadre du projet. L’objectif est de fournir :
- une **bibliothèque robuste** (`libCoDec.so`) implémentant l’encodeur et le décodeur,
- une **application de démonstration** simple, fiable et documentée.

Le CoDec permet :
- d’encoder des images **PNM** (PGM niveaux de gris, PPM couleurs RGB) vers le format **DIF**,
- de décoder des fichiers **DIF** vers le format **PNM**.

---

## Fonctionnalités

### Bibliothèque CoDec
La bibliothèque expose au minimum les deux fonctions suivantes :

```c
int pnmtodif(const char* pnminput, const char* difoutput);
```
- Entrée : image PNM (PGM ou PPM)
- Sortie : fichier image compressé au format DIF
- Retour : `0` en cas de succès, `>0` en cas d’erreur

```c
int diftopnm(const char* difinput, const char* pnmoutput);
```
- Entrée : fichier DIF
- Sortie : image PNM (PGM ou PPM)
- Retour : `0` en cas de succès, `>0` en cas d’erreur

La bibliothèque est compilée sous la forme d’une **bibliothèque partagée locale** :
```
libCoDec.so
```

---

### Application de démonstration
Un exécutable permet d’illustrer l’utilisation de la bibliothèque :
- compression d’images standards (PNM, PNG, JPEG, etc.) vers DIF,
- affichage du **taux de compression**,
- décompression des fichiers DIF vers PNM,
- options supplémentaires possibles :
  - `-h` : aide
  - `-v` : mode verbeux
  - `-t` : mesure du temps d’exécution
  - choix d’un visualiseur pour l’affichage des images décodées

---

## Principe de fonctionnement

1. **Transformation différentielle**
   - Les pixels sont parcourus séquentiellement.
   - On encode la différence entre deux pixels consécutifs.
   - En RGB, chaque plan (R, G, B) est traité séparément.

2. **Normalisation des valeurs**
   - Suppression du bit de poids faible (division par 2).
   - Repliement pair/impair pour transformer les valeurs signées en non signées.

3. **Quantification et VLC**
   - Utilisation d’un quantificateur à 4 intervalles.
   - Encodage via un code préfixé de type pseudo‑Huffman.

4. **Format DIF**
   - En-tête binaire (magic number, dimensions, quantificateur).
   - Stockage du premier pixel brut.
   - Données compressées stockées dans un buffer binaire.

---

## Structure du projet

```
L3Prog.NAVARRO.RAMTANI/
├── CoDec/
│   ├── src/        # Sources C de la bibliothèque (encodeur/décodeur)
│   ├── include/    # Headers de la bibliothèque
│   └── lib/        # Bibliothèque générée (libdif.so)
│
├── app/
│   ├── src/        # Application de démonstration (Python)
│
├── makelib         # Makefile pour compiler la bibliothèque CoDec
├── makeapp         # Makefile pour compiler/lancer l’application
└── README.md
```

## Compilation

### Bibliothèque
Depuis le répertoire `L3Prog.NAVARRO.RAMTANI/` :
```bash
make -f makelib libCoDec.so
```

### Application
Depuis le répertoire `L3Prog.NAVARRO.RAMTANI/` :
```bash
make -f makeapp
```

---

## Exemples d’utilisation

Compression d’une image PNM :
```bash
./main -c image.pgm image.dif
```

Décompression d’un fichier DIF :
```bash
./main -d image.dif image.pnm
```

Mode verbeux et mesure du temps :
```bash
./main -v -t image.ppm
```

---

## Répartition du travail

- **RAMTANI Walid 1** :
  - Transformation différentielle
  - Encodage VLC
  - Gestion du buffer binaire

- **NAVARRO Achille 2** :
  - Décodage VLC
  - Reconstruction de l’image
  - Application de démonstration et options CLI

---

## État du projet

✔ Encodage PNM → DIF fonctionnel  
✔ Décodage DIF → PNM fonctionnel  
✔ Gestion des images niveaux de gris et RGB  
✔ Respect du format DIF spécifié


## Auteurs

- **RAMTANI Walid 1**
- **NAVARRO  Achille**

Projet réalisé dans le cadre de la **Licence 3 Informatique – Université** (2025–2026).

# CoDec
