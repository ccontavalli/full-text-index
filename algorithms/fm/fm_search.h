
int count_row_mu(fm_index *index, uchar *pattern, uint32_t len, uint32_t sp, uint32_t ep);

int fm_multi_count(fm_index *index, uchar *pattern, uint32_t len, multi_count **list);

int fm_boyermoore(fm_index *s, uchar * pattern, uint32_t length, uint32_t ** occ, uint32_t * numocc);
