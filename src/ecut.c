//
// File:	ecut.c
// Author:	Ivan Kluzak
// Date:	10/29/2013
// Notes:	A utility similar to cut but with regex cutting
//
#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// --------------------------------------------------------------------------------------
// CONFIGURATION:

#define MAX_BUF		4096
#define ECUT_VERSION	"1.2"
#define MAX_OVECTOR	1024

// --------------------------------------------------------------------------------------

// For some reason this isn't showing as defined on my system even though it's in string.h
#ifndef strnlen 
size_t strnlen(const char *,size_t);
#endif

//
// Display usage and exit
void Usage() {
	printf("\nUsage:\n\tecut [OPTIONS] PATTERN [FILE]\n\n\tWhere PATTERN is a perl compatible regular expression\n\nExample:\n\t ps -aef | ecut \"^(\\S+)\\s+(\\d+)\"\n\n");
	printf("OPTIONS:\n");
	printf("\t-d <delimeter>  = to separate multiple match results on a single line, default is ','\n");
	printf("\t-h, --help      = this help\n");
	printf("\t-n              = no carriage return between lines\n");
	printf("\t-v              = invert the match, print lines that don't match the regex\n");
        printf("\t-V              = display version information\n");
	printf("\n\n");
        exit(0);
}

//
// Display version information and exit
void Version() {
	printf("ecut version %s\nThis is free software: you are free to change and redistribute it.\n", ECUT_VERSION);
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
	printf("\n");
	printf("Written by Ivan Kluzak, sage@sooper.com\n\n");
	exit(0);
}

//
// Main program 
int main(int argc, char **argv) {

	FILE		*in = stdin;				// Default to stdin
	const char	*fname = NULL;				// filename ptr ( if any )
	char		buf[MAX_BUF];				// input line buffer, max line length of 4096
	char		*res = NULL;				// result of our line reads
	int		count = 0;				// Current line count
	pcre		*compiled = NULL;			// The compiled pcre pointer
	const char	*pcreErr = NULL;			// pcre error pointer
	int		pcreEOffset = 0;
	char		*exp = NULL;				// our passed in regex
	int		pres = 0;				// match result count
	int		ovector[MAX_OVECTOR];			// pcre matches
	int		match_error = 1;			// Exit if we found an error in the pcre_exec()
	int		get_result = 0;				// result of the pcre_get_substring() function
	int		f_invert = 0;				// Invert match?
	int		f_expset = 0;				// Was regex set already?
	int		f_nocrlf = 0;				// No carriage return between lines?
	char		*crlf = "\n";				// For display with the f_nocrlf flag
	const char	*psubStr = NULL;			// Matched field pointer
	const char	*delim = ",";				// delimeter if any...

	if (argc == 1) {
		Usage();
	}

	// Check out the command line arguments
	for (int i=0; i < argc; i++) {
		if (!strncmp(argv[i], "-h",2)) {
			Usage();
		} else if (!strncmp(argv[i], "--help",6)) {
			Usage();
		} else if (!strncmp(argv[i], "-V",2)) {
			Version();
		} else if (!strncmp(argv[i], "-v",2)) {
			f_invert = 1;				// Invert the match, print lines that don't match
		} else if (!strncmp(argv[i], "-n",2)) {
			f_nocrlf = 1;				// No carriage return between lines
		} else if (!strncmp(argv[i], "-d",2)) {
			if ((i+1) < argc) {
				delim = argv[i+1];
				i++;
			} else {
				printf("error: -d requires a delimeter be specified\n");
				exit(-1);
			}
		} else {
			if (i > 0) {
				if (f_expset) {
					// Assume next is filename
					if (fname) {
						// If fname .. then display Usage()
						Usage();
					} else {
						fname = argv[i];
					}
				} else {
					exp = argv[i];
					f_expset = 1;
				}
			} else {
				exp = ".*";			// Match everything if no parameters
			}
		}
	}

	// If we got a fname, open it.. otherwise, carry on
	if (fname) {
		in = fopen(fname, "rb");
		if (!in) {
			printf("error: opening '$fname' for input\n");
			exit(-1);
		}
	}

	//
	// Setup the regular expression
	compiled = pcre_compile(exp, 0, &pcreErr, &pcreEOffset, NULL);	
	if (!compiled) {
		printf("error: compiling pcre '%s': %s\n", exp, pcreErr);
		if (fname) { fclose(in); }
		exit(-1);
	}

	//
	// Search the input string line by line for matches
	while (!feof(in)) {

		match_error = 1;				// Set to 1 before we go through, to bail on error
		memset(buf, 0, MAX_BUF);
		res = fgets(buf, (MAX_BUF-1), stdin);
		if (strnlen(buf, MAX_BUF) >= (MAX_BUF-1)) {
			printf("error: maximum line length reached on line[%i]\n", count);
			if (fname) { fclose(in); } 
			exit(-1);
		}

		if (res) {
			//printf("line[%i]: %s", count, res);	
			pres = pcre_exec(compiled, NULL, res, strnlen(res,MAX_BUF), 0, 0, ovector, MAX_OVECTOR);
			if (pres < 0) {
				switch(pres) {
					case PCRE_ERROR_NOMATCH:
						// This does not constitute an error for us
						if (f_invert) {	// Was the INVERT match flag specified on the command line?
							printf("%s", res);
						}
						match_error = 0;
						break;
					case PCRE_ERROR_NULL: 
						printf("error: Something was null\n"); 
						break;
					case PCRE_ERROR_NOMEMORY: 
						printf("error: ran out of memory\n"); 
						break; 
					default: 
						printf("error: Unknown error\n"); 
						break;
				}
				if (match_error) {
					printf("error: pcre_exec() failed on line[%i]\n", count);
					if (fname) { fclose(in); }
					exit(-1);
				}
			} else {
			    //
			    // Else we have a match:
			    //
			    //printf("line[%i]: %i, matched %s", count, pres, res);
			    if (!f_invert) {
				//
				// Did we match ONLY the line?  This is OK, print it.  Sort of like egrep.. sort of.
				if (pres == 1) {
					printf("%s", res);
				} else {
				    //
				    // Otherwise, we have specific matches to display
				    for(int j=1; j < pres; j++) {
        				get_result = pcre_get_substring(buf, ovector, pres, j, &(psubStr));
					if (get_result == PCRE_ERROR_NOMEMORY) {
						printf("error: out of memory in pcre_get_substring()\n");
						if (fname) { fclose(in); }
						exit(-1);
					} else if (get_result == PCRE_ERROR_NOSUBSTRING) {
						printf("error: invalid string number passed to pcre_get_substring()\n");
						if (fname) { fclose(in); }
						exit(-1);
					}
					//
					// If not the lst match, display the match and the delimeter
					if (j < (pres-1)) {
						printf("%s%s", psubStr, delim);
					} else {
						// Else just display the match and a line break
						crlf = (f_nocrlf) ? "" : "\n" ; 
						printf("%s%s", psubStr, crlf);
					}
					pcre_free_substring(psubStr);
				    } // end for()
      				} // end else()
			    } // end if(f_invert)
			} // end else()
		}
		count++;					// Line counter
	}

	// If we got a filename, close the input handle
	if (fname) { 
		fclose(in); 
	}
	pcre_free(compiled);					// Free the complied regex
	return 0;
}
