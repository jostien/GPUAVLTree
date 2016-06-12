/* GPUAVLTree, Copyright (c) 2016 Jost Neigenfind <jostie@gmx.de>
 * AVL-Tree implementation - non-recursive implementation for GPUs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

int height(int p, __local int *heights)
{
    return p > -1 ? heights[p] : 0;
}

int bfactor(int p, __local int *lefts, __local int *rights, __local int *heights)
{
    return height(rights[p], heights)-height(lefts[p], heights);
}

void fixheight(int p, __local int *lefts, __local int *rights, __local int *heights)
{
    int hl = height(lefts[p], heights);
    int hr = height(rights[p], heights);
    heights[p] = (hl > hr ? hl : hr) + 1;
}

int rotateright(int p, __local int *lefts, __local int *rights, __local int *heights)
{
    int q = lefts[p];
    lefts[p] = rights[q];
    rights[q] = p;
    fixheight(p, lefts, rights, heights);
    fixheight(q, lefts, rights, heights);
    return q;
}

int rotateleft(int q, __local int *lefts, __local int *rights, __local int *heights)
{
    int p = rights[q];
    rights[q] = lefts[p];
    lefts[p] = q;
    fixheight(q, lefts, rights, heights);
    fixheight(p, lefts, rights, heights);
    return p;
}

int balance(int p, __local int *lefts, __local int *rights, __local int *heights) // balancing the p node
{
    fixheight(p, lefts, rights, heights);
    if(bfactor(p, lefts, rights, heights) == 2)
    {
        if(bfactor(rights[p], lefts, rights, heights) < 0)
            rights[p] = rotateright(rights[p], lefts, rights, heights);
        return rotateleft(p, lefts, rights, heights);
    }
    if(bfactor(p, lefts, rights, heights) == -2)
    {
        if(bfactor(lefts[p], lefts, rights, heights) > 0)
            lefts[p] = rotateleft(lefts[p], lefts, rights, heights);
        return rotateright(p, lefts, rights, heights);
    }
    return p; // balancing is not required
}

int contains(int p, int k, __local int *keys, __local int *lefts, __local int *rights)
{
    while (p > -1){
        if (k == keys[p])
            return true;
        else if (k < keys[p])
            p = lefts[p];
        else
            p = rights[p];
    }

    return false;
}

int insert(int p, int k, __local int *size, int n, __local int *keys, __local int *lefts, __local int *rights, __local int *heights, __local int *frees, __local int *used, __local int *p_stack, __local int *lr_stack) // insert k key in a tree with p root
{
    int root = p;

    int stack_size = 0;
    while (p > -1){
        if (k == keys[p])
            return root;

        p_stack[stack_size] = p;
        if (k < keys[p]){
            p = lefts[p];
            lr_stack[stack_size] = 0;
        } else {
            p = rights[p];
            lr_stack[stack_size] = 1;
        }
        stack_size++;
    }

    int index = frees[n - size[0] - 1];

    // save index in lookup table
    used[index] = n - size[0] - 1;

    keys[index] = k;
    lefts[index] = -1;
    rights[index] = -1;
    heights[index] = 1;

    size[0]++;

    int ret = index;
    for (int i = stack_size - 1; i >= 0; i--){
        if (lr_stack[i] == 0)
            lefts[p_stack[i]] = ret;
        else
            rights[p_stack[i]] = ret;
        ret = balance(p_stack[i], lefts, rights, heights);
    }
    return ret;
}

int findmin(int p, __local int *lefts) // find a node with minimal key in a p tree
{
    while (lefts[p] > -1)
        p = lefts[p];
    return p;
}

int removemin(int p, __local int *lefts, __local int *rights, __local int *heights, __local int *p_stack) // deleting a node with minimal key from a p tree
{
    int stack_size = 0;
    while (lefts[p] > -1){
        p_stack[stack_size] = p;
        p = lefts[p];
        stack_size++;
    }

    if (stack_size == 0)
        return rights[p];

    int ret = -1;
    for (int i = stack_size - 1; i >= 0; i--){
        lefts[p_stack[i]] = (i == stack_size - 1 ? rights[p] : ret);
        ret = balance(p_stack[i], lefts, rights, heights);
    }
    return ret;
}

int remove(int p, int k, __local int *size, int n, __local int *keys, __local int *lefts, __local int *rights, __local int *heights, __local int *frees, __local int *used, __local int *p_stack, __local int *lr_stack, __local int *p_stack2) // deleting k key from p tree
{
    int root = p;

    int stack_size = 0;
    while (k != keys[p]){
        if (p < 0)
            return root;

        p_stack[stack_size] = p;
        if (k < keys[p]){
            p = lefts[p];
            lr_stack[stack_size] = 0;
        } else {
            p = rights[p];
            lr_stack[stack_size] = 1;
        }
        stack_size++;
    }

    int q = lefts[p];
    int r = rights[p];

    ////////////////////////////////////////////
    // get index of freed field
    int index_right = used[p];
    // get index of field to copy to freed field
    int index_left = n - size[0];
    int svd = frees[index_left];
    frees[index_left] = frees[index_right];
    frees[index_right] = svd;

    // update lookup table
    used[svd] = index_right;
    ////////////////////////////////////////////

    //delete p;
    keys[p] = -1;
    lefts[p] = -1;
    rights[p] = -1;
    heights[p] = -1;
    frees[n - size[0]] = p;

    size[0]--;

    int ret = q;
    if (r > -1){
        int min = findmin(r, lefts);
        rights[min] = removemin(r, lefts, rights, heights, p_stack2);
        lefts[min] = q;
        ret = balance(min, lefts, rights, heights);
    }
    for (int i = stack_size - 1; i >= 0; i--){
        if (lr_stack[i] == 0)
            lefts[p_stack[i]] = ret;
        else
            rights[p_stack[i]] = ret;
        ret = balance(p_stack[i], lefts, rights, heights);
    }
    return ret;
}

void clear(__local int *array, int n){
    for (int i = 0; i < n; i++)
        array[i] = -1;
}
