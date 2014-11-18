This is a tool for taking the file fma3.2.owl from
http://sig.biostr.washington.edu/share/downloads/fma/FMA_Release/alt/v3.2.1/owl_file/fma_3.2.1_owl_file.zip
and adjusting it to a more useful format.  The following changes are made:

* All annotations not relevant to the RICORDO project are removed.
* IRI structure is changed to contain FMA ID number.
* All "placeholder" classes are removed.

This shrinks the FMA size from 192MB to 40MB.

******************
How to use it
******************

Compile the c files using "make" (or directly using "gcc -Wall -g cutter.c trie.c -o cutter").

Run the file with the following syntax:
cutter <path/to/fma3.2.owl> <path/to/outputfile>
