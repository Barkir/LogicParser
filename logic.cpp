#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

#include "buff.h"
#include "logic.h"

#define DEBUG

typedef struct _node
{
    void * value;
    struct _node * left;
    struct _node * right;

} Node;

struct _tree
{
    Node * root;
    TreeInit init;
    TreeCmp cmp;
    TreeFree free;
};

static int _create_node(Tree * t, Node ** node, const void * pair);
int _tree_parse(Tree* tree, Node ** node, const char ** string);
Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out);
Node * _insert_tree(Tree * t, Node ** root, const void * pair);
void _destroy_tree(Tree * t, Node * n);

int SyntaxError(char exp, char real, const char * func, int line);
Node ** StringTokenize(const char * string, int * p);

Node * GetG(Node ** nodes, int * p);
Node * GetXor(Node ** nodes, int * p);
Node * GetAnd(Node ** nodes, int * p);
Node * GetInv(Node ** nodes, int * p);
Node * GetP(Node ** nodes, int * p);
Node * GetN(Node ** nodes, int * p);
Node * GetX(Node ** nodes, int * p);

Node * _copy_branch(Node * node);
Node * _copy_node(Node * node);
Field * _copy_field(Field * field);
Node * _create_node(Field * val, Node * left, Node * right);
Field * _create_field(field_t val, enum types type);

field_t _node_count(Node * node, field_t val, field_t var);

unsigned int NodeColor(Node * node);
field_t NodeValue(Node * node);
enum types NodeType(Node * node);


int FindVar(Node * node);

Tree * CreateTree(TreeInit init, TreeCmp cmp, TreeFree free)
{
    Tree * t = (Tree*) malloc(sizeof(Tree));
    if (!t) return NULL;
    *t = (Tree) {NULL, init, cmp, free};
    return t;
}

static int _create_node(Tree * t, Node ** node, const void * pair)
{
    if (!*node)
    {
        if (((*node) = (Node *) malloc(sizeof(Node))) == NULL) return 0;
        **node = (Node) {t->init ? t->init(pair) : (void*) pair, NULL, NULL};
        return 0;
    }

    if (t->cmp(pair, (*node)->value) > 0)
        return _create_node(t, &(*node)->right, pair);
    else
        return _create_node(t, &(*node)->left, pair);

    (*node)->left = (*node)->right = NULL;

    return 0;
}

int CreateNode(Tree * t, const void * pair)
{
    return _create_node(t, &t->root, pair);
}

Node * _insert_tree(Tree * t, Node ** root, const void * pair)
{
    if (!*root)
    {
        if ((*root = (Node*) malloc(sizeof(Node))) == NULL) return NULL;
        (*root)->value = t->init ? t->init(pair) : (void*) pair;
        (*root)->right = NULL;
        (*root)->left = NULL;
        return *root;
    }

    if (t->cmp(pair, (*root)->value) > 0)
        return _insert_tree(t, &(*root)->right, pair);
    else
        return _insert_tree(t, &(*root)->left, pair);
}

int InsertTree(Tree * t, const void * pair)
{
    return !!_insert_tree(t, &t->root, pair);
}

const char * _oper_to_string(Node * node)
{
    field_t value = NodeValue(node);
    switch((int) value)
    {
        case '&':   return "and";
        case '|':   return "or";
        case '^':   return "xor";
        case '~' :  return "not";
        default:    return "?";
    }
}

Tree * _tree_dump_func(Tree * tree, Node ** node, FILE * Out)
{

    if (!tree)
        return NULL;

    if (!*node) return NULL;

    unsigned int color = NodeColor(*node);
    field_t field = NodeValue(*node);

    switch (((Field*)(*node)->value)->type)
    {
        case OPER:  fprintf(Out, "node%p [shape = Mrecord; label = \"{%s | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, _oper_to_string(*node), *node, color);
                    break;

        case VAR:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%c | %p}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, (int) field, *node, color);
                    break;
        case NUM:   fprintf(Out, "node%p [shape = Mrecord; label = \"{%lg}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, field, color);
                    break;

        default:    fprintf(Out, "node%p [shape = Mrecord; label = \"{}\"; style = filled; fillcolor = \"#%06X\"];\n",
                    *node, color);
                    break;

    }

    if ((*node)->left)
        fprintf(Out, "node%p -> node%p\n", *node, (*node)->left);

    if ((*node)->right)
        fprintf(Out, "node%p -> node%p\n", *node, (*node)->right);

    _tree_dump_func(tree, &(*node)->left, Out);
    _tree_dump_func(tree, &(*node)->right, Out);

    return tree;
}

