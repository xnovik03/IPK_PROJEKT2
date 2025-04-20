# Changelog

## [Unreleased]

### TCP
- **Problém s chybami**: V TCP verzi klienta máme problém s odesíláním a zpracováním **ERR** a **ERROR** zpráv. Tyto zprávy nejsou neuplne správně zpracovány .
  
### UDP
- **Funkční příkazy**: Implementace příkazu `/auth` funguje správně a umožňuje autentifikaci uživatele. Dále je částečně funkční i příkaz `/bye`, který správně odesílá zprávu o ukončení sezení, ale zatím není plně implementován pro všechny scénáře.
- **Chybné příkazy**: Ostatní příkazy, jako `/join` nebo `/rename`, zatím nepracují uplne správně a vykazují různé problémy.

