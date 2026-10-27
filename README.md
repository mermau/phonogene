# Phonogène (Collateral Effects) — prototipo 0.1

Emulazione del Phonogène di Pierre Schaeffer e Jacques Poullin (GRM, 1953).
Strumento VST3 / AU / Standalone con ingresso sidechain.

## Cosa fa questo prototipo
- **Nastro da file** (WAV, AIFF, FLAC…: «CHARGER» o trascinamento) oppure **da ingresso** (sidechain in Live, ingresso della scheda nello Standalone) registrato con «ENREG.» (max 30 s).
- **Chromatique**: dodici tasti = dodici capstan, rapporti 2^(n/12); un solo nastro e una sola testina, quindi monofonico con priorità all'ultimo tasto. Le ottave MIDI ricadono nell'unica ottava (interruttore «ÉTENDU» per la tastiera intera).
- **À coulisse**: motore a velocità continua con la leva (0–2×, mod wheel) e pitch bend, con inerzia propria; il tasto innesta il nastro (l'altezza del tasto non conta, ÉTENDU non ha effetto).
- **Universel**: testina rotante (regolatore temporale di Springer). La tastiera dà l'altezza, la leva COULISSE la velocità del nastro (durata, fino al quasi congelamento), indipendenti. TÊTES (2–4 testine sul tamburo), FENÊTRE (distanza tra le testine sul nastro, 10–250 ms; 40 ms come i segmenti del regolatore di Springer), DURETÉ (da dissolvenza morbida a commutazione secca: gli artefatti di raccordo tipici della macchina). INERTIE fa scivolare altezza e velocità.
- **Nastro registrato salvato nel progetto** (FLAC nello stato del plugin, ricampionato se cambia la frequenza) ed **esportabile** in WAV o AIFF con «EXPORTER».
- **OCTAVE**: seconda ottava della tastiera (velocità ×2), come il cromatico che trasponeva «sur une à deux octaves».
- **MONO**: nastro a una traccia come l'originale. **COLLURE** (opzionale): la giunta adesiva dell'anello passa sulla testina a ogni giro.
- **Meccanica**: il soffio è inciso sul nastro e si traspone con la lettura; pleurage e scintillio seguono la velocità del motore; DÉBUT/LONGUEUR cambiano con una dissolvenza di 10 ms.
- **Anti-aliasing**: lettura con sinc finestrato a banda limitata, taglio scalato sulla velocità (fino a 4×).
- **Interfaccia ridimensionabile** (angolo in basso a destra, proporzioni fisse), dimensione salvata nel progetto; i file si caricano in background.
- **CC MIDI** su tutti i comandi, con MIDI learn dal menu del tasto destro (tabella in `INSTALL.txt`).
- Inerzia meccanica di avvio/arresto, pleurage e flutter sulla velocità reale, soffio solo a nastro in movimento, risposta della testina e saturazione.

## Ipotesi da verificare sulle fonti
- Il tasto DO corrisponde alla velocità nominale.
- Costanti di tempo dell'innesto, entità del wow/flutter e banda passante: tarate a orecchio.

## Build
GitHub Actions (`.github/workflows/build-macos.yml`) produce `Phonogene-macOS.zip` (universale arm64/x86_64).
Locale: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
Test offline: `-DPHONOGENE_TESTS=ON`, target `PhonogeneTest`.
