# Arbejds-portfolio

## Produktkrav

- Hvordan laver man positionsbestemmelse af f.eks. mobile enheder, uden adgang til GPS?
- Kan man tælle hvor mange enheder (og dermed måske personer), der er i et område?

Er man indendørs har man ofte ikke mulighed for at få en god positionsbestemmelse med GPS. I hvertfald ikke i metrosystemet, eller indenfor i et storcenter. Ikke på en måde så man tydeligt kan se hvor bygningen, eller i hvilket lokale, en person eller en enhed findes.
Desuden kan man ønske at få oplysninger om hvormange (personer/enheder) der færdes i et område, uden at de har tilmeldt sig. Men må man det?

Hvordan kan man bestemme hvor en enhed befinder sig, uden at have adgang til GPS? Inde i bygninger er GPS signaler ikke særligt tilgængelige, og/eller særlige nøjagtige.

### Formål

I skal undersøge hvordan det er muligt at bestemme

- om en enhed (mobiltelefon) er i området
- hvor i området
- hvor nøjagtigt

Samtidig skal I også forholde jer til

- hvad det er tilladt at monitorere
- om hvem
- hvormeget, hvorlænge osv.

Undersøg hvordan man kan håndtere viden om enhederne så lidt som muligt, og alligevel registrere at der er enheder tilstede, hvor og hvornår.

### Produktmål

I skal lave en opstilling med flere IoT-enheder, som indsamler oplysninger om mobilenheder (f.eks.mobiltelefoner) med wifi.
Det område I arbejder med kan være jeres bord eller hele lokalet.

Ved at placere flere IoT-enheder i hjørnerne af området, kan man få oplysninger om afstanden fra hver IoT-enhed til mobilenheden. På baggrund af disse oplysninger kan beregne geometrisk, hvor mobilenheden befinder sig. Nøjagtigheden bliver ikke så stor, men det må vi leve med.

I skal programmere IoT-enhederne i opstillingen til at kommunike med hinanden, så de kan samle oplysninger om afstand til mobilenheder, så der bliver mulighed for stedsbestemmelse.
Skal en af IoT-enhederne være server, eller skal de alle samle data? Det er jeres valg.

Jeres opstalling af IoT-enheder skal indsende data til MQTT serveren (den som er sat op lokalt) med data om

- et unikt id,
- et tidspunkt, og
- et kordinat (x,y)

En anden applikation (som ikke er indeholdt i denne opgave) kan så udtrække oplysningerne, og presentere statistiske sammendrag som hyppighed over dagen, et heatmap over hvor i området, osv

### Procesmål

Beskriv fremgangsmåde og teknologier der kan bruges til at nå målet.

Der er flere forskellige teknologier og strategier at vælge, og I skal lave noter om hvilke i har fundet oplysniger om, og lave forsøg med at implementere en eller flere. Implementerirngerne kan starte som nogle forsøg på hurtigt at se om det er muligt at bruge teknologien. Hvis det ikke er, kan man gå videre til noget andet. Hvis der er gode resultater, kan det bruges til at kombinere med flere.

Beskriv hvad I finder, hvad I prøver af, og hvordan den nye viden kan bruges.

Her er logbog og portfolie vigtige dokumentationsformer. I behøver ikke lave en traditionel rapport, bare i har ført logbogen og noteret i portfoliet. Helst undervejs.

### Hovedproblematikker

Der er et antal hovedproblematikker:

- Triangulering
    - Hvordan ser matematikken ud?
- Afstand mellem målestation og mobil
    - RSSI
        - Kan findes med “sniffing” i wifi modulet promicius mode
    - Roundtrip
- Kommunikation mellem målestationer
    - ESP MESH
    - ESP NOW

### Persondata

Når vi indsamler data om alle enheder i “luften”, så er nogen af disse data noget der har med perosner at gøre. Det gør at vi skal overveje hvordan vi behandler disse oplysninger. Projektet kommer muligvis i berøring med Persondata lovgivningen, som igen refererer til EUs GDPR.

