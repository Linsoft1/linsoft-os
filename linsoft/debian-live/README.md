# Linsoft OS 1.5

Aktuálne vydanie je **Linsoft OS 1.5 (28. 10. 2026)**, Debian 13/Openbox s jadrom `6.12.113-linsoft-amd64`. Nasledujúci opis pôvodnej lokálnej zostavy 1.1 platí aj pre túto vývojovú vetvu; číslo vydania v ISO, bootovacích ponukách a systémových informáciách je 1.5. Vydanie 1.5 nie je priamou aktualizáciou odlišnej vetvy LFS/XFCE 1.4 publikovanej na GitHube. Secure Boot treba vypnúť.

Pre publikovanie sa initramfs komprimuje cez XZ (`xz-utils`) a SquashFS používa 1 MiB bloky, aby ISO neprekročilo limit jedného súboru v GitHub Releases. Obsah a aplikácie sa tým neodoberajú.

Linsoft 1.1 je vlastná slovenská live distribúcia postavená na Debiane 13 s vlastným zostaveným upstream jadrom Linux 6.12.113. Jadro používa konfiguráciu odvodenú od Debianu, obsahuje moduly pre live boot a hardvér a má vlastnú verziu `6.12.113-linsoft-amd64`. Používa samostatnú reláciu Openbox bez desktopového prostredia XFCE, vlastný panel Tint2 a Linsoft Hub v štýle ponuky Štart. Pri štarte zobrazuje vlastné logo Linsoft a indikátor priebehu načítania cez Plymouth; bootovacie menu, obraz ISO, systémové informácie a inštalátor používajú jednotné označenie Linsoft. Prihlásenie aj zamknutá obrazovka používajú tapetu Linsoft OS a používateľský profil zobrazuje logo Linsoft. Panel je tmavý, priesvitný a zaoblený; zobrazuje tlačidlo Linsoft, pripnuté aplikácie, otvorené okná, systémovú oblasť a hodiny s dátumom. Názvy pracovných plôch sa na paneli nezobrazujú; medzi štyrmi plochami prepínaš skratkami `Super+1` až `Super+4`. V systémovej oblasti je indikátor klávesnice: slovenské rozloženie je predvolené, kliknutím na vlajku prepneš na anglické (US) a späť; prepínať môžeš aj skratkou `Alt+Shift`. Hub otvoríš kliknutím na tlačidlo LINSOFT alebo skratkou `Super+Medzerník`; jeho okraj sa zarovná s okrajom panela a spodná hrana dosadne na panel. Obsahuje vyhľadávanie, kategórie, pripnuté aplikácie a prístup k nastaveniam, vzhľadu a uzamknutiu obrazovky. Jeho tmavé rozhranie zjednocujú modré akcenty, kontrastné stavy tlačidiel, zaoblené karty a upravený panel vyhľadávania. Hub sa umiestni na monitor, na ktorom sa nachádza kurzor, a prispôsobí svoju veľkosť dostupnej pracovnej ploche; pri užšom okne zobrazí aplikácie a pripnuté položky pod sebou. Po opätovnom otvorení obnoví zoznam aplikácií, takže sa zobrazia aj programy nainštalované cez Linsoft Store. Vo výsledkoch vyhľadávania sa môžeš pohybovať šípkami a aplikáciu spustiť klávesom Enter. `Alt+F2` otvorí vyhľadávanie aplikácií a správca nastavení poskytuje spoločné miesto na úpravu zariadení a pracovnej plochy. Upozornenia zabezpečuje XFCE Notify Daemon. Obrazovku zamkneš skratkou `Super+L`. Téma Linsoft-Blue prináša zaoblené rohy, tiene a prechody.

Okná zatvoríš kliknutím na tlačidlo `X` v titulkovom paneli alebo pomocou `Alt+F4`; tlačidlá zatvorenia, maximalizácie aj minimalizácie reagujú na kliknutie. Ďalšie ovládanie: `Alt+Tab` prepína okná, `Alt+Medzerník` otvorí ponuku okna vrátane presunutia a zmeny veľkosti. `Super+Šípka doľava/doprava` zarovná okno na polovicu obrazovky, `Super+Šípka nahor` ho maximalizuje alebo obnoví a `Super+Shift+Šípka nahor` prepína maximalizáciu na výšku. `Super+Šípka nadol` obnoví maximalizované okno a `Super+Shift+Šípka nadol` ho minimalizuje. Rozhranie dopĺňajú tmavá GTK téma, vlastné ikony, tapeta Linsoft Flow a plocha bez ikon. Logo Linsoft sa zobrazuje v bootovacích ponukách a počas štartu cez Plymouth. Locale a predvolené rozloženie klávesnice sú nastavené na slovenčinu; dostupné je aj rozloženie anglické (US). Linsoft Store (`Super+S`) umožňuje inštalovať aplikácie z katalógu `config/includes.chroot/usr/share/linsoft/store/catalog.json`, obnoviť zoznam balíkov a aktualizovať nainštalované systémové balíky.

