/* kv.c — contoh ekstensi GNE: key-value in-memory dengan state per-modul.
 *
 * Pola ini sama dengan yang dipakai ekstensi Redis/Database: state C
 * digantungkan pada instance data modul lewat api->set_data, sehingga
 * aman bila satu .so dimuat oleh beberapa interpreter.
 */
#include "gne.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const gne_host_api *api;

typedef struct kv_entry {
	char *key;
	char *val;
	struct kv_entry *next;
} kv_entry;

static kv_entry **find(kv_entry **head, const char *k)
{
	kv_entry **cur = head;
	while (*cur) {
		if (strcmp((*cur)->key, k) == 0)
			return cur;
		cur = &(*cur)->next;
	}
	return cur;
}

/* Salin string argument ke memori C (handle argumen hanya sementara). */
static char *dup_arg(gne_ctx *ctx, gne_handle h)
{
	size_t len;
	char *out;
	if (api->str_len(ctx, h, &len) != 0)
		return NULL;
	out = malloc(len + 1);
	if (!out)
		return NULL;
	if (api->str_copy(ctx, h, out, len + 1) < 0) {
		free(out);
		return NULL;
	}
	return out;
}

static int kv_set(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	kv_entry *e;
	kv_entry **slot;
	char *k = dup_arg(ctx, argv[0]);
	char *v = dup_arg(ctx, argv[1]);
	(void)argc;
	if (!k || !v) {
		free(k);
		free(v);
		api->throw(ctx, "kv_error", 500, "set() butuh dua string");
		return 1;
	}
	slot = find((kv_entry **)api->get_data(ctx), k);
	if (*slot) {
		free((*slot)->val);
		(*slot)->val = v;
		free(k);
	} else {
		e = calloc(1, sizeof(*e));
		if (!e) {
			free(k);
			free(v);
			api->throw(ctx, "kv_oom", 500, "gagal alokasi");
			return 1;
		}
		e->key = k;
		e->val = v;
		e->next = NULL;
		*slot = e;
	}
	*ret = api->null(ctx);
	return 0;
}

static int kv_get(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	char *k = dup_arg(ctx, argv[0]);
	kv_entry *e;
	(void)argc;
	if (!k) {
		*ret = api->null(ctx);
		return 0;
	}
	e = *find((kv_entry **)api->get_data(ctx), k);
	free(k);
	if (!e) {
		*ret = api->null(ctx);
		return 0;
	}
	*ret = api->string(ctx, e->val, strlen(e->val));
	return 0;
}

static int kv_has(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	char *k = dup_arg(ctx, argv[0]);
	kv_entry *e;
	(void)argc;
	e = k ? *find((kv_entry **)api->get_data(ctx), k) : NULL;
	free(k);
	*ret = api->bool_new(ctx, e != NULL);
	return 0;
}

static int kv_del(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	char *k = dup_arg(ctx, argv[0]);
	kv_entry **slot;
	(void)argc;
	slot = k ? find((kv_entry **)api->get_data(ctx), k) : NULL;
	free(k);
	if (slot && *slot) {
		kv_entry *dead = *slot;
		*slot = dead->next;
		free(dead->key);
		free(dead->val);
		free(dead);
		*ret = api->bool_new(ctx, 1);
	} else {
		*ret = api->bool_new(ctx, 0);
	}
	return 0;
}

static int kv_count(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	kv_entry *e;
	int64_t n = 0;
	(void)argc; (void)argv;
	for (e = *(kv_entry **)api->get_data(ctx); e; e = e->next)
		n++;
	*ret = api->int_new(ctx, n);
	return 0;
}

/* save($path) / load($path) — bukti persistensi sederhana. */
static int kv_save(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	char *path = dup_arg(ctx, argv[0]);
	kv_entry *e;
	FILE *f;
	(void)argc;
	if (!path) {
		api->throw(ctx, "kv_error", 500, "save() butuh path string");
		return 1;
	}
	f = fopen(path, "w");
	if (!f) {
		free(path);
		api->throw(ctx, "kv_io", 500, "gagal membuka berkas untuk menulis");
		return 1;
	}
	for (e = *(kv_entry **)api->get_data(ctx); e; e = e->next)
		fprintf(f, "%s\t%s\n", e->key, e->val);
	fclose(f);
	free(path);
	*ret = api->bool_new(ctx, 1);
	return 0;
}

static int kv_load(gne_ctx *ctx, int argc, const gne_handle *argv, gne_handle *ret)
{
	char *path = dup_arg(ctx, argv[0]);
	FILE *f;
	char line[4096];
	int n = 0;
	(void)argc;
	if (!path) {
		api->throw(ctx, "kv_error", 500, "load() butuh path string");
		return 1;
	}
	f = fopen(path, "r");
	if (!f) {
		free(path);
		*ret = api->int_new(ctx, 0);
		return 0; /* berkas belum ada = 0 baris, bukan galat */
	}
	while (fgets(line, sizeof(line), f)) {
		char *tab = strchr(line, '\t');
		size_t len;
		if (!tab)
			continue;
		*tab = 0;
		len = strlen(tab + 1);
		if (len && tab[len] == '\n')
			tab[len] = 0; /* alamat terakhir = (tab+1)+len-1 */
		/* set ulang lewat jalur resmi kv_set */
		{
			gne_handle k = api->string(ctx, line, strlen(line));
			gne_handle v = api->string(ctx, tab + 1, strlen(tab + 1));
			gne_handle dummy;
			kv_set(ctx, 2, (const gne_handle[]){k, v}, &dummy);
		}
		n++;
	}
	fclose(f);
	free(path);
	*ret = api->int_new(ctx, n);
	return 0;
}

int gne_module_init(const gne_host_api *a, gne_ctx *ctx, gne_handle *out)
{
	kv_entry **state;
	api = a;
	if (api->abi != GNE_ABI) {
		api->throw(ctx, "gne_abi", 500, "ABI berbeda");
		return 1;
	}
	state = calloc(1, sizeof(kv_entry *));
	if (!state) {
		api->throw(ctx, "kv_oom", 500, "gagal mengalokasikan state");
		return 1;
	}
	api->set_data(ctx, state);

	api->define_fn(ctx, "set", 2, 2, kv_set);
	api->define_fn(ctx, "get", 1, 1, kv_get);
	api->define_fn(ctx, "has", 1, 1, kv_has);
	api->define_fn(ctx, "del", 1, 1, kv_del);
	api->define_fn(ctx, "count", 0, 0, kv_count);
	api->define_fn(ctx, "save", 1, 1, kv_save);
	api->define_fn(ctx, "load", 1, 1, kv_load);
	*out = api->module(ctx);
	return 0;
}
