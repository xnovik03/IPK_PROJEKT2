# IPK25-CHAT klient



##  Obsah

1. [Teorie](#teorie)
    - [Úvod do TCP a UDP](#úvod-do-tcp-a-udp)
    - [TCP](#tcp)
    - [UDP](#udp)
2. [UML diagramy](#uml-diagramy)
    - [Obecná struktura](#obecná-struktura)
    - [Zprávy (Messages)](#zprávy-messages)
    - [Hlavní smyčka](#hlavní-smyčka)
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




## Zdroje a použitá literatura

https://linux.die.net/man/3/inet_pton


 https://www.geeksforgeeks.org/udp-server-client-implementation-c/


 https://beej.us/guide/bgnet/html/#two-types-of-internet-sockets

 
  https://beej.us/guide/bgnet/html/split/client-server-background.html#a-simple-stream-client

  https://www.kiv.zcu.cz/~txkoutny/download/javanet.pdf (pro dokumentaciju teorija )

  https://tsplus.net/cs/remote-access/blog/rdp-network-ports-tcp-vs-udp (pro dokumentaciju teorija )
  https://cs.wikipedia.org/wiki/User_Datagram_Protocol (pro dokumentaciju teorija )
  https://www.geeksforgeeks.org/udp-server-client-implementation-c/ (pro dokumentaciju teorija )
