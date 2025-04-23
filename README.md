# IPK25-CHAT klient


## Obsah

1. [Teorie](#teorie)
    - [Úvod do TCP a UDP](#úvod-do-tcp-a-udp)
    - [TCP](#tcp)
    - [UDP](#udp)
2. [UML diagramy](#uml-diagramy)
    - [Struktura aplikace](#struktura-aplikace)
3. [Testování](#testování)
4. [Zdroje a použitá literatura](#zdroje-a-použitá-literatura)


---

##  Teorie

### Úvod do TCP a UDP

Pokud chtějí dvě aplikace komunikovat, musí nejdříve navázat spojení. Teprve po jeho navázání
mohou posílat data. Protokol TCP zaručuje, že data vyslaná jednou aplikací dojdou druhé aplikaci
v tom samém pořadí, v jakém byla odeslána, a zároveň spolehlivě (žádná se neztratí). K aplikacím,
které vyžadují spolehlivý přenos, patří např. http, ftp nebo telnet.
Naproti tomu protokol UDP nezaručuje ani spolehlivost ani pořadí přicházejících dat. V pod-
statě se do sítě vyšlou pouze nezávislé datagramy. Nenavazuje se ani spojení mezi dvěma aplika-
cemi.

### TCP

Transmission Control Protocol (TCP) je jedním z hlavních protokolů sady Internet Protocol (IP). TCP je navržen k poskytování spolehlivého, uspořádaného a ověřeného doručování dat mezi aplikacemi běžícími na hostech komunikujících prostřednictvím IP sítě. 

#### Implementační detaily:
- V jazyce C++ se pro implementaci TCP serveru a klienta využívají následující funkce:​

    socket(): Vytvoření socketu.

    bind(): Přiřazení adresy a portu socketu.

    listen(): Nastavení socketu do režimu naslouchání (pouze server).

    accept(): Přijetí příchozího spojení (pouze server).

    connect(): Navázání spojení se serverem (pouze klient).

    send() a recv(): Odesílání a přijímání dat.

    close(): Uzavření spojení.

### UDP

UDP (User Datagram Protocol) je jeden ze sady protokolů internetu. O protokolu UDP říkáme, že nedává záruky na datagramy, které přenáší mezi počítači v síti.

#### Implementační detaily:

- V jazyce C++ se pro implementaci UDP serveru a klienta využívají následující funkce:​

    socket(): Vytvoření socketu.

    bind(): Přiřazení adresy a portu socketu (pouze server).

    sendto(): Odeslání dat na specifikovanou adresu a port.

    recvfrom(): Přijetí dat a získání adresy odesílatele.

    close(): Uzavření socketu.​
 



---

##  UML diagramy
![](diagram.png)


##  Struktura aplikace


### Základní třída: `ChatClient`

Tato abstraktní třída slouží jako společný základ pro oba typy klientů – TCP a UDP. Obsahuje metodu `sendByeMessage()`, která umožňuje klientovi odeslat zprávu pro ukončení spojen.

### Zpracování vstupu: `InputHandler`

Třída `InputHandler` má  analýzovat a zpracovavat vstup od uživatee. Poskytuje metody jako `parseAuthCommand()` a `parseJoinCommand()`, které pomáhají identifikovat a interpretovat příkazy zadané uživatelm.

### TCP klient: `TcpChatClient`

Třída `TcpChatClient` dědí ze základní třídy `ChatClient` a implementuje funkcionalitu specifickou pro TCP komunikci. Mezi hlavní metody patří `connectToServer()`, `run()`, `sendByeMessage()`, `printHelp()`, `sendChannelJoinConfirmation()`, `process_reply()` a `processInvalidMessage). Tato třída také spolupracuje s třídami `MessageTcp` a `InputHandler` pro zpracování zpráv a vstpů.

### Zprávy pro TCP: `MessageTcp`

Třída `MessageTcp` definuje strukturu a typy zpráv používaných v TCP komunikaci. Obsahuje výčet `Type` s hodnotami jako `AUTH`, `JOIN`, `BYE`, `ERR`, `REPLY`.Mezi metody patří `fromBuffer()`, `getType()`, `getContent()`, `sendMessage()` a různé  metody pro vytváření specifických typů zráv.

### UDP klient: `UdpChatClient`

Třída `UdpChatClient` také dědí ze základní třídy `ChatClient` a implementuje funkcionalitu  pro UDP komunikaci.Obsahuje metody jako `connectToServer()`, `run()`, `handleCommand()`, `handleAuthCommand()`, `handleJoinCommand()`, `handleRenameCommand()`, `sendMessage()`, `sendByeMessage()` a `receiveServerResponseUDP)`.Tato třída spolupracuje s třídami `MessageUdp`, `InputHandler`, `UdpCommandBuilder` a `UdpReliableTransort`.

### Zprávy pro UDP: `MessagUdp`

Třída `MessageUdp` definuje strukturu zpráv používaných v UDP komuiaci. Obsahuje typ zprávy (`UdpMessageType`), identifikátor zprávy (`messageId`) a obsah zprávy (`paylod`).Poskytuje metody `packUdpMessage()` a `unpackUdpMessage()` pro serializaci a deserializacizpráv.

### Stavitel příkazů pro UDP: `UdpCommandBulder`

Třída `UdpCommandBuilder` slouží k vytváření různých typů UDP správ. Poskytuje metody jako `buildAuthUdpMessage()`, `buildJoinUdpMessage()`, `buildMsgUdpMessage()`, `buildConfirmUdpMessage()` a `buildReplyUdpMesage()`.

### Spolehlivý přenos pro UDP: `UdpReliableTrasport`

Třída `UdpReliableTransport` zajištuje spolehlivyj přenos zpráv v UDP, které je nespolhlivé.Udržuje mapu `pendingMessages` pro sledování nevyřízených zpráv a poskytuje metody `sendMessageWithConfirm()` a `processIncomingPacket()` pro odesílání zpráv s potvrzením a zpracování příchozích paketů.

### Hlavní soubor: `main.cpp`

Soubor `main.cpp` tvoří vstupní bod celé aplikace. Provádí: zpracování parametrů příkazové řádky (transport, adresa, port),výběr odpovídajícího klienta podle protokolu (tcp nebo udp),zachytávání signálu SIGINT a zajištění odeslání BYE zprávy při ukončení,
spuštění hlavní smyčky klienta voláním run().

---

## Testování

1. Manuální testování pomocí `netcat` (nc)
 Pro testování  klienta byl využit jednoduchý server, který byl vytvořen pomocí nástroje nc (Netcat). Tento server přijímal a odesílal zprávy dle specifikovaných protokolů a reagoval na příkazy od klienta.

### TCP
Kroky testování:

  **Spuštění klienta**  
   V jednom terminálu byl spuštěn klient následujícím příkazem:
   ```bash
   ./ipk25chat-client -t tcp -s 127.0.0.1
   ```

  **Spuštění testovacího TCP serveru (`nc`)**  
   Ve druhém terminálu byl spuštěn server :
   ```bash
   nc -l 4567
   ```

  **Zadávání příkazů na straně klienta**  
   Na straně klienta byly postupně zadány tyto příkazy:
    - `/auth pp oo rr`
    - `/rename petr`
    - `/join pp`
    - `Hello ` 
    - `Ctrl+c` (BYE)

**Pozorování výstupu na straně serveru (`nc`)**  
   Všechny odeslané zprávy byly viditelné ve výstupu `netcat`, což umožňuje manuálně ověřit:
    - správný formát zpráv,
    - správné pořadí,
    - správné zakončení řádkem \r\n,
    - a přítomnost výstupních zpráv jako  `AUTH`, `JOIN`, `MSG`, `BYE`.

![img_2.png](img_2.png)


### UDP
Kroky testování:

  **Spuštění klienta**  
   V jednom terminálu byl spuštěn UDP klient s nízkým timeoutem a jedním pokusem o retransmisi:
   ```bash
   ./ipk25chat-client -t udp -s 127.0.0.1 -p 3000 -d 100 -r 1
   ```

  **Spuštění testovacího UDP serveru (`nc`)**  
   Ve druhém terminálu byl spuštěn UDP server:
   ```bash
   nc -u -l 3000
   ```

  **Zadání příkazu na straně klienta**  
  Na straně klienta byly postupně zadány tyto příkazy:

  - `/auth pp ee rr`
  -  `Ctrl+c` (BYE)

   Zpráva byla odeslána jako binární UDP paket, který `netcat` zobrazil v surové podobě.

  **Simulace výpadku potvrzení (CONFIRM)**  
   Protože `netcat` neodpovídá na zprávy, klient neobdržel potvrzení (CONFIRM) a aktivoval **retransmisi**.

  **Výstup klienta**  
   Na straně klienta se zobrazily ladicí zprávy potvrzující retransmise:
   ```
  UdpChatClient.cpp:596  | sendRawUdpMessage | Sent message with ID 0 (type 2, size 12)
  UdpChatClient.cpp:190  | handleAuthCommand | UDP AUTH message sent.
  UdpChatClient.cpp:565  | checkRetransmissions | Checking message ID 0: elapsed = 45 ms
  UdpChatClient.cpp:565  | checkRetransmissions | Checking message ID 0: elapsed = 145 ms
  UdpChatClient.cpp:575  | checkRetransmissions | [RETRANS] Resending message ID 0
  UdpChatClient.cpp:565  | checkRetransmissions | Checking message ID 0: elapsed = 100 ms
  ERROR: Confirmation not received for message ID 0

   ```
  **Zobrazení výsledku v `netcat`**  
   Na straně `netcat` byly vidět dva identické příchozí pakety – originální zpráva a její retransmise.

####  Výstupní statistiky

Na konci běhu klienta se zobrazí souhrn:
```
main.cpp:23   |   signalHandler | Total retransmissions: 1
```


![img_3.png](img_3.png)


2. Použití testů: K ověření správnosti implementace byly využity automatizované testy. Testy jsou k dispozici na 
GitHubu: https://github.com/Vlad6422/VUT_IPK_CLIENT_TESTS.git (nejsem autor).  
![img_1.png](img_1.png)

Testy byly prováděny postupně, tedy každý test byl spuštěn jednotlivě. Po spuštění každého testu byla komunikace sledována v aplikaci Wireshark, kde byl použit plugin ipk25Chat pro analýzu UDP a TCP paketů

Např. test: tcp_auth_nok_ok

![](test1.jpg)

3. Testování se serverem ze zadání (DISCORD server)

###TCP
 **Autentizace (/auth)**  
   Byl zadán příkaz pro přihlášení uživatele:
   ```bash
   /auth kiralox
   ```
   Server potvrdil úspěšné přihlášení zprávou `REPLY OK is authenticated`.

  **Zaslání zprávy (MSG)**  
   Následně klient odeslal běžnou zprávu do výchozího kanálu:
   ```bash
   popo
   ```
   Tato zpráva byla serverem přijata a správně zobrazena ostatním uživatelům.

 **Připojení do jiného kanálu (/join)**  
   Klient se přesunul do kanálu s názvem `ovocko`:
   ```bash
   /join ovocko
   ```

 **Zaslání zprávy v kanálu `ovocko`**  
   V novém kanálu klient odeslal opět zprávu:
   ```bash
   popo
   ```
   I tato zpráva byla správně zobrazena a potvrzena serverem.

 **Ukončení relace Ctr+c (bye)**  
   Na závěr klient úspěšně ukončil spojení:
   Server správně reagoval zprávou `BYE`.

### Výsledky testu:

- Všechny příkazy byly serverem správně interpretovány.
- Příkazy `/auth`, `/join`, `MSG`, `BYE` byly ověřeny jako funkční .

![img.png](img.png)



## Zdroje a použitá literatura

- https://beej.us/guide/bgnet/html/split/client-server-background.html#a-simple-stream-client (Použit pro implementaci funkce connectToServer() v TCP )

- https://linux.die.net/man/3/inet_pton (Použití pro konverzi adresy: Funkce inet_pton() byla použita pro převod textového formátu IP adresy serveru na síťový formát, což je nezbytné pro správné přiřazení IP adresy do struktury sockaddr_in v TCP a UDP komunikaci.)


- https://www.geeksforgeeks.org/udp-server-client-implementation-c/ (Použití pro UDP komunikaci: Tento článek byl použit pro pochopení implementace základní UDP server-klient komunikace, která byla následně implementována pro odesílání a příjem UDP zpráv mezi klientem a serverem, včetně funkcí sendto() a recvfrom())


- https://beej.us/guide/bgnet/html/#two-types-of-internet-sockets (Použití pro pochopení typů socketů: Tento odkaz poskytl informace o dvou hlavních typech internetových socketů, které byly aplikovány při rozhodování o využití TCP a UDP soketů ve vývoji klienta a serveru.)


- https://www.kiv.zcu.cz/~txkoutny/download/javanet.pdf (byly využity pro teorie v dokumentaci )


- https://tsplus.net/cs/remote-access/blog/rdp-network-ports-tcp-vs-udp (byly využity pro teorie v dokumentaci  )


- https://cs.wikipedia.org/wiki/User_Datagram_Protocol (byly využity pro teorie v dokumentaci  )


  https://www.geeksforgeeks.org/udp-server-client-implementation-c/ (byly využity pro teorie v dokumentaci  )
  

- https://www.planttext.com/ (byly využity pro teorie v dokumentaci )

### Použití ChatGPT při vývoji

Během vývoje tohoto projektu jsem využívala ChatGPT jako poradce pro zlepšení struktury a optimalizaci kódu. Tento nástroj mi poskytl návrhy a doporučení, jak vylepšit funkce, opravit chyby a zefektivnit logiku kódu. Když jsem čelila výzvám při implementaci složitějších funkcí nebo optimalizaci kódu, ChatGPT mi nabídl nápady a směry, jak dosáhnout lepších a efektivnějších řešení.