- Hvilke disse data er personlige?
    - Er de følsomme?
    - Hele tiden, eller nogen gange?
    - Er f.eks. en MAC-adresse følsom personlig data?
- Beriges data ved at indsamle f.eks. MAC adressse, sammen med tid og sted?
    - Hvad gør det ved data-ansvaret?
- Kan vi begrænse hvilke oplysninger der opsamles, i forhold til formålet?

---

## Spørgsmål som indgår i bedømmelsen
Opgavens karakter er udpræget eksplorativ. Det vil sige at I skal undersøge hvad og hvordan det kan lade sig gøre. Så undesøgelsen tæller i sig selv. Det tæller også at kode noget. Så altså begge dele.

- [Hvordan ser løsningen ud](#hvordan-ser-løsningen-ud)
    - Arkitektur
    - Teknologier
- [Dataprocess](#dataprocess)
    - primær dataopsamling
    - data berigelse
    - beregning
    - data lagring
    - evt data visualisering
- [Datasikkerhed](#datasikkerhed)
    - Følsomhed
        - hvad er personlige data?
        - hvad er risikoen ved at behandle andre folks personlige data?
        - i hvor høj grad er de data projektet benytter personlige?
            - er data mere personlige når de beriges?
        - hvor længe er der brug for at have personlige data i projektet?
    - Beskyttelse
        - hvem kan få adgang?
        - hvor svært er det for udenforstående at skaffe sig adgang til data?
        - eller at lytte med?

--- 

## Besvarelse

### Hvordan ser løsningen ud?

Løsningen består af flere små IoT-enheder (ESP32) placeret rundt i klasselokalet. Hver enhed fungerer som en passiv WiFi-sniffer og som netværksklient:

- Sniffer: Kører WiFi i promiscuous mode og opsamler MAC adresser og RSSI.
- Netværk: Sender anonymiserede rapporter (hash af MAC, RSSI, sensorens x,y og tidsstempel) til en MQTT-broker.
- Web Server: Læser rapporter fra MQTT-broker og republisher x og y koordinat når den får 3 rapporter på samme hashed MAC adresse (1 fra hver sensor)

Arkitektur:

- Sensorlag: Flere ESP32-enheder i kendte positioner (angivet som `DEVICE_X`/`DEVICE_Y`).
- Kommunikationslag: MQTT over WiFi til central lagring.
- Behandlingslag: Web server abonnerer på MQTT, samler målinger, konverterer RSSI -> afstand og udfører triangulering for at estimere position.

Teknologier: ESP32 (Arduino framework), WiFi promiscuous sniffing, SHA-256 hashing for pseudonymisering, NTP for tidsstempler, MQTT (PubSubClient) og en simpel RSSI->afstand-model + triangulerings-algoritme.

### Dataprocess

Primær dataopsamling:

- Hver sensor opsamler: hashed device id (SHA-256 af MAC), RSSI, sensor_x, sensor_y og et NTP-synkroniseret tidsstempel i formatet `yyyy/MM/dd hh:mm:ss:ms`.

Data-berigelse og behandling:

- Server samler samtidige målinger for samme hashed id fra flere sensorer inden for et lille tidsvindue.
- RSSI-værdier omregnes til grove afstandsskøn, ved tre uafhængige målinger udføres trilateration/triangulering for at estimere position.

Data lagring og publicering:

- Hver sensor publicerer JSON til MQTT med felter: `device_id`, `rssi`, `sensor_x`, `sensor_y`, `timestamp`.

### Datasikkerhed

Hvilke data er personlige?

- MAC-adresser er identifikatorer som, i kombination med tid og position, kan afsløre bevægelsesmønstre og dermed være følsomme.

Risici ved berigelse:

- Kombination af MAC + tid + sted øger risikoen for re-identifikation. Derfor må data behandles efter princippet om dataminimering.

Derfor bliver MAC-adresser hashet inden de sendes til mqtt-broker, så vi aldrig gemmer rå MAC-adresser