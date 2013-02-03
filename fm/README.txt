Release notes for FM-index,   Version 2.0
September, 2005
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Copyright (C) 2005
Paolo Ferragina (ferragina@di.unipi.it)
Rossano Venturini (venturin@cli.di.unipi.it)

Dipartimento di Informatica, University of Pisa, Italy



  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


The present software offers a new implementation of the compressed
full-text index data structure, called FM-index.
The FM-index was proposed by Ferragina and Manzini in:

[FOCS'00]   Opportunistic data structures with applications.
            Paolo Ferragina and Giovanni Manzini.
            In Proc. IEEE Symp. on Foundations of Computer Science (FOCS),
            Redondo Beach, CA, (USA), 2000.

An implementation of the FM-index (vers. 1.0) was presented in:

[SODA '01]  An experimental study of an opportunistic index.
            Paolo Ferragina and Giovanni Manzini.
            In Proc. ACM-SIAM Symp. on Discrete Algorithms (SODA),
            Washington DC, (USA), 2001

The current software adopts innovative algorithmic solutions for the
implementation of some basic steps of the FM-index inner working. This
is the reason why we denoted it as 'vers. 2.0'. Details about these
new features are available at
http://roquefort.di.unipi.it/~ferrax/fmindexV2/index.html.

In order to work properly, the software needs the additional
package 'ds_ssort.a', designed by Giovanni Manzini, and available at
http://www.mfn.unipmn.it/~manzini/lightweight/index.html.
However, you do not need to download this package because it
is included within the FM-index software.


To compile the FM-index software you must issue the command 'make'
in the FM-index home directory. This creates three useful things:

1. fm_build: a command to build the FM-index data structure;

2. fm_search: a command to search the FM-index, or visualize parts
of the indexed text (snippets);

3. fm_index.a: the FM-index library including all compression and
indexing functionalities.

"fm_build" and "fm_search" are command-line programs that allow to
build the FM-index of a file, to search patterns in it, and to
visualize some parts of the indexed file. This way, you can use the
FM-index either as a full-text search engine, or as a compressed
storage engine that allows you to display parts of the
indexed-compressed text without its full decompression.

In order to use the FM-index over a text T, you need first to index T via
fm_build, and then you can query the FM-index by issuing the command fm_search.
This latter command allows you to count or locate the occurrences of the
queried pattern, or you can use it to display text snippets.

Some examples of usage follow (try also "fm_build -h" and "fm_search -h"):

# fm_build myfile -o imyfile
(creates imyfile.fmi which contains the FM-index)

# fm_search -c foo imyfile
(counts # of occurrences of the pattern 'foo' in 'myfile')

# fm_search -l foo imyfile
(finds and reports the positions of the occurrences of 'foo' in 'myfile')

# fm_search -e 100 -n 10 imyfile
(extracts the substring myfile[100, 100+10-1])

# fm_search -s 10 foo imyfile
(prints 10 characters surrounding each occurrences of the pattern 'foo')

# fm_search -d myfiledec imyfile
(creates 'myfiledec' which is a copy of the original file 'myfile')

Of course you can develop your own search engine by exploiting the
functionalities offered by the FM-index library. In this case you need
to include in your software the three files: fm_index.a, interface.h
and ds_ssort.a.  Please have a look at the two sources
fm_search_main.c and build_index_Example.c for working examples on the
usage of the library.

We point out that the FM-index library follows the API suggested
by the Pizza&Chili site for implementations of compressed full-text indexes.
For further details on this initiative please have a look at the two mirrors
http://pizzachili.di.unipi.it and http://pizzachili.dcc.uchile.cl/.

All the cited libraries follow the GNU-GPL license, as ours.

The present software should be considered an ALPHA version.
We will be glad to receive your comments and bug reports.

Paolo Ferragina                 Rossano Venturini
ferragina@di.unipi.it           venturin@cli.di.unipi.it
