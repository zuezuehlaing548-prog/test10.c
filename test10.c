/*
 * 과제 10: 정렬 알고리즘의 성능 및 안정성 비교
 * 컴파일: gcc -O2 -o sort_compare sort_compare.c
 * 실행  : ./sort_compare
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N         10000      /* 데이터 개수 */
#define TRIALS    100        /* 무작위 실험 반복 횟수 */
#define KEY_MAX   1000000    /* 키 최댓값 */
#define SEED      12345ULL   /* 난수 시드 (바꾸면 다른 결과) */

/* 데이터 하나: 키 2개 + 원래 입력 순서 번호 */
typedef struct {
    int key1;
    int key2;
    int order;   /* 안정성 검증용 번호 (정렬 기준 아님) */
} Data;

/* 비교 횟수, 이동 횟수 (전역 카운터) */
static long long g_cmp = 0;
static long long g_mov = 0;

/* ---------- 난수 (xorshift64: 어떤 컴퓨터에서도 같은 결과) ---------- */
static unsigned long long rng_state = SEED;

static unsigned long long rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static int rand_range(int max)   /* 0 ~ max */
{
    return (int)(rng_next() % (unsigned long long)(max + 1));
}

/* ---------- 데이터 생성 ---------- */
static void make_random(Data *a, int n, int key_max)
{
    for (int i = 0; i < n; i++) {
        a[i].key1  = rand_range(key_max);
        a[i].key2  = rand_range(KEY_MAX);
        a[i].order = i;
    }
}

/* 시험 데이터 준비용 (비교/이동 횟수에 포함 안 됨) */
static int cmp_for_qsort(const void *x, const void *y)
{
    const Data *p = (const Data *)x;
    const Data *q = (const Data *)y;
    if (p->key1 != q->key1) return (p->key1 > q->key1) - (p->key1 < q->key1);
    return (p->order > q->order) - (p->order < q->order);
}

static void make_sorted(Data *dst, const Data *src, int n)
{
    memcpy(dst, src, sizeof(Data) * n);
    qsort(dst, n, sizeof(Data), cmp_for_qsort);
}

static void make_reverse(Data *dst, const Data *src, int n)
{
    Data *tmp = (Data *)malloc(sizeof(Data) * n);
    make_sorted(tmp, src, n);
    for (int i = 0; i < n; i++) dst[i] = tmp[n - 1 - i];
    free(tmp);
}

/* 일부 정렬: 정렬된 데이터에서 전체의 5%(500쌍)를 무작위로 교환 */
static void make_partial(Data *dst, const Data *src, int n)
{
    make_sorted(dst, src, n);
    int swaps = n / 20;
    for (int s = 0; s < swaps; s++) {
        int i = rand_range(n - 1);
        int j = rand_range(n - 1);
        Data t = dst[i];
        dst[i] = dst[j];
        dst[j] = t;
    }
}

/* ---------- 정렬 알고리즘 ---------- */

/* 데이터 교환 (이동 3회) */
static void swap_data(Data *x, Data *y)
{
    Data temp = *x;  g_mov++;
    *x = *y;         g_mov++;
    *y = temp;       g_mov++;
}

/* [1] 선택 정렬 */
static void selection_sort(Data *a, int n)
{
    for (int i = 0; i < n - 1; i++) {
        int min = i;
        for (int j = i + 1; j < n; j++) {
            g_cmp++;
            if (a[j].key1 < a[min].key1)
                min = j;
        }
        if (min != i)
            swap_data(&a[i], &a[min]);
    }
}

/* [2-1] 버블 정렬 (일반: 모든 순회 수행) */
static void bubble_sort(Data *a, int n)
{
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            g_cmp++;
            if (a[j].key1 > a[j + 1].key1)
                swap_data(&a[j], &a[j + 1]);
        }
    }
}

