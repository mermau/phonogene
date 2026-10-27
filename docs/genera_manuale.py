# Genera il manuale d'uso del plugin Phonogene (Collateral Effects) in PDF.
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Image,
                                PageBreak, Table, TableStyle, KeepTogether, NextPageTemplate, CondPageBreak)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from PIL import Image as PILImage

F = "/usr/share/fonts/truetype/dejavu/"
pdfmetrics.registerFont(TTFont("Sans", F + "DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("Sans-Bold", F + "DejaVuSans-Bold.ttf"))
pdfmetrics.registerFont(TTFont("Sans-Oblique", F + "DejaVuSans-Oblique.ttf"))
pdfmetrics.registerFont(TTFont("Cond-Bold", F + "DejaVuSansCondensed-Bold.ttf"))
pdfmetrics.registerFont(TTFont("Serif", F + "DejaVuSerif.ttf"))
pdfmetrics.registerFont(TTFont("Serif-Italic", F + "DejaVuSerif-Italic.ttf"))
pdfmetrics.registerFont(TTFont("Serif-Bold", F + "DejaVuSerif-Bold.ttf"))
from reportlab.pdfbase.pdfmetrics import registerFontFamily
registerFontFamily("Sans", normal="Sans", bold="Sans-Bold", italic="Sans-Oblique", boldItalic="Sans-Bold")
registerFontFamily("Serif", normal="Serif", bold="Serif-Bold", italic="Serif-Italic", boldItalic="Serif-Bold")

INK = colors.HexColor("#262624")
DIM = colors.HexColor("#5c5b55")
ENAMEL = colors.HexColor("#cdcbc2")
PLATE = colors.HexColor("#ecebe5")
AMBER = colors.HexColor("#c98a1e")
RED = colors.HexColor("#b8331a")

W, H = A4
M = 20 * mm

body = ParagraphStyle("body", fontName="Serif", fontSize=10, leading=14.6, textColor=INK, alignment=TA_JUSTIFY,
                      spaceAfter=6)
small = ParagraphStyle("small", parent=body, fontSize=8.6, leading=11.8, textColor=DIM)
h1 = ParagraphStyle("h1", fontName="Cond-Bold", fontSize=19, leading=23, textColor=INK, spaceBefore=4, spaceAfter=10)
h2 = ParagraphStyle("h2", fontName="Cond-Bold", fontSize=12.5, leading=16, textColor=INK, spaceBefore=10, spaceAfter=5)
h3 = ParagraphStyle("h3", fontName="Sans-Bold", fontSize=10, leading=13, textColor=INK, spaceBefore=6, spaceAfter=3)
caption = ParagraphStyle("caption", fontName="Sans-Oblique", fontSize=8.2, leading=10.5, textColor=DIM,
                         alignment=TA_CENTER, spaceBefore=3, spaceAfter=10)
bullet = ParagraphStyle("bullet", parent=body, leftIndent=12, bulletIndent=2, spaceAfter=3, alignment=TA_LEFT)
cell = ParagraphStyle("cell", fontName="Serif", fontSize=8.8, leading=11.5, textColor=INK)
cellb = ParagraphStyle("cellb", parent=cell, fontName="Sans-Bold", fontSize=8.4)
tip = ParagraphStyle("tip", parent=body, fontSize=9.4, leading=13.4, leftIndent=8, rightIndent=8,
                     textColor=INK, alignment=TA_LEFT)

story = []


def P(text, style=body):
    if style is h1:
        story.append(CondPageBreak(80 * mm))   # un titolo di capitolo non resta in fondo alla pagina
    elif style is h2 or style is h3:
        story.append(CondPageBreak(30 * mm))
    story.append(Paragraph(text, style))


def B(items):
    for t in items:
        story.append(Paragraph(t, bullet, bulletText="•"))


def img(path, width, cap=None):
    w, h = PILImage.open(path).size
    story.append(Image(path, width=width, height=width * h / w))
    if cap:
        story.append(Paragraph(cap, caption))


def box(title, text):
    t = Table([[Paragraph(f"<b>{title}</b>", ParagraphStyle("bt", parent=tip, fontName="Sans-Bold", fontSize=9))],
               [Paragraph(text, tip)]], colWidths=[W - 2 * M])
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), PLATE),
        ("LINEBEFORE", (0, 0), (0, -1), 3, AMBER),
        ("LEFTPADDING", (0, 0), (-1, -1), 10), ("RIGHTPADDING", (0, 0), (-1, -1), 10),
        ("TOPPADDING", (0, 0), (-1, 0), 7), ("BOTTOMPADDING", (0, -1), (-1, -1), 8),
    ]))
    story.append(Spacer(1, 4))
    story.append(KeepTogether(t))
    story.append(Spacer(1, 8))


