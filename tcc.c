/*
 *  TCC - Tiny C Compiler (Simplified Main with Working -run)
 * 
 *  Optimized for clarity, C90 compliance, and correct execution logic.
 */

#ifndef ONE_SOURCE
# define ONE_SOURCE 1
#endif

#include "tcc.h"

#if ONE_SOURCE
# include "libtcc.c"
#endif
#include "tcctools.c"

/* --- Simplified Help & Version Strings --- */

static const char help_msg[] =
    "Usage: tcc [options] [-o outfile] [-c] infile(s)...\n"
    "       tcc [options] -run infile [args...]\n"
    "\nGeneral:\n"
    "  -c           Compile only (generate .o)\n"
    "  -o outfile   Set output filename\n"
    "  -run         Compile and run immediately\n"
    "  -v           Show version\n"
    "  -vv          Show search paths\n"
    "  -h           Show this help\n"
    "  -w           Disable warnings\n"
    "\nPreprocessor:\n"
    "  -Idir        Add include path\n"
    "  -Dsym[=val]  Define symbol\n"
    "  -E           Preprocess only\n"
    "\nLinker:\n"
    "  -Ldir        Add library path\n"
    "  -llib        Link library\n"
    "  -shared      Generate shared library\n"
    ;

static const char version_msg[] =
    "tcc version " TCC_VERSION 
#ifdef TCC_GITHASH
    " (" TCC_GITHASH ")"
#endif
    "\n";

/* --- Helper Functions --- */

static void print_search_dirs(TCCState *s)
{
    int i;
    printf("install: %s\n", s->tcc_lib_path);
    
    printf("include:\n");
    if (!s->nb_sysinclude_paths) printf("  -\n");
    for(i = 0; i < s->nb_sysinclude_paths; i++)
        printf("  %s\n", s->sysinclude_paths[i]);

    printf("libraries:\n");
    if (!s->nb_library_paths) printf("  -\n");
    for(i = 0; i < s->nb_library_paths; i++)
        printf("  %s\n", s->library_paths[i]);
        
#ifdef TCC_TARGET_UNIX
    printf("crt:\n");
    if (!s->nb_crt_paths) printf("  -\n");
    for(i = 0; i < s->nb_crt_paths; i++)
        printf("  %s\n", s->crt_paths[i]);
#endif
}

static void apply_env_paths(TCCState *s)
{
    char *path;
    if ((path = getenv("C_INCLUDE_PATH"))) tcc_add_sysinclude_path(s, path);
    if ((path = getenv("CPATH")))          tcc_add_include_path(s, path);
    if ((path = getenv("LIBRARY_PATH")))   tcc_add_library_path(s, path);
}

static const char* get_default_ext(TCCState *s)
{
    if (s->output_type == TCC_OUTPUT_DLL) return ".dll";
    if (s->output_type == TCC_OUTPUT_OBJ) return ".o";
    return ".out"; // Default for executable
}

static char* generate_output_name(TCCState *s, const char *input_file)
{
    char buf[1024];
    const char *base;
    char *ext;

    if (!input_file || !strcmp(input_file, "-"))
        base = "a";
    else {
        base = tcc_basename(input_file);
        /* Truncate if too long */
        if (strlen(base) > sizeof(buf) - 10)
            base = "a";
    }

    snprintf(buf, sizeof(buf), "%s", base);
    ext = tcc_fileextension(buf);
    
    if (*ext) {
        /* Replace existing extension */
        strcpy(ext, get_default_ext(s));
    } else {
        /* Append extension */
        strcat(buf, get_default_ext(s));
    }
    
    return tcc_strdup(buf);
}

/* --- Main Entry Point --- */

