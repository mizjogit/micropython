/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <assert.h>

#include "py/nlr.h"
#include "py/smallint.h"
#include "py/objint.h"
#include "py/objstr.h"
#include "py/objtype.h"
#include "py/runtime0.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/stream.h"

#if MICROPY_PY_BUILTINS_FLOAT
#include <math.h>
#endif

#if MICROPY_PY_IO
extern struct _mp_dummy_t mp_sys_stdout_obj; // type is irrelevant, just need pointer
#endif

// args[0] is function from class body
// args[1] is class name
// args[2:] are base objects
STATIC mp_obj_t mp_builtin___build_class__(size_t n_args, const mp_obj_t *args) {
    assert(2 <= n_args);

    // set the new classes __locals__ object
    mp_obj_dict_t *old_locals = mp_locals_get();
    mp_obj_t class_locals = mp_obj_new_dict(0);
    mp_locals_set(MP_OBJ_TO_PTR(class_locals));

    // call the class code
    mp_obj_t cell = mp_call_function_0(args[0]);

    // restore old __locals__ object
    mp_locals_set(old_locals);

    // get the class type (meta object) from the base objects
    mp_obj_t meta;
    if (n_args == 2) {
        // no explicit bases, so use 'type'
        meta = MP_OBJ_FROM_PTR(&mp_type_type);
    } else {
        // use type of first base object
        meta = MP_OBJ_FROM_PTR(mp_obj_get_type(args[2]));
    }

    // TODO do proper metaclass resolution for multiple base objects

    // create the new class using a call to the meta object
    mp_obj_t meta_args[3];
    meta_args[0] = args[1]; // class name
    meta_args[1] = mp_obj_new_tuple(n_args - 2, args + 2); // tuple of bases
    meta_args[2] = class_locals; // dict of members
    mp_obj_t new_class = mp_call_function_n_kw(meta, 3, 0, meta_args);

    // store into cell if neede
    if (cell != mp_const_none) {
        mp_obj_cell_set(cell, new_class);
    }

    return new_class;
}
MP_DEFINE_CONST_FUN_OBJ_VAR(mp_builtin___build_class___obj, 2, mp_builtin___build_class__);

STATIC mp_obj_t mp_builtin_abs(mp_obj_t o_in) {
    if (0) {
        // dummy
#if MICROPY_PY_BUILTINS_FLOAT
    } else if (mp_obj_is_float(o_in)) {
        mp_float_t value = mp_obj_float_get(o_in);
        // TODO check for NaN etc
        if (value < 0) {
            return mp_obj_new_float(-value);
        } else {
            return o_in;
        }
#if MICROPY_PY_BUILTINS_COMPLEX
    } else if (MP_OBJ_IS_TYPE(o_in, &mp_type_complex)) {
        mp_float_t real, imag;
        mp_obj_complex_get(o_in, &real, &imag);
        return mp_obj_new_float(MICROPY_FLOAT_C_FUN(sqrt)(real*real + imag*imag));
#endif
#endif
    } else {
        // this will raise a TypeError if the argument is not integral
        return mp_obj_int_abs(o_in);
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_abs_obj, mp_builtin_abs);

STATIC mp_obj_t mp_builtin_all(mp_obj_t o_in) {
    mp_obj_t iterable = mp_getiter(o_in);
    mp_obj_t item;
    while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
        if (!mp_obj_is_true(item)) {
            return mp_const_false;
        }
    }
    return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_all_obj, mp_builtin_all);

STATIC mp_obj_t mp_builtin_any(mp_obj_t o_in) {
    mp_obj_t iterable = mp_getiter(o_in);
    mp_obj_t item;
    while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
        if (mp_obj_is_true(item)) {
            return mp_const_true;
        }
    }
    return mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_any_obj, mp_builtin_any);

STATIC mp_obj_t mp_builtin_bin(mp_obj_t o_in) {
    mp_obj_t args[] = { MP_OBJ_NEW_QSTR(MP_QSTR__brace_open__colon__hash_b_brace_close_), o_in };
    return mp_obj_str_format(MP_ARRAY_SIZE(args), args, NULL);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_bin_obj, mp_builtin_bin);

