/* Compare 2 result files */
   
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef unsigned char uchar;
#ifndef __USE_MISC
typedef unsigned long ulong;
#endif

#define COUNT 		('C')
#define LOCATE 		('L')
#define EXTRACT 	('E')
#define DISPLAY 	('D')

void cmp_locate(int typeq);
void cmp_extract(void);
void cmp_display(void);
void print_strings(uchar *t, ulong len); 

char *Fname1, *Fname2;
ulong Strlen;
FILE *Pfile1, *Pfile2;

 int compare_ulongs( const void* a, const void* b ) {
   ulong* arg1 = (ulong*) a;
   ulong* arg2 = (ulong*) b;
   if( *arg1 < *arg2 ) return -1;
   else if( *arg1 == *arg2 ) return 0;
   else return 1;
 }	
 
 int compare_strings( const void* a, const void* b ) {
   uchar** arg1 = (uchar**) a;
   uchar** arg2 = (uchar**) b;
   ulong i;

   for(i=0;i<Strlen;i++)
   		if ((*arg1)[i] != (*arg2)[i]) 
			if ((*arg1)[i] > (*arg2)[i]) return 1;
				else return -1;
   return 0;
}

int main (int argc, char *argv[]) {

	uchar type1, type2;

	if(argc != 3) {
		fprintf(stderr, "\nUsage: %s resultfile1 resultfile2\n", argv[0]);
		fprintf(stderr, "      to compare the two results files produced by run_queries\n\n");
		exit(1);
	}
	
	Fname1 = argv[1];
	Fname2 = argv[2];

	Pfile1 = fopen(Fname1,"r");
	if(Pfile1==NULL){
		fprintf(stderr,"Error: cannot open the file %s for reading\n",Fname1);
		exit(1);
	}
	Pfile2 = fopen(Fname2,"r");
	if(Pfile2==NULL){
		fprintf(stderr,"Error: cannot open the file %s for reading\n",Fname2);
		exit(1);
	}
	
	/* Read number occs */
	if(fread(&type1,sizeof(uchar),1,Pfile1) != 1) {
		fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
		exit(1);
	}

	if(fread(&type2,sizeof(uchar),1,Pfile2) != 1) {
		fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
		exit(1);
	}	

	if(type1!=type2) {
		fprintf(stderr, "Result files %s and %s have a different query type\n", Fname1, Fname2);
		exit(0);
	}
	switch (type1) {
		case LOCATE: 
			cmp_locate(1);
			break;
		case COUNT:
			cmp_locate(0);
			break;
		case EXTRACT:
			cmp_extract();			
			break;
		case DISPLAY:
			cmp_display();
			break;
		default: 
			fprintf(stderr,"Unknow query type\n");
			exit(1);
		}
		


	fclose(Pfile1);
	fclose(Pfile2);
	exit(0);
}

void cmp_extract()	{

	ulong pos1, pos2;
	ulong nchar1, nchar2;
	uchar *text1, *text2;
	int error1, error2;

	while(1) {
		/* Read pattern len */
		error1 = fread(&pos1,sizeof(ulong),1,Pfile1);
		error2 = fread(&pos2,sizeof(ulong),1,Pfile2);
		if(error1-error2 !=0) {
			fprintf(stderr, "The number of positions in files %s and %s differ\n", Fname1, Fname2);
			break;
		}
		if(error1==0) break; // No more positionsin
		
		if(pos1!=pos2) {
			fprintf(stdout,"The positions in files %s and %s differ\n", Fname1, Fname2);
			break;
		}
		
		if (fread(&nchar1,sizeof(ulong), 1, Pfile1) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
			exit(1);
		}
		if (fread(&nchar2,sizeof(ulong), 1, Pfile2) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
			exit(1);
		}
		
		/* read text */
		text1 = malloc(sizeof(*text1) * nchar1);
		text2 = malloc(sizeof(*text2) * nchar2);
		if((text1==NULL)||(text2==NULL)) {
			fprintf(stderr,"Error: cannot allocate\n", Fname1, Fname1);
			exit(1);
		}	
		
		if (fread(text1,sizeof(uchar), nchar1,Pfile1) != nchar1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
			exit(1);
		}
		if (fread(text2,sizeof(uchar), nchar2, Pfile2) != nchar2) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
			exit(1);
		}
	
		if(nchar1!=nchar2) {
			fprintf (stdout, "The number of characters extracted differ: %lu chars %s and %lu chars %s\n", nchar1, Fname1, nchar2, Fname2);
		}
	    
		if((nchar1!=nchar2)||(strncmp(text1, text2, nchar1 < nchar2 ? nchar1: nchar2))) {
			fprintf(stdout, "Text extracted from position %lu differ\n",pos1);
			fprintf(stdout, "Text from %s:\n",Fname1);
			print_strings(text1, nchar1);
			fprintf(stdout, "\nText from %s:\n",Fname2);
			print_strings(text2, nchar2);
			fprintf(stdout, "\n\n");
		}
		free(text1); free(text2);
	}
}		

