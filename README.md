# Contour 0.1.1 — Filth Assembly

VCV Rack 2 -moduuli: animoidun Perlin-maaston suljetut
korkeuskäyrät ohjaavat enintään neljää itsenäistä XY-modulaatiopalloa.
20 HP:n paneelissa näkyvät vain viivakäyrät ja numeroidut pallot.

Version 0.1.1 muutokset koskevat vain ulkoasua: uusi 20 HP:n paneeli,
kaksiosaiset 270° nupit, 8 mm:n Velocity-nupit ja omat SVG-jakit.
Läpinäkyvä 59 × 59 mm:n näyttö on kohdassa (21,4; 11,5) mm;
pallot ja korkeuskäyrät ovat limenvihreitä (#b9ea09ff). DSP, parametrit,
CV-käsittely ja tallennusmuoto ovat ennallaan.

## Versio ja ympäristö

Kehitysversio **0.1.1** vastaa Rack-manifestin versiota **2.1.1**.
Rack edellyttää pluginin pääversion vastaavan Rackin pääversiota:
[VCV:n manifestiohje](https://vcvrack.com/manual/Manifest#version).
Pluginin ja moduulin slug on `Contour`; alkuperäiset brändi-, tekijä- ja
lisenssitiedot on säilytetty. Tämä on paikallinen prototyyppi, ei julkaisu.

Käännetty ja ladattu Linux x64:llä käyttäen SDK:ta `~/Rack-SDK`
(2.6.6) ja Rack Free 2.6.6:ta. Makefilen oletus on `RACK_DIR=$(HOME)/Rack-SDK`.
SDK-polun voi aina antaa komentorivillä. Ei uusia ulkoisia riippuvuuksia.

## Käännös, testit ja paikallinen asennus

Tarvitaan Rack 2 SDK, C++11-kääntäjä, make ja jq; pakkaamiseen myös zstd.

```sh
cd ~/dev/vcv/Contour
make -j4 RACK_DIR="$HOME/Rack-SDK"
make test RACK_DIR="$HOME/Rack-SDK"
make dist RACK_DIR="$HOME/Rack-SDK"
```

Sulje Rack ennen asennusta. Kopioi `dist/Contour-2.1.1-lin-x64.vcvplugin`
Rackin käyttäjähakemiston `plugins-lin-x64`-kansioon. Linuxin tavallinen
käyttäjähakemisto on `~/.local/share/Rack2`; tarkista käyttämäsi hakemisto
Rackin **Help → Open user folder** -toiminnolla. Varmista ensin, ettei siellä
ole korvattavaa omaa Contour-versiota.

```sh
mkdir -p ~/.local/share/Rack2/plugins-lin-x64
cp -i dist/Contour-2.1.1-lin-x64.vcvplugin ~/.local/share/Rack2/plugins-lin-x64/
```

Käynnistä Rack uudelleen, avaa moduuliselain hiiren oikealla painikkeella ja
hae **Contour** (Filth Assembly). Vaihtoehtoisesti avaa tämän projektin
`tests/Scope.vcv`: siinä 1X/1Y on kytketty VCV Scopeen. Testipatch vaatii
VCV Free / Fundamental -pluginin. Scopen aikajänne on 8 s ja näyttöalue
10 V; 1 x 2 -tilassa voi tarkastella XY-liikettä.

Tämän koneen Rack-binaarin voi käynnistää näin:

```sh
cd ~/Rack2Free
LD_LIBRARY_PATH="$PWD" ./Rack ~/dev/vcv/Contour/tests/Scope.vcv
```

Asennuspaketti sisältää myös README:n ja Scope-testipatchin. Pakettia ei ole
asennettu varsinaiseen käyttäjäprofiiliin; version 0.1.1 GUI-koe käytti
erillistä `/tmp/contour-011-gui`-käyttäjähakemistoa.

## Säätimet ja jännitteet

Nopeuksien yksikkö "leveyttä/s" tarkoittaa sisäisen yksikköneliön leveyttä.
Nopeusasteikot ovat logaritmiset. Evolve- ja Velocity-säätimien alin 0,1 %
on pysäytysalue; sen yläpuolella käytetään alla olevia nopeuksia.

| Säädin | Alue | Oletus |
| --- | --- | --- |
| Balls | kokonaisluku 1–4 | 2 |
| Contours | 1–4 kiinteää kirkkaustasoa | 3 |
| Evolve | stop / 0,002–0,5 kohinan aikayksikköä/s | 35 %, noin 0,0138/s |
| Fractality | 0–100 % | 40 % |
| Slew | 0,02–4 leveyttä/s | 0,25 leveyttä/s |
| Velocity 1 | stop / 0,005–2 leveyttä/s | 40 %, noin 0,0547/s |
| Velocity 2 | sama | 45 %, noin 0,0739/s |
| Velocity 3 | sama | 50 %, noin 0,0997/s |
| Velocity 4 | sama | 55 %, noin 0,1346/s |

**Evolve** kuljettaa jatkuvan 3D-Perlin-kohinan aikakoordinaattia. Nolla
pysäyttää maaston, mutta Fractalityn muuttaminen muuttaa sitä edelleen.
**Fractality** muuttaa neljän oktaavin painoja jatkuvasti. Perustaajuus on
2,6 jaksoa/leveys, suurin 20,8; pienimpiä ruudukon alaisia yksityiskohtia ei
lisätä. Keskustapainotus on `sin(pi*x)*sin(pi*y)`, reunat nollassa.
Kentän amplitudia ei normalisoida framekohtaisilla ääriarvoilla.
Kirkkaustasot ovat 0,20 / 0,32 / 0,44 / 0,56. Contours lisää ensimmäiset N
tasoa muuttamatta aikaisempia tasoja; yksi taso voi tuottaa monta silmukkaa.

**Velocity** on matka käyrän kaaripituutta pitkin sekunnissa. Iso silmukka
kierretään siksi hitaammin kuin pieni. Kaikki pallot kulkevat sisäisessä
koordinaatistossa vastapäivään. Nollanopeudellakin elävä rata tai siirtymä
voi liikuttaa palloa.

**Slew**, tooltipissä **Transition speed limit**, rajoittaa vain
radalle liittymisen todellista kaksiulotteista nopeutta. Suurempi arvo
nopeuttaa siirtymää. Liittymispiste ei etene Velocityn mukana, mutta seuraa
rataa sen muuttuessa. Siirtymällä ei ole aikakatkaisua. Tavallinen radan
muodonmuutos voi ylittää tämän nopeuden seurantatilassa.

Kuudessa CV-inputissa käytetään vain ensimmäistä kanavaa. CV/5 lisätään
säätimen normalisoituun arvoon ennen nopeusmuunnosta ja tulos rajataan
välille 0–1. +5 V nostaa koko alueen, −5 V laskee koko alueen. Kytkemätön
input antaa 0 V. Slew'llä ei ole CV-inputia. Jatkuvien parametrien muutokset
pehmennetään 12 ms:n aikavakiolla erillään siirtymärajoituksesta.

Kahdeksan outputia ovat monofonisia: `X=10*x−5 V`, `Y=10*y−5 V`.
Oikea ja yläreuna ovat +5 V, vasen ja alareuna −5 V, keskellä molemmat 0 V.
Näytön Y-akseli käännetään vain piirrossa. Piirrettävät pallot ja CV
käyttävät samaa todellista sijaintia, myös siirtymässä. Näyttö ottaa
30 Hz:n tilannekuvia; CV lasketaan jokaisella audionäytteellä.

## Ratojen jakaminen ja reunatapaukset

Ilman rataa oleva pallo varaa ensisijaisesti lähimmän vapaan silmukan.
Varauksessa lasketaan mukaan myös siirtymätilassa olevat pallot. Jos kaikki
radat ovat varattuja, sallitaan yhteinen rata ja alkuvaiheet erotellaan.
Useamman pallon radalta yksi voi siirtyä vähintään 200 ms säilyneelle
vapaalle radalle; vapaaehtoisilla vaihdoilla on 500 ms:n jäähy.
Yksin omalla radallaan oleva ei siirry vain uuden vapaan radan vuoksi.
Myös eri kirkkaustasojen väliset vaihdot sallitaan. Reitin valinta
arvioidaan enintään 100 Hz:n taajuudella, liike jatkuu audionäytteittäin.

Radan katoaminen käynnistää uuden haun ilman syntymis- tai jäähyodotusta
(seuraavassa valintapäivityksessä, enintään noin 10 ms). Tyhjässä maastossa
pidetään viimeinen sijainti. Käynnistys ja uudelleen aktivoitava pallo
aloittavat keskeltä ja liittyvät Slew-rajoitetusti. Balls-arvon pienentyessä
suurinumeroisimmat pallot poistuvat ja niiden jännitteet häivytetään
lineaarisesti 0 V:iin 10 ms:ssa. Epäkelpoiset jännitteet korvataan nollalla.

Patchiin tallentuvat Rack-parametrit ja kohinan 32-bittinen seed.
Vaiheet, aikakoordinaatti ja siirtymien keskeneräinen tila eivät tallennu.
Reset arpoo uuden seedin. Tallennetun patchin avaaminen aloittaa liikkeen
uudelleen samalla kohinan seedillä.

## Toteutus ja rajat

`src/core.hpp` on Rackista riippumaton algoritmiydin. `src/Contour.cpp`
liittää sen Rackiin ja piirtää paneelin. `res/Contour_panel.svg` sisältää
uuden paneelin poluiksi muunnetuilla teksteillä; components-taso on
piilotettu, ja sen merkkien keskipisteet määrittävät komponenttien paikat.
`res/knob_static.svg` on paikallaan pysyvä pohja, `res/knob_top.svg`
kiertyvä kansi ja `res/jack.svg` liitäntöjen grafiikka.

Maastotyösäie laskee 96×96-kentän noin 30 Hz:llä. Marching squares käyttää
ruudukon särmätunnisteita segmenttien liittämiseen ja asymptoottista
satulapäättäjää. Avoimet polut, alle 0,09:n kaaripituudet ja alle 0,0002:n
pinta-alat hylätään. Enintään 48 silmukkaa, 192 pistettä/silmukka.
Uudelleennäytteistetyn lopullisen polylinen kaaripituus lasketaan erikseen,
ja sekä näyttö että pallot käyttävät tätä geometriaa.

Silmukat yhdistetään edelliseen saman tason geometriaan keskipisteen ja
pinta-alan perusteella. Täydellistä jakautumis-/yhdistymishistoriaa ei ole.
Pallon vanha liittymispiste projisoidaan uuteen geometriaan. Alle 7,5 %:n
paikkamuutokset interpoloidaan 33 ms:n aikana kompensaatiosiirtymällä,
jolloin CV ei hypi maaston 30 Hz:n päivitysten tahdissa. Isompi korjaus
vaihtaa pallon Slew-rajoitettuun liittymistilaan. Interpoloinnin aikana
pallo voi olla hieman viimeisimmän piirretyn viivan vieressä.

Työsäie → audio ja audio → näyttö käyttävät kolmen kiinteän puskurin
omistuspostilaatikoita (free/writing/ready/reading). Tuottaja jättää
julkaisun väliin, jos puskureita ei ole vapaana; kuluttaja ottaa uusimman
valmiin version. Audiosäie ei odota lukkoja, lue tiedostoja eikä allokoi
geometriamuistia. Geometrian kopiointi ja reittihaku ovat rajattuja.
Työsäie pysäytetään ja liitetään moduulia poistettaessa. Jokaisella
moduuli-instanssilla on oma työsäie.

Tunnetut prototyyppirajat: geometrinen identiteetti on heuristinen;
suurilla Evolve/Fractality-arvoilla siirtymiä voi tulla usein. Hidas Slew
voi jättää pallon pitkäksi aikaa tavoittelemaan liikkuvaa kohdetta.
48 silmukan rajan jälkeen löydetyt radat jätetään
pois. Muodonmuutoksen interpolointi ei ole täydellinen topologinen
morfismi. Hyvin pitkien sessioiden aikakoordinaatin liukulukutarkkuutta,
monien instanssien kuormaa ja muita käyttöjärjestelmiä ei ole varmennettu.

## Varmennus

- `make` onnistui SDK 2.6.6:lla Linux x64:llä.
- `make test`: kaikki 16 Balls/Contours-yhdistelmää, 384 maastopäivitystä,
  jatkuva Fractality ja Evolve, rajatut suljetut radat, tasojen pysyvyys,
  2D-siirtymäraja, varaukset, vapaan radan hystereesi, yhteisten ratojen
  eri vaiheet, radan katoaminen, tyhjä ja degeneroitunut geometria,
  kaaripituusnopeus ja 44,1/48/96 kHz:n riippumattomuus.
- Sama ydintestistö läpäisi myös AddressSanitizer- ja UndefinedBehaviorSanitizer-ajon.
- Rackiin linkitetty integraatiotesti ajaa oikeaa moduulia ja työsäiettä:
  kahdeksan monoporttia, äärelliset ±5 V jännitteet, todellisen sijainnin
  ja outputin vastaavuus, poly-CV:n kanava 0, häivytys, seedin JSON ja
  toistuva instanssin luonti/tuhoaminen sekä postilaatikon rinnakkainen
  tuottaja/kuluttaja (100 000 julkaisuyritystä).
- Rack Free 2.6.6 latasi pluginin ja piirsi paneelin. Scope-testipatch
  avattiin oikeassa GUI:ssa; käyrät, pallot ja 1X/1Y-signaalit näkyivät.
- Äänilaitteen pätkimättömyyttä, kattavaa pitkäkestoista suorituskykyä
  tai kaikkien käyttöliittymäsäätimien käsikäyttöä ei ole varmennettu.

Version 0.1.1 varmennus: `make`, `make test` ja `make dist` onnistuivat.
Rack Free 2.6.6 latasi manifestiversion 2.1.1 ja Scope-testipatchin.
Oikeasta Rack-ikkunasta tarkistettiin uusi paneeli, SVG-nupit ja jakit,
näytön ruudukon säilyminen sekä limenvihreät käyrät ja numeroidut pallot.
Moduulin laskenta- ja tallennuskoodi on verrattu edelliseen committiin:
se on muuttumaton. Ydinalgoritmitiedostoa ei muutettu.
