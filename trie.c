#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "trie.h"
#include "macro.h"

trie *iritrie;

trie *blank_trie( void )
{
  trie *t;

  CREATE( t, trie, 1 );
  t->parent = NULL;
  t->children = NULL;
  t->label = NULL;
  t->data = NULL;
  t->isNodeId = 0;

  return t;
}

trie *trie_strdup( char *buf, trie *base )
{
  char *bptr;
  trie *t;
  int cnt;

  bptr = buf;
  t = base;

  for(;;)
  {
    trie_strdup_main_loop:

    if ( !*bptr )
      return t;

    if ( t->children )
    {
      trie **child, *blank=NULL;

      for ( child = t->children, cnt=0; *child; child++ )
      {
        cnt++;

        if ( !*((*child)->label) )
        {
          blank = *child;
          continue;
        }

        if ( *((*child)->label) == *bptr )
        {
          /*
           * Found an edge with same initial char as bptr
           */
          char *bx, *lx;

          for ( bx = bptr, lx = ((*child)->label); ; )
          {
            if ( !*lx )
            {
              bptr = bx;
              t = *child;
              goto trie_strdup_main_loop;
            }

            if ( !*bx )
            {
              trie *inter = blank_trie(), *cx = *child;
              char *tmp;

              inter->parent = t;
              inter->label = strndup(cx->label, lx-(cx->label) );
              CREATE( inter->children, trie *, 2 );
              inter->children[0] = cx;
              inter->children[1] = NULL;

              tmp = cx->label;
              cx->label = strdup(lx);
              free( tmp );
              cx->parent = inter;
              *child = inter;

              return inter;
            }

            if ( *lx != *bx )
            {
              trie *inter = blank_trie(), *cx = *child, *cxx;
              char *oldlabel;

              inter->parent = t;
              CREATE( inter->children, trie *, 3 );
              inter->children[0] = cx;
              inter->children[1] = blank_trie();
              inter->children[2] = NULL;

              cxx = inter->children[1];
              cxx->parent = inter;
              cx->parent = inter;
              *child = inter;

              inter->label = strndup(cx->label, (lx-cx->label));

              oldlabel = cx->label;

              cx->label = strdup(lx);
              free(oldlabel);
              cxx->label = strdup(bx);

              return cxx;
            }

            lx++;
            bx++;
          }
        }
      }

      /*
       * No edges have the same first char as bptr.
       * If there was a blank edge, use that.
       */
      if ( blank )
      {
        free( blank->label );
        blank->label = strdup(bptr);
        return blank;
      }
      else
      {
        /*
         * If no edges have same first char as bptr,
         * and there are no blank edges, then create new edge.
         */
        trie **children, *cx;

        CREATE( children, trie *, cnt + 2 );
        if ( t->children )
          memcpy( children, t->children, sizeof(trie*) * cnt );
        children[cnt] = blank_trie();
        cx = children[cnt];
        cx->parent = t;
        cx->label = strdup( bptr );
        children[cnt+1] = NULL;

        free( t->children );
        t->children = children;

        return cx;
      }
    }
    else
    {
      /*
       * No outgoing edges:
       * Create one.
       */
      trie *cx;

      CREATE( t->children, trie *, 2 );
      t->children[0] = blank_trie();
      cx = t->children[0];
      cx->parent = t;
      cx->label = strdup(bptr);

      t->children[1] = NULL;

      return cx;
    }
  }
}

trie *trie_search( char *buf, trie *base )
{
  char *bptr;
  trie *t;

  bptr = buf;
  t = base;

  for(;;)
  {
    trie_search_main_loop:

    if ( !*bptr )
      return t;

    if ( t->children )
    {
      trie **child;

      for ( child = t->children; *child; child++ )
      {
        if ( *((*child)->label) == *bptr )
        {
          /*
           * Found an edge with same initial char as bptr
           */
          char *bx, *lx;

          for ( bx = bptr, lx = ((*child)->label); ; )
          {
            if ( !*lx )
            {
              bptr = bx;
              t = *child;
              goto trie_search_main_loop;
            }

            if ( !*bx )
              return NULL;

            if ( *lx != *bx )
              return NULL;

            lx++;
            bx++;
          }
        }
      }

      return NULL;
    }
    else
      return NULL;
  }
}


char *trie_to_static( trie *t )
{
  static char buf[MAX_STRING_LEN + 1], *bptr;
  trie *ancestor;
  char *label, *lptr;

  bptr = &buf[MAX_STRING_LEN];
  *bptr = '\0';

  ancestor = t;

  while ( ancestor->parent )
  {
    label = ancestor->label;

    lptr = &label[strlen(label)-1];
    do
    {
      if ( bptr <= buf )
      {
        fprintf( stderr, "Trie: trie_to_static ran out of space!\n" );
        abort();
      }
      *bptr-- = *lptr--;
    }
    while ( lptr >= label );

    ancestor = ancestor->parent;
  }

  return &bptr[1];
}