void cmp_locate(int typeq){

	ulong plen1, plen2;
	ulong nocc1, nocc2;
	ulong *pos1, *pos2;
	uchar *pattern1, *pattern2;
	int error1, error2;

	while(1) {
		/* Read pattern len */
		error1 = fread(&plen1,sizeof(ulong),1,Pfile1);
		error2 = fread(&plen2,sizeof(ulong),1,Pfile2);
		if(error1-error2 !=0) {
			fprintf(stdout, "The number of patterns in the files %s and %s differ\n", Fname1, Fname2);
			break;
		}
		if(error1==0) break; // No more patterns 
	
		if(plen1!=plen2) {
			fprintf(stdout,"The length of a pattern in the files %s and %s differ\n", Fname1, Fname2);
			break;
		}
		
		/* read pattern */
		pattern1 = malloc(sizeof(*pattern1)*plen1);
		pattern2 = malloc(sizeof(*pattern2)*plen2);
	
		if ((pattern2==NULL)||(pattern1==NULL)) {
			fprintf(stderr,"Error: cannot allocate\n", Fname1, Fname1);
			exit(1);
		}
	
		if (fread(pattern1,sizeof(uchar),plen1,Pfile1) != plen1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
			exit(1);
		}
		if (fread(pattern2,sizeof(uchar),plen1, Pfile2) != plen1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
			exit(1);
		}
		
		if(strncmp(pattern1,pattern2,plen1)) {
			fprintf(stdout,"A pattern in the files %s and %s differ\n", Fname1, Fname2);
			break;
		}	

		/* Read number occs */
		if(fread(&nocc1,sizeof(ulong),1,Pfile1) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
			break;
		}

		if(fread(&nocc2,sizeof(ulong),1,Pfile2) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
			break;
		}
		
		if(nocc1!=nocc2) {
			fprintf(stdout,"The number of occurences of the pattern ");
			print_strings(pattern1, plen1);
			fprintf(stdout," differ: %s %lu occs, %s %lu occs\n",  Fname1, nocc1, Fname2, nocc2);
			
		}
		if((nocc1==0)&&(nocc2==0)) {
			free(pattern1);
			free(pattern2);
			continue;
		}

		/* Read positions and compare */
		if(typeq) {
		
			pos1 = malloc(sizeof(*pos1) * nocc1);
			pos2 = malloc(sizeof(*pos1) * nocc2);
			if((pos2==NULL)||(pos1==NULL)) {
				fprintf(stderr,"Error: cannot allocate\n");
				exit(1);
			}	
		
			fread(pos1, sizeof(ulong), nocc1, Pfile1);
			fread(pos2, sizeof(ulong), nocc2, Pfile2);
		
			/* sort positions */
			qsort(pos1, nocc1, sizeof(ulong), compare_ulongs );
			qsort(pos2, nocc2, sizeof(ulong), compare_ulongs );

			ulong i,j;
			j = i = 0;
			while((i<nocc1) && (j<nocc2)) {
				if(pos1[i] < pos2[j]) { 
						fprintf(stdout,"Pattern ");
						print_strings(pattern1, plen1);
						fprintf(stdout,": position %lu not in file %s\n", pos1[i++],Fname2);
						continue;
				}
				if(pos1[i] > pos2[j]) { 
						fprintf(stdout,"Pattern ");
						print_strings(pattern1, plen1);
						fprintf(stdout,": position %lu not in file %s\n", pos2[j++],Fname1);
						continue;
				}	
				j++;i++;
			}

			for(;i<nocc1;i++) {
				fprintf(stdout,"Pattern ");
				print_strings(pattern1, plen1);
				fprintf(stdout,": position %lu not in file %s\n", pos1[i], Fname2);
			}
			
			for(;j<nocc2;j++) { 
				fprintf(stdout,"Pattern ");
				print_strings(pattern1, plen1);
				fprintf(stdout,": position %lu not in file %s\n", pos2[j],Fname1);	
			}

			free(pos1);	
			free(pos2);		
		}
		free(pattern1);
		free(pattern2);
	}
}