def table(rows, widths, header=True):
    data = [[Paragraph(c, cellb if (header and i == 0) else cell) for c in r] for i, r in enumerate(rows)]
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    st = [("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#b3b1a8")),
          ("VALIGN", (0, 0), (-1, -1), "TOP"),
          ("LEFTPADDING", (0, 0), (-1, -1), 5), ("RIGHTPADDING", (0, 0), (-1, -1), 5),
          ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]
    if header:
        st.append(("BACKGROUND", (0, 0), (-1, 0), ENAMEL))
    for i in range(1 if header else 0, len(rows)):
        if i % 2 == 0:
            st.append(("BACKGROUND", (0, i), (-1, i), colors.HexColor("#f5f4ef")))
    t.setStyle(TableStyle(st))
    story.append(t)
    story.append(Spacer(1, 8))


# ---------------------------------------------------------------------------- copertina
def cover(c, doc):
    c.saveState()
    c.setFillColor(ENAMEL)
    c.rect(0, 0, W, H, stroke=0, fill=1)
    # targhetta d'acciaio
    pw, ph = 120 * mm, 46 * mm
    x, y = (W - pw) / 2, H - 92 * mm
    c.setFillColor(colors.HexColor("#8f938e"))
    c.roundRect(x + 1.2 * mm, y - 1.6 * mm, pw, ph, 3 * mm, stroke=0, fill=1)
    c.setFillColor(colors.HexColor("#dfe1dc"))
    c.setStrokeColor(colors.HexColor("#2a2b29"))
    c.setLineWidth(1)
    c.roundRect(x, y, pw, ph, 3 * mm, stroke=1, fill=1)
    c.setStrokeColor(colors.HexColor("#ffffff"))
    c.setLineWidth(0.25)
    for i in range(2, int(ph / mm) - 1, 1):
        c.line(x + 2 * mm, y + i * mm, x + pw - 2 * mm, y + i * mm)
    for sx, sy in [(x + 5 * mm, y + 5 * mm), (x + pw - 5 * mm, y + 5 * mm), (x + 5 * mm, y + ph - 5 * mm), (x + pw - 5 * mm, y + ph - 5 * mm)]:
        c.setFillColor(colors.HexColor("#a6a9a4")); c.setStrokeColor(colors.HexColor("#3a3c39"))
        c.circle(sx, sy, 1.6 * mm, stroke=1, fill=1)
        c.line(sx - 1.1 * mm, sy - 0.4 * mm, sx + 1.1 * mm, sy + 0.4 * mm)
    c.setFillColor(INK)
    c.setFont("Cond-Bold", 34)
    c.drawCentredString(W / 2, y + 25 * mm, "PHONOGÈNE")
    c.setFont("Sans", 10)
    c.drawCentredString(W / 2, y + 16 * mm, "chromatique  ·  à coulisse  ·  universel")
    c.setFont("Sans-Bold", 8.5)
    c.drawCentredString(W / 2, y + 8 * mm, "COLLATERAL EFFECTS")
    # titolo
    c.setFont("Cond-Bold", 22)
    c.drawCentredString(W / 2, y - 22 * mm, "Manuale d'uso")
    c.setFont("Serif-Italic", 11)
    c.setFillColor(DIM)
    c.drawCentredString(W / 2, y - 30 * mm, "Emulazione del Phonogène di Pierre Schaeffer e Jacques Poullin")
    c.drawCentredString(W / 2, y - 36 * mm, "VST3  ·  AU  ·  Standalone  —  versione 0.2")
    # immagine dell'interfaccia
    iw = 150 * mm
    ih = iw * 740 / 1000
    c.drawImage("phonogene_ui.png", (W - iw) / 2, 30 * mm, iw, ih, preserveAspectRatio=True, mask="auto")
    c.setFont("Sans", 8)
    c.drawCentredString(W / 2, 20 * mm, "Collateral Effects  ·  2026")
    c.restoreState()


def page(c, doc):
    c.saveState()
    c.setStrokeColor(colors.HexColor("#b3b1a8"))
    c.setLineWidth(0.5)
    c.line(M, H - 13 * mm, W - M, H - 13 * mm)
    c.setFont("Sans-Bold", 7.5)
    c.setFillColor(DIM)
    c.drawString(M, H - 11 * mm, "PHONOGÈNE  ·  MANUALE D'USO")
    c.drawRightString(W - M, H - 11 * mm, "COLLATERAL EFFECTS")
    c.line(M, 13 * mm, W - M, 13 * mm)
    c.setFont("Sans", 8)
    c.drawCentredString(W / 2, 8 * mm, str(doc.page))
    c.restoreState()


doc = BaseDocTemplate("Phonogene_manuale.pdf", pagesize=A4, leftMargin=M, rightMargin=M,
                      topMargin=20 * mm, bottomMargin=20 * mm,
                      title="Phonogene — Manuale d'uso", author="Collateral Effects",
                      subject="Emulazione del Phonogène di Schaeffer e Poullin")