STATIC mp_obj_t mp_builtin_callable(mp_obj_t o_in) {
    if (mp_obj_is_callable(o_in)) {
        return mp_const_true;
    } else {
        return mp_const_false;
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_callable_obj, mp_builtin_callable);

STATIC mp_obj_t mp_builtin_chr(mp_obj_t o_in) {
    #if MICROPY_PY_BUILTINS_STR_UNICODE
    mp_uint_t c = mp_obj_get_int(o_in);
    char str[4];
    int len = 0;
    if (c < 0x80) {
        *str = c; len = 1;
    } else if (c < 0x800) {
        str[0] = (c >> 6) | 0xC0;
        str[1] = (c & 0x3F) | 0x80;
        len = 2;
    } else if (c < 0x10000) {
        str[0] = (c >> 12) | 0xE0;
        str[1] = ((c >> 6) & 0x3F) | 0x80;
        str[2] = (c & 0x3F) | 0x80;
        len = 3;
    } else if (c < 0x110000) {
        str[0] = (c >> 18) | 0xF0;
        str[1] = ((c >> 12) & 0x3F) | 0x80;
        str[2] = ((c >> 6) & 0x3F) | 0x80;
        str[3] = (c & 0x3F) | 0x80;
        len = 4;
    } else {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "chr() arg not in range(0x110000)"));
    }
    return mp_obj_new_str(str, len, true);
    #else
    mp_int_t ord = mp_obj_get_int(o_in);
    if (0 <= ord && ord <= 0xff) {
        char str[1] = {ord};
        return mp_obj_new_str(str, 1, true);
    } else {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "chr() arg not in range(256)"));
    }
    #endif
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_chr_obj, mp_builtin_chr);

STATIC mp_obj_t mp_builtin_dir(size_t n_args, const mp_obj_t *args) {
    // TODO make this function more general and less of a hack

    mp_obj_dict_t *dict = NULL;
    mp_map_t *members = NULL;
    if (n_args == 0) {
        // make a list of names in the local name space
        dict = mp_locals_get();
    } else { // n_args == 1
        // make a list of names in the given object
        if (MP_OBJ_IS_TYPE(args[0], &mp_type_module)) {
            dict = mp_obj_module_get_globals(args[0]);
        } else {
            mp_obj_type_t *type;
            if (MP_OBJ_IS_TYPE(args[0], &mp_type_type)) {
                type = MP_OBJ_TO_PTR(args[0]);
            } else {
                type = mp_obj_get_type(args[0]);
            }
            if (type->locals_dict != NULL && type->locals_dict->base.type == &mp_type_dict) {
                dict = type->locals_dict;
            }
        }
        if (mp_obj_is_instance_type(mp_obj_get_type(args[0]))) {
            mp_obj_instance_t *inst = MP_OBJ_TO_PTR(args[0]);
            members = &inst->members;
        }
    }

    mp_obj_t dir = mp_obj_new_list(0, NULL);
    if (dict != NULL) {
        for (mp_uint_t i = 0; i < dict->map.alloc; i++) {
            if (MP_MAP_SLOT_IS_FILLED(&dict->map, i)) {
                mp_obj_list_append(dir, dict->map.table[i].key);
            }
        }
    }
    if (members != NULL) {
        for (mp_uint_t i = 0; i < members->alloc; i++) {
            if (MP_MAP_SLOT_IS_FILLED(members, i)) {
                mp_obj_list_append(dir, members->table[i].key);
            }
        }
    }
    return dir;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_dir_obj, 0, 1, mp_builtin_dir);

