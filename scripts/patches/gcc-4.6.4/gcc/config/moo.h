#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS() \
	do { \
		builtin_define_std ("moo"); \
		builtin_define_std ("unix"); \
		builtin_assert ("system=moo"); \
		builtin_assert ("system=unix"); \
	} while (0);

#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt0.o%s"

#undef ENDFILE_SPEC
#define ENDFILE_SPEC ""

#undef  NO_IMPLICIT_EXTERN_C
#define NO_IMPLICIT_EXTERN_C 1