frame = Frame(M, 18 * mm, W - 2 * M, H - 38 * mm, id="f", leftPadding=0, rightPadding=0)
doc.addPageTemplates([PageTemplate(id="cover", frames=[frame], onPage=cover),
                      PageTemplate(id="body", frames=[frame], onPage=page)])

story.append(NextPageTemplate("body"))
story.append(PageBreak())

# ---------------------------------------------------------------------------- indice
P("Indice", h1)
toc = [
    ("1", "La macchina originale"), ("2", "Perché usare il Phonogene oggi"), ("3", "Installazione e primi passi"),
    ("4", "Panoramica dell'interfaccia"), ("5", "Il nastro: sorgenti, anello, registrazione"),
    ("6", "Modo CHROMATIQUE"), ("7", "Modo À COULISSE"), ("8", "Modo UNIVERSEL"),
    ("9", "Il carattere del nastro"), ("10", "Indicatori"), ("11", "MIDI: tastiera, CC e MIDI learn"),
    ("12", "Ricette di musique concrète"), ("13", "Limiti e note tecniche"), ("A", "Tabella dei comandi"),
]
table([[n, t] for n, t in toc], [12 * mm, W - 2 * M - 12 * mm], header=False)

# ---------------------------------------------------------------------------- 1
story.append(PageBreak())
P("1  ·  La macchina originale", h1)
P("Il Phonogène nasce a Parigi all'inizio degli anni Cinquanta, nello studio di musique concrète della "
  "radio francese, da un'idea di <b>Pierre Schaeffer</b> e dal progetto dell'ingegnere <b>Jacques Poullin</b>. "
  "La musique concrète parte da suoni registrati – rumori, voci, strumenti, oggetti – che vengono "
  "trasformati e montati direttamente sul supporto. Dopo i primi anni con i dischi a solco chiuso, il nastro "
  "magnetico rese possibile tagliare, incollare e chiudere ad anello i frammenti; il Phonogène fu lo strumento "
  "che permise di <i>suonare</i> questi anelli a velocità diverse, cioè a diverse altezze, come uno strumento.")
P("Il principio è semplice: un nastro letto più velocemente suona più acuto e più breve, più lentamente più "
  "grave e più lungo. Il Phonogène trasforma questo fatto fisico in un gesto musicale. Ne esistettero tre versioni.")
P("Il Phonogène chromatique", h2)
P("Un anello di nastro passava davanti a un'unica testina di lettura fissa e poteva essere trascinato da "
  "<b>dodici capstan di diametri diversi</b>. Una piccola <b>tastiera di un'ottava</b> sceglieva quale capstan, "
  "tramite il proprio rullo pressore, trascinasse il nastro: ogni tasto corrispondeva a una velocità, ogni "
  "velocità a un semitono. Con un solo nastro e una sola testina la macchina era <b>monofonica</b> e adatta "
  "soprattutto a suoni brevi. Le fonti attribuiscono la costruzione alla ditta Tolana (1953).")
P("Il Phonogène à coulisse", h2)
P("Detto anche a variazione continua: una <b>leva</b> agiva direttamente sulla velocità del motore, che poteva "
  "scendere quasi fino all'arresto passando per tutte le velocità intermedie. Era lo strumento dei glissandi; "
  "uno dei primi esempi noti è il lungo glissando del <i>Voile d'Orphée</i> di Pierre Henry (1953). Fu costruito "
  "dalla ditta SAREG.")
P("Il Phonogène universel", h2)
P("Completato più tardi (le fonti indicano il 1963), introduceva una <b>testina rotante</b> secondo il principio "
  "del regolatore temporale di Springer: più testine montate su un tamburo leggono a turno il nastro. In questo "
  "modo <b>altezza e durata diventano indipendenti</b>: si può trasporre un suono senza accorciarlo, o allungarlo "
  "senza renderlo più grave – un'anticipazione meccanica del time-stretching digitale.")
img("phonogene_ui_universel.png", 88 * mm, "Il plugin in modo UNIVERSEL: al posto della testina fissa compare il tamburo rotante.")
box("Fedeltà e ipotesi",
    "Il plugin riproduce il comportamento meccanico delle tre macchine: capstan di diametri diversi, innesto dei "
    "rulli, inerzia del motore, salto di lettura tra le testine del tamburo. Alcuni dettagli non sono documentati "
    "con precisione e sono stati tarati a orecchio: quale tasto corrisponda alla velocità nominale (qui il DO), "
    "le costanti di tempo dell'innesto, l'entità del pleurage e la banda passante della testina.")