STATIC mp_obj_t mp_builtin_divmod(mp_obj_t o1_in, mp_obj_t o2_in) {
    return mp_binary_op(MP_BINARY_OP_DIVMOD, o1_in, o2_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_builtin_divmod_obj, mp_builtin_divmod);

STATIC mp_obj_t mp_builtin_hash(mp_obj_t o_in) {
    // result is guaranteed to be a (small) int
    return mp_unary_op(MP_UNARY_OP_HASH, o_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_hash_obj, mp_builtin_hash);

STATIC mp_obj_t mp_builtin_hex(mp_obj_t o_in) {
    return mp_binary_op(MP_BINARY_OP_MODULO, MP_OBJ_NEW_QSTR(MP_QSTR__percent__hash_x), o_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_hex_obj, mp_builtin_hex);

STATIC mp_obj_t mp_builtin_iter(mp_obj_t o_in) {
    return mp_getiter(o_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_iter_obj, mp_builtin_iter);

#if MICROPY_PY_BUILTINS_MIN_MAX

STATIC mp_obj_t mp_builtin_min_max(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs, mp_uint_t op) {
    mp_map_elem_t *key_elem = mp_map_lookup(kwargs, MP_OBJ_NEW_QSTR(MP_QSTR_key), MP_MAP_LOOKUP);
    mp_map_elem_t *default_elem;
    mp_obj_t key_fn = key_elem == NULL ? MP_OBJ_NULL : key_elem->value;
    if (n_args == 1) {
        // given an iterable
        mp_obj_t iterable = mp_getiter(args[0]);
        mp_obj_t best_key = MP_OBJ_NULL;
        mp_obj_t best_obj = MP_OBJ_NULL;
        mp_obj_t item;
        while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
            mp_obj_t key = key_fn == MP_OBJ_NULL ? item : mp_call_function_1(key_fn, item);
            if (best_obj == MP_OBJ_NULL || (mp_binary_op(op, key, best_key) == mp_const_true)) {
                best_key = key;
                best_obj = item;
            }
        }
        if (best_obj == MP_OBJ_NULL) {
            default_elem = mp_map_lookup(kwargs, MP_OBJ_NEW_QSTR(MP_QSTR_default), MP_MAP_LOOKUP);
            if (default_elem != NULL) {
                best_obj = default_elem->value;
            } else {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "arg is an empty sequence"));
            }
        }
        return best_obj;
    } else {
        // given many args
        mp_obj_t best_key = MP_OBJ_NULL;
        mp_obj_t best_obj = MP_OBJ_NULL;
        for (mp_uint_t i = 0; i < n_args; i++) {
            mp_obj_t key = key_fn == MP_OBJ_NULL ? args[i] : mp_call_function_1(key_fn, args[i]);
            if (best_obj == MP_OBJ_NULL || (mp_binary_op(op, key, best_key) == mp_const_true)) {
                best_key = key;
                best_obj = args[i];
            }
        }
        return best_obj;
    }
}

STATIC mp_obj_t mp_builtin_max(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_builtin_min_max(n_args, args, kwargs, MP_BINARY_OP_MORE);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_max_obj, 1, mp_builtin_max);

STATIC mp_obj_t mp_builtin_min(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_builtin_min_max(n_args, args, kwargs, MP_BINARY_OP_LESS);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_min_obj, 1, mp_builtin_min);

#endif

STATIC mp_obj_t mp_builtin_next(mp_obj_t o) {
    mp_obj_t ret = mp_iternext_allow_raise(o);
    if (ret == MP_OBJ_STOP_ITERATION) {
        nlr_raise(mp_obj_new_exception(&mp_type_StopIteration));
    } else {
        return ret;
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_next_obj, mp_builtin_next);

STATIC mp_obj_t mp_builtin_oct(mp_obj_t o_in) {
    return mp_binary_op(MP_BINARY_OP_MODULO, MP_OBJ_NEW_QSTR(MP_QSTR__percent__hash_o), o_in);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_oct_obj, mp_builtin_oct);

STATIC mp_obj_t mp_builtin_ord(mp_obj_t o_in) {
    mp_uint_t len;
    const char *str = mp_obj_str_get_data(o_in, &len);
    #if MICROPY_PY_BUILTINS_STR_UNICODE
    if (MP_OBJ_IS_STR(o_in)) {
        len = unichar_charlen(str, len);
        if (len == 1) {
            if (!UTF8_IS_NONASCII(*str)) {
                goto return_first_byte;
            }
            mp_int_t ord = *str++ & 0x7F;
            for (mp_int_t mask = 0x40; ord & mask; mask >>= 1) {
                ord &= ~mask;
            }
            while (UTF8_IS_CONT(*str)) {
                ord = (ord << 6) | (*str++ & 0x3F);
            }
            return mp_obj_new_int(ord);
        }
    } else {
        // a bytes object
        if (len == 1) {
        return_first_byte:
            return MP_OBJ_NEW_SMALL_INT(((const byte*)str)[0]);
        }
    }
    #else
    if (len == 1) {
        // don't sign extend when converting to ord
        return mp_obj_new_int(((const byte*)str)[0]);
    }
    #endif

    if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
            "ord expects a character"));
    } else {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
            "ord() expected a character, but string of length %d found", len));
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_ord_obj, mp_builtin_ord);

