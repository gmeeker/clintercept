/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
** Clint OpenCL debugging utilities
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

/* Red-black trees as described in "Introduction to Algorithms" by
** Cormen, Leiserson, Rivest.
*/

#include "clint_tree.h"

#define BLACK 0
#define RED 1

void clint_rb_tree_left_rotate(ClintRbTreeNode **tree, ClintRbTreeNode *x)
{
  ClintRbTreeNode *y = x->right;
  x->right = y->left;
  if (y->left != NULL) {
    y->left->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == NULL) {
    *tree = y;
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }
  y->left = x;
  x->parent = y;
}

void clint_rb_tree_right_rotate(ClintRbTreeNode **tree, ClintRbTreeNode *x)
{
  ClintRbTreeNode *y = x->left;
  x->left = y->right;
  if (y->right != NULL) {
    y->right->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == NULL) {
    *tree = y;
  } else if (x == x->parent->right) {
    x->parent->right = y;
  } else {
    x->parent->left = y;
  }
  y->right = x;
  x->parent = y;
}

void clint_rb_tree_insert(ClintRbTreeNode **tree, ClintRbTreeNode *x)
{
  x->color = RED;
  while (x != *tree && x->parent->color == RED) {
    ClintRbTreeNode *y;
    if (x->parent->parent != NULL && x->parent == x->parent->parent->left) {
      if (x->parent->parent != NULL)
        y = x->parent->parent->right;
      else
        y = NULL;
      if (y != NULL && y->color == RED) {
        x->parent->color = BLACK;
        y->color = BLACK;
        x->parent->parent->color = RED;
        x = x->parent->parent;
      } else {
        if (x == x->parent->right) {
          x = x->parent;
          clint_rb_tree_left_rotate(tree, x);
        }
        x->parent->color = BLACK;
        x->parent->parent->color = RED;
        clint_rb_tree_right_rotate(tree, x->parent->parent);
      }
    } else {
      if (x->parent->parent != NULL)
        y = x->parent->parent->left;
      else
        y = NULL;
      if (y != NULL && y->color == RED) {
        x->parent->color = BLACK;
        y->color = BLACK;
        x->parent->parent->color = RED;
        x = x->parent->parent;
      } else {
        if (x == x->parent->left) {
          x = x->parent;
          clint_rb_tree_right_rotate(tree, x);
        }
        x->parent->color = BLACK;
        if (x->parent->parent != NULL) {
          x->parent->parent->color = RED;
          clint_rb_tree_left_rotate(tree, x->parent->parent);
        }
      }
    }
  }
  (*tree)->color = BLACK;
}

ClintRbTreeNode *clint_tree_successor(ClintRbTreeNode *x)
{
  ClintRbTreeNode *y;

  if (x->right != NULL) {
    x = x->right;
    while (x->left != NULL) {
      x = x->left;
    }
    return x;
  }
  y = x->parent;
  while (y != NULL && x == y->right) {
    x = y;
    y = y->parent;
  }
  return y;
}

void clint_rb_tree_delete_fixup(ClintRbTreeNode **tree, ClintRbTreeNode *x, ClintRbTreeNode *p)
{
  ClintRbTreeNode *w;

  while (x != (*tree) && (x == NULL || x->color == BLACK)) {
    if (x == p->left) {
      w = p->right;
      if (w != NULL && w->color == RED) {
        w->color = BLACK;
        p->color = RED;
        clint_rb_tree_left_rotate(tree, p);
        w = p->right;
      }
      if (w == NULL) {
        x = p;
      } else if ((w->left == NULL || w->left->color == BLACK) &&
                 (w->right == NULL || w->right->color == BLACK)) {
        w->color = RED;
        x = p;
      } else {
        if (w->right == NULL || w->right->color == BLACK) {
          if (w->left != NULL)
            w->left->color = BLACK;
          w->color = RED;
          clint_rb_tree_right_rotate(tree, w);
          w = p->right;
        }
        w->color = p->color;
        p->color = BLACK;
        if (w->right != NULL)
          w->right->color = BLACK;
        clint_rb_tree_left_rotate(tree, p);
        x = (*tree);
      }
    } else {
      w = p->left;
      if (w != NULL && w->color == RED) {
        w->color = BLACK;
        p->color = RED;
        clint_rb_tree_right_rotate(tree, p);
        w = p->left;
      }
      if (w == NULL) {
        x = p;
      } else if ((w->left == NULL || w->left->color == BLACK) &&
                 (w->right == NULL || w->right->color == BLACK)) {
        w->color = RED;
        x = p;
      } else {
        if (w->left == NULL || w->left->color == BLACK) {
          if (w->right != NULL)
            w->right->color = BLACK;
          w->color = RED;
          clint_rb_tree_left_rotate(tree, w);
          w = p->left;
        }
        w->color = p->color;
        p->color = BLACK;
        if (w->left != NULL)
          w->left->color = BLACK;
        clint_rb_tree_right_rotate(tree, p);
        x = (*tree);
      }
    }
    if (x != NULL)
      p = x->parent;
    else
      p = NULL;
  }
  if (x != NULL)
    x->color = BLACK;
}

void clint_rb_tree_delete(ClintRbTreeNode **tree, ClintRbTreeNode *z)
{
  ClintRbTreeNode *x;
  ClintRbTreeNode *y;

  if (z->left == NULL || z->right == NULL) {
    y = z;
  } else {
    y = clint_tree_successor(z);
  }
  if (y->left != NULL) {
    x = y->left;
  } else {
    x = y->right;
  }
  if (x != NULL) {
    x->parent = y->parent;
  }
  if (y->parent == NULL) {
    (*tree) = x;
  } else if (y == y->parent->left) {
    y->parent->left = x;
  } else {
    y->parent->right = x;
  }
  if (y != z) {
    if (y->color == BLACK) {
      clint_rb_tree_delete_fixup(tree, x, y->parent);
    }
    /* Move y into z's position. */
    y->left = z->left;
    y->right = z->right;
    if (y->left != NULL) {
      y->left->parent = y;
    }
    if (y->right != NULL) {
      y->right->parent = y;
    }
    y->parent = z->parent;
    if (z->parent != NULL) {
      if (z->parent->left == z) {
        z->parent->left = y;
      } else {
        z->parent->right = y;
      }
    } else {
      (*tree) = y;
    }
    y->color = z->color;
  } else {
    if (y->color == BLACK) {
      clint_rb_tree_delete_fixup(tree, x, y->parent);
    }
  }
}