# ---------------------------------------------------------------------------- 2
P("2  ·  Perché usare il Phonogene oggi", h1)
P("Un campionatore moderno trasporta un suono con un calcolo. Il Phonogène lo fa con una <i>macchina</i>, "
  "e la macchina lascia tracce: il nastro impiega un istante ad accelerare quando si preme un tasto e rallenta "
  "fino a fermarsi quando lo si rilascia; la velocità oscilla appena; la testina rotante cuce il suono con "
  "piccole giunture udibili. Sono queste imperfezioni a dare carattere al risultato.")
P("Nell'estetica di Schaeffer l'interesse non è riconoscere la fonte di un suono, ma ascoltarlo come "
  "<b>oggetto sonoro</b>, per la sua forma, la sua grana, il suo andamento. Trasporre un frammento di voce, una "
  "porta che sbatte o una nota di pianoforte lungo dodici semitoni, o stirarlo fino a fermarlo, è un modo "
  "concreto di allontanarlo dalla sua origine e di farlo diventare materiale musicale.")
B(["<b>Melodie dai rumori</b>: un colpo, un respiro, un oggetto metallico diventano una frase suonabile sulla tastiera.",
   "<b>Glissandi e cadute</b>: con la coulisse un suono sale, scende, si ferma, si piega come un nastro trattenuto con il dito.",
   "<b>Tempo sospeso</b>: con l'universale un frammento può essere congelato, rallentato o accelerato senza cambiare altezza.",
   "<b>Gesto in tempo reale</b>: tutto è suonabile dalla tastiera MIDI e dalle manopole del controller, in studio o dal vivo."])

# ---------------------------------------------------------------------------- 3
P("3  ·  Installazione e primi passi", h1)
P("Installazione su macOS", h2)
B(["Copia <b>Phonogene.vst3</b> in <font face='Sans'>/Library/Audio/Plug-Ins/VST3</font> e <b>Phonogene.component</b> in "
   "<font face='Sans'>/Library/Audio/Plug-Ins/Components</font>.",
   "Il plugin non è firmato da uno sviluppatore Apple: dal Terminale rimuovi la quarantena con<br/>"
   "<font face='Sans' size='8.5'>sudo xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/Phonogene.vst3 "
   "/Library/Audio/Plug-Ins/Components/Phonogene.component</font>",
   "Riavvia Ableton Live e fai rianalizzare i plugin. L'applicazione <b>Phonogene.app</b> funziona anche da sola (Standalone)."])
P("In Ableton Live", h2)
P("Il Phonogene è uno <b>strumento</b>: va inserito su una traccia MIDI. Per suonare un file basta caricarlo; "
  "per registrare dal vivo serve il <b>sidechain</b>:")
B(["Crea una traccia audio con l'ingresso della scheda audio (o con una clip da usare come sorgente).",
   "Nel dispositivo Phonogene apri il menu <b>Sidechain</b> e scegli quella traccia come sorgente (<i>Audio From</i>).",
   "Porta <b>SOURCE</b> su <b>ENTRÉE</b>, premi <b>ENREG.</b>, suona la sorgente, ripremi ENREG.: l'anello è pronto.",
   "Suona la tastiera del controller (ottava di C3) o quella a schermo con il mouse."])
P("Nella versione Standalone l'ingresso è direttamente quello della scheda audio, scelto nelle impostazioni audio dell'applicazione.", small)
box("In trenta secondi",
    "1) Trascina un file WAV o AIFF sul plugin.  2) Premi i tasti della tastiera: ogni tasto è un capstan, "
    "ogni capstan un semitono.  3) Gira DÉBUT e LONGUEUR per scegliere il frammento.  4) Prova MODE su COULISSE e "
    "muovi la leva, poi su UNIVERSEL e abbassa la leva fino quasi a fermare il nastro.")