STATIC mp_obj_t mp_builtin_pow(size_t n_args, const mp_obj_t *args) {
    assert(2 <= n_args && n_args <= 3);
    switch (n_args) {
        case 2: return mp_binary_op(MP_BINARY_OP_POWER, args[0], args[1]);
        default: return mp_binary_op(MP_BINARY_OP_MODULO, mp_binary_op(MP_BINARY_OP_POWER, args[0], args[1]), args[2]); // TODO optimise...
    }
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_pow_obj, 2, 3, mp_builtin_pow);

STATIC mp_obj_t mp_builtin_print(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    mp_map_elem_t *sep_elem = mp_map_lookup(kwargs, MP_OBJ_NEW_QSTR(MP_QSTR_sep), MP_MAP_LOOKUP);
    mp_map_elem_t *end_elem = mp_map_lookup(kwargs, MP_OBJ_NEW_QSTR(MP_QSTR_end), MP_MAP_LOOKUP);
    const char *sep_data = " ";
    mp_uint_t sep_len = 1;
    const char *end_data = "\n";
    mp_uint_t end_len = 1;
    if (sep_elem != NULL && sep_elem->value != mp_const_none) {
        sep_data = mp_obj_str_get_data(sep_elem->value, &sep_len);
    }
    if (end_elem != NULL && end_elem->value != mp_const_none) {
        end_data = mp_obj_str_get_data(end_elem->value, &end_len);
    }
    #if MICROPY_PY_IO
    void *stream_obj = &mp_sys_stdout_obj;
    mp_map_elem_t *file_elem = mp_map_lookup(kwargs, MP_OBJ_NEW_QSTR(MP_QSTR_file), MP_MAP_LOOKUP);
    if (file_elem != NULL && file_elem->value != mp_const_none) {
        stream_obj = MP_OBJ_TO_PTR(file_elem->value); // XXX may not be a concrete object
    }

    mp_print_t print = {stream_obj, mp_stream_write_adaptor};
    #endif
    for (mp_uint_t i = 0; i < n_args; i++) {
        if (i > 0) {
            #if MICROPY_PY_IO
            mp_stream_write_adaptor(stream_obj, sep_data, sep_len);
            #else
            mp_print_strn(&mp_plat_print, sep_data, sep_len, 0, 0, 0);
            #endif
        }
        #if MICROPY_PY_IO
        mp_obj_print_helper(&print, args[i], PRINT_STR);
        #else
        mp_obj_print_helper(&mp_plat_print, args[i], PRINT_STR);
        #endif
    }
    #if MICROPY_PY_IO
    mp_stream_write_adaptor(stream_obj, end_data, end_len);
    #else
    mp_print_strn(&mp_plat_print, end_data, end_len, 0, 0, 0);
    #endif
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_print_obj, 0, mp_builtin_print);

STATIC mp_obj_t mp_builtin___repl_print__(mp_obj_t o) {
    if (o != mp_const_none) {
        #if MICROPY_PY_IO
        mp_obj_print_helper(&mp_sys_stdout_print, o, PRINT_REPR);
        mp_print_str(&mp_sys_stdout_print, "\n");
        #else
        mp_obj_print_helper(&mp_plat_print, o, PRINT_REPR);
        mp_print_str(&mp_plat_print, "\n");
        #endif
        #if MICROPY_CAN_OVERRIDE_BUILTINS
        mp_obj_t dest[2] = {MP_OBJ_SENTINEL, o};
        mp_type_module.attr(MP_OBJ_FROM_PTR(&mp_module_builtins), MP_QSTR__, dest);
        #endif
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin___repl_print___obj, mp_builtin___repl_print__);

STATIC mp_obj_t mp_builtin_repr(mp_obj_t o_in) {
    vstr_t vstr;
    mp_print_t print;
    vstr_init_print(&vstr, 16, &print);
    mp_obj_print_helper(&print, o_in, PRINT_REPR);
    return mp_obj_new_str_from_vstr(&mp_type_str, &vstr);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_repr_obj, mp_builtin_repr);

STATIC mp_obj_t mp_builtin_round(size_t n_args, const mp_obj_t *args) {
    mp_obj_t o_in = args[0];
    if (MP_OBJ_IS_INT(o_in)) {
        return o_in;
    }
#if MICROPY_PY_BUILTINS_FLOAT
    mp_int_t num_dig = 0;
    if (n_args > 1) {
        num_dig = mp_obj_get_int(args[1]);
        mp_float_t val = mp_obj_get_float(o_in);
        mp_float_t mult = MICROPY_FLOAT_C_FUN(pow)(10, num_dig);
        // TODO may lead to overflow
        mp_float_t rounded = MICROPY_FLOAT_C_FUN(round)(val * mult) / mult;
        return mp_obj_new_float(rounded);
    }
    mp_float_t val = mp_obj_get_float(o_in);
    mp_float_t rounded = MICROPY_FLOAT_C_FUN(round)(val);
    mp_int_t r = rounded;
    // make rounded value even if it was halfway between ints
    if (val - rounded == 0.5) {
        r = (r + 1) & (~1);
    } else if (val - rounded == -0.5) {
        r &= ~1;
    }
    if (n_args > 1) {
        return mp_obj_new_float(r);
    }
#else
    mp_int_t r = mp_obj_get_int(o_in);
#endif
    return mp_obj_new_int(r);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_round_obj, 1, 2, mp_builtin_round);

STATIC mp_obj_t mp_builtin_sum(size_t n_args, const mp_obj_t *args) {
    assert(1 <= n_args && n_args <= 2);
    mp_obj_t value;
    switch (n_args) {
        case 1: value = MP_OBJ_NEW_SMALL_INT(0); break;
        default: value = args[1]; break;
    }
    mp_obj_t iterable = mp_getiter(args[0]);
    mp_obj_t item;
    while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
        value = mp_binary_op(MP_BINARY_OP_ADD, value, item);
    }
    return value;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_sum_obj, 1, 2, mp_builtin_sum);

