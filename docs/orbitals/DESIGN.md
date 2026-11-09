> Historical design, not the current interface specification. See [manual](README.md) and [mathematics](MATHEMATICS.md) for the implemented model. Inline formula formatting was converted for Obsidian.

# FILTH ASSEMBLY — Orbitals

**Design document · v0.1 · 31.10.2026**
**Kohdealusta:** VCV Rack 2, C++
**Tyyppi:** audiotaajuinen, kolmiulotteista aaltofunktiota lukeva oskillaattori
**Status:** toteutuksen lähtöspesifikaatio

> **Ydinajatus:** Orbitals tuottaa ääntä näytteistämällä oikean vetyatomin (tai vedynkaltaisen yksielektronisen ionin) atomiorbitaalin etumerkillistä aaltofunktiota pisteessä, joka liikkuu ohjelmallisesti määriteltyä kolmiulotteista rataa pitkin. Rata ja aaltofunktion arvo lasketaan ajon aikana jokaiselle audionäytteelle. Wavetableja, 3D-tekstuuritaulukoita tai valmiita aaltomuotoja ei käytetä.

## 1. Tavoite ja rajaus

Orbitals on oma oskillaattorityyppinsä, ei atomigrafiikalla koristeltu perinteinen syntetisaattori. Soitin hyödyntää aitojen kvanttimekaanisten aaltofunktioiden geometriaa, solmupintoja, säteittäisiä solmuja ja tilojen superpositiota äänenmuodostuksessa.

**Tavoitteet**

- Oikeat, analyyttiset vedynkaltaiset atomiorbitaalit; aluksi $Z=1$ (vety).
- Täysin reaaliaikainen kolmiulotteisen lukuradan ja kenttäarvon laskenta.
- Musiikillisesti säädettävä perustaajuuden ohjaus, 1 V/okt ja modulaariset CV-sisääntulot.
- Selkeä ero fysikaalisen perustan ja taiteellisen sonifikaation välillä.
- Reaaliaikaturvallinen C++-ydin, joka voidaan myöhemmin käyttää myös VST3-plugissa.
- Käsin kosketeltava, visuaalisesti kiinnostava modulaatio: radan sijainti, muoto, kallistus ja kentän rotaatio.

**Ei tehdä v0.1:ssä:** elektronien simuloituja ratoja, Schrödingerin yhtälön numeerista ratkaisua, monielektronisten atomien approksimoituja orbitaaleja, 3D-wavetableja, tiedostosta ladattavia wavetableja, 16-äänistä polyfoniaa, fysikaalisen atomin sähkömagneettisen säteilyn mallia tai VST-versiota.

## 2. Fysikaalinen perusta

### 2.1 Stationaariset vedyn aaltofunktiot

Lähtökohtana on vedynkaltaisen yhden elektronin Coulomb-potentiaalin normalisoitu tilafunktio:

$$
\psi_{nlm}(r,\vartheta,\varphi)=R_{nl}(r)Y_l^m(\vartheta,\varphi).
$$

Kvanttiluvut ovat $n\ge1$, $0\le l<n$ ja $-l\le m\le l$. Orbitaalin s-, p-, d- ja f-luokka vastaa $l=0,1,2,3$. Säteittäinen osa on

$$
R_{nl}(r)=\sqrt{\left(\frac{2Z}{na_0}\right)^3\frac{(n-l-1)!}{2n(n+l)!}}\;e^{-\rho/2}\rho^l L_{n-l-1}^{2l+1}(\rho),
\qquad \rho=\frac{2Zr}{na_0}.
$$

$a_0$ on Bohrin säde, $Z$ ytimen varausluku ja $L$ yleistetty Laguerre-polynomi. Toteutuksessa koordinaatit esitetään **Bohrin säteen yksiköissä**: $a_0=1$ laskennan mittayksikössä; oletusarvo $Z=1$. Tämä on pelkkä yksikkövalinta, ei aaltofunktion muodon approksimaatio.

### 2.2 Reaaliset orbitaalit

Tavalliset kompleksiset palloharmoniset funktiot $Y_l^m$ muunnetaan standardin mukaisiksi **reaalisiksi palloharmonisiksi** funktioiksi. Tämä tuottaa kemian tutut $p_x$, $p_y$, $p_z$, $d_{xy}$, $d_{xz}$, $d_{yz}$, $d_{x^2-y^2}$ ja $d_{z^2}$ -muodot. Nämä ovat edelleen fysikaalisesti kelvollisia vedyn energian ominaistiloja (saman energian degeneroituneiden tilojen lineaarikombinaatioita).