void cmp_display(){

	ulong plen1, plen2;
	ulong nocc1, nocc2;
	uchar *pattern1, *pattern2;
	ulong snlen1, snlen2;
	ulong i,j;
	uchar **tmp;
	int error1, error2;

	while(1) {
		/* Read pattern len */
		error1 = fread(&plen1,sizeof(ulong),1,Pfile1);
		error2 = fread(&plen2,sizeof(ulong),1,Pfile2);
		if(error1-error2 !=0) {
			fprintf(stdout, "The number of patterns in the files %s and %s differ\n", Fname1, Fname2);
			break;
		}
		if(error1==0) break; // No more patterns 
	
		if(plen1!=plen2) {
			fprintf(stdout,"The length of a pattern in the files %s and %s differ\n", Fname1, Fname2);
			break;
		}
		
		/* read pattern */
		pattern1 = malloc(sizeof(*pattern1)*plen1);
		pattern2 = malloc(sizeof(*pattern2)*plen2);
	
		if ((pattern2==NULL)||(pattern1==NULL)) {
			fprintf(stderr,"Error: cannot allocate\n", Fname1, Fname1);
			exit(1);
		}
	
		if (fread(pattern1,sizeof(uchar),plen1,Pfile1) != plen1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
			exit(1);
		}
		if (fread(pattern2,sizeof(uchar),plen1, Pfile2) != plen1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
			exit(1);
		}
	
		if(strncmp(pattern1,pattern2, plen1)) {
			fprintf(stdout,"A pattern in the files %s and %s differ\n", Fname1, Fname2);
			break;
		}	

		/* Read number occs */
		if(fread(&nocc1,sizeof(ulong),1,Pfile1) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
			break;
		}

		if(fread(&nocc2,sizeof(ulong),1,Pfile2) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
			break;
		}
		
		if(nocc1!=nocc2) {
			fprintf(stdout,"The number of occurences of the pattern ");
		    print_strings(pattern1, plen1);
			fprintf(stdout," differ %s %lu occs, %s %lu occs\n",  Fname1, nocc1, Fname2, nocc2);
		}
		if((nocc1==0)&&(nocc2==0)) {
			free(pattern1);
			free(pattern2);
			continue;
		}

		/* Read snippets len */
		if(fread(&snlen1,sizeof(ulong),1,Pfile1) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
			break;
		}

		if(fread(&snlen2,sizeof(ulong),1,Pfile2) != 1) {
			fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
			break;
		}
		
		if(snlen1!=snlen2) {
			fprintf(stdout,"Length of snippets of the pattern ");
			print_strings(pattern1, plen1);
			fprintf(stdout," differ %s %lu occs, %s %lu occs\n",  Fname1, nocc1, Fname2, nocc2);
			break;
		}
		if((snlen1==0)||(snlen2==0)) {
			free(pattern1);
			free(pattern2);
			continue;
		}
		Strlen = snlen1;	
	
		/* Read snippets and compare */
		uchar *snippets1[nocc1];
		uchar *snippets2[nocc2];
		
		for(i=0; i<nocc1; i++) {
			snippets1[i] = malloc(sizeof(uchar) * snlen1);
			if(snippets1[i] == NULL) {
				fprintf(stderr,"Error: cannot allocate\n");
				exit(1);
			}

			if (fread(snippets1[i],sizeof(uchar), snlen1, Pfile1) != snlen1) {
				fprintf (stderr, "Error: cannot read the file %s\n", Fname1);
				exit(1);
			}
		}
		
	
		for(i=0; i<nocc2; i++) {
			snippets2[i] = malloc(sizeof(uchar) * snlen2);
			if(snippets2[i] == NULL) {
				fprintf(stderr,"Error: cannot allocate\n");
				exit(1);
			}
			
			if (fread(snippets2[i],sizeof(uchar), snlen2, Pfile2) != snlen2) {
				fprintf (stderr, "Error: cannot read the file %s\n", Fname2);
				exit(1);
			}
		}	

		/* Compare naive! Sorts the 2 arrays and searchs each strings 
	       with binary search */
		qsort(snippets1, nocc1, sizeof(uchar *), compare_strings );
		qsort(snippets2, nocc2, sizeof(uchar *), compare_strings );
	
		uchar **res;	
		ulong occs;
	    occs = nocc1 > nocc2 ? nocc1 : nocc2;
		tmp = malloc(nocc1*sizeof(uchar *));
		if(tmp==NULL) {
			fprintf(stderr,"Error: cannot allocate\n", Fname1, Fname1);
			exit(1);
		}	
	
		tmp = memcpy(tmp, snippets2, nocc2*sizeof(uchar *));
		occs = nocc2;

		for(i=0;i<nocc1;i++) {
			res = (uchar **) bsearch(&(snippets1[i]), snippets2, occs, sizeof(uchar *), compare_strings);
			if (res == NULL) {
					fprintf(stdout,"Pattern ");
					print_strings(pattern1, plen1);
					fprintf(stdout, " snippet :\n");
					print_strings(snippets1[i], snlen1);
					fprintf(stdout,"\ndoesn't appear in the file %s\n\n", Fname2);
			} else { /* the founded snippet is removed */
				memmove(res, res+1, (occs-(res-snippets2+1))*sizeof(uchar *));
				occs--;
			}

		}
		
		memcpy(snippets2, tmp, nocc2*sizeof(uchar *));
        tmp = memcpy(tmp, snippets1, nocc1*sizeof(uchar *));		
		occs = nocc1;
	
		for(i=0;i<nocc2;i++) {
			res = (uchar **) bsearch(&(snippets2[i]), snippets1, occs, sizeof(uchar *) , compare_strings);
			if ( res == NULL ) {
					fprintf(stdout,"Pattern ");
					print_strings(pattern2, plen2);
					fprintf(stdout, " snippet :\n");
					print_strings(snippets2[i], snlen2);
					fprintf(stdout,"\ndoesn't appear in the file %s\n\n", Fname1);
			} else { /* the founded snippet is removed */
				memmove(res, res+1, (occs-(res-snippets1+1))*sizeof(uchar *));
				occs--;
			}
			
		}

		for(i=0;i<nocc2;i++)  
			free(snippets2[i]);
		
		for(i=0;i<nocc1;i++)
			free(tmp[i]);
		free(tmp);
	
	}
}

void print_strings(uchar *t, ulong len) {
	ulong i;
	for(i=0; i<len; i++) {
		switch (t[i]) {
			case '\n': fprintf(stdout, "[\\n]"); break;
			case '\b': fprintf(stdout, "[\\b]"); break;				
			case '\e': fprintf(stdout, "[\\e]"); break;
			case '\f': fprintf(stdout, "[\\f]"); break;
			case '\r': fprintf(stdout, "[\\r]"); break;
			case '\t': fprintf(stdout, "[\\t]"); break;
			case '\v': fprintf(stdout, "[\\v]"); break;
			case '\a': fprintf(stdout, "[\\a]"); break;
			case '\0': fprintf(stdout, "[\\0]"); break;
			default: 
			fprintf(stdout, "%c", t[i]);
			break;
		}
	}		
}
