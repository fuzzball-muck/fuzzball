#include <check.h>
#include <stdlib.h>
#include <string.h>

/* Import the actual function from src/mcp.c */
extern int mcp_frame_process_input(void *mfr, const char *linein, char *outbuf, int bufsize);

START_TEST(test_buffer_overflow_protection)
{
    /* Invariant: Buffer reads/writes never exceed the declared bufsize */
    const int BUFSIZE = 64;
    const int CANARY_SIZE = 16;
    
    /* Payloads: exploit case (10x), boundary case (2x), valid input */
    char payload_10x[640];
    char payload_2x[128];
    const char *payload_valid = "valid input";
    
    memset(payload_10x, 'A', sizeof(payload_10x) - 1);
    payload_10x[sizeof(payload_10x) - 1] = '\0';
    
    memset(payload_2x, 'B', sizeof(payload_2x) - 1);
    payload_2x[sizeof(payload_2x) - 1] = '\0';
    
    const char *payloads[] = { payload_10x, payload_2x, payload_valid };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /* Allocate buffer with canary bytes after to detect overflow */
        char *buf = malloc(BUFSIZE + CANARY_SIZE);
        ck_assert_ptr_nonnull(buf);
        
        memset(buf, 0, BUFSIZE);
        memset(buf + BUFSIZE, 0xDE, CANARY_SIZE);  /* Canary pattern */
        
        /* Call with NULL mfr - function should handle gracefully or we test output bounds */
        mcp_frame_process_input(NULL, payloads[i], buf, BUFSIZE);
        
        /* Verify canary bytes are untouched - no buffer overflow occurred */
        for (int j = 0; j < CANARY_SIZE; j++) {
            ck_assert_msg((unsigned char)buf[BUFSIZE + j] == 0xDE,
                "Buffer overflow detected at canary byte %d with payload %d", j, i);
        }
        
        free(buf);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_overflow_protection);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}