Käytä normalisoituja funktioita ja dokumentoi valittu vaihe-/etumerkkikonventio; eri lähteet voivat antaa vastakkaisen globaalin etumerkin samalle orbitaalille. Tämä ei muuta orbitaalin todennäköisyysjakaumaa mutta kääntää sonifioidun signaalin napaisuuden.

**Äänilähde on $\psi$, ei $|\psi|^2$.** Todennäköisyystiheys $|\psi|^2$ on aina epänegatiivinen, jolloin tärkeä napaisuus- ja solmupintatieto katoaisi. Aaltofunktion etumerkki/reaaliosa ei kuitenkaan ole suoraan fysikaalisesti mitattu äänenpaine: sen käyttäminen audiosignaalina on tarkoituksellinen sonifikaatiovalinta.

### 2.3 Referenssifunktiot yksikkökoordinaateissa

Alkuvaiheen yksikkötesteissä, kun $Z=a_0=1$:

$$
\psi_{1s}(x,y,z)=\frac{1}{\sqrt{\pi}}e^{-r},\quad r=\sqrt{x^2+y^2+z^2}.
$$

$$
\psi_{2p_z}(x,y,z)=\frac{z}{4\sqrt{2\pi}}e^{-r/2}.
$$

Nämä ovat oikeita normalisoituja vetyorbitaaleja, eivät pelkkiä niiden muotoa muistuttavia funktioita. Muiden tilojen evaluointi johdetaan samasta yleiskaavasta tai sitä vastaavista tarkasti normalisoiduista karteesisista polynomimuodoista.

### 2.4 Mitä laite *ei* väitä

Elektroni ei fysiikassa kierrä tässä määriteltyä audiotaajuista lukurataa. **Lukurata on virtuaalinen mittauspolku matemaattisen kentän sisällä.** Samoin oskillaattorin kuultu sävelkorkeus on lukunopeus, ei atomin elektronisen siirtymän todellinen taajuus. Mahdollinen tilojen ajallinen interpolointi tehdään musiikillisen ohjauksen ehdoilla, ei väitteenä reaalisesta atomidynamiikasta.

## 3. Äänenmuodostuksen periaate

### 3.1 Näytekohtainen laskenta

Jokaisella audionäytteellä `k`:

1. Päivitetään jatkuva vaihe $p\in[0,1)$ taajuudella `f`.
2. Muodostetaan vaihe $\theta=2\pi p$.
3. Lasketaan lukupään sijainti $\mathbf q(\theta)=(x,y,z)$.
4. Sovelletaan radan offset, rotaatio ja skaalatut koordinaatit.
5. Evaluoidaan valittujen reaalisten orbitaalien $\psi(\mathbf q)$ arvot suoraan analyyttisesti.
6. Muodostetaan painotettu summa (jos käytössä on kaksi tilaa).
7. Poistetaan tarpeellinen DC-komponentti, tehdään hallittu äänenvoimakkuusskaalaus ja kirjoitetaan lähtöjännite.

$$
p_{k+1}=\operatorname{fract}\left(p_k+\frac{f_k}{f_s}\right),\qquad
s_k=\sum_{i=1}^{N} w_i\,\psi_i\!\left(\mathbf q(2\pi p_k)\right).
$$

Oskillaattorin vaihe ja kentän geometrian ohjaus pidetään toisistaan erillään. Jatkuvaa vaihetta ei nollata, kun orbitaali tai radan asetukset muuttuvat.

### 3.2 Kolmiulotteinen lukurata

MVP:n ratageneraattori on **siirretty, skaalattu ja vaiheistettu suljettu Lissajous-rata**:

$$
\begin{aligned}
x(\theta)&=c_x+A_x\cos(\theta),\\
y(\theta)&=c_y+A_y\sin(k_y\theta+\delta_y),\\
z(\theta)&=c_z+A_z\sin(k_z\theta+\delta_z).
\end{aligned}
$$

- `c`: keskipisteen sijainti suhteessa ytimeen.
- `A`: amplitudit / ulottuvuudet akseleilla.
- `k_y`, `k_z`: kokonaislukuiset suhdeluvut; v0.1: 1–4.
- $\delta$: akselien vaihe-erot.
- `ROTATION`: koko radan/koordinaatiston rotaatio suhteessa orbitaaliin.

