# Firma e notarizzazione macOS (Nebbie Translate)

Per aprire l’app **senza** passare da Impostazioni → Privacy e sicurezza, Apple richiede:

1. **Apple Developer Program** (account a pagamento, ~99 USD/anno)
2. Certificato **Developer ID Application**
3. **Notarizzazione** del `.app` con `notarytool`
4. **Stapling** del ticket Apple sull’app prima di metterla nel DMG

La firma ad hoc usata finora evita l’errore «danneggiata», ma **non** soddisfa Gatekeeper per download da Internet.

## Secret GitHub da configurare

Nel repository: **Settings → Secrets and variables → Actions → New repository secret**

| Secret | Descrizione |
|--------|-------------|
| `APPLE_CERTIFICATE_BASE64` | File `.p12` del certificato Developer ID Application, codificato in base64 |
| `APPLE_CERTIFICATE_PASSWORD` | Password usata per esportare il `.p12` |
| `APPLE_TEAM_ID` | Team ID (10 caratteri), visibile su [developer.apple.com/account](https://developer.apple.com/account) |

**Autenticazione notarizzazione** (scegli **una** delle due opzioni):

### Opzione A — App-specific password (più semplice)

| Secret | Descrizione |
|--------|-------------|
| `APPLE_ID` | Email dell’account Apple Developer |
| `APPLE_APP_SPECIFIC_PASSWORD` | Password generata su [appleid.apple.com](https://appleid.apple.com) → Accesso e sicurezza → Password per le app |

### Opzione B — API key App Store Connect (consigliata in CI)

| Secret | Descrizione |
|--------|-------------|
| `APP_STORE_CONNECT_API_KEY_ID` | Key ID |
| `APP_STORE_CONNECT_API_ISSUER_ID` | Issuer ID |
| `APP_STORE_CONNECT_API_KEY_BASE64` | Contenuto del file `.p8` in base64 |

## Creare il certificato Developer ID

1. Iscriviti al [Apple Developer Program](https://developer.apple.com/programs/)
2. Su un Mac: **Accesso Portachiavi** → Assistente certificati → Certificato Developer ID Application
   - Oppure crea il CSR e scarica il certificato dal portale Apple
3. Esporta certificato + chiave privata come **`.p12`**
4. Codifica in base64:
   ```bash
   base64 -i DeveloperID.p12 | pbcopy
   ```
5. Incolla il risultato in `APPLE_CERTIFICATE_BASE64`

## Verifica locale (Mac con certificato installato)

```bash
export APPLE_TEAM_ID="XXXXXXXXXX"
export APPLE_ID="tu@email.com"
export APPLE_APP_SPECIFIC_PASSWORD="xxxx-xxxx-xxxx-xxxx"

./scripts/package-dmg-translate.sh
```

Con i secret configurati, lo script esegue firma Developer ID + notarizzazione + stapling.

## CI

Il workflow **Nebbie Translate (release packages)** importa il certificato, firma e notarizza l’app, poi crea il DMG.

Se i secret mancano, il job **macos-dmg** fallisce con un messaggio che rimanda a questo documento.

Dopo aver aggiunto i secret: **Actions → Nebbie Translate (release packages) → Run workflow**.

## Risultato atteso

- Doppio click su `nebbie-translate.app` → si apre normalmente
- Nessun pannello «Privacy e sicurezza»
- Nessun `xattr` o tasto destro → Apri
