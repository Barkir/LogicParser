#ifndef LOGIC_H
#define LOGIC_H

typedef struct _tree Tree;

enum types
{
    ERROR = -1,
    OPER = 0,
    VAR = 1,
    NUM = 2
};

typedef double field_t;

typedef struct _field
{
    enum types type;
    field_t value;

} Field;

typedef void *  (*TreeInit)    (const void*);
typedef int     (*TreeCmp)      (const void*, const void*);
typedef void    (*TreeFree)     (void*);
typedef int     (*TreeCb)       (Tree * t, int level, const void*);

const int DEF_SIZE = 1024;

enum colors
{
    OPER_COLOR = 0XEFF94F,
    NUM_COLOR = 0X5656EC,
    VAR_COLOR = 0X70Df70
};

enum operations
{
    ADD,
    SUB,
    MUL,
    DIV,
    POW,
    SIN,
    COS
};

enum errors
{
    SUCCESS,
    ALLOCATE_MEMORY_ERROR,
    MEMCPY_ERROR,
    FOPEN_ERROR,
    FCLOSE_ERROR
};

Tree * CreateTree(TreeInit init, TreeCmp cmp, TreeFree free);

int CreateNode(Tree * t, const void * pair);

int InsertTree(Tree * t, const void * pair);

int TreeParse(Tree * tree, const char * filename);

Tree * TreeDump(Tree * tree, const char * FileName);

Tree * TexDump(Tree * tree, const char * filename);

field_t CountTree(Tree * tree);

void DestroyTree(Tree * t);


#endif