Kokonaislukusuhteilla rata sulkeutuu yhden perusvaiheen jälkeen. **Radan sulkeutuminen ei takaa kuultua perustaajuutta**: symmetria voi tuottaa esimerkiksi vain kaksinkertaisen taajuuden, DC:n tai hiljaisuuden. Keskipisteen siirto on tämän vuoksi tärkeä *musiikillinen* parametri. Myöhemmässä versiossa voidaan lisätä ei-kokonaislukuiset suhdeluvut ja kvasiperiodiset radat, mutta tällöin yksiselitteinen soittokorkeus ei ole aina taattu.

### 3.3 Orbitaalin rotaatio

Kun halutaan pyörittää kenttää, evaluoidaan se rotaatiomatriisilla muunnetussa pisteessä:

$$
\psi_{\mathrm{rot}}(\mathbf q)=\psi(R^{-1}\mathbf q).
$$

Tämä säilyttää itse orbitaalin matemaattisen muodon. Hidas `SPIN`-modulaatio voi tuottaa jatkuvaa timbraalista muutosta. Kentän rotaatio ei saa vaikuttaa varsinaiseen audiovaiheeseen eikä vahingossa nollata oskillaattoria.

### 3.4 Superpositio

Versiossa 0.1 voidaan toteuttaa kahden **reaalisen, normalisoidun** tilan yhdistelmä samalla $n$-tasolla:

$$
\Psi(\mathbf q)=w_A\psi_A(\mathbf q)+w_B\psi_B(\mathbf q),\qquad
w_A^2+w_B^2=1.
$$

Esimerkiksi $w_A=\cos\alpha$, $w_B=\sin\alpha$, missä $\alpha\in[0,\pi/2]$. Ortogonaalisille tiloille tämä säilyttää kentän kvanttimekaanisen normin. Yhdistelmä on itsekin sallittu kvanttitila. Jos tilat ovat eri $n$-tasoilta, oikea ajallinen kehitys sisältäisi niiden eri energioihin liittyvät kompleksiset suhteelliset vaihetekijät: siksi **eri $n$-tasojen morphing jätetään v0.1:n ulkopuolelle tai merkitään myöhemmin selvästi taiteelliseksi interpolaatioksi**.

### 3.5 Taajuus ja ulostulo

- Sisäinen taajuusohjaus `1 V/oct`; nimellistaajuus esimerkiksi C4 = 261,626 Hz, kun pitch-offset ja CV ovat nolla.
- $f=f_{\mathrm{ref}}\,2^{\mathrm{pitch}+V_{\mathrm{oct}}}$; negatiivinen vaiheaskel voidaan tukea myöhemmin erillisenä suunnanvaihtona.
- Lähtö tavoitellaan VCV Rackin tyypilliselle audiosignaalin tasolle `±5 V`, hallitulla vahvistuksella.
- `LEVEL` toimii viimeisenä gain-säätönä. Per-orbitaali **kiinteä, etukäteen määritelty kalibrointikerroin** tasaa käyttöä, mutta ei muuta $\psi$:n laskentaa; ei huomaamatonta nopeatempoista AGC:tä.
- Oletuksena maltillinen DC-estosuodin (esim. noin 5 Hz), koska yksittäinen kentän läpi kulkeva rata voi tuottaa merkittävän keskiarvon. Käyttäjälle voidaan tarjota `DC COUPLED` -valinta myöhemmin.
- Nollaa lähestyvän signaalin voimakas automaattinen kompensointi kielletään: solmupinnalla kulkeminen saa tuottaa hiljaisuuden.

## 4. Käyttöliittymä ja CV

### 4.1 Etupaneelin pääsäätimet (MVP)

| Säädin | Toiminta |
|---|---|
| `ORBITAL A` | Ensimmäisen tilan valinta (n/l/reaalinen orientaatio) |
| `ORBITAL B` | Toisen tilan valinta; aluksi vain sama $n$ sallitaan superpositiossa |
| `MIX` | A/B-superposition kulma, normitettu sekoitus |
| `PITCH` | Taajuusoffset oktaaveina/puolisävelaskelina |
| `RADIUS` | Lukuradan kolmen akselin yhteinen skaala |
| `OFFSET` | Lukuradan keskipisteen etäisyys ytimestä; yksinkertainen v0.1-ohjaus (esim. X-akselilla) |
| `SHAPE` | Ratapresetit / akselisuhteet ja vaihe-erot, suljettu rata säilyy |
| `TILT` | Radan tai kentän kallistus (kiertokulma) |
| `SPIN` | Kentän hidas kierto; bipolaarinen nopeus |
| `LEVEL` | Lähtötaso |