/* [2-2] 버블 정렬 (조기 종료: 한 순회에서 교환이 없으면 종료) */
static void bubble_sort_early(Data *a, int n)
{
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            g_cmp++;
            if (a[j].key1 > a[j + 1].key1) {
                swap_data(&a[j], &a[j + 1]);
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

/* [3-A] 삽입 정렬 : data[j].key1 >  current.key1 */
static void insertion_sort_gt(Data *a, int n)
{
    for (int i = 1; i < n; i++) {
        Data cur = a[i];  g_mov++;           /* 임시 변수에 저장 */
        int j = i - 1;
        while (j >= 0) {
            g_cmp++;
            if (a[j].key1 > cur.key1) {
                a[j + 1] = a[j];  g_mov++;   /* 한 칸 이동 */
                j--;
            } else {
                break;
            }
        }
        a[j + 1] = cur;  g_mov++;            /* 최종 위치에 대입 */
    }
}

/* [3-B] 삽입 정렬 : data[j].key1 >= current.key1 */
static void insertion_sort_ge(Data *a, int n)
{
    for (int i = 1; i < n; i++) {
        Data cur = a[i];  g_mov++;
        int j = i - 1;
        while (j >= 0) {
            g_cmp++;
            if (a[j].key1 >= cur.key1) {
                a[j + 1] = a[j];  g_mov++;
                j--;
            } else {
                break;
            }
        }
        a[j + 1] = cur;  g_mov++;
    }
}

/* ---------- 검증 함수 ---------- */

/* key1 기준 오름차순인지 확인 */
static int is_sorted(const Data *a, int n)
{
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 > a[i + 1].key1) return 0;
    return 1;
}

/* 안정 정렬인지 확인: key1이 같으면 order가 커져야 함 */
static int is_stable(const Data *a, int n)
{
    for (int i = 0; i + 1 < n; i++)
        if (a[i].key1 == a[i + 1].key1 && a[i].order > a[i + 1].order)
            return 0;
    return 1;
}

/* ---------- 실험 도우미 ---------- */
typedef void (*SortFn)(Data *, int);

typedef struct {
    const char *name;
    SortFn fn;
} Algo;

/* 4가지 방식 (출력 예시 형식에 맞춤, 삽입 정렬은 Case A 사용) */
static Algo algos[4] = {
    { "Selection Sort",                selection_sort    },
    { "Bubble Sort",                   bubble_sort       },
    { "Bubble Sort (Early Termination)", bubble_sort_early },
    { "Insertion Sort",                insertion_sort_gt }
};

/* 정렬 1번 실행: 사본을 만들어 정렬하고 횟수 기록 */
static int run_once(SortFn fn, const Data *orig, int n,
                    long long *cmp, long long *mov)
{
    Data *work = (Data *)malloc(sizeof(Data) * n);
    memcpy(work, orig, sizeof(Data) * n);   /* 복사는 횟수에 포함 안 함 */

    g_cmp = 0;
    g_mov = 0;
    fn(work, n);
    *cmp = g_cmp;
    *mov = g_mov;

    int ok = is_sorted(work, n);
    free(work);
    return ok;
}

/* 안정성 검증: 같은 key1이 많은 시험 데이터 사용 */
static void stability_test(const char *name, SortFn fn, const Data *orig, int n)
{
    Data *work = (Data *)malloc(sizeof(Data) * n);
    memcpy(work, orig, sizeof(Data) * n);
    fn(work, n);

    int sorted = is_sorted(work, n);
    int stable = is_stable(work, n);
    printf("%-21s : %s (정렬 %s)\n", name,
           stable ? "Stable" : "Unstable",
           sorted ? "OK" : "FAIL");
    free(work);
}

/* 작은 예시 데이터로 정렬 전후 순서를 보여줌 (README 설명용) */
static void small_example(const char *name, SortFn fn, const Data *orig, int n)
{
    Data *work = (Data *)malloc(sizeof(Data) * n);
    memcpy(work, orig, sizeof(Data) * n);
    fn(work, n);
    printf("  %-20s: ", name);
    for (int i = 0; i < n; i++)
        printf("(%d,#%d) ", work[i].key1, work[i].order);
    printf("\n");
    free(work);
}

/* 입력 상태 1개에 대해 4가지 정렬 실행 후 출력 */
static void test_state(const char *title, const Data *orig, int n)
{
    printf("%s\n\n", title);
    for (int k = 0; k < 4; k++) {
        long long c, m;
        int ok = run_once(algos[k].fn, orig, n, &c, &m);
        printf("%s\n", algos[k].name);
        printf("Comparisons : %lld\n", c);
        printf("Movements   : %lld\n", m);
        printf("Sorted Check: %s\n\n", ok ? "OK" : "FAIL");
    }
    printf("------------------------------------\n\n");
}

int main(void)
{
    Data *base = (Data *)malloc(sizeof(Data) * N);
    Data *tmp  = (Data *)malloc(sizeof(Data) * N);

    printf("Data Count : %d\n\n", N);

    /* ===== 1. 무작위 데이터 100회 반복 ===== */
    long long total_cmp[4] = {0}, total_mov[4] = {0};
    int all_ok = 1;

    for (int t = 0; t < TRIALS; t++) {
        make_random(base, N, KEY_MAX);        /* 반복마다 새 데이터 */
        for (int k = 0; k < 4; k++) {         /* 같은 데이터를 모두에게 제공 */
            long long c, m;
            if (!run_once(algos[k].fn, base, N, &c, &m)) all_ok = 0;
            total_cmp[k] += c;
            total_mov[k] += m;
        }
    }

    printf("Random Data\n");
    printf("Number of Trials : %d\n", TRIALS);
    printf("All Results Sorted : %s\n\n", all_ok ? "OK" : "FAIL");
    for (int k = 0; k < 4; k++) {
        printf("%s\n", algos[k].name);
        printf("Total Comparisons   : %lld\n", total_cmp[k]);
        printf("Average Comparisons : %.2f\n", (double)total_cmp[k] / TRIALS);
        printf("Average Movements   : %.2f\n\n", (double)total_mov[k] / TRIALS);
    }
    printf("------------------------------------\n\n");

    /* ===== 2. 입력 상태별 실험 (같은 원본에서 생성) ===== */
    make_random(base, N, KEY_MAX);

    make_sorted(tmp, base, N);
    test_state("Sorted Data", tmp, N);

    make_reverse(tmp, base, N);
    test_state("Reverse-Sorted Data", tmp, N);

    make_partial(tmp, base, N);
    test_state("Partially Sorted Data", tmp, N);

    /* ===== 3. 안정성 검증 ===== */
    /* 큰 시험 데이터: key1을 0~99로 제한해서 중복이 많이 생김 */
    make_random(base, N, 99);

    printf("Stability Verification\n");
    printf("(test data: %d items, key1 in 0~99, many duplicates)\n\n", N);
    stability_test("Selection Sort",      selection_sort,    base, N);
    stability_test("Bubble Sort",         bubble_sort,       base, N);
    stability_test("Bubble Sort (Early)", bubble_sort_early, base, N);
    stability_test("Insertion Sort (>)",  insertion_sort_gt, base, N);
    stability_test("Insertion Sort (>=)", insertion_sort_ge, base, N);

    /* 작은 예시 데이터 (README에 쓰기 좋음) */
    Data ex[8] = {
        {5, 10, 0}, {3, 20, 1}, {5, 30, 2}, {3, 40, 3},
        {1, 50, 4}, {5, 60, 5}, {3, 70, 6}, {1, 80, 7}
    };
    printf("\nSmall Example (key1, #original order)\n");
    printf("  Original            : ");
    for (int i = 0; i < 8; i++) printf("(%d,#%d) ", ex[i].key1, ex[i].order);
    printf("\n");
    small_example("Selection Sort",     selection_sort,    ex, 8);
    small_example("Bubble Sort",        bubble_sort,       ex, 8);
    small_example("Insertion Sort (>)",  insertion_sort_gt, ex, 8);
    small_example("Insertion Sort (>=)", insertion_sort_ge, ex, 8);

    free(base);
    free(tmp);
    return 0;
}
