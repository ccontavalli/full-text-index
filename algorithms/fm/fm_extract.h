
uint32_t go_back(fm_index *index, uint32_t row, uint32_t len, uchar *dest);

uint32_t go_forw(fm_index *s, uint32_t row, uint32_t len, uchar *dest);

int fm_unbuild(fm_index *index, uchar ** text, uint32_t *length); /* extract whole text */

int fm_snippet(fm_index *s, uint32_t row, uint32_t plen, uint32_t clen, 
               uchar *dest, uint32_t *snippet_length);

int fm_invert_bwt(fm_index *s);
int fm_compute_lf(fm_index *s);