Yksinkertainen ensiversio ei tarvitse kaikkia mahdollisia 3D-akselikohtaisia säätimiä paneeliin. Tarkat `x/y/z`-offsetit, akseliamplitudit ja vaihe-erot voidaan aluksi tarjota kontekstivalikossa. Tärkeintä on päästä nopeasti kuultavaan lopputulokseen.

### 4.2 Liitännät

**Tuloliitännät:** `V/OCT`, `FM`, `RADIUS CV`, `OFFSET CV`, `MIX CV`, `TILT CV`, `SPIN CV`, `RESET/SYNC`.

**Lähtöliitännät:** `AUDIO` (mono), mahdollisesti erillinen `X`, `Y`, $Z$ CV-ulostulo myöhemmässä versiossa.

`FM` on aluksi lineaarinen tai eksponentiaalinen malliltaan *erikseen dokumentoitava*, ettei CV:n vaikutus jää epäselväksi. Määrittele myös jänniteskaalat ja tulokohtaiset vaimennukset toteutusvaiheessa. Audiotaajuisia CV-muutoksia käsitellään näytekohtaisesti; jatkuvien käsisäätimien muutokset tasoitetaan napsahdusten välttämiseksi.

### 4.3 Visualisointi

Keskialueella on orbitaalin kolmiulotteinen, hitaasti liikkuva **todennäköisyyspilvi** sekä lukurata ja sen tämänhetkinen piste. Positiivinen/negatiivinen aaltofunktion vaihe kuvataan kahdella selvästi erottuvalla värillä; tiheys tai pistepilven opacity perustuu $|\psi|^2$:een. Tämä grafiikka **ei** ole audiosignaalin laskentamenetelmä.

UI laskee tai piirtää pilven omalla matalalla päivitysnopeudellaan, esimerkiksi 15–30 FPS. Audio-thread ei piirrä eikä odota UI:ta. Näytölle riittää aluksi kevyt 2D-projektio, mikäli oikea 3D on liian suuri työ v0.1:een.

## 5. DSP-arkkitehtuuri

```text
VCV Rack: parameter knobs + CV inputs
                │
                ▼
         ParameterMapper
       (scale + smoothing)
                │
                ▼
         PhaseAccumulator ◄── V/OCT + FM
                │
                ▼
         Trajectory3D
    (Lissajous + offset + rotation)
                │
                ▼
        OrbitalEvaluator
   (R_nl × real spherical harmonic)
         A            B
          ╲          ╱
           ▼        ▼
           QuantumMixer
                │
                ▼
        DCBlock → Gain → AUDIO
```

**Ehdotettu koodijako:**

- `QuantumNumbers.h` — $n$, $l$, $m$/reaaliorientaation tunniste ja validointi.
- `HydrogenOrbitals.h/.cpp` — normalisoitujen kenttien analyyttinen evaluointi ja yksikkötestit.
- `Trajectory3D.h/.cpp` — deterministiset, suljetut audiotaajuiset 3D-radat.
- `OrbitalEngine.h/.cpp` — vaihe, kentän arviointi, superpositio ja audiokäsittely; **ei VCV-riippuvuutta**.
- `OrbitalModule.cpp` — Rack-parametrit, portit, patch-persistenssi.
- `OrbitalWidget.cpp` — etupaneeli, näyttö ja grafiikka.

### 5.1 Reaaliaikainen prosessointi

```cpp
void OrbitalEngine::processSample(const Controls& c, float& out) noexcept {
    const double f = computeFrequency(c.pitch, c.fm);
    const double theta = twoPi * phase_;
    Vec3 p = trajectory_.evaluate(theta, c.path);
    p = rotateIntoFieldCoordinates(p, c.rotation);

    const double a = orbitals_.evaluate(c.stateA, p);
    const double b = orbitals_.evaluate(c.stateB, p);
    const double sample = c.weightA * a + c.weightB * b;

    out = outputStage_.process(static_cast<float>(sample), c.level);
    phase_ = wrap01(phase_ + f / sampleRate_);
}
```