STATIC mp_obj_t mp_builtin_sorted(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    assert(n_args >= 1);
    if (n_args > 1) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
                                          "must use keyword argument for key function"));
    }
    mp_obj_t self = mp_type_list.make_new(&mp_type_list, 1, 0, args);
    mp_obj_list_sort(1, &self, kwargs);

    return self;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_sorted_obj, 1, mp_builtin_sorted);

// See mp_load_attr() if making any changes
STATIC inline mp_obj_t mp_load_attr_default(mp_obj_t base, qstr attr, mp_obj_t defval) {
    mp_obj_t dest[2];
    // use load_method, raising or not raising exception
    ((defval == MP_OBJ_NULL) ? mp_load_method : mp_load_method_maybe)(base, attr, dest);
    if (dest[0] == MP_OBJ_NULL) {
        return defval;
    } else if (dest[1] == MP_OBJ_NULL) {
        // load_method returned just a normal attribute
        return dest[0];
    } else {
        // load_method returned a method, so build a bound method object
        return mp_obj_new_bound_meth(dest[0], dest[1]);
    }
}

STATIC mp_obj_t mp_builtin_getattr(size_t n_args, const mp_obj_t *args) {
    mp_obj_t defval = MP_OBJ_NULL;
    if (n_args > 2) {
        defval = args[2];
    }
    return mp_load_attr_default(args[0], mp_obj_str_get_qstr(args[1]), defval);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_getattr_obj, 2, 3, mp_builtin_getattr);

STATIC mp_obj_t mp_builtin_setattr(mp_obj_t base, mp_obj_t attr, mp_obj_t value) {
    mp_store_attr(base, mp_obj_str_get_qstr(attr), value);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_builtin_setattr_obj, mp_builtin_setattr);

STATIC mp_obj_t mp_builtin_hasattr(mp_obj_t object_in, mp_obj_t attr_in) {
    qstr attr = mp_obj_str_get_qstr(attr_in);

    mp_obj_t dest[2];
    // TODO: https://docs.python.org/3/library/functions.html?highlight=hasattr#hasattr
    // explicitly says "This is implemented by calling getattr(object, name) and seeing
    // whether it raises an AttributeError or not.", so we should explicitly wrap this
    // in nlr_push and handle exception.
    mp_load_method_maybe(object_in, attr, dest);

    return mp_obj_new_bool(dest[0] != MP_OBJ_NULL);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_builtin_hasattr_obj, mp_builtin_hasattr);

STATIC mp_obj_t mp_builtin_globals(void) {
    return MP_OBJ_FROM_PTR(mp_globals_get());
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_builtin_globals_obj, mp_builtin_globals);

STATIC mp_obj_t mp_builtin_locals(void) {
    return MP_OBJ_FROM_PTR(mp_locals_get());
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_builtin_locals_obj, mp_builtin_locals);

// These are defined in terms of MicroPython API functions right away
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_id_obj, mp_obj_id);
MP_DEFINE_CONST_FUN_OBJ_1(mp_builtin_len_obj, mp_obj_len);

