GPUAVLTree - Imperative AVL-Tree implementation for GPUs
--------------------------------------------------------

Stuff in code.c can be put into kernel.cl. More information about recursive AVL-Tree
implementation, see

http://kukuruku.co/hub/cpp/avl-trees

Initialization
--------------

Something like

    __local int keys[512];
    __local int lefts[512];
    __local int rights[512];
    __local int heights[512];
    __local int frees[512];
    __local int used[512];
    __local int size[1];
    __local int root[1];

    // worst case height of avl-tree is 1.44*log2(n + 2) - 0.328, e.g., 1.44*log2(512 + 2) - 0.328 = 12.64;
    __local int p_stack[13];
    __local int lr_stack[13];
    __local int p_stack2[13];

    clear(keys, 512);
    clear(lefts, 512);
    clear(rights, 512);
    clear(heights, 512);

    for (int i = 0; i < 512; i++)
        frees[i] = i;

    size[0] = 0;
    root[0] = -1;
