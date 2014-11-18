#define QUICK_GETC( ch, fp )\
do\
{\
  if ( read_ptr == read_end )\
  {\
    fread_len = fread( read_buf, sizeof(char), READ_BLOCK_SIZE, fp );\
    if ( fread_len < READ_BLOCK_SIZE )\
      read_buf[fread_len] = '\0';\
    read_ptr = read_buf;\
  }\
  ch = *read_ptr++;\
}\
while(0)

#define CREATE(result, type, number)\
do\
{\
    if (!((result) = (type *) calloc ((number), sizeof(type))))\
    {\
        fprintf(stderr, "Malloc failure at %s:%d\n", __FILE__, __LINE__ );\
        abort();\
    }\
} while(0)
