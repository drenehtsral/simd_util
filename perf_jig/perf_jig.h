#ifndef _PERF_JIG_H_
#define _PERF_JIG_H_

#define MAX_PERF_FUNC_ARGS  (18)

static inline int example_perf_test_function(const char **args)
{
    return 0;
}

typedef struct perf_func_entry {
    typeof(&example_perf_test_function) func;
    const char *desc;
    const char *arg_names[MAX_PERF_FUNC_ARGS];
} perf_func_entry_t;

#define PERF_FUNC_ENTRY(fname, text, rest...)                                                   \
    static const __attribute__((__section__("test_desc_section"), __used__)) perf_func_entry_t  \
    desc_perf_test_##fname  = {                                                                 \
        .func = &perf_test_##fname, .desc = text, .arg_names = {#fname, ##rest}                 \
    } /* end of macro */

#define ARG_VALID(_a) ({ const char * __a = (_a); (__a && __a[0]); })

extern const perf_func_entry_t __start_test_desc_section;
extern const perf_func_entry_t __stop_test_desc_section;

#endif /* _PERF_JIG_H_ */