# ---------------------------------------------------------------------------- 4
story.append(PageBreak())
P("4  ·  Panoramica dell'interfaccia", h1)
img("panoramica.png", W - 2 * M, "L'interfaccia completa, con i numeri usati nella tabella che segue.")
table([
    ["N.", "Elemento", "Funzione"],
    ["1", "Pozzetto", "La meccanica: anello di nastro, 12 capstan con rulli pressori, testina (o tamburo), perno tenditore."],
    ["2", "Spie CABESTANS", "Indicano il capstan innestato (cromatico) o l'altezza suonata (universale)."],
    ["3", "Tachimetro", "Velocità reale del nastro; in coulisse anche il motore, in universale anche l'altezza."],
    ["4", "TÊTE ROTATIVE", "TÊTES, FENÊTRE, DURETÉ: i comandi del tamburo, attivi in modo universale."],
    ["5", "Targhetta", "Marca e versione della macchina scelta con MODE."],
    ["6", "MODE", "Chromatique, à coulisse, universel."],
    ["7", "SOURCE", "FICHIER (file caricato) o ENTRÉE (nastro registrato dall'ingresso)."],
    ["8", "ENREG.", "Registra l'ingresso sull'anello; la spia rossa è accesa durante la registrazione."],
    ["9", "ÉCOUTE", "Monitor: manda in uscita anche il segnale in ingresso."],
    ["10", "MONO", "Nastro a una traccia, come l'originale: lo stesso segnale su entrambe le uscite."],
    ["11", "Finestra del nastro", "Forma d'onda, regione dell'anello (in ambra) e punto di lettura (linea rossa)."],
    ["12", "CHARGER / EXPORTER", "Carica un file come nastro; esporta la registrazione in WAV o AIFF."],
    ["13", "Tastiera", "Un'ottava, DO–SI; suonabile anche con il mouse."],
    ["14", "OCTAVE / ÉTENDU", "Seconda ottava della tastiera (velocità ×2); tastiera estesa a tutte le ottave del controller."],
    ["15", "COULISSE", "La leva della velocità continua (coulisse) o della durata (universale)."],
    ["16", "Manopole", "DÉBUT, LONGUEUR, INERTIE, NIVEAU, PLEURAGE, SOUFFLE, SATURATION."],
    ["17", "RETOUR / COLLURE", "Ripartenza dall'inizio dell'anello; giunta adesiva dell'anello udibile."],
], [10 * mm, 38 * mm, W - 2 * M - 48 * mm])
P("I selettori MODE, SOURCE e TÊTES cambiano posizione con un clic o con un breve trascinamento. La finestra si "
  "ridimensiona dall'angolo in basso a destra, mantenendo le proporzioni; si apre sempre a 1000 × 740 punti. "
  "Con il tasto destro su qualsiasi comando si apre il menu MIDI (capitolo 11).", small)

# ---------------------------------------------------------------------------- 5
P("5  ·  Il nastro: sorgenti, anello, registrazione", h1)
P("Come sulla macchina originale, si lavora su un <b>anello di nastro</b>: un frammento che si ripete. "
  "Il Phonogene ne ha due, selezionati con <b>SOURCE</b>.")
P("SOURCE · FICHIER", h3)
P("Il nastro è un file audio (WAV, AIFF, FLAC e gli altri formati supportati dal sistema), fino a 120 secondi. "
  "Si carica con <b>CHARGER</b> o trascinandolo sul plugin; il caricamento avviene in background e la finestra del "
  "nastro mostra «CHARGEMENT…». Il file viene ricampionato alla frequenza del progetto e ricaricato "
  "automaticamente alla riapertura (il percorso è salvato nel progetto).")
P("SOURCE · ENTRÉE", h3)
P("Il nastro è registrato dall'ingresso: il sidechain in Live, la scheda audio nello Standalone. "
  "<b>ENREG.</b> avvia e ferma la registrazione (fino a 30 secondi); durante la registrazione la spia è accesa e il "
  "nastro può già essere suonato. <b>ÉCOUTE</b> manda in uscita anche l'ingresso, per sentire cosa si sta "
  "registrando. La registrazione è <b>salvata nel progetto</b> (compressa senza perdite) e sopravvive ai cambi di "
  "frequenza di campionamento; <b>EXPORTER</b> la salva come file WAV o AIFF a 24 bit.")
P("MONO", h3)
P("Il Phonogène leggeva un nastro a una sola traccia con un'unica testina: il suono era monofonico. Con "
  "<b>MONO</b> in posizione I i due canali del nastro vengono sommati e le due uscite ricevono lo stesso segnale, "
  "soffio compreso. In posizione O il plugin conserva lo stereo del materiale caricato.")
P("DÉBUT e LONGUEUR", h3)
P("Scelgono quale parte del nastro forma l'anello: <b>DÉBUT</b> è l'inizio (0–1 della lunghezza del nastro), "
  "<b>LONGUEUR</b> la durata (dallo 0,5 % al 100 %). La regione appare in ambra nella finestra del nastro. Anelli "
  "corti producono ripetizioni ritmiche e, alle velocità alte, ronzii e timbri nuovi; anelli lunghi lasciano "
  "riconoscere il materiale.")
P("I due comandi si possono girare mentre il nastro suona: il passaggio alla nuova regione avviene con una "
  "dissolvenza di 10 ms, senza clic.", small)
P("COLLURE", h3)
P("Sulla macchina l'anello era chiuso con una giunta di nastro adesivo. Con <b>COLLURE</b> in posizione I la "
  "giunta passa sulla testina a ogni giro: un breve calo di livello in corrispondenza del punto di chiusura "
  "dell'anello, che dura circa 6 ms alla velocità nominale e si allunga quando il nastro rallenta (è un tratto "
  "fisso di nastro). È opzionale: il segno ritmico tipico dei loop di musique concrète, da attivare quando serve.")
box("Perché un anello",
    "Il solco chiuso del disco e poi l'anello di nastro sono gli strumenti con cui Schaeffer isolava un frammento "
    "dal suo contesto. Ripetuto, il frammento smette di raccontare da dove viene e diventa oggetto sonoro.")

