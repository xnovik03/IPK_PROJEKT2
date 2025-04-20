# IPK25-CHAT klient



##  Obsah

1. [Teorie](#teorie)
    - [Úvod do TCP a UDP](#úvod-do-tcp-a-udp)
    - [TCP](#tcp)
    - [UDP](#udp)
2. [UML diagramy](#uml-diagramy)
    - [ Struktura aplikace](# Struktura-aplikace)
    
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

 



## Zdroje a použitá literatura

https://linux.die.net/man/3/inet_pton


 https://www.geeksforgeeks.org/udp-server-client-implementation-c/


 https://beej.us/guide/bgnet/html/#two-types-of-internet-sockets

 
  https://beej.us/guide/bgnet/html/split/client-server-background.html#a-simple-stream-client

  https://www.kiv.zcu.cz/~txkoutny/download/javanet.pdf (pro dokumentaciju teorija )

  https://tsplus.net/cs/remote-access/blog/rdp-network-ports-tcp-vs-udp (pro dokumentaciju teorija )
  https://cs.wikipedia.org/wiki/User_Datagram_Protocol (pro dokumentaciju teorija )
  https://www.geeksforgeeks.org/udp-server-client-implementation-c/ (pro dokumentaciju teorija )

  https://www.planttext.com/ (pro generaciju uml diagramu)
