#include <stdio.h>
#include <stdlib.h>

#include "logic.h"
#include "buff.h"

void * FieldInit(const void * field);
int FieldCmp(const void * f1, const void * f2);
void FieldDestroy(void * field);

void * FieldInit(const void * field)
{
    Field * result = (Field*) malloc(sizeof(Field));
    result->value = ((const Field*) field)->value;
    result->type = ((const Field*)field)->type;
    return (void*)result;
}

int FieldCmp(const void * f1, const void * f2)
{
    field_t val1 = ((const Field*)f1)->value;
    field_t val2 = ((const Field*)f2)->value;
    if (val1 < val2) return -1;
    if (val1 > val2) return 1;
    return 0;
}

void FieldDestroy(void * field)
{
    free((Field*) field);
}

int main(void)
{
    Tree * tree = CreateTree(FieldInit, FieldCmp, free);
    TreeParse(tree, "toparse.txt");
    TreeDump(tree, "tree");

    Tree * dnf_tree = DnfTree(tree);
    Tree * knf_tree = KnfTree(tree);

    TreeDump(dnf_tree, "dnf_tree");
    TreeDump(knf_tree, "knf_tree");

    TexDump(dnf_tree, "dnf_tex");
    TexDump(knf_tree, "knf_tex");
    TexDump(tree, "tex");

    DestroyTree(dnf_tree);
    DestroyTree(knf_tree);
    DestroyTree(tree);

}
