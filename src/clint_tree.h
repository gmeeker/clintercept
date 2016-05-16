/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
** CLIntercept OpenCL debugging utilities
** Copyright (c) 2012, Digital Anarchy, Inc.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** * Redistributions of source code must retain the above copyright notice,
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice,
**   this list of conditions and the following disclaimer in the documentation
**   and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CLINT_TREE_H_
#define _CLINT_TREE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct ClintRbTreeNode {
  struct ClintRbTreeNode *left;
  struct ClintRbTreeNode *right;
  struct ClintRbTreeNode *parent;
  int color;
} ClintRbTreeNode;

void clint_rb_tree_left_rotate(ClintRbTreeNode **tree, ClintRbTreeNode *x);
void clint_rb_tree_right_rotate(ClintRbTreeNode **tree, ClintRbTreeNode *x);
void clint_rb_tree_insert(ClintRbTreeNode **tree, ClintRbTreeNode *x);
ClintRbTreeNode *clint_tree_successor(ClintRbTreeNode *x);
void clint_rb_tree_delete_fixup(ClintRbTreeNode **tree, ClintRbTreeNode *x, ClintRbTreeNode *p);
void clint_rb_tree_delete(ClintRbTreeNode **tree, ClintRbTreeNode *z);
ClintRbTreeNode *clint_tree_first(ClintRbTreeNode *tree);
ClintRbTreeNode *clint_tree_next(ClintRbTreeNode *tree);

#define CLINT_TREE_ELEMS(T, KEY)                                    \
  union {                                                           \
    ClintRbTreeNode rb_node;                                        \
    struct { T *left, *right, *parent; } pointers;                  \
  } _node;                                                          \
  KEY _key

#define CLINT_DEFINE_TREE_FUNCS(type, KEY)                          \
type *clint_tree_find_##type(type *tree, KEY key);                  \
type *clint_tree_first_##type(type *tree);                          \
type *clint_tree_next_##type(type *tree);                           \
void clint_tree_insert_##type(type **tree, KEY key, type *z);       \
void clint_tree_delete_##type(type **tree, KEY key);                \
void clint_tree_erase_##type(type **tree, type *x);

#define CLINT_IMPL_TREE_FUNCS(type, KEY)                            \
type *clint_tree_find_##type(type *tree, KEY key)                   \
{                                                                   \
  while (tree != NULL) {                                            \
    if (key == tree->_key)                                          \
      return tree;                                                  \
    if (key < tree->_key)                                           \
      tree = tree->_node.pointers.left;                             \
    else                                                            \
      tree = tree->_node.pointers.right;                            \
  }                                                                 \
  return tree;                                                      \
}                                                                   \
                                                                    \
type *clint_tree_first_##type(type *tree)                           \
{                                                                   \
  return (type*)clint_tree_first(&tree->_node.rb_node);             \
}                                                                   \
                                                                    \
type *clint_tree_next_##type(type *tree)                            \
{                                                                   \
  return (type*)clint_tree_next(&tree->_node.rb_node);              \
}                                                                   \
                                                                    \
void clint_tree_insert_##type(type **tree, KEY key, type *z)        \
{                                                                   \
  type *y = NULL;                                                   \
  type *x = *tree;                                                  \
                                                                    \
  z->_key = key;                                                    \
  z->_node.pointers.left = z->_node.pointers.right = NULL;          \
  while (x != NULL) {                                               \
    y = x;                                                          \
    if (z->_key < x->_key) {                                        \
      x = x->_node.pointers.left;                                   \
    } else {                                                        \
      x = x->_node.pointers.right;                                  \
    }                                                               \
  }                                                                 \
  z->_node.pointers.parent = y;                                     \
  if (y == NULL) {                                                  \
    *tree = z;                                                      \
  } else if (z->_key < y->_key) {                                   \
    y->_node.pointers.left = z;                                     \
  } else {                                                          \
    y->_node.pointers.right = z;                                    \
  }                                                                 \
  clint_rb_tree_insert((ClintRbTreeNode**)tree, &z->_node.rb_node); \
}                                                                   \
                                                                    \
void clint_tree_delete_##type(type **tree, KEY key)                 \
{                                                                   \
  type *x = clint_tree_find_##type(*tree, key);                     \
  clint_rb_tree_delete((ClintRbTreeNode**)tree, &x->_node.rb_node); \
}                                                                   \
                                                                    \
void clint_tree_erase_##type(type **tree, type *x)                  \
{                                                                   \
  clint_rb_tree_delete((ClintRbTreeNode**)tree, &x->_node.rb_node); \
}

#ifdef __cplusplus
}
#endif

#endif // _CLINT_TREE_H_
