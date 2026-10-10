# Pacchetti Linux (Izanagi / Cypher)

Guida passo-passo per creare il `.deb` su **Ubuntu/Debian** (incluso il **NUC**).

---

## 1. Prerequisiti (una tantum)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev dpkg-dev
```

Oppure dalla root del repo:

```bash
./scripts/install-deps.sh
```

---

## 2. Codice aggiornato

```bash
cd ~/src/izanagi   # il tuo clone
git checkout main
git pull origin main
```

---

## 3. Build e test (consigliato)

```bash
./scripts/build.sh --test
```

Opzionale (mondo di esempio nel pacchetto, come in CI):

```bash
./scripts/fetch-test-data.sh
```

---

## 4. Creare il pacchetto Izanagi

```bash
./scripts/package-deb.sh
```

Lo script:

- compila (salvo `--no-build`);
- prepara `dist/sample-mudroot`;
- produce **`dist/izanagi_<versione>_<arch>.deb`** (es. `izanagi_0.1.42_amd64.deb`).

Solo Cypher:

```bash
./scripts/package-deb-translate.sh
```

### Opzioni utili

| Opzione / variabile | Effetto |
|---------------------|---------|
| `./scripts/package-deb.sh --no-build` | Usa `build/` esistente |
| `NEBBIE_VERSION=0.2.0 ./scripts/package-deb.sh` | Versione nel nome `.deb` |

---

## 5. Installare sul NUC (prova)

```bash
sudo dpkg -i dist/izanagi_*.deb
sudo apt-get install -f
izanagi    # GUI
nebbiedit info /usr/share/nebbie-editor/sample-mudroot/lib
```

---

## 6. Distribuire senza CI

Copia il file `.deb` dove serve (rete, USB, server). Non serve il sorgente sulla macchina di destinazione.

---

## 7. Alternativa: CI GitHub (nessun build sul NUC)

Dopo `git push` su `main`, il workflow **Izanagi (release packages)** o **Izanagi & Cypher (packages)** produce il `.deb` su `ubuntu-latest`. Scarica l’artifact Linux dalla pagina Actions (vedi [GUIDA_PACCHETTI.md](GUIDA_PACCHETTI.md)).

---

## Problemi comuni

| Problema | Soluzione |
|----------|-----------|
| `GUI binary missing` | Installa `qt6-base-dev`, ricompila con `./scripts/build.sh` |
| `dpkg-deb not found` | `sudo apt-get install dpkg-dev` |
| Qt runtime sul PC di installazione | Il `.deb` dichiara dipendenze `libqt6*`; `apt-get install -f` dopo `dpkg -i` |

Vedi anche [PLATFORM.md](PLATFORM.md).