# ---------------------------------------------------------------------------- 6
P("6  ·  Modo CHROMATIQUE", h1)
img("phonogene_ui.png", 118 * mm, "Modo cromatico: FA# premuto, il rullo FA# è innestato e il nastro scorre a 1,414.")
P("Ogni tasto innesta il rullo pressore di un capstan; il diametro del capstan decide la velocità del nastro "
  "secondo il temperamento equabile (rapporto 2<super>n/12</super>). Il <b>DO</b> corrisponde alla velocità "
  "nominale, il <b>SI</b> a quasi il doppio. Lasciato il tasto, il rullo si stacca e il nastro rallenta fino a "
  "fermarsi: il suono scende e svanisce in modo naturale.")
B(["<b>Monofonico, ultima nota</b>: un solo nastro, una sola testina. Premendo un secondo tasto senza staccare il "
   "primo, il nastro scivola alla nuova velocità (legato).",
   "<b>RETOUR</b> (I) fa ripartire il nastro dall'inizio dell'anello a ogni nuovo attacco, come un campione. "
   "In posizione O il nastro riprende da dove si era fermato, come una bobina vera.",
   "<b>OCTAVE</b> porta la tastiera all'ottava superiore (velocità ×2): secondo il GRM il Phonogène chromatique "
   "trasponeva «sur une à deux octaves». Con OCTAVE il tachimetro passa alla scala 0–4.",
   "<b>ÉTENDU</b> estende la tastiera: in O tutte le ottave del controller ricadono nell'unica ottava della "
   "macchina, in I ogni tasto ha la sua velocità (C3 = nominale, C4 = doppia, C2 = metà). Oltre 4× il filtro "
   "anti-aliasing non scende più.",
   "<b>INERTIE</b> è il tempo con cui il nastro raggiunge la nuova velocità: 2 ms dà attacchi secchi, valori "
   "alti danno partenze e arresti a glissando. Sulla macchina reale l'innesto stava probabilmente sotto i "
   "300 ms; oltre è un uso espressivo."])

# ---------------------------------------------------------------------------- 7
P("7  ·  Modo À COULISSE", h1)
img("phonogene_ui_coulisse.png", 118 * mm, "Modo coulisse: la leva a 0,6, il nastro trascinato dal primo capstan.")
P("Qui la velocità è continua. La leva <b>COULISSE</b> regola il motore da quasi fermo (0) a due volte la "
  "velocità nominale; il <b>pitch bend</b> la sposta ulteriormente (±12 semitoni) e la <b>mod wheel</b> "
  "(CC 1) controlla la leva dal controller.")
B(["<b>I tasti non scelgono l'altezza</b>: qualsiasi tasto innesta il nastro, che si stacca al rilascio. "
   "ÉTENDU qui non ha effetto e appare attenuato.",
   "<b>INERTIE</b> diventa la massa del motore con il suo volano: la velocità insegue la leva con un tempo tre "
   "volte maggiore (fino a 3 s). Leva veloce e inerzia alta danno glissandi lunghi e morbidi.",
   "<b>Verso l'arresto</b> il suono cambia come su un vero magnetofono: il livello cala (circa 6 dB per ottava "
   "sotto la velocità nominale), i bassi spariscono, aumentano scintillio e deriva, compare un leggero rombo "
   "meccanico che cresce con SOUFFLE."])
box("Un gesto da provare",
    "Tieni premuto un tasto, porta la leva a 1, poi falla scendere lentamente fino in fondo: il suono si "
    "abbassa, si sgrana e si spegne nel rombo del motore, come il nastro trattenuto con un dito.")

# ---------------------------------------------------------------------------- 8
P("8  ·  Modo UNIVERSEL", h1)
P("Il tamburo con le testine rotanti separa le due grandezze che il nastro lega insieme:")
B(["la <b>tastiera</b> (con OCTAVE, ÉTENDU e pitch bend) decide l'<b>altezza</b>;",
   "la leva <b>COULISSE</b> decide la <b>velocità del nastro</b>, cioè la durata: a 2 il frammento scorre il doppio "
   "più in fretta senza cambiare altezza; vicino a 0 il nastro quasi si ferma e il suono resta sospeso;",
   "<b>INERTIE</b> fa scivolare sia l'altezza tra una nota e l'altra sia la velocità del nastro."])
