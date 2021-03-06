/*
 * Routines to dirty/fault-in mapped pages.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "arch.h"
#include "maps.h"
#include "random.h"
#include "utils.h"

static unsigned int nr_pages(struct map *map)
{
	return map->size / page_size;
}

struct faultfn {
	void (*func)(struct map *map);
};

/*****************************************************************************/
/* dirty page routines */

static void dirty_one_page(struct map *map)
{
	char *p = map->ptr;

	p[rand() % (map->size - 1)] = rand();
}

static void dirty_whole_mapping(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr;

	nr = nr_pages(map);

	for (i = 0; i < nr; i++)
		p[i * page_size] = rand();
}

static void dirty_every_other_page(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr, first;

	nr = nr_pages(map);

	first = rand_bool();

	for (i = first; i < nr; i+=2)
		p[i * page_size] = rand();
}

static void dirty_mapping_reverse(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr;

	nr = nr_pages(map) - 1;

	for (i = nr; i > 0; i--)
		p[i * page_size] = rand();
}

/* dirty a random set of map->size pages. (some may be faulted >once) */
static void dirty_random_pages(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr;

	nr = nr_pages(map);

	for (i = 0; i < nr; i++)
		p[(rand() % nr) * page_size] = rand();
}

/* Dirty the last page in a mapping
 * Fill it with ascii, in the hope we do something like
 * a strlen and go off the end. */
static void dirty_last_page(struct map *map)
{
	char *p = map->ptr;

	memset((void *) p + ((map->size - 1) - page_size), 'A', page_size);
}

static const struct faultfn write_faultfns[] = {
	{ .func = dirty_one_page },
	{ .func = dirty_whole_mapping },
	{ .func = dirty_every_other_page },
	{ .func = dirty_mapping_reverse },
	{ .func = dirty_random_pages },
	{ .func = dirty_last_page },
};

/*****************************************************************************/
/* routines to fault in pages */

static void read_one_page(struct map *map)
{
	char *p = map->ptr;
	unsigned long offset = (rand() % (map->size - 1)) & PAGE_MASK;
	char buf[page_size];

	p += offset;
	memcpy(buf, p, page_size);
}


static void read_whole_mapping(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr;
	char buf[page_size];

	nr = nr_pages(map);

	for (i = 0; i < nr; i++)
		memcpy(buf, p + (i * page_size), page_size);
}

static void read_every_other_page(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr, first;
	char buf[page_size];

	nr = nr_pages(map);

	first = rand_bool();

	for (i = first; i < nr; i+=2)
		memcpy(buf, p + (i * page_size), page_size);
}

static void read_mapping_reverse(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr;
	char buf[page_size];

	nr = nr_pages(map) - 1;

	for (i = nr; i > 0; i--)
		memcpy(buf, p + (i * page_size), page_size);
}

/* fault in a random set of map->size pages. (some may be faulted >once) */
static void read_random_pages(struct map *map)
{
	char *p = map->ptr;
	unsigned int i, nr;
	char buf[page_size];

	nr = nr_pages(map);

	for (i = 0; i < nr; i++)
		memcpy(buf, p + ((rand() % nr) * page_size), page_size);
}

/* Fault in the last page in a mapping */
static void read_last_page(struct map *map)
{
	char *p = map->ptr;
	char buf[page_size];

	memcpy(buf, p + ((map->size - 1) - page_size), page_size);
}

static const struct faultfn read_faultfns[] = {
	{ .func = read_one_page },
	{ .func = read_whole_mapping },
	{ .func = read_every_other_page },
	{ .func = read_mapping_reverse },
	{ .func = read_random_pages },
	{ .func = read_last_page },
};

/*****************************************************************************/

/*
 * Routine to perform various kinds of write operations to a mapping
 * that we created.
 */
void dirty_mapping(struct map *map)
{
	bool rw = rand_bool();

	if (rw == TRUE) {
		/* Check mapping is writable, or we'll segv.
		 * TODO: Perhaps we should do that, and trap it, mark it writable,
		 * then reprotect after we dirtied it ? */
		if (!(map->prot & PROT_WRITE))
			return;

		write_faultfns[rand() % ARRAY_SIZE(write_faultfns)].func(map);
		return;
	} else {
		if (!(map->prot & PROT_READ))
			return;

		read_faultfns[rand() % ARRAY_SIZE(read_faultfns)].func(map);
	}
}
