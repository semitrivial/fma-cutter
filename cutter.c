#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macro.h"
#include "trie.h"

#define READ_BLOCK_SIZE 1000000
#define MAX_LINE_SIZE 131071
#define TEMP_BUFFER_SIZE 2000000

int line_begins( char *line, char *init );
void line_to_buf( char *line, char **bptr, char *end );
void add_iri_translation( char *ch_old, char *ch_new );
trie *get_translation( char *ch_old );
void mark_as_node_id( char *key );
int is_end_of_xmlns( char *x );
void fprintf_preamble( FILE *out, FILE *props );

trie *blank_trie( void );
trie *trie_strdup( char *buf, trie *base );
trie *trie_search( char *buf, trie *base );
char *trie_to_static( trie *t );

extern trie *iritrie;

int main(int argc, char *argv[])
{
  FILE *in, *out, *props;
  char c;
  int linenum = 0, fUnwantedClass = 0, fmaid = -1, fLabel = 0, fDescription = 0, i, fNonSpace=0, isNodeId=0;
  int classes_kept=0, classes_discarded=0, total_classes=0;
  char ch_old[MAX_LINE_SIZE], ch_new[MAX_LINE_SIZE];

  /*
   * Variables for QUICK_GETC
   */
  char read_buf[READ_BLOCK_SIZE], *read_end = &read_buf[READ_BLOCK_SIZE], *read_ptr = read_end;
  int fread_len;

  char line[MAX_LINE_SIZE+1], *lptr = line, *end = &line[MAX_LINE_SIZE];
  char buf[TEMP_BUFFER_SIZE+1], *bptr = buf, *bend = &buf[TEMP_BUFFER_SIZE];

  char *objprops[] =
  {
    "part_of",
    "attaches_to",
    "bounds",
    "branch_of",
    "constitutional_part_of",
    "regional_part_of",
    "systemic_part_of",
    "constitutional_part",
    "regional_part",
    "systemic_part",
    NULL
  };

  if ( argc < 3 )
  {
    printf( "Syntax: cutter <uncut fma 3.2 owlfile> <output filename>\n" );
    return 0;
  }

  iritrie = blank_trie();

  in = fopen( argv[1], "r" );
  out = fopen( argv[2], "w" );
  props = fopen( "props.txt", "r" );

  fprintf_preamble( out, props );

  if ( !in )
  {
    fprintf( stderr, "Couldn't read the file %s\n", argv[1] );
    return 0;
  }

  if ( !out )
  {
    fprintf( stderr, "Couldn't write to the file %s\n", argv[2] );
    return 0;
  }

  if ( !props )
  {
    fprintf( stderr, "Couldn't open utility file props.txt for reading\n" );
    return 0;
  }

  printf( "Opened %s for reading.\nOpened %s for writing.\n", argv[1], argv[2] );

  printf( "First pass: counting total number of classes...\n" );

  for(;;)
  {
    QUICK_GETC( c, in );

    if ( !c )
      break;

    if ( c != '\n' )
    {
      *lptr++ = c;

      if ( lptr >= end )
      {
        fprintf( stderr, "There was a line too long (max line length is %d)\n", MAX_LINE_SIZE );
        return 0;
      }

      continue;
    }

    *lptr = '\0';
    lptr = line;

    if ( line_begins( line, "<rdf:Description" ) )
      total_classes++;
  }

  printf( "First pass done.\n" );

  rewind(in);
  read_ptr = read_end;

  printf( "Second pass: translating IRIs...\n" );

  for(;;)
  {
    QUICK_GETC( c, in );

    if ( !c )
      break;

    if ( c != '\n' )
    {
      if ( c == ' ' )
      {
        if ( !fNonSpace )
          continue;
      }
      else
        fNonSpace = 1;

      *lptr++ = c;
      continue;
    }

    *lptr = '\0';
    lptr = line;
    fNonSpace = 0;
    linenum++;

    if ( line_begins( line, "<rdf:Description rdf:about=\"" )
    ||   line_begins( line, "<rdf:Description rdf:nodeID=\"" ) )
    {
      char *ptr;
      char *token = line_begins( line, "<rdf:Description rdf:about=\"" ) ? "<rdf:Description rdf:about=\"" : "<rdf:Description rdf:nodeID=\"";

      isNodeId = line_begins( line, "<rdf:Description rdf:about=\"" ) ? 0 : 1;

      for ( ptr = &line[strlen(token)]; *ptr; ptr++ )
        if ( *ptr == '"' )
          break;

      if ( !*ptr )
        goto weird_description;

      *ptr = '\0';
      sprintf( ch_old, "%s", &line[strlen(token)] );

      fDescription = 1;
      continue;
    }

    if ( line_begins( line, "<default:FMAID rdf:datatype=\"http://www.w3.org/2001/XMLSchema#string\">" ) )
    {
      char *ptr;

      if ( fLabel )
      {
        fprintf( stderr, "Line %d: Multiple FMA IDs?\n", linenum );
        return 0;
      }

      for ( ptr = &line[strlen("<default:FMAID rdf:datatype=\"http://www.w3.org/2001/XMLSchema#string\">")]; *ptr; ptr++ )
        if ( *ptr == '<' )
          break;

      if ( !*ptr )
      {
        fprintf( stderr, "Line %d: Weird FMAID\n", linenum );
        return 0;
      }

      *ptr = '\0';
      sprintf( ch_new, "http://purl.org/obo/owlapi/fma#FMA_%s", &line[strlen("<default:FMAID rdf:datatype=\"http://www.w3.org/2001/XMLSchema#string\">")] );

      fLabel = 1;
    }

    if ( line_begins( line, "</rdf:Description>" ) )
    {
      if ( !fLabel )
        add_iri_translation( ch_old, ch_old );
      else
      {
        if ( !fDescription )
        {
          weird_description:

          fprintf( stderr, "Line %d: Weird description?\n", linenum );
          return 0;
        }

        add_iri_translation( ch_old, ch_new );

        fLabel = fDescription = 0;
      }

      if ( isNodeId )
      {
        mark_as_node_id( ch_old );
        isNodeId = 0;
      }
      continue;
    }
  }

  fLabel = 0;
  fNonSpace = 0;
  linenum = 0;
  rewind(in);
  read_ptr = read_end;

  printf( "Second pass done.\n" );

  printf( "Final pass: Modifying FMA and printing output to %s...\n", argv[2] );

  for(;;)
  {
    QUICK_GETC( c, in );

    if ( !c )
    {
      *lptr = '\0';
      fprintf( out, "%s", line );
      break;
    }

    if ( c != '\n' )
    {
      if ( c == ' ' )
      {
        if ( !fNonSpace )
          continue;
      }
      else
        fNonSpace = 1;

      *lptr++ = c;
      continue;
    }

    linenum++;
    fNonSpace = 0;
    *lptr++ = '\n';
    *lptr = '\0';
    lptr = line;

    if ( fUnwantedClass )
    {
      if ( line_begins( line, "</rdf:Description>" ) )
        fUnwantedClass = 0;

      continue;
    }

    if ( line_begins( line, "<rdf:RDF" ) || line_begins( line, "xmlns:" ) )
      continue;

    if ( line_begins( line, "<rdf:Description" ) )
    {
      char *x = &line[strlen("<rdf:Description")];

      if ( !line_begins( x, "rdf:about=\"http" )
      ||   line_begins( line, "<rdf:Description rdf:about=\"http://www.w3.org/2002/07/owl#Class\">" )
      ||   line_begins( line, "<rdf:Description rdf:about=\"http://www.owl-ontologies.com/Ontology1303404274.owl\">" ) )
      {
        fUnwantedClass = 1;
        classes_discarded++;
        continue;
      }
    }

    if ( line_begins( line, "</rdf:Description>" ) )
    {
      if ( fmaid == -1 )
      {
        fprintf( stderr, "In file %s: line number %d: This class did not have an FMAID assigned to it\n", argv[1], linenum );
        return 0;
      }

      /*if ( fLabel == 0 )
        printf( "No English label found for this class: %d\n", fmaid );*/

      fprintf( out, "<Class rdf:about=\"http://purl.org/obo/owlapi/fma#FMA_%d\">\n", fmaid );
      fprintf( out, "%s", buf );
      fprintf( out, "</Class>\n\n" );

      classes_kept++;
      bptr = buf;
      *bptr = '\0';
      fmaid = -1;
      fLabel = 0;

      continue;
    }

    if ( line_begins( line, "<rdfs:subClassOf rdf:resource=\"" ) )
    {
      char *ptr;
      char derived[MAX_LINE_SIZE*2];

      for ( ptr = &line[strlen("<rdfs:subClassOf rdf:resource=\"")]; *ptr; ptr++ )
        if ( *ptr == '"' )
          break;

      if ( !*ptr )
      {
        fprintf( stderr, "Line %d: Weird subclass axiom\n", linenum );
        return 0;
      }

      *ptr = '\0';

      sprintf( derived, "<rdfs:subClassOf rdf:resource=\"%s\"/>\n", trie_to_static(get_translation(&line[strlen("<rdfs:subClassOf rdf:resource=\"")])) );

      line_to_buf( derived, &bptr, bend );
      continue;
    }

    if ( line_begins( line, "<rdfs:label xml:lang=\"en\">" ) )
    {
      char labelline[MAX_LINE_SIZE*2];

      sprintf( labelline, "<rdfs:label rdf:datatype=\"http://www.w3.org/2001/XMLSchema#string\">%s", &line[strlen("<rdfs:label xml:lang=\"en\">")] );
      line_to_buf( labelline, &bptr, bend );
      fLabel = 1;
      continue;
    }

    if ( line_begins( line, "<default:FMAID rdf:datatype=\"http://www.w3.org/2001/XMLSchema#string\">" ) )
    {
      char *numname, *nptr;

      numname = &line[strlen("<default:FMAID rdf:datatype=\"http://www.w3.org/2001/XMLSchema#string\">")];
      for ( nptr = numname; *nptr; nptr++ )
        if ( *nptr == '<' )
          break;

      if ( !*nptr )
      {
        fprintf( stderr, "Line %d: Malformed FMAID line\n", linenum );
        return 0;
      }

      *nptr = '\0';
      fmaid = strtoul( numname, NULL, 10 );
      continue;
    }

    for ( i = 0; objprops[i]; i++ )
    {
      char token[256], *ptr;

      sprintf( token, "<default:%s rdf:resource=\"", objprops[i] );

      if ( line_begins( line, token ) )
      {
        char derived[MAX_LINE_SIZE*2];
        trie *transl;

        line_to_buf( "<rdfs:subClassOf>\n", &bptr, end );
        line_to_buf( " <Restriction>\n", &bptr, end );

        if ( i == 0 ) // Special case for "part_of"
          sprintf( derived, "  <onProperty rdf:resource=\"http://www.geneontology.org/formats/oboInOwl#part_of\"/>\n" );
        else
          sprintf( derived, "  <onProperty rdf:resource=\"http://purl.org/obo/owlapi/fma#%s\"/>\n", objprops[i] );

        line_to_buf( derived, &bptr, end );

        for ( ptr = &line[strlen(token)]; *ptr; ptr++ )
          if ( *ptr == '"' )
            break;

        if ( !*ptr )
        {
          fprintf( stderr, "Line %d: Weird IRI\n", linenum );
          return 0;
        }

        *ptr = '\0';
        transl = get_translation(&line[strlen(token)]);

        sprintf( derived, "  <someValuesFrom rdf:resource=\"%s\"/>\n", trie_to_static( transl ) );

        if ( transl->data == transl )
          printf( "Warning: On line %d we found annotation to a weird resource\n", linenum );

        line_to_buf( derived, &bptr, end );

        line_to_buf( " </Restriction>\n", &bptr, end );
        line_to_buf( "</rdfs:subClassOf>\n", &bptr, end );

        break;
      }
    }
    if ( objprops[i] )
      continue;
  }

  fprintf( out, "\n</rdf:RDF>\n" );

  fclose(in);
  fclose(out);
  printf( "Done.\n" );
  return 1;
}