int main(int argc, char **argv)
{
    /* C90: All declarations at the top */
    TCCState *s;
    TCCState *s1; /* Required by TCC internal macros */
    int ret = 0;
    int i;
    int do_run = 0;
    int do_preprocess_only = 0;
    int do_compile_only = 0;
    const char *outfile = NULL;
    const char *first_input = NULL;
    int new_argc;
    int run_arg_start = 0; /* Index in original argv where program args start */
    
    /* 1. Initialize State */
    s = s1 = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create TCC state\n");
        return 1;
    }

    /* 2. Manual High-Level Argument Parsing */
    /* 
       Strategy:
       1. Identify high-level flags (-run, -c, -E, -o).
       2. Stop flag processing when we hit the first non-flag argument (source file).
       3. Everything after the source file is considered arguments for the executed program.
    */
    
    run_arg_start = argc; /* Default: no program args */
    
    for (i = 1; i < argc; i++) {
        char *arg = argv[i];
        
        /* If we hit a non-flag argument, it's likely the source file.
           Stop processing TCC flags. Rest are program args. */
        if (arg[0] != '-' || arg[1] == '\0') {
            run_arg_start = i;
            break;
        }

        if (!strcmp(arg, "-run")) {
            do_run = 1;
            argv[i] = NULL; /* Remove from TCC parsing */
        } else if (!strcmp(arg, "-c")) {
            do_compile_only = 1;
            argv[i] = NULL;
        } else if (!strcmp(arg, "-E")) {
            do_preprocess_only = 1;
            argv[i] = NULL;
        } else if (!strcmp(arg, "-o") && i + 1 < argc) {
            outfile = argv[i+1];
            argv[i] = NULL;     /* Remove flag */
            argv[i+1] = NULL;   /* Remove value */
            i++;
        } else if (!strcmp(arg, "-v")) {
            printf("%s", version_msg);
        } else if (!strcmp(arg, "-vv")) {
            apply_env_paths(s);
            tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
            print_search_dirs(s);
            tcc_delete(s);
            return 0;
        } else if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            fputs(help_msg, stdout);
            tcc_delete(s);
            return 0;
        }
        /* Other flags (-I, -D, -L, etc.) are left in argv for tcc_parse_args */
    }

    /* Compact argv: remove NULL entries (handled flags) */
    new_argc = 1; /* Keep program name */
    for (i = 1; i < argc; i++) {
        if (argv[i] != NULL) {
            argv[new_argc++] = argv[i];
        }
    }
    argv[new_argc] = NULL;

    /* 3. Determine Output Type based on modes */
    if (do_run) {
        s->output_type = TCC_OUTPUT_MEMORY;
    } else if (do_preprocess_only) {
        s->output_type = TCC_OUTPUT_PREPROCESS;
    } else if (do_compile_only) {
        s->output_type = TCC_OUTPUT_OBJ;
    } else {
        s->output_type = TCC_OUTPUT_EXE;
    }

    /* 4. Apply Environment Variables */
    apply_env_paths(s);

    /* 5. Parse Remaining Arguments via TCC Internal Parser */
    {
        int dummy_opt;
        /* tcc_parse_args expects pointer to argc and argv */
        tcc_parse_args(s, &new_argc, &argv);
    }

    /* 6. Validation */
    if (s->nb_files == 0) {
        tcc_error_noabort("no input files");
        tcc_delete(s);
        return 1;
    }

    if (do_compile_only && s->nb_files > 1 && outfile) {
        tcc_error_noabort("cannot specify output file with -c for multiple files");
        tcc_delete(s);
        return 1;
    }
    
    if (do_run && s->nb_files > 1) {
         /* TCC can technically run multiple files if they link together, 
            but typically -run is for a single entry point. 
            We allow it, but note that argv passed to program will be tricky. 
            Standard practice: -run usually takes one main file. */
    }

    /* 7. Set Final Output Type & Config */
    tcc_set_output_type(s, s->output_type);

    /* 8. Process Files */
    for (i = 0; i < s->nb_files; i++) {
        struct filespec *f = s->files[i];
        
        if (!first_input) first_input = f->name;

        if (f->type & AFF_TYPE_LIB) {
            ret = tcc_add_library(s, f->name);
        } else {
            if (s->verbose) printf("-> %s\n", f->name);
            ret = tcc_add_file(s, f->name);
        }

        if (ret) break;
    }

    /* 9. Final Output Generation / Execution */
    if (!ret) {
        if (do_run) {
#ifdef TCC_IS_NATIVE
            /* 
               Prepare argv for the executed program.
               It should be: [program_name] [args...]
               program_name is usually the source file name or "a.out".
               args are everything after the source file in the original command line.
            */
            int run_argc = 0;
            char **run_argv = NULL;
            
            /* Construct run_argv */
            run_argc = (argc - run_arg_start) + 1; /* +1 for program name */
            run_argv = (char **)tcc_malloc(run_argc * sizeof(char *));
            
            /* Program name: use outfile if specified, else source name */
            if (outfile) {
                run_argv[0] = (char *)outfile;
            } else {
                run_argv[0] = (char *)first_input;
            }
            
            /* Copy remaining args */
            for (i = 0; i < argc - run_arg_start; i++) {
                run_argv[i+1] = argv[run_arg_start + i];
            }
            
            ret = tcc_run(s, run_argc, run_argv);
            tcc_free(run_argv);
#else
            tcc_error_noabort("-run is not supported on this target");
            ret = 1;
#endif
        } else if (do_compile_only) {
             /* Handle -c output */
             if (s->nb_files == 1) {
                 const char *obj_out = outfile;
                 char *generated = NULL;
                 if (!obj_out) {
                     generated = generate_output_name(s, first_input);
                     obj_out = generated;
                 }
                 ret = tcc_output_file(s, obj_out);
                 if (generated) tcc_free(generated);
             } else {
                 /* Multiple files with -c: TCC doesn't natively support 
                    outputting multiple .o files in one pass easily without 
                    resetting state. We error out for simplicity. */
                 tcc_error_noabort("multiple input files with -c require separate invocations");
                 ret = 1;
             }
        } else {
            /* Standard linking to executable/library */
            const char *final_outfile = outfile;
            char *generated_outfile = NULL;
            
            if (!final_outfile) {
                generated_outfile = generate_output_name(s, first_input);
                final_outfile = generated_outfile;
            }
            
            if (s->output_type != TCC_OUTPUT_PREPROCESS) {
                ret = tcc_output_file(s, final_outfile);
            }
            
            if (generated_outfile) {
                tcc_free(generated_outfile);
            }
        }
    }

    /* 10. Cleanup */
    tcc_delete(s);
    return ret ? 1 : 0;
}