P("I comandi del tamburo", h2)
table([
    ["Comando", "Che cosa fa", "Come si sente"],
    ["TÊTES (2 · 3 · 4)", "Numero di testine sul tamburo; il nastro ne avvolge una alla volta.",
     "Cambia la frequenza dei raccordi; il cambio avviene con una breve dissolvenza."],
    ["FENÊTRE (10–250 ms)", "Distanza tra due testine misurata sul nastro: a ogni cambio di testina la lettura salta indietro di questo tratto. Predefinita 40 ms, la durata dei segmenti del regolatore di Springer (4 testine a 90°).",
     "Valori corti: suono granuloso, ronzante. Valori lunghi: più liscio, con ripetizioni udibili."],
    ["DURETÉ (0–1)", "Durata del passaggio tra una testina e la successiva.",
     "0: due testine in dissolvenza, morbido. 1: commutazione secca, con i clic di raccordo tipici della macchina."],
], [36 * mm, 66 * mm, W - 2 * M - 102 * mm])
P("Su un suono puro il salto tra le testine sposta un poco la fase: si sente come un leggero battimento e "
  "l'altezza può scostarsi di qualche hertz. È intrinseco al principio delle testine rotanti; quando FENÊTRE "
  "contiene un numero intero di periodi del suono, l'altezza torna esatta. FENÊTRE e DURETÉ possono essere mosse "
  "mentre si suona: seguono la manopola gradualmente, senza crepitii; muovendo FENÊTRE in fretta si sente un "
  "breve glissando, effetto naturale del tratto di nastro che cambia lunghezza.", small)

# ---------------------------------------------------------------------------- 9
P("9  ·  Il carattere del nastro", h1)
table([
    ["Manopola", "Funzione"],
    ["PLEURAGE", "Pleurage (wow, ~0,5 Hz) e scintillio (flutter, ~7 Hz) applicati alla velocità reale del nastro, più una lenta deriva casuale. Nascono dalla rotazione delle parti meccaniche: in coulisse e in universale la loro frequenza segue la velocità del motore e rallenta con lui. A 0 il trasporto è perfetto."],
    ["SOUFFLE", "Soffio del nastro, presente solo quando il nastro scorre. Il soffio è inciso sul nastro, quindi si traspone con la lettura: più grave quando il nastro rallenta, più acuto quando accelera. In coulisse aumenta anche il rombo vicino all'arresto."],
    ["SATURATION", "Saturazione morbida della testina, che arrotonda i picchi e aggiunge densità."],
    ["NIVEAU", "Livello d'uscita, da −36 a +12 dB."],
], [32 * mm, W - 2 * M - 32 * mm])
P("In tutte le modalità la lettura passa anche per la risposta della testina (perdita di estremi gravi e acuti) "
  "e per un filtro <b>anti-aliasing</b>: alle velocità superiori a 1× i contenuti che supererebbero il limite "
  "di Nyquist vengono filtrati invece di ripiegarsi come fischi metallici.")

# ---------------------------------------------------------------------------- 10
P("10  ·  Indicatori", h1)
B(["<b>Pozzetto</b>: il nastro scorre alla velocità reale; i capstan girano sempre (in coulisse alla velocità del "
   "motore); il rullo innestato è cerchiato d'ambra; in universale il tamburo gira alla velocità relativa testina–nastro.",
   "<b>Spie CABESTANS</b>: il capstan innestato in cromatico, l'altezza in universale; spente in coulisse.",
   "<b>Tachimetro</b>: velocità del nastro da 0 a 2 (da 0 a 4 con OCTAVE); sotto, in coulisse la velocità del motore, in universale l'altezza.",
   "<b>Finestra del nastro</b>: forma d'onda, regione dell'anello e punto di lettura."])

# ---------------------------------------------------------------------------- 11
P("11  ·  MIDI: tastiera, CC e MIDI learn", h1)
P("La tastiera della macchina corrisponde all'<b>ottava di C3</b> del controller (note MIDI 60–71, la "
  "convenzione di Live e Logic): C3 = DO, C#3 = DO#, … B3 = SI. Con ÉTENDU in O anche le altre ottave suonano, "
  "ricadendo nell'unica ottava della macchina. Il <b>pitch bend</b> agisce in coulisse e in universale.")
P("CC predefiniti", h2)
table([
    ["Comando", "CC", "Comando", "CC", "Comando", "CC"],
    ["DÉBUT", "74", "SATURATION", "80", "MODE", "27"],
    ["LONGUEUR", "75", "TÊTES", "81", "SOURCE", "28"],
    ["INERTIE", "76", "FENÊTRE", "82", "ENREG.", "29"],
    ["NIVEAU", "77", "DURETÉ", "83", "ÉCOUTE", "30"],
    ["PLEURAGE", "78", "COULISSE", "1 (mod wheel)", "RETOUR", "31"],
    ["SOUFFLE", "79", "OCTAVE", "15", "ÉTENDU", "14"],
    ["", "", "MONO", "16", "COLLURE", "17"],
], [30 * mm, 20 * mm, 30 * mm, 30 * mm, 30 * mm, W - 2 * M - 140 * mm])
P("MIDI learn", h2)
B(["Tasto destro su un comando: il menu mostra il CC assegnato.",
   "<b>MIDI learn</b>: il comando lampeggia in ambra finché non muovi una manopola del controller. Se quel CC era già "
   "usato da un altro comando, viene tolto da lì.",
   "<b>Oublier le CC</b> rimuove l'assegnazione; <b>Rétablir les CC par défaut</b> torna alla tabella sopra.",
   "Le assegnazioni sono salvate nel progetto e le variazioni arrivano al DAW come automazione.",
   "Commutatori e selettori: valore ≥ 64 = I; i selettori dividono l'escursione del CC tra le posizioni."])