void line_to_buf( char *line, char **bptr, char *end )
{
  int len = strlen(line);

  if ( &(*bptr)[len] >= end-1 )
  {
    fprintf( stderr, "Buffer ran out.  A class went on too long.\n" );
    abort();
  }

  sprintf( *bptr, "%s", line );
  *bptr += len;
}

int line_begins( char *line, char *init )
{
  while ( *line == ' ' || *line == '\t' )
    line++;

  for (;;)
  {
    if ( !*init )
      return 1;

    if ( !*line )
      return 0;

    if ( *line != *init )
      return 0;

    line++, init++;
  }
}

void add_iri_translation( char *ch_old, char *ch_new )
{
  trie *tr_old, *tr_new;

  tr_old = trie_strdup( ch_old, iritrie );

  if ( tr_old->data )
  {
    fprintf( stderr, "add_iri_translation: duplicate translation found\n" );
    fprintf( stderr, "ch_old: %s\n", ch_old );
    fprintf( stderr, "ch_new: %s\n", ch_new );
    abort();
  }

  tr_new = trie_strdup( ch_new, iritrie );

  tr_old->data = tr_new;
}

trie *get_translation( char *ch_old )
{
  trie *t = trie_search( ch_old, iritrie );

  if ( !t )
  {
    fprintf( stderr, "get_translation: no match: %s\n", ch_old );
    abort();
  }

  if ( !t->data )
  {
    fprintf( stderr, "get_translation: no data: %s\n", ch_old );
    abort();
  }

  return t->data;
}

void mark_as_node_id( char *key )
{
  trie *t = trie_search( key, iritrie );

  if ( !t )
  {
    fprintf( stderr, "mark_as_node_id: couldn't find trie entry for %s\n", key );
    abort();
  }

  t->isNodeId = 1;
}

void fprintf_preamble( FILE *out, FILE *props )
{
  char ch;

  for(;;)
  {
    ch = getc( props );

    if ( !ch || ch == EOF )
      break;
    else
      fprintf( out, "%c", ch );
  }
  fprintf( out, "\n\n" );
}