STATIC const mp_rom_map_elem_t mp_module_builtins_globals_table[] = {
    // built-in core functions
    { MP_ROM_QSTR(MP_QSTR___build_class__), MP_ROM_PTR(&mp_builtin___build_class___obj) },
    { MP_ROM_QSTR(MP_QSTR___import__), MP_ROM_PTR(&mp_builtin___import___obj) },
    { MP_ROM_QSTR(MP_QSTR___repl_print__), MP_ROM_PTR(&mp_builtin___repl_print___obj) },

    // built-in types
    { MP_ROM_QSTR(MP_QSTR_bool), MP_ROM_PTR(&mp_type_bool) },
    { MP_ROM_QSTR(MP_QSTR_bytes), MP_ROM_PTR(&mp_type_bytes) },
    #if MICROPY_PY_BUILTINS_BYTEARRAY
    { MP_ROM_QSTR(MP_QSTR_bytearray), MP_ROM_PTR(&mp_type_bytearray) },
    #endif
    #if MICROPY_PY_BUILTINS_COMPLEX
    { MP_ROM_QSTR(MP_QSTR_complex), MP_ROM_PTR(&mp_type_complex) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_dict), MP_ROM_PTR(&mp_type_dict) },
    #if MICROPY_PY_BUILTINS_ENUMERATE
    { MP_ROM_QSTR(MP_QSTR_enumerate), MP_ROM_PTR(&mp_type_enumerate) },
    #endif
    #if MICROPY_PY_BUILTINS_FILTER
    { MP_ROM_QSTR(MP_QSTR_filter), MP_ROM_PTR(&mp_type_filter) },
    #endif
    #if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_float), MP_ROM_PTR(&mp_type_float) },
    #endif
    #if MICROPY_PY_BUILTINS_SET && MICROPY_PY_BUILTINS_FROZENSET
    { MP_ROM_QSTR(MP_QSTR_frozenset), MP_ROM_PTR(&mp_type_frozenset) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_int), MP_ROM_PTR(&mp_type_int) },
    { MP_ROM_QSTR(MP_QSTR_list), MP_ROM_PTR(&mp_type_list) },
    { MP_ROM_QSTR(MP_QSTR_map), MP_ROM_PTR(&mp_type_map) },
    #if MICROPY_PY_BUILTINS_MEMORYVIEW
    { MP_ROM_QSTR(MP_QSTR_memoryview), MP_ROM_PTR(&mp_type_memoryview) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_object), MP_ROM_PTR(&mp_type_object) },
    #if MICROPY_PY_BUILTINS_PROPERTY
    { MP_ROM_QSTR(MP_QSTR_property), MP_ROM_PTR(&mp_type_property) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_range), MP_ROM_PTR(&mp_type_range) },
    #if MICROPY_PY_BUILTINS_REVERSED
    { MP_ROM_QSTR(MP_QSTR_reversed), MP_ROM_PTR(&mp_type_reversed) },
    #endif
    #if MICROPY_PY_BUILTINS_SET
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&mp_type_set) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_str), MP_ROM_PTR(&mp_type_str) },
    { MP_ROM_QSTR(MP_QSTR_super), MP_ROM_PTR(&mp_type_super) },
    { MP_ROM_QSTR(MP_QSTR_tuple), MP_ROM_PTR(&mp_type_tuple) },
    { MP_ROM_QSTR(MP_QSTR_type), MP_ROM_PTR(&mp_type_type) },
    { MP_ROM_QSTR(MP_QSTR_zip), MP_ROM_PTR(&mp_type_zip) },

    { MP_ROM_QSTR(MP_QSTR_classmethod), MP_ROM_PTR(&mp_type_classmethod) },
    { MP_ROM_QSTR(MP_QSTR_staticmethod), MP_ROM_PTR(&mp_type_staticmethod) },

    // built-in objects
    { MP_ROM_QSTR(MP_QSTR_Ellipsis), MP_ROM_PTR(&mp_const_ellipsis_obj) },
    #if MICROPY_PY_BUILTINS_NOTIMPLEMENTED
    { MP_ROM_QSTR(MP_QSTR_NotImplemented), MP_ROM_PTR(&mp_const_notimplemented_obj) },
    #endif

    // built-in user functions
    { MP_ROM_QSTR(MP_QSTR_abs), MP_ROM_PTR(&mp_builtin_abs_obj) },
    { MP_ROM_QSTR(MP_QSTR_all), MP_ROM_PTR(&mp_builtin_all_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&mp_builtin_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_bin), MP_ROM_PTR(&mp_builtin_bin_obj) },
    { MP_ROM_QSTR(MP_QSTR_callable), MP_ROM_PTR(&mp_builtin_callable_obj) },
    #if MICROPY_PY_BUILTINS_COMPILE
    { MP_ROM_QSTR(MP_QSTR_compile), MP_ROM_PTR(&mp_builtin_compile_obj) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_chr), MP_ROM_PTR(&mp_builtin_chr_obj) },
    { MP_ROM_QSTR(MP_QSTR_dir), MP_ROM_PTR(&mp_builtin_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_divmod), MP_ROM_PTR(&mp_builtin_divmod_obj) },
    #if MICROPY_PY_BUILTINS_EVAL_EXEC
    { MP_ROM_QSTR(MP_QSTR_eval), MP_ROM_PTR(&mp_builtin_eval_obj) },
    { MP_ROM_QSTR(MP_QSTR_exec), MP_ROM_PTR(&mp_builtin_exec_obj) },
    #endif
    #if MICROPY_PY_BUILTINS_EXECFILE
    { MP_ROM_QSTR(MP_QSTR_execfile), MP_ROM_PTR(&mp_builtin_execfile_obj) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_getattr), MP_ROM_PTR(&mp_builtin_getattr_obj) },
    { MP_ROM_QSTR(MP_QSTR_setattr), MP_ROM_PTR(&mp_builtin_setattr_obj) },
    { MP_ROM_QSTR(MP_QSTR_globals), MP_ROM_PTR(&mp_builtin_globals_obj) },
    { MP_ROM_QSTR(MP_QSTR_hasattr), MP_ROM_PTR(&mp_builtin_hasattr_obj) },
    { MP_ROM_QSTR(MP_QSTR_hash), MP_ROM_PTR(&mp_builtin_hash_obj) },
    { MP_ROM_QSTR(MP_QSTR_hex), MP_ROM_PTR(&mp_builtin_hex_obj) },
    { MP_ROM_QSTR(MP_QSTR_id), MP_ROM_PTR(&mp_builtin_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_isinstance), MP_ROM_PTR(&mp_builtin_isinstance_obj) },
    { MP_ROM_QSTR(MP_QSTR_issubclass), MP_ROM_PTR(&mp_builtin_issubclass_obj) },
    { MP_ROM_QSTR(MP_QSTR_iter), MP_ROM_PTR(&mp_builtin_iter_obj) },
    { MP_ROM_QSTR(MP_QSTR_len), MP_ROM_PTR(&mp_builtin_len_obj) },
    { MP_ROM_QSTR(MP_QSTR_locals), MP_ROM_PTR(&mp_builtin_locals_obj) },
    #if MICROPY_PY_BUILTINS_MIN_MAX
    { MP_ROM_QSTR(MP_QSTR_max), MP_ROM_PTR(&mp_builtin_max_obj) },
    { MP_ROM_QSTR(MP_QSTR_min), MP_ROM_PTR(&mp_builtin_min_obj) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_next), MP_ROM_PTR(&mp_builtin_next_obj) },
    { MP_ROM_QSTR(MP_QSTR_oct), MP_ROM_PTR(&mp_builtin_oct_obj) },
    { MP_ROM_QSTR(MP_QSTR_ord), MP_ROM_PTR(&mp_builtin_ord_obj) },
    { MP_ROM_QSTR(MP_QSTR_pow), MP_ROM_PTR(&mp_builtin_pow_obj) },
    { MP_ROM_QSTR(MP_QSTR_print), MP_ROM_PTR(&mp_builtin_print_obj) },
    { MP_ROM_QSTR(MP_QSTR_repr), MP_ROM_PTR(&mp_builtin_repr_obj) },
    { MP_ROM_QSTR(MP_QSTR_round), MP_ROM_PTR(&mp_builtin_round_obj) },
    { MP_ROM_QSTR(MP_QSTR_sorted), MP_ROM_PTR(&mp_builtin_sorted_obj) },
    { MP_ROM_QSTR(MP_QSTR_sum), MP_ROM_PTR(&mp_builtin_sum_obj) },

    // built-in exceptions
    { MP_ROM_QSTR(MP_QSTR_BaseException), MP_ROM_PTR(&mp_type_BaseException) },
    { MP_ROM_QSTR(MP_QSTR_ArithmeticError), MP_ROM_PTR(&mp_type_ArithmeticError) },
    { MP_ROM_QSTR(MP_QSTR_AssertionError), MP_ROM_PTR(&mp_type_AssertionError) },
    { MP_ROM_QSTR(MP_QSTR_AttributeError), MP_ROM_PTR(&mp_type_AttributeError) },
    { MP_ROM_QSTR(MP_QSTR_EOFError), MP_ROM_PTR(&mp_type_EOFError) },
    { MP_ROM_QSTR(MP_QSTR_Exception), MP_ROM_PTR(&mp_type_Exception) },
    { MP_ROM_QSTR(MP_QSTR_GeneratorExit), MP_ROM_PTR(&mp_type_GeneratorExit) },
    { MP_ROM_QSTR(MP_QSTR_ImportError), MP_ROM_PTR(&mp_type_ImportError) },
    { MP_ROM_QSTR(MP_QSTR_IndentationError), MP_ROM_PTR(&mp_type_IndentationError) },
    { MP_ROM_QSTR(MP_QSTR_IndexError), MP_ROM_PTR(&mp_type_IndexError) },
    { MP_ROM_QSTR(MP_QSTR_KeyboardInterrupt), MP_ROM_PTR(&mp_type_KeyboardInterrupt) },
    { MP_ROM_QSTR(MP_QSTR_KeyError), MP_ROM_PTR(&mp_type_KeyError) },
    { MP_ROM_QSTR(MP_QSTR_LookupError), MP_ROM_PTR(&mp_type_LookupError) },
    { MP_ROM_QSTR(MP_QSTR_MemoryError), MP_ROM_PTR(&mp_type_MemoryError) },
    { MP_ROM_QSTR(MP_QSTR_NameError), MP_ROM_PTR(&mp_type_NameError) },
    { MP_ROM_QSTR(MP_QSTR_NotImplementedError), MP_ROM_PTR(&mp_type_NotImplementedError) },
    { MP_ROM_QSTR(MP_QSTR_OSError), MP_ROM_PTR(&mp_type_OSError) },
    { MP_ROM_QSTR(MP_QSTR_OverflowError), MP_ROM_PTR(&mp_type_OverflowError) },
    { MP_ROM_QSTR(MP_QSTR_RuntimeError), MP_ROM_PTR(&mp_type_RuntimeError) },
    { MP_ROM_QSTR(MP_QSTR_StopIteration), MP_ROM_PTR(&mp_type_StopIteration) },
    { MP_ROM_QSTR(MP_QSTR_SyntaxError), MP_ROM_PTR(&mp_type_SyntaxError) },
    { MP_ROM_QSTR(MP_QSTR_SystemExit), MP_ROM_PTR(&mp_type_SystemExit) },
    { MP_ROM_QSTR(MP_QSTR_TypeError), MP_ROM_PTR(&mp_type_TypeError) },
