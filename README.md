# Catalogo componenti elettronici

Programma in C++ (C++17, da terminale) per catalogare e gestire i componenti
elettronici del proprio laboratorio: resistori, condensatori, integrati, LED,
moduli e così via.

## Funzionalità

- **Archivio** dei componenti con categoria, sigla/descrizione, valore, package,
  quantità, scorta minima, posizione (cassetto, scatola…), fornitore, prezzo e note.
- **Valori in notazione ingegneristica**: si possono scrivere come si trovano
  sugli schemi (`4k7`, `2M2`, `220R`, `4R7`, `100n`, `0,1uF`, `4µ7`, `1mH`…).
  Il programma li interpreta e li mostra in modo uniforme (`4,7 kΩ`, `100 nF`).
- **Ricerca** per testo, categoria e intervallo di valori
  (es. tutti i resistori tra 1 kΩ e 10 kΩ). Cercando `4k7` si trova anche un
  resistore inserito come `4700` o `4,7k`.
- **Prelievi e carichi** rapidi (`-5`, `+100`, `=40`), con avviso quando si
  scende sotto la scorta minima.
- **Scorte in esaurimento**: elenco dei componenti da riordinare.
- **Statistiche**: numero di articoli, pezzi totali e valore del magazzino,
  anche per categoria.
- **Codice colori dei resistori** (3, 4 e 5 bande), nei due sensi: dai colori
  al valore e dal valore ai colori. Mostra anche se quel resistore è in magazzino.
- **Controllo dei doppioni**: se si inserisce un componente già presente
  (anche con il valore scritto in un altro modo), propone di sommare la quantità.
- **Salvataggio automatico** dopo ogni modifica, in un file CSV che si può
  aprire anche con Excel o LibreOffice Calc.

## Compilazione

Serve un compilatore C++17 (GCC 9+, Clang 9+, Visual Studio 2019+).

### Con CMake (Linux, macOS, Windows)

```sh
cmake -S . -B build
cmake --build build --config Release
```

L'eseguibile si trova in `build/catalogo` (su Windows `build\Release\catalogo.exe`
con Visual Studio, `build\catalogo.exe` con MinGW).

Per eseguire i test automatici:

```sh
ctest --test-dir build -C Release
```

### Senza CMake, con un solo comando

```sh
g++ -std=c++17 -O2 -Iinclude src/*.cpp -o catalogo
```

Su Windows con MinGW il comando è lo stesso (si ottiene `catalogo.exe`).

## Utilizzo

```sh
./catalogo                       # usa il file componenti.csv nella cartella corrente
./catalogo laboratorio.csv       # usa un file a scelta
./catalogo esempi/componenti_esempio.csv   # prova con i dati di esempio
```

Si apre il menu principale:

```
==============================================
     CATALOGO COMPONENTI ELETTRONICI
==============================================
 Archivio: componenti.csv (23 componenti)
 ! 4 componenti sotto la scorta minima
----------------------------------------------
  1) Elenco componenti
  2) Cerca
  3) Scheda di un componente
  4) Aggiungi componente
  5) Modifica componente
  6) Preleva / carica quantità
  7) Elimina componente
  8) Scorte in esaurimento
  9) Statistiche
 10) Codice colori resistori
  0) Esci
```

Alcuni suggerimenti:

- Quando il programma chiede un componente si può scrivere il suo **ID**
  oppure una parola da cercare (es. `ne555`): se c'è un solo risultato viene
  proposto direttamente.
- Durante la modifica, **Invio** mantiene il valore attuale e **`-`** cancella il campo.
- Prezzi e decimali si possono scrivere con la virgola o con il punto.
- Un componente è "sotto scorta" quando la quantità è pari o inferiore alla
  scorta minima (con scorta minima 0 viene segnalato solo quando è esaurito).

Esempio di elenco:

```
 ID  Categoria     Sigla/Descrizione        Valore   Package  Q.tà  Min  Posizione
 --------------------------------------------------------------------------------------------
  1  Resistore     Film metallico 1/4 W 1%  220 Ω    THT        80   20  Cassettiera A, cassetto 1
  3  Resistore     Film metallico 1/4 W 1%  4,7 kΩ   THT      15 !   20  Cassettiera A, cassetto 3
  6  Condensatore  Ceramico multistrato     100 nF   THT       150   30  Cassettiera B, cassetto 1
  9  Condensatore  Elettrolitico 16 V       1000 µF  THT       4 !    5  Cassettiera B, cassetto 5
```

### Notazione dei valori

| Prefisso | Fattore | Esempi |
|---|---|---|
| `p` | 10⁻¹² | `22p` = 22 pF |
| `n` | 10⁻⁹ | `100n` = 100 nF |
| `u` oppure `µ` | 10⁻⁶ | `4u7` = 4,7 µF |
| `m` | 10⁻³ | `1m` = 1 mH |
| `R` | 1 | `4R7` = 4,7 Ω, `220R` = 220 Ω |
| `k` | 10³ | `4k7` = 4,7 kΩ |
| `M` | 10⁶ | `2M2` = 2,2 MΩ |
| `G` | 10⁹ | |

Attenzione alle maiuscole: `m` minuscola è *milli*, `M` maiuscola è *mega*.
L'unità di misura finale (`Ω`, `ohm`, `F`, `H`…) è facoltativa.

## Il file dei dati

I dati sono salvati in un file CSV con separatore `;` e decimali con la virgola,
il formato che Excel usa con le impostazioni italiane:

```
id;categoria;nome;valore;package;quantita;scorta_minima;posizione;fornitore;prezzo_unitario;note
1;Resistore;Film metallico 1/4 W 1%;220;THT;80;20;Cassettiera A, cassetto 1;;0,02;Per LED a 5 V
```

Il file si può modificare anche a mano o con un foglio di calcolo (per esempio
per importare molti componenti in una volta): al successivo avvio il programma
segnala eventuali righe non valide. Il salvataggio avviene prima su un file
temporaneo e poi sostituisce l'originale, così un'interruzione improvvisa non
rovina l'archivio. Conviene comunque farne ogni tanto una copia di sicurezza.

Categorie disponibili: Resistore, Condensatore, Induttore, Diodo, LED,
Transistor, Circuito integrato, Connettore, Elettromeccanico, Sensore, Modulo, Altro.

## Struttura del progetto

```
include/        intestazioni (.h)
src/
  main.cpp        avvio del programma
  app.cpp         menu e dialogo con l'utente
  console.cpp     lettura dell'input da tastiera
  inventory.cpp   archivio: ricerca, ordinamento, statistiche, lettura/scrittura CSV
  component.cpp   componente e categorie
  units.cpp       notazione ingegneristica (4k7, 100n…)
  colorcode.cpp   codice colori dei resistori
  csv.cpp         formato CSV
  textutil.cpp    funzioni di supporto per testo e numeri
tests/          test automatici
esempi/         archivio di esempio
```
