#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//TODO: Insert node autobalancing (to ensure +/- O(log(n))).

typedef struct{
    char* key;
    int value;
}kvalue_t;

struct BinaryTreeNode {
    kvalue_t key;
    struct BinaryTreeNode *left, *right;
};

int size = 0;
pthread_mutex_t size_mutex;

void init(void) __attribute__((constructor));

void init(void) {
    pthread_mutex_init(&size_mutex, NULL);
}

struct BinaryTreeNode* newNodeCreate(kvalue_t value) {
    struct BinaryTreeNode* temp = (struct BinaryTreeNode*)malloc(
        sizeof(struct BinaryTreeNode));
    temp->key = value;
    temp->left = temp->right = NULL;
    pthread_mutex_lock(&size_mutex);
    size = 1;
    pthread_mutex_unlock(&size_mutex);
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
        return node;
    }else if(strcmp(value.key, node->key.key) > 0){
        node->right = deleteNode(node->right, value);
        return node;
    }
    
    pthread_mutex_lock(&size_mutex);
    size--;
    pthread_mutex_unlock(&size_mutex);
    struct BinaryTreeNode* n = smallestElement(node->right);

    if(n == NULL)\
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