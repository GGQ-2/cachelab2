

/*
请注意，你的代码不能出现任何 int/short/char/float/double/auto 等局部变量/函数传参，我们仅允许使用 reg 定义的寄存器变量。
其中 reg 等价于一个 int。

你不能自己申请额外的内存，即不能使用 new/malloc，作为补偿我们传入了一段 buffer，大小为 BUFFER_SIZE = 64，你可以视情况使用。

我们的数组按照 A, B, C, buffer 的顺序在内存上连续紧密排列，且 &A = 0x30000000（这是模拟的设定，不是 A 的真实地址）

如果你需要以更自由的方式访问内存，你可以以相对 A 的方式访问，比如 A[100]，用 *(0x30000000) 是无法访问到的。

如果你有定义常量的需求（更严谨的说法是，你想定义的是汇编层面的立即数，不应该占用寄存器），请参考下面这种方式使用宏定义来完成。
*/

#include "cachelab.h"

#define m case0_m
#define n case0_n
#define p case0_p

// 我们用这个 2*2*2 的矩阵乘法来演示寄存器是怎么被分配的
void gemm_case0(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer) {  // allocate 0 1 2 3
    for (reg i = 0; i < m; ++i) {                                   // allocate 4
        for (reg j = 0; j < p; ++j) {                               // allocate 5
            reg tmpc = 0;                                           // allocate 6
            for (reg k = 0; k < n; ++k) {                           // allocate 7
                reg tmpa = A[i * n + k];                            // allocate 8
                reg tmpb = B[k * p + j];                            // allocate 9
                tmpc += tmpa * tmpb;
            }  // free 9 8
            // free 7
            C[i * p + j] = tmpc;
        }  // free 6
        // free 5
    }
    // free 4
}  // free 3 2 1 0

#undef m
#undef n
#undef p

#define TILE_SIZE_M 8
#define TILE_SIZE_N 8
#define TILE_SIZE_P 8
#define PREFETCH_DISTANCE 8
#define m case1_m
#define n case1_n
#define p case1_p

void gemm_case1(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer) {
    for (reg bi = 0; bi < m; bi += TILE_SIZE_M) {
        for (reg bj = 0; bj < p; bj += TILE_SIZE_N) {
            for (reg bk = 0; bk < n; bk += TILE_SIZE_P) {
                for (reg ii = 0; ii < TILE_SIZE_M && bi + ii < m; ++ii) {
                    reg temp_row[TILE_SIZE_N] = {0};
                    for (reg kk = 0; kk < TILE_SIZE_P && bk + kk < n; kk += 2) {
                        if (kk + PREFETCH_DISTANCE < TILE_SIZE_P && bk + kk + PREFETCH_DISTANCE < n) {
                            reg _ = A[(bi + ii) * n + (bk + kk + PREFETCH_DISTANCE)];
                            _ = B[(bk + kk + PREFETCH_DISTANCE) * p + bj];
                        }
                        reg tmpa1 = A[(bi + ii) * n + (bk + kk)];
                        reg tmpa2 = 0;
                        if (kk + 1 < TILE_SIZE_P && bk + kk + 1 < n) {
                            tmpa2 = A[(bi + ii) * n + (bk + kk + 1)];
                        }
                        for (reg jj = 0; jj < TILE_SIZE_N && bj + jj < p; ++jj) {
                            reg tmpb1 = B[(bk + kk) * p + (bj + jj)];
                            reg tmpb2 = 0;
                            if (kk + 1 < TILE_SIZE_P && bk + kk + 1 < n) {
                                tmpb2 = B[(bk + kk + 1) * p + (bj + jj)];
                            }
                            temp_row[jj] += tmpa1 * tmpb1 + tmpa2 * tmpb2;
                        }
                    }
                    for (reg jj = 0; jj < TILE_SIZE_N && bj + jj < p; ++jj) {
                        reg tmpc = C[(bi + ii) * p + (bj + jj)];
                        tmpc += temp_row[jj];
                        C[(bi + ii) * p + (bj + jj)] = tmpc;
                    }
                }
            }
        }
    }
}

#undef m
#undef n
#undef p
#undef TILE_SIZE_M
#undef TILE_SIZE_N
#undef TILE_SIZE_P
#undef PREFETCH_DISTANCE

