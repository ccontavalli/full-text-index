/* Errors type */

/* define errors */
#define FM_OK 		( 0)
#define FM_GENERR 	(-1)	   	// Errore generale
#define FM_OUTMEM 	(-2)	   	// Errore allocazione memoria
#define FM_CONFERR 	(-11)      	// Errore nella configurazione 

/* read/write errors */
#define FM_READERR  (-12) 		// Errore nella lettura file 
#define FM_FILEERR 	(-14) 		// Problem with some file

/* decompress errors */
#define FM_FORMATERR 	(-3)  	// Formato file da decomprimere errato
#define FM_COMPNOTSUP 	(-4) 	// Metodo di compressione o decompressione non supportato
#define FM_DECERR 		(-5)    // Errore in decompressione 
#define FM_COMPNOTCORR 	(-8) 	// File compresso non corretto

/* locate/count/extract errors */
#define FM_SEARCHERR 	(-6)    // Errore nella ricerca causato da posizioni errate
#define FM_NOMARKEDCHAR (-7) 	// Nessun carattere marcato impossibile search/extract
#define FM_MARKMODENOTKNOW (-9) // Tipo di marcamento sconosciuto

#define FM_NOTIMPL (-13) 		// Function non implemented