Pri vypínaní zostáva zobrazené logo Linsoft a téma Plymouth používa text „VYPÍNAM SYSTÉM“. Pri reštarte zobrazuje „REŠTARTUJEM SYSTÉM“ a pri štarte „SPÚŠŤAM SYSTÉM“.

## Čo potrebuješ

- Počítač s Debianom 13 alebo kompatibilnou distribúciou založenou na Debiane
- Pripojenie na internet
- Aspoň 30 GB voľného miesta na zostavenie jadra a ISO
- 4 GB RAM alebo viac

ISO sa zostavuje na Linuxe pomocou `live-build`. Vo Windows ho zostav vo virtuálnom stroji s Debianom. WSL neodporúčame ako prvé prostredie na tvorbu bootovateľného ISO.

## Zostavenie ISO

V Debiane otvor terminál v priečinku projektu a spusti:

```sh
sudo apt update
sudo apt install live-build build-essential bc bison curl debhelper flex libssl-dev libelf-dev dwarves xz-utils
sudo bash ./build.sh
```

Skript najprv prevezme zdrojový kód upstream jadra Linux 6.12.113, zostaví vlastný balík `linux-image-linsoft-amd64` z konfigurácie v `config/kernel/linsoft-amd64.config` a potom vytvorí ISO. Pri prvom zostavení sa stiahnu zdroje jadra a potrebné balíky, takže zostavenie môže trvať dlhšie. Live systém obsahuje bežné firmvérové balíky z Debianu; inštalátory Broadcom b43/b43legacy sú vynechané, pretože pri inštalácii sťahujú firmvér z externého servera. Vlastné jadro nie je podpísané pre Secure Boot; pred spustením Linsoft vypni Secure Boot v nastaveniach UEFI. Výsledné ISO `live-image-amd64.hybrid.iso` je určené pre amd64 a nesie označenie `LINSOFT`.

## Inštalácia a bezpečnosť

Najskôr spusti Linsoft v live režime a over, že funguje na tvojom počítači. Inštalátor spustíš z položky `Linsoft Installer` v bootovacej ponuke, ak je v obraze dostupná. Pred inštaláciou si zálohuj dôležité súbory a pozorne skontroluj cieľový disk a rozdelenie oddielov. Automatické rozdelenie môže vymazať existujúci systém a všetky dáta na zvolenom disku; nepokračuj, ak si nie si istý výberom. Pri prvom pokuse uprednostni virtuálny stroj alebo samostatný testovací disk.

Linsoft je testovacia distribúcia. Dostupnosť inštalátora, Secure Boot a kompatibilita hardvéru sa líšia podľa počítača a nie sú certifikované pre všetky zariadenia. Aktualizácia v Linsoft Store používa nakonfigurované Debian repozitáre; v live režime sa systémové zmeny po reštarte stratia.

## Otestovanie hardvéru

Linsoft Hub a ponuka Openbox obsahujú nastavenia obrazovky, siete, zvuku, napájania a Bluetooth. Na kontrolu hardvéru otvor `Informácie o PC`; výstup nástroja `inxi` sa zobrazí v termináli. Otestuj najmä:

- Wi‑Fi a Ethernet,
- zvuk, mikrofón, klávesnicu, touchpad a Bluetooth,
- rozlíšenie, externý monitor, jas a uspávanie/zobúdzanie,
- inštaláciu aplikácie, aktualizáciu systému a reštart po aktualizácii.

Výsledky z virtuálneho stroja nenahrádzajú test na reálnom počítači. Pred zverejnením treba osobitne otestovať inštaláciu, UEFI/Secure Boot, rôzne grafické a sieťové adaptéry a obnovu po zlyhaní.

## Otestovanie ISO

Najprv otestuj ISO vo virtuálnom stroji, napríklad vo VirtualBoxe alebo GNOME Boxes. Pred zverejnením over kontrolný súčet, bootovanie v UEFI/BIOS a položky inštalátora. Zápis ISO na USB vymaže jeho pôvodný obsah. Podpora Secure Boot závisí od podpisových balíkov a firmvéru počítača.

