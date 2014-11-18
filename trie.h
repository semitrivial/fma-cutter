#define MAX_STRING_LEN 32768

typedef struct TRIE trie;

struct TRIE
{
  trie *parent;
  char *label;
  trie **children;
  trie *data;
  int isNodeId;
};