Tree * TreeDump(Tree * tree, const char * FileName)
{
    FILE * Out = fopen(FileName, "wb");

    fprintf(Out, "digraph\n{\n");
    _tree_dump_func(tree, &tree->root, Out);
    fprintf(Out, "}\n");

    char command[DEF_SIZE] = "";
    sprintf(command, "dot %s -T png -o %s.png", FileName, FileName);

    fclose(Out);

    system(command);

    return tree;
}

char * _tex_dump_func(Tree * tree, Node ** node)
{

    enum types type = NodeType(*node);
    field_t value = NodeValue(*node);
    char * oper = NULL;
    char * number = NULL;
    char * variable = NULL;
    char * left = NULL;
    char * right = NULL;

    switch((int) type)
    {
        case NUM:
            number = (char*) calloc(DEF_SIZE, 1);
            sprintf(number, "%lg", value);
            return number;

        case VAR:
            variable = (char*) calloc(DEF_SIZE, 1);
            sprintf(variable, "%c", (int) value);
            return variable;

        case OPER:

            if ((*node)->left) left = _tex_dump_func(tree, &(*node)->left);
            if ((*node)->right)right = _tex_dump_func(tree, &(*node)->right);
            char * oper = (char*) calloc(DEF_SIZE + (left?strlen(left):10) + (right?strlen(right):10), 1);
            if (!oper) return NULL;

            switch((int) value)
            {
                case '^':   sprintf(oper, "({%s} \\oplus {%s})", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '&':   sprintf(oper, "({%s} \\wedge {%s})", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '|':   sprintf(oper, "({%s} \\lor {%s})", left, right);
                            free(left);
                            free(right);
                            return oper;

                case '~':   sprintf(oper, "\\neg({%s})", left);
                            free(left);
                            free(right);
                            return oper;

                default:    return NULL;
            }
    }

    return oper;
}

Tree * TexDump(Tree * tree, const char * filename)
{
    FILE * Out = fopen(filename, "wb");

    fprintf(Out,    "\\documentclass[12]{article}\n"
                    "\\usepackage{amsmath}\n"
                    "\\usepackage{amssymb}\n"
                    "\\usepackage{graphicx} % Required for inserting images\n"
                    "\\usepackage[utf8]{inputenc}\n"
                    "\\usepackage[russian]{babel}\n"
                    "\\begin{document}\n"
                    "\\begin{small}\n");

    fprintf(Out, "\n\\[");
    char * expression = _tex_dump_func(tree, &tree->root);
    fprintf(Out, expression);

    fprintf(Out, "\\]\n");
    fprintf(Out,    "\\end{small}\n"
                    "\\end{document}\n");

    fclose(Out);

    char command[DEF_SIZE] = "";
    sprintf(command, "pdflatex --output-directory=./tmp %s", filename);

    system(command);

    free(expression);
    return tree;
}

#ifdef DEBUG
#define DESTROY(...)                                                             \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __func__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }
#else
#define DESTROY(...)
#endif

int TreeParse(Tree * tree, const char * filename)
{
    FILE * file = fopen(filename, "rb");
    if (ferror(file)) return FOPEN_ERROR;

    char * expression = CreateBuf(file);
    if (!expression) return ALLOCATE_MEMORY_ERROR;

    const char * ptr = expression;
    int pointer = 0;

    Node ** array = StringTokenize(ptr, &pointer);

    printf("%p\n", (*array));

    pointer = 0;
    tree->root = GetG(array, &pointer);

    pointer = 0;
    while (array[pointer])
    {
        free(array[pointer]->value);
        free(array[pointer]);
        pointer++;
    }

    free(array);

    if (fclose(file) == EOF) return FCLOSE_ERROR;
    free(expression);

    return 1;
}

field_t _node_count(Node * node, field_t val)
{
    enum types type = NodeType(node);
    field_t field = NodeValue(node);
    if (type == NUM) return field;

    if (type == OPER)
    {
        switch((int) field)
        {
            case '+': val = _node_count(node->left, val) + _node_count(node->right, val); break;
            case '-': val = _node_count(node->left, val) - _node_count(node->right, val); break;
            case '*': val = _node_count(node->left, val) * _node_count(node->right, val); break;
            case '/': val = _node_count(node->left, val) / _node_count(node->right, val); break;
            case '^': val = pow(_node_count(node->left, val), _node_count(node->right, val)); break;
            default: return 0;
        }
    }

    return val;
}

field_t CountTree(Tree * tree)
{
    return _node_count(tree->root, 0);
}

void _destroy_tree(Tree * t, Node * n)
{
    if (!n) return;

    DESTROY("SUBTREE %p value = %lg (%c) %p. Destroying.", n, NodeValue(n), (int) NodeValue(n), ((Field*) n->value));

    _destroy_tree(t, n->left);
    _destroy_tree(t, n->right);

    if (t->free) t->free(n->value);
    DESTROY("SUBTREE %p. Destroyed.", n);
    free(n);
}

void DestroyTree(Tree * t)
{
    DESTROY("STARTED TREE DESTROY");
    _destroy_tree(t, t->root);
    free(t);

}

field_t NodeValue(Node * node)
{
    if (!node) return NULL;
    DESTROY("Getting node value %p %lg", node, ((Field*)(node->value))->value);
    return ((Field*)(node->value))->value;
}

enum types NodeType(Node * node)
{
    if (!node) return ERROR;
    return ((Field*)(node->value))->type;
}


unsigned int NodeColor(Node * node)
{
    unsigned int color = 0;
    switch (((Field*) (node->value))->type)
        {
        case OPER:
            color = OPER_COLOR;
            break;

        case VAR:
            color = VAR_COLOR;
            break;

        case NUM:
            color = NUM_COLOR;
            break;

        default:
            break;
        }

    return color;
}

int FindVar(Node * node)
{
    if (!node) return 0;
    if (NodeType(node) == VAR) return VAR;


    int result = FindVar(node->left);
    if (!result) result = FindVar(node->right);
    return result;
}

int _need_to_simplify(Node ** node)
{
    // if (FindVar(*node) != VAR) return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->left) == 1))  return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->left) == 0))  return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->right) == 0)) return 1;
    if ((int)NodeValue((*node)) == '*' && (NodeValue((*node)->right) == 1)) return 1;
    return 0;
}

// PARSER

#define SYNTAX_ERROR(exp, real)                     \
    {                                               \
        SyntaxError(exp, real, __func__, __LINE__); \
    }                                               \

#ifdef DEBUG
#define PARSER(...)                                                             \
    {                                                                            \
    fprintf(stderr, ">>> %s:%d: ", __func__, __LINE__);                          \
    fprintf(stderr, __VA_ARGS__);                                                \
    fprintf(stderr, "\n");                                                       \
    }
#else
#define PARSER(...)
#endif

Field * _create_field(field_t val, enum types type)
{
    Field * field = (Field*) calloc(1, sizeof(Field));
    if (!field) return NULL;

    field->value = val;
    field->type = type;

    return field;
}

Node * _create_node(Field * val, Node * left, Node * right)
{
    Node * node = (Node*) calloc(1, sizeof(Node));
    if (!node) return NULL;
    node->value = val;

    if (left) node->left = left;
    if (right) node->right = right;
    return node;
}

Field * _copy_field(Field * field)
{
    if (!field) return NULL;
    Field * copy_field = (Field*) calloc(1, sizeof(Field));
    if (!copy_field) return NULL;

    copy_field->value = field->value;
    copy_field->type = field->type;

    return copy_field;
}

Node * _copy_node(Node * node)
{
    if (!node) return NULL;
    PARSER("Copying node %p with value %lg...", node, ((Field*)node->value)->value);
    Field * copy_field = _copy_field((Field*)node->value);
    if (!copy_field) return NULL;
    Node * copy_node = (Node*) calloc(1, sizeof(Node));
    if (!copy_node) return NULL;

    copy_node->value = copy_field;
    PARSER("Created node with value %lg", copy_field->value);
    if (node->left)  copy_node->left = node->left;
    if (node->right) copy_node->right = node->right;

    return copy_node;
}

Node * _copy_branch(Node * node)
{
    if (!node) return NULL;
    PARSER("Copying branch %p with value %lg...", node, ((Field*)node->value)->value);
    Node * result = _copy_node(node);
    if (!result) return NULL;

    result->left = _copy_branch(node->left);
    result->right = _copy_branch(node->right);

    return result;
}

int SyntaxError(char exp, char real, const char * func, int line)
{
    fprintf(stderr, ">>> SyntaxError: <expected %c> <got %c> %s %d\n", exp, real, func, line);
    assert(0);
}


Node * _oper_token(const char * string, int * p)
{
    Field * field = _create_field((field_t)string[*p], OPER);
    if  (!field) return NULL;
    Node * result = _create_node(field, NULL, NULL);
    if (!result) return NULL;
    (*p)++;
    return result;
}

Node * _find_name(char * result)
{
    printf("Need to find name %s... ", result);
    Field * field = _create_field((field_t) result[0], VAR);
    if (!field) return NULL;
    Node * node = _create_node(field, NULL, NULL);
    if (!node) return NULL;
    return node;
}

Node * _name_token(const char * string, int * p)
{
    int start_p = *p;

    while(isalpha(string[*p])) (*p)++;

    char * result = (char*) calloc((*p) - start_p + 1, 1);
    memcpy(result, &string[start_p], (*p) - start_p);

    Node * name = _find_name(result);
    free(result);
    if (!name) return NULL;
    return name;
}

Node * _number_token(const char * string, int * p)
{
    char * end = NULL;
    field_t number = strtod(&(string[*p]), &end);
    if (!end) return NULL;
    *p += (int)(end - &string[*p]);
    Field * field = _create_field(!!number, NUM);
    if (!field) return NULL;
    Node * num = _create_node(field, NULL, NULL);
    if (!num) return NULL;
    return num;
}

Node * _get_token(const char * string, int * p)
{
    if (string[*p] == '(' ||
        string[*p] == ')' ||
        string[*p] == '&' ||
        string[*p] == '|' ||
        string[*p] == '^' ||
        string[*p] == '~')
        {printf("operator %c! ", string[*p]); return _oper_token(string, p);}

    if (isalpha(string[*p])) {printf("name %c! ", string[*p]); return _name_token(string, p);}

    if (isdigit(string[*p])) {printf("number %c! ", string[*p]); return _number_token(string, p);}

    return NULL;

}

Node ** StringTokenize(const char * string, int * p)
{
    int size = 0;
    size_t arr_size = DEF_SIZE;
    Node ** nodes = (Node**) calloc(arr_size, sizeof(Node*));
    while (string[*p] != 0)
    {
        nodes[size] = _get_token(string, p);
        printf("got node %u %p with value %lg!\n", size+1, *(nodes + size), NodeValue(nodes[size]));
        size++;
    }
    printf("%p\n");
    return nodes;
}

Node * GetG(Node ** nodes, int * p)
{
    PARSER("Got node %p", nodes[*p]);
    Node * result = GetXor(nodes, p);
    PARSER("Got result!");
    if (!nodes[*p]) return result;
    // if (nodes[*p]) SYNTAX_ERROR(0, NodeValue(nodes[*p]));
    PARSER("PARSING ENDED");
    (*p)++;
    return result;
}

Node * GetXor(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting E... Got node %p", nodes[*p]);
    Node * val1 = GetAnd(nodes, p);
    if (!nodes[*p]) return val1;

    while ((int)NodeValue(nodes[*p]) == '^' || (int)NodeValue(nodes[*p]) == '|')
    {
        PARSER("Got node %p", nodes[*p]);
        int op = (int) NodeValue(nodes[*p]);

        Field * operation = NULL;
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        (*p)++;
        if (!nodes[*p]) return NULL;

        Node * val2 = GetAnd(nodes, p);
        PARSER("Got val2");

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetXor Finished");
    return val1;
}

Node * GetAnd(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting T... Got node %p", nodes[*p]);
    PARSER("Getting pow in T val1..."); Node * val1 = GetInv(nodes, p);
    if (!nodes[*p]) {PARSER("GetAnd Finished!"); return val1;}

    while ((int)NodeValue(nodes[*p]) == '&')
    {
        int op = (int)NodeValue(nodes[*p]);

        Field * operation = NULL;

        operation = _create_field((field_t) op, OPER);

        if (!operation) return NULL;


        (*p)++;
        if (!nodes[*p]) return val1;

        PARSER("Getting pow in T val2..."); Node * val2 = GetInv(nodes, p);

        if (!(val1 = _create_node(operation, val1, val2))) return NULL;
    }
    PARSER("GetAnd Finished");
    return val1;
}

Node * GetInv(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Getting ~... Got node %p", nodes[*p]);
    Node * val1 = NULL;
    while ((int)NodeValue(nodes[(*p)]) == '~')
    {
        PARSER("Got '~'");
        int op = (int) NodeValue(nodes[(*p)]);

        Field * operation = NULL;
        if (!(operation = _create_field((field_t) op, OPER))) return NULL;

        (*p)++;

        val1 = GetP(nodes, p);

        if (!(val1 = _create_node(operation, val1, NULL))) return NULL;
    }
    if (!nodes[*p]) {PARSER("GetInv Finished"); return val1;}
    if (!val1) val1 = GetP(nodes, p);
    PARSER("GetInv Finished");
    return val1;
}

Node * GetP(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("node = %p. Getting P...", nodes[*p]);
    if ((int) NodeValue(nodes[*p]) == '(')
    {
        (*p)++;
        PARSER("Got '('");
        Node * val = GetXor(nodes, p);
        PARSER("Got node %p", nodes[*p]);
        if (!nodes[*p]) return val;
        if ((int) NodeValue(nodes[(*p)]) != ')') SYNTAX_ERROR(')', (int) NodeValue(nodes[(*p)]));
        PARSER("Got ')'");
        (*p)++;
        return val;
    }
    else if (NodeType(nodes[*p]) == VAR) return GetX(nodes, p);
    else return GetN(nodes, p);
}

Node * GetN(Node ** nodes, int * p)
{
    if (!nodes[*p]) return NULL;
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}

Node * GetX(Node ** nodes, int * p)
{
    PARSER("Got node %p", nodes[*p]);
    Node * result = _copy_node(nodes[*p]);
    (*p)++;
    if (!result) return NULL;
    return result;
}

#define NOT _create_field((field_t) '~', OPER)
#define AND _create_field((field_t) '&', OPER)
#define OR _create_field((field_t) '|', OPER)

Node * _dnf_or(Node * node)
{
    return _create_node(NOT,
                _create_node(AND,
                        _create_node(NOT, _copy_node(node->left), NULL),
                        _create_node(NOT, _copy_node(node->right), NULL)), NULL);

}

Node * _dnf_xor(Node * node)
{
    return _create_node(NOT,
                _create_node(AND,
                        _create_node(NOT, _create_node(AND, _create_node(NOT, _copy_node(node->left), NULL), _copy_node(node->right)), NULL),
                        _create_node(NOT, _create_node(AND, _create_node(NOT, _copy_node(node->right), NULL), _copy_node(node->left)), NULL)), NULL);
}

Node * _dnf_tree(Node * node)
{

    if (node->left) node->left = _dnf_tree(node->left);
    if (node->right) node->right = _dnf_tree(node->right);

    switch((int)NodeValue(node))
    {
        case '|':   return _dnf_or(node);
        case '^':   return _dnf_xor(node);
        default:    return node;

    }

    return node;

}

Node * _simplify_tree(Node * node)
{
    if (node->left) node->left = _simplify_tree(node->left);
    if (node->right) node->right = _simplify_tree(node->right);

    if ((int)NodeValue(node) == '~' && (int)NodeValue(node->left) == '~')
    {
        Node * toReturn = node->left->left;
        free(node->left);
        free(node->right);
        return toReturn;
    }

    return node;
}

Tree * DnfTree(Tree * tree)
{
    Node * ret = _copy_branch(tree->root);
    if (!ret) return NULL;

    Tree * new_tree = CreateTree(tree->init, tree->cmp, tree->free);
    if (!new_tree) return NULL;

    ret = _dnf_tree(ret);
    ret = _simplify_tree(ret);
    new_tree->root = ret;

    if (!ret) return NULL;
    return new_tree;
}

Node * _knf_and(Node * node)
{
    return _create_node(NOT,
                _create_node(OR,
                    _create_node(NOT, _copy_node(node->left), NULL),
                    _create_node(NOT, _copy_node(node->right), NULL)), NULL);
}


Node * _knf_xor(Node * node)
{
    return _create_node(OR,
            _knf_and(_create_node(AND, _create_node(NOT, _copy_node(node->left), NULL), _copy_node(node->right))),
            _knf_and(_create_node(AND, _create_node(NOT, _copy_node(node->right), NULL), _copy_node(node->left))));
}

Node * _knf_tree(Node * node)
{

    if (node->left) node->left = _knf_tree(node->left);
    if (node->right) node->right = _knf_tree(node->right);

    switch((int)NodeValue(node))
    {
        case '&':   return _knf_and(node);
        case '^':   return _knf_xor(node);
        default:    return node;

    }

    return node;

}

Tree * KnfTree(Tree * tree)
{
    Node * ret = _copy_branch(tree->root);
    if (!ret) return NULL;

    Tree * new_tree = CreateTree(tree->init, tree->cmp, tree->free);
    if (!new_tree) return NULL;

    ret = _knf_tree(ret);
    ret = _simplify_tree(ret);
    new_tree->root = ret;

    if (!ret) return NULL;
    return new_tree;
}