#if MICROPY_MODULE_ESP_QUEUE
    { MP_OBJ_NEW_QSTR(MP_QSTR_Empty), (mp_obj_t)&mp_type_Empty },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Full), (mp_obj_t)&mp_type_Full },
#endif
    #if MICROPY_PY_BUILTINS_STR_UNICODE
    { MP_ROM_QSTR(MP_QSTR_UnicodeError), MP_ROM_PTR(&mp_type_UnicodeError) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_ValueError), MP_ROM_PTR(&mp_type_ValueError) },
    #if MICROPY_EMIT_NATIVE
    { MP_ROM_QSTR(MP_QSTR_ViperTypeError), MP_ROM_PTR(&mp_type_ViperTypeError) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_ZeroDivisionError), MP_ROM_PTR(&mp_type_ZeroDivisionError) },
    // Somehow CPython managed to have OverflowError not inherit from ValueError ;-/
    // TODO: For MICROPY_CPYTHON_COMPAT==0 use ValueError to avoid exc proliferation

    // Extra builtins as defined by a port
    MICROPY_PORT_BUILTINS
};

MP_DEFINE_CONST_DICT(mp_module_builtins_globals, mp_module_builtins_globals_table);

const mp_obj_module_t mp_module_builtins = {
    .base = { &mp_type_module },
    .name = MP_QSTR_builtins,
    .globals = (mp_obj_dict_t*)&mp_module_builtins_globals,
};