Tämä on **pseudokoodia**, ei suoraan käännettävä header-sopimus: esimerkiksi suotimien tila, parametrien tasoitus, poistettavat B-tilan laskut, reset-käsittely ja mahdollinen oversampling tarkentuvat toteutuksessa.

### 5.2 Optimointi

- Suunnittele v0.1 tukemaan $n=1,\ldots,4$, $l=0,\ldots,3$, $l<n$. `4f` voidaan aluksi jättää UI:ssa piiloon, jos se tarvitsee vielä testejä.
- Esilaske **vain matemaattiset kertoimet** (normalisoinnit, faktoriaalit, Laguerre-polynomien kertoimet), kun orbitaali vaihtuu. **Älä esilaske kenttää tai aaltomuotoa.**
- Käytä pienille `n/l`-arvoille eksplisiittisiä, testattuja karteesisia polynomimuotoja tai stabiileja rekursioita. Vältä `atan2`/`acos`-laskuja jokaisella näytteellä, jos sama voidaan laskea karteesisesti.
- Käsittele $r=0$ ilman jakamista nollalla; polynomimuotoinen arviointi helpottaa tätä.
- Vältä audiosäikeessä muistivarauksia, lukkoja, tiedostokäsittelyä ja käyttöliittymäoperaatioita.
- Jos B-tilan paino on nolla, jätä B:n arviointi tekemättä; vastaavasti A:lle.
- CV-signaali luetaan audiotaajuudella, mutta hitaita parametreja voidaan tarvittaessa päivittää harvemmin ja interpoloida turvallisesti.
- Ota CPU-profiilit ensin ilman oversamplingia. Lisää valinnainen 2×/4× oversampling asianmukaisella alipäästösuodatetulla alasnäytteistyksellä vasta, kun aliasointi on mitattu; pelkkä tiheämpi laskenta ei yksin riitä.

### 5.3 Napsahdukset ja aliasointi

Käsisäätimien muutokset tasoitetaan. Diskreetti orbitaalinvaihto tehdään esimerkiksi 5–20 ms:n ristiinhäivytyksellä. Suurilla taajuuksilla jyrkät spatiaalisen kentän muutoskohdat ja suuret geometriset suhdeluvut voivat synnyttää Nyquist-rajan ylittäviä harmonisia. Mittaa aliasointi käytännössä ja määritä rajat taajuudelle sekä oversampling-asetukselle.

## 6. MVP / v0.1

**Ensimmäinen soitettava julkaisu:**

1. Mono-oskillaattori, `V/OCT`, `PITCH`, `AUDIO` ja lähtötason säätö.
2. Oikeat, normalisoidut `1s`, `2s`, `2p_x`, `2p_y`, `2p_z` sekä viisi `3d`-orbitaalia.
3. Näytekohtainen, suljettu 3D-lukurata; säädöt `RADIUS`, `OFFSET`, `SHAPE`, `TILT`.
4. Orbitaalien rotaatio `SPIN` ja kaksi saman $n$-tason tilaa sekoittava `MIX`.
5. Tärkeimpien parametrien CV, DC-estosuodin ja hallittu ulostulotaso.
6. Kevyt radan visualisointi; 3D-pilvi saa olla v0.1:n lopussa toteutettava lisä.

**V0.1:n ulkopuolelle:** eri $n$-tasojen fysikaalinen aikaevoluutio, monimutkaiset epäharmoniset radat, polyfonia, ulkoinen wavetable-tuonti, jälkikäsittelyefektit ja VST3.

**MVP:n oletuspresetiä varten:** käytä siirrettyä Lissajous-rataa, jonka säde vaihtelee ja joka ei pysy kokonaan valitun orbitaalin solmupinnalla. Tämä on tärkeää: esimerkiksi `1s` luettuna tarkalla keskipisteisellä ympyräradalla antaisi vakioarvon eikä soivaa ääntä.

## 7. Testaus ja hyväksymiskriteerit

### 7.1 Matemaattiset testit

- `1s` ja `2p_z` vastaavat luvun 2.3 referenssifunktioita valituissa pisteissä.
- $p_x$ vaihtaa etumerkkiä, kun $x$ vaihtaa etumerkkiä; $p_z=0$, kun $z=0$.
- `d`-orbitaalien oikeat solmupinnat ja pariteetti vastaavat teoriaa.
- Reaalisten tilojen normalisointi ja ortogonaalisuus testataan erillisellä offline-integraatiolla tarkoituksenmukaisella numeerisella toleranssilla (integroinnin katkaisu huomioiden).
- $r=0$ ja suuret `r`-arvot antavat äärelliset luvut ilman `NaN`/`Inf`-arvoja.
- Saman $n$-tason ortogonaalisten tilojen superpositio säilyttää normin valitulla painotuksella.

