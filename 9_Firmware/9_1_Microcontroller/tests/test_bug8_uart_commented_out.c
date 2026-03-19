/*******************************************************************************
 * test_bug8_uart_commented_out.c
 *
 * Bug #8: Debug helpers uart_print() and uart_println() in main.cpp
 * (lines 958-970) are commented out with block comments.
 *
 * Test strategy:
 *   Read the source file and verify the functions are inside comment blocks.
 *   This is a static analysis / string-search test — no compilation of
 *   the source under test is needed.
 ******************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Path to the source file (relative from test execution dir) */
#define SOURCE_FILE "../9_1_3_C_Cpp_Code/main.cpp"

/* Helper: read entire file into malloc'd buffer. Returns NULL on failure. */
static char *read_file(const char *path, long *out_size)
{
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }
    long nread = (long)fread(buf, 1, size, f);
    buf[nread] = '\0';
    fclose(f);
    if (out_size) *out_size = nread;
    return buf;
}

int main(void)
{
    printf("=== Bug #8: uart_print/uart_println commented out ===\n");

    long size;
    char *src = read_file(SOURCE_FILE, &size);
    if (!src) {
        printf("  Could not open %s — trying alternate path...\n", SOURCE_FILE);
        /* Try from the tests/ directory */
        src = read_file("../9_1_3_C_Cpp_Code/main.cpp", &size);
    }
    if (!src) {
        /* Try absolute path */
        src = read_file("/Users/ganeshpanth/PLFM_RADAR/9_Firmware/9_1_Microcontroller/9_1_3_C_Cpp_Code/main.cpp", &size);
    }
    assert(src != NULL && "Could not open main.cpp");
    printf("  Read %ld bytes from main.cpp\n", size);

    /* ---- Test A: Find "static void uart_print" — should be inside a comment ---- */
    const char *uart_print_sig = "static void uart_print(const char *msg)";
    char *pos = strstr(src, uart_print_sig);
    assert(pos != NULL && "uart_print function signature not found in source");
    printf("  Found uart_print signature at offset %ld\n", (long)(pos - src));

    /* Walk backwards from pos to find if we're inside a block comment */
    /* Look for the nearest preceding block-comment open '/ *' that isn't closed */
    int inside_comment = 0;
    for (char *p = src; p < pos; p++) {
        if (p[0] == '/' && p[1] == '*') {
            inside_comment = 1;
            p++;  /* skip past '*' */
        } else if (p[0] == '*' && p[1] == '/') {
            inside_comment = 0;
            p++;  /* skip past '/' */
        }
    }
    printf("  uart_print is inside block comment: %s\n",
           inside_comment ? "YES" : "NO");
    assert(inside_comment == 1);
    printf("  PASS: uart_print is commented out\n");

    /* ---- Test B: Find "static void uart_println" — also in comment ---- */
    const char *uart_println_sig = "static void uart_println(const char *msg)";
    pos = strstr(src, uart_println_sig);
    assert(pos != NULL && "uart_println function signature not found in source");
    printf("  Found uart_println signature at offset %ld\n", (long)(pos - src));

    inside_comment = 0;
    for (char *p = src; p < pos; p++) {
        if (p[0] == '/' && p[1] == '*') {
            inside_comment = 1;
            p++;
        } else if (p[0] == '*' && p[1] == '/') {
            inside_comment = 0;
            p++;
        }
    }
    printf("  uart_println is inside block comment: %s\n",
           inside_comment ? "YES" : "NO");
    assert(inside_comment == 1);
    printf("  PASS: uart_println is commented out\n");

    /* ---- Test C: Verify the comment pattern matches lines 957-970 ---- */
    /* Find the opening '/ *' that contains uart_print */
    char *comment_start = NULL;
    char *comment_end = NULL;
    int in_cmt = 0;
    for (char *p = src; p < src + size - 1; p++) {
        if (p[0] == '/' && p[1] == '*') {
            if (!in_cmt) {
                in_cmt = 1;
                comment_start = p;
            }
            p++;
        } else if (p[0] == '*' && p[1] == '/') {
            if (in_cmt) {
                comment_end = p + 2;
                /* Check if uart_print is within this comment */
                char *found = strstr(comment_start, uart_print_sig);
                if (found && found < comment_end) {
                    /* Count lines from start to comment_start */
                    int line = 1;
                    for (char *lp = src; lp < comment_start; lp++) {
                        if (*lp == '\n') line++;
                    }
                    printf("  Comment block containing uart_print starts at line %d\n", line);
                    /* Verify it's approximately around line 958 */
                    assert(line >= 955 && line <= 965);
                    printf("  PASS: Comment block is at expected location (~line 958)\n");
                    break;
                }
            }
            in_cmt = 0;
            p++;
        }
    }

    free(src);
    printf("=== Bug #8: ALL TESTS PASSED ===\n\n");
    return 0;
}