#define TILE_SIZE_M 16
#define TILE_SIZE_N 16
#define TILE_SIZE_P 16
#define PREFETCH_DISTANCE 16
#define m case2_m
#define n case2_n
#define p case2_p

void gemm_case2(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer) {
    for (reg bi = 0; bi < m; bi += TILE_SIZE_M) {
        for (reg bj = 0; bj < p; bj += TILE_SIZE_N) {
            for (reg bk = 0; bk < n; bk += TILE_SIZE_P) {
                for (reg ii = 0; ii < TILE_SIZE_M && bi + ii < m; ++ii) {
                    reg temp_row[TILE_SIZE_N] = {0};
                    for (reg kk = 0; kk < TILE_SIZE_P && bk + kk < n; ++kk) {
                        reg tmpa = A[(bi + ii) * n + (bk + kk)];
                        for (reg jj = 0; jj < TILE_SIZE_N && bj + jj < p; ++jj) {
                            reg tmpb = B[(bk + kk) * p + (bj + jj)];
                            temp_row[jj] += tmpa * tmpb;
                        }
                    }
                    for (reg jj = 0; jj < TILE_SIZE_N && bj + jj < p; ++jj) {
                        reg tmpc1 = C[(bi + ii) * p + (bj + jj)];
                        tmpc1 += temp_row[jj];
                        C[(bi + ii) * p + (bj + jj)] = tmpc1;
                    }
                }
            }
        }
    }
}

#undef m
#undef n
#undef p
#undef TILE_SIZE_M
#undef TILE_SIZE_N
#undef TILE_SIZE_P
#undef PREFETCH_DISTANCE

#define TILE_SIZE_M 16
#define TILE_SIZE_N 16
#define TILE_SIZE_P 16
#define PREFETCH_DISTANCE 16
#define m case3_m
#define n case3_n
#define p case3_p

void gemm_case3(ptr_reg A, ptr_reg B, ptr_reg C, ptr_reg buffer) {
    for (reg bi = 0; bi < m; bi += TILE_SIZE_M) {
        for (reg bj = 0; bj < p; bj += TILE_SIZE_N) {
            for (reg bk = 0; bk < n; bk += TILE_SIZE_P) {
                for (reg ii = 0; ii < TILE_SIZE_M && bi + ii < m; ++ii) {
                    reg temp_row[TILE_SIZE_N] = {0};
                    for (reg kk = 0; kk < TILE_SIZE_P && bk + kk < n; kk += 2) {
                        if (kk + PREFETCH_DISTANCE < TILE_SIZE_P && bk + kk + PREFETCH_DISTANCE < n) {
                            reg _ = A[(bi + ii) * n + (bk + kk + PREFETCH_DISTANCE)];
                            _ = B[(bk + kk + PREFETCH_DISTANCE) * p + bj];
                        }
                        reg tmpa1 = A[(bi + ii) * n + (bk + kk)];
                        reg tmpa2 = 0;
                        if (kk + 1 < TILE_SIZE_P && bk + kk + 1 < n) {
                            tmpa2 = A[(bi + ii) * n + (bk + kk + 1)];
                        }
                        for (reg jj = 0; jj < TILE_SIZE_N && bj + jj < p; ++jj) {
                            reg tmpb1 = B[(bk + kk) * p + (bj + jj)];
                            reg tmpb2 = 0;
                            if (kk + 1 < TILE_SIZE_P && bk + kk + 1 < n) {
                                tmpb2 = B[(bk + kk + 1) * p + (bj + jj)];
                            }
                            temp_row[jj] += tmpa1 * tmpb1 + tmpa2 * tmpb2;
                        }
                    }
                    for (reg jj = 0; jj < TILE_SIZE_N && bj + jj < p; ++jj) {
                        reg tmpc = C[(bi + ii) * p + (bj + jj)];
                        tmpc += temp_row[jj];
                        C[(bi + ii) * p + (bj + jj)] = tmpc;
                    }
                }
            }
        }
    }
}

#undef m
#undef n
#undef p
#undef TILE_SIZE_M
#undef TILE_SIZE_N
#undef TILE_SIZE_P
#undef PREFETCH_DISTANCE
