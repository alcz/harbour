/* Last Translator: DF7BE */

#include "hbapilng.h"

static HB_LANG s_lang =
{
   {
      /* Identification */

      "de_AT",
      "German-Austria",
      "Deutsch-Österreich",
      "",
      "UTF8",
      "",

      /* Month names */

      "Jänner",
      "Feber",
      "März",
      "April",
      "Mai",
      "Juni",
      "Juli",
      "August",
      "September",
      "Oktober",
      "November",
      "Dezember",

      /* Day names */

      "Sonntag",
      "Montag",
      "Dienstag",
      "Mittwoch",
      "Donnerstag",
      "Freitag",
      "Samstag",

      /* CA-Cl*pper compatible natmsg items */

      "Datenbank Dateien Anz. Sätze   Letzte Änderung Größe",
      "Möchten Sie mehr Beispiele?",
      "Anz. Seiten",
      "** Zwischensumme **",
      "* Teilsumme *",
      "*** Summe ***",
      "Einfg.",
      "      ",
      "Ungültiges Datum",
      "Bereich: ",
      " - ",
      "J/N",
      "UNGÜLTIGER AUSDRUCK",

      /* Error description names
         Fehlermeldungen  */

      "Unbekannter Fehler",            /* Unknown error */
      "Argument ungültig",             /* Argument error */
      "Grenzwert überschritten",       /* Bound error */
      "Zeichenketten-Überlauf",        /* String overflow */
      "Numerischer Überlauf",          /* Numeric overflow */
      "Division durch 0",              /* Zero divisor */
      "Numerik-Fehler",                /* Numeric error */
      "Syntax-Fehler",                 /* Syntax error */
      "Operation zu komplex",          /* Operation too complex */
      "",
      "",
      "Zu wenig Speicher",             /* Memory low */
      "Unbekannte Funktion",           /* Undefined function */
      "Keine exportierte Methode",     /* No exported method */
      "Variable existiert nicht",      /* Variable does not exist */
      "Alias  existiert nicht",        /* Alias does not exist */
      "Keine exportierte Variable",    /* No exported variable */
      "Unzulässiges Zeichen in Alias", /* Illegal characters in alias */
      "Alias in Benutzung",            /* Alias already in use */
      "",
      "Fehler beim Anlagen",             /* Create error */
      "Fehler beim Öffnen",            /* Open error */
      "Fehler beim Schließen",         /* Close error */
      "Lesefehler",                    /* Read error */
      "Schreibfehler",                 /* Write error */
      "Fehler beim Drucken",           /* Print error */
      "",
      "",
      "",
      "",
      "Operation nicht unterstützt",         /* Operation not supported */
      "Grenzwert überschritten",             /* Limit exceeded */
      "Korruption festgestellt",             /* Corruption detected */
      "Falscher Datentyp",                   /* Data type error */
      "Unpassende Datenlänge",               /* Data width error */
      "Arbeitsbereich nicht aktiv",          /* Workarea not in use */
      "Arbeitsbereich nicht indiziert",      /* Workarea not indexed */
      "Exklusive Sperre erforderlich",       /* Exclusive required */
      "Sperre erforderlich",                 /* Lock required */
      "Schreiben nicht erlaubt",             /* Write not allowed */
      "Sperre beim Anhängen nicht gesetzt",  /* Append lock failed */
      "Setzen einer Sperre fehlgeschlagen",  /* Lock Failure */
      "",
      "",
      "",
      "Fehler im Objekt-Destruktor",         /* Object destructor failure */
      "Array Zugriff",                       /* array access */
      "Array Zuweisung",                     /* array assign */
      "Array Dimensionierung",               /* array dimension */
      "kein Array",                          /* not an array */
      "bedingt",                             /* conditional */

      /* Internal error names
         Meldungstexte interner Fehler  */

      "Nicht zu behebender Fehler %d: ",                   /* Unrecoverable error %d:  */
      "Fehlerbehebung nicht möglich",                      /* Error recovery failure */
      "ERRORBLOCK() für Fehlerbehandlung nicht vorhanden", /* No ERRORBLOCK() for error */
      "Zu viele rekursive Aufrufe des Fehler-Handlers",    /* Too many recursive error handler calls */
      "RDD ungültig oder kann nicht geladen werden",       /* RDD invalid or failed to load */
      "Ungültiger Typ der Methode von %s",                 /* Invalid method type from %s */
      "hb_xgrab konnte keinen Speicher reservieren",       /* hb_xgrab can't allocate memory */
      "hb_xrealloc wurde mit NULL-Zeiger aufgerufen",      /* hb_xrealloc called with a NULL pointer */
      "hb_xrealloc wurde mit ungültiger Zeiger-Adresse aufgerufen", /* hb_xrealloc called with an invalid pointer */
      "hb_xrealloc erhält keine erneute Speicherzuweisung",         /* hb_xrealloc can't reallocate memory */
      "hb_xfree wurde mit ungültiger Zeiger-Adresse aufgerufen",    /* hb_xfree called with an invalid pointer */
      "hb_xfree wurde mit NULL-Zeiger aufgerufen",         /* hb_xfree called with a NULL pointer */
      "Kann Einstiegsprozedur nicht finden: '%s'",         /* Can't locate the starting procedure: '%s' */
      "Einstiegsprozedur nicht vorhanden",                 /* No starting procedure */
      "Nicht unterstützter VM OP-Code",                    /* Unsupported VM opcode */
      "Symbol-Eintrag erwartet von %s",                    /* Symbol item expected from %s */
      "Ungültiger Symbol-Typ für self von %s",             /* Invalid symbol type for self from %s */
      "Codeblock erwartet von %s",                         /* Codeblock expected from %s */
      "Ungültiger Eintrags-Typ beim Holen von Stapel %s",  /* Incorrect item type on the stack trying to pop from %s */
      "Stapelspeicher Unterlauf",                          /* Stack underflow */
      "Ein Element kann nicht auf sich selbst kopiert werden: %s", /* An item was going to be copied to itself from %s */
      "Ungültiger Symbol-Eintrag als MEMVAR %s übergeben", /* Invalid symbol item passed as memvar %s */
      "Pufferspeicher Überlauf",                            /* Memory buffer overflow */
      "hb_xgrab erfordert Zuweisung von 0 Bytes",           /* hb_xgrab requested to allocate zero bytes */
      "hb_xrealloc erfordert eine neue Größe von 0 Bytes",  /* hb_xrealloc requested to resize to zero bytes */
      "hb_xalloc erfordert eine neue Größe von 0 Bytes",    /* hb_xalloc requested to allocate zero bytes */

      /* Texts */

      "DD.MM.YYYY",
      "J",
      "N"
   }
};

#define HB_LANG_ID      DE_AT
#include "hbmsgreg.h"
