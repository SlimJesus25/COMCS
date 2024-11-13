#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// TODO: Update tree height on insert/remove time, and update the height procedure.

typedef struct{
    char* key;
    int value;
}kvalue_t;

struct BinaryTreeNode {
    kvalue_t key;
    struct BinaryTreeNode *left, *right;
};

int size = 0;
int height = 0;
pthread_mutex_t size_mutex;

void init(void) __attribute__((constructor));

void init(void) {
    pthread_mutex_init(&size_mutex, NULL);
}

int tree_height(struct BinaryTreeNode* root){
    if(root == NULL)
        return 0;
    return fmax(tree_height(root->left), tree_height(root->right)) + 1;
}

int balanceFactor(struct BinaryTreeNode* node){
    return tree_height(node->left) - tree_height(node->right);
}

struct BinaryTreeNode* rightRotation(struct BinaryTreeNode* node){
    struct BinaryTreeNode* leftSon = node->left;

    node->left = leftSon->right;
    leftSon->right = node;

    node = leftSon;

    return node;
}

struct BinaryTreeNode* leftRotation(struct BinaryTreeNode* node){
    struct BinaryTreeNode* rightSon = node->right;

    node->right = rightSon->left;
    rightSon->left = node;

    node = rightSon;

    return node;
}

struct BinaryTreeNode* twoRotations(struct BinaryTreeNode* node){
    if(balanceFactor(node) < 0){
        node->left = leftRotation(node->left);
        node = rightRotation(node);
    }else{
        node->right = rightRotation(node->right);
        node = leftRotation(node);
    }
    return node;
}

struct BinaryTreeNode* balanceNode(struct BinaryTreeNode* node){
    int bf = balanceFactor(node);

    if(bf < -1){
        if(balanceFactor(node->left) + bf > bf)
            return twoRotations(node);
        else
            return rightRotation(node);
    }else if (bf > 1){
        if(balanceFactor(node->right) + bf < bf)
            return twoRotations(node);
        else
            return leftRotation(node);
    }
    return node;
}

struct BinaryTreeNode* newNodeCreate(kvalue_t value) {
    struct BinaryTreeNode* temp = (struct BinaryTreeNode*)malloc(
        sizeof(struct BinaryTreeNode));
    temp->key = value;
    temp->left = temp->right = NULL;
    return temp;
}

struct BinaryTreeNode* searchNode(struct BinaryTreeNode* root, char* target){

    if(root == NULL || strcmp(root->key.key, target) == 0)
        return root;
    
    if(strcmp(root->key.key, target) < 0)
        return searchNode(root->right, target);
    return searchNode(root->left, target);
}

struct BinaryTreeNode* insertNode(struct BinaryTreeNode* node, kvalue_t value){
    if(node == NULL){
        pthread_mutex_lock(&size_mutex);
        size++;
        pthread_mutex_unlock(&size_mutex);
        return newNodeCreate(value);
    }
    if(strcmp(value.key, node->key.key) < 0)
        node->left = insertNode(node->left, value);
    else if(strcmp(value.key, node->key.key) > 0)
        node->right = insertNode(node->right, value);

    node = balanceNode(node);
    return node;
}

struct BinaryTreeNode* smallestElement(struct BinaryTreeNode* root){
    if(root == NULL || root->left == NULL)
        return root;
    return smallestElement(root->left);
}

struct BinaryTreeNode* deleteNode(struct BinaryTreeNode* node, kvalue_t value){
    if(node == NULL)
        return NULL;

    if(strcmp(value.key, node->key.key) < 0){
        node->left = deleteNode(node->left, value);
        node = balanceNode(node);
        return node;
    }else if(strcmp(value.key, node->key.key) > 0){
        node->right = deleteNode(node->right, value);
        node = balanceNode(node);
        return node;
    }
    
    pthread_mutex_lock(&size_mutex);
    size--;
    pthread_mutex_unlock(&size_mutex);
    struct BinaryTreeNode* n = smallestElement(node->right);

    if(n == NULL)
        return NULL;

    n->left = node->left;
    n->right = node->right;
    return n;
}

int tree_size(){
    int s;
    pthread_mutex_lock(&size_mutex);
    s = size;
    pthread_mutex_unlock(&size_mutex);
    return s;
}