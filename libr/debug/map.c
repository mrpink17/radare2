/* radare - LGPL - Copyright 2009-2015 - pancake */

#include <r_debug.h>
#include <r_list.h>

R_API void r_debug_map_list(RDebug *dbg, ut64 addr, int rad) {
	const char *fmtstr;
	char buf[128];
	int notfirst = R_FALSE;
	RListIter *iter;
	RDebugMap *map;
	if (!dbg) return;

	switch (rad) {
	case 'j':
		dbg->cb_printf ("[");
		r_list_foreach (dbg->maps, iter, map) {
			if (notfirst) dbg->cb_printf (",");
			dbg->cb_printf ("{\"name\":\"%s\",",map->name);
			dbg->cb_printf ("\"addr\":%"PFMT64u",", map->addr);
			dbg->cb_printf ("\"addr_end\":%"PFMT64u",", map->addr_end);
			dbg->cb_printf ("\"type\":\"%c\",", map->user?'u':'s');
			dbg->cb_printf ("\"perm\":\"%s\"}", r_str_rwx_i (map->perm));
			notfirst = R_TRUE;
		}
		r_list_foreach (dbg->maps_user, iter, map) {
			if (notfirst) dbg->cb_printf (",");
			dbg->cb_printf ("{\"name\":\"%s\",",map->name);
			dbg->cb_printf ("\"addr\":%"PFMT64u",", map->addr);
			dbg->cb_printf ("\"addr_end\":%"PFMT64u",", map->addr_end);
			dbg->cb_printf ("\"type\":\"%c\",", map->user?'u':'s');
			dbg->cb_printf ("\"perm\":\"%s\"}", r_str_rwx_i (map->perm));
			notfirst = R_TRUE;
		}
		dbg->cb_printf ("]\n");
		break;
	case '*':
		r_list_foreach (dbg->maps, iter, map) {
			char *name = r_str_newf ("%s.%s", map->name,
				r_str_rwx_i (map->perm));
			r_name_filter (name, 0);
			dbg->cb_printf ("f map.%s 0x%08"PFMT64x" 0x%08"PFMT64x"\n",
				name, map->addr_end - map->addr, map->addr);
			free (name);
		}
		r_list_foreach (dbg->maps_user, iter, map) {
			char *name = r_str_newf ("%s.%s", map->name,
				r_str_rwx_i (map->perm));
			r_name_filter (name, 0);
			dbg->cb_printf ("f map.%s 0x%08"PFMT64x" 0x%08"PFMT64x"\n",
				name, map->addr_end - map->addr, map->addr);
			free (name);
		}
		break;
	default:
		fmtstr = dbg->bits& R_SYS_BITS_64?
			"sys %04s 0x%016"PFMT64x" %c 0x%016"PFMT64x" %c %s %s\n":
			"sys %04s 0x%08"PFMT64x" %c 0x%08"PFMT64x" %c %s %s\n";
		r_list_foreach (dbg->maps, iter, map) {
			r_num_units (buf, map->size);
			dbg->cb_printf (fmtstr,
				buf, map->addr, (addr>=map->addr && addr<map->addr_end)?'*':'-',
				map->addr_end, map->user?'u':'s',
				r_str_rwx_i (map->perm), map->name, buf);
		}
		fmtstr = dbg->bits& R_SYS_BITS_64?
			"usr %04s 0x%016"PFMT64x" - 0x%016"PFMT64x" %c %x %s\n":
			"usr %04s 0x%08"PFMT64x" - 0x%08"PFMT64x" %c %x %s\n";
		r_list_foreach (dbg->maps_user, iter, map) {
			r_num_units (buf, map->size);
			dbg->cb_printf ("usr %04s 0x%016"PFMT64x" - 0x%016"PFMT64x" %c %x %s\n",
				buf, map->addr, map->addr_end,
				map->user?'u':'s', map->perm, map->name);
		}
		break;
	}
}

R_API RDebugMap *r_debug_map_new(char *name, ut64 addr, ut64 addr_end, int perm, int user) {
	RDebugMap *map;
	if (name == NULL || addr >= addr_end) {
		eprintf ("r_debug_map_new: error assert(\
			%"PFMT64x">=%"PFMT64x")\n", addr, addr_end);
		return NULL;
	}
	map = R_NEW (RDebugMap);
	if (!map) return NULL;
	map->name = strdup (name);
	map->file = NULL;
	map->addr = addr;
	map->addr_end = addr_end;
	map->size = addr_end-addr;
	map->perm = perm;
	map->user = user;
	return map;
}

R_API RList *r_debug_modules_list(RDebug *dbg) {
	if (dbg && dbg->h && dbg->h->modules_get) {
		return dbg->h->modules_get (dbg);
	}
	return NULL;
}

R_API int r_debug_map_sync(RDebug *dbg) {
	int ret = R_FALSE;
	if (dbg && dbg->h && dbg->h->map_get) {
		RList *newmaps = dbg->h->map_get (dbg);
		if (newmaps) {
			r_list_free (dbg->maps);
			dbg->maps = newmaps;
			ret = R_TRUE;
		}
	}
	return ret;
}

R_API RDebugMap* r_debug_map_alloc(RDebug *dbg, ut64 addr, int size) {
	RDebugMap *map = NULL;
	if (dbg && dbg->h && dbg->h->map_alloc) {
		map = dbg->h->map_alloc (dbg, addr, size);
	}
	return map;
}

R_API int r_debug_map_dealloc(RDebug *dbg, RDebugMap *map) {
	int ret = R_FALSE;
	ut64 addr = map->addr;
	if (dbg && dbg->h && dbg->h->map_dealloc) {
		if (dbg->h->map_dealloc (dbg, addr, map->size)) {
			ret = R_TRUE;
		}
	}
	return ret;
}

R_API RDebugMap *r_debug_map_get(RDebug *dbg, ut64 addr) {
	RDebugMap *map, *ret = NULL;
	RListIter *iter;
	r_list_foreach (dbg->maps, iter, map) {
		if (addr >= map->addr && addr <= map->addr_end) {
			ret = map;
			break;
		}
	}
	return ret;
}

R_API void r_debug_map_free(RDebugMap *map) {
	free (map->name);
	free (map);
}

R_API RList *r_debug_map_list_new() {
	RList *list = r_list_new ();
	if (!list) return NULL;
	list->free = (RListFree)r_debug_map_free;
	return list;
}

