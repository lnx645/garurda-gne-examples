/* hello.c — ekstensi GNE mini untuk smoke test. */
#include "gne.h"
#include <string.h>

static const gne_host_api *api;
static int64_t call_count; /* state instance (dibagantung di set_data) */

static int counter_next(gne_ctx *ctx, int argc, const gne_handle *argv,
			gne_handle *ret)
{
	int64_t n = 0;
	gne_handle v;
	(void)argc;
	if (api->obj_get(ctx, argv[0], "n", &v) == 0)
		api->get_int(ctx, v, &n);
	n++;
	v = api->int_new(ctx, n);
	api->obj_set(ctx, argv[0], "n", v);
	*ret = api->int_new(ctx, n);
	return 0;
}

static int add(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	int64_t a, b;
	(void)argc;
	if (api->get_int(ctx, argv[0], &a) != 0 ||
	    api->get_int(ctx, argv[1], &b) != 0) {
		api->throw(ctx, "type_error", 500, "add() butuh dua integer");
		return 1;
	}
	call_count++;
	*ret = api->int_new(ctx, a + b);
	return 0;
}

static int boom(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	(void)argc; (void)argv; (void)ret;
	api->throw(ctx, "hello_error", 418, "sengaja meledak");
	return 1;
}

static int join(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	char buf[256];
	size_t n = 0;
	int i;
	buf[0] = 0;
	for (i = 0; i < argc; i++) {
		size_t len;
		if (api->str_len(ctx, argv[i], &len) != 0)
			continue;
		if (len + 1 >= sizeof(buf) - n)
			break;
		api->str_copy(ctx, argv[i], buf + n, sizeof(buf) - n);
		n += len;
	}
	*ret = api->string(ctx, buf, n);
	return 0;
}

/* callback: panggil closure Garurda lewat api->call */
static int twice(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	gne_handle one, out;
	(void)argc;
	one = api->int_new(ctx, 1);
	if (api->call(ctx, argv[0], 1, &one, &out) != 0)
		return 1;
	*ret = out;
	return 0;
}

/* instance data: simpan handle argumen (retain) untuk dibaca nanti */
static int stash(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	(void)argc;
	api->retain(ctx, argv[0]);
	api->set_data(ctx, (void *)1); /* sentuh instance data */
	extern uint64_t g_stash;
	g_stash = (uint64_t)argv[0];
	*ret = api->bool_new(ctx, 1);
	return 0;
}

uint64_t g_stash;

static int unstash(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	extern uint64_t g_stash;
	(void)argc; (void)argv;
	if (!g_stash) {
		*ret = api->null(ctx);
		return 0;
	}
	/* kembalikan handle tersimpan; retain dulu karena host mengambil
	 * satu referensi dari *ret */
	api->retain(ctx, g_stash);
	*ret = g_stash;
	return 0;
}

static int calls(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	(void)argc; (void)argv;
	*ret = api->int_new(ctx, call_count);
	return 0;
}

int gne_module_init(const gne_host_api *a, gne_ctx *ctx, gne_handle *out)
{
	gne_handle counter, ns;
	api = a;
	if (api->abi != GNE_ABI) {
		api->throw(ctx, "gne_abi", 500, "ABI berbeda");
		return 1;
	}
	api->define_fn(ctx, "add", 2, 2, add);
	api->define_fn(ctx, "boom", 0, 0, boom);
	api->define_fn(ctx, "join", 0, -1, join);
	api->define_fn(ctx, "twice", 1, 1, twice);
	api->define_fn(ctx, "stash", 1, 1, stash);
	api->define_fn(ctx, "unstash", 0, 0, unstash);
	api->define_fn(ctx, "calls", 0, 0, calls);

	counter = api->object(ctx);
	api->obj_set(ctx, counter, "n", api->int_new(ctx, 0));
	api->define_method(ctx, counter, "next", 0, 0, counter_next, counter);
	ns = api->module(ctx);
	api->obj_set(ctx, ns, "counter", counter);

	*out = api->module(ctx);
	return 0;
}
