/**
 * @file vtrecv.h
 * @author CuBeatSystems
 * @author Shinichiro Nakamura
 * @copyright
 * ===============================================================
 * Natural Tiny Shell (NT-Shell) Version 0.3.1
 * ===============================================================
 * Copyright (c) 2010-2016 Shinichiro Nakamura
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * @note
 * An implementation of Paul Williams' DEC compatible state machine parser.
 * This code is in the public domain.
 *
 * @author Joshua Haberman <joshua@reverberate.org>
 * @author Shinichiro Nakamura : Modified for Natural Tiny Shell (NT-Shell)
 */

#ifndef VTRECV_H
#define VTRECV_H

/**
 * @brief オリジナルに含まれるLUTを使うかどうかを決定する。
 * @details
 * オリジナルでは、シーケンスの遷移をテーブル参照で実装してあった。
 * 15のステートで取りうる256パターンの入力を全網羅するテーブルである。
 * これは3840個のテーブルデータを持つことになる。
 *
 * テーブル参照はメモリに対してリニアアクセス可能なプロセッサにおいて
 * 固定時間で動作する。テーブル参照のメリットは固定時間での処理である。
 *
 * 一方、新たに実装した方法は、重複するデータが多数存在する事に着目した
 * もので、区間毎に適用するシーケンスを定義したテーブルを用いる。
 * これはテーブルを線形探索するため後方にあるデータになるほど動作は遅い。
 * しかし、コードサイズはオリジナルの全網羅形式のテーブルよりも小さい。
 *
 * @retval 0 使わない。
 * @retval 1 使う。
 */
#define USE_ORIGINAL_LUT (0)

#define MAX_INTERMEDIATE_CHARS 2
#define ACTION(state_change) (vtrecv_action_t)((state_change & 0x0F) >> 0)
#define STATE(state_change)  (vtrecv_state_t)((state_change & 0xF0) >> 4)

typedef enum {
    VTRECV_STATE_CSI_ENTRY = 1,
    VTRECV_STATE_CSI_IGNORE = 2,
    VTRECV_STATE_CSI_INTERMEDIATE = 3,
    VTRECV_STATE_CSI_PARAM = 4,
    VTRECV_STATE_DCS_ENTRY = 5,
    VTRECV_STATE_DCS_IGNORE = 6,
    VTRECV_STATE_DCS_INTERMEDIATE = 7,
    VTRECV_STATE_DCS_PARAM = 8,
    VTRECV_STATE_DCS_PASSTHROUGH = 9,
    VTRECV_STATE_ESCAPE = 10,
    VTRECV_STATE_ESCAPE_INTERMEDIATE = 11,
    VTRECV_STATE_GROUND = 12,
    VTRECV_STATE_OSC_STRING = 13,
    VTRECV_STATE_SOS_PM_APC_STRING = 14,
} vtrecv_state_t;

typedef enum {
    VTRECV_ACTION_NONE = 0,
    VTRECV_ACTION_CLEAR = 1,
    VTRECV_ACTION_COLLECT = 2,
    VTRECV_ACTION_CSI_DISPATCH = 3,
    VTRECV_ACTION_ESC_DISPATCH = 4,
    VTRECV_ACTION_EXECUTE = 5,
    VTRECV_ACTION_HOOK = 6,
    VTRECV_ACTION_IGNORE = 7,
    VTRECV_ACTION_OSC_END = 8,
    VTRECV_ACTION_OSC_PUT = 9,
    VTRECV_ACTION_OSC_START = 10,
    VTRECV_ACTION_PARAM = 11,
    VTRECV_ACTION_PRINT = 12,
    VTRECV_ACTION_PUT = 13,
    VTRECV_ACTION_UNHOOK = 14,
    VTRECV_ACTION_ERROR = 15,
} vtrecv_action_t;

typedef unsigned char state_change_t;
struct vtrecv;
typedef void (*vtrecv_callback_t)(struct vtrecv*, vtrecv_action_t, unsigned char);
typedef struct vtrecv {
    vtrecv_state_t    state;
    vtrecv_callback_t cb;
    unsigned char      intermediate_chars[MAX_INTERMEDIATE_CHARS+1];
    int                num_intermediate_chars;
    char               ignore_flagged;
    int                params[16];
    int                num_params;
    void*              user_data;
} vtrecv_t;

#ifdef __cplusplus
extern "C" {
#endif

void vtrecv_init(vtrecv_t *parser, vtrecv_callback_t cb);
void vtrecv_execute(vtrecv_t *parser, unsigned char *data, int len);

#ifdef __cplusplus
}
#endif

#endif