# ---------------------------------------------------------------------------- 12
P("12  ·  Ricette di musique concrète", h1)
recipes = [
    ("Melodia di un oggetto", "Registra un colpo su un oggetto metallico. CHROMATIQUE, RETOUR in I, LONGUEUR breve, "
     "INERTIE 20–40 ms. Suona una melodia: il timbro dell'oggetto cambia a ogni altezza."),
    ("Étude aux chemins de fer", "Un loop ritmico (passi, una macchina, un treno) in CHROMATIQUE con RETOUR in O: "
     "cambiando tasto il ritmo accelera e rallenta con l'altezza, come nei primi studi di Schaeffer."),
    ("La caduta di Orfeo", "Una voce lunga in COULISSE, INERTIE alta, leva da 1 a 0 in alcuni secondi, PLEURAGE e "
     "SOUFFLE a metà: un glissando discendente che si spegne nel rombo."),
    ("Accordo congelato", "Una nota di pianoforte in UNIVERSEL, leva quasi a 0, FENÊTRE 80–120 ms, DURETÉ bassa: il "
     "suono resta sospeso e si può cambiare altezza dalla tastiera senza farlo ripartire."),
    ("Grana meccanica", "UNIVERSEL, TÊTES 2, DURETÉ alta, FENÊTRE 15–30 ms: le giunture tra le testine diventano un "
     "ronzio ritmico, la firma sonora del tamburo rotante."),
]
for t, d in recipes:
    story.append(KeepTogether([Paragraph(t, h3), Paragraph(d, body)]))

# ---------------------------------------------------------------------------- 13
P("13  ·  Limiti e note tecniche", h1)
B(["Monofonico in tutte le modalità, come la macchina originale (priorità all'ultima nota).",
   "Registrazione fino a 30 s; file fino a 120 s, ricampionati alla frequenza del progetto.",
   "Anti-aliasing efficace fino a 4× la velocità nominale.",
   "La versione per Logic Pro (AU con sidechain su strumento) non è ancora stata verificata.",
   "Taratura a orecchio: velocità nominale sul DO, costanti di inerzia, entità del pleurage e risposta della "
   "testina; potranno essere affinate confrontandole con registrazioni d'epoca."])

# ---------------------------------------------------------------------------- appendice
P("A  ·  Tabella dei comandi", h1)
table([
    ["Comando", "Valori", "Predefinito", "Modalità"],
    ["MODE", "Chromatique · À coulisse · Universel", "Chromatique", "—"],
    ["SOURCE", "Fichier · Entrée", "Fichier", "tutte"],
    ["ENREG.", "O · I", "O", "tutte, con SOURCE Entrée"],
    ["ÉCOUTE", "O · I", "O", "tutte"],
    ["RETOUR", "O · I", "I", "tutte"],
    ["ÉTENDU", "O · I", "O", "cromatica, universale"],
    ["OCTAVE", "O · I", "O", "cromatica, universale"],
    ["MONO", "O · I", "O", "tutte"],
    ["COLLURE", "O · I", "O", "tutte"],
    ["DÉBUT", "0,00 – 1,00", "0,00", "tutte"],
    ["LONGUEUR", "0,005 – 1,00", "1,00", "tutte"],
    ["INERTIE", "2 – 2000 ms", "35 ms", "tutte"],
    ["NIVEAU", "−36 – +12 dB", "0 dB", "tutte"],
    ["PLEURAGE / SOUFFLE / SATURATION", "0,00 – 1,00", "0,25 / 0,20 / 0,20", "tutte"],
    ["COULISSE", "0,02 – 2,00", "1,00", "coulisse (velocità), universale (durata)"],
    ["TÊTES", "2 · 3 · 4", "4", "universale"],
    ["FENÊTRE", "10 – 250 ms", "40 ms", "universale"],
    ["DURETÉ", "0,00 – 1,00", "0,50", "universale"],
    ["Pitch bend", "± 12 semitoni", "—", "coulisse, universale"],
], [52 * mm, 50 * mm, 32 * mm, W - 2 * M - 134 * mm])
P("Fonti storiche: J. Poullin (1999), citato nelle voci enciclopediche sul Phonogène; INA GRM, «Machines, Tools, "
  "Studios»; intervista a François Bayle (O. Julien, <i>Canadian University Music Review</i>, 2004).", small)

doc.build(story)
print("ok")
