#include "fm_index.h"
#include "fm_errors.h"

/*
 * Sends to stderr an error message corresponding to error code e 
 */
char* error_index (int e)
{
	switch (e)
	{
	case FM_GENERR:
		return "Appeared general error";
		break;
	case FM_OUTMEM:
		return "Malloc failed. Not capable to allocate memory";
		break;
	case FM_CONFERR:
		return "Some parameters of configuration are not correct";
		break;
	case FM_COMPNOTSUP:
		return "Compression Algorithm not support";
		break;
	case FM_DECERR:
		return "General error on decompression";
		break;
	case FM_COMPNOTCORR:
		return "Compressed file is not correct";
		break;
	case FM_SEARCHERR:
		return "Error during search";
		break;
	case FM_NOMARKEDCHAR:
		return "Impossible to make search/extract without marked chars";
		break;
	case FM_READERR:
		return "Can't open or read the file";
		break;
	case FM_NOTIMPL:
		return "Function not implemented";
		break;
	default:
		return "Appeared general error\n";
		break;
	}
}