### 7.2 Audiotestit

- `1 V/oct` tuplaa **lukuradan läpikäyntinopeuden**. Kuultu alin spektripiikki voi symmetrian vuoksi silti olla 2×/3× suhteessa tähän; käyttäjälle täytyy tehdä ero selväksi.
- Soiva ja toistuva ääni löytyy vähintään yhdestä oletusasetuksesta jokaiselle MVP:n orbitaliperheelle.
- Orbitaalinvaihto ja nuppien liikuttelu eivät tuota hallitsemattomia napsahduksia.
- Nullapolku / solmupintaa seuraava rata saa olla hiljainen ilman että vahvistus räjähtää.
- Audioprosessissa ei tapahdu muistivarauksia eikä käyttöliittymäriippuvia kutsuja.
- Äänentuotto toimii vähintään 44,1 / 48 / 96 kHz:n Rack-engine-taajuuksilla.
- Aliasointi ja CPU-kuorma mitataan sekä yhdessä että useassa rinnakkaisessa moduuli-instanssissa.

## 8. Kehitysjärjestys

**Vaihe A — matematiikka erilleen.** Toteuta `HydrogenOrbitals` ja testit. Varmista arvot, solmupinnat, normalisointi ja etumerkkikonventio. Ei vielä Rack-grafiikkaa.

**Vaihe B — ensimmäinen ääni.** Toteuta `Trajectory3D` ja yksittäinen `2p_z`-oskillaattori; tulosta suoraan audiosignaali. Varmista, että radan offset, kulma ja säde todella muuttavat spektriä.

**Vaihe C — soittokelpoinen moduuli.** Lisää muut tilat, patch-parametrit, `V/OCT`, CV, tasoitus ja lähtöaste. Profiloi CPU ja analysoi FFT:llä spektri sekä aliasointi.

**Vaihe D — superpositio ja visualisointi.** Lisää samaan $n$-tasoon rajattu A/B-miksaus, kentän rotaatio sekä lukuradan ja orbitaalin grafiikka. Viimeistele paneeli Filth Assemblyn limenvihreällä visuaalisella kielellä.

## 9. Avoimet tekniset valinnat

- **Soittokorkeus:** käytetäänkö pitch-arvona lukuradan taajuutta (MVP:n päätös: kyllä) vai tarjotaanko myöhemmin spektrin perustaajuutta seuraava *pitch compensation* -tila? Jälkimmäinen ei aina ole mahdollinen.
- **Suuren taajuuden äänenlaatu:** mitkä oversampling-moodit ovat tarpeen todellisen spektrianalyysin perusteella?
- **Grafiikka:** toteutetaanko pistepilvi kokonaan erillisessä käyttöliittymälogiikassa vai tarvitaanko myöhemmin GPU-kiihdytystä?
- **Orbitaalivalikoima:** näytetäänkö kaikki $n\le4$ -tilat vai kuratoitu lista? Matematiikkaydintä ei tarvitse rajata UI:n valikoimaan.
- **VST-laajennus:** säilytetäänkö moottorin API mahdollisimman ympäristöriippumattomana? **Kyllä.**

## 10. Lähteet ja tekniset referenssit

1. [Lecture Notes in Physical Chemistry II — The hydrogen atom](https://www.physchem.no/physchem_book/chapters-11-hydrogen.html). Säteittäisen osan normalisointi, orbitalit sekä reaaliset palloharmoniset.
2. [TU Delft — Introduction to Quantum Mechanics, Equations and constants](https://interactivetextbooks.tudelft.nl/introduction-to-quantum-mechanics/content/equationsandconstants.html). Säteittäiset funktiot ja Laguerre-polynomit.
3. [VCV Rack — Plugin API Guide](https://vcvrack.com/manual/PluginGuide). Parametrit, audioprosessointi, sample rate, CV, polyfonia ja SIMD.
4. [VCV Rack — Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial). Rack SDK, C++-moduulin perusrakenne ja audion jännitetasot.

---

**Suunnitteluperiaate:** *Real atomic mathematics. Invented trajectories. Unpredictable sound.*
**Filth Assembly — Sound decisions.**
