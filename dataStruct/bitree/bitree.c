#include <stdio.h>
#include <stdlib.h>

struct binaryTree
{
    int data;
    struct binaryTree *left;
    struct binaryTree *right;
};

void binaryTree_insert(struct binaryTree **tree, int val)
{
    struct binaryTree *node = NULL;
    if( !(*tree) )
    {
        node = (struct binaryTree *)malloc(sizeof(struct binaryTree));
        node->left = node->right = NULL;
        node->data = val;
        *tree = node;
        return;
    }

    if(val < (*tree)->data)
    {
       binaryTree_insert(&(*tree)->left, val);
    }
    else if(val > (*tree)->data)
    {
        binaryTree_insert(&(*tree)->right, val);
    }
}

void binaryTree_delete(struct binaryTree *tree)
{
    if(tree)
    {
        binaryTree_delete(tree->left);
        binaryTree_delete(tree->right);
        free(tree);
    }
}

void preorder(struct binaryTree *tree)
{
    if(tree)
    {
        printf("%d\n", tree->data);
        preorder(tree->left);
        preorder(tree->right);
    }
}

void inorder(struct binaryTree *tree)
{
    if(tree)
    {
        inorder(tree->left);
        printf("%d\n", tree->data);
        inorder(tree->right);
    }
}

void postorder(struct binaryTree *tree)
{
    if(tree)
    {
        postorder(tree->left);
        postorder(tree->right);
        printf("%d\n", tree->data);
    }
}

int main(void)
{
    node * root;
    node * tmp;
    //int i;

    root = NULL;
    /* Inserting nodes into tree */
    insert(&root,9);
    insert(&root,4);
    insert(&root,15);
    insert(&root,6);
    insert(&root,12);
    insert(&root,17);
    insert(&root,2);

    printf("Pre Order Display\n");
    print_preorder(root);

    printf("In Order Display\n");
    print_inorder(root);

    printf("Post Order Display\n");
    print_postorder(root);


    /* Deleting all nodes of tree */
    deltree(root);
}
