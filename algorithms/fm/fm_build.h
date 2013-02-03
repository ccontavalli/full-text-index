int fm_build(fm_index *index, uchar *text, uint32_t length);

int fm_read_file(char *filename, uchar **textt, uint32_t *length);

int fm_build_config (fm_index *index, suint tc,  suint freq, 
							uint32_t bsl1, uint32_t bsl2, suint owner);
