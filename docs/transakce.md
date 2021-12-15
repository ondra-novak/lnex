#Směna

```
{
	"buyer": ""
	"seller": ""
	"order":"",
	"withdraw":true/false
	"wire":{
		"ref":"",
		"vs":"",
		"ss":"",
		"ks":"",
		"amount":"",		
	},
	"asset":{
		"invoice":"",
		"type":"ln|address"
		"amount":"",
		"fee":"",
	},
	"state": {
		"send":[{
					"timestamp":0,
					"amount":0,
					"id":0
		 }],
		"receive":[{
					"timestamp":0,
					"amount":0,
					"id":0
		 }],		
		"withdraw":{
			"filled":true/false,
			"timestamp":0,
			"last_error":""
		},
		"refund":{
			"txs":[{
					"timestamp":0,
					"amount":0,
					"id":0
				  }]
			"filled":true/false,
		},		
	},
	"status": "created/open/open_recv/open_send/filled/attantion/refund/canceled",
	"status_timestamp: 0,

}
	
```

* **buyer** -  ID bankovního spojení na kupujícího
* **seller**-  ID bankovního spojení na prodávajícího
* **order** -  ID pokynu, který je zdrojem této směny (seller)
* **withdraw** - pokud je nastaveno na true, pak se nejedna o směnu, ale withdraw. V takovém případě
				se používají pouze informace **asset** a **state/withdraw** a **status**
				Pokud je směna vytvořena s tímto příznakem, tak po zamčení balance se přepne do stavu
				**filled** a mohou se coiny odeslat
* **wire** - informace o platbě 
				- ref - reference (zpráva pro příjemce)
				- vs - variablni symbol
				- ss - specifický symbol
				- ks - konstantní symbol
				- amount - množství k zaplacení
 				
* **asset** - Informace o kryptu
				- amount - objem koupeného krypta
				- invoice - LN invoice
				- type - typ invoice
				- fee - fee zaplacené za směnu (blokuje se o fee více)
				
* **state** - stav směny
				- send - stav zaslané platby			
				- receive - stav přijaté platby
				- withdraw - stav placení invoice
				- refund - stav refundu
					- filled - true, pokud je tato položka splněna
					- last_error - text poslední chyby
					- txs - výpis transakcí pro splnění položky. Předpokládá se jedna, ale pokud by
						kupec zaslal transakci ve více platbách, pak zde bude víc transakcí
						například pokud by na první pokus zaslal méně.
					
* **status** - postavení (status)
				- created - směna je vytvořena, čeká se na kupujícího až potvrdí objednávku
						- částka je blokovaná
				- open - směna je otevřena, probíhá plnění - zatím nic nevyplněno
						- částka je blokovaná
				- open_recv - směna je otevřena, probíhá plnění - čeká se na přichod platby (odeslano)
						- částka je blokovaná
				- open_send - směna je otevřena, probíhá plnění - čeká se na potvrzení platby (přijato)
						- částka je blokovaná
				- filled - směna je dokončena a může být vyplacena
						- částka je blokovaná
				- attantion - směna vyžaduje zásah prodávajícího
						- částka je blokovaná
				- refund - byl zahájen refund, výplata kupujícímu už není možná 
						- částka je blokovaná
				- canceled - transakce byla stornována, 
						- čáska není blokována
				- proměnná status neexistuje, pak je ve fázi vytváření. Vytvoření směny vyžaduje
				  asynchroní potvrzení pro zamknutí balance.
										
* **status_timestamp** - timestamp změny statusu


## Jednotlivé fáze




### Created

Kupující akceptoval směnu, byly mu zobrazena částka a platební údaje. Nyní se čeká 10 minut,
uživatel může čekání zkrátit kliknutím na potvrzovací tlačítko ("Zaplaceno")

Během této doby může uživatel požádat o zrušení objednávky.

### Open

Do tohoto stavu přechází objednávka automaticky, nebo ručně potvrzením platby kupujícím.

Systém v takovém případě se dotazuje stavu bankovních účtů kupujícího i prodávajícího

Dotazování probíhá takto - první dotaz proběhne ihned po překlopení objednávky do stavu Open a to 
na účet kupujícího. Pokud není platba potvrzena, další dotaz proběhne za 1 minutu a s dalšími dotazy 
se interval zvyšuje:
1,2,4,8,16,32,64,128,256,512 - zůstává na 512.

(návrh, agregace požadavků, pokud počet požadavků na jeden účet je větší množství, dotazuje se jednou a vyřizují se hromadně)

Směna přechází do dalšího stavu podle výsledku dotazování.

 - platba potvrzena na obou bankách - přechod do stavu **filled**
 - platba potvrzena, ale nižší, než bylo požadováno - přechod do stavu **attantion**
 - platba potvrzena, ale vyšší, než požadováno - přechod do stavu **attantion**
 - platba potvrzena pouze u kupujícího - nic - čeká se dál, max 15 dní, pak přejde do stavu **canceled**
 - platba potvrzena pouze u prodávajícího - do 24h je stále **open** pak přejde do stavu **attantion**

