
---

# Changelog

### TCP
- Na základě provedených testů fungují správně všechny základní příkazy: `/auth`, `/rename`, `/join`, `msg` a `bye`.

### UDP
- Při připojení ke serveru (např. Discord) fungoval správně pouze příkaz `/auth`. Zpráva se nezobrazila a příkazy jako `/join` a další zprávy nefungovaly správně.
- Výpisy nejsou plně sjednocené do formátu `print_debug()`. Většina debug informací je stále vypisována pomocí `std::cerr`.

--- 

