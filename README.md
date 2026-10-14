# Phonogène (Collateral Effects) — prototipo 0.1

Emulazione del Phonogène di Pierre Schaeffer e Jacques Poullin (GRM, 1953).
Strumento VST3 / AU / Standalone con ingresso sidechain.

## Cosa fa questo prototipo
- **Nastro da file** (WAV, AIFF, FLAC…: «CHARGER» o trascinamento) oppure **da ingresso** (sidechain in Live, ingresso della scheda nello Standalone) registrato con «ENREG.» (max 30 s).
- **Chromatique**: dodici tasti = dodici capstan, rapporti 2^(n/12); un solo nastro e una sola testina, quindi monofonico con priorità all'ultimo tasto. Le ottave MIDI ricadono nell'unica ottava (interruttore «ÉTENDU» per la tastiera intera).
- **À coulisse**: velocità continua con la leva (0–2×) e pitch bend; il tasto apre il trasporto.
- **Universel**: non ancora implementato (suona come il cromatico).
- Inerzia meccanica di avvio/arresto, pleurage e flutter sulla velocità reale, soffio solo a nastro in movimento, risposta della testina e saturazione.

## Ipotesi da verificare sulle fonti
- Il tasto DO corrisponde alla velocità nominale.
- Costanti di tempo dell'innesto, entità del wow/flutter e banda passante: tarate a orecchio.

## Build
GitHub Actions (`.github/workflows/build-macos.yml`) produce `Phonogene-macOS.zip` (universale arm64/x86_64).
Locale: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
Test offline: `-DPHONOGENE_TESTS=ON`, target `PhonogeneTest`.
