/*
 * SYSCALL_DEFINE3(execve,
 *                const char __user *, filename,
 *                const char __user *const __user *, argv,
 *                const char __user *const __user *, envp)
 *
 * On success, execve() does not return
 * on error -1 is returned, and errno is set appropriately.
 *
 * TODO: Redirect stdin/stdout.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "arch.h"	// page_size
#include "random.h"	// generate_random_page
#include "sanitise.h"
#include "shm.h"
#include "syscall.h"
#include "trinity.h"	// __unused__
#include "utils.h"

static unsigned long ** gen_ptrs_to_crap(void)
{
	void **ptr;
	unsigned int i;
	unsigned int count = rand() % 32;

	/* Fabricate argv */
	ptr = zmalloc(count * sizeof(void *));	// FIXME: LEAK

	for (i = 0; i < count; i++) {
		ptr[i] = zmalloc(page_size);	// FIXME: LEAK
		generate_random_page((char *) ptr[i]);
	}

	return (unsigned long **) ptr;
}

static void sanitise_execve(struct syscallrecord *rec)
{
	/* we don't want to block if something tries to read from stdin */
	fclose(stdin);

	/* Fabricate argv */
	rec->a2 = (unsigned long) gen_ptrs_to_crap();

	/* Fabricate envp */
	rec->a3 = (unsigned long) gen_ptrs_to_crap();
}

struct syscallentry syscall_execve = {
	.name = "execve",
	.num_args = 3,
	.arg1name = "name",
	.arg1type = ARG_PATHNAME,
	.arg2name = "argv",
	.arg2type = ARG_ADDRESS,
	.arg3name = "envp",
	.arg3type = ARG_ADDRESS,
	.sanitise = sanitise_execve,
	.group = GROUP_VFS,
	.flags = EXTRA_FORK,
	.errnos = {
		.num = 17,
		.values = {
			E2BIG, EACCES, EFAULT, EINVAL, EIO, EISDIR, ELIBBAD, ELOOP,
			EMFILE, ENOENT, ENOEXEC, ENOMEM, ENOTDIR, EPERM, ETXTBSY,
			/* currently undocumented in man page. */
			ENAMETOOLONG, ENXIO,
		},
	},
};