## Stav overenia tohto obrazu

Kontrolný súčet ISO je uložený v `SHA256SUMS`. Táto zostava obsahuje vlastné jadro `6.12.113-linsoft-amd64` a zodpovedajúci live initrd. Manifest balíkov a konfigurácia GRUBu boli overené: bootovacie položky odkazujú na toto jadro a initrd. Téma Plymouth Linsoft a logo sú vložené do initrd; obraz tiež obsahuje Linsoft banner inštalátora a predvolený názov hostiteľa `linsoft`. Zostavenie ISO úspešne dokončilo live-build a kontrolný súčet ISO sa zhoduje. Pri testovacom priamom štarte jadra a initrd v QEMU s ISO pripojeným ako live médium systém dosiahol prihlasovací cieľ `linsoft login:` a zaznamenal pokus o spustenie LightDM. Tento test obchádza zavádzač, takže nepotvrdzuje BIOS/UEFI štart cez GRUB ani zobrazenie grafickej plochy. Tieto cesty treba ešte otestovať; fyzický hardvér a čistá inštalácia aktuálneho obrazu tiež nie sú potvrdené.

Čistá inštalácia z aktuálneho obrazu a prvý štart z nainštalovaného virtuálneho disku ešte nie sú potvrdené. Predchádzajúca zostava dokončila inštaláciu na samostatný prázdny virtuálny disk s kapacitou 25,8 GB, ale to nepotvrdzuje inštaláciu tejto zostavy.

Tento obraz zatiaľ nepovažuj za finálne verejné vydanie. Pred vydaním treba dokončiť čistú inštaláciu aktuálnej zostavy, overiť názov hostiteľa a prvý štart nainštalovaného systému. Na fyzickom počítači treba otestovať BIOS/UEFI, Wi‑Fi, zvuk, Bluetooth, uspávanie, prebúdzanie a externý monitor. Secure Boot a kompatibilita s fyzickým hardvérom nie sú certifikované.

## Prispôsobenie

- Zmeň balíky v `config/package-lists/linsoft.list.chroot`.
- Správanie Openboxu uprav v `config/includes.chroot/etc/xdg/openbox/rc.xml`, kontextové menu v `config/includes.chroot/etc/xdg/openbox/menu.xml` a spúšťanie služieb v `config/includes.chroot/etc/xdg/openbox/autostart`. Tému okien uprav v `config/includes.chroot/usr/share/themes/Linsoft-Blue/openbox-3/themerc` a tiene v `config/includes.chroot/etc/xdg/picom.conf`.
- Spúšťač Linsoft Hub uprav v `config/includes.chroot/usr/bin/linsoft-hub` a jeho vzhľad v `config/includes.chroot/usr/share/linsoft/linsoft-hub.css`.
- Panel Tint2 uprav v `config/includes.chroot/etc/xdg/tint2/linsoft.tint2rc`; jeho spúšťanie a ďalšie služby relácie v `config/includes.chroot/etc/xdg/openbox/autostart`. Kategórie a pripnuté aplikácie Hubu uprav v `config/includes.chroot/usr/bin/linsoft-hub`.
- Ikony uprav v `config/includes.chroot/usr/share/icons/Linsoft-Icons/` a tapetu v `config/includes.chroot/usr/share/backgrounds/linsoft/`.
- Predvolenú reláciu a automatické prihlásenie nastav v `config/includes.chroot/etc/lightdm/lightdm.conf.d/99-linsoft.conf`; dostupná relácia je definovaná v `config/includes.chroot/usr/share/xsessions/linsoft.desktop`. GTK tému používateľského profilu nastav v `config/includes.chroot/etc/skel/`.
- Zmeň texty v `config/includes.chroot/etc/`.
- Zmeň bootovaciu grafiku GNU GRUB a ISOLINUX v `config/includes.binary/` a tému Plymouth v `config/includes.chroot/usr/share/plymouth/themes/linsoft/`. Banner inštalátora uprav v `config/includes.binary/installer/linsoft-installer-banner.svg`; po úprave obnov PNG pomocou `rsvg-convert`. Predvolený názov hostiteľa inštalátora nastavuje bootovací hook `config/hooks/normal/9000-linsoft-branding.hook.binary`.
- Verziu a základnú konfiguráciu uprav v `build.sh`.

Toto je základná testovacia verzia, nie hotový systém na bežné používanie. Pred zverejnením otestuj inštaláciu, aktualizácie, hardvér a licencie zahrnutých balíkov.
