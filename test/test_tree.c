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

#include "clint_tree.h"

#include <assert.h>
#include <string.h>

typedef struct TestTreeElem {
  CLINT_TREE_ELEMS(struct TestTreeElem, int);
} TestTreeElem;

CLINT_DEFINE_TREE_FUNCS(TestTreeElem, int);
CLINT_IMPL_TREE_FUNCS(TestTreeElem, int);

static void clint_rb_tree_test(ClintRbTreeNode *x)
{
  if (x != NULL) {
    if (x->left) {
      assert(x->left->parent == x);
    }
    if (x->right) {
      assert(x->right->parent == x);
    }
    if (x->parent) {
      assert(x->parent->left == x || x->parent->right == x);
    }
    clint_rb_tree_test(x->left);
    clint_rb_tree_test(x->right);
  }
}

#define COUNT 1000

int main(int argc, const char *argv[])
{
  TestTreeElem *tree = NULL;
  TestTreeElem *x;
  const int count = COUNT;
  int order[COUNT];
  int i, j;

  (void)argc;
  (void)argv;
  for (i = 0; i < count; i++) {
    x = (TestTreeElem*)malloc(sizeof(TestTreeElem));
    order[i] = rand() % count;
    clint_tree_insert_TestTreeElem(&tree, order[i], x);
    clint_rb_tree_test(&tree->_node.rb_node);
  }
  for (i = 0; i < count; i++) {
    j = rand() % (count - i);
    x = clint_tree_find_TestTreeElem(tree, order[j]);
    assert(x != NULL);
    clint_tree_erase_TestTreeElem(&tree, x);
    clint_rb_tree_test(&tree->_node.rb_node);
    free(x);
    memmove(order + j, order + j + 1, sizeof(int) * (count - i - j - 1));
  }
}