### Filled

Směna je dokončena, nakoupené množství se začne posílat přes invoice do LN peněženky. Systém
se snaží opakovaně zaplatit invoice dokud nevyprší. V tomto stavu může zasáhnout kupující a změnit
invoice. Pokud invoice vyprší, musí kupující nahrát nové invoice

Z hlediska systému je směna plně dokončena a způsob převodu do peněženky je záležitost kupujícího. Jakmile je nakoupená měna převedena, je to poznačeno ve /state/withdraw/filled_amount

### Attantion

Při přechodu to tohoto stavu dostává prodávající notifikaci, že transakce vyžaduje pozornost.

Prodávající musí rozhodnout, zda se směnu dokončí, nebo refunduje
 - prodávající zvolí refund, směna přejde do stavu **refund**
 - prodávající zvolí finish, směna přejde do stavu **filled**
 
V případě že kupující poslal jinou částku, než objednal se při **finish** přepočítá blokovaná částka, za předpokladu, že není překročena celková balance orderu. V takovém případě nelze **finish** zvolit, a lze zvolit pouze **refund**

V případě, že platba nebyla potvrzena u kupujícího, obdrží prodávající informaci o možném riziku praní špinavých peněz a je mu doporučen **refund**.

V režimu **attantion** může směna zůstat až 7 dní, potom se automaticky přepne do **filled**

### Refund

V tomto stavu již není možné transakci dokončit ve stavu filled. Prodávající má stále blokované prostředky dokud neprovede refund platbu ve stejné výši ale opačným směrem. 

Pokud je odchozí transakce potvrzena, pak směna přejde do stavu **canceled + active=false** 

Ve stavu refund není timeout. Dokud prodávající nepošle refund platbu, prodávaná měna se neodemkne

### Canceled

Směna byla komplet zrušena, a neblokuje prostředky


## Kontrola směn po finálních stavech

Systém aktivně nekontroluje dokončené směny, ale v případě, že získá výpis z účtu ať už při jiné směně, nebo ručně, pak může provést dodatečnou kontrolu i směn ve stavu **canceled** které nebyly refundovány

Pokud je zaznamenaná platba na účtu příjemce (prodávající), přejde tato směna ihned do stavu **refund** -tj začne blokovat protředky dokud není platba vrácena

Toto opatření je pro situace, kdy směna je storonována nebo vyprší timeout a přesto nakonec platba dorazí. Neexistuje žádný timeout který by ukončoval tento monitoring.

## Timeouty

### created 

10 minut

### open

1 hodina pro buyer - kupující by měl zaplatit co nejdřív, jinak je směna zrušena
7 dní pro seller - platba se může v bankovním systému zdržet až týden.

### attantion

7 dní pro přepnutí na **filled**

### refund

není timeout

### filled

Timeout na vyplacení invoice je dán expirací invoice
Pokud invoice vyprší, musí uživatel vložit jiné

### canceled

není timeout

# Deposit

Deposit slouží k naplnění orderu

```
{
	"order": "",
	"amount": 0,
	"filled": false/true,
	"invoice: "invoice text",
	"expiration": 0,
}
```

# Monitoring bank

Monitorování bankovních účtů je plánováno.

```
{
	"attempt":0,
	"failures":0,
	"last_error":"",
	"next_check":0,
}
```

Monitoring je naplánován na **next_check**. Pokud **next_check** není definován, pak se nemonitoruje. Při každém cyklu je zvýšeno číslo **attempt**. Hodnota 0 znamená, že se ještě nemonitorovalo. První pokus nastaví toto číslo na 1, atd...

Counter **failures** počítá po sobě jdoucí selhání. Pokud počet selhání dosáhne určitého množství, je
bankovní spojení vyřazeno a označeno za neplatné

Při selhání se do pole **last_error** vloží popis chyby

## Plánování

Pro záhájení monitoringu (otevření směny), je  pole "attempt" vynulováno a pole "next_check" je též vynulováno.

Po každém dotazu se zvýží **attempt** o 1. Z toho se spočítá další dotaz jako 2^attempt minut.

Po prvním dotazu se tedy další opakuje za 2 minuty, další za 4, 8, 16, 32, 64 ,128, 256, 360. Víc jak 360 minut se nečeká. Tedy každý další dotaz je naplánován na 360 minut (banky garantují dostupnost služem min 4x denně)

Po získání dat z banky se data použíji k analýze otevřených směn. Pokud zůstávají některé směny otevřené, pak se naplánuje další monitoring. Pokud ne, položka "next_check" se nastaví na null, a monitoring se zastaví

Pokud dojde k selhání, zvýší se čítač **failures** o 1 a naplánuje se další pokus na 2^failures, max 360
Pole **failures** se nuluje v okamžiku, kdy se podaří úspěšně provést načtení dat. Po **12** selhání se bankovní spojení vyřadí


